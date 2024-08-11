/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../redIO/redIO.h"
#include "../../common/core/depot.h"
#include "../../common/core/feedback.h"
#include "../../common/engine/mesh.h"
#include "../../common/engine/renderCommands.h"
#include "../../common/engine/clipMap.h"
#include "../../common/physics/physicsSettings.h"
#include "../../common/engine/environmentManager.h"
#include "../../common/engine/worldIterators.h"
#include "../../common/engine/pathlibWorld.h"
#include "../../common/engine/pathlibInstanceMap.h"
#include "../../common/engine/pathlibVisualizer.h"
#include "../../common/engine/pathlib.h"
#include "../../common/engine/pathlibnavmesh.h"
#include "../../common/engine/pathlibNavmeshArea.h"
#include "../../common/engine/pathlibStreamingManager.h"
#include "../../common/engine/triggerAreaComponent.h"
#include "../../common/engine/renderFragment.h"
#include "../../common/engine/streamingSectorData.h"
#include "../../common/engine/sectorDataStreaming.h"
#include "world.h"
#include "entity.h"
#include "../../games/r4/r4interiorAreaComponent.h"
#include "foliageEditionController.h"
#include "umbraScene.h"
#include "baseEngine.h"
#include "minimapGenerator.h"
#include "stripeComponent.h"
#include "environmentDefinition.h"
#include "dynamicLayer.h"
#include "layerGroup.h"
#include "layerInfo.h"
#include "pathlibNavmeshComponent.h"
#include "meshComponent.h"
#include "gameTimeManager.h"

String ToString( enum EMinimapMask value )
{
	switch( value )
	{
	case MM_Full:
		return TXT("Full");
	case MM_Albedo:
		return TXT("Albedo");
	case MM_Normals:
		return TXT("Normals");
	case MM_NormalsNoTrees:
		return TXT("NormalsNoTrees");
	case MM_Water:
		return TXT("Water");
	case MM_Foliage:
		return TXT("Foliage");
	case MM_Volumes:
		return TXT("Volumes");
	case MM_Objects:
		return TXT("Objects");
	case MM_Heightmap:
		return TXT("Heightmap");
	case MM_Terrain:
		return TXT("Terrain");
	case MM_Roads:
		return TXT("Roads");
	case MM_Bridges:
		return TXT("Bridges");
	case MM_DEBUG_GrassInstanceHeatMap:
		return TXT("DEBUG_GrassInstanceHeatMap");
	case MM_DEBUG_TreeInstanceHeatMap:
		return TXT("DEBUG_TreeInstanceHeatMap");
	case MM_DEBUG_GrassLayerHeatMap:
		return TXT("DEBUG_GrassLayerHeatMap");
	}

	return String::EMPTY;
}

String ToString( enum EDirLayout value )
{
	switch( value )
	{
	case DL_Default:
		return TXT("Default");
	case DL_Photoshop:
		return TXT("Photoshop");
	}

	return String::EMPTY;
}

String ToString( enum EGenerationMode value )
{
	switch( value )
	{
	case GM_CurrentTile:
		return TXT("CurrentTile");
	case GM_CurrentTilePlus:
		return TXT("CurrentTilePlus");
	case GM_TileRange:
		return TXT("TileRange");
	case GM_AllTiles:
		return TXT("AllTiles");
	}

	return String::EMPTY;
}

SMinimapExteriorSettings::SMinimapExteriorSettings()
	: m_dirLayout( DL_Default )
	, m_imageSize( 1024 )
	, m_imageOffset( 0 )
	, m_imageZoom( 0 )
	, m_envSettingsPath( TXT("") )
	, m_fileNamePrefix( TXT("Tile") )
	, m_continueMode( true )
{
	for( Uint32 i=0; i<MM_Count; ++i )
	{
		m_enabledMasks[i] = false;
	}
}

SMinimapSettings::SMinimapSettings()
	: m_generationMode( GM_AllTiles )
	, m_tileCountPlus( 0 )
	, m_tileRange( 0, 0, 0, 0 )
	, m_outputDir( TXT("") )

{
	/* intentionally empty */
}

CMinimapGenerator::CMinimapGenerator()
{
	// get main variables
	m_world = GGame->GetActiveWorld();
	m_terrain = m_world->GetTerrain();
}

CMinimapGenerator::~CMinimapGenerator()
{
	/* intentionally empty */
}

void CMinimapGenerator::SetSettings( const SMinimapSettings& settings )
{
	m_settings = settings;
}

const SMinimapSettings& CMinimapGenerator::GetSettings() const
{
	return m_settings;
}

void CMinimapGenerator::GenerateExteriors()
{
	RememberEngineSettings();
	SetNewEngineSettings();

	// Prepare to render
	PrepareViewport( m_settings.m_exteriors.m_imageSize );
	CountTilesToRender();

	// Render all tiles for selected masks
	InternalGenerateExteriors();

	// Destroy 
	DestroyViewport();

	RevertEngineSettings();
}

void CMinimapGenerator::PrepareViewport( Uint32 viewportSize )
{
	// Create viewport window
	m_viewportSize = viewportSize;
	m_viewport = GRender->CreateViewport( nullptr, nullptr, TXT("Minimaps"), m_viewportSize, m_viewportSize, VWM_Windowed, false );
	if( m_viewport == nullptr )
	{
		RED_HALT( "Mini maps generator cannot create new viewport" );
	}
}

void CMinimapGenerator::DestroyViewport()
{
	// Destroy the viewport
	m_viewport.Reset();
}

void CMinimapGenerator::RememberEngineSettings()
{
	// save current settings
	m_engineSettingsBackup.m_gameEnvParams					= m_world->GetEnvironmentManager()->GetGameEnvironmentParams();
	m_engineSettingsBackup.m_skyboxParams					= m_world->GetEnvironmentParameters().m_skybox;
	m_engineSettingsBackup.m_asyncMaterialCompilation		= GRender->GetAsyncCompilationMode();
#ifdef USE_UMBRA
	m_engineSettingsBackup.m_umbraOcclusion					= CUmbraScene::IsUsingOcclusionCulling();
#else
	m_engineSettingsBackup.m_umbraOcclusion					= false;
#endif

	m_engineSettingsBackup.m_dontCreateTrees				= SPhysicsSettings::m_dontCreateTrees;
	m_engineSettingsBackup.m_dontCreateRagdolls				= SPhysicsSettings::m_dontCreateRagdolls;
	m_engineSettingsBackup.m_dontCreateDestruction			= SPhysicsSettings::m_dontCreateDestruction;
	m_engineSettingsBackup.m_dontCreateCloth				= SPhysicsSettings::m_dontCreateCloth;
	m_engineSettingsBackup.m_dontCreateClothOnGPU			= SPhysicsSettings::m_dontCreateClothOnGPU;
	m_engineSettingsBackup.m_dontCreateClothSecondaryWorld	= SPhysicsSettings::m_dontCreateClothSecondaryWorld;
	m_engineSettingsBackup.m_dontCreateParticles			= SPhysicsSettings::m_dontCreateParticles;
	m_engineSettingsBackup.m_dontCreateParticlesOnGPU		= SPhysicsSettings::m_dontCreateParticlesOnGPU;

	m_engineSettingsBackup.m_envDefinition					= m_world->GetEnvironmentParameters().m_environmentDefinition;
	m_engineSettingsBackup.m_scenesEnvDefinition			= m_world->GetEnvironmentParameters().m_scenesEnvironmentDefinition;
	
	if( GGame != nullptr )
	{
		m_engineSettingsBackup.m_gameTime = GGame->GetTimeManager()->GetTime();
	}
}

void CMinimapGenerator::SetNewEngineSettings()
{
	// set new settings
#ifdef USE_UMBRA
	CUmbraScene::UseOcclusionCulling( false );
#endif
	GRender->SetAsyncCompilationMode( false );

	SPhysicsSettings::m_dontCreateTrees = true;
	SPhysicsSettings::m_dontCreateRagdolls = true;
	SPhysicsSettings::m_dontCreateDestruction = true;
	SPhysicsSettings::m_dontCreateCloth = true;
	SPhysicsSettings::m_dontCreateClothOnGPU = true;
	SPhysicsSettings::m_dontCreateClothSecondaryWorld = true;
	SPhysicsSettings::m_dontCreateParticles = true;
	SPhysicsSettings::m_dontCreateParticlesOnGPU = true;

	// 
	//SRenderSettings temporarySettings = SRenderSettingsManager::GetInstance().GetSettings();
	//temporarySettings.m_grassDistanceScale = 1000.f;
	//temporarySettings.m_terrainScreenSpaceErrorThreshold = 0.0f;
	//SRenderSettingsManager::GetInstance().SetSettings( temporarySettings );

	//
	if( m_settings.m_exteriors.m_envSettingsPath.Empty() == false )
	{
		SWorldEnvironmentParameters params = m_world->GetEnvironmentParameters();
		String depotPath = String::EMPTY;
		GDepot->GetAbsolutePath( depotPath );
		String resourcePath = m_settings.m_exteriors.m_envSettingsPath;
		resourcePath.Replace( depotPath, TXT("") );
		CResource* res = GDepot->LoadResource( resourcePath );
		CEnvironmentDefinition* envdef = Cast< CEnvironmentDefinition >( res );
		if( envdef != nullptr || nullptr != params.m_scenesEnvironmentDefinition )
		{
			params.m_environmentDefinition = envdef;			
			params.m_scenesEnvironmentDefinition = nullptr;
			m_world->SetEnvironmentParameters( params );
			m_world->UpdateLoadingState( true );
		}
	}

	if( GGame != nullptr )
	{
		Int32 seconds = 0;
		Int32 minutes = 0;
		Int32 hours = 11;
		GGame->GetTimeManager()->SetTime( GameTime( 0, hours, minutes, seconds ), true );
		GGame->Tick( 1.0f );
	}

	//
	SWorldSkyboxParameters temporarySkyboxParams = m_world->GetEnvironmentParameters().m_skybox;
	temporarySkyboxParams.m_cloudsMesh = nullptr;
	temporarySkyboxParams.m_moonMesh = nullptr;
	temporarySkyboxParams.m_sunMesh = nullptr;
	temporarySkyboxParams.m_skyboxMesh = nullptr;
	m_world->SetupSceneSkyboxParams( temporarySkyboxParams );

	// turn off autohide for all meshes on the map
	TDynArray< CDrawableComponent* > components;
	GGame->GetActiveWorld()->GetAttachedComponentsOfClass( components );
	for( Uint32 i=0; i<components.Size(); ++i )
	{
		components[i]->SetForceNoAutohide( true );
	}

#ifndef NO_EDITOR
	CFoliageEditionController &controller = m_world->GetFoliageEditionController();
	SFoliageRenderParams foliageRenderParams;
	foliageRenderParams.m_foliageDistShift = 5.0f;
	foliageRenderParams.m_grassDistScale = 0.0f;
	controller.UpdateFoliageRenderParams( foliageRenderParams );
#endif	// NO_EDITOR

	( new CRenderCommand_UpdateTerrainScreenSpaceErrorThreshold( m_terrain->GetTerrainProxy(), -1.0f ) )->Commit();
}

void CMinimapGenerator::RevertEngineSettings()
{
	SPhysicsSettings::m_dontCreateTrees					= m_engineSettingsBackup.m_dontCreateTrees;
	SPhysicsSettings::m_dontCreateRagdolls				= m_engineSettingsBackup.m_dontCreateRagdolls;
	SPhysicsSettings::m_dontCreateDestruction			= m_engineSettingsBackup.m_dontCreateDestruction;
	SPhysicsSettings::m_dontCreateCloth					= m_engineSettingsBackup.m_dontCreateCloth;
	SPhysicsSettings::m_dontCreateClothOnGPU			= m_engineSettingsBackup.m_dontCreateClothOnGPU;
	SPhysicsSettings::m_dontCreateClothSecondaryWorld	= m_engineSettingsBackup.m_dontCreateClothSecondaryWorld;
	SPhysicsSettings::m_dontCreateParticles				= m_engineSettingsBackup.m_dontCreateParticles;
	SPhysicsSettings::m_dontCreateParticlesOnGPU		= m_engineSettingsBackup.m_dontCreateParticlesOnGPU;

	m_world->GetEnvironmentManager()->SetGameEnvironmentParams( m_engineSettingsBackup.m_gameEnvParams );
	m_world->SetupSceneSkyboxParams( m_engineSettingsBackup.m_skyboxParams );
	m_world->GetEnvironmentManager()->DisableHDR();

	//
	GRender->SetAsyncCompilationMode( m_engineSettingsBackup.m_asyncMaterialCompilation );
#ifdef USE_UMBRA
	CUmbraScene::UseOcclusionCulling( m_engineSettingsBackup.m_umbraOcclusion );
#endif

	// turn off autohide for all meshes on the map
	TDynArray< CDrawableComponent* > components;
	GGame->GetActiveWorld()->GetAttachedComponentsOfClass( components );
	for( Uint32 i=0; i<components.Size(); ++i )
	{
		components[i]->SetForceNoAutohide( false );
	}

	m_world->GetFoliageEditionController().SetDebugVisualisationMode( VISUALISE_NONE );

	// revert environment definition
	SWorldEnvironmentParameters params = m_world->GetEnvironmentParameters();
	params.m_environmentDefinition = m_engineSettingsBackup.m_envDefinition;
	params.m_scenesEnvironmentDefinition = m_engineSettingsBackup.m_scenesEnvDefinition;
	m_world->SetEnvironmentParameters( params );

	if( GGame != nullptr )
	{
		GGame->GetTimeManager()->SetTime( m_engineSettingsBackup.m_gameTime, false );
	}

	// Force the LOD level in the render scene
	( new CRenderCommand_ChangeSceneForcedLOD( m_world->GetRenderSceneEx(), -1 ) )->Commit();


	( new CRenderCommand_UpdateTerrainScreenSpaceErrorThreshold( m_terrain->GetTerrainProxy(), -1.0f ) )->Commit();
}

CRenderFrame* CMinimapGenerator::PrepareRenderFrame( enum EMinimapMask mask, const Vector& cameraPos, Float tileOffset )
{
	// Prepare frame info for rendering
	CRenderFrameInfo frameInfo( m_viewport.Get() );

	// Set the camera to orthographic view from above
	CRenderCamera& renderCam = frameInfo.m_camera;
	renderCam.Set( cameraPos, EulerAngles( 0, 270, 0 ), 0.0f, 1, renderCam.GetNearPlane(), renderCam.GetFarPlane(), m_tileSize + ( 2*tileOffset ) );

	frameInfo.m_occlusionCamera = renderCam;

	// set default settings
	SetDefaultSettingsForRenderFrame( &frameInfo );

	// Set special settings for active mask
	SetRenderFrameSettingsForActiveMask( &frameInfo, mask );

	// Create render frame from render frame info
	CRenderFrame* frame = m_world->GenerateFrame( m_viewport.Get() , frameInfo );
	if( frame == nullptr )
	{
		RED_HALT( "Failed to create the first frame for minimap screenshot, this will crash" );
	}

	// Set special settings which were wrong modificated in GenerateFrame function
	SetRenderFrameSettingsAfterGenerateFrame( &frame->GetFrameInfo() );

	return frame;
}

void CMinimapGenerator::SetDefaultSettingsForRenderFrame( CRenderFrameInfo* frameInfo )
{
	frameInfo->m_renderingMode = RM_Shaded;
	frameInfo->m_present = true;
	frameInfo->m_instantAdaptation = true;
	frameInfo->m_baseLightingParameters.m_hdrAdaptationDisabled = true;
	frameInfo->SetMaterialDebugMode( MDM_None );

	frameInfo->m_envParametersGame.m_displaySettings.m_enableInstantAdaptation = false;
	frameInfo->m_envParametersGame.m_displaySettings.m_enableGlobalLightingTrajectory = false;
	frameInfo->m_envParametersGame.m_displaySettings.m_enableEnvProbeInstantUpdate = false;
	frameInfo->m_envParametersGame.m_displaySettings.m_allowEnvProbeUpdate = false;
	frameInfo->m_envParametersGame.m_displaySettings.m_allowBloom = false;
	frameInfo->m_envParametersGame.m_displaySettings.m_allowColorMod = false;
	frameInfo->m_envParametersGame.m_displaySettings.m_allowAntialiasing = false;
	frameInfo->m_envParametersGame.m_displaySettings.m_allowGlobalFog = false;
	frameInfo->m_envParametersGame.m_displaySettings.m_allowDOF = false;
	frameInfo->m_envParametersGame.m_displaySettings.m_allowSSAO = false;
	frameInfo->m_envParametersGame.m_displaySettings.m_allowCloudsShadow = false;
	frameInfo->m_envParametersGame.m_displaySettings.m_allowVignette = false;
	frameInfo->m_envParametersGame.m_displaySettings.m_disableTonemapping = true;
	frameInfo->m_envParametersGame.m_displaySettings.m_forceCutsceneDofMode = false;
	frameInfo->m_envParametersGame.m_displaySettings.m_allowWaterShader = true;
	frameInfo->m_envParametersGame.m_displaySettings.m_displayMode = EMM_None;	
	frameInfo->m_envParametersGame.m_dayCycleOverride.m_fakeDayCycleEnable = true;
	frameInfo->m_envParametersGame.m_dayCycleOverride.m_fakeDayCycleHour = 12.0f;

	frameInfo->m_envParametersArea.m_sky.m_activated = true;
	frameInfo->m_envParametersArea.m_sky.m_activatedActivateFactor = true;
	frameInfo->m_envParametersArea.m_sky.m_activateFactor = 0.0f;

	//frameInfo->SetShowFlag( SHOW_Antialias, false );
	frameInfo->SetShowFlag( SHOW_PreferTemporalAA, false );
	//frameInfo->SetShowFlag( SHOW_Bloom, false );
	//frameInfo->SetShowFlag( SHOW_Blur, false );
	frameInfo->SetShowFlag( SHOW_DepthOfField, false );
	//frameInfo->SetShowFlag( SHOW_DOF, false );
	frameInfo->SetShowFlag( SHOW_Flares, false );
	frameInfo->SetShowFlag( SHOW_Particles, false );
	frameInfo->SetShowFlag( SHOW_PostProcess, false );
	frameInfo->SetShowFlag( SHOW_Selection, false );
	frameInfo->SetShowFlag( SHOW_Gizmo, false );
	//frameInfo->SetShowFlag( SHOW_Sharpen, false );
	frameInfo->SetShowFlag( SHOW_Spawnset, false );
	//frameInfo->SetShowFlag( SHOW_SSAO, false );
	frameInfo->SetShowFlag( SHOW_Stickers, false );
	frameInfo->SetShowFlag( SHOW_Shadows, false );
	frameInfo->SetShowFlag( SHOW_SpeedTreeShadows, false );
	frameInfo->SetShowFlag( SHOW_AllowApexShadows, false );
	//frameInfo->SetShowFlag( SHOW_Vignette, false );
	frameInfo->SetShowFlag( SHOW_Wireframe, false );
	frameInfo->SetShowFlag( SHOW_VisualDebug, false );
	frameInfo->SetShowFlag( SHOW_NavMesh, false );
	frameInfo->SetShowFlag( SHOW_NavMeshOverlay, false );
	frameInfo->SetShowFlag( SHOW_NavTerrain, false );
	frameInfo->SetShowFlag( SHOW_NavTerrainHeight, false );
	frameInfo->SetShowFlag( SHOW_GenericGrass, false );

	//
	frameInfo->SetShowFlag( SHOW_Foliage, true );
	frameInfo->SetShowFlag( SHOW_Lighting, true );
	frameInfo->SetShowFlag( SHOW_Meshes, true );
	frameInfo->SetShowFlag( SHOW_Terrain, true );
	frameInfo->SetShowFlag( SHOW_RenderDeepTerrain, true );
	frameInfo->SetShowFlag( SHOW_Refraction, true );
	frameInfo->SetShowFlag( SHOW_Stripes, true );
	frameInfo->SetShowFlag( SHOW_Decals, true );
	frameInfo->SetShowFlag( SHOW_Apex, true );

	// set default environment parameters
	CGameEnvironmentParams& params = m_world->GetEnvironmentManager()->GetGameEnvironmentParams();
	params.m_displaySettings.m_enableInstantAdaptation = false;
	params.m_displaySettings.m_enableGlobalLightingTrajectory = false;
	params.m_displaySettings.m_enableEnvProbeInstantUpdate = false;
	params.m_displaySettings.m_allowEnvProbeUpdate = false;
	params.m_displaySettings.m_allowBloom = false;
	params.m_displaySettings.m_allowColorMod = false;
	params.m_displaySettings.m_allowAntialiasing = false;
	params.m_displaySettings.m_allowGlobalFog = false;
	params.m_displaySettings.m_allowDOF = false;
	params.m_displaySettings.m_allowSSAO = false;
	params.m_displaySettings.m_allowCloudsShadow = false;
	params.m_displaySettings.m_allowVignette = false;
	params.m_displaySettings.m_disableTonemapping = true;
	params.m_displaySettings.m_forceCutsceneDofMode = false;
	params.m_displaySettings.m_allowWaterShader = true;
	params.m_displaySettings.m_displayMode = EMM_None;	
	params.m_dayCycleOverride.m_fakeDayCycleEnable = true;
	params.m_dayCycleOverride.m_fakeDayCycleHour = 12.0f;

	m_world->GetFoliageEditionController().SetDebugVisualisationMode( VISUALISE_NONE );

	// Force the LOD level in the render scene
	( new CRenderCommand_ChangeSceneForcedLOD( m_world->GetRenderSceneEx(), 0 ) )->Commit();

	//
	( new CRenderCommand_UpdateTerrainScreenSpaceErrorThreshold( m_terrain->GetTerrainProxy(), 0 ) )->Commit();
}

void CMinimapGenerator::SetRenderFrameSettingsForActiveMask( CRenderFrameInfo* frameInfo, enum EMinimapMask mask )
{
	CGameEnvironmentParams& params = m_world->GetEnvironmentManager()->GetGameEnvironmentParams();

	switch( mask )
	{
	case MM_Full:
		params.m_displaySettings.m_displayMode = EMM_None;
		frameInfo->m_envParametersGame.m_displaySettings.m_displayMode = EMM_None;
		break;
	case MM_Albedo:
		params.m_displaySettings.m_displayMode = EMM_GBuffAlbedo;
		frameInfo->m_envParametersGame.m_displaySettings.m_displayMode = EMM_GBuffAlbedo;
		break;
	case MM_Normals:
		params.m_displaySettings.m_displayMode = EMM_GBuffNormalsWorldSpace;
		frameInfo->m_envParametersGame.m_displaySettings.m_displayMode = EMM_GBuffNormalsWorldSpace;
		break;
	case MM_NormalsNoTrees:
		params.m_displaySettings.m_displayMode = EMM_GBuffNormalsWorldSpace;
		frameInfo->m_envParametersGame.m_displaySettings.m_displayMode = EMM_GBuffNormalsWorldSpace;
		frameInfo->SetShowFlag( SHOW_Foliage, false );
		break;
	case MM_Volumes:
		params.m_displaySettings.m_displayMode = EMM_InteriorsVolume;
		frameInfo->m_envParametersGame.m_displaySettings.m_displayMode = EMM_InteriorsVolume;
		break;
	case MM_Water:
		frameInfo->SetShowFlag( SHOW_Foliage, false );
		frameInfo->SetShowFlag( SHOW_Lighting, false );
		frameInfo->SetShowFlag( SHOW_Meshes, true );
		frameInfo->SetShowFlag( SHOW_ReversedProjection, false );
		frameInfo->SetShowFlag( SHOW_Shadows, false );
		frameInfo->SetShowFlag( SHOW_SpeedTreeShadows, false );
		frameInfo->SetShowFlag( SHOW_AllowApexShadows, false );
		frameInfo->SetShowFlag( SHOW_Terrain, true );
		frameInfo->SetShowFlag( SHOW_Refraction, true );
		frameInfo->SetShowFlag( SHOW_Stripes, false );
		frameInfo->SetShowFlag( SHOW_Decals, false );
		frameInfo->SetShowFlag( SHOW_Apex, false );

		frameInfo->SetMaterialDebugMode( MDM_WaterMode );
		break;
	case MM_Foliage:
		frameInfo->SetShowFlag( SHOW_Foliage, true );
		frameInfo->SetShowFlag( SHOW_Lighting, true );
		frameInfo->SetShowFlag( SHOW_Meshes, false );
		frameInfo->SetShowFlag( SHOW_ReversedProjection, false );
		frameInfo->SetShowFlag( SHOW_Shadows, false );
		frameInfo->SetShowFlag( SHOW_SpeedTreeShadows, false );
		frameInfo->SetShowFlag( SHOW_AllowApexShadows, false );
		frameInfo->SetShowFlag( SHOW_Terrain, false );
		frameInfo->SetShowFlag( SHOW_Refraction, false );
		frameInfo->SetShowFlag( SHOW_Stripes, false );
		frameInfo->SetShowFlag( SHOW_Decals, false );
		frameInfo->SetShowFlag( SHOW_Apex, false );

		params.m_displaySettings.m_allowWaterShader = false;

		frameInfo->m_envParametersGame.m_displaySettings.m_allowWaterShader = false;
		break;
	case MM_Objects:
		frameInfo->SetShowFlag( SHOW_Foliage, false );
		frameInfo->SetShowFlag( SHOW_Lighting, true );
		frameInfo->SetShowFlag( SHOW_Meshes, true );
		frameInfo->SetShowFlag( SHOW_ReversedProjection, false );
		frameInfo->SetShowFlag( SHOW_Shadows, false );
		frameInfo->SetShowFlag( SHOW_SpeedTreeShadows, false );
		frameInfo->SetShowFlag( SHOW_AllowApexShadows, false );
		frameInfo->SetShowFlag( SHOW_Terrain, false );
		frameInfo->SetShowFlag( SHOW_Refraction, false );
		frameInfo->SetShowFlag( SHOW_Stripes, false );
		frameInfo->SetShowFlag( SHOW_Decals, false );

		params.m_displaySettings.m_allowWaterShader = false;

		frameInfo->m_envParametersGame.m_displaySettings.m_allowWaterShader = false;
		break;
	case MM_Heightmap:
		frameInfo->SetShowFlag( SHOW_Foliage, false );
		frameInfo->SetShowFlag( SHOW_Lighting, false );
		frameInfo->SetShowFlag( SHOW_Meshes, false );
		frameInfo->SetShowFlag( SHOW_ReversedProjection, false );
		frameInfo->SetShowFlag( SHOW_Shadows, false );
		frameInfo->SetShowFlag( SHOW_SpeedTreeShadows, false );
		frameInfo->SetShowFlag( SHOW_AllowApexShadows, false );
		frameInfo->SetShowFlag( SHOW_Terrain, true );
		frameInfo->SetShowFlag( SHOW_RenderDeepTerrain, true );
		frameInfo->SetShowFlag( SHOW_Refraction, false );
		frameInfo->SetShowFlag( SHOW_Stripes, false );
		frameInfo->SetShowFlag( SHOW_Decals, false );
		frameInfo->SetShowFlag( SHOW_Apex, false );

		params.m_displaySettings.m_allowWaterShader = false;

		frameInfo->m_envParametersGame.m_displaySettings.m_allowWaterShader = false;
		frameInfo->SetMaterialDebugMode( MDM_Heightmap );

		break;
	case MM_Terrain:
		frameInfo->SetShowFlag( SHOW_Foliage, false );
		frameInfo->SetShowFlag( SHOW_Lighting, false );
		frameInfo->SetShowFlag( SHOW_Meshes, false );
		frameInfo->SetShowFlag( SHOW_ReversedProjection, false );
		frameInfo->SetShowFlag( SHOW_Shadows, false );
		frameInfo->SetShowFlag( SHOW_SpeedTreeShadows, false );
		frameInfo->SetShowFlag( SHOW_AllowApexShadows, false );
		frameInfo->SetShowFlag( SHOW_Terrain, true );
		frameInfo->SetShowFlag( SHOW_RenderDeepTerrain, true );
		frameInfo->SetShowFlag( SHOW_Refraction, false );
		frameInfo->SetShowFlag( SHOW_Stripes, false );
		frameInfo->SetShowFlag( SHOW_Decals, false );
		frameInfo->SetShowFlag( SHOW_Apex, false );

		params.m_displaySettings.m_allowWaterShader = false;

		frameInfo->m_envParametersGame.m_displaySettings.m_allowWaterShader = false;
		break;
	case MM_Roads:
		frameInfo->SetShowFlag( SHOW_Foliage, false );
		frameInfo->SetShowFlag( SHOW_Lighting, false );
		frameInfo->SetShowFlag( SHOW_Meshes, false );
		frameInfo->SetShowFlag( SHOW_ReversedProjection, false );
		frameInfo->SetShowFlag( SHOW_Shadows, false );
		frameInfo->SetShowFlag( SHOW_SpeedTreeShadows, false );
		frameInfo->SetShowFlag( SHOW_AllowApexShadows, false );
		frameInfo->SetShowFlag( SHOW_Terrain, true );
		frameInfo->SetShowFlag( SHOW_Refraction, false );
		frameInfo->SetShowFlag( SHOW_Stripes, true );
		frameInfo->SetShowFlag( SHOW_Decals, false );
		frameInfo->SetShowFlag( SHOW_Apex, false );

		frameInfo->SetMaterialDebugMode( MDM_WaterMode );

		params.m_displaySettings.m_allowWaterShader = false;

		frameInfo->m_envParametersGame.m_displaySettings.m_allowWaterShader = false;
		break;
	case MM_Bridges:
		SetRenderFrameSettingsForActiveMask( frameInfo, MM_Objects );
		break;
	case MM_DEBUG_GrassInstanceHeatMap:
		SetRenderFrameSettingsForActiveMask( frameInfo, MM_Terrain );

		params.m_displaySettings.m_displayMode = EMM_GBuffAlbedo;
		frameInfo->m_envParametersGame.m_displaySettings.m_displayMode = EMM_GBuffAlbedo;

		m_world->GetFoliageEditionController().SetDebugVisualisationMode( VISUALISE_GRASSINSTANCES );
		break;
	case MM_DEBUG_TreeInstanceHeatMap:
		SetRenderFrameSettingsForActiveMask( frameInfo, MM_Terrain );

		params.m_displaySettings.m_displayMode = EMM_GBuffAlbedo;
		frameInfo->m_envParametersGame.m_displaySettings.m_displayMode = EMM_GBuffAlbedo;

		m_world->GetFoliageEditionController().SetDebugVisualisationMode( VISUALISE_TREEINSTANCES );
		break;
	case MM_DEBUG_GrassLayerHeatMap:
		SetRenderFrameSettingsForActiveMask( frameInfo, MM_Terrain );

		params.m_displaySettings.m_displayMode = EMM_GBuffAlbedo;
		frameInfo->m_envParametersGame.m_displaySettings.m_displayMode = EMM_GBuffAlbedo;

		m_world->GetFoliageEditionController().SetDebugVisualisationMode( VISUALISE_GRASSLAYERS );
		break;
	}
}

void CMinimapGenerator::SetRenderFrameSettingsAfterGenerateFrame( CRenderFrameInfo* frameInfo )
{
	frameInfo->m_envParametersArea.m_sky.m_activated = true;
	frameInfo->m_envParametersArea.m_sky.m_activatedActivateFactor = true;
	frameInfo->m_envParametersArea.m_sky.m_activateFactor = 0.0f;

	// Illuminate two times larger area than the rendered tile
	Float tileOffset = (Float)m_settings.m_exteriors.m_imageOffset;
	frameInfo->m_envParametersArea.m_globalLight.m_sunColorCenterArea.SetValueScalar( 2*( m_tileSize + ( 2*tileOffset ) ) );
}

void CMinimapGenerator::CountTilesToRender()
{
	// Obtain clipmap parameters
	SClipmapParameters params;
	m_terrain->GetClipmapParameters( &params );
	Int32 tiles = params.clipmapSize/params.tileRes;
	m_tileSize = params.terrainSize/(Float)tiles;
	m_highestElevation = params.highestElevation;

	// Obtain camera position if the tile selection is not "all tiles"
	if( m_settings.m_generationMode != GM_AllTiles )
	{
		m_terrain->GetTileFromPosition( m_settings.m_exteriors.m_cameraPosition, m_tileStartX, m_tileStartY );
	}

	// Fill the area rect
	if( m_settings.m_generationMode == GM_CurrentTile )
	{
		m_tileEndX = m_tileStartX + 1;
		m_tileEndY = m_tileStartY + 1;
	}
	else if( m_settings.m_generationMode == GM_CurrentTilePlus )
	{
		Int32 tileCountPlus = (Int32)m_settings.m_tileCountPlus;
		m_tileEndX = Clamp( m_tileStartX + tileCountPlus, 0, tiles ) + 1;
		m_tileEndY = Clamp( m_tileStartY + tileCountPlus, 0, tiles ) + 1;
		m_tileStartX = Clamp( m_tileStartX - tileCountPlus, 0, tiles );
		m_tileStartY = Clamp( m_tileStartY - tileCountPlus, 0, tiles );
	}
	else if( m_settings.m_generationMode == GM_TileRange )
	{
		m_tileEndX = Clamp( (Int32)m_settings.m_tileRange.Max.X, 0, tiles ) + 1;
		m_tileEndY = Clamp( (Int32)m_settings.m_tileRange.Max.Y, 0, tiles ) + 1;
		m_tileStartX = Clamp( (Int32)m_settings.m_tileRange.Min.X, 0, tiles );
		m_tileStartY = Clamp( (Int32)m_settings.m_tileRange.Min.Y, 0, tiles );
	}
	else if( m_settings.m_generationMode == GM_AllTiles )
	{
		m_tileStartX = m_tileStartY = 0;
		m_tileEndX = tiles;
		m_tileEndY = tiles;
	}

	Uint32 scale = m_settings.m_exteriors.m_imageZoom;
	m_tileSize /= scale;
	m_tileStartX *= scale;
	m_tileStartY *= scale;
	m_tileEndX *= scale;
	m_tileEndY *= scale;
}

String CMinimapGenerator::CreatePathForTileImage( EMinimapMask maskType, Int32 x, Int32 y )
{
	String prefix = ToString( maskType );
	String fileNamePrefix = m_settings.m_exteriors.m_fileNamePrefix;

	// get world name
	CFilePath worldPath( m_world->GetFriendlyName() );
	String worldName = worldPath.GetFileName();

	// get destination directory
	String descDir = m_settings.m_outputDir;

	// set empty string as a result dir
	String fullPath = String::EMPTY;
	String dirPath = String::EMPTY;

	// choose directories layout
	if( m_settings.m_exteriors.m_dirLayout == DL_Photoshop )
	{
		dirPath = String::Printf( TXT("%s%s\\exterior\\%s%ix%i\\"), descDir.AsChar(), worldName.AsChar(), fileNamePrefix.AsChar(), x, y );
		fullPath = dirPath +  prefix + TXT(".png");
	}
	else
	{
		dirPath = String::Printf( TXT("%s%s\\exterior\\%s\\"), descDir.AsChar(), worldName.AsChar(), prefix.AsChar() );
		fullPath = String::Printf( TXT("%s%s%ix%i.png"), dirPath.AsChar(), fileNamePrefix.AsChar(), x, y );
	}

	// Create directories
	GFileManager->CreatePath( dirPath );

	return fullPath;
}

String CMinimapGenerator::CreatePathForInteriorImage( const String& caveName )
{
	// get world name
	CFilePath worldPath( m_world->GetFriendlyName() );
	String worldName = worldPath.GetFileName();

	// get destination directory
	String descDir = m_settings.m_outputDir;

	//
	String dirPath = String::Printf( TXT("%s%s\\interior\\"), descDir.AsChar(), worldName.AsChar() );
	String fullPath = String::Printf( TXT("%s%s.png"), dirPath.AsChar(), caveName.AsChar() );

	// Create directories
	GFileManager->CreatePath( dirPath );

	return fullPath;
}

void CMinimapGenerator::InternalGenerateExteriors()
{
#if defined(RED_PLATFORM_WINPC) && !defined(NO_EDITOR) && !defined(RED_FINAL_BUILD)
	RED_LOG( RED_LOG_CHANNEL(MinimapGenerator), TXT("Minimap exterior generation begin") );
	GFeedback->BeginTask( TXT("Mini maps generation ..."), true );
	Uint32 allTileCount = ( m_tileEndX - m_tileStartX ) * ( m_tileEndY - m_tileStartY );

	Int32 continueFromX = m_tileStartX;
	Int32 continueFromY = m_tileStartY;
	if( m_settings.m_exteriors.m_continueMode == true )
	{
		if( CheckUnsuccessfulFile( continueFromX, continueFromY ) == true )
		{
			RED_LOG( RED_LOG_CHANNEL(MinimapGenerator), String::Printf( TXT("Continue generation from tile: %ix%i"), continueFromX, continueFromY ).AsChar() );
		}
	}

	//
	Int32 initializeX = m_tileStartX;
	Int32 initializeY = m_tileStartY;
	if( m_settings.m_exteriors.m_continueMode == true )
	{
		initializeX = continueFromX;
		initializeY = continueFromY;
	}

	// iterate all tiles
	for( Int32 y=initializeY; y<m_tileEndY; ++y )
	{
		if( m_settings.m_exteriors.m_continueMode == true && y != initializeY )
		{
			initializeX = m_tileStartX;
		}

		for( Int32 x=initializeX; x<m_tileEndX; ++x )
		{
			SaveUnsuccessfulFile( x, y );

			if( GFeedback->IsTaskCanceled() == true )
			{
				RemoveUnsuccessfulFile();
				GFeedback->EndTask();
				return;
			}

			// update progress
			const Uint32 progressValue = ( y - m_tileStartY ) * ( m_tileEndY - m_tileStartY ) + ( x - m_tileStartX ) + 1;
			GFeedback->UpdateTaskProgress( progressValue, allTileCount );
			GFeedback->UpdateTaskInfo( String::Printf( TXT("Generating tile %i from %i"), progressValue, allTileCount ).AsChar() );
			GSplash->UpdateProgress( TXT("Generating tile %i from %i"), progressValue, allTileCount );

			// set mask progression
			Uint32 maskProgress = 0;

			// interate all selected masks
			for( Uint32 i=0; i<MM_Count; ++i )
			{
				if( m_settings.m_exteriors.m_enabledMasks[i] == true )
				{
					EMinimapMask activeMask = static_cast< EMinimapMask >( i );

					// Merge file path and create need directories
					String fullPath = CreatePathForTileImage( activeMask, x, y );

					// Get tile offset
					Float tileOffset = (Float)m_settings.m_exteriors.m_imageOffset;

					// Calculate camera position
					Vector cameraPos = m_terrain->GetTerrainCorner() + Vector( x*m_tileSize + m_tileSize*0.5f - tileOffset, y*m_tileSize + m_tileSize*0.5f - tileOffset, m_highestElevation + 1 );	// 1 meter is buffer for camera

					// Prepare render frame
					CRenderFrame* frame = PrepareRenderFrame( activeMask, cameraPos, tileOffset );

					// Update the camera
					Vector cameraStreamingPos = cameraPos;
					m_world->GetTerrain()->GetHeightForWorldPositionSync( cameraPos, 0, cameraStreamingPos.Z );
					m_world->UpdateCameraPosition( cameraStreamingPos );
					m_world->SetStreamingReferencePosition( cameraStreamingPos );
					m_world->GetStreamingSectorData()->UpdateStreaming( true, true );
					m_world->GetSectorDataStreaming()->BeginStreaming();
					m_world->GetSectorDataStreaming()->FinishStreaming();
					m_world->ForceFinishAsyncResourceLoads();

					CClipMap* terrainClipMap = m_world->GetTerrain();
					SClipMapUpdateInfo clipMapUpdateInfo;
					clipMapUpdateInfo.m_viewerPosition = cameraStreamingPos;
					clipMapUpdateInfo.m_timeDelta = 0.1f;
					terrainClipMap->Update(clipMapUpdateInfo);

					m_world->EnableStreaming( false );
					m_world->UpdateCameraPosition( cameraPos );

					// update world before render it
					m_world->UpdateLoadingState( true );

					while( m_world->HasPendingStreamingTasks() == true )
					{
						Sleep(1);
					}

					// update foliage before render it
					CFoliageEditionController& controller = m_world->GetFoliageEditionController();
					controller.WaitUntilAllFoliageResourceLoaded();

					// Manages visibility of objects which are rendered depending on the mask
					PrepareObjects( activeMask );

					// update world before render it
					m_world->UpdateLoadingState( true );

					// Render tile
					m_world->RenderWorld( frame );

					// Relese render frame
					frame->Release();

					// Flush screen
					GRender->Flush();

					// Save rendered image to file on disk
					SScreenshotParameters parameters( m_viewportSize, m_viewportSize, fullPath, SF_PNG, 4, 90.0f, SRF_PlainScreenshot );
					parameters.m_noWatermark = true;
					SScreenshotSystem::GetInstance().TakeSimpleScreenshotNow( parameters, SCF_SaveToDisk | SCF_UseProvidedFilename );

					m_world->EnableStreaming( true );
				}
			}
		}
	}

	RevertObjectsVisibility();
	RemoveUnsuccessfulFile();
	GFeedback->EndTask();
	RED_LOG( RED_LOG_CHANNEL(MinimapGenerator), TXT("Minimap exterior generation end") );
#endif
}

void CMinimapGenerator::PrepareObjects( EMinimapMask mask )
{
	RevertObjectsVisibility();

	// show or hide objects depending on the mask
	switch( mask )
	{
 	case MM_Water:
 		FilterAppropriateObjects( TXT("minimapwaterrender"), true );
 		break;
	case MM_Objects:
		FilterAppropriateObjects( TXT("minimapmeshrender"), true );
		break;
	case MM_Bridges:
		FilterAppropriateObjects( TXT("minimapbridgerender"), true );
		break;
	case MM_Roads:
#ifndef NO_EDITOR
		for( WorldAttachedComponentsIterator it( GGame->GetActiveWorld() ); it; ++it )
		{
			CStripeComponent* component = Cast< CStripeComponent >( *it );
			if( component != nullptr )
			{
				component->SetProjectToTerrain( false );
			}
		}
#endif// NO_EDITOR
		break;
 	default:
		FilterAppropriateObjects( TXT("minimaptotallynottorender"), false );
 		break;
	}
}

void CMinimapGenerator::FilterAppropriateObjects( const String& tagName, Bool shouldBeVisible )
{
	for( WorldAttachedComponentsIterator it( GGame->GetActiveWorld() ); it; ++it )
	{
		CDrawableComponent* component = Cast< CDrawableComponent >( *it );
		if( component != nullptr )
		{
			CEntity* entity = component->GetEntity();
			const TagList& entityTags = entity->GetTags();
			const TagList& componentTags = component->GetTags();
			if( entityTags.HasTag( CName( tagName ) ) ==  true || componentTags.HasTag( CName( tagName ) ) ==  true )
			{
				component->ToggleVisibility( shouldBeVisible );
			}
			else
			{
				component->ToggleVisibility( !shouldBeVisible );
			}
		}
	}
}

void CMinimapGenerator::SaveUnsuccessfulFile( Int32 x, Int32 y )
{
	// get world name
	CFilePath worldPath( m_world->GetFriendlyName() );
	const String worldName = worldPath.GetFileName();
	const String unsuccessfulFileDir =  String::Printf( TXT("%s%s\\"), m_settings.m_outputDir.AsChar(), worldName.AsChar() );
	const String unsuccessfulFilePath = String::Printf( TXT("%s%s.unsuccessfulgeneration"), unsuccessfulFileDir.AsChar(), worldName.AsChar() );

	// Create directories
	GFileManager->CreatePath( unsuccessfulFileDir );

	// Create file
	Uint32 helperSizer = 0;
	Red::IO::CNativeFileHandle unsuccessfulFile;
	if( unsuccessfulFile.Open( unsuccessfulFilePath.AsChar(), Red::IO::eOpenFlag_WriteNew ) == false )
	{
		RED_LOG_ERROR( MinimapGenerator, TXT("Cannot open file: %s"), unsuccessfulFilePath.AsChar() );
		return;
	}

	// write info to file
	String text = String::Printf( TXT("%i;%i\n"), x, y );
	unsuccessfulFile.Write( text.AsChar(), text.GetLength() * sizeof( Char ), helperSizer );
	if( helperSizer < text.Size() )
	{
		RED_LOG_WARNING( MinimapGenerator, TXT("Cannot write all data to file: %s"), unsuccessfulFilePath.AsChar() );
	}

	// close file
	if( unsuccessfulFile.Flush() == false )
	{
		RED_LOG_ERROR( MinimapGenerator, TXT("Cannot flush file: %s"), unsuccessfulFilePath.AsChar() );
	}

	if( unsuccessfulFile.Close() == false )
	{
		RED_LOG_ERROR( MinimapGenerator, TXT("Cannot close file: %s"), unsuccessfulFilePath.AsChar() );
	}
}

void CMinimapGenerator::RemoveUnsuccessfulFile()
{
	// get world name
	CFilePath worldPath( m_world->GetFriendlyName() );
	const String worldName = worldPath.GetFileName();
	const String unsuccessfulFileDir =  String::Printf( TXT("%s%s\\"), m_settings.m_outputDir.AsChar(), worldName.AsChar() );
	const String unsuccessfulFilePath = String::Printf( TXT("%s%s.unsuccessfulgeneration"), unsuccessfulFileDir.AsChar(), worldName.AsChar() );

	if( GFileManager->FileExist( unsuccessfulFilePath ) == true )
	{
		GFileManager->DeleteFile( unsuccessfulFilePath );
	}
}

Bool CMinimapGenerator::CheckUnsuccessfulFile( Int32& x, Int32& y )
{
	// get world name
	CFilePath worldPath( m_world->GetFriendlyName() );
	const String worldName = worldPath.GetFileName();
	const String unsuccessfulFileDir =  String::Printf( TXT("%s%s\\"), m_settings.m_outputDir.AsChar(), worldName.AsChar() );
	const String unsuccessfulFilePath = String::Printf( TXT("%s%s.unsuccessfulgeneration"), unsuccessfulFileDir.AsChar(), worldName.AsChar() );

	if( GFileManager->FileExist( unsuccessfulFilePath ) == true )
	{
		// Create file
		Uint32 helperSizer = 0;
		Red::IO::CNativeFileHandle unsuccessfulFile;
		if( unsuccessfulFile.Open( unsuccessfulFilePath.AsChar(), Red::IO::eOpenFlag_Read ) == false )
		{
			RED_LOG_ERROR( MinimapGenerator, TXT("Cannot open file: %s"), unsuccessfulFilePath.AsChar() );
			return false;
		}

		// read info from file
		Char textBuffer[256];
		unsuccessfulFile.Read( &textBuffer, sizeof(textBuffer), helperSizer );
		String text = textBuffer;
		TDynArray< String > tokens = text.Split( TXT(";") );
		if( tokens.Size() != 2 )
		{
			RED_LOG_ERROR( MinimapGenerator, TXT("Wrong file format") );

			// close file
			if( unsuccessfulFile.Close() == false )
			{
				RED_LOG_ERROR( MinimapGenerator, TXT("Cannot close file: %s"), unsuccessfulFilePath.AsChar() );
			}

			return false;
		}

		// convert from string to integer
		FromString< Int32 >( tokens[0], x );
		FromString< Int32 >( tokens[1], y );

		// close file
		if( unsuccessfulFile.Close() == false )
		{
			RED_LOG_ERROR( MinimapGenerator, TXT("Cannot close file: %s"), unsuccessfulFilePath.AsChar() );
		}

		return true;
	}

	return false;
}

Bool CMinimapGenerator::IsUnsuccessfulFileExist( const String& destinationDir, Int32& x, Int32& y )
{
	m_settings.m_outputDir = destinationDir;
	return CheckUnsuccessfulFile( x, y );
}

void CMinimapGenerator::GenerateInteriors()
{
	RememberEngineSettings();
	SetNewEngineSettings();

	CountTilesToRender();

	InternalGenerateInteriors();

	RevertEngineSettings();
}

CRenderFrame* CMinimapGenerator::PrepareRenderFrameForInterior( const Vector& camPos, Float zoom )
{
	// Prepare frame info for rendering
	CRenderFrameInfo frameInfo( m_viewport.Get()  );

	// Set the camera to orthographic view from above
	CRenderCamera& renderCam = frameInfo.m_camera;
	renderCam.Set( camPos, EulerAngles( 0, 270, 0 ), 0.0f, 1, renderCam.GetNearPlane(), renderCam.GetFarPlane(), zoom );

	// set default settings
	SetDefaultSettingsForRenderFrame( &frameInfo );

	// set navmesh flags
	frameInfo.SetShowFlag( SHOW_Foliage, false );
	frameInfo.SetShowFlag( SHOW_Lighting, false );
	frameInfo.SetShowFlag( SHOW_Meshes, false );
	frameInfo.SetShowFlag( SHOW_ReversedProjection, false );
	frameInfo.SetShowFlag( SHOW_Shadows, false );
	frameInfo.SetShowFlag( SHOW_SpeedTreeShadows, false );
	frameInfo.SetShowFlag( SHOW_Terrain, false );
	frameInfo.SetShowFlag( SHOW_Refraction, false );
	frameInfo.SetShowFlag( SHOW_Stripes, false );
	frameInfo.SetShowFlag( SHOW_Decals, false );
	frameInfo.SetShowFlag( SHOW_NavMesh, false );
	frameInfo.SetShowFlag( SHOW_NavMeshOverlay, false );
	frameInfo.SetShowFlag( SHOW_Apex, false );

	// Create render frame from render frame info
	CRenderFrame* frame = m_world->GenerateFrame( m_viewport.Get() , frameInfo );
	//CRenderFrame* frame = GRender->CreateFrame( nullptr, frameInfo );
	if( frame == nullptr )
	{
		RED_HALT( "Failed to create the first frame for minimap screenshot, this will crash" );
	}

	// Set special settings which were wrong modificated in GenerateFrame function
	if ( frame )
	{
		SetRenderFrameSettingsAfterGenerateFrame( &frame->GetFrameInfo() );
	}

	return frame;
}

void CMinimapGenerator::InternalGenerateInteriors()
{
#if defined(RED_PLATFORM_WINPC) && !defined(NO_EDITOR) && !defined(RED_FINAL_BUILD)
	GFeedback->BeginTask( TXT("Mini maps interiors generation ..."), true );

	// clear error messages
	m_interiorsErrors.ClearFast();

	Uint32 allTileCount = ( m_tileEndX - m_tileStartX ) * ( m_tileEndY - m_tileStartY );

	// processed entities
	TDynArray< String > processedInteriorComponents;

	// iterate all tiles
	for( Int32 y=m_tileStartY; y<m_tileEndY; ++y )
	{
		for( Int32 x=m_tileStartX; x<m_tileEndX; ++x )
		{
			if( GFeedback->IsTaskCanceled() == true )
			{
				GFeedback->EndTask();
				return;
			}

			// update progress
			const Uint32 progressValue = ( y - m_tileStartY ) * ( m_tileEndY - m_tileStartY ) + ( x - m_tileStartX ) + 1;
			GFeedback->UpdateTaskProgress( progressValue, allTileCount );
			GFeedback->UpdateTaskInfo( String::Printf( TXT("Generating tile %i from %i"), progressValue, allTileCount ).AsChar() );

			// Calculate camera position
			Vector cameraPos = m_terrain->GetTerrainCorner() + Vector( x*m_tileSize + m_tileSize*0.5f, y*m_tileSize + m_tileSize*0.5f, m_highestElevation + 1 );	// 1 meter is buffer for camera

			// Update the camera
			m_world->UpdateCameraPosition( cameraPos );
			m_world->SetStreamingReferencePosition( cameraPos );

			// update world before render it
			m_world->UpdateLoadingState( true );

			// find all interior area trigger
			for( WorldAttachedComponentsIterator it( m_world ); it; ++it )
			{
				CComponent* component = ( *it );

				CEntity* parentEntity = component->GetEntity();
				Vector parentPosition = parentEntity->GetPosition();
				Int32 parentX = (Int32)((parentPosition.X - m_terrain->GetTerrainCorner().X ) / m_tileSize );
				Int32 parentY = (Int32)((parentPosition.Y - m_terrain->GetTerrainCorner().Y ) / m_tileSize );

				if( x != parentX || y != parentY )
				{
					continue;
				}

				CR4InteriorAreaComponent* interiorComponent = Cast< CR4InteriorAreaComponent >( component );
				if( interiorComponent == nullptr )
				{
					continue;
				}

				String name = interiorComponent->GetTexture();
				if( name.Empty() == true )
				{
					CR4InteriorAreaEntity* interiorEntity = Cast< CR4InteriorAreaEntity >( parentEntity );
					if( interiorEntity != nullptr )
					{
						name = interiorEntity->GetTexture();
						if( name.Empty() == true )
						{
							m_interiorsErrors += String::Printf( TXT("Interior component in \"%s\" entity  does not have name in \"Texture\" property. Map for this components won't be generated. \n"), parentEntity->GetName().AsChar() );
							continue;
						}
					}
					else
					{
						m_interiorsErrors += String::Printf( TXT("Interior component in \"%s\" entity  does not have name in \"Texture\" property. Map for this components won't be generated. \n"), parentEntity->GetName().AsChar() );
						continue;
					}
				}

				Vector position = component->GetPosition();
				EulerAngles rotation = component->GetRotation();

				// 
				if( processedInteriorComponents.FindPtr( name ) != nullptr )
				{
					continue;
				}

				// add entity name to processed entities
				processedInteriorComponents.PushBack( name );

				// Render navmesh for interior
				RenderNavMeshForInterior( name, interiorComponent->GetBoundingBox(), interiorComponent->GetWorldPosition() + Vector( 0, 0, m_highestElevation + 1 ), interiorComponent->GetWorldPosition() );
			}
		}
	}

	GFeedback->EndTask();
#endif
}

void CMinimapGenerator::RenderNavMeshForInterior( const String& name, const Box& renderArea, const Vector& cameraPos, const Vector& componentPos )
{
#if defined(RED_PLATFORM_WINPC) && !defined(NO_EDITOR) && !defined(RED_FINAL_BUILD)
	// Merge file path and create need directories
	String fullPath = CreatePathForInteriorImage( name );

	// calculate viewport size
	Vector boxSize = renderArea.CalcSize();
	Float longerSide = ( boxSize.X > boxSize.Y ) ? boxSize.X : boxSize.Y;
	Uint32 viewportSize = (Uint32)( ( m_settings.m_exteriors.m_imageSize * (Uint32)longerSide ) / m_tileSize );

	PrepareViewport( viewportSize );

	m_world->UpdateCameraPosition( Vector( componentPos.X, componentPos.Y, cameraPos.Z ) );

	// Prepare render frame
	CRenderFrame* frame = PrepareRenderFrameForInterior( cameraPos, longerSide );

	CollectAndDrawNavmeshes( frame, renderArea );

	//Render tile
	m_world->RenderWorld( frame );

	// Relese render frame
	frame->Release();

	// Flush screen
	GRender->Flush();

	//
	SScreenshotParameters parameters( m_viewportSize, m_viewportSize, fullPath, SF_PNG, 4, 90.0f, SRF_PlainScreenshot );
	parameters.m_noWatermark = true;
	SScreenshotSystem::GetInstance().TakeSimpleScreenshotNow( parameters, SCF_SaveToDisk | SCF_UseProvidedFilename );

	DestroyViewport();
#endif
}

void CMinimapGenerator::CollectAndDrawNavmeshes( CRenderFrame* frame, const Box& renderArea )
{
#if defined(RED_PLATFORM_WINPC) && !defined(NO_EDITOR) && !defined(RED_FINAL_BUILD)
	CPathLibWorld* m_pathlib = m_world->GetPathLibWorld();
	do
	{
		m_pathlib->Tick();
	} while ( m_pathlib->GetStreamingManager()->IsJobRunning() );
	const PathLib::CInstanceMap* instanceMap = m_pathlib->GetInstanceMap();

	struct Functor : public PathLib::CInstanceMap::CInstanceFunctor
	{
		Functor( CRenderFrame* frame, const Box& renderArea, String* errors )
			: m_frame( frame )
			, m_renderArea( renderArea )
			, m_errors( errors )
		{
			/* intentionally empty */
		}

		Bool Handle( PathLib::CNavmeshAreaDescription* naviArea ) override
		{
			Red::TUniquePtr< PathLib::CNavmeshRenderer > naviRenderer( new PathLib::CNavmeshRenderer( naviArea->GetId() ) );
			if( naviRenderer != nullptr )
			{
				IRenderResource* navmeshPreview = nullptr;

				PathLib::CNavmesh* navmesh = naviArea->GetNavmesh();

				if( navmesh == nullptr )
				{
					return false;
				}

				PathLib::CNavmesh::TriangleIndex trianglesCount = navmesh->GetTrianglesCount();
				PathLib::CNavmesh::VertexIndex vertexCount = navmesh->GetVertexesCount();
				if( trianglesCount != 0 )
				{
					TDynArray< DebugVertex > vertices;
					TDynArray< Uint32 > indices;

					Uint32 index = 0;

					// Indices + phantom edges
					for( PathLib::CNavmesh::TriangleIndex i = 0; i < trianglesCount; ++i )
					{
						PathLib::CNavmesh::VertexIndex vertsIndexes[3];
						navmesh->GetTriangleVertsIndex( i, vertsIndexes );

						ASSERT( vertsIndexes[2] < vertexCount && vertsIndexes[ 1 ] < vertexCount && vertsIndexes[ 0 ] < vertexCount );

						if( m_renderArea.Contains( navmesh->GetVertex( vertsIndexes[0] ) ) == true &&
							m_renderArea.Contains( navmesh->GetVertex( vertsIndexes[1] ) ) == true &&
							m_renderArea.Contains( navmesh->GetVertex( vertsIndexes[2] ) ) == true )
						{		
							indices.PushBack( index++ );
							indices.PushBack( index++ );
							indices.PushBack( index++ );

							vertices.PushBack( DebugVertex( Vector( navmesh->GetVertex( vertsIndexes[2] ) ), Color::WHITE ) );
							vertices.PushBack( DebugVertex( Vector( navmesh->GetVertex( vertsIndexes[1] ) ), Color::WHITE ) );
							vertices.PushBack( DebugVertex( Vector( navmesh->GetVertex( vertsIndexes[0] ) ), Color::WHITE ) );
						}
					}

					if( indices.Empty() == true )
					{
						( *m_errors ) += String::Printf( TXT("Interior component does not contain any nav mesh data. \n") );
						return true;
					}

					navmeshPreview = GRender->UploadDebugMesh( vertices, indices );
				}

				if( navmeshPreview != nullptr )
				{
					new ( m_frame ) CRenderFragmentDebugMesh( m_frame, Matrix::IDENTITY, navmeshPreview, false );
				}
				else
				{
					RED_LOG( RED_LOG_CHANNEL(MinimapGenerator), TXT("Nav mesh was not uploaded to renderer.") );
				}
			}

			return true;
		}

		CRenderFrame*	m_frame;
		Box				m_renderArea;
		String*			m_errors;
	} functor( frame, renderArea, &m_interiorsErrors );

	instanceMap->IterateAreasAt( renderArea, &functor );
#endif
}

void CMinimapGenerator::RevertObjectsVisibility()
{
#if defined(RED_PLATFORM_WINPC) && !defined(NO_EDITOR) && !defined(RED_FINAL_BUILD)
	for( WorldAttachedComponentsIterator it( GGame->GetActiveWorld() ); it; ++it )
	{
		CDrawableComponent* component = Cast< CDrawableComponent >( *it );
		if( component != nullptr )
		{
			component->ToggleVisibility( true );
		}
	}
#endif
}

void CMinimapGenerator::GatherAllEntityTemplatesWithInterior( TDynArray< String >& finalPaths, const TDynArray<String>& specifiedEntities )
{
#if defined(RED_PLATFORM_WINPC) && !defined(NO_EDITOR) && !defined(RED_FINAL_BUILD)
	// TEMPORARY
	TDynArray<String> brokenEntity;
	brokenEntity.PushBack( TXT("Z:\\r4data\\items\\cutscenes\\pouch_01\\pouch_01.w2ent") );
	brokenEntity.PushBack( TXT("Z:\\r4data\\items\\cutscenes\\trophy_lessog\\trophy_lessog.w2ent") );
	brokenEntity.PushBack( TXT("Z:\\r4data\\items\\quest_items\\q304\\q304_item__ripped_page_notebook.w2ent") );
	// ~TEMPORARY

	TDynArray< String > paths;

	String depothPath = String::EMPTY;
	GDepot->GetAbsolutePath( depothPath );
	GFileManager->FindFiles( depothPath, TXT("*.w2ent"), paths, true );

	GFeedback->BeginTask( TXT("Looking for entity templates with interior component"), true );
	Uint32 index = 0;

	for( String path : paths )
	{
		if( GFeedback->IsTaskCanceled() == true )
		{
			break;
		}

		GFeedback->UpdateTaskProgress( index, paths.Size() );

		// HACK: exclude broken entities
		Bool isBroken = false;

		for( Uint32 i=0; i<brokenEntity.Size(); ++i )
		{
			if( path == brokenEntity[i] )
			{
				isBroken = true;
				break;
			}
		}

		if( isBroken == true )
		{
			continue;
		}

		// If there are specified entities in text control - look for them
		Bool specifiedFound = true;

		if( specifiedEntities.Size() > 0 )
		{
			specifiedFound = false;
		}

		for( Uint32 i=0; i<specifiedEntities.Size(); ++i )
		{
			CFilePath filePath( path );
			if( filePath.GetFileName().ContainsSubstring( specifiedEntities[i] ) )
			{
				specifiedFound = true;
				break;
			}
		}

		if( specifiedFound == false )
		{
			continue;
		}

		String relativePath = path;
		relativePath.Replace( depothPath, TXT("") );

		CResource* resource = GDepot->LoadResource( relativePath );
		if( resource != nullptr )
		{
			CEntityTemplate* entTemplate = Cast< CEntityTemplate >( resource );
			if( entTemplate != nullptr )
			{
				CEntity* entity = entTemplate->CreateInstance( nullptr, EntityTemplateInstancingInfo() );

				if( entity != nullptr )
				{
					const TDynArray< CComponent* >& components = entity->GetComponents();

					for( auto it=components.Begin(); it!=components.End(); ++it )
					{
						CComponent* component = ( *it );
						CR4InteriorAreaComponent* interiorComponent = Cast< CR4InteriorAreaComponent >( component );
						if( interiorComponent != nullptr )
						{
							finalPaths.PushBackUnique( relativePath );
						}
					}
					entity->Discard();
				}
				entTemplate->Discard();
			}
			resource->Discard();
		}
		++index;
	}

	GFeedback->EndTask();
#endif
}

void CMinimapGenerator::CreateInstances( const TDynArray< String >& paths, TDynArray< CEntity* >& entities )
{
#if defined(RED_PLATFORM_WINPC) && !defined(NO_EDITOR) && !defined(RED_FINAL_BUILD)
	GFeedback->BeginTask( TXT("Creating entities"), true );
	Uint32 index = 0;
	Vector position = Vector::ZERO_3D_POINT;
	position.Z = 500.0f;
	Uint32 entityCounter = 0;

	//
	CWorld* world = GGame->GetActiveWorld();
	if( world != nullptr )
	{
		CLayerGroup* worldLayerGroup = world->GetWorldLayers();
		for( String path : paths )
		{
			if( GFeedback->IsTaskCanceled() == true )
			{
				break;
			}
			GFeedback->UpdateTaskProgress( index, paths.Size() );

			//
			CResource* resource = GDepot->LoadResource( path );
			if( resource != nullptr )
			{
				CEntityTemplate* entTemplate = Cast< CEntityTemplate >( resource );
				if( entTemplate != nullptr )
				{
					// Create template for entity
					EntitySpawnInfo einfo;
					einfo.m_spawnPosition = position;
					einfo.m_detachTemplate = false;
					einfo.m_template = entTemplate;

					// Create entity
					CLayerInfo* info = worldLayerGroup->FindLayerByPath( TXT("interiors") );
					if( info == nullptr )
					{
						info = worldLayerGroup->CreateLayer( TXT("interiors") );
					}

					//
					CLayer* layer = info->GetLayer();
					if( layer == nullptr )
					{
						info->Reload();
						layer = info->GetLayer();
					}

					if( layer != nullptr )
					{
						CEntity* entity = layer->CreateEntitySync( einfo );
						entities.PushBack( entity );

						Box boxGameplay;
						entity->GetGameplayBounds( boxGameplay );

						//
						entityCounter++;
						position.Y += boxGameplay.Max.Z / 2.0f;
						if( entityCounter % 10 == 0 )
						{
							position.X += boxGameplay.Max.Z / 2.0f;
							position.Y = 0.0f;
						}
					}
				}
			}
		}

		++index;
	}

	GFeedback->EndTask();
#endif
}

void CMinimapGenerator::CreateNavmesh( TDynArray< CEntity* >& entities )
{
#if defined(RED_PLATFORM_WINPC) && !defined(NO_EDITOR) && !defined(RED_FINAL_BUILD)
	m_tempHandlerFeedback = GFeedback;
	GFeedback = &GNullFeedback;

	String navmeshInstanceEntityTemplatePath = TXT("engine\\templates\\editor\\navmesh_instance.w2ent");

	//
	CResource* resource = GDepot->LoadResource( navmeshInstanceEntityTemplatePath );
	CEntityTemplate* navmeshTemplate = Cast< CEntityTemplate >( resource );

	CWorld* world = GGame->GetActiveWorld();
	CLayerGroup* worldLayerGroup = world->GetWorldLayers();
	for( auto it=entities.Begin(); it!=entities.End(); ++it )
	{
		CEntity* entity = ( *it );
		Box entityBBox = entity->CalcBoundingBox();

		if( navmeshTemplate != nullptr )
		{
			// Create entity
			CLayerInfo* info = worldLayerGroup->FindLayerByPath( TXT("navmesh") );
			if( info == nullptr )
			{
				info = worldLayerGroup->CreateLayer( TXT("navmesh") );
			}

			//
			CLayer* layer = info->GetLayer();
			if( layer == nullptr )
			{
				info->Reload();
				layer = info->GetLayer();
			}

			if( layer != nullptr )
			{
				// Create template for entity
				EntitySpawnInfo navmeshInfo;
				navmeshInfo.m_spawnPosition = entity->GetPosition();
				navmeshInfo.m_detachTemplate = true;
				navmeshInfo.m_template = navmeshTemplate;

				// create navmesh entity
				CEntity* navmeshEntity = layer->CreateEntitySync( navmeshInfo );

				// Note: have to check, because CreateEntitySync may fail for different reasons and this method is supposed to return nullptr without crashing.
				if ( navmeshEntity )
				{
					// Create streamed components (they will be unloaded if not needed in the next camera move)
					navmeshEntity->CreateStreamedComponents( SWN_NotifyWorld );
				}

				Box navmeshBBox = navmeshEntity->CalcBoundingBox();

				// set proper scale
				Float scaleX = ( entityBBox.Max.X - entityBBox.Min.X ) / ( navmeshBBox.Max.X - navmeshBBox.Min.X );
				Float scaleY = ( entityBBox.Max.Y - entityBBox.Min.Y ) / ( navmeshBBox.Max.Y - navmeshBBox.Min.Y );
				Float scaleZ = ( entityBBox.Max.Z - entityBBox.Min.Z ) / ( navmeshBBox.Max.Z - navmeshBBox.Min.Z );
				Vector scale( scaleX, scaleY, scaleZ );
				navmeshEntity->SetScale( scale );

				const TDynArray< CComponent* >& components = navmeshEntity->GetComponents();

				for( auto compIt=components.Begin(); compIt!=components.End(); ++compIt )
				{
					CNavmeshComponent* navmeshComponent = Cast< CNavmeshComponent >( *compIt );
					if( navmeshComponent != nullptr )
					{
						TDynArray< Vector > roots = navmeshComponent->GetGenerationRoots();
						Vector worldPos = navmeshEntity->GetPosition();
						navmeshEntity->SetPosition( worldPos - Vector( 0.0f, 0.0f, roots[0].Z ) );

						// Update the camera
						world->UpdateCameraPosition( entity->GetPosition() );
						m_world->SetStreamingReferencePosition( entity->GetPosition() );

						// update world before render it
						/*do 
						{
						world->UpdateLoadingState( true );
						} while ( world->HasDelayedActionsPending() == true );*/

						SNavmeshParams& params = navmeshComponent->GetNavmeshParams();
						params.m_useTerrainInGeneration = true;
						navmeshComponent->GenerateNavmeshAsync();
					}

				}
			}
		}
	}

	GFeedback = m_tempHandlerFeedback;
#endif
}

void CMinimapGenerator::CreateNewInteriors()
{

}

String CMinimapGenerator::GetFoundEntityTemplateCount() const
{
	return TXT("N/A");
}

String CMinimapGenerator::GetCreatedInstanceCount() const
{
	return TXT("N/A");
}
