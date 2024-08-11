#pragma once
#include "../../common/game/journalPath.h"
#include "../../common/game/questGraphBlock.h"

class CJournalQuestMonsterKnownGraphBlock : public CQuestGraphBlock
{
	DECLARE_ENGINE_CLASS( CJournalQuestMonsterKnownGraphBlock, CQuestGraphBlock, 0 )

public:

	CJournalQuestMonsterKnownGraphBlock();
	virtual ~CJournalQuestMonsterKnownGraphBlock();

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
	THandle< CJournalPath > m_manualQuest;
};

BEGIN_CLASS_RTTI( CJournalQuestMonsterKnownGraphBlock )
	PARENT_CLASS( CQuestGraphBlock )
	PROPERTY_CUSTOM_EDIT( m_manualQuest, TXT( "Should point to a quest with a beastiary entry set" ), TXT( "JournalPropertyBrowserQuest_Quest" ) )
END_CLASS_RTTI()
