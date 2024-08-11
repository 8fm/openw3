#pragma once
#include "../../common/game/journalPath.h"
#include "../../common/game/questGraphBlock.h"


class CJournalQuestMappinStateBlock : public CQuestGraphBlock
{
	DECLARE_ENGINE_CLASS( CJournalQuestMappinStateBlock, CQuestGraphBlock, 0 )
public:
	CJournalQuestMappinStateBlock();
	virtual ~CJournalQuestMappinStateBlock();

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

	THandle< CJournalPath >	m_mappinEntry;
	Bool					m_enableOnlyIfLatest;
	Bool					m_disableAllOtherMapPins;
};

BEGIN_CLASS_RTTI( CJournalQuestMappinStateBlock )
	PARENT_CLASS( CQuestGraphBlock )
	PROPERTY_CUSTOM_EDIT( m_mappinEntry, TXT( "Mappin" ), TXT( "JournalPropertyBrowserQuest_Mappin" ) )
	PROPERTY_EDIT( m_enableOnlyIfLatest, TXT( "Enable only if there are no enabled mappins after this one" ) )
	PROPERTY_EDIT( m_disableAllOtherMapPins, TXT( "When enabling a mappin, disable all other mappins in the same objective" ) )
END_CLASS_RTTI()