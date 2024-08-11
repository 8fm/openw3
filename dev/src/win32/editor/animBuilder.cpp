/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "lazyWin32feedback.h"
#include "animBuilder.h"
#include "animBuilderTimeline.h"
#include "defaultCharactersIterator.h"
#include "../../common/engine/virtualAnimation.h"
#include "../../common/engine/virtualSkeletalAnimation.h"
#include "callbackData.h"
#include "behaviorBoneSelectionDlg.h"
#include "ghostConfigDialog.h"
#include "../../common/engine/playedAnimation.h"
#include "../../common/engine/skeleton.h"

enum
{
	ID_TOOL_RESET = wxID_HIGHEST,
	ID_TOOL_PLAY,
	ID_TOOL_STOP,
	ID_PLAY_ONE,
	ID_PLAY_ONE_BACK,
	ID_EX_MOTION,
	ID_GHOSTS,
	ID_GHOSTS_CONFIG
};

wxIMPLEMENT_CLASS( CEdAnimationBuilder, wxSmartLayoutPanel );

BEGIN_EVENT_TABLE( CEdAnimationBuilder, wxSmartLayoutPanel ) 
	EVT_MENU( XRCID( "fileSave"), CEdAnimationBuilder::OnSave )
END_EVENT_TABLE()

//////////////////////////////////////////////////////////////////////////

CEdAnimationBuilder::CEdAnimationBuilder( wxWindow* parent, CSkeletalAnimationSetEntry* animationEntry, const CAnimatedComponent* templateComponent )
	: wxSmartLayoutPanel( parent, TXT("AnimationBuilder"), false )
	, m_animatedComponent( NULL )
	, m_playedAnimation( NULL )
	, m_animationEntry( animationEntry )
	, m_ghostCount( 10 )
	, m_ghostType( PreviewGhostContainer::GhostType_Entity )
{
	SetTitle( wxString::Format( wxT("Animation builder - %s - [%s]"), animationEntry->GetName().AsString().AsChar(), animationEntry->GetAnimSet()->GetDepotPath().AsChar() ) );

	m_animation = SafeCast< CVirtualSkeletalAnimation >( animationEntry->GetAnimation() );
	m_animation->ConnectEditorMixerLinstener( this );

	SetMinSize( wxSize( 800, 800 ) );

	m_mgr.SetManagedWindow( this );

	wxIcon iconSmall;
	iconSmall.CopyFromBitmap( SEdResources::GetInstance().LoadBitmap( _T("IMG_TOOL") ) );
	SetIcon( iconSmall );

	m_playIcon	= SEdResources::GetInstance().LoadBitmap( TEXT("IMG_CONTROL_PLAY") );
	m_pauseIcon = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_CONTROL_PAUSE") );

	{
		wxPanel* mainPanel = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
		wxBoxSizer* mainSizer = new wxBoxSizer( wxVERTICAL );

		{
			m_toolbar = new wxToolBar( mainPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTB_HORIZONTAL ); 

			m_toolbar->AddTool( ID_TOOL_RESET, wxT("Reset"), SEdResources::GetInstance().LoadBitmap(_T("IMG_CONTROL_START")), wxT("Reset") );
			m_toolbar->Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdAnimationBuilder::OnResetPlayback, this, ID_TOOL_RESET );

			m_toolbar->AddTool( ID_TOOL_PLAY, wxT("Play"), SEdResources::GetInstance().LoadBitmap(_T("IMG_CONTROL_PLAY")), wxT("Play/Pause") );
			m_toolbar->Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdAnimationBuilder::OnPlayPause, this, ID_TOOL_PLAY );

			{
				wxStaticLine * staticline = new wxStaticLine( m_toolbar, wxID_ANY, wxDefaultPosition, wxSize( -1,15 ), wxLI_VERTICAL );
				m_toolbar->AddControl( staticline );
			}

			m_toolbar->AddTool( ID_PLAY_ONE_BACK, wxT("PlayOneBack"), SEdResources::GetInstance().LoadBitmap(_T("IMG_CONTROL_REWIND")), wxT("Play back one frame") );
			m_toolbar->Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdAnimationBuilder::OnPlayBackOneFrame, this, ID_PLAY_ONE_BACK );

			m_toolbar->AddTool( ID_PLAY_ONE, wxT("PlayOne"), SEdResources::GetInstance().LoadBitmap(_T("IMG_CONTROL_FORWARD")), wxT("Play one frame") );
			m_toolbar->Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdAnimationBuilder::OnPlayOneFrame, this, ID_PLAY_ONE );

			{
				wxStaticLine * staticline = new wxStaticLine( m_toolbar, wxID_ANY, wxDefaultPosition, wxSize( -1,15 ), wxLI_VERTICAL );
				m_toolbar->AddControl( staticline );
			}

			m_toolbar->AddSeparator();

			m_toolbar->AddTool( ID_EX_MOTION, wxT("ExMotion"), SEdResources::GetInstance().LoadBitmap(_T("IMG_CAR")), wxT("Use motion"), wxITEM_CHECK );
			m_toolbar->Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdAnimationBuilder::OnToggleExMotion, this, ID_EX_MOTION );

			m_toolbar->AddTool( ID_GHOSTS, wxT( "Ghosts" ), SEdResources::GetInstance().LoadBitmap( wxT( "IMG_ANIMATION_GHOST" ) ), wxT( "Ghosts" ), wxITEM_CHECK );
			m_toolbar->Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdAnimationBuilder::OnToggleGhosts, this, ID_GHOSTS );

			m_toolbar->AddTool( ID_GHOSTS_CONFIG, wxT( "Configure Ghosts" ), SEdResources::GetInstance().LoadBitmap( wxT( "IMG_ANIMATION_GHOSTCONFIG" ) ), wxT( "Configure Ghosts" ) );
			m_toolbar->Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdAnimationBuilder::OnGhostConfig, this, ID_GHOSTS_CONFIG );

			m_toolbar->AddSeparator();

			{
				wxStaticLine * staticline = new wxStaticLine( m_toolbar, wxID_ANY, wxDefaultPosition, wxSize( -1,15 ), wxLI_VERTICAL );
				m_toolbar->AddControl( staticline );
			}

			m_toolbar->AddSeparator();

			TStaticArray< Int32, 4 > presets;
			presets.PushBack( 0 );
			presets.PushBack( 10 );
			presets.PushBack( 100 );
			presets.PushBack( 200 );

			for( Uint32 i = 0; i < presets.Size(); ++i )
			{
				wxButton* button = new wxButton( m_toolbar, wxID_ANY, ( ToString( presets[ i ] ) + TXT( "%" ) ).AsChar(), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
				button->Bind( wxEVT_COMMAND_BUTTON_CLICKED, &CEdAnimationBuilder::OnBtnTime, this, wxID_ANY, wxID_ANY, new TCallbackData< Int32 >( presets[ i ] ) ); 
				button->SetToolTip( String::Printf( TXT( "Playback animation at %i%%" ), presets[ i ] ).AsChar() );
				
				m_toolbar->AddControl( button );
			}

			m_timeSilder = new wxSlider( m_toolbar, wxID_ANY, 100, 0, 200, wxDefaultPosition, wxSize( 250, -1 ) );
			m_timeSilder->Bind( wxEVT_SCROLL_THUMBTRACK, &CEdAnimationBuilder::OnTimeSliderChanged, this );
			m_timeSilder->Bind( wxEVT_SCROLL_CHANGED, &CEdAnimationBuilder::OnTimeSliderChanged, this );
			m_timeSilder->SetToolTip( TXT( "Change animation playback speed" ) );

			for( Uint32 i = 0; i < presets.Size(); ++i )
			{
				m_timeSilder->SetTick( presets[ i ] );
			}

			m_toolbar->AddControl( m_timeSilder );

			m_toolbar->Realize();
		}

		mainSizer->Add( m_toolbar, 0, wxEXPAND, 5 );

		//timeline = new wxPanel( mainPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
		{
			m_timeline = new CEdAnimBuilderTimeline( mainPanel, this );
			m_timeline->Bind( usrEVT_TIMELINE_REQUEST_SET_TIME, &CEdAnimationBuilder::OnTimelineRequestSetTime, this );
			m_timeline->Bind( usrEVT_REFRESH_PREVIEW, &CEdAnimationBuilder::OnTimelineChanged, this );
		}
		mainSizer->Add( m_timeline, 1, wxEXPAND | wxALL, 5 );

		mainPanel->SetSizer( mainSizer );
		mainPanel->Layout();
		mainSizer->Fit( mainPanel );

		wxAuiPaneInfo info;
		info.Name( wxT("Timeline") ).PinButton( true ).CloseButton( false ).Center().Dockable( true ).MinSize(100,100).BestSize(800, 800);
		m_mgr.AddPane( mainPanel, info );
	}

	{
		m_preview = new CEdAnimationPreview( this, false, this );
		m_preview->SetDefaultContextMenu();
		m_preview->AddRotationWidgets();
		m_preview->AddTranslationWidgets();

		wxAuiPaneInfo info;
		info.Name( wxT("Preview") ).PinButton( true ).CloseButton( false ).Top().Dockable( true ).MinSize(100,100).BestSize(800, 800);
		m_mgr.AddPane( m_preview, info );
	}

	{
		wxNotebook* notebook = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
		
		wxPanel* browserParentPanel = new wxPanel( notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
		
		wxBoxSizer* browserSizer = new wxBoxSizer( wxVERTICAL );
		SEdAnimationTreeBrowserSettings s;
		m_animationBrowser = new CEdAnimationTreeBrowser( browserParentPanel, s );
		browserSizer->Add( m_animationBrowser, 1, wxEXPAND | wxALL, 5 );

		browserParentPanel->SetSizer( browserSizer );
		browserParentPanel->Layout();
		browserSizer->Fit( browserParentPanel );

		notebook->AddPage( browserParentPanel, wxT("Browser"), false );
		wxPanel* htmlPanel = new wxPanel( notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
		wxBoxSizer* htmlSizer = new wxBoxSizer( wxVERTICAL );

		m_htmlWin = new wxHtmlWindow( htmlPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHW_SCROLLBAR_AUTO );
		htmlSizer->Add( m_htmlWin, 1, wxALL|wxEXPAND, 5 );

		htmlPanel->SetSizer( htmlSizer );
		htmlPanel->Layout();
		htmlSizer->Fit( htmlPanel );
		notebook->AddPage( htmlPanel, wxT("Description"), false );

		wxAuiPaneInfo info;
		info.Name( wxT("Browser") ).PinButton( true ).CloseButton( false ).Right().Dockable( true ).MinSize(200,300).BestSize(320, 700);
		m_mgr.AddPane( notebook, info );
	}

	/*{
		m_animationBrowser = new CEdAnimationTreeBrowser( this );

		wxAuiPaneInfo info;
		info.Name( wxT("Browser") ).PinButton( true ).CloseButton( false ).Right().Dockable( true ).MinSize(200,300).BestSize(320, 700);
		m_mgr.AddPane( m_animationBrowser, info );
	}*/

	m_fkEditor = new AnimFKPosePreviewEditor( m_preview->GetPreviewWorld() );
	m_fkEditor->InitItemContainer();
	m_fkEditor->Reset();

	m_ikEditor = new AnimIKPosePreviewEditor( m_preview->GetPreviewWorld() );
	m_ikEditor->InitItemContainer();
	m_ikEditor->Reset();

	m_mgr.Update();

	Layout();
	Show();

	SetupPerspective();

	m_timeline->Fill( m_animation );

	if ( templateComponent )
	{
		CloneEntityFrom( templateComponent );
	}
	else
	{
		LoadDefaultEntity();
	}

	if ( m_animatedComponent && m_animatedComponent->GetSkeleton() )
	{
		m_fkEditor->FillPose( m_animatedComponent->GetSkeleton() );
		m_ikEditor->FillPose( m_animatedComponent->GetSkeleton() );
	}

	UpdatePlayPauseToolItem();
}

CEdAnimationBuilder::~CEdAnimationBuilder()
{
	m_animation->ConnectEditorMixerLinstener( NULL );

	m_fkEditor->DestroyItemContainer();
	delete m_fkEditor;

	m_ikEditor->DestroyItemContainer();
	delete m_ikEditor;

	m_mgr.UnInit();
}

void CEdAnimationBuilder::CloneEntityFrom( const CAnimatedComponent* component )
{
	m_preview->CloneAndUseAnimatedComponent( component );

	m_animationBrowser->CloneAndUseAnimatedComponent( component );
	m_animatedComponent = m_preview->GetAnimatedComponent();
	m_animatedComponent->SetUseExtractedMotion( false );

	m_playedAnimation = m_preview->SetBodyAnimation( m_animationEntry );

	if ( m_animatedComponent->GetSkeleton() )
	{
		m_fkEditor->FillPose( m_animatedComponent->GetSkeleton() );
		m_ikEditor->FillPose( m_animatedComponent->GetSkeleton() );
	}
}

void CEdAnimationBuilder::LoadEntity( const String& fileName, const String& component, const String& defaultComponent )
{
	m_preview->LoadEntity( fileName, component );

	m_animationBrowser->LoadEntity( fileName, component );
	m_animatedComponent = m_preview->GetAnimatedComponent();
	m_animatedComponent->SetUseExtractedMotion( false );

	m_playedAnimation = m_preview->SetBodyAnimation( m_animationEntry );

	if ( m_animatedComponent->GetSkeleton() )
	{
		m_fkEditor->FillPose( m_animatedComponent->GetSkeleton() );
		m_ikEditor->FillPose( m_animatedComponent->GetSkeleton() );
	}
}

void CEdAnimationBuilder::UnloadEntity()
{
	m_playedAnimation = NULL;
}

void CEdAnimationBuilder::RequestAddVirtualAnimation( VirtualAnimation& anim, EVirtualAnimationTrack track )
{
	// Check if we can add animation now
	// TODO

	if ( m_animationEntry->GetAnimSet()->MarkModified() )
	{
		VERIFY( m_animation->AddAnimation( anim, track ) );
	}
}

void CEdAnimationBuilder::RequestVirtualAnimationChange( const VirtualAnimationID& animation, const VirtualAnimation& dest )
{
	// Check if we can change animation now
	// TODO

	if ( m_animationEntry->GetAnimSet()->MarkModified() )
	{
		m_animation->SetAnimation( animation, dest );
	}
}

void CEdAnimationBuilder::RequestRemoveVirtualAnimation( const VirtualAnimationID& animation )
{
	// Check if we can remove animation now
	// TODO

	if ( m_animationEntry->GetAnimSet()->MarkModified() )
	{
		DeselectEvent();

		VERIFY( m_animation->RemoveAnimation( animation ) );
	}
}

void CEdAnimationBuilder::RequestVirtualAnimationDupication( const VirtualAnimationID& animation )
{
	if ( m_animationEntry->GetAnimSet()->MarkModified() )
	{
		const CVirtualSkeletalAnimation* cont = m_animation;
		const VirtualAnimation& vanim = cont->GetVirtualAnimations( animation.m_track )[ animation.m_index ];
		VirtualAnimation newAnim = vanim;
		newAnim.m_time = vanim.m_time + vanim.GetDuration();
		VERIFY( m_animation->AddAnimation( newAnim, animation.m_track ) );
	}
}

void CEdAnimationBuilder::RequestAddVirtualMotion( VirtualAnimationMotion& motion )
{
	// Check if we can change animation now
	// TODO

	if ( m_animationEntry->GetAnimSet()->MarkModified() )
	{
		m_animation->AddMotion( motion );
	}
}

void CEdAnimationBuilder::RequestRemoveVirtualMotion( const VirtualAnimationMotionID& motion )
{
	// Check if we can change animation now
	// TODO

	if ( m_animationEntry->GetAnimSet()->MarkModified() )
	{
		DeselectEvent();

		m_animation->RemoveMotion( motion );
	}
}

void CEdAnimationBuilder::RequestVirtualMotionChange( const VirtualAnimationMotionID& motion, const VirtualAnimationMotion& dest )
{
	// Check if we can change animation now
	// TODO

	if ( m_animationEntry->GetAnimSet()->MarkModified() )
	{
		m_animation->SetMotion( motion, dest );
	}
}

void CEdAnimationBuilder::RequestAddVirtualFK( VirtualAnimationPoseFK& data )
{
	// Check if we can change animation now
	// TODO

	if ( m_animationEntry->GetAnimSet()->MarkModified() )
	{
		m_animation->AddFK( data );
	}
}

void CEdAnimationBuilder::RequestRemoveVirtualFK( const VirtualAnimationPoseFKID& dataID )
{
	// Check if we can change animation now
	// TODO

	if ( m_animationEntry->GetAnimSet()->MarkModified() )
	{
		DeselectEvent();

		m_animation->RemoveFK( dataID );
	}
}

void CEdAnimationBuilder::RequestVirtualFKChange( const VirtualAnimationPoseFKID& dataID, const VirtualAnimationPoseFK& data )
{
	// Check if we can change animation now
	// TODO

	if ( m_animationEntry->GetAnimSet()->MarkModified() )
	{
		m_animation->SetFK( dataID, data );

		SetRotationWidgetMode();
		SetWidgetLocalSpace();
		m_preview->EnableWidgets();
	}
}

void CEdAnimationBuilder::RequestAddVirtualIK( VirtualAnimationPoseIK& data )
{
	// Check if we can change animation now
	// TODO

	if ( CheckControlRig() && m_animationEntry->GetAnimSet()->MarkModified() )
	{
		m_animation->AddIK( data );

		SetTranslationWidgetMode();
		SetWidgetModelSpace();
		m_preview->EnableWidgets();
	}
}

void CEdAnimationBuilder::RequestRemoveVirtualIK( const VirtualAnimationPoseIKID& dataID )
{
	// Check if we can change animation now
	// TODO

	if ( m_animationEntry->GetAnimSet()->MarkModified() )
	{
		DeselectEvent();

		m_animation->RemoveIK( dataID );
	}
}

void CEdAnimationBuilder::RequestVirtualIKChange( const VirtualAnimationPoseIKID& dataID, const VirtualAnimationPoseIK& data )
{
	// Check if we can change animation now
	// TODO

	if ( m_animationEntry->GetAnimSet()->MarkModified() )
	{
		m_animation->SetIK( dataID, data );
	}
}

void CEdAnimationBuilder::SelectAnimationEvent( const VirtualAnimationID& id )
{
	
}

void CEdAnimationBuilder::SelectMotionEvent( const VirtualAnimationMotionID& id )
{

}

void CEdAnimationBuilder::SelectFKEvent( const VirtualAnimationPoseFKID& id )
{
	if ( m_playedAnimation )
	{
		m_playedAnimation->Pause();
	}

	m_fkEditor->Set( m_animation->GetVirtualFKs()[ id ] );
}

void CEdAnimationBuilder::SelectIKEvent( const VirtualAnimationPoseIKID& id )
{
	if ( m_playedAnimation )
	{
		m_playedAnimation->Pause();
	}

	m_ikEditor->Set( m_animation->GetVirtualIKs()[ id ] );
}

void CEdAnimationBuilder::DeselectEvent()
{
	m_fkEditor->Reset();
	m_ikEditor->Reset();

	if ( m_playedAnimation && m_playedAnimation->IsPaused() )
	{
		m_playedAnimation->Unpause();
	}

	m_preview->DisableWidgets();
}

void CEdAnimationBuilder::SelectBonesAndWeightsForAnimation( const VirtualAnimationID& animation )
{
	if ( !m_animatedComponent )
	{
		return;
	}

	const CSkeleton* skeleton = m_animatedComponent->GetSkeleton();
	if ( !skeleton )
	{
		return;
	}

	const CVirtualSkeletalAnimation* temp = m_animation;
	VirtualAnimation vanim = temp->GetVirtualAnimations( animation.m_track )[ animation.m_index ];

	const Bool withWeight = animation.m_track == VAT_Override;

	TDynArray< Int32 >& vanimBones = vanim.m_bones;
	TDynArray< Float >& vanimWeights = vanim.m_weights;

	ASSERT( vanimBones.Size() == vanimWeights.Size() );

	TDynArray< ISkeletonDataProvider::BoneInfo > bones;
	const Uint32 bonesNum = m_animatedComponent->GetBones( bones );

	//dex++: switched to generalized CSkeleton interface
	CEdBoneSelectionDialog dlg( this, skeleton, true, withWeight );
	//dex--

	for ( Uint32 i=0; i<vanimBones.Size(); ++i )
	{
		const Int32 boneIdx = vanimBones[ i ];

		dlg.SelectBones( bones[ boneIdx ].m_name.AsString(), false, false );

		if ( withWeight )
		{
			dlg.SelectBoneWeight( bones[ boneIdx ].m_name.AsString(), vanimWeights[ i ] );
		}
	}

	dlg.DoModal();

	vanimBones.Clear();
	dlg.GetSelectedBones( vanimBones );

	if ( withWeight )
	{
		vanimWeights.Clear();
		dlg.GetBonesWeight( vanimWeights );

		ASSERT( vanimBones.Size() == vanimWeights.Size() );
	}

	RequestVirtualAnimationChange( animation, vanim );
}

void CEdAnimationBuilder::StartDebuggingAnimation( const VirtualAnimationID& animation, const Color& color )
{
	const Uint32 size = m_debugAnims.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		if ( m_debugAnims[ i ].m_first == animation )
		{
			ASSERT( 0 );
			return;
		}
	}

	m_debugAnims.PushBack( TDebugAnim( animation, color ) );
}

void CEdAnimationBuilder::StopDebuggingAnimation( const VirtualAnimationID& animation )
{
	const Uint32 size = m_debugAnims.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		if ( m_debugAnims[ i ].m_first == animation )
		{
			m_debugAnims.RemoveAt( i );
			return;
		}
	}
}

Bool CEdAnimationBuilder::IsDebuggingAnimation( const VirtualAnimationID& animation ) const
{
	const Uint32 size = m_debugAnims.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		if ( m_debugAnims[ i ].m_first == animation )
		{
			return true;
		}
	}
	return false;
}

Bool CEdAnimationBuilder::IsDebuggingAnimation( const VirtualAnimationID& animation, Color& color ) const
{
	const Uint32 size = m_debugAnims.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		if ( m_debugAnims[ i ].m_first == animation )
		{
			color = m_debugAnims[ i ].m_second;
			return true;
		}
	}
	return false;
}

void CEdAnimationBuilder::OnPreviewTick( Float dt )
{
	Tick( dt );
}

void CEdAnimationBuilder::OnPreviewGenerateFragments( CRenderFrame *frame )
{
	DrawSkeletons( VAT_Base, frame );
	DrawSkeletons( VAT_Override, frame );
}

void CEdAnimationBuilder::DrawSkeletons( EVirtualAnimationTrack track, CRenderFrame *frame )
{
	if ( m_playedAnimation && m_animatedComponent )
	{
		VirtualAnimationMixer mixer( m_animation );

		TDynArray< ISkeletonDataProvider::BoneInfo > bones;
		const Uint32 bonesNum = m_animatedComponent->GetBones( bones );

		TDynArray< Matrix > transformsWS;
		transformsWS.Resize( bonesNum );

		SBehaviorGraphOutput pose;
		pose.Init( m_animation->GetBonesNum(), m_animation->GetTracksNum() );

		const Matrix& localToWorld = m_animatedComponent->GetLocalToWorld();

		const Float time = m_playedAnimation->GetTime();

		const CVirtualSkeletalAnimation* temp = m_animation;
		const TDynArray< VirtualAnimation >& vanims = temp->GetVirtualAnimations( track );

		const Uint32 size = m_debugAnims.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			if ( m_debugAnims[ i ].m_first.m_track != track )
			{
				continue;
			}

			const VirtualAnimation& vanim = vanims[ m_debugAnims[ i ].m_first.m_index ];
			Float animTime = 0.f;
			Float animWeight = 0.f;

			if ( mixer.CalcBlendingParams( time, vanim, animTime, animWeight ) )
			{
				const Color& color = m_debugAnims[ i ].m_second;

				if ( vanim.m_cachedAnimation->GetAnimation()->Sample( animTime, pose.m_numBones, pose.m_numFloatTracks, pose.m_outputPose, pose.m_floatTracks ) )
				{
					pose.GetBonesModelSpace( m_animatedComponent, transformsWS );

					SkeletonBonesUtils::MulMatrices( transformsWS.TypedData(), transformsWS.TypedData(), transformsWS.Size(), &localToWorld );

					SkeletonRenderingUtils::DrawSkeleton( transformsWS, bones, vanim.m_bones, color, frame );
				}
			}
		}
	}
}

Bool CEdAnimationBuilder::CheckControlRig()
{
	if ( m_animationEntry && m_animationEntry->GetAnimSet() )
	{
		const CSkeletalAnimationSet* set = m_animationEntry->GetAnimSet();
		if ( set->GetSkeleton() )
		{
			const CSkeleton* skeleton = set->GetSkeleton();
			if ( skeleton->GetControlRigDefinition() && skeleton->GetDefaultControlRigPropertySet() )
			{
				return true;
			}
			else
			{
				wxString msg = wxString::Format( wxT("Animation set's have skeleton doesn't have control rig params. Go to skeleton editor and setup it. Resource '%s'"), skeleton->GetDepotPath().AsChar() );
				wxMessageBox( msg, wxT("Error") );
				return false;
			}
		}
		else
		{
			wxMessageBox( wxT("Animation set doesn't have skeleton. Go to anim browser editor and connect it."), wxT("Error") );
			return false;
		}
	}

	wxMessageBox( wxT("Error"), wxT("Error") );

	return false;
}

void CEdAnimationBuilder::UpdatePlayPauseToolItem()
{
	if ( IsPaused() )
	{
		m_toolbar->SetToolNormalBitmap( ID_TOOL_PLAY, m_playIcon );
	}
	else
	{
		m_toolbar->SetToolNormalBitmap( ID_TOOL_PLAY, m_pauseIcon );
	}
}

void CEdAnimationBuilder::OnLoadPreviewEntity( CAnimatedComponent* component )
{
	//...
}

void CEdAnimationBuilder::OnPreviewHandleSelection( const TDynArray< CHitProxyObject* >& objects )
{
	m_fkEditor->HandleItemSelection( objects );
	m_ikEditor->HandleItemSelection( objects );
}

void CEdAnimationBuilder::OnAnimationTracksSampled( Uint32 boneNumIn, Uint32 tracksNumIn, AnimQsTransform* bonesOut, AnimFloat* tracksOut )
{
	m_fkEditor->UpdatePosePre( boneNumIn, bonesOut );
}

void CEdAnimationBuilder::OnFKTracksSampled( Uint32 boneNumIn, Uint32 tracksNumIn, AnimQsTransform* bonesOut, AnimFloat* tracksOut )
{
	m_ikEditor->UpdatePosePre( boneNumIn, bonesOut );
}

void CEdAnimationBuilder::OnIKTracksSampled( Uint32 boneNumIn, Uint32 tracksNumIn, AnimQsTransform* bonesOut, AnimFloat* tracksOut )
{

}

void CEdAnimationBuilder::OnAllTracksSampled( Uint32 boneNumIn, Uint32 tracksNumIn, AnimQsTransform* bonesOut, AnimFloat* tracksOut )
{
	m_fkEditor->UpdatePosePost( boneNumIn, bonesOut );
	m_ikEditor->UpdatePosePost( boneNumIn, bonesOut );
}

void CEdAnimationBuilder::Tick( Float timeDelta )
{
	if ( m_playedAnimation )
	{
		m_timeline->SetCurrentTime( m_playedAnimation->GetTime() );
		m_timeline->Repaint();
	}
}

void CEdAnimationBuilder::SetPause( Bool flag )
{
	m_preview->Pause( flag );

	UpdatePlayPauseToolItem();
}

Bool CEdAnimationBuilder::IsPaused() const
{
	return m_preview->IsPaused();
}

void CEdAnimationBuilder::TogglePause()
{
	SetPause( !IsPaused() );
}

void CEdAnimationBuilder::SetTimeMul( Float factor )
{
	m_preview->SetTimeMul( factor );
}

void CEdAnimationBuilder::SetExtractedMotion( Bool flag )
{
	m_animatedComponent->SetUseExtractedMotion( flag );
}

Bool CEdAnimationBuilder::UseExtractedMotion() const
{
	return m_animatedComponent->UseExtractedMotion();
}

void CEdAnimationBuilder::SetTranslationWidgetMode()
{
	m_preview->SetTranslationWidgetMode();
}

void CEdAnimationBuilder::SetRotationWidgetMode()
{
	m_preview->SetRotationWidgetMode();
}

void CEdAnimationBuilder::SetWidgetModelSpace()
{
	m_preview->SetWidgetModelSpace();
}

void CEdAnimationBuilder::SetWidgetLocalSpace()
{
	m_preview->SetWidgetLocalSpace();
}

void CEdAnimationBuilder::SetAnimationTime( Float time )
{
	if ( m_playedAnimation )
	{
		m_playedAnimation->SetTime( time );
	}
	m_preview->RefreshWorld();
}

void CEdAnimationBuilder::SetupPerspective()
{
	wxString str = wxT("layout2|name=Timeline;caption=;state=2044;dir=5;layer=0;row=0;pos=0;prop=100000;bestw=800;besth=800;minw=100;minh=100;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|name=Preview;caption=;state=2044;dir=1;layer=0;row=0;pos=0;prop=100000;bestw=800;besth=800;minw=100;minh=100;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|name=Browser;caption=;state=2044;dir=2;layer=1;row=0;pos=0;prop=100000;bestw=320;besth=700;minw=200;minh=300;maxw=-1;maxh=-1;floatx=905;floaty=355;floatw=400;floath=334|dock_size(5,0,0)=360|dock_size(1,0,0)=557|dock_size(2,1,0)=322|");
	m_mgr.LoadPerspective( str, true );
	m_mgr.Update();
}

Bool CEdAnimationBuilder::CheckAnimationBeforeSaving()
{
	// czy nie ma wolnych miejsc?
	//...

	return true;
}

void CEdAnimationBuilder::LoadDefaultEntity()
{
	DefaultCharactersIterator it;
	if ( it )
	{
		LoadEntity( it.GetPath(), String::EMPTY, String::EMPTY );
	}
}

void CEdAnimationBuilder::OnTimelineRequestSetTime( wxCommandEvent& event )
{
	// No comment... Mcinek
	TClientDataWrapper< Float>* clientData = dynamic_cast< TClientDataWrapper< Float >* >( event.GetClientObject() );
	ASSERT( clientData != NULL );

	Float time = clientData->GetData();

	SetAnimationTime( time );
}

void CEdAnimationBuilder::OnTimelineChanged( wxCommandEvent& event )
{
	//m_timeline->RefreshTimeline();
	m_preview->Refresh();
}

void CEdAnimationBuilder::OnSave( wxCommandEvent& event )
{
	if ( CheckAnimationBeforeSaving() )
	{
		CSkeletalAnimationSet* set = m_animationEntry->GetAnimSet();
		set->OnAnimsetPreSaved();
		set->Save();
	}
}

void CEdAnimationBuilder::OnResetPlayback( wxCommandEvent& event )
{
	SetAnimationTime( 0.f );
}

void CEdAnimationBuilder::OnPlayPause( wxCommandEvent& event )
{
	SetPause( !IsPaused() );
}

void CEdAnimationBuilder::OnPlayOneFrame( wxCommandEvent& event )
{
	m_preview->ForceOneFrame();
}

void CEdAnimationBuilder::OnPlayBackOneFrame( wxCommandEvent& event )
{
	m_preview->ForceOneFrameBack();
}

void CEdAnimationBuilder::OnTimeSliderChanged( wxCommandEvent& event )
{
	SetTimeMul( (Float)m_timeSilder->GetValue() / 100.f );
}

void CEdAnimationBuilder::OnBtnTime( wxCommandEvent& event )
{
	TCallbackData< Int32 >* data = static_cast< TCallbackData< Int32 >* >( event.m_callbackUserData );
	SetTimeMul( data->GetData() / 100.f );

	m_timeSilder->SetValue( Clamp( data->GetData(), 0, 200 ) );
}

void CEdAnimationBuilder::OnToggleExMotion( wxCommandEvent& event )
{
	SetExtractedMotion( !UseExtractedMotion() );
}

void CEdAnimationBuilder::OnToggleGhosts( wxCommandEvent& event )
{
	if ( event.IsChecked() )
	{
		m_preview->ShowGhosts( m_ghostCount, m_ghostType );
	}
	else
	{
		m_preview->HideGhosts();
	}
}

void CEdAnimationBuilder::OnGhostConfig( wxCommandEvent& event )
{
	CEdGhostConfigDialog* dialog = new CEdGhostConfigDialog( this, m_ghostCount, m_ghostType );
	dialog->Bind( wxEVT_GHOSTCONFIGUPDATED, &CEdAnimationBuilder::OnConfigureGhostsOK, this );

	dialog->Show();
}

void CEdAnimationBuilder::OnConfigureGhostsOK( wxCommandEvent& event )
{
	CEdGhostConfigDialog* dialog = wxStaticCast( event.GetEventObject(), CEdGhostConfigDialog );

	m_ghostCount = dialog->GetCount();
	m_ghostType = dialog->GetType();

	if( m_preview->HasGhosts() )
	{
		m_preview->ShowGhosts( m_ghostCount, m_ghostType );
	}
}
