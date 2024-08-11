#include "build.h"
#include "spawnTreeMultiEntry.h"

#include "../core/instanceDataLayoutCompiler.h"

#include "../engine/jobSpawnEntity.h"

#include "aiParamInjectHandler.h"
#include "aiSpawnTreeParameters.h"
#include "encounter.h"
#include "entityPool.h"
#include "reactionSceneActor.h"
#include "spawnTreeNodeListOperations.inl"



IMPLEMENT_ENGINE_CLASS( CSpawnTreeEntrySubDefinition );
IMPLEMENT_ENGINE_CLASS( CCreaturePartyEntry );


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


//////////////////////////////////////////////////////////////////////////
// CSpawnTreeEntrySubDefinition
//////////////////////////////////////////////////////////////////////////
CEncounterCreatureDefinition* CSpawnTreeEntrySubDefinition::GetCreatureDefinition( CSpawnTreeInstance& instance ) const
{
	Int16 id = instance[ i_creatureDefinition ];
	if ( id >= 0 )
	{
		return instance.GetEncounter()->GetCreatureDefinition( id );
	}
	return NULL;
}
void CSpawnTreeEntrySubDefinition::OnFullRespawn( CSpawnTreeInstance& instance ) const
{
	for( Uint32 i = 0, n = m_initializers.Size(); i != n; ++i )
	{
		ISpawnTreeInitializer* initializer = m_initializers[ i ];
		if ( initializer )
		{
			initializer->OnFullRespawn( instance );
		}
	}
}
void CSpawnTreeEntrySubDefinition::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	compiler << i_creatureDefinition;
	compiler << i_templateLoadRequest;

	for( Uint32 i = 0, n = m_initializers.Size(); i != n; ++i )
	{
		ISpawnTreeInitializer* initializer = m_initializers[ i ];
		if ( initializer )
		{
			initializer->OnBuildDataLayout( compiler );
		}
	}
}
void CSpawnTreeEntrySubDefinition::OnInitData( CSpawnTreeInstance& instance, CSpawnTreeInitializationContext& context ) const
{
	instance[ i_creatureDefinition ]	= instance.GetEncounter()->GetCreatureDefinitionId( m_creatureDefinition );

	for( Uint32 i = 0, n = m_initializers.Size(); i != n; ++i )
	{
		ISpawnTreeInitializer* initializer = m_initializers[ i ];
		if ( initializer )
		{
			initializer->OnInitData( instance );
		}
	}
}
void CSpawnTreeEntrySubDefinition::OnDeinitData( CSpawnTreeInstance& instance ) const
{
	RED_ASSERT( !instance[ i_templateLoadRequest ].IsRequesting(), TXT( "Party template request was leaking." ) );

	for( Uint32 i = 0, n = m_initializers.Size(); i != n; ++i )
	{
		ISpawnTreeInitializer* initializer = m_initializers[ i ];
		if ( initializer )
		{
			initializer->OnDeinitData( instance );
		}
	}
}

void CSpawnTreeEntrySubDefinition::Deactivate( CSpawnTreeInstance& instance ) const
{
	instance[ i_templateLoadRequest ].Clear();
}

Bool CSpawnTreeEntrySubDefinition::GenerateIdsRecursively()
{
	Bool result = IEdSpawnTreeNode::GenerateIdsRecursively();

	for( Uint32 i = 0, n = m_initializers.Size(); i != n; ++i )
	{
		ISpawnTreeInitializer* initializer = m_initializers[ i ];
		if ( initializer )
		{
			result |= initializer->GenerateIdsRecursively();
		}
	}
	return result;
}

void CSpawnTreeEntrySubDefinition::OnPostLoad()
{
	TBaseClass::OnPostLoad();

	RemoveNullChildren();
}

CObject* CSpawnTreeEntrySubDefinition::AsCObject()
{
	return this;
}
IEdSpawnTreeNode* CSpawnTreeEntrySubDefinition::GetParentNode() const
{
	CObject* parent = GetParent();
	if ( parent )
	{
		return Cast< CCreaturePartyEntry >( parent );
	}
	return NULL;
}
Bool CSpawnTreeEntrySubDefinition::CanAddChild() const
{
	return true;
}
void CSpawnTreeEntrySubDefinition::AddChild( IEdSpawnTreeNode* node )
{
	ListOperations::AddChild( m_initializers, node );
}
void CSpawnTreeEntrySubDefinition::RemoveChild( IEdSpawnTreeNode* node )
{
	ListOperations::RemoveChild( m_initializers, node );
}
Int32 CSpawnTreeEntrySubDefinition::GetNumChildren() const
{
	return m_initializers.Size();
}
IEdSpawnTreeNode* CSpawnTreeEntrySubDefinition::GetChild( Int32 index ) const
{
	return m_initializers[ index ];
}
Bool CSpawnTreeEntrySubDefinition::UpdateChildrenOrder()
{
	return ListOperations::UpdateChildrenOrder( m_initializers );
}
void CSpawnTreeEntrySubDefinition::GetRootClassForChildren( TDynArray< CClass* >& rootClasses, ESpawnTreeType spawnTreeType ) const
{
	rootClasses.PushBack( ISpawnTreeInitializer::GetStaticClass() );
}
Bool CSpawnTreeEntrySubDefinition::CanSpawnChildClass( const CClass* classId, ESpawnTreeType spawnTreeType ) const
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

	if ( !defObject->IsSpawnableOnPartyMembers() )
	{
		return false;
	}

	CCreaturePartyEntry* party = Cast< CCreaturePartyEntry >( GetParent() );
	if ( !party )
	{
		return false;
	}

	if ( party->IsInitializerClassConflicting( defObject ) )
	{
		return false;
	}

	for ( Uint32 i = 0, n = m_initializers.Size(); i != n; ++i )
	{
		ISpawnTreeInitializer* initializer = m_initializers[ i ];
		if ( initializer && initializer->IsConflicting( defObject ) )
		{
			return false;
		}
	}

	return true;
}
Bool CSpawnTreeEntrySubDefinition::CanBeHidden() const
{
	return true;
}
Bool CSpawnTreeEntrySubDefinition::IsHiddenByDefault() const
{
	return true;
}
Color CSpawnTreeEntrySubDefinition::GetBlockColor() const
{
	return Color::GRAY;
}
String CSpawnTreeEntrySubDefinition::GetBlockCaption() const
{
	String memberName = m_partyMemberId.Empty() ? String( TXT("Member") ) : m_partyMemberId.AsString();
	String creatureDefinitionName = m_creatureDefinition.Empty() ? String( TXT("UNDEFINED") ) : String::Printf( TXT( "'%ls'" ), m_creatureDefinition.AsChar() );
	String multitude;
	if ( m_creatureCount != 1 )
	{
		multitude = String::Printf( TXT(" x%d"), m_creatureCount );
	}

	return String::Printf( TXT("%s %s%s"), memberName.AsChar(), creatureDefinitionName.AsChar(), multitude.AsChar() );

}
String CSpawnTreeEntrySubDefinition::GetEditorFriendlyName() const
{
	return TXT("Party member");
}
//////////////////////////////////////////////////////////////////////////
// CPartySpawnOrganizer
//////////////////////////////////////////////////////////////////////////
IMPLEMENT_ENGINE_CLASS( CPartySpawnOrganizer );

void CPartySpawnOrganizer::PrePartySpawn( CCreaturePartyEntry* entry, TDynArray< EntitySpawnInfo >& spawnInfo, SEncounterSpawnPoint* sp )
{
	// default, empty implementation
}
void CPartySpawnOrganizer::PostPartySpawn( const CEncounterCreaturePool::Party &party )
{
	// default, empty implementation
}

//////////////////////////////////////////////////////////////////////////
// CCreaturePartyEntry::SpawnInfoIterator
//////////////////////////////////////////////////////////////////////////
void CCreaturePartyEntry::SpawnInfoIterator::Progress()
{
	while ( m_guysToSpawnLeft <= 0 )
	{
		m_subDefIndex = m_subDefNextIndex++;
		m_subDef = m_owner->m_subDefinitions[ m_subDefIndex ];
		m_guysToSpawnLeft = m_subDef->GetSpawnedCreaturesCount();
	}
}

CCreaturePartyEntry::SpawnInfoIterator::SpawnInfoIterator( CCreaturePartyEntry* entry )
	: m_owner( entry )
	, m_subDefNextIndex( 0 )
	, m_guysToSpawnLeft( 0 )
{
}

void CCreaturePartyEntry::SpawnInfoIterator::FetchNext()
{
	--m_guysToSpawnLeft;
	Progress();
}

//////////////////////////////////////////////////////////////////////////
// CCreaturePartyEntry::InitializersIterator
//////////////////////////////////////////////////////////////////////////
CCreaturePartyEntry::InitializersIterator::InitializersIterator( const CCreaturePartyEntry& entry, CSpawnTreeInstance& instance )
	: Super( entry, instance )
	, m_subDefIt( entry.m_subDefinitions.Begin() ) 
	, m_subDefEnd( entry.m_subDefinitions.End() )
	, m_initializerIdx( 0 )
{
	
}

Bool CCreaturePartyEntry::InitializersIterator::Next( const ISpawnTreeInitializer*& outInitializer, CSpawnTreeInstance*& instanceBuffer )
{
	while ( m_subDefIt != m_subDefEnd )
	{
		CSpawnTreeEntrySubDefinition* def = *m_subDefIt;
		const auto& initializers = def->GetInitializers();
		if ( m_initializerIdx < initializers.Size() )
		{
			outInitializer = initializers[ m_initializerIdx ];
			instanceBuffer = &m_instance;

			++m_initializerIdx;
			return true;
		}
		m_initializerIdx = 0;
		++m_subDefIt;
	}
	return Super::Next( outInitializer, instanceBuffer );
}
void CCreaturePartyEntry::InitializersIterator::Reset()
{
	Super::Reset();

	const CCreaturePartyEntry& entry = static_cast< const CCreaturePartyEntry& >( m_entry );

	m_subDefIt = entry.m_subDefinitions.Begin();
	m_subDefEnd = entry.m_subDefinitions.End();
	m_initializerIdx = 0;
}

//////////////////////////////////////////////////////////////////////////
// CCreaturePartyEntry
//////////////////////////////////////////////////////////////////////////
CCreaturePartyEntry::CCreaturePartyEntry()
	: CBaseCreatureEntry()
	, m_blockChats( true )
	, m_synchronizeWork( true )
{
}

CCreaturePartyEntry::~CCreaturePartyEntry()
{
	CPartySpawnOrganizer* partySpawn = m_partySpawnOrganizer.Get();
	if ( partySpawn )
	{
		delete partySpawn;
	}
}

IJobEntitySpawn* CCreaturePartyEntry::CreateSpawnJob( const Vector3& pos, Float yaw, Uint32 sp, CSpawnTreeInstance& instance, Int32 spawnGroupToUse  )
{
	CEncounter* encounter = instance.GetEncounter();

	Uint32 spawnCount = instance[ i_spawnCount ];
	TDynArray< EntitySpawnInfo > spawnInfos( spawnCount );
	CEncounterParameters* params = encounter->GetEncounterParameters();

	Uint32 spawnListIterator = 0;

	for ( Uint32 i = 0, n = m_subDefinitions.Size(); i != n; ++i )
	{
		CSpawnTreeEntrySubDefinition* subdef = m_subDefinitions[ i ];
		CEncounterCreatureDefinition* creatureDefinition = subdef->GetCreatureDefinition( instance );
		CEntityTemplate* entityTemplate = subdef->GetTemplateLoadRequest( instance ).GetEntityTemplate();
		Uint32 spawnCreaturesCount = subdef->GetSpawnedCreaturesCount();
		for ( Uint32 i = 0; i < spawnCreaturesCount; ++i )
		{
			EntitySpawnInfo& data = spawnInfos[ spawnListIterator++ ];
			data.m_template = entityTemplate;
			data.m_tags = creatureDefinition->GetTags();
			data.m_spawnPosition = pos;
			data.m_spawnRotation = EulerAngles(0.0f, 0.0f, m_randomizeRotation ? GEngine->GetRandomNumberGenerator().Get< Float >( 360.0f ) : yaw );
			data.m_importantEntity = m_group == 0;
			data.m_entityFlags = EF_DestroyableFromScript;
			data.m_encounterEntryGroup = spawnGroupToUse;

			ReserveAPForSpawn( instance, data, sp );
			// Run initializers
			data.AddHandler( new CAiSpawnSystemParamInjectHandler( params ) );

			const auto& initializers = GetRuntimeInitializers( instance );
			for( auto it = initializers.Begin(), end = initializers.End(); it != end; ++it )
			{
				if ( (*it).m_initializer )
				{
					(*it).m_initializer->OnCreatureSpawn( data, (*it).m_instance, this );
				}
			}

			const auto& subdefInitializers = subdef->GetInitializers();
			for( auto it = subdefInitializers.Begin(), end = subdefInitializers.End(); it != end; ++it )
			{
				if ( *it )
				{
					(*it)->OnCreatureSpawn( data, &instance, this );
				}
			}

			if ( data.m_appearances.Empty() )
			{
				data.m_appearances.PushBack( creatureDefinition->SelectUniqueAppearance( entityTemplate ) );
			}
		}
	}

	ASSERT( spawnListIterator == spawnCount );

	CPartySpawnOrganizer* spawnOrganizer = m_partySpawnOrganizer.Get();
	if ( spawnOrganizer )
	{
		SEncounterSpawnPoint spawnPoint = encounter->GetSpawnPoints().GetSpawnPoint( sp );
		spawnOrganizer->PrePartySpawn( this, spawnInfos, &spawnPoint );
	}
	
	return GCommonGame->GetEntityPool()->SpawnEntities( std::move( spawnInfos ), CEntityPool::SpawnType_Encounter );
}
Bool CCreaturePartyEntry::WasCreatureLimitReached( CSpawnTreeInstance& instance, Int32& groupToUse )
{
	Int32 totalToSpawn = instance[ i_spawnCount ];
	if ( m_group != 0 )
	{
		CEntityPool* entityPool = GCommonGame->GetEntityPool();
		if ( entityPool->GetEntitySpawnedCount() + totalToSpawn > entityPool->GetMaxEntitySpawnCount() )
		{
			return true;
		}
	}

	// check if we have hit creature definition spawn limit
	for ( Uint32 i = 0, n = m_subDefinitions.Size(); i != n; ++i )
	{
		CSpawnTreeEntrySubDefinition* subdef = m_subDefinitions[ i ];

		CEncounterCreatureDefinition* def = subdef->GetCreatureDefinition( instance );
		if ( def->WasSpawnLimitReached( subdef->GetSpawnedCreaturesCount() ) )
		{
			return true;
		}
	}

	groupToUse = m_group;
	Int32 limit = CEncounterGlobalSettings::GetInstance().GetLimitForGroup( groupToUse );;
	// if there is no space in current group, check less important groups
	do 
	{		
		if( GCommonGame->GetEncounterSpawnGroup().GetGroupCount( groupToUse ) + totalToSpawn < limit )
		{
			break;
		}
		++groupToUse;
		limit = CEncounterGlobalSettings::GetInstance().GetLimitForGroup( groupToUse );
	} while( limit >=0 );

	return limit == -1;	
}

Bool CCreaturePartyEntry::AreInitializersSaving( CSpawnTreeInstance& instance ) const
{
	InitializersIterator iterator( *this, instance );

	return ISpawnTreeInitializer::AreInitializersStateSaving( iterator );
}

Bool CCreaturePartyEntry::SaveInitializers( CSpawnTreeInstance& instance, IGameSaver* writer ) const
{
	InitializersIterator iterator( *this, instance );

	return ISpawnTreeInitializer::SaveInitializersState( iterator, writer );
}

Bool CCreaturePartyEntry::LoadInitializers( CSpawnTreeInstance& instance, IGameLoader* reader ) const
{
	InitializersIterator iterator( *this, instance );

	return ISpawnTreeInitializer::LoadInitializersState( iterator, reader );
}

CCreaturePartyEntry::ELoadTemplateResult CCreaturePartyEntry::UpdateLoading( CSpawnTreeInstance& instance, SSpawnTreeUpdateSpawnContext& context )
{
	if ( instance[ i_noSpawnPointDefined ] )
	{
		return LOAD_FAILED;
	}

	CTemplateLoadBalancer* loader = GCommonGame->GetTemplateLoadBalancer();

	Bool isEverythingLoaded = true;
	Bool isSomethingFailed = false;

	for ( Uint32 i = 0, subDefinitionCount = m_subDefinitions.Size(); i != subDefinitionCount; ++i )
	{
		CSpawnTreeEntrySubDefinition* subDef		= m_subDefinitions[ i ];
		CEncounterCreatureDefinition* creatureDef	= subDef ? subDef->GetCreatureDefinition( instance ) : nullptr;
		if ( creatureDef == nullptr )
		{
			return LOAD_FAILED;
		}
		const TSoftHandle< CEntityTemplate >& templateHandle = creatureDef->GetEntityTemplate();
		CTemplateLoadRequest& request = subDef->GetTemplateLoadRequest( instance ); 
		request.Initialize( templateHandle );

		CTemplateLoadBalancer::SRequestInfo info = loader->RegisterTemplateRequest( request );
		if ( !info.m_isLoaded )
		{
			isEverythingLoaded = false;
		}
		else
		{
			if ( !CEncounterCreatureDefinition::CheckEntityTemplate( request.GetEntityTemplate() ) )
			{
				isSomethingFailed = true;
				break;
			}
		}
		if ( info.m_isInvalid )
		{
			isSomethingFailed = true;
			break;
		}
	}

	if ( isSomethingFailed )
	{
		return LOAD_FAILED;
	}
	if ( isEverythingLoaded )
	{
		return LOAD_SUCCESS;
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
		for ( Uint32 i = 0, subDefinitionCount = m_subDefinitions.Size(); i != subDefinitionCount; ++i )
		{
			CSpawnTreeEntrySubDefinition* subDef		= m_subDefinitions[ i ];
			CTemplateLoadRequest& request = subDef->GetTemplateLoadRequest( instance );
			loader->UnregisterTemplateRequest( request );
		}
		
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

	isEverythingLoaded = true;
	isSomethingFailed = false;

	for ( Uint32 i = 0, subDefinitionCount = m_subDefinitions.Size(); i != subDefinitionCount; ++i )
	{
		CSpawnTreeEntrySubDefinition* subDef		= m_subDefinitions[ i ];
		CTemplateLoadRequest& request = subDef->GetTemplateLoadRequest( instance );
		CTemplateLoadBalancer::SRequestInfo info = loader->UpdateTemplateRequest( request, loadBalancerPriority );
		if ( !info.m_isLoaded )
		{
			isEverythingLoaded = false;
		}
		else
		{
			if ( !CEncounterCreatureDefinition::CheckEntityTemplate( request.GetEntityTemplate() ) )
			{
				isSomethingFailed = true;
				break;
			}
		}
		if ( info.m_isInvalid )
		{
			isSomethingFailed = true;
			break;
		}
	}
	if ( isSomethingFailed )
	{
		return LOAD_FAILED;
	}
	if ( isEverythingLoaded )
	{
		return LOAD_SUCCESS;
	}
	return LOAD_PENDING;
}

Uint32 CCreaturePartyEntry::GetMatchingEntryHashValue( CSpawnTreeInstance& instance )
{
	Uint32 & matchingEntryHashValue = instance[ i_matchingEntryHashValue ];
	if ( matchingEntryHashValue != 0 )
	{
		return matchingEntryHashValue;
	}
	// value was not initialized yet
	for ( Int32 i = 0, subDefinitionCount = m_subDefinitions.Size(); i < subDefinitionCount; ++i )
	{
		const CSpawnTreeEntrySubDefinition *const subDef	= m_subDefinitions[ i ];
		if ( subDef )
		{
			const CName & definitionName						= subDef->GetCreatureDefinitionName();
			matchingEntryHashValue								^= definitionName.GetSerializationHash().GetValue() * subDef->GetSpawnedCreaturesCount();
		}
	}
	return matchingEntryHashValue;
}

Bool CCreaturePartyEntry::IsCreatureElligible( CSpawnTreeInstance& instance, const CEncounterCreaturePool::SCreature &creature )
{
	// if guy is not in party - bail out
	if ( (creature.m_stateFlags & CEncounterCreaturePool::SCreature::FLAG_IS_IN_PARTY) == 0 )
	{
		return false;
	}
	
	if ( (Uint32)creature.m_lastOwningEntry != (Uint32)SPAWN_TREE_INVALID_DESPAWNER_ID )
	{
		CEncounter* encounter						= instance.GetEncounter();
		CEncounter::SActiveEntry* lastOwningEntry	= encounter->FindActiveCreatureEntry( creature.m_lastOwningEntry );	
		ASSERT( lastOwningEntry );
		if ( lastOwningEntry )
		{
			CCreaturePartyEntry *const lastOwningPartyEntry = static_cast<CCreaturePartyEntry*>( lastOwningEntry->m_entry );
			if ( lastOwningPartyEntry->GetMatchingEntryHashValue( *lastOwningEntry->m_instanceBuffer ) != GetMatchingEntryHashValue( instance ) )
			{
				return false;
			}
		}
	}
	return true;
}

Bool CCreaturePartyEntry::UpdateCollectCreaturesToSteal( CSpawnTreeInstance& instance, CEncounter::SActiveEntry& entry )
{
	CEncounter* encounter = instance.GetEncounter();

	// Try 'gathering' guyz from detached boyz
	CEncounterCreaturePool& creaturePool							= encounter->GetCreaturePool();
	CEncounterCreaturePool::SCreatureList& detachedCreatures		= creaturePool.GetDetachedCreaturesList();
	const CEncounterCreaturePool::CPartiesManager& partiesManager	= creaturePool.GetPartiesManager();

	Bool isGatheringDetachedCreatures = false;
	for ( auto creatureIt = detachedCreatures.Begin(), end = detachedCreatures.End(); creatureIt != end; )
	{
		CEncounterCreaturePool::SCreature& creature = *creatureIt;
		ASSERT( creature.m_listId == 0 );
		// Possibly current creature entry might be removed from creature list during execution.
		// In this case we should progress iterator, and iterator will still point to detached list member.
		++creatureIt;

		if ( creature.m_lastOwningEntry == GetUniqueListId( instance ) )
		{
			// check if we are re-collecting our own creature
			// determine whole party
			const CEncounterCreaturePool::Party* party = partiesManager.GetParty( creature.m_actor );

			// cannot collect this creature
			if ( !party )
			{
				continue;
			}

			// steal whole party
			for ( auto it = party->Begin(), end = party->End(); it != end; ++it )
			{
				const auto& partyMember = *it;
				CActor* actor = partyMember.m_actor;
				CEncounterCreaturePool::SCreature* partyCreature = creaturePool.GetCreatureEntry( actor );
				StealCreature( instance, *partyCreature );
			}
		}
		else
		{
			if ( !creature.IsInParty() || !IsCreatureElligible( instance, creature ) )
			{
				continue;
			}

			// filter by name
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

			// determine whole party
			const CEncounterCreaturePool::Party* party = partiesManager.GetParty( creature.m_actor );

			// cannot collect this creature
			if ( !party )
			{
				continue;
			}

			Bool acceptParty = true;
			for ( auto it = party->Begin(), end = party->End(); it != end; ++it )
			{
				const auto& partyMember = *it;
				Uint32 subDefIndex = partyMember.m_partyIndex;
				if ( subDefIndex >= m_subDefinitions.Size() )
				{
					acceptParty = false;
					break;
				}
				const CSpawnTreeEntrySubDefinition* subDef = m_subDefinitions[ subDefIndex ];
				if ( subDef->GetPartyMemberName() != partyMember.m_memberName )
				{
					acceptParty = false;
					break;
				}
				CActor* actor = partyMember.m_actor;
				const CEncounterCreaturePool::SCreature* partyCreature = creaturePool.GetCreatureEntry( actor );
				if ( subDef->GetCreatureDefinitionId( instance ) != partyCreature->m_creatureDefId )
				{
					acceptParty = false;
					break;
				}
				if ( !AcceptActorByInitializers( instance, actor, subDefIndex ) )
				{
					acceptParty = false;
					break;
				}
			}
			if ( !acceptParty )
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
			delayRatio = (delayRatio + 1.f) / 2.f;															// [ 0 .. 1 ] sqr distribution around 0.5

			SSpawnTreeEntryStealCreatureState steal;
			steal.m_isDelayed = true;
			steal.m_previousOwnerEntry = creature.m_lastOwningEntry;
			steal.m_actor = creature.m_actor;
			steal.m_delayTimeout = GGame->GetEngineTime() + delayRatio * instance[ i_setup ].m_stealingDelay;

			auto& stealingInProgress = instance[ i_stealingInProgress ];

			// steal whole party
			for ( auto it = party->Begin(), end = party->End(); it != end; ++it )
			{
				const auto& partyMember = *it;
				CActor* actor = partyMember.m_actor;
				CEncounterCreaturePool::SCreature* partyCreature = creaturePool.GetCreatureEntry( actor );
				StealCreature( instance, *partyCreature );

				steal.m_subdefIndex = Uint8( partyMember.m_partyIndex );
				steal.m_actor = actor;
				stealingInProgress.PushBack( steal );
			}
		}

		++(instance[ i_numCreaturesSpawned ]);

		// TODO: support for greedy stealing
		//if ( WasSpawnLimitReachedCommon( instance ) && !instance[ i_setup ].m_greedyStealing )
		return true;
	};
	return false;
}

ISpawnTreeInitializer::EOutput CCreaturePartyEntry::ActivateInitializersOnSteal( CSpawnTreeInstance& instance, CActor* actor, SSpawnTreeEntryStealCreatureState& stealState, Bool initial )
{
	auto funInitializer =
		[ this, actor, &instance ] ( const ISpawnTreeInitializer* initializer ) -> ISpawnTreeInitializer::EOutput
	{
		if ( initializer->CallActivateWhenStealing() )
		{
			return initializer->Activate( actor, &instance, this, ISpawnTreeInitializer::EAR_Steal );
		}
		return ISpawnTreeInitializer::OUTPUT_SUCCESS;
	};

	Uint32 subdefIndex = stealState.m_subdefIndex;
	CSpawnTreeEntrySubDefinition* subDef = m_subDefinitions[ subdefIndex ];
	const auto& initializers = subDef->GetInitializers();

	ISpawnTreeInitializer::EOutput returnVal = TBaseClass::ActivateInitializersOnSteal( instance, actor, stealState, initial );

	if ( initial )
	{
		for ( Uint32 i = 0, n = initializers.Size(); i != n; ++i )
		{
			switch ( funInitializer( initializers[ i ] ) )
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

				stealState.m_postponedInitializers.PushBack( 0x8000 | i );
				break;
			}
		}
	}
	else
	{
		for ( Int32 postIdx = stealState.m_postponedInitializers.Size() - 1; postIdx >= 0; --postIdx  )
		{
			Uint32 initializerIdx = stealState.m_postponedInitializers[ postIdx ];
			if ( (initializerIdx & 0x8000) == 0 )
			{
				continue;
			}
			switch( funInitializer( initializers[ initializerIdx & 0x7fff ] ) )
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

	if( returnVal == ISpawnTreeInitializer::OUTPUT_SUCCESS )
	{
		CReactionSceneActorComponent* rsActor = actor->FindComponent< CReactionSceneActorComponent >();
		if( rsActor && m_blockChats )
		{
			rsActor->LockChats();
		}

		if( m_synchronizeWork )
		{
			actor->SignalGameplayEvent( CNAME( AI_EnablePartyWorkSynchronization ) );
		}
	}	

	return returnVal;
}

Int32 CCreaturePartyEntry::GetCreatureDefinitionsCount() const
{
	return m_subDefinitions.Size();
}
Int16 CCreaturePartyEntry::GetCreatureDefinitionId( CSpawnTreeInstance& instance, Int32 i ) const
{
	return m_subDefinitions[ i ]->GetCreatureDefinitionId( instance );
}
Bool CCreaturePartyEntry::UsesCreatureDefinition( CName creatureDefinition ) const
{
	for ( auto it = m_subDefinitions.Begin(), end = m_subDefinitions.End(); it != end; ++it )
	{
		if ( ( *it )->GetCreatureDefinitionName() == creatureDefinition )
		{
			return true;
		}
	}
	return false;
}
Bool CCreaturePartyEntry::OnSpawnJobIsDoneInternal( CSpawnTreeInstance& instance, IJobEntitySpawn* spawnJob, CEncounter::SActiveEntry& encounterEntryData )
{
	Uint32 n = spawnJob->GetEntitiesCount();

	CEncounter* encounter = instance.GetEncounter();

	CEncounterCreaturePool::Party party;
	party.Reserve( n );

	SpawnInfoIterator subDefIterator( this );
	// create entries
	for ( Uint32 i = 0; i != n; ++i )
	{
		subDefIterator.FetchNext();
		CEntity* entity = spawnJob->GetSpawnedEntity( i );
		if ( entity )
		{
			CSpawnTreeEntrySubDefinition* subDef = subDefIterator.GetSubdefinition();
			Uint32 subDefIndex = subDefIterator.GetSubdefinitionIdx();
			ASSERT( entity->IsA< CActor >(), TXT("Pre spawning tests should make double check that spawn job is indeed spawning ACTORS") );
			CActor* actor = static_cast< CActor* >( entity );
			Int16 creatureDefId = subDef->GetCreatureDefinitionId( instance );

			CEncounter::SCreatureData* creature = encounter->RegisterSpawnedCreature( actor, encounterEntryData, creatureDefId );

			ISpawnTreeInitializer::EOutput initializerResult = ActivateInitializers( instance, actor, spawnJob->IsSpawnedFromPool( i ) ? EST_PoolSpawn : EST_NormalSpawn, subDefIndex );
			ASSERT( initializerResult == ISpawnTreeInitializer::OUTPUT_SUCCESS, TXT( "Something is wrong with encouter initializers, please check them" ) );

			CEncounterCreaturePool::PartyMember m;
			m.m_actor = static_cast< CActor* >( entity );
			m.m_memberName = subDef->GetPartyMemberName();
			m.m_partyIndex = CEncounterCreaturePool::CPartiesManager::PartyIndex( subDefIndex );
			party.PushBack( m );
		}
	}
	// create party
	if ( party.Empty() )
	{
		return false;
	}
	++(instance[ i_numCreaturesSpawned ]);

	const CEncounterCreaturePool::Party* createdParty = encounter->GetCreaturePool().GetPartiesManager().CreateParty( encounter->GetCreaturePool(), Move( party ) );

	CPartySpawnOrganizer* spawnOrganizer = m_partySpawnOrganizer.Get();
	if ( spawnOrganizer )
	{
		spawnOrganizer->PostPartySpawn( *createdParty );
	}
		
	return true;
}
void CCreaturePartyEntry::OnCreatureRemoval( CSpawnTreeInstance& instance, CEncounterCreaturePool::SCreature& creature, ESpawnTreeCreatureRemovalReason removalReason, Bool isCreatureDetached  )
{
	CEncounter* encounter = instance.GetEncounter();
	CActor* actor = creature.m_actor;
	CEncounterCreaturePool& creaturePool = encounter->GetCreaturePool();
	const auto& partiesManager = creaturePool.GetPartiesManager();
	const CEncounterCreaturePool::Party* party = partiesManager.GetParty( actor );

	if ( !isCreatureDetached )
	{
		if ( party && party->Size() == 1 )			// TODO: Thats a hacky logic - subject for a change
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
	}

	
	if ( creature.m_stateFlags & CEncounterCreaturePool::SCreature::FLAG_WAS_ACTIVATED_BY_ENTRY )
	{
		const auto& initializers = GetRuntimeInitializers( instance );
		for ( auto it = initializers.Begin(), end = initializers.End(); it != end; ++it )
		{
			ISpawnTreeInitializer* initializer = it->m_initializer;
			initializer->OnCreatureRemoval( *it->m_instance, actor, removalReason, this );
		}
		if ( party )
		{
			Int32 partyIndex = partiesManager.GetPartyIndex( actor, party );
			if ( partyIndex >= 0 )
			{
				const auto& defInitializers = m_subDefinitions[ partyIndex ]->GetInitializers();
				for ( auto it = defInitializers.Begin(), end = defInitializers.End(); it != end; ++it )
				{
					ISpawnTreeInitializer* initializer = *it;
					initializer->OnCreatureRemoval( instance, actor, removalReason, this );
				}
			}
		}
	}
	

	CReactionSceneActorComponent* rsActor = actor->FindComponent< CReactionSceneActorComponent >();
	if( rsActor && m_blockChats )
	{
		rsActor->UnlockChats();
	}	
}
Int32 CCreaturePartyEntry::AcceptActorByInitializers( CSpawnTreeInstance& instance, CActor* actor, Int32 definitionIndex )
{
	CSpawnTreeEntrySubDefinition* subDefinition = m_subDefinitions[ definitionIndex ];
	const auto& initializers = subDefinition->GetInitializers();
	Bool accepted = true;
	for ( auto it = initializers.Begin(), end = initializers.End(); it != end; ++it )
	{
		ISpawnTreeInitializer* initializer = *it;
		if ( initializer->Accept( actor  ) == false )
		{
			accepted = false;
		}
	}
	if ( accepted == false )
	{
		return false;
	}
	return TBaseClass::AcceptActorByInitializers( instance, actor, definitionIndex );
}
ISpawnTreeInitializer::EOutput CCreaturePartyEntry::ActivateInitializers( CSpawnTreeInstance& instance, CActor* actor, ESpawnType spawnType, Int32 definitionIndex )
{
	ISpawnTreeInitializer::EOutput returnVal = ISpawnTreeInitializer::OUTPUT_SUCCESS;
	CSpawnTreeEntrySubDefinition* subDefinition = m_subDefinitions[ definitionIndex ];
	const auto& initializers = subDefinition->GetInitializers();
	for ( auto it = initializers.Begin(), end = initializers.End(); it != end; ++it )
	{
		ISpawnTreeInitializer* initializer = *it;
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
			
			switch ( initializer->Activate( actor, &instance, this, ISpawnTreeInitializer::ActivationReason( spawnType ) ) )
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
	if ( returnVal != ISpawnTreeInitializer::OUTPUT_SUCCESS )
	{
		return returnVal;	
	}	

	returnVal = TBaseClass::ActivateInitializers( instance, actor, spawnType, definitionIndex);

	if( returnVal == ISpawnTreeInitializer::OUTPUT_SUCCESS )
	{
		CReactionSceneActorComponent* rsActor = actor->FindComponent< CReactionSceneActorComponent >();
		if( rsActor && m_blockChats )
		{
			rsActor->LockChats();
		}

		if( m_synchronizeWork )
		{
			actor->SignalGameplayEvent( CNAME( AI_EnablePartyWorkSynchronization ) );
		}		
	}

	return returnVal;
}

void CCreaturePartyEntry::DeactivateInitializers( CSpawnTreeInstance& instance, CActor* actor, Int32 definitionIndex )
{
	if ( definitionIndex == -1 )
	{
		CEncounter* encounter = instance.GetEncounter();
		CEncounterCreaturePool& creaturePool = encounter->GetCreaturePool();
		definitionIndex = creaturePool.GetPartiesManager().GetPartyIndex( actor );
		if ( definitionIndex == -1 )
		{
			return TBaseClass::DeactivateInitializers( instance, actor );
		}
	}


	CSpawnTreeEntrySubDefinition* subDefinition = m_subDefinitions[ definitionIndex ];
	const auto& initializers					= subDefinition->GetInitializers();
	for ( auto it = initializers.Begin(), end	= initializers.End(); it != end; ++it )
	{
		ISpawnTreeInitializer* initializer = *it;
		initializer->Deactivate( actor, &instance, this );
	}	

	actor->SignalGameplayEvent( CNAME( AI_DisablePartyWorkSynchronization ) );

	return TBaseClass::DeactivateInitializers( instance, actor );
}

void CCreaturePartyEntry::OnFullRespawn( CSpawnTreeInstance& instance ) const
{
	for ( Uint32 i = 0, n = m_subDefinitions.Size(); i != n; ++i )
	{
		CSpawnTreeEntrySubDefinition* subDefinition = m_subDefinitions[ i ];
		if ( subDefinition )
		{
			subDefinition->OnFullRespawn( instance );
		}
	}
	TBaseClass::OnFullRespawn( instance );
}

void CCreaturePartyEntry::Deactivate( CSpawnTreeInstance& instance )
{
	for ( Uint32 i = 0, n = m_subDefinitions.Size(); i != n; ++i )
	{
		CSpawnTreeEntrySubDefinition* subDefinition = m_subDefinitions[ i ];
		if ( subDefinition )
		{
			subDefinition->Deactivate( instance );
			const auto& initializers = subDefinition->GetInitializers();
			for ( auto it = initializers.Begin(), end = initializers.End(); it != end; ++it )
			{
				ISpawnTreeInitializer* initializer = (*it);
				initializer->OnSpawnTreeDeactivation( instance );
			}

		}
	}

	TBaseClass::Deactivate( instance );
}

Bool CCreaturePartyEntry::CanSpawnInitializerClass( ISpawnTreeInitializer* defObject ) const
{
	if ( !TBaseClass::CanSpawnInitializerClass( defObject ) )
	{
		return false;
	}
	for ( Uint32 i = 0, n = m_subDefinitions.Size(); i != n; ++i )
	{
		CSpawnTreeEntrySubDefinition* subDefinition = m_subDefinitions[ i ];
		if ( subDefinition )
		{
			const auto& initializers = subDefinition->GetInitializers();
			for ( auto it = initializers.Begin(), end = initializers.End(); it != end; ++it )
			{
				ISpawnTreeInitializer* initializer = (*it);
				if ( initializer && defObject->IsConflicting( initializer ) )
				{
					return false;
				}
			}

		}
	}
	return true;
}

void CCreaturePartyEntry::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler ); 

	for ( Uint32 i = 0, n = m_subDefinitions.Size(); i != n; ++i )
	{
		CSpawnTreeEntrySubDefinition* subDefinition = m_subDefinitions[ i ];
		if ( subDefinition )
		{
			subDefinition->OnBuildDataLayout( compiler );
		}
	}
	compiler << i_matchingEntryHashValue;
	compiler << i_spawnCount;
}

void CCreaturePartyEntry::OnInitData( CSpawnTreeInstance& instance, CSpawnTreeInitializationContext& context )
{
	TBaseClass::OnInitData( instance, context );

	Uint16 spawnCount = 0;

	for ( Uint32 i = 0, n = m_subDefinitions.Size(); i != n; ++i )
	{
		CSpawnTreeEntrySubDefinition* subDefinition = m_subDefinitions[ i ];
		if ( subDefinition )
		{
			subDefinition->OnInitData( instance, context );
			spawnCount += m_subDefinitions[ i ]->GetSpawnedCreaturesCount();
		}
	}

	instance[ i_spawnCount ] = spawnCount;
	instance[ i_matchingEntryHashValue ] = 0;
}

void CCreaturePartyEntry::OnDeinitData( CSpawnTreeInstance& instance ) 
{
	TBaseClass::OnDeinitData( instance );

	for ( Uint32 i = 0, n = m_subDefinitions.Size(); i != n; ++i )
	{
		CSpawnTreeEntrySubDefinition* subDefinition = m_subDefinitions[ i ];
		if ( subDefinition )
		{
			subDefinition->OnDeinitData( instance );
		}
	}
}

Bool CCreaturePartyEntry::GenerateIdsRecursively()
{
	Bool result = TBaseClass::GenerateIdsRecursively();

	for ( Uint32 i = 0, n = m_subDefinitions.Size(); i != n; ++i )
	{
		if ( CSpawnTreeEntrySubDefinition* const subDefinition = m_subDefinitions[ i ] )
		{
			result |= subDefinition->GenerateIdsRecursively();
		}
	}
	return result;
}

Bool CCreaturePartyEntry::UpdateChildrenOrder()
{
	Bool b = TBaseClass::UpdateChildrenOrder();

	b = DefListOperations::UpdateChildrenOrder( m_subDefinitions ) || b;

	return b;
}
String CCreaturePartyEntry::GetBlockCaption() const
{
	return TXT("Party");
}

String CCreaturePartyEntry::GetBlockDebugCaption( const CSpawnTreeInstance& instanceBuffer ) const
{
	String str;
	if ( instanceBuffer[ i_noSpawnPointDefined ] )
	{
		// Error overwriting string
		str = String::Printf( TXT("Party [No SP!]" ) );
	}
	else if ( IsActive( instanceBuffer ) )
	{
		str = String::Printf( TXT("Party (%d of %d)"), instanceBuffer[ i_numCreaturesSpawned ] - instanceBuffer[ i_numCreatureDead ], GetNumCreaturesToSpawn( instanceBuffer ) );
	}
	else
	{
		str = TXT("Party");
	}

	return str;
}
String CCreaturePartyEntry::GetEditorFriendlyName() const
{
	return TXT("Party");
}
Bool CCreaturePartyEntry::CanSpawnChildClass( const CClass* classId, ESpawnTreeType spawnTreeType ) const
{
	if ( classId->IsA< CSpawnTreeEntrySubDefinition >() )
	{
		return true;
	}
	if ( !TBaseClass::CanSpawnChildClass( classId, spawnTreeType ) )
	{
		return false;
	}
	return true;
}
void CCreaturePartyEntry::AddChild( IEdSpawnTreeNode* node )
{
	CClass* nodeClass = node->AsCObject()->GetClass();
	if ( nodeClass->IsA< ISpawnTreeInitializer >() )
	{
		TBaseClass::AddChild( node );
	}
	else if ( nodeClass->IsA< CSpawnTreeEntrySubDefinition >() )
	{
		DefListOperations::AddChild( m_subDefinitions, node );
	}
}
void CCreaturePartyEntry::RemoveChild( IEdSpawnTreeNode* node )
{
	if ( !node || node->AsCObject()->IsA< ISpawnTreeInitializer >() )
	{
		TBaseClass::RemoveChild( node );
	}
	else if ( node->AsCObject()->IsA< CSpawnTreeEntrySubDefinition >() )
	{
		DefListOperations::RemoveChild( m_subDefinitions, node );
	}
}

IEdSpawnTreeNode* CCreaturePartyEntry::GetChild( Int32 index ) const
{
	Int32 prev = TBaseClass::GetNumChildren();
	if ( index < prev )
	{
		return TBaseClass::GetChild( index );
	}
	index -= prev;
	return m_subDefinitions[ index ];
}
Int32 CCreaturePartyEntry::GetNumChildren() const
{
	return TBaseClass::GetNumChildren() + m_subDefinitions.Size();
}

void CCreaturePartyEntry::GetRootClassForChildren( TDynArray< CClass* >& rootClasses, ESpawnTreeType spawnTreeType ) const
{
	TBaseClass::GetRootClassForChildren( rootClasses, spawnTreeType );

	rootClasses.PushBack( CSpawnTreeEntrySubDefinition::GetStaticClass() );
}

CSpawnTreeEntrySubDefinition*	CCreaturePartyEntry::AddPartyMember( bool inEditor )
{
	CSpawnTreeEntrySubDefinition* newNode = CreateObject< CSpawnTreeEntrySubDefinition >( this );
	if( inEditor )
	{
		static const Int32 GRAPH_POS_CHANGE = 2;

#ifndef NO_EDITOR
		newNode->OnCreatedInEditor();
#endif 
#ifndef NO_EDITOR_GRAPH_SUPPORT
		newNode->SetGraphPosition( GetGraphPosX() +  GRAPH_POS_CHANGE, GetGraphPosY() + GRAPH_POS_CHANGE );
#endif
	}
	newNode->GenerateId();
	AddChild( newNode );

	return newNode;
}

void CCreaturePartyEntry::funcAddPartyMember( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, inEditor, false );
	FINISH_PARAMETERS;

	THandle< CSpawnTreeEntrySubDefinition > addedMembreHandle =  AddPartyMember( inEditor );

	RETURN_HANDLE( CSpawnTreeEntrySubDefinition, addedMembreHandle );
}

void CCreaturePartyEntry::GatherBudgetingStats(ICreatureDefinitionContainer* container, SBudgetingStats& stats)
{
	for ( Uint32 i = 0; i < m_subDefinitions.Size(); ++i )
	{
		m_subDefinitions[i]->GatherBudgetingStats( container, stats );	
	}
}

void CSpawnTreeEntrySubDefinition::GatherBudgetingStats(ICreatureDefinitionContainer* container, SBudgetingStats& stats)
{
	CEncounterCreatureDefinition* creatureDefinition = container->GetCreatureDefinition( m_creatureDefinition );

	if( creatureDefinition != nullptr )
	{
		SBudgetingStats::TemplateData budgetData;

		budgetData.m_template = creatureDefinition->GetEntityTemplate().Get();

		TDynArray< CEntityAppearance > appearancesList =  budgetData.m_template->GetAppearances();
		TDynArray< const CEntityAppearance* > appList;
		budgetData.m_template->GetAllEnabledAppearances( appList );
		budgetData.m_usedApperances = appList;

		budgetData.m_maxSpawnLimit = m_creatureCount;
		stats.m_usedTemplates.PushBack( budgetData );
	}
}