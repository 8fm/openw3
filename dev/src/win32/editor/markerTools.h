/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#ifndef NO_MARKER_SYSTEMS

#include "detachablePanel.h"

class CEdPOIPanel;
class CEdReviewPanel;
class CEdStickersPanel;
class CEdGeneralMarkersPanel;

class CEdMarkersEditor : public IEditorTool
{
	DECLARE_ENGINE_CLASS( CEdMarkersEditor, IEditorTool, 0 );

public:
	CEdMarkersEditor(void) : m_markersPanel(NULL) { /*intentionally empty*/ };
	~CEdMarkersEditor(void) { /*intentionally empty*/ };

	// Edit mode control
	virtual String GetCaption() const { return TXT("Markers"); };
	virtual Bool Start( CEdRenderingPanel* viewport, CWorld* world, wxSizer* m_panelSizer, wxPanel* panel, const TDynArray< CComponent* >& selection );
	virtual void End();
	void OpenPOIsEditor( const String& name );

private:
	CEdGeneralMarkersPanel*	m_markersPanel;
};

BEGIN_CLASS_RTTI( CEdMarkersEditor );
	PARENT_CLASS( IEditorTool );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////
// CEdGeneralMarkersPanel

class CEdGeneralMarkersPanel : public CEdDraggablePanel, public IEdEventListener
{
public:	
	CEdGeneralMarkersPanel(wxWindow* parent);
	~CEdGeneralMarkersPanel();

	// event listener
	void DispatchEditorEvent( const CName& name, IEdEventData* data );
	void OpenPOIsEditor( const String& name );

private:
	void OnSelectTab( wxCommandEvent& event );

	wxNotebook*			m_mainNotebook;

	// marker tools
	CEdReviewPanel*		m_reviewFlagTool;
	CEdPOIPanel*		m_poiTool;
	CEdStickersPanel*	m_stickerTool;

	CEdDetachablePanel	m_detachablePanel;
};

#endif
