
#include "build.h"
#include "questStaticCameraStop.h"
#include "questGraphSocket.h"
#include "../engine/graphConnectionRebuilder.h"

IMPLEMENT_ENGINE_CLASS( CQuestStaticCameraStopBlock )

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CQuestStaticCameraStopBlock::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	TBaseClass::OnRebuildSockets();

	// Create sockets
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( In ), LSD_Input, LSP_Left ) );
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( Out ), LSD_Output, LSP_Right ) );
}

#endif

void CQuestStaticCameraStopBlock::OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const
{
	TBaseClass::OnActivate( data, inputName, parentThread );

	GGame->ActivateGameCamera( m_blendTime );

	ActivateOutput( data, CNAME( Out ) );
}