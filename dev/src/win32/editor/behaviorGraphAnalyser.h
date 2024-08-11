/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "behaviorEditorPanel.h"

class CEdBehaviorInstanceProfiler : public CEdBehaviorEditorSimplePanel
{
	DECLARE_EVENT_TABLE()

	Bool		m_connect;

public:
	CEdBehaviorInstanceProfiler( CEdBehaviorEditor* editor );

	virtual wxString	GetPanelName() const	{ return wxT("Instance"); }
	virtual wxString	GetPanelCaption() const { return wxT("Graph instance profiler"); }
	virtual wxString	GetInfo() const			{ return wxT("Graph runtime ( instance ) data"); }
	wxAuiPaneInfo		GetPaneInfo() const;

	virtual void OnReset();
	virtual void OnInstanceReload();
	virtual void OnTick( Float dt );

protected:
	void OnConnect( wxCommandEvent& event );

protected:
	void Connect( Bool flag );
	void RefreshPanel();
};

//////////////////////////////////////////////////////////////////////////
