
#include "build.h"
#include "animFriend.h"
#include "lazyWin32feedback.h"
#include "animBuilderTimeline.h"
#include "defaultCharactersIterator.h"
#include "callbackData.h"
#include "behaviorBoneSelectionDlg.h"
#include "../../common/engine/behaviorGraphStack.h"
#include "../../common/engine/skeletalAnimationEntry.h"
#include "../../common/engine/animatedSkeleton.h"
	

#define ID_TOOL_RESET		5001
#define ID_TOOL_PLAY		5002
#define ID_TOOL_STOP		5003
#define ID_PLAY_ONE			5004
#define ID_PLAY_ONE_BACK	5005
#define ID_EX_MOTION		5006
#define ID_GHOSTS			5007

BEGIN_EVENT_TABLE( CEdAnimationFriend, wxSmartLayoutPanel ) 
END_EVENT_TABLE()

//////////////////////////////////////////////////////////////////////////

CEdAnimationFriend::CEdAnimationFriend( wxWindow* parent )
	: wxSmartLayoutPanel( parent, TXT("AnimationFriend"), false )
	, m_animatedComponent( NULL )
{
	SetTitle( wxString::Format( wxT("Animation friend") ) );

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
			m_toolbar->Connect( ID_TOOL_RESET, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAnimationFriend::OnResetPlayback ), NULL, this );

			m_toolbar->AddTool( ID_TOOL_PLAY, wxT("Play"), SEdResources::GetInstance().LoadBitmap(_T("IMG_CONTROL_PLAY")), wxT("Play/Pause") );
			m_toolbar->Connect( ID_TOOL_PLAY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAnimationFriend::OnPlayPause ), NULL, this );


			{
				wxStaticLine * staticline = new wxStaticLine( m_toolbar, wxID_ANY, wxDefaultPosition, wxSize( -1,15 ), wxLI_VERTICAL );
				m_toolbar->AddControl( staticline );
			}

			m_toolbar->AddTool( ID_PLAY_ONE_BACK, wxT("PlayOneBack"), SEdResources::GetInstance().LoadBitmap(_T("IMG_CONTROL_REWIND")), wxT("Play back one frame") );
			m_toolbar->Connect( ID_PLAY_ONE_BACK, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAnimationFriend::OnPlayBackOneFrame ), NULL, this );

			m_toolbar->AddTool( ID_PLAY_ONE, wxT("PlayOne"), SEdResources::GetInstance().LoadBitmap(_T("IMG_CONTROL_FORWARD")), wxT("Play one frame") );
			m_toolbar->Connect( ID_PLAY_ONE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAnimationFriend::OnPlayOneFrame ), NULL, this );

			{
				wxStaticLine * staticline = new wxStaticLine( m_toolbar, wxID_ANY, wxDefaultPosition, wxSize( -1,15 ), wxLI_VERTICAL );
				m_toolbar->AddControl( staticline );
			}

			m_toolbar->AddSeparator();

			m_toolbar->AddTool( ID_EX_MOTION, wxT("ExMotion"), SEdResources::GetInstance().LoadBitmap(_T("IMG_CAR")), wxT("Use motion"), wxITEM_CHECK );
			m_toolbar->Connect( ID_EX_MOTION, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAnimationFriend::OnToggleExMotion ), NULL, this );

			m_toolbar->AddSeparator();

			{
				wxStaticLine * staticline = new wxStaticLine( m_toolbar, wxID_ANY, wxDefaultPosition, wxSize( -1,15 ), wxLI_VERTICAL );
				m_toolbar->AddControl( staticline );
			}

			m_toolbar->AddSeparator();

			wxButton* button1 = new wxButton( m_toolbar, -1, TXT("0"), wxDefaultPosition, wxSize( 30, -1 ) );
			button1->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdAnimationFriend::OnBtnTime ), new TCallbackData< Int32 >( 0 ), this ); 
			m_toolbar->AddControl( button1 );

			wxButton* button2 = new wxButton( m_toolbar, -1, TXT("10"), wxDefaultPosition, wxSize( 30, -1 ) );
			button2->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdAnimationFriend::OnBtnTime ), new TCallbackData< Int32 >( 10 ), this ); 
			m_toolbar->AddControl( button2 );

			wxButton* button3 = new wxButton( m_toolbar, -1, TXT("100"), wxDefaultPosition, wxSize( 30, -1 ) );
			button3->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdAnimationFriend::OnBtnTime ), new TCallbackData< Int32 >( 100 ), this ); 
			m_toolbar->AddControl( button3 );

			wxButton* button4 = new wxButton( m_toolbar, -1, TXT("200"), wxDefaultPosition, wxSize( 30, -1 ) );
			button4->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdAnimationFriend::OnBtnTime ), new TCallbackData< Int32 >( 200 ), this ); 
			m_toolbar->AddControl( button4 );

			m_timeSilder = new wxSlider( m_toolbar, -1, 100, 0, 200, wxDefaultPosition, wxSize( 250, -1 ) );
			m_timeSilder->Connect( wxEVT_SCROLL_THUMBTRACK, wxCommandEventHandler( CEdAnimationFriend::OnTimeSliderChanged ), NULL, this );
			m_timeSilder->Connect( wxEVT_SCROLL_CHANGED, wxCommandEventHandler( CEdAnimationFriend::OnTimeSliderChanged ), NULL, this );
			m_toolbar->AddControl( m_timeSilder );

			m_toolbar->Realize();
		}

		mainSizer->Add( m_toolbar, 0, wxEXPAND, 5 );

		{
			m_preview = new CEdAnimationPreviewWithPreviewItems( mainPanel, false, this );
			m_preview->SetDefaultContextMenu();
			m_preview->UseCameraSnapping( false );
			m_preview->SetAnimationGraphEnabled( true );
		}

		mainSizer->Add( m_preview, 1, wxEXPAND | wxALL, 5 );

		mainPanel->SetSizer( mainSizer );
		mainPanel->Layout();
		mainSizer->Fit( mainPanel );

		wxAuiPaneInfo info;
		info.Name( wxT("Preview") ).PinButton( true ).CloseButton( false ).Center().Dockable( true ).MinSize(100,100).BestSize(800, 800);
		m_mgr.AddPane( mainPanel, info );
	}

	{
		/*{
			m_preview = new CEdAnimationPreviewWithPreviewItems( this, false, this );
			m_preview->SetDefaultContextMenu();

			wxAuiPaneInfo info;
			info.Name( wxT("Preview") ).PinButton( true ).CloseButton( false ).Left().Dockable( true ).MinSize(100,100).BestSize(800, 800);
			m_mgr.AddPane( m_preview, info );
		}*/

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

		m_htmlLog = new CEdHtmlLog( htmlPanel );
		htmlSizer->Add( m_htmlLog, 1, wxALL|wxEXPAND, 5 );

		htmlPanel->SetSizer( htmlSizer );
		htmlPanel->Layout();
		htmlSizer->Fit( htmlPanel );
		notebook->AddPage( htmlPanel, wxT("Description"), false );

		wxAuiPaneInfo info;
		info.Name( wxT("Browser") ).PinButton( true ).CloseButton( false ).Right().Dockable( true ).MinSize(200,300).BestSize(320, 700.).Row( 5 ).Layer( 5 );
		m_mgr.AddPane( notebook, info );
	}

	m_mgr.Update();

	Layout();
	Show();

	SetupPerspective();

	LoadDefaultEntity();

	UpdatePlayPauseToolItem();
}

CEdAnimationFriend::~CEdAnimationFriend()
{
	if ( GetPlayedAnimation() )
	{
		GetPlayedAnimation()->RemoveAnimationListener( this );
	}

	m_mgr.UnInit();
}

void CEdAnimationFriend::AddPage( CEdAnimationFriendPage* page )
{
	m_pages.PushBack( page );

	m_mgr.AddPane( page->GetPanelWindow(), page->GetPageInfo() );

	m_mgr.Update();
}

void CEdAnimationFriend::SetAnimationFilter( CEdAnimationTreeBrowserAnimFilter* filter )
{
	m_animationBrowser->SetFilter( filter );;
}
	 
void CEdAnimationFriend::LoadEntity( const String& fileName, const String& component, const String& defaultComponent )
{
	m_preview->LoadEntity( fileName, component );

	m_animationBrowser->LoadEntity( fileName, component );

	CAnimatedComponent* loadedComponent = m_preview->GetAnimatedComponent();

	SetupLoadedEntity( loadedComponent );
}

void CEdAnimationFriend::UnloadEntity()
{
	const Uint32 size = m_pages.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		m_pages[ i ]->OnUnloadPreviewEntity();
	}
}

void CEdAnimationFriend::SetupLoadedEntity( CAnimatedComponent* component )
{
	m_animatedComponent = component;

	if ( m_animatedComponent )
	{
		m_animatedComponent->SetUseExtractedMotion( false );

		//if ( m_animatedComponent->GetBehaviorStack() )
		//{
		//	VERIFY( m_animatedComponent->GetBehaviorStack()->ActivateBehaviorInstances( CName( TXT("PlayerExploration") ) ) );
		//}

		const Uint32 size = m_pages.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			m_pages[ i ]->OnLoadPreviewEntity( m_animatedComponent );
		}
	}
	else
	{
		const Uint32 size = m_pages.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			m_pages[ i ]->OnUnloadPreviewEntity();
		}
	}
}

void CEdAnimationFriend::OnPreviewTick( Float dt )
{
	Tick( dt );
}

void CEdAnimationFriend::OnPreviewViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data )
{
	const Uint32 size = m_pages.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		m_pages[ i ]->OnViewportInput( view, key, action, data );
	}
}

void CEdAnimationFriend::OnPreviewGenerateFragments( CRenderFrame *frame )
{
	const Uint32 size = m_pages.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		m_pages[ i ]->OnGenerateFragments( frame );
	}
}

void CEdAnimationFriend::UpdatePlayPauseToolItem()
{
	if ( m_toolbar )
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
}

void CEdAnimationFriend::OnLoadPreviewEntity( CAnimatedComponent* component )
{
	m_animationBrowser->CloneAndUseAnimatedComponent( component );

	SetupLoadedEntity( component );
}

void CEdAnimationFriend::OnPreviewSelectPreviewItem( IPreviewItem* item )
{
	const Uint32 size = m_pages.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		m_pages[ i ]->OnSelectPreviewItem( item );
	}
}

void CEdAnimationFriend::OnPreviewDeselectAllPreviewItems()
{
	const Uint32 size = m_pages.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		m_pages[ i ]->OnDeselectAllPreviewItems();
	}
}

void CEdAnimationFriend::OnAnimationStarted( const CPlayedSkeletalAnimation* animation )
{
	const Uint32 size = m_pages.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		m_pages[ i ]->OnAnimationStarted( animation );
	}
}

void CEdAnimationFriend::OnAnimationBlendInFinished( const CPlayedSkeletalAnimation* animation )
{
	const Uint32 size = m_pages.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		m_pages[ i ]->OnAnimationBlendInFinished( animation );
	}
}

void CEdAnimationFriend::OnAnimationBlendOutStarted( const CPlayedSkeletalAnimation* animation )
{
	const Uint32 size = m_pages.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		m_pages[ i ]->OnAnimationBlendOutStarted( animation );
	}
}

void CEdAnimationFriend::OnAnimationFinished( const CPlayedSkeletalAnimation* animation )
{
	const Uint32 size = m_pages.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		m_pages[ i ]->OnAnimationFinished( animation );
	}
}

void CEdAnimationFriend::OnAnimationStopped( const CPlayedSkeletalAnimation* animation )
{
	const Uint32 size = m_pages.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		m_pages[ i ]->OnAnimationStopped( animation );
	}
}

void CEdAnimationFriend::Tick( Float timeDelta )
{
	const Uint32 size = m_pages.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		m_pages[ i ]->OnTick( timeDelta );
	}
}

void CEdAnimationFriend::Log( const wxString& msg )
{
	m_htmlLog->Log( msg );
}

void CEdAnimationFriend::SetPause( Bool flag )
{
	m_preview->Pause( flag );

	UpdatePlayPauseToolItem();
}

Bool CEdAnimationFriend::IsPaused() const
{
	return m_preview->IsPaused();
}

void CEdAnimationFriend::TogglePause()
{
	SetPause( !IsPaused() );
}

void CEdAnimationFriend::SetTimeMul( Float factor )
{
	m_preview->SetTimeMul( factor );
}

void CEdAnimationFriend::SetExtractedMotion( Bool flag )
{
	if ( m_animatedComponent )
	{
		m_animatedComponent->SetUseExtractedMotion( flag );
	}
}

Bool CEdAnimationFriend::UseExtractedMotion() const
{
	return m_animatedComponent ? m_animatedComponent->UseExtractedMotion() : false;
}

IPreviewItemContainer* CEdAnimationFriend::GetPreviewItemContainer()
{
	return m_preview;
}

void CEdAnimationFriend::CollectAllAnimations( TDynArray< const CSkeletalAnimationSetEntry* >& anims ) const
{
	m_animationBrowser->CollectAllAnimations( anims );
}

const CAnimatedComponent* CEdAnimationFriend::GetAnimatedComponent() const
{
	return m_animatedComponent;
}

CPlayedSkeletalAnimation* CEdAnimationFriend::PlayAnimation( const CSkeletalAnimationSetEntry* animation )
{
	if ( m_animatedComponent->GetBehaviorStack() && m_animatedComponent->GetBehaviorStack()->IsActive() )
	{
		m_animatedComponent->GetBehaviorStack()->Deactivate();
	}

	CSkeletalAnimationSetEntry* a = const_cast< CSkeletalAnimationSetEntry* >( animation );
	CPlayedSkeletalAnimation* pa = m_animatedComponent ? m_animatedComponent->GetAnimatedSkeleton()->PlayAnimation( a ) : NULL;
	if ( pa )
	{
		pa->AddAnimationListener( this );
	}
	return pa;
}

CPlayedSkeletalAnimation* CEdAnimationFriend::PlaySingleAnimation( const CSkeletalAnimationSetEntry* animation )
{
	if ( m_animatedComponent->GetBehaviorStack() && m_animatedComponent->GetBehaviorStack()->IsActive() )
	{
		m_animatedComponent->GetBehaviorStack()->Deactivate();
	}

	CSkeletalAnimationSetEntry* a = const_cast< CSkeletalAnimationSetEntry* >( animation );
	CPlayedSkeletalAnimation* pa = m_animatedComponent ? m_animatedComponent->GetAnimatedSkeleton()->PlayAnimation( a, true, false ) : NULL;
	if ( pa )
	{
		pa->AddAnimationListener( this );
	}
	return pa;
}

CPlayedSkeletalAnimation* CEdAnimationFriend::GetPlayedAnimation()
{
	return m_animatedComponent && m_animatedComponent->GetAnimatedSkeleton()->IsPlayingAnyAnimation() ? m_animatedComponent->GetAnimatedSkeleton()->GetPlayedAnimation( 0 ) : NULL;
}

void CEdAnimationFriend::StopAnimation()
{
	CPlayedSkeletalAnimation* animation = GetPlayedAnimation();
	if ( animation )
	{
		animation->Stop();
	}

	if ( m_animatedComponent->GetBehaviorStack() && !m_animatedComponent->GetBehaviorStack()->IsActive() )
	{
		m_animatedComponent->GetBehaviorStack()->Activate();
	}
}

void CEdAnimationFriend::PauseAnimation()
{
	CPlayedSkeletalAnimation* animation = GetPlayedAnimation();
	if ( animation )
	{
		animation->Pause();
	}
}

void CEdAnimationFriend::SetCustomPagePerspective( const wxString& code )
{
	//wxString data = m_mgr.SavePerspective();
	//LOG_EDITOR( TXT("%s"), data.c_str() );

	m_mgr.LoadPerspective( code, true );
	m_mgr.Update();
}

void CEdAnimationFriend::SetAnimationTime( Float time )
{
	//if ( m_animatedComponent->GetAnimatedSkeleton()->GetNumPlayedAnimations() > 0 )
	//{
	//	m_animatedComponent->GetAnimatedSkeleton()->GetPlayedAnimation( 0 )->SetTime( time );
	//}

	m_preview->RefreshWorld();
}

void CEdAnimationFriend::SetupPerspective()
{
	//wxString str = wxT("layout2|name=Timeline;caption=;state=2044;dir=5;layer=0;row=0;pos=0;prop=100000;bestw=800;besth=800;minw=100;minh=100;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|name=Preview;caption=;state=2044;dir=1;layer=0;row=0;pos=0;prop=100000;bestw=800;besth=800;minw=100;minh=100;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|name=Browser;caption=;state=2044;dir=2;layer=1;row=0;pos=0;prop=100000;bestw=320;besth=700;minw=200;minh=300;maxw=-1;maxh=-1;floatx=905;floaty=355;floatw=400;floath=334|dock_size(5,0,0)=360|dock_size(1,0,0)=557|dock_size(2,1,0)=322|");
	//m_mgr.LoadPerspective( str, true );
	//m_mgr.Update();
}

void CEdAnimationFriend::LoadDefaultEntity()
{
	DefaultCharactersIterator it;
	if ( it )
	{
		LoadEntity( it.GetPath(), String::EMPTY, String::EMPTY );
	}
}

void CEdAnimationFriend::OnTimelineTimeDrag( wxCommandEvent& event )
{
	// No comment... Mcinek
	TClientDataWrapper< Float>* clientData = dynamic_cast< TClientDataWrapper< Float >* >( event.GetClientObject() );
	ASSERT( clientData != NULL );

	Float time = clientData->GetData();

	SetAnimationTime( time );
}

void CEdAnimationFriend::OnTimelineChanged( wxCommandEvent& event )
{
	m_preview->Refresh();
}

void CEdAnimationFriend::OnResetPlayback( wxCommandEvent& event )
{
	SetAnimationTime( 0.f );
}

void CEdAnimationFriend::OnPlayPause( wxCommandEvent& event )
{
	SetPause( !IsPaused() );
}

void CEdAnimationFriend::OnPlayOneFrame( wxCommandEvent& event )
{
	m_preview->ForceOneFrame();
}

void CEdAnimationFriend::OnPlayBackOneFrame( wxCommandEvent& event )
{
	m_preview->ForceOneFrameBack();
}

void CEdAnimationFriend::OnTimeSliderChanged( wxCommandEvent& event )
{
	SetTimeMul( (Float)m_timeSilder->GetValue() / 100.f );
}

void CEdAnimationFriend::OnBtnTime( wxCommandEvent& event )
{
	TCallbackData< Int32 >* data = static_cast< TCallbackData< Int32 >* >( event.m_callbackUserData );
	SetTimeMul( data->GetData() / 100.f );

	m_timeSilder->SetValue( Clamp( data->GetData(), 0, 200 ) );
}

void CEdAnimationFriend::OnToggleExMotion( wxCommandEvent& event )
{
	SetExtractedMotion( !UseExtractedMotion() );
}
