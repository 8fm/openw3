#include "build.h"
#include "journalQuestTrackBlock.h"
#include "../../common/game/questGraphSocket.h"
#include "../../common/game/journalPath.h"
#include "commonMapManager.h"
#include "journalQuest.h"
#include "../../common/engine/graphConnectionRebuilder.h"

IMPLEMENT_ENGINE_CLASS( CJournalQuestTrackBlock );

CJournalQuestTrackBlock::CJournalQuestTrackBlock()
    : m_questEntry( NULL )
{
	m_name = TXT("Track Quest");
}

CJournalQuestTrackBlock::~CJournalQuestTrackBlock()
{
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

Color CJournalQuestTrackBlock::GetTitleColor() const
{
	if ( m_questEntry )
	{
		if ( !m_questEntry->IsValid() )
		{
			return Color( 255, 70, 70 );
		}
	}
	return TBaseClass::GetTitleColor();
}

Color CJournalQuestTrackBlock::GetClientColor() const
{
	if ( m_questEntry )
	{
		if ( !m_questEntry->IsValid() )
		{
			return Color( 255, 70, 70 );
		}
	}
	return Color( 224, 124, 124 );
}

String CJournalQuestTrackBlock::GetCaption() const
{
	if ( m_questEntry )
	{
		const CJournalBase* target = m_questEntry->GetTarget();
		if( target )
		{
			return target->GetFriendlyName();
		}
	}
	return m_name;
}

void CJournalQuestTrackBlock::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	TBaseClass::OnRebuildSockets();

	// Create sockets
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( In ), LSD_Input, LSP_Left ) );
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( Out ), LSD_Output, LSP_Right ) );
}

String CJournalQuestTrackBlock::GetBlockAltName() const
{
	if( m_questEntry )
	{
		return m_questEntry->GetPathAsString();
	}

	return GetBlockName();
}

#endif


void CJournalQuestTrackBlock::OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const
{
	CWitcherJournalManager* manager = GCommonGame->GetSystem< CWitcherJournalManager >();
	if ( manager )
	{
		CJournalPath* questJournalPath = m_questEntry.Get();
		if ( questJournalPath && questJournalPath->IsValid() )
		{
			const CJournalQuest* quest = Cast< CJournalQuest >( questJournalPath->GetTarget() );
			if ( quest )
			{
				manager->TrackQuest( quest->GetGUID(), false );
			}
		}

		CJournalPath* objectiveJournalPath = m_objectiveEntry.Get();
		if ( objectiveJournalPath && objectiveJournalPath->IsValid() )
		{
			const CJournalQuestObjective* objective = Cast< CJournalQuestObjective >( objectiveJournalPath->GetTarget() );
			if ( objective )
			{
				manager->HighlightObjective( objective );
			}
		}
	}

    ActivateOutput( data, CNAME( Out ) );
}

Bool CJournalQuestTrackBlock::IsValid() const
{
	// they can be null, but can't be invalid
	if ( m_questEntry )
	{
		if ( !m_questEntry->IsValid() )
		{
			return false;
		}
	}
	if ( m_objectiveEntry )
	{
		if ( !m_objectiveEntry->IsValid() )
		{
			return false;
		}
	}
	return true;
}

