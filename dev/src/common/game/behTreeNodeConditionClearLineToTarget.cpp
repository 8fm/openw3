#include "build.h"
#include "behTreeNodeConditionClearLineToTarget.h"

#include "actorsManager.h"
#include "behTreeInstance.h"
#include "../engine/pathlibWorld.h"

////////////////////////////////////////////////////////////////////////
// CBehTreeNodeConditionClearLineToTargetDefinition
////////////////////////////////////////////////////////////////////////

IBehTreeNodeDecoratorInstance* CBehTreeNodeConditionClearLineToTargetDefinition::SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	return new Instance( *this, owner, context, parent );
};


////////////////////////////////////////////////////////////////////////
// CBehTreeNodeConditionClearLineToTargetInstance
////////////////////////////////////////////////////////////////////////

Bool CBehTreeNodeConditionClearLineToTargetInstance::ConditionCheck()
{
	CActor* actor = m_owner->GetActor();
	CNode* target = m_combatTarget ? m_owner->GetCombatTarget().Get() : m_owner->GetActionTarget().Get();

	if ( !target )
	{
		return false;
	}

	const Vector& targetPos = target->GetWorldPositionRef();
	const Vector& myPos = actor->GetWorldPositionRef();

	Float radius = m_useAgentRadius ? actor->GetRadius() : m_customRadius;
	CWorld* world = actor->GetLayer()->GetWorld();

	if ( m_navTest )
	{
		CPathLibWorld* pathlib = world->GetPathLibWorld();
		if ( !pathlib )
		{
			return false;
		}
		if ( !pathlib->TestLine( myPos.AsVector3(), targetPos.AsVector3(), radius, PathLib::CT_NO_ENDPOINT_TEST ) )
		{
			return false;
		}
	}

	if ( m_creatureTest )
	{
		CActor* targetActor = Cast< CActor >( target );
		TStaticArray< CActor*, 2 > arr;
		arr.PushBack( actor );
		if ( targetActor )
		{
			arr.PushBack( targetActor );
		}
		if ( !GCommonGame->GetActorsManager()->TestLine( myPos, targetPos, radius, arr.TypedData(), arr.Size(), true ) )
		{
			return false;
		}
	}


	return true;
}