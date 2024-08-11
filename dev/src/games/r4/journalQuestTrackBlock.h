#pragma once
#include "../../common/game/journalPath.h"
#include "../../common/game/questGraphBlock.h"


class CJournalQuestTrackBlock : public CQuestGraphBlock
{
	DECLARE_ENGINE_CLASS( CJournalQuestTrackBlock, CQuestGraphBlock, 0 )
public:
	CJournalQuestTrackBlock();
	virtual ~CJournalQuestTrackBlock();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetBlockCategory() const { return TXT( "Journal" ); }
	virtual Bool CanBeAddedToGraph( const CQuestGraph* graph ) const { return true; }
	virtual Color GetTitleColor() const;
	virtual Color GetClientColor() const;
	virtual String GetCaption() const;

	//! CGraphBlock interface
	virtual void OnRebuildSockets();
   	virtual String GetBlockAltName() const;

#endif

	virtual void OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const;
	virtual Bool IsValid() const;

private:

	THandle< CJournalPath >	m_questEntry;
	THandle< CJournalPath >	m_objectiveEntry;
};

BEGIN_CLASS_RTTI( CJournalQuestTrackBlock )
	PARENT_CLASS( CQuestGraphBlock )
	PROPERTY_CUSTOM_EDIT( m_questEntry, TXT( "Quest" ), TXT( "JournalPropertyBrowserQuest_Quest" ) )
	PROPERTY_CUSTOM_EDIT( m_objectiveEntry, TXT( "Objective" ), TXT( "JournalPropertyBrowserQuest_Objective" ) )
END_CLASS_RTTI()