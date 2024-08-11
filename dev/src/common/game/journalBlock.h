#pragma once

#include "questGraphSocket.h"
#include "questGraphBlock.h"

class CJournalBlock : public CQuestGraphBlock
{
	DECLARE_ENGINE_CLASS( CJournalBlock, CQuestGraphBlock, 0 )

public:

	CJournalBlock();
	virtual ~CJournalBlock();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetBlockCategory() const { return TXT( "Journal" ); }
	virtual Bool CanBeAddedToGraph( const CQuestGraph* graph ) const { return true; }
	virtual Color GetClientColor() const { return Color( 255, 153, 153 ); }
	virtual String GetCaption() const;

	//! CGraphBlock interface
	virtual void OnRebuildSockets();

	virtual String GetBlockAltName() const;

#endif

	virtual void OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const;
	virtual Bool IsValid() const;

private:
	THandle< CJournalPath > m_entry;
	Bool m_showInfoOnScreen;
};

BEGIN_CLASS_RTTI( CJournalBlock )
	PARENT_CLASS( CQuestGraphBlock )
	PROPERTY_CUSTOM_EDIT( m_entry, TXT( "Journal Entry" ), TXT( "JournalPropertyBrowserNonQuest" ) )
	PROPERTY_EDIT( m_showInfoOnScreen, TXT( "If applicable, will send an event that this entry has changed" ) )
END_CLASS_RTTI();
