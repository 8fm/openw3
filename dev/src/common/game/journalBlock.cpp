#include "build.h"
#include "journalManager.h"
#include "journalBlock.h"
#include "journalPath.h"

#include "commonGame.h"
#include "../engine/graphConnectionRebuilder.h"

IMPLEMENT_ENGINE_CLASS( CJournalBlock );

CJournalBlock::CJournalBlock()
:	m_entry( NULL )
{
	m_name = TXT( "Entry" );
}

CJournalBlock::~CJournalBlock()
{

}

#ifndef NO_EDITOR_GRAPH_SUPPORT

String CJournalBlock::GetBlockAltName() const
{
	if( m_entry )
	{
		return m_entry->GetPathAsString();
	}

	return GetBlockName();
}

String CJournalBlock::GetCaption() const
{
	if ( m_entry )
	{
		const CJournalBase* target = m_entry->GetTarget();
		if( target )
		{
			return target->GetFriendlyName();
		}
	}

	return m_name;
}

void CJournalBlock::OnRebuildSockets()
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

void CJournalBlock::OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const
{
	TBaseClass::OnActivate( data, inputName, parentThread );

	ASSERT( m_entry && m_entry->IsValid() );

	if( !m_entry )
	{
		LOG_GAME( TXT( "Null Journal path" ) );
		return;
	}
	if( !m_entry->IsValid() )
	{
		LOG_GAME( TXT( "Invalid Journal path: \"%s\" (\"%s\")" ), m_entry->GetPathAsString().AsChar(), m_entry->GetFilePaths().AsChar() );
		return;
	}

	CJournalManager* manager = GCommonGame->GetSystem< CJournalManager >();

	if( inputName == CNAME( Activate ) )
	{
		manager->ActivateEntry( m_entry, JS_Active, !m_showInfoOnScreen );
	}

	ActivateOutput( data, CNAME( Out ) );
}

Bool CJournalBlock::IsValid() const
{
	// can't be null & can't be invalid
	return m_entry && m_entry->IsValid();
}