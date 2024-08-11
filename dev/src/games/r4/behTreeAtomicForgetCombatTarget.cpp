#include "build.h"
#include "behTreeAtomicForgetCombatTarget.h"
#include "../../common/game/behTreeInstance.h"


IBehTreeNodeInstance* CBehTreeAtomicForgetCombatTargetDefinition::SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	return new Instance( *this, owner, context, parent );
}

///////////////////////////////////////////////////
// CBehTreeInstanceForgetCombatTargetInstance
void CBehTreeInstanceForgetCombatTargetInstance::Update()
{
	CNewNPC* npc = m_owner->GetNPC();
	if ( npc )
	{
		CActor* combatTarget = m_owner->GetCombatTarget().Get();
		if ( combatTarget )
		{
			npc->ForgetActor( combatTarget );
		}
	}
	
	m_owner->SetCombatTarget( NULL );
	// activate the children nodes :
	Complete( BTTO_SUCCESS );
}

