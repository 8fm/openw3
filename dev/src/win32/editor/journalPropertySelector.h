/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CEdJournalTree;

// Must match order in CEdJournalTree::eTreeCategory
#define JOURNAL_SELECTOR_QUESTS		( 1 << 0 )
#define JOURNAL_SELECTOR_CHARACTERS	( 1 << 1 )
#define JOURNAL_SELECTOR_GLOSSARY	( 1 << 2 )
#define JOURNAL_SELECTOR_TUTORIAL	( 1 << 3 )
#define JOURNAL_SELECTOR_ITEMS		( 1 << 4 )
#define JOURNAL_SELECTOR_CREATURES	( 1 << 5 )
#define JOURNAL_SELECTOR_STORYBOOK	( 1 << 6 )
#define JOURNAL_SELECTOR_PLACES		( 1 << 7 )

#define JOURNAL_SELECTOR_ALL_BUT_QUESTS ( 0xffffffff & ~JOURNAL_SELECTOR_QUESTS )
#define JOURNAL_SELECTOR_ALL		0xffffffff

// Selection of partition tree
class CEdJournalPropertySelector : public ICustomPropertyEditor
{
public:
	CEdJournalPropertySelector( CPropertyItem* propertyItem, Uint32 journalFlags, const CClass* typeSelectable = NULL );
	virtual ~CEdJournalPropertySelector();

	virtual void CreateControls( const wxRect &propertyRect, TDynArray< wxControl* >& outSpawnedControls ) override;
	virtual void CloseControls() override;
	virtual Bool SaveValue() override;
	virtual Bool GrabValue( String& displayValue ) override;

private:

	void OnItemSelected( const wxTreeEvent& event );

	CEdJournalTree* m_tree;
	const CClass* m_typeSelectable;
	Uint32 m_flags;
	THandle< CJournalPath > m_selectedPath;
};
