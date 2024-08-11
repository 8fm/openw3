/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "../engine/renderCommands.h"


void CRenderCommand_RenderScene::Execute()
{
}

/////////////////////////////////////////////////////////////////////////////////////////////

//("HACK FIXME these shouldn't be here")

void CRenderCommand_NewFrame::Execute()
{
}

void CRenderCommand_EndFrame::Execute()
{
}

void CRenderCommand_CancelTextureStreaming::Execute()
{
}

void CRenderCommand_ShutdownTextureStreaming::Execute()
{
}

void CRenderCommand_StartTextureStreamRequest::Execute()
{
}

void CRenderCommand_CancelTextureStreamRequest::Execute()
{
}

void CRenderCommand_StartFramePrefetch::Execute()
{
}

void CRenderCommand_PrepareInitialTextureStreaming::Execute()
{
}

void CRenderCommand_FinishInitialTextureStreaming::Execute()
{
}

void CRenderCommand_SetupEnvProbesPrefetch::Execute()
{
}

void CRenderCommand_TickTextureStreaming::Execute()
{
}

void CRenderCommand_ToggleCinematicMode::Execute()
{
}

void CRenderCommand_TextureCacheAttached::Execute()
{
}

void CRenderCommand_ShowLoadingScreenBlur::Execute()
{
}

void CRenderCommand_HideLoadingScreenBlur::Execute()
{
}

void CRenderCommand_SetLoadingScreenFence::Execute()
{
}

void CRenderCommand_FadeOutLoadingScreen::Execute()
{
}

void CRenderCommand_SetVideoFlash::Execute()
{
}

void CRenderCommand_W3HackShowVideoBackground::Execute()
{
}

void CRenderCommand_W3HackHideVideoBackground::Execute()
{
}

void CRenderCommand_W3HackSetVideoClearRGB::Execute()
{
}

void CRenderCommand_SetVideoMasterVolume::Execute()
{
}

void CRenderCommand_SetVideoVoiceVolume::Execute()
{
}

void CRenderCommand_SetVideoEffectsVolume::Execute()
{
}

void CRenderCommand_SetLoadingOverlayFlash::Execute()
{
}

void CRenderCommand_ToggleLoadingOverlay::Execute()
{
}

void CRenderCommand_ToggleVideoPause::Execute()
{
}

void CRenderCommand_PlayVideo::Execute()
{
}

void CRenderCommand_PlayLoadingScreenVideo::Execute()
{
}

void CRenderCommand_CancelVideo::Execute()
{
}

void CRenderCommand_ToggleLoadingVideoSkip::Execute()
{
}

void CRenderCommand_SetLoadingPCInput::Execute()
{

}

void CRenderCommand_SetExpansionsAvailable::Execute()
{

}

void CRenderCommand_UpdateProgressStatus::Execute()
{
}

void CRenderCommand_UpdateProgress::Execute()
{
}

void CRenderCommand_AddProxyToScene::Execute()
{
}

void CRenderCommand_AddStripeToScene::Execute()
{
}

void CRenderCommand_AddFurToScene::Execute()
{
}

void CRenderCommand_UpdateFurParams::Execute()
{
}

#ifndef NO_EDITOR
#ifdef USE_NVIDIA_FUR
void CRenderCommand_EditorSetFurParams::Execute()
{
}
#endif //USE_NVIDIA_FUR
#endif //NO_EDITOR

void CRenderCommand_RemoveProxyFromScene::Execute()
{
}

void CRenderCommand_RemoveStripeFromScene::Execute()
{
}

void CRenderCommand_RemoveFurFromScene::Execute()
{
}

#ifdef USE_UMBRA
void CRenderCommand_UploadOcclusionDataToScene::Execute()
{
}

void CRenderCommand_SetDoorState::Execute()
{
}

void CRenderCommand_SetCutsceneModeForGates::Execute()
{
}

void CRenderCommand_ClearOcclusionData::Execute()
{
}

void CRenderCommand_UploadObjectCache::Execute()
{
}

void CRenderCommand_SetValidityOfOcclusionData::Execute()
{
}

void CRenderCommand_UpdateQueryThreshold::Execute()
{
}

#ifndef NO_EDITOR
void CRenderCommand_DumpVisibleMeshes::Execute()
{
}
#endif // NO_EDITOR
#endif // USE_UMBRA

#ifdef USE_UMBRA
void CRenderCommand_PerformVisibilityQueries::Execute()
{
	UmbraQueryBatch::Iterator it ( m_queryList );
	while ( it )
	{
		it->SetResult( true );
		++it;
	}
}
#endif

void CRenderCommand_RelinkProxy::Execute()
{
}

void CRenderCommand_BatchSkinAndRelinkProxies::Execute()
{
}

void CRenderCommand_EnvProbeParamsChanged::Execute()
{
}

void CRenderCommand_UpdateSkinningDataAndRelink::Execute()
{
}

void CRenderCommand_BatchUpdateParticlesSimulatationContextAndRelink::Execute()
{
}

void CRenderCommand_Fence::Commit()
{
}

void CRenderCommand_Fence::Execute()
{
}

void CRenderCommand_UpdateHitProxyID::Execute()
{
}

void CRenderCommand_SetSelectionFlag::Execute()
{
}

void CRenderCommand_UpdateColorShiftMatrices::Execute()
{
}

void CRenderCommand_UpdateEffectParameters::Execute()
{
}

void CRenderCommand_UpdateLightParameter::Execute()
{
}

void CRenderCommand_UpdateLightColor::Execute()
{
}

void CRenderCommand_UpdateParticlesSimulatationContext::Execute()
{
}

void CRenderCommand_UpdateParticlesSimulatationContextAndRelink::Execute()
{
}

void CRenderCommand_TakeScreenshot::Execute()
{
}

void CRenderCommand_TakeUberScreenshot::Execute()
{
}

void CRenderCommand_GrabMovieFrame::Execute()
{
}

void CRenderCommand_SetAutoFade::Execute()
{
}

void CRenderCommand_SetTemporaryFade::Execute()
{
}

void CRenderCommand_ToggleContinuousScreenshot::Execute()
{
}

void CRenderCommand_ToggleMeshMaterialHighlight::Execute()
{
}

void CRenderCommand_ToggleMeshChunkHighlight::Execute()
{
}

void CRenderCommand_UpdateMeshRenderParams::Execute()
{
}

void CRenderCommand_SuppressSceneRendering::Execute()
{
}

void CRenderCommand_SetDestructionMeshDissolving::Execute()
{
}

void CRenderCommand_UpdateDestructionMeshActiveIndices::Execute()
{
}

void CRenderCommand_ToggleMeshSelectionOverride::Execute()
{
}

void CRenderCommand_OverrideProxyMaterial::Execute()
{
}

void CRenderCommand_DisableProxyMaterialOverride::Execute()
{
}

void CRenderCommand_SetNormalBlendMaterial::Execute()
{
}

void CRenderCommand_DefineNormalBlendAreas::Execute()
{
}

void CRenderCommand_UpdateNormalBlendWeights::Execute()
{	
}

void CRenderCommand_DisableAllGameplayEffects::Execute()
{
}

void CRenderCommand_AddFocusModePostFx::Execute()
{
}

void CRenderCommand_RemoveFocusModePostFx::Execute()
{
}

void CRenderCommand_EnableExtendedFocusModePostFx::Execute()
{
}

void CRenderCommand_DisableExtendedFocusModePostFx::Execute()
{
}

void CRenderCommand_UpdateFocusHighlightFading::Execute()
{
}

void CRenderCommand_FocusModeSetPlayerPosition::Execute()
{
}

void CRenderCommand_InitSurfacePostFx::Execute()
{
}

void CRenderCommand_AddSurfacePostFx::Execute()
{
}

void CRenderCommand_AddSepiaPostFx::Execute()
{
}
void CRenderCommand_RemoveSepiaPostFx::Execute()
{
}
void CRenderCommand_RemoveDrunkPostFx::Execute()
{}
void CRenderCommand_AddDrunkPostFx::Execute()
{}
void CRenderCommand_ScaleDrunkPostFx::Execute()
{}
void CRenderCommand_EnableCatViewPostFx::Execute()
{}
void CRenderCommand_DisableCatViewPostFx::Execute()
{}
void CRenderCommand_CatViewSetPlayerPosition::Execute()
{}
void CRenderCommand_CatViewSetTintColors::Execute()
{}
void CRenderCommand_CatViewSetBrightness::Execute()
{}
void CRenderCommand_CatViewSetViewRange::Execute()
{}
void CRenderCommand_CatViewSetPulseParams::Execute()
{}
void CRenderCommand_CatViewSetHightlight::Execute()
{}
void CRenderCommand_CatViewSetFog::Execute()
{}

void CRenderCommand_ScreenFadeOut::Execute()
{
}

void CRenderCommand_ScreenFadeIn::Execute()
{
}

void CRenderCommand_SetScreenFade::Execute()
{
}

void CRenderCommand_UpdateFadeParameters::Execute()
{
}

void CRenderCommand_UpdateOverrideParametersBatch::Execute()
{
}

void CRenderCommand_UpdateLightProxyParameter::Execute()
{
}

void CRenderCommand_ChangeSceneForcedLOD::Execute()
{
}

void CRenderCommand_SuspendRendering::Execute()
{
}

void CRenderCommand_ResumeRendering::Execute()
{
}

void CRenderCommand_UpdateFoliageRenderParams::Execute()
{
}

#ifndef NO_EDITOR
void CRenderCommand_UpdateCreateParticleEmitter::Execute()
{
}

void CRenderCommand_RemoveParticleEmitter::Execute()
{
}
#endif

void CRenderCommand_CreateSpeedTreeInstances::Execute()
{
}

void CRenderCommand_CreateSpeedTreeDynamicInstances::Execute()
{
}

void CRenderCommand_RemoveSpeedTreeInstancesRadius::Execute()
{
}

void CRenderCommand_RemoveSpeedTreeDynamicInstancesRadius::Execute()
{
}

void CRenderCommand_RemoveSpeedTreeInstancesRect::Execute()
{
}

void CRenderCommand_RefreshGenericGrass::Execute()
{
}

void CRenderCommand_UpdateDynamicGrassColissions::Execute()
{
}

void CRenderCommand_UpdateGenericGrassMask::Execute()
{
}

void CRenderCommand_UploadGenericGrassOccurrenceMap::Execute()
{
}

void CRenderCommand_UpdateSpeedTreeInstances::Execute()
{

}

void CRenderCommand_AddSpeedTreeProxyToScene::Execute()
{
}

void CRenderCommand_RemoveSpeedTreeProxyFromScene::Execute()
{
}

void CRenderCommand_SetupTreeFading::Execute()
{
}

void CRenderCommand_SetupEnvironmentElementsVisibility::Execute()
{
}

void CRenderCommand_SetupCachets::Execute()
{
}

void CRenderCommand_SetTreeFadingReferencePoints::Execute()
{
}

void CRenderCommand_UpdateClipmap::Execute()
{
}

void CRenderCommand_UpdateGrassSetup::Execute()
{
}

void CRenderCommand_SetTerrainCustomOverlay::Execute()
{
}

void CRenderCommand_ClearTerrainCustomOverlay::Execute()
{
}

void CRenderCommand_SetTerrainProxyToScene::Execute()
{
}

void CRenderCommand_RemoveTerrainProxyFromScene::Execute()
{
}

void CRenderCommand_UpdateTerrainWaterLevels::Execute()
{
}

void CRenderCommand_UpdateTerrainTileHeightRanges::Execute()
{
}

void CRenderCommand_SkyboxSetup::Execute()
{
}

void CRenderCommand_LensFlareSetup::Execute()
{
}

void CRenderCommand_HandleResizeEvent::Execute()
{
}

void CRenderCommand_SetWaterProxyToScene::Execute()
{
}

void CRenderCommand_RemoveWaterProxyFromScene::Execute()
{
}

void CRenderCommand_UpdateWaterProxy::Execute()
{
}

void CRenderCommand_SimulateWaterProxy::Execute()
{
}

void CRenderCommand_AddWaterProxyLocalShape::Execute()
{	
}

void CRenderCommand_SetEntityGroupHiResShadows::Execute()
{
}

void CRenderCommand_SetEntityGroupShadows::Execute()
{
}

#ifdef USE_APEX
void CRenderCommand_UpdateApexRenderable::Execute()
{
}
#endif

void CRenderCommand_FinishFades::Execute()
{
}

void CRenderCommand_UpdateTerrainShadows::Execute()
{
}

void CRenderCommand_SetProxyLightChannels::Execute()
{
}

void CRenderCommand_UpdateMorphRatio::Execute()
{
}

void CRenderCommand_BindEntityGroupToProxy::Execute()
{
}

void CRenderCommand_SetClippingEllipseMatrix::Execute()
{
}

void CRenderCommand_SetDimmingFocusMode::Execute()
{
}

void CRenderCommand_ClearClippingEllipseMatrix::Execute()
{
}

void CRenderCommand_AddDynamicDecalToScene::Execute()
{
}

void CRenderCommand_AddDynamicDecalToSceneForProxies::Execute()
{
}

void CRenderCommand_RemoveDynamicDecalFromScene::Execute()
{
}

void CRenderCommand_UpdateFoliageBudgets::Execute()
{
}

void CRenderCommand_UpdateStripeProperties::Execute()
{
}

void CRenderCommand_SetFoliageVisualisation::Execute()
{
}

void CRenderCommand_UpdateTerrainScreenSpaceErrorThreshold::Execute()
{
}

void CRenderCommand_SetParticlePriority::Execute()
{
}

void CRenderCommand_UpdateSwarmData::Execute()
{
}

void CRenderCommand_AddRenderingExclusionToScene::Execute()
{
}

void CRenderCommand_RemoveRenderingExclusionToScene::Execute()
{
}

void CRenderCommand_ToggleRenderingExclusion::Execute()
{
}

void CRenderCommand_ForceUITextureStreaming::Execute()
{
}