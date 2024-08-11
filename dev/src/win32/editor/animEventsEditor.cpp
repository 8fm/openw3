/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"
#include "animEventsEditor.h"
#include "animBrowserPreview.h"
#include "animEventsTimeline.h"
#include "editorExternalResources.h"
#include "effectBoneListSelection.h"
#include "displayItemDlg.h"
#include "../../common/engine/behaviorGraphStack.h"
#include "../../common/core/depot.h"
#include "../../common/core/feedback.h"
#include "../../common/engine/playedAnimation.h"
#include "../../common/engine/animatedSkeleton.h"
#include "../../common/engine/viewport.h"
#include "../../common/engine/dynamicLayer.h"
#include "../../common/engine/skeleton.h"
#include "../../common/engine/worldTick.h"
#include "../../common/engine/soundSystem.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class CAnimEventsRenameAnimations : public wxDialog
{
	DECLARE_EVENT_TABLE();

public:
	CAnimEventsRenameAnimations( wxWindow* parent, String const & toReplace, String const & replaceWith );
	~CAnimEventsRenameAnimations();

	void OnOK( wxCommandEvent& event );
	void OnCancel( wxCommandEvent& event );

	String GetToReplace() const { return m_toReplace; }
	String ReplaceWith() const { return m_replaceWith; }

private:
	wxTextCtrl* m_toReplaceControl;
	wxTextCtrl* m_replaceWithControl;

	String m_toReplace;
	String m_replaceWith;
};

BEGIN_EVENT_TABLE( CAnimEventsRenameAnimations, wxDialog )
	EVT_BUTTON( XRCID("btnOK"), CAnimEventsRenameAnimations::OnOK )
	EVT_BUTTON( XRCID("btnCancel"), CAnimEventsRenameAnimations::OnCancel )
END_EVENT_TABLE()

CAnimEventsRenameAnimations::CAnimEventsRenameAnimations( wxWindow* parent, String const & toReplace, String const & replaceWith )
{
	wxXmlResource::Get()->LoadDialog( this, parent, wxT("RenameAnimationsDialog") );

	wxString toReplaceString;
	toReplaceString.Printf( TXT("%s"), toReplace.AsChar() );
	m_toReplaceControl = XRCCTRL( *this, "toReplace", wxTextCtrl );
	m_toReplaceControl->SetValue( toReplaceString );

	wxString replaceWithString;
	replaceWithString.Printf( TXT("%s"), replaceWith.AsChar() );
	m_replaceWithControl = XRCCTRL( *this, "replaceWith", wxTextCtrl );
	m_replaceWithControl->SetValue( replaceWithString );

	Layout();
}

CAnimEventsRenameAnimations::~CAnimEventsRenameAnimations()
{
}

void CAnimEventsRenameAnimations::OnOK( wxCommandEvent& event )
{
	m_toReplace = m_toReplaceControl->GetValue();
	m_replaceWith = m_replaceWithControl->GetValue();
	EndDialog(1);
}

void CAnimEventsRenameAnimations::OnCancel( wxCommandEvent& event )
{
	EndDialog(0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE( CEdAnimEventsEditor, wxFrame )
	EVT_LIST_ITEM_SELECTED( XRCID( "listAnimations" ), CEdAnimEventsEditor::OnAnimationChanged )
	EVT_CLOSE( CEdAnimEventsEditor::OnClose )
	EVT_COMMAND_SCROLL( XRCID( "sliderSpeed" ), CEdAnimEventsEditor::OnSpeedChanged )
	EVT_MENU( XRCID( "playButton" ), CEdAnimEventsEditor::OnPlayPause )
	EVT_MENU( XRCID( "resetButton"), CEdAnimEventsEditor::OnReset )	
	EVT_MENU( XRCID( "menuItemSave" ), CEdAnimEventsEditor::OnSave )
	EVT_MENU( XRCID( "menuItemReset" ), CEdAnimEventsEditor::OnResetConfig )
	EVT_MENU( XRCID( "menuItemClose" ), CEdAnimEventsEditor::OnMenuClose )
	EVT_MENU( XRCID( "menuItemUndo" ), CEdAnimEventsEditor::OnMenuUndo )
	EVT_MENU( XRCID( "menuItemRedo" ), CEdAnimEventsEditor::OnMenuRedo )
	EVT_MENU( XRCID( "movementButton"), CEdAnimEventsEditor::OnToggleCarButton )	
	EVT_MENU( XRCID( "menuItemMassActions"), CEdAnimEventsEditor::OnMassActions )
	EVT_TOOL( XRCID( "itemDisplayToolButton" ), CEdAnimEventsEditor::OnItemDisplayToolButton )
	EVT_TOOL( XRCID( "undoItemDisplayToolButton" ), CEdAnimEventsEditor::OnUndoItemDisplayToolButton )
	EVT_TOOL( XRCID( "tposeButton" ), CEdAnimEventsEditor::OnTPoseButton )
	EVT_BUTTON( XRCID( "btnUseSelectedEntity" ), CEdAnimEventsEditor::OnUseSelectedEntity )
	EVT_TEXT_ENTER( XRCID( "animationSearchField" ), CEdAnimEventsEditor::OnAnimationSearchField )
	EVT_BUTTON( XRCID( "clearSearchButton" ), CEdAnimEventsEditor::OnClearSearchButton )
	EVT_BUTTON( XRCID( "renameAnimations" ), CEdAnimEventsEditor::OnRenameAnimations )
	EVT_CHAR_HOOK( CEdAnimEventsEditor::OnCharHook )
END_EVENT_TABLE()

const String CEdAnimEventsEditor::CONFIG_PATH( TXT( "/Frames/AnimEventsEditor/" ) );
const String CEdAnimEventsEditor::CONFIG_ANIMSET( TXT( "AnimSet" ) );
const String CEdAnimEventsEditor::CONFIG_ENTITY( TXT( "Entity" ) );

static int wxCALLBACK ListCompareFunction( wxIntPtr a, wxIntPtr b, wxIntPtr data )
{
	CSkeletalAnimationSetEntry* anim1 = (CSkeletalAnimationSetEntry*)a;
	CSkeletalAnimationSetEntry* anim2 = (CSkeletalAnimationSetEntry*)b;

	String s1 = anim1? anim1->GetName().AsString() : String(TXT(""));
	String s2 = anim2? anim2->GetName().AsString() : String(TXT(""));

	if ( s1 < s2 )
		return -1;
	else if ( s1 > s2 )
		return 1;
	return 0;
}

CEdAnimEventsEditor::CEdAnimEventsEditor( wxWindow* parent, CExtAnimEventsFile* eventsFile )
	: ISmartLayoutWindow( this )
	, m_eventsFile( eventsFile )
	, m_animationSet( NULL )
	, m_updatingGui( false )
	, m_preview( NULL )
	, m_timeline( NULL )
	, m_entityNameLabel( NULL )
	, m_animationSearchField( NULL )
	, m_animationsList( NULL )
	, m_animatedComponent( NULL )
	, m_playedAnimation( NULL )
	, m_speedSlider( NULL )
	, m_playIcon()
	, m_pauseIcon()
	, m_controlToolbar( NULL )
	, m_timelinePanel( NULL )
	, m_tPoseConstraint( -1 )
	, m_refershIconsRequest()
	, m_resettingConfig( false )
{
	ASSERT( m_eventsFile != NULL );

	m_eventsFile->AddToRootSet();

	// Load window
	Bool loadOk = wxXmlResource::Get()->LoadFrame( this, parent, wxT( "eventsFilesFrame" ) );
	ASSERT( loadOk );

	// Load icons
	m_playIcon	= SEdResources::GetInstance().LoadBitmap( TEXT( "IMG_CONTROL_PLAY" ) );
	m_pauseIcon = SEdResources::GetInstance().LoadBitmap( TEXT( "IMG_CONTROL_PAUSE" ) );

	// Set window parameters
	SetTitle( String::Printf( TXT( "Animation Events Editor: %s" ),
		m_eventsFile->GetFile()->GetDepotPath().AsChar() ).AsChar() );
	SetSize( 700, 800 );

	{
		// Create timeline
		m_timelinePanel = XRCCTRL( *this, "timelinePanel", wxPanel );
		ASSERT( m_timelinePanel != NULL );

		m_timeline = new CEdAnimEventsTimeline( m_timelinePanel );
		m_timeline->Connect( usrEVT_TIMELINE_REQUEST_SET_TIME, wxCommandEventHandler( CEdAnimEventsEditor::OnRequestSetTime ), NULL, this );
		m_timeline->Connect( usrEVT_REFRESH_PREVIEW, wxCommandEventHandler( CEdAnimEventsEditor::OnRefreshPreview ), NULL, this );
		m_timeline->Connect( usrEVT_TIMELINE_RESIZED, wxCommandEventHandler( CEdAnimEventsEditor::OnTimelineResized ), NULL, this );
		wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );
		sizer1->Add( m_timeline, 1, wxEXPAND, 0 );
		m_timelinePanel->SetSizer( sizer1 );
		m_timelinePanel->Layout();
	}

	{
		// Create preview
		wxPanel* rp = XRCCTRL( *this, "panelPreview", wxPanel );
		ASSERT( rp != NULL );
		m_preview = new CEdAnimBrowserPreview( rp, this, this );
		wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );
		sizer1->Add( m_preview, 1, wxEXPAND, 0 );
		rp->SetSizer( sizer1 );
		rp->Layout();

		m_preview->SetCameraPosition( Vector( 0, 4, 2 ) );
		m_preview->SetCameraRotation( EulerAngles( 0, -10, 180 ) );
		m_preview->GetViewport()->SetRenderingMode( RM_Shaded );
	}

	// entity name label
	m_entityNameLabel = XRCCTRL( *this, "labelEntity", wxStaticText );
	ASSERT( m_entityNameLabel != NULL );

	// animation search field
	m_animationSearchField = XRCCTRL( *this, "animationSearchField", wxTextCtrl );
	m_clearSearchButton = XRCCTRL( *this, "clearSearchButton", wxButton );
	ASSERT( m_animationSearchField != NULL );
	ASSERT( m_clearSearchButton != NULL );

	// animations list
	m_animationsList = XRCCTRL( *this, "listAnimations", wxListCtrl);
	ASSERT( m_animationsList != NULL );

	// Speed slider
	m_speedSlider = XRCCTRL( *this, "sliderSpeed", wxSlider );
	ASSERT( m_speedSlider != NULL );

	// Control toolbar
	m_controlToolbar = XRCCTRL( *this, "playbackControlToolbar", wxToolBar );
	ASSERT( m_controlToolbar != NULL );
	CreateSkeletonIcons();

	// Auto-play check-box
	m_autoPlayCheckBox = XRCCTRL( *this, "cbAutoPlay", wxCheckBox );
	ASSERT( m_autoPlayCheckBox != NULL );

	{
		// Create undo manager
		m_undoManager = new CEdUndoManager( this );
		m_undoManager->AddToRootSet();
		m_undoManager->SetMenuItems( GetMenuBar()->FindItem( XRCID( "menuItemUndo" ) ), GetMenuBar()->FindItem( XRCID( "menuItemRedo" ) ) );

		m_timeline->SetUndoManager( m_undoManager );
	}

	// Properties page
	{
		// Properties page
		wxPanel* panelProperties = XRCCTRL( *this, "panelProperties", wxPanel );
		wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
		ASSERT( panelProperties != NULL );
		CEdPropertiesPage* propertiesPage = new CEdPropertiesPage( panelProperties, PropertiesPageSettings(), m_undoManager );
		sizer->Add( propertiesPage, 1, wxEXPAND, 0 );
		panelProperties->SetSizer( sizer );
		panelProperties->Layout();
		propertiesPage->SetObject( m_eventsFile );
	}

}

Bool CEdAnimEventsEditor::InitWidget()
{
	LoadOptionsFromConfig();
	
	if( m_animationSet == NULL )
	{
		wxMessageBox( TXT( "You cannot edit events without animation set" ) );
		return false;
	}

	// Fill editor with values from object
	UpdateGui();

	return true;
}

CEdAnimEventsEditor::~CEdAnimEventsEditor()
{
	if ( m_eventsFile )
	{
		m_eventsFile->RemoveFromRootSet();
	}
	if ( m_animationSet )
	{
		m_animationSet->RemoveFromRootSet();
	}

	{
		m_undoManager->RemoveFromRootSet();
		m_undoManager->Discard();
		m_undoManager = NULL;
	}
}

void CEdAnimEventsEditor::UpdateAnimListSelection()
{
	m_updatingGui = true;

	// deselect the previously selected

	for ( long toUnselect = -1; ; ) 
	{
		toUnselect = m_animationsList->GetNextItem( toUnselect, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
		if ( toUnselect == -1 )
		{
			break;
		}

		m_animationsList->SetItemState( toUnselect, 0, wxLIST_STATE_SELECTED );
	}

	// select the active one

	if ( m_playedAnimation != NULL )
	{
		CName name = m_playedAnimation->GetName();
		long item  = m_animationsList->FindItem( -1, name.AsString().AsChar() );

		if ( item != -1 )
		{
			m_animationsList->SetItemState( item , wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED );
			m_animationsList->EnsureVisible( item );
		}
	}

	m_updatingGui = false;
}

void CEdAnimEventsEditor::UpdateGui()
{
	wxString filter = m_animationSearchField->GetValue();
	
	m_updatingGui = true;

	if ( m_clearSearchButton->IsEnabled() == filter.IsEmpty() ) 
	{
		m_clearSearchButton->Enable( !filter.IsEmpty() );
	}

	// fill animations list
	TDynArray< CSkeletalAnimationSetEntry* > anims;
	m_animationSet->GetAnimations( anims );
	m_animationsList->ClearAll();
	Uint32 idx = 0;
	for( Uint32 i = 0; i < anims.Size(); ++i )
	{
		CSkeletalAnimationSetEntry* anim = anims[ i ];

		// Filter out items
		if ( !filter.IsEmpty() && !wxString( anim->GetName().AsString().AsChar() ).Contains( filter ) )
			continue;


		// Insert item to the list
		Int32 id = m_animationsList->InsertItem( idx ++, anim->GetName().AsString().AsChar() );

		// Set pointer to animation as additional data
		m_animationsList->SetItemPtrData( id, ( wxUIntPtr ) anim );
	}
	// get from events
	{
		TDynArray< CExtAnimEvent* > events = m_eventsFile->GetEvents();
		for ( auto iEvent = events.Begin(); iEvent != events.End(); ++ iEvent )
		{
			CExtAnimEvent* evt = *iEvent;

			// Filter out items
			if ( !filter.IsEmpty() && !wxString( evt->GetAnimationName().AsString().AsChar() ).Contains( filter ) )
				continue;

			String noAnimString = (evt->GetAnimationName().AsString() + String(TXT("{no-anim}")));
			if ( m_animationsList->FindItem( -1, evt->GetAnimationName().AsString().AsChar() ) == -1 &&
				 m_animationsList->FindItem( -1, noAnimString.AsChar() ) == -1 )
			{
				// Insert item to the list
				Int32 id = m_animationsList->InsertItem( idx ++, noAnimString.AsChar() );

				// Set pointer to animation as additional data
				m_animationsList->SetItemPtrData( id, ( wxUIntPtr ) 0 );
			}
		}
	}

	// sort the list
	m_animationsList->SortItems( (wxListCtrlCompare)ListCompareFunction, 0 );

	// select the first animation on the _sorted_ list by default
	if ( m_animationsList->GetItemCount() > 0 )
	{
		CSkeletalAnimationSetEntry* entry = reinterpret_cast< CSkeletalAnimationSetEntry* >( m_animationsList->GetItemData( 0 ) );
		SelectAnimation( entry, true );
		UpdateAnimListSelection();
	}

	m_updatingGui = false;
}

void CEdAnimEventsEditor::LoadEntity( const String &entName )
{
	// destroy previous entity
	UnloadEntity();

	// Load entity template
	CEntityTemplate* entityTemplate = LoadResource< CEntityTemplate >( entName );
	if ( entityTemplate == NULL )
	{
		GFeedback->ShowError( String::Printf( TXT( "Failed to load %s entity" ), entName.AsChar() ).AsChar() );
		return;
	}

	// Spawn entity
	EntitySpawnInfo einfo;
	einfo.m_template = entityTemplate;
	einfo.m_name = TXT( "PreviewEntity" );

	m_previewEntity = m_preview->GetPreviewWorld()->GetDynamicLayer()->CreateEntitySync( einfo );			

	if ( m_previewEntity == NULL )
	{
		return;
	}

	// Select animated component
	TDynArray< CAnimatedComponent* > animComponents;
	CollectEntityComponents( m_previewEntity, animComponents );

	if ( animComponents.Size() == 0 )
	{
		GFeedback->ShowError( String::Printf( TXT( "No animated componets in entity %s" ), entName.AsChar() ).AsChar() );
		UnloadEntity();
		return;
	}
	else if ( animComponents.Size() == 1 )
	{
		m_animatedComponent = animComponents[0];
	}
	else
	{
		// Create anim comp name list
		TDynArray< String > animCopmsName;

		// Fill list
		for ( Uint32 i=0; i<animComponents.Size(); i++ )
		{
			animCopmsName.PushBack( animComponents[i]->GetName() );
		}

		const String selectComponent = 
			InputComboBox( NULL, TXT( "Select animated component for animset" ),
			TXT( "Entity has got more then one animated component. Choose one." ),
			animCopmsName[0], animCopmsName);

		// Find select component
		for ( Uint32 i=0; i < animCopmsName.Size(); i++ )
		{
			if ( animCopmsName[i] == selectComponent )
			{
				m_animatedComponent = animComponents[i];
				break;
			}
		}
	}

	ASSERT( m_animatedComponent != NULL );

	// Tick world to allow behaviours process
	CWorldTickInfo info( m_preview->GetPreviewWorld(), 0.1f );
	m_preview->GetPreviewWorld()->Tick( info );

	// Disable behaviour graph
	if ( m_animatedComponent->GetBehaviorStack() )
	{
		m_animatedComponent->GetBehaviorStack()->Deactivate();
	}

	// Add animset to animated component
	m_animatedComponent->AddAnimationSet( m_animationSet );

	// Find animation
	CSkeletalAnimationSetEntry* animation = NULL;
	TDynArray< CSkeletalAnimationSetEntry* > anims;
	m_animationSet->GetAnimations( anims );
	if ( anims.Size() > 0 )
	{
		animation = anims[0];
	}

	// Update preview
	m_preview->OnLoadEntity( m_previewEntity );
	m_preview->SetAnimatedComponent( m_animatedComponent );

	CSoundEmitterComponent* soundEmitterComponent = m_previewEntity->GetSoundEmitterComponent( false );
	UpdateSoundListener( Vector::ZERO_3D_POINT,Vector::ZERO_3D_POINT, Vector::ZERO_3D_POINT, soundEmitterComponent );

	// Update GUI
	m_entityNameLabel->SetLabel( entName.AsChar() );

	SelectAnimation( animation );

	// Update bone editor
	CEffectBoneListSelection::SetEntity( m_previewEntity );

	m_tPoseConstraint = -1;
}

void CEdAnimEventsEditor::UnloadEntity()
{
	if ( m_previewEntity == NULL )
	{
		return;
	}

	// Unload entity from world
	m_preview->GetPreviewWorld()->DelayedActions();
	m_previewEntity->Destroy();
	m_preview->GetPreviewWorld()->DelayedActions(); 
	m_animatedComponent = NULL;
	m_previewEntity = NULL;

	m_preview->OnUnloadEntity();
	m_preview->SetAnimatedComponent( NULL );
	m_preview->SetPlayedAnimation( NULL );

	// Update bone editor
	CEffectBoneListSelection::SetEntity( NULL );

	m_tPoseConstraint = -1;
}

void CEdAnimEventsEditor::OnAnimationChanged( wxListEvent& event )
{
	if( m_updatingGui )
	{
		return;
	}

	// Extract selected animation
	CSkeletalAnimationSetEntry* animationEntry = 
		( CSkeletalAnimationSetEntry* ) event.GetItem().GetData();

	CUndoAnimEventsAnimChange::CreateStep( *m_undoManager, m_timeline, this );

	wxString itemName = event.GetText();
	String itemNameString = String( itemName );
	itemNameString.ReplaceAll( String(TXT("{no-anim}")), String(TXT("")) );
	SelectAnimation( animationEntry, animationEntry? animationEntry->GetName() : CName( itemNameString ) );
}

void CEdAnimEventsEditor::SelectAnimation( CSkeletalAnimationSetEntry* animationEntry, CName const & animName, Bool doNotPlay )
{
	Bool start = m_autoPlayCheckBox->IsChecked() && !doNotPlay;

	if ( !m_animatedComponent )
	{
		return;
	}

	// Play animation
	if ( animationEntry )
	{
		m_playedAnimation =	m_animatedComponent->GetAnimatedSkeleton()->PlayAnimation( animationEntry );
		m_playedAnimation->AddAnimationListener( this );
	}
	else
	{
		m_animatedComponent->GetAnimatedSkeleton()->StopAllAnimation();
		m_playedAnimation = nullptr;
	}

	// Set defaults
	m_animatedComponent->SetUseExtractedMotion( false );
	m_animatedComponent->SetUseExtractedTrajectory( true );

	// Start animation if requested
	if ( m_playedAnimation )
	{
		if( start )
		{
			m_playedAnimation->Unpause();
		}
		else
		{
			m_playedAnimation->Pause();
		}
	}

	// Update preview
	m_preview->SetPlayedAnimation( m_playedAnimation );

	UpdatePlayPauseIcon();

	// Update timeline
	
	// Get events from edited resource
	TDynArray< IEventsContainer* > containers;
	containers.PushBack( m_eventsFile );

	if ( animationEntry )
	{
		// Get events from attached animation set
		containers.PushBack( m_animationSet->FindAnimation( animationEntry->GetName() ) );

		// Get events from other files from animation set
		for( Uint32 i = 0; i < m_animationSet->GetEventsFiles().Size(); ++i )
		{
			containers.PushBackUnique( m_animationSet->GetEventsFiles()[ i ].Get() );
		}

		m_timeline->SetAnimation( animationEntry->GetName(), animationEntry->GetDuration(), containers );
	}
	else
	{
		m_timeline->SetAnimation( animName, -1.0f, containers );
	}
}

void CEdAnimEventsEditor::OnSpeedChanged( wxScrollEvent& event )
{
	Int32 value = m_speedSlider->GetValue();
	Int32 minVal = m_speedSlider->GetMin();
	Int32 maxVal = m_speedSlider->GetMax();

	Float range = (Float)( maxVal - minVal );
	if ( range <= 0.0f )
		range = 1.0f;

	m_preview->SetTimeMultiplier( ( Float ) value / range );
}

void CEdAnimEventsEditor::PlayPauseAnimation()
{
	if( m_playedAnimation == NULL )
	{
		return;
	}

	if ( ! m_playedAnimation->IsPaused() )
	{
		m_playedAnimation->Pause();
	}
	else
	{
		m_playedAnimation->Unpause();
	}

	UpdatePlayPauseIcon();
}

void CEdAnimEventsEditor::ResetAnimation( bool setToEnd )
{
	if ( m_playedAnimation )
	{
		m_playedAnimation->SetTime( setToEnd ? m_playedAnimation->GetDuration() : 0.0f );
	}
}

void CEdAnimEventsEditor::OnPlayPause( wxCommandEvent& event )
{
	PlayPauseAnimation();
}

void CEdAnimEventsEditor::OnReset( wxCommandEvent& event )
{
	ResetAnimation();
}

void CEdAnimEventsEditor::UpdatePlayPauseIcon()
{
	if ( m_playedAnimation && !m_playedAnimation->IsPaused() )
	{
		m_controlToolbar->SetToolNormalBitmap( XRCID( "playButton" ), m_pauseIcon );
	}
	else
	{
		m_controlToolbar->SetToolNormalBitmap( XRCID( "playButton" ), m_playIcon );
	}
}

void CEdAnimEventsEditor::OnMenuClose( wxCommandEvent& event )
{
	OnClose( wxCloseEvent() );
}

void CEdAnimEventsEditor::OnMenuUndo( wxCommandEvent& event )
{
	if ( m_playedAnimation )
	{
		m_playedAnimation->Pause();
		m_playedAnimation->SetTime( 0 );
	}

	m_undoManager->Undo();
}

void CEdAnimEventsEditor::OnMenuRedo( wxCommandEvent& event )
{
	if ( m_playedAnimation )
	{
		m_playedAnimation->Pause();
		m_playedAnimation->SetTime( 0 );
	}

	m_undoManager->Redo();
}

void CEdAnimEventsEditor::OnClose( wxCloseEvent& event )
{
	if ( !m_resettingConfig )
	{
		SaveOptionsToConfig();
	}

	Destroy();
}

void CEdAnimEventsEditor::OnSave( wxCommandEvent& event )
{
	ASSERT( m_eventsFile != NULL );
	m_eventsFile->Save();

	// Save assigned animset with its files if modified
	if( m_animationSet != NULL )
	{
		if( m_animationSet->IsModified() )
		{
			m_animationSet->Save();
		}

		// Save all external files if modified
		TDynArray< THandle< CExtAnimEventsFile > >& files = m_animationSet->GetEventsFiles();
		for( TDynArray< THandle< CExtAnimEventsFile > >::iterator fileIter = files.Begin();
			fileIter != files.End(); ++fileIter )
		{
			CExtAnimEventsFile* file = (*fileIter).Get();
			if( file->IsModified() )
			{
				file->Save();
			}
		}
	}

	SaveOptionsToConfig();
}

void CEdAnimEventsEditor::OnRequestSetTime( wxCommandEvent& event )
{
	TClientDataWrapper< Float>* clientData = dynamic_cast< TClientDataWrapper< Float >* >( event.GetClientObject() );
	ASSERT( clientData != NULL );

	Float time = clientData->GetData();

	if ( m_playedAnimation )
	{
		// Set new time
		m_playedAnimation->SetTime( time );
	}

	// Refresh preview panel
	m_preview->Refresh();
}

void CEdAnimEventsEditor::OnRefreshPreview( wxCommandEvent& event )
{
	// Refresh preview panel
	m_preview->Refresh();
}

String CEdAnimEventsEditor::GetConfigPath( bool perFile ) const
{
	if ( perFile )
	{
		ASSERT( m_eventsFile );
		String absFileName = m_eventsFile->GetFile()->GetAbsolutePath();
		String fileName;
		GDepot->ConvertToLocalPath( absFileName, fileName );
		return CONFIG_PATH + fileName;
	}
	else
	{
		return CONFIG_PATH;
	}
}

void CEdAnimEventsEditor::SaveGUILayout()
{
	SaveLayout( GetConfigPath( false ) );

	// Save splitters positions 

 	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
	CConfigurationScopedPathSetter pathSetter( config, GetConfigPath( false ) );

	wxSplitterWindow* verticalSplitter = XRCCTRL( *this, "verticalSplitter", wxSplitterWindow );
	ASSERT( verticalSplitter != NULL );
	config.Write( TXT( "VerticalSplitter" ), verticalSplitter->GetSashPosition() );

	wxSplitterWindow* horizontalSplitter = XRCCTRL( *this, "horizontalSplitter", wxSplitterWindow );
	ASSERT( horizontalSplitter != NULL );
	config.Write( TXT( "HorizontalSplitter" ), horizontalSplitter->GetSashPosition() );
}

void CEdAnimEventsEditor::LoadGUILayout()
{
	LoadLayout( GetConfigPath( false ) );

	// Load splitters positions 

 	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
	CConfigurationScopedPathSetter pathSetter( config, GetConfigPath( false ) );

	Int32 verticalSplitterPos = config.Read( TXT( "VerticalSplitter" ), 0 );
	wxSplitterWindow* verticalSplitter = XRCCTRL( *this, "verticalSplitter", wxSplitterWindow );
	ASSERT( verticalSplitter != NULL );
	verticalSplitter->SetSashPosition( verticalSplitterPos );

	Int32 hotizontalSplitterPos = config.Read( TXT( "HorizontalSplitter" ), 0 );
	wxSplitterWindow* horizontalSplitter = XRCCTRL( *this, "horizontalSplitter", wxSplitterWindow );
	ASSERT( horizontalSplitter != NULL );
	horizontalSplitter->SetSashPosition( hotizontalSplitterPos );
}

void CEdAnimEventsEditor::SaveOptionsToConfig()
{
	SaveGUILayout();

	if ( m_animationSet == NULL )
		return;

	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
	CConfigurationScopedPathSetter pathSetter( config, GetConfigPath( true ) );

	// Save animset filename
	if ( CDiskFile* file = m_animationSet->GetFile() )
	{
		config.Write( CONFIG_ANIMSET, file->GetDepotPath() );
	}

	// Write entity name
	if( m_previewEntity != NULL )
	{
		config.Write( CONFIG_ENTITY, m_previewEntity->GetEntityTemplate()->GetFile()->GetDepotPath() );
	}
}

void CEdAnimEventsEditor::ResetConfig()
{
	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
	CConfigurationScopedPathSetter pathSetter( config, GetConfigPath( true ) );

	config.DeleteEntry( CONFIG_ANIMSET );
	config.DeleteEntry( CONFIG_ENTITY );
}

void CEdAnimEventsEditor::OnResetConfig( wxCommandEvent& event )
{
	ResetConfig();
	m_resettingConfig = true;
	Close();
}

void CEdAnimEventsEditor::LoadOptionsFromConfig()
{
	String animSetName = String::EMPTY;
	String entityName  = DEFAULT_ENTITY_NAME;

	// Reset the config if the ALT key was pressed during the launch.
	bool forceResetConfig = wxGetKeyState( WXK_ALT );

	if ( forceResetConfig )
	{
		ResetConfig();
	}
	else
	{
		LoadGUILayout();
	
		CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
		CConfigurationScopedPathSetter pathSetter( config, GetConfigPath( true ) );

		// Retrieve animset name
		animSetName = config.Read( CONFIG_ANIMSET, String::EMPTY );

		// Retrieve entity name
		entityName = config.Read( CONFIG_ENTITY, DEFAULT_ENTITY_NAME );
	}

	// If animset is not saved in config, ask user for it
	if ( animSetName.Empty() )
	{
		TDynArray< String > animsets;

		// Find animset extension
		CResource* resource = ClassID< CSkeletalAnimationSet >()->GetDefaultObject< CResource >();
		String ext = String( resource->GetExtension() ).ToLower();

		GFileManager->FindFiles( m_eventsFile->GetFile()->GetDirectory()->GetAbsolutePath(),
			TXT( "*." ) + ext, animsets, true );

		if (animsets.Empty())
		{
			if ( YesNo( TXT("No animsets found, do you want to search whole depot?") ) )
			{
				GFeedback->BeginTask( TXT("Looking for animsets"), false );

				// get all animsets - TODO I guess that there might/should be faster way to do it instead of using windows function
				GFileManager->FindFiles( GDepot->GetRootDataPath(),
					TXT( "*." ) + ext, animsets, true );

				GFeedback->EndTask();
			}
		}

		if (! animsets.Empty())
		{
			for ( TDynArray< String >::iterator iter = animsets.Begin();
					iter != animsets.End(); ++iter )
			{
				GDepot->ConvertToLocalPath( *iter, *iter );
			}

			animSetName = InputComboBox( NULL, TXT( "Select animation set" ),
				TXT( "Select set of animations for editing" ), TXT( "" ), animsets);
		}
	}

	if( animSetName.Empty() )
	{
		return;
	}

	// Load selected animset
	m_animationSet = LoadResource< CSkeletalAnimationSet >( animSetName );
	m_animationSet->AddToRootSet();

	LoadEntity( entityName );
}

void CEdAnimEventsEditor::OnTimelineResized( wxCommandEvent& event )
{
	/* Little hack for refreshing window layout.
	After 4 hours of strumbling, could not make this better */

	static Int32 smallAdder = 1;
	smallAdder *= -1;
	m_timelinePanel->SetSize( m_timelinePanel->GetSize().GetX(),
		m_timelinePanel->GetSize().GetY() + smallAdder );
}

void CEdAnimEventsEditor::OnUseSelectedEntity( wxCommandEvent& event )
{
	String selectedEntity;
	GetActiveResource( selectedEntity );

	LoadEntity( selectedEntity );
}

Bool CEdAnimEventsEditor::IsLoopingEnabled() const
{
	return m_controlToolbar->GetToolState( XRCID( "loopButton" ) );
}

void CEdAnimEventsEditor::OnAnimationFinished( const CPlayedSkeletalAnimation* animation )
{
	ASSERT( m_playedAnimation == animation );

	if ( IsLoopingEnabled() )
	{
		// do nothing
	}
	else if ( m_playedAnimation )
	{
		// pause animation
		m_playedAnimation->Pause();
		m_playedAnimation->SetTime( 0.0f );
		m_refershIconsRequest.Increment();
	}
}

wxToolBar* CEdAnimEventsEditor::GetSkeletonToolbar()
{
	return m_controlToolbar;
}

void CEdAnimEventsEditor::OnToggleCarButton( wxCommandEvent& event )
{
	if( m_playedAnimation != NULL )
	{
		m_animatedComponent->SetUseExtractedMotion( m_controlToolbar->GetToolState( XRCID( "movementButton" ) ) );
		m_animatedComponent->SetUseExtractedTrajectory( m_controlToolbar->GetToolState( XRCID( "movementButton" ) ) );
	}
}

void CEdAnimEventsEditor::OnMassActions( wxCommandEvent& event )
{
	TDynArray< void* > objs;
	TDynArray< CClass* > classes;

	// Figure out which objects to act on
	if ( m_animationsList->GetSelectedItemCount() > 1 )
	{
		int index = -1;
		while ( true )
		{
			TDynArray< IEventsContainer* > containers;
			index = m_animationsList->GetNextItem( index, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
			if ( index == -1 ) break;
			CSkeletalAnimationSetEntry* entry = (CSkeletalAnimationSetEntry*)m_animationsList->GetItemData( index );
			if ( entry == nullptr )
			{
				wxMessageBox( wxT("Invalid selection"), wxT("Error"), wxICON_ERROR|wxOK|wxCENTRE );
				break;
			}
			containers.PushBack( m_eventsFile );
			for( Uint32 i = 0; i < m_animationSet->GetEventsFiles().Size(); ++i )
			{
				containers.PushBackUnique( m_animationSet->GetEventsFiles()[ i ].Get() );
			}
			containers.PushBack( m_animationSet->FindAnimation( entry->GetName() ) );

			for ( Uint32 i=0; i<containers.Size(); ++i )
			{
				TDynArray< CExtAnimEvent* > events;
				containers[i]->GetEventsForAnimation( entry->GetName(), events );
				for ( Uint32 j=0; j<events.Size(); ++j ) 
				{
					objs.PushBack( events[j] );
					classes.PushBack( events[j]->GetClass() );
				}
			}
		}
	}
	else
	{
		int count = m_timeline->GetSelectedItemCount();
		for ( int i=0; i<count; ++i )
		{
			CExtAnimEvent* event = m_timeline->GetSelectedEvent( i );
			if ( !event ) continue;
			objs.PushBack( event );
			classes.PushBack( event->GetClass() );
		}
	}

	// Run the dialog
	if ( objs.Size() == 0 ) return;

	IEdMassActionObject** proxies = (IEdMassActionObject**)RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_Temporary, sizeof(CEdMassActionRTTIProxy*)*objs.Size() );
	for ( size_t i=0; i<objs.Size(); ++i )
	{
		proxies[i] = new CEdMassActionRTTIProxy( objs[i], classes[i] );
	}

	CEdMassActionArrayIterator it( proxies, objs.Size() );

	CEdMassActionContext* ctx = new CEdMassActionContext( &it );
	ctx->AddCommonActionAndConditionTypes();

	CEdMassActionDialog* mad = new CEdMassActionDialog( this, wxT("Mass Actions"), ctx );
	mad->SetDefaults( TXT("animeventeditor") );
	mad->ShowModal();

	mad->Destroy();

	delete ctx;
	for ( size_t i=0; i<objs.Size(); ++i ) 
	{
		delete proxies[i];
	}
	RED_MEMORY_FREE( MemoryPool_Default, MC_Temporary, proxies );
}

void CEdAnimEventsEditor::OnAnimationSearchField( wxCommandEvent& event )
{
	UpdateGui();
	m_animationSearchField->SelectAll();
	m_animationSearchField->SetFocus();
}

void CEdAnimEventsEditor::OnClearSearchButton( wxCommandEvent& event )
{
	m_animationSearchField->SetValue( wxT("") );
	UpdateGui();
	m_animationSearchField->SetFocus();
}

void CEdAnimEventsEditor::OnRenameAnimations( wxCommandEvent& event )
{
	String common;
	{
		// find common starting for all events/animations
		Bool firstJustFound = false;
		TDynArray< CExtAnimEvent* > events = m_eventsFile->GetEvents();
		for ( auto iEvent = events.Begin(); iEvent != events.End(); ++ iEvent )
		{
			CExtAnimEvent* evt = *iEvent;
			if ( firstJustFound )
			{
				String animName = evt->GetAnimationName().AsString();
				while ( ! common.Empty() )
				{
					size_t foundAt;
					if ( animName.FindSubstring( common, foundAt ) &&
						foundAt == 0 )
					{
						break;
					}

					common = common.LeftString( common.GetLength() - 1 );
				}
				if ( common.Empty() )
				{
					break;
				}
			}
			else if ( common.Empty() )
			{
				common = evt->GetAnimationName().AsString();
				firstJustFound = true;
			}
			else
			{
				String animName = evt->GetAnimationName().AsString();
				size_t foundAt;
				if ( ! animName.FindSubstring( common, foundAt ) ||
					foundAt != 0 )
				{
					common = String( TXT("") );
					break;
				}
			}
		}
		if ( common.Empty() || m_animationsList->GetSelectedItemCount() == 1 )
		{
			int index = -1;
			index = m_animationsList->GetNextItem( index, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
			if ( index != -1 )
			{
				wxString itemName = m_animationsList->GetItemText( index );
				common = String( itemName );
				common.ReplaceAll( String(TXT("{no-anim}")), String(TXT("")) );
			}
		}
	}
	CAnimEventsRenameAnimations dlg( nullptr, common, common );
	if ( dlg.ShowModal() )
	{
		// rename all
		TDynArray< CExtAnimEvent* > events = m_eventsFile->GetEvents();
		for ( auto iEvent = events.Begin(); iEvent != events.End(); ++ iEvent )
		{
			CExtAnimEvent* evt = *iEvent;
			String animName = evt->GetAnimationName().AsString();
			if ( dlg.GetToReplace().Empty() )
			{
				animName = dlg.ReplaceWith() + animName;
			}
			else
			{
				animName.ReplaceAll( dlg.GetToReplace(), dlg.ReplaceWith() );
			}
			evt->SetAnimationName( CName( animName ) );
		}
	}
	UpdateGui();
}

CActor*	CEdAnimEventsEditor::GetActorEntity()
{
	if ( m_previewEntity )
	{
		CActor* actor = Cast< CActor >( m_previewEntity );
		if ( actor ) return actor;
	}
	return NULL;
}

void CEdAnimEventsEditor::Tick( Float timeDelta )
{
	if( m_refershIconsRequest.GetValue() > 0 )
	{
		UpdatePlayPauseIcon();
		m_refershIconsRequest.Decrement();
	}

	if( m_previewEntity )
	{
		if( !GGame->GetActiveWorld() )
		{
			GSoundSystem->IssuePreTick( timeDelta );
		}
	}

	// update timeline current time
	if( m_playedAnimation && m_playedAnimation->IsValid() )
	{
		Float animTime = m_playedAnimation->GetTime();

		// Possibly change anim time to obey time limits specified by timeline. Note that we use time limits
		// only when playing animation. When anim is stopped the user is able to set whatever time he wants.
		if( !m_playedAnimation->IsPaused() && m_timeline->TimeLimitsEnabled() )
		{
			Float timeLimitMin, timeLimitaMax;
			m_timeline->GetTimeLimits( timeLimitMin, timeLimitaMax );

			if( animTime < timeLimitMin || timeLimitaMax < animTime )
			{
				animTime = timeLimitMin;
				m_playedAnimation->SetTime( animTime );
			}
		}

		m_timeline->SetCurrentTime( animTime );
	}
	else
	{
		m_timeline->SetCurrentTime( 0.0f );
	}
	m_timeline->Repaint();
}

void CEdAnimEventsEditor::OnItemDisplayToolButton( wxCommandEvent& event )
{
	if ( !m_itemDialog )
	{
		m_itemDialog = new CEdDisplayItemDialog( this, this );
	}

	m_itemDialog->Show();
}

void CEdAnimEventsEditor::OnUndoItemDisplayToolButton( wxCommandEvent& event )
{
	UndoItemDisplay();
}

class AnimatedSkeletonTPose : public IAnimatedSkeletonConstraint
{
public:
	virtual void Update( const CAnimatedComponent* ac, Float dt ) {}

	virtual void PreSample( const CAnimatedComponent* ac, SBehaviorSampleContext* context, SBehaviorGraphOutput& pose ) {}

	virtual void PostSample( const CAnimatedComponent* ac, SBehaviorSampleContext* context, SBehaviorGraphOutput& pose ) 
	{
		const CSkeleton* skeleton = ac->GetSkeleton();
		if ( skeleton )
		{
			const AnimQsTransform* refPose = skeleton->GetReferencePoseLS();
			const Uint32 size = Min< Uint32 >( (Uint32)skeleton->GetBonesNum(), pose.m_numBones );

			for ( Uint32 i=0; i<size; ++i )
			{
#ifdef USE_HAVOK_ANIMATION
				pose.m_outputPose[ i ].setMul( refPose[ i ], pose.m_outputPose[ i ] );
#else
				pose.m_outputPose[ i ].SetMul( refPose[ i ], pose.m_outputPose[ i ] );
#endif
			}
		}
	}
};

void CEdAnimEventsEditor::OnTPoseButton( wxCommandEvent& event )
{
	if ( !m_animatedComponent || !m_animatedComponent->GetAnimatedSkeleton() )
	{
		return;
	}

	if ( event.IsChecked() )
	{
		if ( m_tPoseConstraint == -1 )
		{
			m_tPoseConstraint = m_animatedComponent->GetAnimatedSkeleton()->AddConstraint( new AnimatedSkeletonTPose() );
		}
	}
	else
	{
		if ( m_tPoseConstraint != -1 )
		{
			VERIFY( m_animatedComponent->GetAnimatedSkeleton()->RemoveConstraint( m_tPoseConstraint ) );
			m_tPoseConstraint = -1;
		}
	}
}

void CEdAnimEventsEditor::OnCharHook( wxKeyEvent &event )
{
	switch(event.GetKeyCode())
	{
	case WXK_SPACE:
		// on SPACE play/pause animation
		PlayPauseAnimation();
		break;
	case WXK_HOME:
		// on Ctrl+HOME reset the animation
		if ( event.CmdDown() )
			ResetAnimation();
		else
			event.Skip();
		break;
	case WXK_END:
		// on Ctrl+END go to the animation end
		if ( event.CmdDown() )
			ResetAnimation( true );
		else
			event.Skip();
		break;
	default:
		event.Skip();
	}
}

bool CEdAnimEventsEditor::ShouldBeRestarted() const
{
	return m_resettingConfig;
}