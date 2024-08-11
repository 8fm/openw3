/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../../common/engine/staticCamera.h"

#include "detachablePanel.h"

class CEdStaticCameraToolPanel	: public CEdDraggablePanel
								, public IStaticCameraListener
								, public IPreviewItemContainer
{
	wxDECLARE_CLASS( CEdStaticCameraToolPanel );

private:
	struct MultiTest
	{
		Bool			m_running;
		Bool			m_wasPreview;
		size_t			m_currNum;
		wxArrayString	m_cameras;
		CStaticCamera*	m_previewCameraForBlending;
		THandle< CStaticCamera > m_callbackCameraDeact;
		THandle< CStaticCamera > m_callbackCameraAct;

		MultiTest() { Reset(); }

		void Reset()
		{
			m_running = false;
			m_wasPreview = false;
			m_currNum = 0;
			m_cameras.Clear();
			m_previewCameraForBlending = NULL;
			m_callbackCameraDeact = NULL;
			m_callbackCameraAct = NULL;
		}
	};

private:
	CWorld*					m_world;
	CEdRenderingPanel*		m_viewport;
	CEdPropertiesPage*		m_prop;
	Bool					m_preview;
	Bool					m_autoPreviewInTests;
	Bool					m_init;

	CStaticCamera*			m_camera;

	CEntity*				m_previewPoint;
	Bool					m_cachedBehRenderState;
	Bool					m_cachedVDRenderState;
	Vector					m_clickedPosWS;

	MultiTest				m_multiTest;

	CEdDetachablePanel		m_detachablePanel;

	static const String		PREVIEW_POINT_NAME;

public:
	CEdStaticCameraToolPanel( wxWindow* parent, CWorld* world, CEdRenderingPanel* viewport );
	virtual ~CEdStaticCameraToolPanel();

	void OnToolClosed();

	Bool HandleSelection( const TDynArray< CHitProxyObject* >& objects );
	Bool HandleContextMenu( Int32 x, Int32 y, const Vector& collision );
	Bool OnViewportCalculateCamera( IViewport* view, CRenderCamera &camera );
	Bool OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame );
	Bool OnViewportTick( IViewport* view, Float timeDelta );
	Bool OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data );

	void SelectCamera( CStaticCamera* camera );

public: //IStaticCameraListener
	virtual void OnRunStart( const CStaticCamera* camera );
	virtual void OnRunEnd( const CStaticCamera* camera );
	virtual void OnActivationFinished( const CStaticCamera* camera );
	virtual void OnDeactivationStarted( const CStaticCamera* camera );

public: // IPreviewItemContainer
	virtual CWorld* GetPreviewItemWorld() const { return m_world; }

private:
	CStaticCamera* CreateStaticCamera( const String& name, const Vector& pos, CLayer* layer ) const;
	CStaticCamera* CreateStaticCameraFromView( const String& name, Bool onDynamicLayer = false ) const;
	void CreatePreviewPoint();
	void DestroyPreviewPoint();
	CStaticCamera* GetCameraByName( const String& name ) const;
	CLayer* GetEditorLayer() const;

	void DeselectCamera();
	void CollectCameras( TDynArray< CStaticCamera* >& cameras ) const;
	String GetNameForCamera() const;
	void FreezeCameraTool( Bool flag );

	void SetPreview( Bool flag );
	Bool IsPreview() const;
	Bool IsAutoPreviewInTests() const;

	void FillCameraList();
	void ValidateCameraTestList();

	CStaticCamera* PlayNextTestCamera();
	void RefreshCameraData();
	void RefreshTestedCameraLabel( const String& text );
	void EnableTestNextButton( Bool flag );
	
	Bool ShouldAddPreviewCamera() const;

	void StartTest( Bool multi, Bool ui = true );
	void EndTest( Bool ui = true );

	CStaticCamera* GetCurrentCamera() const;

protected:
	void OnCreateCamera( wxCommandEvent& event );
	void OnTeleportToCamera( wxCommandEvent& event );
	void OnPreview( wxCommandEvent& event );
	void OnAutoPreview( wxCommandEvent& event );
	void OnCameraChoice( wxCommandEvent& event );
	void OnCreateFromView( wxCommandEvent& event );
	void OnQuickTestActivate( wxCommandEvent& event );
	void OnQuickTestDeactivate( wxCommandEvent& event );
	void OnQuickTestNext( wxCommandEvent& event );
	void OnCameraTestAdd( wxCommandEvent& event );
	void OnCameraTestRemove( wxCommandEvent& event );
};

//////////////////////////////////////////////////////////////////////////

class CEdStaticCameraTool : public IEditorTool
{
	DECLARE_ENGINE_CLASS( CEdStaticCameraTool, IEditorTool, 0 );

public:
	CWorld*								m_world;						//!< World shortcut
	CEdRenderingPanel*					m_viewport;						//!< Viewport shortcut
	CEdStaticCameraToolPanel*			m_toolPanel;

public:
	virtual String GetCaption() const { return TXT("Camera"); }

	virtual Bool Start( CEdRenderingPanel* viewport, CWorld* world, wxSizer* panelSizer, wxPanel* panel, const TDynArray< CComponent* >& selection );
	virtual void End();	

	virtual Bool HandleSelection( const TDynArray< CHitProxyObject* >& objects );
	virtual Bool HandleContextMenu( Int32 x, Int32 y, const Vector& collision );
	virtual Bool OnViewportCalculateCamera( IViewport* view, CRenderCamera &camera );
	virtual Bool OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame );
	virtual Bool OnViewportTick( IViewport* view, Float timeDelta );
	virtual Bool OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data );
};

BEGIN_CLASS_RTTI( CEdStaticCameraTool );
	PARENT_CLASS( IEditorTool );
END_CLASS_RTTI();
