/**
 * Copyright © 2012 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "questContentActivatorBlock.h"
#include "questGraphSocket.h"
#include "../engine/graphConnectionRebuilder.h"
#include "../core/contentManager.h"

IMPLEMENT_ENGINE_CLASS( CQuestContentActivatorBlock )

CQuestContentActivatorBlock::CQuestContentActivatorBlock()
{
	m_name = TXT("Activate Content");
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CQuestContentActivatorBlock::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	TBaseClass::OnRebuildSockets();

	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( In ), LSD_Input, LSP_Left ) );
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( Out ), LSD_Output, LSP_Right ) );
}

#endif

void CQuestContentActivatorBlock::OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const
{
	TBaseClass::OnActivate( data, inputName, parentThread );
}

void CQuestContentActivatorBlock::OnExecute( InstanceBuffer& data ) const
{
	TBaseClass::OnExecute( data );

	if ( GContentManager->ActivateContent( m_playGoChunk ) )
	{
		ActivateOutput( data, CNAME( Out ) );
	}
	else
	{
		// Yes, spam it. Shouldn't really be hit.
		WARN_GAME(TXT("CQuestContentActivatorBlock: content %ls not available!"), m_playGoChunk.AsChar() );
	}
}
