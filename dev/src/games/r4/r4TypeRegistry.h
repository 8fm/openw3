/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#ifndef _H_R4_TYPE_REGISTRY
#define _H_R4_TYPE_REGISTRY

// this file contains list of all types in 'r4' project

// Not defined when included in r4.h, but defined when included in r4Classes.cpp
#if !defined( REGISTER_RTTI_TYPE )
#define REGISTER_RTTI_TYPE( _className ) RED_DECLARE_RTTI_NAME( _className ) template<> struct TTypeName< _className >{ static const CName& GetTypeName() { return CNAME( _className ); } };
#define REGISTER_NOT_REGISTERED
#endif

#define REGISTER_RTTI_CLASS( _className ) class _className; REGISTER_RTTI_TYPE( _className );
#define REGISTER_RTTI_STRUCT( _className ) struct _className; REGISTER_RTTI_TYPE( _className );
#define REGISTER_RTTI_ENUM( _className ) enum _className; REGISTER_RTTI_TYPE( _className );


// ------------------------------- ADD CLASSES HERE -------------------------------

REGISTER_RTTI_CLASS( CR4Game );
REGISTER_RTTI_CLASS( CWitcherGameResource );

REGISTER_RTTI_CLASS( CR4TutorialSystem );

//REGISTER_RTTI_CLASS( CR4QuestSystem );
REGISTER_RTTI_CLASS( CR4Player );
REGISTER_RTTI_CLASS( CR4LocomotionDirectController );
REGISTER_RTTI_CLASS( CR4LocomotionDirectControllerScript );

REGISTER_RTTI_STRUCT( SPredictionInfo );
REGISTER_RTTI_CLASS( CHorsePrediction );

REGISTER_RTTI_CLASS( CFocusModeController );
REGISTER_RTTI_CLASS( CGameplayFXSurfacePost );

REGISTER_RTTI_CLASS( CPlayerStateUseVehicle );
REGISTER_RTTI_CLASS( CPlayerStatePostUseVehicle );

REGISTER_RTTI_CLASS( CQuestBehaviorSyncGraphSocket );
REGISTER_RTTI_CLASS( SBehaviorGroup );
REGISTER_RTTI_CLASS( CQuestBehaviorCtrlBlock );
REGISTER_RTTI_CLASS( CQuestBehaviorEventBlock );
REGISTER_RTTI_CLASS( SQuestBehaviorEvent );
REGISTER_RTTI_CLASS( CQuestBehaviorNotificationBlock );
REGISTER_RTTI_CLASS( SQuestBehaviorNotification );
REGISTER_RTTI_CLASS( CQuestPlayAnimationBlock );
REGISTER_RTTI_CLASS( CManageSwitchBlock );
REGISTER_RTTI_CLASS( CQuestChangeWorldBlock );
REGISTER_RTTI_CLASS( CQuestManageFastTravelBlock );
REGISTER_RTTI_CLASS( CQuestUsedFastTravelCondition );
REGISTER_RTTI_CLASS( IUIConditionType );
REGISTER_RTTI_CLASS( CQCIsOpenedMenu );
REGISTER_RTTI_CLASS( CQCIsOpenedJournalEntry );
REGISTER_RTTI_CLASS( CQCIsSentCustomUIEvent );
REGISTER_RTTI_CLASS( CQCIsObjectiveHighlighted );
REGISTER_RTTI_CLASS( CQuestUICondition );
REGISTER_RTTI_CLASS( CTaggedActorsListener );
REGISTER_RTTI_CLASS( IQuestCombatManagerBaseBlock );
REGISTER_RTTI_CLASS( CQuestSpawnVehicleBlock );
REGISTER_RTTI_CLASS( CQuestSpawnNotStreamedBoatBlock );
REGISTER_RTTI_CLASS( CQuestResetScriptedActionsBlock );
REGISTER_RTTI_CLASS( CR4CreateEntityHelper );

REGISTER_RTTI_CLASS( W3HorseComponent );
REGISTER_RTTI_CLASS( W3Boat );
REGISTER_RTTI_CLASS( W3BoatSpawner );

// Encounters
REGISTER_RTTI_CLASS( CDaytimeCondition );
REGISTER_RTTI_CLASS( CPlayerLevelCondition );
REGISTER_RTTI_CLASS( CSpawnConditionFact );


REGISTER_RTTI_CLASS( CWitcherSword );
REGISTER_RTTI_CLASS( RangedWeapon );

REGISTER_RTTI_CLASS( CR4ReactionManager );

// Combos
REGISTER_RTTI_CLASS( CComboDefinition );
REGISTER_RTTI_CLASS( CComboString );
REGISTER_RTTI_CLASS( CComboAspect );
REGISTER_RTTI_STRUCT( SComboAnimationData );
REGISTER_RTTI_CLASS( CComboPlayer );
REGISTER_RTTI_STRUCT( SComboAttackCallbackInfo );

// Combat
REGISTER_RTTI_CLASS( CR4HumanoidCombatComponent );
REGISTER_RTTI_STRUCT( SSoundInfoMapping );

// New Camera
REGISTER_RTTI_CLASS( CR4CameraDirector );
REGISTER_RTTI_CLASS( CCustomCamera );
REGISTER_RTTI_STRUCT( SCameraAnimationDefinition );
REGISTER_RTTI_STRUCT( SCameraMovementData );
REGISTER_RTTI_STRUCT( SCustomCameraPreset );
REGISTER_RTTI_CLASS( ICustomCameraBaseController );
REGISTER_RTTI_CLASS( ICustomCameraPivotPositionController );
REGISTER_RTTI_CLASS( CCustomCameraRopePPC );
REGISTER_RTTI_CLASS( CCustomCameraPlayerPPC );
REGISTER_RTTI_CLASS( CCustomCameraBlendPPC );
REGISTER_RTTI_CLASS( ICustomCameraPivotRotationController );
REGISTER_RTTI_CLASS( CCustomCameraDefaultPRC );
REGISTER_RTTI_CLASS( ICustomCameraPivotDistanceController );
REGISTER_RTTI_CLASS( CCustomCameraDefaultPDC );
REGISTER_RTTI_CLASS( CCustomCameraAdditivePDC );
REGISTER_RTTI_CLASS( ICustomCameraPositionController );
REGISTER_RTTI_CLASS( CCustomCameraSimplePositionController );
REGISTER_RTTI_CLASS( CTrailerCameraPositionController );
REGISTER_RTTI_CLASS( ICustomCameraScriptedPivotPositionController );
REGISTER_RTTI_CLASS( ICustomCameraScriptedPivotRotationController );
REGISTER_RTTI_CLASS( ICustomCameraScriptedPivotDistanceController );
REGISTER_RTTI_CLASS( ICustomCameraScriptedPositionController );
REGISTER_RTTI_CLASS( ICustomCameraScriptedCurveSetPivotPositionController );
REGISTER_RTTI_CLASS( ICustomCameraScriptedCurveSetPivotRotationController );
REGISTER_RTTI_CLASS( ICustomCameraScriptedCurveSetPivotDistanceController );
REGISTER_RTTI_CLASS( ICustomCameraScriptedCurveSetPositionController );
REGISTER_RTTI_CLASS( ICustomCameraCollisionController );
REGISTER_RTTI_CLASS( CCustomCameraOutdoorCollisionController );
REGISTER_RTTI_CLASS( CCustomCameraAutoAvoidanceCollisionController );
REGISTER_RTTI_STRUCT( SCameraDistanceInfo );
REGISTER_RTTI_CLASS( CCombatCameraPositionController );

REGISTER_RTTI_CLASS( CDamageData );
REGISTER_RTTI_STRUCT(SProcessedDamage);

REGISTER_RTTI_CLASS( CR4GuiManager );
REGISTER_RTTI_CLASS( CR4Hud )
REGISTER_RTTI_CLASS( CR4HudModule )
REGISTER_RTTI_CLASS( CR4Menu )
REGISTER_RTTI_CLASS( CR4Popup )
REGISTER_RTTI_CLASS( CInGameConfigWrapper );

REGISTER_RTTI_STRUCT( SMapPinType );
REGISTER_RTTI_STRUCT( SMapPinConfig );
REGISTER_RTTI_CLASS( CCommonMapManager );
REGISTER_RTTI_CLASS( CWorldMap );
REGISTER_RTTI_STRUCT( SWorldMapImageInfo );
REGISTER_RTTI_STRUCT( SStaticMapPin );
REGISTER_RTTI_STRUCT( SUsedFastTravelEvent );
REGISTER_RTTI_STRUCT( SMenuEvent );
REGISTER_RTTI_CLASS( CGameplayFXMedalion );
REGISTER_RTTI_STRUCT( SCommonMapPinInstance );
REGISTER_RTTI_STRUCT( SMapPathDefinition );
REGISTER_RTTI_STRUCT( SMapPathInstance );
REGISTER_RTTI_STRUCT( SCustomMapPinDefinition );

REGISTER_RTTI_CLASS( CWitcherJournalManager );
REGISTER_RTTI_CLASS( SW3BaseStatusEvent );
REGISTER_RTTI_CLASS( SW3JournalObjectiveStatusEvent );
REGISTER_RTTI_CLASS( SW3JournalQuestStatusEvent );
REGISTER_RTTI_CLASS( SW3JournalCharacterEvent );
REGISTER_RTTI_CLASS( SW3JournalCharacterDescriptionEvent );
REGISTER_RTTI_CLASS( SW3JournalGlossaryEvent );
REGISTER_RTTI_CLASS( SW3JournalGlossaryDescriptionEvent );
REGISTER_RTTI_CLASS( SW3JournalTutorialEvent );
REGISTER_RTTI_CLASS( SW3JournalCreatureEvent );
REGISTER_RTTI_CLASS( SW3JournalCreatureDescriptionEvent );
REGISTER_RTTI_CLASS( SW3JournalStoryBookPageEvent );
REGISTER_RTTI_CLASS( SW3JournalPlaceEvent );
REGISTER_RTTI_CLASS( SW3JournalPlaceDescriptionEvent );
REGISTER_RTTI_CLASS( SW3JournalTrackEvent );
REGISTER_RTTI_CLASS( SW3JournalQuestTrackEvent );
REGISTER_RTTI_CLASS( SW3JournalQuestObjectiveTrackEvent );
REGISTER_RTTI_CLASS( SW3JournalQuestObjectiveCounterTrackEvent );
REGISTER_RTTI_CLASS( SW3JournalHighlightEvent );
REGISTER_RTTI_CLASS( SW3JournalHuntingQuestAddedEvent );
REGISTER_RTTI_CLASS( SW3JournalHuntingQuestClueFoundEvent );

REGISTER_RTTI_CLASS( CJournalQuestRoot );
REGISTER_RTTI_CLASS( CJournalQuestGroup );
REGISTER_RTTI_CLASS( CJournalQuest );
REGISTER_RTTI_CLASS( CJournalQuestPhase );
REGISTER_RTTI_CLASS( CJournalQuestDescriptionEntry );
REGISTER_RTTI_CLASS( CJournalQuestDescriptionGroup );
REGISTER_RTTI_CLASS( CJournalQuestObjective );
REGISTER_RTTI_CLASS( CJournalQuestMapPin );
REGISTER_RTTI_CLASS( CJournalQuestItemTag );
REGISTER_RTTI_CLASS( CJournalQuestEnemyTag );
REGISTER_RTTI_CLASS( CJournalCharacterRoot );
REGISTER_RTTI_CLASS( CJournalCharacterGroup );
REGISTER_RTTI_CLASS( CJournalCharacter );
REGISTER_RTTI_CLASS( CJournalCharacterDescription );
REGISTER_RTTI_CLASS( CJournalGlossaryRoot );
REGISTER_RTTI_CLASS( CJournalGlossaryGroup );
REGISTER_RTTI_CLASS( CJournalGlossary );
REGISTER_RTTI_CLASS( CJournalGlossaryDescription );
REGISTER_RTTI_CLASS( CJournalTutorialRoot );
REGISTER_RTTI_CLASS( CJournalTutorialGroup );
REGISTER_RTTI_CLASS( CJournalTutorial );
REGISTER_RTTI_CLASS( CJournalItemRoot );
REGISTER_RTTI_CLASS( CJournalItemGroup );
REGISTER_RTTI_CLASS( CJournalItemSubGroup );
REGISTER_RTTI_CLASS( CJournalItem );
REGISTER_RTTI_CLASS( CJournalItemComponent );
REGISTER_RTTI_CLASS( CJournalCreatureRoot );
REGISTER_RTTI_CLASS( CJournalCreatureGroup );
REGISTER_RTTI_CLASS( CJournalCreatureVirtualGroup );
REGISTER_RTTI_CLASS( CJournalCreature );
REGISTER_RTTI_CLASS( CJournalCreatureDescriptionGroup );
REGISTER_RTTI_CLASS( CJournalCreatureDescriptionEntry );
REGISTER_RTTI_CLASS( CJournalCreatureHuntingClueGroup );
REGISTER_RTTI_CLASS( CJournalCreatureHuntingClue );
REGISTER_RTTI_CLASS( CJournalCreatureGameplayHintGroup );
REGISTER_RTTI_CLASS( CJournalCreatureGameplayHint );
REGISTER_RTTI_CLASS( CJournalCreatureVitalSpotEntry );
REGISTER_RTTI_CLASS( CJournalCreatureVitalSpotGroup );

REGISTER_RTTI_CLASS( CJournalStoryBookRoot );
REGISTER_RTTI_CLASS( CJournalStoryBookPageDescription );
REGISTER_RTTI_CLASS( CJournalStoryBookPage );
REGISTER_RTTI_CLASS( CJournalStoryBookChapter );

REGISTER_RTTI_CLASS( CJournalPlaceRoot );
REGISTER_RTTI_CLASS( CJournalPlaceGroup );
REGISTER_RTTI_CLASS( CJournalPlace );
REGISTER_RTTI_CLASS( CJournalPlaceDescription );

REGISTER_RTTI_CLASS( CJournalQuestBlock );
REGISTER_RTTI_CLASS( CJournalQuestObjectiveCounterGraphBlock );
REGISTER_RTTI_CLASS( CJournalQuestMonsterKnownGraphBlock );
REGISTER_RTTI_STRUCT( SJournalQuestObjectiveData );
REGISTER_RTTI_CLASS( CJournalQuestHuntingBlock );
REGISTER_RTTI_CLASS( CQuestRiderScriptedActionsBlock );

REGISTER_RTTI_STRUCT( SJournalCreatureParams );
REGISTER_RTTI_CLASS( CR4JournalPlaceEntity );

REGISTER_RTTI_STRUCT( SCardDefinition );
REGISTER_RTTI_STRUCT( SDeckDefinition );
REGISTER_RTTI_STRUCT( CollectionCard );
REGISTER_RTTI_CLASS( CR4GwintManager );

REGISTER_RTTI_CLASS( CVitalSpot );
REGISTER_RTTI_CLASS( CVitalSpotsParam );
REGISTER_RTTI_STRUCT( SVitalSpotEnableConditions );

REGISTER_RTTI_CLASS( CR4InteriorAreaComponent );
REGISTER_RTTI_CLASS( CR4InteriorAreaEntity );

REGISTER_RTTI_CLASS( CR4EffectComponent );
// ai
REGISTER_RTTI_CLASS( CCombatDataComponent );
REGISTER_RTTI_CLASS( CR4BehTreeInstance );
REGISTER_RTTI_CLASS( CBehTreeNodeStrafingDefinition );
REGISTER_RTTI_CLASS( CBehTreeStrafingAlgorithmDefinition );
REGISTER_RTTI_CLASS( CBehTreeStrafingAlgorithmListDefinition );
REGISTER_RTTI_CLASS( CBehTreeStrafingAlgorithmFastSurroundDefinition );
REGISTER_RTTI_CLASS( CBehTreeStrafingAlgorithmNeverBackDownDefinition );
REGISTER_RTTI_CLASS( IGameplayEffectExecutor );
REGISTER_RTTI_STRUCT( SActiveVitalSpotEventData );
REGISTER_RTTI_CLASS( CBehTreeNodeVitalSpotActiveDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeConditionAttackersCountDefinition );
REGISTER_RTTI_CLASS( IBehTreeNodeConditionIsInInteriorBaseDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeConditionAmIInInteriorDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeConditionHasAbilityDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeConditionIsPlayerInInteriorDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeConditionAmIOrAPInInteriorDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeIgnoreInteriorsDuringPathfindingDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeConditionIsTargetThePlayerDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeConditionIsDeadDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeConditionIsEnemyAroundDefinition );
REGISTER_RTTI_CLASS( CBehTreeDecoratorChangeBehaviorGraphDefinition );
REGISTER_RTTI_CLASS( CBehTreeDecoratorSetBehaviorVariableDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeLandingDecoratorDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeTakeOffDecoratorDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeFindLandingSpotDecoratorDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeDecoratorDisableTalkInteractionDefinition );
REGISTER_RTTI_CLASS( CBehTreeDecoratorOverrideBehaviorVariableDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeCombatTicketManagerDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeCombatTicketManagedGetDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeCombatTicketHasDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeCombatTicketRequestDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeCombatTicketReleaseDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeCombatTicketLockDefinition );
REGISTER_RTTI_CLASS( IBehTreeNodeCombatTargetSelectionBaseDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeCombatTargetSelectionDefinition );
REGISTER_RTTI_CLASS( CBehTreeValECombatTargetSelectionSkipTarget );
REGISTER_RTTI_CLASS( CBehTreeNodeClosestNonFriendlyTargetSelectionDefinition );
REGISTER_RTTI_CLASS( IBehTreeNodeHLOffenceBaseDefinition );
REGISTER_RTTI_CLASS( IBehTreeNodeHLCombatBaseDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeHLCombatDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeHLAnimalCombatDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeDecoratorBruxaDeathDefinition );
REGISTER_RTTI_CLASS( IBehTreeDecoratorBaseHLDangerDefinition );
REGISTER_RTTI_CLASS( CBehTreeDecoratorHLDangerDefinition );
REGISTER_RTTI_CLASS( CBehTreeDecoratorHLDangerTamableDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeRequestItemsDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeDecoratorHLWildHorseDangerDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeProlongHLCombatDecoratorDefinition );
REGISTER_RTTI_CLASS( CBehTreeAtomicForgetCombatTargetDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodePCLockControlDecoratorDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodePCReleaseControlDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeAtomicSailorFollowPathDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeAtomicSailorMoveToPathDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeAtomicSailorMoveToDefinition );
REGISTER_RTTI_CLASS( CR4BehTreeNodePredefinedPathWithCompanionDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeRiderPursueHorseDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeRiderRotateToHorseDefinition );
REGISTER_RTTI_CLASS( CBehTreeDecoratorRidingManagerDefinition );
REGISTER_RTTI_CLASS( CBehTreeDecoratorRiderHorseReachabilityDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeDecoratorRidingCheckDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeRiderIdleDynamicRootDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeRiderForcedBehaviorDefinition );
REGISTER_RTTI_CLASS( CBehTreeDecoratorRiderPairingLogicDefinition );
REGISTER_RTTI_CLASS( CBehTreeDecoratorRiderPairWithHorseDefinition );
REGISTER_RTTI_CLASS( CBehTreeDecoratorHorsePairWithRiderDefinition );
REGISTER_RTTI_CLASS( CSpawnTreeInitializerRiderIdleAI );
REGISTER_RTTI_CLASS( CSpawnTreeInitializerRiderStartingBehavior );
REGISTER_RTTI_CLASS( IRiderActionParameters );
REGISTER_RTTI_CLASS( IRiderActionTree );
REGISTER_RTTI_CLASS( CBehTreeNodeRiderWaitHorseScriptedActionDefinition );
REGISTER_RTTI_CLASS( CSpawnTreeInitializerIdleFlightAI );
REGISTER_RTTI_CLASS( CBehTreeCluePathData );
REGISTER_RTTI_CLASS( CBehTreeNodePredefinedPathRuberBandDefinition );
REGISTER_RTTI_CLASS( IBehTreeNodeAtomicFlightDefinition );
REGISTER_RTTI_CLASS( IBehTreeNodeAtomicFlyAroundBaseDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeAtomicFlyAroundDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeAtomicFlyAroundPositionDefinition );
REGISTER_RTTI_CLASS( IBehTreeNodeAtomicFlyToBaseDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeAtomicFlyToDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeAtomicFlyToPositionDefinition );
REGISTER_RTTI_CLASS( IBehTreeNodeFlightDecoratorDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeDecoratorGlideDefinition );
REGISTER_RTTI_CLASS( CBehTreeDecoratorHorseSpeedManagerDefinition );
REGISTER_RTTI_CLASS( CVolumePathManager );
REGISTER_RTTI_CLASS( CBehTreeValEHorseMoveType );
REGISTER_RTTI_CLASS( CBehTreeNodeBroadcastReactionSceneDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeConditionShouldPursueCombatTargetDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeDynamicCombatStyleDefinition );
REGISTER_RTTI_CLASS( CHorseParkingActionPointSelector );
REGISTER_RTTI_CLASS( CBehTreeNodeSelectTargetOrMountByTagDecoratorDefinition );
REGISTER_RTTI_CLASS( CBehTreeMetanodeSetupCombatReachability );
REGISTER_RTTI_CLASS( CRainActionPointSelector );
REGISTER_RTTI_CLASS( CBehTreeNodeNotifyCombatActivationDecoratorDefinition );

REGISTER_RTTI_CLASS( CBehTreeNodeSendScaredEventDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeReceiveScaredEventDefinition );
REGISTER_RTTI_CLASS( IBehTreeNodeConditionSpeechDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeConditionIsInChatSceneDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeConditionIsSpeakingDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeConditionHasVoicesetDefintion );
REGISTER_RTTI_CLASS( IBehTreeNodeSpeechDecoratorDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodePlayVoicesetDecoratorDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodePlayVoicesetOnDeactivationDecoratorDefinition );
REGISTER_RTTI_STRUCT( SBecomeScaredEventData );
REGISTER_RTTI_STRUCT( SReactionSceneEvent );

// Ai params
REGISTER_RTTI_CLASS( CHorseRiderSharedParams );
REGISTER_RTTI_CLASS( CAIStorageRiderData );
REGISTER_RTTI_CLASS( CAIStorageAnimalData );
REGISTER_RTTI_CLASS( CAIStorageHorseData );
REGISTER_RTTI_STRUCT( SBehTreePairHorseEventParam );
REGISTER_RTTI_CLASS( CBehTreeTicketData );
REGISTER_RTTI_CLASS( IBehTreeTicketAlgorithmDefinition );
REGISTER_RTTI_CLASS( IBehTreeTicketAlgorithmDecoratorDefinition );
REGISTER_RTTI_CLASS( CBehTreeTicketAlgorithmRandomizeDefinition );
REGISTER_RTTI_CLASS( IBehTreeTicketAlgorithmListDefinition );
REGISTER_RTTI_CLASS( CBehTreeTicketAlgorithmListSumDefinition );
REGISTER_RTTI_CLASS( CBehTreeConstantTicketAlgorithmDefinition );
REGISTER_RTTI_CLASS( CBehTreeIfActiveTicketAlgorithmDefinition );
REGISTER_RTTI_CLASS( CBehTreeDistanceBasedTicketAlgorithmDefinition );
REGISTER_RTTI_STRUCT( CBehTreeDistanceBasedTicketAlgorithmField );
REGISTER_RTTI_CLASS( CBehTreeTimeBasedTicketAlgorithmDefinition );
REGISTER_RTTI_CLASS( ITicketAlgorithmScriptDefinition );
REGISTER_RTTI_CLASS( ITicketAlgorithmScript );
REGISTER_RTTI_CLASS( CBehTreeScriptTicketAlgorithmDefinition );
REGISTER_RTTI_CLASS( CTicketSourceConfiguration );
REGISTER_RTTI_CLASS( CTicketConfigurationParam );
REGISTER_RTTI_CLASS( CTicketsDefaultConfiguration );
REGISTER_RTTI_CLASS( CGlabalTicketSourceProvider );
REGISTER_RTTI_CLASS( CInstantMountPartySpawnOrganizer );

// swarm ai
REGISTER_RTTI_CLASS( CHumbleCrittersLairEntity );
REGISTER_RTTI_CLASS( CFlyingCrittersLairEntity );
REGISTER_RTTI_CLASS( CFlyingCrittersLairEntityScript );
REGISTER_RTTI_CLASS( CFlyingSwarmScriptInput );
REGISTER_RTTI_CLASS( CFlyingSwarmGroup );
REGISTER_RTTI_STRUCT( CFlyingGroupId );
REGISTER_RTTI_CLASS( CSwarmCellMap );
REGISTER_RTTI_CLASS( CSwarmCellMapFactory );
REGISTER_RTTI_CLASS( CSwarmSoundEmitterComponent );

// Steering 
REGISTER_RTTI_CLASS( IMoveSTBaseStrafeTarget );
REGISTER_RTTI_CLASS( CMoveSTStrafeSurroundTarget );
REGISTER_RTTI_CLASS( CMoveSTStrafeTargetRandomly );
REGISTER_RTTI_CLASS( CMoveSTStrafeTargetOneWay );
REGISTER_RTTI_CLASS( CMoveSCFastSurround );
REGISTER_RTTI_CLASS( CMoveSCTargetsCount );
REGISTER_RTTI_CLASS( CMoveSTApplySteeringToPlayerVariables );
REGISTER_RTTI_CLASS( CMoveSTRuberBand );
REGISTER_RTTI_CLASS( CMoveSTHorse );
REGISTER_RTTI_CLASS( CMoveSCRoadMovement );
REGISTER_RTTI_CLASS( CMoveSTOnRoad );

REGISTER_RTTI_CLASS( CBuffImmunity );
REGISTER_RTTI_CLASS( CBuffImmunityParam );

REGISTER_RTTI_CLASS( CMonsterParam );

REGISTER_RTTI_CLASS( CPreAttackEvent );
REGISTER_RTTI_CLASS( CPreAttackEventData );


REGISTER_RTTI_CLASS( CBehaviorGraphHorseNode );
REGISTER_RTTI_STRUCT( SHorseStateOffsets );
REGISTER_RTTI_STRUCT( SAlchemySubstanceData );

REGISTER_RTTI_STRUCT( SEntityMapPinInfo );
REGISTER_RTTI_CLASS( CEntityMapPinsResource );
REGISTER_RTTI_STRUCT( SQuestMapPinInfo );
REGISTER_RTTI_CLASS( CQuestMapPinsResource );
REGISTER_RTTI_STRUCT( SAreaMapPinInfo );
REGISTER_RTTI_CLASS( CAreaMapPinsResource );

REGISTER_RTTI_CLASS( CR4LootDefinitionBase );
REGISTER_RTTI_CLASS( CR4LootItemDefinition );
REGISTER_RTTI_CLASS( CR4LootContainerDefinition );
REGISTER_RTTI_CLASS( CR4LootContainerParam );
REGISTER_RTTI_STRUCT( SR4LootNameProperty );

//	Carrying items
REGISTER_RTTI_CLASS( CBehTreeNodeDecoratorCarryingItemManagerDefinition );
REGISTER_RTTI_CLASS( CCarryableItemsRegistry );
REGISTER_RTTI_CLASS( CCarryableItemStorePointComponent );
REGISTER_RTTI_CLASS( CBehTreeDecoratorCarryingItemsBaseDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeConditionIsCarryingItemDefinition );
REGISTER_RTTI_CLASS( CBehTreeDecoratorFindSourceItemStoreDefinition );
REGISTER_RTTI_CLASS( CBehTreeDecoratorFindDestinationItemStoreDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeCarryingItemBaseDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodePickItemDefinition );
REGISTER_RTTI_CLASS( CBehTreeNodeDropItemDefinition );

REGISTER_RTTI_CLASS( CBehTreeNodeGreetingReactionSceneDecoratorDefinition );

// Animation
REGISTER_RTTI_CLASS( CSkeletalAnimationStepClipParam );

REGISTER_RTTI_CLASS( CR4QuestSystem );
REGISTER_RTTI_CLASS( CJournalQuestMappinStateBlock );
REGISTER_RTTI_CLASS( CJournalQuestTrackBlock );
REGISTER_RTTI_CLASS( CExtAnimAttackEvent );

REGISTER_RTTI_CLASS( CR4TelemetryScriptProxy );
REGISTER_RTTI_CLASS( CR4SecondScreenManagerScriptProxy );
REGISTER_RTTI_CLASS( CR4KinectSpeechRecognizerListenerScriptProxy );
REGISTER_RTTI_CLASS( CR4GlobalEventsScriptsDispatcher );

REGISTER_RTTI_CLASS( CStorySceneAddFactEvent );
REGISTER_RTTI_CLASS( CStorySceneEventSurfaceEffect );
REGISTER_RTTI_CLASS( CExtAnimCutsceneSurfaceEffect );
REGISTER_RTTI_CLASS( CExtAnimCutsceneHideEntityEvent );
REGISTER_RTTI_CLASS( CExtAnimCutsceneDisableDangleEvent );

// gameplay lights
REGISTER_RTTI_CLASS( CGameplayLightComponent )
REGISTER_RTTI_CLASS( CPersistentLightComponent )
REGISTER_RTTI_CLASS( CCityLightManager )

REGISTER_RTTI_STRUCT( CFistfightOpponent );
REGISTER_RTTI_CLASS( CFistfightMinigame );

REGISTER_RTTI_CLASS( CGwintMenuInitData );
REGISTER_RTTI_CLASS( CGwintMinigame );

REGISTER_RTTI_CLASS( CBeehiveEntity );
REGISTER_RTTI_CLASS( W3ToxicCloud );

// DLC
REGISTER_RTTI_CLASS( CR4QuestDLCMounter );
REGISTER_RTTI_CLASS( CR4DefinitionsDLCMounter );
REGISTER_RTTI_CLASS( CR4DefinitionsEntitieTemplatesDLCMounter );
REGISTER_RTTI_CLASS( CR4JournalEntriesDLCMounter );
REGISTER_RTTI_CLASS( CR4JournalDLCMounter );
REGISTER_RTTI_CLASS( CR4RewardsDLCMounter );
REGISTER_RTTI_CLASS( CR4MappinsDLCMounter );
REGISTER_RTTI_CLASS( CR4AttitudesDLCMounter );
REGISTER_RTTI_CLASS( CR4ActionPointCategoriesDLCMounter );
REGISTER_RTTI_CLASS( CR4ScaleformContentDLCMounter );
REGISTER_RTTI_CLASS( CR4VideoDLCMounter );
REGISTER_RTTI_CLASS( CR4EntityTemplateParamDLCMounter );
REGISTER_RTTI_CLASS( CR4FinisherDLC );
REGISTER_RTTI_CLASS( CR4FinishersDLCMounter );
REGISTER_RTTI_CLASS( CR4AnimationsCategoriesDLCMounter );
REGISTER_RTTI_CLASS( CR4SceneAnimationsDLCMounter );
REGISTER_RTTI_CLASS( CR4EntityExternalAppearanceDLC );
REGISTER_RTTI_CLASS( CR4EntityExternalAppearanceDLCMounter );
REGISTER_RTTI_CLASS( CR4DropPhysicsSetupDLCMounter );
REGISTER_RTTI_CLASS( CR4EntityTemplateSlotDLCMounter );
REGISTER_RTTI_CLASS( CR4VoicetagDLCMounter );
REGISTER_RTTI_CLASS( CR4ResourceDefinitionsDLCMounter );
REGISTER_RTTI_CLASS( CR4WorldDescriptionDLC );
REGISTER_RTTI_CLASS( CR4WorldDLCMounter );
REGISTER_RTTI_CLASS( CR4WorldDLCExtender );
REGISTER_RTTI_CLASS( CR4DefinitionsNGPlusDLCMounter );

// player targeting
REGISTER_RTTI_STRUCT( STargetingInfo );
REGISTER_RTTI_STRUCT( SR4PlayerTargetingConsts );
REGISTER_RTTI_STRUCT( SR4PlayerTargetingPrecalcs );
REGISTER_RTTI_STRUCT( SR4PlayerTargetingIn );
REGISTER_RTTI_STRUCT( SR4PlayerTargetingOut );
REGISTER_RTTI_CLASS( CR4PlayerTargeting );

// abilities manager
REGISTER_RTTI_STRUCT( SBaseStat );
REGISTER_RTTI_STRUCT( SResistanceValue );
REGISTER_RTTI_STRUCT( SBlockedAbility );
REGISTER_RTTI_CLASS( W3AbilityManager );

// (scripted) game params
REGISTER_RTTI_CLASS( W3GameParams );

#ifndef NO_EDITOR
REGISTER_RTTI_CLASS( CR4ResourceAnalyzer );
REGISTER_RTTI_CLASS( CR4ItemAnalyzer );
REGISTER_RTTI_CLASS( CR4GameAnalyzer ); // quest+cutscenes+dialogs+community
REGISTER_RTTI_CLASS( CR4CommonAnalyzer );
REGISTER_RTTI_CLASS( CR4GuiAnalyzer );
REGISTER_RTTI_CLASS( CR4StartupAnalyzer );
REGISTER_RTTI_CLASS( CR4DLCAnalyzer );
#endif

#ifdef USE_HAVOK
// Witcher 2 types
REGISTER_RTTI_CLASS( W2MinigameDicePoker )
#endif
// --------------------------------------------------------------------------------

#undef REGISTER_RTTI_CLASS
#undef REGISTER_RTTI_STRUCT
#undef REGISTER_RTTI_ENUM

#if defined( REGISTER_NOT_REGISTERED )
#undef REGISTER_RTTI_TYPE
#undef REGISTER_NOT_REGISTERED
#endif

#endif
