#include "build.h"
#include "spawnTreeEntry.h"

#include "aiParamInjectHandler.h"
#include "aiSpawnTreeParameters.h"
#include "encounter.h"
#include "spawnTreeInitializer.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "../engine/jobSpawnEntity.h"
#include "../engine/game.h"
#include "../engine/idTagManager.h"
#include "entityPool.h"


IMPLEMENT_RTTI_ENUM( EEntryState );

IMPLEMENT_ENGINE_CLASS( CCreatureEntry );

namespace
{
	template < class T >
	class ScopedSwap
	{
		T&		m_memory;
		T		m_value;

	public:
		ScopedSwap( T& val, const T& newVal )
			: m_memory( val )
			, m_value( val )												{ val = newVal; }
		~ScopedSwap()
		{
			m_memory = m_value;
		}
	};
};

////////////////////////////////////////////////////////////////////////////
// CCreatureEntry
CEncounterCreatureDefinition* CCreatureEntry::GetCreatureDefinition( CSpawnTreeInstance& instance ) const
{
	Int16 id = instance[ i_creatureDefinition ];
	if ( id >= 0 )
	{
		return instance.GetEncounter()->GetCreatureDefinition( id );
	}
	return NULL;
}

IJobEntitySpawn* CCreatureEntry::CreateSpawnJob( const Vector3& pos, Float yaw, Uint32 sp, CSpawnTreeInstance& instance, Int32 spawnGroupToUse  )
{
	CEncounterCreatureDefinition* creatureDefinition = GetCreatureDefinition( instance );
	CEncounter* encounter = instance.GetEncounter();
	CEntityTemplate* entityTemplate = instance[ i_templateLoadRequest ].GetEntityTemplate();

	EntitySpawnInfo data;
	data.m_template = entityTemplate;
	data.m_tags = creatureDefinition->GetTags();
	data.m_spawnPosition = pos;
	data.m_spawnRotation = EulerAngles(0.0f, 0.0f, m_randomizeRotation ? GEngine->GetRandomNumberGenerator().Get< Float >( 360.0f ) : yaw );
	data.m_importantEntity = m_group == 0;
	data.m_entityFlags = EF_DestroyableFromScript;
	data.m_encounterEntryGroup = spawnGroupToUse;

	//
	// This is a temporary fix to avoid going through all spawn trees and adding Spawn Persistent initializers to all merchants in encounters.
	// After adding Spawn Persistent initializers in spawn trees, this if should be removed.
	//
	if ( m_quantityMax == 1 && data.m_template->GetEntityObject()->GetClass()->GetName() == CNAME( W3MerchantNPC ) )
	{
		const auto* creatureEntry = encounter->FindActiveCreatureEntry( GetUniqueListId( instance ) );
		data.m_idTag = GCommonGame->GetIdTagManager()->CreateFromUint64( creatureEntry->m_branchHash );	
		data.m_entityNotSavable = false;
	}

	ReserveAPForSpawn( instance, data, sp );

	// Inject params
	data.AddHandler( new CAiSpawnSystemParamInjectHandler( encounter->GetEncounterParameters() ) );

	const auto& initializers = GetRuntimeInitializers( instance );
	for( auto it = initializers.Begin(), end = initializers.End(); it != end; ++it )
	{
		if ( (*it).m_initializer )
		{
			(*it).m_initializer->OnCreatureSpawn( data, (*it).m_instance, this );
		}
	}

	if ( data.m_appearances.Empty() )
	{
		data.m_appearances.PushBack( creatureDefinition->SelectUniqueAppearance( entityTemplate ) );
	}

	return GCommonGame->GetEntityPool()->SpawnEntity( std::move( data ), CEntityPool::SpawnType_Encounter );
}

Bool CCreatureEntry::UsesCreatureDefinition( CName creatureDefinition ) const
{
	return m_creatureDefinition == creatureDefinition;
}

void CCreatureEntry::CollectUsedCreatureDefinitions( TSortedArray< CName >& inOutNames )
{
	if ( !m_creatureDefinition.Empty() )
	{
		inOutNames.InsertUnique( m_creatureDefinition );
	}
}

CCreatureEntry::ELoadTemplateResult CCreatureEntry::UpdateLoading( CSpawnTreeInstance& instance, SSpawnTreeUpdateSpawnContext& context )
{
	if ( instance[ i_noSpawnPointDefined ] )
	{
		 return LOAD_FAILED;
	}

	// request asynchronous load of npc
	Int16 creatureDefId = GetCreatureDefinitionId( instance );
	if ( creatureDefId < 0 )
	{
		return LOAD_FAILED;
	}
	CEncounter* encounter = instance.GetEncounter();
	CEncounterCreatureDefinition* creatureDefinition = encounter->GetCreatureDefinition( creatureDefId );	

	if ( creatureDefinition == nullptr )
	{
		return LOAD_FAILED;
	}

	CTemplateLoadBalancer* loader = GCommonGame->GetTemplateLoadBalancer();

	const TSoftHandle< CEntityTemplate >& templateHandle = creatureDefinition->GetEntityTemplate();
	CTemplateLoadRequest& request = instance[ i_templateLoadRequest ];
	request.Initialize( templateHandle );

	// initialy create request, with ignorable priority
	CTemplateLoadBalancer::SRequestInfo info = loader->RegisterTemplateRequest( request );
	if ( info.m_isLoaded )
	{
		if ( !CEncounterCreatureDefinition::CheckEntityTemplate( request.GetEntityTemplate() ) )
		{
			return LOAD_FAILED;
		}
		return LOAD_SUCCESS;
	}
	if ( info.m_isInvalid )
	{
		return LOAD_FAILED;
	}

	// infrequent priority updates
	EngineTime& timeout = instance[ i_loadPriorityRecalculationTimeout ];
	EngineTime currentTime = GGame->GetEngineTime();
	if ( timeout > currentTime )
	{
		return LOAD_PENDING;
	}

	timeout = currentTime + loader->PriorityUpdateDelay();

	// Determine closest spawnpoint distance and if we 'can spawn'
	Bool canSpawn = false;
	Float closestDistance = 1024.f;

	Vector3	spawnPos;
	Float spawnYaw;
	Uint32 sp = 0xffffffff;
	EFindSpawnResult eFSR;

	::ScopedSwap< Bool > ignoreVision( context.m_ignoreVision, true );
	::ScopedSwap< Bool > ignoreMinDistance( context.m_ignoreMinDistance, true );
	::ScopedSwap< Bool > optimizeTimeouts( context.m_optimizeTimeouts, false );

	eFSR = FindSpawnPoint( instance, context, spawnPos, spawnYaw, sp );

	instance[ i_noSpawnPointDefined ] = false;
	switch ( eFSR )
	{
	case FSR_NoneDefined:
		// Marking error for debug purposes
		instance[ i_noSpawnPointDefined ] = true;
		loader->UnregisterTemplateRequest( request );
		return LOAD_FAILED;

	case FSR_FoundOne:
		canSpawn = true;
		closestDistance = ( spawnPos - context.m_referenceTransform.GetTranslationRef().AsVector3() ).Mag();
		break;

	default:
		ASSUME( false );
	case FSR_FoundNone:
		{
			canSpawn = false;
			if ( FindClosestSpawnPoint( instance, context, spawnPos, spawnYaw, sp ) == FSR_FoundOne )
			{
				closestDistance = ( spawnPos - context.m_referenceTransform.GetTranslationRef().AsVector3() ).Mag();
			}
		}
		break;
	}

	// compute priority!
	CTemplateLoadBalancer::Priority loadBalancerPriority = CTemplateLoadBalancer::ActorPriority( canSpawn, m_group, closestDistance );
	loader->UpdateTemplateRequest( request, loadBalancerPriority );
	if ( info.m_isLoaded )
	{
		if ( !CEncounterCreatureDefinition::CheckEntityTemplate( request.GetEntityTemplate() ) )
		{
			return LOAD_FAILED;
		}
		return LOAD_SUCCESS;
	}
	if ( info.m_isInvalid )
	{
		return LOAD_FAILED;
	}
	return LOAD_PENDING;
}



Bool CCreatureEntry::IsCreatureElligible( CSpawnTreeInstance& instance, const CEncounterCreaturePool::SCreature &creature )
{
	if ( creature.m_stateFlags & CEncounterCreaturePool::SCreature::FLAG_IS_IN_PARTY )
	{
		return false;
	}
	const Int16 creatureDefId = GetCreatureDefinitionId( instance );
	if ( creature.m_creatureDefId != creatureDefId )
	{
		return false;
	}
	return true;
}

Bool CCreatureEntry::UpdateCollectCreaturesToSteal( CSpawnTreeInstance& instance, CEncounter::SActiveEntry& entry )
{
	CEncounter* encounter = instance.GetEncounter();

	// Try 'gathering' guyz from detached boyz
	CEncounterCreaturePool& creaturePool						= encounter->GetCreaturePool();
	CEncounterCreaturePool::SCreatureList& detachedCreatures	= creaturePool.GetDetachedCreaturesList();

	Bool isGatheringDetachedCreatures = false;
	for ( auto creatureIt = detachedCreatures.Begin(), end = detachedCreatures.End(); creatureIt != end; )
	{
		CEncounterCreaturePool::SCreature& creature = *creatureIt;
		ASSERT( creature.m_listId == 0 );
		// Possibly current creature entry might be removed from creature list during execution.
		// In this case we should progress iterator, and iterator will still point to detached list member.
		++creatureIt;

		// check if we are re-collecting our own creature
		if ( creature.m_lastOwningEntry != GetUniqueListId( instance ) )
		{
			// test if creature fits given entry
			if ( !IsCreatureElligible( instance, creature ) )
			{
				continue;
			}

			// name-to-name filtering
			CName lastEntryName;
			CEncounter::SActiveEntry* activeEntry = encounter->FindActiveCreatureEntry( creature.m_lastOwningEntry );
			if ( activeEntry )
			{
				lastEntryName = activeEntry->m_entry->GetName();
			}
			if ( m_nodeName != lastEntryName )
			{
				continue;
			}

			// filter by initializers
			if ( !AcceptActorByInitializers( instance, creature.m_actor ) )
			{
				continue;
			}

			// randomize delay
			Float delayRatio = (GEngine->GetRandomNumberGenerator().Get< Float >()*2.f)-1.f;				// [ -1 .. 1 ]
			Bool neg = delayRatio < 0.f;
			delayRatio = delayRatio * delayRatio;															// [ 0 .. 1 ] sqr distribution around 0
			if ( neg )
			{
				delayRatio = - delayRatio;																	// [ -1 .. 1 ] sqr distribution around 0
			}
			delayRatio = (delayRatio + 1.f) * 0.5f;															// [ 0 .. 1 ] sqr distribution around 0.5

			// push into stealing list
			SSpawnTreeEntryStealCreatureState steal;
			steal.m_isDelayed = true;
			steal.m_previousOwnerEntry = creature.m_lastOwningEntry;
			steal.m_actor = creature.m_actor;
			steal.m_delayTimeout = GGame->GetEngineTime() + delayRatio * instance[ i_setup ].m_stealingDelay;

			instance[ i_stealingInProgress ].PushBack( steal );
		}
		
		StealCreature( instance, creature );

		++(instance[ i_numCreaturesSpawned ]);

		if ( WasSpawnLimitReachedCommon( instance ) && !instance[ i_setup ].m_greedyStealing )
		{
			return true;
		}
	};
	return false;
}


Bool CCreatureEntry::WasCreatureLimitReached( CSpawnTreeInstance& instance, Int32& groupToUse )
{
	if ( m_group != 0 )
	{
		CEntityPool* entityPool = GCommonGame->GetEntityPool();
		if ( entityPool->GetEntitySpawnedCount() >= entityPool->GetMaxEntitySpawnCount() )
		{
			return true;
		}
	}
	groupToUse = m_group;
	Int32 limit = CEncounterGlobalSettings::GetInstance().GetLimitForGroup( groupToUse );;
	// if there is no space in current group, check less important groups
	do 
	{		
		if( GCommonGame->GetEncounterSpawnGroup().GetGroupCount( groupToUse ) < limit )
		{
			break;
		}
		++groupToUse;
		limit = CEncounterGlobalSettings::GetInstance().GetLimitForGroup( groupToUse );
	} while( limit >=0 );
	
	if( limit == -1 )
	{
		return true;
	}

	CEncounter* encounter		= instance.GetEncounter();
	const Int16 creatureDefId	= GetCreatureDefinitionId( instance );
	CEncounterCreatureDefinition*const creatureDefinition = encounter->GetCreatureDefinition( creatureDefId );
	return creatureDefinition->WasSpawnLimitReached();
}
Bool CCreatureEntry::OnSpawnJobIsDoneInternal( CSpawnTreeInstance& instance, IJobEntitySpawn* spawnJob, CEncounter::SActiveEntry& encounterEntryData )
{
	ASSERT( spawnJob->GetEntitiesCount() == 1 );
	// create entries
	CEntity* entity = spawnJob->GetSpawnedEntity( 0 );
	if ( !entity )
	{
		return false;
	}

	CEncounter* encounter = instance.GetEncounter();
	CActor* actor = static_cast< CActor* >( entity );
	Int16 creatureDefId = GetCreatureDefinitionId( instance );
	ASSERT( creatureDefId >= 0 );

	encounter->RegisterSpawnedCreature( actor, encounterEntryData, creatureDefId );

	const ISpawnTreeInitializer::EOutput initializerResult = ActivateInitializers( instance, actor, spawnJob->IsSpawnedFromPool( 0 ) ? EST_PoolSpawn : EST_NormalSpawn );
	ASSERT( initializerResult == ISpawnTreeInitializer::OUTPUT_SUCCESS, TXT( "Something is wrong with encouter initializers, please check them" ) );

	++(instance[ i_numCreaturesSpawned ]);

	return true;
}

Bool CCreatureEntry::OnCreatureLoaded( CSpawnTreeInstance& instance, CEncounterCreaturePool::SCreature& creature, CEncounter::SActiveEntry& encounterEntryData )
{
	CEncounter* encounter = instance.GetEncounter();

	if ( IsActive( instance ) )
	{
		CEncounter* encounter = instance.GetEncounter();

		encounter->RemoveScheduledDespawn( creature );

		CEncounterCreaturePool& creaturePool = encounter->GetCreaturePool();
		creaturePool.AttachCreatureToList( creature, encounterEntryData.m_spawnedActors );
		++(instance[ i_numCreaturesSpawned ]);
	}

	ActivateInitializers( instance, creature.m_actor, EST_GameIsRestored, 0 );
	creature.m_stateFlags |= CEncounterCreaturePool::SCreature::FLAG_WAS_ACTIVATED_BY_ENTRY;

	return true;
}

ISpawnTreeInitializer::EOutput CCreatureEntry::ActivateInitializers( CSpawnTreeInstance& instance, CActor* actor, ESpawnType spawnType, Int32 definitionCount )
{
	ASSERT( definitionCount == 0 );
	return TBaseClass::ActivateInitializers( instance, actor, spawnType, definitionCount );
}

Int32 CCreatureEntry::GetCreatureDefinitionsCount() const
{
	return 1;
}

Int16 CCreatureEntry::GetCreatureDefinitionId( CSpawnTreeInstance& instance, Int32 i ) const
{
	ASSERT( i == 0 );
	return GetCreatureDefinitionId( instance );
}

// Instance buffer interface
void CCreatureEntry::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_creatureDefinition;
	compiler << i_templateLoadRequest;

}

void CCreatureEntry::Deactivate( CSpawnTreeInstance& instance )
{
	instance[ i_templateLoadRequest ].Clear();

	TBaseClass::Deactivate( instance );
}

void CCreatureEntry::OnInitData( CSpawnTreeInstance& instance, CSpawnTreeInitializationContext& context )
{
	TBaseClass::OnInitData( instance, context );

	CEncounter* encounter = instance.GetEncounter();

	instance[ i_creatureDefinition ] = encounter->GetCreatureDefinitionId( m_creatureDefinition );

}
void CCreatureEntry::OnDeinitData( CSpawnTreeInstance& instance )
{
	RED_ASSERT( !instance[ i_templateLoadRequest ].IsRequesting(), TXT("Template request was leaking.") );

	TBaseClass::OnDeinitData( instance );
}

String CCreatureEntry::GetBlockCaption() const
{
	if ( m_quantityMax > 1 )
	{
		if ( m_quantityMax != m_quantityMin )
		{
			return String::Printf( TXT("Entry %s (%d-%d)"), m_creatureDefinition.AsString().AsChar(), m_quantityMin, m_quantityMax );
		}
		else
		{
			return String::Printf( TXT("Entry %s (%d)"), m_creatureDefinition.AsString().AsChar(), m_quantityMax );
		}
	}
	return String::Printf( TXT("Entry %s"), m_creatureDefinition.AsString().AsChar() );
}
String CCreatureEntry::GetBlockDebugCaption( const CSpawnTreeInstance& instanceBuffer ) const
{
	String str = String::Printf( TXT("Entry %s (%d of %d)"), m_creatureDefinition.AsString().AsChar(), instanceBuffer[ i_numCreaturesSpawned ], GetNumCreaturesToSpawn( instanceBuffer ) );

	if ( instanceBuffer[ i_numCreatureDead ] > 0 )
	{
		str = str + String::Printf( TXT(" [%d dead]" ), instanceBuffer[ i_numCreatureDead ] );
	}

	if ( instanceBuffer[ i_waveDelayIsOn ] )
	{
		Float respawnTime = Max( Float( instanceBuffer[ i_waveTimeout ] - GGame->GetEngineTime() ), 0.f );
		str = str + String::Printf( TXT(" [wave respawn in %.1f]" ), respawnTime );
	}

	if ( instanceBuffer[ i_noSpawnPointDefined ] )
	{
		// Error overwriting string
		str = String::Printf( TXT("Entry %s [No SP !]" ), m_creatureDefinition.AsString().AsChar() );
	}

	return Move( str );
}
String CCreatureEntry::GetEditorFriendlyName() const
{
	static String STR( TXT("Entry") );
	return STR;
}
Bool CCreatureEntry::IsHiddenByDefault() const
{
	return true;
}

void CCreatureEntry::GatherBudgetingStats( ICreatureDefinitionContainer* container, SBudgetingStats& stats )
{	
	CEncounterCreatureDefinition* creatureDefinition = container->GetCreatureDefinition( m_creatureDefinition );

	if( creatureDefinition != nullptr )
	{
		SBudgetingStats::TemplateData budgetData;

		budgetData.m_template = creatureDefinition->GetEntityTemplate().Get();
		if ( budgetData.m_template )
		{
			TDynArray< const CEntityAppearance* > appList;
			budgetData.m_template->GetAllEnabledAppearances( appList );
			budgetData.m_usedApperances = appList;

			budgetData.m_maxSpawnLimit = m_quantityMax;
			stats.m_usedTemplates.PushBack( budgetData );
		}
	}	
}
