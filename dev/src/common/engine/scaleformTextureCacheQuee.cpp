/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#include "scaleformTextureCacheQuee.h"
#include "../engine/renderSettings.h"

#include "../renderer/renderTextureStreamingCooked.h"
#include "../renderer/renderTextureBase.h"
#include "../renderer/renderInterface.h"
#include "../core/ioTags.h"
#include "../engine/texture.h"
#include "../engine/textureCache.h"

//////////////////////////////////////////////////////////////////////////
#ifdef USE_SCALEFORM
//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
//	CScaleforTextureChacheJob

CScaleformTextureCacheJob::CScaleformTextureCacheJob( CScaleformTextureCacheQueue* parent, CScaleformTextureCacheImage* image )
	: m_image( image )
	, m_parent( parent )
	, m_status( ESJS_Uninitialized )
	, m_loadTask( nullptr )
	, m_data( nullptr )
{

	if( m_image )
	{
		m_desc.type			= GpuApi::TEXTYPE_2D;
		m_desc.width		= m_image->GetSize().Width;
		m_desc.height		= m_image->GetSize().Height;
		m_desc.initLevels	= (Uint16)m_image->GetMipmapCount();
		m_desc.sliceNum		= 1;
		m_desc.usage		= GpuApi::TEXUSAGE_Samplable | GpuApi::TEXUSAGE_Immutable;
		m_desc.format		= m_image->GetGpuTextureFormat();
		m_desc.inPlaceType	= GpuApi::INPLACE_Texture;

		m_cacheQuery		= image->GetCacheQuery();
	}

}

CScaleformTextureCacheJob::~CScaleformTextureCacheJob()
{
	RED_FATAL_ASSERT( m_status != ESJS_Discarded, "Trying to free task that was not discarded" );

	// WE SHOULD NEVER RELEASE THIS OBJECT WHILE THERE IS A LOADING TASK PENDING
	// this is because the loading task requires the m_loadMemory to be there, we cannot release it without cleaning up this task first
	if ( m_loadTask )
	{
		if ( m_data.IsValid() )
		{
			Red::MemoryFramework::MemoryRegionHandle memory = m_data;
			m_data = nullptr;

			m_loadTask->SetCleanupFunction( [memory]()
			{
				GpuApi::DecrementInFlightTextureMemory( (Uint32)memory.GetSize() );
				GpuApi::UnlockInPlaceMemoryRegion( GpuApi::INPLACE_Texture, memory );	
				GpuApi::ReleaseInPlaceMemoryRegion( GpuApi::INPLACE_Texture, memory );	
			}
			);
		}

		// release the task
		m_loadTask->Release();
		m_loadTask = nullptr;
	}

	ReleaseInPlaceMemory();
}

void	CScaleformTextureCacheJob::ReleaseInPlaceMemory()
{
	// Release the inplace memory if not used
	if ( m_data.IsValid() )
	{
		// Release in flight memory
		GpuApi::DecrementInFlightTextureMemory( GetDataSize() );
		GpuApi::UnlockInPlaceMemoryRegion( GpuApi::INPLACE_Texture, m_data );
		GpuApi::ReleaseInPlaceMemoryRegion( GpuApi::INPLACE_Texture, m_data );
	}

	m_data = nullptr;
}



// Just tests overall budget
static Bool IsStreamingTextureInBudget( Uint32 textureSizeBytes )
{
	// This is only an estimation
	const GpuApi::TextureStats* stats = GpuApi::GetTextureStats();

	// Take both fully streamed and in-progress streaming into consideration, so we don't over-shoot the budget
	const Uint32 usedMemory = stats->m_streamableTextureMemory + stats->m_streamableTextureMemoryInFlight;
	const Uint32 memoryLimit = Config::cvTextureMemoryBudget.Get() * 1024 * 1024;

#ifndef RED_FINAL_BUILD
	if( usedMemory + textureSizeBytes > memoryLimit )
	{
		RED_LOG( RED_LOG_CHANNEL(UIStreaming) , TXT("textureSizeBytes: %d, usedMemory: %d, memoryLimit: %d, over budget:%d ") , textureSizeBytes, usedMemory, memoryLimit, (usedMemory + textureSizeBytes) - memoryLimit );
	}
#endif
	return ( usedMemory + textureSizeBytes <= memoryLimit );
}

// Tests in-flight memory budget
static Bool CanStreamTextureThisFrame( Uint32 textureSizeBytes )
{
	// This is only an estimation
	const Uint32 memoryInFlight = GpuApi::GetTextureStats()->m_streamableTextureMemoryInFlight;
	const Uint32 memoryInFlightLimit = Config::cvTextureInFlightBudget.Get() * 1024 * 1024;
#ifndef RED_FINAL_BUILD
	if( memoryInFlight + textureSizeBytes > memoryInFlightLimit )
	{
		RED_LOG( RED_LOG_CHANNEL(UIStreaming) , TXT("textureSizeBytes: %d, memoryInFlight: %d,      memoryInFlightLimit: %d, over budget:%d ") , textureSizeBytes, memoryInFlight, memoryInFlightLimit, (memoryInFlight + textureSizeBytes) - memoryInFlightLimit );
	}
#endif
	return ( memoryInFlight + textureSizeBytes <= memoryInFlightLimit );
}


EScaleformCacheJobStatus	CScaleformTextureCacheJob::Start()
{
	if ( m_parent->IsSuspended() )
	{
		// Scaleform texture streaming was asked to lay down (probably due to memory defragmentation going on)
		return ESJS_Uninitialized;
	}

	RED_FATAL_ASSERT( m_status == ESJS_Uninitialized , "Texture loading job is arleady pending or ready" );

	Uint32 textureDataSize = GpuApi::CalcTextureSize( m_desc );
	Uint32 uncompressedSize = m_cacheQuery.GetEntry().m_info.m_uncompressedSize; 
	RED_ASSERT( textureDataSize+2048 >= uncompressedSize , TXT("Seems that data size from TexDesc is somehow different than uncompressed size in file ( desc: %u , uncp: %u )") , textureDataSize , uncompressedSize );

	Uint32 textureMemSize = ::Max( textureDataSize , uncompressedSize );

	// We may start streaming new texture if we have memory for it
	if ( !IsStreamingTextureInBudget( textureMemSize ) )
	{
		// We don't want to discard textures. Just log it, so you'd notice there is too low memory
		// return ESJS_Uninitialized;
	}

	// We may not have enough in-flight memory this frame. Can happen on long frames with massive new texture streaming footprints
	if ( !CanStreamTextureThisFrame( textureMemSize ) )
	{
		return ESJS_Uninitialized;
	}

	if( !m_data.IsValid() )
	{
		m_data = GpuApi::AllocateInPlaceMemoryRegion( GpuApi::INPLACE_Texture, textureMemSize, GpuApi::MC_InPlaceScaleform, GetBaseAlignment(), Red::MemoryFramework::Region_Shortlived );
		if ( !m_data.IsValid() )
		{
			RED_LOG_WARNING( RED_CHANNEL( ScaleformStreaming ) , TXT( "TextureStreaming: Unable allocate buffer for streaming '%ls'" ), m_cacheQuery.GetPath().AsChar() );
			return ESJS_Error;
		}

#ifdef ENABLE_MEMORY_REGION_DEBUG_STRINGS
		m_data.SetDebugString( m_cacheQuery.GetPath().AsChar() );
#endif

		// Allocate in-flight memory
		GpuApi::IncrementInFlightTextureMemory( GetDataSize() );
	}

	const auto ret = m_cacheQuery.LoadDataAsync( 0, m_data.GetRawPtr() , GetDataSize() , 0, m_loadTask );

	if ( ret == CTextureCacheQuery::eResult_Failed )
	{
		// we've failed
		return ( m_status = ESJS_Error );
	}
	else if ( ret == CTextureCacheQuery::eResult_NotReady )
	{
		// we are not ready yet to create the decompression task
		return ESJS_Uninitialized;
	}
			
	return ( m_status = ESJS_Processing );

}


Bool	CScaleformTextureCacheJob::Check()
{
	RED_FATAL_ASSERT( m_loadTask , "No loading task" );
	RED_FATAL_ASSERT( m_status != ESJS_Ready , "Task is aready ready" );

	// Has the task ended ?
	void* outLoadedData = nullptr;
	const auto ret = m_loadTask->GetData( outLoadedData );
	if ( ret == IFileDecompressionTask::eResult_Failed )
	{
		// cleanup the task
		// NOTE: it's ok to free the memory right away because there'll be no loading into it
		m_loadTask->Release();
		m_loadTask = nullptr;

		// we've failed
		m_status = ESJS_Error;
		return true; // finished
	}
	else if ( ret == IFileDecompressionTask::eResult_NotReady )
	{
		// still pending
		return false;
	}

	// We have finished loading and decompressing the texture, release the loading task
	m_loadTask->Release();
	m_loadTask = nullptr;

	m_status = ESJS_Ready;

	return true;
}


void	CScaleformTextureCacheJob::Cancel()
{
	if( m_parent )
	{
		m_parent->FinishJob( this );
	}
}


GpuApi::TextureRef CScaleformTextureCacheJob::CreateTexture()
{ 

	if( m_data.IsValid() )
	{

		if( m_status == ESJS_Ready )
		{
			GpuApi::TextureInitData initData;
			initData.m_isCooked = true;
			initData.m_cookedData = m_data;

			// Remembder buffer size here. After CreateTexture buffer might be invalidated, and data size will be lost
			Uint32 bufferSize = GetDataSize();

#ifndef RED_FINAL_BUILD
			m_parent->IncreaseMemoryCount( GpuApi::CalcTextureSize(m_desc) );
			m_parent->LogCurrentMemory();
#endif

			GpuApi::FlushCpuCache( m_data.GetRawPtr(), (Uint32)m_data.GetSize() );

			GpuApi::TextureRef texture = GpuApi::CreateTexture( m_desc, GpuApi::TEXG_UI, &initData );
			GpuApi::UnlockInPlaceMemoryRegion( GpuApi::INPLACE_Texture, m_data );

			if( texture.isNull() )
			{
				ReleaseInPlaceMemory();
			}
			// After creating texture, inplace buffer doesn't exists anymore, so decrement its size
			else
			{
				GpuApi::DecrementInFlightTextureMemory( bufferSize );
			}

			m_data = nullptr;

			m_status = ESJS_Finished;

			return texture;
		}

		ReleaseInPlaceMemory();

	}

	return GpuApi::TextureRef::Null();
}

Uint32 CScaleformTextureCacheJob::GetDataSize() const
{
	//if ( !m_cacheQuery )
	//{
	//	return false;
	//}

	// return m_cacheQuery.GetEntry().m_info.m_uncompressedSize;
	return static_cast<Uint32>( m_data.GetSize() );
}

Uint32 CScaleformTextureCacheJob::GetBaseAlignment() const
{
	if( !m_cacheQuery )
	{
		return (Uint32)-1;
	}
	return m_cacheQuery.GetEntry().m_info.m_baseAlignment;
}

//////////////////////////////////////////////////////////////////////////
//	CScaleformTextureCacheQuee

CScaleformTextureCacheQueue::CScaleformTextureCacheQueue()
	: m_undefPlaceholder( NULL )
	, m_pendingPlaceholder( NULL )
#ifndef RED_FINAL_BUILD
	, m_currentMemory( 0 )
	, m_peakMemory( 0 )
#endif
	, m_frameToResumeRendering( 0 )
{              
}

CScaleformTextureCacheQueue::~CScaleformTextureCacheQueue()
{
	for( auto& i : m_quee )
	{
		delete i;
	}
	m_quee.ClearFast();

	GpuApi::SafeRelease( m_undefPlaceholder );
	GpuApi::SafeRelease( m_pendingPlaceholder );
}

Bool	CScaleformTextureCacheQueue::CreatePlaceholderTexture()
{
	// There is some crazy shit going on on ORBIS:
	// When setuping up Scaleofrm managers there is that line that checks render targets that are
	// currently bound. And afer these clears render targets are somehow fucked up. 
	// We need to restore current state.

	GpuApi::TextureDesc desc;
	desc.type			= GpuApi::TEXTYPE_2D;
	desc.width			= 16;
	desc.height			= 16;
	desc.initLevels		= 1;
	desc.msaaLevel		= 0;
	desc.usage			= GpuApi::TEXUSAGE_Samplable | GpuApi::TEXUSAGE_RenderTarget;
	desc.format			= GpuApi::TEXFMT_R8G8B8A8;

	// Black RGB and no alpha. Some GFX blend modes might use additive blending or alpha blending.
	const GpuApi::Float clearColor[4] = {0,0,0,0};

	m_undefPlaceholder = GpuApi::CreateTexture( desc, GpuApi::TEXG_UI );
	GpuApi::SetTextureDebugPath( m_undefPlaceholder, "scaleformUndefinedPlaceholder" );
	if( m_undefPlaceholder )
	{
		GetRenderer()->ClearColorTarget( m_undefPlaceholder, clearColor );
	}

	m_pendingPlaceholder = GpuApi::CreateTexture( desc, GpuApi::TEXG_UI );
	GpuApi::SetTextureDebugPath( m_pendingPlaceholder, "scaleformPendingPlaceholder" );
	if( m_pendingPlaceholder )
	{
		GetRenderer()->ClearColorTarget( m_pendingPlaceholder, clearColor );
	}

#ifndef RED_FINAL_BUILD
	IncreaseMemoryCount( GpuApi::CalcTextureSize( desc ) * 2 );
#endif

	RED_ASSERT( m_undefPlaceholder && m_pendingPlaceholder );
	return false;

}

void CScaleformTextureCacheQueue::FinishJob( CScaleformTextureCacheJob* job )
{
	// If there is no job or job isn't owned by us
	if( !job || job->GetQueue() != this )
	{
		// Jes, go fuck yourselfe. I dotn want do deal with not my shit
		return;
	}

	m_quee.RemoveFast(job);                               

	// deleta job listener
	job->Release();

}


CScaleformTextureCacheJob* CScaleformTextureCacheQueue::GetJobForTexture( CScaleformTextureCacheImage* image )
{
	// MUTEX

	// Find if we arealdy dont have task for that texture
	CScaleformTextureCacheJob* job = FindJob( image );

	if( job )
	{
		return job;
	}

	job = new CScaleformTextureCacheJob( this , image );

	m_quee.PushBack( job );
		
	return job;
}


CScaleformTextureCacheJob* CScaleformTextureCacheQueue::FindJob( CScaleformTextureCacheImage* image )
{
	for( auto& job : m_quee )
	{
		if( job->GetImage() == image )
		{
			return job;
		}
	}

	return nullptr;
}

#ifndef RED_FINAL_BUILD

void CScaleformTextureCacheQueue::LogCurrentMemory()
{
	// If its above 12MB, alert it
	if( m_currentMemory >= 1024*1024 * 12 )
	{
		RED_LOG( RED_LOG_CHANNEL(UIStreaming) , TXT( "== Scaleform memory info ==\n * Current memory used : %u\n * Memory peak : %u\n============================"	) , m_currentMemory , m_peakMemory );
	}
}

#endif

//////////////////////////////////////////////////////////////////////////
#endif // USE_SCALEFORM
//////////////////////////////////////////////////////////////////////////
