/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "viewport.h"
#include "renderFrameInfo.h"
#include "viewportHook.h"
#include "world.h"
#include "baseEngine.h"
#include "renderGameplayRenderTargetInterface.h"
#include "renderSettings.h"


IMPLEMENT_RTTI_ENUM( EShowFlags );
IMPLEMENT_RTTI_ENUM( ECameraLightModType );

#ifdef USE_ANSEL
extern Bool isAnselSessionActive;
#endif // USE_ANSEL

///////////////////////////////////////////////////////////////////////////////////////////////////
// The following specify which SHOW flags should be available in the Debug, Render, and Post-process
// subsections, for each section. The Physics Debug and Umbra Debug sections have a fixed set of
// SHOW flags, given by GShowPhysicsDebugFilter and GShowUmbraDebugFilter.
///////////////////////////////////////////////////////////////////////////////////////////////////

EShowFlags GShowEditorFilter[] =
{
	SHOW_Meshes					,
	SHOW_Flares					,
	SHOW_FlaresData				,
	SHOW_Brushes				,
	SHOW_Camera					,
	SHOW_ErrorState				,
	SHOW_HierarchicalGrid		,
	SHOW_Shadows				,
	SHOW_ForceAllShadows		,
	SHOW_Dimmers				,
	SHOW_DimmersBBoxes			,
	SHOW_TonemappingCurve		,
	SHOW_Lights					,
	SHOW_UseRegularDeferred		,
	SHOW_AntiLightbleed			,
	SHOW_CameraLights			,
	SHOW_DistantLights			,
	SHOW_LightsBBoxes			,
	SHOW_Terrain				,
	SHOW_Skirt					,
	SHOW_NonClimbable			,
	SHOW_Foliage				,
	SHOW_LocalReflections		,
	SHOW_PostProcess			,
	SHOW_Selection				,
	SHOW_Grid					,
	SHOW_Sprites				,
	SHOW_EnvProbesInstances		,
	SHOW_EnvProbesOverlay		,
	SHOW_EnvProbesBigOverlay	,
	SHOW_VideoOutputLimited		,
	SHOW_FlareOcclusionShapes	,
	SHOW_Particles				,
	SHOW_PhantomShapes			,
	SHOW_Wireframe				,
	SHOW_WalkSurfaces			,
	SHOW_Areas					,
	SHOW_AreaShapes				,
	SHOW_TriggerBounds			,
	SHOW_TriggerActivators		,
	SHOW_TriggerTree			,
	SHOW_Logic					,
	SHOW_Profilers				,
	SHOW_Paths					,
	SHOW_PathSpeedValues		,
	SHOW_Waypoints				,
	SHOW_Stickers				,
	SHOW_Bboxes					,
	SHOW_Exploration			,
	SHOW_Behavior				,
	SHOW_Scenes					,
	SHOW_Lighting				,
	SHOW_TSLighting				,
	SHOW_Decals					,
	SHOW_DynamicDecals			,
	SHOW_Stripes				,
	SHOW_Emissive				,
	SHOW_Spawnset				,
	SHOW_Collision				,
	SHOW_CollisionIfNotVisible	,
	SHOW_GUI					,
	SHOW_VisualDebug			,
	SHOW_Refraction				,
	SHOW_Encounter				,
	SHOW_BboxesParticles		,
	SHOW_BboxesTerrain			,
	SHOW_BboxesDecals			,
	SHOW_BboxesSmallestHoleOverride,
	SHOW_SpritesDecals			,
	SHOW_BuildingBrush			,
	SHOW_Sound					,
	SHOW_SoundReverb			,
	SHOW_SoundAmbients			,
	SHOW_SoundEmitters			,
	SHOW_Gizmo					,
	SHOW_Devices				,
	SHOW_SelectionContacts		,
	SHOW_PEObstacles			,
	SHOW_Bg						,
	SHOW_TriangleStats			,
	SHOW_EnvProbeStats			,
	SHOW_ShadowStats			,
	SHOW_ShadowPreviewDynamic	,
	SHOW_ShadowPreviewStatic	,
	SHOW_ApexStats				,
	SHOW_TerrainStats			,
	SHOW_PreferTemporalAA		,
	SHOW_Underwater				,
	SHOW_MergedMeshes			,
	SHOW_UseVisibilityMask		,
	SHOW_ShadowMeshDebug		,
	SHOW_StreamingCollisionBoxes,

	SHOW_Apex					,
	SHOW_Projectile				,

	SHOW_Wind					,
	SHOW_NavMesh				,
	SHOW_NavMeshTriangles		,
	SHOW_NavMeshOverlay			,
	SHOW_NavTerrain				,
	SHOW_NavTerrainHeight		,
	SHOW_NavGraph				,
	SHOW_NavGraphNoOcclusion	,
	SHOW_NavGraphRegions		,
	SHOW_NavObstacles			,
	SHOW_NavRoads				,
	SHOW_SpeedTreeShadows		,
	SHOW_AllowApexShadows		,
	SHOW_HiResEntityShadows		,
	SHOW_TerrainHoles			,
	SHOW_CollisionSoundOcclusion,
	SHOW_AnimatedProperties		,
	SHOW_CurveAnimations		,
	SHOW_AnimDangles			,
	SHOW_GenericGrass			,
	SHOW_Swarms					,
	SHOW_ReversedProjection		,
	SHOW_CascadesStabilizedRotation	,
	SHOW_DynamicComponent		,
	SHOW_DynamicCollector		,
	SHOW_MeshComponent			,
	SHOW_LodInfo				,
	SHOW_EntityVisibility		,
	SHOW_Histogram				,
	SHOW_AllowDebugPreviewTemporalAA	,
	SHOW_PigmentMap				,
	SHOW_NodeTags				,
	SHOW_HairAndFur				,
	SHOW_ForwardPass			,
	SHOW_GeometrySkinned		,
	SHOW_GeometryStatic			,
	SHOW_GeometryProxies		,
	SHOW_Containers				,
	SHOW_EffectAreas			,
	SHOW_Dismemberment			,
	SHOW_AIBehaviorDebug		,
	SHOW_XB1SafeArea			,
	SHOW_PS4SafeArea			,

	SHOW_Skybox					,
	SHOW_CameraInteriorFactor	,
	SHOW_BrowseDebugPreviews	,

	SHOW_RenderDeepTerrain		,
	SHOW_BBoxesFur				,
	SHOW_BBoxesCloth			,

	SHOW_GameplayPostFx			,
	SHOW_SimpleBuoyancy			,
	SHOW_ScaleformMemoryInfo	,
	SHOW_GameplayLightComponent ,
	SHOW_Wetness				,
	SHOW_CameraVisibility		,
	SHOW_TeleportDetector		,

	SHOW_SoundListener			,
	SHOW_BBoxesDestruction		,

	SHOW_RenderGPUProfiler		,

	SHOW_MAX_INDEX				// the way we mark an end
};


EShowFlags GShowPreviewFilter[] =
{
	SHOW_Meshes					,
	SHOW_Flares					,
	SHOW_Brushes				,
	SHOW_Shadows				,
	SHOW_Dimmers				,
	SHOW_DimmersBBoxes			,
	SHOW_TonemappingCurve		,
	SHOW_Histogram				,
	SHOW_AllowDebugPreviewTemporalAA	,
	SHOW_PigmentMap				,
	SHOW_Lights					,
	SHOW_UseRegularDeferred		,
	SHOW_AntiLightbleed			,
	SHOW_DistantLights			,
	SHOW_LightsBBoxes			,
	SHOW_DepthOfField			,
	SHOW_Sprites				,
	SHOW_EnvProbesInstances		,
	SHOW_Particles				,
	SHOW_PhantomShapes			,
	SHOW_Wireframe				,
	SHOW_WalkSurfaces			,
	SHOW_Areas					,
	SHOW_AreaShapes				,
	SHOW_Locomotion				,
	SHOW_MovableRep				,
	SHOW_Logic					,
	SHOW_Profilers				,
	SHOW_Paths					,
	SHOW_Waypoints				,
	SHOW_Bboxes					,
	SHOW_BboxesParticles		,
	SHOW_BboxesTerrain			,
	SHOW_BboxesDecals			,
	SHOW_BboxesSmallestHoleOverride,
	SHOW_SpritesDecals			,
	SHOW_Exploration			,
	SHOW_Behavior				,
	SHOW_Scenes					,
	SHOW_Lighting				,
	SHOW_TSLighting				,
	SHOW_Decals					,
	SHOW_DynamicDecals			,
	SHOW_Stripes				,
	SHOW_Emissive				,
	SHOW_Spawnset				,
	SHOW_NonClimbable			,
	SHOW_Collision				,
	SHOW_CollisionIfNotVisible	,
	SHOW_GUI					,
	SHOW_VisualDebug			,
	SHOW_Refraction				,
	SHOW_Encounter				,
	SHOW_Gizmo					,
	SHOW_Terrain				,
	SHOW_Foliage				,
	SHOW_LocalReflections		,
	SHOW_Selection				,
	SHOW_Sound					,
	SHOW_SoundReverb			,
	SHOW_SoundAmbients			,
	SHOW_SoundEmitters			,
	SHOW_TriangleStats			,
	SHOW_EnvProbeStats			,
	SHOW_ShadowStats			,
	SHOW_ShadowPreviewDynamic	,
	SHOW_ShadowPreviewStatic	,
	SHOW_ApexStats				,
	SHOW_Wind					,
	//SHOW_NavMesh				,
	//SHOW_NavMeshTriangles		,
	//SHOW_NavMeshOverlay		,
	SHOW_NavObstacles			,
	SHOW_Apex					,
	SHOW_Projectile				,
	SHOW_SpeedTreeShadows		,
	SHOW_AllowApexShadows		,
	SHOW_HiResEntityShadows		,

	SHOW_CollisionSoundOcclusion,

	SHOW_TBN					,
	SHOW_AnimatedProperties		,
	SHOW_CurveAnimations		,
	SHOW_AnimDangles			,

	SHOW_ReversedProjection		,
	SHOW_CascadesStabilizedRotation	,
	SHOW_DynamicComponent		,
	SHOW_MeshComponent			,
	SHOW_LodInfo				,
	SHOW_EntityVisibility		,

	SHOW_NodeTags				,
	SHOW_HairAndFur				,
	SHOW_ForwardPass			,
	SHOW_GeometrySkinned		,
	SHOW_GeometryStatic			,
	SHOW_GeometryProxies		,

	SHOW_Skybox					,
	SHOW_CameraInteriorFactor	,
	SHOW_BrowseDebugPreviews	,

	SHOW_BBoxesFur				,
	SHOW_VideoOutputLimited		,
	SHOW_BBoxesCloth			,

	SHOW_SimpleBuoyancy			,
	SHOW_TeleportDetector		,
	SHOW_SoundListener			,
	SHOW_BBoxesDestruction		,

	SHOW_RenderGPUProfiler		,

	SHOW_MAX_INDEX				// the way we mark an end
};

EShowFlags GShowGameFilter[] =
{
	SHOW_Meshes					,
	SHOW_Flares					,
	SHOW_FlaresData				,
	SHOW_Brushes				,
	SHOW_AI						,
	SHOW_AIBehTree				,
	SHOW_AITickets				,
	SHOW_AISenses				,
	SHOW_AIRanges				,
	SHOW_Camera					,
	SHOW_ErrorState				,
	SHOW_HierarchicalGrid		,
	SHOW_Shadows				,
	SHOW_ForceAllShadows		,
	SHOW_Dimmers				,
	SHOW_DimmersBBoxes			,
	SHOW_TonemappingCurve		,
	SHOW_Histogram				,
	SHOW_AllowDebugPreviewTemporalAA	,
	SHOW_PigmentMap				,
	SHOW_Lights					,
	SHOW_UseRegularDeferred		,
	SHOW_AntiLightbleed			,
	SHOW_CameraLights			,
	SHOW_DistantLights			,
	SHOW_LightsBBoxes			,
	SHOW_Terrain				,
	SHOW_Skirt					,
	SHOW_Foliage				,
	SHOW_LocalReflections		,
	SHOW_PostProcess			,
	SHOW_Grid					,
	SHOW_Sprites				,
	SHOW_EnvProbesInstances		,
	SHOW_EnvProbesOverlay		,
	SHOW_EnvProbesBigOverlay	,
	SHOW_VideoOutputLimited		,
	SHOW_FlareOcclusionShapes	,
	SHOW_Particles				,
	SHOW_PhantomShapes			,
	SHOW_Wireframe				,
	SHOW_WalkSurfaces			,
	SHOW_Areas					,
	SHOW_AreaShapes				,
	SHOW_TriggerBounds			,
	SHOW_TriggerTree			,
	SHOW_TriggerActivators		,
	SHOW_Locomotion				,
	SHOW_MovableRep				,
	SHOW_Logic					,
	SHOW_Profilers				,
	SHOW_Paths					,
	SHOW_PathSpeedValues		,
	SHOW_Waypoints				,
	SHOW_Stickers				,
	SHOW_Steering				,
	SHOW_Bboxes					,
	SHOW_BboxesParticles		,
	SHOW_BboxesTerrain			,
	SHOW_BboxesDecals			,
	SHOW_BboxesSmallestHoleOverride,
	SHOW_SpritesDecals			,
	SHOW_Exploration			,
	SHOW_Behavior				,
	SHOW_Scenes					,
	SHOW_Lighting				,
	SHOW_TSLighting				,
	SHOW_Decals					,
	SHOW_DynamicDecals			,
	SHOW_Stripes				,
	SHOW_Emissive				,
	SHOW_Spawnset				,
	SHOW_NonClimbable			,
	SHOW_Collision				,
	SHOW_GUI					,
	SHOW_VisualDebug			,
	SHOW_Refraction				,
	SHOW_Encounter				,
	SHOW_Sound					,
	SHOW_SoundReverb			,
	SHOW_SoundAmbients			,
	SHOW_SoundEmitters			,
	SHOW_Gizmo					,
	SHOW_GUIFallback			,
	SHOW_OnScreenMessages		,
	SHOW_Devices				,
	SHOW_PEObstacles			,
	SHOW_Bg						,
	SHOW_TriangleStats			,
	SHOW_EnvProbeStats			,
	SHOW_ShadowStats			,
	SHOW_ShadowPreviewDynamic	,
	SHOW_ShadowPreviewStatic	,
	SHOW_ApexStats				,
	SHOW_TerrainStats			,
	SHOW_PreferTemporalAA		,
	SHOW_Underwater				,
	SHOW_Formations				,
	SHOW_Wind					,
	SHOW_NavMesh				,
	SHOW_NavMeshTriangles		,
	SHOW_NavMeshOverlay			,
	SHOW_NavTerrain				,
	SHOW_NavTerrainHeight		,
	SHOW_NavGraph				,
	SHOW_NavGraphNoOcclusion	,
	SHOW_NavGraphRegions		,
	SHOW_NavObstacles			,
	SHOW_NavRoads				,
	SHOW_VolumetricPaths		,
	SHOW_Apex					,
	SHOW_Projectile				,
	SHOW_SpeedTreeShadows		,
	SHOW_AllowApexShadows		,
	SHOW_HiResEntityShadows		,
	SHOW_CollisionSoundOcclusion,
	SHOW_AnimatedProperties		,
	SHOW_CurveAnimations		,
	SHOW_AnimDangles			,
	SHOW_MergedMeshes			,
	SHOW_UseVisibilityMask		,
	SHOW_ShadowMeshDebug		,
	SHOW_StreamingCollisionBoxes,

	SHOW_TerrainHoles			,
	SHOW_FocusMode				,
	SHOW_Crowd					,
	SHOW_GenericGrass			,

	SHOW_BoatSailing			,
    SHOW_BoatWaterProbbing      ,
    SHOW_BoatInput              ,
    SHOW_BoatHedgehog			,
	SHOW_BoatDestruction		,
	SHOW_BoatBuoyancy			,
	SHOW_BoatPathFollowing		,

	SHOW_Swarms					,
	SHOW_Interactions			,
	SHOW_AIActions				,

	SHOW_ReversedProjection		,
	SHOW_CascadesStabilizedRotation	,

	SHOW_DynamicComponent		,
	SHOW_DynamicCollector		,
	SHOW_MeshComponent			,
	SHOW_LodInfo				,
	SHOW_EntityVisibility		,
	SHOW_ActorLodInfo			,
	SHOW_CommunityAgents		,

	SHOW_NodeTags				,
	SHOW_HairAndFur				,
	SHOW_ForwardPass			,
	SHOW_GeometrySkinned		,
	SHOW_GeometryStatic			,
	SHOW_GeometryProxies		,

	SHOW_Containers				,

	SHOW_EffectAreas			,
	SHOW_Dismemberment			,

	SHOW_AIBehaviorDebug		,
	SHOW_TemplateLoadBalancer	,

	// These *MUST* be kept together.
	SHOW_StreamingTree0			,
	SHOW_StreamingTree1			,
	SHOW_StreamingTree2			,
	SHOW_StreamingTree3			,
	SHOW_StreamingTree4			,
	SHOW_StreamingTree5			,
	SHOW_StreamingTree6			,
	SHOW_StreamingTree7			,

	SHOW_XB1SafeArea			,
	SHOW_PS4SafeArea			,

	SHOW_Skybox					,
	SHOW_CameraInteriorFactor	,
	SHOW_BrowseDebugPreviews	,

	SHOW_RenderDeepTerrain		,

	SHOW_BBoxesFur				,
	SHOW_BBoxesCloth			,
	SHOW_RoadFollowing			,

	SHOW_GameplayPostFx			,

	SHOW_SimpleBuoyancy			,
	SHOW_ScaleformMemoryInfo	,
	SHOW_GameplayLightComponent ,
	SHOW_Wetness				,
	SHOW_CameraVisibility		,
	SHOW_TeleportDetector		,
	SHOW_MapTracking			,
	SHOW_QuestMapPins			,
	SHOW_SoundListener			,
	SHOW_BBoxesDestruction		,

	SHOW_RenderGPUProfiler		,

	SHOW_MAX_INDEX				// the way we mark an end
};


///////////////////////////////////////////////////////////////////////////////////////////////////
// The following define which SHOW flags are on by default, in each section.
///////////////////////////////////////////////////////////////////////////////////////////////////

EShowFlags GShowGameMask[] = 
{
	SHOW_Meshes					,
	SHOW_Flares					,
	SHOW_Brushes				,
	SHOW_Shadows				,	
	SHOW_Dimmers				,
	SHOW_Lights					,
	SHOW_UseRegularDeferred		,
	SHOW_AntiLightbleed			,
	SHOW_CameraLights			,
	SHOW_DistantLights			,
	SHOW_Terrain				,
	SHOW_Skirt					,
	SHOW_Foliage				,
	SHOW_LocalReflections		,
	SHOW_Particles				,
	SHOW_PostProcess			,
	SHOW_Lighting				,
	SHOW_TSLighting				,
	SHOW_Decals					,
	SHOW_DynamicDecals			,
	SHOW_Stripes				,
	SHOW_Emissive				,
	SHOW_GUI					,
	SHOW_VisualDebug			,
	SHOW_Refraction				,
	SHOW_Profilers				,
	SHOW_PreferTemporalAA		,
	SHOW_Underwater				,
	SHOW_OnScreenMessages		,
	SHOW_PostProcess			,
	SHOW_Apex					,
	SHOW_SpeedTreeShadows		,
	SHOW_AllowApexShadows		,
	SHOW_HiResEntityShadows		,
	SHOW_GenericGrass			,
	SHOW_MergedMeshes			,
	SHOW_UseVisibilityMask		,

	SHOW_UmbraCullTerrain		,
	SHOW_UmbraCullFoliage		,
	SHOW_UmbraCullShadows		,
	SHOW_UmbraCullSpeedTreeShadows,

	SHOW_ReversedProjection		,
	SHOW_CascadesStabilizedRotation	,

	SHOW_HairAndFur				,
	SHOW_ForwardPass			,
	SHOW_GeometrySkinned		,
	SHOW_GeometryStatic			,
	SHOW_GeometryProxies		,

	SHOW_Skybox					,

	SHOW_GameplayPostFx			,

	SHOW_MAX_INDEX				// the way we mark an end
};

EShowFlags GShowEditorMask[] = 
{
	SHOW_Meshes					,
	SHOW_Flares					,
	SHOW_Brushes				,
	SHOW_AI						,
	SHOW_Shadows				,	
	SHOW_Dimmers				,
	SHOW_Lights					,
	SHOW_UseRegularDeferred		,
	SHOW_AntiLightbleed			,
	SHOW_CameraLights			,
	SHOW_DistantLights			,
	SHOW_Terrain				,
	SHOW_Skirt					,
	SHOW_Foliage				,
	SHOW_LocalReflections		,
	SHOW_Particles				,
	SHOW_PostProcess			,
	SHOW_Lighting				,
	SHOW_TSLighting				,
	SHOW_Emissive				,
	SHOW_Decals					,
	SHOW_DynamicDecals			,
	SHOW_Stripes				,
	SHOW_GUI					,
	SHOW_VisualDebug			,
	SHOW_Refraction				,
	SHOW_Gizmo					,
	SHOW_OnScreenMessages		,
	SHOW_PreferTemporalAA		,
	SHOW_Underwater				,
	SHOW_Wind					,
	SHOW_Apex					,
	SHOW_SpeedTreeShadows		,
	SHOW_AllowApexShadows		,
	SHOW_HiResEntityShadows		,
	SHOW_GenericGrass			,
	SHOW_MergedMeshes			,
	SHOW_UseVisibilityMask		,

	SHOW_UmbraCullTerrain		,
	SHOW_UmbraCullFoliage		,
	SHOW_UmbraCullShadows		,
	SHOW_UmbraCullSpeedTreeShadows,

	SHOW_ReversedProjection		,
	SHOW_CascadesStabilizedRotation	,

	SHOW_HairAndFur				,
	SHOW_ForwardPass			,
	SHOW_GeometrySkinned		,
	SHOW_GeometryStatic			,
	SHOW_GeometryProxies		,

	SHOW_Skybox					,

	SHOW_RenderDeepTerrain		,

	SHOW_GameplayPostFx			,

	SHOW_MAX_INDEX				// the way we mark an end
};

EShowFlags GShowPreviewMask[] = 
{
	SHOW_Meshes					,
	SHOW_Flares					,
	SHOW_Shadows				,
	SHOW_Dimmers				,
	SHOW_Lights					,
	SHOW_UseRegularDeferred		,
	SHOW_AntiLightbleed			,
	SHOW_DistantLights			,
	SHOW_Particles				,
	SHOW_Lighting				,
	SHOW_TSLighting				,
	SHOW_Decals					,
	SHOW_DynamicDecals			,
	SHOW_Stripes				,
	SHOW_Emissive				,
	SHOW_VisualDebug			,
	SHOW_Gizmo					,
	SHOW_TriangleStats			,
	SHOW_EnvProbeStats			,
	SHOW_Wind					,
	SHOW_Apex					,
	SHOW_SpeedTreeShadows		,
	SHOW_AllowApexShadows		,
	SHOW_HiResEntityShadows		,

	SHOW_ReversedProjection		,
	SHOW_CascadesStabilizedRotation	,

	SHOW_HairAndFur				,
	SHOW_ForwardPass			,
	SHOW_GeometrySkinned		,
	SHOW_GeometryStatic			,
	SHOW_GeometryProxies		,

	SHOW_Skybox					,

	SHOW_MAX_INDEX				// the way we mark an end
};


///////////////////////////////////////////////////////////////////////////////////////////////////
// The following sort the SHOW flags into separate subsections. If a SHOW flag does not appear in
// any of the following, it will be in the Debug subsection.
///////////////////////////////////////////////////////////////////////////////////////////////////

EShowFlags GShowRenderFilter[] = 
{
	SHOW_Meshes					,
	SHOW_Flares					,
	SHOW_Brushes				,
	SHOW_Shadows				,
	SHOW_ForceAllShadows		,
	SHOW_Dimmers				,
	SHOW_Lights					,
	SHOW_UseRegularDeferred		,
	SHOW_AntiLightbleed			,
	SHOW_CameraLights			,
	SHOW_DistantLights			,
	SHOW_Terrain				,
	SHOW_Skirt					,
	SHOW_Foliage				,
	SHOW_LocalReflections		,
	SHOW_Particles				,
	SHOW_PostProcess			,
	SHOW_DepthOfField			,
	SHOW_Refraction				,
	SHOW_Lighting				,
	SHOW_TSLighting				,
	SHOW_Decals					,
	SHOW_DynamicDecals			,
	SHOW_Stripes				,
	SHOW_Emissive				,
	SHOW_TriangleStats			,
	SHOW_EnvProbeStats			,
	SHOW_ShadowStats			,
	SHOW_ShadowPreviewDynamic	,
	SHOW_ShadowPreviewStatic	,
	SHOW_ApexStats				,
	SHOW_TerrainStats			,
	SHOW_Apex					,
	SHOW_SpeedTreeShadows		,
	SHOW_AllowApexShadows		,
	SHOW_HiResEntityShadows		,
	SHOW_GenericGrass			,
	SHOW_MergedMeshes			,
	SHOW_UseVisibilityMask		,

	SHOW_OcclusionStats			,

	SHOW_ReversedProjection		,
	SHOW_CascadesStabilizedRotation	,
	SHOW_HairAndFur				,
	SHOW_ForwardPass			,
	SHOW_GeometrySkinned		,
	SHOW_GeometryStatic			,
	SHOW_GeometryProxies		,

	SHOW_Skybox					,

	SHOW_RenderDeepTerrain		,

	SHOW_MAX_INDEX				// the way we mark an end
};

EShowFlags GShowPostProcessFilter[] =
{
	SHOW_PreferTemporalAA		,
	SHOW_Underwater				,
	SHOW_GameplayPostFx			,

	SHOW_MAX_INDEX				// the way we mark an end
};

EShowFlags GShowPhysicsDebugFilter[] =
{
	SHOW_ApexDebugAll				,
	SHOW_ApexClothCollisionSolid	,
	SHOW_ApexClothCollisionWire		,
	SHOW_ApexClothSkeleton			,
	SHOW_ApexClothBackstop			,
	SHOW_ApexClothBackstopPrecise	,
	SHOW_ApexClothMaxDistance		,
	SHOW_ApexClothVelocity			,
	SHOW_ApexClothSkinnedPosition	,
	SHOW_ApexClothActiveTethers		,
	SHOW_ApexClothLength			,
	SHOW_ApexClothCrossSection		,
	SHOW_ApexClothBending			,
	SHOW_ApexClothShearing			,
	SHOW_ApexClothZeroStretch		,
	SHOW_ApexClothVirtualCollision	,
	SHOW_ApexClothSelfCollision		,
	SHOW_ApexClothPhysicsMeshWire	,
	SHOW_ApexClothPhysicsMeshSolid	,
	SHOW_ApexClothWind				,
	SHOW_ApexClothLocalSpace		,
	SHOW_ApexDestructSupport		,
	SHOW_PhysXVisualization			,
    SHOW_PhysXPlatforms             ,
	SHOW_PhysXTraceVisualization	,
	SHOW_PhysXMaterials				,
	SHOW_PhysActorDampers			,
	SHOW_PhysActorVelocities		,
	SHOW_PhysActorMasses			,
	SHOW_ApexFractureRatio			,
	SHOW_ApexFracturePoints			,
	SHOW_ApexThresoldLeft			,
	SHOW_PhysMotionIntensity		,
	SHOW_PhysActorIterations		,
	SHOW_PhysActorFloatingRatio		,
	SHOW_MAX_INDEX
};

EShowFlags GShowUmbraDebugFilter[] =
{
	SHOW_OcclusionStats			,
	SHOW_UmbraFrustum			,
	SHOW_UmbraObjectBounds		,
	SHOW_UmbraPortals			,
	SHOW_UmbraVisibleVolume		,
	SHOW_UmbraViewCell			,
	SHOW_UmbraVisibilityLines	,
	SHOW_UmbraStatistics		,
	SHOW_UmbraOcclusionBuffer	,
	SHOW_UmbraCullShadows		,
	SHOW_UmbraCullSpeedTreeShadows,
	SHOW_UmbraShowOccludedNonStaticGeometry,
	SHOW_UmbraStreamingVisualization,
	SHOW_UmbraCullTerrain		,
	SHOW_UmbraCullFoliage		,
	SHOW_UmbraShowFoliageCells	,
	SHOW_CullTerrainWithFullHeight,

	SHOW_MAX_INDEX
};


//////////////////////////////////////////////////////////////////////////


static CAreaEnvironmentParamsAtPoint& GetDefaultEnvParams()
{
	static CAreaEnvironmentParamsAtPoint values;
	static Bool init = true;
	if ( init )
	{
		CAreaEnvironmentParams defaultParams( EnvResetMode_CurvesDefault );
		values = CAreaEnvironmentParamsAtPoint( defaultParams );
		init = false;
	}

	return values;
}

#ifdef USE_ANSEL
extern Bool isAnselSessionActive;
#endif // USE_ANSEL

CRenderFrameInfo::CRenderFrameInfo()
	: m_viewport( NULL )
	, m_renderingMode( RM_Wireframe )
	, m_allowPostSceneRender( true )
	, m_forceGBufferClear( false )
	, m_present( true )
	, m_canvasToWorld( Matrix::IDENTITY )
	, m_width( 0 )
	, m_height( 0 )
	, m_instantAdaptation( true )
	, m_clearColor( Color::BLACK )
	, m_worldTime( 0.0f )
	, m_engineTime( 0.0f )
	, m_cleanEngineTime( 0.0f )
	, m_gameTime( 0.0f )
	, m_gameDays( 0 )
	, m_envParametersArea( GetDefaultEnvParams() )
	, m_envBlendingFactor( 0.0f )
	, m_materialDebugMode( MDM_None )
	, m_isLastFrameForced( false )
	, m_forceFade( false )
	, m_drawHUD( true )
	, m_renderFeaturesFlags ( (Uint32)DMFF_MASK_ALL )
	, m_allowSequentialCapture( false )
	, m_multiplanePresent( false )
	, m_enableFPSDisplay( false )
	, m_globalWaterLevelAtCameraPos( -100000.f )
	, m_renderTarget( NULL )
	, m_tonemapFixedLumiance( -1.0f )
	, m_backgroundTextureColorScale(1.0,1.0,1.0,1.0)
	, m_isWorldScene( false )
	, m_isGamePaused( false )
	, m_gameplayCameraLightsFactor( 1 )
	, m_customRenderResolution( false )
	, m_allowSkinningUpdate( true )
	, m_hiResShadowMaxExtents( -1.0f )
{
	ResetCommonSettings();

	Red::System::MemoryZero( m_renderingMask, sizeof( Bool ) * SHOW_MAX_INDEX );
	for ( Uint32 i = 0; GShowEditorMask[ i ] < SHOW_MAX_INDEX; ++i )
	{
		SetShowFlag( GShowEditorMask[ i ], true );
	}

	m_frameTime = static_cast< Float >( Red::System::Clock::GetInstance().GetTimer().GetSeconds() );

#ifdef RED_PLATFORM_DURANGO
	m_multiplanePresent = true;
#endif
}

CRenderFrameInfo::CRenderFrameInfo( IViewport* viewport, Bool instantAdaptation, Bool instantDissolve )
	: m_viewport( viewport )
	, m_renderingMode( viewport->GetRenderingMode() )
	, m_allowPostSceneRender( true )
	, m_forceGBufferClear( false )
	, m_present( true )
	, m_width( viewport->GetWidth() )
	, m_height( viewport->GetHeight() )
	, m_canvasToWorld( Matrix::IDENTITY )
	, m_instantAdaptation( instantAdaptation )
	, m_clearColor( Color::BLACK )
	, m_worldTime( 0.0f )
	, m_gameTime( 0.0f )
	, m_gameDays( 0 )
	, m_engineTime( 0.0f )
	, m_cleanEngineTime( 0.0f )
	, m_envParametersArea( GetDefaultEnvParams() )
	, m_isNonInteractiveRendering( false )
	, m_materialDebugMode( MDM_None )
	, m_isLastFrameForced( false )
	, m_forceFade( false )
	, m_drawHUD( true )
	, m_renderFeaturesFlags ( (Uint32)DMFF_MASK_ALL )
	, m_allowSequentialCapture( false )
	, m_multiplanePresent( false )
	, m_enableFPSDisplay( false )
	, m_renderTarget( NULL )
	, m_tonemapFixedLumiance( -1.0f )
	, m_backgroundTextureColorScale(1.0,1.0,1.0,1.0)
	, m_isWorldScene( false )
	, m_isGamePaused( false )
	, m_gameplayCameraLightsFactor( 1 )
	, m_customRenderResolution( false )
	, m_allowSkinningUpdate( true )
	, m_hiResShadowMaxExtents( -1.0f )
{
	ASSERT( viewport );

	ResetCommonSettings();
	
	Red::System::MemoryCopy( m_renderingMask, viewport->GetRenderingMask(), sizeof( Bool ) * SHOW_MAX_INDEX );

	RefreshCameraParams();

	m_frameTime = static_cast< Float >( Red::System::Clock::GetInstance().GetTimer().GetSeconds() );

#ifdef RED_PLATFORM_DURANGO
	//used in empty frame
	m_multiplanePresent = true;
#endif
}

CRenderFrameInfo::CRenderFrameInfo( Uint32 width, Uint32 height, ERenderingMode mode, const CRenderCamera& camera, EShowFlags singleShowFlag )
	: m_viewport( NULL )
	, m_renderingMode( mode )
	, m_camera( camera )
	, m_allowPostSceneRender( true )
	, m_forceGBufferClear( false )
	, m_occlusionCamera( camera )
	, m_present( true )
	, m_width( width )
	, m_height( height )
	, m_canvasToWorld( Matrix::IDENTITY )
	, m_instantAdaptation( true )
	, m_clearColor( Color::BLACK )
	, m_worldTime( 0.0f )
	, m_engineTime( 0.0f )
	, m_cleanEngineTime( 0.0f )
	, m_gameTime( 0.0f )
	, m_gameDays( 0 )
	, m_envParametersArea( GetDefaultEnvParams() )
	, m_materialDebugMode( MDM_None )
	, m_isLastFrameForced( false )
	, m_forceFade( false )
	, m_drawHUD( true )
	, m_renderFeaturesFlags ( (Uint32)DMFF_MASK_ALL )
	, m_allowSequentialCapture( false )
	, m_multiplanePresent( false )
	, m_enableFPSDisplay( false )
	, m_renderTarget( NULL )
	, m_tonemapFixedLumiance( -1.0f )
	, m_backgroundTextureColorScale(1.0,1.0,1.0,1.0)
	, m_isWorldScene( false )
	, m_isGamePaused( false )
	, m_gameplayCameraLightsFactor( 1 )
	, m_customRenderResolution( false )
	, m_allowSkinningUpdate( true )
	, m_hiResShadowMaxExtents( -1.0f )
{
	ResetCommonSettings();

	// Update matrices
	UpdateMatrices();

	Red::System::MemoryZero( m_renderingMask, sizeof( Bool ) * SHOW_MAX_INDEX );
	if ( singleShowFlag == SHOW_ALL_FLAGS )
	{
		for ( Uint32 i = 0; i < SHOW_MAX_INDEX; ++i )
		{
			m_renderingMask[ i ] = true;
		}
	}
	else
	{
		m_renderingMask[ singleShowFlag ] = true;
	}

	m_frameTime = static_cast< Float >( Red::System::Clock::GetInstance().GetTimer().GetSeconds() );

#ifdef RED_PLATFORM_DURANGO
	m_multiplanePresent = true;
#endif
}

CRenderFrameInfo::CRenderFrameInfo( Uint32 width, Uint32 height, ERenderingMode mode, const EShowFlags* mask, const CRenderCamera& camera )
	: m_viewport( NULL )
	, m_renderingMode( mode )
	, m_camera( camera )
	, m_allowPostSceneRender( true )
	, m_forceGBufferClear( false )
	, m_occlusionCamera( camera )
	, m_present( true )
	, m_width( width )
	, m_height( height )
	, m_canvasToWorld( Matrix::IDENTITY )
	, m_instantAdaptation( true )
	, m_clearColor( Color::BLACK )
	, m_worldTime( 0.0f )
	, m_engineTime( 0.0f )
	, m_cleanEngineTime( 0.0f )
	, m_gameTime( 0.0f )
	, m_gameDays( 0 )
	, m_envParametersArea( GetDefaultEnvParams() )
	, m_materialDebugMode( MDM_None )
	, m_drawHUD( true )
	, m_renderFeaturesFlags ( (Uint32)DMFF_MASK_ALL )
	, m_allowSequentialCapture( false )
	, m_multiplanePresent( false )
	, m_enableFPSDisplay( false )
	, m_renderTarget( NULL )
	, m_tonemapFixedLumiance( -1.0f )
	, m_backgroundTextureColorScale(1.0,1.0,1.0,1.0)
	, m_isWorldScene( false )
	, m_isGamePaused( false )
	, m_gameplayCameraLightsFactor( 1 )
	, m_customRenderResolution( false )
	, m_allowSkinningUpdate( true )
	, m_hiResShadowMaxExtents( -1.0f )
{
	ResetCommonSettings();

	Red::System::MemoryZero( m_renderingMask, sizeof( Bool ) * SHOW_MAX_INDEX );
	for ( Uint32 i = 0; mask[ i ] < SHOW_MAX_INDEX; ++i )
	{
		m_renderingMask[ mask[ i ] ] = true;
	}

	// Update matrices
	UpdateMatrices();

#ifdef RED_PLATFORM_DURANGO
	m_multiplanePresent = true;
#endif
}

void CRenderFrameInfo::ResetCommonSettings()
{
	// reset to some defaults, good for editor, game overrides this settings anyway
	m_requestedNumCascades = 2;
	m_cascadeEndDistances[0] = 4.0f;
	m_cascadeEndDistances[1] = 18.0f;
	m_cascadeEndDistances[2] = 50.0f;
	m_cascadeEndDistances[3] = 120.0f;
	m_cascadeFilterSizes[0] = 0.01f;
	m_cascadeFilterSizes[1] = 0.01f;
	m_cascadeFilterSizes[2] = 0.01f;
	m_cascadeFilterSizes[3] = 0.01f;
	m_shadowEdgeFade[0] = 1.0f;
	m_shadowEdgeFade[1] = 2.0f;
	m_shadowEdgeFade[2] = 4.0f;
	m_shadowEdgeFade[3] = 8.0f;
	m_shadowBiasOffsetSlopeMul = 1;
	m_shadowBiasOffsetConst = 0.f;
	m_shadowBiasCascadeMultiplier = 0.f;
	m_speedTreeCascadeFilterSizes[0] = 0.01f;
	m_speedTreeCascadeFilterSizes[1] = 0.01f;
	m_speedTreeCascadeFilterSizes[2] = 0.01f;
	m_speedTreeCascadeFilterSizes[3] = 0.01f;
	m_shadowBiasOffsetConstPerCascade[0] = 0;
	m_shadowBiasOffsetConstPerCascade[1] = 0;
	m_shadowBiasOffsetConstPerCascade[2] = 0;
	m_shadowBiasOffsetConstPerCascade[3] = 0;
	m_speedTreeShadowGradient = 0.f;
	m_hiResShadowBiasOffsetSlopeMul = CWorldShadowConfig::DefaultHiResShadowBiasOffsetSlopeMul();
	m_hiResShadowBiasOffsetConst = CWorldShadowConfig::DefaultHiResShadowBiasOffsetConst();
	m_hiResShadowTexelRadius = CWorldShadowConfig::DefaultHiResShadowTexelRadius();
	m_terrainShadowsDistance = 0.0f;
	m_terrainMeshShadowDistance = 0.0f;
	m_terrainMeshShadowFadeRange = 0.0f;
	m_terrainShadowsBaseSmoothing = 1.0f;
	m_terrainShadowsTerrainDistanceSoftness = 0.0f;
	m_terrainShadowsMeshDistanceSoftness = 0.0f;
}

void CRenderFrameInfo::RefreshCameraParams()
{
	// Get camera for this frame using viewport hook
	if ( m_viewport )
	{
		if ( m_viewport->GetViewportHook() )
		{
			m_viewport->GetViewportHook()->OnViewportCalculateCamera( m_viewport, m_camera );
			m_occlusionCamera = m_camera;
		}
	}

	// Update matrices
	UpdateMatrices();
}

void CRenderFrameInfo::UpdateEnvCameraParams()
{
	Bool changed = false;

	if ( !m_camera.GetNonDefaultNearRenderingPlane() )
	{
		Float nearPlane = Clamp( m_worldRenderSettings.m_cameraNearPlane, 0.2f, 50.f );

		if ( m_envParametersDayPoint.m_nearPlaneWeightForced < 1.f )
		{
			ASSERT( m_envParametersDayPoint.m_nearPlaneWeightForced >= 0.f );

			nearPlane = Max( m_envParametersDayPoint.m_nearPlaneWeightForced * Min( 1.0f, nearPlane ) , 0.2f );
		}

		m_camera.SetNearPlane( nearPlane );
		
		changed = true;
	}

	if ( !m_camera.GetNonDefaultFarRenderingPlane() )
	{
		m_camera.SetFarPlane( m_worldRenderSettings.m_cameraFarPlane );
		changed = true;
	}

	if ( changed )
	{
		m_camera.CalculateMatrices(); 
		m_occlusionCamera = m_camera;

		// Update matrices
		UpdateMatrices();
	}
}

Float CRenderFrameInfo::CalcScreenSpaceScale( const Vector& centerPoint ) const
{
	// Transform point from world space to screen space
	Vector worldSpacePoint( centerPoint.X, centerPoint.Y, centerPoint.Z, 1.0f );
	Vector screenSpacePoint = m_camera.GetWorldToScreen().TransformVectorWithW( worldSpacePoint );

	// Calculate render scale ( empirical :P )
	const Matrix& projectionMatrix = m_camera.GetViewToScreen();
	return screenSpacePoint.W * ( 256.0f / m_width / projectionMatrix.V[0].X );
}

void CRenderFrameInfo::ProjectPoints( const Vector* points, Vector* screenSpacePoints, Uint32 count ) const
{
	if ( count == 0 )
		return;

	ASSERT( screenSpacePoints );

	// Get viewport dimensions
	const Float halfWidth = m_width * 0.5f;
	const Float halfHeight = m_height * 0.5f;

	// Project points
	for ( Uint32 i=0; i<count; i++ )
	{
		// Transform to screen space
		Vector worldSpacePoint( points[i].X, points[i].Y, points[i].Z, 1.0f );
		Vector screenSpacePoint = m_camera.GetWorldToScreen().TransformVectorWithW( worldSpacePoint );

		// Behind near plane
		if ( screenSpacePoint.W < 0.001f )
		{
			// Clamp to one of the screen corners
			screenSpacePoint.X = ( screenSpacePoint.X < 0.0f) ? -1.0f : 1.0f;
			screenSpacePoint.Y = ( screenSpacePoint.Y < 0.0f) ? -1.0f : 1.0f;
			screenSpacePoint.Z = 0.0f;
			screenSpacePoint.W = 0.0f;
		}
		else
		{
			// Vertex inside screen space, project
			screenSpacePoint.Div4( screenSpacePoint.W );
		}

		// Convert to pixel coordinates
		screenSpacePoints[i].X = ( screenSpacePoint.X + 1.0f ) * halfWidth;
		screenSpacePoints[i].Y = ( -screenSpacePoint.Y + 1.0f ) * halfHeight;
		screenSpacePoints[i].Z = screenSpacePoint.Z;
		screenSpacePoints[i].W = 1.0f;
	}
}

void CRenderFrameInfo::UpdateMatrices()
{
	// Calculate canvas matrix, used for 2D drawing
	Matrix canvasToScreen( 
		Vector( 2.0f / (Float)m_width, 0.0f, 0.0f, 0.0f ),
		Vector( 0.0f, -2.0f / (Float)m_height, 0.0f, 0.0f ),
		Vector( 0.0f, 0.0f, 1.0f, 0.0f ),
		Vector( -1.0f /*+ 0.5f / (Float)GetWidth()*/, 1.0f /*- 0.5f / (Float)GetHeight()*/, 0.0f, 1.0f )
	);

	// Canvas matrix transforms from 2D screen space to world space
	m_canvasToWorld = canvasToScreen * m_camera.GetScreenToWorld();
}

//dex++: Initialize frame shadow info from world shadow info
void CRenderFrameInfo::SetShadowConfig( const CWorldShadowConfig& params )
{
	// cascades
	const Vector cascadeExtraDistance = Vector( 
		Config::cvCascadeShadowDistanceScale0.Get(), 
		Config::cvCascadeShadowDistanceScale1.Get(), 
		Config::cvCascadeShadowDistanceScale2.Get(), 
		Config::cvCascadeShadowDistanceScale3.Get() );

	m_requestedNumCascades = Clamp( params.m_numCascades, 1, MAX_CASCADES );

	const Float extraAnselDistanceMultiplier = 
#ifdef USE_ANSEL
	isAnselSessionActive ? 1.4f :
#endif // USE_ANSEL
	1.f;

	m_cascadeEndDistances[0] = Max<Float>( ( params.m_cascadeRange1 * extraAnselDistanceMultiplier ) * cascadeExtraDistance.X, 1.0f );
	m_cascadeEndDistances[1] = Max<Float>( ( params.m_cascadeRange2 * extraAnselDistanceMultiplier ) * cascadeExtraDistance.Y, m_cascadeEndDistances[0] + 1.0f );
	m_cascadeEndDistances[2] = Max<Float>( ( params.m_cascadeRange3 * extraAnselDistanceMultiplier ) * cascadeExtraDistance.Z, m_cascadeEndDistances[1] + 1.0f );
	m_cascadeEndDistances[3] = Max<Float>( ( params.m_cascadeRange4 * extraAnselDistanceMultiplier ) * cascadeExtraDistance.W, m_cascadeEndDistances[2] + 1.0f );

	// Shadow cascades filters. They will be clamped to texel size later
	m_cascadeFilterSizes[0] = Max<Float>( 0.0f, params.m_cascadeFilterSize1 );
	m_cascadeFilterSizes[1] = Max<Float>( 0.0f, params.m_cascadeFilterSize2 );
	m_cascadeFilterSizes[2] = Max<Float>( 0.0f, params.m_cascadeFilterSize3 );
	m_cascadeFilterSizes[3] = Max<Float>( 0.0f, params.m_cascadeFilterSize4 );

	// Clamp edge values to 5-20% of shadow size
	m_shadowEdgeFade[0] = Clamp<Float>( params.m_shadowEdgeFade1 , m_cascadeEndDistances[0] * 0.05f , m_cascadeEndDistances[0] * 0.2f );
	m_shadowEdgeFade[1] = Clamp<Float>( params.m_shadowEdgeFade2 , m_cascadeEndDistances[1] * 0.05f , m_cascadeEndDistances[1] * 0.2f );
	m_shadowEdgeFade[2] = Clamp<Float>( params.m_shadowEdgeFade3 , m_cascadeEndDistances[2] * 0.05f , m_cascadeEndDistances[2] * 0.2f );
	m_shadowEdgeFade[3] = Clamp<Float>( params.m_shadowEdgeFade4 , m_cascadeEndDistances[3] * 0.05f , m_cascadeEndDistances[3] * 0.2f );

	m_shadowBiasOffsetConstPerCascade[0] = Max<Float>( 0.0f, params.m_shadowBiasOffsetConstCascade1 );
	m_shadowBiasOffsetConstPerCascade[1] = Max<Float>( 0.0f, params.m_shadowBiasOffsetConstCascade2 );
	m_shadowBiasOffsetConstPerCascade[2] = Max<Float>( 0.0f, params.m_shadowBiasOffsetConstCascade3 );
	m_shadowBiasOffsetConstPerCascade[3] = Max<Float>( 0.0f, params.m_shadowBiasOffsetConstCascade4 );
	m_shadowBiasOffsetSlopeMul = params.m_shadowBiasOffsetSlopeMul;
	m_shadowBiasOffsetConst = params.m_shadowBiasOffsetConst;
	m_shadowBiasCascadeMultiplier = params.m_shadowBiasCascadeMultiplier;
	m_hiResShadowBiasOffsetSlopeMul = params.m_hiResShadowBiasOffsetSlopeMul;
	m_hiResShadowBiasOffsetConst = params.m_hiResShadowBiasOffsetConst;
	m_hiResShadowTexelRadius = params.m_hiResShadowTexelRadius;

	// speed tree shadow filter
	m_speedTreeCascadeFilterSizes[0] = params.m_speedTreeShadowFilterSize1;
	m_speedTreeCascadeFilterSizes[1] = params.m_speedTreeShadowFilterSize2;
	m_speedTreeCascadeFilterSizes[2] = params.m_speedTreeShadowFilterSize3;
	m_speedTreeCascadeFilterSizes[3] = params.m_speedTreeShadowFilterSize4;
	for ( Uint32 i=0; i<4; ++i )
	{
		if ( m_speedTreeCascadeFilterSizes[i] < 0 )
		{
			m_speedTreeCascadeFilterSizes[i] = m_cascadeFilterSizes[i];
			ASSERT( m_speedTreeCascadeFilterSizes[i] >= 0 );
		}
	}
	m_speedTreeShadowGradient = Max<Float>( 0.0f,  params.m_speedTreeShadowGradient );

	// terrain shadows
	m_terrainShadowsDistance = params.m_useTerrainShadows ? Max<Float>( 0.0f, params.m_terrainShadowsDistance ) : 0.0f;
	m_terrainShadowsFadeRange = Max<Float>( 0.0f, Min<Float>( m_terrainShadowsDistance, params.m_terrainShadowsFadeRange ) );
	m_terrainMeshShadowDistance = Max<Float>( 0.0f, params.m_terrainMeshShadowDistance );
	m_terrainMeshShadowFadeRange = Max<Float>( 0.0f, Min<Float>( m_terrainMeshShadowDistance, params.m_terrainMeshShadowFadeRange ) );
	m_terrainShadowsBaseSmoothing = Clamp<Float>( params.m_terrainShadowsBaseSmoothing, 0.1f, 10.0f );
	m_terrainShadowsTerrainDistanceSoftness = Max<Float>( params.m_terrainShadowsTerrainDistanceSoftness, 0.0f );
	m_terrainShadowsMeshDistanceSoftness = Max<Float>( Min<Float>( params.m_terrainShadowsMeshDistanceSoftness, params.m_terrainShadowsMeshDistanceSoftness), 0.0f );
}
//dex--

CRenderFrameInfo CRenderFrameInfo::BuildEnvProbeFaceRenderInfoBase( const CRenderFrameInfo &originalFrameInfo )
{
	CRenderFrameInfo info;

	// Unchanged - modified by renderFrameInfo adapt
	//info.m_camera
	//info.m_width
	//info.m_height

	// basic init

	info.m_renderingMode = RM_Shaded;
	info.m_allowPostSceneRender = false;
	info.m_forceGBufferClear = false;
	info.m_present = false;
	// m_canvasToWorld <- handled by UpdateMatrices()
	info.m_instantAdaptation = false;
	info.m_clearColor = Color::RED;
	info.m_worldTime = originalFrameInfo.m_worldTime;
	info.m_engineTime = originalFrameInfo.m_engineTime;
	info.m_cleanEngineTime = originalFrameInfo.m_cleanEngineTime;
	info.m_gameTime = originalFrameInfo.m_gameTime;
	info.m_gameDays = originalFrameInfo.m_gameDays;

	info.m_envParametersDayPoint.m_interiorFallbackAmbientTexture = originalFrameInfo.m_envParametersDayPoint.m_interiorFallbackAmbientTexture;
	info.m_envParametersDayPoint.m_interiorFallbackReflectionTexture = originalFrameInfo.m_envParametersDayPoint.m_interiorFallbackReflectionTexture;

	info.m_envParametersDayPoint = originalFrameInfo.m_envParametersDayPoint;
	info.m_envParametersArea = originalFrameInfo.m_envParametersArea;
	info.m_envParametersAreaBlend1 = originalFrameInfo.m_envParametersAreaBlend1;
	info.m_envParametersAreaBlend2 = originalFrameInfo.m_envParametersAreaBlend2;
	info.m_envBlendingFactor = 0;
	info.m_envParametersGame = originalFrameInfo.m_envParametersGame;
	info.m_speedTreeParameters = originalFrameInfo.m_speedTreeParameters;
	info.m_worldRenderSettings = originalFrameInfo.m_worldRenderSettings;
	info.m_baseLightingParameters = originalFrameInfo.m_baseLightingParameters;
	info.m_frameTime = originalFrameInfo.m_frameTime;

	info.m_isNonInteractiveRendering = false;
	info.m_isLastFrameForced = false;
	info.m_drawHUD = false;
	info.m_materialDebugMode = MDM_None;
	info.m_renderFeaturesFlags = DMFF_MASK_LIGHTING;
	info.m_allowSequentialCapture = false;

	info.m_requestedNumCascades = originalFrameInfo.m_requestedNumCascades;
	for ( Uint32 i=0; i<MAX_CASCADES; ++i )
	{
		info.m_cascadeEndDistances[i] = originalFrameInfo.m_cascadeEndDistances[i];
		info.m_cascadeFilterSizes[i] = originalFrameInfo.m_cascadeFilterSizes[i];
		info.m_speedTreeCascadeFilterSizes[i] = originalFrameInfo.m_speedTreeCascadeFilterSizes[i];
		info.m_shadowBiasOffsetConstPerCascade[i] = originalFrameInfo.m_shadowBiasOffsetConstPerCascade[i];
	}
	info.m_speedTreeShadowGradient = originalFrameInfo.m_speedTreeShadowGradient;
	info.m_shadowBiasOffsetSlopeMul = originalFrameInfo.m_shadowBiasOffsetSlopeMul;
	info.m_shadowBiasOffsetConst = originalFrameInfo.m_shadowBiasOffsetConst;
	info.m_shadowBiasCascadeMultiplier = originalFrameInfo.m_shadowBiasCascadeMultiplier;
	info.m_hiResShadowBiasOffsetSlopeMul = originalFrameInfo.m_hiResShadowBiasOffsetSlopeMul;
	info.m_hiResShadowBiasOffsetConst = originalFrameInfo.m_hiResShadowBiasOffsetConst;
	info.m_hiResShadowTexelRadius = originalFrameInfo.m_hiResShadowTexelRadius;

	info.m_terrainShadowsDistance = originalFrameInfo.m_terrainShadowsDistance;
	info.m_terrainShadowsFadeRange = originalFrameInfo.m_terrainShadowsFadeRange;
	info.m_terrainMeshShadowDistance = originalFrameInfo.m_terrainMeshShadowDistance;
	info.m_terrainMeshShadowFadeRange = originalFrameInfo.m_terrainMeshShadowFadeRange;
	info.m_terrainShadowsBaseSmoothing = originalFrameInfo.m_terrainShadowsBaseSmoothing;
	info.m_terrainShadowsTerrainDistanceSoftness = originalFrameInfo.m_terrainShadowsTerrainDistanceSoftness;
	info.m_terrainShadowsMeshDistanceSoftness = originalFrameInfo.m_terrainShadowsMeshDistanceSoftness;

	info.UpdateMatrices();

	// No sun and moon in the probes
	{
		info.m_envParametersArea.m_sunParams.m_sunSize.SetValueScalar( 0 );
		info.m_envParametersArea.m_sunParams.m_moonSize.SetValueScalar( 0 );
	}

	// No fog (it's being added in a separate pass)
	info.m_envParametersArea.m_globalFog.SetDisabledParams();

	// Disable camera lights
	info.m_cameraLightsModifiersSetup.SetDisabled();
	info.m_envParametersArea.m_cameraLightsSetup.SetDisabledValues();
	info.m_envParametersAreaBlend1.m_cameraLightsSetup.SetDisabledValues();
	info.m_envParametersAreaBlend2.m_cameraLightsSetup.SetDisabledValues();

	return info;
}

void CRenderFrameInfo::AdaptEnvProbeFaceRenderInfo( const CRenderCamera &camera, Uint32 width, Uint32 height )
{
	m_camera	= camera;
	m_width		= width;
	m_height	= height;
}

bool CRenderFrameInfo::IsCloudsShadowVisible() const
{	
	return 
		m_envParametersGame.m_displaySettings.m_allowCloudsShadow &&
		( !m_envParametersDayPoint.m_cloudsShadowTexture.IsNull() || !m_envParametersDayPoint.m_fakeCloudsShadowTexture.IsNull() );
}

void CRenderFrameInfo::SetGameTime( Float gameTime, Uint32 gameDays )
{ 
	m_gameTime = gameTime; 
	m_gameDays = gameDays;
}

void CRenderFrameInfo::SetGameplayRenderTarget( class IRenderGameplayRenderTarget* renderTarget )
{
	RED_FATAL_ASSERT( !m_renderTarget , "You cannor set twice renderTarget on render frame info" );
	m_renderTarget = renderTarget;
}

void CRenderFrameInfo::SetDayPointEnvParams( const SDayPointEnvironmentParams& params )
{
	m_envParametersDayPoint = params;
}

void CRenderFrameInfo::SetAreaEnvParams( const CAreaEnvironmentParamsAtPoint& params )
{
	m_envParametersArea = params;
}

void CRenderFrameInfo::SetGameEnvParams( const CGameEnvironmentParams& params )
{
	m_envParametersGame = params;
}

void CRenderFrameInfo::SetBaseLightingParams( const CRenderBaseLightParams& params )
{
	m_baseLightingParameters = params;
}

void CRenderFrameInfo::SetSpeedTreeParams( const SGlobalSpeedTreeParameters& params )
{
	m_speedTreeParameters = params;
}

void CRenderFrameInfo::SetWorldRenderSettings( const SWorldRenderSettings& params )
{
	m_worldRenderSettings = params;
}

void CRenderFrameInfo::SetCameraLightModifiers( const SCameraLightsModifiersSetup& params )
{
	m_cameraLightsModifiersSetup = params;
}

void CRenderFrameInfo::SetFrameTime( Float frameWorldTime )
{
	m_worldTime = frameWorldTime;
}

void CRenderFrameInfo::SetMaterialDebugMode( EMaterialDebugMode mdm )
{
	m_materialDebugMode = mdm;
}

void CRenderFrameInfo::SetSubpixelOffset( Float x, Float y, Uint32 screenW, Uint32 screenH )
{
	m_camera.SetSubpixelOffset( x , y , screenW , screenH );
	m_occlusionCamera.SetSubpixelOffset( x , y , screenW , screenH );
}

void CRenderFrameInfo::SetSize( Uint32 width, Uint32 height )
{
	m_width = width;
	m_height = height;
}

Float CRenderFrameInfo::GetRenderingDebugOption( EVisualDebugCommonOptions option  ) const
{
	if( m_viewport )
	{
		return m_viewport->GetRenderingDebugOption( option );
	}
	return 0.0f;
}

Bool CRenderFrameInfo::IsClassRenderingDisabled( CClass * type ) const
{
	if( m_viewport )
	{
		return m_viewport->IsClassRenderingDisabled( type );
	}
	return true;
}

Bool CRenderFrameInfo::IsTemplateRenderingDisabled( const CEntityTemplate * entTemplate, const CClass* componentClass ) const
{
	if( m_viewport )
	{
		return m_viewport->IsTemplateRenderingDisabled( entTemplate, componentClass );
	}
	return true;
}

Bool CRenderFrameInfo::IsTerrainToolStampVisible() const
{
	if( m_viewport )
	{
		return m_viewport->IsTerrainToolStampVisible();
	}
	return false;
}

Bool CRenderFrameInfo::IsGrassMaskPaintMode() const
{
	if( m_viewport )
	{
		return m_viewport->IsGrassMaskPaintMode();
	}
	return false;
}
