#ifndef _H_R6_NAMES_REGISTRY
#define _H_R6_NAMES_REGISTRY

#if !defined( REGISTER_NAME )
#define REGISTER_NAME( name_ ) RED_DECLARE_NAME( name_ )
#define REGISTER_NAMED_NAME( varname_, string_ ) RED_DECLARE_NAMED_NAME( varname_, string_ )
#define REGISTER_NOT_REGISTERED
#endif

// Events
//-------------------------------------------------------------------
// Upgrades system
REGISTER_NAME( Export_HandleEvent )
REGISTER_NAME( eventHandlerName )
REGISTER_NAME( rootAnimatedComponent )
REGISTER_NAME( parent )
REGISTER_NAME( weapon_l )
REGISTER_NAME( weapon_r )
//REGISTER_NAME( weapon )
REGISTER_NAME( shoot_position )
REGISTER_NAME( barrel )
REGISTER_NAME( Export_ApplyChanges )

// Upgrade events handlers
REGISTER_NAME( BrodecastHandler )
REGISTER_NAME( ForwardHandler )
REGISTER_NAME( FrameShootHandler )
REGISTER_NAME( BarrelShootHandler )
REGISTER_NAME( FrameReloadHandler )
REGISTER_NAME( PlayAnimationHandler )
REGISTER_NAME( CallScriptFunctionHandler )
REGISTER_NAME( FrameStartAutoFireHandler )
REGISTER_NAME( FrameStopAutoFireHandler )

// Upgrade events
REGISTER_NAME( MuzzleFlash )
REGISTER_NAME( OnShootLeft )
REGISTER_NAME( OnShootRight )
REGISTER_NAME( OnPlayAnimation )
REGISTER_NAME( OnShotFired )
REGISTER_NAME( OnStopShooting )

// Events
REGISTER_NAME( OnInit )
REGISTER_NAME( OnShoot )
REGISTER_NAME( OnEnemyNoticed )
REGISTER_NAME( OnEnemyDisapeare )
REGISTER_NAME( OnOwnerKilled )
REGISTER_NAME( Export_Initialize )
REGISTER_NAME( DemoStartMenu )

// Stats
REGISTER_NAME( MaxAmmo )
REGISTER_NAME( RateOfFire )
REGISTER_NAME( Damage )
REGISTER_NAME( RealoadTime )

// Game flow
REGISTER_NAME( OnTickScript )
REGISTER_NAME( OnGameEndedR6 )

// Tick components
REGISTER_NAME( OnInitialize )
REGISTER_NAME( OnComponentStartEntering )
REGISTER_NAME( OnComponentCompleteEntering )
REGISTER_NAME( OnComponentStartLeaving )
REGISTER_NAME( OnComponentCompleteLeaving )

// Player
REGISTER_NAME( OnPlayerTeleported )
REGISTER_NAME( Shoot )
REGISTER_NAME( Reload )
REGISTER_NAME( GI_Crouch )
REGISTER_NAME( GI_Sprint )
REGISTER_NAME( GI_Jump )
REGISTER_NAME( ExplorationCamera )

// AV
REGISTER_NAME( GI_AV_ACC )
REGISTER_NAME( GI_AV_DCC )
REGISTER_NAME( GI_AV_AxisL_X )
REGISTER_NAME( GI_AV_AxisL_Y )
REGISTER_NAME( GI_AV_AxisR_X )
REGISTER_NAME( GI_AV_AxisR_Y )

// Dialogs
REGISTER_NAME( OnAddSubtitle )
REGISTER_NAME( OnRemoveSubtitle )
REGISTER_NAME( OnDialogueInterrupted )
REGISTER_NAME( OnDialogueEnded )
REGISTER_NAME( OnDialogueLineChanged )
REGISTER_NAME( OnDialogueLineChangedWithTarget )
REGISTER_NAME( OnDialogueOptionAdded )
REGISTER_NAME( OnDialogueOptionRemoved )
REGISTER_NAME( OnDialogueOptionRemovedAll )
REGISTER_NAME( OnSetDisplayMode )
REGISTER_NAME( Connector )
REGISTER_NAME( lines )

// Interaction
REGISTER_NAME( OnStartInteracting )
REGISTER_NAME( OnStopInteracting )
REGISTER_NAME( GetInteractionName )
REGISTER_NAME( CanInteract )
REGISTER_NAME( OnAbortInteraction )
REGISTER_NAMED_NAME( m_interactionRange, "Interaction Range" )
REGISTER_NAME( m_interactionRadius )
REGISTER_NAME( OnInteractionTurnedOn )
REGISTER_NAME( OnInteractionTurnedOff )


// Modules
REGISTER_NAME( OnComDeviceShow )
REGISTER_NAME( OnComDeviceHide )
REGISTER_NAME( OnInteractionGUIShow )
REGISTER_NAME( OnInteractionGUIHide )
REGISTER_NAME( OnShowInteractionProgress )
REGISTER_NAME( OnInteractionSuceed )
REGISTER_NAME( OnInteractionFailed )
REGISTER_NAME( OnDisplayFocusedInteraction )
REGISTER_NAME( OnFocusedInteractionScrollUp )
REGISTER_NAME( OnFocusedInteractionScrollDown )
REGISTER_NAME( OnFocusedInteractionShow )
REGISTER_NAME( OnFocusedInteractionHide )
REGISTER_NAME( OnClearDialogueLine )

// Hit and Damaged
//REGISTER_NAME( OnDeath )
REGISTER_NAME( OnHit )

// Firearm
REGISTER_NAME( OnFire )
REGISTER_NAME( OnRequestFire )
REGISTER_NAME( OnRequestSecondaryFire )
REGISTER_NAME( OnRequestReload )
REGISTER_NAME( OnImpact )

// Tooltip
REGISTER_NAME( OnLookAt		)
REGISTER_NAME( OnLookAway	)
REGISTER_NAME( OnInteracted	)

// Aim help
REGISTER_NAME( OnShotCorrected )
REGISTER_NAME( AimHelpTrace )

//Interactions
REGISTER_NAME( Export_Perform )
REGISTER_NAME( Export_GetUniqueName )
REGISTER_NAME( Export_IsFulfilled )
REGISTER_NAME( implant )
REGISTER_NAME( skill )
REGISTER_NAME( program )
REGISTER_NAME( usable_item )
REGISTER_NAME( general )
REGISTER_NAME( drone )
REGISTER_NAME( InitInteraction )
REGISTER_NAME( OnStartInteraction )
REGISTER_NAME( OnStopInteraction )
REGISTER_NAME( OnInteractionUpdate )

// Switch
REGISTER_NAME( OnSwitched )
REGISTER_NAME( OnSwitchedOn )
REGISTER_NAME( OnSwitchedOff )

// Dialog
REGISTER_NAME( numOutputs )
REGISTER_NAME( Interrupted )
REGISTER_NAME( connectors )
REGISTER_NAME( voiceTag ) 
REGISTER_NAME( Defualt )
REGISTER_NAME( Nobody )
REGISTER_NAME( Unnamed )
REGISTER_NAME( Fork )
REGISTER_NAME( Text )
REGISTER_NAME( Fact )
REGISTER_NAME( OutputTerminate )
REGISTER_NAME( RequestFocus )
REGISTER_NAME( Branch )
REGISTER_NAME( Checkpoint )
REGISTER_NAME( CommunicatorSwitch )
REGISTER_NAME( Events )
REGISTER_NAME( Flow )

// Crowd
REGISTER_NAME( Crowd )

// AI
REGISTER_NAME( text )
REGISTER_NAME( channel )
REGISTER_NAME( AITree )
REGISTER_NAME( DebugLog )
REGISTER_NAME( AIAction )
REGISTER_NAME( InputDecorator )
REGISTER_NAME( SelectTargetByTagDecoratorR6 )
REGISTER_NAME( ChooseInteractionDecorator )
REGISTER_NAME( CanBeStartedOn )
REGISTER_NAME( OnStart )
REGISTER_NAME( OnStop )
REGISTER_NAME( OnReset )
REGISTER_NAME( SequenceCheckAvailabilityR6 )
REGISTER_NAME( PlayerStateSelector )
REGISTER_NAME( ConditionIsPlayer )
REGISTER_NAME( PlayerState )
REGISTER_NAME( Walk )
REGISTER_NAME( Sprint )
REGISTER_NAME( Jump )
REGISTER_NAME( JumpGoingToLand )
REGISTER_NAME( PlayerMovementData )

// Body parts
//------------------------------------------------------------------------------------------------------------------------------------
REGISTER_NAME( chest )

// Keys
//------------------------------------------------------------------------------------------------------------------------------------
REGISTER_NAME( GC_DialogOption_1 )
REGISTER_NAME( GC_DialogOption_2 )
REGISTER_NAME( GC_DialogOption_3 )
REGISTER_NAME( GC_DialogOption_4 )





// behavior slots
//------------------------------------------------------------------------------------------------------------------------------------
REGISTER_NAME( lowerbody )
REGISTER_NAME( lookaround )
REGISTER_NAME( interactions )
REGISTER_NAME( retargeting )





// Combat
//------------------------------------------------------------------------------------------------------------------------------------
REGISTER_NAME( PlayerTeam )


#if defined( REGISTER_NOT_REGISTERED )
#undef REGISTER_NAME
#undef REGISTER_NAMED_NAME
#undef REGISTER_NOT_REGISTERED
#endif

#endif // _H_R6_NAMES_REGISTRY
