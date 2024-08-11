#include "build.h"
#include "entity.h"
#include "entityTickManager.h"
#include "baseEngine.h"

CEntityTickManager::CEntityTickManager()
	: m_position( Vector::ZEROS )
	, m_positionValid( false )
{
	const SGameplayLODConfig& lodConfig = GGame->GetGameplayConfig().m_LOD;

	m_tickList.SetMaxBudgetedProcessingTime( lodConfig.m_entitiesTickTime );

	m_budgetableDistance = lodConfig.m_entitiesBudgetableTickDistance;
	m_budgetableDistanceSqr = m_budgetableDistance * m_budgetableDistance;

	m_disableDistance = lodConfig.m_entitiesDisableTickDistance;
	m_disableDistanceSqr = m_disableDistance * m_disableDistance;
}

CEntityTickManager::~CEntityTickManager()
{
}

void CEntityTickManager::Add( CEntity* entity )
{
	EntityProxy* proxy = GetProxy( entity );
	if ( proxy )
	{
		return;
	}

	proxy = new EntityProxy( entity, DetermineEntityLOD( entity ) );
	m_proxies.Insert( proxy );
	if ( proxy->GetLOD() != EntityProxy::LOD_DisabledTick )
	{
		m_tickList.Add( proxy, proxy->GetLOD() == EntityProxy::LOD_BudgetedTick );
	}
}

void CEntityTickManager::Remove( CEntity* entity )
{
	if ( EntityProxy* proxy = GetProxy( entity) )
	{
		m_proxies.Erase( proxy );
		if ( proxy->GetLOD() != EntityProxy::LOD_DisabledTick )
		{
			m_tickList.Remove( proxy, proxy->GetLOD() == EntityProxy::LOD_BudgetedTick );
		}
		delete proxy;
	}
}

void CEntityTickManager::SetReferencePosition( const Vector& position )
{
	m_positionValid = true;
	m_position = position;
}

CEntityTickManager::EntityProxy::LOD CEntityTickManager::DetermineEntityLOD( CEntity* entity )
{
	const Vector& entityPosition = entity->GetWorldPositionRef();
	const Float distanceSqr = m_position.DistanceSquaredTo2D( entityPosition );

	if( entity->CheckDynamicFlag( EEntityDynamicFlags::EDF_ForceNoLOD ) )
	{
		return EntityProxy::LOD_TickEveryFrame;
	}
	else if ( distanceSqr < m_disableDistanceSqr || entity->CheckDynamicFlag( EEntityDynamicFlags::EDF_AlwaysTick ) ) 
	{
		if( distanceSqr < m_budgetableDistanceSqr )
		{
			return EntityProxy::LOD_TickEveryFrame;
		}

		return EntityProxy::LOD_BudgetedTick;
	}

	return EntityProxy::LOD_DisabledTick;
}

void CEntityTickManager::Tick( Float deltaTime )
{
	if ( !m_positionValid )
	{
		return;
	}

	// Get start time

	Uint64 timeStart = 0;
	Red::System::Clock::GetInstance().GetTimer().GetTicks( timeStart );

	// Update LOD and accumulate delta time for all entities
	{
		PC_SCOPE_PIX( EntitiesUpdateLOD );

		for ( EntityProxy* proxy : m_proxies )
		{
			proxy->AccumulateDeltaTime( deltaTime );

			const EntityProxy::LOD newLOD = DetermineEntityLOD( proxy->GetEntity() );
			if ( proxy->GetLOD() == newLOD )
			{
				continue;
			}

			switch ( proxy->GetLOD() )
			{
			case EntityProxy::LOD_TickEveryFrame:
				if ( newLOD == EntityProxy::LOD_BudgetedTick )
				{
					m_tickList.ChangeBudgeted( proxy, true );
				}
				else
				{
					m_tickList.Remove( proxy, proxy->GetLOD() == EntityProxy::LOD_BudgetedTick );
				}
				break;
			case EntityProxy::LOD_BudgetedTick:
				if ( newLOD == EntityProxy::LOD_TickEveryFrame )
				{
					m_tickList.ChangeBudgeted( proxy, false );
				}
				else
				{
					m_tickList.Remove( proxy, proxy->GetLOD() == EntityProxy::LOD_BudgetedTick );
				}
				break;
			case EntityProxy::LOD_DisabledTick:
				m_tickList.Add( proxy, newLOD == EntityProxy::LOD_BudgetedTick );
				break;
			}

			proxy->SetLOD( newLOD );
		}
	}

	// Tick entities
	{
		PC_SCOPE_PIX( EntitiesTick );

		struct EntityTicker
		{
			RED_FORCE_INLINE void Process( EntityProxy* proxy )
			{
				CEntity* entity = proxy->GetEntity();
				Float accumulatedTime = proxy->GetAccumulatedDeltaTime();
				proxy->ResetAccumulatedDeltaTime();
				if ( entity->IsAttached() )
				{
					entity->OnTick( accumulatedTime );
				}
			}
		} entityTicker;

		m_tickList.Process( entityTicker );
	}

	// Update stats

	Uint64 timeEnd = 0;
	Red::System::Clock::GetInstance().GetTimer().GetTicks( timeEnd );

	m_stats.m_statsCount += m_tickList.GetNumRecentlyProcessedBudgeted() + m_tickList.GetNumNonBudgeted();
	m_stats.m_statsTime += ( timeEnd - timeStart );
}

#ifdef USE_ANSEL
void CEntityTickManager::TickNPCLODs()
{
	if ( !m_positionValid )
	{
		return;
	}

	// Tick entities
	{
		PC_SCOPE_PIX( EntitiesLODTick );

		struct EntityLODTicker
		{
			RED_FORCE_INLINE void Process( EntityProxy* proxy )
			{
				CEntity* entity = proxy->GetEntity();
				if ( entity->IsAttached() )
				{
					entity->HACK_ANSEL_OnLODTick();
				}
			}
		} entityTicker;

		m_tickList.ProcessAll( entityTicker );
	}
}
#endif // USE_ANSEL