/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "sceneExplorer.h"
#include "staticCameraTool.h"
#include "../../common/engine/staticCamera.h"
#include "../../common/game/actionAreaVertex.h"
#include "../../common/core/gatheredResource.h"
#include "../../common/engine/hitProxyObject.h"
#include "../../common/engine/viewport.h"
#include "../../common/engine/dynamicLayer.h"
#include "../../common/engine/renderFrame.h"


wxIMPLEMENT_CLASS( CEdStaticCameraToolPanel, CEdDraggablePanel );

//////////////////////////////////////////////////////////////////////////

CGatheredResource resStaticCameraTemplate( TXT("gameplay\\camera\\static_camera.w2ent"), 0 );

//////////////////////////////////////////////////////////////////////////

#define ID_CREATE_CAMERA        4001

//////////////////////////////////////////////////////////////////////////

const String CEdStaticCameraToolPanel::PREVIEW_POINT_NAME = TXT("StaticCameraPreviewPoint");

CEdStaticCameraToolPanel::CEdStaticCameraToolPanel( wxWindow* parent, CWorld* world, CEdRenderingPanel* viewport )
	: m_world ( world )
	, m_viewport( viewport )
	, m_preview( false )
	, m_autoPreviewInTests( true )
	, m_camera( NULL )
	, m_init( false )
{
	m_multiTest.Reset();

	wxXmlResource::Get()->LoadPanel( this, parent, TXT("StaticCameraToolPanel") );

	// Header - Choice and preview button
	{
		XRCCTRL( *this, "StaticCameraList", wxChoice )->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( CEdStaticCameraToolPanel::OnCameraChoice ), NULL, this );
		XRCCTRL( *this, "buttTeleport", wxButton )->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdStaticCameraToolPanel::OnTeleportToCamera ), NULL, this );
		XRCCTRL( *this, "buttPreview", wxToggleButton )->Connect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( CEdStaticCameraToolPanel::OnPreview ), NULL, this );
	}

	// Main notebook - create
	{
		XRCCTRL( *this, "buttCreateFromView", wxButton )->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdStaticCameraToolPanel::OnCreateFromView ), NULL, this );
	}

	// Main notebook - Configuration
	{
		wxPanel* rp = XRCCTRL( *this, "propPanel", wxPanel );
		wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );
		PropertiesPageSettings settings;
		m_prop = new CEdPropertiesPage( rp, settings, nullptr );
		sizer1->Add( m_prop, 1, wxEXPAND, 0 );
		rp->SetSizer( sizer1 );
		rp->Layout();
	}

	// Main notebook - Tests
	{
		XRCCTRL( *this, "buttTestActivate", wxButton )->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdStaticCameraToolPanel::OnQuickTestActivate ), NULL, this );
		XRCCTRL( *this, "buttTestDeactivate", wxButton )->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdStaticCameraToolPanel::OnQuickTestDeactivate ), NULL, this );
		XRCCTRL( *this, "buttTestNext", wxButton )->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdStaticCameraToolPanel::OnQuickTestNext ), NULL, this );
		XRCCTRL( *this, "buttTestActivate", wxButton )->Enable( true );
		XRCCTRL( *this, "buttTestNext", wxButton )->Enable( false );
		XRCCTRL( *this, "buttTestDeactivate", wxButton )->Enable( false );

		XRCCTRL( *this, "testAdd", wxButton )->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdStaticCameraToolPanel::OnCameraTestAdd ), NULL, this );
		XRCCTRL( *this, "testRemove", wxButton )->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdStaticCameraToolPanel::OnCameraTestRemove ), NULL, this );

		XRCCTRL( *this, "checkAutoPreview", wxCheckBox )->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( CEdStaticCameraToolPanel::OnAutoPreview ), NULL, this );
	}

	// Visual debug
	m_cachedBehRenderState = viewport->GetViewport()->GetRenderingMask()[ SHOW_Behavior ];
	m_cachedVDRenderState = viewport->GetViewport()->GetRenderingMask()[ SHOW_VisualDebug ];
	viewport->GetViewport()->SetRenderingMask( SHOW_Behavior );
	viewport->GetViewport()->SetRenderingMask( SHOW_VisualDebug );

	FillCameraList();

	FreezeCameraTool( true );

	// Create item container
	InitItemContainer();

	m_detachablePanel.Initialize( this, TXT( "Camera" ) );

	m_init = true;
}

CEdStaticCameraToolPanel::~CEdStaticCameraToolPanel()
{
	m_init = false;

	// Destroy item container
	DestroyItemContainer();

	if ( m_camera )
	{
		DeselectCamera();
	}

	if ( m_viewport && m_viewport->GetViewport() )
	{
		if ( !m_cachedBehRenderState )
		{
			m_viewport->GetViewport()->ClearRenderingMask( SHOW_Behavior );
		}
		if ( !m_cachedVDRenderState )
		{
			m_viewport->GetViewport()->ClearRenderingMask( SHOW_VisualDebug );
		}
	}
}

void CEdStaticCameraToolPanel::OnToolClosed()
{
	if ( m_init )
	{
		// Destroy item container
		DestroyItemContainer();

		EndTest( false );
	}
}

Bool CEdStaticCameraToolPanel::HandleSelection( const TDynArray< CHitProxyObject* >& objects )
{
	if ( !m_init )
	{
		return false;
	}

	CSelectionManager* selectionMgr = m_world->GetSelectionManager();
	CSelectionManager::CSelectionTransaction transaction( *selectionMgr );

	for ( Uint32 i=0; i<objects.Size(); i++ )
	{
		CStaticCamera *cam = objects[i]->GetHitObject()->FindParent< CStaticCamera >();
		if ( cam && !cam->IsSelected() )
		{
			selectionMgr->DeselectAll();
			selectionMgr->Select( cam );
			SelectCamera( cam );

			return true;
		}
		else if ( cam )
		{			
			return false;
		}
	}

	return false;
}

Bool CEdStaticCameraToolPanel::HandleContextMenu( Int32 x, Int32 y, const Vector& collision )
{
	if ( !m_init )
	{
		return false;
	}

	m_world->ConvertScreenToWorldCoordinates( m_viewport->GetViewport(), x, y, m_clickedPosWS );

	wxMenu menu;

	menu.Append( ID_CREATE_CAMERA, TXT("Create static camera") );
	menu.Connect( ID_CREATE_CAMERA, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdStaticCameraToolPanel::OnCreateCamera ), NULL, this );

	PopupMenu( &menu );

	return true;
}

Bool CEdStaticCameraToolPanel::OnViewportCalculateCamera( IViewport* view, CRenderCamera &camera )
{
	if ( !m_init )
	{
		return false;
	}

	if ( IsPreview() )
	{
		GGame->GetActiveWorld()->GetCameraDirector()->OnViewportCalculateCamera( view, camera );
		
		return true;
	}

	return false;
}

Bool CEdStaticCameraToolPanel::OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame )
{
	if ( !m_init )
	{
		return false;
	}

	CStaticCamera* cam = GetCurrentCamera();

	if ( cam )
	{
		// Entity pos
		frame->AddDebugSphere( cam->GetWorldPosition(), 0.3f, Matrix::IDENTITY, Color::YELLOW );

		// Camera pos
		Vector dir;
		Matrix pos;
		if ( cam->GetCameraDirection( dir ) && cam->GetCameraMatrixWorldSpace( pos ) )
		{
			Vector start = pos.GetTranslation();
			Vector end = start + dir * 1000.f;

			frame->AddDebugLine( start, end, Color::YELLOW );
		}

		// Preview
		if ( IsPreview() )
		{
			return true;
		}
	}

	return false;	
}

Bool CEdStaticCameraToolPanel::OnViewportTick( IViewport* view, Float timeDelta )
{
	if ( !m_init )
	{
		return false;
	}

	if ( m_multiTest.m_running )
	{
		const CStaticCamera* cam = m_multiTest.m_callbackCameraDeact.Get();
		if ( cam )//&& !cam->IsBlending() )
		{
			EndTest();
		}
	}

	return false;
}

Bool CEdStaticCameraToolPanel::OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data )
{
	if ( !m_init )
	{
		return false;
	}

	if ( m_camera && IsPreview() )
	{
		if ( RIM_IS_KEY_DOWN( IK_LControl ) )
		{
			Vector offset( Vector::ZERO_3D_POINT );
			Bool used = false;
			const Matrix& mat = m_camera->GetLocalToWorld();
			static const Float step = 0.1f;

			// Steps = 10cm
			if ( ( key == IK_W || key == IK_Up ) && action == IACT_Press )
			{
				offset = mat.TransformVector( Vector( 0.f, step, 0.f ) );
				used = true;
			}
			else if ( ( key == IK_A || key == IK_Left ) && action == IACT_Press )
			{
				offset = mat.TransformVector( Vector( step, 0.f, 0.f ) );
				used = true;
			}
			else if ( ( key == IK_D || key == IK_Right ) && action == IACT_Press )
			{
				offset = mat.TransformVector( Vector( -step, 0.f, 0.f ) );
				used = true;
			}
			else if ( ( key == IK_S || key == IK_Down ) && action == IACT_Press )
			{
				offset = mat.TransformVector( Vector( 0.f, -step, 0.f ) );
				used = true;
			}
			else if ( ( key == IK_Q ) && action == IACT_Press )
			{
				offset = mat.TransformVector( Vector( 0.f, 0.f, step ) );
				used = true;
			}
			else if ( ( key == IK_E ) && action == IACT_Press )
			{
				offset = mat.TransformVector( Vector( 0.f, 0.f, -step ) );
				used = true;
			}

			if ( used )
			{
				m_camera->SetPosition( m_camera->GetWorldPosition() + offset );
				return true;
			}
		}
		else if ( RIM_IS_KEY_DOWN( IK_LShift ) )
		{
			EulerAngles offset = m_camera->GetWorldRotation();
			Bool used = false;
			const Matrix& mat = m_camera->GetLocalToWorld();
			static const Float step = 2.5f;

			// Steps
			if ( ( key == IK_W || key == IK_Up ) && action == IACT_Press )
			{
				offset = EulerAngles( 0.f, step, 0.f );
				used = true;
			}
			else if ( ( key == IK_A || key == IK_Left ) && action == IACT_Press )
			{
				offset = EulerAngles( 0.f, 0.f, step );
				used = true;
			}
			else if ( ( key == IK_D || key == IK_Right ) && action == IACT_Press )
			{
				offset = EulerAngles( 0.f, 0.f, -step );
				used = true;
			}
			else if ( ( key == IK_S || key == IK_Down ) && action == IACT_Press )
			{
				offset = EulerAngles( 0.f, -step, 0.f );
				used = true;
			}

			if ( used )
			{
				offset = ( offset.ToMatrix() * mat ).ToEulerAngles();
				m_camera->SetRotation( offset );
				return true;
			}
		}
	}

	return false;
}

void CEdStaticCameraToolPanel::DeselectCamera()
{
	m_camera = NULL;

	ClearItems();

	m_prop->SetNoObject();

	FillCameraList();

	FreezeCameraTool( true );
}

void CEdStaticCameraToolPanel::SelectCamera( CStaticCamera* camera )
{
	if ( camera )
	{
		DeselectCamera();

		FreezeCameraTool( false );

		m_camera = camera;

		if ( IsPreview() )
		{
			m_camera->SetActive();
		}

		m_prop->SetObject( m_camera );
	}
	else
	{
		FreezeCameraTool( true );
	}

	FillCameraList();
}

void CEdStaticCameraToolPanel::FreezeCameraTool( Bool flag )
{
	Bool enabled = !flag;

	wxPanel* panel2 = XRCCTRL( *this, "mainConfig", wxPanel );
	panel2->Enable( enabled );

	wxPanel* panel3 = XRCCTRL( *this, "TestPanel", wxPanel );
	panel3->Enable( enabled );

	XRCCTRL( *this, "buttPreview", wxToggleButton )->Enable( enabled );
	XRCCTRL( *this, "buttTeleport", wxButton )->Enable( enabled );
}

String CEdStaticCameraToolPanel::GetNameForCamera() const
{
	return InputBox( m_viewport, wxT("Camera tool"), wxT("Write camera name"), wxT("") );
}

void CEdStaticCameraToolPanel::CollectCameras( TDynArray< CStaticCamera* >& cameras ) const
{
	if ( m_world )
	{
		CollectAllEntities( m_world, cameras );

		TDynArray< CEntity* > entities;
		m_world->GetDynamicLayer()->GetEntities( entities );

		for ( Uint32 i=0; i<entities.Size(); ++i )
		{
			CStaticCamera* cam = Cast< CStaticCamera >( entities[ i ] );
			if ( cam )
			{
				cameras.PushBack( cam );
			}
		}
	}
}

void CEdStaticCameraToolPanel::FillCameraList()
{
	TDynArray< CStaticCamera* > cameras;
	CollectCameras( cameras );

	wxChoice* cameraList = XRCCTRL( *this, "StaticCameraList", wxChoice );

	cameraList->Freeze();
	cameraList->Clear();

	if ( cameras.Size() == 0 )
	{
		cameraList->AppendString( wxT("<empty>") );
		cameraList->Select( 0 );
		cameraList->Enable( false );
	}
	else
	{
		Uint32 noNameNum = 1;

		for ( Uint32 i=0; i<cameras.Size(); ++i )
		{
			const String& camName = cameras[i]->GetName();

			if ( camName.Empty() )
			{
				ASSERT( camName.Empty() );
				cameraList->AppendString( wxString::Format( wxT("<no name #%d>"), noNameNum ) );
				noNameNum++;
			}
			else
			{
				cameraList->AppendString( camName.AsChar() );	
			}
		}

		if ( m_camera )
		{
			cameraList->SetStringSelection( m_camera->GetName().AsChar() );
		}
		else
		{
			cameraList->Select( 0 );
		}

		cameraList->Enable( true );
	}

	cameraList->Thaw();

	wxListBox* list = XRCCTRL( *this, "testList1", wxListBox );
	list->Freeze();
	list->Clear();

	for ( Uint32 i=0; i<cameras.Size(); ++i )
	{
		list->AppendString( cameras[ i ]->GetName().AsChar() );
	}

	list->Thaw();
}

CStaticCamera* CEdStaticCameraToolPanel::CreateStaticCamera( const String& name, const Vector& pos, CLayer* layer ) const
{
	CEntityTemplate* templ = resStaticCameraTemplate.LoadAndGet< CEntityTemplate >();

	if ( !layer->MarkModified() )
	{
		return NULL;
	}

	EntitySpawnInfo info;
	info.m_spawnPosition = pos;
	info.m_template = templ;
	info.m_name = name;

	// Create entity
	CStaticCamera* camera = SafeCast< CStaticCamera >( layer->CreateEntitySync( info ) );

	return camera;
}

CStaticCamera* CEdStaticCameraToolPanel::CreateStaticCameraFromView( const String& name, Bool onDynamicLayer ) const
{
	CLayer* layer = NULL;

	if ( onDynamicLayer )
	{
		if ( GGame && GGame->GetActiveWorld() && GGame->GetActiveWorld()->GetDynamicLayer() )
		{
			layer = GGame->GetActiveWorld()->GetDynamicLayer();
		}
	}
	else
	{
		layer = GetEditorLayer();
	}

	if ( !layer )
	{
		return NULL;
	}

	CStaticCamera* camera = CreateStaticCamera( name, Vector::ZERO_3D_POINT, layer );
	if ( camera )
	{
		camera->SetDefaults();

		Vector newPos;
		EulerAngles newRot;

		Bool ret = camera->GetPositionFromView( m_viewport->GetCameraPosition(), m_viewport->GetCameraRotation(), newPos, newRot );
		ASSERT( ret );

		camera->SetPosition( newPos );
		camera->SetRotation( newRot );

		camera->SetNoSolver();

		// Camera will be used in this tick so it have to have proper data in post update tick
		layer->GetWorld()->DelayedActions();
		camera->ForceUpdateTransformNodeAndCommitChanges();
		camera->ForceUpdateBoundsNode();
	}

	return camera;
}

void CEdStaticCameraToolPanel::CreatePreviewPoint()
{
	if( m_previewPoint != NULL )
	{
		return;
	}

	EntitySpawnInfo sinfo;
	sinfo.m_name = PREVIEW_POINT_NAME;
	m_previewPoint = m_world->GetDynamicLayer()->CreateEntitySync( sinfo );
	ASSERT( m_previewPoint );

	CPreviewHelperComponent* component = Cast< CPreviewHelperComponent >( m_previewPoint->CreateComponent( ClassID< CPreviewHelperComponent >(), SComponentSpawnInfo() ) );
	ASSERT( component );

	component->SetName( TXT("") );
	component->SetColor( Color::LIGHT_GREEN );
	component->SetVisible( false );
}

void CEdStaticCameraToolPanel::DestroyPreviewPoint()
{
	if( m_previewPoint == NULL )
	{
		return;
	}

	m_previewPoint->Destroy();
	m_previewPoint = NULL;
}

CStaticCamera* CEdStaticCameraToolPanel::GetCameraByName( const String& name ) const
{
	TDynArray< CStaticCamera* > cameras;
	CollectCameras( cameras );

	for ( Uint32 i=0; i<cameras.Size(); ++i )
	{
		if ( cameras[ i ]->GetName() == name )
		{
			return cameras[ i ];
		}
	}

	return NULL;
}

void CEdStaticCameraToolPanel::RefreshCameraData()
{

}

void CEdStaticCameraToolPanel::RefreshTestedCameraLabel( const String& text )
{
	wxTextCtrl* edit = XRCCTRL( *this, "editTestCamera", wxTextCtrl );
	edit->SetLabel( text.AsChar() );
}

void CEdStaticCameraToolPanel::EnableTestNextButton( Bool flag )
{
	XRCCTRL( *this, "buttTestNext", wxButton )->Enable( flag );
}

CStaticCamera* CEdStaticCameraToolPanel::PlayNextTestCamera()
{
	if ( m_multiTest.m_running )
	{
		if ( m_multiTest.m_currNum >= m_multiTest.m_cameras.Count() )
		{
			m_multiTest.m_running = false;
			EndTest();
			return NULL;
		}

		String camName = m_multiTest.m_cameras[ m_multiTest.m_currNum ].wc_str();

		m_multiTest.m_currNum++;

		CStaticCamera* cam = GetCameraByName( camName );
		if ( cam )
		{
			if ( cam == m_multiTest.m_previewCameraForBlending && m_multiTest.m_currNum >= m_multiTest.m_cameras.Count() )
			{
				RefreshTestedCameraLabel( TXT("Deactivation") );

				m_multiTest.m_callbackCameraDeact = cam;

				EnableTestNextButton( !cam->HasTimeout() );

				return NULL;
			}

			if ( !cam->IsRunning() )
			{
				Bool ret = cam->Run( this );
				ASSERT( ret );

				String camText = String::Printf( TXT("%d. %s"), m_multiTest.m_currNum - 1, cam->GetName().AsChar() );

				RefreshTestedCameraLabel( camText );
			}
			else
			{
				ASSERT( !cam->IsRunning() );
				PlayNextTestCamera();	
			}

			if ( cam == m_multiTest.m_previewCameraForBlending && m_multiTest.m_currNum == 1 )
			{
				GGame->GetActiveWorld()->GetCameraDirector()->Update( 0.f );

				// First camera
				CStaticCamera* nextCam = PlayNextTestCamera();
				ASSERT( nextCam );

				RefreshTestedCameraLabel( TXT("Activation") );

				m_multiTest.m_callbackCameraAct = nextCam;
			}

			EnableTestNextButton( !cam->HasTimeout() );
		}

		return cam;
	}
	else
	{
		ASSERT( 0 );
	}

	return NULL;
}

void CEdStaticCameraToolPanel::OnRunStart( const CStaticCamera* camera )
{
	
}

void CEdStaticCameraToolPanel::OnRunEnd( const CStaticCamera* camera )
{
	PlayNextTestCamera();
}

void CEdStaticCameraToolPanel::OnActivationFinished( const CStaticCamera* camera )
{
	ASSERT( m_multiTest.m_running );

	const CStaticCamera* cam = m_multiTest.m_callbackCameraAct.Get();
	if ( cam == camera )
	{
		String camText = String::Printf( TXT("1. %s"), cam->GetName().AsChar() );
		RefreshTestedCameraLabel( camText.AsChar() );
	}
	else if ( cam )
	{
		ASSERT( 0 );
	}

	m_multiTest.m_callbackCameraAct = NULL;
}

void CEdStaticCameraToolPanel::OnDeactivationStarted( const CStaticCamera* camera )
{
	
}

void CEdStaticCameraToolPanel::OnTeleportToCamera( wxCommandEvent& event )
{
	if ( m_camera && m_camera->GetSelectedCameraComponent() )
	{		
		wxTheFrame->GetWorldEditPanel()->LookAtNode( m_camera->GetSelectedCameraComponent() );
	}
}

void CEdStaticCameraToolPanel::StartTest( Bool multi, Bool ui )
{
	{
		XRCCTRL( *this, "CreatePanel", wxPanel )->Enable( false );
		XRCCTRL( *this, "mainConfig", wxPanel )->Enable( false );
		XRCCTRL( *this, "lockPanel", wxPanel )->Enable( false );
		XRCCTRL( *this, "buttTestActivate", wxButton )->Enable( false );
		XRCCTRL( *this, "buttTestDeactivate", wxButton )->Enable( true );
		XRCCTRL( *this, "testList1", wxListBox )->Enable( false );
		XRCCTRL( *this, "testList2", wxListBox )->Enable( false );
		XRCCTRL( *this, "testAdd", wxButton )->Enable( false );
		XRCCTRL( *this, "testRemove", wxButton )->Enable( false );
		XRCCTRL( *this, "checkMulti", wxCheckBox )->Enable( false );
	}

	wxListBox* list = XRCCTRL( *this, "testList2", wxListBox );

	m_multiTest.Reset();
	m_multiTest.m_running = true;

	if ( multi )
	{
		m_multiTest.m_cameras = list->GetStrings();
	}
	else
	{
		ASSERT( m_camera );
		m_multiTest.m_cameras.push_back( m_camera->GetName().AsChar() );
	}

	if ( ShouldAddPreviewCamera() )
	{
		static const String CAM_NAME = TXT("__STARTING_POINT__");

		m_multiTest.m_previewCameraForBlending = CreateStaticCameraFromView( CAM_NAME, true );
		if ( m_multiTest.m_previewCameraForBlending )
		{
			m_multiTest.m_previewCameraForBlending->SetEditorPreviewSettings();

			m_multiTest.m_cameras.Insert( CAM_NAME.AsChar(), 0 );
			m_multiTest.m_cameras.push_back( CAM_NAME.AsChar() );
		}
	}

	if ( IsAutoPreviewInTests() && IsPreview() == false )
	{
		m_multiTest.m_wasPreview = false;
		SetPreview( true );
	}

	PlayNextTestCamera();
}

Bool CEdStaticCameraToolPanel::ShouldAddPreviewCamera() const
{
	return !( GGame && GGame->IsActive() );
}

CStaticCamera* CEdStaticCameraToolPanel::GetCurrentCamera() const
{
	if ( m_multiTest.m_running )
	{
		ASSERT( m_multiTest.m_currNum - 1 < m_multiTest.m_cameras.Count() );
		return GetCameraByName( m_multiTest.m_cameras[ m_multiTest.m_currNum - 1 ].wc_str() );
	}
	else
	{
		return m_camera;
	}
}

void CEdStaticCameraToolPanel::EndTest( Bool ui )
{
	if ( ui )
	{
		XRCCTRL( *this, "CreatePanel", wxPanel )->Enable( true );
		XRCCTRL( *this, "mainConfig", wxPanel )->Enable( true );
		XRCCTRL( *this, "lockPanel", wxPanel )->Enable( true );
		XRCCTRL( *this, "testList1", wxListBox )->Enable( true );
		XRCCTRL( *this, "testList2", wxListBox )->Enable( true );
		XRCCTRL( *this, "testAdd", wxButton )->Enable( true );
		XRCCTRL( *this, "testRemove", wxButton )->Enable( true );
		XRCCTRL( *this, "buttTestActivate", wxButton )->Enable( true );
		XRCCTRL( *this, "buttTestDeactivate", wxButton )->Enable( false );
		XRCCTRL( *this, "checkMulti", wxCheckBox )->Enable( true );

		RefreshTestedCameraLabel( TXT("") );

		EnableTestNextButton( false );
	}

	if ( m_multiTest.m_previewCameraForBlending )
	{
		m_multiTest.m_previewCameraForBlending->Destroy();
		m_multiTest.m_previewCameraForBlending = NULL;
	}

	if ( IsAutoPreviewInTests() )
	{
		if ( m_multiTest.m_wasPreview && !IsPreview() )
		{
			if( m_camera ) m_camera->SetActive();
			SetPreview( true );
		}
		else if ( !m_multiTest.m_wasPreview && IsPreview() )
		{
			SetPreview( false );
		}
	}

	m_multiTest.Reset();
}

//////////////////////////////////////////////////////////////////////////

void CEdStaticCameraToolPanel::OnQuickTestActivate( wxCommandEvent& event )
{
	if ( m_camera )
	{
		RefreshCameraData();

		wxCheckBox* check = XRCCTRL( *this, "checkMulti", wxCheckBox );

		StartTest( check->GetValue() );
	}
}

void CEdStaticCameraToolPanel::OnQuickTestDeactivate( wxCommandEvent& event )
{
	EndTest();
}

void CEdStaticCameraToolPanel::OnQuickTestNext( wxCommandEvent& event )
{
	PlayNextTestCamera();
}

void CEdStaticCameraToolPanel::OnCameraTestAdd( wxCommandEvent& event )
{
	wxListBox* list1 = XRCCTRL( *this, "testList1", wxListBox );
	wxListBox* list2 = XRCCTRL( *this, "testList2", wxListBox );

	wxArrayInt sels;
	if ( list1->GetSelections( sels ) > 0 )
	{
		for ( size_t i=0; i<sels.size(); ++i )
		{
			list2->AppendString( list1->GetString( sels[ i ] ) );
		}
	}

	ValidateCameraTestList();
}

void CEdStaticCameraToolPanel::ValidateCameraTestList()
{
	wxListBox* list2 = XRCCTRL( *this, "testList2", wxListBox );

	wxString prev;

	TDynArray< Int32 > toRemove;
	wxArrayString items = list2->GetStrings();

	for ( size_t i = 0; i<items.Count(); ++i )
	{
		wxString curr = items[ i ];

		if ( curr == prev )
		{
			toRemove.PushBack( i );
		}

		prev = curr;
	}

	for ( Int32 i=(Int32)toRemove.Size()-1; i>=0; --i )
	{
		Int32 num = toRemove[ i ];

		items.RemoveAt( num );
	}

	list2->Freeze();
	list2->Clear();
	list2->Append( items );
	list2->Thaw();
}	

void CEdStaticCameraToolPanel::OnCameraTestRemove( wxCommandEvent& event )
{
	wxListBox* list2 = XRCCTRL( *this, "testList2", wxListBox );

	wxArrayString items = list2->GetStrings();

	wxArrayInt sels;
	if ( list2->GetSelections( sels ) > 0 )
	{
		for ( size_t i=0; i<sels.size(); ++i )
		{
			wxString name = list2->GetString( i );

			items.Remove( name );
		}
	}

	list2->Freeze();
	list2->Clear();

	for ( size_t i=0; i<items.Count(); ++i )
	{
		list2->AppendString( items[ i ] );
	}

	list2->Thaw();
}

CLayer* CEdStaticCameraToolPanel::GetEditorLayer() const
{
	// Get selected layer
	CLayer *layer = wxTheFrame->GetSceneExplorer()->GetActiveLayer();
	if ( layer == NULL )
	{
		wxMessageBox( TXT("Cannot create static camera, no layer activated!"), TXT("Camera tool"), wxOK | wxCENTRE, m_viewport );
	}

	return layer;
}

void CEdStaticCameraToolPanel::OnCreateCamera( wxCommandEvent& event )
{
	CLayer* layer = GetEditorLayer();
	if ( !layer )
	{
		return;
	}

	String name = GetNameForCamera();
	if ( name.Empty() )
	{
		return;
	}

	CStaticCamera* camera = CreateStaticCamera( name, m_clickedPosWS, layer );
	if ( camera )
	{
		camera->SetDefaults();

		FillCameraList();
		SelectCamera( camera );
	}
}

void CEdStaticCameraToolPanel::OnCreateFromView( wxCommandEvent& event )
{
	String name = GetNameForCamera();
	if ( name.Empty() )
	{
		return;
	}

	CStaticCamera* camera = CreateStaticCameraFromView( name );
	if ( camera )
	{
		FillCameraList();
		SelectCamera( camera );
	}
}

void CEdStaticCameraToolPanel::OnPreview( wxCommandEvent& event )
{
	m_preview = event.IsChecked();
}

void CEdStaticCameraToolPanel::OnAutoPreview( wxCommandEvent& event )
{
	m_autoPreviewInTests = event.IsChecked();
}

Bool CEdStaticCameraToolPanel::IsPreview() const
{
	return m_preview;
}

void CEdStaticCameraToolPanel::SetPreview( Bool flag )
{
	XRCCTRL( *this, "buttPreview", wxToggleButton )->SetValue( flag );
	m_preview = flag;
}

Bool CEdStaticCameraToolPanel::IsAutoPreviewInTests() const
{
	return m_autoPreviewInTests;
}

void CEdStaticCameraToolPanel::OnCameraChoice( wxCommandEvent& event )
{
	wxChoice* cameraList = XRCCTRL( *this, "StaticCameraList", wxChoice );

	Int32 selection = cameraList->GetSelection();
	if ( selection != wxNOT_FOUND )
	{
		String selStr = cameraList->GetStringSelection().wc_str();

		TDynArray< CStaticCamera* > cameras;
		CollectCameras( cameras );
		
		for ( Uint32 i=0; i<cameras.Size(); ++i )
		{
			if ( cameras[i]->GetName() == selStr )
			{
				SelectCamera( cameras[i] );
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CEdStaticCameraTool );

Bool CEdStaticCameraTool::Start( CEdRenderingPanel* viewport, 
								 CWorld* world, 
								 wxSizer* panelSizer, 
								 wxPanel* panel, 
								 const TDynArray< CComponent* >& selection )
{
	// Remember world
	m_world = world;

	// Remember viewport
	m_viewport = viewport;

	// Create tool panel
	m_toolPanel = new CEdStaticCameraToolPanel( panel, m_world, viewport );

	// Create panel for custom window
	panelSizer->Add( m_toolPanel, 1, wxEXPAND, 5 );
	panel->Layout();

	// Check if static camera is selected
	if( m_world->GetSelectionManager()->GetSelectionCount() > 0 )
	{
		TDynArray< CEntity* > selected;
		m_world->GetSelectionManager()->GetSelectedEntities( selected );
		for( TDynArray< CEntity* >::iterator entityIter = selected.Begin();
			entityIter != selected.End(); ++entityIter )
		{
			if( ( *entityIter )->IsA< CStaticCamera >() )
			{
				// Select initially this camera
				m_toolPanel->SelectCamera( Cast< CStaticCamera >( *entityIter ) );
				break;
			}
		}
	}

	// Only entities
	world->GetSelectionManager()->SetGranularity( CSelectionManager::SG_Entities );

	// Initialized
	return true;
}

void CEdStaticCameraTool::End()
{
	if ( m_toolPanel )
	{
		m_toolPanel->OnToolClosed();
	}

	m_toolPanel = NULL;
}

Bool CEdStaticCameraTool::HandleContextMenu( Int32 x, Int32 y, const Vector& collision )
{
	return m_toolPanel ? m_toolPanel->HandleContextMenu( x, y, collision ) : false;
}

Bool CEdStaticCameraTool::HandleSelection( const TDynArray< CHitProxyObject* >& objects )
{
	return m_toolPanel ? m_toolPanel->HandleSelection( objects ) : false;
}

Bool CEdStaticCameraTool::OnViewportCalculateCamera( IViewport* view, CRenderCamera &camera )
{
	return m_toolPanel ? m_toolPanel->OnViewportCalculateCamera( view, camera ) : false;
}

Bool CEdStaticCameraTool::OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame )
{
	return m_toolPanel ? m_toolPanel->OnViewportGenerateFragments( view, frame ) : false;
}

Bool CEdStaticCameraTool::OnViewportTick( IViewport* view, Float timeDelta )
{
	return m_toolPanel ? m_toolPanel->OnViewportTick( view, timeDelta ) : false;
}

Bool CEdStaticCameraTool::OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data ) 
{ 
	return m_toolPanel ? m_toolPanel->OnViewportInput( view, key, action, data ) : false;
}
