
#pragma once

#include "../smartLayout.h"
#include "../poppedUp.h"

class CEdItemSelectorImpl;

//! Dropdown-like tree selector with built-in search. Each selectable element has an user-data pointer attached.
//! You can either use it directly, or derive your on class providing the 'Populate' method to fill up the tree.
template < typename ItemType >
class CEdItemSelectorDialog
{
public:
	//! Set storeFilter to true when you want to preserve search-box state even after the window is closed
	CEdItemSelectorDialog( wxWindow* parent, const String& configPath = String::EMPTY, const String& title = TXT("Item selector"), Bool storeFilter = false )
		: m_impl( parent, configPath, title, storeFilter )
	{
	}

	~CEdItemSelectorDialog()
	{
	}

	//! Sets an image list to be used in the tree
	void SetImageList( wxImageList* imageList )
	{
		m_impl.SetImageList( imageList );
	}

	//! Add item by the name and a parent. Assumes that every item has an unique name (otherwise use path-based method)
	void AddItem( const String& itemName, ItemType* data, const String& parentItemName, Bool isSelectable, Int32 icon = -1, Bool isSelected = false )
	{
		m_impl.AddItem( itemName, (void *)data, parentItemName, isSelectable, icon, isSelected );
	}

	//! Add item by specifying a full path (separated by '|')
	void AddItem( const String& itemPath, ItemType* data, Bool isSelectable, Int32 icon = -1, Bool isSelected = false )
	{
		m_impl.AddItem( itemPath, (void *)data, isSelectable, icon, isSelected );
	}

	//! Returns an item text associated with given data or String::EMPTY if not found
	String FindNameForData( ItemType* data ) const
	{
		return m_impl.FindNameForData( (void *)data );
	}

	//!
	Bool IsEmpty() const
	{
		return m_impl.IsEmpty();
	}

	//! Shows the dialog modally and returns the result (nullptr if canceled). Optionally, you can give a position. 
	//! By default it shows itself at the mouse cursor location.
	ItemType* Execute( wxPoint* position = nullptr )
	{
		Populate();

		if ( m_impl.ShowModal( position ) == wxID_OK )
		{
			return static_cast< ItemType* >( m_impl.GetResult() );
		}

		return nullptr;
	}

protected:
	//! Override this to fill up the list by calling AddItem
	virtual void Populate() {};

private:
	CEdItemSelectorImpl m_impl;
};


class CEdItemSelectorImpl 
	: public CEdPoppedUp< wxPanel >
	, ISavableToConfig
{
public:
	static const Char PATH_SEPARATOR = L'|';

	CEdItemSelectorImpl( wxWindow* parent, const String& configPath, const String& tile, Bool storeFilter );

	~CEdItemSelectorImpl();

	void SetImageList( wxImageList* imageList );
	void AddItem( const String& itemPath, void* data, Bool isSelectable, Int32 icon, Bool isSelected );
	void AddItem( const String& itemName, void* data, const String& parentItemName, Bool isSelectable, Int32 icon, Bool isSelected );
	void* GetResult() { return m_result; }
	String FindNameForData( void* data ) const;
	int ShowModal( wxPoint* position );
	void EndModal( int retCode );
	Bool IsEmpty() const { return m_elements.Empty(); }

private:
	void RebuildTree( const String& filter );
	void SaveSelection();
	void RestoreSelection();

	void OnTreeKeyDown( wxTreeEvent& event );
	void OnTreeCharHook( wxKeyEvent& event );
	void OnSearchCtrlCharHook( wxKeyEvent& event );
	void OnTreeItemActivated( wxTreeEvent& event );
	void OnSearchTextChanged( wxCommandEvent& event );
	void OnSearchCancel( wxCommandEvent& event );
	void OnTimer( wxTimerEvent& event );

	virtual void SaveOptionsToConfig() override;
	virtual void LoadOptionsFromConfig() override;

	struct Element
	{
		String	m_path;
		void*	m_userData;
		Bool	m_selectable;
		Int32	m_iconIdx;

		Element() {} // needed for TSortedMap :(

		Element( const String& path, void* userData, Bool selectable, Int32 iconIdx )
			: m_path( path ), m_userData( userData ), m_selectable( selectable ), m_iconIdx( iconIdx )
		{}

		typedef TTreeItemDataWrapper< const Element* > Wrapper;
	};

	TSortedMap< String, Element > m_elements;
	wxTreeCtrl*   m_tree;
	wxSearchCtrl* m_searchCtrl;
	wxTimer*      m_searchTimer;
	String        m_lastSelectionPath;
	String        m_configPath;
	void*         m_result;
	Bool          m_storeFilter;
};
