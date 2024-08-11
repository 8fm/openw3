#include "build.h"
#include "spawnTreeDespawnConfiguration.h"

#include "aiActionParameters.h"
#include "encounter.h"
#include "entityPool.h"


IMPLEMENT_ENGINE_CLASS( SSpawnTreeDespawnConfiguration );
IMPLEMENT_ENGINE_CLASS( SSpawnTreeAIDespawnConfiguration );

////////////////////////////////////////////////////////////////////
// SSpawnTreeDespawnConfiguration
////////////////////////////////////////////////////////////////////
SSpawnTreeDespawnConfiguration::SSpawnTreeDespawnConfiguration()
	: m_canDespawnOnSight( false )
	, m_minDespawnRange( 25.f )
	, m_maxDespawnRange( -1.f )
	, m_forceDespawnRange( 100.f )
	, m_despawnDelayMin( 2.5f )
	, m_despawnDelayMax( 7.5f )
	, m_despawnTime( 8.f )
{}


////////////////////////////////////////////////////////////////////
// SSpawnTreeAIDespawnConfiguration
////////////////////////////////////////////////////////////////////
SSpawnTreeAIDespawnConfiguration::SSpawnTreeAIDespawnConfiguration()
{
	m_canDespawnOnSight = true;
	m_minDespawnRange = 0.f;
	m_maxDespawnRange = 100.f;
	m_despawnTime = 0.f;
}


////////////////////////////////////////////////////////////////////
// CEncounterDespawnAction
////////////////////////////////////////////////////////////////////
CSpawnTreeDespawnAction::CSpawnTreeDespawnAction( const SSpawnTreeDespawnConfiguration& configuration, CActor* actor, SpawnTreeDespawnerId id )
	: m_configuration( configuration )
	, m_id( id )
	, m_aiPriority( 0 )
	, m_initialDelay( true )
	, m_aiActivated( false )
	, m_actor( actor )
	, m_delay( GGame->GetEngineTime() + GEngine->GetRandomNumberGenerator().Get< Float >( configuration.m_despawnDelayMin , configuration.m_despawnDelayMax ) )
	, m_ai( NULL )
{}

CSpawnTreeDespawnAction::CSpawnTreeDespawnAction( const SSpawnTreeDespawnConfiguration& configuration, IAIActionTree* ai, Int8 aiPriority, CActor* actor, SpawnTreeDespawnerId id )
	: m_configuration( configuration )
	, m_id( id )
	, m_aiPriority( aiPriority )
	, m_initialDelay( true )
	, m_aiActivated( false )
	, m_actor( actor )
	, m_delay( GGame->GetEngineTime() + GEngine->GetRandomNumberGenerator().Get< Float >( configuration.m_despawnDelayMin , configuration.m_despawnDelayMax ) )
	, m_ai( ai )
{}

Bool CSpawnTreeDespawnAction::CheckConditions( const Vector& referencePosition, CActor* actor )
{
	Float distanceSq = (actor->GetWorldPositionRef() - referencePosition).SquareMag3();

	// TODO: Insteady of keeping pure copy of m_configuration I could precompute all square distances, but don't want to overoptimize now.
	if ( distanceSq < m_configuration.m_minDespawnRange * m_configuration.m_minDespawnRange )
	{
		return false;
	}

	if ( m_configuration.m_maxDespawnRange > 0.f && distanceSq > m_configuration.m_maxDespawnRange * m_configuration.m_maxDespawnRange )
	{
		return false;
	}

	// Visibility test
	if ( !m_configuration.m_canDespawnOnSight && distanceSq < m_configuration.m_forceDespawnRange * m_configuration.m_forceDespawnRange )
	{
		if ( actor->WasVisibleLastFrame() )
		{
			return false;
		}
	}

	return true;
}

Bool CSpawnTreeDespawnAction::UpdateDespawn( const Vector& referencePosition )
{
	Float engineTime = GGame->GetEngineTime();
	if ( m_initialDelay )
	{
		if ( engineTime < m_delay )
		{
			return false;
		}
		m_initialDelay = false;
		m_delay = engineTime + m_configuration.m_despawnTime;
	}

	CActor* actor = m_actor.Get();
	if( !actor )
	{
		return true;
	}

	if ( !CheckConditions( referencePosition, actor ) )
	{
		m_delay = engineTime + m_configuration.m_despawnTime;
		return false;
	}

	if ( engineTime < m_delay )
	{
		return false;
	}

	// process despawn
	IAIActionTree* ai = m_ai.Get();
	if ( ai )
	{
		if ( !actor->ForceAIBehavior( ai, m_aiPriority ) )
		{
			// behavior wasn't forced properly - gotta retry next update
			return false;
		}
		m_id = SPAWN_TREE_INVALID_DESPAWNER_ID;
		return true;
	}
	else
	{
		m_id = SPAWN_TREE_INVALID_DESPAWNER_ID;
		if( actor->GetLayer() && actor->IsAttached() )
		{
			GCommonGame->GetEntityPool()->AddEntity( actor );
		}
		else
		{
			actor->Destroy();
		}

		//encounter->OnCreatureDespawned( actor, !actor->IsAlive() );
		//if ( actor->IsAttached() )
		//{
		//	actor->GetLayer()->GetWorld()->RegisterEntityDestroy( actor );
		//}
		//else
		//{
		//	actor->Destroy();
		//}
		
		return true;
	}
}