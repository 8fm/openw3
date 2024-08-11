#include "build.h"
#include "questStoryPhaseSetterBlock.h"
#include "questGraphSocket.h"
#include "questStoryPhaseProperty.h"
#include "../engine/graphConnectionRebuilder.h"


IMPLEMENT_ENGINE_CLASS( CQuestStoryPhaseSetterBlock )


CQuestStoryPhaseSetterBlock::CQuestStoryPhaseSetterBlock() 
{
	m_name = TXT( "Story phase setter" );
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CQuestStoryPhaseSetterBlock::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	TBaseClass::OnRebuildSockets();

	// Create sockets
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( In ), LSD_Input, LSP_Left ) );
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( Out ), LSD_Output, LSP_Right ) );
}

#endif

void CQuestStoryPhaseSetterBlock::OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const
{
	TBaseClass::OnActivate( data, inputName, parentThread );

	// activate a community story phase
	for ( TDynArray< IQuestSpawnsetAction* >::const_iterator it = m_spawnsets.Begin();
		it != m_spawnsets.End(); ++it )
	{
		const IQuestSpawnsetAction* action = *it;
		if ( action )
		{
			action->ResetCachedData();
			action->Perform();
		}
	}

	ActivateOutput( data, CNAME( Out ) );
}

void CQuestStoryPhaseSetterBlock::CollectContent( IQuestContentCollector& collector ) const
{
	TBaseClass::CollectContent( collector );

	for ( TDynArray< IQuestSpawnsetAction* >::const_iterator it = m_spawnsets.Begin();
		it != m_spawnsets.End(); ++it )
	{
		const IQuestSpawnsetAction* action = *it;
		if ( action )
		{
			action->CollectContent( collector );
		}
	}
}
