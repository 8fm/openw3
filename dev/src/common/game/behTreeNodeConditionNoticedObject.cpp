#include "build.h"

#include "behTreeInstance.h"
#include "behTreeNodeConditionNoticedObject.h"
#include "newNpcNoticedObject.h"

BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeNodeConditionTargetNoticedDefinition );
BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeNodeConditionCombatTargetNoticedDefinition )

CBehTreeNodeConditionTargetNoticedInstance::CBehTreeNodeConditionTargetNoticedInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
: CBehTreeNodeConditionInstance( def, owner, context, parent )
{}

CBehTreeNodeConditionCombatTargetNoticedInstance::CBehTreeNodeConditionCombatTargetNoticedInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: CBehTreeNodeConditionTargetNoticedInstance( def, owner, context, parent )
{}

Bool CBehTreeNodeConditionTargetNoticedInstance::ConditionCheck()
{
	CNewNPC* npc = m_owner->GetNPC();
	if( !npc )
		return false;
	
	CActor* testedActor = GetActorToTest();
	if( !testedActor )
		return false;

	const TDynArray< NewNPCNoticedObject >& noticedObjects = npc->GetNoticedObjects();
	for( Uint32 i=0; i<noticedObjects.Size(); ++i )
	{
		if( noticedObjects[ i ].m_actorHandle.Get() == testedActor )
		{
			return noticedObjects[ i ].IsVisible();
		}
	}

	return true;
}

CActor* CBehTreeNodeConditionCombatTargetNoticedInstance::GetActorToTest()
{
	return m_owner->GetCombatTarget();
}