/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "enemiesCachedData.h"
#include "../../common/game/actorsManager.h"
#include "../../common/game/movingPhysicalAgentComponent.h"
#include "../../common/game/movableRepresentationPhysicalCharacter.h"

void CEnemiesCachedData::Setup( Float range, Float heightTolerance, Uint32 maxEnemies, const CName& tag, Uint32 flags )
{
	m_boundingBox.Min = Vector( -range, -range, -heightTolerance );
	m_boundingBox.Max = Vector( range, range, heightTolerance );

	m_maxEnemies = maxEnemies;
	m_tag = tag;
	m_flags = flags;
}

void CEnemiesCachedData::Update( CActor& player )
{
	UpdateEnemies( player );

	// Visibility test
	CPhysicsWorld* world = nullptr;
	if ( !GGame->GetActiveWorld()->GetPhysicsWorld( world ) )
	{
		return;
	}

	Float height = 1.0f;
	if ( const CMovingPhysicalAgentComponent* mpac = Cast<CMovingPhysicalAgentComponent>( player.GetMovingAgentComponent() ) )
	{
		if ( const CMRPhysicalCharacter* pc = mpac->GetPhysicalCharacter() )
		{
			height = pc->GetCurrentHeight() * 0.9f; // more or less at eye level
		}
	}

	const Vector startPos = player.GetWorldPositionRef() + Vector::EZ * height;

	CNewNpcSensesManager* sensesManager = GCommonGame->GetNpcSensesManager();

	const TDynArray< SEnemyInfo >::const_iterator end = m_enemies.End();
	for( TDynArray< SEnemyInfo >::iterator it = m_enemies.Begin(); it != end; ++it )
	{
		CActor* actor = it->m_actor.Get();
		if( actor )
		{
			VisibilityQueryId& query = it->m_traceQuery;

			if( query.IsValid() )
			{
				CNewNpcSensesManager::EVisibilityQueryState state = sensesManager->GetQueryState( query );

				switch( state )
				{
				case CNewNpcSensesManager::QS_True:
					it->m_isVisible = true;
					query = VisibilityQueryId::INVALID;
					break;
				case CNewNpcSensesManager::QS_False:
					it->m_isVisible = false;
				case CNewNpcSensesManager::QS_NotFound:
					query = VisibilityQueryId::INVALID;
					break;
				case CNewNpcSensesManager::QS_NotReady:
					continue;
				}
			}

			Vector endPos = actor->GetWorldPositionRef();
			Float enemyHeight = 1.0f;
			if ( const CMovingPhysicalAgentComponent* mpac = Cast<CMovingPhysicalAgentComponent>( actor->GetMovingAgentComponent() ) )
			{
				if ( const CMRPhysicalCharacter* pc = mpac->GetPhysicalCharacter() )
				{
					Vector dir	= startPos - endPos;
					Float mag2	= dir.Mag2();
					Float radius	= pc->GetCombatRadius();
					if( mag2 > radius )
					{
						dir.X /= mag2;
						dir.Y /= mag2;
						dir.Z	= 0.0f;
						endPos += dir *radius;
					}
					// Chatacter is just below or above the enemy
					else
					{
						endPos += mpac->GetWorldForward() * radius;
					}
					enemyHeight = pc->GetCurrentHeight() * 0.5f;
				}
			}
			endPos.Z += enemyHeight;

			query = sensesManager->SubmitQuery( actor, startPos, endPos );
		}
	}
}

void CEnemiesCachedData::UpdateEnemies( CActor& player )
{
	CGameplayStorage::SSearchParams searchParams;
	searchParams.m_origin = player.GetWorldPositionRef();
	searchParams.m_flags = m_flags;
	searchParams.m_tag = m_tag;
	searchParams.m_target = &player;
	TDynArray< TPointerWrapper< CActor > > enemiesPtr;
	GCommonGame->GetActorsManager()->GetClosestToEntity( player, enemiesPtr, m_boundingBox, m_maxEnemies, searchParams.GetFilters(), searchParams.GetFiltersCount() );

	m_enemies.Reserve( enemiesPtr.Size() );

	for( Int32 i = m_enemies.SizeInt() - 1; i >= 0; --i )
	{
		CActor* actor = m_enemies[i].m_actor.Get();

		Bool valid = false;
		for( auto it = enemiesPtr.Begin(), end = enemiesPtr.End(); it != end; ++it )
		{
			CActor* newActor = (*it).Get();
			if( newActor == actor )
			{
				enemiesPtr.EraseFast( it );
				valid = true;
				break;
			}
		}

		if( !valid )
		{
			m_enemies.RemoveAtFast( i );
		}
	}

	for( auto it = enemiesPtr.Begin(), end = enemiesPtr.End(); it != end; ++it )
	{
		CActor* actor = (*it).Get();
		if( actor )
		{
			const SEnemyInfo enemy = { THandle< CActor >( actor ), VisibilityQueryId(), false };
			m_enemies.PushBack( enemy );
		}
	}
}

Bool CEnemiesCachedData::IsVisible( const THandle<CActor>& actor ) const
{
	struct Pred : public Red::System::NonCopyable
	{
		const THandle<CActor>& m_actor;

		Pred( const THandle<CActor>& actor ) : m_actor( actor ) {};

		Bool operator()( const SEnemyInfo& info ) const { return info.m_actor == m_actor; }
	};

	TDynArray< SEnemyInfo >::const_iterator iter = FindIf( m_enemies.Begin(), m_enemies.End(), Pred( actor ) );
	if( iter != m_enemies.End() && iter->m_actor.IsValid() ) return iter->m_isVisible;

	return false;
}

void CEnemiesCachedData::GetVisibleEnemies( TDynArray< CActor* >& actors ) const
{
	const TDynArray< SEnemyInfo >::const_iterator end = m_enemies.End();
	for( TDynArray< SEnemyInfo >::const_iterator it = m_enemies.Begin(); it != end; ++it )
	{
		if( it->m_isVisible )
		{
			CActor* actor = it->m_actor.Get();
			if ( actor != nullptr )
			{
				actors.PushBack( actor );
			}
		}
	}
}

void CEnemiesCachedData::GetVisibleEnemies( TDynArray< THandle< CActor > >& actors ) const
{
	const TDynArray< SEnemyInfo >::const_iterator end = m_enemies.End();
	for( TDynArray< SEnemyInfo >::const_iterator it = m_enemies.Begin(); it != end; ++it )
	{
		if( it->m_isVisible && it->m_actor.IsValid() )
		{
			actors.PushBack( it->m_actor );
		}
	}
}

void CEnemiesCachedData::GetAllEnemiesInRange( TDynArray< THandle< CActor > >& actors ) const
{
	const TDynArray< SEnemyInfo >::const_iterator end = m_enemies.End();
	for( TDynArray< SEnemyInfo >::const_iterator it = m_enemies.Begin(); it != end; ++it )
	{
		if( it->m_actor.IsValid() )
		{
			actors.PushBack( it->m_actor );
		}
	}
}
