#include "build.h"
#include "spawnTreeBaseEntry.h"

#include "../core/diskFile.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "../engine/gameTimeManager.h"
#include "../engine/gameSaveManager.h"
#include "../engine/idTagManager.h"

#include "encounter.h"
#include "gameFastForwardSystem.h"
#include "spawnTreeInitializationContext.h"
#include "spawnTreeInitializer.h"
#include "spawnTreeInitializerSpawner.h"
#include "spawnTreeSpawnStrategy.h"
#include "actionPointManager.h"
#include "actionPointComponent.h"
#include "actionPointReservationReleaseHandler.h"

#include "spawnTreeNodeListOperations.inl"

IMPLEMENT_ENGINE_CLASS( SSpawnTreeEntryStealCreatureState )
IMPLEMENT_ENGINE_CLASS( SSpawnTreeEntrySetup )
IMPLEMENT_ENGINE_CLASS( CBaseCreatureEntry )

//////////////////////////////////////////////////////////////
// SSpawnTreeUpdateSpawnContext
//////////////////////////////////////////////////////////////
void SSpawnTreeUpdateSpawnContext::SetTickIn( Float time )
{
	m_runtimeData->m_delayedUpdate = m_currentTime + time;
}

//////////////////////////////////////////////////////////////
// CBaseCreatureEntry::InitializersIterator
//////////////////////////////////////////////////////////////
Bool CBaseCreatureEntry::InitializersIterator::Next( const ISpawnTreeInitializer*& outInitializer, CSpawnTreeInstance*& instanceBuffer )
{
	if ( m_it != m_end )
	{
		outInitializer = *m_it;
		instanceBuffer = &m_instance;

		++m_it;
		return true;
	}

	return false;
}
void CBaseCreatureEntry::InitializersIterator::Reset()
{
	m_it = m_entry.m_initializers.Begin();
	m_end = m_entry.m_initializers.End();
}

//////////////////////////////////////////////////////////////
// CBaseCreatureEntry
//////////////////////////////////////////////////////////////

CBaseCreatureEntry::CBaseCreatureEntry()
	: m_quantityMin( 1 )
	, m_quantityMax( 1 )
	, m_spawnInterval( 0.f )
	, m_waveDelay( 0.f )
	, m_waveCounterHitAtDeathRatio( 1.f )
	, m_randomizeRotation( false )
	, m_group( 1 )
	, m_recalculateDelay( GameTime::DAY )
{}

CBaseCreatureEntry::~CBaseCreatureEntry()
{
}

void CBaseCreatureEntry::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler				<<				i_initializers;
	compiler				<<				i_tickable;
	compiler				<<				i_stealingInProgress;
	compiler				<<				i_spawnTimeout;
	compiler				<<				i_waveTimeout;
	compiler				<<				i_loadPriorityRecalculationTimeout;
	compiler				<<				i_spawnStrategy;
	compiler				<<				i_setup;
	compiler				<<				i_entryUniqueId;
	compiler				<<				i_numCreatureDead;
	compiler				<<				i_numCreaturesToSpawn;
	compiler				<<				i_numCreaturesSpawned;
	compiler				<<				i_listId;
	compiler				<<				i_baseSpawnerValid;
	compiler				<<				i_isUpdated;
	compiler				<<				i_isCreatureDefinitionInitialized;
	compiler				<<				i_waveDelayIsOn;
	compiler				<<				i_noSpawnPointDefined;
	compiler				<<				i_isSpawnJobRunning;
	compiler				<<				i_timeToNextTick;
	compiler				<<				i_recalculateTimeout;

	m_baseSpawner.OnBuildDataLayout( compiler );

	for ( auto it = m_initializers.Begin(), end = m_initializers.End(); it != end; )
	{
		if ( (*it) )
		{
			(*it)->OnBuildDataLayout( compiler );
			++it;
		}
		else
		{
			m_initializers.Erase( it );
		}
	}
}
void CBaseCreatureEntry::OnInitData( CSpawnTreeInstance& instance, CSpawnTreeInitializationContext& context )
{
	TBaseClass::OnInitData( instance, context );

	CEncounter* encounter = instance.GetEncounter();

	instance[ i_spawnTimeout ] = EngineTime::ZERO;
	instance[ i_numCreatureDead ] = 0;
	instance[ i_numCreaturesToSpawn ] = 0;
	instance[ i_numCreaturesSpawned ] = 0;
	instance[ i_listId ] = encounter->GetCreaturePool().GetUniqueListId();
	instance[ i_isUpdated ] = false;
	instance[ i_isCreatureDefinitionInitialized ] = false;
	instance[ i_waveDelayIsOn ] = false;
	instance[ i_noSpawnPointDefined ] = false;
	instance[ i_isSpawnJobRunning ] = false;
	instance[ i_baseSpawnerValid ] = m_baseSpawner.IsValid();
	instance[ i_timeToNextTick ] = 0.0f;
	instance[ i_recalculateTimeout ] = 0;

	m_baseSpawner.OnInitData( instance );

	for ( auto it = m_initializers.Begin(), end = m_initializers.End(); it != end; ++it )
	{
		if ( (*it) )
		{
			(*it)->OnInitData( instance );
		}
	}

	const auto& topLevelInitializers = context.GetTopInitializers();
	auto& compiledInitializersList = instance[ i_initializers ];

	compiledInitializersList.Reserve( m_initializers.Size() + topLevelInitializers.Size() + 1 );

	// process base initializers
	for ( Uint32 i = 0, n = m_initializers.Size(); i != n; ++i )
	{
		if ( ISpawnTreeInitializer* init = m_initializers[ i ].Get() )
		{
			if( init->IsA<ISpawnTreeSpawnStrategy>() )
			{
				instance[ i_spawnStrategy ].m_initializer = static_cast<ISpawnTreeSpawnStrategy*>(init);
				instance[ i_spawnStrategy ].m_instance = &instance;
			}

			if ( init->HasSubInitializer() )
			{
				compiledInitializersList.PushBack( SCompiledInitializer( init->GetSubInitializer(), &instance ) );
			}

			compiledInitializersList.PushBack( SCompiledInitializer( init, &instance ) );
		}
	}
	Uint32 baseInitializers = compiledInitializersList.Size();

	// process top level initializers
	for ( auto itTop = topLevelInitializers.Begin(), endTop = topLevelInitializers.End(); itTop != endTop; ++itTop )
	{
		SCompiledInitializer topInitializer = *itTop;
		Bool isConflicting = false;
		Bool isOverriding = false;
		for ( Int32 i = baseInitializers-1; i >= 0; --i )
		{
			if ( compiledInitializersList[ i ].m_initializer->IsConflicting( topInitializer.m_initializer ) )
			{
				if ( topInitializer.m_initializer->IsOverridingDeepInitializers() )
				{
					compiledInitializersList.RemoveAt( i );
					--baseInitializers;
				}
				else
				{
					isConflicting = true;
					break;
				}
				
			}
		}
		if ( !isConflicting )
		{
			if( topInitializer.m_initializer->IsA<ISpawnTreeSpawnStrategy>() )
			{
				instance[ i_spawnStrategy ].m_initializer = static_cast<ISpawnTreeSpawnStrategy*>(topInitializer.m_initializer);
				instance[ i_spawnStrategy ].m_instance = topInitializer.m_instance;
			}

			compiledInitializersList.PushBack( topInitializer );
		}
	}

	// determine tickable initializers
	auto& compiledTickableList = instance[ i_tickable ];
	for ( auto it = compiledInitializersList.Begin(), end = compiledInitializersList.End(); it != end; ++it )
	{
		if ( it->m_initializer->IsTickable() )
		{
			compiledTickableList.PushBack( *it );
		}

		if ( it->m_initializer->IsOverridingDeepInitializers() && it->m_initializer->IsSpawner() )
		{
			instance[ i_baseSpawnerValid ] = false;
		}
	}
	
	// 'i_setup' initialization
	UpdateSetup( instance );

	RED_FATAL_ASSERT( instance[ i_spawnStrategy ].m_initializer, "Encounter doesn't have a spawn strategy assigned!" );

	instance[ i_entryUniqueId ] = encounter->RegisterEntry( this, &instance );
}


void CBaseCreatureEntry::OnDeinitData( CSpawnTreeInstance& instance )
{
	m_baseSpawner.OnDeinitData( instance );

	for ( auto it = m_initializers.Begin(), end = m_initializers.End(); it != end; ++it )
	{
		(*it)->OnDeinitData( instance );
	}
}

Bool CBaseCreatureEntry::GenerateIdsRecursively()
{
	Bool result = TBaseClass::GenerateIdsRecursively();

	for ( auto it = m_initializers.Begin(), end = m_initializers.End(); it != end; ++it )
	{
		if ( (*it) )
		{
			result |= (*it)->GenerateIdsRecursively();
		}
	}
	return result;
}

void CBaseCreatureEntry::GenerateHashRecursively( Uint64 parentHash, CSpawnTreeInstance* parentBuffer )
{
	TBaseClass::GenerateHashRecursively( parentHash, parentBuffer );

	if ( parentBuffer )
	{
		CEncounter* const encounter = parentBuffer->GetEncounter();
		encounter->SaveEntryHash( this, parentBuffer, m_id ^ parentHash );
	}
}

void CBaseCreatureEntry::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );

	if ( property->GetName() == TXT( "quantityMin" ) )
	{
		if ( m_quantityMax < m_quantityMin )
		{
			m_quantityMax = m_quantityMin;
		}
	}
	else if ( property->GetName() == TXT( "quantityMax" ) )
	{
		if ( m_quantityMin > m_quantityMax )
		{
			m_quantityMin = m_quantityMax;
		}
	}
}

RED_DEFINE_STATIC_NAME( isImportant );
Bool CBaseCreatureEntry::OnPropertyMissing( CName propertyName, const CVariant& readValue )
{
	if ( propertyName == CNAME( isImportant ) )
	{
		Bool important;
		if ( readValue.AsType( important ) )
		{
			if( important )
			{
				m_group = 0;
			}

			return true;
		}
	}

	return TBaseClass::OnPropertyMissing( propertyName, readValue );
}

Bool CBaseCreatureEntry::OnCreatureLoaded( CSpawnTreeInstance& instance, CEncounterCreaturePool::SCreature& creature, CEncounter::SActiveEntry& encounterEntryData )
{
	ASSERT( false, TXT("Not implemented!") );
	return false;
}

void CBaseCreatureEntry::OnCreatureRemoval( CSpawnTreeInstance& instance, CEncounterCreaturePool::SCreature& creature, ESpawnTreeCreatureRemovalReason removalReason, Bool isCreatureDetached )
{
	if ( !isCreatureDetached )
	{
		--(instance[ i_numCreaturesSpawned ]);

		if ( removalReason == SPAWN_TREE_REMOVAL_KILLED && m_waveDelay > 0.f )
		{
			++(instance[ i_numCreatureDead ] );

			if ( !instance[ i_waveDelayIsOn ] )
			{
				Float ratio = Float( instance[ i_numCreatureDead ] ) / Max( 1.f, Float( GetNumCreaturesToSpawn( instance ) ) );
				if ( ratio >= m_waveCounterHitAtDeathRatio )
				{
					instance[ i_waveDelayIsOn ] = true;
					instance[ i_waveTimeout ] = GGame->GetEngineTime() + m_waveDelay * GEngine->GetRandomNumberGenerator().Get< Float >( 0.75f, 1.25f );
				}
			}
		}
	}

	if ( creature.m_stateFlags & CEncounterCreaturePool::SCreature::FLAG_WAS_ACTIVATED_BY_ENTRY )
	{
		CActor* actor = creature.m_actor;
		const auto& initializers = GetRuntimeInitializers( instance );
		for ( auto it = initializers.Begin(), end = initializers.End(); it != end; ++it )
		{
			ISpawnTreeInitializer* initializer = it->m_initializer;
			initializer->OnCreatureRemoval( *it->m_instance, actor, removalReason, this );
		}
	}
}

Int32 CBaseCreatureEntry::AcceptActorByInitializers( CSpawnTreeInstance& instance, CActor* actor, Int32 definitionIndex )
{
	Bool accepted = true;
	const auto& initializers = GetRuntimeInitializers( instance );
	for ( auto it = initializers.Begin(), end = initializers.End(); it != end; ++it )
	{
		ISpawnTreeInitializer* initializer = (*it).m_initializer;
		if ( initializer->Accept( actor  ) == false )
		{
			accepted = false;
		}
	}
	return accepted;
}

ISpawnTreeInitializer::EOutput CBaseCreatureEntry::ActivateInitializers( CSpawnTreeInstance& instance, CActor* actor, ESpawnType spawnType, Int32 definitionCount )
{
	ISpawnTreeInitializer::EOutput returnVal = ISpawnTreeInitializer::OUTPUT_SUCCESS; // success by default
	const auto& initializers = GetRuntimeInitializers( instance );
	// call initializers on actor
	for ( auto it = initializers.Begin(), end = initializers.End(); it != end; ++it )
	{
		ISpawnTreeInitializer* initializer = (*it).m_initializer;
		Bool processInitializer = false;
		switch ( spawnType )
		{
		case EST_NormalSpawn:
			processInitializer = initializer->CallActivateOnSpawn();
			break;
		case EST_PoolSpawn:
			processInitializer = initializer->CallActivateOnPoolSpawn();
			break;
		case EST_GameIsRestored:
			processInitializer = initializer->CallActivateOnRestore();
			break;
		default:
			ASSUME( false );
		}
		if ( processInitializer )
		{
			switch ( initializer->Activate( actor, (*it).m_instance, this, ISpawnTreeInitializer::ActivationReason( spawnType ) ) )
			{
			case ISpawnTreeInitializer::OUTPUT_SUCCESS:
				break;
			default:
			case ISpawnTreeInitializer::OUTPUT_FAILED:
				returnVal = ISpawnTreeInitializer::OUTPUT_FAILED; // failed
				break;
			case ISpawnTreeInitializer::OUTPUT_POSTPONED:
				returnVal = ISpawnTreeInitializer::OUTPUT_POSTPONED; // pending
				break;
			}
		}
	}
	return returnVal;
}

ISpawnTreeInitializer::EOutput CBaseCreatureEntry::ActivateInitializersOnSteal( CSpawnTreeInstance& instance, CActor* actor, SSpawnTreeEntryStealCreatureState& stealState, Bool initial )
{
	ISpawnTreeInitializer::EOutput returnVal = ISpawnTreeInitializer::OUTPUT_SUCCESS; // success by default
	const auto& initializers = GetRuntimeInitializers( instance );

	auto funInitializer =
		[ this, actor ] ( const SCompiledInitializer& i ) -> ISpawnTreeInitializer::EOutput
		{
			ISpawnTreeInitializer* initializer = i.m_initializer;
			if ( initializer->CallActivateWhenStealing() )
			{
				return initializer->Activate( actor, i.m_instance, this, ISpawnTreeInitializer::EAR_Steal );
			}
			return ISpawnTreeInitializer::OUTPUT_SUCCESS;
		};

	if ( initial )
	{
		for ( Uint32 initializerIdx = 0, n = initializers.Size(); initializerIdx != n; ++initializerIdx )
		{
			switch( funInitializer( initializers[ initializerIdx ] ) )
			{
			case ISpawnTreeInitializer::OUTPUT_SUCCESS:
				break;
			default:
			case ISpawnTreeInitializer::OUTPUT_FAILED:
				returnVal = ISpawnTreeInitializer::OUTPUT_FAILED; // failed
				break;
			case ISpawnTreeInitializer::OUTPUT_POSTPONED:
				if ( returnVal == ISpawnTreeInitializer::OUTPUT_SUCCESS )
				{
					returnVal = ISpawnTreeInitializer::OUTPUT_POSTPONED; // pending
				}
				
				stealState.m_postponedInitializers.PushBack( initializerIdx );
				break;
			}
		}
	}
	else
	{
		for ( Int32 postIdx = stealState.m_postponedInitializers.Size() - 1; postIdx >= 0; --postIdx  )
		{
			Uint32 initializerIdx = stealState.m_postponedInitializers[ postIdx ];
			if ( (initializerIdx & 0x8000) != 0 )
			{
				continue;
			}
			switch( funInitializer( initializers[ initializerIdx ] ) )
			{
			case ISpawnTreeInitializer::OUTPUT_SUCCESS:
				stealState.m_postponedInitializers.RemoveAtFast( postIdx );
				break;
			default:
			case ISpawnTreeInitializer::OUTPUT_FAILED:
				returnVal = ISpawnTreeInitializer::OUTPUT_FAILED; // failed
				break;
			case ISpawnTreeInitializer::OUTPUT_POSTPONED:
				if ( returnVal == ISpawnTreeInitializer::OUTPUT_SUCCESS )
				{
					returnVal = ISpawnTreeInitializer::OUTPUT_POSTPONED; // pending
				}
				break;
			}
		}
	}
	return returnVal;
}

void CBaseCreatureEntry::DeactivateInitializers( CSpawnTreeInstance& instance, CActor* actor, Int32 definitionCount )
{
	const auto& initializers = GetRuntimeInitializers( instance );
	for ( auto it = initializers.Begin(), end = initializers.End(); it != end; ++it )
	{
		ISpawnTreeInitializer* initializer = (*it).m_initializer;
		initializer->Deactivate( actor, (*it).m_instance, this );
	}
}

Bool CBaseCreatureEntry::CreateDespawners( CSpawnTreeInstance& instance, CActor* actor, SpawnTreeDespawnerId despawnerId )
{
	Bool ret = false;

	CEncounter* encounter = instance.GetEncounter();
	const auto& initializers = GetRuntimeInitializers( instance );
	for ( auto it = initializers.Begin(), end = initializers.End(); it != end; ++it )
	{
		ISpawnTreeInitializer* initializer = (*it).m_initializer;
		ret = initializer->CreateDespawners( encounter->GetDespawner(), actor, despawnerId ) || ret;
	}

	return ret;
}

EFindSpawnResult CBaseCreatureEntry::FindClosestSpawnPoint( CSpawnTreeInstance& instance, SSpawnTreeUpdateSpawnContext& context, Vector3& outPosition, Float& outYaw, Uint32& outSP )
{
	Float closestDistSq = FLT_MAX;
	const auto& initializers = GetRuntimeInitializers( instance );
	EFindSpawnResult initializersFindSpawnResult = FSR_NoneDefined;
	for ( auto it = initializers.Begin(), end = initializers.End(); it != end; ++it )
	{
		ISpawnTreeInitializer* initializer = (*it).m_initializer;
		if ( initializer )
		{
			const EFindSpawnResult findSpawnResult = initializer->FindClosestSpawnPoint( *(*it).m_instance, context, outPosition, outYaw, outSP, closestDistSq );
			if( findSpawnResult == FSR_FoundOne )
			{
				initializersFindSpawnResult = FSR_FoundOne;
			}
			else if ( initializersFindSpawnResult == FSR_NoneDefined )
			{
				initializersFindSpawnResult = findSpawnResult;
			}
		}
	}

	if ( instance[ i_baseSpawnerValid ] )
	{
		const EFindSpawnResult findSpawnResult = m_baseSpawner.FindClosestSpawnPoint( instance, context, outPosition, outYaw, outSP, closestDistSq );
		if( findSpawnResult == FSR_FoundOne )
		{
			initializersFindSpawnResult = FSR_FoundOne;
		}
		else if ( initializersFindSpawnResult == FSR_NoneDefined )
		{
			initializersFindSpawnResult = findSpawnResult;
		}
	}

	return initializersFindSpawnResult;
}

EFindSpawnResult CBaseCreatureEntry::FindSpawnPoint( CSpawnTreeInstance& instance, SSpawnTreeUpdateSpawnContext& context, Vector3& outPosition, Float& outYaw, Uint32& outSP )
{
	const auto& initializers = GetRuntimeInitializers( instance );
	EFindSpawnResult initializersFindSpawnResult = FSR_NoneDefined;
	for ( auto it = initializers.Begin(), end = initializers.End(); it != end; ++it )
	{
		ISpawnTreeInitializer* initializer = (*it).m_initializer;
		if ( initializer )
		{
			const EFindSpawnResult findSpawnResult = initializer->FindSpawnPoint( *(*it).m_instance, instance[ i_spawnStrategy ], context, outPosition, outYaw, outSP );
			if( findSpawnResult == FSR_FoundOne )
			{
				return FSR_FoundOne;
			}
			if ( findSpawnResult == FSR_FoundNone )
			{
				initializersFindSpawnResult = FSR_FoundNone;
			}
		}
	}

	if ( instance[ i_baseSpawnerValid ] )
	{
		const EFindSpawnResult findSpawnResult = m_baseSpawner.FindSpawnPoint( instance, instance[ i_spawnStrategy ], context, outPosition, outYaw, outSP );
		if ( findSpawnResult != FSR_NoneDefined )
		{
			return findSpawnResult;
		}
	}

	return initializersFindSpawnResult;
}

void CBaseCreatureEntry::OnSpawnPointsCollectionLoaded( CSpawnTreeInstance& instance )
{
	// initialize spawners
	CEncounter* encounter = instance.GetEncounter();

	// WARNING: this code MUST be symethrical with CookSpawnPoints()

	CWayPointsCollection::GroupId nextSpawnerId = 0;
	CWayPointsCollection::GroupId entryIdMask = Uint32( GetUniqueListId( instance ) ) << 16;
	const CWayPointsCollection*	spCollection = encounter->GetSpawnPoints().GetCollection();

	auto funInitializer =
		[ &nextSpawnerId, &spCollection, entryIdMask ] ( CSpawnTreeInstance& instance, const ISpawnTreeBaseSpawner& spawner )
	{
		CWayPointsCollection::GroupId groupId = nextSpawnerId++ | entryIdMask;
		CWayPointsCollection::GroupIndex groupIndex = spCollection->GetWPGroupIndexById( groupId );
		if ( groupIndex >= 0 )
		{
			spawner.SetSpawnPointListIndex( instance, groupIndex );
		}
	};

	const auto& initializers = GetRuntimeInitializers( instance );
	EFindSpawnResult initializersFindSpawnResult = FSR_NoneDefined;
	for ( auto it = initializers.Begin(), end = initializers.End(); it != end; ++it )
	{
		ISpawnTreeInitializer* initializer = (*it).m_initializer;
		if ( !initializer )
		{
			continue;
		}
		if ( initializer->IsSpawner() )
		{
			ISpawnTreeSpawnerInitializer* spawnInitializer = static_cast< ISpawnTreeSpawnerInitializer* >( initializer );
			funInitializer( *(*it).m_instance, *spawnInitializer->GetSpawner() );
		}
		if ( initializer->HasSubInitializer() )
		{
			ISpawnTreeInitializer* subInitializer = initializer->GetSubInitializer();
			if ( subInitializer->IsSpawner() )
			{
				ISpawnTreeSpawnerInitializer* spawnInitializer = static_cast< ISpawnTreeSpawnerInitializer* >( subInitializer );
				funInitializer( *(*it).m_instance, *spawnInitializer->GetSpawner() );
			}
		}
	}

	if ( instance[ i_baseSpawnerValid ] )
	{
		funInitializer( instance, m_baseSpawner );
	}	
}


#ifndef NO_EDITOR
void CBaseCreatureEntry::CookSpawnPoints( CSpawnTreeInstance& instance, CWayPointsCollection::Input& waypointsInput )
{
	// WARNING: this code MUST be symethrical with OnSpawnPointsCollectionLoaded()
	CWayPointsCollection::GroupId nextSpawnerId = 0;
	CWayPointsCollection::GroupId entryIdMask = Uint32( GetUniqueListId( instance ) ) << 16;

	auto funInitializer =
		[ &instance, &waypointsInput, &nextSpawnerId, entryIdMask ] ( const ISpawnTreeBaseSpawner& spawner )
	{
		TDynArray< CWayPointsCollection::SComponentMapping > waypointList;
		spawner.CollectWaypoints( instance, waypointsInput, waypointList );
		if ( waypointList.Empty() )
		{
			nextSpawnerId++;
			return;
		}
		CWayPointsCollection::Input::Group group;
		group.m_groupId = nextSpawnerId++ | entryIdMask;
		group.m_list = Move( waypointList );
		waypointsInput.m_groups.PushBack( group );
	};

	const auto& initializers = GetRuntimeInitializers( instance );
	EFindSpawnResult initializersFindSpawnResult = FSR_NoneDefined;
	for ( auto it = initializers.Begin(), end = initializers.End(); it != end; ++it )
	{
		ISpawnTreeInitializer* initializer = (*it).m_initializer;
		if ( !initializer )
		{
			continue;
		}
		if ( initializer->IsSpawner() )
		{
			ISpawnTreeSpawnerInitializer* spawnInitializer = static_cast< ISpawnTreeSpawnerInitializer* >( initializer );
			funInitializer( *spawnInitializer->GetSpawner() );
		}
		if ( initializer->HasSubInitializer() )
		{
			ISpawnTreeInitializer* subInitializer = initializer->GetSubInitializer();
			if ( subInitializer->IsSpawner() )
			{
				ISpawnTreeSpawnerInitializer* spawnInitializer = static_cast< ISpawnTreeSpawnerInitializer* >( subInitializer );
				funInitializer( *spawnInitializer->GetSpawner() );
			}
		}
	}

	if ( instance[ i_baseSpawnerValid ] )
	{
		funInitializer( m_baseSpawner );
	}
}
#endif			// NO_EDITOR

void CBaseCreatureEntry::Activate( CSpawnTreeInstance& instance )
{
	if ( GGame->GetTimeManager()->GetTime() >= instance[ i_recalculateTimeout ] )
	{
		instance[ i_numCreaturesToSpawn ] = RecalculateNumCreaturesToSpawn( instance );
		instance[ i_recalculateTimeout ] = GGame->GetTimeManager()->GetTime() + m_recalculateDelay;
	}
	instance[ i_active ] = true;
	
	if ( !instance[ i_isUpdated ] )
	{
		instance.GetEncounter()->RegisterForUpdates( this, &instance );
		instance[ i_isUpdated ] = true;
	}
}

void CBaseCreatureEntry::Deactivate( CSpawnTreeInstance& instance )
{
	for ( auto it = m_initializers.Begin(), end = m_initializers.End(); it != end; ++it )
	{
		(*it)->OnSpawnTreeDeactivation( instance );
	}

	m_baseSpawner.OnSpawnTreeDeactivation( instance );

	CEncounter* encounter = instance.GetEncounter();
	CEncounter::SActiveEntry* activeEntry = encounter->FindActiveCreatureEntry( GetUniqueListId( instance ) );
	if ( activeEntry )
	{
		CEncounterCreaturePool& creaturePool = encounter->GetCreaturePool();
		auto& entryList = activeEntry->m_spawnedActors;
		while ( !entryList.Empty() )
		{
			CEncounterCreaturePool::SCreature* entry = entryList.First();
			creaturePool.DetachCreature( *entry );
		}
	}
	encounter->UnregisterForUpdates( this, &instance );

	instance[ i_active ] = false;
	instance[ i_isCreatureDefinitionInitialized ] = false;
	instance[ i_numCreaturesSpawned ] = 0;
	instance[ i_loadPriorityRecalculationTimeout ] = EngineTime::ZERO;
}

Bool CBaseCreatureEntry::UpdateSpawn( CSpawnTreeInstance& instance, SSpawnTreeUpdateSpawnContext& context )
{
	if ( IsActive( instance ) == false )
	{
		if ( instance[ i_numCreaturesSpawned ] == 0 )
		{
			instance[ i_isUpdated ] = false;
			return false;
		}
		return true;
	}
	if ( instance[ i_stealingInProgress ].Empty() )
	{
		context.SetTickIn( GEngine->GetRandomNumberGenerator().Get< Float >( 0.1f, 0.14f ) );
	}
	// check if we have hit spawn limit
	if ( WasSpawnLimitReachedCommon( instance ) )
	{
		context.SetTickIn( SEncounterSettings::GetSpawnLimitReachedDelay() );
		HandleWaveRespawn( instance );
		// do nothing - everyone is spawned and happy or dying in agony
	}
	else
	{
		if ( UpdateCollectCreaturesToSteal( instance, *context.m_runtimeData ) )
		{
			context.SetTickIn( 0.0f );
			return true;
		}

		Bool templateLoaded = instance[ i_isCreatureDefinitionInitialized ];
		if ( !templateLoaded )
		{
			switch ( UpdateLoading( instance, context ) )
			{
			case LOAD_FAILED:
				context.SetTickIn( 10.f );
				break;
			case LOAD_PENDING:
				// Loading not finished
				context.SetTickIn( 0.f );
				break;
			case LOAD_SUCCESS:
				instance[ i_isCreatureDefinitionInitialized ] = true;
				break;
			default:
				ASSUME( false );
			}
		}
		
		if ( templateLoaded )
		{
			if ( context.m_spawnJobLimit > 0 && TestSpawnInterval( instance ) && !instance[ i_isSpawnJobRunning ] )
			{
				UpdateSpawnAnew( instance, context );
			}
		}
	}

	UpdateProcessCreatureStealing( instance, *context.m_runtimeData );


	return true;
}

Bool CBaseCreatureEntry::FastForwardUpdateSpawn( CSpawnTreeInstance& instance, SSpawnTreeUpdateSpawnContext& context, CEncounter::FastForwardUpdateContext& fastForwardContext )
{
	if ( IsActive( instance ) == false )
	{
		if ( instance[ i_numCreaturesSpawned ] == 0 )
		{
			instance[ i_isUpdated ] = false;
			return false;
		}
		return true;
	}

	instance[ i_waveTimeout ] = 0.f;
	HandleWaveRespawn( instance );

	// Check spawn limits
	if ( WasSpawnLimitReachedCommon( instance ) )
	{
		return true;
	}

	// Load templates
	Bool templateLoaded = instance[ i_isCreatureDefinitionInitialized ];
	if ( !templateLoaded )
	{
		switch ( UpdateLoading( instance, context ) )
		{
		case LOAD_FAILED:
			break;
		case LOAD_PENDING:
			// Loading not finished
			fastForwardContext.m_isLoadingTemplates = true;
			break;
		case LOAD_SUCCESS:
			instance[ i_isCreatureDefinitionInitialized ] = true;
			break;
		default:
			ASSUME( false );
		}
	}

	if ( !templateLoaded )
	{
		return true;
	}

	// Don't spawn hostiles around player feature implementation.
	// NOTICE: Hacky but important, its better to have this feature 'concentrated' in one point.
	Bool ignoredMinDistance = false;
	if ( fastForwardContext.m_globalContext->m_parameters.m_dontSpawnHostilesClose && context.m_ignoreMinDistance )
	{
		CEncounter* encounter = instance.GetEncounter();
		CAttitudeManager* atMan = GCommonGame->GetSystem< CAttitudeManager >();

		Bool areSpawnedCreaturesHostile = false;
		Int32 creatureDefCount = GetCreatureDefinitionsCount();
		for ( Int32 i = 0; i < creatureDefCount; ++i )
		{
			CEncounterCreatureDefinition* creatureDef = encounter->GetCreatureDefinition( GetCreatureDefinitionId( instance, i ) );
			ASSERT( creatureDef, TXT("Impossible to not have a creature definition if we actually did pass UpdateLoading phase!") );
			CEntityTemplate* entityTemplate = creatureDef->GetEntityTemplate().Get();
			CActor* actor = Cast< CActor >( entityTemplate->GetEntityObject() );
			if ( actor )
			{
				CName baseAttitudeGroup = actor->GetBaseAttitudeGroup();
				if ( !baseAttitudeGroup.Empty() )
				{
					EAIAttitude attitude;
					Bool isCustom;
					atMan->GetAttitude( baseAttitudeGroup, CNAME( player ), attitude, isCustom );
					if ( attitude == AIA_Hostile )
					{
						areSpawnedCreaturesHostile = true;
						break;
					}
				}
			}
		}
		if ( areSpawnedCreaturesHostile )
		{
			context.m_ignoreMinDistance = false;
			ignoredMinDistance = true;
		}
	}

	// Do spawn
	if ( context.m_spawnJobLimit > 0 && !instance[ i_isSpawnJobRunning ] )
	{
		UpdateSpawnAnew( instance, context );
	}

	if ( ignoredMinDistance )
	{
		context.m_ignoreMinDistance = true;
	}
	

	return true;
}

void CBaseCreatureEntry::OnFullRespawn( CSpawnTreeInstance& instance ) const
{
	auto& compiledInitializersList = instance[ i_initializers ];

	for ( auto it = compiledInitializersList.Begin(), end = compiledInitializersList.End(); it != end; ++it )
	{
		(*it).m_initializer->OnFullRespawn( *(*it).m_instance );
	}

	instance[ i_numCreatureDead ] = 0;
	instance[ i_waveDelayIsOn ] = false;

	UpdateSetup( instance );
}

void CBaseCreatureEntry::StealCreature( CSpawnTreeInstance& instance, CEncounterCreaturePool::SCreature& creature )
{
	CEncounter* encounter = instance.GetEncounter();
	
	encounter->RemoveScheduledDespawn( creature );

	// 'steal' guy
	creature.m_lastOwningEntry				= GetUniqueListId( instance );
	CEncounter::SActiveEntry* activeEntry	= encounter->FindActiveCreatureEntry( creature.m_lastOwningEntry );
	CEncounterCreaturePool& creaturePool	= encounter->GetCreaturePool();
	Bool wasEmpty = activeEntry->m_spawnedActors.Empty();
	creaturePool.AttachCreatureToList( creature, activeEntry->m_spawnedActors );

	// HACK!!! - stealing merchants
	// we need to assign new IdTag (based on current branch hash) to a stolen W3Merchant
	// (there can be only one actor for this branch, so we check if the list was empty before the insertion)
	if ( wasEmpty )
	{
		CActor* actor = creature.m_actor;
		if ( actor != nullptr && actor->GetClass()->GetName() == CNAME( W3MerchantNPC ) )
		{
			actor->SetIdTag( GGame->GetIdTagManager()->CreateFromUint64( activeEntry->m_branchHash ) );
		}
	}
}


Bool CBaseCreatureEntry::UpdateProcessCreatureStealing( CSpawnTreeInstance& instance, CEncounter::SActiveEntry& entry )
{
	StealingInProgress& stealingList = instance[ i_stealingInProgress ];
	if ( stealingList.Empty() )
	{
		return false;
	}

	CEncounter* encounter = instance.GetEncounter();
	CEncounterCreaturePool& creaturePool = encounter->GetCreaturePool();
	EngineTime currTime = GGame->GetEngineTime();

	for ( Int32 listIdx = stealingList.Size()-1; listIdx >=0; --listIdx )
	{
		auto& actorData = stealingList[ listIdx ];
		CActor* actor = actorData.m_actor.Get();
		if ( !actor )
		{
			stealingList.RemoveAtFast( listIdx );
			continue;
		}
		
		CEncounterCreaturePool::SCreature* creature = creaturePool.GetCreatureEntry( actor );
		if ( !creature )
		{
			// actor was removed from the encounter (eg. killed)
			stealingList.RemoveAtFast( listIdx );
			continue;
		}

		Bool initialActivation = false;

		if ( actorData.m_isDelayed )
		{
			if ( actorData.m_delayTimeout > currTime )
			{
				continue;
			}

			// run deinitiailizers
			actorData.m_isDelayed = false;
			if ( creature->m_stateFlags & CEncounterCreaturePool::SCreature::FLAG_WAS_ACTIVATED_BY_ENTRY )
			{
				CEncounter::SActiveEntry* previousEntry = encounter->FindActiveCreatureEntry( actorData.m_previousOwnerEntry );
				if ( previousEntry )
				{
					previousEntry->m_entry->DeactivateInitializers( *previousEntry->m_instanceBuffer, actor );
				}
			}

			initialActivation = true;
		}


		// run initializers
		switch( ActivateInitializersOnSteal( instance, actor, actorData, initialActivation ) )
		{
		case ISpawnTreeInitializer::OUTPUT_SUCCESS:
			stealingList.RemoveAtFast( listIdx );
			creature->m_stateFlags |= CEncounterCreaturePool::SCreature::FLAG_WAS_ACTIVATED_BY_ENTRY;
			break;
		case ISpawnTreeInitializer::OUTPUT_FAILED:
			{
				stealingList.RemoveAtFast( listIdx );
				// detach actor
				creaturePool.DetachCreature( *creature );
			}

			break;
		case ISpawnTreeInitializer::OUTPUT_POSTPONED:
			break;
		default:
			ASSERT( false );
			ASSUME( false );
		}
	}
	return false;
}

void CBaseCreatureEntry::ReserveAPForSpawn( CSpawnTreeInstance& instance, EntitySpawnInfo& data, Uint32 sp )
{
	CEncounter* encounter = instance.GetEncounter();
	CEncounterSpawnPoints& spawnPoints = encounter->GetSpawnPoints();
	const CWayPointsCollection*	spCollection = spawnPoints.GetCollection();

	CWayPointComponent* wp = encounter->GetSpawnPoints().GetRuntimeWP( sp ).GetWaypoint( GGame->GetActiveWorld(), *spCollection, spCollection->GetWaypoint( sp ) );
	if ( !wp )
	{
		return;
	}

	CActionPointComponent* actionPoint = Cast< CActionPointComponent >( wp );
	if ( !actionPoint )
	{
		return;
	}

	actionPoint->SetReserved( CActionPointManager::REASON_SPAWNING );
	data.AddHandler( new CActionPointReservationReleaseHandler( actionPoint ) );
}
void CBaseCreatureEntry::UpdateSpawnAnew( CSpawnTreeInstance& instance, SSpawnTreeUpdateSpawnContext& context )
{
	// spawn new guyz
	CEncounter *const encounter = instance.GetEncounter();
	// check if we have hit creature definition spawn limit
	Int32 spawnGroupToUse;
	if ( WasCreatureLimitReached( instance, spawnGroupToUse ) == false && spawnGroupToUse >= 0 )
	{
		// find spawn point
		Vector3					spawnPos;
		Float					spawnYaw;
		Uint32					sp = 0xffffffff;
		EFindSpawnResult eFSR		= FindSpawnPoint( instance, context, spawnPos, spawnYaw, sp );
		instance[ i_noSpawnPointDefined ] = false;
		switch ( eFSR )
		{
		case FSR_FoundOne:
			{
				// do spawn
				IJobEntitySpawn* job = CreateSpawnJob( spawnPos, spawnYaw, sp, instance, spawnGroupToUse );
				if ( job )
				{
					instance[ i_isSpawnJobRunning ] = true;
					PreCreatureSpawn( instance );
					encounter->SpawnAsync( job, this, instance );
					--context.m_spawnJobLimit;
				}
			}
			break;
		case FSR_NoneDefined:
			{
				// Marking error for debug purposes
				instance[ i_noSpawnPointDefined ] = true;
			}
			break;
		default : // FSR_FoundNone
			// do nothing wait for spawn point to become available
			break;
		}
	}
}
void CBaseCreatureEntry::CollectSpawnTags( TagList& tagList )
{
	m_baseSpawner.CollectTags( tagList );
	for ( auto it = m_initializers.Begin(), end = m_initializers.End(); it != end; ++it )
	{
		ISpawnTreeInitializer* initializer = *it;
		initializer->CollectSpawnTags( tagList );
		if ( initializer->HasSubInitializer() )
		{
			initializer->GetSubInitializer()->CollectSpawnTags( tagList );
		}
	}
	//m_spawnConfiguration.Coll
	//tagList.AddTags(  );
}

void CBaseCreatureEntry::OnEvent( CSpawnTreeInstance& instance, CName eventName ) const
{
	auto& compiledInitializersList = instance[ i_initializers ];

	for ( auto it = compiledInitializersList.Begin(), end = compiledInitializersList.End(); it != end; ++it )
	{
		it->m_initializer->OnEvent( this, *it->m_instance, eventName, &instance );
	}
}

void CBaseCreatureEntry::UpdateEntriesSetup( CSpawnTreeInstance& instance ) const
{
	UpdateSetup( instance );
}

Bool CBaseCreatureEntry::OnSpawnJobIsDone( CSpawnTreeInstance& instance, IJobEntitySpawn* spawnJob, CEncounter::SActiveEntry& encounterEntryData )
{
	instance[ i_isSpawnJobRunning ] = false;
	return OnSpawnJobIsDoneInternal( instance, spawnJob, encounterEntryData );
}

void CBaseCreatureEntry::UpdateSetup( CSpawnTreeInstance& instance ) const
{
	Float prevSpawnRatio = instance[ i_setup ].m_spawnRatio;

	SSpawnTreeEntrySetup setup;

	auto& compiledInitializersList = instance[ i_initializers ];

	for ( auto it = compiledInitializersList.Begin(), end = compiledInitializersList.End(); it != end; ++it )
	{
		it->m_initializer->UpdateEntrySetup( this, *it->m_instance, setup );
	}

	instance[ i_setup ] = setup;

	if ( prevSpawnRatio != setup.m_spawnRatio )
	{
		ForceRecalculateNumCreaturesToSpawn( instance );
	}
}

void CBaseCreatureEntry::TickInitializers( CSpawnTreeInstance& instance, CEncounterCreaturePool::SCreatureList& creatures, SSpawnTreeUpdateSpawnContext& context )
{
	const auto& initializers = instance[ i_tickable ];
	for ( auto it = initializers.Begin(), end = initializers.End(); it != end; ++it )
	{
		(*it).m_initializer->Tick( creatures, *(*it).m_instance, context );
	}
}

Bool CBaseCreatureEntry::CheckEntityTemplate( CEntityTemplate* entityTemplate )
{
	if ( !entityTemplate )
	{
		HALT( "ENCOUNTER ENTRY HAS EMPTY TEMPLATE!");
		return false;
	}
	const CObject* templateInstance = entityTemplate->GetTemplateInstance();
	if ( templateInstance && !templateInstance->IsA< CActor >() )
	{
		HALT( "ENCOUNTER ENTRY HAS TEMPLATE '%ls' THAT IS NOT AN 'CActor'!", entityTemplate->GetFile()->GetDepotPath().AsChar() );
		return false;
	}
	return true;
}

void CBaseCreatureEntry::PreCreatureSpawn( CSpawnTreeInstance& instance )
{
	if ( m_spawnInterval )
	{
		instance[ i_spawnTimeout ] = GGame->GetEngineTime() + GEngine->GetRandomNumberGenerator().Get< Float >( 0.5f, 1.5f ) * m_spawnInterval;
	}
}

CEntityTemplate* CBaseCreatureEntry::CheckIsCreatureDefinitionReady( CEncounterCreatureDefinition* creatureDefinition, CSpawnTreeInstance& instance )
{
	switch( creatureDefinition->m_entityTemplate.GetAsync() )
	{
	default:
	case BaseSoftHandle::ALR_Failed:
		// TODO: mark as fucked up
		//instance[ i_creatureDefinition ] = -1;
		// no break
	case BaseSoftHandle::ALR_InProgress:
		return NULL;
	case BaseSoftHandle::ALR_Loaded:
		break;
	}

	CEntityTemplate* entityTemplate = creatureDefinition->m_entityTemplate.Get();
	if ( !CheckEntityTemplate( entityTemplate) )
	{
		return NULL;
	}
	return entityTemplate;
}
void CBaseCreatureEntry::HandleWaveRespawn( CSpawnTreeInstance& instance ) const
{
	if ( instance[ i_waveDelayIsOn ] )
	{
		if ( instance[ i_waveTimeout ] < GGame->GetEngineTime() )
		{
			// respawn wave
			instance[ i_numCreatureDead ] = 0;
			instance[ i_waveDelayIsOn ] = false;
			// randomize new wave creature count
			instance[ i_numCreaturesToSpawn ] = RecalculateNumCreaturesToSpawn( instance );
		}
	}
}

Int16 CBaseCreatureEntry::RecalculateNumCreaturesToSpawn(  CSpawnTreeInstance& instance ) const
{
	Int16 baseVal = ( m_quantityMin < m_quantityMax ) ? GEngine->GetRandomNumberGenerator().Get< Int16 >( static_cast< Int16 >( m_quantityMin ), static_cast< Int16 >( m_quantityMax ) ) : Int16( m_quantityMin );
	
	return ( Int16) Red::Math::MCeil( ( baseVal * instance[ i_setup ].m_spawnRatio ) );
}

Bool CBaseCreatureEntry::CanSpawnInitializerClass( ISpawnTreeInitializer* defObject ) const
{
	if ( IsInitializerClassConflicting( defObject ) )
	{
		return false;
	}
	return true;
}
Bool CBaseCreatureEntry::IsInitializerClassConflicting( ISpawnTreeInitializer* defObject ) const
{
	if ( !defObject->IsSpawnable() )
	{
		return true;
	}

	for ( Uint32 i = 0, n = m_initializers.Size(); i != n; ++i )
	{
		if ( defObject->IsConflicting( m_initializers[ i ].Get() ) )
		{
			return true;
		}
	}

	return false;
}

Bool CBaseCreatureEntry::CanAddChild() const
{
	return true;
}
void CBaseCreatureEntry::AddChild( IEdSpawnTreeNode* node )
{
	ListOperations::AddChild( m_initializers, node );
}
void CBaseCreatureEntry::RemoveChild( IEdSpawnTreeNode* node )
{
	ListOperations::RemoveChildHandle( m_initializers, node );
}
Int32 CBaseCreatureEntry::GetNumChildren() const
{
	return m_initializers.Size();
}
IEdSpawnTreeNode* CBaseCreatureEntry::GetChild( Int32 index ) const
{
	return m_initializers[ index ].Get();
}
Bool CBaseCreatureEntry::UpdateChildrenOrder()
{
	Bool dirty = ListOperations::UpdateChildrenOrder( m_initializers );
	return TBaseClass::UpdateChildrenOrder() || dirty;
}
void CBaseCreatureEntry::GetRootClassForChildren( TDynArray< CClass* >& rootClasses, ESpawnTreeType spawnTreeType ) const
{
	rootClasses.PushBack( ISpawnTreeInitializer::GetStaticClass() );
}
Bool CBaseCreatureEntry::CanSpawnChildClass( const CClass* classId, ESpawnTreeType spawnTreeType ) const
{
	if ( classId->IsAbstract() )
	{
		return false;
	}
	ISpawnTreeInitializer* defObject = const_cast< CClass* >( classId )->GetDefaultObject< ISpawnTreeInitializer >();
	if ( !defObject )
	{
		return false;
	}

	if ( !CanSpawnInitializerClass( defObject ) )
	{
		return false;
	}

	return true;
}

Bool CBaseCreatureEntry::AreInitializersSaving( CSpawnTreeInstance& instance ) const
{
	InitializersIterator iterator( *this, instance );

	return ISpawnTreeInitializer::AreInitializersStateSaving( iterator );
}
Bool CBaseCreatureEntry::SaveInitializers( CSpawnTreeInstance& instance, IGameSaver* writer ) const
{
	InitializersIterator iterator( *this, instance );

	return ISpawnTreeInitializer::SaveInitializersState( iterator, writer );
}
Bool CBaseCreatureEntry::LoadInitializers( CSpawnTreeInstance& instance, IGameLoader* reader ) const
{
	InitializersIterator iterator( *this, instance );

	return ISpawnTreeInitializer::LoadInitializersState( iterator, reader );
}

Bool CBaseCreatureEntry::IsNodeStateSaving( CSpawnTreeInstance& instance ) const
{
	// check if we are keeping data about 
	if ( instance[ i_numCreatureDead ] > 0 )
	{
		return true;
	}
	if ( instance[ i_waveDelayIsOn ] )
	{
		return true;
	}
	if ( m_quantityMin != m_quantityMax && GGame->GetTimeManager()->GetTime() < instance[ i_recalculateTimeout ] )
	{
		return true;
	}
	if ( AreInitializersSaving( instance ) )
	{
		return true;
	}

	return false;
}
void CBaseCreatureEntry::SaveNodeState( CSpawnTreeInstance& instance, IGameSaver* writer ) const
{
	EngineTime engineTime = GGame->GetEngineTime();

	Int16 numCreaturesDead = instance[ i_numCreatureDead ];
	Int16 numCreaturesToSpawn = instance[ i_numCreaturesToSpawn ];
	Bool isWaveDelayOn = instance[ i_waveDelayIsOn ];
	Float waveDelay = isWaveDelayOn ? Float( instance[ i_waveTimeout ] - engineTime ) : 0.f;
	GameTime recalculateTimeout = instance[ i_recalculateTimeout ];

	writer->WriteValue< Int16 >( CNAME( dead ), numCreaturesDead );
	writer->WriteValue< Bool >( CNAME( on ), isWaveDelayOn );
	writer->WriteValue< Float >( CNAME( delay ), waveDelay );
	writer->WriteValue< Int16 >( CNAME( toSpawn ), numCreaturesToSpawn );
	writer->WriteValue< GameTime >( CNAME( recalculateTimeout ), recalculateTimeout );	

	SaveInitializers( instance, writer );
}
Bool CBaseCreatureEntry::LoadNodeState( CSpawnTreeInstance& instance, IGameLoader* reader ) const
{
	EngineTime engineTime = GGame->GetEngineTime();

	Int16 numCreaturesDead = 0;
	Bool isWaveDelayOn = false;
	Float waveDelay = 0.f;
	Int16 numCreaturesToSpawn = 0;
	GameTime recalculateTimeout = 0;

	reader->ReadValue< Int16 >( CNAME( dead ), numCreaturesDead );
	reader->ReadValue< Bool >( CNAME( on ), isWaveDelayOn );
	reader->ReadValue< Float >( CNAME( delay ), waveDelay );
	if ( reader->GetSaveVersion() >= SAVE_VERSION_NUM_CREATURES_TO_SPAWN_SAVING )
	{
		reader->ReadValue< Int16 >( CNAME( toSpawn ), numCreaturesToSpawn );
		reader->ReadValue< GameTime >( CNAME( recalculateTimeout ), recalculateTimeout );
	}

	instance[ i_numCreatureDead ] = numCreaturesDead;
	instance[ i_waveDelayIsOn ] = isWaveDelayOn;
	instance[ i_waveTimeout ] = engineTime + waveDelay;
	instance[ i_numCreaturesToSpawn ] = numCreaturesToSpawn;
	instance[ i_recalculateTimeout ] = recalculateTimeout;

	LoadInitializers( instance, reader );

	return true;
}

void CBaseCreatureEntry::PostLoadGameplayState( CSpawnTreeInstance& instance ) const
{
	UpdateSetup( instance );
}

Bool CBaseCreatureEntry::CanBeHidden() const
{
	return true;
}
Color CBaseCreatureEntry::GetBlockColor() const
{
	return Color( 30, 192, 30 );
}
String CBaseCreatureEntry::GetBitmapName() const
{
	static String STR( TXT("IMG_SPAWNTREE_CREATURE_ENTRY") );
	return STR;
}

String CBaseCreatureEntry::GetEditorFriendlyName() const
{
	static String STR( TXT("BaseEntry") );
	return STR;
}
