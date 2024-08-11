#include "build.h"
#include "questPhase.h"
#include "questGraph.h"
#include "../core/factory.h"

IMPLEMENT_ENGINE_CLASS( CQuestPhase );

CQuestPhase::CQuestPhase()
{
#ifndef NO_EDITOR
	m_graph = CreateObject< CQuestGraph >( this );
#endif
}

void CQuestPhase::SetGraph( CQuestGraph *graph ) 
{ 
	if ( m_graph )
	{
		m_graph->Discard();
	}

	if ( graph )
	{
		#if defined( W2_PLATFORM_WIN32 )
		m_graph = Cast< CQuestGraph >( graph->Clone( this ) );
		#else
		m_graph = (CQuestGraph*)( graph->Clone( this ) );
		#endif
	}
	else
	{
		m_graph = CreateObject< CQuestGraph >( this );
	}

	MarkModified();
}

void CQuestPhase::CleanupSourceData()
{
#ifndef NO_EDITOR_GRAPH_SUPPORT
	if ( m_graph )
	{
		m_graph->CleanupSourceData();
	}
#endif
}

//////////////////////////////////////////////////////////////////////////

class CQuestPhaseFactory : public IFactory
{
	DECLARE_ENGINE_CLASS( CQuestPhaseFactory, IFactory, 0 );

public:
	CQuestPhaseFactory()
	{
		m_resourceClass = ClassID< CQuestPhase >();
	}

	virtual CResource* DoCreate( const FactoryOptions& options );
};

BEGIN_CLASS_RTTI( CQuestPhaseFactory )
	PARENT_CLASS( IFactory )
END_CLASS_RTTI()

IMPLEMENT_ENGINE_CLASS( CQuestPhaseFactory );

CResource* CQuestPhaseFactory::DoCreate( const FactoryOptions& options )
{
	CQuestPhase *questPhase = ::CreateObject< CQuestPhase >( options.m_parentObject );
	return questPhase;
}
