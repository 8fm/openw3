/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "definitionsManager.h"

#include "jobTreeLeaf.h"
#include "jobTree.h"
#include "itemEntity.h"
#include "actorActionWork.h"
#include "aiHistory.h"
#include "communitySystem.h"

#include "../../common/engine/behaviorGraphOutput.h"
#include "../../common/engine/behaviorGraphStack.h"
#include "../engine/behaviorGraphContext.h"
#include "movingPhysicalAgentComponent.h"
#include "actionPointComponent.h"


// DESIGN DECISIONS MADE:
// 1.) ANIMATIONS MANAGEMENT - has to stay here, as this action is the execution context for
//     a job tree that is a constant resource - and even though it defines which animations
//     should be executed and how, a single action from the tree can simultaneously be executed
//     for many NPCs. 
//     I also took an NPC's class for consideration - turning it into a slot animation controller,
//     however again it's this action that establishes a symmetrical context for playing the animation,
//     so I decided to put this responsibility here.
// 2.) POSITIONING - has been moved out from the action, as it's not one of its responsibilities.
//	   Action work is now only responsible for playing a job tree, and managing the working NPC's state
//     accordingly. Positioning's not a part of that. If one needs to have an NPC at the exact spot
//     for working - get him there beforehand.

const CName ActorActionWork::WORK_ANIM_SLOT_NAME( TXT("NPC_ANIM_SLOT") );
const Float ActorActionWork::WORK_INTERACTION_PRIORITY = 1000000.0f;

ActorActionWork::ActorActionWork( CActor* actor )
	: ActorAction( actor, ActorAction_Working )
	, m_isInfinite( false )
	, m_NPCActor( NULL )
	, m_currentApId( ActionPointBadID )
	, m_currentAction( NULL )
	, m_workActivity( AWA_Idle )
	, m_animated( NULL )
	, m_executionMode( JEM_Normal )
	, m_actorInteractionPriority( 0.0f )
{
}

ActorActionWork::~ActorActionWork()
{
}

// ----------------------------------------------------------------------------
// Work execution
// ----------------------------------------------------------------------------
Bool ActorActionWork::StartWorking( CJobTree* jobTree, CName category, Bool skipEntryAnimations )
{
	// Store actor as NPC ( this action is suitable only for NPC )
	m_NPCActor = Cast< CNewNPC >( m_actor );
	ASSERT( m_NPCActor );

	if ( !m_NPCActor->GetMovingAgentComponent() )
	{
		WORK_ERR( TXT("ActorActionWork::StartWorking - actor %s has no moving agent component!!!"), m_NPCActor->GetFriendlyName().AsChar() );
		return false;
	}
	// Store ptr to the animated component
	m_animated = m_actor->GetRootAnimatedComponent();
	if( !m_animated )
	{
#ifndef NO_ERROR_STATE
		m_actor->SetErrorState( TXT("ActorActionWork: no animated component") );
#endif
		return false;
	}


	m_jobContext.Reset();
	m_actionContext.Reset();
	m_actionContext.m_NPCActor = m_NPCActor;
	m_currentCategory = category;

	{
		Bool jobTreeOk = false;
		if ( jobTree )
		{
			// Setup movement
			CJobTreeNode* rootNode = jobTree->GetRootNode();
			if ( rootNode )
			{
				jobTreeOk = true;
				m_jobTree = jobTree;
				m_jobTreeRoot = rootNode;

				m_jobContext.m_currentCategories.PushBack( category );
				rootNode->InitExecutionContext( m_jobContext, m_animated, skipEntryAnimations );
			}
		}
		if ( !jobTreeOk )
		{
			return false;
		}
	}

	if ( skipEntryAnimations )
	{
		m_startAnimationOffset = GEngine->GetRandomNumberGenerator().Get< Float >( GGame->GetGameplayConfig().m_workMaxAnimOffset );
	}
	else
	{
		m_startAnimationOffset = 0.0f;
	}

	m_freezer.StartWorking( m_actor );

	m_workActivity = AWA_Initialize;
	return true;
}

void ActorActionWork::OnWorkSequenceStart()
{
	m_currentApId = m_NPCActor->GetActionSchedule().GetActiveAP();
	m_currentOwner = GCommonGame->GetSystem< CCommunitySystem >()->GetActionPointManager()->GetOwner( m_currentApId );

	// inform that we're currently working in the AP
	if ( m_NPCActor != NULL )
	{
		m_NPCActor->SetCurrentlyWorkingInAP( m_currentApId );
	}
}

void ActorActionWork::OnWorkSequenceFinished()
{
	PutOutAny( m_actionContext );

	if ( m_currentApId != ActionPointBadID )
	{
		// finishing the work, disable the action point denied areas to bring it back to the default state
		m_currentApId = ActionPointBadID;
	}

	CJobTree* jobTree = m_jobTree.Get();
	if ( jobTree )
	{
		jobTree->ApplyJobTreeExitSettings( m_NPCActor, m_executionMode == JEM_ForcedBreaking, m_jobContext );
	}

	m_workActivity = AWA_Idle;
	m_jobTreeRoot = NULL;
	m_currentAction = NULL;
	m_executionMode = JEM_Normal;
	m_actionContext.Reset();

	// remove the static obstacle we places earlier
	CMovingPhysicalAgentComponent* mac = Cast< CMovingPhysicalAgentComponent >( GetMAC() );
	//ASSERT( mac ); <- MG: happens often after delayed actions removal
	if( mac )
	{
		//mac->SetStaticObstacle( false );
		//mac->SetInteractionPriority( m_actorInteractionPriority );
		m_actorInteractionPriority = 0.0f;

		// make sure the agent's not sliding any more
		mac->Slide( Vector::ZERO_3D_POINT, EulerAngles( 0.0f, 0.0f, 0.0f ) );

		mac->SetSkipUpdateAndSampleFramesBias();
		mac->SetSkipUpdateAndSampleFramesLimit();

		CActionPointManager* actionPointManager = GCommonGame->GetSystem< CCommunitySystem >()->GetActionPointManager();
		CActionPointComponent* actionPoint = actionPointManager->GetAP( m_currentApId );
		if( ! actionPoint->KeepIKActive() )
		{
			mac->AccessAnimationProxy().EnableIKDueToWork( true );
		}
		else
		{
			mac->AccessAnimationProxy().SetUseExtendedIKOffset( false );
		}
	}

	RED_FATAL_ASSERT( mac->AccessAnimationProxy().UseExtendedIKOffset() == false, "Please check ext ik logic");

	if ( m_NPCActor != NULL )
	{
		m_NPCActor->SetCurrentlyNotWorkingInAP();
	}

	m_freezer.FinishWorking( m_actor );
}

void ActorActionWork::OnJobActionStarted()
{
	if ( CMovingAgentComponent* mac = Cast< CMovingAgentComponent >( m_actor->GetRootAnimatedComponent() ) )
	{
		mac->SetSkipUpdateAndSampleFramesBias( 2, 0 ); // can't use addition here as it may break flow when being close
		mac->SetSkipUpdateAndSampleFramesLimit( 3 );
		CActionPointManager* actionPointManager = GCommonGame->GetSystem< CCommunitySystem >()->GetActionPointManager();
		CActionPointComponent* actionPoint = actionPointManager->GetAP( m_currentApId );
		if( ! actionPoint->KeepIKActive() )
		{
			mac->AccessAnimationProxy().EnableIKDueToWork( false );
		}
		else
		{
			mac->AccessAnimationProxy().SetUseExtendedIKOffset( true );
		}
	}

	if ( m_currentAction )
	{
		m_currentAction->OnStarted( m_currentApId, m_actionContext );
	}

	RegisterAnimEventHandler();
}

void ActorActionWork::OnJobActionFinished()
{
	if ( m_jobTree.Get() )
	{
		if ( m_currentAction )
		{
			m_currentAction->OnFinished( m_currentApId, m_actionContext );
		}
	}

	UnregisterAnimEventHandler();
}

Bool ActorActionWork::Update( Float timeDelta )
{
	Bool result = true;
	Bool keepProcessing = true;


	Bool jobTreeAvailable = m_jobTree.Get() != NULL;
	ASSERT( jobTreeAvailable && TXT( "Job tree got streamed out" ) );
	if ( !jobTreeAvailable )
	{
		m_currentAction = NULL;
		if ( m_workActivity == AWA_Animation )
		{
			m_workActivity = AWA_BreakAnimation;
		}
		else
		{
			m_workActivity = AWA_Stopping;
		}
	}

	m_freezer.Update( m_actor, timeDelta, m_jobContext.m_isLeaf && !m_actionContext.IsUsingItems() );

	while( keepProcessing )
	{
		// State machine
		switch( m_workActivity )
		{
		case AWA_Initialize:
			{
				OnWorkSequenceStart();

				CMovingAgentComponent* mac = GetMAC();
				ASSERT( mac );

				ASSERT( m_currentAction == NULL );
				if ( m_currentApId == ActionPointBadID || !mac /*|| mac->HasEnabledCollisions() == true*/ )
				{
					m_workActivity = AWA_AdjustPosition;
				}
				else
				{
					m_workActivity = AWA_InitialPositionAdjustment;
				}
				keepProcessing = false;

				break;
			}

		case AWA_InitialPositionAdjustment:
			{
				ASSERT( m_currentApId != ActionPointBadID );
				Vector actionExecPos;
				Float actionExecOrientation;

				if ( !GCommonGame->GetSystem< CCommunitySystem >()->GetActionPointManager()->GetActionExecutionPosition( m_currentApId, &actionExecPos, &actionExecOrientation ) )
				{
					m_workActivity = AWA_Stopping;
					actionExecPos = Vector::ZERO_3D_POINT;
					actionExecOrientation = 0.0f;
					break;
				}

				Bool isPrecisePlacementNeeded = false;
				CJobTree* jobTreePtr = m_jobTree.Get();
				if ( jobTreePtr )
				{
					isPrecisePlacementNeeded = jobTreePtr->GetSettings().m_needsPrecision;
				}

				CMovingAgentComponent* mac = GetMAC();
				ASSERT( mac );
				if( mac )
				{
					Vector translation = actionExecPos - mac->GetWorldPosition();
					Float rotation = EulerAngles::AngleDistance( mac->GetWorldYaw(), actionExecOrientation );

					Bool initialPositionReached = true;
					Float distToAP = 0.0f;
					if ( isPrecisePlacementNeeded == true )
					{
						distToAP = translation.Mag3();
						initialPositionReached = distToAP <= CActionPointComponent::PRECISE_PLACEMENT_MARGIN && MAbs( rotation ) <= CActionPointComponent::PRECISE_PLACEMENT_MARGIN;
					}
					else
					{
						distToAP = translation.Mag2();
						initialPositionReached = distToAP <= CActionPointComponent::PRECISE_PLACEMENT_MARGIN && MAbs( rotation ) <= CActionPointComponent::PRECISE_PLACEMENT_MARGIN && MAbs( translation.Z ) < CActionPointComponent::MAXIMUM_Z_DISPLACEMENT;
						translation.Z = 0.0f;
					}

					if ( initialPositionReached == false )
					{
						if ( distToAP > 2.0f )
						{
							mac->TeleportTo( actionExecPos, EulerAngles( 0.0f, 0.0f, rotation ), false );
						}
						else
						{
							mac->Slide( translation, EulerAngles( 0.0f, 0.0f, rotation ) );
						}
						keepProcessing = false;
					}
					else
					{
						mac->Slide( Vector::ZERO_3D_POINT, EulerAngles( 0.0f, 0.0f, 0.0f ) );
						m_workActivity = AWA_AdjustPosition;
						keepProcessing = true;
					}
				}
				else
				{
					m_workActivity = AWA_AdjustPosition;
					keepProcessing = true;
				}

				break;
			}

		case AWA_AdjustPosition:
			{
				AdjustPosition();
				m_workActivity = AWA_NextAction;
				break;
			}

		case AWA_StartWorking:
			{
				if ( StartAnimation( WORK_ANIM_SLOT_NAME, m_currentAction ) )
				{
					m_freezer.ActorPlayAnimation( m_actor, m_jobContext.m_isLeaf && !m_actionContext.IsUsingItems() );

					m_workActivity = AWA_Animation;
					OnJobActionStarted();
				}
				else
				{
					m_workActivity = AWA_Failing;
				}
				break;
			}

		case AWA_ProcessItems:
			{
				SetupItems();
				m_workActivity = AWA_StandbyItems;
			}
			// No need to break now

		case AWA_StandbyItems:
			{
				if ( !m_NPCActor->HasLatentItemAction() )
				{
					m_workActivity = AWA_StartWorking;
					keepProcessing = true;
				}
				else
				{
					keepProcessing = false;
				}
				break;
			}

		case AWA_NextAction:
			{
				OnJobActionFinished();

				m_currentAction = GetNextAction();

				if ( m_currentAction )
				{
					m_workActivity = AWA_ProcessItems;
				}
				else
				{
					m_workActivity = AWA_Stopping;
				}
				break;
			}
		case AWA_Animation:
			{
				if ( m_freezer.IsAnimTimeoutOccured() )
				{
					m_freezer.ResetAnimTimer();

					m_workActivity = AWA_NextAction;
					keepProcessing  = true;
				}
				else
				{
					keepProcessing = false;
				}

				break;
			}

		case AWA_BreakAnimation:
			{
				m_animated->GetBehaviorStack()->StopSlotAnimation( WORK_ANIM_SLOT_NAME );

				if ( m_executionMode == JEM_Normal )
				{
					m_workActivity = AWA_Stopping;
				}
				else
				{
					m_workActivity = AWA_NextAction;
				}
				break;
			}

		case AWA_Stopping:
			{
				OnWorkSequenceFinished();
				m_actor->ActionEnded( ActorAction_Working, ActorActionResult_Succeeded );
				keepProcessing = false;
				result = false;
				break;
			}

		case AWA_Failing:
			{
				OnWorkSequenceFinished();
				m_actor->ActionEnded( ActorAction_Working, ActorActionResult_Failed );
				keepProcessing = false;
				result = false;
				break;
			}
		}
	}

	return result;
}

Bool ActorActionWork::ExitWorking( Bool fast /*= false*/ )
{
	if ( m_executionMode == JEM_Breaking || m_executionMode == JEM_ForcedBreaking )
	{
		// already breaking
		return true;
	}

	if ( m_workActivity == AWA_Animation )
	{
		if ( fast )
		{
			m_executionMode = JEM_ForcedBreaking;
		}
		else
		{
			m_executionMode = JEM_Breaking;
		}
		m_workActivity = AWA_BreakAnimation;
	}

	return true;
}

void ActorActionWork::Stop()
{
	WORK_LOG( TXT("---- Work Stop") );
	OnJobActionFinished();
	if ( m_animated->GetBehaviorStack() )
	{
		m_animated->GetBehaviorStack()->StopSlotAnimation( WORK_ANIM_SLOT_NAME );
	}
	OnWorkSequenceFinished();
	m_workActivity = AWA_Idle;
}

Bool ActorActionWork::CanUseLookAt() const
{
	return m_currentAction ? m_currentAction->GetAllowedLookatLevel() != LL_Null: true;
}

void ActorActionWork::OnBeingPushed( const Vector& direction, Float rotation, Float speed, EPushingDirection animDirection )
{
	ExitWorking( true );
	ActorAction::OnBeingPushed( direction, rotation, speed, animDirection );
}

CMovingAgentComponent* ActorActionWork::GetMAC() const
{
	CMovingAgentComponent* mac = m_actor->GetMovingAgentComponent();
	if ( !mac )
	{
		return NULL;
	}
	return mac;
}

Bool ActorActionWork::AdjustPosition() const
{
	// at this point all movement related state changes have been made, so we can safely
	// change agent's location
	Matrix localToWorld = m_actor->GetLocalToWorld();
	Vector newPosition = m_actor->GetWorldPosition() + localToWorld.TransformVector( m_jobContext.m_currentTranslation );
	EulerAngles newOrientation = m_actor->GetWorldRotation();
	newOrientation.Yaw += m_jobContext.m_currentRotation;

	CMovingAgentComponent* mac = GetMAC();
	ASSERT( mac );
	if( mac )
	{
		mac->TeleportTo( newPosition, newOrientation, false );
	}
	/*

	Matrix localToWorld = m_actor->GetLocalToWorld();
	Vector translation = localToWorld.TransformVector( m_jobContext.m_initialTranslation );
	Float rotation = m_jobContext.m_currentRotation;

	CMovingAgentComponent* mac = GetMAC();
	ASSERT( mac );
	if( mac )
	{
		mac->Slide( translation, EulerAngles( 0.0f, 0.0f, rotation ) );
	}
	*/
	return true;
}
// -------------------------------------------------------------------------
// Animations
// -------------------------------------------------------------------------

Bool ActorActionWork::StartAnimation( const CName& slotName, const CJobActionBase* actionBase, Bool noBlendIn )
{
	// Play animation on slot
	if ( !m_animated->GetBehaviorStack() )
	{
	#ifndef NO_ERROR_STATE
		m_actor->SetErrorState( TXT("ActorActionWork: no behavior stack") );
	#endif
		return false;
	}

	// Get game config
	SGameplayConfig& gameplayConfig = GGame->GetGameplayConfig();

	// Setup slot
	SBehaviorSlotSetup slotSetup;
	slotSetup.m_blendInType = BTBM_Destination;
	slotSetup.m_blendIn = noBlendIn ? 0.f : actionBase->GetBlendIn();
	slotSetup.m_blendOutType = BTBM_Source;
	slotSetup.m_blendOut = actionBase->GetBlendOut();
	slotSetup.m_listener = this;
	slotSetup.m_speedMul = GEngine->GetRandomNumberGenerator().Get< Float >( gameplayConfig.m_workAnimSpeedMulMin , gameplayConfig.m_workAnimSpeedMulMax );
	slotSetup.m_offset = m_startAnimationOffset;
	
	// Reset offset
	m_startAnimationOffset = 0.0f;

	// Play animation
	if ( !m_animated->GetBehaviorStack()->PlaySlotAnimation( slotName, actionBase->GetAnimName(), &slotSetup ) )
	{
		#ifndef NO_ERROR_STATE
		m_actor->SetErrorState( String::Printf( TXT("ActorActionWork: unable to play animation %s"), actionBase->GetAnimName().AsString().AsChar() ) );
		#endif
		WORK_WARN( TXT("Couldn't play animation %s, job tree: %s, npc: %s"), actionBase->GetAnimName().AsString().AsChar(), m_jobTree.Get()->GetDepotPath().AsChar(), m_NPCActor->GetFriendlyName().AsChar() );
		return false;
	}

	// We are animating
	return true;
}

void ActorActionWork::OnSlotAnimationEnd( const CBehaviorGraphAnimationBaseSlotNode * sender, CBehaviorGraphInstance& instance, ISlotAnimationListener::EStatus status )
{
	// Skip current node if animation failed
	if( status == ISlotAnimationListener::S_Finished || status == ISlotAnimationListener::S_BlendOutStarted || status == ISlotAnimationListener::S_Stopped )
	{
		// End this job action and proceed to the next one
		m_workActivity = AWA_NextAction;
	}
	else
	{
		m_workActivity = AWA_Failing;
	}
}

void ActorActionWork::HandleEvent( const CAnimationEventFired &event )
{
	if ( m_jobTreeRoot.Get() && m_currentAction )
	{
		m_currentAction->OnAnimEvent( m_currentApId, m_actionContext, event );
	}
}

void ActorActionWork::RegisterAnimEventHandler()
{
	// Register this action as animation event handler
	m_animated->GetAnimationEventNotifier( CNAME( take_item ) )->RegisterHandler( this );
	m_animated->GetAnimationEventNotifier( CNAME( leave_item ) )->RegisterHandler( this );
	m_animated->GetAnimationEventNotifier( CNAME( drop_item ) )->RegisterHandler( this );
}

void ActorActionWork::UnregisterAnimEventHandler()
{
	// Unregister this action from anim event handlers
	m_animated->GetAnimationEventNotifier( CNAME( take_item ) )->UnregisterHandler( this );
	m_animated->GetAnimationEventNotifier( CNAME( leave_item ) )->UnregisterHandler( this );
	m_animated->GetAnimationEventNotifier( CNAME( drop_item ) )->UnregisterHandler( this );
}

void ActorActionWork::PutOutAny( SJobActionExecutionContext& actionContext )
{
	if ( actionContext.m_item1.IsUsed() )
	{
		PutOutAny( actionContext, actionContext.m_item1 );
	}
	if ( actionContext.m_item2.IsUsed() )
	{
		PutOutAny( actionContext, actionContext.m_item2 );
	}
}

void ActorActionWork::PutOutAny( SJobActionExecutionContext& actionContext, SJobActionExecutionContext::SItemInfo& itemInfo )
{
	// If there was an item picked up from action point, it will be put away now
	if ( itemInfo.m_apItem )
	{
		// Get required pointers
		CNewNPC* npc = actionContext.m_NPCActor;
		ASSERT( npc );
		CInventoryComponent* npcInventory = npc->GetInventoryComponent();

		CActionPointManager* apMgr = GCommonGame->GetSystem< CCommunitySystem >()->GetActionPointManager();
		CInventoryComponent* apInventory = ( m_currentApId != ActionPointBadID ) ? apMgr->GetInventory( m_currentApId ) : NULL;
		// MG: If the npcInventory is gone, it may cause the fake item to be stored in the save game.
		if ( npcInventory && apInventory && apMgr )
		{
			// Grab item data
			SInventoryItem* apItem = apInventory->GetItem( itemInfo.m_apItem );
			ASSERT( apItem && "ARGHHHH! Report this!" );
			if ( apItem )
			{
				// Revalidate item entity proxy for further use
				SInventoryItem* npcItem = npcInventory->GetItem( itemInfo.m_npcItem );
				if ( npcItem != nullptr )
				{
					apItem->MoveProxy( npcItem );
				}
				apItem->SetIsProxyTaken( false );
				apItem->RevalidateProxy();

				// Unmount item from npc hand
				npcInventory->UnMountItem( itemInfo.m_npcItem );

				// Remove logic item from npc inventory
				npcInventory->RemoveFakeItem( itemInfo.m_npcItem );

				if ( apItem->IsMounted() )
				{
					// Mount item back to AP
					CInventoryComponent::SMountItemInfo mountInfo;
					mountInfo.m_force = true;
					apInventory->MountItem( itemInfo.m_apItem, mountInfo );
				}
				else
				{
					// Despawn item entity. It will be spawned again when used by npc.
					apInventory->DespawnItem( itemInfo.m_apItem );
				}
			}
		}
		else
		{
			ITEM_ERR( TXT("Couldn't perform PutOutAny, apInventory or apMgr is gone") );
			return;
		}
	}

	// Clear action context item info
	itemInfo.Reset();
}


// ----------------------------------------------------------------------------
// Debug
// ----------------------------------------------------------------------------
void ActorActionWork::GenerateDebugFragments( CRenderFrame* frame )
{
}

String ActorActionWork::GetDescription() const
{
	TActionPointID apId = m_NPCActor->GetActionSchedule().GetActiveAP();
	String friendlyAPName = GCommonGame->GetSystem< CCommunitySystem >()->GetActionPointManager()->GetFriendlyAPName( apId );
	return String::Printf( TXT( "Working in AP %s - %s" ), apId.ToString().AsChar(), friendlyAPName.AsChar() );
}

// ----------------------------------------------------------------------------
// Actions iteration
// ----------------------------------------------------------------------------

const CJobActionBase* ActorActionWork::GetNextAction()
{
	CJobTreeNode *jobTreeNode = m_jobTreeRoot.Get();
	if ( jobTreeNode == NULL )
	{
		// action point was most likely despawned, the whole thing will change after job trees are made CResource
		return NULL;
	}

	const CJobActionBase* action = NULL;

	// Reset node leaf flag
	m_jobContext.m_isLeaf = false;

	// Get action appropriate for current situation ( forced breaking for instance )
	switch ( m_executionMode )
	{
	case JEM_Normal:
		// Get next normal action
		action = jobTreeNode->GetNextAction( m_jobContext );

		// Deal with post animation when the next AP will be the same
		if ( action && m_jobContext.IsCurrentActionLeave() &&
			!GCommonGame->GetSystem< CCommunitySystem >()->GetActionPointManager()->HasPreferredNextAPs( m_currentApId ) &&
			( m_NPCActor->GetActionSchedule().UsesLastAP() ||
			m_NPCActor->GetAvailableActionPointsCount( true, m_currentOwner ) == 0 )
			)
		{
			m_jobContext.Reset();
			m_jobContext.m_currentCategories.PushBack( m_currentCategory );
			m_actionContext.Reset();
			m_actionContext.m_NPCActor = m_NPCActor;
			
			action = jobTreeNode->GetNextAction( m_jobContext );
			if ( m_jobContext.IsCurrentActionApproach() && ( !action || action->IsSkippable() ) )
			{
				// Omit pre
				action = jobTreeNode->GetNextAction( m_jobContext );
			}
		}

		break;
	case JEM_Breaking:
		// Get only exit actions
		action = jobTreeNode->GetNextExitAction( m_jobContext );
		break;
	case JEM_ForcedBreaking:
		// Get only exit actions
		action = jobTreeNode->GetNextForcedExitAction( m_jobContext );
		break;
	}

	return action;
}

Bool ActorActionWork::IsInfinite()
{
	return m_isInfinite;
}

Bool ActorActionWork::SetupItems()
{
	CInventoryComponent* npcInventory = m_NPCActor->GetInventoryComponent();
	if ( !npcInventory )
	{
		return false;
	}

	Bool anyActionStarted = false;

	// The below code sets anyActionsStarted value to true, without checking value returned by latent draw/holster methods.
	// This can cause unnecessary 1 frame idle when item doesn't support latent draw/holster actions properly.
	// However it won't cause problems, and on top of that, such items shouldn't exist in the final game
	m_NPCActor->IssueRequiredItems( SActorRequiredItems( m_jobContext.m_leftItem, m_jobContext.m_rightItem ) );
	m_jobContext.m_leftItem = CNAME( Any );
	m_jobContext.m_rightItem = CNAME( Any );

	return anyActionStarted;
}

Bool ActorActionWork::CanReact() const
{
	CJobTree* jobTree = m_jobTree.Get();

	return jobTree ? jobTree->GetSettings().m_ignoreHardReactions : true;
}


void ActorActionWork::StartWorkAnimInstant()
{
	m_currentAction = GetNextAction();

	if ( m_currentAction && StartAnimation( WORK_ANIM_SLOT_NAME, m_currentAction , true ) )
	{
		m_freezer.ActorPlayAnimation( m_actor, m_jobContext.m_isLeaf && !m_actionContext.IsUsingItems() );

		m_workActivity = AWA_Animation;
		OnJobActionStarted();
	}
}


//////////////////////////////////////////////////////////////////////////

Bool CActor::ActionStartWorking( CJobTree* jobTree, CName category, Bool skipEntryAnimations )
{
	PC_SCOPE( AI_ActorActionWork );

	// Check if current action may be canceled by other actions
	if ( m_action && ! m_action->IsCancelingAllowed() )
	{
		AI_EVENT( this, EAIE_Action, EAIR_Failure, TXT("ActionWork"), String::Printf( TXT("Can't start action work for: %s - he's currently performing a different action that can't be canceled"), GetName().AsChar() ) );
		return false;
	}

	// Cancel other action
	ActionCancelAll();

	// Start action
	if ( !m_actionWork.StartWorking( jobTree, category, skipEntryAnimations ) )
	{
		AI_EVENT( this, EAIE_Action, EAIR_Failure, TXT("ActionWork"), String::Printf( TXT("Can't start action work for: %s"), GetName().AsChar() ) );
		return false;
	}

	// Start action
	m_action = &m_actionWork;
	m_latentActionResult = ActorActionResult_InProgress;
	m_latentActionType   = ActorAction_Working;

	OnActionStarted( ActorAction_Working );

	return true;
}

Bool CActor::ActionExitWorking( Bool fast /*= false*/ )
{
	// Check if current action is action working
	if ( m_action == NULL || m_action->GetType() != ActorAction_Working )
	{
		AI_EVENT( this, EAIE_Action, EAIR_Failure, TXT("ActionExitWorking"), String::Printf( TXT("Can't break work for actor %s - he's not working"), GetName().AsChar() ) );
		return false;
	}

	// Start action
	if ( !m_actionWork.ExitWorking( fast ) )
	{
		AI_EVENT( this, EAIE_Action, EAIR_Failure, TXT("ActionExitWorking"), String::Printf( TXT("Can't break work for actor %s - he simply refused"), GetName().AsChar() ) );
		return false;
	}
	
	// Altered working action to exiting state
	return true;
}

extern Bool GLatentFunctionStart;


void CActor::funcActionExitWork( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_OPT( Bool, isFast, false );
	FINISH_PARAMETERS;

	// Only in threaded code
	ASSERT( stack.m_thread );

	// Start test
	ACTION_START_TEST;

	// Starting !
	if ( GLatentFunctionStart )
	{
		// Start action
		Bool actionResult = ActionExitWorking( isFast );
		if ( !actionResult )
		{
			// Already failed
			RETURN_BOOL( false );
			return;
		}

		// Yield the thread to pause execution
		stack.m_thread->ForceYield();
		return;
	}

	if ( m_latentActionType == ActorAction_Working && m_latentActionResult == ActorActionResult_InProgress && m_actionWork.IsBreaking() )
	{
		// Yield the thread to pause execution
		stack.m_thread->ForceYield();
		return;
	}

	// Get the state
	const Bool succeeded = m_latentActionResult == ActorActionResult_Succeeded;

	// Return state
	RETURN_BOOL( succeeded );
}

void CActor::funcActionExitWorkAsync( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_OPT( Bool, isFast, false );
	FINISH_PARAMETERS;

	// Start test
	ACTION_START_TEST;
	Bool actionResult = ActionExitWorking( isFast );

	RETURN_BOOL( actionResult );
}

//////////////////////////////////////////////////////////////////////////

ActorActionWorkFreezer::ActorActionWorkFreezer()
{
	Reset();
}

void ActorActionWorkFreezer::Reset()
{
	ASSERT( !m_isAnimFreezing );
	m_isAnimFreezing = false;
	m_animFreezingTime = 0.f;
	m_invisibilityTime = 0.f;
}

void ActorActionWorkFreezer::FreezeActor( CActor* actor )
{
	ASSERT( !m_isAnimFreezing );
	actor->FreezeAllAnimatedComponents();
	m_isAnimFreezing = true;
}

void ActorActionWorkFreezer::UnfreezeActor( CActor* actor )
{
	ASSERT( m_isAnimFreezing );
	actor->UnfreezeAllAnimatedComponents();
	m_isAnimFreezing = false;
}

void ActorActionWorkFreezer::RestoreActor( CActor* actor )
{
	if ( m_isAnimFreezing )
	{
		UnfreezeActor( actor );
	}
}

void ActorActionWorkFreezer::Update( CActor* actor, Float timeDelta, Bool isCurrentActionLeaf )
{
	if ( !GGame->GetGameplayConfig().m_useWorkFreezing )
	{
		RestoreActor( actor );
		Reset();
		return;
	}

	{
		const Bool isVisible = IsActorVisible( actor );
		const Float distSqr = actor->GetWorldPositionRef().DistanceSquaredTo( GGame->GetPlayerEntity()->GetWorldPositionRef() );

		const Bool canFreeze = 
			isCurrentActionLeaf && ( !actor->IsSpeaking() && !actor->IsInNonGameplayScene() && (
			( !isVisible && IsOutsideInvisibleRange( distSqr ) ) ) );

		if ( canFreeze )
		{
			if ( !m_isAnimFreezing )
			{
				m_invisibilityTime += timeDelta;

				if ( IsInvisibilityTimeoutOccured() )
				{
					FreezeActor( actor );
				}
			}
			else
			{
				m_animFreezingTime += timeDelta;
			}
		}
		else 
		{
			RestoreActor( actor );
			Reset();
		}
	}
}

void ActorActionWorkFreezer::ActorPlayAnimation( CActor* actor, Bool isCurrentActionLeaf )
{
	if ( IsActorFrozen() )
	{
		UpdateActorPose( actor );
	}
}

void ActorActionWorkFreezer::UpdateActorPose( CActor* actor ) const
{
	// Ok this code is tricky and low level but this is function for optimalization
	if ( GGame->GetGameplayConfig().m_workResetInFreezing )
	{
		CAnimatedComponent* ac = actor->GetRootAnimatedComponent();
		if ( ac && ac->GetBehaviorStack() && ac->GetBehaviorGraphSampleContext() )
		{
			IBehaviorGraphSlotInterface slot;

			if ( ac->GetBehaviorStack()->GetSlot( ActorActionWork::WORK_ANIM_SLOT_NAME, slot ) )
			{
				SBehaviorSampleContext* context = ac->GetBehaviorGraphSampleContext();

				// Get pose
				SBehaviorGraphOutput& pose = context->PrepareForSample( false );

#ifndef IGNORE_ANIM_STREAMING
				slot.GetSlotCompressedPose( pose );
#else
				slot.GetSlotPose( pose );
#endif

				// Pose correction
				if ( context->ShouldCorrectPose() )
				{
					context->SetPoseCorrection( pose );
				}

				// Trajectory extraction
				if ( ac->UseExtractedTrajectory() && ac->HasTrajectoryBone() )
				{
					pose.ExtractTrajectory( ac );
				}

				ac->ForceBehaviorPose( pose );			
			}
		}
	}
}

Bool ActorActionWorkFreezer::IsActorVisible( CEntity* entity ) const
{
	return entity->WasVisibleLastFrame();
}

Bool ActorActionWorkFreezer::IsOutsideVisibleRange( Float distSqr ) const
{
	return false;
}

Bool ActorActionWorkFreezer::IsOutsideInvisibleRange( Float distSqr ) const
{
	const Float radius = GGame->GetGameplayConfig().m_workFreezingRadiusForInvisibleActors;
	return radius > 0.f ? distSqr > radius * radius : false;
}

Bool ActorActionWorkFreezer::IsActorFrozen() const
{
	return m_isAnimFreezing;
}

Bool ActorActionWorkFreezer::IsInvisibilityTimeoutOccured() const
{
	return m_invisibilityTime >= GGame->GetGameplayConfig().m_workFreezingDelay;
}

Bool ActorActionWorkFreezer::IsAnimTimeoutOccured() const
{
	return m_animFreezingTime >= GGame->GetGameplayConfig().m_workMaxFreezingTime;
}

void ActorActionWorkFreezer::ResetAnimTimer()
{
	m_animFreezingTime = 0.f;
}

void ActorActionWorkFreezer::StartWorking( CActor* actor )
{
	Reset();
}

void ActorActionWorkFreezer::FinishWorking( CActor* actor )
{
	RestoreActor( actor );
	Reset();
}
