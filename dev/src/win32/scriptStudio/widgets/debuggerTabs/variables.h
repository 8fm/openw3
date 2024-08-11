/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef __VARIABLES_DEBUGGER_TAB_BASE_H__
#define __VARIABLES_DEBUGGER_TAB_BASE_H__

#include "base.h"

#include <set>

#include "../../debuggerLocalsHelper.h"

// Base class for watch/locals type windows
class CSSVariablesTabBase : public CSSDebuggerTabBase
{
	wxDECLARE_CLASS( CSSVariablesTabBase );

public:
	CSSVariablesTabBase( wxAuiNotebook* parent );
	virtual ~CSSVariablesTabBase();

	virtual void Connect( const wxChar* ip ) override final;
	virtual void CopyToClipboard() override final;
	virtual void DebuggingStopped() override final;

protected:

	static const wxColour UNVERIFIED_COLOUR;
	static const wxColour MODIFIED_COLOUR;
	static const wxColour NORMAL_COLOUR;
	static const wxColour MODIFYACK_FAILED_COLOUR;

	enum eColumn
	{
		Col_Name,
		Col_Value
	};

	class CSSVariablesTabItemData : public wxTreeItemData
	{
	public:
		wxString path;
		Red::System::Uint32 id;
		Red::System::Bool modifiable;

		CSSVariablesTabItemData()
		:	id( idPool++ )
		{
		}
	
	private:
		static Red::System::Uint32 idPool;
	};

	void Clear();
	virtual bool HasFocus() const override;

	wxTreeItemId Search( const wxString& searchTerm ) const;
	wxTreeItemId Search( const wxTreeItemId& startingItem, const wxString& searchTerm ) const;
	bool ItemContainsSearchTerm( const wxTreeItemId& item, const wxString& searchTerm ) const;

	void SelectAndEnsureVisible( const wxTreeItemId& item );

	int CalculatePage() const;
	void MoveSelection( int numItems, bool up = false );

	void OnQuickSearchKeyDown( wxKeyEvent& event );
	void OnQuickSearchTextChanged( wxCommandEvent& event );

	wxTreeItemId CreateNewItem( wxString& path );
	wxTreeItemId CreateNewItem( const wxTreeItemId& parent, wxString& path );
	wxTreeItemId CreateNewItem( const wxTreeItemId& parent, wxString& path, const wxString& childName );
	wxTreeItemId ForceCreateNewItem( const wxTreeItemId& parent, wxString& path, const wxString& childName );

	void RefreshItem( const wxTreeItemId& item );
	void SetItem( const wxTreeItemId& item, const SDebugValue& property );

	void CopyToClipboard( const wxString& text );

	void AddMembersToVar( const wxTreeItemId& parent, Red::System::Uint32 numChildren, const SDebugValueChild* children, const wxString& parentPath, std::set< wxTreeItemId >& untouchedChildren );
	void OnLocalsEvent( CLocalsEvent& event );
	void OnItemExpand( wxTreeEvent& event );
	void OnItemCollapse( wxTreeEvent& event );
	void OnItemDeleted( wxTreeEvent& event );
	void GetItemChildren( const wxTreeItemId& parent, std::set< wxTreeItemId >& children ) const;
	void OnSizeEvent( wxSizeEvent& event );
	virtual void OnKeyDown( wxTreeEvent& event );

	virtual Red::System::Uint32 GetItemExpansionStamp() = 0;

	void OnDoubleClick( wxMouseEvent& event );
	virtual void OnUserCreateNewItem();
	virtual void OnUserStartEditItemName( const wxTreeItemId& item );
	virtual void OnUserStartEditItemValue( const wxTreeItemId& item );
	void OnLocalsModifyAckEvent( CLocalsModificationAckEvent& event );
	void OnStartLabelEdit( wxTreeEvent& event );
	void OnEndLabelEdit( wxTreeEvent& event );

	wxcode::wxTreeListCtrl* m_tree;
	wxTextCtrl* m_quickSearchCtrl;

	CDebuggerLocalsHelper m_helper;

	typedef map< Red::System::Uint32, wxTreeItemId > TIdToItemMap;
	typedef map< wxString, wxTreeItemId > TPathToItemMap;
	typedef map< Red::System::Uint32, TPathToItemMap > TParentIdToChildPaths;

	TIdToItemMap m_idsToItems;
	TParentIdToChildPaths m_childrenMap;

	Red::System::Uint32 m_currentStackFrameIndex;
	Red::System::Uint32 m_stamp;

	wxString m_originalValue;
	wxTreeItemId m_modifiedItem;

	mutable wxString m_previousSearchTerm;
	mutable Red::System::Uint32 m_searchSkip;
	mutable Red::System::Uint32 m_searchSkipTarget;

	static const Red::System::Uint32 INVALID_STAMP = 0x696E7664;
};

#endif // __VARIABLES_DEBUGGER_TAB_BASE_H__
