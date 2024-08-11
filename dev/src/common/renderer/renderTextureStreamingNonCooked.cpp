/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "renderTextureStreamingNonCooked.h"
#include "renderTextureBase.h"
#include "../core/fileLatentLoadingToken.h"
#include "../engine/texture.h"
#include "../engine/textureCache.h"


// NOTE : Streaming from a non-cooked source does not copy from an existing texture, it loads the full texture data from disk.
// This is mainly because non-cooked textures don't use in-place, and async texture load/copy aren't available for PS4,
// so we can't build a command list to copy over the existing mips.


//////////////////////////////////////////////////////////////////////////////////////////////

CRenderTextureStreamingTaskNotCooked::CRenderTextureStreamingTaskNotCooked( const GpuApi::TextureDesc& textureDesc, Int8 streamingMipIndex, IBitmapTextureStreamingSource* streamingSource )
	: IRenderTextureStreamingTask( textureDesc, streamingMipIndex )
	, ILoadJob( JP_Texture, false /*gc blocker*/ )
	, m_streamingSource( streamingSource )
	, m_textureCount( streamingSource->GetNumTextures() )
	, m_mipCount( streamingSource->GetNumMipmaps() )
	, m_discarded( false )
{
	RED_FATAL_ASSERT( m_streamingSource != nullptr, "Cannot stream without a streaming source!" );

	// Keep reference to streaming source
	m_streamingSource->AddRef();
	m_streamingMipIndex = Min( m_streamingMipIndex, (Int8)m_mipCount );

	// Increment in-flight memory stats asap
	m_memoryUsedInFlight = CalculateTotalMemoryForLoad();
	GpuApi::IncrementInFlightTextureMemory( m_memoryUsedInFlight );

	// Issue ourselves as the loading job
	SJobManager::GetInstance().Issue( this );
}

CRenderTextureStreamingTaskNotCooked::~CRenderTextureStreamingTaskNotCooked()
{
	// Release the reference to original texture
	GpuApi::SafeRelease( m_texture );

	// Release the reference to streaming source
	SAFE_RELEASE( m_streamingSource );

	// Free any texture buffers we still have around. If we were canceled, then we may not have freed
	// them earlier.
	for ( Uint32 i = 0; i < m_streamedData.Size(); ++i )
	{
		if ( m_streamedData[i] )
		{
			GpuApi::FreeTextureData( m_streamedData[i] );
			m_streamedData[i] = nullptr;
		}
	}

	// Decrement the in-flight memory as it is guaranteed to be free now
	GpuApi::DecrementInFlightTextureMemory( m_memoryUsedInFlight );
}

void CRenderTextureStreamingTaskNotCooked::Discard()
{
	RED_FATAL_ASSERT( !m_discarded, "Discard called twice on the same job" );

	// Cancel if we can.
	Cancel();

	// release our internal reference
	if ( !m_discarded )
	{
		m_discarded = true;
		Release();
	}
}

Uint32 CRenderTextureStreamingTaskNotCooked::CalculateTotalMemoryForLoad() const
{
	const Uint32 mipsToLoad = GetActualMipsToLoad();

	Uint32 totalMemory = 0;	
	for ( Uint32 tex_i = 0; tex_i < m_textureCount; ++tex_i )
	{
		for ( Uint32 mip_i = 0; mip_i < mipsToLoad; ++mip_i )
		{
			// Calculate real index of the mip to load and get info about that mipmap placement and size
			const Uint32 mipIndex = m_streamingMipIndex + mip_i;
			const SBitmapTextureStreamingMipInfo* mipInfo = m_streamingSource->GetMipLoadingInfo( mipIndex, tex_i );
			if ( mipInfo->m_isCooked )
			{
				Uint32 mipSize = 0;
				if ( GpuApi::CalculateCookedTextureMipOffsetAndSize( m_textureDesc, mip_i, tex_i, nullptr, &mipSize ) )
				{
					totalMemory += mipSize;
				}
			}
			else
			{
				totalMemory += mipInfo->m_size;
			}
		}
	}

	return totalMemory;
}

Uint32 CRenderTextureStreamingTaskNotCooked::GetActualMipsToLoad() const
{
	// Knowing the total texture mip count calculate how many mipmaps to load
	return m_mipCount - m_streamingMipIndex;
}

EJobResult CRenderTextureStreamingTaskNotCooked::Process()
{
	RED_FATAL_ASSERT( m_streamingSource != nullptr, "No streaming source in job" );
	RED_FATAL_ASSERT( GetActualMipsToLoad() > 0, "No mipmaps to stream, why was the job created ?" );

	// Cooked streaming source is always from texture cache.
	if ( !StreamFromStreamingSource() )
		return JR_Failed;

	// Done
	return JR_Finished;
}

Bool CRenderTextureStreamingTaskNotCooked::StreamFromStreamingSource()
{
	// Knowing the total texture mip count calculate how many mipmaps to load
	const Uint32 mipsToLoad = GetActualMipsToLoad();

	RED_WARNING( m_streamingSource->GetNumTextures() == m_textureDesc.sliceNum, "Streaming source provides incorrect number of textures: %u != %u", m_streamingSource->GetNumTextures(), m_textureDesc.sliceNum );
	if ( m_streamingSource->GetNumTextures() != m_textureDesc.sliceNum )
	{
		return false;
	}

	// THIS DESCRIPTOR IS ONLY FOR THE COOKED DATA READING, DO NOT USE IT FOR THE TEXTURE CREATION!
	// TextureDesc for the full streaming source. We might be streaming just a section of it, but to get proper offsets we need to
	// treat it as a whole.
	GpuApi::TextureDesc fullTexDesc = m_textureDesc;
	fullTexDesc.width = m_streamingSource->GetBaseWidth();
	fullTexDesc.height = m_streamingSource->GetBaseHeight();
	fullTexDesc.initLevels = m_streamingSource->GetNumMipmaps();

	// Load all required data.
	m_streamedData.Resize( m_textureCount * mipsToLoad );
	for ( Uint32 tex_i = 0; tex_i < m_textureCount; ++tex_i )
	{
		// Resume file loading, it resumes at mip 0
		IFileLatentLoadingToken* loadingToken = m_streamingSource->CreateLoadingToken( tex_i );
		if ( !loadingToken )
		{
			WARN_RENDERER( TXT("TextureStreaming: no source loading token. Failed.") );
			return false;
		}

		// Resume file reading
		IFile* sourceFile = loadingToken->Resume( 0 );
		if ( !sourceFile )
		{
			WARN_RENDERER( TXT("TextureStreaming: unable to resume loading from '%ls'."), loadingToken->GetDebugInfo().AsChar() );
			delete loadingToken;
			return false;
		}

		delete loadingToken;

		// Load mipmaps
		for ( Uint32 mip_i = 0; mip_i < mipsToLoad; ++mip_i )
		{
			// Early exit if canceled on the way..
			if ( IsCanceled() )
			{
				delete sourceFile;
				return true;
			}

			const Uint32 dataIndex = mip_i + tex_i * mipsToLoad;

			// Calculate real index of the mip to load and get info about that mipmap placement and size
			const Uint32 mipIndex = m_streamingMipIndex + mip_i;
			const SBitmapTextureStreamingMipInfo* mipInfo = m_streamingSource->GetMipLoadingInfo( mipIndex, tex_i );


			Uint32 mipOffset, mipSize;
			if ( mipInfo->m_isCooked )
			{
				// TODO : Worry about this? Need to decompress from texture cache and extract the mip...
				if ( !GpuApi::CalculateCookedTextureMipOffsetAndSize( fullTexDesc, mipIndex, tex_i, &mipOffset, &mipSize ) )
				{
					delete sourceFile;
					WARN_RENDERER( TXT( "TextureStreaming: Unable to get mip offset/size for mip %u, slice %u" ), mipIndex, tex_i );
					return false;
				}
			}
			else
			{
				mipOffset = mipInfo->m_offset;
				mipSize = mipInfo->m_size;
			}


			// Allocate temporary memory for the mip data.
			m_streamedData[ dataIndex ] = GpuApi::AllocateTextureData( mipSize, 16);

			if ( m_streamedData[ dataIndex ] == nullptr )
			{
				delete sourceFile;
				WARN_RENDERER( TXT( "TextureStreaming: Unable to lock mip %i for streaming" ), mipIndex );
				return false;
			}

			// Load data
			sourceFile->Seek( mipOffset );
			sourceFile->Serialize( m_streamedData[ dataIndex ], mipSize );

			GetRenderer()->GetTextureStreamingManager()->AddTotalDataLoaded( mipSize );
		}

		// Close streaming file
		delete sourceFile;
	}

	// Fill in TextureLevelInitData with the stuff we just finished loading.
	TDynArray< GpuApi::TextureLevelInitData > initMipData( m_textureCount * mipsToLoad );
	for ( Uint32 tex_i = 0; tex_i < m_textureCount; ++tex_i )
	{
		for ( Uint32 mip_i = 0; mip_i < mipsToLoad; ++mip_i )
		{
			const Uint32 dataIndex = mip_i + tex_i * mipsToLoad;

			if ( m_streamedData[ dataIndex ] != nullptr )
			{
				const Uint32 actualMipIndex = m_streamingMipIndex + mip_i;
				const SBitmapTextureStreamingMipInfo* mipInfo = m_streamingSource->GetMipLoadingInfo( actualMipIndex, tex_i );

				initMipData[ dataIndex ].m_data = m_streamedData[ dataIndex ];
				initMipData[ dataIndex ].m_isCooked = mipInfo->m_isCooked;
			}
		}
	}

	GpuApi::TextureInitData initData;
	initData.m_isCooked = false;
	initData.m_mipsInitData = initMipData.TypedData();

	// Create high res texture
	m_texture = GpuApi::CreateTexture( m_textureDesc, GpuApi::TEXG_Streamable, &initData );
	if ( !m_texture )
	{
		const Uint32 textureMemory = GpuApi::GetTextureStats()->m_streamableTextureMemory;
		WARN_RENDERER( TXT("Failed to create streaming texture %ix%i, %1.2f MB of textures already loaded"), m_textureDesc.width, m_textureDesc.height, (Float)textureMemory / ( 1024.0f * 1024.0f ) );
		return false;
	}

	SetTextureInfo( m_texture );
	return true;
}


void CRenderTextureStreamingTaskNotCooked::Tick( Bool& /*allowNewStreaming*/ )
{
	// Nothing to do here. Everything's done in Process
}

Bool CRenderTextureStreamingTaskNotCooked::TryFinish( STextureStreamResults& outResults )
{
	// Streaming non-cooked, we just ignore the allowNewStreaming here. We started the load task when we were created, so we don't
	// really have anything to do with it.

	// still loading
	if ( !ILoadJob::HasEnded() )
		return false;

	// failed
	if ( !ILoadJob::HasFinishedWithoutErrors() )
	{
		return true;
	}

	// loaded
	outResults.m_hiresTexture	= m_texture;
	outResults.m_streamedMip	= m_streamingMipIndex;

	m_texture = GpuApi::TextureRef::Null();

	return true;
}

CRenderTextureStreamingTaskNotCooked* CRenderTextureStreamingTaskNotCooked::Create( const GpuApi::TextureDesc& textureDesc, Int8 streamingMipIndex, IBitmapTextureStreamingSource* streamingSource )
{
	return new CRenderTextureStreamingTaskNotCooked( textureDesc, streamingMipIndex, streamingSource );
}

//////////////////////////////////////////////////////////////////////////////////////////////
