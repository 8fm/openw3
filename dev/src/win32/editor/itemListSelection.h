/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "selectionList.h"
#include "itemSelectionDialogs/basicSelectorDialogPropertyEditor.h"

// Item selection
class CItemListSelection : public CEdBasicSelectorDialogPropertyEditor
{
public:
	CItemListSelection( CPropertyItem* item );

private:
	virtual Bool GrabValue( String& displayValue ) override;

	virtual void OnSelectDialog( wxCommandEvent& event ) override;
};

// ability selection
class CAbilityListSelection : public CListSelection
{
protected:
	TDynArray< CName > m_abilities;

public:
	CAbilityListSelection( CPropertyItem* item );

	virtual void CreateControls( const wxRect &propRect, TDynArray< wxControl* >& outSpawnedControls ) override;
};

// Item categories selection
class CItemCategoriesListSelection : public CEdBasicSelectorDialogPropertyEditor
{
public:
	CItemCategoriesListSelection( CPropertyItem* item );

private:
	virtual Bool GrabValue( String& displayValue ) override;

	virtual void OnSelectDialog( wxCommandEvent& event ) override;
};

// Item selection restricted by parent category
class CSuggestedListSelection : public CEdBasicSelectorDialogPropertyEditor
{
public:
	CSuggestedListSelection( CPropertyItem* item );

private:
	virtual Bool GrabValue( String& displayValue ) override;

	virtual void OnSelectDialog( wxCommandEvent& event ) override;
};

// Item selection restricted by category defined in the same non-object class
class CAnimEvent_SuggestedListSelection : public CListSelection
{
protected:
	TDynArray< CName > m_options;

public:
	CAnimEvent_SuggestedListSelection( CPropertyItem* item );

	virtual void CreateControls( const wxRect &propRect, TDynArray< wxControl* >& outSpawnedControls ) override;
};

class CEffectsListSelection : public CListSelection
{

protected:
	TDynArray< CName > m_options;

public:
	CEffectsListSelection( CPropertyItem* item )
		: CListSelection(item)
	{}

	virtual void CreateControls( const wxRect &propRect, TDynArray< wxControl* >& outSpawnedControls ) override;
};


