#pragma once

#include "../../common/game/journalPath.h"
#include "../../common/game/questGraphBlock.h"

class CJournalQuestBlock : public CQuestGraphBlock
{
	DECLARE_ENGINE_CLASS( CJournalQuestBlock, CQuestGraphBlock, 0 )

public:

	CJournalQuestBlock();
	virtual ~CJournalQuestBlock();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetBlockCategory() const { return TXT( "Journal" ); }
	virtual Bool CanBeAddedToGraph( const CQuestGraph* graph ) const { return true; }
	virtual Color GetClientColor() const;
	virtual Color GetTitleColor() const;
	virtual String GetCaption() const;

	//! CGraphBlock interface
	virtual void OnRebuildSockets();

	virtual String GetBlockAltName() const;

#endif

#ifndef NO_EDITOR
	virtual String GetSearchCaption() const;
#endif

	virtual void OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const;
	virtual Bool IsValid() const;

protected:
    void DeleteMapPinStates() const;
    void DeleteMapPinStatesFromEntry( const CJournalBase* entry ) const;

private:
	THandle< CJournalPath > m_questEntry;

	Bool m_showInfoOnScreen;
	Bool m_track;
	Bool m_enableAutoSave;
};

BEGIN_CLASS_RTTI( CJournalQuestBlock )
	PARENT_CLASS( CQuestGraphBlock )
	PROPERTY_CUSTOM_EDIT( m_questEntry, TXT( "Quest" ), TXT( "JournalPropertyBrowserQuest" ) )
	PROPERTY_EDIT( m_showInfoOnScreen, TXT( "Should this action be reported to the player" ) );
	PROPERTY_EDIT( m_track, TXT( "Start tracking this quest when activating" ) );
	PROPERTY_EDIT( m_enableAutoSave, TXT( "Requires 'track' == true to work, anables auto-saving on this block." ) );
END_CLASS_RTTI()
