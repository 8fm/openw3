/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "../../common/engine/behaviorGraphInstance.h"

#include "../../common/game/actor.h"

#include "behaviorControlPanel.h"
#include "behaviorEditor.h"
#include "behaviorGraphEditor.h"


#define ID_PLAY				6001
#define ID_RESET			6002
#define ID_EYE				6003
#define ID_CAMERA			6004
#define ID_EX_MOTION		6005
#define ID_EX_TRAJ			6006
#define ID_SCROLL			6007
#define ID_PLAY_ONE			6008
#define ID_SZMOC			6009
#define ID_FLOOR			6010
#define ID_LOD_LEVEL		6011
#define ID_MESH_LOD_LEVEL	6012
#define ID_INPUTS			6013
#define ID_MIMIC			6014
#define ID_DISP_ITEM		6015
#define ID_UNDO_ITEM		6016
#define ID_DYN_TARGET		6017
#define ID_AUTO_TRACKING	6018

BEGIN_EVENT_TABLE( CEdBehaviorGraphControlPanel, CEdBehaviorEditorSimplePanel )
	EVT_SCROLL( CEdBehaviorGraphControlPanel::OnTimeScroll )
	EVT_MENU( 3001, CEdBehaviorGraphControlPanel::OnToggleSkeleton )
	EVT_MENU( 3002, CEdBehaviorGraphControlPanel::OnToggleSkeletonAxis )
	EVT_MENU( 3003, CEdBehaviorGraphControlPanel::OnToggleSkeletonName )
END_EVENT_TABLE()

CEdBehaviorGraphControlPanel::CEdBehaviorGraphControlPanel( CEdBehaviorEditor* editor )
	: CEdBehaviorEditorSimplePanel( editor )
	, m_itemDialog( NULL )
{
	// Load icons
	m_playIcon	=		SEdResources::GetInstance().LoadBitmap( TXT("IMG_CONTROL_PLAY") );
	m_pauseIcon =		SEdResources::GetInstance().LoadBitmap( TXT("IMG_CONTROL_PAUSE") );
	m_mimicOnIcon =		SEdResources::GetInstance().LoadBitmap( TXT("IMG_MIMIC_ON") );
	m_mimicOffIcon =	SEdResources::GetInstance().LoadBitmap( TXT("IMG_MIMIC_OFF") );
	m_mimicDisIcon =	SEdResources::GetInstance().LoadBitmap( TXT("IMG_MIMIC_DIS") );

	SetSize( 200, 50 );

	wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );

	// Time factor slider and label
	m_timeSlider = new wxSlider( this, -1, 100, 0, 100 );
	sizer->Add( m_timeSlider, 0, wxEXPAND | wxALL, 3 );

	// Tool sizer
	wxBoxSizer* toolSizer = new wxBoxSizer( wxHORIZONTAL );

	// Toolbar
	m_toolbar = new wxToolBar( this, -1 );

	m_toolbar->AddTool( ID_RESET, wxT("Reset"), SEdResources::GetInstance().LoadBitmap(_T("IMG_CONTROL_START")), wxT("Reset") );
	m_toolbar->Connect( ID_RESET, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBehaviorGraphControlPanel::OnResetPlayback ), NULL, this );

	m_toolbar->AddTool( ID_PLAY, wxT("Play"), SEdResources::GetInstance().LoadBitmap(_T("IMG_CONTROL_PLAY")), wxT("Play/Pause") );
	m_toolbar->Connect( ID_PLAY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBehaviorGraphControlPanel::OnPlayPause ), NULL, this );

	m_toolbar->AddTool( ID_PLAY_ONE, wxT("PlayOne"), SEdResources::GetInstance().LoadBitmap(_T("IMG_CONTROL_FORWARD")), wxT("Play one frame") );
	m_toolbar->Connect( ID_PLAY_ONE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBehaviorGraphControlPanel::OnPlayOneFrame ), NULL, this );

	m_toolbar->AddSeparator();

	m_toolbar->AddTool( ID_SZMOC, wxT("Szmoc"), SEdResources::GetInstance().LoadBitmap(_T("IMG_FORK")), wxT("Show activation alpha"), wxITEM_CHECK );
	m_toolbar->Connect( ID_SZMOC, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBehaviorGraphControlPanel::OnToggleShowAlpha ), NULL, this );

	m_toolbar->AddTool( ID_AUTO_TRACKING, wxT("AutoTracking"), SEdResources::GetInstance().LoadBitmap(_T("IMG_BUG")), wxT("Auto tracking"), wxITEM_CHECK );
	m_toolbar->Connect( ID_AUTO_TRACKING, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBehaviorGraphControlPanel::OnAutoTracking ), NULL, this );

	m_toolbar->AddTool( ID_EYE, wxT("CameraEye"), SEdResources::GetInstance().LoadBitmap(_T("IMG_EYE")), wxT("Player camera"), wxITEM_CHECK );
	m_toolbar->Connect( ID_EYE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBehaviorGraphControlPanel::OnToggleCameraPlayer ), NULL, this );

	m_toolbar->AddTool( ID_CAMERA, wxT("CameraMoving"), SEdResources::GetInstance().LoadBitmap(_T("IMG_CAMERA")), wxT("Moving camera"), wxITEM_CHECK );
	m_toolbar->Connect( ID_CAMERA, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBehaviorGraphControlPanel::OnToggleCameraMoving ), NULL, this );

	m_toolbar->AddTool( ID_EX_MOTION, wxT("ExMotion"), SEdResources::GetInstance().LoadBitmap(_T("IMG_CAR")), wxT("Extract motion"), wxITEM_CHECK );
	m_toolbar->Connect( ID_EX_MOTION, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBehaviorGraphControlPanel::OnToggleExMotion ), NULL, this );

	m_toolbar->AddTool( ID_EX_TRAJ, wxT("ExTraj"), SEdResources::GetInstance().LoadBitmap(_T("IMG_BDI_READ")), wxT("Extract trajectory"), wxITEM_CHECK );
	m_toolbar->Connect( ID_EX_TRAJ, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBehaviorGraphControlPanel::OnToggleExTrajectory ), NULL, this );

	m_toolbar->AddTool( ID_MIMIC, wxT("Mimic"), SEdResources::GetInstance().LoadBitmap(_T("IMG_MIMIC_DIS")), wxT("Mimic") );
	m_toolbar->Connect( ID_MIMIC, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBehaviorGraphControlPanel::OnToggleMimic ), NULL, this );

	m_toolbar->AddSeparator();

	m_toolbar->AddTool( ID_FLOOR, wxT("Floor"), SEdResources::GetInstance().LoadBitmap(_T("IMG_FLOOR")), wxT("Show floor") );
	m_toolbar->Connect( ID_FLOOR, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBehaviorGraphControlPanel::OnToggleFloor ), NULL, this );

	m_toolbar->AddTool( ID_DYN_TARGET, wxT("Target"), SEdResources::GetInstance().LoadBitmap(_T("IMG_COMPONENT")), wxT("Dynamic target"), wxITEM_CHECK );
	m_toolbar->Connect( ID_DYN_TARGET, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBehaviorGraphControlPanel::OnToggleDynamicTarget ), NULL, this );

	m_toolbar->AddSeparator();

	m_toolbar->AddTool( ID_DISP_ITEM, wxT("ItemGrab"), SEdResources::GetInstance().LoadBitmap(_T("IMG_ITEM_GRAB")), wxT("Grab item in preview") );
	m_toolbar->Connect( ID_DISP_ITEM, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBehaviorGraphControlPanel::OnDisplayItem ), NULL, this );

	m_toolbar->AddTool( ID_UNDO_ITEM, wxT("ItemUndo"), SEdResources::GetInstance().LoadBitmap(_T("IMG_ITEM_UNDO")), wxT("Undo last item grab") );
	m_toolbar->Connect( ID_UNDO_ITEM, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBehaviorGraphControlPanel::OnDisplayItemUndo ), NULL, this );

	m_toolbar->Realize();
	toolSizer->Add( m_toolbar, 0, wxEXPAND|wxALL, 0 );

	{
		m_lodLevel = new wxChoice( this, ID_LOD_LEVEL );
		m_lodLevel->AppendString( wxT("LOD 0") );
		m_lodLevel->AppendString( wxT("LOD 1") );
		m_lodLevel->AppendString( wxT("LOD 2") );
		m_lodLevel->Select( 0 );
		m_lodLevel->Connect( ID_LOD_LEVEL, wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( CEdBehaviorGraphControlPanel::OnLodLevelChanged ), NULL, this );
		toolSizer->Add( m_lodLevel, 0, wxEXPAND|wxALL, 0 );
	}

	{
		m_meshLodLevel = new wxChoice( this, ID_MESH_LOD_LEVEL );
		m_meshLodLevel->AppendString( wxT("MESH LOD AUTO") );
		m_meshLodLevel->AppendString( wxT("MESH LOD 0") );
		m_meshLodLevel->AppendString( wxT("MESH LOD 1") );
		m_meshLodLevel->AppendString( wxT("MESH LOD 2") );
		m_meshLodLevel->Select( 0 );
		m_meshLodLevel->Connect( ID_MESH_LOD_LEVEL, wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( CEdBehaviorGraphControlPanel::OnMeshLodLevelChanged ), NULL, this );
		toolSizer->Add( m_meshLodLevel, 0, wxEXPAND|wxALL, 0 );
	}

	sizer->Add( toolSizer );

	// Skeleton icons
	CreateSkeletonIcons();

	SetSizer( sizer );	
	Layout();
}

wxAuiPaneInfo CEdBehaviorGraphControlPanel::GetPaneInfo() const
{
	wxAuiPaneInfo info = CEdBehaviorEditorPanel::GetPaneInfo();

	info.CloseButton( false ).Left().LeftDockable( true ).MinSize( 200, 25 ).BestSize( 200, 50 ).Position( 1 );

	return info;
}

wxToolBar* CEdBehaviorGraphControlPanel::GetSkeletonToolbar()
{
	return m_toolbar;
}

void CEdBehaviorGraphControlPanel::RefreshAllButtons()
{
	UpdateCameraButtons();
	UpdatePlayButtons();
	UpdateMimicButtons();

	m_toolbar->ToggleTool( ID_EX_MOTION, GetEditor()->UseExtractedMotion() );
	m_toolbar->ToggleTool( ID_EX_TRAJ, GetEditor()->UseExtractedTrajectory() );
	m_toolbar->ToggleTool( ID_SZMOC, GetEditor()->IsActivationAlphaEnabled() );

	m_toolbar->ToggleTool( TOOL_SKELETON, GetEditor()->IsSkeletonBonesVisible() );
	m_toolbar->ToggleTool( TOOL_AXIS, GetEditor()->IsSkeletonBoneAxisVisible() );
	m_toolbar->ToggleTool( TOOL_NAMES, GetEditor()->IsSkeletonBoneNamesVisible() );
}

void CEdBehaviorGraphControlPanel::OnLoadEntity()
{
	RefreshAllButtons();

	GetEditor()->SetTimeFactor( GetTimeFactor() );
}

void CEdBehaviorGraphControlPanel::OnReset()
{
	RefreshAllButtons();

	GetEditor()->SetTimeFactor( GetTimeFactor() );
}

void CEdBehaviorGraphControlPanel::OnDebug( Bool flag )
{
	m_toolbar->EnableTool( ID_EX_TRAJ, !flag );
	m_toolbar->EnableTool( ID_EX_MOTION, !flag );
	m_toolbar->EnableTool( ID_PLAY, !flag );
	m_toolbar->EnableTool( ID_PLAY_ONE, !flag );
	m_toolbar->EnableTool( ID_RESET, !flag );
	m_toolbar->EnableTool( ID_CAMERA, !flag );
	m_toolbar->EnableTool( ID_EYE, !flag );

	// Force szmoc
	m_toolbar->ToggleTool( ID_SZMOC, true );
	GetEditor()->EnableActivationAlpha( true );

	CBehaviorGraphInstance* instance = GetBehaviorGraphInstance();
	if ( instance )
	{
		instance->ProcessActivationAlphas();
		GetEditor()->GetGraphEditor()->GetPanelWindow()->Refresh();
	}
}

void CEdBehaviorGraphControlPanel::OnPlayOneFrame( wxCommandEvent& event )
{
	GetEditor()->PlayOneFrame();
}

void CEdBehaviorGraphControlPanel::OnPlayPause( wxCommandEvent& event )
{
	// Set new value
	Bool temp = GetEditor()->IsPaused();
	GetEditor()->SetPause( !temp );		

	UpdatePlayButtons();
}

void CEdBehaviorGraphControlPanel::OnToggleShowAlpha( wxCommandEvent& event )
{
	GetEditor()->EnableActivationAlpha( event.IsChecked() );
}

void CEdBehaviorGraphControlPanel::OnAutoTracking( wxCommandEvent& event )
{
	GetEditor()->EnableAutoTracking( event.IsChecked() );
}

void CEdBehaviorGraphControlPanel::UpdatePlayButtons()
{
	// Update icon
	if ( GetEditor()->IsPaused() )
	{
		m_toolbar->SetToolNormalBitmap( ID_PLAY, m_playIcon );
	}
	else
	{
		m_toolbar->SetToolNormalBitmap( ID_PLAY, m_pauseIcon );
	}
};

void CEdBehaviorGraphControlPanel::UpdateMimicButtons()
{
	CEntity* entity = GetEntity();
	if ( entity )
	{
		IActorInterface* actor = entity->QueryActorInterface();
		if ( actor )
		{
			m_toolbar->EnableTool( ID_MIMIC, true );

			actor->HasMimic() ? 
				m_toolbar->SetToolNormalBitmap( ID_MIMIC, m_mimicOnIcon ) :
				m_toolbar->SetToolNormalBitmap( ID_MIMIC, m_mimicOffIcon );
		}
		else
		{
			m_toolbar->SetToolNormalBitmap( ID_MIMIC, m_mimicDisIcon );

			m_toolbar->EnableTool( ID_MIMIC, false );
		}
	}
}

void CEdBehaviorGraphControlPanel::OnToggleMimic( wxCommandEvent& event )
{
	CEntity* entity = GetEntity();
	if ( entity )
	{
		IActorInterface* actor = entity->QueryActorInterface();
		if ( actor )
		{
			if ( actor->HasMimic() )
			{
				actor->MimicOff();
			}
			else
			{
				actor->MimicOn();
			}
		}
	}

	UpdateMimicButtons();
}

void CEdBehaviorGraphControlPanel::OnResetPlayback( wxCommandEvent& event )
{	
	GetEditor()->RecreateBehaviorGraphInstance();
}

void CEdBehaviorGraphControlPanel::OnToggleCameraPlayer( wxCommandEvent& event )
{
	GetEditor()->SetEyeCamera( event.IsChecked() );

	UpdateCameraButtons();
}

void CEdBehaviorGraphControlPanel::OnToggleCameraMoving( wxCommandEvent& event )
{
	GetEditor()->SetMovingCamera( event.IsChecked() );

	UpdateCameraButtons();
}

void CEdBehaviorGraphControlPanel::UpdateCameraButtons()
{
	m_toolbar->ToggleTool( ID_EYE, GetEditor()->UseEyeCamera() );
	m_toolbar->ToggleTool( ID_CAMERA, GetEditor()->UseMovingCamera() );
}

void CEdBehaviorGraphControlPanel::OnToggleExMotion( wxCommandEvent& event )
{
	GetEditor()->SetExtractedMotion( event.IsChecked() );
}

void CEdBehaviorGraphControlPanel::OnToggleExTrajectory( wxCommandEvent& event )
{
	GetEditor()->SetExtractedTrajectory( event.IsChecked() );
}

Float CEdBehaviorGraphControlPanel::GetTimeFactor() const
{
	Int32 value = m_timeSlider->GetValue();
	Int32 minVal = m_timeSlider->GetMin();
	Int32 maxVal = m_timeSlider->GetMax();

	Float range = (Float)( maxVal - minVal );
	if ( range <= 0.0f )
	{
		range = 1.0f;
	}

	return (Float)value / range;
}

void CEdBehaviorGraphControlPanel::OnTimeScroll( wxScrollEvent& event )
{
	GetEditor()->SetTimeFactor( GetTimeFactor() );
}

void CEdBehaviorGraphControlPanel::OnToggleSkeleton( wxCommandEvent& event )
{
	GetEditor()->ToggleSkeletonBones();
}

void CEdBehaviorGraphControlPanel::OnToggleSkeletonAxis( wxCommandEvent& event )
{
	GetEditor()->ToggleSkeletonBoneAxis();
}

void CEdBehaviorGraphControlPanel::OnToggleSkeletonName( wxCommandEvent& event )
{
	GetEditor()->ToggleSkeletonBoneNames();
}

void CEdBehaviorGraphControlPanel::OnToggleFloor( wxCommandEvent& event )
{
	GetEditor()->OnToggleFloor();
}

void CEdBehaviorGraphControlPanel::OnToggleDynamicTarget( wxCommandEvent& event )
{
	GetEditor()->OnDynTarget( event.IsChecked() );
}

void CEdBehaviorGraphControlPanel::OnConnectInputs( wxCommandEvent& event )
{
	GetEditor()->ConnectInputs( event.IsChecked() );
}

void CEdBehaviorGraphControlPanel::OnDisplayItem( wxCommandEvent& event )
{
	if ( !m_itemDialog )
	{
		m_itemDialog = new CEdDisplayItemDialog( this, GetEditor() );
	}

	m_itemDialog->Show();
}

void CEdBehaviorGraphControlPanel::OnDisplayItemUndo( wxCommandEvent& event )
{
	GetEditor()->UndoItemDisplay();
}

void CEdBehaviorGraphControlPanel::OnLodLevelChanged( wxCommandEvent& event )
{
	Int32 level = event.GetInt();
	GetEditor()->SetLodLevel( (EBehaviorLod)level );
}

void CEdBehaviorGraphControlPanel::OnMeshLodLevelChanged( wxCommandEvent& event )
{
	Int32 level = event.GetInt();
	GetEditor()->SetMeshLodLevel( level-1 );
}

//////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE( CEdBehaviorGraphStackPanel, CEdBehaviorEditorSimplePanel )
	EVT_CHECKLISTBOX( XRCID( "instList" ), CEdBehaviorGraphStackPanel::OnCheckChanged )
	EVT_BUTTON( XRCID( "buttUp" ), CEdBehaviorGraphStackPanel::OnInstUp )
	EVT_BUTTON( XRCID( "buttDown" ), CEdBehaviorGraphStackPanel::OnInstDown )
END_EVENT_TABLE()

CEdBehaviorGraphStackPanel::CEdBehaviorGraphStackPanel( CEdBehaviorEditor* editor )
	: CEdBehaviorEditorSimplePanel( editor )
{
	// Load designed frame from resource
	wxPanel* innerPanel = wxXmlResource::Get()->LoadPanel( this, wxT("BehaviorEditorStackPanel") );
	SetMinSize( innerPanel->GetSize() );

	m_list = XRCCTRL( *this, "instList", wxCheckListBox );
	m_stack = XRCCTRL( *this, "stackList", wxListBox );

	wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
	sizer->Add( innerPanel, 1, wxEXPAND|wxALL, 0 );

	SetSizer( sizer );
	Layout();
}

wxAuiPaneInfo CEdBehaviorGraphStackPanel::GetPaneInfo() const
{
	wxAuiPaneInfo info = CEdBehaviorEditorPanel::GetPaneInfo();

	info.Floatable( true ).Float().MinSize( 200, 100 ).BestSize( 375, 300 ).Dockable( false );

	return info;
}

void CEdBehaviorGraphStackPanel::OnReset()
{
	FillStackList();
}

void CEdBehaviorGraphStackPanel::OnLoadEntity()
{
	FillStackList();
}

void CEdBehaviorGraphStackPanel::OnUnloadEntity()
{
	m_list->Clear();
	m_stack->Clear();
}

void CEdBehaviorGraphStackPanel::OnInstanceReload()
{
	UpdateStack();
}

void CEdBehaviorGraphStackPanel::OnDebug( Bool flag )
{
	m_list->Clear();
	m_stack->Clear();

	m_list->Enable( !flag );
	m_stack->Enable( !flag );
}

void CEdBehaviorGraphStackPanel::OnInstUp( wxCommandEvent& event )
{
	Int32 sel = m_stack->GetSelection();
	if ( sel > 0 )
	{
		wxArrayString instances = m_stack->GetStrings();

		wxString temp = instances[ sel - 1 ];
		instances[ sel - 1 ] = instances[ sel ];
		instances[ sel ] = temp;

		m_stack->Freeze();
		m_stack->Clear();
		m_stack->Append( instances );
		m_stack->Thaw();

		m_stack->SetSelection( sel - 1 );

		UpdateStack();
	}
}

void CEdBehaviorGraphStackPanel::OnInstDown( wxCommandEvent& event )
{
	Int32 sel = m_stack->GetSelection();
	if ( sel >= 0 && sel < (Int32)m_stack->GetCount() - 1 )
	{
		wxArrayString instances = m_stack->GetStrings();

		wxString temp = instances[ sel + 1 ];
		instances[ sel + 1 ] = instances[ sel ];
		instances[ sel ] = temp;

		m_stack->Freeze();
		m_stack->Clear();
		m_stack->Append( instances );
		m_stack->Thaw();

		m_stack->SetSelection( sel + 1 );

		UpdateStack();
	}
}

void CEdBehaviorGraphStackPanel::FillStack()
{
	m_stack->Freeze();
	m_stack->Clear();

	CAnimatedComponent* ac = GetAnimatedComponent();
	if ( ac && ac->GetBehaviorStack() )
	{
		// Get instances
		TDynArray< CName > instances;
		ac->GetBehaviorInstanceSlots( instances );

		// Base
		m_stack->Append( CNAME( BehaviorEditorGraph ).AsString().AsChar() );

		// Selection
		wxArrayInt aSel;
		m_list->GetSelections( aSel );

		for ( size_t i=0; i<aSel.GetCount(); ++i )
		{
			const Int32 index = aSel[ i ];

			if ( index < (Int32)instances.Size() )
			{
				m_stack->Append( instances[ index ].AsString().AsChar() );	
			}
			else
			{
				ASSERT( index < (Int32)instances.Size() );
			}
		}
	}

	m_stack->Thaw();
}

void CEdBehaviorGraphStackPanel::UpdateStack()
{
	CAnimatedComponent* ac = GetAnimatedComponent();
	if ( ac && ac->GetBehaviorStack() )
	{
		CBehaviorGraphStack* stack = ac->GetBehaviorStack();

		wxArrayString newInstances = m_stack->GetStrings();

		TDynArray< CName > names;

		Int32 size = (Int32)newInstances.GetCount();
		
		for ( Int32 i=size-1; i>=0; --i )
		{
			String inst = newInstances[i].wc_str();
			CName name( inst );
			names.PushBack( name );
		}

		CBehaviorGraph* graph = GetBehaviorGraph();
		stack->ActivateBehaviorInstance( graph, CNAME( BehaviorEditorGraph ), &names );
	}
}

void CEdBehaviorGraphStackPanel::FillStackList()
{
	m_list->Freeze();
	m_list->Clear();

	CAnimatedComponent* ac = GetAnimatedComponent();
	if ( ac && ac->GetBehaviorStack() )
	{
		const CName& orInstName = GetEditor()->GetOriginalInstanceName();

		// Get instances
		TDynArray< CName > instances;
		ac->GetBehaviorInstanceSlots( instances );

		Bool isInstanceInResource = false;

		for ( Uint32 i=0; i<instances.Size(); ++i )
		{
			const CName& inst = instances[i];
			wxString str( inst.AsString().AsChar() );

			Bool active = ac->GetBehaviorStack()->HasActiveInstance( inst );

			if ( inst == orInstName || active )
			{
				str = wxT("[>]") + str;
				m_list->AppendString( str );

				m_list->Check( i, true );

				if ( inst == orInstName )
				{
					isInstanceInResource = true;
				}
			}
			else
			{
				m_list->AppendString( str );
			}
		}

		if ( isInstanceInResource == false )
		{
			Bool active = ac->GetBehaviorStack()->HasActiveInstance( GetBehaviorGraphInstance()->GetInstanceName() );
			ASSERT( active );

			m_list->AppendString( CNAME( BehaviorEditorGraph ).AsString().AsChar() );
			m_list->Check( instances.Size(), true );
		}
	}

	m_list->Thaw();

	FillStack();
	UpdateStack();
}

void CEdBehaviorGraphStackPanel::OnCheckChanged( wxCommandEvent& event )
{
	Int32 pos = event.GetSelection();

	CAnimatedComponent* ac = GetAnimatedComponent();

	if ( pos < 0 || ac == NULL || ac->GetBehaviorStack() == NULL )
	{
		return;
	}

	Bool value = m_list->IsChecked( pos );
	String str = m_list->GetString( pos );

	const CName& instName = GetBehaviorGraphInstance()->GetInstanceName();
	const CName& orInstName = GetEditor()->GetOriginalInstanceName();

	TDynArray< CName > instances;
	ac->GetBehaviorInstanceSlots( instances );

	if ( str.BeginsWith( TXT("[>]") ) )
	{
		m_list->Check( pos, true );
		return;
	}

	wxString selInstName( instances[ pos ].AsString().AsChar() );
	Int32 found = m_stack->FindString( selInstName );

	if ( value )
	{
		m_list->Check( pos, true );
		
		if ( found == -1 )
		{
			m_stack->AppendString( selInstName );
		}
		else
		{
			ASSERT( found == -1 );
		}
	}
	else
	{
		m_list->Check( pos, false );
		
		if ( found != -1 )
		{
			m_stack->Delete( found );
		}
		else
		{
			ASSERT( found != -1 );
		}
	}

	m_stack->Refresh();

	UpdateStack();
}

//////////////////////////////////////////////////////////////////////////

#define ID_LL_LEVEL		8001
#define ID_LL_ACTIVE	8002
#define ID_LL_VAR		8003

BEGIN_EVENT_TABLE( CEdBehaviorGraphLookAtPanel, CEdBehaviorEditorSimplePanel )
	EVT_RADIOBUTTON( XRCID( "radioNull" ), CEdBehaviorGraphLookAtPanel::OnLookAtLevelChanged )
	EVT_RADIOBUTTON( XRCID( "radioEyes" ), CEdBehaviorGraphLookAtPanel::OnLookAtLevelChanged )
	EVT_RADIOBUTTON( XRCID( "radioHead" ), CEdBehaviorGraphLookAtPanel::OnLookAtLevelChanged )
	EVT_RADIOBUTTON( XRCID( "radioBody" ), CEdBehaviorGraphLookAtPanel::OnLookAtLevelChanged )
	EVT_CHOICE( XRCID( "choiceVar" ), CEdBehaviorGraphLookAtPanel::OnVarChanged )
	EVT_CHOICE( XRCID( "lookAtType" ), CEdBehaviorGraphLookAtPanel::OnLookAtTypeChanged )
	EVT_CHOICE( XRCID( "lookAtTypeList" ), CEdBehaviorGraphLookAtPanel::OnLookAtTypeListChanged )
	EVT_CHECKBOX( XRCID( "checkAuto" ), CEdBehaviorGraphLookAtPanel::OnAuto )
	EVT_TEXT_ENTER( XRCID( "editAutoRadius" ), CEdBehaviorGraphLookAtPanel::OnAutoParamChanged )
	EVT_TEXT_ENTER( XRCID( "editAutoHMin" ), CEdBehaviorGraphLookAtPanel::OnAutoParamChanged )
	EVT_TEXT_ENTER( XRCID( "editAutoHMax" ), CEdBehaviorGraphLookAtPanel::OnAutoParamChanged )
	EVT_COMMAND_SCROLL( XRCID( "sliderAutoHor" ), CEdBehaviorGraphLookAtPanel::OnAutoParamSpeedChanged )
	EVT_COMMAND_SCROLL( XRCID( "sliderAutoVer" ), CEdBehaviorGraphLookAtPanel::OnAutoParamSpeedChanged )
	EVT_COMMAND_SCROLL_THUMBTRACK( XRCID( "sliderAutoHor" ), CEdBehaviorGraphLookAtPanel::OnAutoParamSpeedChanged )
	EVT_COMMAND_SCROLL_THUMBTRACK( XRCID( "sliderAutoVer" ), CEdBehaviorGraphLookAtPanel::OnAutoParamSpeedChanged )
	EVT_BUTTON( XRCID( "buttAutoReset" ), CEdBehaviorGraphLookAtPanel::OnAutoResetTarget )
END_EVENT_TABLE()

CEdBehaviorGraphLookAtPanel::CEdBehaviorGraphLookAtPanel( CEdBehaviorEditor* editor )
	: CEdBehaviorEditorSimplePanel( editor )
	, m_autoTracking( false )
{
	// Load designed frame from resource
	wxPanel* innerPanel = wxXmlResource::Get()->LoadPanel( this, wxT("BehaviorEditorLookAtPanel") );
	SetMinSize( innerPanel->GetSize() );

	m_varList = XRCCTRL( *this, "choiceVar", wxChoice );

	m_lookAtState = XRCCTRL( *this, "radioOn", wxRadioButton );

	wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
	sizer->Add( innerPanel, 1, wxEXPAND|wxALL, 0 );

	FillLookAtTypes();

	SetSizer( sizer );
	Layout();
}

wxAuiPaneInfo CEdBehaviorGraphLookAtPanel::GetPaneInfo() const
{
	wxAuiPaneInfo info = CEdBehaviorEditorPanel::GetPaneInfo();

	info.Floatable( true ).Float().MinSize( 280, 240 ).BestSize( 280, 240 ).Dockable( false );

	return info;
}

void CEdBehaviorGraphLookAtPanel::OnLoadEntity()
{
	FillVarList();
	CheckActorMimic();
	UpdateActorLookAtLevel();
}

void CEdBehaviorGraphLookAtPanel::OnUnloadEntity()
{
	m_varList->Clear();
}

void CEdBehaviorGraphLookAtPanel::OnTick( Float dt )
{
	CheckActorMimic();

	CActor* actor = GetActor();
	if ( actor )
	{
		if ( m_lookAtState->GetValue() )
		{
			Vector target( 0.f, 2.5f, 1.7f );

			if ( m_autoTracking )
			{
				UpdateAutoTracking( dt );
				target = GetTargetFromAutoTracking();
			}
			else if ( m_varList->GetSelection() != -1 )
			{
				CBehaviorGraphInstance* instance = GetBehaviorGraphInstance();

				String sel = m_varList->GetStringSelection().wc_str();

				target = instance->GetVectorValue( CName( sel.AsChar() ) );
			}

			if ( !actor->IsLookAtEnabled() || !Vector::Near3( m_prevTarget, target ) )
			{
				actor->DisableLookAts();
				SetActorLookAt( actor, target );
			}

			m_prevTarget = target;

			if ( !actor->IsLookAtEnabled() )
			{
				m_lookAtState->SetValue( false );
				XRCCTRL( *this, "radioOff", wxRadioButton )->SetValue( true );
				ResetPrevTarget();
			}
		}
		else if ( actor->IsLookAtEnabled() )
		{
			actor->DisableLookAts();
		}
	}
}

void CEdBehaviorGraphLookAtPanel::ResetPrevTarget()
{
	m_prevTarget = Vector::ZERO_3D_POINT;
}

void CEdBehaviorGraphLookAtPanel::SetActorLookAt( CActor* actor, const Vector& target ) const
{
	wxChoice* typeList = XRCCTRL( *this, "lookAtType", wxChoice );
	wxChoice* list = XRCCTRL( *this, "lookAtTypeList", wxChoice );

	Int32 type = typeList->GetSelection();
	Int32 param = list->GetSelection();

	//if ( type == 0 )
	{
		SLookAtDebugStaticInfo info;
		info.m_target = target;
		actor->EnableLookAt( info );
	}
	/*else if ( type == 1 )
	{
		if ( param == 0 )
		{
			SLookAtReactionStaticInfo info;
			info.m_target = target;
			info.m_type = RLT_Glance;
			actor->EnableLookAt( info );
		}
		else if ( param == 1 )
		{
			SLookAtReactionStaticInfo info;
			info.m_target = target;
			info.m_type = RLT_Look;
			actor->EnableLookAt( info );
		}
		else if ( param == 2 )
		{
			SLookAtReactionStaticInfo info;
			info.m_target = target;
			info.m_type = RLT_Gaze;
			actor->EnableLookAt( info );
		}
		else if ( param == 3 )
		{
			SLookAtReactionStaticInfo info;
			info.m_target = target;
			info.m_type = RLT_Stare;
			actor->EnableLookAt( info );
		}
	}
	else if ( type == 2 )
	{
		SLookAtDialogStaticInfo info;
		info.m_target = target;
		actor->EnableLookAt( info );
	}
	else if ( type == 3 )
	{
		// TODO
	}
	else
	{
		ASSERT( 0 );
	}*/
}

void CEdBehaviorGraphLookAtPanel::OnInstanceReload()
{
	FillVarList();
}

CActor* CEdBehaviorGraphLookAtPanel::GetActor() const
{
	return Cast< CActor >( GetEntity() );
}

void CEdBehaviorGraphLookAtPanel::FillLookAtTypes()
{	
	wxChoice* list = XRCCTRL( *this, "lookAtType", wxChoice );
	list->Freeze();
	list->Clear();

	list->AppendString( wxT("Editor") );
	list->AppendString( wxT("Reaction") );
	list->AppendString( wxT("Dialog") );
	list->AppendString( wxT("Script") );
	list->SetSelection( 0 );

	list->Thaw();

	FillLookAtTypeList();
}

void CEdBehaviorGraphLookAtPanel::FillLookAtTypeList()
{
	wxChoice* typeList = XRCCTRL( *this, "lookAtType", wxChoice );
	Int32 sel = typeList->GetSelection();

	wxChoice* list = XRCCTRL( *this, "lookAtTypeList", wxChoice );
	list->Freeze();
	list->Clear();

	if ( sel == 1 )
	{
		list->AppendString( wxT("Glance") );
		list->AppendString( wxT("Look") );
		list->AppendString( wxT("Gaze") );
		list->AppendString( wxT("Stare") );
		list->SetSelection( 0 );
	}

	list->Thaw();
}

void CEdBehaviorGraphLookAtPanel::FillVarList()
{
	wxString sel = m_varList->GetStringSelection();

	m_varList->Freeze();
	m_varList->Clear();

	const CBehaviorGraph* graph = GetBehaviorGraph();
	if ( graph )
	{
		auto list = graph->GetVectorVariables().GetVariables();

		for ( auto it = list.Begin(), end = list.End(); it != end; it != end )
		{
			m_varList->AppendString( it->m_first.AsChar() );
		}
	}

	if ( !sel.IsEmpty() )
	{
		m_varList->SetStringSelection( sel );
	}

	wxCommandEvent fake;
	OnVarChanged( fake );

	m_varList->Thaw();
}

void CEdBehaviorGraphLookAtPanel::OnVarChanged( wxCommandEvent& event )
{
	
}

void CEdBehaviorGraphLookAtPanel::OnLookAtTypeChanged( wxCommandEvent& event )
{
	FillLookAtTypeList();
	ResetPrevTarget();
}

void CEdBehaviorGraphLookAtPanel::OnLookAtTypeListChanged( wxCommandEvent& event )
{
	ResetPrevTarget();
}

void CEdBehaviorGraphLookAtPanel::OnAuto( wxCommandEvent& event )
{
	m_autoTracking = event.IsChecked();
}

void CEdBehaviorGraphLookAtPanel::UpdateAutoTracking( Float dt )
{
	m_autoTrackingParam.m_horValue += dt * m_autoTrackingParam.m_horSpeed;
	m_autoTrackingParam.m_verValue += dt * m_autoTrackingParam.m_verValueDeltaSign * m_autoTrackingParam.m_verSpeed;

	if ( m_autoTrackingParam.m_horValue > M_PI * 2.f )
	{
		m_autoTrackingParam.m_horValue -= M_PI * 2.f;
	}

	if ( m_autoTrackingParam.m_verValue > m_autoTrackingParam.m_hMax )
	{
		m_autoTrackingParam.m_verValueDeltaSign = -1.f;
		m_autoTrackingParam.m_verValue = m_autoTrackingParam.m_hMax;
	}
	else if ( m_autoTrackingParam.m_verValue < m_autoTrackingParam.m_hMin )
	{
		m_autoTrackingParam.m_verValueDeltaSign = 1.f;
		m_autoTrackingParam.m_verValue = m_autoTrackingParam.m_hMin;
	}
}

Vector CEdBehaviorGraphLookAtPanel::GetTargetFromAutoTracking() const
{
	Vector target;

	target.X = m_autoTrackingParam.m_radius * MSin( m_autoTrackingParam.m_horValue );
	target.Y = m_autoTrackingParam.m_radius * MCos( m_autoTrackingParam.m_horValue );

	target.Z = m_autoTrackingParam.m_verValue;

	return target;
}

void CEdBehaviorGraphLookAtPanel::OnAutoResetTarget( wxCommandEvent& event )
{
	m_autoTrackingParam.ResetTarget();
}

void CEdBehaviorGraphLookAtPanel::OnAutoParamSpeedChanged( wxScrollEvent& event )
{
	wxSlider* hor = XRCCTRL( *this, "sliderAutoHor", wxSlider );
	wxSlider* ver = XRCCTRL( *this, "sliderAutoVer", wxSlider );

	m_autoTrackingParam.m_horSpeed = GetValueFromSlider( hor );
	m_autoTrackingParam.m_verSpeed = GetValueFromSlider( ver );
}

void CEdBehaviorGraphLookAtPanel::OnAutoParamChanged( wxCommandEvent& event )
{
	wxTextCtrl* radius = XRCCTRL( *this, "editAutoRadius", wxTextCtrl );
	wxTextCtrl* min = XRCCTRL( *this, "editAutoHMin", wxTextCtrl );
	wxTextCtrl* max = XRCCTRL( *this, "editAutoHMax", wxTextCtrl );

	m_autoTrackingParam.m_radius = GetValueFromEditBox( radius );
	m_autoTrackingParam.m_hMin = GetValueFromEditBox( min );
	m_autoTrackingParam.m_hMax = GetValueFromEditBox( max );
}

Float CEdBehaviorGraphLookAtPanel::GetValueFromSlider( wxSlider* slider ) const
{
	return ((Float)slider->GetValue()) / 25.f;
}

Float CEdBehaviorGraphLookAtPanel::GetValueFromEditBox( wxTextCtrl* edit ) const
{
	String valueStr = edit->GetValue().wc_str();
	Float value = 0.f;
	FromString( valueStr, value );
	return value;
}

void CEdBehaviorGraphLookAtPanel::UpdateActorLookAtLevel()
{
	CActor* actor = GetActor();
	if ( actor )
	{
		wxRadioButton* radioNull = XRCCTRL( *this, "radioNull", wxRadioButton );
		wxRadioButton* radioEyes = XRCCTRL( *this, "radioEyes", wxRadioButton );
		wxRadioButton* radioHead = XRCCTRL( *this, "radioHead", wxRadioButton );
		wxRadioButton* radioBody = XRCCTRL( *this, "radioBody", wxRadioButton );
	
		if ( radioNull->GetValue() )
		{
			actor->SetLookAtLevel( LL_Null );
		}
		else if ( radioEyes->GetValue() )
		{
			actor->SetLookAtLevel( LL_Eyes );
		}
		else if ( radioHead->GetValue() )
		{
			actor->SetLookAtLevel( LL_Head );
		}
		else if ( radioBody->GetValue() )
		{
			actor->SetLookAtLevel( LL_Body );
		}
	}
}

void CEdBehaviorGraphLookAtPanel::OnLookAtLevelChanged( wxCommandEvent& event )
{
	UpdateActorLookAtLevel();
}

void CEdBehaviorGraphLookAtPanel::CheckActorMimic()
{
	wxStaticText* text = XRCCTRL( *this, "textMimic", wxStaticText );

	CActor* actor = GetActor();
	if ( actor )
	{
		if ( actor->HasMimic() )
		{
			text->SetLabel( wxT("Mimic is ENABLED") );
		}
		else
		{
			text->SetLabel( wxT("Mimic is DISABLED") );
		}
	}
	else
	{
		text->SetLabel( wxT("Entity is NOT CActor") );
	}
}

//////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE( CEdBehaviorSimInputCanvas, CEdCanvas )
END_EVENT_TABLE()

CEdBehaviorSimInputCanvas::AnimBg::AnimBg()
	: m_time( 0.f )
{
}

void CEdBehaviorSimInputCanvas::AnimBg::Update( Float dt )
{
	static Float mul = 3.f;
	static Float duration = 3.f;

	 m_time += mul * dt;

	 if ( m_time >= duration )
	 {
		 m_time -= duration;

		 duration = GEngine->GetRandomNumberGenerator().Get< Float >( 3.f , 20.f );
	 }
}

void CEdBehaviorSimInputCanvas::AnimBg::Print( Int32 width, Int32 height, CEdBehaviorSimInputCanvas* canvas ) const
{
	wxColor colorGray( 10, 10, 10 );

	const Int32 border = (Int32)( 0.1f * Min( width, height ) );
	canvas->DrawCircle( border, border, width - 2 * border, height - 2 * border, colorGray, 3.f );

	const Float eyeFactor = ( m_time > 2.f && m_time < 3.f ) || ( m_time > 17.f && m_time < 18.f ) ? 1.f - MSin( M_PI * ( m_time ) ) : 1.f;

	const Int32 eyeH = (Int32)( 0.1f * height );
	const Int32 eyeHF = (Int32)( 0.1f * eyeFactor * height );
	const Int32 eyeW = (Int32)( 0.1f * width );

	const Int32 posEyeY = (Int32)( 0.5f * height );
	const Int32 posEyeXL = (Int32)( 0.4f * width );
	const Int32 posEyeXR = (Int32)( 0.6f * width );

	canvas->FillCircle( posEyeXL, posEyeY - eyeHF, eyeW, eyeHF, colorGray );
	canvas->FillCircle( posEyeXR, posEyeY - eyeHF, eyeW, eyeHF, colorGray );
}

CEdBehaviorSimInputCanvas::CEdBehaviorSimInputCanvas( wxWindow* parent )
	: CEdCanvas( parent )
	, m_running( false )
	, m_inputX( 0.f )
	, m_inputY( 0.f )
{

}

void CEdBehaviorSimInputCanvas::PaintCanvas( Int32 width, Int32 height )
{
	static wxColour back( 100, 100, 100 );

	Clear( back );

	if ( m_running )
	{
		wxColor colorGray( 10, 10, 10 );

		const Int32 borderX = (Int32)( 0.1f * width );
		const Int32 borderY = (Int32)( 0.1f * height );

		const Int32 startX = borderX;
		const Int32 startY = borderY;
		const Int32 endX = width - borderX;
		const Int32 endY = height - borderY;
		
		const Int32 lenX = endX - startX;
		const Int32 lenY = endY - startY;

		const Int32 centerX = startX + lenX / 2;
		const Int32 centerY = startY + lenY / 2;

		DrawCircle( startX, startY, lenX, lenY, colorGray, 3.f );

		Int32 itemH = (Int32)( 0.3f * height );
		Int32 itemW = (Int32)( 0.3f * width );

		Int32 posX = centerX + (Int32)( m_inputX * lenX / 2 );
		Int32 posY = centerY + (Int32)( -m_inputY * lenY / 2 );

		FillCircle( posX - itemW/2, posY - itemH/2, itemW, itemH, colorGray );
	}
	else
	{
		m_bg.Print( width, height, this );
	}
}

void CEdBehaviorSimInputCanvas::OnCanvasTick( Float dt )
{
	if ( !m_running )
	{
		m_bg.Update( dt );
	}
}

void CEdBehaviorSimInputCanvas::SetRunning( Bool state )
{
	m_running = state;
}

void CEdBehaviorSimInputCanvas::SetInputX( Float var )
{
	m_inputX = var;
}

void CEdBehaviorSimInputCanvas::SetInputY( Float var )
{
	m_inputY = var;
}

BEGIN_EVENT_TABLE( CEdBehaviorSimInputTool, CEdBehaviorEditorSimplePanel )
	EVT_MENU( XRCID( "buttConnect" ), CEdBehaviorSimInputTool::OnConnect )
	EVT_CHOICE( XRCID( "varX" ), CEdBehaviorSimInputTool::OnVarXChanged )
	EVT_CHOICE( XRCID( "varY" ), CEdBehaviorSimInputTool::OnVarYChanged )
END_EVENT_TABLE()

CEdBehaviorSimInputTool::CEdBehaviorSimInputTool( CEdBehaviorEditor* editor )
	: CEdBehaviorEditorSimplePanel( editor )
	, m_connect( false )
	, m_varFloatX( 0.f )
	, m_varFloatY( 0.f )
	, m_varSpeed( 0.f )
	, m_varRotation( 0.f )
{
	// Load designed frame from resource
	wxPanel* innerPanel = wxXmlResource::Get()->LoadPanel( this, wxT("BehaviorEditorGraphSimInputPanel") );
	SetMinSize( innerPanel->GetSize() );

	wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
	sizer->Add( innerPanel, 1, wxEXPAND|wxALL, 0 );

	{
		wxPanel* canvasPanel = XRCCTRL( *this, "canvasPanel", wxPanel );
		wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );

		m_canvas = new CEdBehaviorSimInputCanvas( canvasPanel );

		sizer1->Add( m_canvas, 1, wxEXPAND, 0 );
		canvasPanel->SetSizer( sizer1 );
		canvasPanel->Layout();
	}

	SetSizer( sizer );	
	Layout();
}

wxAuiPaneInfo CEdBehaviorSimInputTool::GetPaneInfo() const
{
	wxAuiPaneInfo info = CEdBehaviorEditorPanel::GetPaneInfo();

	info.Dockable( false ).Floatable( true ).Float().MinSize( GetMinSize() );

	return info;
}

void CEdBehaviorSimInputTool::OnConnect( wxCommandEvent& event )
{
	Connect( event.IsChecked() );
}

void CEdBehaviorSimInputTool::OnVarXChanged( wxCommandEvent& event )
{
	wxChoice* choice = XRCCTRL( *this, "varX", wxChoice );
	if ( choice->GetSelection() >= 0 )
	{
		m_varX = choice->GetStringSelection().c_str();
	}
}

void CEdBehaviorSimInputTool::OnVarYChanged( wxCommandEvent& event )
{
	wxChoice* choice = XRCCTRL( *this, "varY", wxChoice );
	if ( choice->GetSelection() >= 0 )
	{
		m_varY = choice->GetStringSelection().c_str();
	}
}

void CEdBehaviorSimInputTool::Connect( Bool flag )
{
	m_connect = flag;

	m_canvas->SetRunning( m_connect );

	RefreshPanel();
}

void CEdBehaviorSimInputTool::OnReset()
{
	RefreshPanel();
}

void CEdBehaviorSimInputTool::OnInstanceReload()
{
	RefreshPanel();
}

void CEdBehaviorSimInputTool::OnTick( Float dt )
{
	m_canvas->OnCanvasTick( dt );
	m_canvas->Refresh();

	UpdateAndSetVariables();
}

void CEdBehaviorSimInputTool::RefreshPanel()
{
	FillVariables();
	wxCommandEvent fake;
	OnVarXChanged( fake );
	OnVarYChanged( fake );
}

void CEdBehaviorSimInputTool::FillVariables()
{
	wxChoice* choiceX = XRCCTRL( *this, "varX", wxChoice );
	wxChoice* choiceY = XRCCTRL( *this, "varY", wxChoice );

	FillChoicesVars( choiceX );
	FillChoicesVars( choiceY );
}

void CEdBehaviorSimInputTool::FillChoicesVars( wxChoice* choice )
{
	choice->Freeze();
	choice->Clear();

	TDynArray< CName > names;
	GetEditor()->GetBehaviorGraph()->EnumVariableNames( names );

	const Uint32 size = names.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		choice->AppendString( names[ i ].AsChar() );
	}

	choice->Thaw();
}

void CEdBehaviorSimInputTool::UpdateAndSetVariables()
{
	if ( m_connect )
	{
		if ( !m_varX.Empty() )
		{
			GetEditor()->GetBehaviorGraphInstance()->SetFloatValue( CName( m_varX.AsChar() ), m_varRotation );
		}
		
		if ( !m_varY.Empty() )
		{
			GetEditor()->GetBehaviorGraphInstance()->SetFloatValue( CName( m_varY.AsChar() ), m_varSpeed );
		}
	}
}
