#include "build.h"
#include "questGraphSocket.h"
#include "questFactsDBChangingBlock.h"
#include "factsDB.h"
#include "../engine/gameTimeManager.h"
#include "../engine/graphConnectionRebuilder.h"

IMPLEMENT_ENGINE_CLASS( CQuestFactsDBChangingBlock )

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CQuestFactsDBChangingBlock::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	TBaseClass::OnRebuildSockets();

	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( In ), LSD_Input, LSP_Left ) );

	// Create output
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( Out ), LSD_Output, LSP_Right ) );
}

#endif

void CQuestFactsDBChangingBlock::OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const
{
	if ( m_factID.GetLength() == 0 )
	{
		return;
	}

	const EngineTime& time = GGame->GetEngineTime();

	if ( m_setExactValue )
	{
		GCommonGame->GetSystem< CFactsDB >()->RemoveFact( m_factID );
	}

	GCommonGame->GetSystem< CFactsDB >()->AddFact( m_factID, m_value, time, CFactsDB::EXP_NEVER );

	// exit immediately
	ActivateOutput( data, CNAME( Out ) );
}
