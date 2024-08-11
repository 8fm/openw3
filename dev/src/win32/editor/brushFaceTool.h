/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "detachablePanel.h"

class CBrushFaceToolPanel;

// Tool
class CEdBrushFaceEdit : public IEditorTool
{
	DECLARE_ENGINE_CLASS( CEdBrushFaceEdit, IEditorTool, 0 );

public:
	CWorld*						m_world;
	CBrushFaceToolPanel*		m_panel;
	TDynArray< CBrushFace* >	m_selectedFaces;
	CEdRenderingPanel*			m_viewport;

public:
	CEdBrushFaceEdit();
	virtual String GetCaption() const;

	// IEditorTool stuff
	virtual Bool Start( CEdRenderingPanel* viewport, CWorld* world, wxSizer* m_panelSizer, wxPanel* panel, const TDynArray< CComponent* >& selection );
	virtual void End();	
	virtual Bool HandleSelection( const TDynArray< CHitProxyObject* >& objects );
	virtual Bool HandleActionClick( Int32 x, Int32 y );
	virtual Bool HandleContextMenu( Int32 x, Int32 y, const Vector& collision );
	virtual Bool OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame );
	virtual Bool OnViewportClick( IViewport* view, Int32 button, Bool state, Int32 x, Int32 y );
	virtual Bool OnViewportMouseMove( const CMousePacket& packet );
	virtual Bool OnViewportTrack( const CMousePacket& packet );
	virtual Bool OnViewportTick( IViewport* view, Float timeDelta );
	virtual Bool OnDelete();

protected:
	void DeselectAllFaces();
	void SelectFace( CBrushFace* face );
	void DeselectFace( CBrushFace* face );
};

BEGIN_CLASS_RTTI( CEdBrushFaceEdit )
	PARENT_CLASS( IEditorTool );
END_CLASS_RTTI();

// Panel
class CBrushFaceToolPanel : public CEdDraggablePanel
{
	wxDECLARE_CLASS( CBrushFaceToolPanel );

private:
	CEdDetachablePanel						m_detachablePanel;

protected:
	CEdBrushFaceEdit*						m_tool;
	CEdPropertiesBrowserWithStatusbar*		m_properties;

public:
	CBrushFaceToolPanel( wxWindow* parent, CEdBrushFaceEdit* tool );

	void UpdateFaceProperties();
	void OnPropertiesUpdated( wxCommandEvent& event );
};
