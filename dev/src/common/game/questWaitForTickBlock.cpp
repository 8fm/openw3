#include "build.h"
#include "questGraphSocket.h"
#include "questWaitForTickBlock.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "../engine/graphConnectionRebuilder.h"
#ifndef NO_TEST_FRAMEWORK
#include "../engine/testFramework.h"
#endif

IMPLEMENT_ENGINE_CLASS( CQuestWaitForTickBlock )

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CQuestWaitForTickBlock::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	TBaseClass::OnRebuildSockets();

	// Create sockets
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( In ), LSD_Input, LSP_Left ) );
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( Out ), LSD_Output, LSP_Right ) );
}

#endif

void CQuestWaitForTickBlock::OnExecute( InstanceBuffer& data ) const
{
	TBaseClass::OnExecute( data  );

#ifndef NO_TEST_FRAMEWORK
	if( !STestFramework::GetInstance().IsActive() || STestFramework::GetInstance().GetCurrentTick() >= static_cast< Uint64 >( m_tick ) ) 
#endif
	{
		ActivateOutput( data, CNAME( Out ) );
	}
}