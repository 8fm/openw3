/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#ifndef _H_ENGINE_TYPE_REGISTRY
#define _H_ENGINE_TYPE_REGISTRY

// this file contains list of all types in 'core' project

// Not defined when included in engine.h, but defined when included in engineClasses.cpp
#if !defined( REGISTER_RTTI_TYPE )
#define REGISTER_RTTI_TYPE( _className ) RED_DECLARE_RTTI_NAME( _className ) template<> struct TTypeName< _className >{ static const CName& GetTypeName() { return CNAME( _className ); } static Bool IsArray() { return false; } };
	#define REGISTER_NOT_REGISTERED
#endif

#define REGISTER_RTTI_CLASS( _className ) class _className; REGISTER_RTTI_TYPE( _className );
#define REGISTER_RTTI_STRUCT( _className ) struct _className; REGISTER_RTTI_TYPE( _className );
#define REGISTER_RTTI_ENUM( _className ) enum _className; REGISTER_RTTI_TYPE( _className );

///

REGISTER_RTTI_STRUCT( SGameplayConfig );
REGISTER_RTTI_STRUCT( SGameplayConfigLookAts );
REGISTER_RTTI_STRUCT( SGameplayConfigGameCamera );
REGISTER_RTTI_STRUCT( SActorLODConfig );
REGISTER_RTTI_STRUCT( SGameplayLODConfig );

REGISTER_RTTI_STRUCT( SCollisionData );
REGISTER_RTTI_STRUCT( GameTime );
REGISTER_RTTI_STRUCT( GameTimeInterval );
REGISTER_RTTI_STRUCT( GameTimeWrapper );
REGISTER_RTTI_STRUCT( SBehaviorGraphBoneInfo );
REGISTER_RTTI_STRUCT( SBehaviorGraphTrackInfo );
REGISTER_RTTI_STRUCT( SBoneTransform );
REGISTER_RTTI_STRUCT( SGraphLayer );
REGISTER_RTTI_CLASS( IAttachment );
REGISTER_RTTI_CLASS( CBoundedComponent );
REGISTER_RTTI_CLASS( CDrawableComponent );
REGISTER_RTTI_CLASS( ITexture );
REGISTER_RTTI_CLASS( TextureGroup );
REGISTER_RTTI_CLASS( CTextureArrayEntry );
REGISTER_RTTI_CLASS( ISlot );
REGISTER_RTTI_CLASS( CCurve );
REGISTER_RTTI_STRUCT( SPropertyAnimation );
REGISTER_RTTI_STRUCT( SMultiCurve );
REGISTER_RTTI_STRUCT( SMultiCurvePosition );
REGISTER_RTTI_STRUCT( SCurveEaseParam );
REGISTER_RTTI_CLASS( CPropertyAnimationSet );
REGISTER_RTTI_CLASS( CubeFace );
REGISTER_RTTI_CLASS( CGame );
REGISTER_RTTI_CLASS( CWorldShadowConfig );
REGISTER_RTTI_STRUCT( SGlobalSpeedTreeParameters );
REGISTER_RTTI_STRUCT( SWorldSkyboxParameters );
REGISTER_RTTI_STRUCT( SWorldRenderSettings );
REGISTER_RTTI_STRUCT( SWorldMotionBlurSettings );
REGISTER_RTTI_STRUCT( SWorldEnvironmentParameters );
REGISTER_RTTI_STRUCT( SLensFlareGroupsParameters );
REGISTER_RTTI_CLASS( CWorld );
REGISTER_RTTI_CLASS( CLayer );
REGISTER_RTTI_CLASS( CSectorData );
REGISTER_RTTI_CLASS( CCookedMeshEntity );

REGISTER_RTTI_CLASS( CDynamicLayer );
REGISTER_RTTI_CLASS( CEnvironmentDefinition );
REGISTER_RTTI_CLASS( CEnvAmbientProbesGenParameters );
REGISTER_RTTI_CLASS( CEnvReflectionProbesGenParameters );
REGISTER_RTTI_CLASS( CEnvCameraLightParameters );
REGISTER_RTTI_CLASS( CEnvCameraLightsSetupParameters );
REGISTER_RTTI_CLASS( CEnvGlobalSkyParameters );
REGISTER_RTTI_STRUCT( CGlobalLightingTrajectory );
REGISTER_RTTI_STRUCT( SBokehDofParams );
REGISTER_RTTI_CLASS( CEnvToneMappingParameters );
REGISTER_RTTI_CLASS( CEnvToneMappingCurveParameters );
REGISTER_RTTI_CLASS( CEnvBloomNewParameters );
REGISTER_RTTI_CLASS( CEnvDistanceRangeParameters );
REGISTER_RTTI_CLASS( CEnvColorModTransparencyParameters );
REGISTER_RTTI_CLASS( CEnvShadowsParameters );
REGISTER_RTTI_CLASS( CEnvGlobalFogParameters );
REGISTER_RTTI_CLASS( CEnvDepthOfFieldParameters );
REGISTER_RTTI_CLASS( CEnvNVSSAOParameters );
REGISTER_RTTI_CLASS( CEnvMSSSAOParameters );
REGISTER_RTTI_CLASS( CEnvRadialBlurParameters );
REGISTER_RTTI_CLASS( CEnvDayCycleOverrideParameters );
REGISTER_RTTI_CLASS( CEnvBrightnessTintParameters );
REGISTER_RTTI_CLASS( CEnvDisplaySettingsParams );
REGISTER_RTTI_CLASS( CEnvSpeedTreeRandomColorParameters );
REGISTER_RTTI_CLASS( CEnvSpeedTreeParameters );
REGISTER_RTTI_CLASS( CEnvGlobalLightParameters );
REGISTER_RTTI_CLASS( CEnvInteriorFallbackParameters );
REGISTER_RTTI_CLASS( CEnvParametricBalanceParameters );
REGISTER_RTTI_CLASS( CEnvFinalColorBalanceParameters );
REGISTER_RTTI_CLASS( CEnvSharpenParameters );
REGISTER_RTTI_CLASS( CEnvPaintEffectParameters );
REGISTER_RTTI_CLASS( CEnvWaterParameters );
REGISTER_RTTI_CLASS( CEnvSunAndMoonParameters );
REGISTER_RTTI_CLASS( CEnvGameplayEffectsParameters );
REGISTER_RTTI_CLASS( CEnvMotionBlurParameters );
REGISTER_RTTI_CLASS( CEnvWindParameters );
REGISTER_RTTI_CLASS( CEnvDialogLightParameters );
REGISTER_RTTI_CLASS( CEnvColorGroupsParameters );
REGISTER_RTTI_CLASS( CEnvFlareColorParameters );
REGISTER_RTTI_CLASS( CEnvFlareColorGroupsParameters );
REGISTER_RTTI_CLASS( CAreaEnvironmentParams );
REGISTER_RTTI_CLASS( CGameEnvironmentParams );
REGISTER_RTTI_STRUCT( CWindParameters );
REGISTER_RTTI_CLASS( CLayerInfo );
REGISTER_RTTI_CLASS( CLayerGroup );
REGISTER_RTTI_STRUCT( SSkeletonBone );
REGISTER_RTTI_STRUCT( SSkeletonTrack );
REGISTER_RTTI_CLASS( CSkeleton );
REGISTER_RTTI_CLASS( IAnimationBuffer );
REGISTER_RTTI_CLASS( CAnimationBufferUncompressed );
REGISTER_RTTI_CLASS( CAnimationBufferBitwiseCompressed );
REGISTER_RTTI_STRUCT( SAnimationBufferBitwiseCompressedData );
REGISTER_RTTI_STRUCT( SAnimationBufferBitwiseCompressedBoneTrack );
REGISTER_RTTI_STRUCT( SAnimationBufferBitwiseCompressionSettings );
REGISTER_RTTI_CLASS( CSkeletalAnimation );
REGISTER_RTTI_CLASS( CVirtualSkeletalAnimation );
#ifndef NO_DEFAULT_ANIM
REGISTER_RTTI_CLASS( CDebugAnimation );
#endif
REGISTER_RTTI_CLASS( CSkeletalAnimationSet );
REGISTER_RTTI_STRUCT( SCompressedPoseInfo );
REGISTER_RTTI_CLASS( CAnimatedComponent );
REGISTER_RTTI_CLASS( CAnimatedComponentPhysicsRepresentation );
REGISTER_RTTI_STRUCT( SAnimatedComponentSyncSettings );
REGISTER_RTTI_STRUCT( SAnimatedComponentSlotAnimationSettings );
REGISTER_RTTI_CLASS( CSkeletalAnimatedComponent );
REGISTER_RTTI_CLASS( CWetnessComponent );
REGISTER_RTTI_STRUCT( SSlotInfo );
REGISTER_RTTI_CLASS( CSlotComponent );
REGISTER_RTTI_CLASS( CIndirectSlot );
REGISTER_RTTI_CLASS( CMimicComponent );
REGISTER_RTTI_CLASS( CMimicFaces );
REGISTER_RTTI_CLASS( CMimicFace );
REGISTER_RTTI_STRUCT( SMimicTrackPose );
REGISTER_RTTI_CLASS( CSkeletalAnimationSetEntry );
REGISTER_RTTI_CLASS( CAnimationSlots );
REGISTER_RTTI_CLASS( CBezierComponent );
REGISTER_RTTI_STRUCT( SAppearanceAttachment );
REGISTER_RTTI_STRUCT( SAppearanceAttachments );
REGISTER_RTTI_CLASS( CAppearanceComponent );
REGISTER_RTTI_CLASS( CInputManager );
REGISTER_RTTI_CLASS( CGestureSystem );
REGISTER_RTTI_STRUCT( SInputAction );

REGISTER_RTTI_STRUCT( SCustomClippingPlanes );

REGISTER_RTTI_CLASS( COverrideStreamingDistanceComponent );

// Animation Events
REGISTER_RTTI_CLASS( CAnimEventSerializer );
REGISTER_RTTI_CLASS( CExtAnimEvent );
REGISTER_RTTI_CLASS( CExtAnimDurationEvent );
REGISTER_RTTI_CLASS( CExtAnimExplorationEvent );
REGISTER_RTTI_CLASS( CExtAnimCutsceneActorEffect );
REGISTER_RTTI_CLASS( CExtAnimCutsceneBodyPartEvent );
REGISTER_RTTI_CLASS( CExtAnimCutsceneDialogEvent );
REGISTER_RTTI_CLASS( CExtAnimCutsceneBreakEvent );
REGISTER_RTTI_CLASS( CExtAnimCutsceneEffectEvent );
REGISTER_RTTI_CLASS( CExtAnimCutsceneSlowMoEvent );
REGISTER_RTTI_CLASS( CExtAnimCutsceneWindEvent );
REGISTER_RTTI_CLASS( CExtAnimCutsceneEnvironmentEvent );
REGISTER_RTTI_CLASS( CExtAnimCutsceneLightEvent );
REGISTER_RTTI_CLASS( CExtAnimCutsceneBokehDofEvent );
REGISTER_RTTI_CLASS( CExtAnimCutsceneBokehDofBlendEvent );
REGISTER_RTTI_CLASS( CExtAnimCutsceneFadeEvent );
REGISTER_RTTI_CLASS( CExtAnimCutsceneSoundEvent );
REGISTER_RTTI_CLASS( CExtAnimComboEvent );
REGISTER_RTTI_CLASS( CExtAnimHitEvent );
REGISTER_RTTI_CLASS( CExtAnimMorphEvent );
REGISTER_RTTI_CLASS( CExtAnimCutsceneEvent );
REGISTER_RTTI_CLASS( CExtAnimCutsceneDurationEvent );
REGISTER_RTTI_CLASS( CExtAnimCutsceneSetClippingPlanesEvent );
#ifdef USE_EXT_ANIM_EVENTS
REGISTER_RTTI_CLASS( CExtAnimEventFileFactory );
#endif	//USE_EXT_ANIM_EVENTS
REGISTER_RTTI_CLASS( CExtAnimRaiseEventEvent );
REGISTER_RTTI_CLASS( CExtAnimDisableDialogLookatEvent );

// Script Animation Events
REGISTER_RTTI_CLASS( CExtAnimScriptEvent );
REGISTER_RTTI_CLASS( CEASMultiValueSimpleEvent );
REGISTER_RTTI_CLASS( CExtAnimScriptDurationEvent );
REGISTER_RTTI_CLASS( CEASSlideToTargetEvent );
REGISTER_RTTI_CLASS( CEASEnumEvent );
REGISTER_RTTI_CLASS( CEASMultiValueEvent );

// Script Animation Event Properties
REGISTER_RTTI_STRUCT( SAnimationEventAnimInfo );
REGISTER_RTTI_STRUCT( SSlideToTargetEventProps );
REGISTER_RTTI_STRUCT( SEnumVariant );
REGISTER_RTTI_STRUCT( SMultiValue );

REGISTER_RTTI_CLASS( CExtAnimEventsFile );

REGISTER_RTTI_CLASS( CPoseBBoxGenerator );
REGISTER_RTTI_CLASS( CSkeleton2SkeletonMapper );
REGISTER_RTTI_CLASS( IPoseCompression );
REGISTER_RTTI_CLASS( CPoseCompressionNone );
REGISTER_RTTI_CLASS( CPoseCompressionDefault );
REGISTER_RTTI_CLASS( CPoseCompressionDefaultWithExtraBones );
REGISTER_RTTI_CLASS( CPoseCompressionCharacter );
REGISTER_RTTI_CLASS( CPoseCompressionCamera );
REGISTER_RTTI_CLASS( ICompressedPose );
REGISTER_RTTI_CLASS( CNoCompressedPose );
REGISTER_RTTI_CLASS( CDefaultCompressedPose2 );
REGISTER_RTTI_CLASS( CDefaultCompressedPoseWithExtraBones );
REGISTER_RTTI_CLASS( CCharacterCompressedPose );
REGISTER_RTTI_CLASS( CCameraCompressedPose );
REGISTER_RTTI_CLASS( IMotionExtraction );
REGISTER_RTTI_CLASS( CUncompressedMotionExtraction );
REGISTER_RTTI_CLASS( CHavokMotionExtraction );
REGISTER_RTTI_CLASS( CLineMotionExtraction );
REGISTER_RTTI_CLASS( CLineMotionExtraction2 );
REGISTER_RTTI_CLASS( IMotionExtractionCompression );
REGISTER_RTTI_CLASS( CMotionExtractionLineCompression );
REGISTER_RTTI_CLASS( CMotionExtractionLineCompression2 );
REGISTER_RTTI_CLASS( IAnimationCompression );
REGISTER_RTTI_CLASS( CNoAnimationCompression );
REGISTER_RTTI_CLASS( CWaveletAnimationCompression );
REGISTER_RTTI_CLASS( CDeltaAnimationCompression );
REGISTER_RTTI_CLASS( CSplineAnimationCompression );
REGISTER_RTTI_CLASS( CAnimationFpsCompression );
REGISTER_RTTI_CLASS( CSourceTexture );
REGISTER_RTTI_CLASS( CBitmapTexture );
REGISTER_RTTI_CLASS( CCubeTexture );
REGISTER_RTTI_CLASS( CTextureArray );
REGISTER_RTTI_CLASS( CSwfTexture );
REGISTER_RTTI_CLASS( CFont );
REGISTER_RTTI_CLASS( CNode );
REGISTER_RTTI_CLASS( CComponent );
REGISTER_RTTI_STRUCT( SEntitySpawnData );
REGISTER_RTTI_CLASS( CEntity );
REGISTER_RTTI_CLASS( CPeristentEntity );
REGISTER_RTTI_CLASS( CEffectEntity );
REGISTER_RTTI_STRUCT( PersistentRef );
REGISTER_RTTI_CLASS( CEntityGroup );
REGISTER_RTTI_CLASS( CGameplayEntityParam );
REGISTER_RTTI_CLASS( CTemplateListParam );
REGISTER_RTTI_CLASS( CCharacterControllerParam );
REGISTER_RTTI_STRUCT( SControllerRadiusParams );
REGISTER_RTTI_STRUCT( SVirtualControllerParams );
REGISTER_RTTI_CLASS( CMovableRepresentationCreator );
REGISTER_RTTI_CLASS( IPerformableAction )
REGISTER_RTTI_CLASS( CCameraDirector );
REGISTER_RTTI_CLASS( CCamera );
REGISTER_RTTI_CLASS( CStaticCamera );
REGISTER_RTTI_CLASS( CSoundAmbientEmitter );
REGISTER_RTTI_STRUCT( CEntityOnLayerReference );
REGISTER_RTTI_CLASS( CMeshTypeResource );
REGISTER_RTTI_CLASS( CMesh );
REGISTER_RTTI_CLASS( CPhysicsDestructionResource );
REGISTER_RTTI_STRUCT( SMeshChunkPacked );
REGISTER_RTTI_STRUCT( SMeshCookedData );
REGISTER_RTTI_STRUCT( SMeshSoundInfo );
REGISTER_RTTI_CLASS( CFurMeshResource );

REGISTER_RTTI_CLASS( CMergedWorldGeometry );
REGISTER_RTTI_CLASS( IMergedWorldGeometryData );
REGISTER_RTTI_CLASS( CMergedWorldGeometryEntity );
REGISTER_RTTI_STRUCT( CMergedWorldGeometryGridCoordinates );
REGISTER_RTTI_CLASS( CMergedMeshComponent );
REGISTER_RTTI_CLASS( CMergedShadowMeshComponent );
REGISTER_RTTI_CLASS( CMergedWorldGeometryShadowData );

REGISTER_RTTI_CLASS( CSkeletalAnimationContainer );
//REGISTER_RTTI_CLASS( CSkeletonMapper );
REGISTER_RTTI_CLASS( CMeshTypeComponent );
REGISTER_RTTI_CLASS( CMeshComponent );
REGISTER_RTTI_CLASS( CMorphedMeshComponent );
REGISTER_RTTI_CLASS( CMorphedMeshManagerComponent );
REGISTER_RTTI_STRUCT( SFlareParameters );
REGISTER_RTTI_STRUCT( SLensFlareElementParameters );
REGISTER_RTTI_STRUCT( SLensFlareParameters );
REGISTER_RTTI_CLASS( CFlareComponent );
REGISTER_RTTI_CLASS( CWindowComponent );
REGISTER_RTTI_CLASS( CWaterComponent );
REGISTER_RTTI_CLASS( CWindAreaComponent );
REGISTER_RTTI_CLASS( CDaycycleGraphicsEntity );
REGISTER_RTTI_CLASS( CBehaviorAnimationMultiplyEntity );
REGISTER_RTTI_CLASS( CSkyTransformComponent );
REGISTER_RTTI_CLASS( CSkyTransformEntity );
REGISTER_RTTI_CLASS( CStaticMeshComponent );
REGISTER_RTTI_CLASS( CRigidMeshComponent );
REGISTER_RTTI_CLASS( CRigidMeshComponentCooked );
REGISTER_RTTI_CLASS( CDynamicColliderComponent );
REGISTER_RTTI_CLASS( CSimpleBuoyancyComponent );
REGISTER_RTTI_STRUCT( SDynamicCollider );
REGISTER_RTTI_CLASS( CDeniedAreaComponent );
REGISTER_RTTI_CLASS( CDecalComponent );
REGISTER_RTTI_CLASS( CSwarmRenderComponent );
#ifdef RED_ENABLE_STRIPE
REGISTER_RTTI_STRUCT( SStripeControlPoint );
REGISTER_RTTI_CLASS( CStripeComponent );
#endif
REGISTER_RTTI_CLASS( CFurComponent );
REGISTER_RTTI_CLASS( CScriptedDestroyableComponent);
REGISTER_RTTI_CLASS( CPhantomComponent );
REGISTER_RTTI_CLASS( CPhantomAttachment );
REGISTER_RTTI_CLASS( CFootstepData );
REGISTER_RTTI_CLASS( CEntityTemplate );
REGISTER_RTTI_CLASS( CEntityTemplateParam );
REGISTER_RTTI_CLASS( CCharacterEntityTemplate );
REGISTER_RTTI_CLASS( EntitySlot );
REGISTER_RTTI_STRUCT( SComponentInstancePropertyEntry );
REGISTER_RTTI_CLASS( CMeshSkinningAttachment );
REGISTER_RTTI_CLASS( CClothAttachment );
REGISTER_RTTI_CLASS( CAnimatedAttachment );
REGISTER_RTTI_CLASS( CHardAttachment );
REGISTER_RTTI_CLASS( CDependencyAttachment );
REGISTER_RTTI_CLASS( CSkinningAttachment );
REGISTER_RTTI_CLASS( CDummyComponent );
REGISTER_RTTI_CLASS( CSoundEmitterComponent );
REGISTER_RTTI_STRUCT( SSoundProperty );
REGISTER_RTTI_STRUCT( SSoundSwitch );
REGISTER_RTTI_CLASS( CSpriteComponent );
REGISTER_RTTI_STRUCT( SLightFlickering );
REGISTER_RTTI_CLASS( CLightComponent );
REGISTER_RTTI_CLASS( CPointLightComponent );
REGISTER_RTTI_CLASS( CSpotLightComponent );
REGISTER_RTTI_CLASS( CDimmerComponent );
REGISTER_RTTI_CLASS( SEnvProbeGenParams );
REGISTER_RTTI_CLASS( CEnvProbeComponent );
REGISTER_RTTI_CLASS( CEffectDummyComponent );
REGISTER_RTTI_CLASS( CPathComponent );
REGISTER_RTTI_CLASS( CAreaComponent );
REGISTER_RTTI_CLASS( CAreaTestComponent );
REGISTER_RTTI_CLASS( CNegativeAreaComponent );
REGISTER_RTTI_CLASS( CPlanarShapeComponent );
REGISTER_RTTI_CLASS( CCameraComponent );
REGISTER_RTTI_CLASS( CStickerComponent );
REGISTER_RTTI_CLASS( CSkeletonBoneSlot );
REGISTER_RTTI_CLASS( CExternalProxyComponent );
REGISTER_RTTI_CLASS( CExternalProxyAttachment );
REGISTER_RTTI_STRUCT( CColorShift )
REGISTER_RTTI_STRUCT( SCharacterWindParams );
REGISTER_RTTI_CLASS( CAnimDangleConstraint_Dress );
REGISTER_RTTI_CLASS( CDressMeshComponent );
REGISTER_RTTI_CLASS( CCameraOrientedComponent );
REGISTER_RTTI_CLASS( IAnimationController );
REGISTER_RTTI_CLASS( CSingleAnimationController );
REGISTER_RTTI_CLASS( CSequentialAnimationController );
REGISTER_RTTI_CLASS( CRandomAnimationController );
REGISTER_RTTI_CLASS( CRandomWithWeightAnimationController );
REGISTER_RTTI_CLASS( CAnimatedEntity );
REGISTER_RTTI_CLASS( CBezierMovableComponent );
REGISTER_RTTI_CLASS( CBehaviorGraphControlRigNode );
REGISTER_RTTI_CLASS( TCrDefinition);
REGISTER_RTTI_CLASS( TCrPropertySet );
REGISTER_RTTI_CLASS( CControlRigSettings );
REGISTER_RTTI_CLASS( CImpostorMeshComponent );
REGISTER_RTTI_CLASS( CStreamingAreaComponent );
REGISTER_RTTI_CLASS( CTeleportDetectorData );
REGISTER_RTTI_STRUCT( STeleportBone );
#ifndef NO_OBSTACLE_MESH_DATA
REGISTER_RTTI_CLASS( CNavigationObstacle );
REGISTER_RTTI_STRUCT( SNavigationObstacleShape );
#endif

REGISTER_RTTI_CLASS( CDestructionSystemComponent );
REGISTER_RTTI_CLASS( CClothComponent );
REGISTER_RTTI_CLASS( CDestructionComponent );
REGISTER_RTTI_STRUCT( SPhysicsDestructionAdditionalInfo );
REGISTER_RTTI_STRUCT( SBoneIndiceMapping );

REGISTER_RTTI_CLASS( CGameplayEffectsComponent );

REGISTER_RTTI_CLASS( CDismembermentComponent );
REGISTER_RTTI_STRUCT( SDismembermentWoundSingleSpawn );
REGISTER_RTTI_CLASS( CDismembermentWound );
REGISTER_RTTI_STRUCT( SDismembermentWoundDecal );
REGISTER_RTTI_CLASS( CEntityDismemberment );
REGISTER_RTTI_STRUCT( SDismembermentWoundFilter );
REGISTER_RTTI_STRUCT( SDismembermentEffect );

REGISTER_RTTI_STRUCT( VoicetagAppearancePair );
REGISTER_RTTI_STRUCT( SEntityTemplateColoringEntry );
REGISTER_RTTI_STRUCT( SEntityTemplateOverride );
REGISTER_RTTI_STRUCT( CComponentReference );
REGISTER_RTTI_STRUCT( CEntityBodyPartState );
REGISTER_RTTI_STRUCT( CEntityBodyPart );
REGISTER_RTTI_STRUCT( CEntityAppearance );
REGISTER_RTTI_CLASS( CEntityExternalAppearance );
REGISTER_RTTI_STRUCT( SAttachmentReplacement );
REGISTER_RTTI_STRUCT( SAttachmentReplacements );
REGISTER_RTTI_STRUCT( SStreamedAttachment );

REGISTER_RTTI_CLASS( CBoxComponent );
REGISTER_RTTI_CLASS( CTriggerAreaComponent );
REGISTER_RTTI_CLASS( CTriggerAreaEnvironmentVisibilityComponent );
REGISTER_RTTI_CLASS( CTriggerAreaExpansionPackComponent );
REGISTER_RTTI_CLASS( CTriggerActivatorComponent );
REGISTER_RTTI_CLASS( CSoftTriggerAreaComponent );
REGISTER_RTTI_CLASS( CSoundAmbientAreaComponent );
REGISTER_RTTI_STRUCT( SReverbDefinition );
REGISTER_RTTI_STRUCT( SSoundGameParameterValue );
REGISTER_RTTI_STRUCT( SSoundParameterCullSettings );
REGISTER_RTTI_STRUCT( SSoundAmbientDynamicSoundEvents );
REGISTER_RTTI_CLASS( CCutsceneTemplate );
REGISTER_RTTI_CLASS( CCutsceneInstance );
REGISTER_RTTI_STRUCT( SCutsceneActorDef );
REGISTER_RTTI_CLASS( ICutsceneModifier );
REGISTER_RTTI_CLASS( CCutsceneModifierFreezer );
REGISTER_RTTI_CLASS( CBgCutsceneEntity );

REGISTER_RTTI_CLASS( CAllocatedBehaviorGraphOutput );
REGISTER_RTTI_CLASS( CAllocatedLipsyncBehaviorGraphOutput );
REGISTER_RTTI_CLASS( CBehaviorGraph );
REGISTER_RTTI_CLASS( CBehaviorGraphInstance );
REGISTER_RTTI_STRUCT( SBehaviorSnapshotDataStateMachine );
REGISTER_RTTI_CLASS( CBehaviorGraphInstanceSnapshot );
REGISTER_RTTI_STRUCT( SBehaviorGraphInstanceSlot );
REGISTER_RTTI_CLASS( CBehaviorGraphStack );	
REGISTER_RTTI_CLASS( CBehaviorGraphStackSnapshot );
REGISTER_RTTI_STRUCT( CBehaviorNodeSyncData );
REGISTER_RTTI_CLASS( CBehaviorGraphNode );
REGISTER_RTTI_CLASS( CBehaviorGraphContainerNode );
REGISTER_RTTI_CLASS( CBehaviorGraph2DVariableNode );
REGISTER_RTTI_CLASS( CBehaviorGraph2DVectorVariableNode );
REGISTER_RTTI_CLASS( CBehaviorGraph2DMultiVariablesNode );
REGISTER_RTTI_CLASS( CBehaviorGraphStageNode );
REGISTER_RTTI_CLASS( CBehaviorGraphBaseNode );
REGISTER_RTTI_CLASS( CBehaviorGraphValueBaseNode );
REGISTER_RTTI_CLASS( CAnimationSyncToken );
REGISTER_RTTI_CLASS( CBehaviorGraphAnimationNode );
REGISTER_RTTI_CLASS( CBehaviorGraphAnimationExtNode );
REGISTER_RTTI_CLASS( CBehaviorGraphAnimationSwitchNode )
REGISTER_RTTI_CLASS( CBehaviorGraphAnimationManualSwitchNode );
REGISTER_RTTI_CLASS( CBehaviorGraphAnimationEnumSwitchNode );
REGISTER_RTTI_CLASS( CBehaviorGraphAnimationEnumSequentialSwitchNode );
REGISTER_RTTI_CLASS( CBehaviorGraphAnimationRandomSwitchNode );
#ifndef NO_EDITOR_GRAPH_SUPPORT
REGISTER_RTTI_CLASS( CAnimatedComponentAnimationSyncToken );
#endif
REGISTER_RTTI_CLASS( CBehaviorGraphAnimationBaseSlotNode );
REGISTER_RTTI_CLASS( CBehaviorGraphMimicBaseSlotNode );
REGISTER_RTTI_CLASS( CBehaviorGraphAnimationSlotNode );
REGISTER_RTTI_CLASS( CBehaviorGraphMimicSlotNode );
REGISTER_RTTI_CLASS( CBehaviorGraphMimicEventSlotNode );
REGISTER_RTTI_CLASS( CBehaviorGraphBaseMimicNode );
REGISTER_RTTI_CLASS( CBehaviorGraphMimicLipsyncFilterNode );
REGISTER_RTTI_CLASS( CBehaviorGraphAnimationSlotWithCurveNode );
REGISTER_RTTI_CLASS( CBehaviorGraphMimicSlotWithCurveNode );
REGISTER_RTTI_CLASS( CBehaviorGraphMimicSlotWithSwapingNode );
REGISTER_RTTI_CLASS( CBehaviorGraphAnimationAdditiveSlotNode );
REGISTER_RTTI_CLASS( CBehaviorGraphAnimationSelAdditiveSlotNode );
REGISTER_RTTI_CLASS( CBehaviorGraphMimicAdditiveSlotNode );
REGISTER_RTTI_CLASS( CBehaviorGraphAnimationLipsyncSlotNode );
REGISTER_RTTI_CLASS( CBehaviorGraphMimicsAnimationNode );
REGISTER_RTTI_CLASS( CBehaviorGraphMimicsEventAnimationNode );
REGISTER_RTTI_CLASS( CBehaviorGraphMimicsBoneAnimationNode );
REGISTER_RTTI_CLASS( CBehaviorGraphMimicsGeneratorNode );
REGISTER_RTTI_CLASS( IBehaviorMimicConstraint );
REGISTER_RTTI_CLASS( CBehaviorMimicLookAtConstraint );
REGISTER_RTTI_CLASS( CBehaviorMimicHeadConstraint );
REGISTER_RTTI_CLASS( CBehaviorMimiLipsyncCorrectionConstraint );
REGISTER_RTTI_CLASS( CBehaviorGraphMimicsConverterNode );
REGISTER_RTTI_CLASS( CBehaviorMimicCloseEyesConstraint );
REGISTER_RTTI_CLASS( CBehaviorGraphMimicsBoneConverterNode );
REGISTER_RTTI_CLASS( CBehaviorGraphMimicsBlendNode );
REGISTER_RTTI_CLASS( CBehaviorGraphMimicGainNode );
REGISTER_RTTI_CLASS( CBehaviorGraphMimicMathOpNode );
REGISTER_RTTI_CLASS( CBehaviorGraphMimicFilterNode );
REGISTER_RTTI_CLASS( CBehaviorGraphMimicFilterNodeInvert );
REGISTER_RTTI_CLASS( CBehaviorGraphMimicOutputNode );
REGISTER_RTTI_CLASS( CBehaviorGraphMimicStageNode );
REGISTER_RTTI_CLASS( CBehaviorGraphMimicParentInputNode );
REGISTER_RTTI_CLASS( CBehaviorGraphMimicAnimationManualSwitchNode );
REGISTER_RTTI_CLASS( CBehaviorGraphMimicAnimationEnumSwitchNode );
REGISTER_RTTI_CLASS( CBehaviorGraphPoseSlotNode );
REGISTER_RTTI_CLASS( IBehaviorSyncMethod );
REGISTER_RTTI_CLASS( CBehaviorSyncMethodNone );
REGISTER_RTTI_CLASS( IBehaviorSyncMethodEvent )
REGISTER_RTTI_CLASS( CBehaviorSyncMethodEventStart );
REGISTER_RTTI_CLASS( CBehaviorSyncMethodEventProp );
REGISTER_RTTI_CLASS( CBehaviorSyncMethodTime );
REGISTER_RTTI_CLASS( CBehaviorSyncMethodDuration );
REGISTER_RTTI_CLASS( CBehaviorSyncMethodOffset );
REGISTER_RTTI_CLASS( CBehaviorSyncMethodProp );
REGISTER_RTTI_CLASS( CBehaviorSyncMethodSyncPoints );
REGISTER_RTTI_CLASS( CBehaviorSyncMethodSyncPointsStartOnly );
REGISTER_RTTI_CLASS( CBehaviorGraphBlendNode );
REGISTER_RTTI_CLASS( CBehaviorGraphBlend3Node );
REGISTER_RTTI_CLASS( CBehaviorGraphBlendAdditiveNode );
REGISTER_RTTI_CLASS( CBehaviorGraphBlendMultipleNode );
REGISTER_RTTI_CLASS( CBehaviorGraphBlendOverrideNode );
REGISTER_RTTI_CLASS( CBehaviorGraphBlockEventNode );
REGISTER_RTTI_CLASS( CBehaviorGraphSyncOverrideNode );
REGISTER_RTTI_CLASS( CBehaviorGraphJoinNode );
REGISTER_RTTI_CLASS( CBehaviorGraphInjectorNode );
REGISTER_RTTI_CLASS( CBehaviorGraphStaticConditionNode );
REGISTER_RTTI_CLASS( IBehaviorGraphStaticCondition );
REGISTER_RTTI_CLASS( CBehaviorGraphStaticCondition_AnimTag );
REGISTER_RTTI_CLASS( CBehaviorGraphMotionExBlendNode );
REGISTER_RTTI_CLASS( CBehaviorGraphMotionExValueNode );
REGISTER_RTTI_CLASS( CBehaviorGraphMotionRotChangeValueNode );
REGISTER_RTTI_CLASS( CBehaviorGraphOutputNode );	
REGISTER_RTTI_CLASS( CBehaviorGraphOverrideFloatTracksNode );
REGISTER_RTTI_CLASS( CBehaviorGraphValueNode );
REGISTER_RTTI_CLASS( CBehaviorGraphVariableBaseNode );
REGISTER_RTTI_CLASS( CBehaviorGraphVariableNode );
REGISTER_RTTI_CLASS( CBehaviorGraphInternalVariableNode );
REGISTER_RTTI_CLASS( CBehaviorGraphSetInternalVariableFloatInOutNode );
REGISTER_RTTI_CLASS( CBehaviorGraphSetInternalVariableNode );
REGISTER_RTTI_CLASS( CBehaviorGraphInternalVariableCounterNode );
REGISTER_RTTI_CLASS( CBehaviorGraphEventWatchdogNode );
REGISTER_RTTI_CLASS( CBehaviorGraphVectorValueNode );
REGISTER_RTTI_CLASS( CBehaviorGraphVectorValueBaseNode );
REGISTER_RTTI_CLASS( CBehaviorGraphVectorVariableNode );
REGISTER_RTTI_CLASS( CBehaviorGraphInternalVectorVariableNode );
REGISTER_RTTI_CLASS( CBehaviorGraphConstantVectorValueNode );
REGISTER_RTTI_CLASS( CBehaviorGraphVectorMathNode );
REGISTER_RTTI_CLASS( CBehaviorGraphVectorVariableInputSocket );
REGISTER_RTTI_CLASS( CBehaviorGraphVectorVariableOutputSocket );
REGISTER_RTTI_CLASS( CBehaviorGraphParentInputNode );
REGISTER_RTTI_CLASS( CBehaviorGraphParentValueInputNode );
REGISTER_RTTI_CLASS( CBehaviorGraphParentVectorValueInputNode );
REGISTER_RTTI_CLASS( CBehaviorGraphAnimationInputSocket );
REGISTER_RTTI_CLASS( CBehaviorGraphExactlyAnimationInputSocket );
REGISTER_RTTI_CLASS( CBehaviorGraphAnimationOutputSocket );
REGISTER_RTTI_CLASS( CBehaviorGraphVariableInputSocket );
REGISTER_RTTI_CLASS( CBehaviorGraphVariableOutputSocket );
REGISTER_RTTI_CLASS( CBehaviorGraphStateInSocket );
REGISTER_RTTI_CLASS( CBehaviorGraphStateOutSocket );
REGISTER_RTTI_CLASS( CBehaviorGraphTransitionSocket );
REGISTER_RTTI_CLASS( CBehaviorGraphMimicAnimationInputSocket );
REGISTER_RTTI_CLASS( CBehaviorGraphMimicAnimationOutputSocket );
REGISTER_RTTI_CLASS( CBehaviorGraphCharacterRotationNode );
REGISTER_RTTI_CLASS( CBehaviorGraphAdjustDirectionNode );
REGISTER_RTTI_CLASS( CBehaviorGraphTopLevelNode );
REGISTER_RTTI_CLASS( CBehaviorGraphDampValueNode );
REGISTER_RTTI_CLASS( CBehaviorGraphDampAngularValueNode );
REGISTER_RTTI_CLASS( CBehaviorGraphDampAngularValueNodeDiff );
REGISTER_RTTI_CLASS( CBehaviorGraphDampVectorValueNode );
REGISTER_RTTI_CLASS( CBehaviorGraphCurveDampValueNode );
REGISTER_RTTI_CLASS( CBehaviorGraphCurveMapValueNode );
REGISTER_RTTI_CLASS( CBehaviorGraphCustomDampValueNode );
REGISTER_RTTI_CLASS( CBehaviorGraphRandomNode );
REGISTER_RTTI_CLASS( CBehaviorGraphRandomSelectNode );
REGISTER_RTTI_CLASS( CBehaviorGraphMimicRandomNode );
REGISTER_RTTI_CLASS( CBehaviorGraphMimicRandomBlendNode );
REGISTER_RTTI_CLASS( CBehaviorGraphMimicEyesCorrectionNode );
REGISTER_RTTI_CLASS( CBehaviorGraphMimicBlinkControllerNode );
REGISTER_RTTI_CLASS( CBehaviorGraphMimicBlinkControllerNode_Setter );
REGISTER_RTTI_CLASS( CBehaviorGraphMimicBlinkControllerNode_Watcher );
REGISTER_RTTI_CLASS( CBehaviorGraphMimicBlinkControllerNode_Blend );
REGISTER_RTTI_CLASS( CBehaviorGraphLipsyncControlValueCorrectionNode );
REGISTER_RTTI_STRUCT( SBehaviorGraphStateBehaviorGraphSyncInfo );
REGISTER_RTTI_CLASS( CBehaviorGraphStateNode );
REGISTER_RTTI_CLASS( CBehaviorGraphPointerTransitionNode );
REGISTER_RTTI_CLASS( CBehaviorGraphPointerStateNode );
REGISTER_RTTI_CLASS( CBehaviorGraphDefaultSelfActStateNode );
REGISTER_RTTI_CLASS( CBehaviorGraphComboStateNode );
REGISTER_RTTI_CLASS( CBehaviorGraphOffensiveComboStateNode );
REGISTER_RTTI_CLASS( CBehaviorGraphDefensiveComboStateNode );
REGISTER_RTTI_CLASS( CBehaviorGraphSimpleDefensiveComboStateNode );
REGISTER_RTTI_CLASS( CBehaviorGraphComboTransitionInterface );
REGISTER_RTTI_CLASS( IBehaviorGraphComboModifier );
REGISTER_RTTI_CLASS( CBehaviorGraphComboLevelModifier );
REGISTER_RTTI_CLASS( CBehaviorGraphComboStartingAnimationModifier );
REGISTER_RTTI_CLASS( CBehaviorGraphComboCooldownModifier );
REGISTER_RTTI_CLASS( CBehaviorGraphComboTransitionNode );
REGISTER_RTTI_CLASS( CBehaviorGraphGlobalComboTransitionNode );
REGISTER_RTTI_STRUCT( SBehaviorComboAnimation );
REGISTER_RTTI_STRUCT( SBehaviorComboAnim );
REGISTER_RTTI_STRUCT( SBehaviorComboElem );
REGISTER_RTTI_STRUCT( SBehaviorComboDistance );
REGISTER_RTTI_STRUCT( SBehaviorComboDirection );
REGISTER_RTTI_STRUCT( SBehaviorComboLevel );
REGISTER_RTTI_STRUCT( SBehaviorComboWay );
REGISTER_RTTI_STRUCT( SBehaviorComboAttack );
REGISTER_RTTI_CLASS( CBehaviorGraphFlowConnectionNode );
REGISTER_RTTI_CLASS( CBehaviorGraphFlowTransitionNode );
REGISTER_RTTI_CLASS( CBehaviorGraphFrozenStateNode );
REGISTER_RTTI_CLASS( CBehaviorGraphPoseMemoryNode );
REGISTER_RTTI_CLASS( CBehaviorGraphPoseMemoryNode_Mimic );
REGISTER_RTTI_STRUCT( SBehaviorGraphTransitionSetInternalVariable );
REGISTER_RTTI_CLASS( CBehaviorGraphStateTransitionNode );
REGISTER_RTTI_CLASS( CBehaviorGraphStateTransitionBlendNode );
REGISTER_RTTI_CLASS( CBehaviorGraphStateTransitionGlobalBlendNode );
REGISTER_RTTI_CLASS( CBehaviorGraphStateTransitionGlobalBlendStreamingNode );
REGISTER_RTTI_CLASS( CBehaviorGraphStateTransitionMatchToPoseNode );
REGISTER_RTTI_CLASS( CBehaviorGraphStateTransitionMatchFromPoseNode );
REGISTER_RTTI_CLASS( CBehaviorGraphMatchFromPoseNode );
REGISTER_RTTI_CLASS( CBehaviorGraphStatePelvisTransitionNode );
REGISTER_RTTI_CLASS( CBehaviorGraphStateMachineNode ); 
REGISTER_RTTI_CLASS( CBehaviorGraphSelfActivatingStateMachineNode ); 
REGISTER_RTTI_CLASS( CBehaviorGraphSelfActivatingOverrideStateMachineNode ); 
REGISTER_RTTI_CLASS( CBehaviorGraphSyncOverrideStateMachineNode );
REGISTER_RTTI_CLASS( CBehaviorGraphSelfActivatingAdditiveStateMachineNode );
REGISTER_RTTI_CLASS( CBehaviorGraphDefaultSelfActAdditiveStateNode );
REGISTER_RTTI_CLASS( CBehaviorGraphStateAdditiveTransitionNode );
REGISTER_RTTI_CLASS( CBehaviorGraphRotateBoneNode );
REGISTER_RTTI_CLASS( CBehaviorGraphScaleBoneNode );
REGISTER_RTTI_CLASS( CBehaviorGraphRotateLimitNode );
REGISTER_RTTI_CLASS( CBehaviorGraphPivotRotationNode );
REGISTER_RTTI_CLASS( CBehaviorGraphActorTiltNode );
REGISTER_RTTI_CLASS( CBehaviorGraphFilterNode );
REGISTER_RTTI_CLASS( CBehaviorGraphTranslateBoneNode );
REGISTER_RTTI_CLASS( CBehaviorGraphEngineValueNode );
REGISTER_RTTI_CLASS( CBehaviorGraphEngineVectorValueNode );
REGISTER_RTTI_CLASS( CBehaviorGraphMathNode );
REGISTER_RTTI_CLASS( CBehaviorGraphWrapNode );
REGISTER_RTTI_CLASS( CBehaviorGraphWaveValueNode );
REGISTER_RTTI_CLASS( CBehaviorGraphFloatValueNode );
REGISTER_RTTI_CLASS( CBehaviorGraphValueModifierNode );
REGISTER_RTTI_CLASS( CBehaviorGraphEditorValueNode );
REGISTER_RTTI_CLASS( CBehaviorGraphRandomValueNode );
REGISTER_RTTI_CLASS( CBehaviorGraphMapWorldSpaceDirectionToModelSpaceRangeNode );
REGISTER_RTTI_CLASS( CBehaviorGraphMapRangeNode );
REGISTER_RTTI_STRUCT( SBehaviorGraphMapToDiscreteMapper );
REGISTER_RTTI_CLASS( CBehaviorGraphMapToDiscreteNode );
REGISTER_RTTI_CLASS( CBehaviorGraphSnapToZeroNode );
REGISTER_RTTI_CLASS( CBehaviorGraphCameraControllerNode );
REGISTER_RTTI_CLASS( CBehaviorGraphCameraVerticalDampNode );
REGISTER_RTTI_CLASS( CBehaviorGraphGetBoneTransformNode );
REGISTER_RTTI_CLASS( CBehaviorEventDescription );
REGISTER_RTTI_CLASS( CBaseBehaviorVariable );	
REGISTER_RTTI_CLASS( CBehaviorVariable );	
REGISTER_RTTI_CLASS( CBehaviorVectorVariable );
REGISTER_RTTI_CLASS( IBehaviorStateTransitionCondition );
REGISTER_RTTI_CLASS( CMultiTransitionCondition );
REGISTER_RTTI_CLASS( CCompositeTransitionCondition );
REGISTER_RTTI_CLASS( CCompositeSimultaneousTransitionCondition );
REGISTER_RTTI_CLASS( CVariableValueStateTransitionCondition );
REGISTER_RTTI_CLASS( CInternalVariableStateTransitionCondition );
REGISTER_RTTI_CLASS( CEventStateTransitionCondition );
REGISTER_RTTI_CLASS( CAlwaysTransitionCondition );
REGISTER_RTTI_CLASS( CIsRagdolledTransitionCondition );
REGISTER_RTTI_CLASS( CAnimEventTransitionCondition );
REGISTER_RTTI_CLASS( CDelayStateTransitionCondition );
REGISTER_RTTI_CLASS( CTimeThresholdStateTransitionCondition );
REGISTER_RTTI_CLASS( CParentInputValueStateTransitionCondition );
REGISTER_RTTI_CLASS( CAnimationEndCondition );
REGISTER_RTTI_CLASS( CBehaviorGraphAlignToGroundNode );
REGISTER_RTTI_CLASS( CBehaviorGraphChooseRecoverFromRagdollAnimNode );
REGISTER_RTTI_CLASS( CBehaviorGraphGetCustomTrackNode );
REGISTER_RTTI_CLASS( CBehaviorGraphGetFloatTrackNode );
REGISTER_RTTI_CLASS( CBehaviorGraphComparatorNode );
REGISTER_RTTI_CLASS( CBehaviorGraphEnumComparatorNode );
REGISTER_RTTI_CLASS( IBehaviorGraphNotifier );
REGISTER_RTTI_CLASS( CBehaviorGraphConstraintNode);
REGISTER_RTTI_CLASS( IBehaviorConstraintObject );
REGISTER_RTTI_CLASS( CBehaviorConstraintBoneObject );
REGISTER_RTTI_CLASS( CBehaviorConstraintVectorObject );
REGISTER_RTTI_CLASS( CBehaviorConstraintComponentObject );
REGISTER_RTTI_CLASS( CBehaviorGraphConstraintNodeLookAt );
REGISTER_RTTI_CLASS( CBehaviorGraphLookAtNode );
REGISTER_RTTI_CLASS( CBehaviorGraphLookAtUsingAnimationsProcessingNode );
REGISTER_RTTI_CLASS( CBehaviorGraphLookAtUsingAnimationsCommonBaseNode );
REGISTER_RTTI_CLASS( CBehaviorGraphLookAtUsingAnimationsNode );
REGISTER_RTTI_CLASS( CBehaviorGraphLookAtUsingEmbeddedAnimationsNode );
REGISTER_RTTI_CLASS( CBehaviorNodeParentChild );
REGISTER_RTTI_STRUCT( SLookAtAnimationPairDefinition );
REGISTER_RTTI_STRUCT( SLookAtAnimationPairInstance );
REGISTER_RTTI_STRUCT( SLookAtAnimationPairInputBasedDefinition );
REGISTER_RTTI_STRUCT( SLookAtAnimationPairInputBasedInstance );
REGISTER_RTTI_STRUCT( SSimpleAnimationPlayback );
REGISTER_RTTI_STRUCT( SSimpleAnimationPlaybackSet );
REGISTER_RTTI_CLASS( CBehaviorGraphSynchronizeAnimationsToParentNode );
REGISTER_RTTI_STRUCT( SSynchronizeAnimationToParentDefinition );
REGISTER_RTTI_STRUCT( SSynchronizeAnimationToParentInstance );
REGISTER_RTTI_CLASS( CBehaviorGraphPointCloudLookAtNode );
REGISTER_RTTI_CLASS( IBehaviorGraphPointCloudLookAtTransition );
REGISTER_RTTI_CLASS( IBehaviorGraphPointCloudLookAtTransition_Vector );
REGISTER_RTTI_CLASS( CBehaviorGraphPointCloudLookAtTransition_Vertical );
REGISTER_RTTI_CLASS( CBehaviorGraphPointCloudLookAtTransition_Spherical );
REGISTER_RTTI_CLASS( CBehaviorGraphPointCloudLookAtSecMotion );
REGISTER_RTTI_CLASS( CBehaviorGraphRecentlyUsedAnimsStateNode );
REGISTER_RTTI_CLASS( CBehaviorGraphIk2Node );
REGISTER_RTTI_CLASS( CBehaviorGraphIk3Node );
REGISTER_RTTI_CLASS( CBehaviorGraphIk2BakerNode );
REGISTER_RTTI_CLASS( CBehaviorGraphLookAtSystemNode );
REGISTER_RTTI_CLASS( CBehaviorGraphConstraintNodeChain );
REGISTER_RTTI_CLASS( CBehaviorGraphMimicLookAtSystemNode );
REGISTER_RTTI_CLASS( CBehaviorGraphConstraintNodeCameraLookAt );
REGISTER_RTTI_CLASS( CBehaviorGraphConstraintNodeCameraFocus );
REGISTER_RTTI_STRUCT( STwoBonesIKSolverBoneData );
REGISTER_RTTI_STRUCT( STwoBonesIKSolverData );
REGISTER_RTTI_STRUCT( STwoBonesIKSolver );
REGISTER_RTTI_STRUCT( SApplyRotationIKSolverData );
REGISTER_RTTI_STRUCT( SApplyRotationIKSolver );
REGISTER_RTTI_CLASS( CBehaviorGraphConstraintNodeRoll );
REGISTER_RTTI_CLASS( CBehaviorGraphConstraintNodeParentAlign );
REGISTER_RTTI_CLASS( CBehaviorGraphConstraintNodeBoneInterpolate );
REGISTER_RTTI_CLASS( CBehaviorGraphConstraintReset );
REGISTER_RTTI_CLASS( CBehaviorGraphAnimEventTrackNode );	
REGISTER_RTTI_CLASS( CBehaviorGraphHeadingNode );
REGISTER_RTTI_CLASS( CBehaviorGraphMotionExRotAngleNode );
REGISTER_RTTI_CLASS( CBehaviorGraphMotionExToAngleNode );
REGISTER_RTTI_CLASS( CBehaviorGraphPositionControllerBaseNode );
REGISTER_RTTI_CLASS( CBehaviorGraphCutsceneControllerNode );
REGISTER_RTTI_CLASS( CBehaviorGraphRotationControllerNode );
REGISTER_RTTI_CLASS( CBehaviorGraphPositionControllerNode );
REGISTER_RTTI_CLASS( CBehaviorGraphPositionControllerWithDampNode );
REGISTER_RTTI_CLASS( CBehaviorGraphMotionExFilterNode );
REGISTER_RTTI_CLASS( CBehaviorGraphMemoryNode );
REGISTER_RTTI_CLASS( CBehaviorGraphValueAccNode );
REGISTER_RTTI_CLASS( CBehaviorGraphInputNode );
REGISTER_RTTI_CLASS( CBehaviorGraphTPoseNode );
REGISTER_RTTI_CLASS( CBehaviorGraphIdentityPoseNode );
REGISTER_RTTI_CLASS( CBehaviorGraphConstraintCameraDialog );
REGISTER_RTTI_CLASS( CBehaviorGraphScriptNode );
REGISTER_RTTI_CLASS( IBehaviorScript );
REGISTER_RTTI_STRUCT( SBehaviorScriptContext );
REGISTER_RTTI_CLASS( CBehaviorGraphOneMinusNode );
REGISTER_RTTI_CLASS( CBehaviorGraphValueClampNode );
REGISTER_RTTI_CLASS( CBehaviorGraphSelectionValueNode );
REGISTER_RTTI_CLASS( CBehaviorGraphValueInterpolationNode );
REGISTER_RTTI_CLASS( CBehaviorGraphLatchValueNode );
REGISTER_RTTI_CLASS( CBehaviorGraphLatchVectorValueNode );
REGISTER_RTTI_CLASS( CBehaviorGraphLatchForSomeTimeValueNode );
REGISTER_RTTI_CLASS( CBehaviorGraphTimerValueNode );
REGISTER_RTTI_CLASS( CBehaviorGraphMimicsModifierNode );
REGISTER_RTTI_CLASS( CBehaviorGraphSpeedModulationNode );
//REGISTER_RTTI_CLASS( CBehaviorGraphDampAngularDifferentialValueNode );
REGISTER_RTTI_CLASS( CBehaviorGraphSpringAngularDampValueNode );
REGISTER_RTTI_CLASS( CBehaviorGraphSpringDampValueNode );
REGISTER_RTTI_CLASS( CBehaviorGraphOptSpringDampValueNode );
REGISTER_RTTI_CLASS( CBehaviorGraphPoseConstraintNode );
REGISTER_RTTI_CLASS( CBehaviorGraphPoseConstraintWithTargetNode );
REGISTER_RTTI_CLASS( CBehaviorGraphMimicPoseNode );
REGISTER_RTTI_CLASS( CAnimConstraintsParam );
REGISTER_RTTI_CLASS( CAnimGlobalParam );
REGISTER_RTTI_CLASS( CAnimBehaviorsParam );
REGISTER_RTTI_CLASS( CAnimAnimsetsParam );
REGISTER_RTTI_CLASS( CAnimMimicParam );
REGISTER_RTTI_CLASS( CBehaviorGraphAnimationManualSlotNode );
REGISTER_RTTI_CLASS( CBehaviorGraphAnimationManualWithInputSlotNode );
REGISTER_RTTI_CLASS( CBehaviorGraphAnimationMixerSlotNode );
REGISTER_RTTI_CLASS( CBehaviorGraphMimicManualSlotNode );
REGISTER_RTTI_STRUCT( SRuntimeAnimationData );
REGISTER_RTTI_STRUCT( SAnimationFullState );
REGISTER_RTTI_STRUCT( SAnimationMappedPose );
REGISTER_RTTI_STRUCT( SSlotEventAnim );
REGISTER_RTTI_CLASS( CAnimSlotsParam );
REGISTER_RTTI_STRUCT( VirtualAnimation );
REGISTER_RTTI_STRUCT( VirtualAnimationMotion );
REGISTER_RTTI_STRUCT( VirtualAnimationPoseFK );
REGISTER_RTTI_STRUCT( VirtualAnimationPoseIK );
REGISTER_RTTI_STRUCT( VirtualAnimationLayer );
REGISTER_RTTI_CLASS( ISkeletalAnimationSetEntryParam );
REGISTER_RTTI_CLASS( CSkeletalAnimationTrajectoryParam );
REGISTER_RTTI_CLASS( CSkeletalAnimationTrajectoryTrackParam );
REGISTER_RTTI_CLASS( CSkeletalAnimationAttackTrajectoryParam );
REGISTER_RTTI_CLASS( CAnimPointCloudLookAtParam );
REGISTER_RTTI_CLASS( AnimationTrajectoryPlayerScriptWrapper );
REGISTER_RTTI_STRUCT( SAnimationTrajectoryPlayerToken );
REGISTER_RTTI_STRUCT( SAnimationTrajectoryPlayerInput );
REGISTER_RTTI_CLASS( CSkeletalAnimationHitParam );
REGISTER_RTTI_CLASS( CActionMoveAnimationProxy );
REGISTER_RTTI_CLASS( CBehaviorGraphRetargetCharacterNode );
REGISTER_RTTI_CLASS( IBehaviorGraphRetargetCharacterNodeMethod );
REGISTER_RTTI_CLASS( CBehaviorGraphRetargetCharacterNodeMethod_Scale );
REGISTER_RTTI_CLASS( CBehaviorGraphRetargetCharacterNodeMethod_Skeleton );
REGISTER_RTTI_CLASS( CBehaviorGraphRetargetCharacterNodeMethod_SkeletonMapper );
REGISTER_RTTI_CLASS( CAnimDangleComponent );
REGISTER_RTTI_CLASS( CAnimDangleBufferComponent );
REGISTER_RTTI_CLASS( IAnimDangleConstraint );
REGISTER_RTTI_CLASS( CAnimSkeletalDangleConstraint );
REGISTER_RTTI_CLASS( CAnimSkeletalMultiDangleConstraint );
REGISTER_RTTI_CLASS( CAnimSkeletalMultiDangleConstraint_Painter );
REGISTER_RTTI_CLASS( CAnimDangleConstraint_Collar );
REGISTER_RTTI_CLASS( CAnimDangleConstraint_Pusher );
REGISTER_RTTI_CLASS( CAnimDangleConstraint_Hood );
REGISTER_RTTI_CLASS( CAnimDangleConstraint_Breast );
REGISTER_RTTI_CLASS( CAnimDangleConstraint_Hinge );
REGISTER_RTTI_CLASS( CDyngResource );
REGISTER_RTTI_CLASS( CAnimDangleConstraint_Dyng );
REGISTER_RTTI_CLASS( CAnimationSyncToken_Dyng );
REGISTER_RTTI_CLASS( CBehaviorGraphBlendMultipleCondNode );
REGISTER_RTTI_CLASS( IBehaviorGraphBlendMultipleCondNode_DampMethod );
REGISTER_RTTI_CLASS( CBehaviorGraphBlendMultipleCondNode_ConstDampMethod );
REGISTER_RTTI_CLASS( IBehaviorGraphBlendMultipleCondNode_Condition );
REGISTER_RTTI_CLASS( CBehaviorGraphBlendMultipleCondNode_Multi );
REGISTER_RTTI_CLASS( CBehaviorGraphBlendMultipleCondNode_AnimEvent );
REGISTER_RTTI_CLASS( CBehaviorGraphBlendMultipleCondNode_AnimEnd );
REGISTER_RTTI_CLASS( CBehaviorGraphBlendMultipleCondNode_Transition );
REGISTER_RTTI_CLASS( CBehaviorGraphCharacterMotionToWSNode );
REGISTER_RTTI_CLASS( CDropPhysicsComponent );
REGISTER_RTTI_CLASS( CDropPhysicsSetup );
REGISTER_RTTI_STRUCT( SDropPhysicsCurves );

REGISTER_RTTI_CLASS( CBrushFace );
REGISTER_RTTI_CLASS( CBrushComponent );
REGISTER_RTTI_CLASS( CBrushBuilder );
REGISTER_RTTI_CLASS( CBrushCompiledData);

REGISTER_RTTI_STRUCT( ParticleBurst );
REGISTER_RTTI_STRUCT( EmitterDurationSettings );
REGISTER_RTTI_STRUCT( EmitterDelaySettings );
REGISTER_RTTI_STRUCT( SParticleEmitterLODLevel );
REGISTER_RTTI_STRUCT( SSeedKeyValue );
REGISTER_RTTI_STRUCT( SParticleSystemLODLevel );
//REGISTER_RTTI_CLASS( CParticleData );
REGISTER_RTTI_CLASS( CParticleSystem );
REGISTER_RTTI_CLASS( CParticleEmitter );
REGISTER_RTTI_CLASS( IParticleModule );
REGISTER_RTTI_CLASS( IParticleInitializer );
REGISTER_RTTI_CLASS( IParticleModificator );
REGISTER_RTTI_CLASS( CParticleComponent );

REGISTER_RTTI_CLASS( IParticleDrawer );
REGISTER_RTTI_CLASS( CParticleDrawerBillboard );
REGISTER_RTTI_CLASS( CParticleDrawerRain );
REGISTER_RTTI_CLASS( CParticleDrawerEmitterOrientation );
REGISTER_RTTI_CLASS( CParticleDrawerMotionBlur );
REGISTER_RTTI_CLASS( CParticleDrawerSphereAligned );
REGISTER_RTTI_CLASS( CParticleDrawerTrail );
REGISTER_RTTI_CLASS( CParticleDrawerFacingTrail );
REGISTER_RTTI_CLASS( CParticleDrawerScreen );
REGISTER_RTTI_CLASS( CParticleDrawerMesh );
REGISTER_RTTI_CLASS( CParticleDrawerBeam );
REGISTER_RTTI_CLASS( CDecalSpawner );

REGISTER_RTTI_CLASS( CParticleInitializerPosition );
REGISTER_RTTI_CLASS( CParticleInitializerSize );
REGISTER_RTTI_CLASS( CParticleInitializerSize3D );
REGISTER_RTTI_CLASS( CParticleInitializerColor );
REGISTER_RTTI_CLASS( CParticleInitializerAlpha );
REGISTER_RTTI_CLASS( CParticleInitializerLifeTime );
REGISTER_RTTI_CLASS( CParticleInitializerRotation );
REGISTER_RTTI_CLASS( CParticleInitializerRotation3D );
REGISTER_RTTI_CLASS( CParticleInitializerRotationRate );
REGISTER_RTTI_CLASS( CParticleInitializerRotationRate3D );
REGISTER_RTTI_CLASS( CParticleInitializerSpawnCircle );
REGISTER_RTTI_CLASS( CParticleInitializerSpawnBox );
REGISTER_RTTI_CLASS( CParticleInitializerSpawnSphere );
REGISTER_RTTI_CLASS( CParticleInitializerVelocity );
REGISTER_RTTI_CLASS( CParticleInitializerVelocitySpread );
REGISTER_RTTI_CLASS( CParticleInitializerVelocityInherit );
REGISTER_RTTI_CLASS( CParticleInitializerRandomFlip );
REGISTER_RTTI_CLASS( CParticleInitializerCollisionSpawn );

REGISTER_RTTI_CLASS( CParticleModificatorSizeOverLife );
REGISTER_RTTI_CLASS( CParticleModificatorSize3DOverLife );
REGISTER_RTTI_CLASS( CParticleModificatorVelocityOverLife );
REGISTER_RTTI_CLASS( CParticleModificatorColorOverLife );
REGISTER_RTTI_CLASS( CParticleModificatorAlphaOverLife );
REGISTER_RTTI_CLASS( CParticleModificatorAlphaByDistance );
REGISTER_RTTI_CLASS( CParticleModificatorAlphaOverEffect );
REGISTER_RTTI_CLASS( CParticleModificatorRotationOverLife );
REGISTER_RTTI_CLASS( CParticleModificatorRotation3DOverLife );
REGISTER_RTTI_CLASS( CParticleModificatorRotationRateOverLife );
REGISTER_RTTI_CLASS( CParticleModificatorRotationRate3DOverLife );
REGISTER_RTTI_CLASS( CParticleModificatorAcceleration );
REGISTER_RTTI_CLASS( CParticleModificatorTarget );
REGISTER_RTTI_CLASS( CParticleModificatorTargetNode );
REGISTER_RTTI_CLASS( CParticleModificatorTextureAnimation );
REGISTER_RTTI_CLASS( CParticleModificatorVelocityTurbulize );
REGISTER_RTTI_CLASS( CParticleModificatorCollision );

REGISTER_RTTI_CLASS( IEvaluator );
REGISTER_RTTI_CLASS( IEvaluatorFloat );
REGISTER_RTTI_CLASS( CEvaluatorFloatConst );
REGISTER_RTTI_CLASS( CEvaluatorFloatRandomUniform );
REGISTER_RTTI_CLASS( CEvaluatorFloatStartEnd );
REGISTER_RTTI_CLASS( CEvaluatorFloatCurve );
REGISTER_RTTI_CLASS( CEvaluatorFloatRainStrength );
REGISTER_RTTI_CLASS( IEvaluatorColor );
REGISTER_RTTI_CLASS( CEvaluatorColorConst );
REGISTER_RTTI_CLASS( CEvaluatorColorRandom );
REGISTER_RTTI_CLASS( CEvaluatorColorStartEnd );
REGISTER_RTTI_CLASS( CEvaluatorColorCurve );
REGISTER_RTTI_CLASS( IEvaluatorVector );
REGISTER_RTTI_CLASS( CEvaluatorVectorConst );
REGISTER_RTTI_CLASS( CEvaluatorVectorRandomUniform );
REGISTER_RTTI_CLASS( CEvaluatorVectorCurve );
REGISTER_RTTI_CLASS( CEvaluatorVectorStartEnd );

REGISTER_RTTI_CLASS( CScriptSoundSystem );
REGISTER_RTTI_CLASS( CSoundEntityParam );
REGISTER_RTTI_CLASS( CSoundListenerComponent );

REGISTER_RTTI_STRUCT( SAreaEnvironmentPoint );
REGISTER_RTTI_CLASS( CAreaEnvironmentComponent );

//REGISTER_RTTI_CLASS( CRagdollBody );
//REGISTER_RTTI_CLASS( CRagdollConstraint );
REGISTER_RTTI_CLASS( CRagdoll );
//REGISTER_RTTI_CLASS( CPhysicsSystem );
//REGISTER_RTTI_CLASS( CPhysicsSystemSlot );

REGISTER_RTTI_CLASS( CGraphSocket );
REGISTER_RTTI_CLASS( CGraphConnection );
REGISTER_RTTI_CLASS( CGraphBlock );
REGISTER_RTTI_CLASS( CGraphHelperBlock );
REGISTER_RTTI_CLASS( CCommentGraphBlock );
REGISTER_RTTI_CLASS( CDescriptionGraphBlock );

REGISTER_RTTI_CLASS( IMaterialDefinition );
REGISTER_RTTI_CLASS( CMaterialBlockInterpolator )
REGISTER_RTTI_CLASS( CMaterialBlock );
REGISTER_RTTI_CLASS( CMaterialParameter );
REGISTER_RTTI_CLASS( IMaterial );
REGISTER_RTTI_CLASS( CMaterialInstance );
REGISTER_RTTI_CLASS( CMaterialGraph );
REGISTER_RTTI_CLASS( CMaterialParameterColor );
REGISTER_RTTI_CLASS( CMaterialParameterTexture );
REGISTER_RTTI_CLASS( CMaterialParameterTextureArray );
REGISTER_RTTI_CLASS( CMaterialParameterCube );
REGISTER_RTTI_CLASS( CMaterialParameterVector );
REGISTER_RTTI_CLASS( CMaterialParameterScalar );
REGISTER_RTTI_CLASS( CMaterialParameterEngineValue );
REGISTER_RTTI_CLASS( CMaterialParameterEnvColorGroup );

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

REGISTER_RTTI_CLASS( CMaterialInputSocket );
REGISTER_RTTI_CLASS( CMaterialOutputSocket );
REGISTER_RTTI_CLASS( CMaterialInputTextureSocket );
REGISTER_RTTI_CLASS( CMaterialOutputTextureSocket );
REGISTER_RTTI_CLASS( CMaterialInputCubeSocket );
REGISTER_RTTI_CLASS( CMaterialOutputCubeSocket );
REGISTER_RTTI_CLASS( CMaterialInputBoolSocket );
REGISTER_RTTI_CLASS( CMaterialOutputBoolSocket );

REGISTER_RTTI_CLASS( CMaterialBlockInput );
REGISTER_RTTI_CLASS( CMaterialBlockOutput );
REGISTER_RTTI_CLASS( CMaterialEncapsulatedGraph );
REGISTER_RTTI_CLASS( CMaterialRootBlock );
REGISTER_RTTI_CLASS( CMaterialRootDecalBlock );

REGISTER_RTTI_CLASS( CMaterialBlockLighting );
REGISTER_RTTI_CLASS( CMaterialBlockOutputColor );
REGISTER_RTTI_CLASS( CMaterialBlockOutputColorDeferred );
REGISTER_RTTI_CLASS( CMaterialBlockOutputColorHair );
REGISTER_RTTI_CLASS( CMaterialBlockOutputColorEye );
REGISTER_RTTI_CLASS( CMaterialBlockOutputColorEyeOverlay );
REGISTER_RTTI_CLASS( CMaterialBlockOutputColorEnhanced );
REGISTER_RTTI_CLASS( CMaterialBlockOutputColorSkin );

#endif

REGISTER_RTTI_CLASS( CMaterialOverrideAttachment );
REGISTER_RTTI_CLASS( CMaterialOverrideAttachmentSelectByChunk );
REGISTER_RTTI_CLASS( CMaterialOverrideComponent );

REGISTER_RTTI_CLASS( CCollisionMesh );
REGISTER_RTTI_CLASS( ICollisionShape );
REGISTER_RTTI_CLASS( CCollisionShapeConvex );
REGISTER_RTTI_CLASS( CCollisionShapeTriMesh );
REGISTER_RTTI_CLASS( CCollisionShapeBox );
REGISTER_RTTI_CLASS( CCollisionShapeCapsule );
REGISTER_RTTI_CLASS( CCollisionShapeSphere );

REGISTER_RTTI_CLASS( CFXBase );
REGISTER_RTTI_CLASS( CFXDefinition );
REGISTER_RTTI_CLASS( CFXTrackGroup );
REGISTER_RTTI_CLASS( CFXTrack );
REGISTER_RTTI_CLASS( CFXTrackItem );
REGISTER_RTTI_CLASS( CFXTrackItemCurveBase );
REGISTER_RTTI_CLASS( CFXTrackItemParameterFloat );
REGISTER_RTTI_CLASS( CFXTrackItemSoundEvent );
REGISTER_RTTI_CLASS( CFXTrackItemCameraShake );
REGISTER_RTTI_CLASS( CFXTrackItemParameterColor );
REGISTER_RTTI_CLASS( CFXTrackItemParticles );
REGISTER_RTTI_CLASS( CFXTrackItemPause );
REGISTER_RTTI_CLASS( CFXTrackItemRadialBlur );
REGISTER_RTTI_CLASS( CFXTrackItemFullscreenBlur );
REGISTER_RTTI_CLASS( CFXTrackItemMovement );
REGISTER_RTTI_CLASS( CFXTrackItemFlare );
REGISTER_RTTI_CLASS( CFXTrackItemMaterialOverride );
REGISTER_RTTI_CLASS( CFXTrackItemGlobalSpacePhysicalForce );
REGISTER_RTTI_CLASS( CFXTrackItemDisableHDRAdaptation );
REGISTER_RTTI_CLASS( CFXTrackItemEnvironmentModifier );
REGISTER_RTTI_CLASS( CFXTrackItemBrightnessTint );
REGISTER_RTTI_CLASS( CFXTrackItemDynamicLight );
REGISTER_RTTI_CLASS( CFXTrackItemForceFeedback );
REGISTER_RTTI_CLASS( IFXSpawner );
REGISTER_RTTI_CLASS( IFXPhysicalForce );
REGISTER_RTTI_CLASS( CFXExplosionImplosionPhysicalForce );
REGISTER_RTTI_CLASS( CFXFractureDesctruction );
REGISTER_RTTI_CLASS( CFXSpawnerComponent );
REGISTER_RTTI_CLASS( CFXWaterSpawner );
REGISTER_RTTI_CLASS( CFXSimpleSpawner );
REGISTER_RTTI_CLASS( CFXTrackItemSetDissolve );

REGISTER_RTTI_CLASS( CFoliageScene );
REGISTER_RTTI_CLASS( CFoliageResource );
REGISTER_RTTI_STRUCT( SFoliageLODSetting );

REGISTER_RTTI_CLASS( CVisualDebug );
REGISTER_RTTI_CLASS( CDebugAttributesManager );

REGISTER_RTTI_CLASS( CLocalizedContent );

// PathLib
REGISTER_RTTI_CLASS( CNavmeshComponent );
REGISTER_RTTI_CLASS( CNavmeshBorderAreaComponent );
REGISTER_RTTI_CLASS( CNavmeshGenerationRootComponent );
REGISTER_RTTI_CLASS( CPathLibRoughtTerrainComponent );
REGISTER_RTTI_CLASS( CNavmeshInputAttachment );
REGISTER_RTTI_CLASS( CNavmesh );
REGISTER_RTTI_CLASS( CNavmeshFactory );
REGISTER_RTTI_CLASS( CPathLibSettings );
REGISTER_RTTI_CLASS( CPathLibWorld );
REGISTER_RTTI_STRUCT( SNavmeshParams );

// GUI

REGISTER_RTTI_CLASS( CSwfResource );
REGISTER_RTTI_STRUCT( SSwfFontDesc );
REGISTER_RTTI_STRUCT( SSwfHeaderInfo );
REGISTER_RTTI_STRUCT( SFlashRenderTargetCamera );

#ifndef NO_TEST_FRAMEWORK
	REGISTER_RTTI_STRUCT( SRecordedInput );
#endif // NO_TEST_FRAMEWORK

REGISTER_RTTI_CLASS( CSRTBaseTree );

REGISTER_RTTI_CLASS( CClipMap );
REGISTER_RTTI_CLASS( CClipMapCookedData );
REGISTER_RTTI_STRUCT( STerrainTextureParameters );
REGISTER_RTTI_CLASS( CTerrainTile );

#ifdef USE_UMBRA
REGISTER_RTTI_CLASS( CUmbraScene );
REGISTER_RTTI_CLASS( CUmbraTile );
#endif // USE_UMBRA

REGISTER_RTTI_STRUCT( SWorldDescription );
REGISTER_RTTI_CLASS( CGameResource );

REGISTER_RTTI_CLASS( CApexResource );
REGISTER_RTTI_CLASS( CApexClothResource );
REGISTER_RTTI_CLASS( CApexDestructionResource );

REGISTER_RTTI_CLASS( CTimerScriptKeyword );
REGISTER_RTTI_CLASS( CGlobalWater ); //tickable water object
REGISTER_RTTI_CLASS( CResourceSimplexTree );

REGISTER_RTTI_CLASS( CNormalBlendComponent );
REGISTER_RTTI_CLASS( CNormalBlendAttachment );
REGISTER_RTTI_CLASS( INormalBlendDataSource );

REGISTER_RTTI_STRUCT( STimeScaleSource );
REGISTER_RTTI_STRUCT( STimeScaleSourceSet );

REGISTER_RTTI_CLASS( CVegetationBrushEntry );
REGISTER_RTTI_CLASS( CVegetationBrush );
REGISTER_RTTI_CLASS( CGenericGrassMask );
REGISTER_RTTI_CLASS( CGrassCellMask );
REGISTER_RTTI_CLASS( CGrassOccurrenceMap );

REGISTER_RTTI_CLASS( CCurveEntity );
REGISTER_RTTI_CLASS( CCurveComponent );
REGISTER_RTTI_CLASS( CCurveControlPointEntity );
REGISTER_RTTI_CLASS( CCurveControlPointComponent );
REGISTER_RTTI_CLASS( CCurveTangentControlPointEntity );
REGISTER_RTTI_CLASS( CCurveTangentControlPointComponent );

REGISTER_RTTI_STRUCT( SDynamicDecalMaterialInfo );

REGISTER_RTTI_CLASS( CDynamicFoliageComponent );

REGISTER_RTTI_CLASS( CSwitchableFoliageResourceFactory );
REGISTER_RTTI_STRUCT( SSwitchableFoliageEntry );
REGISTER_RTTI_CLASS( CSwitchableFoliageResource );
REGISTER_RTTI_CLASS( CSwitchableFoliageComponent );

REGISTER_RTTI_STRUCT( SFurVisualizers );
REGISTER_RTTI_STRUCT( SFurSimulation );
REGISTER_RTTI_STRUCT( SFurVolume );
REGISTER_RTTI_STRUCT( SFurStrandWidth );
REGISTER_RTTI_STRUCT( SFurStiffness );
REGISTER_RTTI_STRUCT( SFurClumping );
REGISTER_RTTI_STRUCT( SFurWaveness );
REGISTER_RTTI_STRUCT( SFurPhysicalMaterials );
REGISTER_RTTI_STRUCT( SFurColor );
REGISTER_RTTI_STRUCT( SFurDiffuse );
REGISTER_RTTI_STRUCT( SFurSpecular );
REGISTER_RTTI_STRUCT( SFurGlint );
REGISTER_RTTI_STRUCT( SFurShadow );
REGISTER_RTTI_STRUCT( SFurGraphicalMaterials );
REGISTER_RTTI_STRUCT( SFurCulling );
REGISTER_RTTI_STRUCT( SFurDistanceLOD );
REGISTER_RTTI_STRUCT( SFurDetailLOD );
REGISTER_RTTI_STRUCT( SFurLevelOfDetail );
REGISTER_RTTI_STRUCT( SFurMaterialSet );

REGISTER_RTTI_CLASS( CCurveEntitySpawner );
REGISTER_RTTI_STRUCT( SEntityWeight );

REGISTER_RTTI_STRUCT( SEventGroupsRanges );
REGISTER_RTTI_STRUCT( SSavegameInfo );
REGISTER_RTTI_STRUCT( CEntityTemplateCookedEffect );

#undef REGISTER_RTTI_CLASS
#undef REGISTER_RTTI_STRUCT
#undef REGISTER_RTTI_ENUM

#if defined( REGISTER_NOT_REGISTERED )
	#undef REGISTER_RTTI_TYPE
	#undef REGISTER_NOT_REGISTERED
#endif

#endif
