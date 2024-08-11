/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

class CEdCustomControlListPanel;

//////////////////////////////////////////////////////////////////////////

class CEdCustomControl : public wxPanel
{
protected:
	CEdCustomControlListPanel*	m_parentList;
	wxBitmapButton*				m_killButton;
	wxBitmapButton*				m_findButton;
	Bool						m_selected;

public:
	CEdCustomControl( wxWindow* parent, const wxString& xrcName, Bool hasKillButton = false, Bool hasFindButton = false );
	~CEdCustomControl();

	void OnLeftDown( wxMouseEvent& event );
	void OnMouseEnter( wxMouseEvent& event );

	void Select( Bool doRefresh = true );
	void Deselect( Bool doRefresh = true );
	Bool IsSelected() const { return m_selected; }

	//////////////////////////////////////////////////////////////////////////
	// Callbacks

	// Derive this class to setup your control after creation
	virtual void DoSetup() {}
};

//////////////////////////////////////////////////////////////////////////

class CEdCustomControlListPanel : public ISavableToConfig, public wxScrolledWindow, public CDropTarget 
{
	friend class CEdCustomControl;

public:
	CEdCustomControlListPanel( wxWindow* parent, Bool allowMultiselection );

	CEdCustomControl* AddItem( CObject* object );

	void SelectItem( CObject* object, Bool state = true );

	void SelectItem( CEdCustomControl* control, Bool state = true );

	void DeselectAll();

	Bool IsItemSelected( CObject* object );

	void SetupControl( CObject* object );

protected:
	//////////////////////////////////////////////////////////////////////////
	// Necessary abstracts to be implemented in the deriving class

	// What objects does this list contain?
	virtual CClass* GetExpectedObjectClass() const = 0;

	// Create an instance of the custom control
	virtual CEdCustomControl* CreateCustomControl( CObject* object ) = 0;

	//////////////////////////////////////////////////////////////////////////
	// Optional virtuals, can be implemented in the deriving class

	// Item was just added to the list
	virtual void OnAddItem( CObject* object, CEdCustomControl* customControl ) {}

	// Item is about to be removed. Implementor can prevent this by returning false.
	virtual Bool OnItemRemove( CObject* object, CEdCustomControl* customControl ) { return true; }

	// Validate object for addition
	virtual Bool CanAddItem( CObject* object ) const { return true; }

	// An item has been selected.
	virtual void PostItemSelection( CObject* object, CEdCustomControl* customControl ) {}

	// Mouse cursor entered the item area
	virtual void OnItemHovered( CEdCustomControl* control ) {}

	//////////////////////////////////////////////////////////////////////////
	// Inside helpers

	//! Clear the control, may be used to rebuild it. Doesn't call any OnXXX methods!
	void Clear();

	//! Find object in the list
	CEdCustomControl* FindPanel( CObject* object );

	//! Find control in the list
	CObject* FindObject( CEdCustomControl* panel );

private:
	//////////////////////////////////////////////////////////////////////////
	// UI callbacks
	virtual Bool OnDropResources( wxCoord x, wxCoord y, TDynArray< CResource* > &resources ) override;
	virtual wxDragResult OnDragOver(wxCoord x, wxCoord y, wxDragResult def) override;

	void OnMouseLeftDown( wxMouseEvent& event );
	void OnKillButtonClicked( wxCommandEvent& event );
	void OnFindButtonClicked( wxCommandEvent& event );
	void OnItemClickedLMB( CEdCustomControl* control );

	struct SListItem
	{
		THandle< CObject >	m_object;	// An object this item represents
		CEdCustomControl*	m_panel;	// A panel visualizing this object

		friend Bool operator==( const SListItem& lhs, const SListItem& rhs )
		{
			return lhs.m_object == rhs.m_object && lhs.m_panel == rhs.m_panel;
		}
	};

	TDynArray< SListItem >	m_items;
	Bool					m_allowMultiselection;
};