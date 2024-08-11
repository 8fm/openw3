#include "build.h"
#include "questGraphSocket.h"
#include "questLogStateBlock.h"
#include "../engine/graphConnectionRebuilder.h"
#ifndef NO_TEST_FRAMEWORK
#include "../engine/testFramework.h"
#endif

IMPLEMENT_ENGINE_CLASS( CQuestLogStateBlock )

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CQuestLogStateBlock::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	TBaseClass::OnRebuildSockets();

	// Create sockets
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( In ), LSD_Input, LSP_Left ) );
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( Out ), LSD_Output, LSP_Right ) );
}

#endif

void CQuestLogStateBlock::OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const
{
	TBaseClass::OnActivate( data, inputName, parentThread );

#ifndef NO_TEST_FRAMEWORK
	if( STestFramework::GetInstance().IsActive() ) 
	{
		STestFramework::GetInstance().ReportState( m_state );
	}
#endif

	ActivateOutput( data, CNAME( Out ) );
}
