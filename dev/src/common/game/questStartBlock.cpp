#include "build.h"
#include "questGraphSocket.h"
#include "questStartBlock.h"
#include "questsSystem.h"
#include "questGraph.h"
#include "quest.h"
#include "../engine/graphConnectionRebuilder.h"


IMPLEMENT_ENGINE_CLASS( CQuestStartBlock )

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CQuestStartBlock::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	// Create sockets
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( Space ), LSD_Output, LSP_Right ) );
}

Bool CQuestStartBlock::CanBeAddedToGraph( const CQuestGraph* graph ) const
{
	return graph && graph->GetParent()->IsA< CQuest >();
}

#endif

void CQuestStartBlock::OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const
{
	TBaseClass::OnActivate( data, inputName, parentThread );
	ActivateOutput( data, CNAME( Space ) );
}
