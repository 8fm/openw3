#include "build.h"
#include "questGraphSocket.h"
#include "questEndBlock.h"
#include "questGraph.h"
#include "quest.h"
#include "../engine/graphConnectionRebuilder.h"

IMPLEMENT_ENGINE_CLASS( CQuestEndBlock )

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CQuestEndBlock::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	// Create sockets
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( Space ), LSD_Input, LSP_Left ) );
}

Bool CQuestEndBlock::CanBeAddedToGraph( const CQuestGraph* graph ) const
{
	return graph && graph->GetParent()->IsA< CQuest >();
}

#endif

void CQuestEndBlock::OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const
{
	TBaseClass::OnActivate( data, inputName, parentThread );
	LOG_GAME( TXT( "----> End block reached" ) );

	// Schedule new world to load
}
