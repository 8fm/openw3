#include "build.h"
#include "actor.h"

#include "../core/gatheredResource.h"
#include "../core/scriptingSystem.h"

#include "../engine/animGlobalParam.h"
#include "../engine/appearanceComponent.h"
#include "../engine/behaviorGraphAnimationSlotNode.h"
#include "../engine/behaviorGraphNotifier.h"
#include "../engine/behaviorGraphStack.h"
#include "../engine/characterControllerParam.h"
#include "../engine/clipMap.h"
#include "../engine/morphedMeshManagerComponent.h"
#include "../engine/physicsCharacterWrapper.h"
#include "../physics/physicsWrapper.h"
#include "../engine/renderCommands.h"
#include "../engine/soundEntityParam.h"
#include "../engine/soundStartData.h"
#include "../engine/visualDebug.h"

#include "actorsManager.h"
#include "actorSpeech.h"
#include "aiHistory.h"
#include "aiParameters.h"
#include "aiPresetParam.h"
#include "aiRedefinitionParameters.h"
#include "attackRange.h"
#include "baseDamage.h"
#include "behTree.h"
#include "behTreeDynamicNodeEvent.h"
#include "behTreeInstance.h"
#include "behTreeMachine.h"
#include "behTreeNode.h"
#include "behTreeNodeArbitrator.h"
#include "behTreeNodePlayScene.h"
#include "behTreeScriptedNode.h"
#include "commonGame.h"
#include "characterStats.h"
#include "definitionsManager.h"
#include "entityParams.h"
#include "factsDB.h"
#include "headManagerComponent.h"
#include "inventoryComponent.h"
#include "movableRepresentationPhysicalCharacter.h"
#include "movingAgentComponent.h"
#include "movingPhysicalAgentComponent.h"
#include "nodeStorage.h"
#include "reactionsManager.h"
#include "storySceneActorMap.h"
#include "storySceneComponent.h"
#include "storySceneInput.h"
#include "storySceneSystem.h"
#include "storySceneVoicetagMapping.h"
#include "storyScenePlayer.h"
#include "wayPointComponent.h"
#include "voicesetPlayer.h"
#include "questExternalScenePlayer.h"
#include "questsSystem.h"
#include "../engine/mimicComponent.h"


// ----------------------------------------------------------------------------
// Scripting support
// ----------------------------------------------------------------------------

void CActor::funcSignalGameplayEvent( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, name, CName::NONE );
	FINISH_PARAMETERS;

	if ( !name.Empty() )
	{
		SignalGameplayEvent( name, NULL, NULL );
	}
}

void CActor::funcSignalGameplayEventParamCName( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, name, CName::NONE );
	GET_PARAMETER( CName, param, CName::NONE );
	FINISH_PARAMETERS;

	if ( !name.Empty() )
	{
		SignalGameplayEvent( name, param );
	}
}

void CActor::funcSignalGameplayEventParamInt( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, name, CName::NONE );
	GET_PARAMETER( Int32, param, 0 );
	FINISH_PARAMETERS;

	if ( !name.Empty() )
	{
		SignalGameplayEvent( name, param );
	}
}

void CActor::funcSignalGameplayEventParamFloat( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, name, CName::NONE );
	GET_PARAMETER( Float, param, 0.0f );
	FINISH_PARAMETERS;

	if ( !name.Empty() )
	{
		SignalGameplayEvent( name, param );
	}
}

void CActor::funcSignalGameplayEventParamObject( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, name, CName::NONE );
	GET_PARAMETER( THandle< CObject >, obj, NULL );
	FINISH_PARAMETERS;

	if ( !name.Empty() )
	{
		SignalGameplayEvent( name, obj.Get() );
	}
}

void CActor::funcSignalGameplayEventReturnCName( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, name, CName::NONE );
	GET_PARAMETER( CName, defaultVal, CName::NONE );
	FINISH_PARAMETERS;

	CName val = name.Empty() ? defaultVal : SignalGameplayEventReturnCName( name, defaultVal );
	
	RETURN_NAME( val );
}
void CActor::funcSignalGameplayEventReturnInt( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, name, CName::NONE );
	GET_PARAMETER( Int32, defaultVal, 0 );
	FINISH_PARAMETERS;

	Int32 val = name.Empty() ? defaultVal : SignalGameplayEventReturnInt( name, defaultVal );
	RETURN_INT( val );
}
void CActor::funcSignalGameplayEventReturnFloat( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, name, CName::NONE );
	GET_PARAMETER( Float, defaultVal, 0.f );
	FINISH_PARAMETERS;

	Float val = name.Empty() ? defaultVal : SignalGameplayEventReturnFloat( name, defaultVal );
	RETURN_FLOAT( val );
}

void CActor::funcSignalGameplayDamageEvent( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, name, CName::NONE );
	GET_PARAMETER( THandle<CBaseDamage>, data, NULL );
	FINISH_PARAMETERS;

	if ( !name.Empty() )
	{
		SignalGameplayDamageEvent( name, data.Get() );
	}
}

void CActor::funcSignalGameplayAnimEvent( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, name, CName::NONE );
	GET_PARAMETER( CName, data, CName::NONE );
	FINISH_PARAMETERS;

	if ( !name.Empty() )
	{
		SignalGameplayAnimEvent( name, data );
	}
}

void CActor::funcForceAIUpdate( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	CBehTreeMachine* machine = GetBehTreeMachine();
	if ( machine )
	{
		machine->ForceUpdate();
	}
}

void CActor::funcGetRadius( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_FLOAT( GetRadius() );
}

void CActor::funcPlayVoiceset( CScriptStackFrame& stack, void* result )
{
	//value unused
	GET_PARAMETER( Int32, priority, 0 );
	GET_PARAMETER( String, voiceset, String() );
	GET_PARAMETER_OPT( Bool, breakCurrentSpeach, false );
	FINISH_PARAMETERS;

	//To backlog : remove setting priority value from scripts
	RETURN_BOOL( PlayVoiceset( BTAP_AboveIdle , voiceset ) );
}

void CActor::funcStopAllVoicesets( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_OPT( Bool, cleanupQueue, true );
	FINISH_PARAMETERS;
	StopAllVoicesets( cleanupQueue );
}

void CActor::funcHasVoiceset( CScriptStackFrame& stack, void* result )
{	
	GET_PARAMETER( String, voiceset, String() );
	FINISH_PARAMETERS;

	RETURN_INT( HasVoiceset( voiceset ) );
}

void CActor::funcIsRotatedTowards( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CNode >, node, NULL );
	GET_PARAMETER_OPT( Float, maxAngle, 10.0f );
	FINISH_PARAMETERS;

	CNode *pNode = node.Get();
	if( pNode )
	{
		RETURN_BOOL( IsRotatedTowards( pNode, maxAngle ) );
	}
	else
	{
		RETURN_BOOL( false );
	}
}

void CActor::funcIsRotatedTowardsPoint( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, point, Vector::ZEROS );
	GET_PARAMETER_OPT( Float, maxAngle, 10.0f );
	FINISH_PARAMETERS;
	RETURN_BOOL( IsRotatedTowards( point, maxAngle ) );
}

void CActor::funcGetAliveFlag( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_BOOL( IsAlive() );
}

void CActor::funcSetAlive( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, flag, false )
	FINISH_PARAMETERS;
	SetAlive( flag );
}

void CActor::funcIsExternalyControlled( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_BOOL( IsExternalyControlled() );
}

void CActor::funcIsInCombat( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_BOOL( IsInCombat() );
}

void CActor::funcIsMoving( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_BOOL( IsMoving() );
}

void CActor::funcGetMoveDestination( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_STRUCT( Vector, GetMoveDestination() );
}

void CActor::funcGetPositionOrMoveDestination( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_STRUCT( Vector, GetPositionOrMoveDestination() );
}

void CActor::funcGetVisualDebug( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_OBJECT( m_visualDebug );
}

void CActor::funcGetVoicetag( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_NAME( GetVoiceTag() );
}

void CActor::funcGetSkeletonType( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	// Default to men
	Int32 ret = ST_Man;

	if ( m_template.IsValid() )
	{
		// Find parameter
		CAnimGlobalParam* animParam = m_template->FindParameter< CAnimGlobalParam >( true );
		if( animParam != NULL )
		{
			ret = animParam->GetSkeletonType();
		}
	}

	RETURN_INT( ret );
}

void CActor::funcGetFallTauntEvent( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	CacheSoundParams();

	// Default - empty
	String ret;

	if( m_cachedSoundParams != NULL )
	{
		ret = m_cachedSoundParams->GetFallTauntEvent();
	}

	RETURN_STRING( ret );
}

void CActor::funcSetBehaviorMimicVariable( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, varName, CName::NONE );
	GET_PARAMETER( Float, var, 0.0f );
	FINISH_PARAMETERS;

	RETURN_BOOL( SetMimicVariable( varName, var ) );
}

void CActor::funcRaiseBehaviorMimicEvent( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, evtName, CName::NONE );
	FINISH_PARAMETERS;

	Bool ret( false );

	CMimicComponent* m = GetMimicComponent();
	if ( m && m->GetBehaviorStack() )
	{
		ret = m->GetBehaviorStack()->GenerateBehaviorEvent( evtName );
	}

	RETURN_BOOL( ret );
}

void CActor::funcSetLookAtMode( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( ELookAtMode, mode, LM_Dialog );
	FINISH_PARAMETERS;

	SetLookAtMode( mode );
}

void CActor::funcResetLookAtMode( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( ELookAtMode, mode, LM_Dialog );
	FINISH_PARAMETERS;

	ResetLookAtMode( mode );
}

void CActor::funcGetVisibility( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_BOOL( !IsHiddenInGame() );
}

void CActor::funcSetVisibility( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, isVisible, true );
	FINISH_PARAMETERS;

	SetHideInGame( !isVisible, false, HR_Scripts );
}

void CActor::funcSetAppearance( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, appearanceName, CName::NONE );
	FINISH_PARAMETERS;

	ApplyAppearance( appearanceName );
}

void CActor::funcGetAppearance( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_NAME( GetAppearance() );
}

void CActor::funcGetHeadAngleHorizontal( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	Float headHorAngle = 0.f;

	if ( GetMovingAgentComponent() )
	{
		const ISkeletonDataProvider* provider = GetMovingAgentComponent()->QuerySkeletonDataProvider();
		if ( provider )
		{
			Int32 boneIndex = provider->FindBoneByName( TXT("head") );
			if ( boneIndex >= 0 )
			{
				Matrix headWorldMatrix = provider->GetBoneMatrixWorldSpace( boneIndex );
				headHorAngle = headWorldMatrix.ToEulerAngles().Pitch;
			}
		}
	}

	RETURN_FLOAT( headHorAngle );
}

void CActor::funcGetHeadAngleVertical( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	Float headHorAngle = 0.f;

	if ( GetMovingAgentComponent() )
	{
		const ISkeletonDataProvider* provider = GetMovingAgentComponent()->QuerySkeletonDataProvider();
		if ( provider )
		{
			Int32 boneIndex = provider->FindBoneByName( TXT("head") );
			if ( boneIndex >= 0 )
			{
				Matrix headWorldMatrix = provider->GetBoneMatrixWorldSpace( boneIndex );
				headHorAngle = headWorldMatrix.ToEulerAngles().Yaw;
			}
		}
	}

	RETURN_FLOAT( headHorAngle );
}

void CActor::funcGetMovingAgentComponent( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_OBJECT( GetMovingAgentComponent() );
}

void CActor::funcGetMorphedMeshManagerComponent( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_OBJECT( GetMorphedMeshManagerComponent() );
}

void CActor::funcEnablePathEngineAgent( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, flag, false );
	FINISH_PARAMETERS;

	if ( CMovingAgentComponent* mac = GetMovingAgentComponent() )
	{
		mac->SetMotionEnabled( flag );
	}
}

void CActor::funcIsRagdollObstacle( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	bool isRagdollObstacle = IsRagdollObstacle();

	RETURN_BOOL( isRagdollObstacle );
}

void CActor::funcClearRotationTarget( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	ClearRotationTarget();
}

void CActor::funcSetRotationTarget( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CNode >, node, NULL );
	GET_PARAMETER_OPT( Bool, clamping, true );
	FINISH_PARAMETERS;
	SetRotationTarget( node, clamping );
}

void CActor::funcSetRotationTargetPos( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, pos, Vector::ZEROS );
	GET_PARAMETER_OPT( Bool, clamping, true );
	FINISH_PARAMETERS;
	SetRotationTarget( pos, clamping );
}

void CActor::funcEnableCollisionInfoReportingForItem( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SItemUniqueId, id, SItemUniqueId::INVALID );
	GET_PARAMETER( Bool, enable, false );
	FINISH_PARAMETERS;

	CInventoryComponent *inventory = GetInventoryComponent();
	if ( !inventory )
	{
		SCRIPT_WARNING( stack, TXT( "No inventory component in actor '%" ) RED_PRIWs TXT( "'" ), GetFriendlyName().AsChar() );
		return;
	}

	SInventoryItem *item = inventory->GetItem( id );
	if ( !item )
	{
		SCRIPT_WARNING( stack, TXT( "Actor '%" ) RED_PRIWs TXT( "' has no item of id '%ld'"), GetFriendlyName().AsChar(), id.GetValue() );
		return;
	}

	CItemEntityProxy *itemEntityProxy = item->GetItemEntityProxy();
	if ( !itemEntityProxy )
	{
		SCRIPT_WARNING( stack, TXT( "Actor '%" ) RED_PRIWs TXT( "' has item of id '%ld', but the item has no entity." ), GetFriendlyName().AsChar(), id.GetValue() );
		return;
	}
	SItemEntityManager::GetInstance().EnableCollisionInfoReporting( itemEntityProxy );
}

void CActor::funcEnablePhysicalMovement( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, enable, false );
	FINISH_PARAMETERS;

	CMovingAgentComponent *agent = GetMovingAgentComponent();
	if ( !agent || !agent->IsA< CMovingPhysicalAgentComponent >() )
	{
		SCRIPT_WARNING( stack, TXT( "No CMovingPhysicalAgentComponent in actor '%" ) RED_PRIWs TXT( "'" ), GetFriendlyName().AsChar() );
		return;
	}

	CMovingPhysicalAgentComponent *physicalAgent = Cast< CMovingPhysicalAgentComponent > ( agent );
	physicalAgent->SetPhysicalRepresentationRequest( enable ? CMovingAgentComponent::Req_On : CMovingAgentComponent::Req_Off, CMovingAgentComponent::LS_Scripts );
	physicalAgent->ResetMoveRepresentation(); // resolve request to return the correct result below

	RETURN_BOOL( physicalAgent->IsPhysicalRepresentationEnabled() );
}

void CActor::funcEnableCharacterCollisions( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, enable, false );
	FINISH_PARAMETERS;

	CMovingAgentComponent* agent = GetMovingAgentComponent();
	if ( !agent || !agent->IsA< CMovingPhysicalAgentComponent >() )
	{
		SCRIPT_WARNING( stack, TXT( "No CMovingPhysicalAgentComponent in actor '%" ) RED_PRIWs TXT( "'" ), GetFriendlyName().AsChar() );
		return;
	}

	CMovingPhysicalAgentComponent* physicalAgent = Cast< CMovingPhysicalAgentComponent > ( agent );
	physicalAgent->EnableCharacterCollisions( enable );
}

void CActor::funcEnableStaticCollisions( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, enable, false );
	FINISH_PARAMETERS;

	CMovingAgentComponent* agent = GetMovingAgentComponent();
	if ( !agent || !agent->IsA< CMovingPhysicalAgentComponent >() )
	{
		SCRIPT_WARNING( stack, TXT( "No CMovingPhysicalAgentComponent in actor '%" ) RED_PRIWs TXT( "'" ), GetFriendlyName().AsChar() );
		return;
	}

	CMovingPhysicalAgentComponent* physicalAgent = Cast< CMovingPhysicalAgentComponent > ( agent );
	physicalAgent->EnableStaticCollisions( enable );
}

void CActor::funcEnableDynamicCollisions( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, enable, false );
	FINISH_PARAMETERS;

	CMovingAgentComponent* agent = GetMovingAgentComponent();
	if ( !agent || !agent->IsA< CMovingPhysicalAgentComponent >() )
	{
		SCRIPT_WARNING( stack, TXT( "No CMovingPhysicalAgentComponent in actor '%" ) RED_PRIWs TXT( "'" ), GetFriendlyName().AsChar() );
		return;
	}

	CMovingPhysicalAgentComponent* physicalAgent = Cast< CMovingPhysicalAgentComponent > ( agent );
	physicalAgent->EnableDynamicCollisions( enable );
}

void CActor::funcIsCharacterCollisionsEnabled( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	CMovingAgentComponent* agent = GetMovingAgentComponent();
	if ( !agent || !agent->IsA< CMovingPhysicalAgentComponent >() )
	{
		SCRIPT_WARNING( stack, TXT( "No CMovingPhysicalAgentComponent in actor '%" ) RED_PRIWs TXT( "'" ), GetFriendlyName().AsChar() );
		return;
	}

	CMovingPhysicalAgentComponent* physicalAgent = Cast< CMovingPhysicalAgentComponent > ( agent );
	RETURN_BOOL( physicalAgent->IsCharacterCollisionsEnabled() );
}

void CActor::funcIsStaticCollisionsEnabled( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	CMovingAgentComponent* agent = GetMovingAgentComponent();
	if ( !agent || !agent->IsA< CMovingPhysicalAgentComponent >() )
	{
		SCRIPT_WARNING( stack, TXT( "No CMovingPhysicalAgentComponent in actor '%" ) RED_PRIWs TXT( "'" ), GetFriendlyName().AsChar() );
		return;
	}

	CMovingPhysicalAgentComponent* physicalAgent = Cast< CMovingPhysicalAgentComponent > ( agent );
	RETURN_BOOL( physicalAgent->IsStaticCollisionsEnabled() );
}

void CActor::funcIsDynamicCollisionsEnabled( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	CMovingAgentComponent* agent = GetMovingAgentComponent();
	if ( !agent || !agent->IsA< CMovingPhysicalAgentComponent >() )
	{
		SCRIPT_WARNING( stack, TXT( "No CMovingPhysicalAgentComponent in actor '%" ) RED_PRIWs TXT( "'" ), GetFriendlyName().AsChar() );
		return;
	}

	CMovingPhysicalAgentComponent* physicalAgent = Cast< CMovingPhysicalAgentComponent > ( agent );
	RETURN_BOOL( physicalAgent->IsDynamicCollisionsEnabled() );
}

// ----------------------------------------------------------------------------
// Scripting support for latent actions
// ----------------------------------------------------------------------------

void CActor::funcIsReadyForNewAction( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_BOOL( IsReadyForNewAction() );
}

void CActor::funcActionCancelAll( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	if( CanPerformActionFromScript( stack ) )
	{
		ActionCancelAll();
	}
}

void CActor::funcGetCurrentActionPriority( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_INT( GetCurrentActionPriority() );
}

void CActor::funcIsDoingSomethingMoreImportant( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int32, priority, 0 );
	FINISH_PARAMETERS;

	Bool ret = IsDoingSomethingMoreImportant( priority );
	RETURN_BOOL( ret );
}

void CActor::funcIsCurrentActionInProgress( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	Bool res = m_latentActionResult == ActorActionResult_InProgress;
	RETURN_BOOL( res );
}

void CActor::funcIsCurrentActionSucceded( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	Bool res = m_latentActionResult == ActorActionResult_Succeeded;
	RETURN_BOOL( res );
}

void CActor::funcIsCurrentActionFailed( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	Bool res = m_latentActionResult == ActorActionResult_Failed;
	RETURN_BOOL( res );
}

void CActor::funcGetAnimationTimeMultiplier( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	Float var = GetAnimationTimeMultiplier();
	RETURN_FLOAT( var );
}

void CActor::funcSetAnimationTimeMultiplier( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, mult, 1.f );
	FINISH_PARAMETERS;
	SetAnimationTimeMultiplier( mult );
}



void CActor::funcUseItem( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SItemUniqueId, itemId, SItemUniqueId::INVALID );
	FINISH_PARAMETERS;

	RETURN_BOOL( UseItem( itemId ) );
}

void CActor::funcIsInNonGameplayScene( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_BOOL( m_isInNonGameplayScene );
}

void CActor::funcIsInGameplayScene( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_BOOL( m_isInGameplayScene );
}

void CActor::funcPlayScene( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, sceneInput, String::EMPTY );
	FINISH_PARAMETERS;

	if ( m_tags.Empty() == true || GCommonGame->GetSystem< CQuestsSystem >() == NULL )
	{
		RETURN_BOOL( false );
		return;
	}

	CName actorTag = m_tags.GetTags()[ 0 ];
	Bool sceneWasPlayed = GCommonGame->GetSystem< CQuestsSystem >()->GetScriptedDialogPlayer()
		->StartDialog( actorTag, sceneInput );

	RETURN_BOOL(  sceneWasPlayed );
}

void CActor::funcStopAllScenes( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	StopAllScenes( SCR_STOPPED );
}

void CActor::funcEmptyHands( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	EmptyHands();
}

void CActor::funcPlayLine( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Uint32, lineId, 0 );
	GET_PARAMETER( Bool, asSubtitle, false );
	FINISH_PARAMETERS;
	Int32 flags = ASM_Text | ASM_Voice | ASM_Lipsync;
	if ( asSubtitle == true )
	{
		flags |= ASM_Subtitle;
	}
	ActorSpeechData data( lineId, StringAnsi(), false, flags );
	SpeakLine( data );
}

void CActor::funcPlayLineByStringKey( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, stringKey, String::EMPTY );
	GET_PARAMETER( Bool, asSubtitle, false );
	FINISH_PARAMETERS;
	Int32 flags = ASM_Text | ASM_Voice | ASM_Lipsync;
	if ( asSubtitle == true )
	{
		flags |= ASM_Subtitle;
	}
	ActorSpeechData data( stringKey, StringAnsi(), false, flags );
	SpeakLine( data );
}

void CActor::funcEndLine( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	CancelSpeech();
}

void CActor::funcIsSpeaking( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_OPT( Uint32, stringId, 0 );
	FINISH_PARAMETERS;

	RETURN_BOOL( IsSpeaking( stringId ) );
}

void CActor::funcEnableDynamicLookAt( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CNode >, nodeHandler, NULL );
	GET_PARAMETER( Float, duration, 10.0f );
	FINISH_PARAMETERS;

	CNode* node = nodeHandler.Get();
	if ( node )
	{
		EnableDynamicLookAt( node, duration );
	}
}

void CActor::funcEnableStaticLookAt( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, pos, Vector() );
	GET_PARAMETER_OPT( Float, duration, 10.0f );
	FINISH_PARAMETERS;

	SLookAtScriptStaticInfo info;
	info.m_duration = duration;
	info.m_target = pos;

	EnableLookAt( info );
}

void CActor::funcDisableLookAt( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	m_lookAtController.SetNoScriptLookAts();
}

void CActor::funcCutBodyPart( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, prefixA, String::EMPTY );
	GET_PARAMETER( String, prefixB, String::EMPTY );
	GET_PARAMETER( CName, boneName, CName::NONE );
	FINISH_PARAMETERS;

//TODO: Not implemented

	Bool rVal = false;
	RETURN_BOOL( rVal );
}

void CActor::funcIsAttackableByPlayer( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_BOOL( IsAttackableByPlayer() );
}

void CActor::funcSetAttackableByPlayerPersistent( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, flag, true )
	FINISH_PARAMETERS;
	SetAttackableByPlayerPersistent( flag );
}

void CActor::funcSetAttackableByPlayerRuntime( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, flag, true );
	GET_PARAMETER_OPT( Float, timeout, 10.0f );
	FINISH_PARAMETERS;
	SetAttackableByPlayerRuntime( flag, timeout );
}

void CActor::funcInAttackRange( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CGameplayEntity >, entity, NULL );
	GET_PARAMETER_OPT( CName, rangeName, CName::NONE );
	FINISH_PARAMETERS;

	CGameplayEntity* pEnt = entity.Get();
	if( pEnt )
	{
		RETURN_BOOL( InAttackRange( pEnt, rangeName ) );
	}
	else
	{
		RETURN_BOOL( false );
	}
}

Vector CActor::GetNearestPointInPersonalSpaceAt(const Vector& myPosition, const Vector& otherPosition) const
{
	Vector vecToRefPos = otherPosition - myPosition;
	vecToRefPos.Z = 0.0f;

	Float personalSpaceRadius = 0.4f;

	CMovingPhysicalAgentComponent* mac = Cast< CMovingPhysicalAgentComponent >( GetMovingAgentComponent() );
	if ( mac )
	{
		const CMRPhysicalCharacter* character = mac->GetPhysicalCharacter();
		if ( character )
		{
#ifdef USE_PHYSX
			CPhysicsCharacterWrapper* controller = character->GetCharacterController();
			if ( controller )
			{
#ifdef USE_PHYSX
				personalSpaceRadius = controller->GetVirtualRadius();
#endif
			}
#endif // USE_PHYSX
		}
	}
	
	Float vecToRefDist = vecToRefPos.Mag2();
	if( vecToRefDist < personalSpaceRadius )
	{
		Vector nearestPos = otherPosition;
		nearestPos.Z = myPosition.Z;
		return nearestPos;
	}
	else
	{
		Vector nearestPos = myPosition + vecToRefPos * personalSpaceRadius / vecToRefDist;
		return nearestPos;
	}
}

void CActor::funcGetNearestPointInPersonalSpace( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, position, Vector::ZEROS );
	FINISH_PARAMETERS;

	RETURN_STRUCT( Vector, GetNearestPointInPersonalSpaceAt( GetWorldPosition(), position ) );
}

void CActor::funcGetNearestPointInPersonalSpaceAt( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, myPosition, Vector::ZEROS );
	GET_PARAMETER( Vector, otherPosition, Vector::ZEROS );
	FINISH_PARAMETERS;

	RETURN_STRUCT( Vector, GetNearestPointInPersonalSpaceAt( myPosition, otherPosition ) );
}

void CActor::funcGatherEntitiesInAttackRange( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( TDynArray< THandle< CGameplayEntity > >, output, TDynArray< THandle< CGameplayEntity > > () );
	GET_PARAMETER_OPT( CName, rangeName, CName::NONE );
	FINISH_PARAMETERS;

	const CAIAttackRange* attackRange = CAIAttackRange::Get( this, rangeName );
	if ( attackRange )
	{
		attackRange->GatherEntities( this, output );
	}
}

void CActor::funcGetCurrentActionType( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_INT( GetCurrentActionType() );
}

void CActor::funcSetErrorState( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, description, String::EMPTY );
	FINISH_PARAMETERS;

#ifndef NO_ERROR_STATE
	SetErrorState( description );
#endif
}

void CActor::funcCalculateHeight( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	Float returnValue( 0.f );

#ifdef USE_HAVOK
	CMovingAgentComponent* mac = GetMovingAgentComponent();
	if ( mac )
	{
		Vector point;
		if ( mac->GetRagdollInstance() && mac->GetRagdollInstance()->GetCurrentState() == RS_Falling )
		{
			point = TO_CONST_VECTOR_REF( mac->GetRagdollInstance()->GetRigidBody( 0 )->getPosition() );
		}
		else
		{
			point = GetWorldPosition();
		}

		returnValue = CTraceTool::StaticPointProjectionTest( GetLayer()->GetWorld(), point, 1000.f );
	}
#endif

	RETURN_FLOAT( returnValue );
}

void CActor::funcPlayMimicAnimationAsync( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, animation, CName::NONE );
	FINISH_PARAMETERS;

	RETURN_BOOL( PlayMimicAnimation( animation, CNAME( MIMIC_SLOT ) ) );
}


void CActor::funcWasVisibleLastFrame( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_BOOL( WasVisibleLastFrame() );
}

void CActor::funcPushInDirection( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, pusherPos, Vector( 0.0f, 0.0f, 0.0f ) );
	GET_PARAMETER( Vector, pushDir, Vector( 0.0f, 0.0f, 0.0f ) );
	GET_PARAMETER_OPT( Float, speed, 5.0f );
	GET_PARAMETER_OPT( Bool, playAnimation, true );
	GET_PARAMETER_OPT( Bool, applyRotation, true );
	FINISH_PARAMETERS;

	PushInDirection( pusherPos, pushDir, speed, playAnimation, applyRotation );
}

void CActor::funcPushAway( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CMovingAgentComponent >, hPusher, NULL );
	GET_PARAMETER_OPT( Float, strength, 1.0f );
	GET_PARAMETER_OPT( Float, speed, 5.0f );
	FINISH_PARAMETERS;

	CMovingAgentComponent* pusher = hPusher.Get();
	CMovingAgentComponent* myMac = GetMovingAgentComponent();
	if ( !pusher || !myMac || myMac->GetAbsoluteMoveSpeed() > 0.0f  )
	{
		return;
	}

	// calculate the pushing factors
	Vector pusherPos = pusher->GetWorldPosition();
	Vector myPos = myMac->GetWorldPosition();
	Vector pusherHeading = pusher->GetVelocity().Normalized2();
	Vector dirFromPusher = myPos - pusherPos;
	Float pushAngle = Vector::Dot2( pusherHeading, dirFromPusher );
	Vector castPos = pusherPos + pusherHeading * pushAngle;

	// if we're traveling at 5 [m/s], then let's assume that we can
	//change the pushed agent's position by 1 [m]
	Float pushStrength = pusher->GetAbsoluteMoveSpeed() / 5.0f;
	pushStrength *= strength; // take a push strength factor into account
	Vector pushDir = ( myPos - castPos ).Normalized2() * pushStrength;

	// play the animation only if the push strength
	// will inflict a push that will significantly change the agent's position
	Bool playAnimation = ( pushStrength >= 0.3f );
	PushInDirection( pusherPos, pushDir, speed, playAnimation );
}

void CActor::funcCanPlayQuestScene( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_BOOL( CanPlayQuestScene() );
}

void CActor::funcHasInteractionScene( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_BOOL( HasInteractionScene() );
}

void CActor::funcCanTalk( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_OPT( Bool, ignoreCurrentSpeech, false );
	FINISH_PARAMETERS;
	RETURN_BOOL( CanTalk( ignoreCurrentSpeech ) );
}

void CActor::funcGetActorAnimState( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_INT( GetActorAnimState() );
}

void CActor::funcIsInView( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	ASSERT( GGame->GetActiveWorld() );
	const Bool isPointInCamera = GGame->GetActiveWorld()->GetCameraDirector()->IsPointInView( GetWorldPosition(), 1.1f );
	RETURN_BOOL( isPointInCamera );
}

void CActor::funcSetMovementType( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( EExplorationState, state, EX_Normal );
	FINISH_PARAMETERS;

	Bool res = SetMovementType( state );
	RETURN_BOOL( res );
}

void CActor::funcGetMovementType( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_INT( m_movementType );
}

void CActor::funcHasLatentItemAction( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_BOOL( HasLatentItemAction() );
}

extern Bool GLatentFunctionStart;

void CActor::funcWaitForFinishedAllLatentItemActions( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	ASSERT( stack.m_thread );

	if ( !HasLatentItemAction() )
	{
		RETURN_BOOL( true );
		return;
	}

	const Float waitedTime = stack.m_thread->GetCurrentLatentTime();
	if ( waitedTime <= 10.f )
	{
		stack.m_thread->ForceYield();
		return;
	}

	RETURN_BOOL( false );
}

void CActor::funcDrawItems( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool			, instant	 , false );
	GET_PARAMETER( SItemUniqueId, itemId	 , SItemUniqueId::INVALID );
	GET_PARAMETER_OPT( SItemUniqueId, itemId2, SItemUniqueId::INVALID );
	GET_PARAMETER_OPT( SItemUniqueId, itemId3, SItemUniqueId::INVALID );
	FINISH_PARAMETERS;

	RETURN_BOOL (	!DrawItem( itemId, instant )
		&&  ( itemId2 == SItemUniqueId::INVALID || ! DrawItem( itemId2, instant ) )
		&&  ( itemId3 == SItemUniqueId::INVALID || ! DrawItem( itemId3, instant ) ) )
}

void CActor::funcHolsterItems( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool			, instant	 , false );
	GET_PARAMETER( SItemUniqueId, itemId	 , SItemUniqueId::INVALID );
	GET_PARAMETER_OPT( SItemUniqueId, itemId2, SItemUniqueId::INVALID );
	GET_PARAMETER_OPT( SItemUniqueId, itemId3, SItemUniqueId::INVALID );
	FINISH_PARAMETERS;

	RETURN_BOOL (	!HolsterItem( itemId, instant )
		&&  ( itemId2 == SItemUniqueId::INVALID || ! HolsterItem( itemId2, instant ) )
		&&  ( itemId3 == SItemUniqueId::INVALID || ! HolsterItem( itemId3, instant ) ) );
}

void CActor::funcHolsterItemsLatent( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SItemUniqueId, itemId	 , SItemUniqueId::INVALID );
	GET_PARAMETER_OPT( SItemUniqueId, itemId2, SItemUniqueId::INVALID );
	GET_PARAMETER_OPT( SItemUniqueId, itemId3, SItemUniqueId::INVALID );
	FINISH_PARAMETERS;
	ASSERT( stack.m_thread );

	if ( GLatentFunctionStart )
	{
		if (	!HolsterItem( itemId, false )
			&&  ( itemId2 == SItemUniqueId::INVALID || ! HolsterItem( itemId2, false ) )
			&&  ( itemId3 == SItemUniqueId::INVALID || ! HolsterItem( itemId3, false ) ) )
		{
			RETURN_BOOL( false );
			return;
		}
	}

	CItemEntityManager& itemEntityManager = SItemEntityManager::GetInstance();
	if( itemEntityManager.HasActorLatentActionForItem( this, itemId ) ||
		itemEntityManager.HasActorQueuedLatentActionForItem( this, itemId ) )
	{
		stack.m_thread->ForceYield();
		return;
	}

	RETURN_BOOL( true );
}

void CActor::funcDrawItemsLatent( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SItemUniqueId, itemId	 , SItemUniqueId::INVALID );
	GET_PARAMETER_OPT( SItemUniqueId, itemId2, SItemUniqueId::INVALID );
	GET_PARAMETER_OPT( SItemUniqueId, itemId3, SItemUniqueId::INVALID );
	FINISH_PARAMETERS;
	ASSERT( stack.m_thread );

	if ( GLatentFunctionStart )
	{
		if (	!DrawItem( itemId, false )
			&&  ( itemId2 == SItemUniqueId::INVALID || ! DrawItem( itemId2, false ) )
			&&  ( itemId3 == SItemUniqueId::INVALID || ! DrawItem( itemId3, false ) ) )
		{
			RETURN_BOOL( false );
			return;
		}
	}

	CItemEntityManager& itemEntityManager = SItemEntityManager::GetInstance();
	if( itemEntityManager.HasActorLatentActionForItem( this, itemId ) ||
		itemEntityManager.HasActorQueuedLatentActionForItem( this, itemId ) )
	{
		stack.m_thread->ForceYield();
		return;
	}

	RETURN_BOOL( true );
}

void CActor::funcDrawWeaponAndAttackLatent( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SItemUniqueId, itemId, SItemUniqueId::INVALID );
	FINISH_PARAMETERS;

	ASSERT( stack.m_thread );

	// Starting
	if ( GLatentFunctionStart )
	{
		if ( !DrawWeaponAndAttack( itemId ) )
		{
			RETURN_BOOL( false );
			return;
		}
	}

	if ( SItemEntityManager::GetInstance().HasActorLatentActionForItem( this, itemId ) )
	{
		stack.m_thread->ForceYield();
		return;
	}

	RETURN_BOOL( true );
}

void CActor::funcIssueRequiredItems( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, leftItem, CName::NONE );
	GET_PARAMETER( CName, rightItem, CName::NONE );
	FINISH_PARAMETERS;

	SActorRequiredItems info( leftItem, rightItem );
	IssueRequiredItems( info );
}

void CActor::funcSetRequiredItems( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, leftItem, CName::NONE );
	GET_PARAMETER( CName, rightItem, CName::NONE );
	FINISH_PARAMETERS;

	SActorRequiredItems info( leftItem, rightItem );
	SetRequiredItems( info );
}


void CActor::funcIssueRequiredItemsGeneric( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( TDynArray< CName >, items, TDynArray< CName >() );
	GET_PARAMETER_REF( TDynArray< CName >, slots, TDynArray< CName >() );
	FINISH_PARAMETERS;
	ASSERT( items.Size() == slots.Size() );
		SActorRequiredItems info;
	Int32 size = items.Size();
	for( Int32 i = 0 ; i < size ; ++i )
	{
		info.m_slotAndItem.PushBack( TPair<CName,CName>(slots[i], items[i]) );
	}
	IssueRequiredItems( info );
}

void CActor::funcSetRequiredItemsGeneric( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( TDynArray< CName >, items, TDynArray< CName >() );
	GET_PARAMETER_REF( TDynArray< CName >, slots, TDynArray< CName >() );
	FINISH_PARAMETERS;
	ASSERT( items.Size() == slots.Size() );
		SActorRequiredItems info;
	Int32 size = items.Size();
	for( Int32 i = 0 ; i < size ; ++i )
	{
		info.m_slotAndItem.PushBack( TPair<CName,CName>(slots[i], items[i]) );
	}
	SetRequiredItems( info );
}



void CActor::funcProcessRequiredItems( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_OPT( Bool, instant, false );
	FINISH_PARAMETERS;

	ASSERT( stack.m_thread );

	// Starting
	if ( GLatentFunctionStart )
	{
		ProcessRequiredItemsState( instant );
	}

	if ( SItemEntityManager::GetInstance().HasActorAnyLatentAction( this ) )
	{
		stack.m_thread->ForceYield();
		return;
	}
}

void CActor::funcPlayPushAnimation( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( EPushingDirection, pushDirection, EPD_Front );
	FINISH_PARAMETERS;

	PlayPushAnimation( pushDirection );
}

void CActor::funcForceAIBehavior( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< IAITree >, tree, NULL );
	GET_PARAMETER( EArbitratorPriorities, priority, BTAP_Unavailable );
	GET_PARAMETER_OPT( CName, forceEventName, SForcedBehaviorEventData::GetEventName() );
	FINISH_PARAMETERS;
	Bool ret = false;
	IAITree* def = tree.Get();
	Int16 forceActionId = -1;
	if ( def )
	{
		ret = ForceAIBehavior( def, IBehTreeNodeDefinition::Priority(priority), &forceActionId, forceEventName );
	}
	RETURN_INT( (Int32)forceActionId );
}

void CActor::funcCancelAIBehavior( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int32, forceActionId, -1 );
	FINISH_PARAMETERS;
	Bool ret = false;

	if ( forceActionId != -1 )
	{
		ret = CancelAIBehavior( (Int16)forceActionId );
	}
	RETURN_BOOL( ret );
}

void CActor::funcSetDynamicBehavior( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< IAITree >, treeHandle, NULL );
	GET_PARAMETER( CName, eventName, CName::NONE );
	GET_PARAMETER( Bool, interrupt, false );
	FINISH_PARAMETERS;

	Bool ret;
	IAITree* tree = treeHandle.Get();
	if ( tree )
	{
		ret = ForceDynamicBehavior( tree, eventName, interrupt );
	}
	else
	{
		ret = CancelDynamicBehavior( eventName, interrupt );
	}
	RETURN_BOOL( ret );
}

void CActor::funcActivateAndSyncBehaviorWithItemsParallel( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, name, CName::NONE );
	GET_PARAMETER_OPT( Float, timeout, 10.f );
	FINISH_PARAMETERS;

	ASSERT( stack.m_thread );

	ACTION_START_TEST;

	CAnimatedComponent* ac = GetRootAnimatedComponent();
	CBehaviorGraphStack* behStack = ac ? ac->GetBehaviorStack() : NULL;

	if ( behStack )
	{
		if ( behStack->IsGraphAvailable(name) )
		{
			RETURN_BOOL( false );
			return;
		}

		if ( behStack->ActivateAndSyncBehaviorInstances( name ) )
		{
			ProcessRequiredItemsState();
			BEH_LOG( TXT("CActor:ActivateAndSyncBehaviorWithItemsParallel - Waiting for behavior synchronization and processing items in parallel... - '%ls'"), GetName().AsChar() );
		}
		else
		{
			// Default
			Bool ret = behStack->ActivateBehaviorInstances( name );

			String resStr = ret ? TXT("Success") : TXT("Failure");

			ProcessRequiredItemsState();
			BEH_LOG( TXT("CActor:ActivateAndSyncBehaviorWithItemsParallel - Couldn't synchronize stack. Default activation was processed - '%ls'. Processing items started... - '%ls'"), resStr.AsChar(), GetName().AsChar() );
		}


		if ( !behStack->IsSynchronizing() && !HasLatentItemAction() )
		{
			LOG_GAME( TXT("CActor:ActivateAndSyncBehaviorWithItemsParallel - Finished - '%ls'"), GetName().AsChar() );
			RETURN_BOOL( true ); // Warning: True is returned even if instant ActivateBehaviorInstane call above failed. I let that happen as returned value is not used anywhere but be warned.
			return;
		}

		// Timeout
		const Float waitedTime = stack.m_thread->GetCurrentLatentTime();
		if ( waitedTime <= timeout )
		{
			stack.m_thread->ForceYield();
			return;
		}

		BEH_LOG( TXT("CActor:ActivateAndSyncBehaviorWithItemsParallel - Timeout '%f' - '%ls'"), timeout, GetName().AsChar() );
	}

	LOG_GAME( TXT("CActor:ActivateAndSyncBehaviorWithItemsParallel - Fail - '%ls'"), GetName().AsChar() );
	RETURN_BOOL( false );
}

void CActor::funcActivateAndSyncBehaviorWithItemsSequence( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, name, CName::NONE );
	GET_PARAMETER_OPT( Float, timeout, 10.f );
	FINISH_PARAMETERS;

	ASSERT( stack.m_thread );

	ACTION_START_TEST;

	CAnimatedComponent* ac = GetRootAnimatedComponent();
	CBehaviorGraphStack* behStack = ac ? ac->GetBehaviorStack() : NULL;

	if ( behStack )
	{		
		if ( behStack->IsGraphAvailable( name ) )
		{
			RETURN_BOOL( false );
			return;
		}

		if ( behStack->ActivateAndSyncBehaviorInstances( name ) )
		{
			BEH_LOG( TXT("CActor:ActivateAndSyncBehaviorWithItemsSequence - Waiting for behavior synchronization... - '%ls'"), GetName().AsChar() );
		}
		else
		{
			// Default
			Bool ret = behStack->ActivateBehaviorInstances( name );

			String resStr = ret ? TXT("Success") : TXT("Failure");

			ProcessRequiredItemsState();
			BEH_LOG( TXT("CActor:ActivateAndSyncBehaviorWithItemsSequence - Couldn't synchronize stack. Default activation was processed - '%ls' - '%ls'. Processing required items now."), resStr.AsChar(), GetName().AsChar() );
		}

		if ( !behStack->IsSynchronizing() && !HasLatentItemAction() )
		{
			// Synchronizing ended. Items handling finished or didn't start yet
			ProcessRequiredItemsState();

			if ( !HasLatentItemAction() )
			{
				// Items processing finished as well
				LOG_GAME( TXT("CActor:ActivateAndSyncBehaviorWithItemsSequence - Finished - '%ls'"), GetName().AsChar() );
				RETURN_BOOL( true );
				return;
			}
			else
			{
				// Starting items processing
				LOG_GAME( TXT("CActor:ActivateAndSyncBehaviorWithItemsSequence - Finished synchronizing behavior. Started processing required items - '%ls'"), GetName().AsChar() );
			}
		}

		// Timeout
		const Float waitedTime = stack.m_thread->GetCurrentLatentTime();
		if ( waitedTime <= timeout )
		{
			stack.m_thread->ForceYield();
			return;
		}

		BEH_LOG( TXT("CActor:ActivateAndSyncBehaviorWithItemsSequence - Timeout '%f' - '%ls'"), timeout, GetName().AsChar() );
	}

	LOG_GAME( TXT("CActor:ActivateAndSyncBehaviorWithItemsSequence - Fail - '%ls'"), GetName().AsChar() );
	RETURN_BOOL( false );
}

void CActor::funcIsUsingExploration( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_BOOL( IsUsingExploration() );
}

void CActor::funcGetAnimCombatSlots( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, animSlotName, CName::NONE );
	GET_PARAMETER_REF( TDynArray< Matrix >, slots, TDynArray< Matrix >() );
	GET_PARAMETER( Uint32, slotsNum, 0 );
	GET_PARAMETER( Matrix, mainEnemyMatrix, Matrix::IDENTITY );
	FINISH_PARAMETERS;

	Bool ret = GetAnimCombatSlots( animSlotName, slots, slotsNum, mainEnemyMatrix );

	RETURN_BOOL( ret );
}

void CActor::funcIsAttackedBy( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CActor >, actor, NULL );
	FINISH_PARAMETERS;

	Bool ret = false;
	CActor* a = actor.Get();
	if ( a )
	{
		ret = IsAttackedBy( a );
	}

	RETURN_BOOL( ret );
}

void CActor::funcRegisterAttacker( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, registration, true );
	GET_PARAMETER( THandle< CActor >, actor, NULL );
	FINISH_PARAMETERS;

	RETURN_BOOL( RegisterAttacker( actor.Get(), registration ) );
}

void CActor::funcGetScriptStorageObject( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, objectName, CName::NONE );
	FINISH_PARAMETERS;

	IScriptable* obj = nullptr;

	if ( m_behTreeMachine )
	{
		if ( CBehTreeInstance* behTreeInstance = m_behTreeMachine->GetBehTreeInstance() )
		{
			obj = behTreeInstance->ScriptableStorageFindItem( objectName );
		}
	}

	RETURN_OBJECT( obj );
}

void CActor::funcGetCharacterStatsParam( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( TDynArray< CName >, effects, TDynArray< CName >() );
	FINISH_PARAMETERS;

	CEntityTemplate* temp = GetEntityTemplate();
	if(temp)
	{
		TDynArray<CGameplayEntityParam*> params;
		temp->CollectGameplayParams( params , CCharacterStatsParam::GetStaticClass() ) ;
		for( auto i = params.Begin() ; i!= params.End(); ++i  )
		{
			const TDynArray< CName >& abilitiesArray = Cast<CCharacterStatsParam>(*i)->GetAbilities();
			for ( auto j = abilitiesArray.Begin(); j != abilitiesArray.End(); ++j )
			{
				const CName& abilityName = *j;
				if ( abilityName != CName::NONE )
				{
					effects.PushBackUnique( abilityName );
				}
			}
		}
	}
}

void CActor::funcGetAutoEffects( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( TDynArray< CName >, effects, TDynArray< CName >() );
	FINISH_PARAMETERS;

	CEntityTemplate* temp = GetEntityTemplate();
	if(temp)
	{
		TDynArray<CGameplayEntityParam*> params;
		temp->CollectGameplayParams( params , CAutoEffectsParam::GetStaticClass() ) ;
		for( auto i = params.Begin() ; i!= params.End(); ++i  )
		{
			const TDynArray< CName >& effectsArray = Cast<CAutoEffectsParam>(*i)->GetAutoEffects();
			for ( auto j = effectsArray.Begin(); j != effectsArray.End(); ++j )
			{
				const CName& effectName = *j;
				if ( effectName != CName::NONE )
				{
					effects.PushBackUnique( effectName );
				}
			}
		}
	}
}

void CActor::funcSetInteractionPriority( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( InteractionPriorityType, priority, InteractionPriorityTypeZero );
	FINISH_PARAMETERS;

	InteractionPriorityType interactionPriority = priority;
	InteractionPriorityType previousPriority = InteractionPriorityTypeZero;

	CMovingPhysicalAgentComponent *mac = Cast< CMovingPhysicalAgentComponent > ( GetMovingAgentComponent() );
	if ( mac )
		previousPriority = mac->SetInteractionPriority( interactionPriority );
}

void CActor::funcSetOriginalInteractionPriority( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( InteractionPriorityType, priority, InteractionPriorityTypeZero );
	FINISH_PARAMETERS;

	CMovingPhysicalAgentComponent *mac = Cast< CMovingPhysicalAgentComponent > ( GetMovingAgentComponent() );
	if ( mac )
		mac->SetOriginalInteractionPriority( priority );
}

void CActor::funcRestoreOriginalInteractionPriority( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	CMovingPhysicalAgentComponent *mac = Cast< CMovingPhysicalAgentComponent > ( GetMovingAgentComponent() );
	if ( mac )
		mac->RestoreOriginalInteractionPriority();
}

void CActor::funcGetOriginalInteractionPriority( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	InteractionPriorityType originalPriority	= InteractionPriorityTypeZero;

	CMovingPhysicalAgentComponent *mac = Cast< CMovingPhysicalAgentComponent > ( GetMovingAgentComponent() );
	if ( mac )
		originalPriority	= mac->GetOriginalInteractionPriority();

	RETURN_STRUCT( InteractionPriorityType, originalPriority );
}

void CActor::funcGetInteractionPriority( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	CMovingPhysicalAgentComponent *mac = Cast< CMovingPhysicalAgentComponent > ( GetMovingAgentComponent() );
	InteractionPriorityType prio = InteractionPriorityTypeZero;
	if ( mac )
		prio = mac->GetInteractionPriority();

	RETURN_INT( (Int32)prio );
}

void CActor::funcSetUnpushableTarget( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CActor >, target, NULL );
	FINISH_PARAMETERS;

	CActor *actor( target.Get() ), *previousUnpushableTarget( NULL );
	CMovingPhysicalAgentComponent *mac = Cast< CMovingPhysicalAgentComponent > ( GetMovingAgentComponent() );
	if ( mac )
	{
		if ( actor )
		{
			CMovingPhysicalAgentComponent *targetMAC = Cast< CMovingPhysicalAgentComponent > ( actor->GetMovingAgentComponent() );
			if ( targetMAC )
			{
				CMovingPhysicalAgentComponent *prevMAC = mac->SetUnpushableTarget( targetMAC );
				if ( prevMAC )
				{
					previousUnpushableTarget = Cast< CActor > ( prevMAC->GetEntity() );
				}
			}
		}
		else
		{
			mac->SetUnpushableTarget( NULL );
		}
	}


	RETURN_OBJECT( previousUnpushableTarget );
}

void CActor::funcGetHeadBoneIndex( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_INT( GetHeadBone() );
}

void CActor::funcGetTorsoBoneIndex( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_INT( m_torsoBoneIndex );
}
void CActor::funcGetTarget( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_OBJECT( GetScriptTarget() );
}

void CActor::funcIsDangerous( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CActor >, actor, nullptr );
	FINISH_PARAMETERS;

	Bool ret = false;
	CActor* pActor = actor.Get();
	if ( pActor )
	{
		ret = IsDangerous( pActor );
	}

	RETURN_BOOL( ret );
}

void CActor::funcGetAttitude( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CActor >, actor, nullptr );
	FINISH_PARAMETERS;
	RETURN_INT( GetAttitude( actor.Get() ) );
}

void CActor::funcSetAttitude( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CActor >, actor, nullptr );
	GET_PARAMETER( EAIAttitude, att, CAttitudes::GetDefaultAttitude() );
	FINISH_PARAMETERS;
	CActor *pActor = actor.Get();
	if ( pActor != nullptr )
	{
		SetAttitude( pActor, att );
	}
	else
	{
		WARN_GAME( TXT("NULL actor in SetAttitude on actor %s"), GetName().AsChar() );
		SET_ERROR_STATE( this, TXT("NULL actor in SetAttitude") );
	}
}

void CActor::funcResetAttitude( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CActor >, actor, nullptr );
	FINISH_PARAMETERS;
	CActor *pActor = actor.Get();
	if ( pActor != nullptr )
	{
		ResetAttitude( pActor );
	}
	else
	{
		WARN_GAME( TXT("NULL actor in ResetAttitude on actor %s"), GetName().AsChar() );
		SET_ERROR_STATE( this, TXT("NULL actor in ResetAttitude") );
	}
}

void CActor::funcHasAttitudeTowards( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CActor >, actor, nullptr );
	FINISH_PARAMETERS;
	
	Bool res = false;
	
	CActor *pActor = actor.Get();
	if ( pActor != nullptr )
	{
		EAIAttitude attitude = CAttitudes::GetDefaultAttitude();
		CAttitudeManager *atMan = GCommonGame->GetSystem< CAttitudeManager >();
		RED_ASSERT( atMan != nullptr, TXT( "AttitudeManager is null") );
		res = atMan->GetActorAttitude( this, actor, attitude );
	}
	else
	{
		WARN_GAME( TXT("NULL actor in HasAttitudeTowards on actor %s"), GetName().AsChar() );
		SET_ERROR_STATE( this, TXT("NULL actor in HasAttitudeTowards") );
	}

	RETURN_BOOL( res );
}

void CActor::funcClearAttitudes( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, hostile, false );
	GET_PARAMETER( Bool, neutral, false );
	GET_PARAMETER( Bool, friendly, false );
	FINISH_PARAMETERS;
	ClearAttitudes( hostile, neutral, friendly );
}

void CActor::funcGetAttitudeGroup( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_NAME( GetAttitudeGroup() );
}
void CActor::funcGetBaseAttitudeGroup( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_NAME( GetBaseAttitudeGroup() );
}

void CActor::funcSetBaseAttitudeGroup( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, groupName, CName::NONE );
	FINISH_PARAMETERS;

	SetBaseAttitudeGroup( groupName );
}

void CActor::funcResetBaseAttitudeGroup( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	ResetBaseAttitudeGroup( true );
}

void CActor::funcSetTemporaryAttitudeGroup( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, groupName, CName::NONE );
	GET_PARAMETER( Int8, priority, 0 );
	FINISH_PARAMETERS;

	SetTemporaryAttitudeGroup( groupName, (Int32)priority );
}

void CActor::funcResetTemporaryAttitudeGroup( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int8, priority, 0 );
	FINISH_PARAMETERS;
	ResetTemporaryAttitudeGroup( (Int32)priority );
}

void CActor::funcSetAttitudeGroup( CScriptStackFrame& stack, void* result )
{
	// deprecated
	funcSetBaseAttitudeGroup( stack, result );
}

void CActor::funcResetAttitudeGroup( CScriptStackFrame& stack, void* result )
{
	// deprecated
	funcResetBaseAttitudeGroup( stack, result );
}

void CActor::funcCanStealOtherActor( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CActor >, actorHandle, nullptr );
	FINISH_PARAMETERS;
	Bool canSteal = false;

	if ( const CActor* const actor = actorHandle.Get() )
	{
		canSteal = CanStealOtherActor( actor );
	}
	
	RETURN_BOOL( canSteal );
}

void CActor::funcResetClothAndDangleSimulation( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	ResetClothAndDangleSimulation();
}


void CActor::funcEnableCollisions( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, val, false );
	FINISH_PARAMETERS
		CMovingAgentComponent* mac = GetMovingAgentComponent();
	ASSERT( mac );

	if ( mac )
	{
		CPhysicsWrapperInterface* wrapper = mac->GetPhysicsRigidBodyWrapper();
		if( wrapper )
		{
			wrapper->SetFlag(  PRBW_CollisionDisabled , !val );
		}
		mac->ForceEntityRepresentation( !val, CMovingAgentComponent::LS_Default );
		mac->SetUseExtractedMotion( val );
	}
}

void CActor::funcSetDebugAttackRange( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, range, CName::NONE );
	FINISH_PARAMETERS

#ifndef NO_EDITOR
		if( range )
		{
			m_debugAttackRange = GetAttackRange( range );
		}
		else
		{
			m_debugAttackRange = NULL;
		}
#endif
}

void CActor::funcEnableDebugARTraceDraw( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, val, true );
	FINISH_PARAMETERS

#ifndef NO_EDITOR
		m_debugEnableTraceDraw = val;
#endif
}

void CActor::funcPredictWorldPosition( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, inTime, 1.0f );
	FINISH_PARAMETERS;

	Vector outPos;
	PredictWorldPosition( inTime, outPos );
	RETURN_STRUCT( Vector, outPos );
}
