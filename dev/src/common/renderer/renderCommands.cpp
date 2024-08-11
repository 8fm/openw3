/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "../engine/renderCommands.h"

#include "guiRenderSystemScaleform.h"
#include "renderTextureArray.h"
#include "renderCube.h"
#include "renderViewport.h"

#include "gameplayFx.h"
#include "gameplayFXCatView.h"
#include "gameplayFxDrunk.h"
#include "gameplayFxFocusMode.h"
#include "gameplayFxOutline.h"
#include "gameplayFxSepia.h"
#include "gameplayFxSurface.h"

#include "renderElementMap.h"
#include "renderSequenceGrabber.h"
#include "renderSkinningData.h"
#include "renderSwarmData.h"
#include "renderProxyStripe.h"
#include "renderProxyFur.h"
#include "renderProxyParticles.h"
#include "renderProxySpeedTree.h"
#include "renderProxyWater.h"
#include "renderProxyFlare.h"
#include "renderProxyTerrain.h"
#include "renderProxyApex.h"
#include "renderProxyMesh.h"
#include "renderProxyLight.h"
#include "renderProxySpotLight.h"
#include "renderProxyPointLight.h"
#include "renderProxySwarm.h"
#include "renderElementMeshChunk.h"
#include "renderScene.h"
#include "renderFence.h"
#include "renderTerrainUpdateData.h"
#include "renderGrassUpdateData.h"
#include "renderEntityGroup.h"
#include "renderPostProcess.h"
#include "renderMeshBatcher.h"
#include "renderThread.h"
#include "renderMesh.h"
#include "renderMaterial.h"
#include "renderTerrainShadows.h"
#include "renderEnvProbe.h"
#include "renderProxyMorphedMesh.h"
#include "renderSkybox.h"
#include "renderTextureStreaming.h"
#include "../engine/foliageInstance.h"
#include "renderVideoPlayer.h"
#include "renderVideo.h"
#include "renderLoadingScreen.h"
#include "renderLoadingScreenFence.h"
#include "renderLoadingOverlay.h"
#include "renderLoadingBlur.h"
#include "renderEnvProbeManager.h"
#include "renderTextureStreamRequest.h"
#include "renderScaleformTexture.h"
#include "renderProxyDestructionMesh.h"

#ifndef NO_DEBUG_PAGES
extern volatile Float GLastRenderFrameTime;
#endif // !NO_DEBUG_PAGES

void CRenderCommand_RenderScene::Execute()
{
	PC_SCOPE_RENDER_LVL1( CMDRenderScene );

	// Do not render if non interactive rendering is on
	if ( GRenderThread->IsNonInteractiveRenderingEnabled() 
#ifndef NO_EDITOR
		|| GRenderThread->IsSuspended()
#endif
	   )
	{
		return;
	}

	CRenderSceneEx* scene = static_cast< CRenderSceneEx* >( m_scene );

	// Measure the time needed to render scene
	RED_PROFILING_TIMER( renderTimer );

#ifndef NO_EDITOR
	if ( m_forcePrefetch )
	{
		GetRenderer()->SetupInitialFrame( m_frame, scene, true, false );

		IRenderFramePrefetch* prefetch = GRender->CreateRenderFramePrefetch( m_frame, scene );
		GetRenderer()->EnqueueFramePrefetch( prefetch );

		while ( !prefetch->IsFinished() )
		{
			CRenderFramePrefetch* nextPendingPrefetch = GetRenderer()->GetNextPendingFramePrefetch();
			nextPendingPrefetch->AddRef();
			GetRenderer()->RenderFrame( m_frame, scene );
			
			while ( !nextPendingPrefetch->IsFinished() )
			{
				GetRenderer()->GetTextureStreamingManager()->TickDuringSuppressedRendering();
			}

			nextPendingPrefetch->Release();
		}

		prefetch->Release();
	}
#endif

	// Render frame
	GetRenderer()->RenderFrame( m_frame, scene );

	// Flush counters
#ifndef NO_DEBUG_PAGES
	IRenderDebugCounter::FlushAll();

	// Get the results
	RED_PROFILING_TIMER_GET_DELTA( GLastRenderFrameTime, (Float)renderTimer );
#endif

	// Write next frame
	if ( GScreenshotSequence && scene )
	{
		PC_SCOPE_RENDER_LVL1( GrabScreenshot );
		GScreenshotSequence->GrabScreenshot( m_frame, scene, false );
	}
}

void CRenderCommand_NewFrame::Execute()
{
	PC_SCOPE( CMDNewFrame );
	GetRenderer()->NewFrame();
}

void CRenderCommand_EndFrame::Execute()
{
	PC_SCOPE( CMDEndFrame );
	GetRenderer()->EndFrame();
}

void CRenderCommand_CancelTextureStreaming::Execute()
{
	PC_SCOPE_RENDER_LVL1( CMDCancelTextureStreaming );
	GetRenderer()->GetTextureStreamingManager()->CancelTextureStreaming(m_flushOnlyUnused);
}

void CRenderCommand_ShutdownTextureStreaming::Execute()
{
	PC_SCOPE_RENDER_LVL1( CMDStopAllTextureStreaming );
	GetRenderer()->GetTextureStreamingManager()->StopFurtherStreaming();
}

void CRenderCommand_StartTextureStreamRequest::Execute()
{
	if ( m_request != nullptr )
	{
		static_cast< CRenderTextureStreamRequest* >( m_request )->Start();
	}
}

void CRenderCommand_CancelTextureStreamRequest::Execute()
{
	if ( m_request != nullptr )
	{
		static_cast< CRenderTextureStreamRequest* >( m_request )->Cancel();
	}
}

void CRenderCommand_StartFramePrefetch::Execute()
{
	if ( m_prefetch != nullptr )
	{
		GetRenderer()->EnqueueFramePrefetch( m_prefetch );
	}
}


void CRenderCommand_PrepareInitialTextureStreaming::Execute()
{
	if ( m_resetDistances )
	{
		GetRenderer()->ResetTextureStreamingDistances( static_cast< CRenderSceneEx* >( m_scene ) );
	}
	GetRenderer()->GetTextureStreamingManager()->SetPrefetchMipDrop( false );
}

void CRenderCommand_FinishInitialTextureStreaming::Execute()
{
	GetRenderer()->GetTextureStreamingManager()->SetPrefetchMipDrop( true );
}

void CRenderCommand_SetupEnvProbesPrefetch::Execute()
{
	if ( !(m_scene && static_cast< CRenderSceneEx* >( m_scene )->IsWorldScene()) )
	{
		return;
	}

	CRenderEnvProbeManager *envprobeManager = GetRenderer()->GetEnvProbeManager();

	RED_FATAL_ASSERT( envprobeManager, "Expects envprobe manager to be present" );
	if ( envprobeManager )
	{
		envprobeManager->SetupPrefetch( m_position );
	}
}

void CRenderCommand_TickTextureStreaming::Execute()
{
	if ( m_flushOnePrefetch )
	{
		GetRenderer()->FlushOneFramePrefetch();
	}

	GetRenderer()->GetTextureStreamingManager()->TickDuringSuppressedRendering();
}

void CRenderCommand_ToggleCinematicMode::Execute()
{
	GetRenderer()->GetTextureStreamingManager()->EnableCinematicMode( m_cinematicMode );
}

void CRenderCommand_TextureCacheAttached::Execute()
{
	GetRenderer()->GetTextureStreamingManager()->OnNewTextureCacheAttached();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void CRenderCommand_ShowLoadingScreenBlur::Execute()
{
	GRenderThread->GetLoadingScreenBlur()->Init( GGame->GetViewport() , m_blurScale , m_timeScale , m_useFallback );
}

void CRenderCommand_HideLoadingScreenBlur::Execute()
{
	GRenderThread->GetLoadingScreenBlur()->Deinit( m_fadeTime );
}

/////////////////////////////////////////////////////////////////////////////////////////////

void CRenderCommand_SetLoadingScreenFence::Execute()
{
	PC_SCOPE( CMDSetLoadingScreenFence );
	GRenderThread->GetLoadingScreen()->SetFence( static_cast< CRenderLoadingScreenFence* >( m_fence ), m_fadeInTime, m_hideAtStart );
}

/////////////////////////////////////////////////////////////////////////////////////////////

void CRenderCommand_FadeOutLoadingScreen::Execute()
{
	PC_SCOPE( UpdateLoadingScreenFenceState );
	GRenderThread->GetLoadingScreen()->FadeOut( static_cast< CRenderLoadingScreenFence* >( m_fence ), m_fadeOutTime );
}

/////////////////////////////////////////////////////////////////////////////////////////////

void CRenderCommand_SetVideoFlash::Execute()
{
	PC_SCOPE( SetVideoFlash );

	GRenderThread->GetVideoPlayer()->InitWithFlash( m_videoFlash );
}

namespace ScaleformVideoHelpers
{
	extern void SetVideoClearRGB( const Color& rgb );
	extern const Color& GetVideoClearRGB();
}

void CRenderCommand_W3HackSetVideoClearRGB::Execute()
{
	ScaleformVideoHelpers::SetVideoClearRGB( m_rgb );
}

void CRenderCommand_W3HackShowVideoBackground::Execute()
{
	const Color& rgb = ScaleformVideoHelpers::GetVideoClearRGB();
	GRenderThread->GetVideoPlayer()->ShowBackground( rgb );
}

void CRenderCommand_W3HackHideVideoBackground::Execute()
{
	GRenderThread->GetVideoPlayer()->HideBackground();
}

void CRenderCommand_SetVideoMasterVolume::Execute()
{
	PC_SCOPE( SetVideoMasterVolume );

	GRenderThread->GetVideoPlayer()->SetMasterVolumePercent( m_volumePercent );
}

void CRenderCommand_SetVideoVoiceVolume::Execute()
{
	PC_SCOPE( SetVideoVoiceVolume );

	GRenderThread->GetVideoPlayer()->SetVoiceVolumePercent( m_volumePercent );
}

void CRenderCommand_SetVideoEffectsVolume::Execute()
{
	PC_SCOPE( SetVideoEffectsVolume );

	GRenderThread->GetVideoPlayer()->SetEffectsVolumePercent( m_volumePercent );
}

void CRenderCommand_SetLoadingOverlayFlash::Execute()
{
	PC_SCOPE( SetLoadingOverlayFlash );

	GRenderThread->GetLoadingOverlay()->InitWithFlash( m_loadingOverlayFlash );
}

void CRenderCommand_ToggleLoadingOverlay::Execute()
{
	PC_SCOPE( ToggleLoadingOverlay );

	CRenderLoadingOverlay* loadingOVerlay = GRenderThread->GetLoadingOverlay();
	if ( m_visible )
	{
		loadingOVerlay->FadeIn( m_caption );
	}
	else
	{
		loadingOVerlay->FadeOut( m_caption );
	}
}

void CRenderCommand_ToggleVideoPause::Execute()
{
	PC_SCOPE( ToggleVideoPause );

	GRenderThread->GetVideoPlayer()->ToggleVideoPause( m_pause );
}

void CRenderCommand_PlayVideo::Execute()
{
	PC_SCOPE( PlayVideo );

	GRenderThread->GetVideoPlayer()->PlayVideo( static_cast< CRenderVideo* >( m_renderVideo ), m_threadIndex );
}

void CRenderCommand_PlayLoadingScreenVideo::Execute()
{
	PC_SCOPE( PlayLoadingScreenVideo );

	GRenderThread->GetLoadingScreen()->PlayVideo( static_cast< CRenderVideo* >( m_renderVideo ), 
												  static_cast< CRenderLoadingScreenFence* >( m_fence ) );
}

void CRenderCommand_CancelVideo::Execute()
{
	PC_SCOPE( CancelVideo );

	GRenderThread->GetVideoPlayer()->CancelVideo( static_cast< CRenderVideo* >( m_renderVideo ) );
}

void CRenderCommand_ToggleLoadingVideoSkip::Execute()
{
	PC_SCOPE( PlayLoadingVideo );

	GRenderThread->GetLoadingScreen()->ToggleVideoSkip( static_cast< CRenderLoadingScreenFence* >( m_fence ), m_enabled );
}

void CRenderCommand_SetLoadingPCInput::Execute()
{
	PC_SCOPE( PlayLoadingVideo );
	
	GRenderThread->GetLoadingScreen()->SetPCInput( static_cast< CRenderLoadingScreenFence* >( m_fence ), m_enabled );
}

void CRenderCommand_SetExpansionsAvailable::Execute()
{
	PC_SCOPE( SetExpansionsAvailable );

	GRenderThread->GetLoadingScreen()->SetExpansionsAvailable( static_cast< CRenderLoadingScreenFence* >( m_fence ), m_ep1, m_ep2 );
}

/////////////////////////////////////////////////////////////////////////////////////////////

void CRenderCommand_ForceUITextureStreaming::Execute()
{
	PC_SCOPE( ForceUITextureStreaming );
	if ( GRender->GetRenderScaleform() && GRender->GetRenderScaleform()->GetTextureManager() )
	{
		CRenderScaleformTextureManager* scaleformTextureManager = static_cast<CRenderScaleformTextureManager*>( GRender->GetRenderScaleform()->GetTextureManager() );
		scaleformTextureManager->ForceTextureStreaming( m_pinTextures );
		scaleformTextureManager->ProcessTextures(); // process now, otherwise it's a race against rendering a frame when checking for completion after a GRender->Flush
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

void CRenderCommand_UpdateProgressStatus::Execute()
{
	PC_SCOPE( CMDUpdateProgressStatus );
	//GRenderThread->SetLoadingStatus( m_status );
}

void CRenderCommand_UpdateProgress::Execute()
{
	PC_SCOPE( CMDUpdateProgress );
	GRenderThread->GetLoadingScreen()->SetProgress( m_progress );
}

void CRenderCommand_AddProxyToScene::Execute()
{
	PC_SCOPE( CMDAddProxyToScene );
	CRenderSceneEx* scene = static_cast< CRenderSceneEx* >( m_scene );
	IRenderProxyBase* proxy = static_cast< IRenderProxyBase* >( m_proxy );
	scene->AddProxy( proxy );
}

void CRenderCommand_AddStripeToScene::Execute()
{
	PC_SCOPE( CMDAddStripeToScene );
	CRenderSceneEx* scene = static_cast< CRenderSceneEx* >( m_scene );
	scene->AddStripe( m_proxy );
}

void CRenderCommand_AddFurToScene::Execute()
{
	PC_SCOPE( CMDAddFurToScene );
	CRenderSceneEx* scene = static_cast< CRenderSceneEx* >( m_scene );
	scene->AddFur( m_proxy );
}

void CRenderCommand_UpdateFurParams::Execute()
{
	PC_SCOPE( CMDUpdateFurWind );
	CRenderProxy_Fur* proxy = static_cast< CRenderProxy_Fur* >( m_proxy );
	proxy->UpdateFurParams( m_wind, m_wetness );
}

#ifndef NO_EDITOR
#ifdef USE_NVIDIA_FUR
void CRenderCommand_EditorSetFurParams::Execute()
{
	PC_SCOPE( CMDEditorSetFurParams );
	CRenderProxy_Fur* proxy = static_cast< CRenderProxy_Fur* >( m_proxy );
	proxy->EditorSetFurParams( m_params, m_materialIndex );
}
#endif // USE_NVIDIA_FUR
#endif //NO_EDITOR

void CRenderCommand_RemoveProxyFromScene::Execute()
{
	PC_SCOPE( CMDRemoveProxyFromScene );
	CRenderSceneEx* scene = static_cast< CRenderSceneEx* >( m_scene );
	IRenderProxyBase* proxy = static_cast< IRenderProxyBase* >( m_proxy );
	scene->RemoveProxy( proxy );
}

void CRenderCommand_RemoveStripeFromScene::Execute()
{
	PC_SCOPE( CMDAddStripeToScene );
	CRenderSceneEx* scene = static_cast< CRenderSceneEx* >( m_scene );
	scene->RemoveStripe( m_proxy );
}

void CRenderCommand_RemoveFurFromScene::Execute()
{
	PC_SCOPE( CMDRemoveFurFromScene );
	CRenderSceneEx* scene = static_cast< CRenderSceneEx* >( m_scene );
	scene->RemoveFur( m_proxy );
}

#ifdef USE_UMBRA
void CRenderCommand_UploadOcclusionDataToScene::Execute()
{
	PC_SCOPE( CMDUploadOcclusionDataToScene );
	CRenderSceneEx* scene = static_cast< CRenderSceneEx* >( m_renderScene );
	CRenderOcclusionData* occlusionData = static_cast< CRenderOcclusionData* >( m_occlusionData );
	scene->UploadOcclusionData( occlusionData );
	scene->GetRenderElementMap()->BuildRemapArray( occlusionData->GetTomeCollection(), m_remapTable, m_objectIDToIndexMap );
}

void CRenderCommand_SetDoorState::Execute()
{
	PC_SCOPE( CMDSetDoorState );
	CRenderSceneEx* scene = static_cast< CRenderSceneEx* >( m_scene );
	scene->SetDoorState( m_objectId, m_opened );
}

void CRenderCommand_SetCutsceneModeForGates::Execute()
{
	PC_SCOPE( CMDSetCutsceneModeForGates );
	CRenderSceneEx* scene = static_cast< CRenderSceneEx* >( m_scene );
	scene->SetCutsceneModeForGates( m_isCutscene );
}

void CRenderCommand_ClearOcclusionData::Execute()
{
	PC_SCOPE( CMDClearOcclusionData );
	CRenderSceneEx* scene = static_cast< CRenderSceneEx* >( m_scene );
	scene->UploadOcclusionData( nullptr );
	TVisibleChunksIndices emptyRemapArray;
	TObjectIDToIndexMap emptyObjectIDToIndexMap;
	scene->GetRenderElementMap()->BuildRemapArray( nullptr, emptyRemapArray, emptyObjectIDToIndexMap );
}

void CRenderCommand_UploadObjectCache::Execute()
{
	PC_SCOPE( CMDUploadObjectCache );
	CRenderSceneEx* scene = static_cast< CRenderSceneEx* >( m_scene );
	RED_FATAL_ASSERT( scene && scene->GetRenderElementMap(), "Scene invalid or renderElementMap invalid" );
	scene->GetRenderElementMap()->SetObjectCache( m_objectCache );	
}

void CRenderCommand_SetValidityOfOcclusionData::Execute()
{
	PC_SCOPE( CMDSetValidityOfOcclusionData );
	CRenderSceneEx* scene = static_cast< CRenderSceneEx* >( m_scene );
	scene->SetTomeCollectionValid( m_isDataValid );
}

void CRenderCommand_UpdateQueryThreshold::Execute()
{
	PC_SCOPE( CMDUpdateQueryThreshold );
	CRenderSceneEx* scene = static_cast< CRenderSceneEx* >( m_scene );
	scene->UpdateQueryThresholdParameter( m_value );
}

#ifndef NO_EDITOR
void CRenderCommand_DumpVisibleMeshes::Execute()
{
	PC_SCOPE( CMDDumpVisibleMeshes );
	CRenderSceneEx* scene = static_cast< CRenderSceneEx* >( m_scene );
	scene->DumpVisibleMeshes( m_map, m_path );
}
#endif

void CRenderCommand_PerformVisibilityQueries::Execute()
{
	PC_SCOPE( CMDPerformOcclusionQueries );
	CRenderSceneEx* scene = static_cast< CRenderSceneEx* >( m_scene );
	scene->PerformVisibilityQueriesBatch( m_queryList );
}

#endif // USE_UMBRA

void CRenderCommand_RelinkProxy::Execute()
{
	PC_SCOPE( CMDRelinkProxy );
	IRenderProxyBase* proxy = static_cast< IRenderProxyBase* >( m_proxy );
	proxy->Relink( m_boundingBox, m_localToWorld );
}

void CRenderCommand_BatchSkinAndRelinkProxies::Execute()
{
	PC_SCOPE( CMDBatchSkinAndRelinkProxies );

	for ( Uint8 i = 0; i < m_proxyCount; ++i )
	{
		IRenderProxyBase* proxy = static_cast< IRenderProxyBase* >( m_proxies[i] );
		CRenderSkinningData* skinningData = static_cast<CRenderSkinningData*>( m_skinningData[i] );

		if ( skinningData != nullptr && proxy != nullptr )
		{
			ERenderProxyType rpType = proxy->GetType();

			if ( rpType == RPT_Mesh )
			{
				CRenderProxy_Mesh* meshProxy = static_cast< CRenderProxy_Mesh* >( proxy );
				if ( meshProxy->GetSkinningData() == skinningData )
				{
					skinningData->AdvanceRead();
				}
				else
				{
					meshProxy->SetSkinningData( skinningData );
				}
			}
			else if ( rpType == RPT_Fur )
			{
				CRenderProxy_Fur* furProxy = static_cast< CRenderProxy_Fur* >( proxy );

				if ( furProxy->GetSkinningData() == skinningData )
				{
					skinningData->AdvanceRead();
				}
				else
				{
					furProxy->SetSkinningData( skinningData );
				}
			}
			else
			{
				RED_HALT("Unhandled proxy type");
			}
		}

		if ( m_relinkInfos[i].m_hasBBox ) 
		{
			proxy->Relink( m_relinkInfos[i].m_bbox, m_relinkInfos[i].m_transform );
		}
		else
		{
			if ( m_relinkInfos[i].m_transform != proxy->GetLocalToWorld() )
			{
				proxy->RelinkTransformOnly( m_relinkInfos[i].m_transform );
			}
		}
	}
}

void CRenderCommand_EnvProbeParamsChanged::Execute()
{
	PC_SCOPE( CMDEnvProbeTransformationChanged );
	CRenderEnvProbe *probe = static_cast< CRenderEnvProbe* >( m_probe );
	probe->SetProbeParams( m_params );
}

void CRenderCommand_UpdateSkinningDataAndRelink::Execute()
{
	PC_SCOPE( CMDUpdateSkinningDataAndRelink );
	IRenderProxyBase* proxy = static_cast< IRenderProxyBase* >( m_proxy );

	if ( proxy && proxy->GetType() == RPT_Mesh )
	{
		CRenderProxy_Mesh* meshProxy = static_cast< CRenderProxy_Mesh* >( proxy );
		if ( meshProxy->GetSkinningData() == m_data )
		{
			static_cast<CRenderSkinningData*>( m_data )->AdvanceRead();
		}
		else
		{
			meshProxy->SetSkinningData( static_cast< CRenderSkinningData* >( m_data ) );
		}
	}
	else if ( proxy && proxy->GetType() == RPT_Fur )
	{
		CRenderProxy_Fur* furProxy = static_cast< CRenderProxy_Fur* >( proxy );

		if ( furProxy->GetSkinningData() == m_data )
		{
			static_cast<CRenderSkinningData*>( m_data )->AdvanceRead();
		}
		else
		{
			furProxy->SetSkinningData( m_data );
		}
	}
	else
	{
		RED_HALT("Unhandled proxy type");
	}

	if ( m_transformOnly ) 
	{
		if ( m_localToWorld != proxy->GetLocalToWorld() )
		{
			proxy->RelinkTransformOnly( m_localToWorld );
		}
	}
	else
	{
		proxy->Relink( m_boundingBox, m_localToWorld );
	}
}

void CRenderCommand_UpdateLightProxyParameter::Execute()
{
	PC_SCOPE( CMDUpdateLightParameter );
	IRenderProxyBase* proxy = static_cast< IRenderProxyBase* >( m_proxy );
	if ( proxy && ( proxy->GetType() == RPT_PointLight || proxy->GetType() == RPT_SpotLight ) )
	{
		IRenderProxyLight* lightProxy = static_cast< IRenderProxyLight* >( proxy );
		lightProxy->UpdateLightParameter( m_data, m_parameter );
	}
}

#ifndef RED_FINAL_BUILD
Double transferTime = 0.0f;
#endif

void CRenderCommand_Fence::Commit()
{
#ifndef RED_FINAL_BUILD
	transferTime = Red::System::Clock::GetInstance().GetTimer().GetSeconds();
#endif
	IRenderCommand::Commit();
}

void CRenderCommand_Fence::Execute()
{
	PC_SCOPE( CMDFence );
	CRenderFence* fence = static_cast< CRenderFence* >( m_fence );
	fence->SignalWaitingThreads();
}

void CRenderCommand_UpdateHitProxyID::Execute()
{
	PC_SCOPE( CMDUpdateHitProxyID );
	IRenderProxyBase* proxy = static_cast< IRenderProxyBase* >( m_proxy );
	proxy->UpdateHitProxyID( m_id );
};

void CRenderCommand_SetSelectionFlag::Execute()
{
	PC_SCOPE( CMDSetSelectionFlag );
	IRenderProxyBase* proxy = static_cast< IRenderProxyBase* >( m_proxy );
	proxy->UpdateSelection( m_flag );
}

void CRenderCommand_SetAutoFade::Execute()
{
	PC_SCOPE( CMDSetForceHideFlag );
	IRenderProxyFadeable* proxy = static_cast< IRenderProxyFadeable* >( m_proxy );
	proxy->SetFadeType( m_type );
}

void CRenderCommand_SetTemporaryFade::Execute()
{
	PC_SCOPE( CMDUpdateTemporaryFade );
	IRenderProxyFadeable* proxy = static_cast< IRenderProxyFadeable* >( m_proxy );
	proxy->SetTemporaryFade();
}

void CRenderCommand_FinishFades::Execute()
{
	PC_SCOPE( CMDFinishFaded );
	CRenderSceneEx* scene = static_cast< CRenderSceneEx* >( m_scene );
	scene->FinishDissolves();
}

void CRenderCommand_UpdateColorShiftMatrices::Execute()
{
	PC_SCOPE( CMDUpdateColorShiftMatrices );
	IRenderProxyDrawable* proxy = static_cast< IRenderProxyDrawable* >( m_proxy );
	proxy->UpdateColorShiftMatrices( m_colorShift0, m_colorShift1 );
}

void CRenderCommand_UpdateEffectParameters::Execute()
{
	PC_SCOPE( CMDUpdateEffectParameter );

	IRenderProxyDrawable* proxy = static_cast< IRenderProxyDrawable* >( m_proxy );
	proxy->UpdateEffectParameters( m_paramValue, m_paramIndex );
}


void CRenderCommand_UpdateOverrideParametersBatch::Execute()
{
	IRenderProxyDrawable* proxy = static_cast< IRenderProxyDrawable* >( m_proxy );
	proxy->UpdateEffectParametersOverride( m_paramValue );
}

void CRenderCommand_UpdateLightParameter::Execute()
{
	PC_SCOPE( CMDUpdateLightParameter );
	IRenderProxyLight* proxy = static_cast< IRenderProxyLight* >( m_proxy );
	proxy->UpdateParameter( m_paramName, m_paramValue );
}

void CRenderCommand_UpdateLightColor::Execute()
{
	PC_SCOPE( CMDUpdateLightColor );
	IRenderProxyLight* proxy = static_cast< IRenderProxyLight* >( m_proxy );
	proxy->UpdateColor( m_color );
}

void CRenderCommand_UpdateParticlesSimulatationContext::Execute()
{
	PC_SCOPE( CMDUpdateParticlesContext );
	CRenderProxy_Particles* proxy = static_cast< CRenderProxy_Particles* >( m_proxy );
	if ( proxy )
	{
		// (in)sanity check
		proxy->UpdateSimulationContext( m_contextUpdate );
	}
}

void CRenderCommand_UpdateParticlesSimulatationContextAndRelink::Execute()
{
	PC_SCOPE( CMDUpdateParticlesContext );
	CRenderProxy_Particles* proxy = static_cast< CRenderProxy_Particles* >( m_proxy );

	if ( proxy )
	{
		// (in)sanity check
		proxy->RelinkTransformOnly( m_localToWorld );
		proxy->UpdateSimulationContext( m_contextUpdate );
	}
}

void CRenderCommand_BatchUpdateParticlesSimulatationContextAndRelink::Execute()
{
	PC_SCOPE( CMDBatchUpdateParticlesSimulatationContextAndRelink );
	for( Uint8 i = 0; i < m_proxyCount; ++i )
	{
		const SUpdateParticlesBatchedCommand& updateCmd = m_batchedCommands[i];
		CRenderProxy_Particles* proxy = static_cast< CRenderProxy_Particles* >( updateCmd.m_renderProxy );
		// (in)sanity check
		if ( proxy )
		{
			if( updateCmd.m_relink )
			{
				proxy->RelinkTransformOnly( updateCmd.m_localToWorld );
				proxy->UpdateWindVector( updateCmd.m_simulationContext.m_windVector );
				proxy->UpdateWindVectorOnly( updateCmd.m_simulationContext.m_windVectorOnly );
			}
			else
			{
				proxy->UpdateSimulationContext( updateCmd.m_simulationContext );
			}
		}
	}
}

void CRenderCommand_TakeScreenshot::Execute()
{
	PC_SCOPE( CMDTakeScreenshot );
	GetRenderer()->TakeOneRegularScreenshot( m_screenshotParameters );
}

void CRenderCommand_TakeUberScreenshot::Execute()
{
	PC_SCOPE( CMDTakeUberScreenshot );
	CRenderSceneEx* scene = static_cast< CRenderSceneEx* >( m_scene );
	GetRenderer()->TakeOneUberScreenshot( m_frame, scene, m_screenshotParameters, m_status );
}

void CRenderCommand_GrabMovieFrame::Execute()
{
	// HACK DX10 no movie grabbing now
	//PC_SCOPE( CMDGrabMovieFrame );
	//CRenderMovie* movie = static_cast< CRenderMovie* >( m_movie );
	//movie->AddFrame(); 
}

void CRenderCommand_ToggleContinuousScreenshot::Execute()
{
	PC_SCOPE( CMDToggleContinuousScreenshot );

	// Delete current one
	if ( GScreenshotSequence )
	{
		delete GScreenshotSequence;
		GScreenshotSequence = NULL;
	}

	// Create new grabber
	if ( m_isEnabled )
	{
		ESaveFormat saveFormat = SF_BMP;
		switch ( m_saveFormat )
		{
		case FCSF_PNG: saveFormat = SF_PNG; break;
		case FCSF_BMP: saveFormat = SF_BMP; break;
		case FCSF_DDS: saveFormat = SF_DDS; break;
		}
		GScreenshotSequence = new CRenderScreenshotSequnceGrabber( saveFormat, m_useUbersampling );
	}
}

/// TEST

class BatchModifierMeshSectionHighlight : public IBatchModifier
{
public:
	IRenderResource*		m_mesh;
	TDynArray< Uint32 >		m_chunkIndices;
	Bool					m_isEnabled;

public:
	BatchModifierMeshSectionHighlight()
		: m_mesh( NULL )
		, m_isEnabled( false )
	{};

	void Enable( IRenderResource* mesh, const TDynArray< Uint32 >& chunkIndices )
	{
		// Enable modifier
		if ( !m_isEnabled )
		{
			SBatchModifier::GetInstance().AddBatchModifier( this );
			m_isEnabled = true;
		}

		// Release
		if ( m_mesh )
		{
			m_mesh->Release();
			m_mesh = NULL;
		}

		// Setup
		m_mesh = mesh;
		m_chunkIndices = chunkIndices;

		// Add ref
		if ( m_mesh )
		{
			m_mesh->AddRef();
		}
	}

	void Disable()
	{
		// Remove modifier
		if ( m_isEnabled )
		{
			SBatchModifier::GetInstance().RemoveBatchModifier( this );
			m_isEnabled = false;
		}

		// Release
		if ( m_mesh )
		{
			m_mesh->Release();
			m_mesh = NULL;
		}
	}

public:
	virtual void ModifyBatch( Batch *b )
	{
		// Force selection
		if ( b->m_mesh == m_mesh )
		{
			if ( m_chunkIndices.Exist( b->m_chunkIndex ) )
			{
				b->m_isSelected = true;
			}
		}
	}
};

void CRenderCommand_ToggleMeshMaterialHighlight::Execute()
{
	static BatchModifierMeshSectionHighlight BatchModifierMeshLighlight;

	if ( m_mesh && m_chunkIndices.Size() )
	{
		BatchModifierMeshLighlight.Enable( m_mesh, m_chunkIndices );
	}
	else
	{
		BatchModifierMeshLighlight.Disable();
	}
}

void CRenderCommand_ToggleMeshChunkHighlight::Execute()
{
	static BatchModifierMeshSectionHighlight BatchModifierMeshHighlight;

	if ( m_mesh && m_chunkIndices.Size() )
	{
		BatchModifierMeshHighlight.Enable( m_mesh, m_chunkIndices );
	}
	else
	{
		BatchModifierMeshHighlight.Disable();
	}
}

void CRenderCommand_UpdateMeshRenderParams::Execute()
{
#ifndef NO_EDITOR
	if ( m_meshProxy == nullptr ){ return; }
	if ( m_meshProxy->GetType() == RPT_Mesh )
	{
		CRenderProxy_Mesh* meshProxy = static_cast< CRenderProxy_Mesh* >( m_meshProxy );
		meshProxy->UpdateMeshRenderParams( m_meshRenderParams );
	}
#endif
}

void CRenderCommand_SuppressSceneRendering::Execute()
{
	// If we're suppressing, and we have a "restore frame", then prime texture streaming with that frame.
	if ( m_state && m_restoreFrame != nullptr && m_restoreScene != nullptr )
	{
		// Don't unload unused textures. If we don't go over-budget while rendering is suppressed, there's no reason to unload
		// everything else.
		GetRenderer()->SetupInitialFrame( m_restoreFrame, static_cast< CRenderSceneEx* >( m_restoreScene ), true, false );
	}

	// Suppress texture rendering
	CRenderViewport* rv = static_cast< CRenderViewport* >( m_viewport );
	rv->SuppressSceneRendering( m_state );
}

//////////////////////////////////////////////////////////////////////////////

void CRenderCommand_SetDestructionMeshDissolving::Execute()
{
	if ( m_proxy->GetType() == RPT_Mesh )
	{
		CRenderProxy_Mesh* meshProxy = static_cast< CRenderProxy_Mesh* >( m_proxy );
		meshProxy->SetDissolveOverride( m_useDissolve );
	}
}

//////////////////////////////////////////////////////////////////////////////

void CRenderCommand_UpdateDestructionMeshActiveIndices::Execute()
{
	if ( m_proxy->GetType() == RPT_Mesh )
	{
		CRenderProxy_DestructionMesh* meshProxy = static_cast< CRenderProxy_DestructionMesh* >( m_proxy );
		meshProxy->UpdateActiveIndices(  Move( m_activeIndices ), Move( m_chunkOffsets ), Move( m_chunkNumIndices ) );
	}
}

//////////////////////////////////////////////////////////////////////////////

/// 
class BatchModifierSelectionOverride : public IBatchModifier
{
public:
	Bool	m_isEnabled;

public:
	BatchModifierSelectionOverride()
		: m_isEnabled( false )
	{};

	void Enable()
	{
		// Enable modifier
		if ( !m_isEnabled )
		{
			SBatchModifier::GetInstance().AddBatchModifier( this );
			m_isEnabled = true;
		}
	}

	void Disable()
	{
		// Remove modifier
		if ( m_isEnabled )
		{
			SBatchModifier::GetInstance().RemoveBatchModifier( this );
			m_isEnabled = false;
		}
	}

public:
	virtual void ModifyBatch( Batch *b )
	{
		b->m_isSelected = true;
	}

	virtual void ModifyMeshSelection( CRenderElement_MeshChunk* meshChunk, Vector &color, Vector& effect )
	{
		// Desaturation
		effect.X = 1.0f; 

		// Red highlight
		meshChunk->GetProxy()->GetSelectionColor( color );
	}
};

void CRenderCommand_ToggleMeshSelectionOverride::Execute()
{
	static BatchModifierSelectionOverride BatchModifierMeshSelection;

	if ( m_state )
	{
		BatchModifierMeshSelection.Enable();
	}
	else
	{
		BatchModifierMeshSelection.Disable();
	}
}

void CRenderCommand_OverrideProxyMaterial::Execute()
{
	IRenderProxyBase *renderProxy = static_cast< IRenderProxyBase* >( m_proxy );

	if ( m_proxy )
	{
		static_cast<IRenderProxyDrawable*>( renderProxy )->SetReplacedMaterial( m_material, m_parameters, m_drawOriginal );
	}
}

void CRenderCommand_DisableProxyMaterialOverride::Execute()
{
	IRenderProxyBase *renderProxy = static_cast< IRenderProxyBase* >( m_proxy );
	static_cast<IRenderProxyDrawable*>( renderProxy )->DisableMaterialReplacement();
}


void CRenderCommand_SetNormalBlendMaterial::Execute()
{
	if ( m_proxy->GetType() == RPT_Mesh )
	{
		CRenderProxy_Mesh* meshProxy = static_cast< CRenderProxy_Mesh* >( m_proxy );
		meshProxy->SetNormalBlendMaterial( m_material, m_parameters, m_sourceBaseMaterial, m_sourceNormalTexture );
	}
}

void CRenderCommand_UpdateNormalBlendWeights::Execute()
{	
	if ( m_numWeights > 0 )
	{
		if ( m_proxy->GetType() == RPT_Mesh )
		{
			CRenderProxy_Mesh* meshProxy = static_cast< CRenderProxy_Mesh* >( m_proxy );
			meshProxy->SetNormalBlendWeights( m_firstWeight, m_numWeights, m_weights );
		}
	}
}

void CRenderCommand_DefineNormalBlendAreas::Execute()
{
	if ( m_proxy->GetType() == RPT_Mesh )
	{
		CRenderProxy_Mesh* meshProxy = static_cast< CRenderProxy_Mesh* >( m_proxy );
		meshProxy->SetNormalBlendAreas( m_firstArea, m_numAreas, m_areas );
	}
}

template <class Effect>
static inline Effect* GetEffect( EPostFXEffect id )
{
	Effect* effect = static_cast<Effect*>( GetRenderer()->GetGameplayFX()->GetEffect( id ) );
	RED_ASSERT( effect , TXT("Effect hasn't beed initialized") );
	RED_ASSERT( effect->GetType() == id , TXT("Effects type is not matching") );
	return effect;
}

void CRenderCommand_DisableAllGameplayEffects::Execute()
{
	if( GetRenderer()->GetGameplayFX() )
	{
		GetRenderer()->GetGameplayFX()->DisableAll();
	}
}

void CRenderCommand_InitSurfacePostFx::Execute()
{
	GetEffect<CSurfacePostFX>(EPE_SURFACE)->SetFillColor( m_fillColor );
}

void CRenderCommand_AddSurfacePostFx::Execute()
{
	GetEffect<CSurfacePostFX>(EPE_SURFACE)->Add( m_position, m_fadeInTime, m_fadeOutTime, m_activeTime, m_range, m_type );
}

void CRenderCommand_AddFocusModePostFx::Execute()
{
	GetEffect<CFocusModeEffectPostFX>(EPE_FOCUS_MODE)->Enable( m_desaturation, m_highlightBoost );
}

void CRenderCommand_RemoveFocusModePostFx::Execute()
{
	GetEffect<CFocusModeEffectPostFX>(EPE_FOCUS_MODE)->Disable( m_forceDisable );
}

void CRenderCommand_EnableExtendedFocusModePostFx::Execute()
{
	GetEffect<CFocusModeEffectPostFX>(EPE_FOCUS_MODE)->EnableExtended( m_fadeInTime );
}

void CRenderCommand_DisableExtendedFocusModePostFx::Execute()
{
	GetEffect<CFocusModeEffectPostFX>(EPE_FOCUS_MODE)->DisableExtended( m_fadeOutTime );
}

void CRenderCommand_UpdateFocusHighlightFading::Execute()
{
	GetEffect<CFocusModeEffectPostFX>(EPE_FOCUS_MODE)->SetFadeParameters( m_fadeNear, m_fadeFar, m_dimmingTime, m_dimmingSpeed );
}

void CRenderCommand_SetDimmingFocusMode::Execute()
{
	GetEffect<CFocusModeEffectPostFX>(EPE_FOCUS_MODE)->SetDimming( m_dimming );
}

void CRenderCommand_FocusModeSetPlayerPosition::Execute()
{
	GetEffect<CFocusModeEffectPostFX>(EPE_FOCUS_MODE)->SetPlayerPosition( m_position );
}

void CRenderCommand_EnableCatViewPostFx::Execute()
{
	GetEffect<CCatViewEffectPostFX>(EPE_CAT_VIEW)->Enable( m_fadeInTime );
}

void CRenderCommand_DisableCatViewPostFx::Execute()
{
	GetEffect<CCatViewEffectPostFX>(EPE_CAT_VIEW)->Disable( m_fadeOutTime );
}

void CRenderCommand_CatViewSetPlayerPosition::Execute()
{
	GetEffect<CCatViewEffectPostFX>(EPE_CAT_VIEW)->SetPlayerPosition( m_position , m_autoPositioning );
}

void CRenderCommand_CatViewSetTintColors::Execute()
{
	GetEffect<CCatViewEffectPostFX>(EPE_CAT_VIEW)->SetTintColors( m_tintNear , m_tintFar );
	GetEffect<CCatViewEffectPostFX>(EPE_CAT_VIEW)->SetDesaturation( m_desaturation );
}

void CRenderCommand_CatViewSetBrightness::Execute()
{
	GetEffect<CCatViewEffectPostFX>(EPE_CAT_VIEW)->SetBrightness( m_brightStrength );
}

void CRenderCommand_CatViewSetViewRange::Execute()
{
	GetEffect<CCatViewEffectPostFX>(EPE_CAT_VIEW)->SetViewRange( m_viewRanger );
}

void CRenderCommand_CatViewSetPulseParams::Execute()
{
	GetEffect<CCatViewEffectPostFX>(EPE_CAT_VIEW)->SetPulseParams( m_base, m_scale, m_speed );
}

void CRenderCommand_CatViewSetHightlight::Execute()
{
	GetEffect<CCatViewEffectPostFX>(EPE_CAT_VIEW)->SetHightlight( m_color , m_hightlightInterior , m_blurSize );
}

void CRenderCommand_CatViewSetFog::Execute()
{
	GetEffect<CCatViewEffectPostFX>(EPE_CAT_VIEW)->SetFog( m_fogDensity, m_fogStartOffset );
}

void CRenderCommand_AddSepiaPostFx::Execute()
{
	GetEffect<CSepiaEffectPostFX>(EPE_SEPIA)->Enable( m_fadeInTime );
}

void CRenderCommand_RemoveSepiaPostFx::Execute()
{
	GetEffect<CSepiaEffectPostFX>(EPE_SEPIA)->Disable( m_fadeOutTime );
}


void CRenderCommand_AddDrunkPostFx::Execute()
{
	GetEffect<CDrunkEffectPostFX>(EPE_DRUNK)->Enable( m_fadeInTime );
}

void CRenderCommand_RemoveDrunkPostFx::Execute()
{
	GetEffect<CDrunkEffectPostFX>(EPE_DRUNK)->Disable( m_fadeOutTime );
}

void CRenderCommand_ScaleDrunkPostFx::Execute()
{
	GetEffect<CDrunkEffectPostFX>(EPE_DRUNK)->Scale( m_scale );
}

void CRenderCommand_ScreenFadeOut::Execute()
{
	GetRenderer()->GetPostProcess()->FadeOut( m_color, m_time );
}

void CRenderCommand_ScreenFadeIn::Execute()
{
	GetRenderer()->GetPostProcess()->FadeIn( m_time );
}

void CRenderCommand_SetScreenFade::Execute()
{
	GetRenderer()->GetPostProcess()->SetFade( m_isIn, m_color, m_progress );
}

void CRenderCommand_UpdateFadeParameters::Execute()
{
	GetRenderer()->GetPostProcess()->Update( m_deltaTime );
}

void CRenderCommand_ChangeSceneForcedLOD::Execute()
{
	static_cast< CRenderSceneEx* >( m_scene )->SetForcedLOD( m_forceLOD );
}

void CRenderCommand_SuspendRendering::Execute()
{
	GetRenderer()->Suspend();
	GRenderThread->SetSuspended( true );
}

void CRenderCommand_ResumeRendering::Execute()
{
	GetRenderer()->Resume();
	GRenderThread->SetSuspended( false );
}

#ifdef USE_SCALEFORM

CRenderCommand_GuiRenderCommandWrapper::CRenderCommand_GuiRenderCommandWrapper( SF::Render::ThreadCommand* command,
																				IRenderScaleform* renderScaleform )
	: m_command( command )
	, m_renderScaleform( renderScaleform )
{
	ASSERT( command );
	ASSERT( renderScaleform );
}

void CRenderCommand_GuiRenderCommandWrapper::Execute()
{
	if (m_command)
	{
		m_command->Execute();
	}
}

#endif

#ifndef NO_EDITOR

void CRenderCommand_UpdateCreateParticleEmitter::Execute()
{
	CRenderProxy_Particles* rpp = static_cast< CRenderProxy_Particles* >( m_renderProxy	);
	CRenderParticleEmitter* rpe = static_cast< CRenderParticleEmitter* >( m_emitter );
	CRenderMaterial* rm = static_cast< CRenderMaterial* >( m_material );
	CRenderMaterialParameters* rmp = static_cast< CRenderMaterialParameters* >( m_materialParameters );

	rpp->UpdateOrCreateEmitter( rpe, rm, rmp, m_envColorGroup );
}

void CRenderCommand_RemoveParticleEmitter::Execute()
{
	CRenderProxy_Particles* rpp = static_cast< CRenderProxy_Particles* >( m_renderProxy	);
	rpp->RemoveEmitter( m_uniqueId );
}

#endif

//////////////////////////////////////////////////////////////////////////
// Speed Tree render commands

void CRenderCommand_UpdateFoliageRenderParams::Execute()
{
	PC_SCOPE( CMDUpdateFoliageRenderSettings );

#if defined(USE_SPEED_TREE) && !defined(NO_EDITOR)
	CRenderProxy_SpeedTree* rpst = static_cast< CRenderProxy_SpeedTree* >( m_proxy );
	rpst->UpdateFoliageRenderParams( m_foliageSRenderParams );
#endif // USE_SPEED_TREE
}

void CRenderCommand_AddSpeedTreeProxyToScene::Execute()
{
	PC_SCOPE( CMDAddSpeedTreeProxyToScene );
	CRenderSceneEx* scene = static_cast< CRenderSceneEx* >( m_scene );
	scene->AddSpeedTreeProxy( m_proxy );
}

void CRenderCommand_RemoveSpeedTreeProxyFromScene::Execute()
{
	PC_SCOPE( CMDRemoveSpeedTreeProxyFromScene );
	CRenderSceneEx* scene = static_cast< CRenderSceneEx* >( m_scene );
	scene->RemoveSpeedTreeProxy( m_proxy );
}

void CRenderCommand_UpdateSpeedTreeInstances::Execute()
{
	PC_SCOPE( CMDUpdateSpeedTreeInstances );
	ASSERT( m_speedTreeProxy );

#ifdef USE_SPEED_TREE
	CRenderProxy_SpeedTree* rpst = static_cast< CRenderProxy_SpeedTree* >( m_speedTreeProxy.Get() );
	rpst->ProcessAllUpdateRequest( std::move( m_updates ) );
#endif
}

void CRenderCommand_CreateSpeedTreeInstances::Execute()
{
	PC_SCOPE( CMDCreateSpeedTreeInstances );
	ASSERT( m_speedTreeProxy );
	ASSERT( m_baseTree );

#ifdef USE_SPEED_TREE
	CRenderProxy_SpeedTree* rpst = static_cast< CRenderProxy_SpeedTree* >( m_speedTreeProxy.Get() );
	rpst->AddStaticInstances( m_baseTree.Get(), m_instancesData, m_box );
#endif
}

void CRenderCommand_CreateSpeedTreeDynamicInstances::Execute()
{
	PC_SCOPE( CMDCreateSpeedTreeDynamicInstances );
	ASSERT( m_speedTreeProxy );
	ASSERT( m_baseTree );

#ifdef USE_SPEED_TREE
	CRenderProxy_SpeedTree* rpst = static_cast< CRenderProxy_SpeedTree* >( m_speedTreeProxy.Get() );
#if 0
	rpst->AddDynamicInstances( m_baseTree.Get(), m_instancesData, m_box );
#else
	rpst->QueueAddDynamicInstances( m_baseTree, m_instancesData, m_box );
#endif
#endif
}

void CRenderCommand_RemoveSpeedTreeInstancesRadius::Execute()
{
	PC_SCOPE( CMDRemoveSpeedTreeInstancesRadius );
	ASSERT( m_baseTree );
	ASSERT( m_speedTreeProxy );

#ifdef USE_SPEED_TREE
	CRenderProxy_SpeedTree* rpst = static_cast< CRenderProxy_SpeedTree* >( m_speedTreeProxy.Get() );
	rpst->RemoveStaticInstances( m_baseTree.Get(), m_position, m_radius );
#endif
}

void CRenderCommand_RemoveSpeedTreeDynamicInstancesRadius::Execute()
{
	PC_SCOPE( CMDRemoveSpeedTreeInstancesRadius );
	ASSERT( m_baseTree );
	ASSERT( m_speedTreeProxy );

#ifdef USE_SPEED_TREE
	CRenderProxy_SpeedTree* rpst = static_cast< CRenderProxy_SpeedTree* >( m_speedTreeProxy.Get() );
#if 0
	rpst->RemoveDynamicInstances( m_baseTree.Get(), m_position, m_radius );
#else
	rpst->QueueRemoveDynamicInstances( m_baseTree, m_position, m_radius );
#endif
#endif
}

void CRenderCommand_RemoveSpeedTreeInstancesRect::Execute()
{
	PC_SCOPE( CMDRemoveSpeedTreeInstancesRect );
	ASSERT( m_baseTree );
	ASSERT( m_speedTreeProxy );

#ifdef USE_SPEED_TREE
	CRenderProxy_SpeedTree* rpst = static_cast< CRenderProxy_SpeedTree* >( m_speedTreeProxy.Get() );
	rpst->RemoveStaticInstances( m_baseTree.Get(), m_rect );
#endif
}

void CRenderCommand_RefreshGenericGrass::Execute()
{
	PC_SCOPE( CMDRefreshGenericGrass );
	ASSERT( m_speedTreeProxy );

#ifdef USE_SPEED_TREE
	CRenderProxy_SpeedTree* rpst = static_cast< CRenderProxy_SpeedTree* >( m_speedTreeProxy.Get() );
	rpst->RefreshGenericGrass();
#endif
}

void CRenderCommand_UpdateDynamicGrassColissions::Execute()
{	
	ASSERT( m_speedTreeProxy );

#ifdef USE_SPEED_TREE
	CRenderProxy_SpeedTree* rpst = static_cast< CRenderProxy_SpeedTree* >( m_speedTreeProxy.Get() );
	rpst->UpdateDynamicGrassColisions( m_collisionsPos );
#endif
}

void CRenderCommand_UpdateGenericGrassMask::Execute()
{
	PC_SCOPE( CMDUpdateGenericGrassMask );
	ASSERT( m_terrainProxy );

	CRenderProxy_Terrain* rpt = static_cast< CRenderProxy_Terrain* >( m_terrainProxy );
	rpt->UpdateGrassMask( m_grassMask, m_grassMaskRes );

	m_grassMask = nullptr;
}

void CRenderCommand_UploadGenericGrassOccurrenceMap::Execute()
{
#ifdef USE_SPEED_TREE
	CRenderProxy_SpeedTree* spt = static_cast< CRenderProxy_SpeedTree* >( m_proxy.Get() );
	spt->SetOccurrenceMasks( m_cellMasks );
#endif
}

void CRenderCommand_SetFoliageVisualisation::Execute()
{
#ifdef USE_SPEED_TREE
	ASSERT( m_speedTreeProxy );
	CRenderProxy_SpeedTree* rpst = reinterpret_cast< CRenderProxy_SpeedTree* >( m_speedTreeProxy );	

	switch( m_mode )
	{
	case VISUALISE_NONE:
		rpst->DisableVisualisation();
		break;
	case VISUALISE_GRASSINSTANCES:
		rpst->EnableGrassInstanceVisualisation();
		break;
	case VISUALISE_TREEINSTANCES:
		rpst->EnableTreeInstanceVisualisation();
		break;
	case VISUALISE_GRASSLAYERS:
		rpst->EnableGrassLayerVisualisation();
		break;
	}
#endif
}

void CRenderCommand_UpdateFoliageBudgets::Execute()
{
#ifdef USE_SPEED_TREE
	PC_SCOPE( CMDUpdateFoliageBudgets );
	ASSERT( m_speedTreeProxy );

	CRenderProxy_SpeedTree* rpst = reinterpret_cast< CRenderProxy_SpeedTree* >( m_speedTreeProxy );	
	rpst->SetGrassDensityBudget( m_grassInstancesPerSqM );
	rpst->SetTreeDensityBudget( m_treeInstancesPerSqM );
	rpst->SetGrassLayerDensityBudget( m_grassLayersPerSqM );
#endif
}


void CRenderCommand_SetupTreeFading::Execute()
{
#ifdef USE_SPEED_TREE
	CRenderProxy_SpeedTree::SetupTreeFading( m_enable );
#endif
}

void CRenderCommand_SetupEnvironmentElementsVisibility::Execute()
{
	CRenderSceneEx* scene = static_cast< CRenderSceneEx* >( m_scene );
	if( scene )
	{
		if( scene->GetTerrain() )
		{
			scene->GetTerrain()->SetVisible( m_terrainVisible );
		}

		if( scene->GetTerrainShadows() )
		{
			scene->GetTerrainShadows()->SetVisible( m_terrainVisible );
		}
		
#ifdef USE_SPEED_TREE
		if( scene->GetSpeedTree() )
		{
			scene->GetSpeedTree()->SetVisible( m_foliageVisible );
		}
#endif
		if( scene->GetWaterProxy() )
		{
			scene->GetWaterProxy()->SetVisible( m_waterVisible );
		}
	}
}

void CRenderCommand_SetupCachets::Execute()
{
	GetRenderer()->SetupCachets(m_enable);
}

void CRenderCommand_SetTreeFadingReferencePoints::Execute()
{
#ifdef USE_SPEED_TREE
	CRenderProxy_SpeedTree* rpst = reinterpret_cast< CRenderProxy_SpeedTree* >( m_speedTreeProxy );
	rpst->SetTreeFadingReferencePoints( m_leftReference, m_rightReference, m_centerReference );
#endif
}


void CRenderCommand_UpdateClipmap::Execute()
{
	PC_SCOPE( CMDUpdateClipmap );
	CRenderProxy_Terrain* rpt = static_cast< CRenderProxy_Terrain* >( m_terrainProxy );	
	CRenderTerrainUpdateData* rud = static_cast< CRenderTerrainUpdateData* >( m_update );
	rpt->Update( rud );
}

void CRenderCommand_UpdateGrassSetup::Execute()
{
	PC_SCOPE( CMDUpdateGrass );

#ifdef USE_SPEED_TREE
	CRenderProxy_Terrain* rpt = static_cast< CRenderProxy_Terrain* >( m_terrainProxy );	
	CRenderProxy_SpeedTree* rpst = static_cast< CRenderProxy_SpeedTree* >( m_vegetationProxy );	

	CRenderGrassUpdateData* rud = static_cast< CRenderGrassUpdateData* >( m_update );
	rpt->Update( rud );

	// Collect base objects in use and callback the vegetation proxy
	TDynArray< IRenderObject* > objectsInUse;
	const TDynArray< SAutomaticGrassDesc >* descsPerTexture = rud->GetDescriptors();
	for ( Uint32 i=0; i<NUM_TERRAIN_TEXTURES_AVAILABLE; ++i )
	{
		const TDynArray< SAutomaticGrassDesc >& descs = descsPerTexture[i];
		for ( Uint32 j=0; j<descs.Size(); ++j )
		{
			objectsInUse.PushBackUnique( descs[j].GetResource() );
		}
	}

	rpst->OnGrassSetupUpdated( objectsInUse );
#endif
}

void CRenderCommand_SetTerrainCustomOverlay::Execute()
{
	CRenderProxy_Terrain* terrain = static_cast< CRenderProxy_Terrain* >( m_terrainProxy );
	if ( m_data.Empty() )
	{
		terrain->SetCustomBitmaskOverlay( 0, 0, nullptr );
	}
	else
	{
		terrain->SetCustomBitmaskOverlay( m_width, m_height, m_data.TypedData() );
	}
}

void CRenderCommand_ClearTerrainCustomOverlay::Execute()
{
	CRenderProxy_Terrain* terrain = static_cast< CRenderProxy_Terrain* >( m_terrainProxy );
	terrain->SetCustomBitmaskOverlay( 0, 0, nullptr );
}

void CRenderCommand_SetTerrainProxyToScene::Execute()
{
	CRenderSceneEx* scene = static_cast< CRenderSceneEx* >( m_scene );
	scene->SetTerrainProxy( m_proxy );
}

void CRenderCommand_RemoveTerrainProxyFromScene::Execute()
{
	CRenderSceneEx* scene = static_cast< CRenderSceneEx* >( m_scene );
	scene->RemoveTerrainProxy( m_proxy );
}

void CRenderCommand_UpdateTerrainWaterLevels::Execute()
{
	CRenderProxy_Terrain* rpt = static_cast< CRenderProxy_Terrain* >( m_terrainProxy );
	rpt->UpdateMinimumWaterLevels( m_minWaterLevels );
}

void CRenderCommand_UpdateTerrainTileHeightRanges::Execute()
{
	CRenderProxy_Terrain* rpt = static_cast< CRenderProxy_Terrain* >( m_terrainProxy );
	rpt->UpdateTileHeightRanges( m_tileHeightRanges );
}


void CRenderCommand_SetWaterProxyToScene::Execute()
{
	CRenderSceneEx* scene = static_cast< CRenderSceneEx* >( m_scene );
	scene->SetWaterProxy( m_proxy );
}

void CRenderCommand_RemoveWaterProxyFromScene::Execute()
{
	CRenderSceneEx* scene = static_cast< CRenderSceneEx* >( m_scene );
	scene->RemoveWaterProxy( m_proxy );
}

void CRenderCommand_UpdateWaterProxy::Execute()
{
	CRenderProxy_Water* rpw = static_cast< CRenderProxy_Water* >( m_waterProxy );
	CRenderTextureArray* rta = static_cast< CRenderTextureArray* >( m_textureArray );		
	
	rpw->UpdateControlTexture( rta );
}

void CRenderCommand_SimulateWaterProxy::Execute()
{
	CRenderProxy_Water* rpw = static_cast< CRenderProxy_Water* >( m_waterProxy );
	rpw->UpdatePhilipsParameters( m_waterParams );
}

void CRenderCommand_AddWaterProxyLocalShape::Execute()
{
	CRenderProxy_Water* rpw = static_cast< CRenderProxy_Water* >( m_waterProxy );
	rpw->AddLocalShapes( m_shapesParams );
}

void CRenderCommand_SkyboxSetup::Execute()
{
	if ( m_scene )
	{
		CRenderSceneEx *scene = static_cast< CRenderSceneEx* >( m_scene );
		CRenderSkybox *skybox = scene->GetSkybox();

		if ( skybox )
		{
			skybox->SetParameters( m_params );
		}
	}
}

void CRenderCommand_LensFlareSetup::Execute()
{
	if ( m_scene )
	{
		CRenderSceneEx *scene = static_cast< CRenderSceneEx* >( m_scene );
		
		scene->SetLensFlareGroupsParameters( m_params );
	}
}

void CRenderCommand_HandleResizeEvent::Execute()
{
#ifdef USE_SCALEFORM
	CRenderScaleform* renderScaleform = GRender ? static_cast< CRenderScaleform* >( GRender->GetRenderScaleform() ) : nullptr;
	if ( renderScaleform && renderScaleform->GetRenderState() == IRenderScaleform::RS_Ready )
	{
		renderScaleform->HandleDeviceLost();
		RED_ASSERT( renderScaleform->GetRenderState() == CRenderScaleform::RS_DeviceLost );
	}
#endif // USE_SCALEFORM

	if( GIsEditor == false )		// We don't want to resize render surfaces when in editor, so we keep the initial ones
	{
		GetRenderer()->ResizeRenderSurfaces( m_width, m_height );
	}
}

void CRenderCommand_SetEntityGroupHiResShadows::Execute()
{
	if ( m_group != NULL )
	{
		CRenderEntityGroup* entityGroup = static_cast<CRenderEntityGroup*>( m_group );
		entityGroup->SetUseHiResShadows( m_flag );
	}
}

void CRenderCommand_SetEntityGroupShadows::Execute()
{
	if ( m_group != NULL )
	{
		CRenderEntityGroup* entityGroup = static_cast<CRenderEntityGroup*>( m_group );
		entityGroup->SetUseShadows( m_flag );
	}
}

void CRenderCommand_BindEntityGroupToProxy::Execute()
{
	if ( NULL == m_proxy )
	{
		return;
	}

	IRenderEntityGroupProxy *entityGroupProxy = nullptr;
	if ( RPT_Mesh == m_proxy->GetType() )
	{
		entityGroupProxy = static_cast< CRenderProxy_Mesh* >( m_proxy );
	}
	else if ( RPT_Fur == m_proxy->GetType() )
	{
		entityGroupProxy = static_cast< CRenderProxy_Fur* >( m_proxy );
	}
#ifdef USE_APEX
	else if ( RPT_Apex == m_proxy->GetType() )
	{
		entityGroupProxy = static_cast< CRenderProxy_Apex* >( m_proxy );
	}
#endif

	if ( entityGroupProxy )
	{
		const Bool isAttached = nullptr != static_cast< IRenderProxyBase* >( m_proxy )->GetScene();
		CRenderEntityGroup* group = static_cast< CRenderEntityGroup* >( m_group );
		entityGroupProxy->SetEntityGroup( group, isAttached );
	}
}

#ifdef USE_APEX
void CRenderCommand_UpdateApexRenderable::Execute()
{
	if ( m_proxy && m_renderable )
	{
		CRenderProxy_Apex* apexProxy = static_cast< CRenderProxy_Apex* >( m_proxy );
		apexProxy->SetApexRenderable( m_renderable );
		apexProxy->SetWetness( m_wetness );
		apexProxy->Relink( m_boundingBox, m_localToWorld );
	}
}
#endif


void CRenderCommand_UpdateTerrainShadows::Execute()
{
	CRenderSceneEx* renderScene = static_cast< CRenderSceneEx* >( m_renderScene );
	renderScene->GetTerrainShadows()->RequestFullUpdate();
}


void CRenderCommand_SetProxyLightChannels::Execute()
{
	if ( !m_proxy ) return;

	IRenderProxyDrawable* drawable = static_cast< IRenderProxyDrawable* >( m_proxy );

	Uint8 lc = static_cast< Uint8 >( drawable->GetLightChannels() );
	
	// Copy over the bits within the mask.
	lc = ( lc & ~m_mask ) | ( m_lightChannels & m_mask );

	drawable->SetLightChannels( lc );
}
void CRenderCommand_UpdateMorphRatio::Execute()
{
	if ( !m_proxy ) return;

	CRenderProxy_MorphedMesh* mesh = static_cast< CRenderProxy_MorphedMesh* >( m_proxy );
	mesh->SetMorphRatio( m_ratio );
}

void CRenderCommand_SetClippingEllipseMatrix::Execute()
{
	if ( !m_proxy ) return;
	static_cast< IRenderProxyDrawable* >( m_proxy )->SetClippingEllipseMatrix( m_meshToEllipse );
}

void CRenderCommand_ClearClippingEllipseMatrix::Execute()
{
	if ( !m_proxy ) return;
	static_cast< IRenderProxyDrawable* >( m_proxy )->ClearClippingEllipse();
}

void CRenderCommand_AddDynamicDecalToScene::Execute()
{
	if ( !m_decal || !m_scene ) return;
	static_cast< CRenderSceneEx* >( m_scene )->QueueDynamicDecalSpawn( m_decal, m_projectOnlyOnStatic );
}

void CRenderCommand_AddDynamicDecalToSceneForProxies::Execute()
{
	if ( !m_decal || !m_scene ) return;
	static_cast< CRenderSceneEx* >( m_scene )->QueueDynamicDecalSpawn( m_decal, m_targetProxies );
}

void CRenderCommand_RemoveDynamicDecalFromScene::Execute()
{
	if ( !m_decal || !m_scene ) return;
	static_cast< CRenderSceneEx* >( m_scene )->RemoveDynamicDecal( m_decal );
}

void CRenderCommand_UpdateStripeProperties::Execute()
{
	if ( m_proxy && m_properties )
	{
		CRenderProxy_Stripe* stripeProxy = static_cast< CRenderProxy_Stripe* >( m_proxy );
		stripeProxy->UpdateProperties( *m_properties );
		delete m_properties;
		m_properties = NULL;
	}
}

void CRenderCommand_UpdateTerrainScreenSpaceErrorThreshold::Execute()
{
	if ( m_proxy != nullptr )
	{
		CRenderProxy_Terrain* terrainProxy = static_cast< CRenderProxy_Terrain* >( m_proxy );
		terrainProxy->SetScreenSpaceErrorThresholdOverride( m_terrainScreenSpaceErrorThreshold );
	}
}

void CRenderCommand_SetParticlePriority::Execute()
{
	if( m_proxy != nullptr && m_proxy->GetType() == RPT_Particles )
	{
		CRenderProxy_Particles* proxyParticles = static_cast< CRenderProxy_Particles* >( m_proxy );
		proxyParticles->SetRenderPriority( m_priority );
	}
}

void CRenderCommand_UpdateSwarmData::Execute()
{
	IRenderProxyBase* proxy = static_cast< IRenderProxyBase* >( m_proxy );

	if ( proxy && proxy->GetType() == RPT_Swarm )
	{
		CRenderProxy_Swarm* swarmProxy = static_cast< CRenderProxy_Swarm* >( proxy );
		CRenderSwarmData* swarmData = static_cast< CRenderSwarmData* >( m_data );
		swarmData->TogglePhase();
		swarmProxy->UpdateBoids( swarmData, m_numBoids );
	}
}

void CRenderCommand_AddRenderingExclusionToScene::Execute()
{
	CRenderSceneEx* scene = static_cast< CRenderSceneEx* >( m_scene );
	scene->AddVisibilityExclusionObject( m_object );
}

void CRenderCommand_RemoveRenderingExclusionToScene::Execute()
{
	CRenderSceneEx* scene = static_cast< CRenderSceneEx* >( m_scene );
	scene->RemoveVisibilityExclusionObject( m_object );
}

void CRenderCommand_ToggleRenderingExclusion::Execute()
{
	CRenderSceneEx* scene = static_cast< CRenderSceneEx* >( m_scene );
	CRenderVisibilityExclusionList* list = static_cast< CRenderVisibilityExclusionList* >( m_object );
	list->SetState( m_state );
	scene->RefreshVisibilityExclusionObject( m_object );
}