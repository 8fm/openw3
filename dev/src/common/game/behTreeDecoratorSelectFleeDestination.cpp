#include "build.h"

#include "behTreeDecoratorSelectFleeDestination.h"
#include "behTreeNode.h"
#include "commonGame.h"
#include "gameWorld.h"
#include "behTreeInstance.h"
#include "../engine/pathlibWorld.h"

BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeDecoratorSelectFleeDestinationDefinition )

CBehTreeDecoratorSelectFleeDestinationInstance::CBehTreeDecoratorSelectFleeDestinationInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: Super( def, owner, context, parent )
	, m_customMoveData( owner )
	, m_fleeRadius( def.m_fleeRadius.GetVal( context ) )
	, m_minDistanceFromDangerSq( def.m_minDistanceFromDanger.GetVal( context ) )
{	
	m_minDistanceFromDangerSq *= m_minDistanceFromDangerSq;
}

Bool CBehTreeDecoratorSelectFleeDestinationInstance::IsAvailable()
{
	// Danger target chosen?
	CNode* target = m_owner->GetActionTarget().Get();
	if( target != NULL )
	{
		return true;
	}

	DebugNotifyAvailableFail();
	return false;
}

Bool CBehTreeDecoratorSelectFleeDestinationInstance::Activate()
{
	if ( SelectNextPoint() && Super::Activate() )
	{
		return true;		
	}

	DebugNotifyActivationFail();
	return false;
}

void CBehTreeDecoratorSelectFleeDestinationInstance::Deactivate()
{
	Super::Deactivate();	
}

void CBehTreeDecoratorSelectFleeDestinationInstance::OnSubgoalCompleted( eTaskOutcome outcome )
{
	Complete( outcome );
}

Bool CBehTreeDecoratorSelectFleeDestinationInstance::SelectNextPoint()
{
	if( m_customMoveData )
	{
		Int32 attemptLimit = 10;

		// World
		CPathLibWorld* pathLib = GCommonGame->GetActiveWorld()->GetPathLibWorld();

		Vector vec = Vector::ZEROS;
		while( vec == Vector::ZEROS || !pathLib->TestLocation( vec, 0 )  )
		{
			if( attemptLimit <= 0 )
			{
				return false;
			}

			// Random point in generalised, available area
			vec.X = GEngine->GetRandomNumberGenerator().Get< Float >( m_owner->GetActor()->GetWorldPosition().X - m_fleeRadius, m_owner->GetActor()->GetWorldPosition().X + m_fleeRadius );
			vec.Y = GEngine->GetRandomNumberGenerator().Get< Float >( m_owner->GetActor()->GetWorldPosition().Y - m_fleeRadius, m_owner->GetActor()->GetWorldPosition().Y + m_fleeRadius );

			CNode* target = m_owner->GetActionTarget().Get();
			if( target )
			{
				// Distance test to target
				if( target->GetWorldPosition().DistanceSquaredTo( vec ) < m_minDistanceFromDangerSq )
				{
					vec = Vector::ZEROS;
				}
			}
			else
			{
				// Lost target? Bail out
				return false;
			}

			--attemptLimit;
		}

		// Use
		m_customMoveData->SetTarget( vec.X, vec.Y, vec.Z );

		return true;
	}

	return false;
}

