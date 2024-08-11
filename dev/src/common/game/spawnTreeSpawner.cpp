#include "build.h"
#include "spawnTreeSpawner.h"
#include "spawnTreeBaseEntry.h"

#include "../core/instanceDataLayoutCompiler.h"

#include "../engine/occlusionSystem.h"

#include "actionPointComponent.h"
#include "actorsManager.h"
#include "communitySystem.h"
#include "encounter.h"
#include "spawnTreeBaseEntry.h"
#include "wayPointCookingContext.h"

IMPLEMENT_RTTI_ENUM( ESpawnTreeSpawnVisibility );
IMPLEMENT_ENGINE_CLASS( CSpawnTreeWaypointSpawner );
IMPLEMENT_ENGINE_CLASS( CSpawnTreeActionPointSpawner );

#ifdef USE_UMBRA
IMPLEMENT_ENGINE_CLASS( SOcclusionSPQuery );
#endif

//////////////////////////////////////////////////////////////////////////
// ISpawnTreeBaseWaypointFilter
//////////////////////////////////////////////////////////////////////////

ISpawnTreeBaseSpawner::ISpawnTreeBaseSpawner()
	: m_spawnpointDelay( 2.f )
{
}

Bool ISpawnTreeBaseSpawner::AcceptWaypoint( const SWayPointCookingData& sp, const CWayPointCookingContext& context ) const
{
	return true;
}

Bool ISpawnTreeBaseSpawner::IsSpawnPointValid( CSpawnTreeInstance& instance, const SEncounterSpawnGroupIterator& sp ) const
{
	return true;
}

void ISpawnTreeBaseSpawner::CollectTags( TagList& tagList ) const
{

}

#ifndef NO_EDITOR
void ISpawnTreeBaseSpawner::CollectWaypoints( CSpawnTreeInstance& instance, CWayPointsCollection::Input& waypointsInput, TDynArray< CWayPointsCollection::SComponentMapping >& outWaypointsList ) const
{
	const CWayPointCookingContext& context = waypointsInput.m_context;
	for ( const CWayPointsCollection::SComponentMapping& wpId : waypointsInput.m_aps )
	{
		const SWayPointCookingData* wp = context.GetWaypoint( wpId );
		ASSERT( wp );

		if ( AcceptWaypoint( *wp, context ) )
		{
			outWaypointsList.PushBack( wpId );
		}
	}
}
#endif

void ISpawnTreeBaseSpawner::OnSpawnTreeDeactivation( CSpawnTreeInstance& instance ) const
{
#ifdef USE_UMBRA
	instance[ i_occlusionQuerries ].ClearFast();
#endif // USE_UMBRA
}

// Instance buffer interface
void ISpawnTreeBaseSpawner::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
#ifdef USE_UMBRA
	compiler << i_occlusionQuerries;
#endif
	compiler << i_spawnPointListIndex;
}
void ISpawnTreeBaseSpawner::OnInitData( CSpawnTreeInstance& instance ) const
{
	instance[ i_spawnPointListIndex ] = -1;
}
void ISpawnTreeBaseSpawner::OnDeinitData( CSpawnTreeInstance& instance ) const
{
}

EFindSpawnResult ISpawnTreeBaseSpawner::FindSpawnPoint( CSpawnTreeInstance& instance, const SCompiledSpawnStrategyInitializer& strategy, SSpawnTreeUpdateSpawnContext& context, Vector3& outPos, Float& outYaw, Uint32& outSP ) const
{
	PC_SCOPE_PIX( BaseSpawner_FindSpawnPoint );

	if ( instance[ i_spawnPointListIndex ] < 0 )
	{
		return FSR_NoneDefined;
	}

	CEncounter* encounter = instance.GetEncounter();
	CEncounterSpawnPoints& spawnPoints = encounter->GetSpawnPoints();
	const CWayPointsCollection& wpCollection = *spawnPoints.GetCollection();

	const EngineTime gameTime = context.m_currentTime;
	Int32 spawnPoint2Find = context.m_spawnJobLimit;
	
#ifdef USE_UMBRA
	TStaticArray<SOcclusionSPQuery,MAX_QUERIES>& querries = instance[ i_occlusionQuerries ];
	for( auto it = querries.Begin(), end = querries.End(); it != end; ++it )
	{
		SOcclusionSPQuery& sp = *it;
		if( sp.m_query->WasProcessed() )
		{
			const Bool occluded = sp.m_query->IsOccluded();
			if( (occluded && m_visibility == STSV_SPAWN_HIDEN) || (!occluded && m_visibility == STSV_SPAWN_ONLY_VISIBLE) )
			{
				const CWayPointsCollection::SWayPointHandle& wp = wpCollection.GetWaypoint( sp.m_spawnPointIndex );
				if ( strategy.m_initializer->CheckSpawnPoint( context, wp.m_position ) != ISpawnTreeSpawnStrategy::VR_Reject )
				{
					outPos = wp.m_position;
					outYaw = wp.m_yaw;
					outSP = sp.m_spawnPointIndex;
					spawnPoints.GetRuntimeWP( sp.m_spawnPointIndex ).m_timeout = gameTime + m_spawnpointDelay;

					querries.ClearFast();

					context.SetTickIn( 0.0f );
					return FSR_FoundOne;
				}
			}
		}
	}
	querries.ClearFast();
#endif


	for (
		SRandomizedEncounterSpawnGroupIterator it( spawnPoints, instance[ i_spawnPointListIndex ] )
		; it
		; ++it )
	{
		SEncounterSpawnPointData& runtimeData = it.RuntimeData();
		if ( ( !context.m_ignoreSpawnerTimeout && runtimeData.m_timeout > gameTime ) || !IsSpawnPointValid( instance, it ) )
		{
			continue;
		}

		const CWayPointsCollection::SWayPointHandle& wp = it.GetBaseData();
		switch( strategy.m_initializer->CheckSpawnPoint( context, wp.m_position ) )
		{
	
		case ISpawnTreeSpawnStrategy::VR_Accept_VisibilityTest:
#ifdef USE_UMBRA
			context.SetTickIn( 0.f );
			// visibility test
			if ( m_visibility != STSV_SPAWN_ALWAYS )
			{
				// camera cone test
				if ( CEncounter::IsPointSeenByPlayer( wp.m_position ) )
				{
					const Vector3 minOffset( 0.4f, 0.4f, 0.f );
					const Vector3 maxOffset( 0.4f, 0.4f, 1.f );
					Box box( wp.m_position - minOffset, wp.m_position + maxOffset );

					COcclusionQueryPtr qPtr = SOcclusionSystem::GetInstance().AddQuery( box );
					if( qPtr )
					{
						TStaticArray<SOcclusionSPQuery,MAX_QUERIES>& querries = instance[ i_occlusionQuerries ];
						RED_FATAL_ASSERT( querries.Size() < MAX_QUERIES, "Number of occlusion querries exceeded the maximum number!" );
						querries.PushBack( Move( SOcclusionSPQuery( it.GetIndex(), Move(qPtr) ) ) );

						// Do not check this point again for some time
						runtimeData.m_timeout = gameTime + 2.f;

						if( querries.Size() == MAX_QUERIES )
						{
							return FSR_FoundNone;
						}
						break;
					}
					else
					{
						return FSR_FoundNone;
					}

				}
				else if ( m_visibility == STSV_SPAWN_ONLY_VISIBLE )
				{
					// point is not visible, so don't spawn
					break;
				}
			}
#endif // USE_UMBRA
			// NOTICE: no break
		case ISpawnTreeSpawnStrategy::VR_Accept_NoVisibilityTest:
			context.SetTickIn( 0.f );
			outPos = wp.m_position;
			outYaw = wp.m_yaw;
			outSP = it.GetIndex();
			runtimeData.m_timeout = gameTime + m_spawnpointDelay;
			return FSR_FoundOne;
			
		default:
			ASSERT( false );
			ASSUME( false );
		case ISpawnTreeSpawnStrategy::VR_Reject:
			break;
		}
	}

	return FSR_FoundNone;
}

EFindSpawnResult ISpawnTreeBaseSpawner::FindClosestSpawnPoint( CSpawnTreeInstance& instance, SSpawnTreeUpdateSpawnContext& context, Vector3& outPosition, Float& outYaw, Uint32& outSP, Float& cloesestDistSq ) const
{
	if ( instance[ i_spawnPointListIndex ] < 0 )
	{
		return FSR_NoneDefined;
	}

	CEncounter* encounter = instance.GetEncounter();
	CEncounterSpawnPoints& spawnPoints = encounter->GetSpawnPoints();
	const CWayPointsCollection& wpCollection = *spawnPoints.GetCollection();
	Vector3 referencePos = context.m_referenceTransform.GetTranslationRef().AsVector3();

	EFindSpawnResult result = FSR_FoundNone;

	for (
		SEncounterSpawnGroupIterator it( spawnPoints, instance[ i_spawnPointListIndex ] )
		; it
		; ++it )
	{
		const CWayPointsCollection::SWayPointHandle& wp = it.GetBaseData();
		Float distSq = (wp.m_position - referencePos).SquareMag();
		if ( distSq < cloesestDistSq )
		{
			cloesestDistSq = distSq;
			outPosition = wp.m_position;
			outYaw = wp.m_yaw;
			outSP = it.GetIndex();
			result = FSR_FoundOne;
		}

	}

	return result;
}

//////////////////////////////////////////////////////////////////////////
// CSpawnTreeWaypointFilter
//////////////////////////////////////////////////////////////////////////
CSpawnTreeWaypointSpawner::CSpawnTreeWaypointSpawner()
	: m_useLocationTest( false )
{}

Bool CSpawnTreeWaypointSpawner::AcceptWaypoint( const SWayPointCookingData& sp, const CWayPointCookingContext& context ) const
{
	return TagList::MatchAll( m_tags, sp.m_tagList );
}


Bool CSpawnTreeWaypointSpawner::IsSpawnPointValid( CSpawnTreeInstance& instance, const SEncounterSpawnGroupIterator& sp ) const
{
	if ( m_useLocationTest )
	{
		if ( !GCommonGame->GetActorsManager()->TestLocation( sp.GetBaseData().m_position, 0.5f ) )
		{
			sp.RuntimeData().m_timeout = GGame->GetEngineTime() + SEncounterSettings::GetSpawnPointInvalidDelay();
			return false;
		}
	}
	return true;
}
#ifndef NO_EDITOR
void CSpawnTreeWaypointSpawner::CollectWaypoints( CSpawnTreeInstance& instance, CWayPointsCollection::Input& waypointsInput, TDynArray< CWayPointsCollection::SComponentMapping >& outWaypointsList  ) const
{
	if ( m_tags.Empty() )
	{
		return;
	}
	Super::CollectWaypoints( instance, waypointsInput, outWaypointsList );
}
#endif // NO_EDITOR

void CSpawnTreeWaypointSpawner::CollectTags( TagList& tagList ) const
{
	tagList.AddTags( m_tags );
}


//////////////////////////////////////////////////////////////////////////
// CSpawnTreeActionPointFilter
//////////////////////////////////////////////////////////////////////////
Bool CSpawnTreeActionPointSpawner::AcceptWaypoint( const SWayPointCookingData& sp, const CWayPointCookingContext& context ) const
{
	if ( !Super::AcceptWaypoint( sp, context ) )
	{
		return false;
	}

	// determine if wp is actionpoint
	if ( sp.m_actionPointDataIndex < 0 )
	{
		return false;
	}

	// get ap specyfic data
	const SActionPointCookingData& ap = context.GetActionPointData( sp.m_actionPointDataIndex );

	const auto& categoriesAP = ap.m_categories;

	// if no categories are specified we're cool
	if ( !categoriesAP.Empty() )
	{
		// TODO: Categories list could be sorted automatically in this case all categories list stuff could be speed up a lot. But its just offline cooking fucktion.
		Bool matchAnyCategory = false;
		for ( auto it = categoriesAP.Begin(), end = categoriesAP.End(); it != end; ++it )
		{
			for ( auto it2 = m_categories.Begin(), end2 = m_categories.End(); it2 != end2; ++it2 )
			{
				if ( (*it) == (*it2) )
				{
					matchAnyCategory = true;
					break;
				}
				if ( matchAnyCategory )
				{
					break;
				}
			}
		}
		if ( !matchAnyCategory )
		{
			return false;
		}
	}
	
	return true;
}

Bool CSpawnTreeActionPointSpawner::IsSpawnPointValid( CSpawnTreeInstance& instance, const SEncounterSpawnGroupIterator& sp ) const
{
	CEncounter* encounter = instance.GetEncounter();
	SEncounterSpawnPointData& runtimeData = sp.RuntimeData();
	CWayPointComponent* waypoint = runtimeData.GetWaypoint( encounter->GetLayer()->GetWorld(), sp.GetWPCollection(), sp.GetBaseData() );
	CActionPointComponent* actionPoint = Cast< CActionPointComponent >( waypoint );

	if ( !actionPoint || !actionPoint->IsFree() )
	{
		runtimeData.m_timeout = GGame->GetEngineTime() + SEncounterSettings::GetSpawnPointInvalidDelay();
		return false;
	}
	return true;
}
