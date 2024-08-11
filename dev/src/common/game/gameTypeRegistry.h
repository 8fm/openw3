/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#ifndef _H_COMMONGAME_TYPE_REGISTRY
#define _H_COMMONGAME_TYPE_REGISTRY

// this file contains list of all types in 'game' project

// Not defined when included in game.h, but defined when included in gameClasses.cpp
#if !defined( REGISTER_RTTI_TYPE )
#define REGISTER_RTTI_TYPE( _className ) RED_DECLARE_RTTI_NAME( _className ) template<> struct TTypeName< _className >{ static const CName& GetTypeName() { return CNAME( _className ); } };
#define REGISTER_NOT_REGISTERED
#endif

#define REGISTER_RTTI_CLASS( _className ) class _className; REGISTER_RTTI_TYPE( _className );
#define REGISTER_RTTI_STRUCT( _className ) struct _className; REGISTER_RTTI_TYPE( _className );
#define REGISTER_RTTI_ENUM( _className ) enum _className; REGISTER_RTTI_TYPE( _className );


// ------------------------------- ADD CLASSES HERE -------------------------------

REGISTER_RTTI_CLASS( CCommonGameResource );
REGISTER_RTTI_CLASS( CCommonGame );
REGISTER_RTTI_CLASS( CGameplayEntity );
REGISTER_RTTI_CLASS( CProjectileTrajectory );
REGISTER_RTTI_CLASS( CActor );
REGISTER_RTTI_CLASS( CNewNPC );
REGISTER_RTTI_CLASS( CNewNPCStateBase );
REGISTER_RTTI_CLASS( CNewNPCStateIdle );
REGISTER_RTTI_CLASS( CNewNPCStateReactingBase );

REGISTER_RTTI_CLASS( CPlayer );
REGISTER_RTTI_CLASS( CPlayerStateBase );
REGISTER_RTTI_CLASS( CPlayerStateMovable );

// CGameplayEntity actions
REGISTER_RTTI_CLASS( IEntityTargetingAction )
REGISTER_RTTI_CLASS( IComponentTargetingAction )
REGISTER_RTTI_CLASS( CAddFactPerformableAction )
REGISTER_RTTI_CLASS( CEnableComponentsPerformableAction )
REGISTER_RTTI_CLASS( CEnableEncounterAction )
REGISTER_RTTI_CLASS( CPlayPropertyAnimationAction )
REGISTER_RTTI_CLASS( IEffectManagmentPerformableAction )
REGISTER_RTTI_CLASS( CPlayEffectPerformableAction )
REGISTER_RTTI_CLASS( CStopAllEffectsPerformableAction )
REGISTER_RTTI_CLASS( CStopEffectPerformableAction )
REGISTER_RTTI_CLASS( CActionPointActivationSwitcher )
REGISTER_RTTI_CLASS( CEnabledDeniedAreaAction )
REGISTER_RTTI_CLASS( CActionBroadcastEvent )
REGISTER_RTTI_STRUCT( SEntityActionsRouterEntry )
REGISTER_RTTI_CLASS( CScriptedAction )

REGISTER_RTTI_CLASS( CScriptedComponent );
REGISTER_RTTI_CLASS( CGameWorld );

// Systems
REGISTER_RTTI_CLASS( IGameSystem );
REGISTER_RTTI_CLASS( CAttitudeManager );
REGISTER_RTTI_CLASS( CFactsDB );
REGISTER_RTTI_CLASS( CInteractionsManager );
REGISTER_RTTI_CLASS( CMinigame );
REGISTER_RTTI_CLASS( CCreateEntityHelper );
REGISTER_RTTI_CLASS( CGameFastForwardSystem );
REGISTER_RTTI_CLASS( CDefinitionsManagerAccessor );

// DLC
REGISTER_RTTI_CLASS( IGameplayDLCMounter );
REGISTER_RTTI_CLASS( CGameplayDLCMounterScripted );
REGISTER_RTTI_CLASS( CSaveFileDLCMounter )
REGISTER_RTTI_CLASS( CDLCDefinition );
REGISTER_RTTI_CLASS( CDLCManager );
REGISTER_RTTI_STRUCT( SDLCLanguagePack );

// AI
REGISTER_RTTI_CLASS( IBoidLairEntity );
REGISTER_RTTI_CLASS( CSwarmLairEntity );
REGISTER_RTTI_CLASS( CBoidPointOfInterestComponent );
REGISTER_RTTI_CLASS( CBoidPointOfInterestComponentScript );
REGISTER_RTTI_STRUCT( SGameplayEventParamInt );
REGISTER_RTTI_STRUCT( SGameplayEventParamFloat );
REGISTER_RTTI_STRUCT( SGameplayEventParamCName );
REGISTER_RTTI_CLASS( CBoidActivationTriggerComponent );
REGISTER_RTTI_CLASS( CBoidAreaComponent );
REGISTER_RTTI_CLASS( CAIPresetParam );
REGISTER_RTTI_CLASS( CGameplayWindComponent );
REGISTER_RTTI_CLASS( CEDSavedAnswer );
REGISTER_RTTI_CLASS( CEdWizardSavedAnswers );

// Action areas
REGISTER_RTTI_CLASS( CActionAreaComponent );
REGISTER_RTTI_CLASS( CActionAreaBlendActor );
REGISTER_RTTI_CLASS( CAnimDef );
REGISTER_RTTI_STRUCT( SAnimShift );
REGISTER_RTTI_CLASS( CVertexEditorEntity );
REGISTER_RTTI_CLASS( CVertexComponent );

// Interaction system
REGISTER_RTTI_CLASS( CInteractionComponent );
REGISTER_RTTI_CLASS( CInteractionToComponentComponent );
REGISTER_RTTI_CLASS( CInteractionTooltipComponent );
REGISTER_RTTI_CLASS( CInteractionAreaComponent );
REGISTER_RTTI_STRUCT( STargetSelectionWeights );
REGISTER_RTTI_STRUCT( STargetSelectionData );

// Behavior Tree
REGISTER_RTTI_STRUCT( SBTNodeResult );
REGISTER_RTTI_CLASS( IBehTreeValueEnum );
REGISTER_RTTI_CLASS( CBehTreeValList );
REGISTER_RTTI_CLASS( CBehTreeValFloat );
REGISTER_RTTI_CLASS( CBehTreeValBool );
REGISTER_RTTI_CLASS( CBehTreeValInt );
REGISTER_RTTI_CLASS( CBehTreeValVector );
REGISTER_RTTI_CLASS( CBehTreeValTemplate );
REGISTER_RTTI_CLASS( CBehTreeValFile );
REGISTER_RTTI_CLASS( CBehTreeValString );
REGISTER_RTTI_CLASS( CBehTreeValCName );
REGISTER_RTTI_CLASS( CBehTreeValRes );
REGISTER_RTTI_CLASS( CBehTreeValEntityHandle );
REGISTER_RTTI_CLASS( CBehTreeValSteeringGraph );
REGISTER_RTTI_CLASS( CBehTreeValFormation );
REGISTER_RTTI_CLASS( CBehTreeValEMoveType );
REGISTER_RTTI_CLASS( CBehTreeValEExplorationType );
REGISTER_RTTI_CLASS( IBehTreeNodeDefinition );
REGISTER_RTTI_CLASS( IBehTreeNodeCompositeDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeScriptDecoratorDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeScriptTerminalDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeBaseRotateToTargetDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeAtomicRotateToTargetDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeFinishAnimationsDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeBreakAnimationsDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeAtomicMatchActionTargetRotationDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeAtomicTeleportDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeTeleportToActionTargetDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeTeleportToActionTargetCheckPositionDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeAtomicTeleportToActionPointDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeAttachToCurveDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeFlyOnCurveDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeAtomicMoveToDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeAtomicMoveToActionTargetDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeAtomicMoveToWanderpointDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeAtomicMoveToCombatTargetDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeAtomicMoveToCustomPointDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeAtomicMoveToActionPointDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeAtomicMoveToPredefinedPathDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeAtomicPursueTargetDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeAtomicActionDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeAtomicPlayAnimationDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeAtomicPlayAnimationEventDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeAtomicPlayAnimationManualMotionExtractionDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeAtomicDamageReactionDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeDecoratorGuardRetreatDefinition );
REGISTER_RTTI_CLASS( IBehTreeNodeDecoratorSteeringTargeterDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeDecoratorSetSteeringTargetNodeDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeDecoratorSetSteeringNamedTargetNodeDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeCustomSteeringDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeKeepDistanceDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeFleeReactionDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodePredefinedPathDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeDecoratorPassMetaobstaclesDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodePredefinedPathWithCompanionDefinition );
REGISTER_RTTI_CLASS( IBehTreeNodeDecoratorDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeConditionSpeedEngineValDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeScriptConditionalDecoratorDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeConditionTeleportToWorkDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeAlreadyAtWorkDefinition );
REGISTER_RTTI_CLASS( IBehTreeNodeWorkRelatedConditionDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeIsAtWorkConditionDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeCanUseChatSceneConditionDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeIsSittingInInteriorConditionDefinition );
REGISTER_RTTI_CLASS( CBehTreeDecoratorUninterruptableDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeSelectPartyMemberDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeConditionPartyMembersCountDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeSelectFormationLeaderDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeChoiceDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeEvaluatingSelectorDefinition );
REGISTER_RTTI_CLASS( IBehTreeTask );
REGISTER_RTTI_CLASS( IBehTreeObjectDefinition );
REGISTER_RTTI_CLASS( IBehTreeTaskDefinition );
REGISTER_RTTI_CLASS( IBehTreeOnSpawnEffector );
REGISTER_RTTI_CLASS( IBehTreeConditionalTaskDefinition );
REGISTER_RTTI_CLASS( CBehTree );
REGISTER_RTTI_CLASS( CBehTreeInstance );
REGISTER_RTTI_CLASS( CBehTreeFactory );
REGISTER_RTTI_CLASS( CBehTreeNodeLoopDecoratorDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeLoopWithTimelimitDecoratorDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeSequenceDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodePersistantSequenceDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeSequenceCheckAvailabilityDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeSequenceFowardAndBackDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeSelectorDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeProbabilitySelectorDefinition );
REGISTER_RTTI_CLASS( IBehTreeNodeSpecialDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeParallelDefinition );
REGISTER_RTTI_CLASS( IBehTreeMetanodeDefinition );
REGISTER_RTTI_CLASS( IBehTreeVoidDefinition );
REGISTER_RTTI_CLASS( CBehTreeCommentDefinition );
REGISTER_RTTI_CLASS( IBehTreeMetanodeOnSpawnDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeTemplateDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeSubtreeDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeSubtreeListDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeConditionDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeConditionChanceDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeConditionTrueDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeConditionReactionEventDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeConditionDistanceToActionTargetDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeConditionDistanceToNamedTargetDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeConditionDistanceToCombatTargetDefinition );
REGISTER_RTTI_CLASS( CBehTreeConditionRespondToMusicDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeConditionDistanceToCustomTargetDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeConditionLineofSightDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeConditionTargetNoticedDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeConditionCombatTargetNoticedDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeConditionLineofSightToNamedTargetDefinition );
REGISTER_RTTI_CLASS( IBehTreeNodeConditionCheckRotationDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeConditionCheckRotationToCombatTargetDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeConditionCheckRotationToActionTargetDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeConditionIsConsciousAtWorkDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeConditionIsWorkingDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeBaseConditionalTreeDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeConditionalTreeDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeConditionalTreeNPCTypeDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeConditionalFlagTreeDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeConditionalChooseBranchDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeConditionIsInAttackRangeDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeConditionIsInCameraViewDefinition );
REGISTER_RTTI_CLASS( IBehTreeNodeConditionIsInGuardAreaDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeConditionIsActionTargetInGuardAreaDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeConditionIsThisActorInGuardAreaDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeConditionIsCustomTargetInGuardAreaDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeConditionClearLineToTargetDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeConditionHasCombatTargetDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeArbitratorDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeBaseForcedBehaviorDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeForcedBehaviorDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeExternalListenerDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeScriptedActionsListReaderDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodePlaySceneDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeDecoratorLookAtDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeAtomicLookAtDefinition );
//REGISTER_RTTI_CLASS( CBehTreeNodePlaySceneIntroDefinition );
REGISTER_RTTI_CLASS( IBehTreeNodeSetupCustomMoveDataDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeSetupCustomMoveTargetToPositionDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeSetCustomMoveTargetToInteractionPointDefintion );
REGISTER_RTTI_CLASS( CBehTreeNodeSetCustomMoveTargetToDestinationPointDefintion );
REGISTER_RTTI_CLASS( CBehTreeNodeNotifyDoorDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeAtomicIdleDefinition );
REGISTER_RTTI_CLASS( IBehTreeNodeExplorationQueueDecoratorDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeExplorationQueueRegisterDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeExplorationQueueUseDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeDecoratorSnapToNavigationDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeCompleteImmediatelyDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeSelectTargetByTagDecoratorDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeSelectTargetByTagDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeSelectTargetByTagInAreaDecoratorDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeDecoratorSelectRandomTerrainPositionInAreaDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeCasualMovementDecoratorDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeSelectPatrolingTargetDecoratorDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeSelectWanderingTargetDecoratorDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeWanderingTaggedTargetDecoratorDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeRandomWanderingTargetDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeHistoryWanderingTargetDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeDynamicWanderingTargetDefinition );
REGISTER_RTTI_CLASS( CBehTreeDecoratorSelectFleeDestinationDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeDecoratorSetupFormationDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeDecoratorLeadFormationDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeSelectActionPointDecoratorDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodePartyWorkSynchronizerDecoratorDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodePerformWorkDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeWorkDecoratorDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeCustomWorkDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeDespawnDefinition );
REGISTER_RTTI_CLASS( CBehTreeMetanodeWorkInitializer );
REGISTER_RTTI_CLASS( CBehTreeNodeActivationDelayDecoratorDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeDelayActivationDecoratorDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeDurationDecoratorDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeDurationRangeDecoratorDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodePokeDecoratorDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeDecoratorSemaphoreDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeDecoratorPriorityOnSemaphoreDefinition );
REGISTER_RTTI_CLASS( CBehTreeDecoratorActivePriorityDefinition );
REGISTER_RTTI_CLASS( IBehTreeNodeDecoratorAsyncQueryDefinition );
REGISTER_RTTI_CLASS( IBehTreeNodeDecoratorAsyncResultDefinition );
REGISTER_RTTI_CLASS( IBehTreeNodeDecoratorWalkableSpotQueryDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeDecoratorWalkableSpotRingQueryDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeDecoratorWalkableSpotClosestQueryDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeDecoratorWalkableSpotResultDefintion );
REGISTER_RTTI_CLASS( CBehTreeNodeDecoratorCompleteInProximityDefinition );
REGISTER_RTTI_CLASS( IBehTreeDynamicNodeBaseDefinition );
REGISTER_RTTI_CLASS( CBehTreeDynamicNodeDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeComplexConditionDefinition );
REGISTER_RTTI_CLASS( IBehTreeAtomicCondition );
REGISTER_RTTI_CLASS( IBehTreeAtomicBinaryCondition );
REGISTER_RTTI_CLASS( CBehTreeAtomicANDCondition );
REGISTER_RTTI_CLASS( CBehTreeAtomicORCondition );
REGISTER_RTTI_CLASS( CBehTreeAtomicNOTCondition );
REGISTER_RTTI_CLASS( CBehTreeAtomicTestSubtreeCondition );
REGISTER_RTTI_CLASS( CBehTreeNodeConditionCounterDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeConditionCounterNewDefinition );
REGISTER_RTTI_CLASS( IBehTreeNodeConditionWasEventFiredRecentlyBaseDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeConditionWasEventFiredRecentlyDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeConditionWasAnyOfEventsFiredRecentlyDefinition );
REGISTER_RTTI_CLASS( CBehTreeAtomicSteerWithTargetDefinition );
REGISTER_RTTI_CLASS( CBehTreeAtomicSteerWithCustomTargetDefinition );
REGISTER_RTTI_CLASS( CBehTreeMachine );
REGISTER_RTTI_CLASS( CAIDefaults );
REGISTER_RTTI_CLASS( IAIParameters ); 
REGISTER_RTTI_CLASS( CAIParameters );
REGISTER_RTTI_CLASS( ICustomValAIParameters );
REGISTER_RTTI_CLASS( CAIRedefinitionParameters );
REGISTER_RTTI_CLASS( IAISpawnTreeParameters );
REGISTER_RTTI_CLASS( IAISpawnTreeSubParameters );
REGISTER_RTTI_CLASS( CEncounterParameters );
REGISTER_RTTI_CLASS( CGuardAreaParameters );
REGISTER_RTTI_CLASS( CIdleBehaviorsDefaultParameters );
REGISTER_RTTI_CLASS( IAITree );
REGISTER_RTTI_CLASS( CAIBaseTree );
REGISTER_RTTI_CLASS( CAITree );
REGISTER_RTTI_CLASS( CAIPerformCustomWorkTree );
REGISTER_RTTI_CLASS( IAIActionTree );
REGISTER_RTTI_CLASS( IAIActionParameters );
REGISTER_RTTI_CLASS( CAIActionSequence );
REGISTER_RTTI_CLASS( IAIExplorationTree );
REGISTER_RTTI_CLASS( CAIDespawnTree );
REGISTER_RTTI_CLASS( CAIDespawnParameters );
REGISTER_RTTI_STRUCT( SGameplayEventParamObject );
REGISTER_RTTI_STRUCT( SArbitratorQueryData );
REGISTER_RTTI_STRUCT( SPlaySceneRequestData );
REGISTER_RTTI_STRUCT( SBehTreeDynamicNodeEventData );
REGISTER_RTTI_STRUCT( SBehTreeDynamicNodeCancelEventData );
REGISTER_RTTI_CLASS( CBehTreeValAreaSelectionMode )
REGISTER_RTTI_STRUCT( SBehTreeAreaSelection )
REGISTER_RTTI_STRUCT( SForcedBehaviorEventData );
REGISTER_RTTI_STRUCT( SDynamicNodeSaveStateRequestEventData );
REGISTER_RTTI_CLASS( CBaseDamage );
REGISTER_RTTI_CLASS( CBehTreeWorkData );
REGISTER_RTTI_CLASS( CBehTreeGuardAreaData );
REGISTER_RTTI_CLASS( CBehTreeNodeBaseIdleDynamicRootDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeIdleDynamicRootDefinition );
REGISTER_RTTI_CLASS( CBehTreePositioningRequest );
REGISTER_RTTI_STRUCT( SPositioningFilter );
REGISTER_RTTI_STRUCT( SClosestSpotFilter );
REGISTER_RTTI_STRUCT( SAIPositionPrediction );
REGISTER_RTTI_CLASS( CBehTreeSpawnContext );
REGISTER_RTTI_STRUCT( CAISteeringGraphData )
REGISTER_RTTI_CLASS( CBehTreeCustomMoveData );
REGISTER_RTTI_CLASS( CBehTreeCounterData );

REGISTER_RTTI_STRUCT( SEncounterGroupLimit );
REGISTER_RTTI_CLASS( CEncounter );
REGISTER_RTTI_CLASS( ISpawnCondition );
REGISTER_RTTI_CLASS( ISpawnScriptCondition );
REGISTER_RTTI_CLASS( CSpawnTree );
REGISTER_RTTI_CLASS( CSpawnTreeFactory );
REGISTER_RTTI_CLASS( ISpawnTreeCompositeNode );
REGISTER_RTTI_CLASS( CSpawnTreeNode );
REGISTER_RTTI_CLASS( CSpawnTreeParallelNode );
REGISTER_RTTI_CLASS( CSpawnTreeSelectRandomNode );
REGISTER_RTTI_CLASS( ISpawnTreeBaseNode );
REGISTER_RTTI_CLASS( ISpawnTreeBranch );
REGISTER_RTTI_CLASS( ISpawnTreeDecorator );
REGISTER_RTTI_CLASS( CSpawnTreeDecoratorInitializersList );
REGISTER_RTTI_CLASS( CSpawnTreeVoidDecorator );
REGISTER_RTTI_CLASS( ISpawnTreeScriptedDecorator );
REGISTER_RTTI_CLASS( CSpawnTreeIncludeTreeNode );
REGISTER_RTTI_CLASS( CSpawnTreeEntryList );
REGISTER_RTTI_CLASS( CBaseCreatureEntry );
REGISTER_RTTI_CLASS( ISpawnTreeLeafNode );
REGISTER_RTTI_CLASS( CSpawnTreeBaseEntryGenerator );
REGISTER_RTTI_CLASS( CWanderAndWorkEntryGenerator );
REGISTER_RTTI_CLASS( CWorkEntryGenerator );
REGISTER_RTTI_STRUCT( SCreatureDefinitionWrapper );
REGISTER_RTTI_STRUCT( SWorkCategoriesWrapper );
REGISTER_RTTI_STRUCT( SWorkCategoryWrapper );
REGISTER_RTTI_STRUCT( SCreatureEntryEntryGeneratorNodeParam );
REGISTER_RTTI_STRUCT( SWanderAndWorkEntryGeneratorParams );
REGISTER_RTTI_STRUCT( SWorkEntryGeneratorParam );
REGISTER_RTTI_STRUCT( SWorkSmartAIEntryGeneratorNodeParam );
REGISTER_RTTI_STRUCT( SWanderHistoryEntryGeneratorParams );
REGISTER_RTTI_STRUCT( SWorkWanderSmartAIEntryGeneratorParam );
REGISTER_RTTI_CLASS( CCreatureEntry );
REGISTER_RTTI_CLASS( CCreaturePartyEntry );
REGISTER_RTTI_CLASS( CPartySpawnOrganizer );
REGISTER_RTTI_CLASS( CPartySpawnpointOrganizer );
REGISTER_RTTI_CLASS( CSpawnTreeEntrySubDefinition );
REGISTER_RTTI_CLASS( CEncounterCreatureDefinition );
REGISTER_RTTI_CLASS( CSpawnTreeQuestPhase );
REGISTER_RTTI_CLASS( CSpawnTreeQuestNode );
REGISTER_RTTI_CLASS( CSpawnTreeTimetableEntry );
REGISTER_RTTI_CLASS( CSpawnTreeConditionNode );
REGISTER_RTTI_STRUCT( SCompiledInitializer );
REGISTER_RTTI_CLASS( ISpawnTreeInitializer );
REGISTER_RTTI_CLASS( ISpawnTreeSpawnMonitorBaseInitializer );
REGISTER_RTTI_CLASS( ISpawnTreeSpawnMonitorInitializer );
REGISTER_RTTI_CLASS( CSpawnTreeInitializerSpawnLimitMonitor );
REGISTER_RTTI_CLASS( CSpawnTreeInitializerCollectDetachedSetup );
REGISTER_RTTI_CLASS( ISpawnTreeInitializerToggleBehavior );
REGISTER_RTTI_CLASS( ISpawnTreeInitializerAI );
REGISTER_RTTI_CLASS( ISpawnTreeScriptedInitializer );
REGISTER_RTTI_CLASS( ISpawnTreeInitializerGuardAreaBase );
REGISTER_RTTI_CLASS( CSpawnTreeInitializerGuardArea );
REGISTER_RTTI_CLASS( CSpawnTreeInitializerGuardAreaByHandle );
REGISTER_RTTI_CLASS( CSpawnTreeInitializerAttitude );
REGISTER_RTTI_CLASS( CSpawnTreeDespawnInitializer );
REGISTER_RTTI_CLASS( CSpawnTreeInitializerPersistent );
REGISTER_RTTI_CLASS( CSpawnTreeAIDespawnInitializer );
REGISTER_RTTI_STRUCT( SSpawnTreeDespawnConfiguration );
REGISTER_RTTI_STRUCT( SSpawnTreeAIDespawnConfiguration );
REGISTER_RTTI_STRUCT( SCompiledSpawnStrategyInitializer );
REGISTER_RTTI_CLASS( ISpawnTreeSpawnStrategy );
REGISTER_RTTI_CLASS( CSimpleSpawnStrategy );
REGISTER_RTTI_STRUCT( SSpawnStrategyRange );
REGISTER_RTTI_CLASS( CMultiRangeSpawnStrategy );
REGISTER_RTTI_STRUCT( SSpawnTreeEntryStealCreatureState );
REGISTER_RTTI_STRUCT( SSpawnTreeEntrySetup );
REGISTER_RTTI_CLASS( CSpawnTreeWaypointSpawner );
REGISTER_RTTI_CLASS( CSpawnTreeActionPointSpawner );
REGISTER_RTTI_CLASS( ISpawnTreeSpawnerInitializer );
REGISTER_RTTI_CLASS( CSpawnTreeInitializerWaypointSpawner );
REGISTER_RTTI_CLASS( CSpawnTreeInitializerActionpointSpawner );
REGISTER_RTTI_CLASS( CSpawnTreeInitializerIdleAI );
REGISTER_RTTI_CLASS( ISpawnTreeInitializerIdleSmartAI );
REGISTER_RTTI_CLASS( CSpawnTreeInitializerStartingBehavior );
REGISTER_RTTI_CLASS( CSpawnTreeInitializerBaseStartingBehavior );
REGISTER_RTTI_CLASS( ISpawnTreeInitializerCommunityAI );
REGISTER_RTTI_CLASS( CSpawnTreeInitializerForceCombat );
REGISTER_RTTI_CLASS( CSpawnTreeInitializerSetAppearance );
REGISTER_RTTI_CLASS( CSpawnTreeInitializerRainHandler );
REGISTER_RTTI_CLASS( CSpawnTreeInitializerAddTag );

//REGISTER_RTTI_ENUM( EArbitratorPriorities );
//REGISTER_RTTI_ENUM( EPathEngineAgentType );

REGISTER_RTTI_CLASS( CGameplayDestructionSystemComponent );
REGISTER_RTTI_CLASS( CWayPointComponent );
REGISTER_RTTI_CLASS( CPatrolPointComponent );
REGISTER_RTTI_STRUCT( SWanderPointConnection );
REGISTER_RTTI_CLASS( CWanderPointComponent );
REGISTER_RTTI_CLASS( CDeniedAreaSaveable );
REGISTER_RTTI_CLASS( CDeniedAreaBlock );
REGISTER_RTTI_CLASS( CMetalinkComponent );
REGISTER_RTTI_CLASS( CMetalinkWithAIQueueComponent );
REGISTER_RTTI_CLASS( CMetalinDestinationComponent );
REGISTER_RTTI_STRUCT( SPartyWaypointData );
REGISTER_RTTI_STRUCT( SPartySpawner );
REGISTER_RTTI_CLASS( CWayPointsCollection );
REGISTER_RTTI_CLASS( CWayPointsCollectionsSet );

// Movement
REGISTER_RTTI_CLASS( CMovingAgentComponent );
REGISTER_RTTI_CLASS( CMovingPhysicalAgentComponent );
REGISTER_RTTI_STRUCT( SMoveLocomotionGoal );
REGISTER_RTTI_STRUCT( SAnimatedSlideSettings );
REGISTER_RTTI_STRUCT( SActionMatchToSettings );
REGISTER_RTTI_STRUCT( SActionMatchToTarget );
REGISTER_RTTI_CLASS( CMovementAdjustor );
REGISTER_RTTI_STRUCT( SMovementAdjustmentRequestTicket );
REGISTER_RTTI_STRUCT( SAnimationProxyData )

// Formations
REGISTER_RTTI_CLASS( CFormationFactory )
REGISTER_RTTI_CLASS( CFormation )
REGISTER_RTTI_CLASS( IFormationLogic )
REGISTER_RTTI_CLASS( CSteeringFormationLogic )
REGISTER_RTTI_CLASS( CSlotFormationLogic )
REGISTER_RTTI_STRUCT( SFormationConstraintDefinition )
REGISTER_RTTI_CLASS( CFormationSlotDefinition )
REGISTER_RTTI_CLASS( IFormationPatternNode )
REGISTER_RTTI_CLASS( CFormationPatternCompositeNode )
REGISTER_RTTI_CLASS( CFormationPatternRepeatNode )
REGISTER_RTTI_CLASS( CFormationPatternSlotsNode )
REGISTER_RTTI_CLASS( CFormationLeaderData )
REGISTER_RTTI_CLASS( CAISteeringFormationLeaderData )
REGISTER_RTTI_CLASS( CAISlotFormationLeaderData )
REGISTER_RTTI_CLASS( CAIFormationData )

// Formations - steering
REGISTER_RTTI_STRUCT( SFormationSteeringInput )
REGISTER_RTTI_CLASS( IFormationSteeringTask )
REGISTER_RTTI_CLASS( IFormationFragmentarySteeringTask )
REGISTER_RTTI_CLASS( CFormationSteerToLeaderSteeringTask )
REGISTER_RTTI_CLASS( CFormationSteerToCenterOfMassSteeringTask )
REGISTER_RTTI_CLASS( CFormationDontBackDownSteeringTask )
REGISTER_RTTI_CLASS( CFormationFaceSteeringTask )
REGISTER_RTTI_CLASS( CFormationSteerToPathSteeringTask )
REGISTER_RTTI_CLASS( CFormationKeepDistanceToMembersSteeringTask )
REGISTER_RTTI_CLASS( CFormationKeepSpeedSteeringTask )
REGISTER_RTTI_CLASS( CFormationKeepAwaylLeaderSteeringTask )
REGISTER_RTTI_CLASS( CFormationKeepComradesSpeedSteeringTask )
REGISTER_RTTI_CLASS( CFormationDontFallBehindSteeringTask )
REGISTER_RTTI_CLASS( CFormationSteerToSlotSteeringTask )
REGISTER_RTTI_CLASS( CFormationCatchupSlotSteeringTask )
REGISTER_RTTI_CLASS( IFormationSteeringCondition )
REGISTER_RTTI_CLASS( CFormationIsMovingSteeringCondition )
REGISTER_RTTI_CLASS( CFormationIsBrokenSteeringCondition )


// Reactions
REGISTER_RTTI_CLASS( CReactionsManager );
REGISTER_RTTI_CLASS( CInterestPointComponent );
REGISTER_RTTI_CLASS( CInterestPointInstance );

// Interest points
REGISTER_RTTI_CLASS( CInterestPoint );
REGISTER_RTTI_CLASS( CScriptedInterestPoint );

// Reaction conditions
REGISTER_RTTI_CLASS( IReactionCondition );
REGISTER_RTTI_CLASS( CReactionOrCondition );
REGISTER_RTTI_CLASS( CReactionAndCondition );
REGISTER_RTTI_CLASS( CReactionScriptedCondition );

// Reaction actions
REGISTER_RTTI_CLASS( IReactionAction );
REGISTER_RTTI_CLASS( CReactionSendEvent );
REGISTER_RTTI_CLASS( CReactionMultiAction );
REGISTER_RTTI_CLASS( CReactionQuestNotification );
REGISTER_RTTI_CLASS( CReactionAttitudeChange );
REGISTER_RTTI_CLASS( CReactionScript );

REGISTER_RTTI_CLASS( CBehTreeReactionEventData );
REGISTER_RTTI_CLASS( CBehTreeReactionManager );

// Reaction scenes
REGISTER_RTTI_CLASS( CReactionScene );
REGISTER_RTTI_CLASS( CReactionSceneActorComponent );

// Potential fields
REGISTER_RTTI_CLASS( IPotentialField );
REGISTER_RTTI_CLASS( CCircularPotentialField );
REGISTER_RTTI_CLASS( CSoundPotentialField );

// steering
REGISTER_RTTI_CLASS( CMoveSteeringBehavior )
REGISTER_RTTI_CLASS( CMoveSteeringBehaviorFactory )
REGISTER_RTTI_CLASS( IMoveSteeringNode )
REGISTER_RTTI_CLASS( IMoveSNComposite )
REGISTER_RTTI_CLASS( CMoveSNComposite )
REGISTER_RTTI_CLASS( CMoveSNCondition )
REGISTER_RTTI_CLASS( CMoveSNTask )
REGISTER_RTTI_CLASS( IMoveSteeringTask )
REGISTER_RTTI_CLASS( CMoveSTChangeSpeed )
REGISTER_RTTI_CLASS( IManageSpeedSteeringTask )
REGISTER_RTTI_CLASS( CMoveSTMaintainSpeed )
REGISTER_RTTI_CLASS( CMoveSTMaintainRandomSpeed )
REGISTER_RTTI_CLASS( CMoveSTResetSteering )
REGISTER_RTTI_CLASS( CMoveSTStop )
REGISTER_RTTI_CLASS( CMoveSTStopOnFreezing )
REGISTER_RTTI_CLASS( CMoveSTArrive )
REGISTER_RTTI_CLASS( CMoveSTApplySteering )
REGISTER_RTTI_CLASS( CMoveSTApplyAnimationSteering )
REGISTER_RTTI_CLASS( CMoveSTRotate )
REGISTER_RTTI_CLASS( CMoveSTSnapToMinimalVelocity )
REGISTER_RTTI_CLASS( CMoveSTMove )
REGISTER_RTTI_CLASS( CMoveSTMoveWithOffset )
REGISTER_RTTI_CLASS( CMoveSTFinalStep )
REGISTER_RTTI_CLASS( CMoveSTKeepNavdata )
REGISTER_RTTI_CLASS( CMoveSTMatchHeadingOrientation )
REGISTER_RTTI_CLASS( CMoveSTKeepAwayWalls )
REGISTER_RTTI_CLASS( CMoveSTKeepAwayWallsInPathfollow )
REGISTER_RTTI_CLASS( CMoveSTSeparateFromActors )
REGISTER_RTTI_CLASS( CMoveSTStep )
REGISTER_RTTI_CLASS( CMoveSTSlide )
REGISTER_RTTI_CLASS( CMoveSTSetBehaviorVariable )
REGISTER_RTTI_CLASS( CMoveSTSetFlags );
REGISTER_RTTI_CLASS( CMoveSTSetupRotationChange )
REGISTER_RTTI_CLASS( CMoveSTMapRotationChangeUsingCustomRotation );
REGISTER_RTTI_CLASS( CMoveSTSetMaxDirectionChange )
REGISTER_RTTI_CLASS( CMoveSTSetMaxRotationChange )
REGISTER_RTTI_CLASS( CMoveSTSetAcceleration )
REGISTER_RTTI_CLASS( CMoveSTAvoidObstacles )
REGISTER_RTTI_CLASS( CMoveSTCollisionResponse )
REGISTER_RTTI_CLASS( CMoveSTForwardCollisionResponse )
REGISTER_RTTI_CLASS( CMoveSTAdjustRotationChanges )
REGISTER_RTTI_CLASS( CMoveSTKeepAwayTarget )
REGISTER_RTTI_CLASS( CMoveSTMoveTightening )
REGISTER_RTTI_CLASS( CMoveSTResolveStucking )
REGISTER_RTTI_CLASS( IMoveSteeringCondition )
REGISTER_RTTI_CLASS( CMoveSCAgentSpeed )
REGISTER_RTTI_CLASS( CMoveSteeringCompositeCondition )
REGISTER_RTTI_CLASS( CMoveSCHeadingOutputLength )
REGISTER_RTTI_CLASS( CMoveSCGoalChannel )
REGISTER_RTTI_CLASS( CMoveSCIsGoalSet )
REGISTER_RTTI_CLASS( CMoveSCIsGoalHeadingSet )
REGISTER_RTTI_CLASS( CMoveSCOrientationMatchHeading )
REGISTER_RTTI_CLASS( CMoveSCFlags )
REGISTER_RTTI_CLASS( CMoveSCCompareBehaviorVariable )
REGISTER_RTTI_CLASS( CMoveSCWaitingForNotification )
REGISTER_RTTI_CLASS( CMoveSCWasNotificationReceived )
REGISTER_RTTI_CLASS( CMoveSCWasEventTriggered )
REGISTER_RTTI_CLASS( CMoveSCAnimEvent )
REGISTER_RTTI_CLASS( CMoveSCHeadingToGoalAngle )
REGISTER_RTTI_CLASS( CMoveSCArrival )
REGISTER_RTTI_CLASS( CMoveSCDistanceToGoal )
REGISTER_RTTI_CLASS( CMoveSCDistanceToDestination )
REGISTER_RTTI_CLASS( CMoveSCDistanceToTarget )
REGISTER_RTTI_CLASS( CMoveSCScriptedCondition )
REGISTER_RTTI_CLASS( CMoveSCHasTargetNodeSetCondition )
REGISTER_RTTI_CLASS( CMoveSCNavigationClearLine )
REGISTER_RTTI_CLASS( CMoveSTWalkSideBySide )
REGISTER_RTTI_CLASS( CMoveSCWalkSideBySide )
REGISTER_RTTI_CLASS( CMoveSTMaintainTargetSpeed )
REGISTER_RTTI_CLASS( CMoveSTSaneMaintainTargetSpeed )
REGISTER_RTTI_CLASS( CMoveSTFaceTargetFacing )
REGISTER_RTTI_CLASS( CTeleporter )
REGISTER_RTTI_CLASS( CMoveTRGScript )
REGISTER_RTTI_CLASS( CMoveSTSpeedMultOnPath )
REGISTER_RTTI_CLASS( IMoveTargetSteeringTask )
REGISTER_RTTI_CLASS( CMoveSTKeepDistanceToTarget )
REGISTER_RTTI_CLASS( CMoveSTNeverBackDown )
REGISTER_RTTI_CLASS( CMoveSTFaceTarget )
REGISTER_RTTI_CLASS( IMoveTargetPositionSteeringTask )

// Scenes
REGISTER_RTTI_CLASS( StorySceneFactory );
REGISTER_RTTI_CLASS( CStorySceneControlPart );
//REGISTER_RTTI_CLASS( IStorySceneSectionOverrideCondition );
//REGISTER_RTTI_CLASS( CStorySceneSectionDistanceOverrideCondition );
//REGISTER_RTTI_CLASS( CStorySceneSectionEavesdroppingOverrideCondition );

REGISTER_RTTI_CLASS( IStorySceneChoiceLineAction );
REGISTER_RTTI_CLASS( CStorySceneChoiceLineActionScripted );
REGISTER_RTTI_CLASS( CStorySceneChoiceLineActionScriptedContentGuard );
REGISTER_RTTI_CLASS( CStorySceneChoiceLineActionStallForContent );

REGISTER_RTTI_CLASS( CStorySceneAction );
REGISTER_RTTI_CLASS( CStorySceneActionMoveTo );
REGISTER_RTTI_CLASS( CStorySceneActionSlide );
REGISTER_RTTI_CLASS( CStorySceneActionTeleport );
REGISTER_RTTI_CLASS( CStorySceneActionStopWork );
REGISTER_RTTI_CLASS( CStorySceneActionStartWork );
REGISTER_RTTI_CLASS( CStorySceneActionRotateToPlayer );
REGISTER_RTTI_CLASS( CStorySceneActionEquipItem );

REGISTER_RTTI_STRUCT( ApertureDofParams );
REGISTER_RTTI_STRUCT( StorySceneCameraDefinition );
REGISTER_RTTI_STRUCT( CEventGeneratorCameraParams );
REGISTER_RTTI_CLASS( CStorySceneDialogsetSlot );

REGISTER_RTTI_CLASS( CStorySceneWaypointComponent );
REGISTER_RTTI_STRUCT( SSceneChoice );

REGISTER_RTTI_STRUCT( SSceneCameraShotDescription );
REGISTER_RTTI_STRUCT( SScenePersonalCameraDescription );
REGISTER_RTTI_STRUCT( SSceneMasterCameraDescription );
REGISTER_RTTI_STRUCT( SSceneCustomCameraDescription );

REGISTER_RTTI_CLASS( CStorySceneDialogset );
REGISTER_RTTI_CLASS( CStorySceneDialogsetInstance );
REGISTER_RTTI_CLASS( CStorySceneDialogsetFactory );

REGISTER_RTTI_STRUCT( SCutsceneActorOverrideMapping );
REGISTER_RTTI_CLASS( IStorySceneItem );
REGISTER_RTTI_CLASS( CStorySceneActor );
REGISTER_RTTI_CLASS( CStorySceneProp );
REGISTER_RTTI_CLASS( CStorySceneEffect );
REGISTER_RTTI_CLASS( CStorySceneLight );
REGISTER_RTTI_STRUCT( SStorySceneAttachmentInfo );
REGISTER_RTTI_STRUCT( SStorySceneLightTrackingInfo );
REGISTER_RTTI_STRUCT( StorySceneExpectedActor );
REGISTER_RTTI_STRUCT( StorySceneDefinition );
REGISTER_RTTI_CLASS( CStorySceneLinkElement );
REGISTER_RTTI_CLASS( CStoryScene );
REGISTER_RTTI_STRUCT( CStorySceneActorTemplate );
REGISTER_RTTI_STRUCT( CStorySceneActorPosition );
REGISTER_RTTI_STRUCT( CStorySceneVoicetagMapping );
REGISTER_RTTI_CLASS( CStorySceneInput );
REGISTER_RTTI_CLASS( CStorySceneOutput );
REGISTER_RTTI_CLASS( CStorySceneEventInfo );
REGISTER_RTTI_CLASS( CStorySceneLocaleVariantMapping );
REGISTER_RTTI_CLASS( CStorySceneSectionVariantElementInfo );
REGISTER_RTTI_CLASS( CStorySceneSection );
REGISTER_RTTI_CLASS( CStorySceneSectionVariant );
REGISTER_RTTI_CLASS( CStorySceneFlowCondition );
REGISTER_RTTI_CLASS( CStorySceneFlowSwitchCase );
REGISTER_RTTI_CLASS( CStorySceneFlowSwitch );
REGISTER_RTTI_CLASS( CStorySceneRandomizer );
REGISTER_RTTI_CLASS( CStorySceneElement );
REGISTER_RTTI_CLASS( CStorySceneLinkHub );
REGISTER_RTTI_CLASS( CAbstractStorySceneLine );
REGISTER_RTTI_CLASS( CStorySceneLine );
REGISTER_RTTI_CLASS( CStorySceneComment );
REGISTER_RTTI_CLASS( CStorySceneQuestChoiceLine );
REGISTER_RTTI_CLASS( CStorySceneCutscenePlayer );
REGISTER_RTTI_CLASS( CStorySceneChoice );
REGISTER_RTTI_CLASS( CStorySceneChoiceLine );
REGISTER_RTTI_CLASS( CStorySceneScriptLine );
REGISTER_RTTI_CLASS( CStorySceneBlockingElement );
REGISTER_RTTI_CLASS( CStoryScenePauseElement );
REGISTER_RTTI_STRUCT( StorySceneScriptParam );
REGISTER_RTTI_CLASS( CStorySceneGraph );
REGISTER_RTTI_CLASS( CStorySceneGraphBlock );
REGISTER_RTTI_CLASS( CStorySceneGraphSocket );
REGISTER_RTTI_CLASS( CStorySceneInputBlock );
REGISTER_RTTI_CLASS( CStorySceneLinkHubBlock );
REGISTER_RTTI_CLASS( CStorySceneOutputBlock );
REGISTER_RTTI_CLASS( CStorySceneSectionBlock );
REGISTER_RTTI_CLASS( CStorySceneCutsceneSection );
REGISTER_RTTI_CLASS( CStorySceneCutsceneSectionBlock );
REGISTER_RTTI_CLASS( CStorySceneFlowConditionBlock );
REGISTER_RTTI_CLASS( CStorySceneFlowSwitchBlock );
REGISTER_RTTI_CLASS( CStorySceneRandomBlock );
REGISTER_RTTI_CLASS( CStorySceneScript );
REGISTER_RTTI_CLASS( CStorySceneScriptingBlock );
REGISTER_RTTI_CLASS( CStorySceneSpawner );
REGISTER_RTTI_CLASS( CStorySceneVideoSection );
REGISTER_RTTI_CLASS( CStorySceneVideoBlock );
REGISTER_RTTI_CLASS( CStorySceneVideoElement );
REGISTER_RTTI_CLASS( CExtAnimDialogKeyPoseMarker );
REGISTER_RTTI_CLASS( CExtAnimDialogKeyPoseDuration );

REGISTER_RTTI_STRUCT( SVoicesetSlot );
REGISTER_RTTI_CLASS( CVoicesetParam );

REGISTER_RTTI_CLASS( CStorySceneComponent );
REGISTER_RTTI_STRUCT( CScenesTableEntry );
REGISTER_RTTI_CLASS( CSceneAreaComponent );
REGISTER_RTTI_CLASS( CStorySceneActorMap );
REGISTER_RTTI_CLASS( CStoryScenePlayer );
REGISTER_RTTI_CLASS( CStorySceneSystem );

REGISTER_RTTI_CLASS( ISceneChoiceMemo );
REGISTER_RTTI_CLASS( CFactsDBChoiceMemo );

// Story scene events
REGISTER_RTTI_STRUCT( SStorySceneCameraBlendKey );
REGISTER_RTTI_CLASS( CStorySceneEvent );
REGISTER_RTTI_CLASS( CStorySceneEventDuration );
REGISTER_RTTI_CLASS( CStorySceneEventCustomCamera );
REGISTER_RTTI_CLASS( CStorySceneEventAnimClip );
REGISTER_RTTI_CLASS( CStorySceneEventCameraAnim );
REGISTER_RTTI_STRUCT( SVoiceWeightCurve );
REGISTER_RTTI_CLASS( CStorySceneEventCurveAnimation );
REGISTER_RTTI_CLASS( CStorySceneEventWalk );
REGISTER_RTTI_CLASS( CStorySceneEventAnimation );
REGISTER_RTTI_CLASS( CStorySceneEventAdditiveAnimation );
REGISTER_RTTI_CLASS( CStorySceneEventOverrideAnimation );
REGISTER_RTTI_CLASS( CStorySceneEventCustomCameraInstance );
REGISTER_RTTI_CLASS( CStorySceneEventStartBlendToGameplayCamera );
REGISTER_RTTI_CLASS( CStorySceneEventDespawn );
REGISTER_RTTI_CLASS( CStorySceneEventVisibility );
REGISTER_RTTI_CLASS( CStorySceneEventLodOverride );
REGISTER_RTTI_CLASS( CStorySceneEventApplyAppearance );
REGISTER_RTTI_CLASS( CStorySceneEventMimicLod );
REGISTER_RTTI_CLASS( CStorySceneEventUseHiresShadows );
REGISTER_RTTI_CLASS( CStorySceneEventPropVisibility );
REGISTER_RTTI_CLASS( CStorySceneEventWeatherChange );
REGISTER_RTTI_CLASS( CStorySceneEventEnterActor );
REGISTER_RTTI_CLASS( CStorySceneEventExitActor );
REGISTER_RTTI_CLASS( CStorySceneEventFade );
REGISTER_RTTI_CLASS( CStorySceneEventLookAt );
REGISTER_RTTI_CLASS( CStorySceneEventLookAtDuration );
REGISTER_RTTI_STRUCT( SStorySceneEventLookAtBlinkSettings );
REGISTER_RTTI_CLASS( CStorySceneEventGameplayLookAt );
REGISTER_RTTI_CLASS( CStorySceneEventMimics );
REGISTER_RTTI_CLASS( CStorySceneEventMimicsAnim );
REGISTER_RTTI_CLASS( CStorySceneEventMimicsPose );
REGISTER_RTTI_CLASS( CStorySceneEventMimicsFilter );
REGISTER_RTTI_CLASS( CStorySceneEventRotate );
REGISTER_RTTI_CLASS( CStorySceneEventSound );
REGISTER_RTTI_CLASS( CStorySceneEventHitSound );
REGISTER_RTTI_CLASS( CStorySceneEventCamera );
REGISTER_RTTI_CLASS( CStorySceneEventGameplayCamera );
REGISTER_RTTI_CLASS( CStorySceneEventCameraBlend );
REGISTER_RTTI_CLASS( CStorySceneCameraBlendEvent );
REGISTER_RTTI_CLASS( CStorySceneEventOpenDoor );
REGISTER_RTTI_CLASS( CStorySceneEventCsCamera );
REGISTER_RTTI_CLASS( CStorySceneEventWorldEntityEffect );
REGISTER_RTTI_CLASS( CStorySceneEventReward );
REGISTER_RTTI_CLASS( CStorySceneEventSetupItemForSync );
REGISTER_RTTI_CLASS( CStorySceneEventTimelapse );
REGISTER_RTTI_CLASS( CStorySceneEventVideoOverlay );
REGISTER_RTTI_CLASS( CStorySceneEventHideScabbard );
REGISTER_RTTI_CLASS( CStorySceneEventCameraLight );
REGISTER_RTTI_STRUCT( SStorySceneCameraLightMod );
REGISTER_RTTI_CLASS( CStorySceneEventAttachPropToSlot)
//REGISTER_RTTI_STRUCT( SStorySceneBezierEventHandler );
//REGISTER_RTTI_STRUCT( SBezierHandlerKey );
REGISTER_RTTI_CLASS( CStorySceneEventBlend );
REGISTER_RTTI_CLASS( CStorySceneEventCurveBlend );
REGISTER_RTTI_CLASS( CStorySceneEventEnhancedCameraBlend );
REGISTER_RTTI_CLASS( CStorySceneEventChangePose );
REGISTER_RTTI_CLASS( CStorySceneEventOverridePlacement );
REGISTER_RTTI_CLASS( CStorySceneEventOverridePlacementDuration );
REGISTER_RTTI_CLASS( CStorySceneEventChangeActorGameState );
REGISTER_RTTI_STRUCT( SStorySceneEventGroupEntry );
REGISTER_RTTI_CLASS( CStorySceneEventGroup );
REGISTER_RTTI_CLASS( CStorySceneEventEquipItem );
REGISTER_RTTI_CLASS( CStorySceneMorphEvent );
REGISTER_RTTI_CLASS( CStorySceneDisablePhysicsClothEvent );
REGISTER_RTTI_CLASS( CStorySceneDanglesShakeEvent );
REGISTER_RTTI_CLASS( CStorySceneDisableDangleEvent );
REGISTER_RTTI_CLASS( CStorySceneResetClothAndDanglesEvent );
REGISTER_RTTI_CLASS( CStorySceneActorEffectEvent );
REGISTER_RTTI_CLASS( CStoryScenePropEffectEvent );
REGISTER_RTTI_CLASS( CStorySceneActorEffectEventDuration );
REGISTER_RTTI_CLASS( CStorySceneEventDialogLine );
REGISTER_RTTI_CLASS( CStorySceneEventScenePropPlacement );
REGISTER_RTTI_CLASS( CStorySceneEventWorldPropPlacement );
REGISTER_RTTI_CLASS( CStorySceneEventPoseKey );
REGISTER_RTTI_STRUCT( SSSBoneTransform );
REGISTER_RTTI_STRUCT( SSSTrackTransform );
REGISTER_RTTI_CLASS( CStorySceneEventLightProperties );
REGISTER_RTTI_CLASS( CStorySceneEventInterpolation );
REGISTER_RTTI_CLASS( CStorySceneEventCameraInterpolation );
REGISTER_RTTI_CLASS( CStorySceneEventCameraInterpolationKey );
REGISTER_RTTI_CLASS( CStorySceneEventPlacementInterpolation );
REGISTER_RTTI_CLASS( CStorySceneEventPlacementInterpolationKey );
REGISTER_RTTI_CLASS( CStorySceneEventLightPropertiesInterpolation );
REGISTER_RTTI_CLASS( CStorySceneEventLightPropertiesInterpolationKey );
REGISTER_RTTI_CLASS( CStorySceneEventClothDisablingInterpolation );
REGISTER_RTTI_CLASS( CStorySceneEventClothDisablingInterpolationKey );
REGISTER_RTTI_CLASS( CStorySceneEventMorphInterpolation );
REGISTER_RTTI_CLASS( CStorySceneEventMorphInterpolationKey );
REGISTER_RTTI_CLASS( CStorySceneEventDangleDisablingInterpolation );
REGISTER_RTTI_CLASS( CStorySceneEventDangleDisablingInterpolationKey );
REGISTER_RTTI_CLASS( CStorySceneEventPropPlacementInterpolation );
REGISTER_RTTI_CLASS( CStorySceneEventPropPlacementInterpolationKey );
REGISTER_RTTI_CLASS( CStorySceneDanglesShakeEventInterpolation );
REGISTER_RTTI_CLASS( CStorySceneDanglesShakeEventInterpolationKey );
REGISTER_RTTI_CLASS( CStorySceneEventCameraLightInterpolation );
REGISTER_RTTI_CLASS( CStorySceneEventCameraLightInterpolationKey );
REGISTER_RTTI_CLASS( CStorySceneEventModifyEnv );
REGISTER_RTTI_CLASS( CStorySceneEventDebugComment );
REGISTER_RTTI_STRUCT( SStorySceneSpotLightProperties );
REGISTER_RTTI_STRUCT( SStorySceneLightDimmerProperties );
REGISTER_RTTI_STRUCT( SStorySceneGameplayActionCallbackInfo );


// Quests
REGISTER_RTTI_CLASS( CQuest );
REGISTER_RTTI_CLASS( CQuestsSystem );
REGISTER_RTTI_CLASS( CQuestFactory );
REGISTER_RTTI_CLASS( CQuestPhase );
REGISTER_RTTI_CLASS( CQuestPhaseFactory );
REGISTER_RTTI_CLASS( CQuestGraph );
REGISTER_RTTI_CLASS( CQuestGraphInstance );
REGISTER_RTTI_CLASS( CQuestGraphSocket );
REGISTER_RTTI_CLASS( CQuestGraphBlock );
REGISTER_RTTI_STRUCT( SQuestThreadSuspensionData );
REGISTER_RTTI_CLASS( CQuestCutControlGraphSocket );
REGISTER_RTTI_CLASS( CQuestConditionBlock );
REGISTER_RTTI_CLASS( CQuestCutControlBlock );
REGISTER_RTTI_CLASS( CQuestPauseConditionBlock );
REGISTER_RTTI_CLASS( CQuestSceneBlock );
REGISTER_RTTI_CLASS( CQuestStartBlock );
REGISTER_RTTI_CLASS( CQuestEndBlock );
REGISTER_RTTI_CLASS( CQuestVariedInputsBlock );
REGISTER_RTTI_CLASS( CQuestAndBlock );
REGISTER_RTTI_CLASS( CQuestXorBlock );
REGISTER_RTTI_CLASS( CQuestScopeBlock );
REGISTER_RTTI_CLASS( CQuestPhaseBlock );
REGISTER_RTTI_CLASS( CQuestPhaseInputBlock );
REGISTER_RTTI_CLASS( CQuestPhaseOutputBlock );
REGISTER_RTTI_CLASS( CQuestContextDialogBlock );
REGISTER_RTTI_CLASS( CQuestFactsDBChangingBlock );
REGISTER_RTTI_CLASS( CQuestStoryPhaseSetterBlock );
REGISTER_RTTI_CLASS( CQuestFastForwardCommunitiesBlock );
REGISTER_RTTI_CLASS( CQuestEncounterPhaseBlock );
REGISTER_RTTI_CLASS( CQuestEncounterManagerBlock );
REGISTER_RTTI_CLASS( CQuestEncounterActivator );
REGISTER_RTTI_CLASS( CQuestEncounterFullRespawn );
REGISTER_RTTI_CLASS( CQuestCheckpointBlock );
REGISTER_RTTI_CLASS( IQuestCondition );
REGISTER_RTTI_CLASS( CQuestLogicOperationCondition );
REGISTER_RTTI_CLASS( CQuestTriggerCondition );
REGISTER_RTTI_CLASS( CQuestInsideTriggerCondition );
REGISTER_RTTI_CLASS( CQuestEnterTriggerCondition );
REGISTER_RTTI_CLASS( CQuestFactsDBConditionBase );
REGISTER_RTTI_CLASS( CQuestFactsDBCondition );
REGISTER_RTTI_CLASS( CQuestGraphMinigameBlock );
REGISTER_RTTI_CLASS( CQuestFactsDBForbiddenCondition );
REGISTER_RTTI_CLASS( IActorConditionType );
REGISTER_RTTI_CLASS( CQuestActorCondition );
REGISTER_RTTI_CLASS( IGameplayEntConditionType );
REGISTER_RTTI_CLASS( CQuestGameplayEntCondition );
REGISTER_RTTI_CLASS( CQuestManyActorsCondition );
REGISTER_RTTI_CLASS( CQuestNoLivingActorsCondition );
REGISTER_RTTI_CLASS( CQCIsAlive );
REGISTER_RTTI_CLASS( CQCHasAbility );
REGISTER_RTTI_CLASS( CQCHasItem );
REGISTER_RTTI_CLASS( CQCDistanceTo );
REGISTER_RTTI_CLASS( CQCHasItemGE );
REGISTER_RTTI_CLASS( CQCAnimationState );
REGISTER_RTTI_CLASS( CQuestLoadingScreenCondition );
REGISTER_RTTI_CLASS( CQuestJournalStatusCondition );
REGISTER_RTTI_CLASS( CQuestTimeCondition );
REGISTER_RTTI_CLASS( CQuestTimePeriodCondition );
REGISTER_RTTI_CLASS( CQuestWaitForCondition );
REGISTER_RTTI_CLASS( CQuestInteractionCondition );
REGISTER_RTTI_CLASS( CQuestFightCondition );
REGISTER_RTTI_CLASS( CQuestReactionCondition );
REGISTER_RTTI_CLASS( CQuestEngineTimeWaitForCondition );
REGISTER_RTTI_CLASS( CQuestThread );
REGISTER_RTTI_CLASS( CQuestInteractionDialogBlock );
REGISTER_RTTI_CLASS( IQuestSpawnsetAction );
REGISTER_RTTI_CLASS( CActivateStoryPhase );
REGISTER_RTTI_CLASS( CDeactivateSpawnset );
REGISTER_RTTI_CLASS( CQuestTimeManagementBlock );
REGISTER_RTTI_CLASS( IQuestTimeFunction );
REGISTER_RTTI_CLASS( CPauseTimeFunction );
REGISTER_RTTI_CLASS( CSetTimeFunction );
REGISTER_RTTI_CLASS( CShiftTimeFunction );
REGISTER_RTTI_CLASS( CQuestTeleportBlock );
REGISTER_RTTI_CLASS( CQuestLookAtBlock );
REGISTER_RTTI_CLASS( CQuestCutsceneCondition );
REGISTER_RTTI_CLASS( CQuestTagsPresenceCondition );
REGISTER_RTTI_CLASS( CQuestScriptBlock );
REGISTER_RTTI_STRUCT( QuestScriptParam );
REGISTER_RTTI_CLASS( CQuestLayersHiderBlock );
REGISTER_RTTI_CLASS( CQuestInterestPointBlock );
REGISTER_RTTI_STRUCT( SScriptedActionSerializedState );
REGISTER_RTTI_CLASS( CBaseQuestScriptedActionsBlock );
REGISTER_RTTI_CLASS( CQuestScriptedActionsBlock );
REGISTER_RTTI_CLASS( CQuestPokeScriptedActionsBlock );
REGISTER_RTTI_STRUCT( SScriptedActionData );
REGISTER_RTTI_STRUCT( SBehTreeExternalListenerPtr );
REGISTER_RTTI_CLASS( CAIQuestScriptedActionsTree );
REGISTER_RTTI_CLASS( CQuestScriptedDialogBlock );
REGISTER_RTTI_CLASS( CQuestScriptedCondition );
REGISTER_RTTI_CLASS( IActorLatentAction );
REGISTER_RTTI_CLASS( IPresetActorLatentAction );
REGISTER_RTTI_CLASS( CCustomBehTreeActorLatentAction );
REGISTER_RTTI_STRUCT( SBlockDesc );
REGISTER_RTTI_STRUCT( SCachedConnections );
REGISTER_RTTI_CLASS( CQuestCameraBlock );
REGISTER_RTTI_CLASS( CQuestStaticCameraRunBlock );
REGISTER_RTTI_CLASS( CQuestStaticCameraStopBlock );
REGISTER_RTTI_CLASS( CQuestStaticCameraSwitchBlock );
REGISTER_RTTI_CLASS( CQuestStaticCameraSequenceBlock );
REGISTER_RTTI_CLASS( CQuestDeletionMarkerBlock );
REGISTER_RTTI_CLASS( CQCActorScriptedCondition );
REGISTER_RTTI_CLASS( CQuestLockNPCBlock );
REGISTER_RTTI_CLASS( CQuestUnlockNPCBlock );
REGISTER_RTTI_CLASS( CVirtualContainerEntity );
REGISTER_RTTI_CLASS( CQuestCharacterCustomizerBlock );
REGISTER_RTTI_CLASS( ICharacterCustomizationOperation );
REGISTER_RTTI_CLASS( CCCOpClearInventory );
REGISTER_RTTI_CLASS( CCCOpVirtualContainerOp );
REGISTER_RTTI_CLASS( CCCOpItemsToVirtualContainer );
REGISTER_RTTI_CLASS( CCCOpItemsFromVirtualContainer );
REGISTER_RTTI_CLASS( CCCOpCustomizeInventory );
REGISTER_RTTI_CLASS( CCCOpItemsRemoveMatchingVirtualContainer );
REGISTER_RTTI_CLASS( CCCOpItemsRemoveMatchingTemplate );
REGISTER_RTTI_CLASS( CCCOpPreserveVirtualContainerContents );
REGISTER_RTTI_CLASS( CCCOpRestoreVirtualContainerContents );
REGISTER_RTTI_CLASS( CQuestRealtimeDelayCondition );
REGISTER_RTTI_CLASS( CQuestHiResRealtimeDelayCondition );
REGISTER_RTTI_CLASS( CQuestInCombatCondition );
REGISTER_RTTI_CLASS( CQuestStateChangeRequestResetBlock );
REGISTER_RTTI_CLASS( CQCItemQuantity );
REGISTER_RTTI_CLASS( CQCAttitude );
REGISTER_RTTI_CLASS( CQuestCameraFocusCondition );
REGISTER_RTTI_CLASS( CQuestInputCondition );
REGISTER_RTTI_CLASS( CQuestColorblindModeCondition );
REGISTER_RTTI_CLASS( CQuestRewardBlock );
REGISTER_RTTI_CLASS( CExtAnimCutsceneQuestEvent );
REGISTER_RTTI_CLASS( CExtAnimCutsceneResetClothAndDangleEvent );
REGISTER_RTTI_CLASS( CExtAnimCutsceneDisableClothEvent );
REGISTER_RTTI_CLASS( CQuestScenePrepareBlock );
REGISTER_RTTI_CLASS( CCounterUtilityGraphBlock );
REGISTER_RTTI_CLASS( CQuestRandomBlock );
REGISTER_RTTI_CLASS( CQuestContentActivatorBlock );
REGISTER_RTTI_CLASS( CQuestEntityMotionBlock );
REGISTER_RTTI_CLASS( CQuestTestBlock );
REGISTER_RTTI_CLASS( CQuestLogStateBlock );
REGISTER_RTTI_CLASS( CQuestMemoryDumpBlock );
REGISTER_RTTI_CLASS( CQuestPerformGCBlock );
REGISTER_RTTI_CLASS( CQuestWaitForTickBlock );

//Inventory
REGISTER_RTTI_CLASS( CItemEntity );
REGISTER_RTTI_STRUCT( SItemUniqueId );
REGISTER_RTTI_STRUCT( SInventoryItemUIData );
REGISTER_RTTI_STRUCT( SAbilityAttributeValue );
REGISTER_RTTI_STRUCT( SAbilityAttribute );
REGISTER_RTTI_STRUCT( SAbility );
REGISTER_RTTI_STRUCT( SInventoryItem );
REGISTER_RTTI_CLASS( CInventoryComponent );
REGISTER_RTTI_CLASS( CInventoryDefinition );
REGISTER_RTTI_CLASS( CInventoryDefinitionEntry );
REGISTER_RTTI_CLASS( IInventoryInitializer );
REGISTER_RTTI_CLASS( CInventoryInitializerRandom );
REGISTER_RTTI_CLASS( CInventoryInitializerUniform );
REGISTER_RTTI_STRUCT( SCustomNodeAttribute );
REGISTER_RTTI_STRUCT( SCustomNode );
REGISTER_RTTI_STRUCT( SItemSet );
REGISTER_RTTI_STRUCT( SItemStat );
REGISTER_RTTI_STRUCT( SItemDamageCurve );
REGISTER_RTTI_STRUCT( SItemTagModifier );
REGISTER_RTTI_STRUCT( SItemParts );
REGISTER_RTTI_STRUCT( SItemChangedData );
REGISTER_RTTI_STRUCT( SIngredientCategory );
REGISTER_RTTI_STRUCT( SIngredientCategoryElement );
REGISTER_RTTI_CLASS( CItemAnimationSyncToken );
REGISTER_RTTI_CLASS( CLootDefinitionBase );
REGISTER_RTTI_CLASS( CHeadManagerComponent );
REGISTER_RTTI_STRUCT( SItemNameProperty );


// Item events
REGISTER_RTTI_CLASS( CExtAnimItemEvent );
REGISTER_RTTI_CLASS( CExtAnimItemEffectEvent );
REGISTER_RTTI_CLASS( CExtAnimItemEffectDurationEvent );
REGISTER_RTTI_CLASS( CExtAnimLookAtEvent );
REGISTER_RTTI_CLASS( CExtAnimItemSyncEvent );
REGISTER_RTTI_CLASS( CExtAnimItemSyncDurationEvent );
REGISTER_RTTI_CLASS( CExtAnimItemSyncWithCorrectionEvent );
REGISTER_RTTI_CLASS( CExtAnimItemAnimationEvent );
REGISTER_RTTI_CLASS( CExtAnimItemBehaviorEvent );
REGISTER_RTTI_CLASS( CExtAnimDropItemEvent );
REGISTER_RTTI_CLASS( CExtAnimReattachItemEvent );

// Community system
REGISTER_RTTI_STRUCT( SJobTreeExecutionContext );
REGISTER_RTTI_STRUCT( SJobTreeSettings );
REGISTER_RTTI_CLASS( CJobTree );
REGISTER_RTTI_CLASS( CJobTreeFactory );
REGISTER_RTTI_CLASS( CJobTreeNode );
REGISTER_RTTI_CLASS( CJobActionBase );
REGISTER_RTTI_CLASS( CJobAction );
REGISTER_RTTI_CLASS( CJobForceOutAction );
REGISTER_RTTI_CLASS( CActionPoint );
REGISTER_RTTI_CLASS( CCommunity );
REGISTER_RTTI_CLASS( CCommunityFactory );
REGISTER_RTTI_CLASS( CCommunityArea );
REGISTER_RTTI_CLASS( CCommunityAreaTypeDefault );
REGISTER_RTTI_CLASS( CCommunityAreaTypeSpawnRadius );
REGISTER_RTTI_CLASS( CCommunityAreaType );
REGISTER_RTTI_CLASS( CCommunityInitializers );
REGISTER_RTTI_CLASS( CCommunitySystem );
REGISTER_RTTI_STRUCT( CSEntitiesEntry );
REGISTER_RTTI_STRUCT( CSStoryPhaseSpawnTimetableEntry );
REGISTER_RTTI_STRUCT( CSStoryPhaseEntry );
REGISTER_RTTI_STRUCT( CSTableEntry );
REGISTER_RTTI_STRUCT( CSLayerName );
REGISTER_RTTI_STRUCT( CSStoryPhaseTimetableACategoriesEntry );
REGISTER_RTTI_STRUCT( CSStoryPhaseTimetableActionEntry );
REGISTER_RTTI_STRUCT( CSStoryPhaseTimetableACategoriesTimetableEntry );
REGISTER_RTTI_STRUCT( CSStoryPhaseTimetableEntry );
REGISTER_RTTI_STRUCT( CSSceneTimetableScenesEntry );
REGISTER_RTTI_STRUCT( CSSceneTimetableEntry );
REGISTER_RTTI_STRUCT( CSSceneTableEntry );
REGISTER_RTTI_STRUCT( CSSpawnType );
REGISTER_RTTI_STRUCT( CSStoryPhaseNames );
REGISTER_RTTI_CLASS( CCommunitySpawnStrategy );
REGISTER_RTTI_CLASS( IEngineSpawnStrategy );
REGISTER_RTTI_CLASS( COldCommunitySpawnStrategy );
REGISTER_RTTI_STRUCT( SActionPointId );
REGISTER_RTTI_CLASS( CActionPointComponent );
REGISTER_RTTI_CLASS( CActionPointManager );

// Entities and components
REGISTER_RTTI_CLASS( CSelfUpdatingComponent )
REGISTER_RTTI_CLASS( CEntityUpdaterComponent )
REGISTER_RTTI_CLASS( CJobBreakerComponent )


// Action point selectors
REGISTER_RTTI_CLASS( CActionPointSelector );
REGISTER_RTTI_CLASS( CCommunityActionPointSelector );
REGISTER_RTTI_CLASS( CTimetableActionPointSelector );
REGISTER_RTTI_CLASS( CSimpleActionPointSelector );
REGISTER_RTTI_CLASS( CWanderActionPointSelector );
REGISTER_RTTI_STRUCT( SEncounterActionPointSelectorPair );

// Character database
REGISTER_RTTI_CLASS( CCharacter );
REGISTER_RTTI_CLASS( CCharacterResource );

// Journal
REGISTER_RTTI_CLASS( CJournalResource );
REGISTER_RTTI_CLASS( CJournalInitialEntriesResource );
REGISTER_RTTI_CLASS( CJournalPath );
REGISTER_RTTI_CLASS( CJournalManager );
REGISTER_RTTI_CLASS( CJournalBase );
REGISTER_RTTI_CLASS( CJournalLink );
REGISTER_RTTI_CLASS( CJournalRoot );
REGISTER_RTTI_STRUCT( SJournalEntryStatus );
REGISTER_RTTI_STRUCT( SJournalEvent );
REGISTER_RTTI_STRUCT( SJournalStatusEvent );

//Rewards
REGISTER_RTTI_STRUCT( SItemReward );
REGISTER_RTTI_STRUCT( SReward );
REGISTER_RTTI_CLASS( CRewardGroup );
REGISTER_RTTI_CLASS( CRewardGroupFactory );

// Bg npcs
REGISTER_RTTI_CLASS( CBgNpc );
REGISTER_RTTI_CLASS( CBgNpcTriggerComponent );
REGISTER_RTTI_CLASS( CBgNpcItemComponent );
REGISTER_RTTI_CLASS( CBgInteractionComponent );
REGISTER_RTTI_CLASS( CBgRootComponent );
REGISTER_RTTI_CLASS( CBgMeshComponent );
REGISTER_RTTI_CLASS( IBgNpcTriggerAction );
REGISTER_RTTI_CLASS( CBgNpcTriggerActionTalk );
REGISTER_RTTI_CLASS( CBgNpcTriggerActionLookAt );
REGISTER_RTTI_CLASS( CBgNpcTriggerActionSwordReaction );
REGISTER_RTTI_STRUCT( SBgNpcJobTree );

// Animations
REGISTER_RTTI_CLASS( CAnimationManualSlotSyncInstance );
REGISTER_RTTI_STRUCT( SAnimationSequencePartDefinition );
REGISTER_RTTI_STRUCT( SAnimationSequenceDefinition );

REGISTER_RTTI_STRUCT( SBehaviorConstraintNodeFloorIKCommonData );
REGISTER_RTTI_STRUCT( SBehaviorConstraintNodeFloorIKVerticalBoneData );
REGISTER_RTTI_STRUCT( SBehaviorConstraintNodeFloorIKMaintainLookBoneData );
REGISTER_RTTI_STRUCT( SBehaviorConstraintNodeFloorIKLegsData );
REGISTER_RTTI_STRUCT( SBehaviorConstraintNodeFloorIKCommon );
REGISTER_RTTI_STRUCT( SBehaviorConstraintNodeFloorIKVerticalBone );
REGISTER_RTTI_STRUCT( SBehaviorConstraintNodeFloorIKMaintainLookBone );
REGISTER_RTTI_STRUCT( SBehaviorConstraintNodeFloorIKLegs );
REGISTER_RTTI_STRUCT( SBehaviorConstraintNodeFloorIKCachedTrace );
REGISTER_RTTI_STRUCT( SBehaviorConstraintNodeFloorIKLeg );
REGISTER_RTTI_STRUCT( SBehaviorConstraintNodeFloorIKWeightHandler );
REGISTER_RTTI_STRUCT( SBehaviorConstraintNodeFloorIKLegsIKWeightHandler );
REGISTER_RTTI_STRUCT( SBehaviorConstraintNodeFloorIKFrontBackWeightHandler );
REGISTER_RTTI_STRUCT( SBehaviorConstraintNodeFloorIKDebugTrace );
REGISTER_RTTI_CLASS( CBehaviorConstraintNodeFloorIKBase );
REGISTER_RTTI_CLASS( CBehaviorConstraintNodeFloorIK );
REGISTER_RTTI_CLASS( CBehaviorConstraintNodeFloorIKHandsOnly );
REGISTER_RTTI_CLASS( CBehaviorConstraintNodeFloorIKBipedLong );
REGISTER_RTTI_CLASS( CBehaviorConstraintNodeFloorIKQuadruped );
REGISTER_RTTI_CLASS( CBehaviorConstraintNodeFloorIKSixLegs );
REGISTER_RTTI_CLASS( CBehaviorConstraintRiderInSaddle );
REGISTER_RTTI_STRUCT( SBehaviorConstraintUprightSpineBonesData );
REGISTER_RTTI_STRUCT( SBehaviorConstraintUprightSpineBones );
REGISTER_RTTI_CLASS( CBehaviorConstraintUprightSpine );
REGISTER_RTTI_CLASS( CBehaviorConstraintApplyOffset );
REGISTER_RTTI_STRUCT( SBehaviorConstraintPutLegIntoStirrupData );
REGISTER_RTTI_STRUCT( SBehaviorConstraintPutLegIntoStirrup );
REGISTER_RTTI_CLASS( CBehaviorConstraintPutLegsIntoStirrups );
REGISTER_RTTI_STRUCT( SBehaviorConstraintTargetWeightHandler );
REGISTER_RTTI_STRUCT( SBehaviorConstraintPullStirrupToLegData );
REGISTER_RTTI_STRUCT( SBehaviorConstraintPullStirrupToLeg );
REGISTER_RTTI_CLASS( CBehaviorConstraintPullStirrupsToLegs );
REGISTER_RTTI_STRUCT( SBehaviorConstraintStirrupsCommmonData );
REGISTER_RTTI_STRUCT( SBehaviorConstraintStirrupsCommmon );
REGISTER_RTTI_STRUCT( SBehaviorConstraintStirrupData );
REGISTER_RTTI_STRUCT( SBehaviorConstraintStirrup );
REGISTER_RTTI_CLASS( CBehaviorConstraintStirrups );
REGISTER_RTTI_STRUCT( SBehaviorConstraintPullReinToHandData );
REGISTER_RTTI_STRUCT( SBehaviorConstraintPullReinToHand );
REGISTER_RTTI_CLASS( CBehaviorConstraintPullReinsToHands );
REGISTER_RTTI_CLASS( CBehaviorGraphAimingWithIKNode );
REGISTER_RTTI_CLASS( CBehaviorConstraintMoveHandsByOffset );

REGISTER_RTTI_CLASS( CLookAtStaticParam );
REGISTER_RTTI_CLASS( CBehaviorGraphPoseConstraintPoseLookAtNode );
REGISTER_RTTI_CLASS( CBehaviorGraphPoseConstraintPoseCurveLookAtNode );
REGISTER_RTTI_STRUCT( SPoseLookAtSegmentData );
REGISTER_RTTI_STRUCT( SPoseLookAtSegment );
REGISTER_RTTI_STRUCT( SPoseLookAtSegmentDampData );
REGISTER_RTTI_CLASS( IBehaviorPoseConstraintPoseLookAtModifier );
REGISTER_RTTI_CLASS( CBPCPoseLookAtCurveTrajModifier );
REGISTER_RTTI_CLASS( CBehaviorGraphGameplayAdditiveNode );
REGISTER_RTTI_STRUCT( SGameplayAdditiveLevel );
REGISTER_RTTI_STRUCT( SGameplayAdditiveAnimation );
REGISTER_RTTI_STRUCT( SGameplayAdditiveAnimRuntimeData );
REGISTER_RTTI_CLASS( CBehaviorGraphGameplaySoundEventsNode );
REGISTER_RTTI_CLASS( CBehaviorGraphRandomAnimTimeNode );
REGISTER_RTTI_CLASS( CBehaviorGraphMorphTrackNode );
REGISTER_RTTI_CLASS( CBehaviorGraphScriptStateNode );
REGISTER_RTTI_CLASS( CBehaviorGraphScriptComponentStateNode );
REGISTER_RTTI_CLASS( CBehaviorGraphScriptStateReportingNode )
REGISTER_RTTI_CLASS( CBehaviorGraphDirectionalMovementNode );
REGISTER_RTTI_CLASS( CBehaviorGraphDirectionalMovementStartNode );
REGISTER_RTTI_CLASS( CBehaviorGraphDirectionalMovementStopNode );
REGISTER_RTTI_CLASS( CChangeMovementDirectionTransitionCondition );
REGISTER_RTTI_CLASS( CChangeFacingDirectionTransitionCondition );
REGISTER_RTTI_CLASS( CIsMovingForwardTransitionCondition );
REGISTER_RTTI_CLASS( CBehaviorGraphStateTransitionFinalStepNode );
REGISTER_RTTI_CLASS( CBehaviorGraphChangeDirectionNode );
REGISTER_RTTI_CLASS( CBehaviorGraphFillMovementVariablesUsingSteeringNode );
REGISTER_RTTI_CLASS( CBehaviorGraphRagdollNode );
REGISTER_RTTI_CLASS( CBehaviorGraphAnimatedRagdollNode );
REGISTER_RTTI_STRUCT( SBehaviorGraphAnimatedRagdollDirDefinition );
REGISTER_RTTI_STRUCT( SBehaviorGraphAnimatedRagdollDirReplacement );
REGISTER_RTTI_CLASS( CBehaviorGraphMaintainVelocityNode );
REGISTER_RTTI_CLASS( CBehaviorGraphStoreBoneNode );
REGISTER_RTTI_CLASS( CBehaviorGraphStoreAnimEventNode );
REGISTER_RTTI_CLASS( CBehaviorGraphRestoreAnimEventNode );
REGISTER_RTTI_CLASS( CBehaviorGraphOnSlopeMovementNode );
REGISTER_RTTI_CLASS( CBehaviorGraphStoreSyncInfoNode );
REGISTER_RTTI_CLASS( CBehaviorGraphRestoreSyncInfoNode );
REGISTER_RTTI_CLASS( CBehaviorGraphConvertSyncInfoIntoCyclesNode );
REGISTER_RTTI_CLASS( CBehaviorGraphFootStepDetectorNode );
REGISTER_RTTI_STRUCT( SFootDetectionBoneInfo );

// Animation events
REGISTER_RTTI_CLASS( CExtAnimEffectDurationEvent );
REGISTER_RTTI_CLASS( CExtAnimEffectEvent );
REGISTER_RTTI_CLASS( CExtAnimGameplayMimicEvent );
REGISTER_RTTI_CLASS( CExtAnimLocationAdjustmentEvent );
REGISTER_RTTI_CLASS( CExtAnimMaterialBasedFxEvent );
REGISTER_RTTI_CLASS( CExtAnimOnSlopeEvent );
REGISTER_RTTI_CLASS( CExtAnimRotationAdjustmentEvent );
REGISTER_RTTI_CLASS( CExtAnimRotationAdjustmentLocationBasedEvent );

// Camera
REGISTER_RTTI_CLASS( CCameraEffectTrigger );

// Vehicles
REGISTER_RTTI_CLASS( CVehicleComponent );
REGISTER_RTTI_CLASS( CAdvancedVehicleComponent );
REGISTER_RTTI_CLASS( CSeatComponent );
REGISTER_RTTI_CLASS( CPilotComponent );

// Boat
REGISTER_RTTI_CLASS( CBoatBodyComponent );
REGISTER_RTTI_CLASS( CBoatComponent );
REGISTER_RTTI_CLASS( CBoatDestructionComponent );
REGISTER_RTTI_STRUCT( SBoatDestructionVolume );

// Exploration
REGISTER_RTTI_CLASS( CExplorationComponent );
REGISTER_RTTI_CLASS( CExpSyncEvent );
REGISTER_RTTI_CLASS( CExpSlideEvent );
REGISTER_RTTI_STRUCT( SExplorationQueryContext );
REGISTER_RTTI_STRUCT( SExplorationQueryToken );
REGISTER_RTTI_CLASS( CScriptedExplorationTraverser );
REGISTER_RTTI_CLASS( CFoundExplorationComponent );
REGISTER_RTTI_CLASS( CCookedExplorations );

// Journal
REGISTER_RTTI_CLASS( CJournalBlock );
REGISTER_RTTI_CLASS( CJournalChildBase );

REGISTER_RTTI_CLASS( CJournalContainerEntry );
REGISTER_RTTI_CLASS( CJournalContainer );

// Misc
REGISTER_RTTI_CLASS( CNodesBinaryStorage );
REGISTER_RTTI_CLASS( CSpawnPointComponent );
REGISTER_RTTI_STRUCT( SPartyWaypointHandle );
REGISTER_RTTI_CLASS( CPartySpawnPointComponent );
REGISTER_RTTI_STRUCT( SAIMinigameParams );
REGISTER_RTTI_CLASS( CAIMinigameParamsWristWrestling );
REGISTER_RTTI_CLASS( CExtAnimProjectileEvent );
#if 0
REGISTER_RTTI_CLASS( CCameraAreaComponent );
#endif
REGISTER_RTTI_CLASS( CAIProfile );
#ifndef NO_EDITOR
REGISTER_RTTI_CLASS( CAIWizardTemplateParam );
#endif
REGISTER_RTTI_CLASS( CAIPresetsTemplateParam );
REGISTER_RTTI_CLASS( CAITemplateParam );
REGISTER_RTTI_CLASS( CAIBaseTreeTemplateParam );
REGISTER_RTTI_CLASS( CAIReaction );
REGISTER_RTTI_STRUCT( SAIReactionRange );
REGISTER_RTTI_STRUCT( SAIReactionFactTest );
REGISTER_RTTI_CLASS( CAIAttackRange );
REGISTER_RTTI_CLASS( CSphereAttackRange );
REGISTER_RTTI_CLASS( CCylinderAttackRange );
REGISTER_RTTI_CLASS( CConeAttackRange );
REGISTER_RTTI_CLASS( CBoxAttackRange );
REGISTER_RTTI_CLASS( CAISenseParams );
REGISTER_RTTI_CLASS( CCharacterStats );
REGISTER_RTTI_CLASS( CCCOpScript );
REGISTER_RTTI_STRUCT( SNavigationCollectCollisionInCircleData );
REGISTER_RTTI_CLASS( CNavigationReachabilityQueryInterface );
REGISTER_RTTI_STRUCT( SAIAttitudeDummy );
REGISTER_RTTI_CLASS( IEntityStateChangeRequest );
REGISTER_RTTI_CLASS( CScriptedEntityStateChangeRequest );
REGISTER_RTTI_CLASS( CEnableDeniedAreaRequest );
REGISTER_RTTI_CLASS( CPlaySoundOnActorRequest );
REGISTER_RTTI_CLASS( CEquipmentDefinition );
REGISTER_RTTI_CLASS( CEquipmentDefinitionEntry );
REGISTER_RTTI_CLASS( IEquipmentInitializer );
REGISTER_RTTI_CLASS( CEquipmentInitializerRandom );
REGISTER_RTTI_CLASS( CEquipmentInitializerUniform );
REGISTER_RTTI_CLASS( CTemplateLoadRequest );
REGISTER_RTTI_CLASS( CWaterDebug );
REGISTER_RTTI_CLASS( CVisualDebug_MovementTrajectory );
#ifdef USE_UMBRA
REGISTER_RTTI_STRUCT( COcclusionQueryPtr );
REGISTER_RTTI_STRUCT( SOcclusionSPQuery );
#endif

// door system
REGISTER_RTTI_CLASS( W3LockableEntity );
REGISTER_RTTI_CLASS( IDoorAttachment );
REGISTER_RTTI_CLASS( CDoorAttachment_AngleAnimation );
REGISTER_RTTI_CLASS( CDoorAttachment_PropertyAnimation );
REGISTER_RTTI_CLASS( CDoorAttachment_GameplayPush );
REGISTER_RTTI_STRUCT( SDoorSoundsEvents );
REGISTER_RTTI_CLASS( CDoorComponent );

REGISTER_RTTI_CLASS( W3Container );

// Gameplay params
REGISTER_RTTI_CLASS( CCharacterStatsParam );
REGISTER_RTTI_CLASS( CAutoEffectsParam );
REGISTER_RTTI_CLASS( CAttackRangeParam );
REGISTER_RTTI_CLASS( CAttackableArea );
REGISTER_RTTI_CLASS( CPlayerTargetPriority );
REGISTER_RTTI_CLASS( CAlternativeDisplayName );
REGISTER_RTTI_CLASS( CEdEntitySetupListParam );
REGISTER_RTTI_CLASS( IEdEntitySetupEffector );
REGISTER_RTTI_CLASS( CEdSpawnEntitySetupEffector );
REGISTER_RTTI_CLASS( CBloodTrailEffect );


// Extended effects
REGISTER_RTTI_CLASS( CFXTrackItemPlayPropertyAnim );
REGISTER_RTTI_CLASS( CFXTrackItemPlayItemEffect );

// Sound
REGISTER_RTTI_CLASS( CExtAnimSoundEvent );
REGISTER_RTTI_CLASS( CExtAnimFootstepEvent );
REGISTER_RTTI_CLASS( CStandPhysicalMaterialAreaComponent );

REGISTER_RTTI_CLASS( CExtForcedLogicalFootstepAnimEvent );

// New GUI
REGISTER_RTTI_CLASS( CGuiScenePlayer );
REGISTER_RTTI_CLASS( CGuiConfigResource );
REGISTER_RTTI_CLASS( CGuiConfigResourceFactory );
REGISTER_RTTI_STRUCT( SMenuDescription );
REGISTER_RTTI_STRUCT( SHudDescription );
REGISTER_RTTI_STRUCT( SPopupDescription );
REGISTER_RTTI_STRUCT( SGuiSceneDescription );
REGISTER_RTTI_CLASS( CHudResourceFactory );
REGISTER_RTTI_CLASS( CMenuResourceFactory );
REGISTER_RTTI_CLASS( CPopupResourceFactory );
REGISTER_RTTI_CLASS( CFlashProxyComponent );

REGISTER_RTTI_STRUCT( SGuiEnhancementInfo );

REGISTER_RTTI_CLASS( CGuiObject );
REGISTER_RTTI_CLASS( IGuiResource );
REGISTER_RTTI_CLASS( IGuiResourceBlock );
REGISTER_RTTI_CLASS( CHudResource );
REGISTER_RTTI_CLASS( CHudModuleResourceBlock );
REGISTER_RTTI_CLASS( CMenuResource );
REGISTER_RTTI_CLASS( CHud );
REGISTER_RTTI_CLASS( CHudModule );
REGISTER_RTTI_CLASS( CMenu );
REGISTER_RTTI_CLASS( CPopupResource );
REGISTER_RTTI_CLASS( CPopup );

REGISTER_RTTI_CLASS( IMenuTimeParam );
REGISTER_RTTI_CLASS( IMenuDisplayParam );
REGISTER_RTTI_CLASS( IMenuFlashParam );
REGISTER_RTTI_CLASS( IMenuBackgroundVideoParam );
REGISTER_RTTI_CLASS( CMenuInheritBackgroundVideoParam );
REGISTER_RTTI_CLASS( CMenuBackgroundVideoFileParam );
REGISTER_RTTI_CLASS( CMenuBackgroundVideoAliasParam );
REGISTER_RTTI_CLASS( CMenuClearBackgroundVideoParam );
REGISTER_RTTI_CLASS( CMenuTimeScaleParam );
REGISTER_RTTI_CLASS( CMenuPauseParam );
REGISTER_RTTI_CLASS( CMenuRenderBackgroundParam );
REGISTER_RTTI_CLASS( CMenuBackgroundEffectParam );
REGISTER_RTTI_CLASS( CMenuHudParam );
REGISTER_RTTI_CLASS( CMenuFlashSymbolParam );
REGISTER_RTTI_CLASS( CMenuFlashSwfParam );
REGISTER_RTTI_CLASS( CMenuDef );

REGISTER_RTTI_CLASS( IPopupTimeParam );
REGISTER_RTTI_CLASS( CPopupTimeScaleParam );
REGISTER_RTTI_CLASS( CPopupPauseParam );
REGISTER_RTTI_CLASS( CPopupDef );

REGISTER_RTTI_CLASS( IScriptedFlash );
REGISTER_RTTI_CLASS( CScriptedFlashObject );
REGISTER_RTTI_CLASS( CScriptedFlashSprite );
REGISTER_RTTI_CLASS( CScriptedFlashArray );
REGISTER_RTTI_CLASS( CScriptedFlashTextField );
REGISTER_RTTI_CLASS( CScriptedFlashFunction );
REGISTER_RTTI_CLASS( CScriptedFlashValueStorage );
REGISTER_RTTI_STRUCT( SFlashArg );

REGISTER_RTTI_CLASS( CGuiManager );

// Wizard
REGISTER_RTTI_CLASS( CWizardBaseNode );
REGISTER_RTTI_CLASS( CWizardQuestionNode );
REGISTER_RTTI_CLASS( CWizardDefinition );
REGISTER_RTTI_CLASS( CWizardDefinitionFactory );

REGISTER_RTTI_CLASS( CScriptedRenderFrame );


// --------------------------------------------------------------------------------

#undef REGISTER_RTTI_CLASS
#undef REGISTER_RTTI_STRUCT
#undef REGISTER_RTTI_ENUM

#if defined( REGISTER_NOT_REGISTERED )
#undef REGISTER_RTTI_TYPE
#undef REGISTER_NOT_REGISTERED
#endif

#endif
