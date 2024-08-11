#include "build.h"

#include "behTreeNodeGreetingReactionScene.h"

#include "../../common/game/behTreeReactionData.h"

BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeNodeGreetingReactionSceneDecoratorDefinition )

Bool CBehTreeNodeGreetingReactionSceneDecoratorInstance::CheckPositioning( CBehTreeReactionEventData* reactionData )
{
	if( !reactionData )
	{
		return false;
	}

	CEntity* invoker = reactionData->GetInvoker();
	if( !invoker )
	{
		return false;
	}

	if( invoker == m_owner->GetActor() )
	{
		return true;
	}
	const Vector invokerForward		= invoker->GetWorldForward();
	const Vector receiverForward	= m_owner->GetActor()->GetWorldForward();

	if( Vector::Dot2( invokerForward, receiverForward ) > 0 )
	{
		return false;
	}

	const Vector& invokerPos		= invoker->GetWorldPositionRef();
	const Vector& receiverPos		= m_owner->GetActor()->GetWorldPositionRef();

	Vector iToR = receiverPos - invokerPos;
	if( Vector::Dot2( invokerForward, iToR ) < 0 )
	{
		return false;
	}
	/*
	float angle = MathUtils::VectorUtils::GetAngleDegBetweenVectors( invokerForward, iToR );
	float distance = iToR.Mag3();

	float yDistance = distance * MCos( angle );
	float xDistance = distance * MSin( angle );

	return xDistance < m_maxXDistance && yDistance < m_maxYDistance;	
	*/
	float distanceSqr = iToR.SquareMag3();
	return m_minDistanceSqr < distanceSqr &&  m_maxDistanceSqr > distanceSqr;
}

Bool CBehTreeNodeGreetingReactionSceneDecoratorInstance::CanBeAssignedToScene( CBehTreeReactionEventData* reactionData )
{
	if( !Super::CanBeAssignedToScene( reactionData ) )
	{
		return false;
	}

	CActor* ownerActor = m_owner->GetActor();
	CActor* invoker = Cast< CActor >( reactionData->GetInvoker() );

	return invoker && ( ownerActor->IsMoving() || invoker->IsMoving() ) && CheckPositioning( reactionData );
}

Bool CBehTreeNodeGreetingReactionSceneDecoratorInstance::ConditionCheck()
{
	if( !Super::ConditionCheck() )
	{
		return false;
	}

	return m_isActive || CheckPositioning( m_reactionData.Get() );	
}