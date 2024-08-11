/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "movementTargeter.h"
#include "actionAreaComponent.h"
#include "explorationScriptSupport.h"
#include "movableRepresentationPathAgent.h"
#include "moveSteeringLocomotionSegment.h"
#include "expManager.h"
#include "gameWorld.h"
#include "../engine/pathlibWorld.h"

RED_DEFINE_STATIC_NAME( OnStartTraversingExploration );
RED_DEFINE_STATIC_NAME( OnFinishTraversingExploration );

ActorActionExploration::ActorActionExploration( CActor* actor, EActorActionType type )
	: ActorActionMoveTo( actor, type )
	, m_status( ActorActionResult_Succeeded )
	, m_markerOldExp( false )
	, m_traverser( nullptr )
	, m_isApproachingExploration( false )
	, m_token( actor )
	, m_steeringGraphTargetNode( )
{
}

ActorActionExploration::~ActorActionExploration()
{
}

Bool ActorActionExploration::StartExploring( CActionAreaComponent* actionArea )
{
	m_markerOldExp = true;

	CMovingAgentComponent* mac = m_actor->GetMovingAgentComponent();
	if ( !mac )
	{
		return false;
	}

	ASSERT( actionArea );
	if ( !actionArea )
	{
		return false;
	}

#if 0
	// verify that the exploration area's not obstructed. For instance
	// someone might have put something on the exploration area ( i.e. the player carrying
	// another person could put him on the exploration area ).
	Vector agentPos = mac->GetAgentPosition();
	Vector explorationAreaPos = actionArea->GetClosestPosition( agentPos );
	Bool isUnobstructed = true;
	CPathAgent* pathAgent = mac->GetPathAgent();
	if ( pathAgent )
	{
		isUnobstructed = pathAgent->TestLocation( explorationAreaPos.AsVector3() );
	}
	
	if ( isUnobstructed == false )
	{
		return false;
	}
#endif

	// check with the scripts if it's ok to traverse an exploration
	Bool eventResult = ( CR_EventSucceeded == m_actor->CallEvent( CNAME( OnStartTraversingExploration ) ) );
	if ( !eventResult )
	{
		return false;
	}

	mac->AttachLocomotionController( *this );
	if ( CanUseLocomotion() )
	{
		m_status = ActorActionResult_InProgress;

#if 0
		actionArea->Traverse( m_actor, mac->GetAgentPosition(), locomotion() );
#endif

		return true;
	}
	else
	{
		return false;
	}
}

Bool ActorActionExploration::MoveAndStartExploring( const SExplorationQueryToken & token, const THandle< IScriptable >& listener, CNode *const steeringGraphTargetNode )
{
	m_markerOldExp				= false;
	m_token						= token;
	m_steeringGraphTargetNode	= steeringGraphTargetNode;
	
	ASSERT( m_token.IsValid() );
	if ( !m_token.IsValid() )
	{
		return false;
	}

	if ( MoveIfRequired() )
	{
		m_status = ActorActionResult_InProgress;
		return true;
	}

	return StartExploring( m_token, listener );
}

Bool ActorActionExploration::CheckIfRequiresToMove(	Vector * moveToLoc, Float * minDist )
{
	if ( m_token.IsValid() )
	{
		Vector moveToLocTemp;
		Float minDistTemp;
		moveToLoc = moveToLoc ? moveToLoc : &moveToLocTemp;
		minDist = minDist ? minDist : &minDistTemp;

		CGameWorld* world = nullptr;
		if ( m_actor && m_actor->IsAttached() )
		{
			world = Cast< CGameWorld > ( m_actor->GetLayer()->GetWorld() );
		}

		if ( world && world->GetExpManager()->GetOracle()->RequiresMoveTo( m_token, *moveToLoc, *minDist ) )
		{
			return true;
		}
	}
	return false;
}

Bool ActorActionExploration::MoveIfRequired()
{
	m_isApproachingExploration = false;
	Vector	moveToLoc;
	Float	minDist;
	if ( CheckIfRequiresToMove( &moveToLoc, &minDist ) )
	{
		CWorld *const world = GGame ? GGame->GetActiveWorld() : nullptr;
		if ( world )
		{
			CPathLibWorld* pathLibWorld = world->GetPathLibWorld();
			const Bool startIsValid		= pathLibWorld->TestLocation( m_actor->GetWorldPositionRef(), PathLib::CT_DEFAULT );
			const Bool targetIsValid	= pathLibWorld->TestLocation( moveToLoc, PathLib::CT_DEFAULT );
			
			if ( startIsValid && targetIsValid )
			{
				m_isApproachingExploration = true;
				StartMove( moveToLoc, 0.0f, MT_Walk, 1.0f, Max( Min( 0.6f, minDist - 0.05f ), minDist * 0.5f ) );
				return true;
			}
		}
	}

	return false;
}

Bool ActorActionExploration::StartExploring( const SExplorationQueryToken & token, const THandle< IScriptable >& listener )
{
	m_markerOldExp = false;
	m_token = token;
	m_explorationListener = listener;

	ASSERT( token.IsValid() );
	if ( !token.IsValid() )
	{
		return false;
	}

	if ( !m_actor->OnExplorationStarted() )
	{
		return false;
	}

	ASSERT( !m_traverser );
	m_traverser = new CScriptedExplorationTraverser;
	ASSERT( m_traverser->GetClass() == ClassID< CScriptedExplorationTraverser >() );

	// auto delete when no longer needed
	m_traverser->EnableReferenceCounting();

	CGameWorld* world = Cast< CGameWorld > ( m_actor->GetLayer()->GetWorld() );
	ASSERT( world );

	if( !m_traverser->Init( token, world->GetExpManager(), m_actor, listener ) )
	{
		m_actor->OnExplorationEnded();

		DeleteTraverser();

		return false;
	}

	THandle< CScriptedExplorationTraverser > traverserH( m_traverser );

	Bool eventResult = ( CR_EventFailed != m_actor->CallEvent( CNAME( OnStartTraversingExploration ), traverserH ) );
	if ( !eventResult )
	{
		m_actor->OnExplorationEnded();

		DeleteTraverser();

		return false;
	}

	m_status = ActorActionResult_InProgress;
	m_isApproachingExploration = false;

	return true;
}

void ActorActionExploration::Stop()
{
	if ( m_isApproachingExploration )
	{
		ActorActionMoveTo::Stop();
		m_status = ActorActionResult_InProgress;
	}
	else
	{
		CMovingAgentComponent* mac = m_actor->GetMovingAgentComponent();
		if ( !mac )
		{
			return;
		}
		m_status = ActorActionResult_Failed;

		if ( m_markerOldExp )
		{
			mac->DetachLocomotionController( *this );
		}

		if ( m_traverser )
		{
			m_traverser->OnActionStoped();
			DeleteTraverser();
		}

		m_actor->OnExplorationEnded();

		m_actor->CallEvent( CNAME( OnFinishTraversingExploration ) );
	}
}

void ActorActionExploration::OnStopDuringMovementUpdate( EActorActionResult result )
{
	// do nothing, don't allow ActorActionMoveTo to cancel this action
}

Bool ActorActionExploration::Update( Float timeDelta )
{
	if ( m_isApproachingExploration )
	{
		if ( ActorActionMoveTo::Update( timeDelta ) )
		{
			// still moving
			return true;
		}
		// we stopped moving, check what should we do now
		if ( CheckIfRequiresToMove() )
		{
			// we still have to move? fail exploration
			m_status = ActorActionResult_Failed;
		}
		else
		{
			// we don't have to move! start exploration
			StartExploring( m_token, m_explorationListener );
			// continue with traverser update
		}
	}

	if ( m_traverser )
	{
		//m_traverser->Update( timeDelta );

		if ( !m_traverser->IsRunning() )
		{

			DeleteTraverser();

			m_status = ActorActionResult_Succeeded;
		}
	}

	CMovingAgentComponent* mac = m_actor->GetMovingAgentComponent();
	if ( !mac || m_status == ActorActionResult_InProgress )
	{
		return true;
	}

	m_actor->ActionEnded( ActorAction_Exploration, m_status );

	m_actor->OnExplorationEnded();

	m_actor->CallEvent( CNAME( OnFinishTraversingExploration ) );

	if ( m_markerOldExp && CanUseLocomotion() )
	{
		mac->DetachLocomotionController( *this );
	}

	return false;
}

void ActorActionExploration::UpdateChannels( const CMovingAgentComponent& agent, SMoveLocomotionGoal& goal, Float timeDelta )
{
	ActorActionMoveTo::UpdateChannels( agent, goal, timeDelta );
	CNode *const steeringGraphTargetNode = m_steeringGraphTargetNode.Get();
	if ( steeringGraphTargetNode )
	{
		goal.SetGoalTargetNode( steeringGraphTargetNode );
	}
}

void ActorActionExploration::OnGCSerialize( IFile& file )
{
	if ( m_traverser )
	{
		file << m_traverser;
	}
}

void ActorActionExploration::GenerateDebugFragments( CRenderFrame* frame )
{
	if ( m_traverser )
	{
		m_traverser->GenerateDebugFragments( frame );
	}
}

void ActorActionExploration::DeleteTraverser()
{
	ASSERT( m_traverser );
	m_traverser->Release();
	m_traverser = NULL;
}

void ActorActionExploration::OnSegmentFinished( EMoveStatus status )
{
	if ( m_isApproachingExploration )
	{
		ActorActionMoveTo::OnSegmentFinished( status );
	}
	else if ( m_markerOldExp )
	{
		if ( status == MS_Completed )
		{
			m_status = ActorActionResult_Succeeded;
		}
		else if ( status == MS_Failed )
		{
			m_status = ActorActionResult_Failed;
		}
	}
	else
	{
		ASSERT(false, TXT("Should be used for either approaching or old exploration"));
	}
}

void ActorActionExploration::OnControllerAttached()
{
	if ( m_isApproachingExploration )
	{
		ActorActionMoveTo::OnControllerAttached();
	}
	else if ( m_markerOldExp )
	{
	}
	else
	{
		ASSERT(false, TXT("Should be used for either approaching or old exploration"));
	}
}

void ActorActionExploration::OnControllerDetached()
{
	if ( m_isApproachingExploration )
	{
		ActorActionMoveTo::OnControllerDetached();
	}
	else if ( m_markerOldExp )
	{
		if ( m_status == ActorActionResult_InProgress )
		{
			m_status = ActorActionResult_Failed;
		}
	}
	else
	{
		ASSERT(false, TXT("Should be used for either approaching or old exploration"));
	}
}

//////////////////////////////////////////////////////////////////////////

Bool CActor::ActionTraverseExploration( const SExplorationQueryToken & token, const THandle< IScriptable >& listener, CNode *const steeringGraphTargetNode )
{
	// Check if current action may be canceled by other actions
	if ( m_action && ! m_action->IsCancelingAllowed() )
	{
		return false;
	}

	// Cancel actions
	ActionCancelAll();

	// Start move to action
	if ( !m_actionExploration.MoveAndStartExploring( token, listener, steeringGraphTargetNode ) )
	{
		return false;
	}

	// Start action
	m_action = &m_actionExploration;
	m_latentActionResult = ActorActionResult_InProgress;
	m_latentActionType   = ActorAction_Exploration;

	OnActionStarted( ActorAction_Exploration );

	return true;
}

//////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

Bool CActor::ActionSlideThrough( CActionAreaComponent* actionArea )
{
	// Check if current action may be canceled by other actions
	if ( m_action && ! m_action->IsCancelingAllowed() )
	{
		return false;
	}

	// Cancel actions
	ActionCancelAll();

	// Start move to action
	if ( !m_actionExploration.StartExploring( actionArea ) )
	{
		return false;
	}

	// Start action
	m_action = &m_actionExploration;
	m_latentActionResult = ActorActionResult_InProgress;
	m_latentActionType   = ActorAction_Exploration;

	OnActionStarted( ActorAction_Exploration );

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////

extern Bool GLatentFunctionStart;

void CActor::funcActionSlideThrough( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CActionAreaComponent >, actionArea, NULL );
	FINISH_PARAMETERS;

	CActionAreaComponent *pActionArea = actionArea.Get();

	// Only in threaded code
	ASSERT( stack.m_thread );
	if ( pActionArea == NULL )
	{
		RETURN_BOOL( false );
	}

	// Start test
	ACTION_START_TEST;

	// Starting !
	if ( GLatentFunctionStart )
	{
		// Start action
		Bool actionResult = ActionSlideThrough( pActionArea );
		if ( !actionResult )
		{
			// Already failed
			RETURN_BOOL( false );
			return;
		}

		// Bump the latent stuff
		m_latentActionIndex = stack.m_thread->GenerateLatentIndex();

		// Yield the thread to pause execution
		stack.m_thread->ForceYield();
		return;
	}

	// Action still in progress
	if ( m_latentActionIndex == stack.m_thread->GetLatentIndex() )
	{
		if ( m_latentActionResult == ActorActionResult_InProgress )
		{
			// Yield the thread to pause execution
			stack.m_thread->ForceYield();
			return;
		}
	}

	// Get the state
	const Bool canceled = ( m_latentActionIndex != stack.m_thread->GetLatentIndex() );
	const Bool succeeded = !canceled && ( m_latentActionResult == ActorActionResult_Succeeded );

	// Reset
	if ( ! canceled )
	{
		m_latentActionResult = ActorActionResult_Failed;
		m_latentActionType = ActorAction_None;

		m_latentActionIndex = 0;
	}

	// Return state
	RETURN_BOOL( succeeded );
}

void CActor::funcActionSlideThroughAsync( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CActionAreaComponent >, actionArea, NULL );
	FINISH_PARAMETERS;

	CActionAreaComponent *pActionArea = actionArea.Get();
	if ( pActionArea == NULL )
	{
		RETURN_BOOL( false );
	}

	// Traverse the exploration
	Bool actionResult = ActionSlideThrough( pActionArea );
	RETURN_BOOL( actionResult );
}

//////////////////////////////////////////////////////////////////////////

void CActor::funcActorActionExploration( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SExplorationQueryToken, explorationToken, SExplorationQueryToken() );
	GET_PARAMETER_OPT( THandle< IScriptable >, listener, NULL );
	GET_PARAMETER( THandle< CNode >, steeringGraphTargetNode, NULL );
	FINISH_PARAMETERS;
	
	SExplorationQueryToken token	= explorationToken;

	if ( !token.IsValid() )
	{
		m_latentActionResult = ActorActionResult_Failed;
		m_latentActionType = ActorAction_None;
		m_latentActionIndex = 0;

		RETURN_BOOL( false );

		return;
	}

	// Only in threaded code
	ASSERT( stack.m_thread );

	// Start test
	ACTION_START_TEST;

	// Starting !
	if ( GLatentFunctionStart )
	{
		// Start action
		Bool actionResult = ActionTraverseExploration( token, listener.Get(), steeringGraphTargetNode.Get() );
		if ( !actionResult )
		{
			// Already failed
			RETURN_BOOL( false );
			return;
		}

		// Bump the latent stuff
		m_latentActionIndex = stack.m_thread->GenerateLatentIndex();

		// Yield the thread to pause execution
		stack.m_thread->ForceYield();
		return;
	}

	// Action still in progress
	if ( m_latentActionIndex == stack.m_thread->GetLatentIndex() )
	{
		if ( m_latentActionResult == ActorActionResult_InProgress )
		{
			// Yield the thread to pause execution
			stack.m_thread->ForceYield();
			return;
		}
	}

	// Get the state
	const Bool canceled = ( m_latentActionIndex != stack.m_thread->GetLatentIndex() );
	const Bool succeeded = !canceled && ( m_latentActionResult == ActorActionResult_Succeeded );

	// Reset
	if ( ! canceled )
	{
		m_latentActionResult = ActorActionResult_Failed;
		m_latentActionType = ActorAction_None;
		m_latentActionIndex = 0;
	}

	// Return state
	RETURN_BOOL( succeeded );
}

