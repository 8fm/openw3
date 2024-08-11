/**
* Copyright @ 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "renderInterface.h"
#include "../core/objectIterator.h"
#include "../core/fileSys.h"
#include "../core/fileDecompression.h"
#include "../engine/renderCommands.h"

// TMPHACK
#ifdef RED_PLATFORM_WINPC
# include "../../win32/platform/inputDeviceManagerWin32.h"
#endif

#include "../engine/renderScaleformCommands.h"
#include "../engine/renderSettings.h"

#include "../gpuApiUtils/gpuApiMemory.h"
#include "guiRenderSystemScaleform.h"

#include "renderTerrainShadows.h"
#include "renderShadowManager.h"
#include "renderViewport.h"
#include "gameplayFx.h"
#include "renderLodBudgetSystem.h"
#include "renderSkinningData.h"
#include "renderSwarmData.h"
#include "renderScene.h"
#include "renderFence.h"
#include "renderPostProcess.h"
#include "renderMeshBatcher.h"
#include "renderParticleBatcher.h"
#include "renderFlaresBatcher.h"
#include "renderBrushFaceBatcher.h"
#include "renderDistantLightBatcher.h"
#include "renderDynamicDecalBatcher.h"
#include "renderSwarmBatcher.h"
#include "renderViewportWindow.h"
#include "renderViewportWindowRoot.h"
#include "renderThread.h"
#include "renderSkinManager.h"
#include "renderResourceIterator.h"
#include "renderEnvProbeManager.h"
#include "renderTexture.h"
#include "renderMaterial.h"
#include "renderMesh.h"
#include "renderDefragHelper.h"
#include "../engine/mesh.h"
#include "../core/feedback.h"


#ifdef USE_APEX
#include "renderApexBatcher.h"
#include "apexRenderInterface.h"
#endif
#include "renderShaderPair.h"
#include "renderRenderSurfaces.h"
#include "renderTextureStreaming.h"

//this is for the temporary solution of releasing constant buffers before destroying the device, please remove as soon as speedtree is fixed
#include "speedTreeRenderInterface.h"

#include "..\core\diskFile.h"
#include "..\engine\environmentManager.h"
#include "..\engine\baseEngine.h"
#include "..\engine\particleEmitter.h"
#include "..\engine\renderFence.h"
#include "renderCollector.h"
#include "..\engine\renderFrame.h"
#include "..\engine\shaderCacheManager.h"
#include "..\engine\renderFragment.h"
#include "..\engine\material.h"
#include "../engine/materialGraph.h"
#include "..\engine\bitmapTexture.h"
#include "..\engine\textureArray.h"
#include "..\engine\cubeTexture.h"
#include "renderVideo.h"
#include "renderLoadingScreenFence.h"
#include "renderGameplayRenderTarget.h"
#include "renderScaleformTexture.h"
#include "../engine/hairworksHelpers.h"

extern IRender* SCreateRender( IPlatformViewport* platformInterface )
{
	return new CRenderInterface( platformInterface );
}

Uint32 GetInitialMSAALevel( Uint32 *outMSAA_X, Uint32 *outMSAA_Y )
{
	Uint32 msaa = Max<Uint32>( 1, Config::cvMsaaLevel.Get() );
	Uint32 msaaX = 1;
	Uint32 msaaY = msaa / msaaX;

	if ( nullptr != outMSAA_X )
		*outMSAA_X = msaaX;
	if ( nullptr != outMSAA_Y )
		*outMSAA_Y = msaaY;
	return msaa;
}

Float GRenderSettingsMipBias = 0.f;

CCascadeShadowResources::CCascadeShadowResources ()
{
	m_shadowmapResolution = 0;
	m_shadowmapMaxCascadeCount = 0;
	m_isRegisteredDynamicTexture = false;
}

CCascadeShadowResources::~CCascadeShadowResources ()
{
	Shut();
}

void CCascadeShadowResources::Init( Uint16 maxCascadeCount, Uint32 resolution, const char *dynamicTextureName )
{
	ASSERT( resolution > 0 );
	ASSERT( maxCascadeCount > 0 && maxCascadeCount <= MAX_CASCADES );

	// Shut first
	Shut();
		
	// Load shadowmap resolution settings
	m_shadowmapResolution = resolution;
	m_shadowmapMaxCascadeCount = maxCascadeCount;

	// Create depthstencil texture array
	{

		GpuApi::TextureDesc desc;
		desc.type		= GpuApi::TEXTYPE_ARRAY;	// array texture, we will render into slicces using VIEWs not the GS
		desc.width		= m_shadowmapResolution;
		desc.height		= m_shadowmapResolution;
		desc.sliceNum	= Clamp<Uint16>( m_shadowmapMaxCascadeCount, 1, MAX_CASCADES );	// limited to the cascade count ( obvious )
		desc.initLevels = 1;
		desc.format		= GpuApi::TEXFMT_D32F;
		desc.usage		= GpuApi::TEXUSAGE_DepthStencil | GpuApi::TEXUSAGE_Samplable | GpuApi::TEXUSAGE_NoDepthCompression;

		m_shadowmapDepthStencilArrayRead = GpuApi::CreateTexture( desc, GpuApi::TEXG_Shadow );
		ASSERT( m_shadowmapDepthStencilArrayRead );
		GpuApi::SetTextureDebugPath(m_shadowmapDepthStencilArrayRead, "shadowMapDSARead");

#ifdef RED_PLATFORM_DURANGO
		Uint32 textureSize = GpuApi::CalcTextureSize( desc );
		desc.esramOffset = (32 * 1024 * 1024) - textureSize;
		desc.usage |= GpuApi::TEXUSAGE_ESRAMResident;

		m_shadowmapDepthStencilArrayWrite = GpuApi::CreateTexture( desc, GpuApi::TEXG_System );
		ASSERT( m_shadowmapDepthStencilArrayWrite );
		GpuApi::SetTextureDebugPath(m_shadowmapDepthStencilArrayWrite, "shadowMapDSAWrite");
#endif
	}

	// Register shadowmap resources as dynamic textures ( for debug preview )
	m_isRegisteredDynamicTexture = (nullptr != dynamicTextureName);
	if ( m_isRegisteredDynamicTexture )
	{
		GpuApi::AddDynamicTexture( m_shadowmapDepthStencilArrayRead, dynamicTextureName );
	}
}

void CCascadeShadowResources::Shut()
{
	if ( m_isRegisteredDynamicTexture )
	{
		GpuApi::RemoveDynamicTexture( m_shadowmapDepthStencilArrayRead );
	}

#ifdef RED_PLATFORM_DURANGO
	GpuApi::SafeRelease( m_shadowmapDepthStencilArrayWrite );
#endif

	GpuApi::SafeRelease( m_shadowmapDepthStencilArrayRead );
	m_shadowmapResolution = 0;
	m_shadowmapMaxCascadeCount = 0;
	m_isRegisteredDynamicTexture = false;
}

//----

CRenderInterface::CRenderInterface( IPlatformViewport* platformViewport )
	: m_stateManager( nullptr )
	, m_debugDrawer( nullptr )
#ifdef USE_SCALEFORM
	, m_renderScaleform( nullptr )
#endif
	, m_meshBatcher( nullptr )
	, m_particleBatcher( nullptr )
	, m_terrainBatcher( nullptr )
	, m_flaresBatcher( nullptr )	
	, m_distantLightBatcher( nullptr )
	, m_dynamicDecalBatcher( nullptr )
	, m_swarmBatcher( nullptr )
	, m_surfacesHideGuard( 0 )
	, _m_surfaces( nullptr )
	, m_lastTickDelta( 0.0f )
	, m_timeScale( 1.0f )
	, m_viewportsOwnedByRenderThread( false )
	, m_renderThread( nullptr )
	, m_currentDeviceState( ERDS_Operational )
	, m_lastTextureEvictionTime( 0.0 )
	, m_dropOneFrame( false )
	, m_renderCachets( false )
	, m_platformViewport( platformViewport )
	, m_DynamicPreviewTexture( 0 )
	, m_DynamicPreviewTextureMip( 0 )
	, m_DynamicPreviewTextureSlice( 0 )
	, m_DynamicPreviewTextureColorMin( 0.0f )
	, m_DynamicPreviewTextureColorMax( 1.0f )
	, m_shadowManager( nullptr )
	, m_envProbeManager( nullptr )
	, m_isAsyncMaterialCompilationEnabled( true )
	, m_numPendingPrefetches( 0 )
#ifdef TEXTURE_MEMORY_DEFRAG_ENABLED
	, m_defragHelper( nullptr )
#endif
#ifdef USE_NVIDIA_FUR
	, m_hairSDK( nullptr )
#endif
#ifndef RED_FINAL_BUILD
	, m_gpuProfiler( nullptr )
#endif
{
	InGameConfig::GRefreshEventDispatcher::GetInstance().RegisterForEvent( CNAME( refreshEngine ), &m_hairWorksRefreshListener );
}

CRenderInterface::~CRenderInterface()
{
	InGameConfig::GRefreshEventDispatcher::GetInstance().UnregisterFromAllEvents( &m_hairWorksRefreshListener );

	GpuApi::SafeRelease( m_computeTileLights );
	GpuApi::SafeRelease( m_computeTileDimmers );
	GpuApi::SafeRelease( m_computeConstantBuffer );
#ifndef RED_PLATFORM_ORBIS
	GpuApi::SafeRelease( m_computeRawConstantBuffer );
#endif
	GpuApi::SafeRelease( m_sharedConstantBuffer );
	GpuApi::SafeRelease( m_backBufferRescaled );

	// Delete Direct3D interface
	CloseDevice();
}

Bool CRenderInterface::CanUseResourceCache() const
{
#ifdef RED_PLATFORM_CONSOLE
	return true;
#else
	return !GIsEditor;
#endif
}

GpuApi::TextureRef CRenderInterface::GetHiResEntityShadowmap( CRenderCollector& collector ) const
{
	if ( m_hiresEntityShadowmap )
	{
		return m_hiresEntityShadowmap;
	}

	if ( !collector.GetScene() || !collector.GetScene()->GetTerrainShadows() )
	{
		return GpuApi::TextureRef::Null();
	}

	return collector.GetScene()->GetTerrainShadows()->GetShadowDepthBuffer();
}

Bool CRenderInterface::SupportsArrayTextures() const
{
	return GpuApi::GetCapabilities().textureArray;
}

Bool CRenderInterface::SupportsDepthBoundTest() const
{
	return GpuApi::GetCapabilities().depthBoundTest;
}

Bool CRenderInterface::Init()
{
	// Create rendering GpuApi environment
	if ( !GpuApi::InitEnv() )
	{
		WARN_RENDERER( TXT("Unable to initialize graphics environment. Make sure driver is intalled."));
		return false;	
	}

#ifdef USE_SCALEFORM
	m_renderScaleform = new CRenderScaleform;
#endif

	// Create gameplay FX manager
	m_gameplayFX = new CGameplayEffects();

	struct CDeferredInit_GameplayFX : public IDeferredInitDelegate
	{
		CGameplayEffects* m_gameplayFX;
		CDeferredInit_GameplayFX(CGameplayEffects* fx) : m_gameplayFX(fx) {}
		void OnBaseEngineInit()
		{
			// Initialize gameplay FX manager. Should be on main thread so that resources can be loaded properly (can't load from other than main thread).
			m_gameplayFX->Init();
		}
	};

	if (GDeferredInit)
	{
		GDeferredInit->AddDelegate(new CDeferredInit_GameplayFX(m_gameplayFX));
	}
	else
	{
		m_gameplayFX->Init();
	}

	RED_VERIFY( CRenderOcclusionData::InitExtraMemory() );

#ifdef TEXTURE_MEMORY_DEFRAG_ENABLED
	auto scratchRegion = GpuApi::AllocateInPlaceMemoryRegion( GpuApi::INPLACE_DefragTemp, 8 * 1024 * 1024, MC_Default );
	GpuApi::UnlockInPlaceMemoryRegion( GpuApi::INPLACE_DefragTemp, scratchRegion );

	m_defragHelper = new CRenderDefragHelper( scratchRegion );

	// Streamed textures pool defrag task
	{
		auto texturesGpuPool = static_cast< Red::MemoryFramework::GpuAllocatorThreadSafe* >( GpuApi::GpuApiMemory::GetInstance()->GetPool( GpuApi::GpuMemoryPool_InPlaceRenderData ) );
		CStreamedAndResidentTexturesDefragTask* streamedTexturesDefragTask = new CStreamedAndResidentTexturesDefragTask( texturesGpuPool, Red::MemoryFramework::GpuAllocatorThreadSafe::DefragShortLivedRegions );
		m_defragHelper->AppendTask( streamedTexturesDefragTask );
	}

#if 0
	// Resident textures pool defrag task
	{
		auto texturesGpuPool = static_cast< Red::MemoryFramework::GpuAllocatorThreadSafe* >( GpuApi::GpuApiMemory::GetInstance()->GetPool( GpuApi::GpuMemoryPool_InPlaceRenderData ) );
		CResidentTexturesDefragTask* residentTexturesDefragTask = new CResidentTexturesDefragTask( texturesGpuPool, Red::MemoryFramework::GpuAllocatorThreadSafe::DefragLongLivedRegions );
		m_defragHelper->AppendTask( residentTexturesDefragTask );
	}
#endif

#ifndef RED_PLATFORM_ORBIS
	// Meshes pool defrag task
	{
#ifdef RED_PLATFORM_DURANGO
		GpuApi::EMemoryPoolLabel meshesPoolLabel = GpuApi::GpuMemoryPool_InPlaceMeshBuffers;
#else // ORBIS
		GpuApi::EMemoryPoolLabel meshesPoolLabel = GpuApi::GpuMemoryPool_RenderData;
#endif
		auto meshesGpuPool = static_cast< Red::MemoryFramework::GpuAllocatorThreadSafe* >( GpuApi::GpuApiMemory::GetInstance()->GetPool( meshesPoolLabel ) );
		CMeshDefragTask* meshesDefragTask = new CMeshDefragTask( meshesGpuPool, Red::MemoryFramework::GpuAllocatorThreadSafe::DefragShortLivedRegions );
		m_defragHelper->AppendTask( meshesDefragTask );
	}
#endif
#endif

	// Initialized
	return true;
}

void CRenderInterface::Flush()
{
	if ( m_renderThread )
	{
		// Insert a fence into the command buffer

		IRenderFence* fence = CreateFence();

		( new CRenderCommand_Fence( fence ) )->Commit();

		// Flush it
		fence->FlushFence();
		fence->Release();
	}
}

void CRenderInterface::Tick( float timeDelta )
{
#ifdef USE_SPEED_TREE
	SSpeedTreeRenderStats::NextFrame();
#endif

	// Update last tick delta
	m_lastTickDelta = Max( 0.f, timeDelta );
	
	// Allow lights flicering in editor, when game is actually paused
#ifdef NO_EDITOR
	// Update time scale (some proxies needs the "game" time for the tick (ie. light flickering)
	if( GGame )
	{
		if( !GGame->IsActive() || GGame->IsPaused() ) m_timeScale = 0.0f;
		else
			m_timeScale = GGame->GetTimeScale();
	}	
#endif

	// Update post processes
	m_postProcess->Update( timeDelta );

	for ( Uint32 i=0; i<m_viewports.Size(); i++ )
	{
		m_viewports[i]->UpdateViewportProperties();
	}

	// Tick viewports
	if ( !GGame || !GGame->IsActive() || GGame->IsPaused() )
	{
		for ( Uint32 i=0; i<m_viewports.Size(); i++ )
		{
			m_viewports[i]->Tick( timeDelta );
		}
	}

#ifndef NO_ASYNCHRONOUS_MATERIALS
	TickRecompilingMaterials();
#endif

	if ( m_dropOneFrame )
	{
		if ( GGame )
		{
			if ( !GGame->IsPaused() )
			{	
				GRender->Flush();

				if ( GGame->GetActiveWorld() && GGame->GetActiveWorld()->GetEnvironmentManager() )
				{
					GGame->GetActiveWorld()->GetEnvironmentManager()->SetInstantAdaptationTrigger();
					GGame->GetActiveWorld()->GetEnvironmentManager()->SetInstantDissolveTrigger();
				}
				m_dropOneFrame = false;

			}
		}
		else
		{
			m_dropOneFrame = false;
		}
	}

#ifdef USE_NVIDIA_FUR
	if( m_hairWorksRefreshListener.GetAndClearRefresh() == true && Config::cvHairWorksLevel.Get() != 0 )
	{
		UpdateHairWorksPreset();
	}
#endif //USE_NVIDIA_FUR
}

void CRenderInterface::TakeScreenshot( const SScreenshotParameters& screenshotParamters, Bool* finished, Bool& status )
{
	CWorld* world = GGame->GetActiveWorld();

	if ( !world )
	{
		status = false;
		// no world to grab data from
		if ( finished )
		{
			*finished = true;
		}		
		return;
	}

	//if ( ( screenshotParamters.m_yaw == 1 ) && ( screenshotParamters.m_pitch == 1 ) )
	{
		// Let the rendering thread do the job
		CRenderFrameInfo frameInfo( GGame->GetViewport() );
		frameInfo.m_clearColor = Color::BLACK;
		SScreenshotSystem::GetInstance().OverrideShowFlags( frameInfo );

		frameInfo.SetShadowConfig( world->GetShadowConfig() );
		CRenderFrame* compiledFrame = world->GenerateFrame( frameInfo.m_viewport, frameInfo );
		if ( compiledFrame )
		{
			( new CRenderCommand_TakeUberScreenshot( compiledFrame, world->GetRenderSceneEx(), screenshotParamters, status ) )->Commit();
			compiledFrame->Release();
		}

		// Flush rendering
		GRender->Flush();
	}
	/*
	TODO: figure out later
	else
	{
		CRenderFrameInfo frameInfoOrig( GGame->GetViewport() );
		world->GenerateFrame( frameInfoOrig );

		EulerAngles oldRotation = frameInfoOrig.m_camera.GetRotation();

		Float screensPitchDiff = 90.0f / ( (screenshotParamters.m_pitch - screenshotParamters.m_pitch % 2 - 2 ) );
		Int32 screensPitchOffset = ( screenshotParamters.m_pitch - screenshotParamters.m_pitch % 2 ) / 2;

		for ( Int32 j = 0; j < screenshotParamters.m_pitch; ++j )
		{
			EulerAngles rotation = oldRotation + EulerAngles( 0, ( j - screensPitchOffset ) * screensPitchDiff, 0 );

			for ( Int32 i = 0; i < screenshotParamters.m_yaw; ++i )
			{
				EulerAngles finalRotation = rotation + EulerAngles( 0, 0, i * ( 360.0f / screenshotParamters.m_yaw ) );
				{
					// Let the rendering thread do the job
					CRenderFrameInfo frameInfo( GGame->GetViewport() );
					world->GenerateFrame( frameInfo );

					frameInfo.m_camera.SetRotation( finalRotation );

					frameInfo.m_clearColor = Color::BLACK;
					CRenderFrame* compiledFrame = GRender->CreateFrame( nullptr, frameInfo );
					if ( compiledFrame )
					{
						( new CRenderCommand_TakeUberScreenshot( compiledFrame, world->GetRenderSceneEx(), screenshotParamters ) )->Commit();
						compiledFrame->Release();
					}

					// Flush rendering
					GRender->Flush();
				}
			}
		}
	}
	*/

	if ( finished )
	{
		*finished = true;
	}	
}

void CRenderInterface::TakeQuickScreenshot( const SScreenshotParameters& screenshotParameters )
{
	TakeOneRegularScreenshot( screenshotParameters );
}

void CRenderInterface::TakeQuickScreenshotNow( const SScreenshotParameters& screenshotParameters )
{
	TakeOneRegularScreenshotNow( screenshotParameters );
}

RED_INLINE static Bool IsRenderTexture( IDynamicRenderResource* res )
{
	return res && res->IsRenderTexture();
}

#if DEBUG_RENDER_ELEMENT_STATS

void DumpLeakStats()
{
	// Engine textures
	{
		Uint32 numTextures = 0;
		for ( ObjectIterator< CBitmapTexture > it; it; ++it )
		{
			CBitmapTexture* bt = *it;
			LOG_RENDERER( TXT("EngineTexture[%i]: %s"), numTextures, bt->GetDepotPath().AsChar() );
			++numTextures;
		}
	}

	// Render textures
	{
		Uint32 numTextures = 0;
		for ( CRenderResourceIterator it; it; ++it )
		{
			if ( IsRenderTexture( *it ) )
			{
				CRenderTexture* tex = static_cast< CRenderTexture* >( *it );
				LOG_RENDERER( TXT("RenderTexture[%i]: %s"), numTextures, tex->GetDebugPath().AsChar() );
				++numTextures;
			}
		}
	}

	// Render materials
	{
		Uint32 numMaterials = 0;
		for ( CRenderResourceIterator it; it; ++it )
		{
			if ( (*it)->GetCategory() == CNAME( RenderMaterialParameters ) || (*it)->GetCategory() == CNAME( RenderMaterial ) )
			{
				CRenderMaterialParameters* tex = static_cast< CRenderMaterialParameters* >( *it );
				LOG_RENDERER( TXT("RenderMaterial[%i]: %s"), numMaterials, tex->GetDebugPath().AsChar() );
				++numMaterials;
			}
		}
	}

	// CRAP
	extern Red::Threads::CAtomic< Int32 > GNumElements;
	LOG_RENDERER( TXT("Num elements: %i"), GNumElements.GetValue() );

	// Crap
	extern void PrintProxyList();
	PrintProxyList();

	// Dump GPU api stats
	GpuApi::DumpResourceStats();
}

#endif

//dex++
void CRenderInterface::RecreateShadowmapResources()
{
	LOG_RENDERER( TXT("Recreating shadowmap resources") );

	// Flush render thread
	IRenderFence* theFence = CreateFence();
	( new CRenderCommand_Fence( theFence ) )->Commit();
	theFence->FlushFence();

	// Destroy and create the shadowmap resources
	ReleaseShadowmapResources();
	InitShadowmapResources();
}
//dex--

void CRenderInterface::RecalculateTextureStreamingSettings()
{
	// Flush render thread
	IRenderFence* theFence = CreateFence();
	( new CRenderCommand_Fence( theFence ) )->Commit();
	theFence->FlushFence();

	for ( CRenderResourceIterator it; it; ++it )
	{
		if ( IsRenderTexture( *it ) )
		{
			CRenderTexture* texture = static_cast< CRenderTexture* >( *it );
			texture->RecalculateMipStreaming();
		}
	}
}

void CRenderInterface::ReloadEngineShaders()
{
	LOG_RENDERER( TXT("Reloading main shaders") );

	// Flush render thread
	IRenderFence* theFence = CreateFence();
	( new CRenderCommand_Fence( theFence ) )->Commit();
	theFence->FlushFence();

#if 0 // Don't do that now - we have proper verification if included shaders have changed
	// **** Don't do it ever in fact, now that it's a chained set of caches.
	// Recreate shader cache
	delete GShaderCache;
	GShaderCache = new CShaderCache();
#endif

	// Start task
	GFeedback->BeginTask( TXT("Reloading engine shaders"), true );
	GFeedback->UpdateTaskProgress( 0, 1 );

	// Reload shaders
	for ( ObjectIterator< CMaterialGraph > it; it; ++it )
	{
		(*it)->ForceRecompilation();
	}

	// End of task
	GFeedback->EndTask();
}

void CRenderInterface::ReloadSimpleShaders()
{
	LOG_RENDERER( TXT("Reloading simple shaders") );

	// clean up the log a bit, so you can actually see errors from last shader recompilation
	RED_LOG_ERROR( RED_LOG_CHANNEL( GpuApi ), TXT("\n     .    .    .    .    .    .    Reloading simple shaders started    .    .    .    .    .    .    \n") );

	// Flush render thread
	IRenderFence* theFence = CreateFence();
	( new CRenderCommand_Fence( theFence ) )->Commit();
	theFence->FlushFence();

	// Count shaders
	Uint32 numTotalShaders = 0;
	for ( CRenderResourceIterator i; i; ++i )
	{
		if (   i->GetCategory() == CNAME( RenderShaderPair )
			|| i->GetCategory() == CNAME( RenderShaderQuadruple )
			|| i->GetCategory() == CNAME( RenderShaderTriple )
			|| i->GetCategory() == CNAME( RenderShaderCompute )
			|| i->GetCategory() == CNAME( RenderShaderStreamOut )
			)
		{
			numTotalShaders++;
		}
	}

	GShaderCache->InvalidateStaticShaders();
	GShaderCache->EnableSaving( false );

	// Start task
	Uint32 numShaders = 0;
	GFeedback->BeginTask( TXT("Reloading simple shaders"), true );
	GFeedback->UpdateTaskProgress( 0, 1 );
	for ( CRenderResourceIterator i; i; ++i )
	{
		if ( i->GetCategory() == CNAME( RenderShaderPair ) )
		{
			// Update progress
			GFeedback->UpdateTaskProgress( numShaders, numTotalShaders );
			numShaders++;

			// Reload
			CRenderShaderPair* pair = (CRenderShaderPair*)(*i);
			pair->Reload();
		}
		if ( i->GetCategory() == CNAME( RenderShaderTriple ) )
		{
			// Update progress
			GFeedback->UpdateTaskProgress( numShaders, numTotalShaders );
			numShaders++;

			// Reload
			CRenderShaderTriple* triple = (CRenderShaderTriple*)(*i);
			triple->Reload();
		}
		if ( i->GetCategory() == CNAME( RenderShaderQuadruple ) )
		{
			// Update progress
			GFeedback->UpdateTaskProgress( numShaders, numTotalShaders );
			numShaders++;

			// Reload
			CRenderShaderQuadruple* quad = (CRenderShaderQuadruple*)(*i);
			quad->Reload();
		}
		if ( i->GetCategory() == CNAME( RenderShaderCompute ) )
		{
			// Update progress
			GFeedback->UpdateTaskProgress( numShaders, numTotalShaders );
			numShaders++;

			// Reload
			CRenderShaderCompute* comp = (CRenderShaderCompute*)(*i);
			comp->Reload();
		}
		if ( i->GetCategory() == CNAME( RenderShaderStreamOut ) )
		{
			// Update progress
			GFeedback->UpdateTaskProgress( numShaders, numTotalShaders );
			numShaders++;

			// Reload
			CRenderShaderStreamOut* comp = (CRenderShaderStreamOut*)(*i);
			comp->Reload();
		}
	}

	GShaderCache->EnableSaving( true );
	GShaderCache->Flush();

	// End of task
	GFeedback->EndTask();	

	RED_LOG_ERROR( RED_LOG_CHANNEL( GpuApi ), TXT("\n     .    .    .    .    .    .    Reloading simple shaders finished    .    .    .    .    .    .    \n") );
}

void CRenderInterface::ReloadTextures()
{
	LOG_RENDERER( TXT("Reloading textures") );

	// Cancel pending streaming before reloading textures :)
	{
		// Send the cancel streaming command
		( new CRenderCommand_CancelTextureStreaming() )->Commit();

		// Flush renderer
		CRenderFenceHelper flushHelper( GRender );
		flushHelper.Flush();
	}

	// Start task
	GFeedback->BeginTask( TXT("Reloading textures"), true );
	GFeedback->UpdateTaskProgress( 0, 1 );

	// Unload all textures first

	// Grab textures to reload
	Uint32 numTextures = 0;
	for ( ObjectIterator< CBitmapTexture > it; it; ++it )
	{
		CBitmapTexture* texture = *it;
		numTextures += 1;

		if ( !texture->HasFlag( OF_Finalized | OF_Discarded ) && texture->GetFile() )
		{
			texture->ReleaseRenderResource();
		}
	}

	Uint32 numTextureArrays = 0;
	for ( ObjectIterator< CTextureArray > it; it; ++it )
	{
		CTextureArray* array = *it;
		numTextureArrays += 1;

		if ( !array->HasFlag( OF_Finalized | OF_Discarded ) && array->GetFile() )
		{
			array->ReleaseRenderResource();
		}
	}

	Uint32 numCubes = 0;
	for ( ObjectIterator< CCubeTexture > it; it; ++it )
	{
		CCubeTexture* cube = *it;
		numCubes += 1;

		if ( !cube->HasFlag( OF_Finalized | OF_Discarded ) && cube->GetFile() )
		{
			cube->ReleaseRenderResource();
		}
	}

	// Create resources - bitmap textures
	Uint32 textureIndex = 0;
	for ( ObjectIterator< CBitmapTexture > it; it; ++it, ++textureIndex )
	{
		CBitmapTexture* texture = *it;

		if ( texture->GetFile() )
		{
			// Do not reload empty textures
			if ( !texture->GetWidth() || !texture->GetHeight() || !texture->GetMipCount() )
			{
				continue;
			}

			// Get texture file path
			String textureName;
			CDiskFile* sourceFile = texture->GetFile();
			if ( sourceFile )
			{
				// Use the file name
				CFilePath filePath( sourceFile->GetDepotPath() );
				textureName = filePath.GetFileName();
			}
			else
			{
				//  Use the class name and size
				textureName = String::Printf( TXT("%s %ix%i"), texture->GetClass()->GetName().AsString().AsChar(), texture->GetWidth(), texture->GetHeight() );
			}

			// Update progress
			GFeedback->UpdateTaskInfo( TXT("Loading '%ls'..."), textureName.AsChar() );
			GFeedback->UpdateTaskProgress( textureIndex, numTextures );

			// Recreate rendering resource, this will reload the texture data
			texture->CreateRenderResource();
		}
	}

	// Create resources - texture arrays
	Uint32 textureArrayIndex = 0;
	for ( ObjectIterator< CTextureArray > it; it; ++it, ++textureArrayIndex )
	{
		CTextureArray* array = *it;
		if ( array && array->GetFile() )
		{
			// Do not reload empty textures
			TDynArray< CBitmapTexture* > textures;
			array->GetTextures( textures );
			if ( textures.Empty() || !array->GetMipCount() )
			{
				continue;
			}

			CBitmapTexture* firstTex = textures[ 0 ];
			if ( !firstTex->GetWidth() || !firstTex->GetHeight() )
			{
				continue;
			}

			// Get texture file path
			String textureName;
			CDiskFile* sourceFile = array->GetFile();
			if ( sourceFile )
			{
				// Use the file name
				CFilePath filePath( sourceFile->GetDepotPath() );
				textureName = filePath.GetFileName();
			}
			else
			{
				//  Use the class name and size
				textureName = String::Printf( TXT("%s %ix%i"), array->GetClass()->GetName().AsString().AsChar(), firstTex->GetWidth(), firstTex->GetHeight() );
			}

			// Update progress
			GFeedback->UpdateTaskInfo( TXT("Loading '%ls'..."), textureName.AsChar() );
			GFeedback->UpdateTaskProgress( textureArrayIndex, numTextureArrays );

			// Recreate rendering resource, this will reload the texture data
			array->CreateRenderResource();
		}
	}

#ifndef NO_CUBEMAP_GENERATION
	// Create resources - cube textures
	Uint32 cubeIndex = 0;
	for ( ObjectIterator< CCubeTexture > it; it; ++it, ++cubeIndex )
	{
		CCubeTexture* cube = *it;
		if ( cube && cube->GetFile() )
		{
			// Get texture file path
			String textureName;
			CDiskFile* sourceFile = cube->GetFile();
			if ( sourceFile )
			{
				// Use the file name
				CFilePath filePath( sourceFile->GetDepotPath() );
				textureName = filePath.GetFileName();
			}
			else
			{
				Uint32 width = 0;
				Uint32 height = 0;
				Uint32 index = 0;
				while ( index < 6 )
				{
					const CubeFace& f = cube->GetFace( index );
					if ( f.GetSize( width, height ) )
					{
						break;
					}
					++index;
				}				

				//  Use the class name and size
				textureName = String::Printf( TXT("%s %ix%i"), cube->GetClass()->GetName().AsString().AsChar(), width, height );
			}

			// Update progress
			GFeedback->UpdateTaskInfo( TXT("Loading '%ls'..."), textureName.AsChar() );
			GFeedback->UpdateTaskProgress( cubeIndex, numCubes );

			// Recreate rendering resource, this will reload the texture data
			cube->CreateRenderResource();
		}
	}
#endif

	// End of task
	GFeedback->EndTask();
}

void CRenderInterface::RecreatePlatformResources()
{
	// Flush render thread
	IRenderFence* theFence = CreateFence();
	( new CRenderCommand_Fence( theFence ) )->Commit();
	theFence->FlushFence();

	// recreate the shadow manager
	if ( m_shadowManager )
	{
		delete m_shadowManager;
		m_shadowManager = new CRenderShadowManager();
	}

	// recreate envprobe manager
	if ( m_envProbeManager )
	{
		delete m_envProbeManager;
		m_envProbeManager = new CRenderEnvProbeManager();
	}

	if ( _m_surfaces )
	{
		Uint32 width = _m_surfaces->GetWidth();
		Uint32 height = _m_surfaces->GetHeight();

		Uint32 msaaLevelX, msaaLevelY;
		GetInitialMSAALevel( &msaaLevelX, &msaaLevelY );

		_m_surfaces->Release();
		_m_surfaces = new CRenderSurfaces( width, height, msaaLevelX, msaaLevelY );
	}
}

// FIXME2<<< TMP STATIC HACK, BUT "WINDOWS" AREN"T REALLY PART OF THE HEADER INTERFACE AND A FAKE VIEWPORT SEEMS CRAPPY TOO
// Needs to be "deactivated" when the game paused by alt-tabbing out in the editor since the render viewport isn't top level and doesn't get the activate message, just the mouse leave... so whatevers
CViewportWindowRoot* GViewportWindowRoot;

ViewportHandle CRenderInterface::CreateViewport( void* TopLevelWindow, void* ParentWindow, const String& title, Uint32 width, Uint32 height, EViewportWindowMode windowMode, Bool vsync /*=false*/ )
{
	//FIXME2<<<
	RED_ASSERT( ::SIsMainThread() );

	if ( ! GViewportWindowRoot )
	{
		GViewportWindowRoot = new CViewportWindowRoot;
	}

	// Create window
#ifdef RED_PLATFORM_ORBIS
	Red::TUniquePtr< CViewportWindow > window( new CViewportWindow( GViewportWindowRoot, title, width, height, (EViewportWindowMode)windowMode, m_viewportsOwnedByRenderThread ) );
#else
	Red::TUniquePtr< CViewportWindow > window(  new CViewportWindow( GViewportWindowRoot, (HWND)TopLevelWindow, (HWND)ParentWindow, title, width, height, (EViewportWindowMode)windowMode, m_viewportsOwnedByRenderThread ) );
#endif

	ASSERT( window, TXT("Window can't be created") );

	// Initialize device if this is the first viewport created
	if ( m_viewports.Size() == 0 )
	{	
		if ( !InitDevice( width, height, windowMode == VWM_Fullscreen, vsync ) )
		{
			// Failed to initialize D3D
			return ViewportHandle();
		}
	}

	// FIXME: Dumb tmp hack... worst case make a generic interface that takes void*
#ifdef RED_PLATFORM_WINPC
	// Should also be the only viewport, but just in case
	if ( m_viewports.Size() == 0 && m_viewportsOwnedByRenderThread )
	{
		RED_ASSERT( GEngine->GetInputDeviceManager() );
		const HWND hWnd = window->GetWindowHandle();
		static_cast< CInputDeviceManagerWin32* >( GEngine->GetInputDeviceManager() )->SetTopLevelHwnd( hWnd );
	}
#endif

	Uint32 renderWidth = width;
	Uint32 renderHeight = height;

	AdjustPlatformResolution( renderWidth, renderHeight );

	// Create viewport wrapper
	Red::TSharedPtr< CRenderViewport > viewport( new CRenderViewport( width, height, (EViewportWindowMode)windowMode, window.Release(), vsync, renderWidth, renderHeight ) );
	m_viewports.PushBack( viewport.Get() );
	
	return viewport;
}

class CRenderCommand_CreateViewport : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_CreateViewport, "CreateViewport" );

public:
	static ViewportHandle		s_createdViewport;

public:
	Uint32						m_width;
	Uint32						m_height;
	EViewportWindowMode			m_windowMode;
	String						m_title;

public:
	CRenderCommand_CreateViewport( const String& title, Uint32 width, Uint32 height, EViewportWindowMode windowMode )
		: m_title( title )
		, m_width( width )
		, m_height( height )
		, m_windowMode( windowMode )
	{};

	virtual void Execute()
	{
		s_createdViewport = GetRenderer()->CreateViewport( nullptr, nullptr, m_title, m_width, m_height, m_windowMode );
	}
};

ViewportHandle CRenderCommand_CreateViewport::s_createdViewport = ViewportHandle();

ViewportHandle CRenderInterface::CreateGameViewport( const String& title, Uint32 width, Uint32 height, EViewportWindowMode windowMode )
{
	if (QuickBoot::g_quickInitViewport)
	{
		return QuickBoot::g_quickInitViewport;
	}

	// Already have render thread
	if ( m_renderThread )
	{
		WARN_RENDERER( TXT("Rendering thread is already created" ) );
		return ViewportHandle();
	}

	// Render thread will own all the viewport
	m_viewportsOwnedByRenderThread = true;

	// Create rendering thread before creating any other shit since we will create Device and the window from the Thread !
	m_renderThread = new CRenderThread( this, true );
	m_renderThread->InitThread();
#if defined( RED_PLATFORM_ORBIS )
	// On the PS4 the O/S interrupts cores starting at zero. So start at the highest on the first core. TBD: give render thread core 3 instead?
	m_renderThread->SetAffinityMask( 1 << 2 );
	m_renderThread->SetPriority( Red::Threads::TP_TimeCritical ); // video decoder will share the same core, but at lower priority, to fill the gaps on vsync
#elif defined( RED_PLATFORM_DURANGO )
	m_renderThread->SetAffinityMask( 1 << 1 );
#endif

	// Spawn command that will create the viewport
	//( new CRenderCommand_CreateViewport( title, width, height, fullscreen ) )->Commit();

	// Flush the render thread so the windows gets created
	Flush();

	// Return created viewport
	//return CRenderCommand_CreateViewport::s_createdViewport;

	return GetRenderer()->CreateViewport( nullptr, nullptr, title, width, height, windowMode );
}

class CRenderCommand_ResizeRenderSurfaces : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_ResizeRenderSurfaces, "ResizeRenderSurfaces" );

public:
	Uint32			m_width;
	Uint32			m_height;

public:
	CRenderCommand_ResizeRenderSurfaces( Uint32 width, Uint32 height )
		: m_width( width )
		, m_height( height )
	{}

	virtual void Execute()
	{
		GetRenderer()->ResizeRenderSurfaces( m_width, m_height );
	}
};


IRenderGameplayRenderTarget* CRenderInterface::CreateGameplayRenderTarget( const AnsiChar* tag )
{
	return new CRenderGameplayRenderTarget( tag );
}

void CRenderInterface::RequestResizeRenderSurfaces( Uint32 width, Uint32 height )
{
	( new CRenderCommand_ResizeRenderSurfaces( width, height ) )->Commit();
	Flush();
}

void CRenderInterface::ResizeRenderSurfaces( Uint32 width, Uint32 height )
{
	if ( _m_surfaces )
	{
		Uint32 msaaLevelX, msaaLevelY;
		GetInitialMSAALevel( &msaaLevelX, &msaaLevelY );

		_m_surfaces->Release();
		_m_surfaces = new CRenderSurfaces( width, height, msaaLevelX, msaaLevelY );

		// Release these. They depend on screen resolution, so we want to recreate them.
		GpuApi::SafeRelease( m_computeTileLights );
		GpuApi::SafeRelease( m_computeTileDimmers );
		InitTiledDeferredConstants();
	}
}

static CMaterialCompilerDefines BuildMaterialCompilerDefinesMSAABase( CRenderInterface &renderInterface )
{
	CMaterialCompilerDefines defines;

	Uint32 msaaLevelX, msaaLevelY;
	GetInitialMSAALevel( &msaaLevelX, &msaaLevelY );
	Uint32 msaaLevel = msaaLevelX * msaaLevelY;
	
	defines.Add( ("MSAA_NUM_SAMPLES_X"),	StringAnsi::Printf( ("%u"), msaaLevelX ) );
	defines.Add( ("MSAA_NUM_SAMPLES_Y"),	StringAnsi::Printf( ("%u"), msaaLevelY ) );
	defines.Add( ("MSAA_NUM_SAMPLES"),		StringAnsi::Printf( ("%u"), msaaLevel ) );
	defines.Add( ("MSAA_NUM_SAMPLES_INV"),	StringAnsi::Printf( ("%.12f"), (Float)(1.f / msaaLevel) ) );

	return defines;
}

Bool CRenderInterface::InitDevice( Uint32 width, Uint32 height, Bool fullscreen, Bool vsync )
{
	GSplash->UpdateProgress( TXT("Initializing render device...") );
	
#ifdef RED_LOGGING_ENABLED
	CTimeCounter initTimer;
#endif

	GRenderSettingsMipBias = Config::cvTextureMipBias.Get();
	if ( !GpuApi::InitDevice( width, height, (GIsEditor ? false : fullscreen), vsync ) )
	{
		WARN_RENDERER( TXT("Unable to create Direct3D device" ) );
		return false;
	}

	GSplash->UpdateProgress( TXT("Initializing shaders...") );

	if ( GShaderCache )
	{
		GShaderCache->EnableSaving( false );
	}

	Uint32 shadersInitialized = 0;
	Uint32 numberOfStaticShaders = 0;

	{
		// count the shaders
		#define RENDER_SHADER_GEN(var,name,defines) ++numberOfStaticShaders
		#define RENDER_SHADER_TESS_GEN(var,name,defines) ++numberOfStaticShaders
		#define RENDER_SHADER_COMPUTE(var,name,defines) ++numberOfStaticShaders
		#define RENDER_SHADER_GEOM_GEN(var,name,defines) ++numberOfStaticShaders
		#define RENDER_SHADER_SO_GEN(var,name,defines,bctOut) ++numberOfStaticShaders

		#include "..\engine\renderShaders.h"

		#undef RENDER_SHADER_GEN
		#undef RENDER_SHADER_TESS_GEN
		#undef RENDER_SHADER_COMPUTE
		#undef RENDER_SHADER_GEOM_GEN
		#undef RENDER_SHADER_SO_GEN
	}
	
	{
		// initialize the shaders
		#define RENDER_SHADER_GEN(var,name,defines) var = CRenderShaderPair::Create( name, defines ); GSplash->UpdateProgress( TXT("Initializing shaders (%u/%u)..."), ++shadersInitialized, numberOfStaticShaders )
		#define RENDER_SHADER_TESS_GEN(var,name,defines) var = CRenderShaderQuadruple::Create( name, defines ); GSplash->UpdateProgress( TXT("Initializing shaders (%u/%u)..."), ++shadersInitialized, numberOfStaticShaders )
		#define RENDER_SHADER_COMPUTE(var,name,defines) var = CRenderShaderCompute::Create( name, defines ); GSplash->UpdateProgress( TXT("Initializing shaders (%u/%u)..."), ++shadersInitialized, numberOfStaticShaders )
		#define RENDER_SHADER_GEOM_GEN(var,name,defines) var = CRenderShaderTriple::Create( name, defines ); GSplash->UpdateProgress( TXT("Initializing shaders (%u/%u)..."), ++shadersInitialized, numberOfStaticShaders )
		#define RENDER_SHADER_SO_GEN(var,name,defines,bctOut) var = CRenderShaderStreamOut::Create( name, defines, bctOut ); GSplash->UpdateProgress( TXT("Initializing shaders (%u/%u)..."), ++shadersInitialized, numberOfStaticShaders )
		#define RENDER_SHADER_DEFINES_NOOPT()		CMaterialCompilerDefines()
		#ifdef RED_PLATFORM_DURANGO
		# define RENDER_SHADER_DEFINES_BASE()		CMaterialCompilerDefines().Add( "__XBOX_WAVESIM_ITERATION_N", "1" ).Add( "STENCIL_TEX_FETCH_CHANNEL", "0" )
		#else
		# define RENDER_SHADER_DEFINES_BASE()		CMaterialCompilerDefines().Add( "__XBOX_WAVESIM_ITERATION_N", "1" ).Add( "STENCIL_TEX_FETCH_CHANNEL", "1" )
		#endif
		#define RENDER_SHADER_MSAA_DEFINES_BASE()	BuildMaterialCompilerDefinesMSAABase( *this )
		
		#ifndef NO_EDITOR
			#define RENDER_SHADER_GEN_EDITOR RENDER_SHADER_GEN
		#else
			#define RENDER_SHADER_GEN_EDITOR(var,name,defines)
		#endif

		CTimeCounter staticShadersTimer;
		#include "..\engine\renderShaders.h"
		RED_LOG( RED_LOG_CHANNEL( Shaders ), TXT("Initializing shaders took: %1.3fms"), staticShadersTimer.GetTimePeriodMS() );

		#undef RENDER_SHADER_GEN
		#undef RENDER_SHADER_TESS_GEN
		#undef RENDER_SHADER_COMPUTE
		#undef RENDER_SHADER_GEOM_GEN
		#undef RENDER_SHADER_SO_GEN
		#undef RENDER_SHADER_GEN_EDITOR
		#undef RENDER_SHADER_DEFINES_NOOPT
		#undef RENDER_SHADER_DEFINES_BASE
		#undef RENDER_SHADER_MSAA_DEFINES_BASE
	}

	if ( GShaderCache )
	{
		GShaderCache->EnableSaving( true );
		GShaderCache->Flush();
	}

	GSplash->UpdateProgress( TXT("Initializing render device...") );

RED_MESSAGE("This has to be investigated further, the whole gamma handling is not really implemented");
#if 0
	//setup gamma ramp
	GpuApi::SetupDeviceGammaMode(GpuApi::GAMMA_LINEAR, 1.0f );
#endif

	// Create normal render surfaces
	{
		ASSERT( 0 == m_surfacesHideGuard );

		Uint32 msaaLevelX, msaaLevelY;
		GetInitialMSAALevel( &msaaLevelX, &msaaLevelY );

		Uint32 rendererWidth  = width;
		Uint32 rendererHeight = height;

		AdjustPlatformResolution( rendererWidth, rendererHeight );

		_m_surfaces = new CRenderSurfaces( rendererWidth, rendererHeight, msaaLevelX, msaaLevelY );
	}

	// Create local sub systems
	m_stateManager = new CRenderStateManager( );
	m_debugDrawer = new CRenderDrawer();
	m_postProcess = new CRenderPostProcess( _m_surfaces );
	m_meshBatcher = new CRenderMeshBatcher( 8192 );
	m_particleBatcher = new CRenderPartricleBatcher();
	m_flaresBatcher = new CRenderFlaresBatcher();
	m_distantLightBatcher = new CRenderDistantLightBatcher();
	m_swarmBatcher = new CRenderSwarmBatcher();
#ifdef USE_APEX
	m_apexBatcher = new CRenderApexBatcher();
#endif
	m_dynamicDecalBatcher = new CRenderDynamicDecalBatcher();
	m_textureStreamingManager = new CRenderTextureStreamingManager();
	
#ifdef USE_NVIDIA_FUR

#ifdef RED_LOGGING_ENABLED
	m_hairSDKLogHandler = new HairWorksHelpers::DefaultLogHandler();
#else
	m_hairSDKLogHandler = nullptr;
#endif

	m_hairSDK = HairWorksHelpers::InitSDK( m_hairSDKLogHandler );
	if ( m_hairSDK )
	{
		TDynArray< String > paths;
		GShaderCache->GetAllFurShaderCaches( paths );

		// Load in the order they were attached. If hairworks encounters a duplicate, it keeps the previous and does not load
		// the new one, so this way we'll have the most recent.
		for ( const String& path : paths )
		{
			if ( !HairWorksHelpers::LoadShaderCache( m_hairSDK, path, true ) )
			{
				ERR_RENDERER( TXT("Failed to load HairWorks shader cache from %ls"), path.AsChar() );
			}
		}

		GFSDK_HAIR_RETURNCODES result;

		result = m_hairSDK->InitRenderResources( GpuApi::Hacks::GetDevice(), GpuApi::Hacks::GetDeviceContext() );
		RED_ASSERT( result == GFSDK_RETURN_OK, TXT("InitRenderResources failed") );

		result = m_hairSDK->CreateShadersFromShaderCache();
		RED_ASSERT( result == GFSDK_RETURN_OK, TXT("CreateShadersFromShaderCache failed") );

		UpdateHairWorksPreset();
	}
	else
	{
		LOG_RENDERER( TXT("HairWorks library not initialized" ) );
	}

#endif

	//dex++
	m_shadowManager = new CRenderShadowManager();
	//dex--

	// Create envprobe manager
	m_envProbeManager = new CRenderEnvProbeManager();

	// Create skin manager
	m_skinManager = new CRenderSkinManager( 512,256 );

	// Initialize shadowmap
	InitShadowmapResources();

	// Initialize tiled deferred
	InitTiledDeferredConstants();

#if 0 // GFx 3
	// Initialize scaleform renderer
	m_scaleformRenderer->SetupDisplay();
#endif // #if 0

	// Create rendering thread if not already created
	if ( !m_renderThread )
	{
		m_renderThread = new CRenderThread( this, false );
		m_renderThread->InitThread();
#if defined( RED_PLATFORM_DURANGO ) || defined( RED_PLATFORM_ORBIS )
		m_renderThread->SetAffinityMask( 1 << 1 );
#endif
	}

#ifdef W2_PLATFORM_WIN32
	// Recreate resources
	for ( ObjectIterator< CBitmapTexture > it; it; ++it )
	{
		(*it)->CreateRenderResource();
	}
#endif

#ifdef RED_LOGGING_ENABLED
	LOG_RENDERER(TXT("Device initialized in %1.2f sec"), initTimer.GetTimePeriod() );
#endif

#ifndef RED_FINAL_BUILD
	m_gpuProfiler = new CRenderGpuProfiler();
#endif

	// Initialized
	return true;
}

void CRenderInterface::CloseDevice()
{
#ifndef RED_FINAL_BUILD
	delete m_gpuProfiler;
#endif

#ifdef USE_NVIDIA_FUR
	// Release HairWorks SDK
	HairWorksHelpers::ShutdownSDK( m_hairSDK );
	if ( m_hairSDKLogHandler )
	{
		delete m_hairSDKLogHandler;
		m_hairSDKLogHandler = nullptr;
	}
#endif

	RED_VERIFY( CRenderOcclusionData::ShutdownExtraMemory() );

	// Release shadowmap resources
	ReleaseShadowmapResources();
	
	// Close state manager
	if ( m_stateManager )
	{
		delete m_stateManager;
		m_stateManager = nullptr;
	}

	// Close debug drawer
	if ( m_debugDrawer )
	{
		delete m_debugDrawer;
		m_debugDrawer = nullptr;
	}

	// Close post processing framework
	if ( m_postProcess )
	{
		delete m_postProcess;
		m_postProcess = nullptr;
	}

	// Delete particle batcher
	if ( m_particleBatcher )
	{
		m_particleBatcher->Release();
		m_particleBatcher = nullptr;
	}

	// Delete mesh batcher
	if ( m_meshBatcher )
	{
		m_meshBatcher->Release();
		m_meshBatcher = nullptr;
	}
	
	if( m_distantLightBatcher )
	{
		m_distantLightBatcher->Release();
		m_distantLightBatcher = nullptr;
	}

#ifdef USE_SPEED_TREE
	//TODO this is a temporary solution, please remove as soon as the memory leak is fixed in speedtree and remove the corresponding include from the top of the file too
	SpeedTree::CShaderConstantGPUAPI::ReleaseGfxResources();
#endif

#ifdef USE_APEX
	if ( m_apexBatcher )
	{
		m_apexBatcher->Release();
		m_apexBatcher = nullptr;
	}
#endif

	if ( m_dynamicDecalBatcher )
	{
		m_dynamicDecalBatcher->Release();
		m_dynamicDecalBatcher = nullptr;
	}

	// Delete swarm batcher
	if ( m_swarmBatcher )
	{
		m_swarmBatcher->Release();
		m_swarmBatcher = nullptr;
	}

	// Delete flares batcher
	if ( m_flaresBatcher )
	{
		m_flaresBatcher->Release();
		m_flaresBatcher = nullptr;
	}

	// Release internal surfaces
	ASSERT( 0 == m_surfacesHideGuard );
	if ( _m_surfaces )
	{
		_m_surfaces->Release();
		_m_surfaces = nullptr;
	}

	//dex++: Delete shadow manager
	if ( m_shadowManager )
	{
		delete m_shadowManager;
		m_shadowManager = nullptr;
	}
	//dex--

	// Release skin manager
	if ( m_skinManager )
	{
		m_skinManager->Release();
		m_skinManager = nullptr;
	}

#ifdef USE_SCALEFORM
	if ( m_renderScaleform )
	{
		if (GRender != nullptr && GRender->GetRenderThread() != nullptr)
		{
			CGuiRenderCommand<TShutdownSystem>::Send();
			GRender->Flush();
		}

		m_renderScaleform->Release();
		m_renderScaleform = nullptr;
	}
#endif

	if ( m_gameplayFX )
	{
		m_gameplayFX->Shutdown();
		delete m_gameplayFX;
		m_gameplayFX = nullptr;
	}

	{
#define RENDER_SHADER_GEN(var,name,...) SAFE_RELEASE( var )
#define RENDER_SHADER_TESS_GEN(var,name,...) SAFE_RELEASE( var )
#define RENDER_SHADER_COMPUTE(var,name,...) SAFE_RELEASE( var )
#define RENDER_SHADER_GEOM_GEN(var,name,...) SAFE_RELEASE( var )
#define RENDER_SHADER_SO_GEN(var,name,...) SAFE_RELEASE( var )
#define RENDER_SHADER_GEN_DEPTHTEXTURES RENDER_SHADER_GEN
#define RENDER_SHADER_GEN_NODEPTHTEXTURES(var,name,...) 

#ifndef NO_EDITOR
#define RENDER_SHADER_GEN_EDITOR RENDER_SHADER_GEN
#else
#define RENDER_SHADER_GEN_EDITOR(var,name,...)
#endif

#include "..\engine\renderShaders.h"

#undef RENDER_SHADER_GEN
#undef RENDER_SHADER_TESS_GEN
#undef RENDER_SHADER_COMPUTE
#undef RENDER_SHADER_GEOM_GEN
#undef RENDER_SHADER_SO_GEN
#undef RENDER_SHADER_GEN_EDITOR
#undef RENDER_SHADER_GEN_DEPTHTEXTURES
#undef RENDER_SHADER_GEN_NODEPTHTEXTURES
	}

	// Release textures
	for ( ObjectIterator< CBitmapTexture > it; it; ++it )
	{
		(*it)->ReleaseRenderResource();
	}

	// Kill rendering thread
	if ( m_renderThread )
	{
		CRenderThread* renderThread = static_cast< CRenderThread* >( m_renderThread );
		renderThread->RequestExit();
		renderThread->JoinThread();
		delete m_renderThread;
		m_renderThread = nullptr;
	}

	// Delete envprobe manager AFTER render thread is destroyed since it may still be prefetching env probes
	if ( m_envProbeManager )
	{
		delete m_envProbeManager;
		m_envProbeManager = nullptr;
	}

#ifdef TEXTURE_MEMORY_DEFRAG_ENABLED
	if ( m_defragHelper )
	{
		delete m_defragHelper;
		m_defragHelper = nullptr;
	}
#endif
	
	SAFE_RELEASE(m_renderScaleform);

	if ( m_textureStreamingManager )
	{
		delete m_textureStreamingManager;
		m_textureStreamingManager = nullptr;
	}

	// Close device
	GpuApi::ShutDevice();
	
	// Show the message
	LOG_RENDERER( TXT("D3D9 renderer closed") );
}

void CRenderInterface::ViewportClosed( CRenderViewport* viewport )
{
	// Remove viewport from the list
	ASSERT( m_viewports.Exist( viewport ) );
	m_viewports.Remove( viewport );

	//// If this was the last viewport close the device
	//if ( !m_viewports.Size() )
	//{
	//	LOG_RENDERER( TXT("Deleted last DX9 viewport, closing device") );
	//	CloseDevice();
	//}
}

void CRenderInterface::Suspend()
{
#ifdef RED_PLATFORM_DURANGO
	GpuApi::SuspendDevice();
#endif
}

void CRenderInterface::Resume()
{
#ifdef RED_PLATFORM_DURANGO
	GpuApi::ResumeDevice();
#endif
}

void CRenderInterface::SetupSurfacesAvailabilityGuard( Bool isAvailable )
{
	m_surfacesHideGuard += (isAvailable ? -1 : 1);
	ASSERT( m_surfacesHideGuard < 10 && "Why so deep. Are we leaking?" );
	ASSERT( m_surfacesHideGuard >= 0 );
}

CRenderSurfaces* CRenderInterface::GetSurfaces() const
{
	ASSERT( 0 == m_surfacesHideGuard && "Looks like surfaces is not meant to be used at this moment. Are we rendering to some custom surfaces?" );
	return _m_surfaces;
}

IRenderThread* CRenderInterface::GetRenderThread()
{
	return m_renderThread;
}

Bool CRenderInterface::IsMSAAEnabled( const CRenderFrameInfo &frameInfo ) const
{
	return GetEnabledMSAALevel( frameInfo ) > 1;
}

Uint32 CRenderInterface::GetEnabledMSAALevel( const CRenderFrameInfo &frameInfo ) const
{
	const Bool isEnabled = 
		Config::cvAllowMSAA.Get() &&
		frameInfo.m_envParametersGame.m_displaySettings.m_allowAntialiasing && 
		EMM_None == frameInfo.m_envParametersGame.m_displaySettings.m_displayMode &&
		nullptr != _m_surfaces;
	return isEnabled ? GetSurfaces()->GetMSAALevel() : 1;
}

ILoadingScreenFence* CRenderInterface::CreateLoadingScreenFence( const SLoadingScreenFenceInitParams& initParams )
{
	return new CRenderLoadingScreenFence( initParams );
}

//dex++: extended with a flag to indicate if the created scene is a world scene
IRenderScene* CRenderInterface::CreateScene( Bool isWorldScene )
{
	return new CRenderSceneEx( isWorldScene );
}
//dex--

IRenderFence* CRenderInterface::CreateFence()
{
	return new CRenderFence();
}

#if MICROSOFT_ATG_DYNAMIC_SCALING
namespace GpuApi
{
	extern Uint32 g_DynamicScaleWidthFullRes;
	extern Uint32 g_DynamicScaleHeightFullRes;
}
#endif

CRenderFrame* CRenderInterface::CreateFrame( CRenderFrame* masterFrame, const CRenderFrameInfo& info )
{
	if ( 0 == info.m_width || 0 == info.m_height )
	{
		return nullptr;
	}

	CRenderFrameOverlayInfo overlayInfo = { info.m_width , info.m_height };

	if( !info.m_customRenderResolution )
	{

		if( Config::cvForcedRendererOverlayResolution.Get() )
		{
			overlayInfo.m_width = Config::cvForcedRendererResolutionWidth.Get();
			overlayInfo.m_height = Config::cvForcedRendererResolutionHeight.Get();	
		}

		if( Config::cvForcedRendererResolution.Get() )
		{
			CRenderFrameInfo commonInfo = info;

#if MICROSOFT_ATG_DYNAMIC_SCALING
			commonInfo.m_width = GpuApi::g_DynamicScaleWidthFullRes;
			commonInfo.m_height = GpuApi::g_DynamicScaleWidthFullRes;
#else
			commonInfo.m_width = Config::cvForcedRendererResolutionWidth.Get();
			commonInfo.m_height = Config::cvForcedRendererResolutionHeight.Get();	
#endif

			// Create rendering frame
			return new CRenderFrame( commonInfo, overlayInfo );
		}

	}

	// Create rendering frame
	return new CRenderFrame( info, overlayInfo );
}

IRenderSkinningData* CRenderInterface::CreateSkinningBuffer( Uint32 numMatrices, Bool extendedBuffer )
{
	if ( numMatrices )
	{
		if (extendedBuffer)
		{
			return new CRenderSkinningData( numMatrices );
		}
		else
		{
			return new CRenderSkinningData( numMatrices, false );
		}
	}

	// Not created
	ASSERT( !"Empty skinning data buffer" );
	return nullptr;
}

IRenderSkinningData* CRenderInterface::CreateNonRenderSkinningBuffer( Uint32 numMatrices )
{
	return new CRenderSkinningDataEngineBuffer( numMatrices );
}

IRenderSwarmData* CRenderInterface::CreateSwarmBuffer( Uint32 numBoids )
{
	return new CRenderSwarmData( numBoids );
}

IRenderScaleform* CRenderInterface::GetRenderScaleform() const
{
#ifdef USE_SCALEFORM
	return m_renderScaleform;
#else
	return nullptr;
#endif
}

#ifdef USE_APEX
physx::apex::NxUserRenderResourceManager* CRenderInterface::CreateApexRenderResourceManager()
{
	return new CApexRenderResourceManager();
}
#endif

Bool CRenderInterface::IsFading()
{
	return GetPostProcess() ? GetPostProcess()->IsFadeInProgress() : false;
}

Bool CRenderInterface::IsBlackscreen()
{
	return GetPostProcess() ? GetPostProcess()->IsBlackscreen() : false;
}

Bool CRenderInterface::IsStreamingTextures() const
{
	return (m_textureStreamingManager && m_textureStreamingManager->IsCurrentlyStreaming()) || IsStreamingGuiTextures() || HasPendingPrefetch();
}


Bool CRenderInterface::IsStreamingGuiTextures() const
{
	CRenderScaleformTextureManager* scaleformTextureManager = static_cast<CRenderScaleformTextureManager*>( GetRenderScaleform()->GetTextureManager() );
	CScaleformTextureCacheQueue* streamingQueue = scaleformTextureManager ? scaleformTextureManager->GetStreamingQueue() : nullptr;
	return (streamingQueue && streamingQueue->GetNumPendingJobs() > 0 );
}

#ifdef USE_NVIDIA_FUR
void CRenderInterface::UpdateHairWorksPreset()
{
	if( m_hairSDK )
	{
		m_hairSDK->SetGlobalStrandSmoothness( Config::cvHairWorksGlobalStrandSmoothness.Get() );
		m_hairSDK->SetGlobalDensityLimit( Config::cvHairWorksGlobalDensityLimit.Get() );
		m_hairSDK->SetGlobalDensityQuality( Config::cvHairWorksGlobalDensityQuality.Get() );
		m_hairSDK->SetGlobalDetailLODFactor( Config::cvHairWorksGlobalDetailLODFactor.Get() );
		m_hairSDK->SetGlobalWidthLimit( Config::cvHairWorksGlobalWidthLimit.Get() );
	}
}
#endif // USE_NVIDIA_FUR

Bool CRenderInterface::IsDuringEnvProbesPrefetch() const
{
	return m_envProbeManager && m_envProbeManager->IsDuringPrefetch();
}

void CRenderInterface::GetPendingStreamingTextures( Uint32& outNumTextures, Uint32& outDataSize ) const
{
	outNumTextures = 0;
	outDataSize = 0;
}

Bool CRenderInterface::IsStreamingMeshes() const
{
	return m_meshStreamingStats.IsLoading();
}

void CRenderInterface::GetPendingStreamingMeshes( Uint32& outNumMeshes, Uint32& outDataSize ) const
{
	m_meshStreamingStats.GetStats( outNumMeshes, outDataSize );
}

namespace DeviceStateStateMachineHelpers
{
	ERenderingDeviceState OnLost( ERenderingDeviceState currentDeviceState )
	{
		switch ( currentDeviceState )
		{
		case ERDS_Operational:
			{
				LOG_RENDERER(TXT("Device lost"));
				return ERDS_Lost_ResourcesNotReleased;
			}
			break;
		case ERDS_Lost_ResourcesNotReleased:
		case ERDS_Lost_ResourcesReleased:
			{
				return currentDeviceState;
			}
			break;
		case ERDS_NotReset:
			{
				return ERDS_Lost_ResourcesReleased;
			}
			break;
		default:
			{
				ASSERT( false && "Error in device state state machine or some device fuckup" );
				return currentDeviceState;
			}
		}
	}

	ERenderingDeviceState OnNotReset( ERenderingDeviceState currentDeviceState )
	{
		switch ( currentDeviceState )
		{
		case ERDS_Operational:
			{
				LOG_RENDERER(TXT("Device lost"));
				return ERDS_Lost_ResourcesNotReleased;
			}
			break;
		case ERDS_Lost_ResourcesNotReleased:
			{
				return currentDeviceState;
			}
			break;
		case ERDS_Lost_ResourcesReleased:
			{
				LOG_RENDERER(TXT("Device reset!"));
				return ERDS_NotReset;
			}
			break;
		case ERDS_NotReset:
			{
				return ERDS_NotReset;
			}
			break;
		default:
			{
				ASSERT( false && "Error in device state state machine or some device fuckup" );
				return currentDeviceState;
			}
		}
	}

}

void CRenderInterface::RefreshDeviceState()
{
	// Test device state, if we are lost then do not render
	switch ( GpuApi::TestDeviceCooperativeLevel() )
	{
	case GpuApi::DEVICECOOPLVL_Operational:
		{
			ASSERT( m_currentDeviceState == ERDS_Operational );
		}
		break;


		//HACK DX10 TBD device lost handling

	//case GpuApi::DEVICECOOPLVL_Lost:
	//	{
	//		m_currentDeviceState = DeviceStateStateMachineHelpers::OnLost( m_currentDeviceState );
	//	}
	//	break;

	//case GpuApi::DEVICECOOPLVL_NotReset:
	//	{
	//		m_currentDeviceState = DeviceStateStateMachineHelpers::OnNotReset( m_currentDeviceState );
	//	}
	//	break;

	//case GpuApi::DEVICECOOPLVL_InternalError:	
	//	{
	//		ASSERT( !"internal error - wtf should we do?" );
	//		LOG_RENDERER(TXT("Internal error"));
	//		m_currentDeviceState = (m_currentDeviceState == ERDS_Operational) ? ERDS_Lost_ResourcesNotReleased : m_currentDeviceState;
	//	}
	//	break;

	default:
		ASSERT( !"invalid device state" );
		break;
	}
}


Bool CRenderInterface::PrepareRenderer()
{
	// At the beginning of each frame, check for rendering device state - if it is lost, game should not tick!
	// If it is, perform needed tasks eg. releasing device or resetting it.
	RefreshDeviceState();

	switch( m_currentDeviceState )
	{
	case ERDS_Lost_ResourcesNotReleased:
		{
			// Flush shit, then release device
			LOG_RENDERER( TXT("Flushing all pending loading") );
			GEngine->FlushAllLoading();
			LOG_RENDERER( TXT("Loading flushed") );
			GRender->Flush();

			ReleaseRenderingResources();

			return false;
		}
	case ERDS_Lost_ResourcesReleased:
		{
#ifdef RED_PLATFORM_WINPC
			// Sleep to wait for events causing device to be in state for reset
			Sleep( 200 );
#endif
			return false;
		}
	case ERDS_NotReset:
		{
			GRender->Flush();

			// Reset shit
			ResetDevice();

			return false;
		}
	case ERDS_Operational:
		{
			// That's what we are hoping for :) 
			return true;
		}
	default:
		{
			ASSERT( false && "Some problematic renderer device state in main tick" );
			return false;
		}
	}
}


void CRenderInterface::ReleaseRenderingResources()
{
#ifdef W2_PLATFORM_WIN32
	LOG_RENDERER( TXT("Releasing device...") );

	m_textureStreamingManager->TryFinishUpdateTask( true, true );

	// Local resources cleanup
	ReleaseShadowmapResources();

	if ( m_postProcess )
	{
		// Release additional postprocess resources (eg. nvidia's ssao stuff)
		m_postProcess->OnLostDevice();
	}

	// Release all dynamic rendering resources
	for ( CRenderResourceIterator i; i; ++i )
	{
		i->OnDeviceLost();
	}

	for ( ObjectIterator< CBitmapTexture > it; it; ++it )
	{
		(*it)->ReleaseRenderResource();
	}

	for ( ObjectIterator< CTextureArray > it; it; ++it )
	{
		(*it)->ReleaseRenderResource();
	}

	for ( ObjectIterator< CCubeTexture> it; it; ++it )
	{
		(*it)->ReleaseRenderResource();

	}

	for ( ObjectIterator< CParticleEmitter > it; it; ++it )
	{
		(*it)->ReleaseRenderResource();
	}

	for ( ObjectIterator< CMesh > it; it; ++it )
	{
		(*it)->ReleaseRenderResource();
	}

	for ( ObjectIterator< IMaterial > it; it; ++it )
	{
		(*it)->RemoveRenderResource();
	}
	
	CDrawableComponent::RecreateProxiesOfRenderableComponents();

	// Flush rendering thread so we have up to date data
	GRender->Flush();

	{
		for ( Uint32 i = 0; i < m_existingRenderScenes.Size(); ++i )
		{
			m_existingRenderScenes[i]->RemovePendingFadeOutRemovals( true );
		}
	}

	LOG_RENDERER( TXT("Device released") );

#endif

	m_currentDeviceState = ERDS_Lost_ResourcesReleased;
}

Bool CRenderInterface::ResetDevice()
{

#ifdef W2_PLATFORM_WIN32
	LOG_RENDERER( TXT("Resetting device...") );

	// Delete all swapchains in all viewports, needed to preform reset
	for ( Uint32 i=0; i<m_viewports.Size(); i++ )
	{
		m_viewports[i]->Reset();
	}

	{
#ifdef USE_SCALEFORM
		CGuiRenderCommand<THandleDeviceLost>::Send();
#endif
		GRender->Flush(); // Making sure it's handled before trying to reset the device
	}

	// Try to reset device
	if ( !GpuApi::ResetDevice() )
	{
		LOG_RENDERER( TXT("Unable to reset device") );
		return false;
	}

	// Reset state manager
	m_stateManager->Reset();

	// Device was reset
	LOG_RENDERER( TXT("Device reset !") );

	// Local resources restore
	InitShadowmapResources();

	// Recreate all dynamic rendering resources
	for ( CRenderResourceIterator i; i; ++i )
	{
		i->OnDeviceReset();
	}

#ifdef USE_SCALEFORM
	CGuiRenderCommand<THandleDeviceReset>::Send();
#endif

	// Flush rendering thread so we have up to date data
	GRender->Flush();

	m_currentDeviceState = ERDS_Operational;

	// Flush rendering thread so we have up to date data
	GRender->Flush();

	for ( ObjectIterator< CBitmapTexture > it; it; ++it )
	{
		(*it)->CreateRenderResource();
	}

	for ( ObjectIterator< CCubeTexture > it; it; ++it )
	{
		(*it)->CreateRenderResource();
	}

	for ( ObjectIterator< IMaterial > it; it; ++it )
	{
		(*it)->CreateRenderResource();
	}

	for ( ObjectIterator< CParticleEmitter > it; it; ++it )
	{
		(*it)->CreateRenderResource();
	}

	for ( ObjectIterator< CMesh > it; it; ++it )
	{
		(*it)->CreateRenderResource();
	}

	CDrawableComponent::RecreateProxiesOfRenderableComponents();

	// Flush rendering thread so we have up to date data
	GRender->Flush();


	// Info
	LOG_RENDERER( TXT("Dynamic resources recreated !") );
#endif

	m_currentDeviceState = ERDS_Operational;
	m_dropOneFrame = true;

	return true;
}

void CRenderInterface::ForceFakeDeviceReset()
{
	// Flush rendering thread so we have up to date data
	GRender->Flush();

	m_currentDeviceState = ERDS_Lost_ResourcesNotReleased;

	ReleaseRenderingResources();

	// Flush rendering thread so we have up to date data
	GRender->Flush();

	m_currentDeviceState = ERDS_NotReset;

	ResetDevice();

	// Flush rendering thread so we have up to date data
	GRender->Flush();
}

void CRenderInterface::ForceFakeDeviceLost()
{
	m_currentDeviceState = ERDS_Lost_ResourcesReleased;
}

void CRenderInterface::ForceFakeDeviceUnlost()
{
	m_currentDeviceState = ERDS_Operational;
}

IRenderVideo* CRenderInterface::CreateVideo( CName videoClient, const SVideoParams& videoParams ) const
{
	return new CRenderVideo( videoClient, videoParams );
}

void CRenderInterface::RenderVolumes( const CRenderFrameInfo& info, CRenderCollector& collector, GpuApi::TextureRef sampleTexture )
{
	PC_SCOPE_RENDER_LVL1(RenderVolumes);

	const Uint32 volumesWidth = info.m_width / WEATHER_VOLUMES_SIZE_DIV;
	const Uint32 volumesHeight = info.m_height / WEATHER_VOLUMES_SIZE_DIV;

	// We need temp texture in order to use depth buffer from renderSurfaces
#ifdef RED_PLATFORM_DURANGO
	const GpuApi::TextureRef tempTarget = GetSurfaces()->GetRenderTargetTex( RTN_DURANGO_InteriorVolume_TempSurface );
#else
	const GpuApi::TextureRef tempTarget = GetSurfaces()->GetRenderTargetTex( RTN_Color3 );
#endif

	// Render volumes
	{
		CGpuApiDeviceStateGrabber targetStateGrabber ( GpuApi::DEVSTATECAT_RenderTargetSetup );

		GpuApi::RenderTargetSetup rtSetup;
		{
			rtSetup.SetColorTarget( 0, tempTarget );		
			rtSetup.SetViewport( volumesWidth, volumesHeight );
			GpuApi::TextureRef depth = GetRenderer()->IsMSAAEnabled( info ) ?  GetRenderer()->GetSurfaces()->GetDepthBufferTexMSAA() : GetRenderer()->GetSurfaces()->GetDepthBufferTex();
			rtSetup.SetDepthStencilTarget( depth );
		}
		GpuApi::SetupRenderTargets( rtSetup );

		GetStateManager().SetCamera( info.m_camera );

		RenderingContext rc( info.m_camera );

		// Setup reversed projection
		const Bool origReversedProjection = GpuApi::IsReversedProjectionState();
		GpuApi::SetReversedProjectionState( info.m_camera.IsReversedProjection() );

		// Bind constants
		GpuApi::BindConstantBuffer( 13, m_computeConstantBuffer, GpuApi::PixelShader );

		// Outer pass
		{
			ClearDepthTarget( GpuApi::GetClearDepthValueRevProjAware() );
			CGpuApiScopedDrawContext outerDrawContext ( GpuApi::DRAWCONTEXT_VolumeExterior );
			collector.RenderElements( ERenderingSortGroup::RSG_Volumes, rc, ERenderingPlane::RPl_Scene, RECG_ALL );
		}

		// Explicit wait for depth writes to finish before the clear since we don't have any under the hood synchronization right now
		GpuApi::WaitForDepthTargetWrites( false );

		// Inner pass		
		{
			ClearDepthTarget( GpuApi::GetClearDepthValueRevProjAware() );
			CGpuApiScopedDrawContext innerDrawContext ( GpuApi::DRAWCONTEXT_VolumeInterior );
			collector.RenderElements( ERenderingSortGroup::RSG_Volumes, rc, ERenderingPlane::RPl_Scene, RECG_ALL );
		}

		// Explicit wait for depth writes once again.
		// This one is needed here as a workaround for depth buffer not being cleared in RenderDeferredSetupGBuffer call right after this function finishes.
		// For some reason interior_combine pass caused depth buffer not being cleared (note that we're doing htile based clear).
		// Note that it worked well when we had the UpdateCameraInteriorFactor between UpdateVolumes and RenderDeferredSetupGBuffer (refactored in the same CL).
		GpuApi::WaitForDepthTargetWrites( true );

		// Unbind constants
		GpuApi::BindConstantBuffer( 13, GpuApi::BufferRef::Null(), GpuApi::PixelShader );

		// Restore reversed projection
		GpuApi::SetReversedProjectionState( origReversedProjection );
	}	

	// Copy back to desired target
	RED_ASSERT( tempTarget != sampleTexture );
	{
		GpuApi::RenderTargetSetup rtSetup;
		rtSetup.SetColorTarget( 0, sampleTexture );		
		rtSetup.SetViewport( volumesWidth, volumesHeight );
		GpuApi::SetupRenderTargets( rtSetup );

		CGpuApiScopedDrawContext innerDrawContext ( GpuApi::DRAWCONTEXT_PostProcSet );

		CRenderShaderPair *shader = m_shaderVolumesCombine;
		RED_ASSERT( shader );
		if ( shader )
		{
			shader->Bind();
		}

		GpuApi::BindTextures( 0, 1, &tempTarget, GpuApi::PixelShader );

		const Int32 clampWidthMax = Max<Uint32>( 1, volumesWidth ) - 1;
		const Int32 clampHeightMax = Max<Uint32>( 1, volumesHeight ) - 1;		
		GetStateManager().SetPixelConst( PSC_Custom_0, Vector ( (Float)volumesWidth, (Float)volumesHeight, (Float)clampWidthMax, (Float)clampHeightMax ) );

		GetRenderer()->GetDebugDrawer().DrawQuad( Vector (-1, -1, 0, 1), Vector (1, 1, 0, 1), 0.5f );
	}	
}

void CRenderInterface::RenderWaterBlends( const CRenderFrameInfo& info, CRenderCollector& collector, GpuApi::TextureRef sampleTexture )
{
	PC_SCOPE_RENDER_LVL1(RenderWaterBlends);

	if( !collector.m_renderCollectorData->m_elements[ ERenderingPlane::RPl_Scene ].m_elements[ RSG_WaterBlend ].Empty() )
	{
		CGpuApiDeviceStateGrabber targetStateGrabber ( GpuApi::DEVSTATECAT_RenderTargetSetup );
		GpuApi::RenderTargetSetup rtSetup;
		{
			rtSetup.SetColorTarget( 0, sampleTexture );
			rtSetup.SetViewport( info.m_width / WEATHER_VOLUMES_SIZE_DIV, info.m_height / WEATHER_VOLUMES_SIZE_DIV );
		}
		GpuApi::SetupRenderTargets( rtSetup );

		GetStateManager().SetCamera( info.m_camera );
		RenderingContext rc( info.m_camera );

		// Setup reversed projection
		const Bool origReversedProjection = GpuApi::IsReversedProjectionState();
		GpuApi::SetReversedProjectionState( info.m_camera.IsReversedProjection() );

		// This writes to BA of the rtn color3 because it later on shared with volme rendering (using RG then)
		CGpuApiScopedDrawContext outerDrawContext ( GpuApi::DRAWCONTEXT_PostProcAdd_BlueAlpha );
		// We are not clearing here since there was a clear in renderrenderframe
		//ClearColorTarget( sampleTexture, Vector( 0.0f, 0.0f, 0.0f, 0.0f ) );
		{
			collector.RenderElements( ERenderingSortGroup::RSG_WaterBlend, rc, ERenderingPlane::RPl_Scene, RECG_ALL );
		}

		// Restore reversed projection
		GpuApi::SetReversedProjectionState( origReversedProjection );
	}
}

#ifdef USE_NVIDIA_FUR
void CRenderInterface::UpdateFurSimulation( CRenderCollector& collector )
{
	if ( !m_hairSDK || Config::cvHairWorksLevel.Get() == 0 ) 
	{
		return;
	}

	// Prevent from simulating many times under the very same frame index
	// This may happen when capturing ubersampled frames
	if( m_hairSimulateFrameTracker.UpdateOncePerFrame( collector.m_frameIndex ) == FUS_AlreadyUpdated )
	{
		return;
	}

	// There is one frame lag due to some skinning updates, so if game is paused the very first frame of paused game need
	// To be pulled with regular update. Only the next ones can be skipped.
	if( m_hairSimulatedLastFrame.Check( !collector.m_info->m_isGamePaused ) )
	{
		// Update fur skinning
		{
			PC_SCOPE_PIX(UpdateFurSkinning);

			collector.UpdateFurSkinning();
		}

		// simulate fur
		{
			PC_SCOPE_PIX(SimulateFur);

			GFSDK_HAIR_RETURNCODES simulationRes = m_hairSDK->StepSimulation();
			if ( simulationRes != GFSDK_RETURN_OK )
			{
				RED_HALT( "fur simulation error" );
			}
		}
	}

}
#endif // USE_NVIDIA_FUR

void CRenderInterface::SetAsyncCompilationMode( bool enable )
{
	m_isAsyncMaterialCompilationEnabled = enable;
}

Bool CRenderInterface::GetAsyncCompilationMode() const
{
	return m_isAsyncMaterialCompilationEnabled;
}

void CRenderInterface::StretchRect( const GpuApi::TextureRef& from, const Rect& fromArea, const GpuApi::TextureRef& to, const Rect& toArea )
{
	PC_SCOPE_RENDER_LVL1( StretchRect );

	RED_ASSERT( from != to, TXT("Cannot copy a texture to itself") );

	// When Gpu copy is not possible, we fallback to either Pixel or Compute shader version.
	static Bool CopyFallbackCompute = false;

	// If no scaling is needed, try letting GpuApi do it directly first.
	GpuApi::Rect gpuRect( fromArea.m_left, fromArea.m_top, fromArea.m_right, fromArea.m_bottom );
	if ( fromArea.Width() == toArea.Width() && fromArea.Height() == toArea.Height() )
	{
		if ( GpuApi::CopyRect( from, gpuRect, 0, to, toArea.m_left, toArea.m_top, 0 ) )
		{
			return;
		}
	}

	const GpuApi::TextureDesc& fromDesc	= GpuApi::GetTextureDesc( from );
	const GpuApi::TextureDesc& toDesc	= GpuApi::GetTextureDesc( to );

	// Can't use backbuffer as UAV
	// Can only use compute for non-MSAA targets. For simplicity, also non-MSAA source.
	if ( CopyFallbackCompute && ( toDesc.usage & GpuApi::TEXUSAGE_RenderTarget ) && toDesc.msaaLevel <= 1 && fromDesc.msaaLevel <= 1 )
	{
		//////////////////////////////////////////////////////////////////////////
		// Copy with compute shader
		struct ComputeData
		{
			Uint32 destRect[4];
			Float destToSourceParams[4];
		};

		// Can't have a texture bound as output and UAV at the same time, so we'll clear the current render target.
		GpuApi::SetupBlankRenderTargets();

		GpuApi::BindTextureUAVs( 0, 1, &to );


		Float widthRatio = (Float)fromArea.Width() / (Float)toArea.Width();
		Float heightRatio = (Float)fromArea.Height() / (Float)toArea.Height();

		ComputeData data = {
			{ (Uint32)toArea.m_left, (Uint32)toArea.m_top, (Uint32)toArea.Width(), (Uint32)toArea.Height() },
			{
				// Scale
				widthRatio / (Float)fromDesc.width,
					heightRatio / (Float)fromDesc.height,
					// Offset. 0.5 to account for texel center
					( (Float)fromArea.m_left + 0.5f * widthRatio ) / (Float)fromDesc.width,
					( (Float)fromArea.m_top + 0.5f * heightRatio ) / (Float)fromDesc.height,
			}
		};
		GpuApi::SetComputeShaderConsts( data );

		GpuApi::BindTextures( 0, 1, &from, GpuApi::ComputeShader );
		GpuApi::SetSamplerStatePreset( 0, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip, GpuApi::ComputeShader );

		{
			// Must match ThreadCount in simpleCopy.fx
			static Uint32 ThreadCount = 16;
			m_simpleCopyCS->Dispatch( ( toArea.Width() + (ThreadCount-1) ) / ThreadCount, ( toArea.Height() + (ThreadCount-1) ) / ThreadCount, 1);
		}

		GpuApi::BindTextureUAVs( 0, 1, nullptr );
		GpuApi::BindTextures( 0, 1, nullptr, GpuApi::ComputeShader );
	}
	else
	{
		//////////////////////////////////////////////////////////////////////////
		// Copy with fullscreen quad

		GpuApi::RenderTargetSetup rtSetup;
		rtSetup.SetColorTarget( 0, to );
		rtSetup.SetViewport( toArea.Width(), toArea.Height(), toArea.m_left, toArea.m_top );
		GpuApi::SetupRenderTargets( rtSetup );

		// Set draw context
		CGpuApiScopedDrawContext scopedDrawContext( GpuApi::DRAWCONTEXT_PostProcSet );

		// Set textures
		GpuApi::BindTextures( 0, 1, &from, GpuApi::PixelShader );
		GpuApi::SetSamplerStatePreset( 0, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip, GpuApi::PixelShader );

		// Render
		const Float fromWidth  = static_cast< Float >( fromDesc.width );
		const Float fromHeight = static_cast< Float >( fromDesc.height );
		const Vector rectVector(
			static_cast< Float >( fromArea.m_left ),
			static_cast< Float >( fromArea.m_top ),
			static_cast< Float >( fromArea.m_right ),
			static_cast< Float >( fromArea.m_bottom ) );
		m_stateManager->SetVertexConst( VSC_Custom_0,	rectVector / Vector( fromWidth, fromHeight, fromWidth, fromHeight ) );

		switch (toDesc.format)
		{
		case GpuApi::TEXFMT_Float_R32G32B32A32:
			m_simpleCopy32_RGBA->Bind();
			break;
		case GpuApi::TEXFMT_Float_R32G32:
			m_simpleCopy32_RG->Bind();
			break;
		case GpuApi::TEXFMT_Float_R32:
			m_simpleCopy32_R->Bind();
			break;
		default:
			m_simpleCopy->Bind();
			break;
		}

		// Make sure we don't have any extra shaders.
		m_stateManager->SetShader( GpuApi::ShaderRef::Null(), RST_GeometryShader );
		m_stateManager->SetShader( GpuApi::ShaderRef::Null(), RST_HullShader );
		m_stateManager->SetShader( GpuApi::ShaderRef::Null(), RST_DomainShader );

		GpuApi::DrawPrimitiveNoBuffers( GpuApi::PRIMTYPE_TriangleStrip, 2 );

		GpuApi::BindTextures( 0, 1, nullptr, GpuApi::PixelShader );
	}
}


void CRenderInterface::StretchRect( const GpuApi::TextureRef& from, const GpuApi::TextureRef& to )
{
	const GpuApi::TextureDesc fromDesc	= GpuApi::GetTextureDesc( from );
	const GpuApi::TextureDesc toDesc	= GpuApi::GetTextureDesc( to );

	Rect fromArea( 0, fromDesc.width, 0, fromDesc.height );
	Rect toArea( 0, toDesc.width, 0, toDesc.height );
	StretchRect( from, fromArea, to, toArea );
}


void CRenderInterface::CopyTextureData( const GpuApi::TextureRef& destRef, Uint32 destMipLevel, Uint32 destArraySlice, const GpuApi::TextureRef& srcRef, Uint32 srcMipLevel, Uint32 srcArraySlice )
{
	// let the engine try and copy it first
	if (GpuApi::CopyTextureData(destRef, destMipLevel, destArraySlice, srcRef, srcMipLevel, srcArraySlice))
	{
		return;
	}

	// GpuApi could not copy so fallback to Computeshader
	PC_SCOPE_RENDER_LVL1( CopyTextureData );

	const GpuApi::TextureDesc& srcDesc	= GpuApi::GetTextureDesc( srcRef );
	const GpuApi::TextureDesc& destDesc	= GpuApi::GetTextureDesc( destRef );

	Uint16 destRTArraySlice = GpuApi::CalculateTextureSliceMipIndex(destDesc, (Uint16)destArraySlice, (Uint16)destMipLevel);

	GpuApi::RenderTargetSetup originalRtSetup = GpuApi::GetRenderTargetSetup();
	m_stateManager->PushShaderSetup();

	{
		// use compute
		struct ComputeData
		{
			Uint32 destRect[4];
			Float destToSourceParams[4];
		};
		// Can't have a texture bound as output and UAV at the same time, so we'll clear the current render target.
		GpuApi::SetupBlankRenderTargets();
		GpuApi::BindTextureMipLevelUAV( 0, destRef, destArraySlice, destMipLevel );

		Float widthRatio = (Float)srcDesc.width / (Float)destDesc.width;
		Float heightRatio = (Float)srcDesc.height / (Float)destDesc.height;
		ComputeData data = {
			{ (Uint32)0, (Uint32)0, (Uint32)destDesc.width, (Uint32)destDesc.height },
			{
				// Scale
				widthRatio / (Float)srcDesc.width,
					heightRatio / (Float)srcDesc.height,
					// Offset. 0.5 to account for texel center
					( (Float)0 + 0.5f * widthRatio ) / (Float)srcDesc.width,
					( (Float)0 + 0.5f * heightRatio ) / (Float)srcDesc.height,
			}
		};
		GpuApi::SetComputeShaderConsts( data );
		GpuApi::BindTextureMipLevel( 0, srcRef, srcArraySlice, srcMipLevel, GpuApi::ComputeShader );
		GpuApi::SetSamplerStatePreset( 0, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip, GpuApi::ComputeShader );
		{
			// Must match ThreadCount in simpleCopy.fx
			static Uint32 ThreadCount = 16;
			m_simpleCopyCS->Dispatch( ( destDesc.width + (ThreadCount-1) ) / ThreadCount, ( destDesc.height + (ThreadCount-1) ) / ThreadCount, 1);
		}
		GpuApi::BindTextureUAVs( 0, 1, nullptr );
		GpuApi::BindTextures( 0, 1, nullptr, GpuApi::ComputeShader );
	}

	m_stateManager->PopShaderSetup();
	GpuApi::SetupRenderTargets( originalRtSetup );
}


Uint32 DivideByFourRoundUp(Uint32 x)
{
	return (x + 3) / 4;
}

GpuApi::Rect DivideByFourRoundUp(const GpuApi::Rect& x)
{
	GpuApi::Rect out;
	out.left = DivideByFourRoundUp(x.left);
	out.right = DivideByFourRoundUp(x.right);
	out.top = DivideByFourRoundUp(x.top);
	out.bottom = DivideByFourRoundUp(x.bottom);
	return out;
}


void CRenderInterface::CopyTextureData( const GpuApi::TextureRef& destRef1, Uint32 destMipLevel, Uint32 destArraySlice, GpuApi::Rect destRect, const GpuApi::TextureRef& srcRef1, Uint32 srcMipLevel, Uint32 srcArraySlice, GpuApi::Rect srcRect )
{
	PC_SCOPE_RENDER_LVL1( CopyTextureData );

#ifndef RED_PLATFORM_ORBIS
	// For a simple case, we can just use CopyRect to copy mip0 slice0.
	if ( destMipLevel == 0 && srcMipLevel == 0 &&
		( destRect.right - destRect.left ) == ( srcRect.right - srcRect.left ) &&
		( destRect.bottom - destRect.top ) == ( srcRect.bottom - srcRect.top )
		)
	{
		if ( GpuApi::CopyRect( srcRef1, srcRect, srcArraySlice, destRef1, destRect.left, destRect.top, destArraySlice ) )
		{
			return;
		}
	}
#endif


	GpuApi::RenderTargetSetup originalRtSetup = GpuApi::GetRenderTargetSetup();
	m_stateManager->PushShaderSetup();

	{
		// use compute
		struct ComputeData
		{
			Uint32 destRect[4];
			Float destToSourceParams[4];
		};
		// Can't have a texture bound as output and UAV at the same time, so we'll clear the current render target.
		GpuApi::SetupBlankRenderTargets();

		const GpuApi::TextureDesc* srcDesc	= &GpuApi::GetTextureDesc( srcRef1 );
		GpuApi::TextureRef srcRef = srcRef1;
		Bool shouldReleaseSrcRef = false;
		if (GpuApi::IsTextureFormatDXT(srcDesc->format))
		{
			GpuApi::TextureDesc newDesc = *srcDesc;
			newDesc.format = GpuApi::TEXFMT_R32G32_Uint;
			newDesc.width = DivideByFourRoundUp(newDesc.width);
			newDesc.height = DivideByFourRoundUp(newDesc.height);

			shouldReleaseSrcRef = true;
			srcRef = GpuApi::CreateTextureAlias(newDesc, srcRef);
			srcDesc = &GpuApi::GetTextureDesc( srcRef );

			// make sure our Rect is scaled appropriately as well
			srcRect = DivideByFourRoundUp(srcRect);
		}

		const GpuApi::TextureDesc* destDesc	= &GpuApi::GetTextureDesc( destRef1 );
		GpuApi::TextureRef destRef = destRef1;
		Bool shouldReleaseDestRef = false;
		if (GpuApi::IsTextureFormatDXT(destDesc->format))
		{
			GpuApi::TextureDesc newDesc = *destDesc;
			newDesc.format = GpuApi::TEXFMT_R32G32_Uint;
			newDesc.width = DivideByFourRoundUp(newDesc.width);
			newDesc.height = DivideByFourRoundUp(newDesc.height);

			shouldReleaseDestRef = true;
			destRef = GpuApi::CreateTextureAlias(newDesc, destRef);
			destDesc = &GpuApi::GetTextureDesc( destRef );

			// make sure our Rect is scaled appropriately as well
			destRect = DivideByFourRoundUp(destRect);
		}

		GpuApi::BindTextureMipLevelUAV( 0, destRef, destArraySlice, destMipLevel );

		Int32 srcRectWidth = srcRect.right - srcRect.left;
		Int32 srcRectHeight = srcRect.bottom - srcRect.top;
		Int32 destRectWidth = destRect.right - destRect.left;
		Int32 destRectHeight = destRect.bottom - destRect.top;

		Float widthRatio = (Float)srcRectWidth / (Float)destRectWidth;
		Float heightRatio = (Float)srcRectHeight / (Float)destRectHeight;
		ComputeData data = {
			{ (Uint32)destRect.left, (Uint32)destRect.top, (Uint32)destRectWidth, (Uint32)destRectHeight },
			{
				// Scale
				widthRatio / (Float)srcDesc->width,
				heightRatio / (Float)srcDesc->height,
				// Offset. 0.5 to account for texel center
				( (Float)srcRect.left + 0.5f * widthRatio ) / (Float)srcDesc->width,
				( (Float)srcRect.top + 0.5f * heightRatio ) / (Float)srcDesc->height,
			}
		};
		GpuApi::SetComputeShaderConsts( data );
		GpuApi::BindTextureMipLevel( 0, srcRef, srcArraySlice, srcMipLevel, GpuApi::ComputeShader );

		GpuApi::SetSamplerStatePreset( 0, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip, GpuApi::ComputeShader );
		{
			// Must match ThreadCount in simpleCopy.fx
			static Uint32 ThreadCount = 16;
			m_simpleCopyCS->Dispatch(( destRectWidth + (ThreadCount-1) ) / ThreadCount, ( destRectHeight + (ThreadCount-1) ) / ThreadCount, 1);
		}
		GpuApi::BindTextureUAVs( 0, 1, nullptr );
		GpuApi::BindTextures( 0, 1, nullptr, GpuApi::ComputeShader );


		if ( shouldReleaseSrcRef )
		{
			GpuApi::SafeRelease( srcRef );
		}
		if ( shouldReleaseDestRef )
		{
			GpuApi::SafeRelease( destRef );
		}
	}

	m_stateManager->PopShaderSetup();
	GpuApi::SetupRenderTargets( originalRtSetup );
}


void CRenderInterface::LoadTextureData2D(const GpuApi::TextureRef &destTex, Uint32 mipLevel, Uint32 arraySlice, const GpuApi::Rect* destRect, const void *srcMemory, Uint32 srcPitch)
{
#if defined( RED_PLATFORM_WINPC ) || defined( RED_PLATFORM_DURANGO )
	GpuApi::LoadTextureData2D(destTex, mipLevel, arraySlice, destRect, srcMemory, srcPitch);
#else

	// Allocate some temporary memory via a Staging texture
	// Copy srcMemory into Staging texture
	// Use Compute to copy the Staging data to the correct mip/slice in the destTex texture
	// Add Staging texture to a queue to be deleted once finished with

	const GpuApi::TextureDesc& destDesc = GpuApi::GetTextureDesc(destTex);

	// It seems there is a problem with using the GPU to tile 16x16 BC1 textures - they come out corrupted. so in this case
	// we'd have to tile the memory on CPU first before letting the GPU copy it which is pointless. This code path will
	// use CPU to tile it directly to the destination.
	if (destDesc.format == GpuApi::TEXFMT_BC1 && destRect && ((destRect->right - destRect->left) <= 16 || (destRect->bottom - destRect->top) <= 16))
	{
		GpuApi::LoadTextureData2D(destTex, mipLevel, arraySlice, destRect, srcMemory, srcPitch);
		return;
	}


	// Create staging texture to hold this slice/mip
	GpuApi::TextureDesc desc;
	desc.type = GpuApi::TEXTYPE_2D;
	desc.width			= CalculateTextureMipDimension( destDesc.width, mipLevel, destDesc.format );
	desc.height			= CalculateTextureMipDimension( destDesc.height, mipLevel, destDesc.format );
	desc.initLevels		= 1;
	desc.sliceNum		= 1;
	desc.usage			= GpuApi::TEXUSAGE_Staging;
	desc.format			= destDesc.format;
	desc.msaaLevel		= destDesc.msaaLevel;
	desc.inPlaceType	= GpuApi::INPLACE_None;

	if (destRect)
	{
		desc.width = destRect->right - destRect->left;
		desc.height = destRect->bottom - destRect->top;
	}

	GpuApi::TextureRef stagingTex = GpuApi::CreateTexture(desc, GpuApi::TEXG_Generic);

	Uint32 stagingTexPitch = 0;
	void* stagingTexMem = GpuApi::LockLevel(stagingTex, 0, 0, GpuApi::BLF_Discard, stagingTexPitch);

	if (stagingTexMem)
	{
		// can we do a single memcopy?
		if (stagingTexPitch == srcPitch)
		{
			Uint32 stagingTexSize = GpuApi::CalcTextureSize(desc);
			Red::MemoryCopy(stagingTexMem, srcMemory, stagingTexSize);
		}
		else // no, must do row by row copy
		{
			Uint32 stagingTexSize = GpuApi::CalcTextureSize(stagingTex);
			Uint32 numRows = desc.height;
			if (GpuApi::IsTextureFormatDXT(desc.format))
			{
				numRows /= 4;
			}

			GPUAPI_ASSERT(stagingTexPitch > srcPitch);
			Int32 rowLength = Min(stagingTexPitch, srcPitch);
			for (int y = 0; y < numRows; ++y)
			{
				Uint8* srcRowPtr = (Uint8*)srcMemory + (srcPitch * y);
				Uint8* dstRowPtr = (Uint8*)stagingTexMem + (stagingTexPitch * y);
				Red::MemoryCopy(dstRowPtr, srcRowPtr, rowLength);
			}
		}

	}
	GpuApi::UnlockLevel( stagingTex, 0, 0 );

	GpuApi::Rect srcRect(0, 0, desc.width, desc.height);
	GpuApi::Rect dstRect(destRect ? destRect->left : 0, destRect ? destRect->top : 0, destRect ? destRect->right : desc.width, destRect ? destRect->bottom : desc.height);

	// ok now initiate the copy on GPU
	CopyTextureData(destTex, mipLevel, arraySlice, dstRect, stagingTex, 0, 0, srcRect);

	// finally release the temporary texture which will be destroyed at the end of the next frame
	GpuApi::Release(stagingTex);

#endif
}


void CRenderInterface::LoadBufferData(const GpuApi::BufferRef &destBuffer, Uint32 offset, Uint32 size, const void *srcMemory)
{
#if defined( RED_PLATFORM_WINPC ) || defined( RED_PLATFORM_DURANGO )
	// In DX we have a convenient way to load new data without managing extra temporary memory ourselves.
	GpuApi::LoadBufferData( destBuffer, offset, size, srcMemory );
#else

	// For PS4, create a temporary buffer with the new data, and then copy it over. BCC_* doesn't really matter.
	GpuApi::BufferInitData initData;
	initData.m_buffer = srcMemory;
	initData.m_elementCount = 0;
	GpuApi::BufferRef tempBuffer = GpuApi::CreateBuffer( size, GpuApi::BCC_Vertex, GpuApi::BUT_Immutable, 0, &initData );
	GpuApi::CopyBuffer( destBuffer, offset, tempBuffer, 0, size );

	GpuApi::Release( tempBuffer );
#endif
}



void CRenderInterface::ClearBufferUAV_Uint( const GpuApi::BufferRef& target, const Uint32 value[4])
{
#if defined( RED_PLATFORM_WINPC ) || defined( RED_PLATFORM_DURANGO )
	GpuApi::ClearBufferUAV_Uint(target, value);
#else
	//////////////////////////////////////////////////////////////////////////
	// Clear with compute shader

	m_stateManager->PushShaderSetup();

	struct ComputeData
	{
		Uint32 clearColor[4];
	};

	// Can't have a texture bound as output and UAV at the same time, so we'll clear the current render target.
	GpuApi::SetupBlankRenderTargets();
	GpuApi::BindTextureUAVs( 0, 1, nullptr );
	GpuApi::BindBufferUAV(target, 0);

	ComputeData data;
	Red::MemoryCopy(data.clearColor, value, sizeof(data.clearColor));
	GpuApi::SetComputeShaderConsts( data );

	const GpuApi::BufferDesc& desc = GpuApi::GetBufferDesc( target );

	{
		// Must match ThreadCount in simpleClear.fx
		static Uint32 ThreadCount = 64;
		Uint32 numValues = desc.size / sizeof(Uint32);

		ASSERT(numValues % ThreadCount == 0);

		m_simpleClearBufferCS->Dispatch ((numValues + ThreadCount-1) / ThreadCount, 1, 1);
	}

	GpuApi::BindConstantBuffer( 0, GpuApi::BufferRef::Null(), GpuApi::ComputeShader );
	GpuApi::BindBufferUAV( GpuApi::BufferRef::Null(), 0 );

	m_stateManager->PopShaderSetup();
#endif
}


void CRenderInterface::ClearColorTarget( const GpuApi::TextureRef& target, const Vector& value )
{
	PC_SCOPE_RENDER_LVL1( ClearColorTarget );

#if defined( RED_PLATFORM_WINPC ) || defined( RED_PLATFORM_DURANGO )
	// In DirectX, it's more efficient to clear the target directly. We have an API that does automatic format conversion,
	// so we'll use that.

	// TODO : Could write memset-esque GNM version for targets with simple formats?

	GpuApi::ClearColorTarget( target, value.A );
#else

	if (GpuApi::ClearColorTarget( target, value.A ))
	{
		return;
	}
	
	GpuApi::RenderTargetSetup rtSetup = GpuApi::GetRenderTargetSetup();

	// When Gpu clear is not possible, we fallback to either Pixel or Compute shader version.
	static Bool ClearFallbackCompute = false;

	m_stateManager->PushShaderSetup();

	// Can only use compute for non-MSAA targets
	if ( ClearFallbackCompute && GpuApi::GetTextureDesc( target ).msaaLevel <= 1 )
	{
		//////////////////////////////////////////////////////////////////////////
		// Clear with compute shader
		struct ComputeData
		{
			Vector clearColor;
		};

		// Can't have a texture bound as output and UAV at the same time, so we'll clear the current render target.
		GpuApi::SetupBlankRenderTargets();

		GpuApi::BindTextureUAVs( 0, 1, &target );

		ComputeData data;
		data.clearColor = value;
		GpuApi::SetComputeShaderConsts( data );

		const GpuApi::TextureDesc& desc = GpuApi::GetTextureDesc( target );

		{
			// Must match ThreadCount in simpleClear.fx
			static Uint32 ThreadCount = 16;
			m_simpleClearCS->Dispatch( ( desc.width + (ThreadCount-1) ) / ThreadCount, ( desc.height + (ThreadCount-1) ) / ThreadCount, 1);
		}

		GpuApi::BindConstantBuffer( 0, GpuApi::BufferRef::Null(), GpuApi::ComputeShader );
		GpuApi::BindTextureUAVs( 0, 1, nullptr );
	}
	else
	{
		//////////////////////////////////////////////////////////////////////////
		// Clear with fullscreen quad

		GpuApi::RenderTargetSetup rtSetup;
		rtSetup.SetColorTarget( 0, target );
		rtSetup.SetViewportFromTarget( target );
		GpuApi::SetupRenderTargets( rtSetup );

		// Clear by drawing a quad over the target.
		CGpuApiScopedDrawContext context( GpuApi::DRAWCONTEXT_PostProcSet );
		GpuApi::eTextureFormat targetFmt = GpuApi::GetTextureDesc(target).format;
		if (targetFmt == GpuApi::TEXFMT_Float_R32 || targetFmt == GpuApi::TEXFMT_Float_R32G32 || targetFmt == GpuApi::TEXFMT_Float_R32G32B32A32)
			m_simpleClear32->Bind();
		else
			m_simpleClear->Bind();

		// Make sure we don't have any extra shaders.
		m_stateManager->SetShader( GpuApi::ShaderRef::Null(), RST_GeometryShader );
		m_stateManager->SetShader( GpuApi::ShaderRef::Null(), RST_HullShader );
		m_stateManager->SetShader( GpuApi::ShaderRef::Null(), RST_DomainShader );

		m_stateManager->SetPixelConst( PSC_Custom_0, value );

		GpuApi::DrawPrimitiveNoBuffers( GpuApi::PRIMTYPE_TriangleStrip, 2 );
	}

	m_stateManager->PopShaderSetup();

	GpuApi::SetupRenderTargets( rtSetup );
#endif
}

void CRenderInterface::ClearColorTarget( const Vector& value )
{
	GpuApi::RenderTargetSetup rtSetup = GpuApi::GetRenderTargetSetup();
	if ( rtSetup.numColorTargets == 0 )
	{
		return;
	}

	for ( Uint32 i = 0; i < rtSetup.numColorTargets; ++i )
	{
		if ( !rtSetup.colorTargets[i].isNull() )
		{
			ClearColorTarget( rtSetup.colorTargets[i], value );
		}
	}

	// Restore old setup
	GpuApi::SetupRenderTargets( rtSetup );
}

void CRenderInterface::ClearDepthTarget( const GpuApi::TextureRef& target, Float depthValue, Int32 slice )
{
#if defined( RED_PLATFORM_WINPC ) || defined( RED_PLATFORM_DURANGO )
	GpuApi::ClearDepthTarget( target, depthValue, slice );
#else

	PC_SCOPE_RENDER_LVL1( ClearDepthTarget );

	if (GpuApi::ClearDepthTarget( target, depthValue, slice ))
	{
		return;
	}


	// Bind target
	GpuApi::RenderTargetSetup rtSetup;
	rtSetup.SetNullColorTarget();
	rtSetup.SetDepthStencilTarget( target, slice );
	rtSetup.SetViewportFromTarget( target );
	GpuApi::SetupRenderTargets( rtSetup );

	// Clear by drawing a quad over the target.
	CGpuApiScopedDrawContext context( GpuApi::DRAWCONTEXT_PostProcSet_DepthWrite );
	m_simpleClearDepth->Bind();

	// Make sure we don't have any extra shaders.
	m_stateManager->SetShader( GpuApi::ShaderRef::Null(), RST_PixelShader );
	m_stateManager->SetShader( GpuApi::ShaderRef::Null(), RST_GeometryShader );
	m_stateManager->SetShader( GpuApi::ShaderRef::Null(), RST_HullShader );
	m_stateManager->SetShader( GpuApi::ShaderRef::Null(), RST_DomainShader );

	m_stateManager->SetVertexConst( VSC_Custom_0, Vector( depthValue, 0, 0, 0 ) );

	GpuApi::DrawPrimitiveNoBuffers( GpuApi::PRIMTYPE_TriangleStrip, 2 );

#endif
}

void CRenderInterface::ClearDepthTarget( Float depthValue )
{
	GpuApi::RenderTargetSetup rtSetup = GpuApi::GetRenderTargetSetup();

	if ( !rtSetup.depthTarget )
	{
		return;
	}

	ClearDepthTarget( rtSetup.depthTarget, depthValue, rtSetup.depthTargetSlice );
		
	// Restore old setup
	GpuApi::SetupRenderTargets( rtSetup );
}

void CRenderInterface::ClearStencilTarget( const GpuApi::TextureRef& target, Uint8 stencilValue, Int32 slice )
{
#if defined( RED_PLATFORM_WINPC ) || defined( RED_PLATFORM_DURANGO )
	GpuApi::ClearStencilTarget( target, stencilValue, slice );
#else

	PC_SCOPE_RENDER_LVL1( ClearStencilTarget );

	// Bind target
	GpuApi::RenderTargetSetup rtSetup;
	rtSetup.SetNullColorTarget();
	rtSetup.SetDepthStencilTarget( target, slice );
	rtSetup.SetViewportFromTarget( target );
	GpuApi::SetupRenderTargets( rtSetup );

	// Clear by drawing a quad over the target.
	CGpuApiScopedDrawContext context( GpuApi::DRAWCONTEXT_PostProcNoColor_SetStencilFull, stencilValue );
	m_simpleClearDepth->Bind();

	// Make sure we don't have any extra shaders.
	m_stateManager->SetShader( GpuApi::ShaderRef::Null(), RST_PixelShader );
	m_stateManager->SetShader( GpuApi::ShaderRef::Null(), RST_GeometryShader );
	m_stateManager->SetShader( GpuApi::ShaderRef::Null(), RST_HullShader );
	m_stateManager->SetShader( GpuApi::ShaderRef::Null(), RST_DomainShader );

	GpuApi::DrawPrimitiveNoBuffers( GpuApi::PRIMTYPE_TriangleStrip, 2 );

#endif
}

void CRenderInterface::ClearStencilTarget( Uint8 stencilValue )
{
	GpuApi::RenderTargetSetup rtSetup = GpuApi::GetRenderTargetSetup();

	if ( !rtSetup.depthTarget )
	{
		return;
	}

	ClearStencilTarget( rtSetup.depthTarget, stencilValue, rtSetup.depthTargetSlice );

	// Restore old setup
	GpuApi::SetupRenderTargets( rtSetup );
}

void CRenderInterface::ClearDepthStencilTarget( const GpuApi::TextureRef& target, Float depthValue, Uint8 stencilValue, Int32 slice )
{
#if defined( RED_PLATFORM_WINPC ) || defined( RED_PLATFORM_DURANGO )
	GpuApi::ClearDepthStencilTarget( target, depthValue, stencilValue, slice );
#else

	if (GpuApi::ClearDepthStencilTarget(target, depthValue, stencilValue, slice))
	{
		return;
	}

	PC_SCOPE_RENDER_LVL1( ClearDepthStencilTarget );

	// Bind target
	GpuApi::RenderTargetSetup rtSetup;
	rtSetup.SetNullColorTarget();
	rtSetup.SetDepthStencilTarget( target, slice );
	rtSetup.SetViewportFromTarget( target );
	GpuApi::SetupRenderTargets( rtSetup );

	// Clear by drawing a quad over the target.
	// TODO : Draw context with stencil set
	CGpuApiScopedDrawContext context( GpuApi::DRAWCONTEXT_NoColor_DepthStencilSet, stencilValue );
	m_simpleClearDepth->Bind();

	// Make sure we don't have any extra shaders.
	m_stateManager->SetShader( GpuApi::ShaderRef::Null(), RST_PixelShader );
	m_stateManager->SetShader( GpuApi::ShaderRef::Null(), RST_GeometryShader );
	m_stateManager->SetShader( GpuApi::ShaderRef::Null(), RST_HullShader );
	m_stateManager->SetShader( GpuApi::ShaderRef::Null(), RST_DomainShader );

	m_stateManager->SetVertexConst( VSC_Custom_0, Vector( depthValue, 0, 0, 0 ) );

	GpuApi::DrawPrimitiveNoBuffers( GpuApi::PRIMTYPE_TriangleStrip, 2 );

#endif
}

void CRenderInterface::ClearDepthStencilTarget( Float depthValue, Uint8 stencilValue )
{
	GpuApi::RenderTargetSetup rtSetup = GpuApi::GetRenderTargetSetup();

	if ( !rtSetup.depthTarget )
	{
		return;
	}

	ClearDepthStencilTarget( rtSetup.depthTarget, depthValue, stencilValue, rtSetup.depthTargetSlice );

	// Restore old setup
	GpuApi::SetupRenderTargets( rtSetup );
}



void CRenderInterface::GenerateMipmaps(const GpuApi::TextureRef& texture)
{
	if (GpuApi::GenerateMipmaps(texture))
		return;

	// fallback to manual mip map generation method
	PC_SCOPE_RENDER_LVL1(GenerateMipmaps);

	GpuApi::RenderTargetSetup rtSetupOld;
	rtSetupOld = GpuApi::GetRenderTargetSetup();

	CGpuApiScopedDrawContext scopedDrawContext( GpuApi::DRAWCONTEXT_PostProcSet );		

	const GpuApi::TextureDesc& desc	= GpuApi::GetTextureDesc( texture );
	GpuApi::SetSamplerStatePreset( 0, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip, GpuApi::PixelShader );
	m_stateManager->SetVertexConst( VSC_Custom_0, Vector(0.0f, 0.0f, 1.0f, 1.0f));
	switch (desc.format)
	{
	case GpuApi::TEXFMT_Float_R32G32B32A32:
		m_simpleCopy32_RGBA->Bind();
		break;
	case GpuApi::TEXFMT_Float_R32G32:
		m_simpleCopy32_RG->Bind();
		break;
	case GpuApi::TEXFMT_Float_R32:
		m_simpleCopy32_R->Bind();
		break;
	default:
		m_simpleCopy->Bind();
		break;
	}

	for (Int16 mip_i = 1; mip_i < desc.initLevels; ++mip_i)
	{
		GpuApi::RenderTargetSetup rtSetup;
		rtSetup.SetColorTarget(0, texture, mip_i);
		rtSetup.SetViewport(desc.width >> mip_i, desc.height >> mip_i);
		GpuApi::SetupRenderTargets(rtSetup);

		GpuApi::BindTextureMipLevel(0, texture, 0, mip_i-1, GpuApi::PixelShader);

		GpuApi::DrawPrimitiveNoBuffers( GpuApi::PRIMTYPE_TriangleStrip, 2 );
	}

	GpuApi::SetupRenderTargets(rtSetupOld);
}


Float CRenderInterface::GetLastGPUFrameDuration()
{
	return m_lastGPUFrameTime;
}

//#ifndef NO_RUNTIME_MATERIAL_COMPILATION
#ifndef NO_ASYNCHRONOUS_MATERIALS
void CRenderInterface::TickRecompilingMaterials()
{
	PC_SCOPE( TickRecompilingMaterials );

	Red::Threads::CScopedLock< Red::Threads::CMutex > mutex( CRenderMaterial::st_recompileMutex );

	for ( Uint32 i = 0; i < CRenderMaterial::st_currentlyRecompilingMaterials.Size(); ++i )
	{
		CRenderMaterial* material = CRenderMaterial::st_currentlyRecompilingMaterials[ i ];
		if ( material->ClearCompilationJobs() )
		{
			CRenderMaterial::st_currentlyRecompilingMaterials.Remove( material );
			material->Release();
		}
	}
}

void CRenderInterface::FlushRecompilingMaterials()
{
	PC_SCOPE( FlushRecompilingMaterials );

	Red::Threads::CScopedLock< Red::Threads::CMutex > mutex( CRenderMaterial::st_recompileMutex );

	for ( Uint32 i = 0; i < CRenderMaterial::st_currentlyRecompilingMaterials.Size(); ++i )
	{
		CRenderMaterial* material = CRenderMaterial::st_currentlyRecompilingMaterials[ i ];
		while ( !material->ClearCompilationJobs() );
		CRenderMaterial::st_currentlyRecompilingMaterials.Remove( material );
		material->Release();		
	}
}

#endif // NO_ASYNCHRONOUS_MATERIALS
//#endif //NO_RUNTIME_MATERIAL_COMPILATION


void CRenderInterface::SuppressSceneRendering( Bool suppress )
{
	if ( suppress )
	{
		++m_suppressRendering;
	}
	else
	{
		RED_ASSERT( m_suppressRendering > 0, TXT("SuppressSceneRendering called to turn off suppression when it isn't suppressed!") );
		if ( m_suppressRendering > 0 )
		{
			--m_suppressRendering;
		}
	}
}


void CRenderInterface::CastMaterialToTexture( Uint32 elements, Uint32 texWidth, Uint32 texHeight, IMaterial* material, Float distance )
{
#ifdef RED_PLATFORM_CONSOLE
	RED_LOG_ERROR( RED_LOG_CHANNEL( GpuApi ), TXT("Unsupported on consoles! For use in the editor only." ) );
	return;
#endif

	// Extract render material resource
	CRenderMaterial* renderMaterial = nullptr;
	CRenderMaterialParameters* renderMaterialParams = nullptr;
	ExtractRenderResource( material, renderMaterialParams );
	ExtractRenderResource( material->GetMaterialDefinition(), renderMaterial );
	if( !renderMaterial || !renderMaterialParams )
	{
		RED_HALT( "Couldn't extract render material resource for the mesh." );
		return;
	}

	GetRenderer()->GetStateManager().SetVertexConst( VSC_Custom_0, Vector( (Float)texWidth, (Float)texHeight, 0, 0 ) );

	// Set draw context
	CGpuApiScopedDrawContext scopedDrawContext( GpuApi::DRAWCONTEXT_PostProcAdd );

	RenderingContext context;
	context.m_pass = RP_GBuffer;
	MaterialRenderingContext materialContext(context);
	materialContext.m_vertexFactory = EMaterialVertexFactory::MVF_MeshStatic;
	materialContext.m_hasExtraStreams = true;
	materialContext.m_proxyMaterialCast = true;

	if( renderMaterial->Bind( materialContext, renderMaterialParams, distance ) )
	{
		GpuApi::DrawPrimitiveNoBuffers( GpuApi::PRIMTYPE_TriangleList, elements );
	}

	renderMaterial->Release();
	renderMaterialParams->Release();
}


CHairWorksRefreshListener::CHairWorksRefreshListener()
	: m_refreshRequest( false )
{
}

void CHairWorksRefreshListener::OnRequestRefresh(const CName& eventName)
{
	m_refreshRequest.SetValue( true );
}

Bool CHairWorksRefreshListener::GetAndClearRefresh()
{
	return m_refreshRequest.CompareExchange( false, true );
}

namespace Config
{
	TConfigVar< Int32, Validation::IntRange< 0, 63 > >		cvTreesLateAllocVSLimit			( "Rendering/LateVSAlloc", "Trees", 50 );
	TConfigVar< Int32, Validation::IntRange< 0, 63 > >		cvGrassLateAllocVSLimit			( "Rendering/LateVSAlloc", "Grass", 28 );
	TConfigVar< Int32, Validation::IntRange< 0, 63 > >		cvTerrainLateAllocVSLimit		( "Rendering/LateVSAlloc", "Terrain", 0 );
	TConfigVar< Int32, Validation::IntRange< 0, 63 > >		cvHairLateAllocVSLimit			( "Rendering/LateVSAlloc", "Hair", 28 );
	TConfigVar< Int32, Validation::IntRange< 0, 63 > >		cvParticlesLateAllocVSLimit		( "Rendering/LateVSAlloc", "Particles", 28 );
}
