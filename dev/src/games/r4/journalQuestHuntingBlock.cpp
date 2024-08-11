#include "build.h"

#include "journalQuestHuntingBlock.h"
#include "r4JournalManager.h"

// Remove include when hack is removed
#include "journalQuestBlock.h"
#include "../../common/engine/graphConnectionRebuilder.h"

IMPLEMENT_ENGINE_CLASS( CJournalQuestHuntingBlock );

CJournalQuestHuntingBlock::CJournalQuestHuntingBlock()
{
	m_name = TXT( "Hunting Clue" );
}

CJournalQuestHuntingBlock::~CJournalQuestHuntingBlock()
{

}

#ifndef NO_EDITOR_GRAPH_SUPPORT

String CJournalQuestHuntingBlock::GetCaption() const
{
	const CJournalBase* target = m_questHuntingTag->GetTarget();

	if( target )
	{
		return target->GetFriendlyName();
	}

	return m_name;
}

void CJournalQuestHuntingBlock::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	TBaseClass::OnRebuildSockets();

	//TODO: Remove Hack

	// Create inputs
	GraphSocketSpawnInfo info( ClassID< CQuestGraphSocket >() );

	info.m_name = CNAME( Found );
	info.m_direction = LSD_Input;
	info.m_placement = LSP_Left;
	info.m_isMultiLink = (info.m_direction == LSD_Input);
	CreateSocket( info );

	// Create output

	info.m_name = CNAME( Out );
	info.m_direction = LSD_Output;
	info.m_placement = LSP_Right;
	info.m_isMultiLink = (info.m_direction == LSD_Input);
	CreateSocket( info );
}

String CJournalQuestHuntingBlock::GetBlockAltName() const
{
	if( m_questHuntingTag )
	{
		return m_questHuntingTag->GetPathAsString();
	}

	return GetBlockName();
}

#endif

void CJournalQuestHuntingBlock::OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const
{
	TBaseClass::OnActivate( data, inputName, parentThread );

	ASSERT( m_questHuntingTag && m_questHuntingTag->IsValid() );

	CWitcherJournalManager* manager = GCommonGame->GetSystem< CWitcherJournalManager >();

	if( inputName == CNAME( Found ) )
	{
		manager->ActivateHuntingQuestClue( m_questHuntingTag, m_creatureHuntingClue );
	}

	ActivateOutput( data, CNAME( Out ) );
}
