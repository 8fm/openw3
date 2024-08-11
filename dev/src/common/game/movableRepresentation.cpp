#include "build.h"
#include "movableRepresentation.h"
#include "../game/gameplayStorage.h"

#include "commonGame.h"

///////////////////////////////////////////////////////////////////////////////

Vector IMovableRepresentation::CheckCollisions( CMovingAgentComponent& host, const Vector& deltaPosition ) const
{
	if ( !host.IsMotionEnabled() || !host.IsCollidable() )
	{
		return deltaPosition;
	}

	// Slide the agents
	Float thisRadius = host.GetRadius();
	Vector thisForward = host.GetHeading();
	Vector thisSide = Vector::Cross( thisForward, Vector::EZ );
	Vector currPos = GetRepresentationPosition();
	Vector target = currPos + deltaPosition;

	Float deltaPositionVal = deltaPosition.Mag2();

	static TDynArray< CMovingAgentComponent* > agents;
	agents.ClearFast();
	QueryMovingAgents( currPos, MAX_AGENT_RADIUS, agents );

	for ( TDynArray< CMovingAgentComponent* >::const_iterator currAgent = agents.Begin(); currAgent != agents.End(); ++currAgent )
	{
		CMovingAgentComponent* otherAgent = *currAgent;
		if ( !otherAgent || otherAgent == &host || !otherAgent->IsMotionEnabled() || !otherAgent->IsCollidable() )
		{
			continue;
		}

		Vector agentPos = otherAgent->GetAgentPosition();
		Vector distVector = target - agentPos;

		// Skip agents that are above/below us
		if ( MAbs( agentPos.Z - currPos.Z ) > 2.f )
		{
			continue;
		}

		// Skip agents that aren't colliding with us
		Float minDist	= thisRadius + otherAgent->GetRadius();
		Float currDist	= distVector.Mag2();
		if ( currDist >= minDist )
		{
			continue;
		}


		// inform about the collision
		//host.OnDynamicCollision( otherAgent );

		if ( deltaPositionVal > 0 || currDist < minDist * 0.5f )
		{

			// verify that the two agents should collide and not slide past each other
			if ( otherAgent->CanSlideAlong( host ) )
			{
				// find the safe position
				Float	distFromCenterOnHeadingAxis	= Vector::Dot2( distVector, -thisForward );
				Float	distToOutside				= MSqrt( minDist*minDist - distFromCenterOnHeadingAxis*distFromCenterOnHeadingAxis );

				Vector	targetOnHeadingAxis			= agentPos - thisForward * distFromCenterOnHeadingAxis;

				Vector	safePassPos					= currPos + thisForward * Vector::Dot2( thisForward, agentPos - currPos );
				Vector	safePassDir					= safePassPos.DistanceSquaredTo2D( agentPos ) <= 1e-2 ? thisSide : ( safePassPos - agentPos ).Normalized2();

				target								= targetOnHeadingAxis + safePassDir * distToOutside;
			}
			else
			{
				Float  dist2D			= minDist - currDist;

				Vector separationDir	= currDist > 1e-3 ? distVector.Normalized2() : thisForward;
				target					+= separationDir * dist2D;
			}
		}
	}

	Vector newDelta = target - currPos;
	if ( newDelta.Mag2() > deltaPositionVal )
	{
		newDelta = newDelta.Normalized2() * deltaPositionVal;
	}
	return newDelta;
}

///////////////////////////////////////////////////////////////////////////////

void IMovableRepresentation::QueryMovingAgents( const Vector& queriedPos, Float queryRad, TDynArray< CMovingAgentComponent* >& outAgents, Bool queryAliveOnly ) const
{
	PC_SCOPE( ActorsStorage );
	struct CollectMoveAgents : public Red::System::NonCopyable
	{
		enum { SORT_OUTPUT = true };

		CollectMoveAgents( TDynArray< CMovingAgentComponent* >& output )
			: m_output( output ) {}
		RED_INLINE Bool operator()( TPointerWrapper< CGameplayEntity > ptr )
		{
			CActor* ac = Cast< CActor >( ptr.Get() );
			if ( ac )
			{
				if ( !ac->IsAlive() )
				{
					return true;
				}
				CMovingAgentComponent* mac = ac->GetMovingAgentComponent();
				if ( mac )
				{
					m_output.PushBack( mac );
				}
			}
			return true;
		}
		TDynArray< CMovingAgentComponent* >& m_output;
	} functor( outAgents );
	GCommonGame->GetGameplayStorage()->TQuery( queriedPos, functor, Box( Vector( -queryRad, -queryRad, -1.5 ), Vector( queryRad, queryRad, 1.5 ) ), true, NULL, 0 );
}

///////////////////////////////////////////////////////////////////////////////
