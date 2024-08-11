/**
* Copyright c 2012 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "behTreeNodePerformWork.h"

#include "../core/mathUtils.h"

#include "../engine/animatedComponent.h"
#include "../engine/appearanceComponent.h"
#include "../engine/behaviorGraphContext.h"
#include "../engine/behaviorGraphStack.h"
#include "../engine/characterControllerParam.h"
#include "../physics/physicsRagdollWrapper.h"
#include "../engine/renderCommands.h"
#include "../engine/extAnimEvent.h"
#include "../engine/skeletalAnimationContainer.h"

#include "actorActionWork.h"
#include "actionPointComponent.h"
#include "actionPointManager.h"
#include "behTreeNode.h"
#include "behTreeNodePlayScene.h"
#include "behTreeInstance.h"
#include "communitySystem.h"
#include "jobTreeLeaf.h"
#include "jobTreeNode.h"
#include "movableRepresentationPathAgent.h"
#include "movementAdjustor.h"
#include "movingPhysicalAgentComponent.h"
#include "extAnimItemEvents.h"
#include "storySceneSystem.h"

RED_DEFINE_STATIC_NAME( AI_PauseWorkAnimation )
RED_DEFINE_STATIC_NAME( AI_UnPauseWorkAnimation )


////////////////////////////////////////////////////////////////////////
// Definition
////////////////////////////////////////////////////////////////////////

IBehTreeNodeInstance* CBehTreeNodePerformWorkDefinition::SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent /*= NULL */ ) const 
{
	return new Instance( *this, owner, context, parent );
}

////////////////////////////////////////////////////////////////////////
// Instance
////////////////////////////////////////////////////////////////////////

const Int32 CBehTreeNodePerformWorkInstance::FORCE_REQUEST_PRIORITY_BONUS = 100;

#define WORK_ANIM_SLOT_NAME CNAME( NPC_ANIM_SLOT )
const InteractionPriorityType CBehTreeNodePerformWorkInstance::BREAKABLE_WORK_INTERACTION_PRIORITY = InteractionPriorityTypeMean;
const InteractionPriorityType CBehTreeNodePerformWorkInstance::WORK_INTERACTION_PRIORITY = InteractionPriorityTypeMax;


CBehTreeNodePerformWorkInstance::CBehTreeNodePerformWorkInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent /*= NULL */ )
	: CBehTreeNodeAtomicActionInstance( def, owner, context, parent )
	, m_immediateActivation( false )
	, m_immediateAdjustment( false )
	, m_initialAnimationWithOffset( false )
	, m_inFastLeaveAnimation( false )
	, m_isAdjusted( false )
	, m_isZCorrectionPending( false )
	, m_ignoreCollisions( false )
	, m_workPaused( false )
	, m_startAnimationWithoutBlend( true )
	, m_disabledIK( true )
#ifdef DEBUG_PERFORM_WORK_ADJUSTMENT
	, m_debugShift( 0.f, 0.f, 0.f )
	, m_debugActionStartedLocation( 0.f, 0.f, 0.f )
	, m_debugRotation( 0.f )
#endif
	, m_workData( owner )
	, m_animated( NULL )
	, m_executionMode( JEM_Normal )
	, m_workActivity( WA_Idle )
	, m_globalBreakingBlendOutTime(0.0f)
	, m_spawnedLeftItem( CName::NONE )
	, m_spawnedRightItem( CName::NONE )
	, m_currentAction( nullptr )
	
{
	{
		SBehTreeEvenListeningData e;
		e.m_eventName = CNAME( AI_ForceLastWork );
		e.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
		context.AddEventListener( e, this );
	}
}

void CBehTreeNodePerformWorkInstance::OnDestruction()
{
	{
		SBehTreeEvenListeningData e;
		e.m_eventName = CNAME( AI_ForceLastWork );
		e.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
		m_owner->RemoveEventListener( e, this );
	}
	Super::OnDestruction();
}

Bool CBehTreeNodePerformWorkInstance::Activate()
{
	m_spawnedLeftItem		= CName::NONE;
	m_spawnedRightItem		= CName::NONE;
	m_inFastLeaveAnimation	= false;
	m_workPaused			= false;

	CNewNPC* npc = m_owner->GetNPC();

	if ( !npc || !npc->GetMovingAgentComponent() )
	{
		WORK_ERR( TXT("ActorActionWork::StartWorking - actor %s has no moving agent component!!!"), npc->GetFriendlyName().AsChar() );
		DebugNotifyActivationFail();		
		return false;
	}

	CBehTreeWorkData* workData = m_workData.Get();
	// Store ptr to the animated component
	m_animated = npc->GetRootAnimatedComponent();
	if( !m_animated )
	{
#ifndef NO_ERROR_STATE
		npc->SetErrorState( TXT("ActorActionWork: no animated component") );
#endif
		DebugNotifyActivationFail();		
		return false;
	}
	m_currentApId = workData->GetSelectedAP();
	m_currentCategory = workData->GetSelectedAPCategory();

	if ( m_currentApId == ActionPointBadID )
	{
		DebugNotifyActivationFail();		
		return false;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//#HACK: check if this tree blocks robed NPCs and if so swap it with another one - Shadi Dadenji
	CActionPointManager* actionPointManager = GCommonGame->GetSystem< CCommunitySystem >()->GetActionPointManager();
	if ( !actionPointManager )
	{
		DebugNotifyActivationFail();		
		return false;	
	}

	CJobTree* jobTree = actionPointManager->GetJobTree( m_currentApId ).Get();
	if ( !jobTree || !jobTree->GetRootNode() )
	{
		DebugNotifyActivationFail();		
		return false;
	}

	//if altJobTree in the tree settings is not null, and the appearance uses a robe, SWAP!
	if ( npc->GetUsesRobe() )
	{
		CJobTree* altJobTree = jobTree->GetSettings().m_altJobTreeRes.Get();
		if ( altJobTree )
		{
			//load the provided alt jobtree in the settings of the current one instead (set up by community designers)
			jobTree = altJobTree;
		}
	}
	//#END HACK
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	CActionPointComponent* apCmp = actionPointManager->GetAP( m_currentApId );
	if( !apCmp )
	{
		DebugNotifyActivationFail();
		return false;
	}

	const Bool spawnToWork = workData->IsTryingToSpawnToWork( m_owner );
	m_startAnimationWithoutBlend = spawnToWork;
	const Bool forceActivation = m_workData->IsInImmediateActivation( m_owner );
	m_immediateActivation = forceActivation || spawnToWork;
	m_immediateAdjustment = m_immediateActivation;
	m_ignoreCollisions = apCmp->GetIgnoreCollisions();


	Vector apPosition;

	if ( m_immediateActivation )
	{
		apPosition = apCmp->GetWorldPositionRef();
	}
	else
	{
		if ( !actionPointManager->GetSafePosition( m_currentApId, &apPosition, NULL ) )
		{
			DebugNotifyActivationFail();		
			return false;
		}
		if ( ( npc->GetWorldPositionRef().AsVector2() - apPosition.AsVector2() ).SquareMag() > 2.f && !( forceActivation && workData->ShouldForceProcessOnActivation() ) )
		{
			DebugNotifyActivationFail();		
			return false;
		}
	}

	CJobTreeNode* jobTreeRoot = jobTree->GetRootNode();

	m_jobTree = jobTree;
	m_jobTreeRoot = jobTreeRoot;

	ResetContexts();

	{
		m_jobContext.m_loopingJob = workData->m_hasLoopingSequence;

		jobTreeRoot->InitExecutionContext( m_jobContext, m_animated, m_immediateActivation );

		m_jobContext.m_workerInventoryComponent = npc->FindComponent< CInventoryComponent >();
		m_jobContext.m_skippedEtryAnimItem = CName::NONE;
	}
	

	if ( m_immediateActivation )
	{
		m_startAnimationOffset = GEngine->GetRandomNumberGenerator().Get< Float >( GGame->GetGameplayConfig().m_workMaxAnimOffset );
		m_initialAnimationWithOffset = true;
	}
	else
	{
		m_startAnimationOffset = 0.0f;
		m_initialAnimationWithOffset = false;
	}

	m_freezer.StartWorking( npc );

	m_workActivity = WA_Initialize;	

	m_isAdjusted = false;
	m_isDone = false;

	if ( workData->ShouldForceProcessOnActivation() || m_immediateActivation )
	{
		const EProcessLogicResult ret = ProcessLogic();
		if ( ret == PLR_Faild )
		{			
			DebugNotifyActivationFail();
			return false;
		}
	}
	
	workData->SetIgnoreHadReactions( jobTree->GetSettings().m_ignoreHardReactions );
	workData->SetIsConscious( jobTree->GetSettings().m_isConscious );
	workData->SetJTType( jobTree->GetSettings().m_jobTreeType );
	workData->SetIsSitting( false );
	workData->CancelSpawnToWork();
	workData->CancelImmediateActivation();

	m_isZCorrectionPending = !m_ignoreCollisions;

	actionPointManager->SetReserved( m_currentApId, CActionPointManager::REASON_PERFORMS_WORK, npc );

	m_globalBreakingBlendOutTime = jobTree->GetSettings().m_globalBreakingBlendOutTime;

	return Super::Activate();
}

Bool CBehTreeNodePerformWorkInstance::Interrupt()
{
	// The following call "duplicates" ApplyJobTreeExitSettings( ..., true, ... ) behavior,
	// which is called as the outcome of ExitWorking( true ).
	// DestroyItemsOnInterrupt();

	//force the fast leave animation here and return false while it plays out
	ExitWorking( true );

	if ( m_workActivity != WA_Completed )
	{
		// action needs time to be finished
		return false;
	}

	// action is no longer running, so we can interrupt this block
	return Super::Interrupt();
}


void CBehTreeNodePerformWorkInstance::Deactivate()
{
	CActor* actor = m_owner->GetActor();
	CMovingAgentComponent* mac = actor ? actor->GetMovingAgentComponent() : NULL;

	if ( CCommunitySystem* communitySystem = GCommonGame->GetSystem< CCommunitySystem >() )
	{
		communitySystem->GetActionPointManager()->SetFree( m_currentApId, CActionPointManager::REASON_PERFORMS_WORK );
	}
	
	if ( mac )
	{
		CBehaviorGraphStack* stack = mac->GetBehaviorStack();

		// detach slot listener
		if ( stack )
		{
			stack->DetachSlotListener( WORK_ANIM_SLOT_NAME, this );
		}
	}

	OnWorkSequenceFinished();
	UnregisterAnimEventHandler();
	UnmountItems();

	Super::Deactivate();
}

void CBehTreeNodePerformWorkInstance::AdjustPosIfNeeded()
{
	if ( m_isAdjusted )
	{
		return;
	}

	Bool immediateAdjustment = m_immediateAdjustment;
	m_isAdjusted = true;
	m_immediateAdjustment = false;
#ifdef DEBUG_PERFORM_WORK_ADJUSTMENT
	m_debugShift.Set( 0.f, 0.f, 0.f );
	m_debugRotation = 0.f;
#endif

	CActor* actor = m_owner->GetActor();
	CMovingAgentComponent* mac = actor->GetMovingAgentComponent();

	if ( mac->GetRagdollPhysicsWrapper() && !mac->GetRagdollPhysicsWrapper()->IsKinematic() )
	{
		return;
	}

	if ( m_currentApId == ActionPointBadID )
	{
		return;
	}

	CActionPointManager* actionPointManager = GCommonGame->GetSystem< CCommunitySystem >()->GetActionPointManager();
	CActionPointComponent* ap = actionPointManager->GetAP( m_currentApId );

	if ( !ap )
	{
		return;
	}

	Vector actionExecPos = ap->GetWorldPosition();
	Float actionExecOrientation = ap->GetWorldYaw();

	Vector executionPosTranslation = m_jobContext.m_currentTranslation;
	Float executionPosRotation = m_jobContext.m_currentRotation;

	if ( immediateAdjustment && m_startAnimationOffset > 0.f )
	{
		Vector additionalMotion;
		Float additionalRotation;
		m_currentAction->GetMotionExtractionForTime( m_animated, additionalMotion, additionalRotation, m_startAnimationOffset );
		if ( executionPosRotation )
		{
			additionalMotion.AsVector2() = MathUtils::GeometryUtils::Rotate2D( additionalMotion.AsVector2(), DEG2RAD( executionPosRotation ) );
		}
		executionPosTranslation += additionalMotion;
		executionPosRotation += additionalRotation;
	}

	executionPosTranslation.AsVector2() = MathUtils::GeometryUtils::Rotate2D( executionPosTranslation.AsVector2(), DEG2RAD( actionExecOrientation ) );

	Float rotation = EulerAngles::AngleDistance( mac->GetWorldYaw(), actionExecOrientation + executionPosRotation );

	actionExecPos += executionPosTranslation;

	const Vector& actorPos = mac->GetWorldPositionRef();
	
	Vector translation = actionExecPos - actorPos;

	mac->CancelMove();
	mac->TeleportTo( actorPos, mac->GetWorldRotation(), false );

	// adjust our position and rotation if needed
	Float distToAPSqr = translation.SquareMag3();
	if( distToAPSqr > 0.05f * 0.05f || Abs( rotation ) > 1.f )
	{
		CMovementAdjustor* adjustor = mac->GetMovementAdjustor();
		SMovementAdjustmentRequest* request = adjustor->CreateNewRequest( CNAME( TeleportToWork ) );
		if ( distToAPSqr > NumericLimits< Float >::Epsilon() )
		{
#ifdef DEBUG_PERFORM_WORK_ADJUSTMENT
			m_debugShift = translation.AsVector3();
#endif
			if ( immediateAdjustment && !m_ignoreCollisions )
			{
				request->SlideTo( actionExecPos )->AdjustLocationVertically();
			}
			else
			{
				request->SlideBy( translation )->AdjustLocationVertically(); // In this case, sliding have to cooperate with animation movement.
			}
		}
		if ( Abs( rotation ) > NumericLimits< Float >::Epsilon() )
		{
			request->RotateBy( rotation );
#ifdef DEBUG_PERFORM_WORK_ADJUSTMENT
			m_debugRotation = rotation;
#endif
		}
		request->AdjustmentDuration( immediateAdjustment ? 0.f : m_jobContext.m_currentActionDuration );
	}
}

void CBehTreeNodePerformWorkInstance::AdjustLeaveAction( Bool considerCurrentAnimation )
{
	CActor* actor = m_owner->GetActor();
	CMovingAgentComponent* mac = actor->GetMovingAgentComponent();

	Vector actionSafePos;
	Float actionSafeOrientation;

	// lazy-compute action point 'safe' position
	if ( !GCommonGame->GetSystem< CCommunitySystem >()->GetActionPointManager()->GetSafePosition( m_currentApId, &actionSafePos, &actionSafeOrientation ) )
	{
		return;
	}

	const Vector& actorPos = mac->GetWorldPositionRef();
	Vector targetPos = actorPos;
	Float leaveDuration = 0.f;

	CPathAgent* pathAgent = mac->GetPathAgent();
	CMovementAdjustor* adjustor = mac->GetMovementAdjustor();
	adjustor->CancelAll();

	if ( considerCurrentAnimation && m_currentAction )
	{
		Float actorYaw = mac->GetWorldYaw();

		Vector leaveTranslation;
		Float leaveRotation;
		
		m_currentAction->GetMotionExtraction( mac, leaveTranslation, leaveRotation, &leaveDuration );
		targetPos.AsVector2() += MathUtils::GeometryUtils::Rotate2D( leaveTranslation.AsVector2(), DEG2RAD( actorYaw ) );
	}


	if ( !pathAgent->TestLocation( targetPos ) )
	{
		pathAgent->DoZCorrection( actionSafePos.AsVector3() );

		SMovementAdjustmentRequest* request = adjustor->CreateNewRequest( CNAME( TeleportToWork ) );
		Vector translation = actionSafePos - targetPos;
		request->SlideBy( translation )->AdjustLocationVertically();
		request->AdjustmentDuration( leaveDuration * 0.98f );
	}
}

CBehTreeNodePerformWorkInstance::EProcessLogicResult CBehTreeNodePerformWorkInstance::ProcessLogic()
{
	if( m_workPaused )
	{
		return PLR_InProgress;
	}

	EProcessLogicResult result = PLR_InProgress;

	CActor* actor = m_owner->GetActor();	

	Bool keepProcessing = true;

	m_freezer.Update( actor, GetOwner()->GetLocalTimeDelta(), m_jobContext.m_isLeaf && !m_actionContext.IsUsingItems() );

	while( keepProcessing )
	{
		// State machine
		switch( m_workActivity )
		{
		case WA_Initialize:
			{
				CMovingAgentComponent* mac = actor->GetMovingAgentComponent();
				ASSERT( mac );

				if ( !mac->IsMotionEnabled() )
				{
					keepProcessing = false;
					break;
				}

				OnWorkSequenceStart();

				ASSERT( m_currentAction == NULL );
				if ( m_currentApId == ActionPointBadID || !mac )
				{
					m_workActivity = WA_NextAction;
				}
				else
				{
					m_workActivity = WA_InitialPositionAdjustment;
				}
				//Bool isTryingToSpawnToWork = m_workData->IsTryingToSpawnToWork( m_owner ) || m_immediateActivation;
				keepProcessing = true;

				break;
			}

		case WA_InitialPositionAdjustment:
			{
				ASSERT( m_currentApId != ActionPointBadID );
				Vector actionExecPos;
				Float actionExecOrientation;

				CActionPointManager* actionPointManager = GCommonGame->GetSystem< CCommunitySystem >()->GetActionPointManager();

				if ( !actionPointManager->GetActionExecutionPosition( m_currentApId, &actionExecPos, &actionExecOrientation ) )
				{
					m_workActivity = WA_Stopping;
					actionExecPos = Vector::ZERO_3D_POINT;
					actionExecOrientation = 0.0f;
					break;
				}

				Bool isPrecisePlacementNeeded = false;

				CJobTree* jobTreePtr = m_jobTree.Get();

				if ( jobTreePtr )
				{
					isPrecisePlacementNeeded = jobTreePtr->GetSettings().m_needsPrecision && m_ignoreCollisions;
				}

				CMovingAgentComponent* mac = actor->GetMovingAgentComponent();
				ASSERT( mac );
				if( mac )
				{
					if ( mac->GetRagdollPhysicsWrapper() && ! mac->GetRagdollPhysicsWrapper()->IsKinematic() )
					{
						// don't adjust ragdolled characters - there has to be a reason for them to be in ragdoll and since they were ragdolled, they could move a lot
						keepProcessing = false;
					}
					else
					{
						// apply action start translation - as sometimes we ommit initial animation
						Vector executionPosTranslation = m_jobContext.m_currentTranslation;
						executionPosTranslation.AsVector2() = MathUtils::GeometryUtils::Rotate2D( executionPosTranslation.AsVector2(), DEG2RAD( actionExecOrientation ) );

						actionExecPos += executionPosTranslation;
						actionExecOrientation += m_jobContext.m_currentRotation;

						Vector translation = actionExecPos - mac->GetWorldPosition();
						Float rotation = EulerAngles::AngleDistance( mac->GetWorldYaw(), actionExecOrientation );

						Bool initialPositionReached = true;
						Float distToAP = 0.0f;

						if ( isPrecisePlacementNeeded == true )
						{
							distToAP = translation.Mag3();
							initialPositionReached = distToAP <= CActionPointComponent::PRECISE_PLACEMENT_MARGIN && MAbs( rotation ) <= CActionPointComponent::PRECISE_ANGLE_MARGIN;
						}
						else
						{
							distToAP = translation.Mag2();
							initialPositionReached = distToAP <= CActionPointComponent::PRECISE_PLACEMENT_MARGIN && MAbs( rotation ) <= CActionPointComponent::PRECISE_ANGLE_MARGIN && MAbs( translation.Z ) < 2.f;
							
							if ( !m_ignoreCollisions )
							{
								translation.Z = 0.0f;
							}
						}

						if ( initialPositionReached == false )
						{
							Bool isTryingToSpawnToWork = m_immediateActivation;
							if ( distToAP > 2.0f || isTryingToSpawnToWork )
							{
								mac->TeleportTo( actionExecPos, EulerAngles( 0.0f, 0.0f, mac->GetWorldYaw() + rotation ), false );
								m_workActivity = WA_NextAction;	
								keepProcessing = isTryingToSpawnToWork;
							}
							else
							{
								mac->Slide( translation, EulerAngles( 0.0f, 0.0f, rotation ) );
								keepProcessing = false;
							}
							
						}
						else
						{
							mac->Slide( Vector::ZERO_3D_POINT, EulerAngles( 0.0f, 0.0f, 0.0f ) );
							m_workActivity = WA_NextAction;
							keepProcessing = true;
						}
					}
				}
				else
				{
					m_workActivity = WA_NextAction;
					keepProcessing = true;
				}

				break;
			}

		case WA_StartWorking:
			{
				if ( StartAnimation( WORK_ANIM_SLOT_NAME, m_currentAction ) )
				{
					UpdateIsSitting();
					m_freezer.ActorPlayAnimation( actor, m_jobContext.m_isLeaf && !m_actionContext.IsUsingItems() );

#ifdef DEBUG_PERFORM_WORK_ADJUSTMENT
					m_debugActionStartedLocation = actor->GetWorldPositionRef().AsVector3();
#endif
					AdjustPosIfNeeded();

					m_workActivity = WA_Animation;
					OnJobActionStarted();
				}
				else
				{					
					m_workActivity = WA_Failing;
				}
				break;
			}

		case WA_ProcessItems:
			{
				SetupItems();
				m_workActivity = WA_StandbyItems;
			}
			// No need to break now

		case WA_StandbyItems:
			{
				if ( !actor->HasLatentItemAction() )
				{
					m_workActivity = WA_StartWorking;
					keepProcessing = true;
				}
				else
				{
					keepProcessing = false;
				}
				break;
			}

		case WA_NextAction:
			{
				OnJobActionFinished();

				m_currentAction = GetNextAction();

				if ( m_currentAction )
				{
					if ( m_jobContext.IsCurrentActionLeave() )
					{
						AdjustLeaveAction( true );
					}
					else if ( m_ignoreCollisions )
					{
						m_isAdjusted = false;
					}
					m_workActivity = WA_ProcessItems;
					actor->SetForcedLookatLevel( true, m_currentAction->GetAllowedLookatLevel() );
				}
				else
				{
					if ( !m_isDone )
					{
						SActionPointId newApId;
						m_owner->GetActor()->SignalGameplayEvent( CNAME( WorkDone ), &newApId, SActionPointId::GetStaticClass() );
						AdjustLeaveAction( false );
						m_isDone = true;
					}
					if ( m_inFastLeaveAnimation ) //if there's no next action and we're in fast leave, means we're trying to break out by failing
						m_workActivity = WA_Failing;
					else
						m_workActivity = WA_Stopping;
				}
				break;
			}
		case WA_Animation:
			{								
				if ( m_freezer.IsAnimTimeoutOccured() )
				{
					m_freezer.ResetAnimTimer();
					m_workActivity = WA_NextAction;
					keepProcessing  = true;
				}
				else
				{
					// z correction
					if ( m_isZCorrectionPending )
					{
						CActor* actor = m_owner->GetActor();
						// check guys visibility
						if ( actor->GetLastFrameVisibility() == RVR_NotVisible )
						{
							CMovingAgentComponent* mac = m_owner->GetActor()->GetMovingAgentComponent();
							// check if guy has physics streamed in around
							if ( ( mac->GetPhysicalRepresentationDisableFlags() & CMovingAgentComponent::LS_Force ) == 0 )
							{
								m_isZCorrectionPending = false;
								mac->GetPathAgent()->ForceInstantZCorrection();
							}
						}
					}
					keepProcessing = false;
				}

				break;
			}

		case WA_BreakAnimation:
			{
				if( m_executionMode == JEM_ForcedBreaking )
				{
					actor->RaiseBehaviorEvent( CNAME( BlendOutFromWork ) );
				}
				m_animated->GetBehaviorStack()->StopSlotAnimation( WORK_ANIM_SLOT_NAME, m_globalBreakingBlendOutTime );

				if ( m_executionMode == JEM_Normal || m_jobContext.IsCurrentActionApproach() )
				{
					m_workActivity = WA_Stopping;
					AdjustLeaveAction( false );
				}
				else
				{
					m_workActivity = WA_NextAction;
				}
				break;
			}

		case WA_Stopping:
			{
				if( m_executionMode == JEM_Breaking || m_executionMode == JEM_ForcedBreaking )
				{
					m_owner->GetInstanceRootNode()->MarkActiveBranchDirty();
				}	
				OnWorkSequenceFinished();
				keepProcessing = false;
				result = PLR_Success;
				break;
			}

		case WA_Failing:
			{
				OnWorkSequenceFinished();
				keepProcessing = false;				
				result = PLR_Faild;
				break;
			}
		case WA_Completed:
		case WA_JustBreak:
			{
				keepProcessing = false;
				result = PLR_Success;
				break;
			}
		}
	}

	return result;
}

void CBehTreeNodePerformWorkInstance::Update()
{
	const EProcessLogicResult ret = ProcessLogic();
	if ( ret == PLR_Success )
	{
		Complete( BTTO_SUCCESS );
	}
	else if ( ret == PLR_Faild )
	{
		Complete( BTTO_FAILED );
	}
	else
	{
		ASSERT( ret == PLR_InProgress );
	}
}

void CBehTreeNodePerformWorkInstance::UpdateIsSitting( )
{
	CBehTreeWorkData* workData = m_workData.Get();
	if( !workData )
		return;

	String animName = m_currentAction->GetAnimName().AsString();	
	Bool isSitting = animName.ContainsSubstring( TXT( "sit" ) ) && !animName.ContainsSubstring( TXT( "stop") );
	workData->SetIsSitting( isSitting );
}

void CBehTreeNodePerformWorkInstance::OnWorkSequenceStart()
{
	CNewNPC* npc = m_owner->GetNPC();

	CActionPointManager* actionPointManager = GCommonGame->GetSystem< CCommunitySystem >()->GetActionPointManager();

	CMovingAgentComponent* mac = npc->GetMovingAgentComponent();
	
	if( mac )
	{		
		mac->SnapToNavigableSpace( false );
		if( m_ignoreCollisions )
		{
			mac->ForceEntityRepresentation( true, CMovingAgentComponent::LS_JobTree );
		}
		if ( CPathAgent* pathAgent = mac->GetPathAgent() )
		{
			// force height snapping while we are in 'animated movement'
			pathAgent->ForceHeightSnapping( true );
		}
	}

	CMovingPhysicalAgentComponent* physMac = Cast< CMovingPhysicalAgentComponent >( mac );
	if( physMac )
	{	
		RED_FATAL_ASSERT( physMac->AccessAnimationProxy().UseExtendedIKOffset() == false, "Please check ext ik logic");


		if( m_ignoreCollisions )
		{
			// physical						
			physMac->EnableDynamicCollisions( false );									
		}
		physMac->SetAnimatedMovement( true );
		physMac->EnableStaticCollisions( false );
		physMac->InvalidatePhysicsCache();		

		// visual		
		physMac->SetSkipUpdateAndSampleFramesBias( 2, 0 ); // can't use addition here as it may break flow when being close
		physMac->SetSkipUpdateAndSampleFramesLimit( 3 );

		CActionPointComponent* actionPoint = actionPointManager->GetAP( m_currentApId );
		if( actionPoint && actionPoint->KeepIKActive() )
		{
			physMac->AccessAnimationProxy().SetUseExtendedIKOffset( true );
			m_disabledIK = false;
		}		
		else
		{
			physMac->AccessAnimationProxy().EnableIKDueToWork( false );
			m_disabledIK = true;
		}

		// place a static obstacle around you, providing it's required
		if ( actionPointManager->IsBreakable( m_currentApId ) )
		{
			physMac->SetInteractionPriority( BREAKABLE_WORK_INTERACTION_PRIORITY );
		}
		else
		{
			physMac->SetInteractionPriority( WORK_INTERACTION_PRIORITY );
		}
	}
	
	// inform that we're currently working in the AP
	npc->SetCurrentlyWorkingInAP( m_currentApId );	

	// setup items for the first action
	SetupItems();
}

void CBehTreeNodePerformWorkInstance::OnWorkSequenceFinished()
{
	CCommunitySystem *const communitySystem = GCommonGame->GetSystem< CCommunitySystem >();
	if ( communitySystem == NULL || communitySystem->GetActionPointManager().Get() == NULL )
	{
		// Needed here because the community system may be released before the BT when closing the tool.
		return;
	}	

	CNewNPC* npc = m_owner->GetNPC();

	// if execution was interrupted we drop item first
	if ( m_executionMode == JEM_ForcedBreaking )
	{
		CJobTree* jobTree = m_jobTree.Get();
		if ( jobTree )
		{
			jobTree->ApplyJobTreeExitSettings( npc, true, m_jobContext );
		}
		PutOutAny( m_actionContext );
	}
	// otherwise we try to put it away
	else
	{
		PutOutAny( m_actionContext );
		CJobTree* jobTree = m_jobTree.Get();
		if ( jobTree )
		{
			jobTree->ApplyJobTreeExitSettings( npc, false, m_jobContext );
		}
	}

	m_workActivity = WA_Completed;
	m_jobTreeRoot = NULL;
	m_currentAction = NULL;
	m_executionMode = JEM_Normal;
	m_actionContext.Reset();

	CMovingAgentComponent* mac = npc->GetMovingAgentComponent();

	if( mac )
	{		
		mac->SnapToNavigableSpace( true );		
		mac->ForceEntityRepresentation( false, CMovingAgentComponent::LS_JobTree );
		if ( CPathAgent* pathAgent = mac->GetPathAgent() )
		{
			pathAgent->ForceHeightSnapping( false );
		}
	}

	CMovingPhysicalAgentComponent* physMac = Cast< CMovingPhysicalAgentComponent >( mac );
	
	if( physMac )
	{
		physMac->RestoreOriginalInteractionPriority();
				
		// physical
		physMac->SetAnimatedMovement( false );
		physMac->EnableStaticCollisions( true );
		physMac->EnableDynamicCollisions( true );
		physMac->InvalidatePhysicsCache();

		// visual
		physMac->SetSkipUpdateAndSampleFramesBias();
		physMac->SetSkipUpdateAndSampleFramesLimit();

		if( m_disabledIK )
		{
			physMac->AccessAnimationProxy().EnableIKDueToWork( true );
		}
		else
		{
			physMac->AccessAnimationProxy().SetUseExtendedIKOffset( false );
		}

		// make sure the agent's not sliding any more
		physMac->Slide( Vector::ZERO_3D_POINT, EulerAngles( 0.0f, 0.0f, 0.0f ) );

		RED_FATAL_ASSERT( physMac->AccessAnimationProxy().UseExtendedIKOffset() == false, "Please check ext ik logic");
	}

	npc->SetCurrentlyNotWorkingInAP();

	m_freezer.FinishWorking( npc );

	CBehTreeWorkData* workData = m_workData.Get();
	if( workData )
	{
		workData->SetIgnoreHadReactions( false );
		workData->SetIsConscious( true );
		workData->SetIsSitting( false );	
		workData->SetJTType( 0 );
	}

	npc->SetForcedLookatLevel( false );
	PauseSotAnimation( false );
}

void CBehTreeNodePerformWorkInstance::OnJobActionStarted()
{
	if ( m_currentAction )
	{
		m_currentAction->OnStarted( m_currentApId, m_actionContext );
	}

	RegisterAnimEventHandler();
}

void CBehTreeNodePerformWorkInstance::OnJobActionFinished()
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

const CJobActionBase* CBehTreeNodePerformWorkInstance::GetNextAction()
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
		
		if ( !m_isDone &&
			( action == nullptr || m_jobContext.IsCurrentActionLeave() ) )
		{
			// Deal with post animation when the next AP will be the same
			SActionPointId newApId;
			m_owner->GetNPC()->SignalGameplayEvent( CNAME( WorkDone ), &newApId, SActionPointId::GetStaticClass() );
			
			if ( newApId == ActionPointBadID || newApId == m_currentApId )
			{
				ResetContexts();

				action = jobTreeNode->GetNextAction( m_jobContext );
				if ( m_jobContext.IsCurrentActionApproach() && ( !action || action->IsSkippable() ) )
				{
					// Omit pre
					action = jobTreeNode->GetNextAction( m_jobContext );
				}
			}
			else
			{
				m_isDone = true;
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
		m_inFastLeaveAnimation = true;
		break;
	}

	return action;
}

void CBehTreeNodePerformWorkInstance::SetupItems()
{
	CActor* actor = m_owner->GetActor();
	CInventoryComponent* npcInventory = actor->GetInventoryComponent();
	if ( !npcInventory )
	{
		return;
	}

	// The below code sets anyActionsStarted value to true, without checking value returned by latent draw/holster methods.
	// This can cause unnecessary 1 frame idle when item doesn't support latent draw/holster actions properly.
	// However it won't cause problems, and on top of that, such items shouldn't exist in the final game
	m_spawnedLeftItem	= m_jobContext.m_leftItem;
	m_spawnedRightItem	= m_jobContext.m_rightItem;

	actor->IssueRequiredItems( SActorRequiredItems( m_jobContext.m_leftItem, m_jobContext.m_rightItem ) );

	m_jobContext.m_leftItem = CNAME( Any );
	m_jobContext.m_rightItem = CNAME( Any );

	FireSkippedItemEvents();
}

void CBehTreeNodePerformWorkInstance::FireSkippedItemEvents()
{
	struct SItemEventCollector
	{
		CAnimatedComponent*			m_animatedComponent;
		const CJobAction*			m_action;
		SJobActionExecutionContext&	m_actionContext;
		TActionPointID				m_apId;


		SItemEventCollector( CAnimatedComponent* animatedComponent, const CJobAction* action, SJobActionExecutionContext& actionContext, TActionPointID apId )
			: m_animatedComponent( animatedComponent )
			, m_action( action )
			, m_actionContext( actionContext )
			, m_apId( apId )
		{}

		void operator()( const CExtAnimEvent* e ) const
		{
			if ( e == nullptr )
			{
				return;
			}
			CClass* eventClass = e->GetClass();
			if ( eventClass == nullptr )
			{
				return;
			}
			// CExtAnimEvent may be named "take_item" or "leave_item" but still should be processed as CExtAnimEvent.
			// That's why we check this condition at the very beginning.
			if ( eventClass->IsA< CExtAnimItemEvent >() )
			{
				CAnimationEventFired fired;
				fired.m_extEvent = e;
				e->Process( fired, m_animatedComponent );
			}
			// Then we check for event name, cause it's more common that other types of events
			// and there's a big chance that we won't need to perform tons of type checks. 
			else if ( e->GetEventName() == CNAME( take_item ) )
			{
				m_action->PerformPickUp( m_apId, m_actionContext );
			}
			else if ( e->GetEventName() == CNAME( leave_item ) )
			{
				m_action->PerformPut( m_apId, m_actionContext );
			}
			else if ( eventClass->IsA< CExtAnimItemEffectEvent >() ||
				 eventClass->IsA< CExtAnimItemEffectDurationEvent >() ||
				 eventClass->IsA< CExtAnimItemAnimationEvent >() ||
				 eventClass->IsA< CExtAnimItemBehaviorEvent >() ||
				 eventClass->IsA< CExtAnimDropItemEvent >() ||
				 eventClass->IsA< CExtAnimItemSyncEvent >() ||
				 eventClass->IsA< CExtAnimItemSyncDurationEvent >() ||
				 eventClass->IsA< CExtAnimItemSyncWithCorrectionEvent >() ||
				 eventClass->IsA< CExtAnimReattachItemEvent >() )				 
			{
				CAnimationEventFired fired;
				fired.m_extEvent = e;
				e->Process( fired, m_animatedComponent );
			}
		}
	};

	const CJobAction* action = m_jobContext.m_skippedItemEventsAction;
	if ( action == nullptr )
	{
		return;
	}

	SItemEventCollector collector( m_animated, action, m_actionContext, m_currentApId );
	action->CollectEvents< CExtAnimEvent >( m_animated, collector );

	// don't fire events again
	m_jobContext.m_skippedItemEventsAction = nullptr;
}

Bool CBehTreeNodePerformWorkInstance::StartAnimation( CName slotName, const CJobActionBase* actionBase )
{
	CActor* actor = m_owner->GetActor();
	// Play animation on slot
	if ( !m_animated->GetBehaviorStack() )
	{
#ifndef NO_ERROR_STATE
		m_owner->GetActor()->SetErrorState( TXT("ActorActionWork: no behavior stack") );
#endif
		return false;
	}

	if ( CMovingAgentComponent* mac = Cast< CMovingAgentComponent >( m_animated ) )
	{
		if ( CPathAgent* pathAgent = mac->GetPathAgent() )
		{
			// if we start playing animation, we shouldn't snap to navigation Z anymore (until of course we are out of that shit)
			pathAgent->ForceHeightSnapping( false );
		}
	}

	float blendInTime	= actionBase->GetBlendIn();
	float blendOutTime	= actionBase->GetBlendOut();

	if( m_immediateActivation && m_startAnimationWithoutBlend )
	{
		blendInTime = 0;
		blendOutTime= 0;
		m_startAnimationWithoutBlend = false;
	}

	// Setup slot

	const CJobForceOutAction* fastLEaveAnim = Cast< CJobForceOutAction >( actionBase );

	SBehaviorSlotSetup slotSetup;
	slotSetup.m_blendInType = BTBM_Destination;
	slotSetup.m_blendIn = blendInTime;
	slotSetup.m_blendOutType = BTBM_Source;
	slotSetup.m_blendOut = blendOutTime;
	slotSetup.m_mergeBlendedSlotEvents = actionBase->ShouldFireBlendedEvents();
	slotSetup.m_listener = this;
	slotSetup.m_offset = 0.f;
	if( fastLEaveAnim )
	{
		slotSetup.m_speedMul = fastLEaveAnim->GetSpeedMul();
	}
	else
	{
		// Get game config
		SGameplayConfig& gameplayConfig = GGame->GetGameplayConfig();
		slotSetup.m_speedMul = GEngine->GetRandomNumberGenerator().Get< Float >( gameplayConfig.m_workAnimSpeedMulMin , gameplayConfig.m_workAnimSpeedMulMax );
	}	

	if ( m_initialAnimationWithOffset )
	{
		slotSetup.m_offset = m_startAnimationOffset;
		m_initialAnimationWithOffset = false;
	}
	

	// Play animation
	if ( !m_animated->GetBehaviorStack()->PlaySlotAnimation( slotName, actionBase->GetAnimName(), &slotSetup ) )
	{
#ifndef NO_ERROR_STATE
		m_owner->GetActor()->SetErrorState( String::Printf( TXT("ActorActionWork: unable to play animation %s"), actionBase->GetAnimName().AsString().AsChar() ) );
#endif
		WORK_WARN( TXT("Couldn't play animation %s, job tree: %s, npc: %s"), actionBase->GetAnimName().AsString().AsChar(), m_jobTree.Get()->GetDepotPath().AsChar(), m_owner->GetActor()->GetFriendlyName().AsChar() );
		return false;
	}

	// We are animating
	return true;
}

RED_INLINE void CBehTreeNodePerformWorkInstance::RegisterAnimEventHandler()
{
	// Register this action as animation event handler
	m_animated->GetAnimationEventNotifier( CNAME( take_item ) )->RegisterHandler( this );
	m_animated->GetAnimationEventNotifier( CNAME( leave_item ) )->RegisterHandler( this );
	m_animated->GetAnimationEventNotifier( CNAME( drop_item ) )->RegisterHandler( this );
}

RED_INLINE void CBehTreeNodePerformWorkInstance::UnregisterAnimEventHandler()
{
	m_animated->GetAnimationEventNotifier( CNAME( take_item ) )->UnregisterHandler( this );
	m_animated->GetAnimationEventNotifier( CNAME( leave_item ) )->UnregisterHandler( this );
	m_animated->GetAnimationEventNotifier( CNAME( drop_item ) )->UnregisterHandler( this );
}

void CBehTreeNodePerformWorkInstance::HandleEvent( const CAnimationEventFired &event )
{
	if ( m_jobTreeRoot.Get() && m_currentAction )
	{
		m_currentAction->OnAnimEvent( m_currentApId, m_actionContext, event );
	}
}

void CBehTreeNodePerformWorkInstance::OnSlotAnimationEnd( const CBehaviorGraphAnimationBaseSlotNode * sender, CBehaviorGraphInstance& instance, ISlotAnimationListener::EStatus status )
{
	if( m_workActivity == WA_BreakAnimation || m_workActivity == WA_JustBreak )
	{
		return;
	}
	// Skip current node if animation failed
	if( status == ISlotAnimationListener::S_Finished || status == ISlotAnimationListener::S_BlendOutStarted || status == ISlotAnimationListener::S_Stopped )
	{	
		// End this job action and proceed to the next one
		m_workActivity = WA_NextAction;	
	}
	else
	{			
		m_workActivity = WA_Failing;	
	}
}

void CBehTreeNodePerformWorkInstance::UnmountItems()
{
	CInventoryComponent* npcInventory = m_owner->GetNPC()->GetInventoryComponent();
	if( !npcInventory )
		return;

	if( m_spawnedLeftItem )
	{		
		npcInventory->UnMountItem( npcInventory->GetItemId( m_spawnedLeftItem ), false );
	}	
	if( m_spawnedRightItem )
	{		
		npcInventory->UnMountItem( npcInventory->GetItemId( m_spawnedRightItem ), false );
	}

	m_spawnedLeftItem	= CName::NONE;
	m_spawnedRightItem	= CName::NONE;
}

void CBehTreeNodePerformWorkInstance::PutOutAny( SJobActionExecutionContext& actionContext )
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

void CBehTreeNodePerformWorkInstance::PutOutAny( SJobActionExecutionContext& actionContext, SJobActionExecutionContext::SItemInfo& itemInfo )
{
	if ( itemInfo.m_apItem )
	{
		// Get required pointers
		CNewNPC* npc = actionContext.m_NPCActor;
		ASSERT( npc );
		CInventoryComponent* npcInventory = npc->GetInventoryComponent();

		CActionPointManager* apMgr = GCommonGame->GetSystem< CCommunitySystem >()->GetActionPointManager();
		CInventoryComponent* apInventory = ( m_currentApId != ActionPointBadID ) ? apMgr->GetInventory( m_currentApId ) : NULL;
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
					CItemEntityProxy* droppedProxy = npcInventory->GetDroppedProxy( itemInfo.m_npcItem );
					// if item was dropped, let's try to claim ownership and wait for drop timeout to mount it again
					if ( droppedProxy != nullptr && SItemEntityManager::GetInstance().ClaimDroppedItem( droppedProxy, apMgr ) )
					{
						apItem->SetProxy( droppedProxy, false );
						apItem->RevalidateProxy();
						apMgr->RegisterDroppedItem( m_currentApId, droppedProxy );
					}
					else
					{
						// Mount item back to AP
						CInventoryComponent::SMountItemInfo mountInfo;
						mountInfo.m_force = true;
						apInventory->MountItem( itemInfo.m_apItem, mountInfo );
					}
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

Bool CBehTreeNodePerformWorkInstance::ExitWorking( Bool fast /*= false */ )
{
	if ( m_executionMode == JEM_Breaking || m_executionMode == JEM_ForcedBreaking )
	{
		// already breaking
		return true;
	}

	if ( m_currentAction != NULL && m_jobContext.IsCurrentActionLeave() && fast == false )
	{
		// already ending work
		return true;
	}

	if ( m_workActivity == WA_Animation || m_workActivity == WA_NextAction )
	{
		if ( fast )
		{
			m_executionMode = JEM_ForcedBreaking;
		}
		else
		{
			m_executionMode = JEM_Breaking;
		}
		m_workActivity = WA_BreakAnimation;
	}

	if( m_workActivity == WA_Initialize || m_workActivity == WA_InitialPositionAdjustment )
	{
		m_workActivity = WA_JustBreak;
	}

	return true;
}

void CBehTreeNodePerformWorkInstance::PauseSotAnimation( Bool pause )
{
	if( m_animated->GetBehaviorStack() )
	{
		m_animated->GetBehaviorStack()->PauseSlotAnimation( WORK_ANIM_SLOT_NAME, pause );
	}
	m_workPaused = pause;
}

Bool CBehTreeNodePerformWorkInstance::OnEvent( CBehTreeEvent& e )
{
	if ( e.m_eventName == CNAME( OnTimetableChanged ) ||  e.m_eventName == CNAME( AI_Load_Forced ) )
	{
		return ExitWorking( false );
	}
	else if( e.m_eventName == CNAME( AI_GetOutOfTheWay ) )
	{		
		CActionPointManager* actionPointManager = GCommonGame->GetSystem< CCommunitySystem >()->GetActionPointManager();
		if( actionPointManager->IsBreakable( m_currentApId ) )
		{
			return ExitWorking( true );
		}
	}
	else if ( e.m_eventName == SPlaySceneRequestData::EVENT_ID )
	{
		SPlaySceneRequestData* data = e.m_gameplayEventData.Get< SPlaySceneRequestData >();
		if ( data != NULL && data->m_enablePlayScene == true && data->m_isGameplayScene == false )
		{
			OnWorkSequenceFinished();
			Complete( BTTO_SUCCESS );

			if ( CBehaviorGraphStack* const behaviorStack = m_animated->GetBehaviorStack() )
			{
				behaviorStack->StopSlotAnimation( WORK_ANIM_SLOT_NAME );
			}
			return false;
		}
		return true;
	}
	else if ( e.m_eventName == CNAME( AI_CantPush ) )
	{
		SGameplayEventParamInt* params = e.m_gameplayEventData.Get< SGameplayEventParamInt >();
		if ( params )
		{
			SActionPointId apId	= m_workData->GetSelectedAP();
			CActionPointManager* actionPointManager = GCommonGame->GetSystem< CCommunitySystem >()->GetActionPointManager();

			if( actionPointManager && !actionPointManager->IsBreakable( apId ) )
			{
				params->m_value	= 1;
			}
			else
			{
				params->m_value	= 0;
			}
			return false;
		}
	}
	else if ( e.m_eventName == CNAME( AI_ForceInterruption ) )
	{
		m_workData->Interrupt();

		Complete( BTTO_SUCCESS );

		if ( CBehaviorGraphStack* const behaviorStack = m_animated->GetBehaviorStack() )
		{
			behaviorStack->StopSlotAnimation( WORK_ANIM_SLOT_NAME );
		}

		return false;
	}
	else if ( e.m_eventName == CNAME( AI_CanTeleportOutOfSceneArea ) )
	{
		SGameplayEventParamInt* params = e.m_gameplayEventData.Get< SGameplayEventParamInt >();
		if ( params )
		{
			params->m_value = 0;
		}
		return false;
	}
	else if( e.m_eventName == CNAME( AI_PauseWorkAnimation ) )
	{		
		PauseSotAnimation( true );	
	}
	else if( e.m_eventName == CNAME( AI_UnPauseWorkAnimation ) )
	{		
		PauseSotAnimation( false );
	}

	return false;
}

Bool CBehTreeNodePerformWorkInstance::OnListenedEvent( CBehTreeEvent& e )
{
	if ( e.m_eventName == CNAME( AI_ForceLastWork ) )
	{		
		CActor* actor = m_owner->GetActor();
		m_workData->ImmediateActivation( m_owner->GetLocalTime() + 0.75f );
		m_workData->ForceProcessOnActivation();	
		return true;
	}
	
	return false;
}

Int32 CBehTreeNodePerformWorkInstance::Evaluate()
{
	return HasValidForceRequest() ? FORCE_REQUEST_PRIORITY_BONUS : m_priority;
}

Bool CBehTreeNodePerformWorkInstance::HasValidForceRequest() const
{
	return m_workData->IsInImmediateActivation( m_owner );
}

void CBehTreeNodePerformWorkInstance::ResetContexts()
{
	m_jobContext.Reset();
	m_jobContext.m_currentCategories.PushBack( m_currentCategory );
	m_jobContext.m_workerInventoryComponent = m_owner->GetActor()->FindComponent< CInventoryComponent >();
	m_actionContext.Reset();
	m_actionContext.m_NPCActor = m_owner->GetNPC();
}


void CBehTreeNodePerformWorkInstance::OnGenerateDebugFragments( CRenderFrame* frame )
{
	Super::OnGenerateDebugFragments( frame );

#ifndef NO_EDITOR
	CActor* actor = m_owner->GetActor();
	if ( !actor )
		return;

	Vector pos = actor->GetWorldPosition();
	Int32 xOffset = -200;
	Int32 yOffset = -5;

	if ( m_jobTree )
	{
		String npcName( actor->GetName().AsChar() );
		String treeName( m_jobTree->GetFile()->GetFileName() );

		if ( m_currentAction )
		{
			String animType = m_currentAction->GetAnimType().AsString();

			//an empty string means it's a regular animation (JobTreeEditor's fault)
			if ( animType == TXT("None") )
			{
				animType = TXT("Anim");
			}

			CActionPointManager* actionPointManager = GCommonGame->GetSystem< CCommunitySystem >()->GetActionPointManager();
			CActionPointComponent* actionPoint = actionPointManager->GetAP( m_currentApId );

			String animName = m_currentAction->GetAnimName().AsString();
			String processName = GetDebugWorkActivityString();

			String finalText0 = 
				String::Printf( TXT( "%s: %s %s%s" ), npcName.AsChar(), treeName.AsChar(), m_ignoreCollisions ? TXT("[ignore col]") : TXT("[normal COLLISION]"), m_immediateActivation ? TXT(" [IMMEDIATE]") : TXT("") );
			String finalText1 =
				String::Printf( TXT( "%s: %s   %s" ), animType.AsChar(), animName.AsChar(), processName.AsChar() );
#ifdef DEBUG_PERFORM_WORK_ADJUSTMENT
			String finalText2 = m_isAdjusted
				? String::Printf( TXT( "%0.2f, %0.2f, %0.2f  rotation: %0.2f"), m_debugShift.X, m_debugShift.Y, m_debugShift.Z, m_debugRotation )
				: String( TXT("No adjustment" ) );
#endif


			frame->AddDebugText( pos, finalText0, xOffset, yOffset++ );
			frame->AddDebugText( pos, finalText1, xOffset, yOffset++ );
#ifdef DEBUG_PERFORM_WORK_ADJUSTMENT
			frame->AddDebugText( pos, finalText2, xOffset, yOffset++ );
#endif
#ifdef DEBUG_ACTION_POINT_RESERVATION
			{
				String text = String::Printf( TXT("Work reserved AP [%08X]"), m_currentApId.CalcHash() );
				frame->AddDebugText( m_owner->GetActor()->GetWorldPositionRef(), text, -180, yOffset++ );
			}
#endif

#ifdef DEBUG_PERFORM_WORK_ADJUSTMENT
			frame->AddDebugCircle( pos, 0.1f, Matrix::IDENTITY, Color::CYAN, 6, true );
			if ( m_jobContext.m_trackExecutionPosition && actionPoint )
			{
				Vector actionExecPos = actionPoint->GetWorldPosition();
				Float actionExecOrientation = actionPoint->GetWorldYaw();

				Vector currentTranslation = m_jobContext.m_currentTranslation;
				Vector nextTranslation = m_jobContext.m_nextTranslation;

				currentTranslation.AsVector2() = MathUtils::GeometryUtils::Rotate2D( currentTranslation.AsVector2(), DEG2RAD( actionExecOrientation ) );
				nextTranslation.AsVector2() = MathUtils::GeometryUtils::Rotate2D( nextTranslation.AsVector2(), DEG2RAD( actionExecOrientation ) );

				Vector expectedPrevExecutionPos = actionPoint->GetWorldPositionRef() + currentTranslation;
				frame->AddDebugCircle( expectedPrevExecutionPos, 0.12f, Matrix::IDENTITY, Color::YELLOW, 6, true );
				Vector expectedNextExecutionPos = actionPoint->GetWorldPositionRef() + nextTranslation;
				frame->AddDebugCircle( expectedNextExecutionPos, 0.1f, Matrix::IDENTITY, Color::RED, 6, true );
				frame->AddDebugCircle( m_debugActionStartedLocation, 0.08f, Matrix::IDENTITY, Color::BLUE, 6, true );
			}
#endif
		}
		else
		{
			frame->AddDebugText( pos, TXT("NO JOBTREE INFO: no job action base found\n" ) + GetDebugWorkActivityString(), xOffset, yOffset++ );
		}
	}
	else
	{
		frame->AddDebugText( pos, TXT("NO JOBTREE INFO: no job tree found\n") + GetDebugWorkActivityString(), xOffset, yOffset++ );
	}

#endif
} 

#ifndef NO_EDITOR	
String CBehTreeNodePerformWorkInstance::GetDebugWorkActivityString() const
{
	String result = TXT("ERROR");

	switch ( m_workActivity )
	{
	case EWorkActivity::WA_Initialize:
		result = TXT("WA_Initialize");
		break;

	case EWorkActivity::WA_InitialPositionAdjustment:
		result = TXT("WA_InitialPositionAdjustment");
		break;

	case EWorkActivity::WA_StartWorking:
		result = TXT("WA_StartWorking");
		break;

	case EWorkActivity::WA_NextAction:
		result = TXT("WA_NextAction");
		break;

	case EWorkActivity::WA_Animation:
		result = TXT("WA_Animation");
		break;

	case EWorkActivity::WA_BreakAnimation:
		result = TXT("WA_BreakAnimation");
		break;

	case EWorkActivity::WA_Stopping:
		result = TXT("WA_Stopping");
		break;

	case EWorkActivity::WA_Failing:
		result = TXT("WA_Failing");
		break;

	case EWorkActivity::WA_Idle:
		result = TXT("WA_Idle");
		break;

	case EWorkActivity::WA_ProcessItems:
		result = TXT("WA_ProcessItems");
		break;

	case EWorkActivity::WA_StandbyItems:
		result = TXT("WA_StandbyItems");
		break;

	case EWorkActivity::WA_Completed:
		result = TXT("WA_Completed");
		break;

	case EWorkActivity::WA_JustBreak:
		result = TXT("WA_JustBreak");
		break;
	}

	return result;
}
#endif

void CBehTreeNodePerformWorkInstance::DestroyItemsOnInterrupt()
{
	CInventoryComponent* npcInventory = m_owner->GetActor()->GetInventoryComponent();

	if ( npcInventory )
	{
		SItemUniqueId leftItemID = npcInventory->GetItemIdHeldInSlot( CNAME( l_weapon ) );
		SItemUniqueId rightItemID = npcInventory->GetItemIdHeldInSlot( CNAME( r_weapon ) );

		if ( leftItemID != SItemUniqueId::INVALID )
		{
			const SItemDefinition* itemDef = GCommonGame->GetDefinitionsManager()->GetItemDefinition( npcInventory->GetItem( leftItemID )->GetName() );
	
			if ( itemDef && !itemDef->IsWeapon() )
				m_owner->GetActor()->EmptyHand( CNAME( l_weapon ), true );							
		}

		if ( rightItemID != SItemUniqueId::INVALID )
		{
			const SItemDefinition* itemDef = GCommonGame->GetDefinitionsManager()->GetItemDefinition( npcInventory->GetItem( rightItemID )->GetName() );

			if ( itemDef && !itemDef->IsWeapon() )
				m_owner->GetActor()->EmptyHand( CNAME( r_weapon ), true );							
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// Work freezer helper
//////////////////////////////////////////////////////////////////////////

ActorWorkFreezer::ActorWorkFreezer()
{
	Reset();
}

void ActorWorkFreezer::Reset()
{
	ASSERT( !m_isAnimFreezing );
	m_isAnimFreezing = false;
	m_animFreezingTime = 0.f;
	m_invisibilityTime = 0.f;
}

void ActorWorkFreezer::FreezeActor( CActor* actor )
{
	ASSERT( !m_isAnimFreezing );
	actor->FreezeAllAnimatedComponents();
	m_isAnimFreezing = true;
}

void ActorWorkFreezer::UnfreezeActor( CActor* actor )
{
	ASSERT( m_isAnimFreezing );
	actor->UnfreezeAllAnimatedComponents();
	m_isAnimFreezing = false;
}

void ActorWorkFreezer::RestoreActor( CActor* actor )
{
	if ( m_isAnimFreezing )
	{
		UnfreezeActor( actor );
	}
}

void ActorWorkFreezer::Update( CActor* actor, Float timeDelta, Bool isCurrentActionLeaf )
{
	if ( !GGame->GetGameplayConfig().m_useWorkFreezing )
	{
		if( IsActorFrozen() )
		{
			RestoreActor( actor );
			Reset();
		}		
		return;
	}

	{
		if( IsActorFrozen() )
		{
			Bool unfreeze = false;

			if ( IsActorVisible( actor ) )
			{
				unfreeze = true;
			}
			else
			{
				Float distSq = actor->GetWorldPositionRef().DistanceSquaredTo( actor->GetLayer()->GetWorld()->GetCameraPosition() );
				if ( !IsOutsideInvisibleRange( distSq ) )
				{
					unfreeze = true;
				}
			}
			if ( unfreeze )
			{
				RestoreActor( actor );
				Reset();
			}
			else
			{
				m_animFreezingTime += timeDelta;
			}
		}
		else
		{
			Bool canFreeze = false;
			if ( isCurrentActionLeaf && ( !actor->IsSpeaking() && !actor->IsInNonGameplayScene() ) )
			{
				if ( !IsActorVisible( actor ) )
				{
					Float distSq = actor->GetWorldPositionRef().DistanceSquaredTo( actor->GetLayer()->GetWorld()->GetCameraPosition() );
					if ( IsOutsideInvisibleRange( distSq ) )
					{
						canFreeze = true;
					}
				}
			}

			if ( canFreeze )
			{
				m_invisibilityTime += timeDelta;
				if ( IsInvisibilityTimeoutOccured() )
				{
					FreezeActor( actor );
				}
			}
		}		
	}
}

void ActorWorkFreezer::ActorPlayAnimation( CActor* actor, Bool isCurrentActionLeaf )
{
	if ( IsActorFrozen() )
	{
		UpdateActorPose( actor );
	}
}

void ActorWorkFreezer::UpdateActorPose( CActor* actor ) const
{
	// Ok this code is tricky and low level but this is function for optimalization
	if ( GGame->GetGameplayConfig().m_workResetInFreezing )
	{
		CAnimatedComponent* ac = actor->GetRootAnimatedComponent();
		if ( ac && ac->GetBehaviorStack() && ac->GetBehaviorGraphSampleContext() )
		{
			IBehaviorGraphSlotInterface slot;

			if ( ac->GetBehaviorStack()->GetSlot( WORK_ANIM_SLOT_NAME, slot ) )
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

Bool ActorWorkFreezer::IsActorVisible( CEntity* entity )
{
	return entity->WasVisibleLastFrame();
}

Bool ActorWorkFreezer::IsOutsideInvisibleRange( Float distSqr )
{	
#ifdef RED_PLATFORM_CONSOLE
	const Float radius = 7.0f;
#else
	const Float radius = GGame->GetGameplayConfig().m_workFreezingRadiusForInvisibleActors;
#endif

	// 0.0f defaults to ALWAYS IN RANGE	
	return radius > 0.f ? distSqr > radius * radius : false;
}

Bool ActorWorkFreezer::IsActorFrozen() const
{
	return m_isAnimFreezing;
}

Bool ActorWorkFreezer::IsInvisibilityTimeoutOccured() const
{
#ifdef RED_PLATFORM_CONSOLE
	return m_invisibilityTime >= 0.5f;
#else
	return m_invisibilityTime >= GGame->GetGameplayConfig().m_workFreezingDelay;
#endif
}

Bool ActorWorkFreezer::IsAnimTimeoutOccured() const
{
#ifdef RED_PLATFORM_CONSOLE
	return m_animFreezingTime >= 10.0f;
#else
	return m_animFreezingTime >= GGame->GetGameplayConfig().m_workMaxFreezingTime;
#endif	
}

void ActorWorkFreezer::ResetAnimTimer()
{
	m_animFreezingTime = 0.f;
}

void ActorWorkFreezer::StartWorking( CActor* actor )
{
	RestoreActor( actor );
	Reset();
}

void ActorWorkFreezer::FinishWorking( CActor* actor )
{
	RestoreActor( actor );
	Reset();
}

