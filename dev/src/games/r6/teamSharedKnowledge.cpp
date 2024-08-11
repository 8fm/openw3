#include "build.h"

#include "teamSharedKnowledge.h"
#include "team.h"
#include "enemyAwareComponent.h"

IMPLEMENT_ENGINE_CLASS( CTeamSharedKnowladge );

const Float CTeamSharedKnowladge::UPDATE_KNOWLEDGE_DELAY = 1;

SR6SharedEnemyInfo::SR6SharedEnemyInfo( SR6EnemyInfo* enemyInfo )
{
	m_enemy			= enemyInfo->m_enemy;
	m_knownPosition = enemyInfo->GetKnownPosition();
}
const TDynArray< SR6SharedEnemyInfo >& CTeamSharedKnowladge::GetEnemiesInfo( )
{	
	return *m_enemiesInfoCurrent;
}


void CTeamSharedKnowladge::MergeKnowladge( CEntity* teamMember, TDynArray< SR6SharedEnemyInfo >& updatedEnemies )
{
	CEnemyAwareComponent* enemyComponent = teamMember->FindComponent< CEnemyAwareComponent >( );
	if( !enemyComponent )
		return;

	TDynArray< SR6EnemyInfo >& enemies = enemyComponent->GetEnemiesInfo();

	for( Uint32 i=0; i<enemies.Size(); ++i )
	{
		Bool found = false;
		for( Uint32 j=0; j<updatedEnemies.Size(); ++j )
		{
			if( updatedEnemies[j].m_enemy.Get() == enemies[i].m_enemy.Get() )
			{
				if( enemies[i].m_isVisible )
				{
					updatedEnemies[j].m_knownPosition = enemies[i].GetKnownPosition();
				}
				found = true;
				break;
			}			
		}

		if( !found )
		{
			Uint32 newIdx = static_cast< Uint32 >( updatedEnemies.Grow( 1 ) );
			new ( &updatedEnemies[newIdx] ) SR6SharedEnemyInfo( &( enemies[i] ) );
		}
	}
}

void CTeamSharedKnowladge::UpdateKnowladge( )
{
	if( m_isUpdating )
		return;

	m_isUpdating = true;

	TDynArray< SR6SharedEnemyInfo >* notCurrent = m_enemiesInfoCurrent != &m_enemiesInfoBuff1 ? &m_enemiesInfoBuff1 : &m_enemiesInfoBuff2;

	notCurrent->Clear();
	const TDynArray< THandle< CTeamMemberComponent > >& members = m_team->GetMembers();

	for( Uint32 i=0; i<members.Size(); ++i )
	{
		if( members[i].Get() != NULL )
		{
			CEntity* memberEnt = members[i].Get()->GetEntity();
			MergeKnowladge( memberEnt, *notCurrent );
		}
	}

	m_enemiesInfoCurrent = notCurrent;
	m_lastUpdateTime = GGame->GetEngineTime();
	m_isUpdating = false;
}

void CTeamSharedKnowladge::SynchronizeMembersKnowladge( )
{
	TDynArray< THandle< CTeamMemberComponent > >& members = m_team->GetMembersNoConst();

	for( Uint32 i=0; i<members.Size(); ++i )
	{
		CTeamMemberComponent* mainMember		= members[ i ].Get();
		if( !mainMember )
			continue;

		CEnemyAwareComponent* mainEnemyAware	= mainMember->GetEntity()->FindComponent< CEnemyAwareComponent >( );
		if( !mainEnemyAware )
			continue;

		TDynArray< SR6EnemyInfo >& mainEnemies = mainEnemyAware->GetEnemiesInfo();

		for( Uint32 j=0; j<mainEnemies.Size(); ++j )
		{
			SR6EnemyInfo& mainEnemyInfo = mainEnemies[j];
			if( !mainEnemyInfo.m_isVisible ) //if I can't see my enemy, I will ask my allies
			{
				Bool enemyFound = false;
				for( Uint32 k=0; k<members.Size() && !enemyFound; ++k )
				{
					CTeamMemberComponent* subMember		= members[ k ].Get();
					if( !subMember )
						continue;

					if( mainMember==subMember )//I will not ask myself
						continue;

					CEnemyAwareComponent* subEnemyAware	= subMember->GetEntity()->FindComponent< CEnemyAwareComponent >( );
					if( !subEnemyAware )
						continue;

					TDynArray< SR6EnemyInfo >& subEnemies = subEnemyAware->GetEnemiesInfo();

					for( Uint32 l=0; l<subEnemies.Size(); ++l )
					{
						SR6EnemyInfo& subEnemyInfo = subEnemies[l];

						if( subEnemyInfo.m_enemy == mainEnemyInfo.m_enemy && subEnemyInfo.m_isVisible  ) //if this ally can see the enemy, I will take data from him
						{
							mainEnemyInfo.m_lastKnownDirection	= subEnemyInfo.m_lastKnownDirection;
							mainEnemyInfo.m_lastKnownPosition	= subEnemyInfo.m_lastKnownPosition;
							mainEnemyInfo.m_movementDirection	= subEnemyInfo.m_movementDirection;
							mainEnemyInfo.m_movementSpeed		= subEnemyInfo.m_movementSpeed;
							mainEnemyInfo.m_predictedPosition	= subEnemyInfo.m_predictedPosition;
							enemyFound							= true;
							break;
						}
					}
				}
			}

		}

				
	}
}

void CTeamSharedKnowladge::Tick( Float timeDelta )
{
	Float now = GGame->GetEngineTime();
	if( now - m_lastUpdateTime > UPDATE_KNOWLEDGE_DELAY )
	{
		UpdateKnowladge( );
		SynchronizeMembersKnowladge( );
	}
}