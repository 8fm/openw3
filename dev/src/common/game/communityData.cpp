/**
 *  Copyright © 2009 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "communityData.h"
#include "storyPhaseLocation.h"
#include "communityUtility.h"
#include "spawnStrategy.h"
#include "communitySystem.h"
#include "../game/wayPointComponent.h"
#include "spawnTreeDespawnInitializer.h"
#include "../core/factory.h"
#include "../core/gameSave.h"
#include "../engine/tagManager.h"

IMPLEMENT_ENGINE_CLASS( CCommunity );
IMPLEMENT_ENGINE_CLASS( CCommunitySpawnStrategy );
IMPLEMENT_ENGINE_CLASS( CCommunityInitializers );
IMPLEMENT_ENGINE_CLASS( CSEntitiesEntry );
IMPLEMENT_ENGINE_CLASS( CSStoryPhaseSpawnTimetableEntry );
IMPLEMENT_ENGINE_CLASS( CSStoryPhaseEntry );
IMPLEMENT_ENGINE_CLASS( CSTableEntry );
IMPLEMENT_ENGINE_CLASS( CSStoryPhaseTimetableACategoriesEntry );
IMPLEMENT_ENGINE_CLASS( CSLayerName );
IMPLEMENT_ENGINE_CLASS( CSStoryPhaseTimetableActionEntry );
IMPLEMENT_ENGINE_CLASS( CSStoryPhaseTimetableACategoriesTimetableEntry );
IMPLEMENT_ENGINE_CLASS( CSStoryPhaseTimetableEntry );
IMPLEMENT_ENGINE_CLASS( CSSceneTimetableScenesEntry );
IMPLEMENT_ENGINE_CLASS( CSSceneTimetableEntry );
IMPLEMENT_ENGINE_CLASS( CSSceneTableEntry );
IMPLEMENT_ENGINE_CLASS( CSSpawnType );
IMPLEMENT_ENGINE_CLASS( CSStoryPhaseNames );
IMPLEMENT_RTTI_ENUM( ECommMapPinType );
IMPLEMENT_RTTI_ENUM( ECommunitySpawnsetType );

RED_DEFINE_STATIC_NAME( eventType );

//////////////////////////////////////////////////////////////////////////

CCommunity::CCommunity()
	: m_isActive( false )
{}

void CCommunity::CacheInternalData()
{
	for ( Uint32 i=0; i<m_storyPhaseTimetable.Size(); ++i )
	{
		m_storyPhaseTimetable[i].CacheLayer();
	}
}

void CCommunity::ResetInternalData()
{
	for ( Uint32 i=0; i<m_storyPhaseTimetable.Size(); ++i )
	{
		m_storyPhaseTimetable[i].ResetLayer();
	}
}


void CCommunity::ActivatePhase( const CName& phaseName )
{
	if ( !GCommonGame || !GCommonGame->GetSystem< CCommunitySystem >() )
	{
		return;
	}

	CCommunitySystem* cs = GCommonGame->GetSystem< CCommunitySystem >();
	cs->OnCommunityActivated( this );

	m_isActive = true;
	m_activePhase = phaseName;
	for ( TDynArray< CSTableEntry >::iterator it = m_communityTable.Begin();
		it != m_communityTable.End(); ++it )
	{		
		it->ActivatePhase( *cs, *this, m_activePhase );	
	}
}

void CCommunity::Deactivate()
{
	if ( !GCommonGame || !GCommonGame->GetSystem< CCommunitySystem >() )
	{
		return;
	}

	CCommunitySystem* cs = GCommonGame->GetSystem< CCommunitySystem >();
	cs->OnCommunityDeactivated( this );

	m_isActive = false;
	m_activePhase = CName::NONE;
	for ( TDynArray< CSTableEntry >::iterator it = m_communityTable.Begin();
		it != m_communityTable.End(); ++it )
	{
		it->ResetActivePhase( *cs, *this );
	}
}

void CCommunity::GetPhaseNames( THashSet< CName >& outNames ) const
{
	for ( TDynArray< CSTableEntry >::const_iterator it = m_communityTable.Begin(); 
		it != m_communityTable.End(); ++it )
	{
		it->GetPhaseNames( outNames );
	}
}

const CSStoryPhaseTimetableEntry* CCommunity::GetTimetable( const CName& timetableName ) const
{
	for ( TDynArray< CSStoryPhaseTimetableEntry >::const_iterator storyPhaseTimetabEntry = m_storyPhaseTimetable.Begin();
		storyPhaseTimetabEntry != m_storyPhaseTimetable.End();
		++storyPhaseTimetabEntry )
	{
		if ( storyPhaseTimetabEntry->m_name == timetableName )
		{
			return &*storyPhaseTimetabEntry;
		}
	}
	return NULL;
}

const CSSceneTableEntry* CCommunity::GetScene( const CName& phaseName ) const
{
	for ( TDynArray< CSSceneTableEntry >::const_iterator it = m_sceneTable.Begin();
		it != m_sceneTable.End(); ++it )
	{
		if ( ( phaseName == CName::NONE && it->m_storyPhaseName.m_tags.Empty() ) ||
			 ( phaseName != CName::NONE && it->m_storyPhaseName.m_tags.HasTag( phaseName ) ) )
		{
			return &*it;
		}
	}
	return NULL;
}

void CCommunity::GetDebugStatus( Uint32 commIdx, ICommunityDebugPage& debugPage ) const
{
	Uint32 idx = 0;
	String commIdxStr = ToString( commIdx );
	for ( TDynArray< CSTableEntry >::const_iterator entryIt = m_communityTable.Begin();
		entryIt != m_communityTable.End(); ++entryIt )
	{
		const CSTableEntry& entry = *entryIt;
		if ( !entry.IsActive() )
		{
			continue;
		}

		const CSStoryPhaseEntry* activePhase = entry.GetActivePhase();
		ASSERT( activePhase );
		if ( !activePhase )
		{
			continue;
		}

		String msg = commIdxStr + TXT( "." ) + ToString( idx++ ) + TXT( ".) " ) + GetFriendlyName()
			+ TXT(" :: ") + entry.m_comment + TXT(" ") 
			+ entry.m_entryID + TXT(" : ") 
			+ activePhase->m_comment + TXT(" ")
			+ activePhase->m_storyPhaseName.m_tags.ToString() + TXT(" ")
			+ activePhase->m_timetableName.AsString().AsChar();
		debugPage.AddText( msg, Color::WHITE );
	}
}

void CCommunity::SaveState( IGameSaver* saver )
{
	CGameSaverBlock block( saver, CNAME(communityData) );

	// Save community general data
	saver->WriteValue( CNAME( isActive ), m_isActive );
	saver->WriteValue( CNAME( activePhase ), m_activePhase );

	// Save tables
	const Uint32 numTables = m_communityTable.Size();
	saver->WriteValue( CNAME(numCommunityTables), numTables	);
	for ( Uint32 i=0; i<numTables; ++i )
	{
		CGameSaverBlock block( saver, CNAME(tableEntry) );

		// Save GUID
		CSTableEntry& tableEntry = m_communityTable[i];
		saver->WriteValue( CNAME(GUID), tableEntry.GetGUID() );

		// Save table
		tableEntry.SaveState( saver );
	}
}

void CCommunity::RestoreState( IGameLoader* reader )
{
	ResetInternalData();

	CGameSaverBlock block( reader, CNAME(communityData) );

	// Restore data
	reader->ReadValue( CNAME( isActive ), m_isActive );
	reader->ReadValue( CNAME( activePhase ), m_activePhase );

	// Restore state
	if ( m_isActive )
	{
		ActivatePhase( m_activePhase );
	}
	else
	{
		Deactivate();
	}

	// Load community tables
	const Uint32 numCommunityTables = reader->ReadValue< Uint32 >( CNAME(numCommunityTables) );
	for ( Uint32 i=0; i<numCommunityTables; ++i )
	{
		CGameSaverBlock block( reader, CNAME(tableEntry) );

		// Read GUID
		CGUID tableGUID = CGUID::ZERO;
		reader->ReadValue( CNAME(GUID), tableGUID );

		// Find table entry
		for ( Uint32 j=0; j<m_communityTable.Size(); ++j )
		{
			CSTableEntry& tableEntry = m_communityTable[j];
			if ( tableEntry.GetGUID() == tableGUID )
			{
				tableEntry.RestoreState( reader );
				break;
			}
		}
	}
}

void CCommunity::UpdateGUIDS()
{
	for ( TDynArray< CSTableEntry >::iterator tableEntryIt = m_communityTable.Begin();
		tableEntryIt != m_communityTable.End(); ++tableEntryIt )
	{
		CSTableEntry& tableEntry = *tableEntryIt;
		tableEntry.m_guid = CGUID::Create();

		for ( TDynArray< CSEntitiesEntry >::iterator entityEntryIt = tableEntry.m_entities.Begin();
			entityEntryIt != tableEntry.m_entities.End(); ++entityEntryIt )
		{
			CSEntitiesEntry& entity = *entityEntryIt;
			entity.m_guid = CGUID::Create();
		}

		for ( TDynArray< CSStoryPhaseEntry >::iterator phaseEntryIt = tableEntry.m_storyPhases.Begin();
			phaseEntryIt != tableEntry.m_storyPhases.End(); ++phaseEntryIt )
		{
			CSStoryPhaseEntry& phase = *phaseEntryIt;
			phase.m_guid = CGUID::Create();
		}
	}
}

Bool CCommunity::CheckGUIDs()
{
	TDynArray< CGUID > allGUIDs;

	for ( TDynArray< CSTableEntry >::iterator tableEntryIt = m_communityTable.Begin();
		tableEntryIt != m_communityTable.End(); ++tableEntryIt )
	{
		CSTableEntry& tableEntry = *tableEntryIt;
		if( tableEntry.m_guid.IsZero() || allGUIDs.Exist( tableEntry.m_guid ) )
		{
			return false;
		}

		allGUIDs.PushBack( tableEntry.m_guid );

		for ( TDynArray< CSEntitiesEntry >::iterator entityEntryIt = tableEntry.m_entities.Begin();
			entityEntryIt != tableEntry.m_entities.End(); ++entityEntryIt )
		{
			CSEntitiesEntry& entity = *entityEntryIt;
			if( entity.m_guid.IsZero() || allGUIDs.Exist( entity.m_guid ) )
			{
				return false;
			}

			allGUIDs.PushBack( entity.m_guid );
		}

		for ( TDynArray< CSStoryPhaseEntry >::iterator phaseEntryIt = tableEntry.m_storyPhases.Begin();
			phaseEntryIt != tableEntry.m_storyPhases.End(); ++phaseEntryIt )
		{
			CSStoryPhaseEntry& phase = *phaseEntryIt;
			if( phase.m_guid.IsZero() || allGUIDs.Exist( phase.m_guid ) )
			{
				return false;
			}

			allGUIDs.PushBack( phase.m_guid );
		}
	}

	return true;
}

void CCommunity::OnSerialize( IFile &file )
{
	// Pass to base class
	TBaseClass::OnSerialize( file );
}

void CCommunity::ResetRTData()
{
	// Reset runtime data
	for ( Uint32 i=0; i<m_communityTable.Size(); ++i )
	{
		// Reset active phase
		CSTableEntry& tableEntry = m_communityTable[i];
		tableEntry.ResetRTData();
	}
}

void CCommunity::OnPostLoad()
{
	// Pass to base class
	TBaseClass::OnPostLoad();

	// Reset runtime data - just in case
	ResetRTData();	
}

Bool CCommunity::InlinedDuplicateTest()
{
	Bool res = true;
	TDynArray< CObject* > objects;

	for( Uint32 i = 0; i < m_communityTable.Size(); i++ )
	{
		const CSTableEntry& entry = m_communityTable[i];
		for( Uint32 j = 0; j < entry.m_entities.Size(); j++ )
		{
			if( entry.m_entities[j].m_initializers )
			{
				if( !objects.PushBackUnique( entry.m_entities[j].m_initializers ) )
				{
					WARN_GAME( String::Printf( TXT("Duplicated initializers %u:%u" ), i, j ).AsChar() );
					res = false;
				}
			}
		}
	}

	return res;
}

//////////////////////////////////////////////////////////////////////////

void CSLayerName::CacheLayer() const
{
	if( !m_cachedLayer.Get() )
	{
		m_cachedLayer = CCommunityUtility::GetCachedLayerInfo( m_layerName );
	}	
	//if ( !m_cachedLayer )
	//{
	//	LOG_GAME( TXT("Community layer '%ls' was not found!"), m_layerName.AsString().AsChar() );
	//}
}

void CSLayerName::ResetLayer() const
{
	m_cachedLayer = nullptr;
}
//////////////////////////////////////////////////////////////////////////

class CCommunityFactory : public IFactory
{
	DECLARE_ENGINE_CLASS( CCommunityFactory, IFactory, 0 );

public:
	CCommunityFactory()
	{
		m_resourceClass = ClassID< CCommunity >();
	}

	virtual CResource* DoCreate( const FactoryOptions& options );
};

BEGIN_CLASS_RTTI( CCommunityFactory )
	PARENT_CLASS( IFactory )	
END_CLASS_RTTI()

IMPLEMENT_ENGINE_CLASS( CCommunityFactory );

CResource* CCommunityFactory::DoCreate( const FactoryOptions& options )
{
	CCommunity *community = ::CreateObject< CCommunity >( options.m_parentObject );
	return community;
}

//////////////////////////////////////////////////////////////////////////

CSTableEntry::CSTableEntry()
	: m_activePhase( NULL )
	//, m_isHiddenSpawn( false )
{
	m_guid = CGUID::Create();
}

Bool CSTableEntry::IsActivePhaseValidWorld( CCommunity* owner )
{	
	if( !m_activePhase )
		return false;
	
	CName timetableName = m_activePhase->m_timetableName;

	if( !timetableName )
		return true;

	
	TDynArray< CSStoryPhaseTimetableEntry >& timetable = owner->GetStoryPhaseTimetable();	

	for ( Uint32 i=0; i<timetable.Size(); ++i )
	{		
		if( timetableName == timetable[i].m_name )
		{
			TDynArray< CSStoryPhaseTimetableACategoriesTimetableEntry > actionCategies = timetable[i].m_actionCategies;

			for( Int32 j=0; j<actionCategies.SizeInt(); ++j )
			{
				TDynArray< CSStoryPhaseTimetableActionEntry > actions = actionCategies[ j ].m_actions;

				for( Int32 k=0; k < actions.SizeInt(); ++k )
				{
					CLayerInfo* layer = actions[ k ].m_layerName.GetCachedLayer();
					if( layer == nullptr )
					{
						return false;
					}
				}
			}	
		}
	}
	
	return true;
	
}

void CSTableEntry::ActivatePhase( CCommunitySystem& cs, 
								 CCommunity& owner, 
								 const CName& phaseName )
{
	CSStoryPhaseEntry* phaseToDeactivate = m_activePhase;

	// activate the new phase
	m_activePhase = FindPhaseEntry( phaseName );


	// deactivate the old phase, providing it was different from the new one
	if ( phaseToDeactivate != m_activePhase )
	{
		if ( m_activePhase )
		{
			cs.ActivateStoryPhase( &owner, this, m_activePhase );
		}

		if ( phaseToDeactivate )
		{
			cs.DeactivateStoryPhase( &owner, this, phaseToDeactivate );
		}
	}
}

void  CSTableEntry::ResetActivePhase( CCommunitySystem& cs, CCommunity& owner ) 
{ 
	if ( m_activePhase )
	{
		cs.DeactivateStoryPhase( &owner, this, m_activePhase );
	}

	m_activePhase = NULL; 
}


Bool CSTableEntry::GetRandomWeightsEntityEntry( CSEntitiesEntry **communityEntitiesEntry /* out */ )
{
	PC_SCOPE( GetRandomEntityEntry );

	Float entityTemplatesWeightSum = 0;
	// TODO: Calculate this only once and cache it
	for ( TDynArray< CSEntitiesEntry >::const_iterator entitiesEntry = m_entities.Begin();
		entitiesEntry != m_entities.End();
		++entitiesEntry )
	{
		entityTemplatesWeightSum += entitiesEntry->m_weight;
	}

	Float randomEntityTemplateValue = GEngine->GetRandomNumberGenerator().Get< Float >( entityTemplatesWeightSum );

	for ( TDynArray< CSEntitiesEntry >::iterator entitiesEntry = m_entities.Begin();
		entitiesEntry != m_entities.End();
		++entitiesEntry )
	{
		if ( entitiesEntry->m_weight == 0 ) continue;

		randomEntityTemplateValue -= entitiesEntry->m_weight;
		if ( randomEntityTemplateValue <= 0 )
		{
			*communityEntitiesEntry = &*entitiesEntry;
			return true;
		}

	}

	return false;
}

void CSTableEntry::GetPhaseNames( THashSet< CName >& outNames ) const
{
	for ( TDynArray< CSStoryPhaseEntry >::const_iterator it = m_storyPhases.Begin(); it != m_storyPhases.End(); ++it )
	{
		if ( !it->m_storyPhaseName.m_tags.Empty() )
		{
			outNames.Insert( it->m_storyPhaseName.m_tags.GetTags()[ 0 ] );
		}
	}
}

void CSTableEntry::SaveState( IGameSaver* saver )
{
	CGameSaverBlock block( saver, CNAME(tableEntryData) );

	// Count phases to save
	const Uint32 numPhases = m_storyPhases.Size();
	saver->WriteValue( CNAME(numPhases), numPhases );

	// Save phases
	for ( Uint32 i=0; i<m_storyPhases.Size(); ++i )
	{
		CGameSaverBlock block( saver, CNAME(phaseEntry) );

		// Save GUID
		CSStoryPhaseEntry& entry = m_storyPhases[i];
		saver->WriteValue( CNAME(GUID), entry.GetGUID() );

		// Save phase runtime data
		entry.SaveState( saver );
	}
}

void CSTableEntry::RestoreState( IGameLoader* loader )
{
	CGameSaverBlock block( loader, CNAME(tableEntryData) );

	// Load number of phases to read
	const Uint32 numPhases = loader->ReadValue< Uint32 >( CNAME(numPhases) );
	for ( Uint32 i=0; i<numPhases; ++i )
	{
		CGameSaverBlock block( loader, CNAME(phaseEntry) );

		// Load GUID
		CGUID phaseGUID = CGUID::ZERO;		
		loader->ReadValue< CGUID >( CNAME(GUID), phaseGUID );

		// Find phase with that GUID
		for ( Uint32 j=0; j<m_storyPhases.Size(); ++j )
		{
			CSStoryPhaseEntry& entry = m_storyPhases[j];
			if ( entry.GetGUID() == phaseGUID )
			{
				entry.RestoreState( loader );
				break;
			}
		}
	}
}

void CSTableEntry::ResetRTData()
{
	// No phase
	m_activePhase = NULL;

	// Reset runtime data
	for ( Uint32 j=0; j<m_storyPhases.Size(); ++j )
	{
		CSStoryPhaseEntry& entry = m_storyPhases[j];
		entry.ResetRTData();
	}
}

CSTableEntry& CSTableEntry::operator=( const CSTableEntry& other )
{
	m_comment			= other.m_comment;
	m_entryID			= other.m_entryID;

	m_entities.Clear();
	m_entities.Grow( other.m_entities.Size() );
	for( Uint32 i=0; i<m_entities.Size(); i++ )
	{
		m_entities[i] = other.m_entities[i];
	}

	m_alwaysSpawned		= other.m_alwaysSpawned;	
	m_storyPhases		= other.m_storyPhases;
	m_activePhase		= other.m_activePhase;
	m_guid				= other.m_guid;

	return *this;
}

CSStoryPhaseEntry* CSTableEntry::FindPhaseEntry( const CName& phaseName )
{
	CSStoryPhaseEntry* phaseEntry = NULL;
	if ( phaseName == CName::NONE )
	{
		// find an unnamed phase
		for ( TDynArray< CSStoryPhaseEntry >::iterator it = m_storyPhases.Begin();
			it != m_storyPhases.End(); ++it )
		{
			if ( it->m_storyPhaseName.m_tags.Empty() )
			{
				phaseEntry = &*it;
				break;
			}
		}
	}
	else
	{
		TagList phasesNames;
		phasesNames.AddTag(phaseName);

		// find a phase with the corresponding name
		for ( TDynArray< CSStoryPhaseEntry >::iterator it = m_storyPhases.Begin();
			it != m_storyPhases.End(); ++it )
		{
			if ( TagList::MatchAny( phasesNames, it->m_storyPhaseName.m_tags ) )
			{
				phaseEntry = &*it;
				break;
			}
		}
	}
	return phaseEntry;
}

///////////////////////////////////////////////////////////////////////////////

CSStoryPhaseEntry::CSStoryPhaseEntry() 
	: m_initializers( NULL )
	, m_startInAP( false )
	, m_useLastAP( false )
	, m_spawnDelay( 0 ) 
	, m_alwaysSpawned( false )
	, m_spawnStrategy( NULL )
{
	m_guid = CGUID::Create();
}

Bool CSStoryPhaseEntry::FillSpawnPointPool( const TDynArray< CNode* >& spawnNodes )
{
	// Create list of waypoints
	for ( Uint32 i=0; i<spawnNodes.Size(); i++ )
	{
		// Easy case with waypoints that are tagged directly
		CWayPointComponent* wp = Cast< CWayPointComponent >( spawnNodes[i] );
#ifndef WAYPOINT_COMPONENT_NO_RUNTIME_VALIDITY
		if ( wp && wp->IsPositionValid() && wp->IsUsedByPathLib() )
#else
		if ( wp )
#endif
		{
			m_rtData.m_spawnPointsPool.PushBack( wp );
			continue;
		}

		// Entity, scan waypoints inside
		CEntity* entity = Cast< CEntity >( spawnNodes[i] );
		if ( entity )
		{
			// Get all waypoints
			for ( ComponentIterator< CWayPointComponent > it( entity ); it; ++it )
			{
				CWayPointComponent* wp = *it;
#ifndef WAYPOINT_COMPONENT_NO_RUNTIME_VALIDITY
				if ( wp->IsPositionValid() && wp->IsUsedByPathLib() )
#endif
				{
					m_rtData.m_spawnPointsPool.PushBack( wp );
				}
			}
		}
	}

	// Return true if there is at least one spawn point
	return m_rtData.m_spawnPointsPool.Size() > 0;
}

Bool CSStoryPhaseEntry::AllocateSpawnPoint( Vector& outPos, Float& outYaw, CLayerInfo*& layerInfo )
{
	SStoryPhaseData& storyPhaseData = GetStoryPhaseData();
	if ( storyPhaseData.m_spawnPointsPool.Empty() )
	{
		// Collect tagged entities from the world
		TDynArray< CNode* > spawnNodes;
		
		GGame->GetActiveWorld()->GetTagManager()->CollectTaggedNodes( m_spawnPointTags, spawnNodes, BCTO_MatchAll );

		if ( !FillSpawnPointPool( spawnNodes ) )
		{
			return false;
		}
	}

	// Get random spawn point from the spawn pool
	Int32 spawnPointIndex = GEngine->GetRandomNumberGenerator().Get< Int32 >( storyPhaseData.m_spawnPointsPool.Size() );
	CWayPointComponent* wp = storyPhaseData.m_spawnPointsPool[ spawnPointIndex ].Get();
	storyPhaseData.m_spawnPointsPool.EraseFast( storyPhaseData.m_spawnPointsPool.Begin() + spawnPointIndex );


	// Waypoint was removed, WTF
	if ( !wp )
	{
		return false;
	}

#ifndef WAYPOINT_COMPONENT_NO_RUNTIME_VALIDITY
	// check position
	if ( !wp->IsPositionValid() )
	{
		return false;
	}
#endif

	// Create agent here !
	outPos = wp->GetWorldPosition();
	outYaw = wp->GetWorldYaw();

	if ( wp->GetLayer() )
	{
		layerInfo = wp->GetLayer()->GetLayerInfo();
	}

	return true;
}

Int32 CSStoryPhaseEntry::GetDesiredNumberSpawnedAgents( const GameTime& currGameTime ) const
{
	GameTime currentGameDayTime = currGameTime % GameTime::DAY;

	// Empty timetable, do not spawn anything
	if ( m_spawnTimetable.Empty() ) 
	{
		return 0;
	}

	// Table with one entry is active all the time
	if ( m_spawnTimetable.Size() == 1 ) 
	{
		return m_spawnTimetable[0].m_quantity;
	}

	// Find the appropriate interval
	for ( TDynArray< CSStoryPhaseSpawnTimetableEntry >::const_iterator spawnTimetableEntry = m_spawnTimetable.Begin();; )
	{
		TDynArray< CSStoryPhaseSpawnTimetableEntry >::const_iterator spawnTimetableEntryNext = spawnTimetableEntry + 1;
		if ( spawnTimetableEntryNext == m_spawnTimetable.End() )
		{
			return spawnTimetableEntry->m_quantity;
		}
		if ( spawnTimetableEntry->m_time <= currentGameDayTime && currentGameDayTime < spawnTimetableEntryNext->m_time )
		{
			return spawnTimetableEntry->m_quantity;
		}
		spawnTimetableEntry = spawnTimetableEntryNext;
	}
}

void CSStoryPhaseEntry::ResetRTData()
{
	m_rtData.m_killedNPCsNum = 0;
	m_rtData.m_spawnDelay = EngineTime::ZERO;
}

const ISpawnStrategy* CSStoryPhaseEntry::GetSpawnStrategy() const
{
	if ( m_spawnStrategy )
	{
		return m_spawnStrategy->m_strategy;
	}
	else
	{
		return NULL;
	}
}

void CSStoryPhaseEntry::SaveState( IGameSaver* saver )
{
	CGameSaverBlock block( saver, CNAME(phaseRtData) );

	// Save RT data
	saver->WriteValue( CNAME(killedNPCsNum), m_rtData.m_killedNPCsNum );
	saver->WriteValue( CNAME(spawnDelay), m_rtData.m_spawnDelay );
}

void CSStoryPhaseEntry::RestoreState( IGameLoader* loader )
{
	CGameSaverBlock block( loader, CNAME(phaseRtData) );

	// Load RT data
	loader->ReadValue( CNAME(killedNPCsNum), m_rtData.m_killedNPCsNum );
	loader->ReadValue( CNAME(spawnDelay), m_rtData.m_spawnDelay );
}

///////////////////////////////////////////////////////////////////////////////

CSEntitiesEntry::CSEntitiesEntry() 
	: m_entityTemplate( NULL )
	, m_weight( 1.0f ) 
	, m_initializers( NULL )
	, m_despawners( NULL )
{
	m_guid = CGUID::Create();
}

Bool CSEntitiesEntry::IsForceDespawned() const 
{ 
	return !( m_despawners && m_despawners->m_initializers.Size() > 0 );
}

Bool CSEntitiesEntry::GetRandomAppearance( CName *appearance /* out */ )
{
	PC_SCOPE( GetRandomAppearance );

	// Count the number of appearances
	Int32 appearancesPoolSize = m_appearancesPool.Size();

	// If zero - fill in with appearances
	if ( appearancesPoolSize == 0 )
	{
		// fill appearances list with new data
		if ( m_appearances.Empty() && m_entityTemplate.Load() )
		{
			m_appearancesPool.PushBack( m_entityTemplate.Get()->GetEnabledAppearancesNames() );

			m_entityTemplate.Release();
		}
		else
		{
			m_appearancesPool.PushBack( m_appearances );
		}

		// Non appearances found, so add default NONE appearance which will mean that this entity will be taken without any appearances
		if ( m_appearancesPool.Empty() )
		{
			m_appearancesPool.PushBack( CName::NONE );
		}
		appearancesPoolSize = m_appearancesPool.Size();
	}

	// If greater than zero - select random appearance and remove it from the pool
	if ( appearancesPoolSize > 0 ) // should always be true
	{
		Int32 index = GEngine->GetRandomNumberGenerator().Get< Int32 >( appearancesPoolSize );

		*appearance = m_appearancesPool[ index ];
		m_appearancesPool.EraseFast( m_appearancesPool.Begin() + index );

		return true;
	}

	return false;
}

CSEntitiesEntry& CSEntitiesEntry::operator=( const CSEntitiesEntry& other )
{
	m_entityTemplate	= other.m_entityTemplate;
	m_appearances		= other.m_appearances;
	m_weight			= other.m_weight;
	m_entitySpawnTags	= other.m_entitySpawnTags;
	m_mappinTag			= other.m_mappinTag;
	m_guid				= other.m_guid;

	if( other.m_initializers )
	{
		ASSERT( other.m_initializers->GetParent() );
		// Set same parent, grideditor will change parent if needed
		m_initializers = SafeCast< CCommunityInitializers >( other.m_initializers->Clone( other.m_initializers->GetParent() ) );
	}
	else
	{
		m_initializers = NULL;
	}

	if( other.m_despawners )
	{
		ASSERT( other.m_despawners->GetParent() );

		m_despawners = SafeCast< CCommunityInitializers >( other.m_despawners->Clone( other.m_despawners->GetParent() ) );
	}

	return (*this);
}
