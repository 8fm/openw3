/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#ifndef _H_R6_TYPE_REGISTRY
#define _H_R6_TYPE_REGISTRY

// this file contains list of all types in 'r6' project

// Not defined when included in r6.h, but defined when included in r6Classes.cpp
#if !defined( REGISTER_RTTI_TYPE )
#define REGISTER_RTTI_TYPE( _className ) RED_DECLARE_RTTI_NAME( _className ) template<> struct TTypeName< _className >{ static const CName& GetTypeName() { return CNAME( _className ); } };
#define REGISTER_NOT_REGISTERED
#endif

#define REGISTER_RTTI_CLASS( _className ) class _className; REGISTER_RTTI_TYPE( _className );
#define REGISTER_RTTI_STRUCT( _className ) struct _className; REGISTER_RTTI_TYPE( _className );
#define REGISTER_RTTI_ENUM( _className ) enum _className; REGISTER_RTTI_TYPE( _className );


// ------------------------------- ADD CLASSES HERE -------------------------------

//------------------------------------------------------------------------------------------------------------------
// Globals
//------------------------------------------------------------------------------------------------------------------
REGISTER_RTTI_CLASS( CR6Game )
REGISTER_RTTI_CLASS( CR6GameResource )


//------------------------------------------------------------------------------------------------------------------
// Player
//------------------------------------------------------------------------------------------------------------------
REGISTER_RTTI_CLASS( CR6Player )
REGISTER_RTTI_CLASS( CPlayerInventoryPanel )
REGISTER_RTTI_CLASS( CPlayerCraftingPanel )
REGISTER_RTTI_STRUCT( SCraftingRecipe )
REGISTER_RTTI_CLASS( CCraftingBench )
REGISTER_RTTI_CLASS( CR6BehaviorSlotComponent )

// Aim
REGISTER_RTTI_STRUCT( SAimHelpParams )
REGISTER_RTTI_CLASS( CAimHelpTargetsGatherer )
REGISTER_RTTI_CLASS( CAimTarget )
REGISTER_RTTI_CLASS( CR6AimingComponent )


//------------------------------------------------------------------------------------------------------------------
// Camera
//------------------------------------------------------------------------------------------------------------------
REGISTER_RTTI_CLASS( CR6CameraDirector )
REGISTER_RTTI_CLASS( CR6CameraAttachment )
REGISTER_RTTI_CLASS( CExplorationCameraComponent )
REGISTER_RTTI_CLASS( CInteractionCameraComponent )
REGISTER_RTTI_CLASS( CSlotCameraComponent )
REGISTER_RTTI_CLASS( CScriptedCameraComponent )
REGISTER_RTTI_CLASS( CAVCameraTest )


//------------------------------------------------------------------------------------------------------------------
// Upgrade system
//------------------------------------------------------------------------------------------------------------------
REGISTER_RTTI_CLASS( CItemPartSlotDefinition )
REGISTER_RTTI_CLASS( CItemPartDefinitionComponent )
REGISTER_RTTI_CLASS( CScriptedUgpradeEventHandler )
REGISTER_RTTI_CLASS( CUpgradeEventHandler )
REGISTER_RTTI_CLASS( CBrodecastEventHandler )
REGISTER_RTTI_CLASS( CCallScriptFunctionEventHandler )
REGISTER_RTTI_CLASS( CForwardEventHandler )
REGISTER_RTTI_CLASS( CPlayAnimationEventHandler )
REGISTER_RTTI_CLASS( CBarrelShootEventHandler )
REGISTER_RTTI_CLASS( CFrameReloadEventHandler )
REGISTER_RTTI_CLASS( CFrameShootEventHandler )
REGISTER_RTTI_CLASS( CFrameStartAutoFireEventHandler )
REGISTER_RTTI_CLASS( CFrameStopAutoFireEventHandler )

REGISTER_RTTI_CLASS( CUpgradeEventDefinition )
REGISTER_RTTI_CLASS( CBaseUpgradeStatsModyfier )
REGISTER_RTTI_CLASS( CEditableUpgradeStatsModyfier )
REGISTER_RTTI_CLASS( CUpgradesSpawnerItemEntity )
REGISTER_RTTI_CLASS( CScriptedStatsModyfier )
REGISTER_RTTI_STRUCT( SEditableUpgradeStatsModifierEntry )
REGISTER_RTTI_STRUCT( SUpgradeEventHandlerParam )

// Upgrade system - customization
REGISTER_RTTI_CLASS( CUpgradesCustomisationComponent )
REGISTER_RTTI_STRUCT( SCustomisationSlotDefinition )
REGISTER_RTTI_STRUCT( SCustomisationCategoryDefinition )

// Upgrade system - params
REGISTER_RTTI_CLASS( CUpgradeEventHandlerParam )
REGISTER_RTTI_CLASS( CSingleNameUpgradeHandlerParam )
REGISTER_RTTI_CLASS( CForwardUpgradeHandlerParam )
REGISTER_RTTI_CLASS( CPlayAnimationUpgradeHandlerParam )
REGISTER_RTTI_CLASS( CShootUpgradeHandlerParam )

// Upgrade system - extensions
REGISTER_RTTI_CLASS( CFirearmFrameDefinitionComponent )
REGISTER_RTTI_CLASS( CFirearmBarrelDefinitionComponent )


//------------------------------------------------------------------------------------------------------------------
// AI
//------------------------------------------------------------------------------------------------------------------
REGISTER_RTTI_CLASS( CR6AISystem )
REGISTER_RTTI_CLASS( CAITreeComponent )
REGISTER_RTTI_CLASS( CAIActionPerformerAttachment )
REGISTER_RTTI_CLASS( CAIAction )
//REGISTER_RTTI_CLASS( CAIActionScripted ) // temp
REGISTER_RTTI_CLASS( CAIActionPlayAnimation )
REGISTER_RTTI_CLASS( CAIActionMove )
REGISTER_RTTI_CLASS( CAIActionMoveN )
REGISTER_RTTI_CLASS( CAIActionMoveToNode )
REGISTER_RTTI_CLASS( CAIActionMoveToInteraction )
REGISTER_RTTI_CLASS( CAIActionMoveAwayFromNode )
REGISTER_RTTI_CLASS( CAIActionRotateMACToNode )
REGISTER_RTTI_CLASS( CAIActionInteraction )
REGISTER_RTTI_CLASS( CAIActionRotateMACToInteraction )

REGISTER_RTTI_CLASS( CR6BehTreeInstance )
REGISTER_RTTI_CLASS( CBehTreeValAIAction )
REGISTER_RTTI_STRUCT( SPlayerMovementData )

REGISTER_RTTI_CLASS( CBehTreeValEInputDecoratorCondition );
REGISTER_RTTI_CLASS( CBehTreeValEInputDecoratorAction );


REGISTER_RTTI_CLASS( CR6NPC )
REGISTER_RTTI_CLASS( CEnemyAwareComponent )
REGISTER_RTTI_CLASS( CNPCMovementControllerComponent )
REGISTER_RTTI_CLASS( CMoveSTAvoidEnemies )
REGISTER_RTTI_STRUCT( SR6VisionParams )
REGISTER_RTTI_STRUCT( SR6ScriptedEnemyInfo )


// Combat
REGISTER_RTTI_CLASS( CCombatUtils )
REGISTER_RTTI_STRUCT( SBulletImpactInfo )
REGISTER_RTTI_CLASS( CBulletProjectile )
REGISTER_RTTI_STRUCT( SRestrictionArea )

// Team
REGISTER_RTTI_CLASS( CTeam )
REGISTER_RTTI_CLASS( CTeamManager )
REGISTER_RTTI_CLASS( CTeamMemberComponent )
REGISTER_RTTI_CLASS( CTeamSharedKnowladge )
REGISTER_RTTI_STRUCT( SCombatLine )
REGISTER_RTTI_STRUCT( SCombatAlley )

// Cover system
REGISTER_RTTI_CLASS( CCoverPointComponent )
REGISTER_RTTI_CLASS( CCoversManager )
REGISTER_RTTI_STRUCT( SCoverRatingsWeights )
REGISTER_RTTI_STRUCT( SCoverAttackInfo )

// Event system
REGISTER_RTTI_CLASS( CEventRouterComponent )
REGISTER_RTTI_CLASS( CR6Component )

// Statistics
REGISTER_RTTI_CLASS( CStatsContainerComponent )
REGISTER_RTTI_STRUCT( SSingleStat )
REGISTER_RTTI_STRUCT( SStatsList )

// Followers
REGISTER_RTTI_CLASS( CFollowerPOIComponent )

//Traits
REGISTER_RTTI_CLASS( IRequirement )
REGISTER_RTTI_CLASS( CTraitRequirement )
REGISTER_RTTI_CLASS( CImplantRequirement )
REGISTER_RTTI_CLASS( CSkillRequirement )
REGISTER_RTTI_CLASS( ITraitAbility )
REGISTER_RTTI_CLASS( CActiveTraitAbility )
REGISTER_RTTI_CLASS( CPassiveTraitAbility )
REGISTER_RTTI_CLASS( CTraitAbilityWrapper )
REGISTER_RTTI_CLASS( CTraitRequirementWrapper )
REGISTER_RTTI_STRUCT( SSkillTableEntry )
REGISTER_RTTI_STRUCT( STraitTableEntry )
REGISTER_RTTI_CLASS( CTraitData )
REGISTER_RTTI_CLASS( CTraitDataFactory )
REGISTER_RTTI_CLASS( CTraitComponent )

// companion
REGISTER_RTTI_CLASS( CCompanionComponent )

//------------------------------------------------------------------------------------------------------------------
// GUI
//------------------------------------------------------------------------------------------------------------------
//GUI Resource
REGISTER_RTTI_CLASS( CR6Hud )
REGISTER_RTTI_CLASS( CR6HudModule )
REGISTER_RTTI_CLASS( CR6Menu )


REGISTER_RTTI_CLASS( CR6GuiManager )

// Interaction System
REGISTER_RTTI_CLASS( CR6InteractionComponent )
REGISTER_RTTI_CLASS( CR6InteractionTurnOnOff )

// R6 Inventory System
REGISTER_RTTI_CLASS( CR6InventoryComponent )
REGISTER_RTTI_CLASS( CR6InventoryItemComponent )


//------------------------------------------------------------------------------------------------------------------
// Dialog
//------------------------------------------------------------------------------------------------------------------
REGISTER_RTTI_CLASS( CInteractiveDialogSystem )
REGISTER_RTTI_CLASS( CInteractiveDialog )
REGISTER_RTTI_CLASS( CR6DialogDisplay )
REGISTER_RTTI_CLASS( CIDInterlocutorComponent )
REGISTER_RTTI_STRUCT( SIDInterlocutorDefinition )
REGISTER_RTTI_STRUCT( SIDActorDefinition )
REGISTER_RTTI_STRUCT( SIDInterlocutorNameWrapper )
REGISTER_RTTI_STRUCT( SRunningDialog )
REGISTER_RTTI_CLASS( CIDTopic ) 

// Text and sound
REGISTER_RTTI_STRUCT( SIDBaseLine )
REGISTER_RTTI_STRUCT( SIDTextLine )
REGISTER_RTTI_STRUCT( SIDConnectorLine )
REGISTER_RTTI_STRUCT( SIDOption )
REGISTER_RTTI_STRUCT( SVoiceTagWrapper )
REGISTER_RTTI_STRUCT( SIDConnectorPackDefinition )
REGISTER_RTTI_CLASS( CIDConnectorPack )

// Graph
REGISTER_RTTI_CLASS( CIDGraph )
REGISTER_RTTI_CLASS( CIDGraphSocket )

REGISTER_RTTI_CLASS( CIDGraphBlock )
REGISTER_RTTI_CLASS( CIDGraphBlockInput )
REGISTER_RTTI_CLASS( CIDGraphBlockOutput )
REGISTER_RTTI_CLASS( CIDGraphBlockOutputTerminate )
REGISTER_RTTI_CLASS( CIDGraphBlockText )
REGISTER_RTTI_CLASS( CIDGraphBlockFlow )
REGISTER_RTTI_CLASS( CIDGraphBlockBranch )
REGISTER_RTTI_CLASS( CIDGraphBlockChoice )
REGISTER_RTTI_CLASS( CIDGraphBlockFork )
REGISTER_RTTI_CLASS( CIDGraphBlockCondition )
REGISTER_RTTI_CLASS( CIDGraphBlockFact )
REGISTER_RTTI_CLASS( CIDGraphBlockComunicatorSwitch )
REGISTER_RTTI_CLASS( CIDGraphBlockRequestFocus )
REGISTER_RTTI_CLASS( CIDGraphBlockInteraction )
REGISTER_RTTI_CLASS( CIDGraphBlockEvents )
REGISTER_RTTI_CLASS( CIDGraphBlockConnector )
REGISTER_RTTI_CLASS( CIDGraphBlockCheckpoint )

// Conditions
REGISTER_RTTI_CLASS( IIDContition )
REGISTER_RTTI_CLASS( IIDScriptedContition )
REGISTER_RTTI_CLASS( CIDScriptedInterlocutorContition )
REGISTER_RTTI_CLASS( CIDScriptedManyInterlocutorsContition )
REGISTER_RTTI_CLASS( IDConditionList )
REGISTER_RTTI_CLASS( CIDActivator )
REGISTER_RTTI_CLASS( CIDAreaContition )
REGISTER_RTTI_CLASS( CIDInterlocutorInAreaContition )
REGISTER_RTTI_CLASS( CIDTaggedEntityInAreaContition )

// Events
REGISTER_RTTI_CLASS( CIdEvent )
REGISTER_RTTI_CLASS( CIdEventGeneral )
REGISTER_RTTI_CLASS( CIdEventInterlocutor )
REGISTER_RTTI_CLASS( CIdEventAI )
REGISTER_RTTI_CLASS( CIdEventAnimation )
REGISTER_RTTI_CLASS( CIdEventEncounter )
REGISTER_RTTI_CLASS( CIdEventQuest )
REGISTER_RTTI_CLASS( CIdEventAbortInteraction )
REGISTER_RTTI_CLASS( CIdEventSound )

REGISTER_RTTI_CLASS( CEventSender )
REGISTER_RTTI_STRUCT( SGeneralEventData )
REGISTER_RTTI_STRUCT( SInterlocutorEventData )
REGISTER_RTTI_STRUCT( SAnimationEventData )
REGISTER_RTTI_STRUCT( SAIEventData )
REGISTER_RTTI_STRUCT( SEncounterPhaseData )
REGISTER_RTTI_STRUCT( SIDSoundEventParams )
REGISTER_RTTI_CLASS( IIDAIEventParam )

// This is here ONLY for backwards compatibility reasons. 
// SIDLine is deprecated and will be removed after content resave. PLEASE DO NOT USE.
#if VER_IDLINES_REFACTOR > VER_MINIMAL
	REGISTER_RTTI_STRUCT( SIDLine )
#endif

//------------------------------------------------------------------------------------------------------------------
// Crowd
//------------------------------------------------------------------------------------------------------------------
REGISTER_RTTI_CLASS( CCrowdManager )
REGISTER_RTTI_CLASS( CCrowdEntryPoint )
REGISTER_RTTI_CLASS( CCrowdArea )
REGISTER_RTTI_CLASS( CCrowdAreaComponent )

//------------------------------------------------------------------------------------------------------------------
// Vehicles
//------------------------------------------------------------------------------------------------------------------
REGISTER_RTTI_STRUCT( SAVIdleData )
REGISTER_RTTI_STRUCT( SAVSpeedData )
REGISTER_RTTI_STRUCT( SAVTurningData )
REGISTER_RTTI_STRUCT( SAVHoveringData )
REGISTER_RTTI_STRUCT( SAVSafetyBrakesData )
REGISTER_RTTI_CLASS( CR6AdvancedVehicleComponent )
REGISTER_RTTI_CLASS( CAVComponentTest )



//------------------------------------------------------------------------------------------------------------------
// Damage system
//------------------------------------------------------------------------------------------------------------------
REGISTER_RTTI_STRUCT( SR6DamageData )


//------------------------------------------------------------------------------------------------------------------
// Interactive objects
//------------------------------------------------------------------------------------------------------------------
REGISTER_RTTI_CLASS( CSwitch )


//------------------------------------------------------------------------------------------------------------------
// Quest
//------------------------------------------------------------------------------------------------------------------
REGISTER_RTTI_CLASS( CR6QuestSystem )
REGISTER_RTTI_CLASS( CQuestGraphBlockInteractiveDialog )
REGISTER_RTTI_CLASS( CQuestObjectiveComponent )
REGISTER_RTTI_CLASS( CQuestGraphR6QuestBlock )
REGISTER_RTTI_CLASS( CQuestGraphR6QuestConditionBlock )
REGISTER_RTTI_CLASS( CQuestGraphR6QuestPauseBlock )

// --------------------------------------------------------------------------------

#undef REGISTER_RTTI_CLASS
#undef REGISTER_RTTI_STRUCT
#undef REGISTER_RTTI_ENUM

#if defined( REGISTER_NOT_REGISTERED )
#undef REGISTER_RTTI_TYPE
#undef REGISTER_NOT_REGISTERED
#endif

#endif
