#include "build.h"


#include "journalQuestMonsterKnownGraphBlock.h"

#include "r4JournalManager.h"
#include "../../common/engine/graphConnectionRebuilder.h"

IMPLEMENT_ENGINE_CLASS( CJournalQuestMonsterKnownGraphBlock );

CJournalQuestMonsterKnownGraphBlock::CJournalQuestMonsterKnownGraphBlock()
:	m_manualQuest( NULL )
{
	m_name = TXT( "Quest Monster Known" );
}

CJournalQuestMonsterKnownGraphBlock::~CJournalQuestMonsterKnownGraphBlock()
{

}

#ifndef NO_EDITOR_GRAPH_SUPPORT

String CJournalQuestMonsterKnownGraphBlock::GetBlockAltName() const
{
	if( m_manualQuest )
	{
		return m_manualQuest->GetPathAsString();
	}

	return GetBlockName();
}

void CJournalQuestMonsterKnownGraphBlock::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	TBaseClass::OnRebuildSockets();

	// Create inputs
	GraphSocketSpawnInfo info( ClassID< CQuestGraphSocket >() );

	info.m_name = CNAME( Activate );
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

void CJournalQuestMonsterKnownGraphBlock::OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const
{
	TBaseClass::OnActivate( data, inputName, parentThread );
		
	ASSERT( m_manualQuest && m_manualQuest->IsValid() );

	if( !m_manualQuest )
	{
		LOG_R4( TXT( "Null Journal Quest path" ) );
		return;
	}
	if( !m_manualQuest->IsValid() )
	{
		LOG_R4( TXT( "Invalid Journal Quest path: \"%s\" (\"%s\")" ), m_manualQuest->GetPathAsString().AsChar(), m_manualQuest->GetFilePaths().AsChar() );
		return;
	}

	CWitcherJournalManager* manager = GCommonGame->GetSystem< CWitcherJournalManager >();

	CJournalBase* entry = m_manualQuest->GetTarget();

	ASSERT( entry->IsA< CJournalQuest >() );

	manager->SetQuestMonsterAvailable(true, entry->GetGUID());

	ActivateOutput( data, CNAME( Out ) );
}

Bool CJournalQuestMonsterKnownGraphBlock::IsValid() const
{
	// can't be null && can't be valid
	return m_manualQuest && m_manualQuest->IsValid();
}