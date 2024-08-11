#include "build.h"
#include "questTagsPresenceCondition.h"
#include "communitySystem.h"
#include "communityData.h"
#include "communityAgentStub.h"
#include "communityAgentStubTagManager.h"
#include "../../common/engine/tagManager.h"

IMPLEMENT_ENGINE_CLASS( CQuestTagsPresenceCondition )

CQuestTagsPresenceCondition::CQuestTagsPresenceCondition()
	: m_all( true )
	, m_howMany( 1 )
	, m_includeStubs( false )
	, m_isFulfilled( false )
	, m_wasRegistered( false )
{
}

CQuestTagsPresenceCondition::~CQuestTagsPresenceCondition()
{
	RegisterCallback( false );
}

void CQuestTagsPresenceCondition::OnActivate()
{
	TBaseClass::OnActivate();

	m_wasRegistered = false;
	m_isFulfilled = CheckCondition();
	if ( !m_isFulfilled )
	{
		RegisterCallback( true );
	}
}

void CQuestTagsPresenceCondition::OnDeactivate()
{
	RegisterCallback( false );
	m_wasRegistered = false;

	TBaseClass::OnDeactivate();
}

Bool CQuestTagsPresenceCondition::OnIsFulfilled()
{
	if ( !m_isFulfilled && !m_wasRegistered )
	{
		if ( RegisterCallback( true ) )
		{
			m_isFulfilled = CheckCondition();
		}
	}
	return m_isFulfilled;
}

Bool CQuestTagsPresenceCondition::CheckCondition()
{
	if ( !GGame )
	{
		return false;
	}

	TDynArray< CEntity* > entities;
	CWorld* world = GGame->GetActiveWorld();
	if ( world != nullptr )
	{
		CTagManager* tagsMgr = world->GetTagManager();
		if ( tagsMgr != nullptr )
		{
			tagsMgr->CollectTaggedEntities( m_tags, entities, BCTO_MatchAny );
		}
	}

	TDynArray< SAgentStub* > stubs;
	if ( m_includeStubs )
	{
		CCommunitySystem* community = GCommonGame->GetSystem< CCommunitySystem >();
		if ( community != nullptr )
		{
			CCommunityAgentStubTagManager* tagsMgr = community->GetAgentStubTagManager();
			if ( tagsMgr != nullptr )
			{
				for ( CName tag : m_tags.GetTags() )
				{
					tagsMgr->CollectTaggedStubs( tag, stubs );
				}
			}
		}
	}

	TagList currentlyPresentTags;
	for ( const CEntity* entity : entities )
	{
		currentlyPresentTags.AddTags( entity->GetTags() );
	}
	Uint32 realStubsCount = 0;
	for ( const SAgentStub* stub : stubs )
	{		
		if ( stub->IsOnlyStub() )
		{
			currentlyPresentTags.AddTags( stub->GetEntitiesEntry()->m_entitySpawnTags );
			realStubsCount++;
		}
	}

	// match the tag names
	Bool matchType;
	if ( m_all )
	{
		matchType = TagList::MatchAll( m_tags, currentlyPresentTags );
	}
	else
	{
		matchType = TagList::MatchAny( m_tags, currentlyPresentTags );
	}

	// match the number of entities that have these tags
	Bool matchNumber = ( entities.Size() + realStubsCount >= m_howMany );

	return matchType && matchNumber;
}

Bool CQuestTagsPresenceCondition::RegisterCallback( Bool reg )
{
	if ( reg != m_wasRegistered && GGame != nullptr && GGame->GetGlobalEventsManager() != nullptr )
	{
		if ( reg )
		{
			GGame->GetGlobalEventsManager()->AddFilteredListener( GEC_Tag, this, m_tags.GetTags() );
		}
		else
		{
			GGame->GetGlobalEventsManager()->RemoveFilteredListener( GEC_Tag, this, m_tags.GetTags() );
		}
		m_wasRegistered = reg;
		return true;
	}
	return false;
}

void CQuestTagsPresenceCondition::OnGlobalEvent( EGlobalEventCategory eventCategory, EGlobalEventType eventType, const SGlobalEventParam& param )
{
	if ( eventCategory == GEC_Tag && m_tags.HasTag( param.Get< CName > () ) )
	{
		m_isFulfilled = CheckCondition();
	}
}
