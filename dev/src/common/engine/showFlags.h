/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

/// Show flags for rendering mask
enum EShowFlags
{
	SHOW_Meshes,
	SHOW_AI,
	SHOW_AISenses,
	SHOW_AIRanges,
	SHOW_Camera,
	SHOW_HierarchicalGrid,
	SHOW_NavMesh,
	SHOW_Shadows,
	SHOW_ForceAllShadows,
	SHOW_Lights,
	SHOW_Terrain,
	SHOW_Foliage,
	SHOW_PostProcess,
	SHOW_Selection,
	SHOW_Grid,
	SHOW_Sprites,
	SHOW_Particles,
	SHOW_Wireframe,
	SHOW_WalkSurfaces,
	SHOW_Areas,
	SHOW_Locomotion,
	SHOW_MovableRep,
	SHOW_Logic,
	SHOW_Profilers,
	SHOW_Paths,
	SHOW_Stickers,
	SHOW_Steering,
	SHOW_Bboxes,
	SHOW_Exploration,
	SHOW_Behavior,
	SHOW_Emissive,
	SHOW_Spawnset,
	SHOW_Collision,
	SHOW_GUI,
	SHOW_VisualDebug,
	SHOW_Decals,
	SHOW_Lighting,
	SHOW_TSLighting,
	SHOW_Refraction,
	SHOW_Encounter,
	SHOW_BboxesParticles,
	SHOW_BboxesTerrain,
	SHOW_NavRoads,
	SHOW_Brushes,
	SHOW_BuildingBrush,
	SHOW_Sound,
	SHOW_Gizmo,
	SHOW_Flares,
	SHOW_FlaresData,
	SHOW_GUIFallback,
	SHOW_OnScreenMessages,
	SHOW_DepthOfField,
	SHOW_LightsBBoxes,
	SHOW_ErrorState,
	SHOW_Devices,
	SHOW_SelectionContacts,
	SHOW_PEObstacles,
	SHOW_Bg,
	SHOW_TriangleStats,
	SHOW_Formations,
	SHOW_Skirt,
	SHOW_Wind,
	SHOW_NonClimbable,
	SHOW_MergedMeshes,
	SHOW_UseVisibilityMask,
	SHOW_ShadowMeshDebug,

	SHOW_NavTerrain,
	SHOW_NavGraph,
	SHOW_NavObstacles,
	SHOW_Apex,
	SHOW_SpeedTreeShadows,
	SHOW_ShadowStats,
	SHOW_ShadowPreviewDynamic,
	SHOW_ShadowPreviewStatic,
	SHOW_PhysXVisualization,
	SHOW_PhysXTraceVisualization,
    SHOW_PhysXPlatforms,
	SHOW_PhysXMaterials,
	SHOW_BboxesDecals,
	SHOW_BboxesSmallestHoleOverride,
	SHOW_SpritesDecals,
	SHOW_HiResEntityShadows,

	SHOW_ApexDebugAll,
	SHOW_ApexClothCollisionSolid,
	SHOW_ApexClothSkeleton,
	SHOW_ApexClothBackstop,
	SHOW_ApexClothMaxDistance,
	SHOW_ApexClothVelocity,
	SHOW_ApexClothSkinnedPosition,
	SHOW_ApexClothActiveTethers,
	SHOW_ApexClothLength,
	SHOW_ApexClothCrossSection,
	SHOW_ApexClothBending,
	SHOW_ApexClothShearing,
	SHOW_ApexClothZeroStretch,
	SHOW_ApexClothSelfCollision,
	SHOW_ApexClothVirtualCollision,
	SHOW_ApexClothPhysicsMeshSolid,
	SHOW_ApexClothPhysicsMeshWire,
	SHOW_ApexClothWind,
	SHOW_ApexClothBackstopPrecise,
	SHOW_ApexClothCollisionWire,
	SHOW_ApexClothLocalSpace,
	SHOW_ApexDestructSupport,
	SHOW_ApexStats,

	SHOW_DistantLights,
	SHOW_CollisionIfNotVisible,

	SHOW_Projectile,
	SHOW_EnvProbesInstances,
	SHOW_TerrainHoles,
	SHOW_CollisionSoundOcclusion,
	SHOW_Dimmers,
	SHOW_DimmersBBoxes,
	SHOW_PhysActorDampers,
	SHOW_PhysActorVelocities,
	SHOW_PhysActorMasses,
	SHOW_TBN,
	SHOW_TonemappingCurve,
	SHOW_ApexFractureRatio,
	SHOW_AnimatedProperties,
	SHOW_PhysMotionIntensity,
	SHOW_AreaShapes,
	SHOW_TriggerBounds,
	SHOW_TriggerActivators,
	SHOW_TriggerTree,
	SHOW_SoundReverb,
	SHOW_CurveAnimations,
	SHOW_AnimDangles,
	SHOW_Wetness,
	SHOW_CameraVisibility,

	SHOW_StreamingTree0,
	SHOW_StreamingTree1,
	SHOW_StreamingTree2,
	SHOW_StreamingTree3,
	SHOW_StreamingTree4,
	SHOW_StreamingTree5,
	SHOW_StreamingTree6,
	SHOW_StreamingTree7,

	SHOW_EnvProbeStats,
	SHOW_FocusMode,
	SHOW_AIBehTree,
	SHOW_NavMeshTriangles,
	SHOW_NavGraphNoOcclusion,
	SHOW_PhantomShapes,
	SHOW_Crowd,

	SHOW_SoundEmitters,
	SHOW_SoundAmbients,
	SHOW_GenericGrass,

	SHOW_ApexFracturePoints,
	SHOW_ApexThresoldLeft,

	SHOW_Underwater,
	SHOW_Scenes,

	SHOW_BoatSailing,
	// Umbra
	SHOW_OcclusionStats,
	SHOW_UmbraFrustum,
	SHOW_UmbraObjectBounds,
	SHOW_UmbraPortals,
	SHOW_UmbraVisibleVolume,
	SHOW_UmbraViewCell,
	SHOW_UmbraVisibilityLines,
	SHOW_UmbraStatistics,
	SHOW_UmbraOcclusionBuffer,
	SHOW_UmbraCullShadows,
	SHOW_UmbraCullSpeedTreeShadows,
	SHOW_UmbraShowOccludedNonStaticGeometry,
	SHOW_UmbraStreamingVisualization,
	SHOW_UmbraCullTerrain,
	SHOW_UmbraCullFoliage,
	SHOW_UmbraShowFoliageCells,

	SHOW_DynamicDecals,

	SHOW_Swarms,
	SHOW_TerrainStats,

	SHOW_EnvProbesOverlay,

	SHOW_Interactions,
	SHOW_AIActions,
	SHOW_Stripes,

	SHOW_Waypoints,
	SHOW_RoadFollowing,
	SHOW_MapTracking,
	SHOW_QuestMapPins,

	SHOW_FlareOcclusionShapes,

	SHOW_ReversedProjection,
	SHOW_CascadesStabilizedRotation,
	SHOW_DynamicCollector,
	SHOW_DynamicComponent,
	SHOW_MeshComponent,
	SHOW_LodInfo,
	SHOW_NavMeshOverlay,
	SHOW_ActorLodInfo,

	SHOW_LocalReflections,
	SHOW_EnvProbesBigOverlay,
	SHOW_VideoOutputLimited,
	SHOW_BoatHedgehog,
	SHOW_BoatDestruction,
	SHOW_BoatBuoyancy,
	SHOW_BoatPathFollowing,

	SHOW_Histogram,
	SHOW_VolumetricPaths,

	SHOW_PigmentMap,

	SHOW_PreferTemporalAA,
	SHOW_AllowDebugPreviewTemporalAA,

	SHOW_AllowApexShadows,

	SHOW_NodeTags,

	SHOW_HairAndFur,
	SHOW_ForwardPass,
	SHOW_GeometrySkinned,
	SHOW_GeometryStatic,
	SHOW_GeometryProxies,
	SHOW_NavGraphRegions,

	SHOW_Containers,
	SHOW_RenderTargetBackground,

	SHOW_AITickets,

	SHOW_EffectAreas,
	SHOW_PathSpeedValues,


	SHOW_PhysActorIterations,

	SHOW_BoatInput,
	SHOW_Dismemberment,

	SHOW_EntityVisibility,
	SHOW_AIBehaviorDebug,

	SHOW_XB1SafeArea,
	SHOW_PS4SafeArea,

	SHOW_Skybox,

	SHOW_CameraLights,
	SHOW_AntiLightbleed,

	SHOW_BoatWaterProbbing,

	SHOW_UseRegularDeferred,

	SHOW_CommunityAgents,

	SHOW_RenderDeepTerrain,
	SHOW_CullTerrainWithFullHeight,

	SHOW_CameraInteriorFactor,
	SHOW_BrowseDebugPreviews,

	SHOW_BBoxesCloth,
	SHOW_BBoxesFur,
	SHOW_BBoxesDestruction,
	SHOW_PhysActorFloatingRatio,

	SHOW_GameplayPostFx,

	SHOW_SimpleBuoyancy,

	SHOW_ScaleformMemoryInfo,

	SHOW_GameplayLightComponent,
	SHOW_TeleportDetector,
	SHOW_SoundListener,

	SHOW_NavTerrainHeight,
	SHOW_StreamingCollisionBoxes,

	SHOW_RenderGPUProfiler,
	SHOW_TemplateLoadBalancer,

	SHOW_DistantLightsDebug,

	SHOW_MAX_INDEX,

	SHOW_ALL_FLAGS					= 0xffffffff
};

BEGIN_ENUM_RTTI( EShowFlags )
	ENUM_OPTION( SHOW_Meshes								)
	ENUM_OPTION( SHOW_AI									)
	ENUM_OPTION( SHOW_AISenses								)
	ENUM_OPTION( SHOW_AIRanges								)
	ENUM_OPTION( SHOW_Camera								)
	ENUM_OPTION( SHOW_HierarchicalGrid						)
	ENUM_OPTION( SHOW_NavMesh								)
	ENUM_OPTION( SHOW_Shadows								)
	ENUM_OPTION( SHOW_ForceAllShadows						)
	ENUM_OPTION( SHOW_Lights								)
	ENUM_OPTION( SHOW_DistantLights							)
	ENUM_OPTION( SHOW_Terrain								)
	ENUM_OPTION( SHOW_Foliage								)
	ENUM_OPTION( SHOW_LocalReflections						)
	ENUM_OPTION( SHOW_PostProcess							)
	ENUM_OPTION( SHOW_Selection								)
	ENUM_OPTION( SHOW_Grid									)
	ENUM_OPTION( SHOW_Sprites								)
	ENUM_OPTION( SHOW_EnvProbesInstances					)
	ENUM_OPTION( SHOW_EnvProbesOverlay						)
	ENUM_OPTION( SHOW_EnvProbesBigOverlay					)
	ENUM_OPTION( SHOW_VideoOutputLimited					)
	ENUM_OPTION( SHOW_FlareOcclusionShapes					)
	ENUM_OPTION( SHOW_Particles								)
	ENUM_OPTION( SHOW_Wireframe								)
	ENUM_OPTION( SHOW_WalkSurfaces							)
	ENUM_OPTION( SHOW_Areas									)
	ENUM_OPTION( SHOW_AreaShapes							)
	ENUM_OPTION( SHOW_TriggerBounds							)
	ENUM_OPTION( SHOW_TriggerTree							)
	ENUM_OPTION( SHOW_TriggerActivators						)
	ENUM_OPTION( SHOW_Locomotion							)
	ENUM_OPTION( SHOW_MovableRep							)
	ENUM_OPTION( SHOW_Logic									)
	ENUM_OPTION( SHOW_Profilers								)
	ENUM_OPTION( SHOW_Paths									)
	ENUM_OPTION( SHOW_Stickers								)
	ENUM_OPTION( SHOW_Steering								)
	ENUM_OPTION( SHOW_Bboxes								)
	ENUM_OPTION( SHOW_Exploration							)
	ENUM_OPTION( SHOW_Behavior								)
	ENUM_OPTION( SHOW_Lighting								)
	ENUM_OPTION( SHOW_TSLighting							)
	ENUM_OPTION( SHOW_Emissive								)
	ENUM_OPTION( SHOW_Spawnset								)
	ENUM_OPTION( SHOW_Collision								)
	ENUM_OPTION( SHOW_CollisionIfNotVisible					)
	ENUM_OPTION( SHOW_GUI									)
	ENUM_OPTION( SHOW_VisualDebug							)
	ENUM_OPTION( SHOW_Decals								)
	ENUM_OPTION( SHOW_DynamicDecals							)
	ENUM_OPTION( SHOW_Refraction							)
	ENUM_OPTION( SHOW_Encounter								)
	ENUM_OPTION( SHOW_BboxesParticles						)
	ENUM_OPTION( SHOW_BboxesTerrain							)
	ENUM_OPTION( SHOW_NavRoads								)
	ENUM_OPTION( SHOW_Brushes								)
	ENUM_OPTION( SHOW_BuildingBrush							)
	ENUM_OPTION( SHOW_Sound									)
	ENUM_OPTION( SHOW_SoundEmitters							)
	ENUM_OPTION( SHOW_SoundAmbients							)
	ENUM_OPTION( SHOW_Gizmo									)
	ENUM_OPTION( SHOW_Flares								)
	ENUM_OPTION( SHOW_FlaresData							)
	ENUM_OPTION( SHOW_GUIFallback							)
	ENUM_OPTION( SHOW_OnScreenMessages						)
	ENUM_OPTION( SHOW_DepthOfField							)
	ENUM_OPTION( SHOW_LightsBBoxes							)
	ENUM_OPTION( SHOW_ErrorState							)
	ENUM_OPTION( SHOW_Devices								)
	ENUM_OPTION( SHOW_SelectionContacts						)
	ENUM_OPTION( SHOW_PEObstacles							)
	ENUM_OPTION( SHOW_Bg									)
	ENUM_OPTION( SHOW_TriangleStats							)
	ENUM_OPTION( SHOW_EnvProbeStats							)
	ENUM_OPTION( SHOW_PreferTemporalAA						)
	ENUM_OPTION( SHOW_Underwater							)
	ENUM_OPTION( SHOW_Formations							)
	ENUM_OPTION( SHOW_NonClimbable							)
	ENUM_OPTION( SHOW_MergedMeshes							)
	ENUM_OPTION( SHOW_UseVisibilityMask						)
	ENUM_OPTION( SHOW_ShadowMeshDebug						)
	ENUM_OPTION( SHOW_StreamingCollisionBoxes				)

	ENUM_OPTION( SHOW_StreamingTree0						)
	ENUM_OPTION( SHOW_StreamingTree1						)
	ENUM_OPTION( SHOW_StreamingTree2						)
	ENUM_OPTION( SHOW_StreamingTree3						)
	ENUM_OPTION( SHOW_StreamingTree4						)
	ENUM_OPTION( SHOW_StreamingTree5						)
	ENUM_OPTION( SHOW_StreamingTree6						)
	ENUM_OPTION( SHOW_StreamingTree7						)

	ENUM_OPTION( SHOW_Skirt									)
	ENUM_OPTION( SHOW_Wind									)
	ENUM_OPTION( SHOW_NavTerrain							)
	ENUM_OPTION( SHOW_NavGraph								)
	ENUM_OPTION( SHOW_NavObstacles							)
	ENUM_OPTION( SHOW_Apex									)
	//dex++
	ENUM_OPTION( SHOW_SpeedTreeShadows						)
	ENUM_OPTION( SHOW_AllowApexShadows						)
	ENUM_OPTION( SHOW_ShadowStats							)
	ENUM_OPTION( SHOW_ShadowPreviewDynamic					)
	ENUM_OPTION( SHOW_ShadowPreviewStatic					)
	ENUM_OPTION( SHOW_HiResEntityShadows					)
	//dex--
	ENUM_OPTION( SHOW_PhysXVisualization					)
	ENUM_OPTION( SHOW_PhysXTraceVisualization				)
    ENUM_OPTION( SHOW_PhysXPlatforms                             )
	ENUM_OPTION( SHOW_BboxesDecals							)
	ENUM_OPTION( SHOW_BboxesSmallestHoleOverride			)
	ENUM_OPTION( SHOW_SpritesDecals							)
	ENUM_OPTION( SHOW_PhysXMaterials						)
	ENUM_OPTION_DESC( TXT("SHOW_Apex -- Enable Visualizations"), SHOW_ApexDebugAll )
	ENUM_OPTION( SHOW_ApexClothCollisionSolid				)
	ENUM_OPTION( SHOW_ApexClothCollisionWire				)
	ENUM_OPTION( SHOW_ApexClothSkeleton						)
	ENUM_OPTION( SHOW_ApexClothWind							)
	ENUM_OPTION( SHOW_ApexClothVelocity						)
	ENUM_OPTION( SHOW_ApexClothMaxDistance					)
	ENUM_OPTION( SHOW_ApexClothBackstop						)
	ENUM_OPTION( SHOW_ApexClothBackstopPrecise				)
	ENUM_OPTION( SHOW_ApexClothSkinnedPosition				)
	ENUM_OPTION( SHOW_ApexClothActiveTethers				)
	ENUM_OPTION( SHOW_ApexClothLength						)
	ENUM_OPTION( SHOW_ApexClothCrossSection					)
	ENUM_OPTION( SHOW_ApexClothBending						)
	ENUM_OPTION( SHOW_ApexClothShearing						)
	ENUM_OPTION( SHOW_ApexClothZeroStretch					)
	ENUM_OPTION( SHOW_ApexClothPhysicsMeshWire				)
	ENUM_OPTION( SHOW_ApexClothPhysicsMeshSolid				)
	ENUM_OPTION( SHOW_ApexClothSelfCollision				)
	ENUM_OPTION( SHOW_ApexClothVirtualCollision				)
	ENUM_OPTION( SHOW_ApexClothLocalSpace					)
	ENUM_OPTION( SHOW_ApexDestructSupport					)
	ENUM_OPTION( SHOW_ApexStats								)
	ENUM_OPTION( SHOW_PhysMotionIntensity					)
	ENUM_OPTION( SHOW_Projectile							)
	ENUM_OPTION( SHOW_TerrainHoles							)
	ENUM_OPTION( SHOW_CollisionSoundOcclusion				)
	ENUM_OPTION( SHOW_Dimmers								)
	ENUM_OPTION( SHOW_DimmersBBoxes							)
	ENUM_OPTION( SHOW_PhysActorDampers						)
	ENUM_OPTION( SHOW_PhysActorVelocities					)
	ENUM_OPTION( SHOW_PhysActorMasses						)
	ENUM_OPTION( SHOW_ApexFractureRatio						)
	ENUM_OPTION( SHOW_PhysMotionIntensity					)
	ENUM_OPTION( SHOW_PhysActorFloatingRatio				)
	ENUM_OPTION( SHOW_SoundReverb							)
	ENUM_OPTION( SHOW_TBN									)
	ENUM_OPTION( SHOW_TonemappingCurve						)
	ENUM_OPTION( SHOW_Histogram								)
	ENUM_OPTION( SHOW_AllowDebugPreviewTemporalAA			)
	ENUM_OPTION( SHOW_PigmentMap							)
	ENUM_OPTION( SHOW_AnimatedProperties					)
	ENUM_OPTION( SHOW_FocusMode								)
	ENUM_OPTION( SHOW_AIBehTree								)
	ENUM_OPTION( SHOW_NavMeshTriangles						)
	ENUM_OPTION( SHOW_NavGraphNoOcclusion					)
	ENUM_OPTION( SHOW_PhantomShapes							)
	ENUM_OPTION( SHOW_Crowd									)
	ENUM_OPTION( SHOW_CurveAnimations						)
	ENUM_OPTION( SHOW_AnimDangles							)
	ENUM_OPTION( SHOW_GenericGrass							)
	ENUM_OPTION( SHOW_ApexFracturePoints					)
	ENUM_OPTION( SHOW_ApexThresoldLeft						)
	ENUM_OPTION( SHOW_Scenes								)
	ENUM_OPTION( SHOW_BoatSailing							)
	ENUM_OPTION( SHOW_Swarms								)
	ENUM_OPTION( SHOW_OcclusionStats						)
	ENUM_OPTION( SHOW_UmbraFrustum							)
	ENUM_OPTION( SHOW_UmbraObjectBounds						)
	ENUM_OPTION( SHOW_UmbraPortals							)
	ENUM_OPTION( SHOW_UmbraVisibleVolume					)
	ENUM_OPTION( SHOW_UmbraViewCell							)
	ENUM_OPTION( SHOW_UmbraVisibilityLines					)
	ENUM_OPTION( SHOW_UmbraStatistics						)
	ENUM_OPTION( SHOW_UmbraOcclusionBuffer					)
	ENUM_OPTION( SHOW_UmbraCullShadows						)
	ENUM_OPTION( SHOW_UmbraCullSpeedTreeShadows				)
	ENUM_OPTION( SHOW_UmbraShowOccludedNonStaticGeometry	)
	ENUM_OPTION( SHOW_UmbraStreamingVisualization			)
	ENUM_OPTION( SHOW_UmbraCullTerrain						)
	ENUM_OPTION( SHOW_UmbraCullFoliage						)
	ENUM_OPTION( SHOW_UmbraShowFoliageCells					)
	ENUM_OPTION( SHOW_TerrainStats							)
	ENUM_OPTION( SHOW_Interactions							)
	ENUM_OPTION( SHOW_AIActions								)
	ENUM_OPTION( SHOW_Stripes								)
	ENUM_OPTION( SHOW_RoadFollowing							)
	ENUM_OPTION( SHOW_Waypoints								)
	ENUM_OPTION( SHOW_ReversedProjection					)
	ENUM_OPTION( SHOW_CascadesStabilizedRotation			)
	ENUM_OPTION( SHOW_DynamicCollector						)
	ENUM_OPTION( SHOW_DynamicComponent						)
	ENUM_OPTION( SHOW_MeshComponent							)
	ENUM_OPTION( SHOW_LodInfo								)
	ENUM_OPTION( SHOW_NavMeshOverlay						)
	ENUM_OPTION( SHOW_ActorLodInfo							)
	ENUM_OPTION( SHOW_BoatHedgehog							)
	ENUM_OPTION( SHOW_BoatDestruction						)
	ENUM_OPTION( SHOW_BoatBuoyancy							)
	ENUM_OPTION( SHOW_BoatPathFollowing						)
	ENUM_OPTION( SHOW_VolumetricPaths						)
	ENUM_OPTION( SHOW_NodeTags								)
	ENUM_OPTION( SHOW_HairAndFur							)
	ENUM_OPTION( SHOW_ForwardPass							)
	ENUM_OPTION( SHOW_GeometrySkinned						)
	ENUM_OPTION( SHOW_GeometryStatic						)
	ENUM_OPTION( SHOW_GeometryProxies						)	
	ENUM_OPTION( SHOW_NavGraphRegions						)
	ENUM_OPTION( SHOW_Containers							)
	ENUM_OPTION( SHOW_RenderTargetBackground				)
	ENUM_OPTION( SHOW_AITickets								)
	ENUM_OPTION( SHOW_EffectAreas							)
	ENUM_OPTION( SHOW_PathSpeedValues						)
	ENUM_OPTION( SHOW_PhysActorIterations					)
	ENUM_OPTION( SHOW_BoatInput								)
	ENUM_OPTION( SHOW_BoatWaterProbbing						)
	ENUM_OPTION( SHOW_Dismemberment							)
	ENUM_OPTION( SHOW_EntityVisibility						)
	ENUM_OPTION( SHOW_AIBehaviorDebug						)
	ENUM_OPTION( SHOW_XB1SafeArea							)
	ENUM_OPTION( SHOW_PS4SafeArea							)
	ENUM_OPTION( SHOW_Skybox								)
	ENUM_OPTION( SHOW_CameraLights							)
	ENUM_OPTION( SHOW_AntiLightbleed						)
	ENUM_OPTION( SHOW_UseRegularDeferred					)
	ENUM_OPTION( SHOW_CommunityAgents						)
	ENUM_OPTION( SHOW_RenderDeepTerrain						)
	ENUM_OPTION( SHOW_CullTerrainWithFullHeight				)
	ENUM_OPTION( SHOW_BBoxesFur								)
	ENUM_OPTION( SHOW_CameraInteriorFactor					)
	ENUM_OPTION( SHOW_BrowseDebugPreviews					)
	ENUM_OPTION( SHOW_BBoxesCloth							)
	ENUM_OPTION( SHOW_GameplayPostFx						)
	ENUM_OPTION( SHOW_SimpleBuoyancy						)
	ENUM_OPTION( SHOW_ScaleformMemoryInfo					)
	ENUM_OPTION( SHOW_GameplayLightComponent				)
	ENUM_OPTION( SHOW_Wetness								)
	ENUM_OPTION( SHOW_CameraVisibility						)
	ENUM_OPTION( SHOW_TeleportDetector						)
	ENUM_OPTION( SHOW_MapTracking							)
	ENUM_OPTION( SHOW_QuestMapPins							)
	ENUM_OPTION( SHOW_SoundListener							)
	ENUM_OPTION( SHOW_NavTerrainHeight						)
	ENUM_OPTION( SHOW_BBoxesDestruction						)
	ENUM_OPTION( SHOW_RenderGPUProfiler						)
	ENUM_OPTION( SHOW_TemplateLoadBalancer					)
	ENUM_OPTION( SHOW_DistantLightsDebug					)	
END_ENUM_RTTI()
