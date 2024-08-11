/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

/// A dialog to select a layer group
class CEdLayerGroupTree : public wxDialog
{
	DECLARE_EVENT_TABLE();

protected:
	CLayerGroup*		m_baseGroup;
	CLayerGroup*		m_selectedGroup;

public:
	//! Get base group
	RED_INLINE CLayerGroup* GetBaseGroup() const { return m_baseGroup; }

	//! Get selected group
	RED_INLINE CLayerGroup* GetSelectedGroup() const { return m_selectedGroup; }

public:
	CEdLayerGroupTree( wxWindow* parent, CLayerGroup* baseGroup, const String& currentPath );

protected:
	void OnOK( wxCommandEvent& event );
	void OnNULL( wxCommandEvent& event );
	void OnCancel( wxCommandEvent& event );
	void OnItemSelected( wxTreeEvent& event );
	void OnItemActivated( wxTreeEvent& event );

protected:
	void AppendTreeItems( wxTreeCtrl* tree, wxTreeItemId parent, CLayerGroup* group, CLayerGroup* groupToSelect );
	CLayerGroup* GetSelectedLayerGroup();
};


