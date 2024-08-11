/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "renderTextureStreamingCooked.h"
#include "renderTextureBase.h"
#include "../core/ioTags.h"
#include "../core/task.h"
#include "../engine/texture.h"
#include "../engine/textureCache.h"


//////////////////////////////////////////////////////////////////////////
// POSSIBLE TODO

/* Maybe don't create new task if we can't start the decompression yet. So
then if a task exists, it is definitely in progress. Except this would mean
reallocating the in-place buffer each time we try... would need to be able
to "reserve" a decompression request before actually setting it up. */

/* Split into separate PC / Console subclasses? Most of it's the same, but
there are a couple localized differences. */

//////////////////////////////////////////////////////////////////////////


#ifndef RED_PLATFORM_CONSOLE

// On PC, finalize by copy data from the existing texture, and the stuff that was loaded from disk, if any.
class CRenderTextureStreamingFinalizerCookedPC : public IRenderTextureStreamingFinalizer
{
private:
	GpuApi::TextureRef							m_sourceTexture;
	GpuApi::TextureRef							m_targetTexture;

	Red::MemoryFramework::MemoryRegionHandle	m_loadedData;			// Memory containing the data loaded from disk.
	Uint8										m_numMipsLoaded;		// Number of mips that were loaded from disk.


public:
	CRenderTextureStreamingFinalizerCookedPC( const GpuApi::TextureRef& source, const GpuApi::TextureRef& target, Red::MemoryFramework::MemoryRegionHandle loadedData, Uint8 numMipsLoaded )
		: m_sourceTexture( source )
		, m_targetTexture( target )
		, m_loadedData( loadedData )
		, m_numMipsLoaded( numMipsLoaded )
	{
		RED_FATAL_ASSERT( !m_sourceTexture.isNull(), "Source texture must not be null for finalizer" );
		RED_FATAL_ASSERT( !m_targetTexture.isNull(), "Target texture must not be null for finalizer" );
		RED_FATAL_ASSERT( ( m_numMipsLoaded > 0 ) == m_loadedData.IsValid(), "loadedData must be provided if and only if numMipsLoaded > 0" );

		GpuApi::AddRef( m_sourceTexture );
		GpuApi::AddRef( m_targetTexture );
	}

	~CRenderTextureStreamingFinalizerCookedPC()
	{
		GpuApi::Release( m_sourceTexture );
		GpuApi::Release( m_targetTexture );

		if ( m_loadedData.IsValid() )
		{
			GpuApi::UnlockInPlaceMemoryRegion( GpuApi::INPLACE_Texture, m_loadedData );
			GpuApi::ReleaseInPlaceMemoryRegion( GpuApi::INPLACE_Texture, m_loadedData );
		}
	}

	virtual void Finalize() override
	{
		const GpuApi::TextureDesc& targetTextureDesc = GpuApi::GetTextureDesc( m_targetTexture );
		const GpuApi::TextureDesc& sourceTextureDesc = GpuApi::GetTextureDesc( m_sourceTexture );


		// Load new data into texture
		if ( m_numMipsLoaded > 0 )
		{
			const Uint8* data = static_cast< const Uint8* >( m_loadedData.GetRawPtr() );

			for ( Uint32 mip_i = 0; mip_i < m_numMipsLoaded; ++mip_i )
			{
				const Uint32 mipWidth = GpuApi::CalculateTextureMipDimension( targetTextureDesc.width, mip_i, targetTextureDesc.format );
				const Uint32 pitch = GpuApi::CalculateTexturePitch( mipWidth, targetTextureDesc.format );

				for ( Uint32 slice_i = 0; slice_i < targetTextureDesc.sliceNum; ++slice_i )
				{
					Uint32 offset;
					GpuApi::CalculateCookedTextureMipOffsetAndSize( targetTextureDesc, mip_i, slice_i, &offset, nullptr );

					GetRenderer()->LoadTextureData2D( m_targetTexture, mip_i, slice_i, nullptr, data + offset, pitch );
				}
			}

			GpuApi::UnlockInPlaceMemoryRegion( GpuApi::INPLACE_Texture, m_loadedData );
			GpuApi::ReleaseInPlaceMemoryRegion( GpuApi::INPLACE_Texture, m_loadedData );
			m_loadedData = nullptr;
		}


		// Copy the remaining from existing texture
		const Uint8 numExistingMips = (Uint8)sourceTextureDesc.initLevels;
		const Uint8 firstMipToCopyFromExisting = ( numExistingMips < targetTextureDesc.initLevels ? 0 : numExistingMips - targetTextureDesc.initLevels );

		for ( Uint32 mip_i = m_numMipsLoaded; mip_i < targetTextureDesc.initLevels; ++mip_i )
		{
			const Uint32 mipWidth = GpuApi::CalculateTextureMipDimension( targetTextureDesc.width, mip_i, targetTextureDesc.format );
			const Uint32 pitch = GpuApi::CalculateTexturePitch( mipWidth, targetTextureDesc.format );
			for ( Uint32 slice_i = 0; slice_i < targetTextureDesc.sliceNum; ++slice_i )
			{
				GpuApi::CopyTextureData( m_targetTexture, mip_i, slice_i, m_sourceTexture, mip_i - m_numMipsLoaded + firstMipToCopyFromExisting, slice_i );
			}
		}
	}
};

#else

// On consoles, issue a DMA copy for the existing data.
class CRenderTextureStreamingFinalizerCookedConsole : public IRenderTextureStreamingFinalizer
{
private:
	GpuApi::TextureRef							m_sourceTexture;
	GpuApi::TextureRef							m_targetTexture;

	Uint32										m_sourceOffset;
	Uint32										m_targetOffset;
	Uint32										m_sizeToCopy;

public:
	CRenderTextureStreamingFinalizerCookedConsole( const GpuApi::TextureRef& source, Uint32 sourceOffset, const GpuApi::TextureRef& target, Uint32 targetOffset, Uint32 size )
		: m_sourceTexture( source )
		, m_targetTexture( target )
		, m_sourceOffset( sourceOffset )
		, m_targetOffset( targetOffset )
		, m_sizeToCopy( size )
	{
		RED_ASSERT( !m_sourceTexture.isNull(), TXT("Must provide source texture") );
		RED_ASSERT( !m_targetTexture.isNull(), TXT("Must provider target texture") );
		GpuApi::AddRefIfValid( m_sourceTexture );
		GpuApi::AddRefIfValid( m_targetTexture );
	}

	~CRenderTextureStreamingFinalizerCookedConsole()
	{
		GpuApi::SafeRelease( m_sourceTexture );
		GpuApi::SafeRelease( m_targetTexture );
	}

	virtual void Finalize() override
	{
		PC_SCOPE_RENDER_LVL1( TEXSTREAM_CopyExistingTexture );

		if ( m_sourceTexture.isNull() || m_targetTexture.isNull() )
		{
			return;
		}

		// NOTE : We don't need to worry about locking memory regions here. This is happening on the render thread so
		// there can't be a defrag in-progress on CPU. And it goes through the GPU command processor so the copies will
		// be in sequence with any defrag copies.
		auto sourceMemory = GpuApi::GetTextureInPlaceMemory( m_sourceTexture );
		RED_ASSERT( sourceMemory.IsValid(), TXT("Source texture has invalid inplace memory handle") );

		auto targetMemory = GpuApi::GetTextureInPlaceMemory( m_targetTexture );
		RED_ASSERT( targetMemory.IsValid(), TXT("Target texture has invalid inplace memory handle") );

		if ( !sourceMemory.IsValid() || !targetMemory.IsValid() )
		{
			return;
		}

		void* srcPtr = OffsetPtr( sourceMemory.GetRawPtr(), m_sourceOffset );
		void* dstPtr = OffsetPtr( targetMemory.GetRawPtr(), m_targetOffset );
		GpuApi::DMAMemory( dstPtr, srcPtr, m_sizeToCopy, true );
	}
};

#endif



CRenderTextureStreamingTaskCooked::CRenderTextureStreamingTaskCooked( const GpuApi::TextureDesc& textureDesc, GpuApi::TextureRef existingTexture, Int8 streamingMipIndex, const Uint8 ioTag )
	: IRenderTextureStreamingTask( textureDesc, streamingMipIndex )
	, m_existingTexture( existingTexture )
	, m_inplaceMemory( nullptr )
	, m_numMipsToLoad( 0 )
	, m_inflightSize( 0 )
	, m_loadDataSize( 0 )
	, m_loadTask( nullptr )
	, m_discarded( false )
	, m_finished( false )
	, m_ioTag( ioTag )
	, m_loadTaskFinished( false )
{
	GpuApi::AddRefIfValid( m_existingTexture );
}

CRenderTextureStreamingTaskCooked::~CRenderTextureStreamingTaskCooked()
{
	RED_FATAL_ASSERT( m_discarded, "Trying to free task that was not discarded" );

	GpuApi::SafeRelease( m_existingTexture );

	Bool notifyNow = true;

	// WE SHOULD NEVER RELEASE THIS OBJECT WHILE THERE IS A LOADING TASK PENDING
	// this is because the loading task requires the m_inplaceMemory to be there, we cannot release it without cleaning up this task first
	if ( m_loadTask )
	{
		// cleanup the loading memory when the task is freed, not sooner
		if ( m_inplaceMemory.IsValid() )
		{
			notifyNow = false;

			Uint32 dataSize = m_inflightSize;
			m_inflightSize = 0;
			Red::MemoryFramework::MemoryRegionHandle memory = m_inplaceMemory;
			m_inplaceMemory = nullptr;

			m_loadTask->SetCleanupFunction( [memory, dataSize]()
				{
					// Release in flight memory
					GpuApi::DecrementInFlightTextureMemory( dataSize );

					GpuApi::UnlockInPlaceMemoryRegion( GpuApi::INPLACE_Texture, memory );
					GpuApi::ReleaseInPlaceMemoryRegion( GpuApi::INPLACE_Texture, memory );

					GetRenderer()->GetTextureStreamingManager()->OnTextureStreamingTaskDiscarded();
				}
			);
		}

		// release the task
		m_loadTask->Release();
		m_loadTask = nullptr;
	}

	// Release the inplace memory if not used
	if ( m_inplaceMemory.IsValid() )
	{
		GpuApi::UnlockInPlaceMemoryRegion( GpuApi::INPLACE_Texture, m_inplaceMemory );
		GpuApi::ReleaseInPlaceMemoryRegion( GpuApi::INPLACE_Texture, m_inplaceMemory );
		m_inplaceMemory = nullptr;
	}

	// Release in flight memory
	GpuApi::DecrementInFlightTextureMemory( m_inflightSize );

	if ( notifyNow )
	{
		GetRenderer()->GetTextureStreamingManager()->OnTextureStreamingTaskDiscarded();
	}
}

CRenderTextureStreamingTaskCooked* CRenderTextureStreamingTaskCooked::Create( const GpuApi::TextureDesc& textureDesc, GpuApi::TextureRef existingTexture, Int8 streamingMipIndex, const class CTextureCacheQuery& query, const Uint8 ioTag, Bool& inoutAllowNewStreaming )
{
	// Make sure we aren't trying to stream from a mip that we can't load.
	const Int8 lowestLoadable = query.GetLowestLoadableMip();
	if ( lowestLoadable < streamingMipIndex )
	{
		ERR_TEXSTREAM( TXT("Trying to stream a mip that can't be loaded from the texture cache! Requested: %d, Lowest allowed: %d"), streamingMipIndex, lowestLoadable );
		return nullptr;
	}

	// Stream from cooked data
	// Find start of the data we need to stream.
	Uint32 mipOffset = 0;

	// We might not be streaming the full texture, if a lower quality setting is set. In this case, we need to find where in the
	// cooked data the mips we want to stream are. So, fill a TextureDesc for the full available data, and find the offset we need.
	{
		GpuApi::TextureDesc fullTexDesc = textureDesc;
		fullTexDesc.width		= query.GetEntry().m_info.m_baseWidth;
		fullTexDesc.height		= query.GetEntry().m_info.m_baseHeight;
		fullTexDesc.initLevels	= query.GetEntry().m_info.m_mipCount;
		GpuApi::CalculateCookedTextureMipOffsetAndSize( fullTexDesc, streamingMipIndex, 0, &mipOffset, nullptr );
	}


	Uint8 numMipsToLoad = (Uint8)textureDesc.initLevels;
	// If we have an existing texture, we can copy some mips from there instead of reloading from disk.
	if ( existingTexture )
	{
		const auto& existingDesc = GpuApi::GetTextureDesc( existingTexture );

		Bool canCopyFromExisting =
#ifdef RED_PLATFORM_CONSOLE
			// On PC, our streaming textures are mostly non-inplace. But we copy via DirectX (and don't have direct access to memory
			// anyways), so it's not a problem.
			existingDesc.IsInPlace() &&
#endif
			existingDesc.format == textureDesc.format &&
			existingDesc.sliceNum == textureDesc.sliceNum;

		if ( !canCopyFromExisting )
		{
			ERR_TEXSTREAM( TXT("Can't copy from existing texture, so we'll be loading everything from disk. Texture was not in-place created, or has different format/slice count.") );
		}
		else
		{
			numMipsToLoad -= Min< Uint8 >( (Uint8)existingDesc.initLevels, numMipsToLoad );
		}
	}

	// If the mips we need to load from cache include the lowest loadable, we need to load the full texture.
	// If we don't need to load any, then it doesn't matter since we'll be copying entirely from the existing texture.
	if ( numMipsToLoad > 0 && streamingMipIndex + numMipsToLoad >= lowestLoadable )
	{
		numMipsToLoad = (Uint8)textureDesc.initLevels;
	}


	// If we are loading everything from cache, we don't need the existing texture for anything.
	if ( numMipsToLoad == textureDesc.initLevels )
	{
		existingTexture = GpuApi::TextureRef::Null();
	}


	// If we need to load something from disk, but we aren't allowed to start new streaming, then don't create the new task.
	// If we aren't loading anything, then we're actually dropping mips and we _always_ want to do this, or risk running out
	// of memory. We won't saturate I/O, so it's no problem.
	if ( numMipsToLoad > 0 && !inoutAllowNewStreaming )
	{
		return nullptr;
	}


	// Get the size of the full texture we're streaming. This includes the mips to load from disk as well as those we'll copy.
	const Uint32 textureDataSize = GpuApi::CalculateCookedTextureSize( textureDesc );
	RED_ASSERT( textureDataSize <= query.GetEntry().m_info.m_uncompressedSize - mipOffset, TXT("Cooked data size mismatch") );

	Uint32 loadMipSize = textureDataSize;
	if ( numMipsToLoad == 0 )
	{
		loadMipSize = 0;
	}
	else if ( numMipsToLoad < textureDesc.initLevels )
	{
		GpuApi::TextureDesc tempDesc = textureDesc;
		tempDesc.width		= GpuApi::CalculateTextureMipDimension( tempDesc.width, numMipsToLoad, tempDesc.format );
		tempDesc.height		= GpuApi::CalculateTextureMipDimension( tempDesc.height, numMipsToLoad, tempDesc.format );
		tempDesc.initLevels	-= numMipsToLoad;

		loadMipSize			-= GpuApi::CalculateCookedTextureSize( tempDesc );
	}

	// Get the size of the data we'll load from disk. 
	const Uint32 dataSizeToLoad = query.GetCompressedSizeForMipRange( streamingMipIndex, numMipsToLoad );


#ifdef RED_PLATFORM_CONSOLE
	RED_ASSERT( textureDataSize > 0, TXT("Texture data size is 0 when trying to stream texture. %ux%u, %u slices, %hs"), textureDesc.width, textureDesc.height, textureDesc.sliceNum, GpuApi::GetTextureFormatName( textureDesc.format ) );
	if ( textureDataSize == 0 )
	{
		return nullptr;
	}

	// On consoles we allocate for the full texture, since we'll be using in-place creation.
	const Uint32 allocSize = textureDataSize;
#else
	// For PC, allocate just enough for the mips we want to load. The remaining mips will be copied directly from the existing
	// texture, so we don't need to allocate space for that.
	const Uint32 allocSize = loadMipSize;
#endif

	Red::MemoryFramework::MemoryRegionHandle cookedData;
	if ( allocSize > 0 )
	{
		cookedData = GpuApi::AllocateInPlaceMemoryRegion( GpuApi::INPLACE_Texture, allocSize, GpuApi::MC_InPlaceTexture, query.GetEntry().m_info.m_baseAlignment, Red::MemoryFramework::Region_Shortlived );

		if ( !cookedData.IsValid() )
		{
			WARN_TEXSTREAM( TXT( "TextureStreaming: Unable to allocate buffer for streaming '%ls'" ), query.GetPath().AsChar() );
			return nullptr;
		}

#ifdef ENABLE_MEMORY_REGION_DEBUG_STRINGS
		cookedData.SetDebugString( query.GetPath().AsChar() );
#endif
	}

	GetRenderer()->GetTextureStreamingManager()->OnTextureStreamingTaskStarted();

	// Create task
	CRenderTextureStreamingTaskCooked* task = new CRenderTextureStreamingTaskCooked( textureDesc, existingTexture, streamingMipIndex, ioTag );
	task->m_inplaceMemory	= cookedData;
	task->m_inflightSize	= textureDataSize;
	task->m_mipOffset		= mipOffset;
	task->m_cacheQuery		= query;
	task->m_numMipsToLoad	= numMipsToLoad;
	task->m_loadDataSize	= dataSizeToLoad;
	task->m_loadMipDataSize	= loadMipSize;

	// Track in-flight memory
	GpuApi::IncrementInFlightTextureMemory( task->m_inflightSize );

	task->Start( inoutAllowNewStreaming );

	return task;
}

void CRenderTextureStreamingTaskCooked::Discard()
{
	RED_FATAL_ASSERT( !m_discarded, "Streaming task already discarded by owner" );

	if ( !m_discarded )
	{
		m_discarded = true;
		delete this;
	}
}


void CRenderTextureStreamingTaskCooked::Start( Bool& outAllowNewStreaming )
{
#ifdef RED_PLATFORM_CONSOLE
	RED_FATAL_ASSERT( m_inplaceMemory.IsValid(), "Starting a texture streaming task, but there's no memory allocated!" );
#endif


	if ( m_loadDataSize > 0 )
	{
		// Try to start the loading right away.
		if ( !TryStartLoadFromCache() )
		{
			outAllowNewStreaming = false;
		}
	}
	else
	{
		// No data to load from cache
		m_loadTaskFinished = true;
	}
}


Bool CRenderTextureStreamingTaskCooked::TryStartLoadFromCache()
{
	RED_FATAL_ASSERT( m_loadTask == nullptr, "Trying to start load task that's already started" );
	RED_FATAL_ASSERT( m_loadDataSize > 0, "Trying to start load task when we have nothing to load" );

	const auto ret = m_cacheQuery.LoadDataAsync( m_streamingMipIndex, m_inplaceMemory.GetRawPtr(), m_loadMipDataSize, m_ioTag, m_loadTask, m_numMipsToLoad );
	if ( ret == CTextureCacheQuery::eResult_Failed )
	{
		ERR_TEXSTREAM( TXT("Failed to start loading texture cache data for %ls"), m_cacheQuery.GetPath().AsChar() );
		return true;
	}
	else if ( ret == CTextureCacheQuery::eResult_NotReady )
	{
		// we are not ready yet to create the decompression task
		return false;
	}

	GetRenderer()->GetTextureStreamingManager()->AddTotalDataLoaded( m_loadDataSize );

	return true;
}


void CRenderTextureStreamingTaskCooked::Tick( Bool& allowNewStreaming )
{
	// If we haven't finished loading, and don't have a load task, try starting now.
	if ( !m_loadTaskFinished && m_loadTask == nullptr )
	{
		// New streaming is blocked, so we won't try starting the task yet. Just return to indicate that we aren't done.
		if ( !allowNewStreaming )
		{
			return;
		}

		if ( !TryStartLoadFromCache() )
		{
			// we are not ready yet to create the decompression task. Block further new loading, since they won't get through either
			allowNewStreaming = false;
			return;
		}

		if ( m_loadTask == nullptr )
		{
			ERR_TEXSTREAM( TXT("Failed to start loading texture cache data for %ls"), m_cacheQuery.GetPath().AsChar() );

			// we've failed
			m_finished = true;
			m_loadTaskFinished = true;
		}
	}
}


Bool CRenderTextureStreamingTaskCooked::TryFinish( STextureStreamResults& outResults )
{
	RED_FATAL_ASSERT( !m_finished, "HasFinished called twice on the same resource that completed - this indicates some invalid logic on the calling site" );
	RED_FATAL_ASSERT( !m_discarded, "Trying to update discarded task" );


	// If we haven't finished loading, and don't have a load task, we aren't done yet.
	if ( !m_loadTaskFinished && m_loadTask == nullptr )
	{
		// Haven't even started yet.
		return false;
	}


	// If we haven't finished loading, and have a load task, check if it's done.
	if ( !m_loadTaskFinished && m_loadTask != nullptr )
	{
		// Has the task ended ?
		void* outLoadedData = nullptr;
		const auto ret = m_loadTask->GetData( outLoadedData );
		if ( ret == IFileDecompressionTask::eResult_Failed )
		{
			ERR_TEXSTREAM( TXT("Failed to when loading texture cache data for %ls"), m_cacheQuery.GetPath().AsChar() );

			// cleanup the task
			// NOTE: it's ok to free the memory right away because there'll be no loading into it
			m_loadTask->Release();
			m_loadTask = nullptr;
			m_loadTaskFinished = true;

			// we've failed
			m_finished = true;
			return true; // finished
		}
		else if ( ret == IFileDecompressionTask::eResult_NotReady )
		{
			// still pending
			return false;
		}

		RED_FATAL_ASSERT( outLoadedData == m_inplaceMemory.GetRawPtr(), "Pointer returned from decompression task doesn't match our in-place buffer" );
		// We have finished loading and decompressing the texture, release the loading task
		m_loadTask->Release();
		m_loadTask = nullptr;
		m_loadTaskFinished = true;
	}


	Finalize( outResults );
	m_finished = true;
	return true;
}


void CRenderTextureStreamingTaskCooked::Finalize( STextureStreamResults& outResults )
{
#ifdef RED_PLATFORM_CONSOLE

	GpuApi::FlushCpuCache( m_inplaceMemory.GetRawPtr(), (Uint32)m_inplaceMemory.GetSize() );

	// This texture is inplace created
	m_textureDesc.inPlaceType = GpuApi::INPLACE_Texture;

	GpuApi::TextureInitData consoleInitData;
	consoleInitData.m_isCooked = true;
	consoleInitData.m_cookedData = m_inplaceMemory;

	// Create high res texture
	GpuApi::TextureRef newTexture = GpuApi::CreateTexture( m_textureDesc, GpuApi::TEXG_Streamable, &consoleInitData );
	GpuApi::UnlockInPlaceMemoryRegion( GpuApi::INPLACE_Texture, m_inplaceMemory );

	// Something did not work - cleanup the memory
	if ( !newTexture )
	{
		GpuApi::ReleaseInPlaceMemoryRegion( GpuApi::INPLACE_Texture, m_inplaceMemory );
	}
	m_inplaceMemory = nullptr;


	// Create high res texture
	outResults.m_hiresTexture	= newTexture;
	outResults.m_streamedMip	= newTexture ? m_streamingMipIndex : -1;

	// If we are copying from an existing texture, set up the finalizer to take care of that.
	if ( !newTexture.isNull() && !m_existingTexture.isNull() && m_numMipsToLoad < m_textureDesc.initLevels )
	{
		const auto& existingDesc = GpuApi::GetTextureDesc( m_existingTexture );

		const Int8 numExistingMips = (Int8)existingDesc.initLevels;
		const Int8 firstMipToCopyFromExisting = ( numExistingMips < m_textureDesc.initLevels ? 0 : numExistingMips - m_textureDesc.initLevels );

		const Int8 numToCopy = m_textureDesc.initLevels - m_numMipsToLoad;
		Uint32 offsetInExisting, offsetInNew;
		GpuApi::CalculateCookedTextureMipOffsetAndSize( existingDesc, firstMipToCopyFromExisting, 0, &offsetInExisting, nullptr );
		GpuApi::CalculateCookedTextureMipOffsetAndSize( m_textureDesc, m_numMipsToLoad, 0, &offsetInNew, nullptr );

		const Uint32 fullTexSize = GpuApi::CalculateCookedTextureSize( m_textureDesc );
		const Uint32 sizeToCopy = fullTexSize - offsetInNew;

		outResults.m_finalizer.Reset( new CRenderTextureStreamingFinalizerCookedConsole( m_existingTexture, offsetInExisting, newTexture, offsetInNew, sizeToCopy ) );
	}

#else

	GpuApi::TextureRef newTexture;

	// If we have no existing texture to copy from, we can just create the texture directly, immutable and in-place.
	if ( !m_existingTexture )
	{
		// This texture is inplace created
		m_textureDesc.inPlaceType = GpuApi::INPLACE_Texture;

		GpuApi::TextureInitData consoleInitData;
		consoleInitData.m_isCooked = true;
		consoleInitData.m_cookedData = m_inplaceMemory;
		newTexture = GpuApi::CreateTexture( m_textureDesc, GpuApi::TEXG_Streamable, &consoleInitData );

		GpuApi::UnlockInPlaceMemoryRegion( GpuApi::INPLACE_Texture, m_inplaceMemory );
		// Creation failed, so we need to release the memory ourselves
		if ( !newTexture )
		{
			GpuApi::ReleaseInPlaceMemoryRegion( GpuApi::INPLACE_Texture, m_inplaceMemory );
		}
		m_inplaceMemory = nullptr;
	}
	// We have an existing texture, so we want to copy some mips from that. We can't create an immutable texture in this case, because
	// we need to copy over data from different sources. The actual copies will happen from the Finalizer, since they need to run on
	// the render thread.
	else
	{
		// Can't be in-place immutable, since we need to copy from the existing texture.
		m_textureDesc.inPlaceType = GpuApi::INPLACE_None;
		m_textureDesc.usage &= ~GpuApi::TEXUSAGE_Immutable;
		newTexture = GpuApi::CreateTexture( m_textureDesc, GpuApi::TEXG_Streamable, nullptr );
	}

	outResults.m_hiresTexture	= newTexture;
	outResults.m_streamedMip	= newTexture ? m_streamingMipIndex : -1;

	if ( newTexture && m_existingTexture )
	{
		outResults.m_finalizer.Reset( new CRenderTextureStreamingFinalizerCookedPC( m_existingTexture, newTexture, m_inplaceMemory, m_numMipsToLoad ) );
		// Finalizer owns this memory now
		m_inplaceMemory = nullptr;
	}

#endif
}
