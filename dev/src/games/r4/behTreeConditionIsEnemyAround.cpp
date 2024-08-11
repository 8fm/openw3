#include "build.h"
#include "behTreeConditionIsEnemyAround.h"
#include "../../common/game/behTreeInstance.h"
#include "../../common/game/newNPCNoticedObject.h"
#include "../../common/game/player.h"

////////////////////////////////////////////////////////////////////////
// CBehTreeNodeConditionIsEnemyAroundDefinition
////////////////////////////////////////////////////////////////////////
IBehTreeNodeDecoratorInstance*	CBehTreeNodeConditionIsEnemyAroundDefinition::SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	return new Instance( *this, owner, context, parent );
}

////////////////////////////////////////////////////////////////////////
// CBehTreeConditionIsEnemyAroundInstance
////////////////////////////////////////////////////////////////////////
CBehTreeConditionIsEnemyAroundInstance::CBehTreeConditionIsEnemyAroundInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: CBehTreeNodeConditionInstance( def, owner, context, parent )
	, m_lastUpdateTime( 0.0f )
	, m_isEnnemyAround( false )

	, m_maxEnemyDistance( def.m_maxEnemyDistance )
	, m_updateDelay( def.m_updateDelay )
{

}

Bool CBehTreeConditionIsEnemyAroundInstance::ConditionCheck()
{
	const Float & localTime = GetOwner()->GetLocalTime();
	if ( m_lastUpdateTime + m_updateDelay < localTime )
	{
		m_lastUpdateTime = localTime;
		CNewNPC*const npc											= GetOwner()->GetNPC();
		const TDynArray< NewNPCNoticedObject > &noticedObjectsArray =  npc->GetNoticedObjects();
		const Vector npcPosition									= npc->GetWorldPosition();

		m_isEnnemyAround = false;
		for ( Uint32 i = 0; i < noticedObjectsArray.Size(); ++i )
		{
			const NewNPCNoticedObject & npcNoticed	= noticedObjectsArray[ i ];
			CActor*const actor						= npcNoticed.m_actorHandle.Get();
			if ( actor == NULL )
			{
				continue;
			}
		
			if ( npc->GetAttitude( actor ) == AIA_Hostile )
			{
				const Vector actorPosition				= actor->GetWorldPosition();
		
				if ( ( actorPosition - npcPosition ).SquareMag3() < m_maxEnemyDistance * m_maxEnemyDistance )
				{
					m_isEnnemyAround = true;
				}
			}
		}
	}
	return m_isEnnemyAround;
}
