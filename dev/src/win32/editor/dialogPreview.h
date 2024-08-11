/**
* Copyright c 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "previewPanel.h"
#include "skeletonPreview.h"
#include "dialogEditorPage.h"
#include "../../common/game/inventoryEditor.h"

#include "../../common/game/storySceneDirector.h"
#include "../../common/game/storyScenePlayerInterface.h"

#include "../../common/game/storySceneEvent.h"
#include "../../common/game/storySceneEventLookAt.h"

#include "../../common/game/storySceneDisplayInterface.h"

class CEdDialogTimeline;
class CStorySceneSection;
class CEdSceneEditor;

enum EScenePreviewAction
{
	SPA_NONE,
	SPA_PLAY,
	SPA_DRAG,
	SPA_STOP,
	SPA_CONTINUE,
};

enum EEditStringId
{
	EDIT_Default = 0,
	EDIT_ControlRig,

	EDIT_MAX
};

class CEdDialogPreview	: public CEdPreviewPanel
						, public IEdEventListener
						, public ISkeletonPreview
{
	DECLARE_EVENT_TABLE();

private:
	CEdSceneEditor*								m_mediator;
	wxSlider*									m_timeSlider;
	Float										m_timePausePrevState;
	Bool										m_timePause;

	wxToolBar*									m_previewToolbar;
	Int32										m_previewCameraPreviewTool;
	Int32										m_previewCameraFreeTool;
	Int32										m_previewCameraEditTool;

	Bool										m_isFullscreen;
	Bool										m_trackLight;

	Float										m_initCameraRotFactor;
	Float										m_initCameraPosFactor;
	Float										m_cameraRotMul;
	Float										m_cameraPosMul;

	CEntity*									m_lightEntity;

	String										m_editString[ EDIT_MAX ];	

public:
	CEdDialogPreview( wxWindow* parent, CEdSceneEditor* dialogEditor );
	virtual ~CEdDialogPreview();

public: // IEdEventListener
	virtual void			DispatchEditorEvent( const CName& name, IEdEventData* data ) override;

public: // CEdRenderingPanel
	virtual void			OnViewportTick( IViewport* view, Float timeDelta ) override;
	virtual void			OnViewportCalculateCamera( IViewport* view, CRenderCamera &camera ) override;
	virtual void			OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame ) override;
	virtual Bool			OnViewportClick( IViewport* view, Int32 button, Bool state, Int32 x, Int32 y ) override;
	virtual CRenderFrame*	OnViewportCreateFrame( IViewport *view ) override;
	virtual void			OnViewportRenderFrame( IViewport *view, CRenderFrame *frame ) override;
	virtual Bool			OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data ) override;

	wxToolBar*				GetToolbar()		{ return m_previewToolbar; }
	void					SetupToolbarEvents( wxToolBar* toolbar );

	virtual void			HandleContextMenu( Int32 x, Int32 y ) override;
	virtual void			HandleSelection( const TDynArray< CHitProxyObject* >& objects ) override;

	void	ToggleTimePause();

protected:
	Float GetTimeMul() const;

	void CreateExtraLight();
	void CreateTransformWidgets();

	void GenerateFragmentsForCurrentCamera( IViewport *view, CRenderFrame *frame );

public: // ISkeletonPreview
	virtual Bool IsSkeleton()  { return true; }
	virtual Bool IsBoneNames() { return true; }
	virtual Bool IsBoneAxis() { return false; }	

	void SetEditString( const String& txt, EEditStringId strid = EDIT_Default );

	void TrackSun( Bool flag ) { m_trackLight = flag; }
	Bool IsTrackSunEnabled() const { return m_trackLight; }

protected:
	void OnCutsceneButton( wxCommandEvent& event );
	void OnLoadWorldButton( wxCommandEvent& event );
	void OnLoadWorldButton2( wxCommandEvent& event );
	void OnFullscreenButton( wxCommandEvent& event );

	void OnCreateCustomCameraButton( wxCommandEvent& event );
	void OnFreeCameraModeButton( wxCommandEvent& event );
	void OnPreviewModeButton( wxCommandEvent& event );
	void OnEditModeButton( wxCommandEvent& event );
	void ToggleTimePauseBtn( wxCommandEvent& event );
	void OnAddLight( wxCommandEvent& event );

public:
	virtual void OnCameraMoved();

	void ToggleCameraTool( Bool previewMode, Bool freeMode, Bool editMode );

	void SlowDownCameraInputs( Bool enable );
};

RED_DECLARE_NAME( StorySceneOld );
RED_DECLARE_NAME( CustomCamera );
