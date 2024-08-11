/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behaviorRagdollPanel.h"
#include "../../common/core/diskFile.h"
#include "../../common/engine/skeletalAnimationContainer.h"

BEGIN_EVENT_TABLE( CEdBehaviorGraphPoseMatcher, CEdBehaviorEditorSimplePanel )
	EVT_TOGGLEBUTTON( XRCID( "buttConnect" ), CEdBehaviorGraphPoseMatcher::OnConnect )
	EVT_TOGGLEBUTTON( XRCID( "buttBest" ), CEdBehaviorGraphPoseMatcher::OnShowBest )
	EVT_TOGGLEBUTTON( XRCID( "buttSel" ), CEdBehaviorGraphPoseMatcher::OnShowSelected )
	EVT_TOGGLEBUTTON( XRCID( "buttAll" ), CEdBehaviorGraphPoseMatcher::OnShowAll )
	EVT_BUTTON( XRCID( "buttAdd" ), CEdBehaviorGraphPoseMatcher::OnAdd )
	EVT_BUTTON( XRCID( "buttRemove" ), CEdBehaviorGraphPoseMatcher::OnRemove )
	EVT_LIST_ITEM_SELECTED( XRCID( "poseList" ), CEdBehaviorGraphPoseMatcher::OnPoseListSelected )
	EVT_COLOURPICKER_CHANGED( XRCID( "poseColor" ), CEdBehaviorGraphPoseMatcher::OnPoseColorChanged )
	EVT_TEXT_ENTER( XRCID( "poseTimeEdit" ), CEdBehaviorGraphPoseMatcher::OnPoseTimeEdit )
	EVT_COMMAND_SCROLL( XRCID( "poseTimeSlider" ), CEdBehaviorGraphPoseMatcher::OnPoseTimeSlider )
END_EVENT_TABLE()

CEdBehaviorGraphPoseMatcher::CEdBehaviorGraphPoseMatcher( CEdBehaviorEditor* editor )
	: CEdBehaviorEditorSimplePanel( editor )
	, m_showBest( false )
	, m_showSelected( true )
	, m_showAll( false )
	, m_connected( false )
#ifdef USE_HAVOK_ANIMATION
	, m_poseMatchUtils( HK_NULL )
#endif
{
	// Load designed frame from resource
	wxPanel* innerPanel = wxXmlResource::Get()->LoadPanel( this, wxT("BehaviorGraphPoseMatcher") );
	SetMinSize( innerPanel->GetSize() );

	wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
	sizer->Add( innerPanel, 1, wxEXPAND|wxALL, 0 );

	m_animTree = XRCCTRL( *this, "animTree", wxTreeCtrl );
	m_poseList = XRCCTRL( *this, "poseList", wxListCtrl );
	m_colorPicker = XRCCTRL( *this, "poseColor", wxColourPickerCtrl );
	m_valueEdit = XRCCTRL( *this, "poseValue", wxTextCtrl );
	m_timeEdit = XRCCTRL( *this, "poseTimeEdit", wxTextCtrl );
	m_timeSlider = XRCCTRL( *this, "poseTimeSlider", wxSlider );
	m_choiceRootBone = XRCCTRL( *this, "choiceRoot", wxChoice );
	m_choiceFirstBone = XRCCTRL( *this, "choiceFirst", wxChoice );
	m_choiceSecondBone = XRCCTRL( *this, "choiceSecond", wxChoice );

	SetupPoseList();

	SetSizer( sizer );
	Layout();
}

wxAuiPaneInfo CEdBehaviorGraphPoseMatcher::GetPaneInfo() const
{
	wxAuiPaneInfo info = CEdBehaviorEditorPanel::GetPaneInfo();

	info.Floatable( true ).Float().MinSize( GetMinSize() ).BestSize( 175, 70 ).Dockable( false );

	return info;
}

void CEdBehaviorGraphPoseMatcher::OnTick( Float dt )
{
	if ( m_connected )
	{
		UpdatePoses();
		UpdatePoseListValues();
	}
}

void CEdBehaviorGraphPoseMatcher::CreatePoseMatchingTool()
{
	Int32 root, first, second;

	if ( GetBones( root, first, second ) )
	{
		//m_poseMatchUtils = new hkaPoseMatchingUtility( root, first, second, hkVector4( 0.f, 0.f, 1.f ) );
	}
}

void CEdBehaviorGraphPoseMatcher::DeletePoseMatchingTool()
{
#ifdef USE_HAVOK_ANIMATION
	delete m_poseMatchUtils;
	m_poseMatchUtils = HK_NULL;
#else
	//HALT( TXT( "Needs to be implemented" ) );
#endif
}

void CEdBehaviorGraphPoseMatcher::OnPanelClose()
{
	DeletePoseMatchingTool();
	DeleteAllPoses();
}

void CEdBehaviorGraphPoseMatcher::OnReset()
{
	CreatePoseMatchingTool();
	FillAnimTree();
}

void CEdBehaviorGraphPoseMatcher::OnLoadEntity()
{
	CreatePoseMatchingTool();
	FillAnimTree();
}

void CEdBehaviorGraphPoseMatcher::OnUnloadEntity()
{
	DeletePoseMatchingTool();
	DeleteAllPoses();
	FillAnimTree();
}

void CEdBehaviorGraphPoseMatcher::OnConnect( wxCommandEvent& event )
{
	m_connected = event.IsChecked();

	for ( Uint32 i=0; i<m_poses.Size(); ++i )
	{
		m_poses[i].m_visualPose->SetVisible( m_connected );
	}
}

void CEdBehaviorGraphPoseMatcher::OnShowBest( wxCommandEvent& event )
{
	m_showBest = event.IsChecked();
}

void CEdBehaviorGraphPoseMatcher::OnShowSelected( wxCommandEvent& event )
{
	m_showSelected = event.IsChecked();
}

void CEdBehaviorGraphPoseMatcher::OnShowAll( wxCommandEvent& event )
{
	m_showSelected = event.IsChecked();
}

void CEdBehaviorGraphPoseMatcher::OnAdd( wxCommandEvent& event )
{
	wxTreeItemId item = m_animTree->GetSelection();

	if ( item.IsOk() && m_animTree->GetChildrenCount( item, true ) == 0 )
	{
		CName animation( m_animTree->GetItemText( item ) );

		ASSERT( GetAnimatedComponent()->GetAnimationContainer()->FindAnimation( animation ) );

		// Create pose
		CreatePose( animation );
	}
}

void CEdBehaviorGraphPoseMatcher::SetupPoseList()
{
	m_poseList->InsertColumn( POSE_LIST_ANIM_COL, wxT("Animation") );
	m_poseList->InsertColumn( POSE_LIST_TIME_COL, wxT("Time") );
	m_poseList->InsertColumn( POSE_LIST_VALUE_COL, wxT("Value") );
}

void CEdBehaviorGraphPoseMatcher::UpdateFullPoseList()
{
	m_poseList->Freeze();
	m_poseList->ClearAll();

	SetupPoseList();

	for ( Uint32 i=0; i<m_poses.Size(); ++i )
	{
		SPoseItem& pose = m_poses[i];

		// Animation
		{
			m_poseList->InsertItem(  i, pose.m_animationName.AsString().AsChar() );
		}

		// Time
		{
			wxListItem info;
			info.SetId( i );
			info.SetColumn( POSE_LIST_TIME_COL );
			info.SetText( wxString::Format( wxT("%.2f"), pose.m_time ) );
			m_poseList->SetItem( info );
		}

		// Value
		{
			wxListItem info;
			info.SetId( i );
			info.SetColumn( POSE_LIST_VALUE_COL );
			info.SetText( wxString::Format( wxT("%.2f"), pose.m_value ) );
			m_poseList->SetItem( info );
		}
	}

	m_poseList->Thaw();

	SelectPose( -1 );
}

void CEdBehaviorGraphPoseMatcher::UpdatePoseListValues()
{
	m_poseList->Freeze();

	for ( Uint32 i=0; i<m_poses.Size(); ++i )
	{
		SPoseItem& pose = m_poses[i];

		wxListItem info;

		info.SetId( i );
		info.SetColumn( POSE_LIST_VALUE_COL );
		info.SetText( wxString::Format( wxT("%.2f"), pose.m_value ) );
		m_poseList->SetItem( info );
	}

	m_poseList->Thaw();
}

void CEdBehaviorGraphPoseMatcher::UpdatePoses()
{
	//...


	UpdatePoseListValues();
}

void CEdBehaviorGraphPoseMatcher::SetPoseTime( SPoseItem& pose, Float time )
{
	pose.m_time = time;
	pose.m_visualPose->SetAnimationAndTime( pose.m_animationName, pose.m_time );
}

void CEdBehaviorGraphPoseMatcher::CreatePose( const CName& animation )
{
	ASSERT( GetEntity() );

	CAnimatedComponent* ac = GetAnimatedComponent();

	SPoseItem pose;
	pose.m_animationName = animation;

	pose.m_visualPose = CreateVisualPose();
	SetPoseTime( pose, 0.f );
	pose.m_visualPose->SetVisible( m_connected );

	pose.m_animEntry = ac->GetAnimationContainer()->FindAnimation( pose.m_animationName );
	ASSERT( pose.m_animEntry );

	m_poses.PushBack( pose );

	UpdateFullPoseList();
}

void CEdBehaviorGraphPoseMatcher::DeletePose( Uint32 index )
{
	ASSERT( GetEntity() );
	ASSERT( index < m_poses.Size() );

	if ( index < m_poses.Size() )
	{
		SPoseItem& pose = m_poses[index];

		DestroyVisualPose( pose.m_visualPose );

		m_poses.Erase( m_poses.Begin() + index );
	}

	UpdateFullPoseList();
}

void CEdBehaviorGraphPoseMatcher::DeleteAllPoses()
{
	ASSERT( GetEntity() );

	for ( Uint32 i=0; i<m_poses.Size(); ++i )
	{
		DestroyVisualPose( m_poses[i].m_visualPose );
	}
	m_poses.Clear();

	UpdateFullPoseList();
}

Color CEdBehaviorGraphPoseMatcher::RandColor() const
{
	static Uint32 colorMask = 1;
	colorMask = (colorMask + 1) % 7;	
	if ( !colorMask ) colorMask = 1;

	return Color(	colorMask & 1 ? 255 : 0, 
		colorMask & 2 ? 255 : 0,
		colorMask & 4 ? 255 : 0 );
}

void CEdBehaviorGraphPoseMatcher::SelectPose( Int32 num )
{
	if ( num == -1 || num >= (Int32)m_poses.Size() )
	{
		m_colorPicker->SetColour( *wxBLACK );
	}
	else
	{
		SPoseItem& pose = m_poses[ num ];

		CAnimatedComponent* ac = GetAnimatedComponent();
		
		
		if ( !pose.m_animEntry )
		{
			ASSERT( pose.m_animEntry );
			return;
		}

		// Color
		ASSERT( pose.m_visualPose );
		const Color& poseColor = pose.m_visualPose->GetColor();
		m_colorPicker->SetColour( wxColour( poseColor.R, poseColor.G, poseColor.B ) );

		Float time = pose.m_time;
		Float totalTime = pose.m_animEntry->GetDuration();

		// Time edit
		m_timeEdit->SetLabel( wxString::Format( wxT("%f"), time ) );

		// Time slider
		{
			Int32 minVal = m_timeSlider->GetMin();
			Int32 maxVal = m_timeSlider->GetMax();
			Float range = (Float)( maxVal - minVal );

			Float p = time/totalTime;
			Int32 value = Clamp< Int32 >( p * range, minVal, maxVal );

			m_timeSlider->SetValue( value );
		}
	}
}

void CEdBehaviorGraphPoseMatcher::OnRemove( wxCommandEvent& event )
{
	long item = m_poseList->GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
	if ( item != -1 && item <= (long)m_poses.Size() )
	{
		DeletePose( (Uint32)item );
	}
}

void CEdBehaviorGraphPoseMatcher::OnPoseListSelected( wxListEvent& event )
{
	long item = m_poseList->GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
	SelectPose( item );
}

void CEdBehaviorGraphPoseMatcher::OnPoseColorChanged( wxColourPickerEvent& event )
{
	long item = m_poseList->GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
	if ( item != -1 && item <= (long)m_poses.Size() )
	{
		wxColour selColour = event.GetColour();
		Color color( selColour.Red(), selColour.Green(), selColour.Blue() );
		m_poses[ item ].m_visualPose->SetColor( color );
	}
}

void CEdBehaviorGraphPoseMatcher::OnPoseTimeEdit( wxCommandEvent& event )
{
	long item = m_poseList->GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
	if ( item != -1 && item <= (long)m_poses.Size() )
	{
		Float duration = m_poses[ item ].m_animEntry->GetDuration();
		Float time;
		
		Bool ret = FromString( m_timeEdit->GetValue().wc_str(), time );
		if ( !ret )
		{
			time = 0.f;
		}

		SetPoseTime( m_poses[ item ], Clamp( time, 0.f, duration ) );
		
		// Edit
		m_timeEdit->SetLabel( wxString::Format( wxT("%.2f"), m_poses[ item ].m_time ) );

		// Slider
		{
			Int32 minVal = m_timeSlider->GetMin();
			Int32 maxVal = m_timeSlider->GetMax();
			Float range = (Float)( maxVal - minVal );
			if ( range <= 0.0f )
			{
				range = 1.0f;
			}

			Float p = (Float)m_poses[ item ].m_time / duration;
			m_timeSlider->SetValue( Clamp< Int32 >( minVal + range * p, minVal, maxVal ) );
		}
	}
}

void CEdBehaviorGraphPoseMatcher::OnPoseTimeSlider( wxScrollEvent& event )
{
	long item = m_poseList->GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
	if ( item != -1 && item <= (long)m_poses.Size() )
	{
		Int32 value = m_timeSlider->GetValue();
		Int32 minVal = m_timeSlider->GetMin();
		Int32 maxVal = m_timeSlider->GetMax();

		Float range = (Float)( maxVal - minVal );
		if ( range <= 0.0f )
		{
			range = 1.0f;
		}

		Float p = (Float)value / range;
		ASSERT( p>=0.f && p<= 1.f );

		Float duration = m_poses[ item ].m_animEntry->GetDuration();

		SetPoseTime( m_poses[ item ], Clamp( duration * p, 0.f, duration ) );

		// Edit
		m_timeEdit->SetLabel( wxString::Format( wxT("%.2f"), m_poses[ item ].m_time ) );
	}
}

void CEdBehaviorGraphPoseMatcher::FillAnimTree()
{
	m_animTree->Freeze();
	m_animTree->DeleteAllItems();

	wxTreeItemId root = m_animTree->AddRoot( wxT("Animations") );

	CAnimatedComponent* ac = GetAnimatedComponent();
	if ( ac )
	{
		const TSkeletalAnimationSetsArray& sets = ac->GetAnimationContainer()->GetAnimationSets();

		for( auto it = sets.Begin(), end = sets.End(); it != end; ++it )
		{
			// Add set
			const CSkeletalAnimationSet* set = ( *it ).Get();
			wxString setName = set->GetFile()->GetFileName().StringBefore(TXT(".")).AsChar();

			wxTreeItemId localRoot = m_animTree->AppendItem( root, setName );

			// Add animations
			TDynArray< CSkeletalAnimationSetEntry* > animation;
			set->GetAnimations( animation );

			for ( Uint32 i=0; i<animation.Size(); ++i )
			{
				if ( animation[i]->GetAnimation() )
				{
					wxString animName( animation[i]->GetAnimation()->GetName().AsString().AsChar() );
					m_animTree->AppendItem( localRoot, animName );
				}
			}
		}
	}

	m_animTree->Expand( root );

	m_animTree->Thaw();
	m_animTree->Refresh();
}

CBehaviorDebugVisualizer* CEdBehaviorGraphPoseMatcher::CreateVisualPose()
{
	CEntity *entity = GetEntity();
	CAnimatedComponent *animatedComponent = GetAnimatedComponent();	

	CBehaviorDebugVisualizer* visualPose = Cast< CBehaviorDebugVisualizer >( entity->CreateComponent( CBehaviorDebugVisualizer::GetStaticClass(), SComponentSpawnInfo() ) );
	visualPose->SetAnimatedComponent( animatedComponent );
	visualPose->SetEditor( GetEditor() );
	visualPose->SetColor( RandColor() );

	return visualPose;
}

void CEdBehaviorGraphPoseMatcher::DestroyVisualPose( CBehaviorDebugVisualizer* visualPose )
{
	GetEntity()->DestroyComponent( visualPose );
}

void CEdBehaviorGraphPoseMatcher::FillBoneChoices()
{
	//m_choiceRootBone;
	//m_choiceFirstBone;
	//m_choiceSecondBone;
}

Bool CEdBehaviorGraphPoseMatcher::GetBones( Int32& root, Int32& first, Int32& second )
{
	return false;
}
