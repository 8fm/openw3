#include "build.h"
#include "questStateChangeRequestResetBlock.h"
#include "questGraphSocket.h"
#include "gameWorld.h"
#include "../engine/graphConnectionRebuilder.h"


IMPLEMENT_ENGINE_CLASS( CQuestStateChangeRequestResetBlock )

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CQuestStateChangeRequestResetBlock::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	TBaseClass::OnRebuildSockets();

	// Create sockets
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( In ), LSD_Input, LSP_Left ) );
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( Out ), LSD_Output, LSP_Right ) );
}

#endif

void CQuestStateChangeRequestResetBlock::OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const
{
	TBaseClass::OnActivate( data, inputName, parentThread );

	GCommonGame->GetActiveWorld()->ResetStateChangeRequest( m_entityTag );
	ActivateOutput( data, CNAME( Out ) );
}
