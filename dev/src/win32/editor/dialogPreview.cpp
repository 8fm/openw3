/**
* Copyright c 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "dialogPreview.h"

#include "dialogEditor.h"
#include "dialogTimeline.h"
#include "dialogEditorSection.h"
#include "dialogEditorChangeSlotPropertyEditor.h"
#include "dialogEditorActions.h"
#include "dialogEditorUtils.h"
#include "viewportWidgetMoveAxis.h"
#include "viewportWidgetRotateAxis.h"
#include "viewportWidgetScaleAxis.h"
#include "controlRigPanel.h"

#include "../../common/game/actor.h"
#include "../../common/game/actorSpeech.h"
#include "../../common/game/storySceneElement.h"
#include "../../common/game/storySceneEvent.h"
#include "../../common/game/storySceneEventCustomCamera.h"
#include "../../common/game/storyScene.h"
#include "../../common/game/storySceneSection.h"
#include "../../common/game/storySceneLine.h"
#include "../../common/game/storySceneChoice.h"
#include "../../common/game/storySceneEventLookAt.h"
#include "../../common/game/storySceneEventEnterActor.h"
#include "../../common/game/storySceneEventExitActor.h"
#include "../../common/game/storySceneEventAnimation.h"
#include "../../common/game/storySceneEventMimics.h"
#include "../../common/game/storySceneEventMimicsAnim.h"
#include "../../common/game/storySceneEventRotate.h"
#include "../../common/game/storySceneEventCustomCameraInstance.h"
#include "../../common/game/storySceneCutsceneSection.h"
#include "../../common/game/storySceneEventSound.h"
#include "../../common/game/storySceneWaypointComponent.h"
#include "../../common/game/storySceneEventChangePose.h"
#include "../../common/game/storySceneEventMimicFilter.h"
#include "../../common/game/storySceneEventMimicPose.h"
#include "../../common/game/sceneLog.h"

#include "../../common/game/movingPhysicalAgentComponent.h"
#include "../../common/engine/cameraComponent.h"
#include "../../common/engine/behaviorGraphAnimationManualSlot.h"
#include "../../common/core/gatheredResource.h"
#include "../../common/engine/soundSystem.h"
#include "../../common/engine/component.h"
#include "../../common/engine/hitProxyObject.h"
#include "../../common/engine/viewport.h"
#include "../../common/engine/dynamicLayer.h"
#include "../../common/engine/pointLightComponent.h"
#include "../../common/engine/renderFrame.h"
#include "../../common/engine/fonts.h"

enum
{
	ID_TOGGLE_TIME = 4000,
	ID_ADD_LIGHT
};

BEGIN_EVENT_TABLE( CEdDialogPreview, wxPanel )
	EVT_BUTTON( ID_TOGGLE_TIME, CEdDialogPreview::ToggleTimePauseBtn )
	EVT_MENU( ID_ADD_LIGHT, CEdDialogPreview::OnAddLight )
END_EVENT_TABLE();

CGatheredResource resDialogPreviewFont( TXT("gameplay\\gui\\fonts\\arial.w2fnt"), RGF_Startup );
CGatheredResource resScenePreviewDefFont( TXT("gameplay\\gui\\fonts\\arial.w2fnt"), RGF_Startup );
CGatheredResource resScenePreviewDefSmallFont( TXT("gameplay\\gui\\fonts\\aldo_pc_15.w2fnt"), RGF_Startup );

RED_DEFINE_NAME( StorySceneOld );
RED_DEFINE_NAME( CustomCamera );

#ifdef DEBUG_SCENES
#pragma optimize("",off)
#endif

CEdDialogPreview::CEdDialogPreview( wxWindow* parent, CEdSceneEditor* dialogEditor )
	: CEdPreviewPanel( parent, true )
	, m_isFullscreen( false )
	, m_mediator( dialogEditor )
	, m_lightEntity( NULL )
	, m_timePausePrevState( 0.f )
	, m_timePause( false )
	, m_trackLight( true )
{
	// Connect preview toolbar
	{
		m_previewToolbar = XRCCTRL( *dialogEditor, "PreviewControlToolbar", wxToolBar );
		SetupToolbarEvents( m_previewToolbar );

		m_previewCameraPreviewTool = XRCID( "PreviewModeButton");
		m_previewCameraFreeTool = XRCID( "FreeCameraModeButton");
		m_previewCameraEditTool = XRCID( "EditModeButton");
	}
	
	// Add time slider
	{
		wxBoxSizer* bSizer2 = new wxBoxSizer( wxHORIZONTAL );

		m_timeSlider = new wxSlider( this, wxID_ANY, 100, 0, 100 );
		m_timeSlider->SetToolTip( wxT("Time mul") );
		bSizer2->Add( m_timeSlider, 1, wxALL, 5 );

		wxBitmapButton* bpButton1 = new wxBitmapButton( this, ID_TOGGLE_TIME, SEdResources::GetInstance().LoadBitmap( TXT("IMG_FSKELETON") ) );
		bpButton1->SetToolTip( wxT("Freeze all ('p'-key)") );
		bSizer2->Add( bpButton1, 0, wxALIGN_CENTER, 5 );

		m_sizer->Add( bSizer2, 0, wxEXPAND, 5 );
	}

	// Camera input factors
	{
		m_initCameraRotFactor = m_cameraInputRotMul;
		m_initCameraPosFactor = m_cameraInputPosMul;
		m_cameraRotMul = 0.5f;
		m_cameraPosMul = 0.5f;
	}

	CreateExtraLight();

	CreateTransformWidgets();

	SEvents::GetInstance().RegisterListener( CNAME( SelectionChanged ), this );
	GetSelectionManager()->SetGranularity( CSelectionManager::SG_Entities );

	GetViewport()->SetRenderingMask( SHOW_Gizmo );
	GetViewport()->SetRenderingMask( SHOW_CurveAnimations );
}

CEdDialogPreview::~CEdDialogPreview()
{
	SEvents::GetInstance().UnregisterListener( this );

	GSoundSystem->SetListenerVectorsManually();
}

void CEdDialogPreview::CreateTransformWidgets()
{
	m_widgetManager->SetWidgetSpace( RPWS_Local );
	m_widgetManager->SetWidgetMode( RPWM_Move );
	
	// Rotation widgets

	const Float rotateGismoSize = 0.5f;

	m_widgetManager->AddWidget( TXT("Rotate"), new CViewportWidgetRotateAxis( EulerAngles( 0, 1, 0 ), Vector::EX, Color::RED, rotateGismoSize ) );
	m_widgetManager->AddWidget( TXT("Rotate"), new CViewportWidgetRotateAxis( EulerAngles( 1, 0, 0 ), Vector::EY, Color::GREEN, rotateGismoSize ) );
	m_widgetManager->AddWidget( TXT("Rotate"), new CViewportWidgetRotateAxis( EulerAngles( 0, 0, 1 ), Vector::EZ, Color::BLUE, rotateGismoSize ) );

	// Translation widgets

	m_widgetManager->AddWidget( TXT("Move"), new CViewportWidgetMoveAxis( Vector::EX, Color::RED ) );
	m_widgetManager->AddWidget( TXT("Move"), new CViewportWidgetMoveAxis( Vector::EY, Color::GREEN ) );
	m_widgetManager->AddWidget( TXT("Move"), new CViewportWidgetMoveAxis( Vector::EZ, Color::BLUE ) );
	m_widgetManager->AddWidget( TXT("Move"), new CViewportWidgetMovePlane( Vector::EY, Vector::EZ, Color::LIGHT_RED ) );
	m_widgetManager->AddWidget( TXT("Move"), new CViewportWidgetMovePlane( Vector::EX, Vector::EZ, Color::LIGHT_GREEN ) );
	m_widgetManager->AddWidget( TXT("Move"), new CViewportWidgetMovePlane( Vector::EX, Vector::EY, Color::LIGHT_BLUE ) );
}

void CEdDialogPreview::SetupToolbarEvents( wxToolBar* toolbar )
{
	toolbar->Connect( XRCID( "CutsceneButton"), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdDialogPreview::OnCutsceneButton ), NULL, this );

	toolbar->Connect( XRCID( "LoadWorldButton"), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdDialogPreview::OnLoadWorldButton ), NULL, this );
	toolbar->Connect( XRCID( "LoadWorldButton2"), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdDialogPreview::OnLoadWorldButton2 ), NULL, this );
	toolbar->Connect( XRCID( "FullscreenButton"), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdDialogPreview::OnFullscreenButton ), NULL, this );
	toolbar->Connect( XRCID( "CreateCustomCameraInstance"), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdDialogPreview::OnCreateCustomCameraButton ), NULL, this );

	toolbar->Connect( XRCID( "FreeCameraModeButton"), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdDialogPreview::OnFreeCameraModeButton ), NULL, this );	
	toolbar->Connect( XRCID( "PreviewModeButton"), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdDialogPreview::OnPreviewModeButton ), NULL, this );
	toolbar->Connect( XRCID( "EditModeButton"), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdDialogPreview::OnEditModeButton ), NULL, this );
}

void CEdDialogPreview::DispatchEditorEvent( const CName& name, IEdEventData* data )
{
	if ( name == CNAME( SelectionChanged ) )
	{		
		const CSelectionManager::SSelectionEventData& eventData = GetEventData< CSelectionManager::SSelectionEventData >( data );
		CWorld * world = eventData.m_world;
		
		if ( eventData.m_world == GetWorld() )
		{
			m_widgetManager->EnableWidgets( GetSelectionManager()->GetSelectionCount() > 0 );
		}		
	}
}

void CEdDialogPreview::HandleContextMenu( Int32 x, Int32 y )
{
	wxMenu menu;
	menu.Append( ID_ADD_LIGHT, TXT("Add light") );
	PopupMenu( &menu, x, y );
}

Float CEdDialogPreview::GetTimeMul() const
{
	return (Float)m_timeSlider->GetValue() / 100.f;
}

void CEdDialogPreview::ToggleTimePause()
{
	if ( m_timePause )
	{
		m_timeSlider->SetValue( (Int32)(m_timePausePrevState*100.f) );
		m_timePause = false;
	}
	else
	{
		m_timePausePrevState = GetTimeMul();
		m_timeSlider->SetValue( 0 );
		m_timePause = true;
	}
}

void CEdDialogPreview::OnViewportTick( IViewport* view, Float timeDelta )
{	
#ifdef PROFILE_DIALOG_EDITOR_TICK
	CTimeCounter timer;
#endif

	timeDelta *= GetTimeMul();

	CEdPreviewPanel::OnViewportTick( view, timeDelta );
	
	m_mediator->OnPreview_ViewportTick( timeDelta );

	SItemEntityManager::GetInstance().OnTick( timeDelta );

#ifdef PROFILE_DIALOG_EDITOR_TICK
	const Float timeElapsed = timer.GetTimePeriodMS();
	SCENE_LOG( TXT("CEdDialogPreview::OnViewportTick - %1.3f"), timeElapsed );
#endif
}

void CEdDialogPreview::OnViewportCalculateCamera( IViewport* view, CRenderCamera &camera )
{
	if ( view->GetHeight() <= 1 || view->GetWidth() <= 1 || IsShown() == false )
	{
		return;
	}

	// Adjust viewport aspect ratio
	view->AdjustSizeWithCachets( wxTheFrame->GetWorldEditPanel()->GetViewportCachetAspectRatio() );

	if ( m_mediator->OnPreview_CalculateCamera( view, camera ) )
	{
		//...
	}
	else
	{
		CEdPreviewPanel::OnViewportCalculateCamera( view, camera );
		camera.SetNearPlane( 0.4f );
		camera.SetNonDefaultNearRenderingPlane();
	}

	if ( m_trackLight )
	{
		const Int32 camRot = (Int32)camera.GetRotation().Yaw;
		const Int32 lightPos = GetLightPosition();
		if ( camRot != lightPos )
		{
			SetLightPosition( camRot-180 );
		}

	}
}

void CEdDialogPreview::OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame )
{
#ifdef PROFILE_DIALOG_EDITOR_TICK
	CTimeCounter timer;
#endif

	if ( view->GetHeight() <= 1 || view->GetWidth() <= 1 || IsShown() == false )
	{
		return;
	}

	CEdPreviewPanel::OnViewportGenerateFragments( view, frame );
	
	m_mediator->OnPreview_GenerateFragments( view, frame );

	{
		const Float avgFPS = GEngine->GetLastTickRate();
		const Float minFPS = GEngine->GetMinTickRate();
		frame->AddDebugScreenText( 20, 20, String::Printf( TXT("avg FPS: %1.2f, min FPS: %1.2f"), avgFPS, minFPS ), 0, false, Color::WHITE, Color::BLACK );	
	}

	CFont* font = resDialogPreviewFont.LoadAndGet< CFont >();
	ASSERT( font );

	// draw edit strings
	for ( Uint32 i = 0; i < EDIT_MAX; ++i )
	{
		if ( false == m_editString[ i ].Empty() )
		{
			frame->AddDebugScreenText( 20, i * 20 + 50, m_editString[ i ], Color::RED, font );
		}
	}

	GenerateFragmentsForCurrentCamera( view, frame );

#ifdef PROFILE_DIALOG_EDITOR_TICK
	const Float timeElapsed = timer.GetTimePeriodMS();
	SCENE_LOG( TXT("CEdDialogPreview::OnViewportGenerateFragments - %1.3f"), timeElapsed );
#endif
}

void CEdDialogPreview::GenerateFragmentsForCurrentCamera( IViewport *view, CRenderFrame *frame )
{
	m_mediator->OnPreview_GenerateFragmentsForCurrentCamera( view, frame );
}

CRenderFrame* CEdDialogPreview::OnViewportCreateFrame( IViewport *view )
{
	if ( view->GetHeight() <= 1 || view->GetWidth() <= 1 || IsShown() == false )
	{
		return NULL;
	}

	CRenderFrameInfo info( view );
	CRenderFrame* previewRenderFrame = m_previewWorld->GenerateFrame( view, info );
	
	return previewRenderFrame;
}

void CEdDialogPreview::OnViewportRenderFrame( IViewport *view, CRenderFrame *frame )
{
	CEdPreviewPanel::OnViewportRenderFrame( view, frame );
}

Bool CEdDialogPreview::OnViewportClick( IViewport* view, Int32 button, Bool state, Int32 x, Int32 y )
{
	if ( view->GetHeight() <= 1 || view->GetWidth() <= 1 || IsShown() == false )
	{
		return false;
	}
	
	return CEdPreviewPanel::OnViewportClick( view, button, state, x, y );
}

void CEdDialogPreview::OnFreeCameraModeButton( wxCommandEvent& event )
{
	m_mediator->OnPreview_FreeCameraMode();
}

void CEdDialogPreview::OnPreviewModeButton( wxCommandEvent& event )
{
	m_mediator->OnPreview_PreviewCameraMode();
}
void CEdDialogPreview::OnEditModeButton( wxCommandEvent& event )
{
	m_mediator->OnPreview_EditCameraMode();
}

void CEdDialogPreview::ToggleTimePauseBtn( wxCommandEvent& event )
{
	ToggleTimePause();
}

void CEdDialogPreview::OnAddLight( wxCommandEvent& event )
{
	m_mediator->OnPreview_AddLight();
}

void CEdDialogPreview::OnCutsceneButton( wxCommandEvent& event )
{
	m_mediator->OnPreview_OpenCutscene();
}

void CEdDialogPreview::OnLoadWorldButton( wxCommandEvent& event )
{
	const Bool ret = m_mediator->OnPreview_PlayInMainWorld( false );

	wxToolBar* toolbar = XRCCTRL( *m_mediator, "PreviewControlToolbar", wxToolBar );
	toolbar->ToggleTool( XRCID("LoadWorldButton"), ret );
}

void CEdDialogPreview::OnLoadWorldButton2( wxCommandEvent& event )
{
	const Bool ret = m_mediator->OnPreview_PlayInMainWorld( true );

	wxToolBar* toolbar = XRCCTRL( *m_mediator, "PreviewControlToolbar", wxToolBar );
	toolbar->ToggleTool( XRCID("LoadWorldButton2"), ret );
}

void CEdDialogPreview::OnFullscreenButton( wxCommandEvent& event )
{
	m_isFullscreen = !m_isFullscreen;
}

Bool CEdDialogPreview::OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data )
{
	Bool moveViewportCam = true;
	if ( m_mediator->OnPreview_ViewportInput( view, key, action, data, moveViewportCam ) )
	{
		return true;
	}

	if ( key == IK_L && action == IACT_Press )
	{
		m_lightEntity->Destroy();
		m_lightEntity = NULL;

		CreateExtraLight();
	}

	if ( key == IK_T && action == IACT_Press )
	{
		m_widgetManager->SetWidgetMode( RPWM_Move );
	}
	else if ( key == IK_R && action == IACT_Press )
	{
		m_widgetManager->SetWidgetMode( RPWM_Rotate );
	}

	if ( key == IK_P && action == IACT_Press )
	{
		ToggleTimePause();
	}

	if ( key == IK_C && action == IACT_Press )
	{
		if ( m_widgetManager->GetWidgetSpace() == RPWS_Local )
		{
			m_widgetManager->SetWidgetSpace( RPWS_Global );
		}
		else if ( m_widgetManager->GetWidgetSpace() == RPWS_Global )
		{
			m_widgetManager->SetWidgetSpace( RPWS_Foreign );
		}
		else if ( m_widgetManager->GetWidgetSpace() == RPWS_Foreign )
		{
			m_widgetManager->SetWidgetSpace( RPWS_Local );
		}
	}

	if ( key == IK_Delete && action == IACT_Press )
	{
		OnDelete();
	}

	if ( moveViewportCam )
	{
		if( key == IK_MouseZ && RIM_IS_KEY_DOWN( IK_Shift ) )
		{
			Float currentFov = GetCameraFov();

			if( data > 0.0f )
			{
				currentFov -= 2.5f;
			}
			else if( data < 0.f )
			{
				currentFov += 2.5f;
			}

			currentFov = Clamp( currentFov, 1.0f, 180.0f );
			SetCameraFov( currentFov );

			return true;
		}

		return CEdPreviewPanel::OnViewportInput( view, key, action, data );
	}

	return false;
}

void CEdDialogPreview::OnCameraMoved()
{
	CEdPreviewPanel::OnCameraMoved();

	m_mediator->OnPreview_CameraMoved();
}

void CEdDialogPreview::ToggleCameraTool( Bool previewMode, Bool freeMode, Bool editMode )
{
	m_previewToolbar->ToggleTool( m_previewCameraPreviewTool, previewMode );
	m_previewToolbar->ToggleTool( m_previewCameraFreeTool, freeMode );
	m_previewToolbar->ToggleTool( m_previewCameraEditTool, editMode );
}

void CEdDialogPreview::SlowDownCameraInputs( Bool enable )
{
	m_cameraInputPosMul = m_initCameraPosFactor * ( enable ? m_cameraPosMul : 1.f );
	m_cameraInputRotMul = m_initCameraRotFactor * ( enable ? m_cameraRotMul : 1.f );
}

void CEdDialogPreview::OnCreateCustomCameraButton( wxCommandEvent& event )
{
	m_mediator->OnPreview_CreateCustomCameraFromView();
}

void CEdDialogPreview::CreateExtraLight()
{
	ASSERT( !m_lightEntity );

	EntitySpawnInfo einfo;
	m_lightEntity = GetPreviewWorld()->GetDynamicLayer()->CreateEntitySync( einfo );
	if ( m_lightEntity )
	{
		static Float xyOffset = 10.f;
		static Float zOffset = 10.f;
		static Float radius = 50.f;
		static Float brightness = 1.5f;

		{
			SComponentSpawnInfo cinfo;
			cinfo.m_spawnPosition = Vector( 0.f, xyOffset, zOffset );
			CPointLightComponent* light = SafeCast< CPointLightComponent >( m_lightEntity->CreateComponent( ClassID< CPointLightComponent >(), cinfo ) );
			light->SetRadius( radius );
			light->SetBrightness( brightness );
		}

		{
			SComponentSpawnInfo cinfo;
			cinfo.m_spawnPosition = Vector( MCos( DEG2RAD( 30.f ) ) * xyOffset, -MSin( DEG2RAD( 30.f ) ) * xyOffset, zOffset );
			CPointLightComponent* light = SafeCast< CPointLightComponent >( m_lightEntity->CreateComponent( ClassID< CPointLightComponent >(), cinfo ) );
			light->SetRadius( radius );
			light->SetBrightness( brightness );
		}

		{
			SComponentSpawnInfo cinfo;
			cinfo.m_spawnPosition = Vector( -MCos( DEG2RAD( 30.f ) ) * xyOffset, -MSin( DEG2RAD( 30.f ) ) * xyOffset, zOffset );
			CPointLightComponent* light = SafeCast< CPointLightComponent >( m_lightEntity->CreateComponent( ClassID< CPointLightComponent >(), cinfo ) );
			light->SetRadius( radius );
			light->SetBrightness( brightness );
		}
	}
}

void CEdDialogPreview::HandleSelection( const TDynArray< CHitProxyObject* >& objects )
{
	if ( m_tool && m_tool->HandleSelection( objects ) )
	{
		return;
	}
	CWorld* world = GetWorld();
	if ( world )
	{
		// Toggle selection
		CSelectionManager* selectionMgr = GetSelectionManager();
		CSelectionManager::CSelectionTransaction transaction(*selectionMgr);

		// Deselect all selected object
		if ( !RIM_IS_KEY_DOWN( IK_LControl ) )
		{
			selectionMgr->DeselectAll();
		}
		// Toggle selection
		for ( Uint32 i=0; i<objects.Size(); i++ )
		{
			CComponent* tc = Cast< CComponent >( objects[i]->GetHitObject() );
			if ( !tc )
			{
				continue;
			}
			if ( tc->IsSelected() && !RIM_IS_KEY_DOWN( IK_LShift ) )
			{
				selectionMgr->Deselect( tc );
			}
			else if ( !tc->IsSelected() )
			{
				selectionMgr->Select( tc );
			}
		}
	}
}

void CEdDialogPreview::SetEditString( const String& txt, EEditStringId strid )
{ 
	m_editString[ strid ] = txt; 
}

#ifdef DEBUG_SCENES
#pragma optimize("",on)
#endif
