#pragma once

#include "itemSelectorDialogBase.h"

class CEdCreateLocalizedStringDialog : public wxDialog
{
	// WX RTTI
	DECLARE_CLASS( CEdCreateLocalizedStringDialog );

public:
	CEdCreateLocalizedStringDialog( wxWindow* parent, const String& defaultStringValue );
	~CEdCreateLocalizedStringDialog();

	Bool Success() const { return m_success; }
	String GetLocalizedString() const;
	String GetKey() const;

private:
	void OnOK( wxCommandEvent& event );
	void OnCancel( wxEvent& event );
	void OnChar( wxKeyEvent& event );

private:
	wxTextCtrl* m_stringCtrl;
	wxTextCtrl* m_keyCtrl;
	CEdHelpBubble* m_helpBubble;

	Bool m_success;
};

//-----------------------------------------------------------------------------------------

wxDECLARE_EVENT( wxEVT_ITEM_SELECTOR_OK, wxCommandEvent );

class CEdItemSelectorDialogBase : public wxSmartLayoutDialog
{
	DECLARE_CLASS( CEdItemSelectorDialogBase );

public:
	CEdItemSelectorDialogBase( wxWindow* parent, const Char* configPath );
	virtual ~CEdItemSelectorDialogBase();

	void Initialize();
	virtual void Populate() = 0;

protected:

	// Will delete wxImageList automatically
	void SetImageList( wxImageList* imageList );
	void AddItem( const String& name, void* data, Bool isSelectable, Int32 icon = -1, Bool selected = false );
	void AddItem( const String& name, void* data, const String& parentName, Bool isSelectable, Int32 icon = -1, Bool selected = false );

private:
	void OnInitialize( wxInitDialogEvent& event );
	void OnShow( wxShowEvent& event );
	void OnClose( wxCloseEvent& event );
	void OnItemSelected( wxTreeEvent& event );
	void OnStartSearchTree( wxTreeEvent& event );
	void OnStartSearch( const wxKeyEvent& event );
	void OnSearchKeyDown( wxKeyEvent& event );
	void OnSearchTextChange( wxCommandEvent& event );
	void OnCloseSearchPanel( wxCommandEvent& event );
	void OnSearchItemSelected( wxCommandEvent& event );
	void OnFocus( wxFocusEvent& event );

	void SelectAndClose( const wxTreeItemId selectedItem );
	void CloseSearchPanel();

private:
	virtual void SaveOptionsToConfig();
	virtual void LoadOptionsFromConfig();

protected:
	wxTreeCtrl* m_tree;
	wxTextCtrl* m_search;
	wxPanel* m_searchPanel;

private:
	typedef THashMap< String, wxTreeItemId > TNameToItemMap;
	TNameToItemMap m_nameToItemMap;

	wxColour m_normalItemColour;
	wxColour m_highlightedItemColour;
	wxTreeItemId m_highlightedItem;

	wxColour m_normalSearchColour;
	wxColour m_nothingFoundSearchColour;

	CEdHelpBubble* m_helpBubble;

	wxTreeItemId m_rootItem;

	Bool m_initialised;

	const Char* m_configPath;
};

//-----------------------------------------------------------------------------------------

class CEdSimpleStringSelectorDialog : public CEdItemSelectorDialogBase, public ILocalizableObject
{
	// WX RTTI
	DECLARE_CLASS( CEdSimpleStringSelectorDialog );

public:
	CEdSimpleStringSelectorDialog( wxWindow* parent, const String& category, const CResource* newStringResource = NULL );
	~CEdSimpleStringSelectorDialog();

private:
	virtual void Populate();
	void OnCreateNewEntry( wxCommandEvent& event );
	void GetLocalizedStrings( TDynArray< LocalizedStringEntry >& localizedStrings );

private:
	String m_category;

	TDynArray< Uint32 > m_ids;
	TDynArray< String > m_displayNames;

	// Data required to make new string
	LocalizedString m_newString;
	String m_newKey;
	const CResource* m_newStringResource;
};

//-----------------------------------------------------------------------------------------

// Cannot edit strings directly, only change which one is selected
class CEdLocalizedStringPropertyEditorReadOnly : public ICustomPropertyEditor
{
public:
	CEdLocalizedStringPropertyEditorReadOnly( CPropertyItem* item, const Char* category );
	~CEdLocalizedStringPropertyEditorReadOnly();

protected:
	virtual void CreateControls( const wxRect &propRect, TDynArray< wxControl* >& outSpawnedControls ) override;
	virtual void CloseControls() override;
	virtual Bool GrabValue( String& displayValue ) override;

private:
	void GrabValue( Uint32& id, String& text );
	void SetDisplayCtrlValues();

	void OnSelectStringDialog( wxCommandEvent& event );
	void OnStringSelected( wxCommandEvent& event );
	void OnChar( wxKeyEvent& event );
	void OnTextUpdated( wxCommandEvent& event );

private:
	String m_category;
	
	wxPanel* m_focusPanel;
	wxTextCtrl* m_textDisplay;
	wxTextCtrl* m_idDisplay;

	LocalizedString m_string;
	Bool m_readOnly;
};
