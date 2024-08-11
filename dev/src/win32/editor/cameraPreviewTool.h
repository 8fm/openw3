/**
* Copyright © 2011 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "detachablePanel.h"

class CEdCameraPreviewToolPanel;

class CEdCameraPreviewTool : public IEditorTool
{
	DECLARE_ENGINE_CLASS( CEdCameraPreviewTool, IEditorTool, 0 );

public:
	Bool				            m_moveKeys[7];
    EulerAngles                     m_rotation;
    EulerAngles                     m_cameraRotation;
	CWorld*				            m_world;
	CEdRenderingPanel*	            m_viewport;
	CCamera*			            m_previewCamera;
	CEntity*			            m_previewEntity;
    CMovingPhysicalAgentComponent*	m_movementComponent;
    CMeshComponent*                 m_meshComponent;
    CEdCameraPreviewToolPanel*      m_panel;

    float                           m_defaultFOV;

public:
	CEdCameraPreviewTool();
	virtual String GetCaption() const { return TXT("Camera preview"); };
    virtual void GetDefaultAccelerator( Int32 &flags, Int32 &key ) const;
	virtual Bool Start( CEdRenderingPanel* viewport, CWorld* world, wxSizer* m_panelSizer, wxPanel* panel, const TDynArray< CComponent* >& selection );
	virtual void End();	

	virtual Bool UsableInActiveWorldOnly() const { return true; }

	virtual Bool OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data );
	virtual Bool OnViewportCalculateCamera( IViewport* view, CRenderCamera &camera );
	virtual Bool OnViewportTick( IViewport* view, Float timeDelta );
};

BEGIN_CLASS_RTTI( CEdCameraPreviewTool );
	PARENT_CLASS( IEditorTool );
END_CLASS_RTTI();

/// Navigation mesh tool panel
class CEdCameraPreviewToolPanel : public CEdDraggablePanel
{
   wxDECLARE_EVENT_TABLE();
   wxDECLARE_CLASS( CEdCameraPreviewToolPanel );

private:
    CEdCameraPreviewTool*   m_tool;
	CWorld*                 m_world;

    wxSpinCtrl*             m_spinCtrl;

	CEdDetachablePanel		m_detachablePanel;

public:
    CEdCameraPreviewToolPanel( CEdCameraPreviewTool* tool, wxWindow* parent, CWorld* world );

    void OnResetFOV( wxCommandEvent &event );
    void OnChangeFOV( wxSpinEvent &event );
    void OnCheckNavMesh( wxCommandEvent &event );
    void OnCheckMesh( wxCommandEvent &event );
};
