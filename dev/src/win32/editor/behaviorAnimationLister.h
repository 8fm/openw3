/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "behaviorEditorPanel.h"

class CEdBehaviorGraphAnimationLister : public CEdBehaviorEditorSimplePanel
{
	DECLARE_EVENT_TABLE()

	wxListCtrl* m_list;

	static const Uint32 LIST_ANIM_NAME = 0;

public:
	CEdBehaviorGraphAnimationLister( CEdBehaviorEditor* editor );

	virtual wxString	GetPanelName() const	{ return wxT("Animation Lister"); }
	virtual wxString	GetPanelCaption() const { return wxT("Animation Lister"); }
	virtual wxString	GetInfo() const			{ return wxT("Animation Lister"); }
	wxAuiPaneInfo		GetPaneInfo() const;

	virtual void OnReset();
	virtual void OnInstanceReload();
	virtual void OnPanelClose();

protected:
	void OnListAnims( wxCommandEvent& event );
	void OnCopyToClipboard( wxCommandEvent& event );

protected:
	void Clear();

	void FillList();
};
