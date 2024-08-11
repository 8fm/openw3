/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "renderEnvProbeManager.h"
#include "../engine/envProbeComponent.h"
#include "../engine/textureCache.h"
#include "../core/jobGenericJobs.h"
#include "../core/loadingJobManager.h"
#include "../core/fileLatentLoadingToken.h"
#include "../core/ioTags.h"
#include "../core/fileDecompression.h"
#include "../gpuApiUtils/gpuApiMemory.h"

// *********************************

static GpuApi::EInPlaceResourceType GetCookedEnvProbeInPlaceType()
{
	return GpuApi::INPLACE_EnvProbe;
}

static GpuApi::TextureDesc BuildEnvProbeTextureDesc( eEnvProbeBufferTexture texType, Bool isCooked )
{
	const Uint32 resolution					= GetEnvProbeDataSourceResolution();
	const GpuApi::eTextureFormat ftt_format	= ENVPROBEBUFFERTEX_Depth == texType ? GpuApi::TEXFMT_Float_R16 : GpuApi::TEXFMT_R8G8B8A8;

	GpuApi::TextureDesc desc;
	desc.type			= GpuApi::TEXTYPE_ARRAY;
	desc.initLevels		= 1;
	desc.sliceNum		= 6;
	desc.width			= resolution;
	desc.height			= resolution;
	desc.usage			= GpuApi::TEXUSAGE_Samplable;
	desc.format			= ftt_format;
	desc.inPlaceType	= GpuApi::INPLACE_None;

	if ( isCooked )
	{
		desc.usage |= GpuApi::TEXUSAGE_Immutable;
		desc.inPlaceType = GetCookedEnvProbeInPlaceType();
	}

	return desc;
}

Bool InitFaceTextures( IEnvProbeDataSource::tFaceTexturesTable *faceTextures, const void *dataBuffer, Uint32 dataSize, TDynArray<Uint8, MC_EnvProbeRender> &tempBuffer )
{
	if ( dataSize != CRenderEnvProbeManager::GetEnvProbeDataSourceSize() )
	{
		return false;
	}

	const Uint32 resolution = GetEnvProbeDataSourceResolution();

	tempBuffer.Resize( 4 * resolution * resolution );

	for ( Uint32 ftt_i=0; ftt_i<ENVPROBEBUFFERTEX_MAX; ++ftt_i )
	{
		// Ensure texture created
		{
			GpuApi::TextureRef &texRef = (*faceTextures)[ftt_i];
			const GpuApi::TextureDesc texDesc = BuildEnvProbeTextureDesc( (eEnvProbeBufferTexture)ftt_i, false );
			if ( !texRef || texDesc != GpuApi::GetTextureDesc( texRef ) )
			{
				GpuApi::SafeRelease( texRef );
			
				texRef = GpuApi::CreateTexture( texDesc, GpuApi::TEXG_Envprobe );
				GpuApi::SetTextureDebugPath( texRef, "envProbeFace" );

				RED_ASSERT( texRef, TXT("Failed to create envprobe texture") );
			}
		}

		const Uint32 ftt_pitch = ENVPROBEBUFFERTEX_Depth == ftt_i ? 2 * resolution : 4 * resolution;

		for ( Uint32 face_i=0; face_i<6; ++face_i )
		{	
			const Uint8 *srcData = static_cast<const Uint8*>( dataBuffer ) + GetEnvProbeDataSourceBufferOffset( face_i, (eEnvProbeBufferTexture)ftt_i );

			const void *loadData = srcData;
			if ( ENVPROBEBUFFERTEX_Albedo == ftt_i )
			{
				// ace_ibl_optimize
				for ( Uint32 i=0, num=resolution*resolution; i<num; ++i )
				{
					tempBuffer[4*i]   = srcData[2*i];
					tempBuffer[4*i+1] = srcData[2*i+1];
					tempBuffer[4*i+2] = 0;
					tempBuffer[4*i+3] = 0;
				}
				loadData = tempBuffer.Data();
			}
			else if ( ENVPROBEBUFFERTEX_Normals == ftt_i )
			{
				// ace_ibl_optimize
				for ( Uint32 i=0, num=resolution*resolution; i<num; ++i )
				{
					tempBuffer[4*i]   = srcData[3*i];
					tempBuffer[4*i+1] = srcData[3*i+1];
					tempBuffer[4*i+2] = srcData[3*i+2];
					tempBuffer[4*i+3] = 0;
				}
				loadData = tempBuffer.Data();
			}

/*
#ifdef USE_DX11
			GpuApi::LoadTextureData2DAsync( (*faceTextures)[ftt_i][face_i], 0, 0, nullptr, loadData, ftt_pitch, deferredContext );
#else
			GpuApi::LoadTextureData2D( (*faceTextures)[ftt_i][face_i], 0, 0, nullptr, loadData, ftt_pitch);
#endif
*/

			GpuApi::LoadTextureData2D( (*faceTextures)[ftt_i], 0, face_i, nullptr, loadData, ftt_pitch);
		}
	}

/*
#ifdef USE_DX11
	GpuApi::SubmitContext( deferredContext );
	deferredContext = NULL;
#endif
*/

	return true;
}

class CEnvProbeDataSourceNonCooked : public IEnvProbeDataSource
{
private:
	const DeferredDataBuffer	*m_pLatentBuffer;
	BufferAsyncDataHandle		m_asyncHandle;

	tFaceTexturesTable			*m_pTargetFaceTextures;
	IEnvProbeDataSource::EState	m_state;

private:
	void UpdateState()
	{
		if ( STATE_InProgress != m_state )
		{
			return;
		}

		if ( !m_asyncHandle )
		{
			ASSERT( !"invalid" );
			m_pTargetFaceTextures = NULL;
			m_state = STATE_Failed;
			return;
		}

		BufferHandle handle;
		BufferAsyncData::EResult state = m_asyncHandle->GetData( handle );
		switch ( state )
		{
		case BufferAsyncData::eResult_Failed:
			m_pTargetFaceTextures = NULL;
			m_state = STATE_Failed;
			break;

		case BufferAsyncData::eResult_NotReady:
			break;

		case BufferAsyncData::eResult_OK:
			{
				ASSERT( handle );
				ASSERT( m_pTargetFaceTextures );
				TDynArray<Uint8, MC_EnvProbeRender> tempBuffer;
				const Bool initResult = InitFaceTextures( m_pTargetFaceTextures, handle->GetData(), handle->GetSize(), tempBuffer );

				m_asyncHandle.Reset();
				m_pTargetFaceTextures = NULL;
				m_state = initResult ? STATE_Loaded : STATE_Failed;
			}
			break;

		default:
			ASSERT( !"Invalid" );
		}		
	}

public:
	CEnvProbeDataSourceNonCooked ( const DeferredDataBuffer* latentBuffer )
		: m_pLatentBuffer ( latentBuffer )
		, m_pTargetFaceTextures ( NULL )
		, m_state ( STATE_NotLoading )
	{}

	~CEnvProbeDataSourceNonCooked ()
	{
		CancelLoading();
	}

	virtual Bool IsLoadable() override
	{
		return nullptr != m_pLatentBuffer && CRenderEnvProbeManager::GetEnvProbeDataSourceSize() == m_pLatentBuffer->GetSize();
	}

	virtual	void StartLoading( tFaceTexturesTable *faceTextures ) override
	{
		CancelLoading();		

		if ( !faceTextures || !IsLoadable() )
		{
			m_state = STATE_Failed;
			return;
		}

		m_pTargetFaceTextures = faceTextures;
		m_state = STATE_InProgress;

		m_asyncHandle = m_pLatentBuffer->AcquireBufferHandleAsync( eIOTag_EnvProbes );
	}

	virtual void RequestFastLoadingFinish() override
	{
		CancelLoading();
	}

	virtual	void CancelLoading() override
	{
		m_asyncHandle.Reset();
		m_pTargetFaceTextures = NULL;
		m_state = STATE_NotLoading;
	}

	virtual void Invalidate() override
	{
		// This gets called on destruction of the envprobe, and it can happen along with layer destruction, so because of the latentBuffer (which is related to the layer in this case)
		// we need to immediately cancel so that it the latent buffer wouldn't be dereferenced.
		
		CancelLoading();
		m_pLatentBuffer = nullptr;
		m_state = STATE_Failed;
	}

	virtual EState GetState() override
	{		
		UpdateState();
		return m_state;
	}

	virtual void Rewind() override
	{
		CancelLoading();
	}
};

// *********************************

class CEnvProbeTexturesDataCookedLoader : public Red::System::NonCopyable
{
	struct STextureData
	{
		STextureData ()
			: m_loadTask( nullptr )
			, m_loadFinished( false )
			, m_allocFailure( false )
		{
			RED_FATAL_ASSERT( !m_query, "Expected non initialized" );
			RED_FATAL_ASSERT( !m_memoryRegion.IsValid(), "Expected non initialized" );
			RED_FATAL_ASSERT( !m_textureRef, "Expected non initialized" );
		}

		CTextureCacheQuery m_query;
		Red::MemoryFramework::MemoryRegionHandle m_memoryRegion;
		GpuApi::TextureRef m_textureRef;
		IFileDecompressionTask *m_loadTask;
		Bool m_allocFailure;
		Bool m_loadFinished;
	};

private:
	const TStaticArray< Uint32, ENVPROBEBUFFERTEX_MAX > m_textureCacheHashes;
	TStaticArray< STextureData, ENVPROBEBUFFERTEX_MAX >	m_textureData;
	Bool m_isCancelRequested;
	Bool m_isInit;

public:
	CEnvProbeTexturesDataCookedLoader ( const TStaticArray< Uint32, ENVPROBEBUFFERTEX_MAX >& textureCacheHashes )
		: m_textureCacheHashes ( textureCacheHashes )		
		, m_isCancelRequested ( false )
		, m_isInit ( false )
	{
		m_isInit = InitTexturesData();
	}

	~CEnvProbeTexturesDataCookedLoader ()
	{
		for ( Uint32 i=0; i<m_textureData.Size(); ++i )
		{
			STextureData &texData = m_textureData[i];

			// Free the memory region
			{
				// Deferred (in case we're during load, set some callback so that memory would get freed right after the task is finished)
				if ( texData.m_loadTask )
				{
					RED_FATAL_ASSERT( texData.m_memoryRegion.IsValid(), "This should be valid when we have a loading task - it's the memory we're loading into" );
					if ( texData.m_memoryRegion.IsValid() )
					{
						Red::MemoryFramework::MemoryRegionHandle memory = texData.m_memoryRegion;
						texData.m_memoryRegion = nullptr; //< mark null so that we wouldn't free it immediately

						texData.m_loadTask->SetCleanupFunction( [memory]()
						{
							GpuApi::UnlockInPlaceMemoryRegion( GetCookedEnvProbeInPlaceType(), memory );
							GpuApi::ReleaseInPlaceMemoryRegion( GetCookedEnvProbeInPlaceType(), memory );
						}
						);
					}
				}

				// Non deferred
				if ( texData.m_memoryRegion.IsValid() )
				{
					GpuApi::UnlockInPlaceMemoryRegion( GetCookedEnvProbeInPlaceType(), texData.m_memoryRegion );
					GpuApi::ReleaseInPlaceMemoryRegion( GetCookedEnvProbeInPlaceType(), texData.m_memoryRegion );
					texData.m_memoryRegion = nullptr;
				}
			}

			// Release the task
			if ( texData.m_loadTask )
			{
				texData.m_loadTask->Release();
				texData.m_loadTask = nullptr;
			}

			// Release the texture
			GpuApi::SafeRelease( m_textureData[i].m_textureRef );
		}
	}

	/// Requesting cancel allows to finish a bit earlier
	void RequestCancel()
	{
		m_isCancelRequested = true;
	}

	/// Returns whether loading has finished - object shouldn't be destroyed otherwise, since it will introduce some memory leaks
	Bool IsFinished() const
	{
		return GetTextureDataResult( nullptr, nullptr );
	}

	/// Returns whether we sucessfully finished
	Bool IsSucceeded() const
	{
		Bool isSucceeded;
		return GetTextureDataResult( &isSucceeded, nullptr ) && isSucceeded;
	}

	/// Get texture data result (returns false if not finished yet)
	Bool GetTextureDataResult( Bool *outSucceeded, Bool *outAllFailuresAllocRelated ) const
	{
		Uint32 numFinished, numSucceeded, numAllocFailures;
		GetTextureDataState( numFinished, numSucceeded, numAllocFailures );

		if ( m_textureData.Size() != numFinished )
		{
			return false;
		}

		// Comparing with ENVPROBEBUFFERTEX_MAX instead of m_textureData because 
		// this is a valid number of texture data, expected by envprobe manager.
		const Bool isSucceeded = ENVPROBEBUFFERTEX_MAX == m_textureData.Size() && ENVPROBEBUFFERTEX_MAX == numSucceeded;
		const Bool isAllFailuresAllocRelated = ENVPROBEBUFFERTEX_MAX == m_textureData.Size() && numAllocFailures > 0 && ENVPROBEBUFFERTEX_MAX == numSucceeded + numAllocFailures;
		RED_FATAL_ASSERT( numSucceeded + numAllocFailures <= m_textureData.Size(), "Did some of the data have both success and allocFailure set?" );
		RED_FATAL_ASSERT( !(isSucceeded && isAllFailuresAllocRelated), "Mutially exclusive" );
		if ( outSucceeded )
			*outSucceeded = isSucceeded;
		if ( outAllFailuresAllocRelated )
			*outAllFailuresAllocRelated = isAllFailuresAllocRelated;
		return true;
	}

	/// Update loading
	void UpdateLoading()
	{
		if ( m_isInit )
		{
			UpdateTexturesDataLoading();
		}
	}

	/// Get loaded texture
	GpuApi::TextureRef GetLoadedTexture( eEnvProbeBufferTexture tex ) const
	{
		RED_FATAL_ASSERT( IsSucceeded(), "Expected succeeded load" );
		return m_textureData[tex].m_textureRef;
	}

private:
	Bool InitTexturesData()
	{
		RED_FATAL_ASSERT( m_textureData.Empty(), "Invalid state" );
		
		//
		if ( ENVPROBEBUFFERTEX_MAX != m_textureCacheHashes.Size() )
		{
			RED_FATAL( "Invalid input data" );
			return false;
		}

		// Allocate texture data
		m_textureData.Resize( ENVPROBEBUFFERTEX_MAX );

		// Init texture data
		for ( Uint32 ftt_i=0; ftt_i<m_textureData.Size(); ++ftt_i )
		{
			STextureData &texData = m_textureData[ftt_i];

			// Init query
			texData.m_query = GTextureCache->FindEntry( m_textureCacheHashes[ftt_i] );
			if ( !texData.m_query )
			{
				LOG_ENGINE( TXT("Failed to find envprobe in texture cache") );
				texData.m_loadFinished = true;
				continue;
			}

			// Get amount of memory to allocate
			const Uint32 texSize = texData.m_query.GetEntry().m_info.m_uncompressedSize;
			if ( texSize <= 0 )
			{
				RED_FATAL( "Invalid envprobe texture size" );
				texData.m_loadFinished = true;
				continue;
			}

			// Allocate memory
			RED_FATAL_ASSERT( !texData.m_memoryRegion.IsValid(), "Expected memory not allocated" );
			texData.m_memoryRegion = GpuApi::AllocateInPlaceMemoryRegion( GetCookedEnvProbeInPlaceType(), texSize, GpuApi::MC_EnvProbe, texData.m_query.GetEntry().m_info.m_baseAlignment, Red::MemoryFramework::Region_Shortlived );
		
			// Mark allocation failure if occured
			if ( !texData.m_memoryRegion.IsValid() )
			{
				RED_FATAL( "Failed to allocate memory for cooked envprobe texture. OOM?" );
				texData.m_loadFinished = true;
				texData.m_allocFailure = true;
				continue;
			}
		}

		return true;
	}

	void UpdateTexturesDataLoading()
	{
		RED_FATAL_ASSERT( ENVPROBEBUFFERTEX_MAX == m_textureData.Size(), "Expected textureData" );

		// Ensure queries are issued
		for ( Uint32 ftt_i=0; ftt_i<m_textureData.Size(); ++ftt_i )
		{
			STextureData &texData = m_textureData[ftt_i];

			if ( texData.m_loadFinished || texData.m_loadTask )
			{
				// Already issued or finished
				continue;
			}

			Bool forceFinish = false;
			if ( m_isCancelRequested )
			{
				forceFinish = true;
			}
			else
			{
				switch ( texData.m_query.LoadDataAsync( 0, texData.m_memoryRegion.GetRawPtr(), texData.m_query.GetEntry().m_info.m_uncompressedSize, eIOTag_EnvProbes, texData.m_loadTask, 1 ) )
				{
				case CTextureCacheQuery::eResult_Failed:
					RED_FATAL_ASSERT( !texData.m_loadTask, "Load tast shouldn't be changed on failure" );
					RED_FATAL( "Failed to start async load" );
					forceFinish = true;
					break;

				case CTextureCacheQuery::eResult_NotReady:
					RED_FATAL_ASSERT( !texData.m_loadTask, "Load tast shouldn't be changed on notReady" );
					break;

				case CTextureCacheQuery::eResult_OK:
					RED_FATAL_ASSERT( texData.m_loadTask, "Expected load task to be created" );
					break;

				default:
					RED_FATAL( "Unhandled result" );
				}
			}

			if ( forceFinish )
			{
				RED_FATAL_ASSERT( !texData.m_loadTask, "Loading task not expected - there would be no reason to force finish when loading task was successfully created" );

				// Free memory region
				GpuApi::UnlockInPlaceMemoryRegion( GetCookedEnvProbeInPlaceType(), texData.m_memoryRegion );
				GpuApi::ReleaseInPlaceMemoryRegion( GetCookedEnvProbeInPlaceType(), texData.m_memoryRegion );
				texData.m_memoryRegion = nullptr;
					
				// Mark as finished
				texData.m_loadFinished = true;
			}
		}

		// Handle finished loading
		for ( Uint32 ftt_i=0; ftt_i<m_textureData.Size(); ++ftt_i )
		{
			STextureData &texData = m_textureData[ftt_i];

			if ( texData.m_loadFinished )
			{
				// We're done with this buffer
				continue;
			}
#ifdef ENABLE_MEMORY_REGION_DEBUG_STRINGS
			cookedData.SetDebugString( query.GetPath().AsChar() );
#endif

			RED_FATAL_ASSERT( !texData.m_allocFailure, "Task not finished so allocFailure not expected" );
			RED_FATAL_ASSERT( !texData.m_textureRef, "Task not finished, so expecting no texture" );

			if ( !texData.m_loadTask )
			{
				// Apparently async load was busy so we'll have to wait for this one to be issued
				continue;
			}

			Bool performFullCleanup = false;
			void* outLoadedData = nullptr;
			switch ( texData.m_loadTask->GetData( outLoadedData ) )
			{
			case IFileDecompressionTask::eResult_OK:
				{
					if ( outLoadedData != texData.m_memoryRegion.GetRawPtr() )
					{
						RED_FATAL( "Expected the same output buffer as we have on input" );
						performFullCleanup = true;
						break;
					}

					// Flush cpu cache. 
					// Texture created here will be used by envProbeManager to immediately prepare some caches textures, 
					// so this might result in permanently invalid data.
					GpuApi::FlushCpuCache( texData.m_memoryRegion.GetRawPtr(), texData.m_query.GetEntry().m_info.m_uncompressedSize );

					// Handle requested cancel
					if ( m_isCancelRequested )
					{
						performFullCleanup = true;
						break;
					}

					// Create texture
					{
						GpuApi::TextureInitData initData;
						initData.m_isCooked = true;
						initData.m_cookedData = texData.m_memoryRegion;

						const GpuApi::TextureDesc texDesc = BuildEnvProbeTextureDesc( (eEnvProbeBufferTexture)ftt_i, true );
						texData.m_textureRef = GpuApi::CreateTexture( texDesc, GpuApi::TEXG_System, &initData );
						if ( !texData.m_textureRef )
						{
							RED_ASSERT( !"Failed to create envprobe texture" );
							texData.m_allocFailure = true;
							performFullCleanup = true;
							break;
						}

						GpuApi::SetTextureDebugPath( texData.m_textureRef, "envProbeFace" );
					}

					// Unlock memory region (we're not releasing it since the texture have been created)
					GpuApi::UnlockInPlaceMemoryRegion( GetCookedEnvProbeInPlaceType(), texData.m_memoryRegion );
					texData.m_memoryRegion = nullptr;

					// Destroy task
					texData.m_loadTask->Release();
					texData.m_loadTask = nullptr;

					// Mark as finished
					texData.m_loadFinished = true;
				}
				break;

			case IFileDecompressionTask::eResult_Failed:
				RED_FATAL( "Decompression failed" );
				performFullCleanup = true;
				break;

			case IFileDecompressionTask::eResult_NotReady:
				// not ready - just skip
				break;

			default:
				RED_FATAL( "Unhandled result" );
				performFullCleanup = true;
			}

			if ( performFullCleanup )
			{
				// Free memory region
				GpuApi::UnlockInPlaceMemoryRegion( GetCookedEnvProbeInPlaceType(), texData.m_memoryRegion );
				GpuApi::ReleaseInPlaceMemoryRegion( GetCookedEnvProbeInPlaceType(), texData.m_memoryRegion );
				texData.m_memoryRegion = nullptr;

				// Destroy task
				texData.m_loadTask->Release();
				texData.m_loadTask = nullptr;

				// Mark as finished
				texData.m_loadFinished = true;
			}
		}
	}

	void GetTextureDataState( Uint32 &outNumFinished, Uint32 &outNumTextures, Uint32 &outNumAllocFailures ) const
	{
		outNumFinished = 0;
		outNumTextures = 0;
		outNumAllocFailures = 0;

		for ( Uint32 ftt_i=0; ftt_i<m_textureData.Size(); ++ftt_i )
		{
			const STextureData &texData = m_textureData[ftt_i];
			if ( texData.m_loadFinished )
			{
				RED_FATAL_ASSERT( !texData.m_memoryRegion.IsValid(), "Expected no memory region allocated" );
				RED_FATAL_ASSERT( !texData.m_loadTask, "Expected no loading task" );
				RED_FATAL_ASSERT( !(texData.m_textureRef && texData.m_allocFailure), "Should be mutually exclusive" );
				outNumTextures += texData.m_textureRef ? 1 : 0;
				outNumAllocFailures += texData.m_allocFailure ? 1 : 0;
				++outNumFinished;
			}
			else
			{
				RED_FATAL_ASSERT( !texData.m_textureRef, "Expected no texture - we're not finished yet" );
			}
		}
	}
};

class CEnvProbeDataSourceCooked : public IEnvProbeDataSource
{
private:
	const TStaticArray< Uint32, ENVPROBEBUFFERTEX_MAX > m_textureCacheHashes;
	CEnvProbeTexturesDataCookedLoader *m_pLoader;
	tFaceTexturesTable *m_pFaceTextures;
	IEnvProbeDataSource::EState	m_state;	

private:
	void UpdateState()
	{
		if ( STATE_InProgress != m_state )
		{
			return;
		}

		m_pLoader->UpdateLoading();

		Bool isSucceeded = false, isAllAllocFailures = false;
		if ( m_pLoader->GetTextureDataResult( &isSucceeded, &isAllAllocFailures ) )
		{
			if ( isSucceeded )
			{
				for ( Uint32 ftt_i=0; ftt_i<ENVPROBEBUFFERTEX_MAX; ++ftt_i )
				{
					GpuApi::SafeRefCountAssign( (*m_pFaceTextures)[ftt_i], m_pLoader->GetLoadedTexture( (eEnvProbeBufferTexture)ftt_i ) );
					RED_FATAL_ASSERT( (*m_pFaceTextures)[ftt_i], "Expected loaded texture" );
				}

				m_state = STATE_Loaded;
			}
			else
			{
				m_state = isAllAllocFailures ? STATE_AllocFailed : STATE_Failed;
			}

			// Destroy loader since it won't be needed anymore
			// and we don't want it to hold any resources
			delete m_pLoader;
			m_pLoader = nullptr;
		}
	}

public:
	CEnvProbeDataSourceCooked ( const TStaticArray< Uint32, ENVPROBEBUFFERTEX_MAX >& textureCacheHashes )
		: m_textureCacheHashes( textureCacheHashes )
		, m_state ( STATE_NotLoading )
		, m_pFaceTextures ( nullptr )
		, m_pLoader ( nullptr )
	{
	}

	~CEnvProbeDataSourceCooked ()
	{
		CancelLoading();
	}

	virtual Bool IsLoadable() override
	{
		return true;
	}

	virtual	void StartLoading( tFaceTexturesTable *faceTextures ) override
	{
		CancelLoading();		

		if ( !faceTextures || !IsLoadable() )
		{
			m_state = STATE_Failed;
			return;
		}

		RED_FATAL_ASSERT( !m_pLoader, "Expected no loader" );
		m_pLoader = new CEnvProbeTexturesDataCookedLoader ( m_textureCacheHashes );
		m_pFaceTextures = faceTextures;
		m_state = STATE_InProgress;
	}

	virtual void RequestFastLoadingFinish() override
	{
		if ( m_pLoader )
		{
			m_pLoader->RequestCancel();
		}
	}

	virtual	void CancelLoading() override
	{
		if ( m_pLoader )
		{
			delete m_pLoader;
			m_pLoader = nullptr;
		}

		m_pFaceTextures = nullptr;
		m_state = STATE_NotLoading;
	}

	virtual void Invalidate() override
	{
		// empty
		// We don't need any immedaite invalidation for cooked data source, since it could cause us going overbudget (tight memory for envprobes) - instead we will just 
		// hint (in the envprobe manager) the dataSource that the load is not needed and then patiently wait for the task to end.		
	}

	virtual EState GetState() override
	{		
		UpdateState();
		return m_state;
	}

	virtual void Rewind() override
	{
		CancelLoading();
	}
};

// *********************************

IEnvProbeDataSource* CRenderInterface::CreateEnvProbeDataSource( CEnvProbeComponent &envProbeComponent )
{
	if ( envProbeComponent.IsCooked() )
	{
		return new CEnvProbeDataSourceCooked( envProbeComponent.GetDataSourceTextureCacheHashes() );
	}
	else
	{
		return new CEnvProbeDataSourceNonCooked( &envProbeComponent.GetDataSourceDataBuffer() );
	}
}

// *********************************
