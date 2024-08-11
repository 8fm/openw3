#include "build.h"
#include "spawnTreeInitializerSpawner.h"

#include "actionPointManager.h"
#include "communitySystem.h"
#include "spawnTreeBaseEntry.h"

IMPLEMENT_ENGINE_CLASS( ISpawnTreeSpawnerInitializer );
IMPLEMENT_ENGINE_CLASS( CSpawnTreeInitializerWaypointSpawner );
IMPLEMENT_ENGINE_CLASS( CSpawnTreeInitializerActionpointSpawner );


const Float CSpawnTreeInitializerActionpointSpawner::MIN_SPAWN_DELAY = 10.f;

////////////////////////////////////////////////////////////////////
// ISpawnTreeSpawnerInitializer
////////////////////////////////////////////////////////////////////
Bool ISpawnTreeSpawnerInitializer::IsConflicting( const ISpawnTreeInitializer* initializer ) const
{
	if ( m_overrideDeepInitializers || initializer->IsOverridingDeepInitializers() )
	{
		if ( initializer->IsA< ISpawnTreeSpawnerInitializer >() )
		{
			return true;
		}
	}
	return false;
}

Bool ISpawnTreeSpawnerInitializer::IsSpawner() const
{
	return true;
}

Bool ISpawnTreeSpawnerInitializer::IsSpawnableOnPartyMembers() const
{
	return false;
}

////////////////////////////////////////////////////////////////////
// CSpawnTreeInitializerWaypointSpawner
////////////////////////////////////////////////////////////////////
void CSpawnTreeInitializerWaypointSpawner::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );
	m_spawner.OnBuildDataLayout( compiler );
}
void CSpawnTreeInitializerWaypointSpawner::OnInitData( CSpawnTreeInstance& instance )
{
	TBaseClass::OnInitData( instance );
	m_spawner.OnInitData( instance );
}
void CSpawnTreeInitializerWaypointSpawner::OnDeinitData( CSpawnTreeInstance& instance )
{
	TBaseClass::OnDeinitData( instance );
	m_spawner.OnDeinitData( instance );
}
EFindSpawnResult CSpawnTreeInitializerWaypointSpawner::FindSpawnPoint( CSpawnTreeInstance& instance, const SCompiledSpawnStrategyInitializer& strategy, SSpawnTreeUpdateSpawnContext& context, Vector3& outPosition, Float& outYaw, Uint32& outSP ) const
{
	return m_spawner.FindSpawnPoint( instance, strategy, context, outPosition, outYaw, outSP );
}
EFindSpawnResult CSpawnTreeInitializerWaypointSpawner::FindClosestSpawnPoint( CSpawnTreeInstance& instance, SSpawnTreeUpdateSpawnContext& context, Vector3& outPosition, Float& outYaw, Uint32& outSP, Float& cloesestDistSq ) const
{
	return m_spawner.FindClosestSpawnPoint( instance, context, outPosition, outYaw, outSP, cloesestDistSq );
}

void CSpawnTreeInitializerWaypointSpawner::CollectSpawnTags( TagList& tagList ) const
{
	m_spawner.CollectTags( tagList );
}
void CSpawnTreeInitializerWaypointSpawner::OnSpawnTreeDeactivation( CSpawnTreeInstance& instance ) const
{
	m_spawner.OnSpawnTreeDeactivation( instance );
}
const ISpawnTreeBaseSpawner* CSpawnTreeInitializerWaypointSpawner::GetSpawner() const
{
	return &m_spawner;
}

String CSpawnTreeInitializerWaypointSpawner::GetBlockCaption() const
{
	const auto& tags = m_spawner.GetTags();
	return String::Printf( TXT("Spawner WP %s"), tags.ToString().AsChar() );
}
String CSpawnTreeInitializerWaypointSpawner::GetEditorFriendlyName() const
{
	static String STR( TXT("Spawner waypoints") );
	return STR;
}

////////////////////////////////////////////////////////////////////
// CSpawnTreeInitializerWaypointSpawner
////////////////////////////////////////////////////////////////////
CSpawnTreeInitializerActionpointSpawner::EOutput CSpawnTreeInitializerActionpointSpawner::Activate( CActor* actor, CSpawnTreeInstance* instance, CBaseCreatureEntry* entry, EActivationReason reason ) const
{
	if ( reason == ISpawnTreeInitializer::EAR_GameIsRestored )
	{
		// when loaded from save, snap us back to action point as we can be saved in position moved a little
		const Float AP_SEARCH_RADIUS = 3.f;

		// on load 
		CCommunitySystem* communitySystem = GCommonGame->GetSystem< CCommunitySystem >();
		CActionPointManager* actionPointManager = communitySystem->GetActionPointManager();
		Vector actorPos = actor->GetWorldPositionRef();
		const TDynArray< CName >& categories = m_spawner.GetCategories();

		// Setting up action point filter
		SActionPointFilter apFilter;
		apFilter.m_onlyFree			= true;
		apFilter.m_actionPointTags	= m_spawner.GetTags();
		apFilter.m_matchAll			= false;
		apFilter.m_sphere			= Sphere( actorPos, AP_SEARCH_RADIUS );
		apFilter.m_askingNPC		= actor;

		// teleport npc straight to closest ap that fits

		// find closest ap that fits - for spawn in work functionality
		Float closestAPDistSq = FLT_MAX;
		CActionPointComponent* closestAP = nullptr;

		CActionPointManager::APFilteredIterator it( *actionPointManager, apFilter );

		while ( it )
		{
			CActionPointComponent* ap = *it;

			Float distSq = (actorPos - ap->GetWorldPositionRef()).SquareMag3();
			if ( distSq < closestAPDistSq )
			{
				Bool hasMatchingCategory = false;
				const auto& apCategories = ap->GetActionCategories();
				// n^2 but actually we rarely have there more then 1-2 values
				for ( auto it = apCategories.Begin(), end = apCategories.End(); it != end; ++it )
				{
					if ( categories.Exist( *it ) )
					{
						hasMatchingCategory = true;
						break;
					}
				}
				
				if ( hasMatchingCategory )
				{
					closestAP = ap;
					closestAPDistSq = distSq;
				}
			}

			++it;
		}

		if ( closestAP )
		{
			Vector pos = closestAP->GetWorldPositionRef();
			Float yaw = closestAP->GetWorldYaw();

			EulerAngles rot( 0.f, 0.f, yaw );
			actor->SetRawPlacement( &pos, &rot, nullptr );
			actor->HACK_UpdateLocalToWorld();
		}
	}

	return OUTPUT_SUCCESS;
}


void CSpawnTreeInitializerActionpointSpawner::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );
	m_spawner.OnBuildDataLayout( compiler );
}
void CSpawnTreeInitializerActionpointSpawner::OnInitData( CSpawnTreeInstance& instance )
{
	TBaseClass::OnInitData( instance );
	m_spawner.OnInitData( instance );
}
void CSpawnTreeInitializerActionpointSpawner::OnDeinitData( CSpawnTreeInstance& instance )
{
	TBaseClass::OnDeinitData( instance );
	m_spawner.OnDeinitData( instance );
}

EFindSpawnResult CSpawnTreeInitializerActionpointSpawner::FindSpawnPoint( CSpawnTreeInstance& instance, const SCompiledSpawnStrategyInitializer& strategy, SSpawnTreeUpdateSpawnContext& context, Vector3& outPosition, Float& outYaw, Uint32& outSP ) const
{
	return m_spawner.FindSpawnPoint( instance, strategy, context, outPosition, outYaw, outSP );
}

EFindSpawnResult CSpawnTreeInitializerActionpointSpawner::FindClosestSpawnPoint( CSpawnTreeInstance& instance, SSpawnTreeUpdateSpawnContext& context, Vector3& outPosition, Float& outYaw, Uint32& outSP, Float& cloesestDistSq ) const
{
	return m_spawner.FindClosestSpawnPoint( instance, context, outPosition, outYaw, outSP, cloesestDistSq );
}

void CSpawnTreeInitializerActionpointSpawner::CollectSpawnTags( TagList& tagList ) const
{
	m_spawner.CollectTags( tagList );
}

void CSpawnTreeInitializerActionpointSpawner::OnSpawnTreeDeactivation( CSpawnTreeInstance& instance ) const
{
	m_spawner.OnSpawnTreeDeactivation( instance );
}

const ISpawnTreeBaseSpawner* CSpawnTreeInitializerActionpointSpawner::GetSpawner() const
{
	return &m_spawner;
}

#ifndef RED_FINAL_BUILD
void CSpawnTreeInitializerActionpointSpawner::OnPostLoad()
{
	TBaseClass::OnPostLoad();
	m_spawner.SetSpawnPointDelay( Max< Float >( MIN_SPAWN_DELAY, m_spawner.GetSpawnPointDelay() ) );
}
#endif
#ifndef NO_RESOURCE_COOKING
void CSpawnTreeInitializerActionpointSpawner::OnCook( class ICookerFramework& cooker )
{
	TBaseClass::OnCook( cooker );
	m_spawner.SetSpawnPointDelay( Max< Float >( MIN_SPAWN_DELAY, m_spawner.GetSpawnPointDelay() ) );
}
#endif
String CSpawnTreeInitializerActionpointSpawner::GetBlockCaption() const
{
	const auto& tags = m_spawner.GetTags();
	return String::Printf( TXT("Spawner AP %s"), tags.ToString().AsChar() );
}
String CSpawnTreeInitializerActionpointSpawner::GetEditorFriendlyName() const
{
	static String STR( TXT("Spawner actionpoints") );
	return STR;
}
