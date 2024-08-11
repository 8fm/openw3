#include "build.h"

#include "communityAgentStubTagManager.h"
#include "communityAgentStub.h"

CCommunityAgentStubTagManager::CCommunityAgentStubTagManager()
	: m_globalEventsReporter( nullptr )
{
	m_globalEventsReporter = new TGlobalEventsReporterImpl< CName >( GEC_Tag );
}

CCommunityAgentStubTagManager::~CCommunityAgentStubTagManager()
{
	delete m_globalEventsReporter;
	m_globalEventsReporter = nullptr;
}

void CCommunityAgentStubTagManager::Update()
{
	if ( m_globalEventsReporter != nullptr )
	{
		m_globalEventsReporter->ReportEvents();
	}
}

void CCommunityAgentStubTagManager::Clear()
{
	m_stubs.Clear();
}

void CCommunityAgentStubTagManager::AddStub( SAgentStub* stub, const CName& tag )
{
	TagStub*& bucket = m_stubs.GetRef( tag, nullptr );

	// Check if already exists

	TagStub* found = bucket;
	while ( found && found->m_agentStub != stub )
	{
		found = found->m_next;
	}

	// Add if doesn't exist

	if ( !found )
	{
		TagStub* newStub = new TagStub();
		RED_FATAL_ASSERT( newStub, "Failed to allocate TagStub. Please increase m_pool capacity." );
		newStub->m_agentStub = stub;
		newStub->m_next = bucket;
		bucket = newStub;
	}

	if ( m_globalEventsReporter != nullptr )
	{
		m_globalEventsReporter->AddEvent( GET_StubTagAdded, tag );
	}
}

void CCommunityAgentStubTagManager::AddStub( SAgentStub* stub, const TagList& tags )
{
	for ( CName tag : tags.GetTags() )
	{
		AddStub( stub, tag );
	}
}

void CCommunityAgentStubTagManager::RemoveStub( SAgentStub* stub, const CName& tag )
{
	auto bucket = m_stubs.Find( tag );
	if ( bucket != m_stubs.End() )
	{
		TagStub** current = &bucket->m_second;
		while ( *current )
		{
			if ( ( *current )->m_agentStub == stub )
			{
				TagStub* next = ( *current )->m_next;
				delete *current;
				*current = next;

				if ( !bucket->m_second )
				{
					m_stubs.Erase( bucket );
				}
				break;
			}

			current = &( *current )->m_next;
		}

		if ( m_globalEventsReporter != nullptr )
		{
			m_globalEventsReporter->AddEvent( GET_StubTagRemoved, tag );
		}
	}
}

void CCommunityAgentStubTagManager::RemoveStub( SAgentStub* stub, const TagList& tags )
{
	for ( CName tag : tags.GetTags() )
	{
		RemoveStub( stub, tag );
	}
}

SAgentStub* CCommunityAgentStubTagManager::GetTaggedStub( const CName& tag )
{
	TagStub** bucket = m_stubs.FindPtr( tag );
	return bucket ? ( *bucket )->m_agentStub : nullptr;
}

void CCommunityAgentStubTagManager::CollectTaggedStubs( const CName& tag, TDynArray< SAgentStub* >& stubs )
{
	stubs.ClearFast();
	if ( TagStub** bucket = m_stubs.FindPtr( tag ) )
	{
		TagStub* current = *bucket;
		while ( current )
		{
			stubs.PushBack( current->m_agentStub );
			current = current->m_next;
		}
	}
}
