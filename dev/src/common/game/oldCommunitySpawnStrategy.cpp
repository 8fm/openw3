#include "build.h"
#include "oldCommunitySpawnStrategy.h"
#include "agentsWorld.h"
#include "communityAgentStub.h"
#include "communitySystem.h"
#include "communityConstants.h"
#include "../engine/areaComponent.h"

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( COldCommunitySpawnStrategy );

///////////////////////////////////////////////////////////////////////////////

COldCommunitySpawnStrategy::COldCommunitySpawnStrategy()
	: m_spawnRadius( 40 )
	, m_despawnRadius( 60 )
{
}

void COldCommunitySpawnStrategy::OnPostLoad()
{
	// fix invalid data! (already in game!)
	if ( m_spawnRadius > m_despawnRadius )
	{
		m_spawnRadius = m_despawnRadius;
	}
	// reduce community flickering
	Float minDespawnRadius = m_spawnRadius + 8.f;
	if ( m_despawnRadius < minDespawnRadius )
	{
		m_despawnRadius = minDespawnRadius;
	}

	TBaseClass::OnPostLoad();
}

///////////////////////////////////////////////////////////////////////////////

void COldCommunitySpawnStrategy::SetLOD( const CAgentsWorld& world, IAgent& agent ) const
{
	SAgentStub* stub = static_cast< SAgentStub* >( &agent );
	
	if( !stub->GetStoryPhaseCurrentOwner()->IsWorldValid() )
	{
		stub->SetLOD( ALOD_Invisible );
		stub->SetLODWeight( LW_LOWEST );
		return;
	}

	if ( ShouldBeSpawned( stub, world ) )
	{
		stub->SetLOD( ALOD_CloseDistance );
	}
	else if ( ShouldBeDespawned( stub, world ) )
	{
		stub->SetLOD( ALOD_Invisible );
	}

	// non-background agents have the highest weight
	stub->SetLODWeight( LW_HIGHEST );
}

///////////////////////////////////////////////////////////////////////////////

Float COldCommunitySpawnStrategy::CalculateCameraRespectiveWeight( const Vector& camForward, const Vector& camPos, const Vector& stubPos, Float spawnRadius )
{
#ifdef PROFILE_COMMUNITY
	PC_SCOPE_PIX( SetLOD_CalculateCameraRespectiveWeight );
#endif

	Vector dirToStub = stubPos - camPos;
	Float distToStub = dirToStub.Mag2();
	// normalize the distance
	distToStub = ( spawnRadius - distToStub ) / spawnRadius;
	if ( distToStub > 1.0f )
	{
		distToStub = 1.0f;
	}
	else if ( distToStub < 0 )
	{
		distToStub = 0;
	}

	// calculate the angle at which the camera sees the stub
	dirToStub.Normalize2();
	Float angleToCam = dirToStub.Dot2( camForward );
	Float angleFactor = 1.0f;
	if ( angleToCam < 0.5f )
	{
		angleFactor = 0;
	}

	// calculate the weight
	Float weight = ( angleFactor + distToStub ) * 0.5f;
	Float invWeight = 1 - weight;

	static Int32 startLOD = (Int32)LW_HIGH;
	static Int32 lodsRange = (Int32)LW_LOWEST - (Int32)LW_HIGH + 1;
	Float lodWeight = startLOD +  ( invWeight * lodsRange );
	return lodWeight;
}

///////////////////////////////////////////////////////////////////////////////

Bool COldCommunitySpawnStrategy::ShouldBeSpawned( SAgentStub *agentStub, const CAgentsWorld& world ) const
{
#ifdef PROFILE_COMMUNITY
	PC_SCOPE_PIX( ShouldBeSpawned );
#endif

	// Reset spawn position
	agentStub->m_spawnPos.Set3( 0, 0, 0 );

	// No agent, not able to spawn
	if ( !agentStub->m_communityAgent.IsEnabled() )
	{
		return false;
	}

	// This NPC can be only in the stub form
	if ( !agentStub->IsOnlyStub() )
	{
		SET_STUB_DEBUG_INFO( agentStub, m_visibilityState, SAgentStubDebugInfo::VS_IsNpcAlready );
		return false;
	}

	// We cannot create actors for stubs when they are despawning
	if ( agentStub->m_state == CAS_Despawning || agentStub->m_state == CAS_ToDespawn )
	{
		SET_STUB_DEBUG_INFO( agentStub, m_visibilityState, SAgentStubDebugInfo::VS_Despawning );
		return false;
	}

	
    if ( !agentStub->ForceSpawn() && !agentStub->IsAlwaysSpawned() )
    {
		Vector agentWorldPos = agentStub->m_communityAgent.GetPosition();
		const Vector diff = GCommonGame->GetPlayer()->GetWorldPositionRef() - agentWorldPos;
		if ( Abs( diff.X ) > m_spawnRadius || Abs( diff.Y ) > m_spawnRadius )
		{
			return false;
		}

	    const Float distanceToAgent = diff.SquareMag2();
	    const Float visibilitySpawnRadiusSquared = m_spawnRadius * m_spawnRadius;
	    if ( distanceToAgent > visibilitySpawnRadiusSquared )
	    {
		    // Don't spawn NPC if it's too far from player
		    SET_STUB_DEBUG_INFO( agentStub, m_visibilityState, SAgentStubDebugInfo::VS_TooFarFromPlayer );
		    return false;
	    }

	    // Visibility Area check
		CCommunitySystem* cs = GCommonGame->GetSystem< CCommunitySystem >();
	    CAreaComponent* visibilityArea = cs->GetVisibilityArea();
	    if ( visibilityArea != nullptr )
	    {
		    // Check if NPC is within area
		    const Bool isStubInArea = visibilityArea->GetBoundingBox().Contains( agentWorldPos ) && visibilityArea->TestPointOverlap( agentWorldPos );
		    if ( !isStubInArea )
		    {
			    const Float visibilityAreaSpawnRadiusSquared = CCommunityConstants::VISIBILITY_AREA_SPAWN_RADIUS * CCommunityConstants::VISIBILITY_AREA_SPAWN_RADIUS;
			    if ( distanceToAgent > visibilityAreaSpawnRadiusSquared )
			    {
				    SET_STUB_DEBUG_INFO( agentStub, m_visibilityState, SAgentStubDebugInfo::VS_VisibilityArea );
				    return false;
			    }
		    }
	    }

	    // We cannot create actors for stubs working on layers that are not loaded
	    if ( agentStub->IsActionPointLoaded() == false )
	    {
		    SET_STUB_DEBUG_INFO( agentStub, m_visibilityState, SAgentStubDebugInfo::VS_WorkingOnUnloadedLayer );
		    return false;
	    }
    }

	// Update debug info
	SET_STUB_DEBUG_INFO( agentStub, m_visibilityState, SAgentStubDebugInfo::VS_ToSpawn );
	return true;
}

///////////////////////////////////////////////////////////////////////////////

Bool COldCommunitySpawnStrategy::ShouldBeDespawned( const SAgentStub *agentStub, const CAgentsWorld& world ) const
{
#ifdef PROFILE_COMMUNITY
	PC_SCOPE_PIX( ShouldBeDespawned );
#endif

	// We cannot despawn actors that are only stubs
	if ( agentStub->IsOnlyStub() )
	{
		return false;
	}

	// despawn of the deactivated communities
	if ( agentStub->m_state == CAS_Despawning || agentStub->m_state == CAS_ToDespawn )
	{
		return true;
	}

	CNewNPC* npc = agentStub->m_npc.Get();
	// For now we will allow despawning npc that are playing scenes,
	// because we have scene restart support. We need to think however,
	// if we can just allow it always, or should we block it in some cases.
	// For example we should block it for non gameplay scenes actors - at least for now.
	
	// don't despawn agents that are in non gameplay scene
	if ( npc->IsInNonGameplayScene() == true )
	{
		return false;
	}
	

	// Do not despawn NPC that are locked by quests
	if ( npc->IsLockedByQuests() )
	{
		return false;
	}

	// agent Stub should always be spawned, regardless of the distance from player
	if ( agentStub->IsAlwaysSpawned() )
	{
		return false;
	}

	if ( agentStub->IsActionPointLoaded() == false )
	{
		// despawn agents that work on an unloaded layer
		return true;
	}

	{
		Vector agentWorldPos = Vector( agentStub->m_communityAgent.GetPosition() );
		const Vector diff = GCommonGame->GetPlayer()->GetWorldPositionRef() - agentWorldPos;
		const Float distanceToAgent = diff.SquareMag2();

		const Float spawnRadiusSquared = m_spawnRadius * m_spawnRadius;
		const Float despawnRadiusSquared = m_despawnRadius * m_despawnRadius;
		if ( distanceToAgent > despawnRadiusSquared && !agentStub->m_npc.Get()->WasVisibleLastFrame() )
		{
			// Despawn because actor is too far
			return true;
		}
		else
		{
			// Visibility Area check
			CCommunitySystem* cs = GCommonGame->GetSystem< CCommunitySystem >();
			CAreaComponent* visibilityArea = cs->GetVisibilityArea();
			if ( visibilityArea != nullptr )
			{
				// Check if NPC is within area
				Bool isStubInArea = visibilityArea->GetBoundingBox().Contains( agentWorldPos ) && visibilityArea->TestPointOverlap( agentWorldPos );
				if ( !isStubInArea )
				{
					const Float visibilityAreaDespawnRadiusSquared = CCommunityConstants::VISIBILITY_AREA_DESPAWN_RADIUS * CCommunityConstants::VISIBILITY_AREA_DESPAWN_RADIUS;
					if ( distanceToAgent > visibilityAreaDespawnRadiusSquared )
					{
						return true;
					}
				}
			}

			// Do not despawn
			return false;
		}
	}
}

