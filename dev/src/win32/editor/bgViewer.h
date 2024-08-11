/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "detachablePanel.h"

class CEdBgViewer : public wxPanel
{
	wxDECLARE_CLASS( CEdBgViewer );

private:
	CWorld*						m_world;
	CEdRenderingPanel*			m_viewport;

	wxGauge*					m_gauge;
	wxStaticText*				m_text1;
	wxStaticText*				m_text2;
	wxStaticText*				m_text3;
	wxStaticText*				m_text4;
	wxStaticText*				m_text5;
	wxStaticText*				m_text6;
	wxStaticText*				m_text7;
	wxStaticText*				m_text8;
	wxStaticText*				m_text9;

	CEdDetachablePanel			m_detachablePanel;

public:
	CEdBgViewer( wxWindow* parent, CWorld* world, CEdRenderingPanel* viewport );
	virtual ~CEdBgViewer();

	void OnToolOpend();
	void OnToolClosed();

	Bool OnViewportTick( IViewport* view, Float timeDelta );

private:
	void RefreshWidgets();
};

//////////////////////////////////////////////////////////////////////////

class CEdBgViewerTool : public IEditorTool
{
	DECLARE_ENGINE_CLASS( CEdBgViewerTool, IEditorTool, 0 );

public:
	CWorld*						m_world;						//!< World shortcut
	CEdRenderingPanel*			m_viewport;						//!< Viewport shortcut
	CEdBgViewer*				m_toolPanel;

public:
	virtual String GetCaption() const { return TXT("Bg"); }

	virtual Bool Start( CEdRenderingPanel* viewport, CWorld* world, wxSizer* panelSizer, wxPanel* panel, const TDynArray< CComponent* >& selection );
	virtual void End();	

	virtual Bool OnViewportTick( IViewport* view, Float timeDelta );
};

BEGIN_CLASS_RTTI( CEdBgViewerTool );
	PARENT_CLASS( IEditorTool );
END_CLASS_RTTI();
