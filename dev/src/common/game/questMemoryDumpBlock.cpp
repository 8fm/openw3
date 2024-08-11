#include "build.h"
#include "questGraphSocket.h"
#include "questMemoryDumpBlock.h"
#include "../engine/graphConnectionRebuilder.h"
#include "../../common/redMemoryFramework/redMemorySystemMemoryStats.h"

IMPLEMENT_ENGINE_CLASS( CQuestMemoryDumpBlock )

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CQuestMemoryDumpBlock::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	TBaseClass::OnRebuildSockets();

	// Create sockets
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( In ), LSD_Input, LSP_Left ) );
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( Out ), LSD_Output, LSP_Right ) );
}

#endif

void CQuestMemoryDumpBlock::OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const
{
	TBaseClass::OnActivate( data, inputName, parentThread );

	AnsiChar* tagAnsi = UNICODE_TO_ANSI( m_tag.AsChar() );
	RED_MEMORY_DUMP_CLASS_MEMORY_REPORT(  tagAnsi );
	RED_MEMORY_DUMP_POOL_MEMORY_REPORT( tagAnsi );

	ActivateOutput( data, CNAME( Out ) );
}
