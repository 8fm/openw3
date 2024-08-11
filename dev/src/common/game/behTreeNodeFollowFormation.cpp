/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behTreeNodeFollowFormation.h"

#include "movementGoal.h"
#include "movableRepresentationPathAgent.h"
#include "behTreeInstance.h"
#include "formation.h"


BEHTREE_STANDARD_SPAWNNODE_FUNCTION( CBehTreeNodeFollowFormationDefinition )
BEHTREE_STANDARD_SPAWNNODE_FUNCTION( CBehTreeNodeCombatFollowFormationDefinition )

////////////////////////////////////////////////////////////////////////////
// CBehTreeNodeFollowFormationInstance
////////////////////////////////////////////////////////////////////////////

CBehTreeNodeFollowFormationInstance::CBehTreeNodeFollowFormationInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: Super( def, owner, context, parent )
	, CBehTreeSteeringGraphCommonInstance()
	, m_runtimeData( owner )
{
}

Bool CBehTreeNodeFollowFormationInstance::Activate()
{
	CNode* targetNode = m_owner->GetActionTarget().Get();
	if ( !targetNode || !targetNode->IsA< CActor >() )
	{
		DebugNotifyActivationFail();
		return false;
	}

	CAIFormationData* runtimeData = m_runtimeData.Get();
	CActor* actor = m_owner->GetActor();

	if ( !runtimeData->IsSetup() )
	{
		DebugNotifyActivationFail();
		return false;
	}
	if ( !IsSteeringGraphInitialized() )
	{
		CFormation* formation = runtimeData->GetFormation();
		CMoveSteeringBehavior* steeringGraph = formation ? formation->GetSteeringGraph() : NULL;
		if ( !steeringGraph )
		{
			DebugNotifyActivationFail();
			return false;
		}
		InitializeSteeringGraph( steeringGraph, m_owner );
	}

	m_completed = false;

	if ( !actor->ActionCustomSteer( this, MT_Sprint, 4.f, MFA_REPLAN ) )
	{
		DebugNotifyActivationFail();
		return false;
	}

	ActivateSteering( m_owner );

	if ( !Super::Activate() )
	{
		actor->ActionCancelAll();
		DeactivateSteering( m_owner );
		DebugNotifyActivationFail();
		return false;
	}

	m_steeringInput.m_leaderData = runtimeData->LeaderData();
	m_steeringInput.m_memberData = runtimeData->MemberData();
	m_cachupTestDelay = 0.f;
	m_requestCachup = false;

	return true;
}
void CBehTreeNodeFollowFormationInstance::Deactivate()
{
	Super::Deactivate();

	m_steeringInput.m_leaderData = NULL;
	m_steeringInput.m_memberData = NULL;

	DeactivateSteering( m_owner );
	CActor* actor = m_owner->GetActor();
	if ( actor )
	{
		actor->ActionCancelAll();
	}
}
void CBehTreeNodeFollowFormationInstance::Update()
{
	if ( m_completed )
	{
		Complete( BTTO_FAILED );
		return;
	}
	if ( m_requestCachup )
	{
		if ( !m_hasPathComputed )
		{
			CActor* actor = m_owner->GetActor();
			CMovingAgentComponent* mac = actor->GetMovingAgentComponent();
			CPathAgent* pathAgent = mac->GetPathAgent();
			if ( pathAgent->IsOnNavdata() || !mac->IsSnapedToNavigableSpace() )
			{
				// pathfinding
				switch ( pathAgent->PlotPath( m_cachupPoint ) )
				{
				case PathLib::PATHRESULT_FAILED_OUTOFNAVDATA:
				case PathLib::PATHRESULT_FAILED:
					// cannot get to target (pathfinding failed)
					m_requestCachup = false;
					m_cachupTestDelay = m_owner->GetLocalTime() + 1.f;
					break;
				case PathLib::PATHRESULT_SUCCESS:
					m_hasPathComputed = true;
					pathAgent->SetupPathfollowing( 1.5f, CPathAgent::PATHFOLLOW_DEFAULT );
					break;
				case PathLib::PATHRESULT_PENDING:
					break;
				default:
					ASSUME( false );
				}
			}
		}
	}
	else if ( m_hasPathComputed )
	{
		CActor* actor = m_owner->GetActor();
		CMovingAgentComponent* mac = actor->GetMovingAgentComponent();
		CPathAgent* pathAgent = mac->GetPathAgent();
		pathAgent->StopMovement();
		m_hasPathComputed = false;
	}
	
}
void CBehTreeNodeFollowFormationInstance::UpdateChannels( const CMovingAgentComponent& agent, SMoveLocomotionGoal& goal, Float timeDelta )
{
	CAIFormationData* runtimeData = m_runtimeData.Get();
	CFormationLeaderData* leaderData = runtimeData->LeaderData();
	if ( !leaderData )
	{
		goal.SetFulfilled( true );
		m_completed = true;
		return;
	}

	leaderData->Precompute( runtimeData->GetExternalOwner() );
	runtimeData->MemberData()->PrepeareSteering( leaderData );

	Float localTime = m_owner->GetLocalTime();
	if ( m_cachupTestDelay < localTime )
	{
		Vector3 followPoint;
		if ( runtimeData->MemberData()->FallingBehindTest( leaderData, followPoint ) )
		{
			m_cachupTestDelay = localTime + 1.f;
			m_requestCachup = false;
		}
		else
		{
			m_cachupTestDelay = localTime + 0.25f;
			m_requestCachup = true;
			m_cachupPoint = followPoint;
			// update current path
			if ( m_hasPathComputed )
			{
				CActor* actor = m_owner->GetActor();
				CMovingAgentComponent* mac = actor->GetMovingAgentComponent();
				CPathAgent* pathAgent = mac->GetPathAgent();

				// NOTICE: it might fail
				if ( !pathAgent->UpdatePathDestination( m_cachupPoint ) )
				{
					pathAgent->StopMovement();
					m_hasPathComputed = false;
				}
			}
		}
	}

	// path following
	if ( m_hasPathComputed && m_requestCachup )
	{
		CActor* actor = m_owner->GetActor();
		CMovingAgentComponent* mac = actor->GetMovingAgentComponent();
		CPathAgent* pathAgent = mac->GetPathAgent();

		Vector3 followPosition;
		if ( pathAgent->FollowPath( followPosition ) )
		{
			// get destination position
			Vector3 destinationPosition = pathAgent->GetDestination();

			// calculate movement parameters
			Vector2 velocity;
			Float orientation;
			Vector3 position = pathAgent->GetPosition();
			Vector2 diff = followPosition.AsVector2() - position.AsVector2();
			Float followDistance = diff.Mag();

			velocity = diff.Normalized();
			orientation = EulerAngles::YawFromXY( velocity.X, velocity.Y );

			goal.SetOrientationGoal( agent, orientation );
			goal.SetHeadingGoal( agent, velocity );
			goal.SetGoalPosition( followPosition );
			goal.SetDistanceToGoal( followDistance );
			goal.SetDestinationPosition( destinationPosition );
			goal.MatchOrientationWithHeading( true );
			goal.SnapToMinimalSpeed( true );
		}
		else
		{
			// pathfollowing failed
			m_hasPathComputed = false;
			pathAgent->StopMovement();
		}
	}

	SFormationSteeringInput::SetGeneralFormationData( goal, &m_steeringInput );
	// NOTICE: leader is addressed directly from formation steering nodes goal.SetGoalTargetNode( leaderData->GetLeader() );
	goal.SetFulfilled( false );
}
Bool CBehTreeNodeFollowFormationInstance::IsFinished() const
{
	return m_completed;
}


////////////////////////////////////////////////////////////////////////////
// CBehTreeNodeCombatFollowFormationInstance
////////////////////////////////////////////////////////////////////////////

void CBehTreeNodeCombatFollowFormationInstance::UpdateChannels( const CMovingAgentComponent& agent, SMoveLocomotionGoal& goal, Float timeDelta )
{
	CActor* actor = m_owner->GetCombatTarget().Get();
	if ( actor )
	{
		goal.SetGoalTargetNode( actor );
	}
	Super::UpdateChannels( agent, goal, timeDelta );
}