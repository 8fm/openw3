#include "build.h"
#include "journalQuestObjectiveCounterGraphBlock.h"

#include "r4JournalManager.h"
#include "../../common/engine/graphConnectionRebuilder.h"

IMPLEMENT_ENGINE_CLASS( CJournalQuestObjectiveCounterGraphBlock );

CJournalQuestObjectiveCounterGraphBlock::CJournalQuestObjectiveCounterGraphBlock()
	: m_manualObjective( NULL )
	, m_showInfoOnScreen( false )
{
	m_name = TXT( "Objective Counter" );
}

CJournalQuestObjectiveCounterGraphBlock::~CJournalQuestObjectiveCounterGraphBlock()
{

}

#ifndef NO_EDITOR_GRAPH_SUPPORT

String CJournalQuestObjectiveCounterGraphBlock::GetBlockAltName() const
{
	if( m_manualObjective )
	{
		return m_manualObjective->GetPathAsString();
	}

	return GetBlockName();
}

void CJournalQuestObjectiveCounterGraphBlock::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	TBaseClass::OnRebuildSockets();

	// Create inputs
	GraphSocketSpawnInfo info( ClassID< CQuestGraphSocket >() );

	info.m_name = CNAME( Increment );
	info.m_direction = LSD_Input;
	info.m_placement = LSP_Left;
	info.m_isMultiLink = (info.m_direction == LSD_Input);
	CreateSocket( info );

	info.m_name = CNAME( Decrement );
	info.m_direction = LSD_Input;
	info.m_placement = LSP_Left;
	info.m_isMultiLink = (info.m_direction == LSD_Input);
	CreateSocket( info );

	info.m_name = CNAME( Reset );
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

#endif

void CJournalQuestObjectiveCounterGraphBlock::OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const
{
	TBaseClass::OnActivate( data, inputName, parentThread );
		
	ASSERT( m_manualObjective && m_manualObjective->IsValid() );

	if( !m_manualObjective )
	{
		LOG_R4( TXT( "Null Journal Quest path" ) );
		return;
	}
	if( !m_manualObjective->IsValid() )
	{
		LOG_R4( TXT( "Invalid Journal Quest path: \"%s\" (\"%s\")" ), m_manualObjective->GetPathAsString().AsChar(), m_manualObjective->GetFilePaths().AsChar() );
		return;
	}

	CWitcherJournalManager* manager = GCommonGame->GetSystem< CWitcherJournalManager >();

	CJournalBase* entry = m_manualObjective->GetTarget();

	ASSERT( entry->IsA< CJournalQuestObjective >() );

	Int32 count = manager->GetQuestObjectiveCount( entry->GetGUID() );

	if( inputName == CNAME( Increment ) )
	{
		++count;
	}
	if( inputName == CNAME( Decrement ) )
	{
		--count;
		if ( count < 0 )
		{
			count = 0;
		}
	}
	if( inputName == CNAME( Reset ) )
	{
		count = 0;
	}

	manager->UpdateQuestObjectiveCounter( entry->GetGUID(), count, !m_showInfoOnScreen );

	ActivateOutput( data, CNAME( Out ) );
}

Bool CJournalQuestObjectiveCounterGraphBlock::IsValid() const
{
	// can't be null & can't be invalid
	return m_manualObjective && m_manualObjective->IsValid();
}