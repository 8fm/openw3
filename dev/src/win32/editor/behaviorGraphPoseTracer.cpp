/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behaviorGraphPoseTracer.h"
#include "behaviorEditor.h"
#include "../../common/engine/skeleton.h"

#define ID_CONNECT		6001
#define ID_SEL_BONE		6002
#define ID_SEL_HELP		6003
#define ID_SHOW_NAMES	6004
#define ID_SHOW_AXIS	6005

BEGIN_EVENT_TABLE( CEdBehaviorGraphPoseTracer, CEdBehaviorEditorSimplePanel )
END_EVENT_TABLE()

CEdBehaviorGraphPoseTracer::CEdBehaviorGraphPoseTracer( CEdBehaviorEditor* editor )
	: CEdBehaviorEditorSimplePanel( editor )
	, m_visualPose( NULL )
{
	SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bSizer167;
	bSizer167 = new wxBoxSizer( wxVERTICAL );

	m_toolbar = new wxToolBar( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTB_HORIZONTAL ); 

	m_toolbar->AddTool( ID_CONNECT, wxT("Connect"), SEdResources::GetInstance().LoadBitmap(_T("IMG_CONNECT")), wxT("Connect"), wxITEM_CHECK );
	m_toolbar->Connect( ID_CONNECT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBehaviorGraphPoseTracer::OnConnect ), NULL, this );

	m_toolbar->AddTool( ID_SEL_BONE, wxT("Bones"), SEdResources::GetInstance().LoadBitmap(_T("IMG_PB_BONE")), wxT("Select bones") );
	m_toolbar->Connect( ID_SEL_BONE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBehaviorGraphPoseTracer::OnSelectBones ), NULL, this );

	m_toolbar->AddTool( ID_SEL_HELP, wxT("Helpers"), SEdResources::GetInstance().LoadBitmap(_T("IMG_PB_BROWSE")), wxT("Select helpers") );
	m_toolbar->Connect( ID_SEL_HELP, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBehaviorGraphPoseTracer::OnSelectHelpers ), NULL, this );

	m_toolbar->AddTool( ID_SHOW_NAMES, wxT("Names"), SEdResources::GetInstance().LoadBitmap(_T("IMG_TEXT")), wxT("Show names") );
	m_toolbar->Connect( ID_SHOW_NAMES, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBehaviorGraphPoseTracer::OnShowNames ), NULL, this );

	m_toolbar->AddTool( ID_SHOW_AXIS, wxT("Axis"), SEdResources::GetInstance().LoadBitmap(_T("IMG_AXIS")), wxT("Show axis") );
	m_toolbar->Connect( ID_SHOW_AXIS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBehaviorGraphPoseTracer::OnShowAxis ), NULL, this );

	m_toolbar->AddSeparator();

	m_toolbar->Realize();

	bSizer167->Add( m_toolbar, 0, wxEXPAND|wxALL, 0 );

	m_nodeName = new wxTextCtrlEx( this, -1, wxT("Select node"), wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
	bSizer167->Add( m_nodeName, 0, wxEXPAND | wxALL, 3 );

	wxSplitterWindow* splitter12 = new wxSplitterWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3D );
	splitter12->SetSashSize( -1 );
	splitter12->SetMinimumPaneSize( 100 );
	wxPanel* panel70 = new wxPanel( splitter12, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer168 = new wxBoxSizer( wxVERTICAL );

	wxSplitterWindow * splitter14 = new wxSplitterWindow( panel70, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3D );
	splitter14->SetSashSize( -1 );
	wxPanel* panel72 = new wxPanel( splitter14, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER );
	wxBoxSizer* bSizer170 = new wxBoxSizer( wxVERTICAL );

	m_trackList = new wxListCtrl( panel72, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_ICON );
	m_trackList->SetMinSize( wxSize( -1, 100 ) );
	m_trackList->SetSize( wxSize( -1, 100 ) );
	bSizer170->Add( m_trackList, 1, wxALL|wxEXPAND, 0 );

	SetupTrackList();

	panel72->SetSizer( bSizer170 );
	panel72->Layout();
	bSizer170->Fit( panel72 );
	wxPanel* panel73 = new wxPanel( splitter14, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer171;
	bSizer171 = new wxBoxSizer( wxVERTICAL );

	m_boneList = new wxListCtrl( panel73, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_ICON );
	m_boneList->SetMinSize( wxSize( -1, 100 ) );
	m_boneList->SetSize( wxSize( -1, 100 ) );
	bSizer171->Add( m_boneList, 1, wxEXPAND, 5 );

	SetupBoneList();

	panel73->SetSizer( bSizer171 );
	panel73->Layout();
	bSizer171->Fit( panel73 );
	splitter14->SplitHorizontally( panel72, panel73, 120 );
	splitter14->SetMinimumPaneSize( 100 );
	bSizer168->Add( splitter14, 1, wxEXPAND, 5 );

	panel70->SetSizer( bSizer168 );
	panel70->Layout();
	bSizer168->Fit( panel70 );
	wxPanel* panel71 = new wxPanel( splitter12, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer169;
	bSizer169 = new wxBoxSizer( wxVERTICAL );

	wxSplitterWindow* splitter15 = new wxSplitterWindow( panel71, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3D );
	splitter15->SetSashSize( -1 );
	splitter15->SetMinimumPaneSize( 100 );
	wxPanel* panel74 = new wxPanel( splitter15, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer172;
	bSizer172 = new wxBoxSizer( wxVERTICAL );

	m_eventList = new wxListCtrl( panel74, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_ICON );
	m_eventList->SetMinSize( wxSize( -1, 100 ) );
	m_eventList->SetSize( wxSize( -1, 100 ) );
	bSizer172->Add( m_eventList, 1, wxEXPAND, 5 );

	SetupEventList();

	panel74->SetSizer( bSizer172 );
	panel74->Layout();
	bSizer172->Fit( panel74 );
	wxPanel* panel75 = new wxPanel( splitter15, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer173;
	bSizer173 = new wxBoxSizer( wxVERTICAL );

	m_motionExText = new wxTextCtrl( panel75, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
	bSizer173->Add( m_motionExText, 0, wxEXPAND, 5 );

	panel75->SetSizer( bSizer173 );
	panel75->Layout();
	bSizer173->Fit( panel75 );
	splitter15->SplitHorizontally( panel74, panel75, 120 );
	bSizer169->Add( splitter15, 1, wxEXPAND, 5 );

	panel71->SetSizer( bSizer169 );
	panel71->Layout();
	bSizer169->Fit( panel71 );
	splitter12->SplitHorizontally( panel70, panel71, 240 );
	bSizer167->Add( splitter12, 1, wxEXPAND, 5 );

	SetSizer( bSizer167 );
	Layout();
}

wxAuiPaneInfo CEdBehaviorGraphPoseTracer::GetPaneInfo() const
{
	wxAuiPaneInfo info = CEdBehaviorEditorPanel::GetPaneInfo();

	info.Floatable( true ).Float().BestSize( 175, 70 ).Dockable( false );

	return info;
}

void CEdBehaviorGraphPoseTracer::OnConnect( wxCommandEvent& event )
{
	m_connected = event.IsChecked();

	if ( !m_connected )
	{
		DestroyVisualPose();
		SelectNode( NULL );
	}
}

void CEdBehaviorGraphPoseTracer::OnSelectBones( wxCommandEvent& event )
{
	if ( m_visualPose )
	{
		m_visualPose->ChooseSelectBones();
	}
}

void CEdBehaviorGraphPoseTracer::OnSelectHelpers( wxCommandEvent& event )
{
	if ( m_visualPose )
	{
		m_visualPose->ChooseBonesHelpers();
	}
}

void CEdBehaviorGraphPoseTracer::OnShowNames( wxCommandEvent& event )
{
	if ( m_visualPose )
	{
		m_visualPose->ToggleBonesName();
	}
}

void CEdBehaviorGraphPoseTracer::OnShowAxis( wxCommandEvent& event )
{
	if ( m_visualPose )
	{
		m_visualPose->ToggleBonesAxis();
	}
}

void CEdBehaviorGraphPoseTracer::OnReset()
{
	DestroyVisualPose();

	m_toolbar->ToggleTool( ID_CONNECT, false );
	m_connected = false;
}

void CEdBehaviorGraphPoseTracer::OnNodesSelect( const TDynArray< CGraphBlock* >& nodes )
{
	if ( m_connected && nodes.Size() == 1 )
	{
		CreateVisualPose();

		CBehaviorGraphNode* node = SafeCast< CBehaviorGraphNode >( nodes[0] );

		SelectNode( node );
	}
	else if ( m_visualPose )
	{
		SelectNode( NULL );
	}
}

void CEdBehaviorGraphPoseTracer::OnNodesDeselect()
{
	SelectNode( NULL );
}

void CEdBehaviorGraphPoseTracer::OnTick( Float dt )
{
	if ( m_connected )
	{
		UpdateTrackList();
		UpdateBoneList();
		UpdateEventList();
		UpdateMotionExText();
	}
}

void CEdBehaviorGraphPoseTracer::SelectNode( CBehaviorGraphNode* node )
{
	if ( m_visualPose )
	{
		m_visualPose->SetNodeOfInterest( node );
	}

	m_nodeName->SetLabel( node ? node->GetName().Empty() ? wxT("<No name>") : node->GetName().AsChar() : wxT("Select node") );

	FillTrackList( node );
	FillBoneList( node );
	FillEventList( node );
}

void CEdBehaviorGraphPoseTracer::CreateVisualPose()
{
	if ( !m_visualPose )
	{
		CEntity *entity = GetEntity();
		CAnimatedComponent *animatedComponent = GetAnimatedComponent();	

		m_visualPose = Cast< CBehaviorDebugVisualizer >( entity->CreateComponent( CBehaviorDebugVisualizer::GetStaticClass(), SComponentSpawnInfo() ) );
		m_visualPose->SetAnimatedComponent( animatedComponent );
		m_visualPose->SetEditor( GetEditor() );
		m_visualPose->SetColor( Color::GREEN );
	}
}

void CEdBehaviorGraphPoseTracer::DestroyVisualPose()
{
	if ( m_visualPose )
	{
		GetEntity()->DestroyComponent( m_visualPose );
		m_visualPose = NULL;
	}
}

void CEdBehaviorGraphPoseTracer::SetupTrackList()
{
	m_trackList->SetWindowStyle( wxLC_REPORT );
	m_trackList->InsertColumn( LIST_TRACK_COL, wxT("Track") );
	m_trackList->InsertColumn( LIST_VALUE_COL, wxT("Value") );

	FillTrackList( NULL );
}

void CEdBehaviorGraphPoseTracer::FillTrackList( CBehaviorGraphNode* node )
{
	Int32 selCount = m_trackList->GetSelectedItemCount();
	long item = selCount > 0 ? m_trackList->GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED ) : 0;

	m_trackList->Freeze();
	m_trackList->DeleteAllItems();

	if ( node )
	{
		CAnimatedComponent *animatedComponent = GetAnimatedComponent();	
		CSkeleton* skeleton = node->IsMimic() ? animatedComponent->GetMimicSkeleton() : animatedComponent->GetSkeleton();
		//dex++: switched to generalized CSkeleton interface
		if ( NULL != skeleton  )
		{
			const Uint32 trackNum = skeleton->GetTracksNum();
			//dex--

			for( Uint32 i=0; i<trackNum; ++i )
			{
				String trackName = String::Printf( TXT("%d.%s"), i, skeleton->GetTrackName(i).AsChar() );
				m_trackList->InsertItem( i, trackName.AsChar() );
			}

			for( Uint32 i=0; i<SBehaviorGraphOutput::NUM_CUSTOM_FLOAT_TRACKS; ++i )
			{
				String trackName = String::Printf( TXT("Custom %d"), i );
				m_trackList->InsertItem( i+trackNum, trackName.AsChar() );
			}

			if ( selCount > 0 && m_trackList->GetItemCount() > item )
			{
				m_trackList->SetItemState( item , wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED );
			}
		}
	}

	m_trackList->Thaw();
}

void CEdBehaviorGraphPoseTracer::UpdateTrackList()
{
	m_trackList->Freeze();

	if ( m_visualPose && m_visualPose->HasNodeOfInterest() )
	{
		TDynArray< Float > tracks;
		m_visualPose->GetPoseTracks( tracks );

		TDynArray< Float > customTracks;
		m_visualPose->GetPoseCustomTracks( customTracks );

		//ASSERT( (Int32)tracks.Size() == m_trackList->GetItemCount() );

		const Uint32 trackSize = tracks.Size();
		for ( Uint32 i=0; i<trackSize; ++i )
		{
			wxListItem info;
			info.SetId( i );
			info.SetColumn( LIST_VALUE_COL );
			info.SetText( wxString::Format( wxT("%.2f"), tracks[i] ) );
			m_trackList->SetItem( info );
		}

		for ( Uint32 i=0; i<customTracks.Size(); ++i )
		{
			wxListItem info;
			info.SetId( i + trackSize );
			info.SetColumn( LIST_VALUE_COL );
			info.SetText( wxString::Format( wxT("%.2f"), customTracks[i] ) );
			m_trackList->SetItem( info );
		}
	}

	m_trackList->Thaw();
}

void CEdBehaviorGraphPoseTracer::SetupBoneList()
{
	m_boneList->SetWindowStyle( wxLC_REPORT );
	m_boneList->InsertColumn( LIST_BONE_COL, wxT("Bone") );
	m_boneList->InsertColumn( LIST_BONE_POS_LS_COL, wxT("LS Position") );
	m_boneList->InsertColumn( LIST_BONE_ROT_LS_COL, wxT("LS Rotation") );
	m_boneList->InsertColumn( LIST_BONE_POS_MS_COL, wxT("MS Position") );
	m_boneList->InsertColumn( LIST_BONE_ROT_MS_COL, wxT("MS Rotation") );
	m_boneList->InsertColumn( LIST_BONE_POS_WS_COL, wxT("WS Position") );
	m_boneList->InsertColumn( LIST_BONE_ROT_WS_COL, wxT("WS Rotation") );

	FillTrackList( NULL );
}

void CEdBehaviorGraphPoseTracer::FillBoneList( CBehaviorGraphNode* node )
{
	Int32 selCount = m_boneList->GetSelectedItemCount();
	long item = selCount > 0 ? m_boneList->GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED ) : 0;

	m_boneList->Freeze();
	m_boneList->DeleteAllItems();

	if ( node )
	{
		CAnimatedComponent *animatedComponent = GetAnimatedComponent();	
		CSkeleton* skeleton = node->IsMimic() ? animatedComponent->GetMimicSkeleton() : animatedComponent->GetSkeleton();
		//dex++: switched to general CSkeleton interface
		if ( NULL != skeleton )
		{
			const Uint32 boneNum = skeleton->GetBonesNum();
			//dex--

			for( Uint32 i=0; i<boneNum; ++i )
			{
				//dex++: 
				const String boneName = skeleton->GetBoneName(i);
				//dex--
				m_boneList->InsertItem( i, boneName.AsChar() );
			}

			if ( selCount > 0 && m_boneList->GetItemCount() > item )
			{
				m_boneList->SetItemState( item , wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED );
			}
		}
	}

	m_boneList->Thaw();
}

void CEdBehaviorGraphPoseTracer::UpdateBoneList()
{
	m_boneList->Freeze();

	if ( m_visualPose && m_visualPose->HasNodeOfInterest() )
	{
		TDynArray< Matrix > bonesLS;
		TDynArray< Matrix > bonesMS;
		TDynArray< Matrix > bonesWS;

		m_visualPose->GetPoseBones( bonesLS, bonesMS, bonesWS );

		//ASSERT( (Int32)bonesLS.Size() == m_boneList->GetItemCount() );
		//ASSERT( (Int32)bonesMS.Size() == m_boneList->GetItemCount() );
		//ASSERT( (Int32)bonesWS.Size() == m_boneList->GetItemCount() );

		for ( Uint32 i=0; i<bonesMS.Size(); ++i )
		{
			wxListItem info;

			{
				// LS
				Vector pos = bonesLS[i].GetTranslation();
				EulerAngles rot = bonesLS[i].ToEulerAngles();

				info.SetId( i );
				info.SetColumn( LIST_BONE_POS_LS_COL );
				info.SetText( wxString::Format( wxT("%.2f;%.2f;%.2f"), pos.X, pos.Y, pos.Z ) );
				m_boneList->SetItem( info );

				info.SetId( i );
				info.SetColumn( LIST_BONE_ROT_LS_COL );
				info.SetText( wxString::Format( wxT("%.2f;%.2f;%.2f"), rot.Pitch, rot.Roll, rot.Yaw ) );
				m_boneList->SetItem( info );
			}

			{
				// MS
				Vector pos = bonesMS[i].GetTranslation();
				EulerAngles rot = bonesMS[i].ToEulerAngles();

				info.SetId( i );
				info.SetColumn( LIST_BONE_POS_MS_COL );
				info.SetText( wxString::Format( wxT("%.2f;%.2f;%.2f"), pos.X, pos.Y, pos.Z ) );
				m_boneList->SetItem( info );

				info.SetId( i );
				info.SetColumn( LIST_BONE_ROT_MS_COL );
				info.SetText( wxString::Format( wxT("%.2f;%.2f;%.2f"), rot.Pitch, rot.Roll, rot.Yaw ) );
				m_boneList->SetItem( info );
			}

			{
				// WS
				Vector pos = bonesWS[i].GetTranslation();
				EulerAngles rot = bonesWS[i].ToEulerAngles();

				info.SetId( i );
				info.SetColumn( LIST_BONE_POS_WS_COL );
				info.SetText( wxString::Format( wxT("%.2f;%.2f;%.2f"), pos.X, pos.Y, pos.Z ) );
				m_boneList->SetItem( info );

				info.SetId( i );
				info.SetColumn( LIST_BONE_ROT_WS_COL );
				info.SetText( wxString::Format( wxT("%.2f;%.2f;%.2f"), rot.Pitch, rot.Roll, rot.Yaw ) );
				m_boneList->SetItem( info );
			}
		}
	}

	m_boneList->Thaw();
}

void CEdBehaviorGraphPoseTracer::SetupEventList()
{
	m_eventList->SetWindowStyle( wxLC_REPORT );
	m_eventList->InsertColumn( LIST_EVENT_NAME_COL, wxT("Anim Events") );

	FillEventList( NULL );
}

void CEdBehaviorGraphPoseTracer::FillEventList( CBehaviorGraphNode* node )
{
	m_eventList->Freeze();
	m_eventList->DeleteAllItems();
	m_eventList->Thaw();
}

void CEdBehaviorGraphPoseTracer::UpdateEventList()
{
	m_eventList->Freeze();
	m_eventList->DeleteAllItems();

	if ( m_visualPose && m_visualPose->HasNodeOfInterest() )
	{
		TDynArray< String > events;

		m_visualPose->GetPoseEvents( events );

		for ( Uint32 i=0; i<events.Size(); ++i )
		{
			m_eventList->InsertItem( i, events[i].AsChar() );
		}
	}

	m_eventList->Thaw();
}

void CEdBehaviorGraphPoseTracer::UpdateMotionExText()
{
	m_motionExText->SetLabel( wxT("-") );

	if ( m_visualPose && m_visualPose->HasNodeOfInterest() )
	{
		String str;
		m_visualPose->GetMotionEx( str );
		m_motionExText->SetLabel( str.AsChar() );
	}
}

//////////////////////////////////////////////////////////////////////////

#define ID_MA_CONNECT		8001

BEGIN_EVENT_TABLE( CEdBehaviorGraphMotionAnalyzer, CEdBehaviorEditorSimplePanel )
END_EVENT_TABLE()

CEdBehaviorGraphMotionAnalyzer::CEdBehaviorGraphMotionAnalyzer( CEdBehaviorEditor* editor )
	: CEdBehaviorEditorSimplePanel( editor )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bSizer567;
	bSizer567 = new wxBoxSizer( wxVERTICAL );

	m_toolbar = new wxToolBar( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTB_HORIZONTAL ); 
	m_toolbar->AddTool( ID_MA_CONNECT, wxT("Connect"), SEdResources::GetInstance().LoadBitmap(_T("IMG_CONNECT")), wxT("Connect"), wxITEM_CHECK );
	m_toolbar->Connect( ID_MA_CONNECT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBehaviorGraphMotionAnalyzer::OnConnect ), NULL, this );
	m_toolbar->Realize();

	bSizer567->Add( m_toolbar, 0, wxEXPAND, 5 );

	wxFlexGridSizer* fgSizer8;
	fgSizer8 = new wxFlexGridSizer( 5, 7, 0, 0 );
	fgSizer8->AddGrowableCol( 0 );
	fgSizer8->SetFlexibleDirection( wxBOTH );
	fgSizer8->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	wxStaticText* m_staticText246 = new wxStaticText( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText246->Wrap( -1 );
	fgSizer8->Add( m_staticText246, 0, wxALL, 5 );

	wxStaticText* m_staticText247 = new wxStaticText( this, wxID_ANY, wxT("Pos X"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText247->Wrap( -1 );
	fgSizer8->Add( m_staticText247, 0, wxALL, 5 );

	wxStaticText* m_staticText249 = new wxStaticText( this, wxID_ANY, wxT("Pos Y"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText249->Wrap( -1 );
	fgSizer8->Add( m_staticText249, 0, wxALL, 5 );

	wxStaticText* m_staticText251 = new wxStaticText( this, wxID_ANY, wxT("Pos Z"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText251->Wrap( -1 );
	fgSizer8->Add( m_staticText251, 0, wxALL, 5 );

	wxStaticText* m_staticText248 = new wxStaticText( this, wxID_ANY, wxT("Rot X"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText248->Wrap( -1 );
	fgSizer8->Add( m_staticText248, 0, wxALL, 5 );

	wxStaticText* m_staticText250 = new wxStaticText( this, wxID_ANY, wxT("Rot Y"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText250->Wrap( -1 );
	fgSizer8->Add( m_staticText250, 0, wxALL, 5 );

	wxStaticText* m_staticText252 = new wxStaticText( this, wxID_ANY, wxT("Rot Z"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText252->Wrap( -1 );
	fgSizer8->Add( m_staticText252, 0, wxALL, 5 );

	{
		wxStaticText* m_staticText243 = new wxStaticText( this, wxID_ANY, wxT("Animation"), wxDefaultPosition, wxDefaultSize, 0 );
		m_staticText243->Wrap( -1 );
		fgSizer8->Add( m_staticText243, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

		m_anim[ 0 ] = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 75,-1 ), wxTE_READONLY );
		fgSizer8->Add( m_anim[ 0 ], 0, wxALL, 5 );

		m_anim[ 1 ] = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 75,-1 ), wxTE_READONLY );
		fgSizer8->Add( m_anim[ 1 ], 0, wxALL, 5 );

		m_anim[ 2 ] = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 75,-1 ), wxTE_READONLY );
		fgSizer8->Add( m_anim[ 2 ], 0, wxALL, 5 );

		m_anim[ 3 ] = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 75,-1 ), wxTE_READONLY );
		fgSizer8->Add( m_anim[ 3 ], 0, wxALL, 5 );

		m_anim[ 4 ] = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 75,-1 ), wxTE_READONLY );
		fgSizer8->Add( m_anim[ 4 ], 0, wxALL, 5 );

		m_anim[ 5 ] = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 75,-1 ), wxTE_READONLY );
		fgSizer8->Add( m_anim[ 5 ], 0, wxALL, 5 );
	}

	{
		wxStaticText* m_staticText243 = new wxStaticText( this, wxID_ANY, wxT("Before loco"), wxDefaultPosition, wxDefaultSize, 0 );
		m_staticText243->Wrap( -1 );
		fgSizer8->Add( m_staticText243, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

		m_animDelta[ 0 ] = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 75,-1 ), wxTE_READONLY );
		fgSizer8->Add( m_animDelta[ 0 ], 0, wxALL, 5 );

		m_animDelta[ 1 ] = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 75,-1 ), wxTE_READONLY );
		fgSizer8->Add( m_animDelta[ 1 ], 0, wxALL, 5 );

		m_animDelta[ 2 ] = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 75,-1 ), wxTE_READONLY );
		fgSizer8->Add( m_animDelta[ 2 ], 0, wxALL, 5 );

		m_animDelta[ 3 ] = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 75,-1 ), wxTE_READONLY );
		fgSizer8->Add( m_animDelta[ 3 ], 0, wxALL, 5 );

		m_animDelta[ 4 ] = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 75,-1 ), wxTE_READONLY );
		fgSizer8->Add( m_animDelta[ 4 ], 0, wxALL, 5 );

		m_animDelta[ 5 ] = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 75,-1 ), wxTE_READONLY );
		fgSizer8->Add( m_animDelta[ 5 ], 0, wxALL, 5 );
	}

	{
		wxStaticText* m_staticText244 = new wxStaticText( this, wxID_ANY, wxT("After loco"), wxDefaultPosition, wxDefaultSize, 0 );
		m_staticText244->Wrap( -1 );
		fgSizer8->Add( m_staticText244, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

		m_entity[ 0 ] = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 75,-1 ), wxTE_READONLY );
		fgSizer8->Add( m_entity[ 0 ], 0, wxALL, 5 );

		m_entity[ 1 ] = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 75,-1 ), wxTE_READONLY );
		fgSizer8->Add( m_entity[ 1 ], 0, wxALL, 5 );

		m_entity[ 2 ] = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 75,-1 ), wxTE_READONLY );
		fgSizer8->Add( m_entity[ 2 ], 0, wxALL, 5 );

		m_entity[ 3 ] = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 75,-1 ), wxTE_READONLY );
		fgSizer8->Add( m_entity[ 3 ], 0, wxALL, 5 );

		m_entity[ 4 ] = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 75,-1 ), wxTE_READONLY );
		fgSizer8->Add( m_entity[ 4 ], 0, wxALL, 5 );

		m_entity[ 5 ] = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 75,-1 ), wxTE_READONLY );
		fgSizer8->Add( m_entity[ 5 ], 0, wxALL, 5 );
	}

	{
		wxStaticText* m_staticText245 = new wxStaticText( this, wxID_ANY, wxT("Diff"), wxDefaultPosition, wxDefaultSize, 0 );
		m_staticText245->Wrap( -1 );
		fgSizer8->Add( m_staticText245, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

		m_diff[ 0 ] = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 75,-1 ), wxTE_READONLY );
		fgSizer8->Add( m_diff[ 0 ], 0, wxALL, 5 );

		m_diff[ 1 ] = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 75,-1 ), wxTE_READONLY );
		fgSizer8->Add( m_diff[ 1 ], 0, wxALL, 5 );

		m_diff[ 2 ] = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 75,-1 ), wxTE_READONLY );
		fgSizer8->Add( m_diff[ 2 ], 0, wxALL, 5 );

		m_diff[ 3 ] = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 75,-1 ), wxTE_READONLY );
		fgSizer8->Add( m_diff[ 3 ], 0, wxALL, 5 );

		m_diff[ 4 ] = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 75,-1 ), wxTE_READONLY );
		fgSizer8->Add( m_diff[ 4 ], 0, wxALL, 5 );

		m_diff[ 5 ] = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 75,-1 ), wxTE_READONLY );
		fgSizer8->Add( m_diff[ 5 ], 0, wxALL, 5 );
	}

	bSizer567->Add( fgSizer8, 1, wxEXPAND, 5 );

	this->SetSizer( bSizer567 );
	this->Layout();

	this->Centre( wxBOTH );

	m_defaultColor = m_anim[ 0 ]->GetBackgroundColour();
}

wxAuiPaneInfo CEdBehaviorGraphMotionAnalyzer::GetPaneInfo() const
{
	wxAuiPaneInfo info = CEdBehaviorEditorPanel::GetPaneInfo();

	info.Floatable( true ).Float().Dockable( false );

	return info;
}

void CEdBehaviorGraphMotionAnalyzer::OnConnect( wxCommandEvent& event )
{
	m_connected = event.IsChecked();
}

void CEdBehaviorGraphMotionAnalyzer::OnReset()
{
	m_toolbar->ToggleTool( ID_MA_CONNECT, false );
	m_connected = false;

	m_animPos = m_animDeltaPos = m_entityPos = m_diffPos = m_lastEntityPos = m_lastAnimPos = Vector::ZERO_3D_POINT;
	m_animRot = m_animDeltaRot = m_entityRot = m_diffRot = m_lastEntityRot = m_lastAnimRot = EulerAngles::ZEROS;

	m_diffFlag = false;

	UpdateTextEdits();
}

void CEdBehaviorGraphMotionAnalyzer::OnTick( Float dt )
{
	if ( m_connected )
	{
		CEntity* entity = GetEditor()->GetEntity();
		CAnimatedComponent* root = entity->GetRootAnimatedComponent();
		if ( entity && root )
		{
			UpdateTextEdits();
		}
	}
}

void CEdBehaviorGraphMotionAnalyzer::OnDebuggerPostSamplePose( const SBehaviorGraphOutput& pose )
{
	if ( m_connected )
	{
		CEntity* entity = GetEditor()->GetEntity();
		CAnimatedComponent* root = entity->GetRootAnimatedComponent();
		if ( entity && root )
		{
			// Entity
			Vector currEntityPos = entity->GetWorldPosition();
			EulerAngles currEntityRot = entity->GetWorldRotation();

			m_entityPos = currEntityPos - m_lastEntityPos;
			m_entityRot = currEntityRot - m_lastEntityRot;

			m_lastEntityPos = currEntityPos;
			m_lastEntityRot = currEntityRot;

			m_animDeltaPos = root->GetAnimationMotionDelta().GetTranslationRef();
			m_animDeltaRot = root->GetAnimationMotionDelta().ToEulerAngles();

			// Anim
#ifdef USE_HAVOK_ANIMATION
			Matrix animDelta;
			HavokTransformToMatrix_Renormalize( pose.m_deltaReferenceFrameLocal, &animDelta );
#else
			RedMatrix4x4 conversionMatrix = pose.m_deltaReferenceFrameLocal.ConvertToMatrixNormalized();
			Matrix animDelta( reinterpret_cast< const Matrix& >( conversionMatrix ) );
#endif
			animDelta.SetTranslation( entity->GetLocalToWorld().TransformVector( animDelta.GetTranslation() ) );

			m_lastAnimPos = m_animPos;
			m_lastAnimRot = m_animRot;

			m_animPos = animDelta.GetTranslationRef();
			m_animRot = animDelta.ToEulerAngles();

			// Diff
			m_diffPos = m_entityPos - m_animDeltaPos;
			m_diffRot = m_entityRot - m_animDeltaRot;

			m_diffFlag = false;

			if ( !Vector::Near3( m_diffPos, Vector::ZERO_3D_POINT ) )
			{
				m_diffFlag = true;
			}
			else
			{
				m_diffPos = Vector::ZERO_3D_POINT;
			}
			if ( !m_diffRot.AlmostEquals( EulerAngles::ZEROS ) )
			{
				m_diffFlag = true;
			}
			else
			{
				m_diffRot = EulerAngles::ZEROS;
			}

			// Print
			//UpdateTextEdits();
		}
	}
}

void CEdBehaviorGraphMotionAnalyzer::UpdateTextEdit( const Vector& pos, const EulerAngles& rot, wxTextCtrl** edits, const wxColour* color )
{
	wxString posX = wxString::Format( wxT("%.4f"), pos.X );
	wxString posY = wxString::Format( wxT("%.4f"), pos.Y );
	wxString posZ = wxString::Format( wxT("%.4f"), pos.Z );

	wxString rotX = wxString::Format( wxT("%.4f"), rot.Pitch );
	wxString rotY = wxString::Format( wxT("%.4f"), rot.Roll );
	wxString rotZ = wxString::Format( wxT("%.4f"), rot.Yaw );

	edits[ 0 ]->SetValue( posX );
	edits[ 1 ]->SetValue( posY );
	edits[ 2 ]->SetValue( posZ );

	edits[ 3 ]->SetValue( rotX );
	edits[ 4 ]->SetValue( rotY );
	edits[ 5 ]->SetValue( rotZ );

	if ( color )
	{
		edits[ 0 ]->SetBackgroundColour( *color );
		edits[ 1 ]->SetBackgroundColour( *color );
		edits[ 2 ]->SetBackgroundColour( *color );
		edits[ 3 ]->SetBackgroundColour( *color );
		edits[ 4 ]->SetBackgroundColour( *color );
		edits[ 5 ]->SetBackgroundColour( *color );
	}
	else
	{
		edits[ 0 ]->SetBackgroundColour( m_defaultColor );
		edits[ 1 ]->SetBackgroundColour( m_defaultColor );
		edits[ 2 ]->SetBackgroundColour( m_defaultColor );
		edits[ 3 ]->SetBackgroundColour( m_defaultColor );
		edits[ 4 ]->SetBackgroundColour( m_defaultColor );
		edits[ 5 ]->SetBackgroundColour( m_defaultColor );
	}
}

void CEdBehaviorGraphMotionAnalyzer::UpdateTextEdits()
{
	UpdateTextEdit( m_lastAnimPos, m_lastAnimRot, m_anim );
	UpdateTextEdit( m_animDeltaPos, m_animDeltaRot, m_animDelta );
	UpdateTextEdit( m_entityPos, m_entityRot, m_entity );
	UpdateTextEdit( m_diffPos, m_diffRot, m_diff, m_diffFlag ? wxRED : NULL );
}

//////////////////////////////////////////////////////////////////////////

#define ID_PROFILER_CONNECT		9001

BEGIN_EVENT_TABLE( CEdBehaviorGraphProfiler, CEdBehaviorEditorSimplePanel )
END_EVENT_TABLE()

CEdBehaviorGraphProfiler::CEdBehaviorGraphProfiler( CEdBehaviorEditor* editor )
	: CEdBehaviorEditorSimplePanel( editor )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bSizer567;
	bSizer567 = new wxBoxSizer( wxVERTICAL );

	m_toolbar = new wxToolBar( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTB_HORIZONTAL ); 
	m_toolbar->AddTool( ID_PROFILER_CONNECT, wxT("Connect"), SEdResources::GetInstance().LoadBitmap(_T("IMG_CONNECT")), wxT("Connect"), wxITEM_CHECK );
	m_toolbar->Connect( ID_PROFILER_CONNECT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBehaviorGraphProfiler::OnConnect ), NULL, this );
	m_toolbar->Realize();

	bSizer567->Add( m_toolbar, 0, wxEXPAND, 5 );

	wxFlexGridSizer* fgSizer8;
	fgSizer8 = new wxFlexGridSizer( 3, 4, 0, 0 );
	fgSizer8->AddGrowableCol( 0 );
	fgSizer8->SetFlexibleDirection( wxBOTH );
	fgSizer8->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	wxStaticText* m_staticText246 = new wxStaticText( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText246->Wrap( -1 );
	fgSizer8->Add( m_staticText246, 0, wxALL, 5 );

	wxStaticText* m_staticText247 = new wxStaticText( this, wxID_ANY, wxT("Update"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText247->Wrap( -1 );
	fgSizer8->Add( m_staticText247, 0, wxALL, 5 );

	wxStaticText* m_staticText249 = new wxStaticText( this, wxID_ANY, wxT("Sample"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText249->Wrap( -1 );
	fgSizer8->Add( m_staticText249, 0, wxALL, 5 );

	wxStaticText* m_staticText251 = new wxStaticText( this, wxID_ANY, wxT("All"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText251->Wrap( -1 );
	fgSizer8->Add( m_staticText251, 0, wxALL, 5 );

	{
		wxStaticText* m_staticText243 = new wxStaticText( this, wxID_ANY, wxT("Curr"), wxDefaultPosition, wxDefaultSize, 0 );
		m_staticText243->Wrap( -1 );
		fgSizer8->Add( m_staticText243, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

		m_updateTimeEdit[0] = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 75,-1 ), wxTE_READONLY );
		fgSizer8->Add( m_updateTimeEdit[0], 0, wxALL, 5 );

		m_sampleTimeEdit[0] = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 75,-1 ), wxTE_READONLY );
		fgSizer8->Add( m_sampleTimeEdit[0], 0, wxALL, 5 );

		m_allTimeEdit[0] = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 75,-1 ), wxTE_READONLY );
		fgSizer8->Add( m_allTimeEdit[0], 0, wxALL, 5 );
	}

	{
		wxStaticText* m_staticText243 = new wxStaticText( this, wxID_ANY, wxT("Avg"), wxDefaultPosition, wxDefaultSize, 0 );
		m_staticText243->Wrap( -1 );
		fgSizer8->Add( m_staticText243, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

		m_updateTimeEdit[1] = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 75,-1 ), wxTE_READONLY );
		fgSizer8->Add( m_updateTimeEdit[1], 0, wxALL, 5 );

		m_sampleTimeEdit[1] = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 75,-1 ), wxTE_READONLY );
		fgSizer8->Add( m_sampleTimeEdit[1], 0, wxALL, 5 );

		m_allTimeEdit[1] = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 75,-1 ), wxTE_READONLY );
		fgSizer8->Add( m_allTimeEdit[1], 0, wxALL, 5 );
	}

	bSizer567->Add( fgSizer8, 1, wxEXPAND, 5 );

	this->SetSizer( bSizer567 );
	this->Layout();

	this->Centre( wxBOTH );

	m_filteredUpdateTime.Reset();
	m_filteredSampleTime.Reset();
}

wxAuiPaneInfo CEdBehaviorGraphProfiler::GetPaneInfo() const
{
	wxAuiPaneInfo info = CEdBehaviorEditorPanel::GetPaneInfo();

	info.Floatable( true ).Float().Dockable( false );

	return info;
}

void CEdBehaviorGraphProfiler::OnConnect( wxCommandEvent& event )
{
	Bool ret = event.IsChecked();
	if ( ret && !m_connected )
	{
		ConnectToInstance();
	}
	else if ( !ret && m_connected )
	{
		DisconnectFromInstance();
	}
}

void CEdBehaviorGraphProfiler::ConnectToInstance()
{
	ASSERT( !m_connected );

	CBehaviorGraphInstance* inst = GetEditor()->GetBehaviorGraphInstance();
	if ( inst )
	{
		ASSERT( !inst->GetEditorListener() );
		inst->SetEditorListener( this );
	}

	m_filteredUpdateTime.Reset();
	m_filteredSampleTime.Reset();

	m_connected = true;
}

void CEdBehaviorGraphProfiler::DisconnectFromInstance()
{
	ASSERT( m_connected );

	CBehaviorGraphInstance* inst = GetEditor()->GetBehaviorGraphInstance();
	if ( inst )
	{
		ASSERT( inst->GetEditorListener() == this );
		inst->RemoveEditorListener();
	}

	m_connected = false;
}

void CEdBehaviorGraphProfiler::OnReset()
{
	//m_toolbar->ToggleTool( ID_PROFILER_CONNECT, false );

	m_updateTimeEdit[0]->SetValue( wxT("0.000") );
	m_sampleTimeEdit[0]->SetValue( wxT("0.000") );
	m_allTimeEdit[0]->SetValue( wxT("0.000") );

	m_updateTimeEdit[1]->SetValue( wxT("0.000") );
	m_sampleTimeEdit[1]->SetValue( wxT("0.000") );
	m_allTimeEdit[1]->SetValue( wxT("0.000") );

	DisconnectFromInstance();
	ConnectToInstance();
}

void CEdBehaviorGraphProfiler::OnTick( Float dt )
{
	if ( m_connected )
	{
		m_updateTimeEdit[0]->SetValue( wxString::Format( wxT("%1.3f"), m_updateTime ) );
		m_sampleTimeEdit[0]->SetValue( wxString::Format( wxT("%1.3f"), m_sampleTime ) );
		m_allTimeEdit[0]->SetValue( wxString::Format( wxT("%1.3f"), m_updateTime + m_sampleTime ) );

		Float updateVal = m_filteredUpdateTime.Update( m_updateTime );
		Float sampleVal = m_filteredUpdateTime.Update( m_sampleTime );

		m_updateTimeEdit[1]->SetValue( wxString::Format( wxT("%1.3f"), updateVal ) );
		m_sampleTimeEdit[1]->SetValue( wxString::Format( wxT("%1.3f"), sampleVal ) );
		m_allTimeEdit[1]->SetValue( wxString::Format( wxT("%1.3f"), updateVal + sampleVal ) );
	}
}

void CEdBehaviorGraphProfiler::OnPreUpdateInstance( Float& dt )
{
	m_updateTimer.ResetTimer();
}

void CEdBehaviorGraphProfiler::OnPostUpdateInstance( Float dt )
{
	m_updateTime = m_updateTimer.GetTimePeriodMS();
}

void CEdBehaviorGraphProfiler::OnPreSampleInstance()
{
	m_sampleTimer.ResetTimer();
}

void CEdBehaviorGraphProfiler::OnPostSampleInstance( const SBehaviorGraphOutput& pose )
{
	m_sampleTime = m_sampleTimer.GetTimePeriodMS();
}
