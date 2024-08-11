/*
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "behTreeNodeAtomicMoveToWanderpoint.h"

#include "behTreeNodeSelectWanderingTarget.h"
#include "behTreeInstance.h"
#include "movableRepresentationPathAgent.h"
#include "wanderPointComponent.h"
#include "../core/mathUtils.h"
#include "../engine/pathlibWorld.h"

BEHTREE_STANDARD_SPAWNNODE_FUNCTION( CBehTreeNodeAtomicMoveToWanderpointDefinition )

Bool CBehTreeNodeAtomicMoveToWanderpointInstance::StartActorMoveTo()
{
	CActor* actor = m_owner->GetActor();
	
	// very important to pass 0.1f as radius because we don't want the actor to stop when he reaches his target !
	if( !actor->ActionMoveTo( m_target, m_heading, m_moveType, m_moveSpeed, 0.01f, MFA_EXIT ) )
	{
		return false;
	}
	return true;
}

Bool CBehTreeNodeAtomicMoveToWanderpointInstance::ComputeTargetAndHeading()
{
	CActor* actor = m_owner->GetActor();
	CMovingAgentComponent* mac = actor->GetMovingAgentComponent();
	if ( !mac )
	{
		return false;
	}
	CPathAgent* pathAgent = mac->GetPathAgent();

	if ( m_forcedPathfinding )
	{
		pathAgent->ForcePathfindingInTrivialCases( false );
		m_forcedPathfinding = false;
	}

	if ( m_isMovingToProxyPoint )
	{
		m_target = m_desiredSpot;
		m_isMovingToProxyPoint = false;
		return true;
	}

	CNode* target = m_owner->GetActionTarget().Get();
	if ( target )
	{
		m_target = target->GetWorldPositionRef();
		m_heading = target->GetWorldYaw();

		CWanderPointComponent* wanderpoint = Cast< CWanderPointComponent >( target );
		if( !wanderpoint )
		{
			CEntity* targetEnt = Cast< CEntity >( target );
			if( targetEnt )
			{
				wanderpoint = targetEnt->FindComponent< CWanderPointComponent >();
			}
		}
		if ( wanderpoint )
		{
			CWanderPointComponent* lastWP = m_lastWanderpoint.Get();
			if ( lastWP )
			{
				Int32 index = -1;
				if ( lastWP->IsConnectedTo( wanderpoint->GetEntity(), index ) )
				{
					if( lastWP->GetConnectionForcePathfinding( index ) )
					{
						pathAgent->ForcePathfindingInTrivialCases( true );
						m_forcedPathfinding = true;
						m_lastWanderpoint = wanderpoint;
						return true;
					}
				}
			}

			if (  wanderpoint->GetWanderpointRadius() > 0.1f )
			{
				CPathLibWorld* pathlib = pathAgent->GetPathLib();
				PathLib::AreaId areaId = pathAgent->GetCachedAreaId();

				Float radius = wanderpoint->GetWanderpointRadius();
				Float actorRadius = actor->GetRadius();

				auto& randomGen = GEngine->GetRandomNumberGenerator();
				Vector desiredSpot;

				Bool rightSideMovement = false;

				// right-side movement implementation
				if ( m_rightSideMovement && lastWP )
				{
					Float lastRadius = lastWP->GetWanderpointRadius() + 1.0f;
					const Vector& prevPos = lastWP->GetWorldPositionRef();

					const Vector& actorPos = actor->GetWorldPositionRef();
					Vector2 prevNodeDiff = actorPos.AsVector2() - prevPos.AsVector2();
					// test if we are close to out last wp
					if ( prevNodeDiff.SquareMag() < lastRadius*lastRadius )
					{
						const Vector& nextPos = wanderpoint->GetWorldPositionRef();
						const Float PI = 3.1415926535897932384626433832795f;
						Vector2 diff = nextPos.AsVector2() - prevPos.AsVector2();
						Float wpDist = diff.Normalize();

						// compute desired location
						desiredSpot = m_target;
						desiredSpot.AsVector2() += 
							MathUtils::GeometryUtils::Rotate2D( diff * Max( 0.1f, radius - actorRadius ), randomGen.Get< Float >( PI, 1.5f * PI ) )
							+ diff * actorRadius;

						// compute if proxy location is required to sharply go to right-side pavement
						Bool useProxy = false;
						if ( wpDist > lastRadius + radius + 2.f && diff.CrossZ( prevNodeDiff ) > 0.f )
						{
							// we are currently on left pavement side, try to compute proxy point
							Float proxyPointDist = lastRadius + 1.f;
							Vector3 proxyPoint = prevPos.AsVector3();
							proxyPoint.AsVector2() += diff * proxyPointDist;
							if ( pathlib->ComputeHeightFrom( areaId, proxyPoint.AsVector2(), actorPos, proxyPoint.Z ) && pathAgent->TestLine( nextPos, proxyPoint ) )
							{
								// proxy point accessible in straight line
								m_isMovingToProxyPoint = true;
								m_proxyPoint = proxyPoint;
							}
						}
						//

						rightSideMovement = true;
					}
				}

				// normal destination selection
				if ( !rightSideMovement )
				{
					Float angle = randomGen.Get< Float >() * 360.f;
					desiredSpot = m_target;
					desiredSpot.AsVector2() += EulerAngles::YawToVector2( angle ) * randomGen.Get< Float >( 0.f, radius );
				}

				Vector3 outPos;
				if ( pathlib->GetClearLineInDirection( areaId, m_target.AsVector3(), desiredSpot.AsVector3(), actorRadius, outPos, PathLib::CT_DEFAULT ) != PathLib::CLEARLINE_INVALID_START_POINT )
				{
					m_desiredSpot = outPos;
				}

				m_desiredSpot = desiredSpot;

				if ( m_isMovingToProxyPoint )
				{
					m_target.AsVector3() = m_proxyPoint;
				}
				else
				{
					m_target.AsVector3() = m_desiredSpot;
				}

				m_lastWanderpoint = wanderpoint;
			}
			
		}

		return true;
	}
	return false;
}

Bool CBehTreeNodeAtomicMoveToWanderpointInstance::OnDestinationReached()
{
	if ( m_isMovingToProxyPoint )
	{
		UpdateTargetAndHeading();
		return true;
	}
	CActor* actor = m_owner->GetActor();
	Int32 retVal = actor->SignalGameplayEventReturnInt( CBehTreeNodeSelectWanderingTargetDecoratorInstance::SelectNextPointRequestName(), false );
	if ( retVal )
	{
		UpdateTargetAndHeading();
		return true;
	}

	return Super::OnDestinationReached();
}
void CBehTreeNodeAtomicMoveToWanderpointInstance::Deactivate()
{
	if ( m_forcedPathfinding )
	{
		CActor* actor = m_owner->GetActor();
		CMovingAgentComponent* mac = actor ? actor->GetMovingAgentComponent() : nullptr;
		CPathAgent* pathAgent = mac ? mac->GetPathAgent() : nullptr;
		if ( pathAgent )
		{
			pathAgent->ForcePathfindingInTrivialCases( false );
		}
		m_forcedPathfinding = false;
	}
	m_isMovingToProxyPoint = false;
	Super::Deactivate();
}

void CBehTreeNodeAtomicMoveToWanderpointInstance::Update()
{
	if ( m_state == E_MOVE )
	{
		CActor* actor = m_owner->GetActor();

		Float distSq = ( actor->GetWorldPositionRef().AsVector2() - m_target.AsVector2() ).SquareMag();
		if ( distSq < m_maxDistance*m_maxDistance )
		{
			if ( !OnDestinationReached() )
			{
				Complete( BTTO_SUCCESS );
			}
			return;
		}
	}

	Super::Update();
}