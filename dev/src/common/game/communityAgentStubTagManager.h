#pragma once

#include "communityAgentStub.h"

class CCommunityAgentStubTagManager
{
protected:
	struct TagStub
	{
		DECLARE_CLASS_MEMORY_POOL( MemoryPool_SmallObjects, MC_Gameplay );
	public:
		SAgentStub*	m_agentStub;
		TagStub*	m_next;
	};

protected:
	THashMap< CName, TagStub* >			m_stubs;				// Map of tagged stubs
	TGlobalEventsReporterImpl< CName >*	m_globalEventsReporter;	// For reporting added/removed tags

public:
	CCommunityAgentStubTagManager();
	~CCommunityAgentStubTagManager();

	//! Update manager
	void Update();

	//! Clear all tag mapping
	void Clear();

	//! Add tagged stub
	void AddStub( SAgentStub* stub, const CName& tag );

	//! Add tagged stub
	void AddStub( SAgentStub* stub, const TagList& tags );

	//! Remove tagged stub
	void RemoveStub( SAgentStub* stub, const CName& tag );

	//! Add tagged stub
	void RemoveStub( SAgentStub* stub, const TagList& tags );

	//! Get any stub that have given tag 
	SAgentStub* GetTaggedStub( const CName& tag );

	//! Get stubs that have given tag 
	void CollectTaggedStubs( const CName& tag, TDynArray< SAgentStub* >& stubs );
};
