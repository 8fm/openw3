#include "build.h"
#include "questGraphSocket.h"
#include "questScopeBlock.h"
#include "questPhaseInputBlock.h"
#include "questGraph.h"
#include "../engine/graphConnectionRebuilder.h"

IMPLEMENT_ENGINE_CLASS( CQuestPhaseInputBlock )

CQuestPhaseInputBlock::CQuestPhaseInputBlock()
{
	m_name = TXT( "In" );
	m_socketID = CNAME( In );
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CQuestPhaseInputBlock::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	// Create sockets
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( Space ), LSD_Output, LSP_Right ) );
}

Bool CQuestPhaseInputBlock::CanBeAddedToGraph( const CQuestGraph* graph ) const
{
	return graph && ( graph->GetParent()->IsExactlyA< CQuestPhase >() || graph->GetParent()->IsA< CQuestScopeBlock >() );
}

#endif

void CQuestPhaseInputBlock::OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const
{
	TBaseClass::OnActivate( data, inputName, parentThread );
	ActivateOutput( data, CNAME( Space ) );
}

