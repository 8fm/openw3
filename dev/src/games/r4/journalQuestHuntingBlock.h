#pragma once
#include "../../common/game/journalPath.h"
#include "../../common/game/questGraphBlock.h"

class CJournalQuestHuntingBlock : public CQuestGraphBlock
{
	DECLARE_ENGINE_CLASS( CJournalQuestHuntingBlock, CQuestGraphBlock, 0 )

public:

	CJournalQuestHuntingBlock();
	virtual ~CJournalQuestHuntingBlock();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetBlockCategory() const { return TXT( "Journal" ); }
	virtual Bool CanBeAddedToGraph( const CQuestGraph* graph ) const { return false; } // OBSOLETE
	virtual Color GetClientColor() const { return Color( 255, 133, 133 ); }
	virtual String GetCaption() const;

	//! CGraphBlock interface
	virtual void OnRebuildSockets();

	virtual String GetBlockAltName() const;

#endif

	THandle< const CJournalPath > GetHuntingQuestPath() const { return m_questHuntingTag.GetConst(); }
	virtual void OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const;

private:
	THandle< CJournalPath > m_questHuntingTag;
	THandle< CJournalPath > m_creatureHuntingClue;
};

BEGIN_CLASS_RTTI( CJournalQuestHuntingBlock )
	PARENT_CLASS( CQuestGraphBlock )
	PROPERTY_CUSTOM_EDIT( m_questHuntingTag, TXT( "Hunting Clue" ), TXT( "JournalPropertyBrowserQuest_Quest" ) )
	PROPERTY_CUSTOM_EDIT( m_creatureHuntingClue, TXT( "Hunting Clue" ), TXT( "HuntingClue" ) )
END_CLASS_RTTI()
