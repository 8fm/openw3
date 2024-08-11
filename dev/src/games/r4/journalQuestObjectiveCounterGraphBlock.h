#pragma once
#include "../../common/game/journalPath.h"
#include "../../common/game/questGraphBlock.h"

class CJournalQuestObjectiveCounterGraphBlock : public CQuestGraphBlock
{
	DECLARE_ENGINE_CLASS( CJournalQuestObjectiveCounterGraphBlock, CQuestGraphBlock, 0 )

public:

	CJournalQuestObjectiveCounterGraphBlock();
	virtual ~CJournalQuestObjectiveCounterGraphBlock();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetBlockCategory() const { return TXT( "Journal" ); }
	virtual Bool CanBeAddedToGraph( const CQuestGraph* graph ) const { return true; }
	virtual Color GetClientColor() const { return Color( 218, 180, 180 ); }
	virtual String GetBlockAltName() const;

	//! CGraphBlock interface
	virtual void OnRebuildSockets();

#endif

	virtual void OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const;
	virtual Bool IsValid() const;

private:
	THandle< CJournalPath > m_manualObjective;
	Bool m_showInfoOnScreen;
};

BEGIN_CLASS_RTTI( CJournalQuestObjectiveCounterGraphBlock )
	PARENT_CLASS( CQuestGraphBlock )
	PROPERTY_CUSTOM_EDIT( m_manualObjective, TXT( "Should point to a quest objective with a manual count" ), TXT( "JournalPropertyBrowserQuest_Objective" ) )
	PROPERTY_EDIT( m_showInfoOnScreen, TXT( "Should this action be reported to the player" ) );
END_CLASS_RTTI()
