/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma once 

#include "detachablePanel.h"

class CEdSceneExporterToolPanel;
struct CLMFMeshFile;

/// Editor tool for navigation meshes
class CEdSceneExporterTool : public IEditorTool
{
	DECLARE_ENGINE_CLASS( CEdSceneExporterTool, IEditorTool, 0 );

protected:
	CEdSceneExporterToolPanel*		m_panel;

public:
	CEdSceneExporterTool();
	~CEdSceneExporterTool();

	virtual String GetCaption() const;
	virtual Bool Start( CEdRenderingPanel* viewport, CWorld* world, wxSizer* m_panelSizer, wxPanel* panel, const TDynArray< CComponent* >& selection );
	virtual void End();
};

BEGIN_CLASS_RTTI( CEdSceneExporterTool );
	PARENT_CLASS( IEditorTool );
END_CLASS_RTTI();

/// Navigation mesh tool panel
class CEdSceneExporterToolPanel : public CEdDraggablePanel
{
	wxDECLARE_CLASS( CEdSceneExporterToolPanel );
	wxDECLARE_EVENT_TABLE();

private:
	CEdDetachablePanel				m_detachablePanel;

protected:
	CEdSceneExporterTool*			m_tool;
	CEdFileDialog					m_exportFile;

public:
	CEdSceneExporterToolPanel( CEdSceneExporterTool* tool, wxWindow* parent, const String& sceneFileName );
	~CEdSceneExporterToolPanel();
	
protected:
	void OnExportScene( wxCommandEvent &event );
};
