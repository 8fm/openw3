/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

/// A dialog to select a layer group
class CEdLayerGroupList : public wxDialog
{
	DECLARE_EVENT_TABLE();

protected:
	CLayerGroup*				m_baseGroup;
	TDynArray< CLayerGroup* >	m_selectedGroups;

public:
	//! Get base group
	RED_INLINE CLayerGroup* GetBaseGroup() const { return m_baseGroup; }

	//! Get selected group
	RED_INLINE const TDynArray< CLayerGroup* >& GetSelectedGroups() const { return m_selectedGroups; }

public:
	CEdLayerGroupList( wxWindow* parent, CLayerGroup* baseGroup, const TDynArray< String >& currentLayerPaths );

protected:
	void OnOK( wxCommandEvent& event );
	void OnClearAll( wxCommandEvent& event );
	void OnFillChildren( wxCommandEvent& event );
	void OnExpandTree( wxCommandEvent& event );
	void OnCancel( wxCommandEvent& event );
	void OnItemActivated( wxMouseEvent& event );
	void OnRemoveLayer( wxListEvent& event );

protected:
	void AppendTreeItems( wxTreeCtrl* tree, wxTreeItemId parent, CLayerGroup* group );
	void UpdateTreeItem( wxTreeCtrl* tree, wxTreeItemId item );
	void UpdateListOfSelectedGroups();
	void UpdateTreeIcons();
	Uint32 GetItemIcon( CLayerGroup* group );
};


