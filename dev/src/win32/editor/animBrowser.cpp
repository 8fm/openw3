/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "animEventsTimeline.h"
#include "animBrowser.h"
#include "shortcutsEditor.h"
#include "assetBrowser.h"
#include "commonDlgs.h"
#include "editorExternalResources.h"
#include "animEventsEditor.h"
#include "animBrowserPreview.h"
#include "effectBoneListSelection.h"
#include "animBrowserCompression.h"
#include "animBrowserBehavior.h"
#include "animPlot.h"
#include "lazyWin32feedback.h"
#include <shellapi.h>
#include "animBrowserTrajectory.h"
#include "animBuilder.h"
#include "ghostConfigDialog.h"
#include "dialogAnimationParameterEditor.h"
#include "undoManager.h"
#include "undoAnimBrowser.h"
#include "defaultCharactersIterator.h"
#include "animBrowserRecompressDialog.h"
#include "animBrowserPasteEventsRelativeToOtherEventDialog.h"
#include "animBrowserFindEventDialog.h"

#include "../../common/engine/behaviorGraphStack.h"
#include "../../common/engine/virtualSkeletalAnimation.h"
#include "../../common/game/actor.h"
#include "../../common/game/inventoryComponent.h"
#include "../../common/game/storySceneAnimationParams.h"
#include "../../common/core/clipboardBase.h"
#include "../../common/core/dependencyLoader.h"
#include "../../common/core/depot.h"
#include "../../common/core/dependencySaver.h"

#include "../../common/engine/animationGameParams.h"
#include "../../common/core/gatheredResource.h"
#include "../../common/engine/animatedSkeleton.h"
#include "../../common/engine/mimicComponent.h"
#include "../../common/engine/viewport.h"
#include "../../common/engine/dynamicLayer.h"
#include "../../common/engine/worldTick.h"
#include "../../common/engine/behaviorGraphUtils.inl"
#include "../../common/engine/skeletonUtils.h"
#include "../../common/engine/meshComponent.h"
#include "../../common/engine/mesh.h"
#include "../../common/engine/poseProvider.h"
#include "../../common/game/movingAgentComponent.h"
#include "../importer/ReFileHelpers.h"
#include "errorsListDlg.h"

#ifdef DEBUG_ANIMS
#pragma optimize("",off)
#endif

// I commented out animation change as it makes not that much sense and may make it impossible to undo/redo while checking out different animations
// #define UNDO_REDO_FOR_ANIMATION_CHANGE

#define for_each(obj, container) \
	for ( auto obj = container.Begin(), end = container.End(); obj != end; ++ obj )

#define for_each_ptr(obj, container) \
	for ( auto iObj = container.Begin(), end = container.End(); iObj != end; ++ iObj ) \
	if ( auto obj = *iObj )

CGatheredResource resSoundEventsReplace( TXT("gameplay\\globals\\sounds\\event_replace.csv"), 0 );

//#define ID_ADD_ANIM_EVENT		4001
//#define ID_REMOVE_ANIM_EVENT	4002
#define ID_RESET_CONFIG         4002
#define ID_SAVE_ANIMATION		4003
#define ID_LOAD_ENTITY			4004
#define ID_CLOSE				4005
#define ID_REMOVE_ANIM			4006
#define ID_COPYALL_EVENTS		4007
#define ID_COPY_EVENTS			4008
#define ID_PASTE_EVENTS			4009
#define ID_PASTETOALL_EVENTS	4010
#define ID_MOVEUP_EVENT			4011
#define ID_MOVEDOWN_EVENT		4012
#define ID_REMOVE_SELECTED_SOUND_EVENT     4014
#define ID_CHANGE_TRACKS_NAME		4015
#define ID_CHANGE_TRACKS_COMMENT	4016
#define ID_SUCK_ANIMATIONS			4017
#define ID_REIMPORT_ANIMATIONS		4018
#define ID_IMPORT_ANIMATIONS		4019
#define ID_REIMPORT_ANIMATION		4020
#define ID_TAKE_SCREENSHOT			4021
								//	4022
#define ID_RENAME_ANIMATION			4023
#define ID_DRAW_ITEM				4025
#define ID_GENERATE_BB				4026
#define ID_GENERATE_BB_ALL			4027
#define ID_GEN_MEM_REP_SET			4028
#define ID_GEN_MEM_REP_ANIM			4029
#define ID_SORT_ANIMSET				4030
#define ID_DO_NOT_CLICK				4031
#define ID_ANIM_PLOT				4032
#define ID_ANIM_MOTION_COMPR		4033
#define ID_REC_COMPRESSED_MEX		4034
#define ID_REC_BBOX					4035
#define ID_REC_COMPRESSED_POSE		4036
#define ID_ALL_REC_COMPRESSED_MEX	4037
#define ID_ALL_REC_BBOX				4038
#define ID_ALL_REC_COMPRESSED_POSE	4039
#define ID_REIMPORT_ADD_1_ANIMATIONS	4040
#define ID_IMPORT_ADD_1_ANIMATIONS		4041
#define ID_REIMPORT_ADD_1_ANIMATION		4042
#define ID_REIMPORT_ADD_2_ANIMATIONS	4043
#define ID_IMPORT_ADD_2_ANIMATIONS		4044
#define ID_REIMPORT_ADD_2_ANIMATION		4045
#define ID_MIMIC_ANIM_PLOT				4046
#define ID_RECREATE_COMPR_POSE			4047
#define ID_RECREATE_COMPR_POSES			4048
#define ID_REIMPORT_ADD_3_ANIMATION		4049
#define ID_IMPORT_ADD_3_ANIMATIONS		4050
#define ID_IMPORT_MS_ANIMATIONS			4051
#define ID_REIMPORT_MS_ANIMATION		4052
#define ID_IMPORT_ADD_4_ANIMATIONS		4053
#define ID_REIMPORT_ADD_4_ANIMATION		4054
#define ID_COPY_ANIM				4055
#define ID_PASTE_ANIM				4056
#define ID_STREAMING_TYPE_1			4057
#define ID_STREAMING_TYPE_2			4058
#define ID_STREAMING_TYPE_3			4059
#define ID_ANIM_MS_PLOT				4060
#define ID_CREATE_VIRTUAL_ANIM		4061
#define ID_EDIT_VIRTUAL_ANIM		4062
#define ID_CALC_BONE_MS				4063
#define ID_CALC_BONE_WS				4064
#define ID_CALC_BONE_WITH_MOTION_MS	4065
#define ID_CALC_BONE_WITH_MOTION_WS	4066
#define ID_SYNC_ANIM_EVENTS_TO		4067
#define ID_SETUP_DIALOG_PARAMS		4068
#define ID_CONVERT_TO_ADD_1			4069
#define ID_CONVERT_TO_ADD_2			4070
#define ID_REIMPORT_NORMAL_ANIMATION	4071
#define ID_IMPORT_NORMAL_ANIMATIONS		4072
#define ID_RECOMPRESS_ANIMATIONS		4073
#define ID_RECOMPRESS_ANIMATION			4074
#define ID_COPY_TO_CLIPBOARD			4075
#define ID_RECOMPRESS_ANIMATIONS_CHOOSE	4076
#define ID_RECOMPRESS_ANIMATION_CHOOSE	4077
#define ID_EXPORT_ANIMATION				4078
#define ID_RELOAD_ENTITY				4079
#define ID_PASTERELATIVETOOTHER_EVENTS	4080
#define ID_FIND_AND_REMOVE_EVENTS		4081
#define ID_FIND_AND_MOVE_EVENTS			4082
#define ID_FIND_AND_EDIT_EVENTS			4083
#define ID_EXPORT_ANIMATION_WITHOUT_TRAJECTORY 4084
#define ID_ADD_DEBUG_POSE_START		4085
#define ID_ADD_DEBUG_POSE_END		4086
#define ID_ADD_DEBUG_POSE_TIME		4087
#define ID_REMOVE_DEBUG_POSE		4088
#define ID_ALL_CHECK_CORRUPTED_ANIMS 4089
#define ID_ALL_CHECK_CORRUPTED_ANIMS_ALL 4090
#define ID_CHECK_PART_STREAMABLE_STATS 4091

#define DEFAULT_ANIM_NAME		"ex_idle"
#define MAX_DIR_SEARCH_DEPTH	5

DEFINE_EVENT_TYPE( wxEVT_ANIM_CONFIRMED )
DEFINE_EVENT_TYPE( wxEVT_ANIM_ABANDONED )

BEGIN_EVENT_TABLE( CEdAnimBrowser, wxFrame )
	EVT_CLOSE( OnClose )

	// Menu
	EVT_MENU( XRCID( "fileSave" ), CEdAnimBrowser::OnSaveAnimation )
	EVT_MENU( XRCID( "restoreAnimation" ), CEdAnimBrowser::OnRestoreAnimation )
	EVT_MENU( XRCID( "menuEditUndo" ), CEdAnimBrowser::OnMenuUndo )
	EVT_MENU( XRCID( "menuEditRedo" ), CEdAnimBrowser::OnMenuRedo )
	EVT_MENU( XRCID( "menuReloadEntity" ), CEdAnimBrowser::OnMenuReloadEntity )

	// Main	
	EVT_NOTEBOOK_PAGE_CHANGED( XRCID("editorsNotebook"), CEdAnimBrowser::OnPageChanged )
	EVT_MENU( ID_RESET_CONFIG, CEdAnimBrowser::OnResetConfig )
	EVT_MENU( ID_CLOSE, CEdAnimBrowser::OnModelessClose )
	EVT_MENU( XRCID( "playButton" ), CEdAnimBrowser::OnPlayPause )
	EVT_MENU( XRCID( "resetButton"), CEdAnimBrowser::OnReset )	
	EVT_MENU( XRCID( "cameraButton" ), CEdAnimBrowser::OnTogglePlayerCameraMode )
	EVT_MENU( XRCID( "movementButton"), CEdAnimBrowser::OnTogglePlayerMotionTrack )	
	EVT_MENU( XRCID( "exTrajButton"), CEdAnimBrowser::OnToggleExtractTrajectory )	
	EVT_MENU( XRCID( "bboxButton"), CEdAnimBrowser::OnToggleBBox )	
	EVT_MENU( XRCID( "floorButton"), CEdAnimBrowser::OnToggleFloor )
	EVT_MENU( XRCID( "itemDisplayToolButton" ), CEdAnimBrowser::OnDisplayItem )
	EVT_MENU( XRCID( "itemUndoToolButton" ), CEdAnimBrowser::OnDisplayItemUndo )
	EVT_MENU( XRCID( "comprPoseButton" ), CEdAnimBrowser::OnCompressedPose )
	EVT_MENU( XRCID( "exCompressed" ), CEdAnimBrowser::OnMotionExTypeChanged )
	EVT_MENU( XRCID( "exUncompressed" ), CEdAnimBrowser::OnMotionExTypeChanged )
	EVT_MENU( XRCID( "mimicCor" ), CEdAnimBrowser::OnMimicCor )
	EVT_MENU( XRCID( "tpose" ), CEdAnimBrowser::OnTPoseCor )
	EVT_MENU( XRCID( "ToggleGhosts" ), CEdAnimBrowser::OnToggleGhosts )
	EVT_MENU( XRCID( "GhostsConfig" ), CEdAnimBrowser::OnConfigureGhosts )
	EVT_MENU( XRCID( "menuZoomExtentsPreview" ), CEdAnimBrowser::OnZoomExtentsPreview )
	EVT_BUTTON( XRCID( "okButton" ), CEdAnimBrowser::OnOk )
	EVT_BUTTON( XRCID( "cancelButton" ), CEdAnimBrowser::OnCancel )
	EVT_COMMAND_SCROLL( XRCID( "speedMulitplier" ), CEdAnimBrowser::OnTimeMultiplierChanged )

	// All animation - ALL_ANIMATIONS
	EVT_TREE_ITEM_ACTIVATED( XRCID( "animList"), CEdAnimBrowser::OnAnimDblClick )
	EVT_TREE_SEL_CHANGED( XRCID( "animList"), CEdAnimBrowser::OnAnimSelected )
	EVT_TEXT_ENTER( XRCID( "animFilter"), CEdAnimBrowser::OnAnimFilter )
	EVT_CHECKBOX( XRCID( "animFilterCheck"), CEdAnimBrowser::OnAnimFilterCheck )

	EVT_CHECKBOX( XRCID( "showAnimDurations"), CEdAnimBrowser::OnShowAnimDurations )
	EVT_CHECKBOX( XRCID( "showAnimMem"), CEdAnimBrowser::OnShowAnimMems )
	EVT_CHECKBOX( XRCID( "showAnimPose"), CEdAnimBrowser::OnShowAnimComprPoses )
	EVT_CHECKBOX( XRCID( "showAnimStreaming"), CEdAnimBrowser::OnShowAnimStreaming )
	EVT_CHECKBOX( XRCID( "showVirtualAnim"), CEdAnimBrowser::OnShowVirtualAnim )

	// Entity animsets page - ANIM_SETS
	EVT_LISTBOX( XRCID( "animSetsList" ), CEdAnimBrowser::OnEntityAnimSetSelected )
	EVT_LISTBOX_DCLICK( XRCID( "animSetsList" ), CEdAnimBrowser::OnEntityAnimSetDblClick )

	// Animset page - ANIMATIONS
	EVT_CHOICE( XRCID( "animSetsCombo"), CEdAnimBrowser::OnAnimSetChanged )
	EVT_LISTBOX( XRCID( "animSetAnimations" ), CEdAnimBrowser::OnAnimSetAnimationSelected )
	EVT_LISTBOX_DCLICK( XRCID( "animSetAnimations" ), CEdAnimBrowser::OnAnimSetAnimationSelectedDblClick )

	// Animation property page - ANIM_PROPERTIES
	EVT_CHOICE( XRCID( "animationCombo"), CEdAnimBrowser::OnAnimChanged )
	EVT_TEXT_ENTER( XRCID( "meStart"), CEdAnimBrowser::OnAnimMotionExChanged )
	EVT_TEXT_ENTER( XRCID( "meEnd"), CEdAnimBrowser::OnAnimMotionExChanged )

	// Keyboard shortcuts
	EVT_CHAR_HOOK (CEdAnimBrowser::OnCharHook )

	// Second control toolbar
	EVT_CHOICE( XRCID( "choicePoseComprName"), CEdAnimBrowser::OnSelectCompressedPose )
	EVT_MENU( XRCID( "poseComprEdit" ), CEdAnimBrowser::OnEditCompressedPose )
	EVT_MENU( XRCID( "poseComprAdd" ), CEdAnimBrowser::OnAddCompressedPose )
	EVT_MENU( XRCID( "poseComprRemove" ), CEdAnimBrowser::OnRemoveCompressedPose )
	EVT_MENU( XRCID( "poseComprPrev" ), CEdAnimBrowser::OnPrevCompressedPose )
	EVT_MENU( XRCID( "poseComprNext" ), CEdAnimBrowser::OnNextCompressedPose )
	EVT_MENU( XRCID( "comprPoseClone" ), CEdAnimBrowser::OnCloneWithCompressedPose )
	EVT_MENU( XRCID( "comprPoseClonFlip" ), CEdAnimBrowser::OnCloneFlipPose )
	EVT_CHOICE( XRCID( "comprPoseCloneColor"), CEdAnimBrowser::OnCloneWithCompressedPoseColor )

	EVT_MENU( ID_ADD_DEBUG_POSE_START, CEdAnimBrowser::OnAddDebugPoseStart )
	EVT_MENU( ID_ADD_DEBUG_POSE_END, CEdAnimBrowser::OnAddDebugPoseEnd )
	EVT_MENU( ID_ADD_DEBUG_POSE_TIME, CEdAnimBrowser::OnAddDebugPoseTime )
	EVT_MENU( ID_REMOVE_DEBUG_POSE, CEdAnimBrowser::OnRemoveDebugPose )
	
END_EVENT_TABLE()

//RED_DEFINE_STATIC_NAME( FileReloadToConfirm )
//RED_DEFINE_STATIC_NAME( FileEdited )
//RED_DEFINE_STATIC_NAME( FileReloadConfirm )
//RED_DEFINE_STATIC_NAME( FileReload )
//RED_DEFINE_STATIC_NAME( EditorPropertyPostChange )
//RED_DEFINE_STATIC_NAME( SimpleTimelineEvent )

const wxColour CEdAnimBrowser::ANIM_COLOR_NORMAL( 0, 200, 0 );
const wxColour CEdAnimBrowser::ANIM_COLOR_MID( 200,200, 0 );
const wxColour CEdAnimBrowser::ANIM_COLOR_LARGE( 200, 0, 0 );

const Float CEdAnimBrowser::ANIM_DUR_NORMAL = 3.f;
const Float CEdAnimBrowser::ANIM_DUR_MID = 5.f;
const Float CEdAnimBrowser::ANIM_DUR_LARGE = 10.f;

const Float CEdAnimBrowser::ANIM_SIZE_NORMAL = 10.f;
const Float CEdAnimBrowser::ANIM_SIZE_MID = 20.f;
const Float CEdAnimBrowser::ANIM_SIZE_LARGE = 40.f;

CEdAnimBrowser::CEdAnimBrowser( wxWindow *parent ) 
	: ISmartLayoutWindow( this )
	, m_previewPanel( NULL )	
	, m_entity( NULL )
	, m_entityTemplate( NULL )
	, m_animatedComponent( NULL )
	, m_mode( ABM_Select )
	, m_timeline( NULL )
	, m_editorsNotebook( NULL )		
	, m_timelinePanel( NULL )
	, m_animsList( NULL )
	, m_entityAnimSetsList( NULL )
	, m_entityAnimSets( NULL )
	, m_animSetAnimationList( NULL )
	, m_animSetAnimations( NULL )
	, m_selectedAnimSet( NULL )
	, m_selectedAnimation( NULL )
	, m_resource( NULL )
	, m_addDynamicAnimset( false )
	, m_useMotionExtraction( false )
	, m_showBBox( false )
	, m_showFloor( false )
	, m_itemDialog( NULL )
	, m_filterMode( BAFM_BeginsWith )
	, m_forceCompressedPose( false )
	, m_motionExType( true )
	, m_extraAnimInfo( I_None )
	, m_mimicCor( false )
	, m_tPoseConstraint( -1 )
	, m_refershIconsRequest()
	, m_ghostCount( 10 )
	, m_ghostType( PreviewGhostContainer::GhostType_Entity )
	, m_resettingConfig( false )
	, m_refreshing( false )
	, m_floor( nullptr )
{
	m_playedAnimation = new PlayedAnimation();

	SetupWidget(parent);
	SetMode( ABM_Select );
		
	LoadGUILayout();

	SetTitle( wxT("AnimBrowser - Select animation") );

	EnableAnimExtraInfo();

	RefreshSecondToolbar();
}

CEdAnimBrowser::CEdAnimBrowser( wxWindow *parent, CResource* res ) 
	: ISmartLayoutWindow( this )
	, m_previewPanel( NULL )	
	, m_entity( NULL )
	, m_entityTemplate( NULL )
	, m_animatedComponent( NULL )
	, m_mode( ABM_Select )
	, m_timeline( NULL )
	, m_editorsNotebook( NULL )		
	, m_timelinePanel( NULL )
	, m_animsList( NULL )
	, m_entityAnimSetsList( NULL )
	, m_entityAnimSets( NULL )
	, m_animSetAnimationList( NULL )
	, m_animSetAnimations( NULL )
	, m_selectedAnimSet( NULL )
	, m_selectedAnimation( NULL )
	, m_resource( res )
	, m_addDynamicAnimset( false )
	, m_showBBox( false )
	, m_showFloor( false )
	, m_filterMode( BAFM_BeginsWith )
	, m_forceCompressedPose( false )
	, m_motionExType( true )
	, m_extraAnimInfo( I_None )
	, m_mimicCor( false )
	, m_tPoseConstraint( -1 )
	, m_ghostCount( 10 )
	, m_ghostType( PreviewGhostContainer::GhostType_Entity )
	, m_floor( nullptr )
{
	m_resource->AddToRootSet();

	m_playedAnimation = new PlayedAnimation();

	SetupWidget(parent);
	LoadOptionsFromConfig();

	ASSERT( m_resource->GetFile() );
	wxString caption = wxString::Format( wxT("AnimBrowser - %s"), m_resource->GetFile()->GetDepotPath().AsChar() );
	SetTitle( wxString::Format( caption ) );

	SEvents::GetInstance().RegisterListener( CNAME( FileReload ), this );
	SEvents::GetInstance().RegisterListener( CNAME( FileReloadConfirm ), this );
	SEvents::GetInstance().RegisterListener( CNAME( EditorPropertyPostChange ), this);
	SEvents::GetInstance().RegisterListener( CNAME( SimpleTimelineEvent ), this);

	EnableAnimExtraInfo();

	RefreshSecondToolbar();

	if ( CSkeletalAnimationSet* set = Cast< CSkeletalAnimationSet >( m_resource ) )
	{
		const TDynArray< CName >& duplicates = set->GetDuplicatedAnimations();
		if ( !duplicates.Empty() )
		{
			String msg = TXT("Duplicated animation found and removed:\n\n");
			for ( CName dup : duplicates )
			{
				msg += dup.AsString() + TXT("\n");
			}

			GFeedback->ShowMsg( TXT("Duplicates found"), msg.AsChar() );
		}
	}
}

CEdAnimBrowser::~CEdAnimBrowser()
{
	if (m_resource) m_resource->RemoveFromRootSet();

	ResetPlayedAnimation();

	if ( m_editorsNotebookDropTarget )
	{
		delete m_editorsNotebookDropTarget;
		m_editorsNotebookDropTarget = NULL;
	}

	{
		m_undoManager->RemoveFromRootSet();
		m_undoManager->Discard();
		m_undoManager = NULL;
	}	
	
	// Unregister hooks
	SEvents::GetInstance().UnregisterListener( this );
}

//////////////////////////////////////////////////////////////////////////
// Use anim browser mode

void CEdAnimBrowser::GetAnimsets(TSkeletalAnimationSetsArray& animSets)
{
	animSets.Clear();

	if (m_mode == ABM_Select)
	{
		if ( !m_animatedComponent )
			return;

		CSkeletalAnimationContainer *animContainer = m_animatedComponent->GetAnimationContainer();
		animSets = animContainer->GetAnimationSets();
	}
	else if (m_mode == ABM_Normal_Animset)
	{
		if ( !m_selectedAnimSet )
			return;

		animSets.PushBack(m_selectedAnimSet);
	}
	else if (m_mode == ABM_Normal_Anim)
	{
		if ( !m_selectedAnimation)
			return;

		CSkeletalAnimationContainer *animContainer = m_animatedComponent->GetAnimationContainer();
		const TSkeletalAnimationSetsArray& contAnimSets = animContainer->GetAnimationSets();

		for( auto it = contAnimSets.Begin(), end = contAnimSets.End(); it != end; ++it )
		{
			TDynArray< CSkeletalAnimationSetEntry* > setAnims;
			( *it )->GetAnimations( setAnims );

			for( Uint32 j=0; j<setAnims.Size(); ++j )
			{
				CSkeletalAnimationSetEntry *currAnim = setAnims[ j ];

				if (m_selectedAnimation==currAnim)
				{
					animSets.PushBack( *it );
					return;
				}
			}
		}
	}
}

void CEdAnimBrowser::GetAnimationsFromAnimset(CSkeletalAnimationSet* animset, TDynArray<CSkeletalAnimationSetEntry*>& anims)
{
	if (m_mode == ABM_Normal_Anim)
	{
		anims.PushBack(m_selectedAnimation);
	}
	else
	{
		animset->GetAnimations(anims);
	}
}

void CEdAnimBrowser::SelectAnimation( const String& name )
{
	CSkeletalAnimationSetEntry *animation = NULL;

	if (m_mode == ABM_Select && m_animatedComponent )
	{
		String animationName = name;
		if( animationName.Empty() )
		{
			// Take last used animation name from config
			CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
			CConfigurationScopedPathSetter pathSetter( config, GetConfigPath( false ) );

			animationName = config.Read( TXT( "LastSelectedAnimation" ), String() );
		}

		animation = m_animatedComponent->GetAnimationContainer()->FindAnimation( CName( animationName ) );
	}
	else if (m_mode == ABM_Normal_Animset && m_selectedAnimSet)
	{
		animation = m_selectedAnimSet->FindAnimation( CName( name ) );
	}

	if ( !animation )
	{
		ResetPlayedAnimation();
		return;
	}

	SelectAnimation( animation );
}

Bool CEdAnimBrowser::FilterAnimName( const CName& animName ) const
{
	// Slow version for testing
	if ( m_filterMode == BAFM_BeginsWith )
	{
		// I know, i know 'AsString'
		return !animName.AsString().BeginsWith( m_filterText );
	}
	else
	{
		return !animName.AsString().ContainsSubstring( m_filterText );
	}
}

void CEdAnimBrowser::SetMode( EAnimBrowserMode mode )
{
	m_mode = mode;

	if ( mode == ABM_Normal_Anim || mode == ABM_Normal_Animset)
	{
		wxPanel* rp = XRCCTRL( *this, "confirmPanel", wxPanel );
		rp->Hide();

		wxNotebook *notebook = XRCCTRL( *this, "editorsNotebook", wxNotebook );
		notebook->Layout();
	}
	else if ( mode == ABM_Select )
	{
		wxPanel* rp = XRCCTRL( *this, "confirmPanel", wxPanel );
		rp->Show();

		wxNotebook *notebook = XRCCTRL( *this, "editorsNotebook", wxNotebook );
		notebook->Layout();
	}
	else
	{
		ASSERT( !"Shouldn get here!" );
	}
}

String CEdAnimBrowser::GetConfigPath( bool perFile ) const
{
	if ( perFile )
	{
		ASSERT( m_resource );
		String absFileName = m_resource->GetFile()->GetAbsolutePath();
		String fileName;
		GDepot->ConvertToLocalPath( absFileName, fileName );
		return TXT("/Frames/AnimBrowser/") + fileName;
	}
	else
	{
		return TXT("/Frames/AnimBrowser/");
	}
}

void CEdAnimBrowser::SaveOptionsToConfig()
{
	SaveGUILayout();

	if (m_entity && m_mode!=ABM_Select)
	{
		CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
		CConfigurationScopedPathSetter pathSetter( config, GetConfigPath( true ) );

		// Entity
		CEntityTemplate* t = Cast<CEntityTemplate>(m_entity->GetTemplate());

		String fileName;
		String absFileName = t->GetFile()->GetAbsolutePath();
		GDepot->ConvertToLocalPath( absFileName, fileName );

		config.Write( TXT("Entity"), fileName );

		// Anim
		if (m_mode == ABM_Normal_Animset)
		{
			if (m_selectedAnimation)
			{
				fileName = m_selectedAnimation->GetAnimation()->GetName().AsString();
				config.Write( TXT("Anim"), fileName );
			}
		}

		for ( Uint32 i=0; i<m_pages.Size(); ++i )
		{
			m_pages[ i ]->Save( config );
		}

 		String colorStr = ToString( m_previewPanel->GetClearColor().ToVector() );
 		config.Write( TXT("Color"), colorStr );
	}
	else if( m_mode == ABM_Select && m_selectedAnimation != NULL )
	{
		CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
		CConfigurationScopedPathSetter pathSetter( config, GetConfigPath( false ) );

		config.Write( TXT( "LastSelectedAnimation" ), m_selectedAnimation->GetName() );
	}
}

void CEdAnimBrowser::SaveGUILayout()
{
	// Save layout
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

void CEdAnimBrowser::LoadGUILayout()
{
	// Load layout
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

void CEdAnimBrowser::LoadOptionsFromConfig()
{	
	// Reset the config if the ALT key was pressed during the launch.
	bool forceResetConfig = wxGetKeyState( WXK_ALT );

	if ( forceResetConfig )
	{
		ResetConfig();
	}
	else
	{
		LoadGUILayout();
	}

	// Load resources
	if ( m_resource->IsA<CSkeletalAnimation>() )			SetMode( ABM_Normal_Anim );
	else if ( m_resource->IsA<CSkeletalAnimationSet>() )	SetMode( ABM_Normal_Animset );

	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
	CConfigurationScopedPathSetter pathSetter( config, GetConfigPath( true ) );

	String entName = config.Read( TXT("Entity"), String::EMPTY );
	String animName = config.Read( TXT("Anim"), String::EMPTY );

	if (m_mode == ABM_Normal_Animset)
	{
		m_selectedAnimSet = Cast<CSkeletalAnimationSet>(m_resource);

		CSkeletalAnimationSetEntry* anim = m_selectedAnimSet->FindAnimation( CName( animName ) );
		if (!anim && m_selectedAnimSet->GetNumAnimations())
		{
			TDynArray< CSkeletalAnimationSetEntry* > anims;
			m_selectedAnimSet->GetAnimations(anims);

			for (Uint32 i=0; i<m_selectedAnimSet->GetNumAnimations(); i++)
			{
				if (anims[i] && anims[i]->GetAnimation()) 
				{
					animName = anims[i]->GetAnimation()->GetName().AsString();
					break;
				}
			}

			ASSERT(animName!=String::EMPTY && TXT("Animset is empty or damaged"));
		}

		CheckAnimsetContent();
	}
	else if (m_mode == ABM_Normal_Anim)
	{
#if 0 // CSkeletalAnimation and CResource are unrelated types !!! Below code would always crash (if it compiled)
		animName = Cast<CSkeletalAnimation>(m_resource)->GetName().AsString();
#endif
	}

	if (entName == String::EMPTY)
	{
		// Find entity, entity directory->animation->animsets files->animations files

		CDirectory* animDir = FindParentDir( m_resource->GetFile()->GetDirectory(), TXT("animation"));
		if (animDir && animDir->GetParent())
		{	
			String entFilePath = animDir->GetParent()->GetAbsolutePath();
			FindEntity(entName, entFilePath);
		}
		else
		{
			entName = DEFAULT_ENTITY_NAME;
		}
	}

	for ( Uint32 i=0; i<m_pages.Size(); ++i )
	{
		m_pages[ i ]->Load( config );
	}

	/*
	String colorStr = config.Read( TXT("Color"), String::EMPTY );
	if ( colorStr.Empty() == false )
	{
		Vector colorVec;
		FromString( colorStr, colorVec );
		Color color( colorVec );
		m_previewPanel->SetClearColor( color );
	}
	*/

	LoadEntity( entName, animName );

	// Set animation
	SelectAnimation( animName );

	CSkeletalAnimationSet* set = FindAnimSetContainingAnimation(m_selectedAnimation);
	if (set) SelectAnimSet( set );

	wxMenuBar* menubar = this->GetMenuBar();
	wxMenu* menu = menubar->GetMenu(0);
	menu->Append( ID_RESET_CONFIG, wxT("Reset") );
	menu->AppendSeparator();
	menu->Append( ID_CLOSE, wxT("Close") );

	// Create preview popup menu
	wxMenu* previewMenu = new wxMenu();
	previewMenu->Append( ID_LOAD_ENTITY, TXT("Use selected entity") );

	{
		wxMenu* subMenu = new wxMenu();

		Uint32 counter = 0;

		for ( DefaultCharactersIterator it; it; ++it )
		{
			counter++;

			subMenu->Append( ID_LOAD_ENTITY + counter, it.GetName().AsChar() );
			previewMenu->Connect( ID_LOAD_ENTITY + counter, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAnimBrowser::OnLoadEntity ), NULL, this ); 
		}

		previewMenu->AppendSubMenu( subMenu, wxT("Entities") );
	}

	previewMenu->Append( ID_TAKE_SCREENSHOT, TXT("Save as bitmap sequence") );
	previewMenu->Append( ID_DRAW_ITEM, TXT("Draw weapon") );
	previewMenu->Append( ID_GENERATE_BB, TXT("Generate bounding box") );
	previewMenu->Connect( ID_LOAD_ENTITY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAnimBrowser::OnLoadEntity ), NULL, this ); 
	previewMenu->Connect( ID_TAKE_SCREENSHOT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAnimBrowser::OnTakeScreenshot ), NULL, this ); 		
	previewMenu->Connect( ID_GENERATE_BB, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAnimBrowser::OnGenerateBoundingBox ), NULL, this ); 

	m_previewPanel->SetContextMenu( previewMenu );

	RefreshPage();

	Show();
}

void CEdAnimBrowser::ResetConfig()
{
	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
	CConfigurationScopedPathSetter pathSetter( config, GetConfigPath( true ) );

	config.DeleteEntry( TXT("Entity") );
	config.DeleteEntry( TXT("Anim") );
	config.DeleteEntry( TXT("Color") );

	for ( Uint32 i=0; i<m_pages.Size(); ++i )
	{
		m_pages[ i ]->Reset( config );
	}
}

wxToolBar* CEdAnimBrowser::GetSkeletonToolbar()
{
	wxToolBar* toolbar = XRCCTRL( *this, "playbackControlToolbar", wxToolBar );
	ASSERT( toolbar );
	return toolbar;
}

void CEdAnimBrowser::HandleSelection( const TDynArray< CHitProxyObject* >& objects )
{
	for ( Uint32 i=0; i<m_pages.Size(); ++i )
	{
		m_pages[i]->HandleSelection( objects );
	}
}

void CEdAnimBrowser::OnSaveAnimset( CSkeletalAnimationSet* set )
{
	set->OnAnimsetPreSaved();
}

void CEdAnimBrowser::Tick( Float timeDelta )
{
	for ( Uint32 i=0; i<m_pages.Size(); ++i )
	{
		m_pages[i]->Tick( timeDelta );
	}

	UpdateCompressedPose();
	if( m_refershIconsRequest.GetValue() > 0 )
	{
		UpdatePlayPauseIcon();
		m_refershIconsRequest.Decrement();
	}

	// update timeline current time
	if( m_playedAnimation && m_playedAnimation->IsValid() )
	{
		Float animTime = m_playedAnimation->GetTime();

		// Possibly change anim time to obey time limits specified by timeline. Note that we use time limits
		// only when playing animation. When anim is stopped the user is able to set whatever time he wants.
		if( !IsPaused() && m_timeline->TimeLimitsEnabled() )
		{
			Float timeLimitMin, timeLimitaMax;
			m_timeline->GetTimeLimits( timeLimitMin, timeLimitaMax );

			if( animTime < timeLimitMin || timeLimitaMax < animTime )
			{
				animTime = timeLimitMin;
				m_playedAnimation->SetTime( animTime );

				// this is after in CEdAnimBrowser::OnTimeDrag() which is a handler for usrEVT_TIMELINE_REQUEST_SET_TIME
				RefreshAnimationForCustomPages();
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

void CEdAnimBrowser::OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame )
{
	for ( Uint32 i=0; i<m_pages.Size(); ++i )
	{
		m_pages[i]->OnViewportGenerateFragments( view, frame );
	}

	if ( m_debugPoses.Size() > 0 )
	{
		for ( Uint32 i=0; i<m_debugPoses.Size(); ++i )
		{
			const DebugPose& pose = m_debugPoses[ i ];

			if ( const CSkeleton* s = pose.m_skeleton.Get() )
			{
				SkeletonRenderingUtils::DrawSkeleton( pose.m_pose, s, pose.m_color, frame );
			}
		}
	}

	if ( m_floor )
	{
		const Float angle = m_floor->GetWorldRotation().Pitch;
		const String txt = String::Printf( TXT("Slope angle: %.3f"), angle );
		frame->AddDebugScreenText( 20, 20, txt, true );
	}
}

void CEdAnimBrowser::SelectItem( CNode* itemNode )
{
	for ( Uint32 i=0; i<m_pages.Size(); ++i )
	{
		m_pages[i]->OnItemSelected( itemNode );
	}
}

void CEdAnimBrowser::CheckAnimsetContent()
{
	String infoMsg;

	if (m_mode == ABM_Select )
	{
		if ( !m_animatedComponent )
			return;

		CSkeletalAnimationContainer *animContainer = m_animatedComponent->GetAnimationContainer();
		TSkeletalAnimationSetsArray animSets = animContainer->GetAnimationSets();

		String msgEmptyAnimset;
		String msgEmptyAnimEntry;

		for ( auto it = animSets.Begin(), end = animSets.End(); it != end; ++it )
		{
			CSkeletalAnimationSet* set = ( *it ).Get();

			// Is animset empty
			if ( !set->GetNumAnimations() )
			{
				// Log
				msgEmptyAnimset += TXT("\n    ") + GetAnimSetName( set );
			
				// Remove
				animContainer->RemoveAnimationSet( set );

				continue;
			}

			if ( set->RemoveEmptyAnimations() )
			{
				msgEmptyAnimEntry += TXT("\n    ") + GetAnimSetName( set );
			}
		}

		// Display info
		String msg;
		if (!msgEmptyAnimset.Empty())
		{
			msg = TXT("Removed empty animsets: ") + msgEmptyAnimset;
		}
		if (!msgEmptyAnimEntry.Empty())
		{
			msg += TXT("\n\nRemoved empty anims entry in animsets: ") + msgEmptyAnimEntry;
		}
		if (!msg.Empty())
		{
			infoMsg += msg;
		}
	}
	else if (m_mode == ABM_Normal_Animset)
	{
		if ( !m_selectedAnimSet )
			return;

		String msgEmptyAnimEntry;

		if (m_selectedAnimSet->RemoveEmptyAnimations())
		{
			msgEmptyAnimEntry += TXT("\n    ") + GetAnimSetName(m_selectedAnimSet);
		}

		// Display info
		if (!msgEmptyAnimEntry.Empty())
		{
			msgEmptyAnimEntry = TXT("\n\nFound empty anims entry in animset: ") + msgEmptyAnimEntry;
			infoMsg += msgEmptyAnimEntry;
		}
	}

	if (!infoMsg.Empty())
	{
		wxMessageBox(infoMsg.AsChar(), wxT("Animsets have damaged content"));
	}
}

void CEdAnimBrowser::AddDynamicAnimset()
{
	if ( m_mode == ABM_Normal_Animset )
	{
		// Add animset to entity?
		if ( !YesNo( TXT("Entity doesn't have animated component with select animset. Do you want to add animset to entity and load it again?") ) )
		{
			WARN_EDITOR( TXT("No animated componets with select animset in entity!") );
			UnloadEntity();
			return;
		}

		// Select animated component
		TDynArray< CAnimatedComponent* > animComponents;
		CollectEntityComponents( m_entity, animComponents );

		if ( animComponents.Size() == 0 )
		{
			WARN_EDITOR( TXT("No animated componets in entity!") );
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

			const String selectComponent = InputComboBox(NULL, TXT("Select animated component for animset"), TXT("Entity has got more then one animated component. Choose one."), animCopmsName[0], animCopmsName);

			// Find select component
			for ( Uint32 i=0; i<animCopmsName.Size(); i++ )
			{
				if ( animCopmsName[i] == selectComponent )
				{
					m_animatedComponent = animComponents[i];
					break;
				}
			}
		}

		ASSERT( m_animatedComponent );

		m_previewPanel->SetAnimatedComponent( m_animatedComponent );

		// Add animset to animated component
		m_selectedAnimSet = SafeCast<CSkeletalAnimationSet>(m_resource);
		m_animatedComponent->AddAnimationSet( m_selectedAnimSet );
		m_addDynamicAnimset = true;

		// Fina animation
		TDynArray< CSkeletalAnimationSetEntry* > anims;
		m_selectedAnimSet->GetAnimations( anims );
		if ( anims.Size() > 0 )
		{
			m_selectedAnimation = anims[0];
		}
	}
}

void CEdAnimBrowser::RemoveDynamicAnimset()
{
	if ( m_addDynamicAnimset )
	{
		ASSERT( m_mode == ABM_Normal_Animset );

		m_animatedComponent->RemoveAnimationSet( m_selectedAnimSet );
		m_addDynamicAnimset = false;
	}
}

//////////////////////////////////////////////////////////////////////////

void CEdAnimBrowser::SetupWidget( wxWindow *parent )
{
	// Load designed frame from resource
	wxXmlResource::Get()->LoadFrame( this, parent, TEXT("AnimBrowser") );

	// Load icons
	m_playIcon	= SEdResources::GetInstance().LoadBitmap( TEXT("IMG_CONTROL_PLAY") );
	m_pauseIcon = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_CONTROL_PAUSE") );

	// timeline
	{
		m_timelinePanel = XRCCTRL( *this, "timelinePanel", wxPanel );
		ASSERT( m_timelinePanel != NULL );

		m_timeline = new CEdAnimEventsTimeline( m_timelinePanel );
		m_timeline->Connect( usrEVT_TIMELINE_REQUEST_SET_TIME, wxCommandEventHandler( CEdAnimBrowser::OnRequestSetTime), NULL, this );
		m_timeline->Connect( usrEVT_TIMELINE_RESIZED, wxCommandEventHandler( CEdAnimBrowser::OnTimelineResized ), NULL, this );
	
		wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
		sizer->Add( m_timeline, 1, wxEXPAND, 0 );
		m_timelinePanel->SetSizer( sizer );
	}

	// preview panel
	{
		wxPanel* rp = XRCCTRL( *this, "previewPanel", wxPanel );
		wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );

		m_previewPanel = new CEdAnimBrowserPreview( rp, this, this );
		m_previewPanel->SetCameraPosition( Vector( 0, 4, 2 ) );
		m_previewPanel->SetCameraRotation( EulerAngles( 0, -10, 180 ) );
		m_previewPanel->GetViewport()->SetRenderingMode( RM_Shaded );

		// attach sound listener to preview's camera position
		Vector forward, up;
		m_previewPanel->GetCameraRotation().ToAngleVectors( &forward, NULL, &up );

		sizer1->Add( m_previewPanel, 1, wxEXPAND, 0 );
		rp->SetSizer( sizer1 );
		rp->Layout();
	}

	m_editorsNotebook = XRCCTRL( *this, "editorsNotebook", wxNotebook );
	m_editorsNotebookDropTarget = new CDropTargetAnimBrowserNotebook(m_editorsNotebook, this);

	m_animsList = XRCCTRL( *this, "animList", wxTreeCtrl );
	m_entityAnimSetsList = XRCCTRL( *this, "animSetsList", wxListBox );
	m_entityAnimSets = XRCCTRL( *this, "animSetsCombo", wxChoice );
	m_animSetAnimationList = XRCCTRL( *this, "animSetAnimations", wxListBox );
	m_animSetAnimations = XRCCTRL( *this, "animationCombo", wxChoice );	

	m_animsList->Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( CEdAnimBrowser::OnAllAnimsRClick ), NULL, this );
	m_entityAnimSetsList->Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( CEdAnimBrowser::OnAnimSetsRClick), NULL, this );
	m_animSetAnimationList->Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( CEdAnimBrowser::OnAnimsRClick), NULL, this );

	// Undo manager
	{
		m_undoManager = new CEdUndoManager( this );
		m_undoManager->AddToRootSet();
		m_undoManager->SetMenuItems( GetMenuBar()->FindItem( XRCID( "menuEditUndo" ) ), GetMenuBar()->FindItem( XRCID( "menuEditRedo" ) ) );

		m_timeline->SetUndoManager( m_undoManager );
	}

	// Create animation properties panel
	{
		wxPanel* rp = XRCCTRL( *this, "AnimPropPanel", wxPanel );
		wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );
		PropertiesPageSettings settings;
		m_animProperties = new CEdPropertiesPage( rp, settings, m_undoManager );
		sizer1->Add( m_animProperties, 1, wxEXPAND, 0 );
		rp->SetSizer( sizer1 );
		rp->Layout();
	}

	// Properties page
	{
		// Properties page
		wxPanel* panelProperties = XRCCTRL( *this, "panelProperties", wxPanel );
		wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
		ASSERT( panelProperties != NULL );
		m_propertiesPage = new CEdPropertiesPage( panelProperties, PropertiesPageSettings(), m_undoManager );
		sizer->Add( m_propertiesPage, 1, wxEXPAND, 0 );
		panelProperties->SetSizer( sizer );
		panelProperties->Layout();
	}

	// DialogParams page
	// temp disabled 
	//{
	//	CAnimBrowserDialogParamsPage* page = new CAnimBrowserDialogParamsPage( m_editorsNotebook, this, m_undoManager );
	//	m_editorsNotebook->AddPage( page, wxT("Dialog params") );
	//	m_pages.PushBack( page );
	//}

	

	// Behavior page
	{
		CAnimBrowserBehaviorPage* page = new CAnimBrowserBehaviorPage( m_editorsNotebook, this, m_undoManager );
		m_editorsNotebook->AddPage( page, wxT("Behavior") );
		m_pages.PushBack( page );
	}

	// Trajectory page
	{
		CAnimBrowserTrajectoryPage* page = new CAnimBrowserTrajectoryPage( m_editorsNotebook, this );
		m_editorsNotebook->AddPage( page, wxT("Trajectory") );
		m_pages.PushBack( page );
	}

	{
		// Havok properties
		wxPanel* rp = XRCCTRL( *this, "havokProp", wxPanel );
		wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );
#ifdef USE_HAVOK_ANIMATION
		m_hkAnimProp = new CEdHavokPropertiesPage( rp );
		sizer1->Add( m_hkAnimProp, 1, wxEXPAND | wxALL, 5 );
#endif
		rp->SetSizer( sizer1 );
		rp->Layout();
	}

	// Compressed pose toolbar
	{
		wxSlider* slider = XRCCTRL( *this, "comprPoseCloneSlider", wxSlider );
		slider->Connect( wxEVT_SCROLL_THUMBTRACK, wxCommandEventHandler( CEdAnimBrowser::OnCloneWithCompressedPoseOffset ), NULL, this );
		slider->Connect( wxEVT_SCROLL_CHANGED, wxCommandEventHandler( CEdAnimBrowser::OnCloneWithCompressedPoseOffset ), NULL, this );
	}

	// enable looping
	wxToolBar* tb = XRCCTRL( *this, "playbackControlToolbar", wxToolBar );
	tb->ToggleTool( XRCID( "loopButton" ), true );
	tb->ToggleTool( XRCID("movementButton"), true );
	tb->ToggleTool( XRCID("exTrajButton"), true );

	// Create skeleton icons
	CreateSkeletonIcons();

	ChangePage(PAGE_ALL_ANIMATIONS);

	// Update icons
	UpdateMotionExTypeIcon();
}

Bool CEdAnimBrowser::CheckUniqueAnimNamesInAnimset(CSkeletalAnimationSet* animset)
{
	TDynArray< CSkeletalAnimationSetEntry* > setAnims;
	GetAnimationsFromAnimset(animset, setAnims);

	for( Uint32 i=0; i<setAnims.Size(); ++i )
	{
		CSkeletalAnimationSetEntry *anim1 = setAnims[ i ];
		for( Uint32 j=i+1; j<setAnims.Size(); ++j )
		{
			CSkeletalAnimationSetEntry *anim2 = setAnims[ j ];
			if ( anim1->GetAnimation() && anim2->GetAnimation() && anim1->GetAnimation()->GetName() == anim2->GetAnimation()->GetName() )
			{
				return false;
			}
		}
	}

	return true;
}

void CEdAnimBrowser::OnPageChanged(wxNotebookEvent& event)
{
	if (event.GetSelection() != -1)
	{
		ChangePage((unsigned int)event.GetSelection());
	}
}

void CEdAnimBrowser::RefreshPage()
{
	RunLaterOnce( [this](){ RefreshPageNow(); } );
}

void CEdAnimBrowser::RefreshPageNow()
{
	m_refreshing = true;
	if ( m_editorsNotebook && m_editorsNotebook->GetSelection() != -1 )
	{
		ChangePage( m_editorsNotebook->GetSelection() );
	}
	m_refreshing = false;
}

void CEdAnimBrowser::ChangePage(const unsigned int page)
{
	if (!m_editorsNotebook) return;

	m_editorsNotebook->ChangeSelection( page );

	ShowCustomPagePanel( page );

	ClearPagesData();

	switch (page)
	{
	case PAGE_ALL_ANIMATIONS :
		ShowPageAllAnimations();
		break;
	case PAGE_ANIM_SETS :
		ShowPageAnimSets();
		break;
	case PAGE_ANIMATIONS :
		ShowPageAnimation();
		break;
	case PAGE_ANIM_PROPERTIES :
		ShowPageAnimProperties();
		break;
	case PAGE_PROPERTIES:
		ShowPageProperties();
		break;
	case PAGE_HAVOK:
		ShowPageHavok();
	case PAGE_TRAJECTORY:
		ShowPageTrajectory();
		break;
	}
}

void CEdAnimBrowser::ClearPagesData()
{
	m_propertiesPage->SetNoObject();
	m_animProperties->SetNoObject();
#ifdef USE_HAVOK_ANIMATION
	m_hkAnimProp->SetNoObject();
#endif
}

//////////////////////////////////////////////////////////////////////////
// All Animation Page

void CEdAnimBrowser::ShowPageAllAnimations()
{
	AllAnimationFillAnimsList();

	if (IsAnimationValid())
	{
		wxTreeItemId itemToSelect = FindAnimationTreeItem( m_selectedAnimation );
		if ( itemToSelect.IsOk() )
		{
			m_animsList->SelectItem( itemToSelect );
		}
	}
}

void CEdAnimBrowser::AllAnimationFillAnimsList()
{
	TSkeletalAnimationSetsArray animSets;
	GetAnimsets(animSets);

	m_animsList->Freeze();
	m_animsList->DeleteAllItems();
	m_animsList->AddRoot( TXT("Animations") );

	Bool useFilter = !m_filterText.Empty();
	
	Bool reselectAnimset = false;
	Bool reselectAnimation = false;

	for( auto it = animSets.Begin(), end = animSets.End(); it != end; ++it )
	{
		CSkeletalAnimationSet* set = ( *it ).Get();

		String setName = GetAnimSetName( set );
		if ( set->IsSelfContained() )
		{
			setName += TXT(" (Self-Contained)");
		}

		Bool isAnimsetEmpty = true;

		wxTreeItemId currSetItem = m_animsList->AppendItem( m_animsList->GetRootItem(), setName.AsChar() );
		m_animsList->SetItemData( currSetItem, new SerializableItemWrapper( set ) );

		TDynArray< CSkeletalAnimationSetEntry* > setAnims;
		GetAnimationsFromAnimset( set, setAnims);
		
		Bool hasUniqueNames = CheckUniqueAnimNamesInAnimset( set );

		for( Uint32 j=0; j<setAnims.Size(); ++j )
		{
			CSkeletalAnimationSetEntry *currAnim = setAnims[ j ];
			if ( currAnim->GetAnimation() )
			{
				if ( useFilter && FilterAnimName( currAnim->GetName() ) )
				{
					if ( currAnim == m_selectedAnimation )
					{
						reselectAnimation = true;
					}

					continue;
				}

				wxString itemName;
				if ( hasUniqueNames || !currAnim->GetAnimSet() || !currAnim->GetAnimSet()->GetFile() || !currAnim->GetAnimSet()->GetFile()->GetDirectory() )
				{
					itemName = wxString( currAnim->GetAnimation()->GetName().AsString().AsChar() );
				}
				else
				{
					String temp = currAnim->GetAnimSet()->GetFile()->GetDirectory()->GetName() + TXT(" - ") + currAnim->GetAnimation()->GetName().AsString();
					itemName = temp.AsChar();
				}

				ApplyAnimListItemExtraData( itemName, currAnim );

				wxTreeItemId item = m_animsList->AppendItem( currSetItem, itemName, -1, -1, new SerializableItemWrapper( currAnim ) );

				ApplyAnimListItemColor( item, currAnim );

				isAnimsetEmpty = false;
			}

			m_animsList->SortChildren( currSetItem );

			if ( useFilter )
			{
				m_animsList->Expand( currSetItem );
			}
		}

		if ( useFilter && isAnimsetEmpty )
		{
			if ( set == m_selectedAnimSet )
			{
				reselectAnimset = true;
			}

			// Remove animset from list
			m_animsList->Delete( currSetItem );
		}
	}

	if ( useFilter )
	{
		if ( reselectAnimset )
		{
			//...
		}
		else if ( reselectAnimation )
		{
			//...
		}
	}

	m_animsList->SortChildren( m_animsList->GetRootItem() );

	if ( useFilter )
	{
		m_animsList->Expand( m_animsList->GetRootItem() );
	}

	m_animsList->Thaw();
}

void CEdAnimBrowser::ApplyAnimListItemExtraData( wxString& data, const CSkeletalAnimationSetEntry *anim )
{
	if ( m_extraAnimInfo == I_Durations )
	{
		data += wxString::Format( wxT("- %1.1fs"), anim->GetDuration() );
	}
	else if ( m_extraAnimInfo == I_Size && anim->GetAnimation() )
	{
		data += wxString::Format( wxT("- %1.1fkB"), (Float)(anim->GetAnimation()->GetSizeOfAnimBuffer()/1024.f) );
	}
	else if ( m_extraAnimInfo == I_ComprPose && anim->GetAnimation() )
	{
		const CName& name = anim->GetAnimation()->GetCompressedPoseName();
		data += wxString::Format( wxT("- %s"), name.AsString().AsChar() );
	}
	else if ( m_extraAnimInfo == I_Streaming && anim->GetAnimation() )
	{
		ESkeletalAnimationStreamingType type = anim->GetAnimation()->GetStreamingType();

		String str;
		if ( type == SAST_Standard ) { str = TXT("Standard"); }
		else if ( type == SAST_Prestreamed ) { str = TXT("Prestreamed"); }
		else if ( type == SAST_Persistent ) { str = TXT("Persistent"); }
		else { str = TXT("ERROR"); }

		data += wxString::Format( wxT("- %s"), str.AsChar() );
	}
	else if ( m_extraAnimInfo == I_Virtual && anim->GetAnimation() && anim->GetAnimation() && anim->GetAnimation()->GetClass()->IsA< CVirtualSkeletalAnimation >() )
	{
		data += wxString::Format( wxT("- Virtual") );
	}
}

void CEdAnimBrowser::ApplyAnimListItemColor( wxTreeItemId& item, const CSkeletalAnimationSetEntry *anim )
{
	if ( m_extraAnimInfo == I_Durations )
	{
		Float weight = 0.f;

		Float animDuration = anim->GetDuration();

		if ( animDuration <= ANIM_DUR_NORMAL )
		{
			weight = 0.f;
		}
		else if ( animDuration > ANIM_DUR_NORMAL && animDuration <= ANIM_DUR_MID )
		{
			weight = animDuration / ANIM_DUR_MID;
		}
		else if ( animDuration > ANIM_DUR_MID && animDuration <= ANIM_DUR_LARGE )
		{
			weight = animDuration / ANIM_DUR_LARGE;
		}

		m_animsList->SetItemTextColour( item, GetAnimColor( weight ) );
	}
	else if ( m_extraAnimInfo == I_Size && anim->GetAnimation() )
	{
		Float weight = 0.f;

		Float animSize = ( Float )anim->GetAnimation()->GetSizeOfAnimBuffer() / 1024.f; //kB

		if ( animSize <= ANIM_SIZE_NORMAL )
		{
			weight = 0.f;
		}
		else if ( animSize > ANIM_SIZE_NORMAL && animSize <= ANIM_SIZE_MID )
		{
			weight = animSize / ANIM_SIZE_MID;
		}
		else if ( animSize > ANIM_SIZE_MID && animSize <= ANIM_SIZE_LARGE )
		{
			weight = animSize / ANIM_SIZE_LARGE;
		}

		m_animsList->SetItemTextColour( item, GetAnimColor( weight ) );
	}
	else if ( m_extraAnimInfo == I_ComprPose && anim->GetAnimation() )
	{
		const CName& name = anim->GetAnimation()->GetCompressedPoseName();
		Float weight = name == CName::NONE ? 1.f : 0.f;

		m_animsList->SetItemTextColour( item, GetAnimColor( weight ) );
	}
	else if ( m_extraAnimInfo == I_Streaming && anim->GetAnimation() )
	{
		ESkeletalAnimationStreamingType type = anim->GetAnimation()->GetStreamingType();

		Float weight = 0.f;

		if ( type == SAST_Standard ) { weight = 0.f; }
		else if ( type == SAST_Prestreamed ) { weight = 2.0f; }
		else if ( type == SAST_Persistent ) { weight = 1.0f; }

		m_animsList->SetItemTextColour( item, GetAnimColor( weight ) );
	}
	else if ( m_extraAnimInfo == I_Virtual && anim->GetAnimation() )
	{
		Float weight = anim->GetAnimation()->GetClass()->IsA< CVirtualSkeletalAnimation >() ? 1.f : 0.f;

		m_animsList->SetItemTextColour( item, GetAnimColor( weight ) );
	}
}

namespace
{
	unsigned char BlendColorChannel( unsigned char a, unsigned char b, Float weight )
	{
		return b > a ? (unsigned char)(a + ( b - a ) * weight) : (unsigned char)(b + ( (Float)a - (Float)b ) * weight );
	}

	wxColour BlendColors( const wxColour& a, const wxColour& b, Float weight )
	{
		return wxColour(	BlendColorChannel( a.Red(), b.Red(), weight ),
							BlendColorChannel( a.Green(), b.Green(), weight ),
							BlendColorChannel( a.Blue(), b.Blue(), weight ) );
	}
}

wxColour CEdAnimBrowser::GetAnimColor( Float weight ) const
{
	if ( weight >= 0.f && weight <= 0.5f )
	{
		return BlendColors( ANIM_COLOR_NORMAL, ANIM_COLOR_MID, 2.f * weight );
	}
	else if ( weight > 0.5f && weight <= 1.f )
	{
		return BlendColors( ANIM_COLOR_MID, ANIM_COLOR_LARGE, 2.f * ( weight - 0.5f ) );
	}

	return ANIM_COLOR_LARGE;
}

void CEdAnimBrowser::OnAnimSelected( wxTreeEvent &event )
{
	wxTreeItemId selectedId = event.GetItem();

	SerializableItemWrapper* animData = (SerializableItemWrapper* )m_animsList->GetItemData( selectedId );
	if ( animData )
	{
		CSkeletalAnimationSetEntry* entry = Cast< CSkeletalAnimationSetEntry >( animData->m_object );
		if ( entry )
		{
			if ( !m_refreshing )
			{
#ifdef UNDO_REDO_FOR_ANIMATION_CHANGE
				CUndoAnimBrowserAnimChange::CreateStep( *m_undoManager, this );
#endif
			}

			SelectAnimSet( FindAnimSetContainingAnimation( entry ) );
			SelectAnimation( entry);
		}
	}
}

void CEdAnimBrowser::OnAnimDblClick( wxTreeEvent &event )
{
	if ( !m_playedAnimation ) return;

	if ( m_playedAnimation->IsEqual( m_selectedAnimation ) )
	{
		if (m_playedAnimation->IsPaused())
		{
			m_playedAnimation->Unpause();
		}
		else if (!m_playedAnimation->IsPaused())
		{
			m_playedAnimation->Pause();
		}

		RefreshAnimationForCustomPages();
	}
	else
	{
		PlayCurrentAnimation();
	}
}

//////////////////////////////////////////////////////////////////////////

void CEdAnimBrowser::ResetPlayedAnimation()
{
	if ( m_animatedComponent && m_animatedComponent->GetAnimatedSkeleton() )
	{
		m_animatedComponent->GetAnimatedSkeleton()->StopAllAnimation();
	}

	m_playedAnimation->Reset();

	m_previewPanel->SetPlayedAnimation( NULL );
}

void CEdAnimBrowser::ReselectCurrentAnimation()
{
	SelectAnimation( m_selectedAnimation );
}

void CEdAnimBrowser::SelectAnimation( CSkeletalAnimationSetEntry *anim )
{
	m_selectedAnimation = anim;

	// timeline
	if( m_selectedAnimSet != NULL && m_selectedAnimation != NULL )
	{
		TDynArray< IEventsContainer* > containers;

		// Put current animation
		containers.PushBack( anim );

		// Get all events file to show as ghost events
		for( Uint32 i = 0; i < m_selectedAnimSet->GetEventsFiles().Size(); ++i )
		{
			containers.PushBack( m_selectedAnimSet->GetEventsFiles()[ i ].Get() );
		}

		m_timeline->SetAnimation( m_selectedAnimation->GetName(), m_selectedAnimation->GetDuration(), containers );
		
	}
	else
	{
		m_timeline->SetAnimation( CName::NONE, 0.0f, TDynArray< IEventsContainer* >() );
	}

	Bool isRunning = true;
	Float currTime = 0.f;

	if ( m_playedAnimation )
	{
		if ( m_playedAnimation->IsPaused() ) isRunning = false;
		currTime = m_playedAnimation->GetTime();

		//m_playedAnimation->DelRef();
	}

	ResetPlayedAnimation();

	PlayCurrentAnimation();

	if ( m_playedAnimation )
	{
		currTime = Clamp( currTime, 0.f, m_playedAnimation->GetDuration() );
		isRunning ? m_playedAnimation->Unpause() : m_playedAnimation->Pause();
		m_playedAnimation->SetTime( currTime );

		if ( const CSkeletalAnimationSetEntry* animEntry = m_playedAnimation->GetAnimationEntry() )
		{
			if ( const CSkeletalAnimation* anim = animEntry->GetAnimation() )
			{
				Float floorAngle = 0.f;
				if ( anim->HasExtractedMotion() )
				{
					Matrix motion( Matrix::IDENTITY );
					anim->GetMovementAtTime( anim->GetDuration(), motion );

					const Float motionZ = motion.GetTranslation().Z;
					if ( MAbs( motionZ ) > 0.05f )
					{
						const Float motionM = motion.GetTranslation().Mag3();
						if ( motionM > 0.f )
						{
							const Float floorAngleRad = MAsin_safe( motionZ / motionM );
							floorAngle = RAD2DEG( floorAngleRad );
						}
					}
				}

				if ( m_floor )
				{
					EulerAngles rot;
					rot.Pitch = floorAngle;
					m_floor->SetRotation( rot );
				}
			}
		}
	}

	SelectAnimationForCustomPages();

	UpdatePlayPauseIcon();

	UpdateMotionExTypeForAnim();

	RefreshCompressedPoseWidgets();

	RecreateCloneWithCompressedPose();
}

Bool CEdAnimBrowser::IsAnimationValid()
{
	if (!m_selectedAnimation) return false;

	TSkeletalAnimationSetsArray animSets;
	GetAnimsets(animSets);

	for( auto it = animSets.Begin(), end = animSets.End(); it != end; ++it )
	{
		CSkeletalAnimationSet* set = ( *it ).Get();

		TDynArray< CSkeletalAnimationSetEntry* > setAnims;
		//animSets[i]->GetAnimations( setAnims );
		GetAnimationsFromAnimset( set, setAnims);

		for( Uint32 j=0; j<setAnims.Size(); ++j )
		{
			if (m_selectedAnimation == setAnims[j])
			{
				return true;
			}
		}
	}

	m_selectedAnimation = NULL;
	return false;
}

void CEdAnimBrowser::SelectAnimSet(CSkeletalAnimationSet *animSet)
{
	m_selectedAnimSet = animSet;
}

Bool CEdAnimBrowser::IsAnimsetValid()
{
	if (!m_selectedAnimSet) return false;

	TSkeletalAnimationSetsArray animSets;
	GetAnimsets(animSets);

	for( auto it = animSets.Begin(), end = animSets.End(); it != end; ++it )
	{
		if ( *it == m_selectedAnimSet)
		{
			return true;
		}
	}

	m_selectedAnimSet = NULL;
	return false;
}

//////////////////////////////////////////////////////////////////////////

void CEdAnimBrowser::ShowPageAnimSets()
{
	EntityAnimsetFillList();

	if (IsAnimsetValid())
	{
		m_entityAnimSetsList->SetStringSelection( GetAnimSetName( m_selectedAnimSet ).AsChar() );
	}
}

void CEdAnimBrowser::EntityAnimsetFillList()
{
	TSkeletalAnimationSetsArray animSets;
	GetAnimsets(animSets);

	m_entityAnimSetsList->Freeze();
	m_entityAnimSetsList->Clear();

	for( auto it = animSets.Begin(), end = animSets.End(); it != end; ++it )
	{
		CSkeletalAnimationSet* set = ( *it ).Get();
		if ( set )
		{
			String setName = GetAnimSetName( set );
			if ( set->IsSelfContained() )
			{
				setName += TXT(" (Self-Contained)");
			}

			m_entityAnimSetsList->Append( setName.AsChar(), set );
		}
	}

	m_entityAnimSetsList->Thaw();
}

void CEdAnimBrowser::OnEntityAnimSetSelected( wxCommandEvent &event )
{	
	Int32 selection = m_entityAnimSetsList->GetSelection();

	if ( selection != wxNOT_FOUND )
	{
		SelectAnimSet( (CSkeletalAnimationSet*)m_entityAnimSetsList->GetClientData( selection ) );		
	}

	RefreshPage();
}

void CEdAnimBrowser::OnEntityAnimSetDblClick( wxCommandEvent &event )
{	
	ChangePage( PAGE_ANIMATIONS );
}

//////////////////////////////////////////////////////////////////////////

void CEdAnimBrowser::ShowPageAnimation()
{
	if (IsAnimsetValid())
	{
		AnimsetFillList();

		m_entityAnimSets->SetStringSelection( GetAnimSetName( m_selectedAnimSet ).AsChar() );

		if (IsAnimationValid())
		{
			m_animSetAnimationList->SetStringSelection( m_selectedAnimation->GetAnimation()->GetName().AsString().AsChar() );
		}
		else
		{
			m_animSetAnimationList->SetSelection( wxNOT_FOUND );
		}
	}
	else
	{
		m_entityAnimSets->Clear();
		m_animSetAnimationList->Clear();
	}
}

void CEdAnimBrowser::AnimsetFillList()
{
	TSkeletalAnimationSetsArray animSets;
	GetAnimsets(animSets);

	m_entityAnimSets->Freeze();
	m_entityAnimSets->Clear();

	for( auto it = animSets.Begin(), end = animSets.End(); it != end; ++it )
	{
		CSkeletalAnimationSet* set = ( *it ).Get();

		m_entityAnimSets->Append( GetAnimSetName( set ).AsChar(), set );
	}

	m_entityAnimSets->Thaw();
	
	m_animSetAnimationList->Freeze();
	m_animSetAnimationList->Clear();

	TDynArray< CSkeletalAnimationSetEntry* > setAnims;
	//m_selectedAnimSet->GetAnimations( setAnims );
	GetAnimationsFromAnimset(m_selectedAnimSet, setAnims);

	for( Uint32 i=0; i<setAnims.Size(); ++i )
	{
		if ( setAnims[i]->GetAnimation() )
		{
			m_animSetAnimationList->Append( setAnims[i]->GetAnimation()->GetName().AsString().AsChar(), setAnims[i] );
		}
	}	

	m_animSetAnimationList->Thaw();
}

void CEdAnimBrowser::OnAnimSetChanged( wxCommandEvent &event )
{
	Int32 selection = m_entityAnimSets->GetSelection();

	if ( selection != wxNOT_FOUND )
	{
		SelectAnimSet( (CSkeletalAnimationSet*)m_entityAnimSets->GetClientData( selection ) );
	}

	RefreshPage();
}


void CEdAnimBrowser::OnAnimSetAnimationSelected( wxCommandEvent &event )
{
	wxArrayInt selections;
	Int32 selectionCount = m_animSetAnimationList->GetSelections( selections );

	if ( selectionCount > 0 )
	{
#ifdef UNDO_REDO_FOR_ANIMATION_CHANGE
		CUndoAnimBrowserAnimChange::CreateStep( *m_undoManager, this );
#endif
		SelectAnimation( (CSkeletalAnimationSetEntry*)m_animSetAnimationList->GetClientData( selections[ 0 ] ) );
	}
}

void CEdAnimBrowser::OnAnimSetAnimationSelectedDblClick( wxCommandEvent &event )
{
	ChangePage(PAGE_ANIM_PROPERTIES);
}

//////////////////////////////////////////////////////////////////////////
// Animation Property

void CEdAnimBrowser::ShowPageAnimProperties()
{
	if ( IsAnimsetValid() )
	{
		AnimProprertiesFillAnimationsList();
	}
	else
	{
		m_animSetAnimations->Clear();
	}

	if (IsAnimationValid())
	{
		// Anim prop
		m_animProperties->SetObject( m_selectedAnimation->GetAnimation() );

		m_animSetAnimations->SetStringSelection( m_selectedAnimation->GetAnimation()->GetName().AsString().AsChar() );

		TDynArray< IEventsContainer* > containers;
		containers.PushBack( m_selectedAnimation );
		m_timeline->SetAnimation( m_selectedAnimation->GetName(), m_selectedAnimation->GetDuration(), containers );

		DispAnimMotionEx();
	}
}

void CEdAnimBrowser::DispAnimMotionEx()
{
	// Get time
	wxTextCtrl* meStart = XRCCTRL( *this, "meStart", wxTextCtrl );
	wxTextCtrl* meEnd = XRCCTRL( *this, "meEnd", wxTextCtrl );

	Float timeStart, timeEnd;

	FromString( meStart->GetValue().wc_str(), timeStart );
	FromString( meEnd->GetValue().wc_str(), timeEnd );

	// Animation motion extraction
	Matrix delta = Matrix::IDENTITY;

	if ( m_selectedAnimation && m_selectedAnimation->GetAnimation() && m_selectedAnimation->GetAnimation()->HasExtractedMotion() )
	{
		if ( timeEnd < NumericLimits<Float>::Epsilon() )
		{
			timeEnd = m_selectedAnimation->GetAnimation()->GetDuration();
		}

		m_selectedAnimation->GetAnimation()->GetMovementBetweenTime( timeStart, timeEnd, 0, delta );
	}

	// Display delta
	wxTextCtrl* meTrans = XRCCTRL( *this, "meTrans", wxTextCtrl );
	wxTextCtrl* meRot = XRCCTRL( *this, "meRot", wxTextCtrl );

	Vector trans = delta.GetTranslation();
	EulerAngles rot = delta.ToEulerAngles();

	String transStr = String::Printf( TXT("%.2f; %.2f; %.2f"), trans.X, trans.Y, trans.Z );
	String rotStr = String::Printf( TXT("%.2f; %.2f; %.2f"), rot.Pitch, rot.Roll, rot.Yaw );

	meTrans->SetValue( transStr.AsChar() );
	meRot->SetValue( rotStr.AsChar() );
};

void CEdAnimBrowser::AnimProprertiesFillAnimationsList()
{
	TDynArray< CSkeletalAnimationSetEntry* > setAnims;
	//m_selectedAnimSet->GetAnimations( setAnims );
	GetAnimationsFromAnimset(m_selectedAnimSet, setAnims);

	m_animSetAnimations->Freeze();
	m_animSetAnimations->Clear();

	for( Uint32 i=0; i<setAnims.Size(); ++i )
	{
		if ( setAnims[i]->GetAnimation() )
		{
			m_animSetAnimations->Append( setAnims[i]->GetAnimation()->GetName().AsString().AsChar(), setAnims[i] );
		}
	}

	m_animSetAnimations->Thaw();
}

void CEdAnimBrowser::OnAnimChanged( wxCommandEvent &event )
{
	Int32 selection = m_animSetAnimations->GetSelection();

	if ( selection != wxNOT_FOUND )
	{
#ifdef UNDO_REDO_FOR_ANIMATION_CHANGE
		CUndoAnimBrowserAnimChange::CreateStep( *m_undoManager, this );
#endif
		SelectAnimation( (CSkeletalAnimationSetEntry*)m_animSetAnimations->GetClientData( selection ) );
	}

	RefreshPage();
}

void CEdAnimBrowser::OnAnimMotionExChanged( wxCommandEvent &event )
{
	RefreshPage();
}

void CEdAnimBrowser::OnSuckAnimaitons( wxCommandEvent &event )
{
	wxMessageBox( TXT("No longer supported"), TXT("Error"), wxOK | wxICON_ERROR );
}

void CEdAnimBrowser::ExportAnimation( CSkeletalAnimationSetEntry *anim, Bool exportTrajectory )
{
	// Find skeleton
	const CSkeleton* skeleton = m_animatedComponent ? m_animatedComponent->GetSkeleton() : nullptr;
	if ( ! skeleton && anim->GetAnimation() )
	{
		// try to get from animation
		skeleton = anim->GetAnimation()->GetSkeleton();
	}
	if (! skeleton)
	{
		// try to get from animset
		TSkeletalAnimationSetsArray animSets;
		GetAnimsets(animSets);
		for_each_ptr( animSet, animSets )
		{
			skeleton = animSet->GetSkeleton();
			if ( skeleton )
			{
				break;
			}
		}
	}

	if ( ! skeleton )
	{
		wxMessageBox( TXT("No skeleton specified!\n\nUse entity for preview, or set skeleton in animset or animation."), TXT("Error"), wxOK | wxICON_ERROR );
		return;
	}

	// Get supported file formats for animations
	TDynArray< CFileFormat > animationFileFormats;
	ISkeletalAnimationExporter::EnumExportFormats( animationFileFormats );

	// Ask for files
	CEdFileDialog fileDialog;
	fileDialog.AddFormats( animationFileFormats );
	fileDialog.SetMultiselection( true );
	fileDialog.SetIniTag( TXT("AnimBrowserAnimExport") );
	if ( fileDialog.DoSave( (HWND) GetHWND(), anim->GetName().AsChar(), true ) )
	{
		String const & file = fileDialog.GetFile();

		CFilePath filePath( file );
		if ( ISkeletalAnimationExporter* animExporter = ISkeletalAnimationExporter::FindExporter( filePath.GetExtension() ) )
		{
			GFeedback->BeginTask( TXT("Exporting animation"), false );
			GFeedback->UpdateTaskInfo( TXT("Exporting '%s'..."), file.AsChar() );
			GFeedback->UpdateTaskProgress( 0, 1 );
			AnimExporterParams options;
			options.m_filePath = file;
			options.m_skeleton = skeleton;
			options.m_exportTrajectory = exportTrajectory;
			animExporter->DoExport( anim->GetAnimation(), options );
			GFeedback->EndTask();
		}
	}		

	RefreshPage();
}

void CEdAnimBrowser::LogMemoryStats()
{
	CSkeletalAnimationSet* logAnimSet = GetSelectedAnimSet();
	if ( ! logAnimSet )
	{
		if ( CSkeletalAnimationSetEntry* selectedAnimation = GetSelectedAnimationEntry( 0 ) )
		{
			logAnimSet = selectedAnimation->GetAnimSet();
		}
	}
	if ( logAnimSet )
	{
		logAnimSet->LogMemoryStats();
	}
}

void CEdAnimBrowser::ImportAnimation( Bool overrideAnimType, ESkeletalAnimationType animType, EAdditiveType type )
{
	// Get the target skeletal animation set
	CSkeletalAnimationSet* animSet = GetSelectedAnimSet();
	if ( animSet )
	{
		// Get supported file formats for animations
		TDynArray< CFileFormat > animationFileFormats;
		ISkeletalAnimationImporter::EnumImportFormats( animationFileFormats );

		// Ask for files
		CEdFileDialog fileDialog;
		fileDialog.AddFormats( animationFileFormats );
		fileDialog.SetMultiselection( true );
		fileDialog.SetIniTag( TXT("AnimBrowserAnimImport") );
		if ( fileDialog.DoOpen( (HWND) GetHWND(), true ) )
		{
			// Animation check
			TDynArray< String > files = fileDialog.GetFiles();
			TDynArray<String> badAnimFiles;

			// in cp delete this array
			TDynArray<String> filesToReimport;

			for ( Int32 i = files.Size()-1; i >= 0; --i )
			{
				IImporter::ImportOptions options;
				options.m_sourceFilePath = files[i];
				if ( !CReFileHelpers::ShouldImportFile( options ) )
				{
					if( options.m_errorCode == IImporter::ImportOptions::EEC_FileToReimport )
					{
						filesToReimport.PushBack( options.m_sourceFilePath );
					}
					else
					{
						badAnimFiles.PushBack( options.m_sourceFilePath );
						files.RemoveAt( i );
					}
				}
			}

			if ( !badAnimFiles.Empty() )
			{
				CEdErrorsListDlg errDialog( m_parent );
				errDialog.SetHeader( String::Printf( TXT("Cannot import '%d' files."), badAnimFiles.Size() ) );
				errDialog.Execute( badAnimFiles );
			}

			// for CP delete this message and do not allow to import old files
			if ( !filesToReimport.Empty() )
			{
				CEdErrorsListDlg errDialog( m_parent );
				errDialog.SetHeader( String::Printf( TXT("Please reexport '%d' files with latest tech."), filesToReimport.Size() ) );
				errDialog.Execute( filesToReimport );
			}

			// Mark modified but allow processing even if not checked out
			animSet->MarkModified();

			// Begin import phase
			GFeedback->BeginTask( TXT("Importing animations"), true );

			// Import animations
			for ( Uint32 i=0; i<files.Size(); i++ )
			{
				// Cancel task
				if ( GFeedback->IsTaskCanceled() )
				{
					break;
				}

				// Update progress
				GFeedback->UpdateTaskInfo( TXT("Importing '%s'..."), files[i].AsChar() );
				GFeedback->UpdateTaskProgress( i, files.Size() );

				// Import animation
				const String& importAnimFile = files[i];

				ImportAnimationOptions options;
				options.m_animationFile = importAnimFile;
				options.m_overrideAnimType = overrideAnimType;
				options.m_type = animType;
				options.m_addType = type;

				if ( type == AT_Animation )
				{
					CSkeletalAnimationSetEntry* selExAnim = NULL;

					if ( !SelectExtraAnimation( selExAnim ) || !selExAnim->GetAnimation() )
					{
						wxMessageBox( wxT( "Select animation for extraction" ), wxT("Error") );
						GFeedback->EndTask();
						return;
					}

					options.m_addAnimationToExtractedFile = selExAnim->GetAnimation()->GetImportFile();
				}

				animSet->ImportAnimation( options );
			}

			// Select last animation from import set
			if ( files.Size() > 0 )
			{
				const String& importAnimFile = files.Back();
				CFilePath filePath( importAnimFile );
				CSkeletalAnimationSetEntry* lastImportedAnimation = animSet->FindAnimation( CName( filePath.GetFileName() ) );
				if ( lastImportedAnimation )
				{
					SelectAnimation( lastImportedAnimation );
				}
			}

			// End import phase
			GFeedback->EndTask();
		}		
	}

	LogMemoryStats();

	RefreshPage();
}

Bool CEdAnimBrowser::SelectExtraAnimation( CSkeletalAnimationSetEntry*& animation ) const
{
	animation = NULL;

	CSkeletalAnimationSet* set = GetSelectedAnimationEntry() ? GetSelectedAnimationEntry()->GetAnimSet() : NULL;
	if ( set )
	{
		static String lastSel = TXT("");

		if ( InputBox( NULL, TXT("Select animation"), TXT("Select animation for extraction"), lastSel ) )
		{
			CName animName( lastSel );

			animation = set->FindAnimation( animName );

			return animation != NULL;
		}
	}

	return false;
}

void CEdAnimBrowser::OnImportAnimations( wxCommandEvent &event )
{
	ImportAnimation();
}

void CEdAnimBrowser::OnImportNormalAnimations( wxCommandEvent &event )
{
	ImportAnimation( true, SAT_Normal );
}

void CEdAnimBrowser::OnImportAddAnimations1( wxCommandEvent &event )
{
	ImportAnimation( true, SAT_Additive, AT_Local );
}

void CEdAnimBrowser::OnImportAddAnimations2( wxCommandEvent &event )
{
	ImportAnimation( true, SAT_Additive, AT_Ref );
}

void CEdAnimBrowser::OnImportAddAnimations3( wxCommandEvent &event )
{
	CSkeletalAnimationSet* set = GetSelectedAnimSet();
	if ( set )
	{
		if ( set->GetSkeleton() )
		{
			ImportAnimation( true, SAT_Additive, AT_TPose );
		}
		else
		{
			wxMessageBox( wxT( "Animset doesn't have skeleton" ), wxT("Error") );
		}
	}
}

void CEdAnimBrowser::OnImportAddAnimations4( wxCommandEvent &event )
{
	ImportAnimation( true, SAT_Additive, AT_Animation );
}

void CEdAnimBrowser::OnImportMSAnimations( wxCommandEvent &event )
{
	CSkeletalAnimationSet* set = GetSelectedAnimSet();
	if ( set )
	{
		if ( set->GetSkeleton() )
		{
			ImportAnimation( true, SAT_MS );
		}
		else
		{
			wxMessageBox( wxT( "Animset doesn't have skeleton" ), wxT("Error") );
		}
	}
}

void CEdAnimBrowser::OnRecompressAnimations( wxCommandEvent &event )
{
	RecompressAnimations();
}

void CEdAnimBrowser::RecompressAnimations( SAnimationBufferBitwiseCompressionPreset* preset )
{
	// Get the target skeletal animation set
	CSkeletalAnimationSet* animSet = GetSelectedAnimSet();
	if ( animSet && animSet->IsSelfContained() )
	{
		// Mark modified but allow processing even if not checked out
		animSet->MarkModified();

		// Begin import phase
		GFeedback->BeginTask( TXT("Recompressing animations"), true );

		if ( preset )
		{
			// set preset in animset
			animSet->SetBitwiseCompressionPreset( *preset );
		}

		// Get animations
		TDynArray< CSkeletalAnimationSetEntry* > animations;
		animSet->GetAnimations( animations );

		CLazyWin32Feedback feedback;

		// Recompress all animations
		for ( Uint32 i=0; i<animations.Size(); i++ )
		{
			// Cancel task
			if ( GFeedback->IsTaskCanceled() )
			{
				break;
			}

			CSkeletalAnimationSetEntry* entry = animations[i];
			if ( entry && entry->GetAnimation() )
			{
				// Update progress
				GFeedback->UpdateTaskInfo( TXT("Recompressing '%s'..."), entry->GetAnimation()->GetName().AsString().AsChar() );
				GFeedback->UpdateTaskProgress( i, animations.Size() );

				if ( entry->GetAnimation() )
				{
					if ( preset )
					{
						entry->GetAnimation()->SetBitwiseCompressionPreset( *preset, false );
					}
					entry->GetAnimation()->Recompress();
				}
			}
		}

		feedback.ShowAll();

		// End import phase
		GFeedback->EndTask();

		// Refresh page
		RefreshPageNow();

		// Select last
		if ( animations.Size() > 0 )
		{
			SelectAnimation( animations.Back() );
		}
	}

	LogMemoryStats();

	RefreshPage();
}

void CEdAnimBrowser::OnRecompressAnimation( wxCommandEvent &event )
{
	RecompressAnimation();
}

void CEdAnimBrowser::RecompressAnimation( SAnimationBufferBitwiseCompressionPreset* preset )
{
	Int32 selIndex = 0;
	Int32 selCount = 0;

	GFeedback->BeginTask( TXT("Recompressing selected animations"), true );
	CLazyWin32Feedback feedback;

	while ( CSkeletalAnimationSetEntry* selectedAnimation = GetSelectedAnimationEntry( selIndex ++, &selCount ) )
	{
		if ( selectedAnimation && selectedAnimation->GetAnimation() )
		{
			// Update progress
			GFeedback->UpdateTaskInfo( TXT("Recompressing '%s'..."), selectedAnimation->GetAnimation()->GetName().AsString().AsChar() );
			GFeedback->UpdateTaskProgress( (Uint32)(selIndex - 1), (Uint32)(selCount) );

			if ( preset )
			{
				selectedAnimation->GetAnimation()->SetBitwiseCompressionPreset( *preset, true );
			}

			selectedAnimation->GetAnimation()->Recompress();
		}
	}

	feedback.ShowAll();
	GFeedback->EndTask();

	LogMemoryStats();

	RefreshPage();
}

void CEdAnimBrowser::OnRecompressAnimationsChoosePreset( wxCommandEvent &event )
{
	SAnimationBufferBitwiseCompressionPreset preset = ABBCP_Custom;
	
	Bool foundOne = false;
	CSkeletalAnimationSet* animSet = GetSelectedAnimSet();
	if ( animSet && animSet->IsSelfContained() )
	{
		TDynArray< CSkeletalAnimationSetEntry* > animations;
		animSet->GetAnimations( animations );

		CLazyWin32Feedback feedback;

		// Recompress all animations
		for ( Uint32 i=0; i<animations.Size(); i++ )
		{
			CSkeletalAnimationSetEntry* selectedAnimation = animations[i];
			if( selectedAnimation->GetAnimation() )
			{
				if ( ! foundOne )
				{
					preset = selectedAnimation->GetAnimation()->GetBitwiseCompressionPreset();
					foundOne = true;
				}
				else if ( preset != selectedAnimation->GetAnimation()->GetBitwiseCompressionPreset() )
				{
					// different ones
					preset = ABBCP_Custom;
					foundOne = false;
					break;
				}
			}
		}
	}

	CAnimBrowserRecompressDialog dlg( nullptr, preset );
	dlg.ShowModal();

	if ( dlg.WantsRecompress() && dlg.GetPreset() != ABBCP_Custom )
	{
		SAnimationBufferBitwiseCompressionPreset preset = dlg.GetPreset();

		RecompressAnimations( &preset );
	}
}

void CEdAnimBrowser::OnRecompressAnimationChoosePreset( wxCommandEvent &event )
{
	SAnimationBufferBitwiseCompressionPreset preset = ABBCP_Custom;
	
	Bool foundOne = false;
	Int32 selIndex = 0;
	while ( CSkeletalAnimationSetEntry* selectedAnimation = GetSelectedAnimationEntry( selIndex ++ ) )
	{
		if( selectedAnimation->GetAnimation() )
		{
			if ( ! foundOne )
			{
				preset = selectedAnimation->GetAnimation()->GetBitwiseCompressionPreset();
				foundOne = true;
			}
			else if ( preset != selectedAnimation->GetAnimation()->GetBitwiseCompressionPreset() )
			{
				// different ones
				preset = ABBCP_Custom;
				foundOne = false;
				break;
			}
		}
	}

	CAnimBrowserRecompressDialog dlg( nullptr, preset );
	dlg.ShowModal();

	if ( dlg.WantsRecompress() && dlg.GetPreset() != ABBCP_Custom )
	{
		SAnimationBufferBitwiseCompressionPreset preset = dlg.GetPreset();

		RecompressAnimation( &preset );
	}
}

void CEdAnimBrowser::OnExportAnimation( wxCommandEvent &event )
{
	if ( CSkeletalAnimationSetEntry* selectedAnimation = GetSelectedAnimationEntry( 0 ) )
	{
		ExportAnimation(selectedAnimation, true);
	}
}

void CEdAnimBrowser::OnExportAnimationWithoutTrajectory( wxCommandEvent &event )
{
	if ( CSkeletalAnimationSetEntry* selectedAnimation = GetSelectedAnimationEntry( 0 ) )
	{
		ExportAnimation(selectedAnimation, false);
	}
}

void CEdAnimBrowser::OnReimportAnimations( wxCommandEvent &event )
{
	// Get the target skeletal animation set
	CSkeletalAnimationSet* animSet = GetSelectedAnimSet();
	if ( animSet && animSet->IsSelfContained() )
	{
		// Mark modified but allow processing even if not checked out
		animSet->MarkModified();

		// Begin import phase
		GFeedback->BeginTask( TXT("Reimporting animations"), true );

		// Get animations
		TDynArray< CSkeletalAnimationSetEntry* > animations;
		animSet->GetAnimations( animations );

		CLazyWin32Feedback feedback;

		// Reimport all animations
		for ( Uint32 i=0; i<animations.Size(); i++ )
		{
			// Cancel task
			if ( GFeedback->IsTaskCanceled() )
			{
				break;
			}

			CSkeletalAnimationSetEntry* entry = animations[i];
			if ( entry && entry->GetAnimation() )
			{
				// Update progress
				GFeedback->UpdateTaskInfo( TXT("Reimport '%s'..."), entry->GetAnimation()->GetName().AsString().AsChar() );
				GFeedback->UpdateTaskProgress( i, animations.Size() );

				// Reimport
				ImportAnimationOptions options;
				options.m_entry = entry;
				options.m_feedback = &feedback;
				options.m_overrideAnimType = false;

				animSet->ReimportAnimation( options );
			}
		}

		feedback.ShowAll();

		// End import phase
		GFeedback->EndTask();

		// Refresh page
		RefreshPageNow();

		// Select last
		if ( animations.Size() > 0 )
		{
			SelectAnimation( animations.Back() );
		}
	}

	LogMemoryStats();

	RefreshPage();
}

void CEdAnimBrowser::OnReimportAnimation( wxCommandEvent &event )
{
	Int32 selIndex = 0;
	while ( CSkeletalAnimationSetEntry* selectedAnimation = GetSelectedAnimationEntry( selIndex ++ ) )
	{
		if ( selectedAnimation )
		{
			CSkeletalAnimationSet* set = selectedAnimation->GetAnimSet();
			if ( set && set->IsSelfContained() )
			{
				ImportAnimationOptions options;
				options.m_entry = selectedAnimation;
				options.m_overrideAnimType = false;

				set->ReimportAnimation( options );
			}
		}
	}

	LogMemoryStats();

	RefreshPage();
}

void CEdAnimBrowser::OnReimportNormalAnimation( wxCommandEvent &event )
{
	Int32 selIndex = 0;
	while ( CSkeletalAnimationSetEntry* selectedAnimation = GetSelectedAnimationEntry( selIndex ++ ) )
	{
		if ( selectedAnimation && selectedAnimation->GetAnimSet() )
		{
			ImportAnimationOptions options;
			options.m_entry = selectedAnimation;

			selectedAnimation->GetAnimSet()->ReimportAnimation( options );
		}
	}

	LogMemoryStats();

	RefreshPage();
}

void CEdAnimBrowser::OnReimportAddAnimation1( wxCommandEvent &event )
{
	Int32 selIndex = 0;
	while ( CSkeletalAnimationSetEntry* selectedAnimation = GetSelectedAnimationEntry( selIndex ++ ) )
	{
		if ( selectedAnimation && selectedAnimation->GetAnimSet() )
		{
			ImportAnimationOptions options;
			options.m_entry = selectedAnimation;
			options.m_type = SAT_Additive;
			options.m_addType = AT_Local;

			selectedAnimation->GetAnimSet()->ReimportAnimation( options );
		}
	}

	LogMemoryStats();

	RefreshPage();
}

void CEdAnimBrowser::OnReimportAddAnimation2( wxCommandEvent &event )
{
	Int32 selIndex = 0;
	while ( CSkeletalAnimationSetEntry* selectedAnimation = GetSelectedAnimationEntry( selIndex ++ ) )
	{
		if ( selectedAnimation && selectedAnimation->GetAnimSet() )
		{
			ImportAnimationOptions options;
			options.m_entry = selectedAnimation;
			options.m_type = SAT_Additive;
			options.m_addType = AT_Ref;

			selectedAnimation->GetAnimSet()->ReimportAnimation( options );
		}
	}

	LogMemoryStats();

	RefreshPage();
}

void CEdAnimBrowser::OnReimportAddAnimation3( wxCommandEvent &event )
{
	Int32 selIndex = 0;
	while ( CSkeletalAnimationSetEntry* selectedAnimation = GetSelectedAnimationEntry( selIndex ++ ) )
	{
		if ( selectedAnimation && selectedAnimation->GetAnimSet() )
		{
			CSkeletalAnimationSet* set = selectedAnimation->GetAnimSet();
			if ( set && set->IsSelfContained() )
			{
				if ( set->GetSkeleton() )
				{
					ImportAnimationOptions options;
					options.m_entry = selectedAnimation;
					options.m_type = SAT_Additive;
					options.m_addType = AT_TPose;

					set->ReimportAnimation( options );
				}
				else
				{
					wxMessageBox( wxT( "Animset doesn't have skeleton" ), wxT("Error") );
				}
			}
		}
	}

	LogMemoryStats();

	RefreshPage();
}

void CEdAnimBrowser::OnReimportAddAnimation4( wxCommandEvent &event )
{
	Int32 selIndex = 0;
	while ( CSkeletalAnimationSetEntry* selectedAnimation = GetSelectedAnimationEntry( selIndex ++ ) )
	{
		if ( selectedAnimation && selectedAnimation->GetAnimSet() )
		{
			CSkeletalAnimationSet* set = selectedAnimation->GetAnimSet();
			if ( set && set->IsSelfContained() )
			{
				ImportAnimationOptions options;
				options.m_entry = selectedAnimation;
				options.m_type = SAT_Additive;
				options.m_addType = AT_Animation;

				CSkeletalAnimationSetEntry* selExAnim = NULL;
				if ( SelectExtraAnimation( selExAnim ) && selExAnim->GetAnimation() )
				{
					options.m_addAnimationToExtractedFile = selExAnim->GetAnimation()->GetImportFile();

					set->ReimportAnimation( options );

					LogMemoryStats();

					RefreshPage();
				}
				else
				{
					wxMessageBox( wxT( "Select animation for extraction" ), wxT("Error") );
				}
			}
		}
	}
}

void CEdAnimBrowser::OnReimportMSAnimation( wxCommandEvent &event )
{
	Int32 selIndex = 0;
	while ( CSkeletalAnimationSetEntry* selectedAnimation = GetSelectedAnimationEntry( selIndex ++ ) )
	{
		if ( selectedAnimation )
		{
			CSkeletalAnimationSet* set = selectedAnimation->GetAnimSet();
			if ( set && set->IsSelfContained() )
			{
				if ( set->GetSkeleton() )
				{
					ImportAnimationOptions options;
					options.m_entry = selectedAnimation;
					options.m_type = SAT_MS;
					options.m_addType = AT_TPose;

					set->ReimportAnimation( options );
				}
				else
				{
					wxMessageBox( wxT( "Animset doesn't have skeleton" ), wxT("Error") );
				}
			}
		}
	}

	LogMemoryStats();

	RefreshPage();
}

void CEdAnimBrowser::OnDuplicateAsAdd1( wxCommandEvent &event )
{
	/*CSkeletalAnimationSetEntry* selectedAnimation = GetSelectedAnimationEntry();
	
	if ( selectedAnimation && selectedAnimation->GetAnimation() )
	{
		CSkeletalAnimationSet* set = selectedAnimation->GetAnimSet();
		if ( set )
		{
			CSkeletalAnimation* animation = selectedAnimation->GetAnimation();
			ASSERT( animation->GetParent() == NULL );

			CSkeletalAnimation* newAnimation = CreateObject< CSkeletalAnimation >( NULL, 0 );
			
			if ( animation->CloneAnimationBufferTo( newAnimation ) )
			{
				String newName = animation->GetName().AsString() + TXT("_add");
				newAnimation->SetName( CName( newName ) );

				if ( animation->ConvertToAdditive( AT_Local ) )
				{
					VERIFY( set->AddAnimation( animation ) );
				}
			}
			else
			{
				ASSERT( 0 );
			}
		}
	}

	LogMemoryStats();

	RefreshPage();*/
}

void CEdAnimBrowser::OnDuplicateAsAdd2( wxCommandEvent &event )
{
	/*CSkeletalAnimationSetEntry* selectedAnimation = GetSelectedAnimationEntry();

	if ( selectedAnimation && selectedAnimation->GetAnimation() )
	{
		CSkeletalAnimationSet* set = selectedAnimation->GetAnimSet();
		if ( set )
		{
			CSkeletalAnimation* animation = selectedAnimation->GetAnimation();
			ASSERT( animation->GetParent() == NULL );

			CSkeletalAnimation* newAnimation = CreateObject< CSkeletalAnimation >( NULL );

			if ( animation->CloneAnimationBufferTo( newAnimation ) )
			{
				String newName = animation->GetName().AsString() + TXT("_add");
				newAnimation->SetName( CName( newName ) );

				if ( animation->ConvertToAdditive( AT_Ref ) )
				{
					VERIFY( set->AddAnimation( animation ) );
				}
			}
			else
			{
				ASSERT( 0 );
			}
		}
	}

	LogMemoryStats();

	RefreshPage();*/
}

void CEdAnimBrowser::OnRenameAnimation( wxCommandEvent &event )
{
	Int32 selIndex = 0;
	while ( CSkeletalAnimationSetEntry* selectedAnimation = GetSelectedAnimationEntry( selIndex ++ ) )
	{
		if ( selectedAnimation && selectedAnimation->GetAnimation() )
		{
			CSkeletalAnimationSet* set = selectedAnimation->GetAnimSet();
			if ( set && set->IsSelfContained() )
			{
				String newName = InputBox( this, TXT("Rename animation"), TXT("Write new name:"), selectedAnimation->GetAnimation()->GetName().AsString() );
				set->RenameAnimation( selectedAnimation, CName( newName ) );
			}
		}
	}

	RefreshPage();
}

void CEdAnimBrowser::OnSortAnimset( wxCommandEvent& event )
{
	CSkeletalAnimationSet* animSet = GetSelectedAnimSet();
	if ( animSet && animSet->IsSelfContained() )
	{
		// Mark modified but allow processing even if not checked out
		animSet->MarkModified();

		RefreshPage();
	}
}

void CEdAnimBrowser::EnableAnimExtraInfo()
{
	if ( m_mode == ABM_Normal_Animset )
	{
		wxCheckBox* ch1 = XRCCTRL( *this, "showAnimDurations", wxCheckBox );
		wxCheckBox* ch2 = XRCCTRL( *this, "showAnimMem", wxCheckBox );
		wxCheckBox* ch3 = XRCCTRL( *this, "showAnimPose", wxCheckBox );
		wxCheckBox* ch4 = XRCCTRL( *this, "showAnimStreaming", wxCheckBox );
		wxCheckBox* ch5 = XRCCTRL( *this, "showVirtualAnim", wxCheckBox );

		ch1->Enable( true );
		ch2->Enable( true );
		ch3->Enable( true );
		ch4->Enable( true );
		ch5->Enable( true );
	}
}

void CEdAnimBrowser::SetAnimExtraInfo( EExtraAnimInfo info )
{
	wxCheckBox* ch1 = XRCCTRL( *this, "showAnimDurations", wxCheckBox );
	wxCheckBox* ch2 = XRCCTRL( *this, "showAnimMem", wxCheckBox );
	wxCheckBox* ch3 = XRCCTRL( *this, "showAnimPose", wxCheckBox );
	wxCheckBox* ch4 = XRCCTRL( *this, "showAnimStreaming", wxCheckBox );
	wxCheckBox* ch5 = XRCCTRL( *this, "showVirtualAnim", wxCheckBox );

	switch ( info )
	{
	case I_None:
		{
			ch1->SetValue( false );
			ch2->SetValue( false );
			ch3->SetValue( false );
			ch4->SetValue( false );
			ch5->SetValue( false );
			break;
		}
	case I_Durations:
		{
			ch1->SetValue( true );
			ch2->SetValue( false );
			ch3->SetValue( false );
			ch4->SetValue( false );
			ch5->SetValue( false );
			break;
		}
	case I_Size:
		{
			ch1->SetValue( false );
			ch2->SetValue( true );
			ch3->SetValue( false );
			ch4->SetValue( false );
			ch5->SetValue( false );
			break;
		}
	case I_ComprPose:
		{
			ch1->SetValue( false );
			ch2->SetValue( false );
			ch3->SetValue( true );
			ch4->SetValue( false );
			ch5->SetValue( false );
			break;
		}
	case I_Streaming:
		{
			ch1->SetValue( false );
			ch2->SetValue( false );
			ch3->SetValue( false );
			ch4->SetValue( true );
			ch5->SetValue( false );
			break;
		}
	case I_Virtual:
		{
			ch1->SetValue( false );
			ch2->SetValue( false );
			ch3->SetValue( false );
			ch4->SetValue( false );
			ch5->SetValue( true );
			break;
		}
	}

	m_extraAnimInfo = info;
}

void CEdAnimBrowser::OnShowAnimDurations( wxCommandEvent& event )
{
	if ( event.IsChecked() )
	{
		SetAnimExtraInfo( I_Durations );
	}
	else
	{
		SetAnimExtraInfo( I_None );
	}

	RefreshPage();
}

void CEdAnimBrowser::OnShowAnimMems( wxCommandEvent& event )
{
	if ( event.IsChecked() )
	{
		SetAnimExtraInfo( I_Size );
	}
	else
	{
		SetAnimExtraInfo( I_None );
	}
	
	RefreshPage();
}

void CEdAnimBrowser::OnShowAnimComprPoses( wxCommandEvent& event )
{
	if ( event.IsChecked() )
	{
		SetAnimExtraInfo( I_ComprPose );
	}
	else
	{
		SetAnimExtraInfo( I_None );
	}

	RefreshPage();
}

void CEdAnimBrowser::OnShowAnimStreaming( wxCommandEvent& event )
{
	if ( event.IsChecked() )
	{
		SetAnimExtraInfo( I_Streaming );
	}
	else
	{
		SetAnimExtraInfo( I_None );
	}

	RefreshPage();
}

void CEdAnimBrowser::OnShowVirtualAnim( wxCommandEvent& event )
{
	if ( event.IsChecked() )
	{
		SetAnimExtraInfo( I_Virtual );
	}
	else
	{
		SetAnimExtraInfo( I_None );
	}

	RefreshPage();
}

void CEdAnimBrowser::OnAnimFilter( wxCommandEvent& event )
{
	m_filterMode = XRCCTRL( *this, "animFilterCheck", wxCheckBox )->IsChecked() ? BAFM_Contain : BAFM_BeginsWith;
	wxString str = event.GetString();
	m_filterText = str.wc_str();

	RefreshPage();
}

void CEdAnimBrowser::OnAnimFilterCheck( wxCommandEvent& event )
{
	m_filterMode = event.IsChecked() ? BAFM_Contain : BAFM_BeginsWith;
}

void CEdAnimBrowser::OnGenMemRepSet( wxCommandEvent& event )
{
	CSkeletalAnimationSet* animSet = GetSelectedAnimSet();
	if ( animSet && animSet->IsSelfContained() )
	{
		TDynArray< CSkeletalAnimationSetEntry* > anims;
		animSet->GetAnimations( anims );

		Uint32 sizeAll = 0;
		Uint32 num = 0;

		for ( Uint32 i=0; i<anims.Size(); ++i )
		{
			CSkeletalAnimation* anim = anims[i]->GetAnimation();
			if ( anim )
			{
				Uint32 size = anim->GetSizeOfAnimBuffer();
				if ( size > 0 )
				{
					sizeAll += size;
					num++;
				}
			}
		}

		if ( num == 0 )
		{
			return;
		}

		String msg = String::Printf( TXT("Size of animations : %1.2fKB"), sizeAll / 1024.0f );

		wxMessageBox( msg.AsChar(), animSet->GetFriendlyName().AsChar() );
	}
}

void CEdAnimBrowser::OnGenMemRepAnim( wxCommandEvent& event )
{
	Int32 selIndex = 0;
	while ( CSkeletalAnimationSetEntry* selectedAnimation = GetSelectedAnimationEntry( selIndex ++ ) )
	{
		if ( selectedAnimation && selectedAnimation->GetAnimation() )
		{
			CSkeletalAnimation* anim = selectedAnimation->GetAnimation();
			Uint32 size = anim->GetSizeOfAnimBuffer();

			String msg = String::Printf( TXT("Size of animation buffer : %1.2fKB"), size / 1024.0f );

			wxMessageBox( msg.AsChar(), anim->GetName().AsString().AsChar() );
		}
	}
}

void CEdAnimBrowser::OnDoNotClick( wxCommandEvent& event )
{
	CSkeletalAnimationSet* animSet = GetSelectedAnimSet();
	if ( animSet && animSet->IsSelfContained() )
	{
		TDynArray< CSkeletalAnimationSetEntry* > anims;
		animSet->GetAnimations( anims );

		for ( Uint32 i=0; i<anims.Size(); ++i )
		{
			CSkeletalAnimation* animation = anims[i]->GetAnimation();
			if ( animation )
			{
				//animation->RemoveShit();
			}
		}
	}
}

void CEdAnimBrowser::OnAnimPlot( wxCommandEvent& event )
{
	if ( m_selectedAnimation )
	{
		const CSkeleton* skeleton = m_animatedComponent ? m_animatedComponent->GetSkeleton() : NULL;

		if ( !skeleton && m_selectedAnimation )
		{
			CSkeletalAnimationSet* set = m_selectedAnimation->GetAnimSet();
			skeleton = set->GetSkeleton();
		}

		CEdAnimBonePlot* plot = new CEdAnimBonePlot( NULL, m_selectedAnimation, skeleton );
		plot->Show();
		plot->SetFocus();
	}
}

void CEdAnimBrowser::OnAnimMSPlot( wxCommandEvent& event )
{
	if ( m_selectedAnimation )
	{
		const CSkeleton* skeleton = m_animatedComponent ? m_animatedComponent->GetSkeleton() : NULL;

		if ( !skeleton && m_selectedAnimation )
		{
			CSkeletalAnimationSet* set = m_selectedAnimation->GetAnimSet();
			skeleton = set->GetSkeleton();
		}

		CEdAnimBoneMSPlot* plot = new CEdAnimBoneMSPlot( NULL, m_selectedAnimation, skeleton );
		plot->Show();
		plot->SetFocus();
	}
}

void CEdAnimBrowser::OnMimicAnimPlot( wxCommandEvent& event )
{
	if ( m_selectedAnimation )
	{
		const CSkeleton* skeleton = m_animatedComponent ? m_animatedComponent->GetMimicSkeleton() : NULL;
		if ( skeleton )
		{
			CEdAnimBonePlot* plot = new CEdAnimBonePlot( NULL, m_selectedAnimation, skeleton );
			plot->Show();
			plot->SetFocus();
		}
	}
}

//////////////////////////////////////////////////////////////////////////
#define ANIM_BROWSER_CALC_BONE( function )\
if ( m_selectedAnimation && m_animatedComponent )\
{\
static String boneName;\
String timeStr = TXT("0.0");\
if ( InputDoubleBox( this, TXT("Calc bone"), TXT("Write bone name and time"), boneName, timeStr ) )\
{\
Float time = 0.f;\
if ( !FromString( timeStr, time ) )\
{\
return;\
}\
Int32 boneIndex = m_animatedComponent->FindBoneByName( boneName.AsChar() );\
if ( boneIndex != -1 )\
{\
TDynArray< Matrix > pose;\
if ( SkeletonUtils::function( m_selectedAnimation->GetAnimation(), time, m_animatedComponent, pose ) )\
{\
if ( pose.SizeInt() > boneIndex )\
{\
LOG_EDITOR( TXT("Bone MS - '%s' - time - '%1.3f'"), boneName.AsChar(), time );\
String pos = ToString( pose[ boneIndex ].GetTranslation() );\
String rot = ToString( pose[ boneIndex ].ToEulerAngles() );\
LOG_EDITOR( TXT("   >Position:") );\
LOG_EDITOR( TXT("%s"), pos.AsChar() );\
LOG_EDITOR( TXT("   >Rotation:") );\
LOG_EDITOR( TXT("%s"), rot.AsChar() );\
wxString str = wxString::Format( wxT("Bone: '%s'\nTime: '%1.3f'\nPositions: %s\nRotation: %s\n\nUse log for copy/paste."), boneName.AsChar(), time, pos.AsChar(), rot.AsChar() );\
wxMessageBox( str, wxT("Result") );\
}\
}\
}\
else\
{\
wxMessageBox( wxString::Format( wxT("Bone '%s' not found "), boneName.AsChar() ), wxT("Error") );\
}\
}\
}
//////////////////////////////////////////////////////////////////////////

void CEdAnimBrowser::OnCalcBoneMS( wxCommandEvent& event )
{
	ANIM_BROWSER_CALC_BONE( SamplePoseMS );
}

void CEdAnimBrowser::OnCalcBoneWithMotionMS( wxCommandEvent& event )
{
	ANIM_BROWSER_CALC_BONE( SamplePoseWithMotionMS );
}

void CEdAnimBrowser::OnCalcBoneWS( wxCommandEvent& event )
{
	ANIM_BROWSER_CALC_BONE( SamplePoseWS );
}

void CEdAnimBrowser::OnCalcBoneWithMotionWS( wxCommandEvent& event )
{
	ANIM_BROWSER_CALC_BONE( SamplePoseWithMotionWS );
}

void CEdAnimBrowser::OnAnimCompressMotion( wxCommandEvent& event )
{
	if ( m_selectedAnimation )
	{
		CSkeleton* skeleton = m_animatedComponent ? m_animatedComponent->GetSkeleton() : NULL;

		CEdMotionExtractionPreview* plot = new CEdMotionExtractionPreview( NULL, m_selectedAnimation, skeleton );
		plot->Show();
		plot->SetFocus();
	}
}

void CEdAnimBrowser::OnRecreateCompressedMotionExtraction( wxCommandEvent& event )
{
	if ( m_selectedAnimation && m_selectedAnimation->GetAnimation() )
	{
		// Mark modified but allow processing even if not checked out
		m_resource->MarkModified();

		m_selectedAnimation->GetAnimation()->CreateCompressedMotionExtraction();
	}
}

void CEdAnimBrowser::OnRecreateBBox( wxCommandEvent& event )
{
	if ( m_selectedAnimation && m_selectedAnimation->GetAnimation() )
	{
		CSkeletalAnimationSet* set = m_selectedAnimation->GetAnimSet();
		if ( set && set->GetSkeleton() )
		{
			m_selectedAnimation->GetAnimation()->CreateBBox( set->GetSkeleton() );
		}
	}
}

void CEdAnimBrowser::OnRecreateAllCompressedMotionExtraction( wxCommandEvent& event )
{
	CSkeletalAnimationSet* set = SafeCast< CSkeletalAnimationSet >( m_resource );
	
	const TDynArray< CSkeletalAnimationSetEntry* >& anims = set->GetAnimations();

	const Uint32 size = anims.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		CSkeletalAnimationSetEntry* entry = anims[ i ];
		if ( entry )
		{
			CSkeletalAnimation* a = entry->GetAnimation();
			if ( a )
			{
				// Mark modified but allow processing even if not checked out
				m_resource->MarkModified();

				a->CreateCompressedMotionExtraction();
			}
		}
	}
}

void CEdAnimBrowser::OnCheckPartStreambleStats( wxCommandEvent& event )
{
	const CSkeletalAnimationSet* set = SafeCast< CSkeletalAnimationSet >( m_resource );
	const TDynArray< CSkeletalAnimationSetEntry* >& entries = set->GetAnimations();

	Uint32 outNonStreamable_all(0);
	Uint32 outPartiallyStreamable_all(0);

	for ( Uint32 i=0; i<entries.Size(); ++i )
	{
		CSkeletalAnimationSetEntry* e = entries[ i ];
		if ( e )
		{
			if ( CSkeletalAnimation* anim = e->GetAnimation() )
			{
				if ( const IAnimationBuffer* buff = anim->GetAnimBuffer() )
				{
					if ( const CAnimationBufferBitwiseCompressed* bitBuff = Cast< const CAnimationBufferBitwiseCompressed >( buff ) )
					{
						Uint32 outNonStreamable(0);
						Uint32 outPartiallyStreamable(0);

						bitBuff->GetPartiallyStreamableMemoryStats( outNonStreamable, outPartiallyStreamable );

						outNonStreamable_all += outNonStreamable;
						outPartiallyStreamable_all += outPartiallyStreamable;
					}
				}
			}
		}
	}

	const Float outNonStreamable_all_MB = ( outNonStreamable_all / 1024.f ) / 1024.f;
	const Float outPartiallyStreamable_all_MB = ( outPartiallyStreamable_all / 1024.f ) / 1024.f;

	GFeedback->ShowMsg( TXT("Partially Streamable Anims Memory Stats"), TXT("NonStreamable: %1.2f[MB] - PartiallyStreamable: %1.2f [MB]"), outNonStreamable_all_MB, outPartiallyStreamable_all_MB );
}

void CEdAnimBrowser::OnCheckCorruptedAnimations( wxCommandEvent& event )
{
	CSkeletalAnimationSet* set = SafeCast< CSkeletalAnimationSet >( m_resource );

	TDynArray< CName > outCorruptedAnimations;

	if ( set->CheckAllAnimations( outCorruptedAnimations, GFeedback ) == false )
	{
		TDynArray< String > temp;
		for ( CName n : outCorruptedAnimations )
		{
			temp.PushBack( n.AsString() );
		}
		TDynArray< Uint32 > temp2;
		GFeedback->ShowList( TXT("List of corrupted animations:"), temp, temp2 );
	}
	else
	{
		GFeedback->ShowMsg( TXT("Checking animations..."), TXT("Everything is ok :)") );
	}
}

void CEdAnimBrowser::OnCheckAllCorruptedAnimations( wxCommandEvent& event )
{
	GFeedback->BeginTask( TXT("Checking animations..."), false);
	GFeedback->UpdateTaskProgress( 0, 100 );

	TDynArray< String > paths;
	GDepot->FindResourcesByExtension( TXT("w2anims"), paths );

	TDynArray< String > corruptedAnimsets;

	for ( Uint32 i=0; i<paths.Size(); i++ )
	{
		CResource* fileResource( nullptr );
		CDiskFile* diskFile = GDepot->FindFile( paths[i] );

		const Bool alreadyLoaded = diskFile->IsLoaded(); 
		if ( alreadyLoaded )
		{
			fileResource = diskFile->GetResource();
		}
		else
		{
			fileResource = LoadResource<CResource>( paths[i] );
		}

		GFeedback->UpdateTaskProgress( i, paths.Size() );
		GFeedback->UpdateTaskInfo( diskFile->GetDepotPath().AsChar() );

		if ( fileResource )
		{
			CSkeletalAnimationSet* set = Cast< CSkeletalAnimationSet >( fileResource );
			if ( set )
			{
				TDynArray< CName > outCorruptedAnimations;
				if ( set->CheckAllAnimations( outCorruptedAnimations, GFeedback ) == false )
				{
					WARN_EDITOR( TXT("Animset '%ls' contains currupted animations:"), set->GetDepotPath().AsChar() );
					for ( CName n : outCorruptedAnimations )
					{
						WARN_EDITOR( TXT("   >%ls"), n.AsChar() );
					}

					corruptedAnimsets.PushBack( String::Printf( TXT("[%d] %ls"), outCorruptedAnimations.Size(), set->GetDepotPath().AsChar() ) );
				}

				if ( !alreadyLoaded )
				{
					diskFile->Unload();
				}
			}
			else
			{
				ASSERT( 0 );
			}
		}
	}

	GFeedback->EndTask();

	if ( corruptedAnimsets.Size() > 0 )
	{
		TDynArray< Uint32 > temp2;
		GFeedback->ShowList( TXT("List of corrupted animsets:"), corruptedAnimsets, temp2 );
	}
}

void CEdAnimBrowser::OnRecreateAllBBox( wxCommandEvent& event )
{
	CSkeletalAnimationSet* set = SafeCast< CSkeletalAnimationSet >( m_resource );
	if ( !set->GetSkeleton() )
	{
		return;
	}

	const TDynArray< CSkeletalAnimationSetEntry* >& anims = set->GetAnimations();

	const Uint32 size = anims.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		CSkeletalAnimationSetEntry* entry = anims[ i ];
		if ( entry )
		{
			CSkeletalAnimation* a = entry->GetAnimation();
			if ( a )
			{
				// Mark modified but allow processing even if not checked out
				m_resource->MarkModified();

				a->CreateBBox( set->GetSkeleton() );
			}
		}
	}
}

void CEdAnimBrowser::OnRecreateComprPose( wxCommandEvent& event )
{
	String poseName = GetSelectedCompressedPose();
	if ( poseName == CName::NONE.AsString() )
	{
		return;
	}

	CSkeletalAnimationSetEntry* selEntry = GetSelectedAnimationEntry();
	if ( selEntry && selEntry->GetAnimation() )
	{
		CSkeletalAnimationSet* set = selEntry->GetAnimSet();

		CLazyWin32Feedback feedback;

		set->RecreateCompressedPose( CName( poseName ), &feedback );

		feedback.ShowAll();
	}
}

void CEdAnimBrowser::OnRecreateComprPoses( wxCommandEvent& event )
{
	CSkeletalAnimationSet* set = GetSelectedAnimSet();
	if ( set )
	{
		CLazyWin32Feedback feedback;

		set->RecreateAllCompressedPoses( &feedback );

		feedback.ShowAll();
	}
}

class CEdAnimationContainer : public CObject
{
	DECLARE_ENGINE_CLASS( CEdAnimationContainer, CObject, 0 );

private:
	TDynArray<CSkeletalAnimationSetEntry*> m_animations;

public:
	CEdAnimationContainer() {}

	void AddAnim(CSkeletalAnimationSetEntry* animation)
	{
		m_animations.PushBack(animation);
	}

	~CEdAnimationContainer()
	{
		for_each_ptr(animation, m_animations)
		{
			delete animation;
		}
	}

	void UnlinkAnimations()
	{
		m_animations.Clear();
	}

	CSkeletalAnimationSetEntry* ExtractAnimation(Uint32 idx)
	{
		if ( idx < m_animations.Size() )
		{
			CSkeletalAnimationSetEntry* ret = m_animations[idx];
			m_animations[idx] = nullptr;
			return ret;
		}
		else
		{
			return nullptr;
		}
	}
};

BEGIN_CLASS_RTTI( CEdAnimationContainer );
	PARENT_CLASS( CObject );
	PROPERTY( m_animations );
END_CLASS_RTTI();

IMPLEMENT_RTTI_CLASS( CEdAnimationContainer );

void CEdAnimBrowser::OnAnimCopy( wxCommandEvent& event )
{
	CEdAnimationContainer* container = new CEdAnimationContainer();

	{
		Int32 selIndex = 0;
		Int32 selCount = 0;

		while ( CSkeletalAnimationSetEntry* selectedAnimation = GetSelectedAnimationEntry( selIndex ++, &selCount ) )
		{
			container->AddAnim( selectedAnimation );
		}
	}

	// Serialize to memory package
	TDynArray< Uint8 > buffer;
	CMemoryFileWriter writer( buffer );
	CDependencySaver saver( writer, NULL );

	DependencySavingContext context( container );
	if ( !saver.SaveObjects( context ) )
	{
		WARN_EDITOR( TXT("Unable to copy selected objects to clipboard") );
		return;
	}

	// Open clipboard
	if ( wxTheClipboard->Open() )
	{
		wxTheClipboard->SetData( new CClipboardData( TXT("Anims"), buffer, true ) );
		wxTheClipboard->Close();
	}
		
	// Discard container object, unlink the animation first so it's not de
	container->UnlinkAnimations();
	container->Discard();
}

void CEdAnimBrowser::OnAnimPaste( wxCommandEvent& event )
{
	CSkeletalAnimationSet* set = GetSelectedAnimSet();
	if ( set && wxTheClipboard->Open())
	{
		CClipboardData data( TXT("Anims") );
		if ( wxTheClipboard->IsSupported( data.GetDataFormat() ) )
		{
			if ( wxTheClipboard->GetData( data ) )
			{
				// Mark modified but allow processing even if not checked out
				set->MarkModified();

				const TDynArray< Uint8 >& objData = data.GetData();

				CMemoryFileReader reader( objData, 0 );
				CDependencyLoader loader( reader, NULL );
				DependencyLoadingContext loadingContext;
				loadingContext.m_parent = set;

				if ( !loader.LoadObjects( loadingContext ) )
				{
					wxTheClipboard->Close();
					return;
				}

				// extract anims
				TDynArray< CSkeletalAnimation* > anims;
				TDynArray< CSkeletalAnimationSetEntry* > animEntries;
				for ( Uint32 i=0; i<loadingContext.m_loadedRootObjects.Size(); i++ )
				{
					CEdAnimationContainer* data = Cast< CEdAnimationContainer >( loadingContext.m_loadedRootObjects[i] );
					if ( nullptr != data )
					{
						Uint32 idx = 0;
						while ( CSkeletalAnimationSetEntry* animEntry = data->ExtractAnimation( idx ) )
						{
							if ( animEntry && animEntry->GetAnimation() )
							{
								animEntries.PushBack( animEntry );
								anims.PushBack( animEntry->GetAnimation() );
							}
							++ idx;
						}
					}
				}

				loader.PostLoad();

				for ( Uint32 i=0; i<anims.Size(); ++i )
				{
					CSkeletalAnimation* animation = anims[ i ];
					CSkeletalAnimationSetEntry* sourceEntry = animEntries[ i ];

					if ( animation->GetCompressedPoseName() != CName::NONE )
					{
						if ( !set->FindCompressedPoseByName( animation->GetCompressedPoseName() ) )
						{
							animation->SetCompressedPoseName( CName::NONE );
						}
					}

					set->AddAnimation( animation );

					// copy events
					if ( CSkeletalAnimationSetEntry* animEntry = set->FindAnimation( animation ) )
					{
						TDynArray< CExtAnimEvent* > events;
						sourceEntry->GetEventsForAnimation( sourceEntry->GetName(), events );
						for_each_ptr( animEvent, events )
						{
							TDynArray< Uint8 > buffer;
							CMemoryFileWriter writer( buffer );
							CEdAnimEventsTimeline::SerializeEvent( animEvent, writer );
							CMemoryFileReader reader( buffer, 0 );
							if ( CExtAnimEvent* newEvent = CEdAnimEventsTimeline::DeserializeAsEvent( reader ) )
							{
								animEntry->AddEvent( newEvent );
							}
						}
					}
				}

				RefreshPage();
			}

		}
		wxTheClipboard->Close();
	}
}

static Bool GetDataFromClipboardToPaste( CClipboardData& data )
{
	if (! wxTheClipboard->Open() )
	{
		return false;
	}

	// get data from clipboard
	Bool dataAvailable = wxTheClipboard->GetData( data );
	wxTheClipboard->Close();

	if( !dataAvailable )
	{
		// no data to paste
		return false;
	}

	return true;
}

void CEdAnimBrowser::OnPasteTimelineEventsForAnimations( wxCommandEvent& event )
{
	CClipboardData data( TXT("TimelineEvent") );
	if (! GetDataFromClipboardToPaste( data ) )
	{
		return;
	}

	CMemoryFileReader reader( data.GetData(), 0 );

	OnPasteTimelineEventsRelativeToOtherEventForAnimationsPerformOn( reader );
}

void CEdAnimBrowser::OnPasteTimelineEventsRelativeToOtherEventForAnimations( wxCommandEvent& event )
{
	CClipboardData data( TXT("TimelineEvent") );
	if (! GetDataFromClipboardToPaste( data ) )
	{
		return;
	}

	CMemoryFileReader reader( data.GetData(), 0 );

	CAnimBrowserPasteEventsRelativeToOtherEventDialog dlg( nullptr, TXT("Paste events relative to other event") );
	if ( dlg.ShowModal() && ! dlg.GetEventName().Empty() )
	{
		OnPasteTimelineEventsRelativeToOtherEventForAnimationsPerformOn( reader, dlg.GetEventName(), dlg.GetRelPos() );
	}
}

void CEdAnimBrowser::OnPasteTimelineEventsRelativeToOtherEventForAnimationsPerformOn( CMemoryFileReader & reader, CName const & relativeToEvent, Float relPos )
{
	Int32 selIndex = 0;
	Int32 selCount = 0;

	CUndoAnimBrowserPasteEvents::PrepareCreationStep( *m_undoManager, this, reader, relativeToEvent, relPos );
	while ( CSkeletalAnimationSetEntry* selectedAnimation = GetSelectedAnimationEntry( selIndex ++, &selCount ) )
	{
		PasteTimelineEventsToAnimation( reader, selectedAnimation, relativeToEvent, relPos );
	}
	CUndoAnimBrowserPasteEvents::FinalizeStep( *m_undoManager );
}

void CEdAnimBrowser::OnPasteTimelineEventsForAnimset( wxCommandEvent& event )
{
	CClipboardData data( TXT("TimelineEvent") );
	if (! GetDataFromClipboardToPaste( data ) )
	{
		return;
	}

	CMemoryFileReader reader( data.GetData(), 0 );

	OnPasteTimelineEventsRelativeToOtherEventForAnimsetPerformOn( reader );
}

void CEdAnimBrowser::OnPasteTimelineEventsRelativeToOtherEventForAnimset( wxCommandEvent& event )
{
	CClipboardData data( TXT("TimelineEvent") );
	if (! GetDataFromClipboardToPaste( data ) )
	{
		return;
	}

	CMemoryFileReader reader( data.GetData(), 0 );

	CAnimBrowserPasteEventsRelativeToOtherEventDialog dlg( nullptr, TXT("Paste events relative to other event") );
	if ( dlg.ShowModal() && ! dlg.GetEventName().Empty() )
	{
		OnPasteTimelineEventsRelativeToOtherEventForAnimsetPerformOn( reader, dlg.GetEventName(), dlg.GetRelPos() );
	}
}

void CEdAnimBrowser::OnPasteTimelineEventsRelativeToOtherEventForAnimsetPerformOn( CMemoryFileReader & reader, CName const & relativeToEvent, Float relPos )
{
	CAnimBrowserPasteEventsRelativeToOtherEventDialog dlg( nullptr, TXT("Paste events relative to other event") );
	if ( dlg.ShowModal() && ! dlg.GetEventName().Empty() )
	{
		if ( CSkeletalAnimationSet* animSet = GetSelectedAnimSet() )
		{
			CUndoAnimBrowserPasteEvents::PrepareCreationStep( *m_undoManager, this, reader, relativeToEvent, relPos );
			for_each_ptr( anim, animSet->GetAnimations() )
			{
				PasteTimelineEventsToAnimation( reader, anim, relativeToEvent, relPos );
			}
			CUndoAnimBrowserPasteEvents::FinalizeStep( *m_undoManager );
		}
	}
}

static void PasteTimelineEventsToAnimationWorker( CMemoryFileReader & reader, CSkeletalAnimationSetEntry* animEntry, Float * atPos, TDynArray< CExtAnimEvent* >* eventsCreated, TDynArray< CExtAnimEvent* > & createdEventsLocal )
{
	reader.Seek(0);

	// Read number of items
	Uint32 numItems;
	reader << numItems;

	TDynArray< CExtAnimEvent* > existingEvents;
	animEntry->GetAllEvents( existingEvents );

	// Read items
	for( Uint32 i = 0; i < numItems; ++i )
	{
		// Read item
		CExtAnimEvent* animEvent = CEdAnimEventsTimeline::DeserializeAsEvent( reader );
		ASSERT( animEvent != NULL );

		Float startTime = atPos? *atPos : animEvent->GetStartTime();
		animEvent->SetAnimationName( animEntry->GetAnimation()->GetName() );

		// avoid adding event if there already is one at the same time, track with same name
		Bool addEvent = true;
		for_each_ptr( existingEvent, existingEvents )
		{
			if ( existingEvent->GetStartTime() == startTime &&
				 existingEvent->GetTrackName() == animEvent->GetTrackName() &&
				 existingEvent->GetEventName() == animEvent->GetEventName() )
			{
				addEvent = false;
				break;
			}
		}

		if ( addEvent )
		{
			animEvent->SetStartTime( startTime );
			animEntry->AddEvent( animEvent );

			if ( eventsCreated )
			{
				eventsCreated->PushBack( animEvent );
			}
			else
			{
				createdEventsLocal.PushBack( animEvent );
			}
		}
		else
		{
			delete animEvent;
		}
	}
}

void CEdAnimBrowser::PasteTimelineEventsToAnimation( CMemoryFileReader & reader, CSkeletalAnimationSetEntry* animEntry, CName const & eventName, Float relPos, TDynArray< CExtAnimEvent* >* eventsCreated )
{
	if (! wxTheClipboard->Open() || ! animEntry || ! animEntry->GetAnimation() )
	{
		return;
	}

	TDynArray< CExtAnimEvent* > createdEventsLocal;

	if ( eventName.Empty() )
	{
		PasteTimelineEventsToAnimationWorker( reader, animEntry, nullptr, eventsCreated, createdEventsLocal );
	}
	else
	{
		TDynArray< CExtAnimEvent* > foundEvents;

		TDynArray< CExtAnimEvent* > events;
		animEntry->GetAllEvents( events );
		for_each_ptr( event, events )
		{
			if ( event->GetEventName() == eventName )
			{
				foundEvents.PushBack( event );
			}
		}

		if ( foundEvents.Size() == 0 )
		{
			return;
		}

		for_each_ptr( foundEvent, foundEvents )
		{
			Float atTime = foundEvent->GetStartTime() + relPos;
			PasteTimelineEventsToAnimationWorker( reader, animEntry, &atTime, eventsCreated, createdEventsLocal );
		}
	}

	if ( ! eventsCreated )
	{
		CUndoAnimBrowserPasteEvents::PrepareAddForEntry( *m_undoManager, animEntry, createdEventsLocal );
	}

	if ( m_selectedAnimation == animEntry )
	{
		// reselect
		SelectAnimation( animEntry );
	}
}

void CEdAnimBrowser::OnFindAndRemoveEventsForAnimations( wxCommandEvent& event )
{
	CAnimBrowserFindEventDialog dlg( nullptr, TXT("Find and remove events") );
	if ( dlg.ShowModal() && ! dlg.GetEventName().Empty() )
	{
		Int32 selIndex = 0;
		Int32 selCount = 0;

		CUndoAnimBrowserFindAndRemoveEvents::PrepareCreationStep( *m_undoManager, this );
		while ( CSkeletalAnimationSetEntry* selectedAnimation = GetSelectedAnimationEntry( selIndex ++, &selCount ) )
		{
			FindAndRemoveEvents( selectedAnimation, dlg.GetEventName() );
		}
		CUndoAnimBrowserFindAndRemoveEvents::FinalizeStep( *m_undoManager );
	}
}

void CEdAnimBrowser::OnFindAndRemoveEventsForAnimset( wxCommandEvent& event )
{
	CAnimBrowserFindEventDialog dlg( nullptr, TXT("Find and remove events") );
	if ( dlg.ShowModal() && ! dlg.GetEventName().Empty() )
	{
		if ( CSkeletalAnimationSet* animSet = GetSelectedAnimSet() )
		{
			CUndoAnimBrowserFindAndRemoveEvents::PrepareCreationStep( *m_undoManager, this );
			for_each_ptr( anim, animSet->GetAnimations() )
			{
				FindAndRemoveEvents( anim, dlg.GetEventName() );
			}
			CUndoAnimBrowserFindAndRemoveEvents::FinalizeStep( *m_undoManager );
		}
	}
}

void CEdAnimBrowser::FindAndRemoveEvents( CSkeletalAnimationSetEntry* animEntry, CName const & eventName )
{
	TDynArray< CExtAnimEvent* > events;
	animEntry->GetAllEvents( events );
	for_each_ptr( event, events )
	{
		if ( event->GetEventName() == eventName )
		{
			CUndoAnimBrowserFindAndRemoveEvents::PrepareAddRemovedEvent( *m_undoManager, animEntry, event );
			animEntry->RemoveEvent( event );
		}
	}

	if ( m_selectedAnimation == animEntry )
	{
		// reselect
		SelectAnimation( animEntry );
	}
}

void CEdAnimBrowser::OnFindAndMoveEventsForAnimations( wxCommandEvent& event )
{
	CAnimBrowserPasteEventsRelativeToOtherEventDialog dlg( nullptr, TXT("Find and move events") );
	if ( dlg.ShowModal() && ! dlg.GetEventName().Empty() )
	{
		Int32 selIndex = 0;
		Int32 selCount = 0;

		CUndoAnimBrowserFindAndMoveEvents::PrepareCreationStep( *m_undoManager, this, dlg.GetEventName(), dlg.GetRelPos() );
		while ( CSkeletalAnimationSetEntry* selectedAnimation = GetSelectedAnimationEntry( selIndex ++, &selCount ) )
		{
			FindAndMoveEvents( selectedAnimation, dlg.GetEventName(), dlg.GetRelPos() );
		}
		CUndoAnimBrowserFindAndMoveEvents::FinalizeStep( *m_undoManager );
	}
}

void CEdAnimBrowser::OnFindAndMoveEventsForAnimset( wxCommandEvent& event )
{
	CAnimBrowserPasteEventsRelativeToOtherEventDialog dlg( nullptr, TXT("Find and move events") );
	if ( dlg.ShowModal() && ! dlg.GetEventName().Empty() )
	{
		if ( CSkeletalAnimationSet* animSet = GetSelectedAnimSet() )
		{
			CUndoAnimBrowserFindAndMoveEvents::PrepareCreationStep( *m_undoManager, this, dlg.GetEventName(), dlg.GetRelPos() );
			for_each_ptr( anim, animSet->GetAnimations() )
			{
				FindAndMoveEvents( anim, dlg.GetEventName(), dlg.GetRelPos() );
			}
			CUndoAnimBrowserFindAndMoveEvents::FinalizeStep( *m_undoManager );
		}
	}
}

void CEdAnimBrowser::FindAndMoveEvents( CSkeletalAnimationSetEntry* animEntry, CName const & eventName, Float relPos )
{
	TDynArray< CExtAnimEvent* > events;
	animEntry->GetAllEvents( events );
	for_each_ptr( event, events )
	{
		if ( event->GetEventName() == eventName )
		{
			Float prevLoc = event->GetStartTime();
			event->SetStartTime( prevLoc + relPos );

			CUndoAnimBrowserFindAndMoveEvents::PrepareAddForEntry( *m_undoManager, animEntry );
		}
	}

	if ( m_selectedAnimation == animEntry )
	{
		// reselect
		SelectAnimation( animEntry );
	}
}

void CEdAnimBrowser::OnFindAndEditEventsForAnimations( wxCommandEvent& event )
{
	TDynArray< CExtAnimEvent* > events;
	CAnimBrowserFindEventDialog dlg( nullptr, TXT("Find and edit events") );
	if ( dlg.ShowModal() && ! dlg.GetEventName().Empty() )
	{
		Int32 selIndex = 0;
		Int32 selCount = 0;

		while ( CSkeletalAnimationSetEntry* selectedAnimation = GetSelectedAnimationEntry( selIndex ++, &selCount ) )
		{
			FindEventsToEdit( selectedAnimation, dlg.GetEventName(), events );
		}
		EditEvents( events );
	}
}

void CEdAnimBrowser::OnFindAndEditEventsForAnimset( wxCommandEvent& event )
{
	TDynArray< CExtAnimEvent* > events;
	CAnimBrowserFindEventDialog dlg( nullptr, TXT("Find and edit events") );
	if ( dlg.ShowModal() && ! dlg.GetEventName().Empty() )
	{
		if ( CSkeletalAnimationSet* animSet = GetSelectedAnimSet() )
		{
			for_each_ptr( anim, animSet->GetAnimations() )
			{
				FindEventsToEdit( anim, dlg.GetEventName(), events );
			}
		}
		EditEvents( events );
	}
}

void CEdAnimBrowser::FindEventsToEdit( CSkeletalAnimationSetEntry* animEntry, CName const & eventName, TDynArray< CExtAnimEvent* > & outEvents )
{
	TDynArray< CExtAnimEvent* > events;
	animEntry->GetAllEvents( events );
	for_each_ptr( event, events )
	{
		if ( event->GetEventName() == eventName )
		{
			outEvents.PushBack( event );
		}
	}
}

void CEdAnimBrowser::EditEvents( TDynArray< CExtAnimEvent* > & events )
{
	if ( events.Size() && m_timeline )
	{
		if ( CEdPropertiesPage* propertiesPage = m_timeline->GetPropertiesPage() )
		{
			propertiesPage->SetObjects( events );
			propertiesPage->Show( true );
			m_timeline->UpdateSizeOfPropertiesPage();
		}
	}
}

void CEdAnimBrowser::OnAnimStreamingTypeChanged( wxCommandEvent& event )
{
	ESkeletalAnimationStreamingType type = SAST_Standard;

	if ( event.GetId() == ID_STREAMING_TYPE_2 )
	{
		type = SAST_Prestreamed;
	}
	else if ( event.GetId() == ID_STREAMING_TYPE_3 )
	{
		type = SAST_Persistent;
	}

	CSkeletalAnimationSetEntry* animEntry = GetSelectedAnimationEntry();
	if ( animEntry && animEntry->GetAnimation() )
	{
		animEntry->GetAnimation()->SetStreamingType( type );

		RefreshPage();
	}
}

void CEdAnimBrowser::OnCreateVirtualAnimation( wxCommandEvent& event )
{
	CSkeletalAnimationSet* animSet = GetSelectedAnimSet();
	if ( animSet && m_animatedComponent )
	{
		String animNameStr;
		if ( InputBox( this, TXT("Virtual animation"), TXT("Animation name:"), animNameStr ) )
		{
			CName animName( animNameStr );

			if ( animSet->FindAnimation( animName ) )
			{
				wxMessageBox( wxString::Format( wxT("Animation with name '%s' is already in animation set."), animName.AsString().AsChar() ), wxT("Error") );
				return;
			}

			CVirtualSkeletalAnimation* animation = new CVirtualSkeletalAnimation;
			animation->SetName( animName );

			if ( animSet->AddAnimation( animation ) )
			{
				// Mark modified but allow processing even if not checked out
				animSet->MarkModified();

				RefreshPage();

				CSkeletalAnimationSetEntry* entry = animSet->FindAnimation( animName );
				ASSERT( entry->GetAnimation() == animation );

				CEdAnimationBuilder* builder = new CEdAnimationBuilder( NULL, entry, m_animatedComponent );
				builder->Show();
				builder->SetFocus();
				builder->Raise();
			}
		}
	}
}

void CEdAnimBrowser::OnEditVirtualAnimation( wxCommandEvent& event )
{
	CSkeletalAnimationSetEntry* animEntry = GetSelectedAnimationEntry();
	if ( animEntry )
	{
		CVirtualSkeletalAnimation* animation = SafeCast< CVirtualSkeletalAnimation >( animEntry->GetAnimation() );
		if ( animation )
		{
			CEdAnimationBuilder* builder = new CEdAnimationBuilder( NULL, animEntry );
			builder->Show();
			builder->SetFocus();
			builder->Raise();
		}
	}
}

//////////////////////////////////////////////////////////////////////////

void CEdAnimBrowser::ShowCustomPagePanel( unsigned int page )
{
	Uint32 enableLater = UINT_MAX;

	// First disable any currently enabled panel
	for ( Uint32 i=0; i<m_pages.Size(); ++i )
	{
		if ( page != i + PAGE_CUSTOM )
		{
			m_pages[i]->EnablePanel( false );
		}
		else
		{
			enableLater = i;
		}
	}

	// And then enable the new custom page panel after all panels have
	// been disabled to avoid having a panel with a greater index that is
	// deselected to destroy a resource used by a selected panel with a
	// lesser index
	if ( enableLater != UINT_MAX )
	{
		m_pages[enableLater]->EnablePanel( true );
	}
}

void CEdAnimBrowser::DestroyCustomPages()
{
	for ( Uint32 i=0; i<m_pages.Size(); ++i )
	{
		m_pages[i]->DestroyPanel();
	}
}

void CEdAnimBrowser::RefreshAnimationForCustomPages()
{
	for ( Uint32 i=0; i<m_pages.Size(); ++i )
	{
		m_pages[i]->OnRefreshAnimation();
	}
}

void CEdAnimBrowser::SelectAnimationForCustomPages()
{
	for ( Uint32 i=0; i<m_pages.Size(); ++i )
	{
		m_pages[i]->OnSelectedAnimation();
	}
}

//////////////////////////////////////////////////////////////////////////

void CEdAnimBrowser::ShowPageProperties()
{
	ASSERT( m_selectedAnimSet != NULL );

	m_propertiesPage->SetObject( m_selectedAnimSet );
}

//////////////////////////////////////////////////////////////////////////

void CEdAnimBrowser::ShowPageHavok()
{
	if ( m_selectedAnimation && m_selectedAnimation->GetAnimation() )
	{
#ifdef USE_HAVOK_ANIMATION
		TDynArray< const HavokDataBuffer* > buffs;
		m_selectedAnimation->GetAnimation()->GetHavokDataBuffers( buffs );

		m_hkAnimProp->SetObject( buffs );
#endif
	}
	else
	{
#ifdef USE_HAVOK_ANIMATION
		m_hkAnimProp->SetNoObject();
#endif
	}
}

//////////////////////////////////////////////////////////////////////////

void CEdAnimBrowser::ShowPageTrajectory()
{
	//...
}

//////////////////////////////////////////////////////////////////////////

void CEdAnimBrowser::OnLoadEntity( wxCommandEvent &event )
{
	if ( m_mode != ABM_Select && m_selectedAnimation && m_selectedAnimation->GetAnimation() )
	{
		if ( event.GetId() > ID_LOAD_ENTITY )
		{
			const Int32 index = event.GetId() - ID_LOAD_ENTITY - 1;
			ASSERT( index >= 0);

			DefaultCharactersIterator it;
			it.GoTo( (Uint32)index );

			if ( it )
			{
				LoadEntity( it.GetPath(), m_selectedAnimation->GetAnimation()->GetName().AsString() );
			}
		}
		else
		{
			String selectedResource;

			if ( GetActiveResource( selectedResource ) )
			{
				LoadEntity( selectedResource, m_selectedAnimation->GetAnimation()->GetName().AsString() );
			}
		}
	}
}

void CEdAnimBrowser::ShowForSelection()
{
	SetMode( ABM_Select );

	Show();
}

CSkeletalAnimationSet* CEdAnimBrowser::GetSelectedAnimSet() const
{
	wxArrayTreeItemIds selectedItems;
	m_animsList->GetSelections( selectedItems );
	for ( Uint32 si = 0; si < selectedItems.size(); ++si )
	{
		const wxTreeItemId& selectedId = selectedItems[si];
		if ( selectedId.IsOk() )
		{	
			SerializableItemWrapper* animData = (SerializableItemWrapper*)m_animsList->GetItemData( selectedId );
			if ( animData )
			{
				return Cast< CSkeletalAnimationSet >( animData->m_object );
			}
		}
	}

	// Nothing selected
	return NULL;
}

CSkeletalAnimationSetEntry* CEdAnimBrowser::GetSelectedAnimationEntry( Int32 selIndex, Int32 * selCount ) const
{
	wxArrayTreeItemIds selectedItems;
	m_animsList->GetSelections( selectedItems );
	if ( selCount )
	{
		*selCount = selectedItems.size();
	}
	for ( Uint32 si = 0; si < selectedItems.size(); ++si )
	{
		const wxTreeItemId& selectedId = selectedItems[si];

		if ( selectedId.IsOk() )
		{	
			SerializableItemWrapper* animData = dynamic_cast< SerializableItemWrapper* >( m_animsList->GetItemData( selectedId ) );
			if ( animData && selIndex <= 0 )
			{
				return Cast< CSkeletalAnimationSetEntry >( animData->m_object );
			}
			-- selIndex;
		}
	}

	// Animation not selected
	return NULL;
}

String CEdAnimBrowser::GetSelectedAnimation() const
{	
	wxArrayTreeItemIds selectedItems;
	m_animsList->GetSelections( selectedItems );
	for ( Uint32 si = 0; si < selectedItems.size(); ++si )
	{
		const wxTreeItemId& selectedId = selectedItems[si];

		if ( selectedId.IsOk() )
		{	
			SerializableItemWrapper* animData = (SerializableItemWrapper*)m_animsList->GetItemData( selectedId );
			if ( animData )
			{
				CSkeletalAnimationSetEntry* entry = Cast< CSkeletalAnimationSetEntry >( animData->m_object );
				if ( entry )
				{
					return m_animsList->GetItemText( selectedId ).wc_str();
				}
			}
		}
	}

	// Animation not selected
	return String::EMPTY;
}

void CEdAnimBrowser::CloneEntityFromComponent( const CComponent *originalComponent )
{
	ASSERT(m_mode == ABM_Select);

	CEntity* prototype = originalComponent->GetEntity();
	if ( prototype )
	{
		if ( CItemEntity* itemEntity = Cast< CItemEntity >( prototype ) )
		{
			if ( itemEntity->GetItemProxy() && itemEntity->GetItemProxy()->GetParentEntity() )
			{
				prototype = itemEntity->GetItemProxy()->GetParentEntity();
			}
		}
	}

	if( prototype && prototype->GetTemplate() )
	{
		CEntityTemplate* etemplate = Cast<CEntityTemplate>( prototype->GetTemplate() );
		SEvents::GetInstance().DispatchEvent( CNAME( FileEdited ), CreateEventData< String >( etemplate->GetFile()->GetDepotPath() ) );

		EntitySpawnInfo spawnInfo;
		spawnInfo.m_template = etemplate;
		spawnInfo.m_tags = prototype->GetTags();
		spawnInfo.m_previewOnly = true;

		if( etemplate->GetEntityObject() && etemplate->GetEntityObject()->IsA< CActor >() )
		{
			CActor* actor = static_cast< CActor* >( etemplate->GetEntityObject() );
			spawnInfo.m_appearances.PushBack( actor->GetAppearance() );
			spawnInfo.m_entityClass = CActor::GetStaticClass();
		}
		m_entity = m_previewPanel->GetPreviewWorld()->GetDynamicLayer()->CreateEntitySync( spawnInfo );
	}
	else if( prototype )
	{
		TDynArray< Uint8 > buffer;
		CMemoryFileWriter writer( buffer );
		CDependencySaver saver( writer, NULL );
		// Save objects
		DependencySavingContext context( prototype );
		if ( !saver.SaveObjects( context ) )
		{
			WARN_EDITOR( TXT("Couldnt clone given entity!") );
			return;
		}
		UnloadEntity();
		LayerEntitiesArray pastedEntities;
		// paste
		m_previewPanel->GetPreviewWorld()->GetDynamicLayer()->PasteSerializedEntities( buffer, pastedEntities, true, Vector( 0.0f, 0.0f, 0.0f ), EulerAngles( 0.0f, 0.0f, 0.0f ) );
		if ( pastedEntities.Size() == 1 )
		{
			m_entity = pastedEntities[0];
		}
	}
	
	if( m_entity )
	{
		TDynArray< CAnimatedComponent* > comps;
		{
			CActor* actor = Cast< CActor >( m_entity ); 
			if( actor != NULL )
			{
				if ( actor->GetInventoryComponent() )
				{
					actor->InitInventory();
					actor->GetInventoryComponent()->SpawnMountedItems();

					CTimeCounter timer;
					while ( timer.GetTimePeriod() < 10.f && SItemEntityManager::GetInstance().IsDoingSomething() )
					{
						SItemEntityManager::GetInstance().OnTick( 0.001f );
					}
				}
			}

			CollectEntityComponentsWithItems( m_entity, comps );
		}

		//find the cloned component
		//m_animatedComponent = m_entity->FindComponent< CAnimatedComponent >( originalComponent->GetName() );
		for ( Uint32 i=0; i<comps.Size(); ++i )
		{
			if ( comps[ i ]->GetName() == originalComponent->GetName() )
			{
				m_animatedComponent = Cast< CAnimatedComponent >( comps[ i ] );
			}
		}
	}
	else
	{
		m_entity = NULL;
		m_animatedComponent = NULL;
		return;
	}

	m_entity->SetPosition( Vector::ZERO_3D_POINT );
	m_entity->SetRotation( EulerAngles::ZEROS );

	CWorldTickInfo info( m_previewPanel->GetPreviewWorld(), 0.1f );
	m_previewPanel->GetPreviewWorld()->Tick( info );

	AnalizeEntity();

	CheckAnimsetContent();

	EntityChanged();

	m_previewPanel->OnLoadEntity( m_entity );
	m_previewPanel->SetAnimatedComponent( m_animatedComponent );
}

void CEdAnimBrowser::LoadEntity(const String &entName, const String &animName )
{
	// destroy previous entity
	UnloadEntity();

	m_entityTemplate = LoadResource< CEntityTemplate>( entName );
	if ( !m_entityTemplate )
	{
		WARN_EDITOR( TXT("Failed to load %s entity"), entName.AsChar() );
		return;
	}

	EntitySpawnInfo einfo;
	einfo.m_template = m_entityTemplate;
	einfo.m_name = TXT("PreviewEntity");
	einfo.m_previewOnly = true;

	m_entity = m_previewPanel->GetPreviewWorld()->GetDynamicLayer()->CreateEntitySync( einfo );

	if ( m_entity )
	{
		//find the animated component
		TDynArray< CAnimatedComponent* > comps;

		CActor* actor = Cast< CActor >( m_entity ); 
		if( actor != NULL )
		{
			if ( actor->GetInventoryComponent() )
			{
				actor->InitInventory();
				actor->GetInventoryComponent()->SpawnMountedItems();

				CTimeCounter timer;
				while ( timer.GetTimePeriod() < 10.f && SItemEntityManager::GetInstance().IsDoingSomething() )
				{
					SItemEntityManager::GetInstance().OnTick( 0.001f );
				}
			}
		}

		CollectEntityComponentsWithItems( m_entity, comps );

		for (Uint32 i=0; i<comps.Size(); i++)
		{
			CSkeletalAnimationSetEntry* anim = comps[i]->GetAnimationContainer()->FindAnimation( CName( animName ) );
			if (anim)
			{
				CAnimatedComponent* temp = m_animatedComponent;	
				m_animatedComponent = comps[i];	//find anim set needs m_animatedComponent
				CSkeletalAnimationSet* set = FindAnimSetContainingAnimation(anim);
				if (set)
				{
					SelectAnimSet( set );
					m_animatedComponent = comps[i];
					m_selectedAnimation = anim;
				}
				else
				{
					m_animatedComponent = temp;
				}
				
			}
		}

		if ( !m_animatedComponent )
		{
			AddDynamicAnimset();
		}

		CWorldTickInfo info( m_previewPanel->GetPreviewWorld(), 0.1f );
		m_previewPanel->GetPreviewWorld()->Tick( info );

		AnalizeEntity();
	}
	else
	{
		WARN_EDITOR( TXT("Failed to load %s entity"), entName.AsChar() );
	}	

	EntityChanged();

	m_previewPanel->OnLoadEntity( m_entity );
	m_previewPanel->SetAnimatedComponent( m_animatedComponent );
	m_previewPanel->SetPlayedAnimation( m_playedAnimation->m_playedAnimation );

	// Update bone editor
	CEffectBoneListSelection::SetEntity( m_entity );
}

void CEdAnimBrowser::AnalizeEntity()
{
	if ( m_entity )
	{
		// Check animated component
		if ( !m_animatedComponent )
		{	
			WARN_EDITOR( TXT("No animated componets in entity!") );
			UnloadEntity();
			return;
		}

		if ( m_animatedComponent->GetBehaviorStack() )
		{
			m_animatedComponent->GetBehaviorStack()->Deactivate();
		}

		CMovingAgentComponent* mac = Cast< CMovingAgentComponent >( m_animatedComponent );
		if ( mac )
		{
			mac->ForceEntityRepresentation( true );
		}

		// Update motion tracking
		{
			wxToolBar* toolbar = XRCCTRL( *this, "playbackControlToolbar", wxToolBar );
			toolbar->ToggleTool( XRCID("movementButton"), false );
			UseMotionExtraction( false );
		}

		if ( CMimicComponent* mimicComponent = Cast< CMimicComponent >( m_animatedComponent ) )
		{
			mimicComponent->MimicHighOn();

			CAnimatedComponent* ac = m_entity->GetRootAnimatedComponent();
			if ( ac && ac->GetBehaviorStack() )
			{
				ac->GetBehaviorStack()->Deactivate();

				CMovingAgentComponent* mac = Cast< CMovingAgentComponent >( ac );
				if ( mac )
				{
					mac->ForceEntityRepresentation( true );
				}
			}

			if ( mimicComponent->GetBehaviorStack() )
			{
				mimicComponent->GetBehaviorStack()->Activate();
			}

			m_playedAnimation->SetMimic( mimicComponent );
		}
	}
}

void CEdAnimBrowser::UnloadEntity()
{
	if ( m_entity )
	{
		RemoveDynamicAnimset();

		m_previewPanel->GetPreviewWorld()->DelayedActions();
		m_entity->Destroy();
		m_previewPanel->GetPreviewWorld()->DelayedActions(); 
		m_animatedComponent = NULL;
		m_entity = NULL;
		m_playedAnimation->Reset();
	}

	m_previewPanel->OnUnloadEntity();
	m_previewPanel->SetAnimatedComponent( NULL );
	m_previewPanel->SetPlayedAnimation( NULL );

	// Update bone editor
	CEffectBoneListSelection::SetEntity( NULL );

	m_tPoseConstraint = -1;
}

void CEdAnimBrowser::ShowEntity( Bool flag )
{
	if ( m_entity )
	{
		m_entity->SetHideInGame( !flag, false, CEntity::HR_Default );
	}
}

void CEdAnimBrowser::UseMotionExtraction( Bool flag )
{
	m_useMotionExtraction = flag;
	if ( m_animatedComponent )
	{
		m_animatedComponent->SetUseExtractedMotion( m_useMotionExtraction );
		
		// To be honest, I don't know, whether this is a good solution or not, seems, that earlier CMovingAgentComponent
		// was not used at all here, but I don't have the time to investigate it. Probably [HACK].
		if ( CMovingAgentComponent* mac = Cast< CMovingAgentComponent >( m_animatedComponent ) )
			mac->SetMotionEnabled( flag );
	}
}

void CEdAnimBrowser::UseTrajectoryExtraction( Bool flag )
{	
	if ( m_animatedComponent )
	{
		m_animatedComponent->SetUseExtractedTrajectory( flag );
	}
}

void CEdAnimBrowser::UpdatePlayPauseIcon()
{
	wxToolBar* tb = XRCCTRL( *this, "playbackControlToolbar", wxToolBar );

	if ( m_playedAnimation && !m_playedAnimation->IsPaused() )
	{
		tb->SetToolNormalBitmap( XRCID( "playButton" ), m_pauseIcon );
	}
	else
	{
		tb->SetToolNormalBitmap( XRCID( "playButton" ), m_playIcon );
	}
}

void CEdAnimBrowser::UpdateMotionExTypeIcon()
{
	wxToolBar* tb = XRCCTRL( *this, "playbackControlToolbar", wxToolBar );

	if ( m_motionExType )
	{
		tb->ToggleTool( XRCID( "exCompressed" ), true );
		tb->ToggleTool( XRCID( "exUncompressed" ), false );
	}
	else
	{
		tb->ToggleTool( XRCID( "exCompressed" ), false );
		tb->ToggleTool( XRCID( "exUncompressed" ), true );
	}
}

void CEdAnimBrowser::UpdateMotionExTypeForAnim()
{
	if ( m_selectedAnimation && m_selectedAnimation->GetAnimation() )
	{
		m_selectedAnimation->GetAnimation()->UseCompressMotionExtraction( m_motionExType ); 
	}
}

void CEdAnimBrowser::PlayCurrentAnimation()
{
	if ( !m_selectedAnimation || !m_animatedComponent || !m_animatedComponent->GetAnimatedSkeleton() )
		return;

	m_playedAnimation->Set( m_animatedComponent, m_selectedAnimation, this );

	m_previewPanel->SetPlayedAnimation( m_playedAnimation->m_playedAnimation );
}

Bool CEdAnimBrowser::IsPaused() const
{
	return m_playedAnimation && m_playedAnimation->IsPaused();
}

void CEdAnimBrowser::Pause( Bool flag )
{
	if ( m_playedAnimation && ( ( IsPaused() && flag == true ) || ( !IsPaused() && flag == false ) ) )
	{
		PlayPauseAnimation();
	}
}

void CEdAnimBrowser::PlayPauseAnimation()
{
	if ( !m_playedAnimation )
	{
		PlayCurrentAnimation();
	}
	else if ( m_playedAnimation && !m_playedAnimation->IsPaused() )
	{
		m_playedAnimation->Pause();
	}
	else if ( m_playedAnimation && m_playedAnimation->IsPaused() )
	{
		m_playedAnimation->Unpause();
	}

	UpdatePlayPauseIcon();

	RefreshAnimationForCustomPages();

	//EnableTimeFlow( !IsTimeFlowEnabled() );

	//
}

void CEdAnimBrowser::ResetAnimation( bool setToEnd )
{
	if ( m_playedAnimation )
	{
		m_playedAnimation->SetTime( setToEnd ? m_playedAnimation->GetDuration() : 0.0f );
	}

	RefreshAnimationForCustomPages();
}

void CEdAnimBrowser::UpdateCompressedPose()
{
	if ( m_playedAnimation && m_forceCompressedPose )
	{
		m_playedAnimation->ForceCompressedPose();
	}

	/*if ( m_isPlayingCompressedPose && m_animatedComponent )
	{
		CSkeletalAnimationSetEntry* animEntry = GetSelectedAnimationEntry();
		if ( animEntry && animEntry->GetAnimation() )
		{
			CSkeletalAnimation* anim = animEntry->GetAnimation();

			CSkeletalAnimationSet* set = animEntry->GetAnimSet();
			if ( set )
			{
				const ICompressedPose* comprPose = set->FindCompressedPoseByName( anim->GetCompressedPoseName() );
				if ( comprPose )
				{
					const CSkeleton* skeleton = m_animatedComponent->GetSkeleton();

					CPoseAllocator* poseAlloc = skeleton->GetPoseAllocator();
					if ( poseAlloc )
					{
						SBehaviorGraphOutput* pose = poseAlloc->AllocPose();

						comprPose->DecompressAndSample( pose->m_numBones, pose->m_outputPose, skeleton );
						m_animatedComponent->ForceBehaviorPose( *pose );

						poseAlloc->ReleasePose( pose );
					}
				}
			}
		}
	}*/
}

void CEdAnimBrowser::OnPlayPause( wxCommandEvent& event )
{
	PlayPauseAnimation();
}

void CEdAnimBrowser::OnReset( wxCommandEvent& event )
{
	ResetAnimation();
}

Bool CEdAnimBrowser::IsLoopingEnabled() const
{
	wxToolBar* tb = XRCCTRL( *this, "playbackControlToolbar", wxToolBar );

	return tb->GetToolState( XRCID( "loopButton" ) );
}

void CEdAnimBrowser::OnOk( wxCommandEvent& event )
{
	// Check select anim
	if ( m_animatedComponent && m_selectedAnimation )
	{
		const CName& selectAnimName = m_selectedAnimation->GetAnimation()->GetName();

		CSkeletalAnimationContainer *animContainer = m_animatedComponent->GetAnimationContainer();
		const TSkeletalAnimationSetsArray& animSets = animContainer->GetAnimationSets();

		// Find
		Bool searching = true;

		for( auto it = animSets.Begin(), end = animSets.End(); ( it != end ) && searching; ++it )
		{
			TDynArray< CSkeletalAnimationSetEntry* > setAnims;
			( *it )->GetAnimations( setAnims );

			for( Uint32 j=0; j<setAnims.Size() && searching; ++j )
			{
				CSkeletalAnimationSetEntry *currAnim = setAnims[ j ];

				if (m_selectedAnimation==currAnim)
				{
					// Ok
					searching = false;
				}
				else if ( currAnim->GetAnimation() && currAnim->GetAnimation()->GetName() == selectAnimName )
				{
					// Warning
					wxString msg = wxT("  This animation is not in the lastest animset.  \n");
					msg += wxT("  The lastest animset is: ")+wxString::Format(wxT("%s"), ( *it )->GetFile()->GetFileName().AsChar())+wxT("  \n");
					msg += wxT("\n  Do you want to change animation?  ");

					wxMessageDialog dialog( 0, msg, wxT("Warning"), wxYES_NO | wxYES_DEFAULT | wxICON_WARNING );
					if ( dialog.ShowModal() == wxID_YES ) 
					{
						return;
					}
					else
					{
						searching = false;
					}
				}
			}
		}
	}


	wxCommandEvent okPressedEvent( wxEVT_ANIM_CONFIRMED );
	wxString animName = m_selectedAnimation ? wxString( m_selectedAnimation->GetName().AsString().AsChar() ) : wxT( "None" ); 
	okPressedEvent.SetString( animName );
	ProcessEvent( okPressedEvent );

	Hide();	
}

void CEdAnimBrowser::OnCancel( wxCommandEvent& event )
{
	wxCommandEvent cancelPressedEvent( wxEVT_ANIM_ABANDONED );
	ProcessEvent( cancelPressedEvent );

	Hide();	
}


void CEdAnimBrowser::OnModelessClose(wxCommandEvent &event )
{
	wxCloseEvent fake;
	OnClose(fake);
}

void CEdAnimBrowser::OnResetConfig( wxCommandEvent& event )
{
	ResetConfig();
	m_resettingConfig = true;
	Close();
}

void CEdAnimBrowser::OnClose( wxCloseEvent& event )
{
	if ( m_mode == ABM_Select )
	{
		wxCommandEvent dummyEvent;
		OnCancel( dummyEvent );
	}
	else
	{
		if ( !m_resettingConfig )
		{
			SaveOptionsToConfig();
		}

		DestroyCustomPages();

		UnloadEntity();
		Destroy();
	}
}

void CEdAnimBrowser::OnRequestSetTime( wxCommandEvent &event )
{
	TClientDataWrapper< Float>* clientData = dynamic_cast< TClientDataWrapper< Float >* >( event.GetClientObject() );
	ASSERT( clientData != NULL );

	Float time = clientData->GetData();

	if ( m_playedAnimation )
	{
		m_playedAnimation->SetTime( time );
	}

	RefreshAnimationForCustomPages();
}

void CEdAnimBrowser::EntityChanged()
{
	RefreshPage();
}

String CEdAnimBrowser::GetAnimSetName( CSkeletalAnimationSet *animSet ) const
{	
	CDiskFile *animSetFile = animSet->GetFile();
	
	if ( animSetFile )
		return animSetFile->GetDepotPath();
	else
		return String( TXT( "<unknown>" ) );
}

wxTreeItemId CEdAnimBrowser::FindAnimationTreeItem( CSkeletalAnimationSetEntry *anim )
{
	TQueue< wxTreeItemId >	items;
	items.Push( m_animsList->GetRootItem() );

	while( !items.Empty() )
	{
		wxTreeItemId &currItem = items.Front();
		items.Pop();

		// check current item
		SerializableItemWrapper* animData = (SerializableItemWrapper*)( m_animsList->GetItemData( currItem ) );
		if ( animData && animData->m_object == anim )
		{
			return currItem;
		}

		// recurse into children
		wxTreeItemIdValue cookie = 0;
		wxTreeItemId child = m_animsList->GetFirstChild( currItem, cookie );
		while( child.IsOk() )
		{
			items.Push( child );
			child = m_animsList->GetNextChild( currItem, cookie );
		}
	}

	return wxTreeItemId();
}

CSkeletalAnimationSet* CEdAnimBrowser::FindAnimSetContainingAnimation( CSkeletalAnimationSetEntry *anim )
{
	TSkeletalAnimationSetsArray animSets;
	GetAnimsets(animSets);

	for( auto it = animSets.Begin(), end = animSets.End(); it != end; ++it )
	{
		CSkeletalAnimationSet* set = ( *it ).Get();

		TDynArray< CSkeletalAnimationSetEntry* > setAnims;
		//animSets[i]->GetAnimations( setAnims );
		GetAnimationsFromAnimset( set, setAnims);

		if ( setAnims.Exist( anim ) )
		{
			return set;
		}
	}
	
	return NULL;
}

void CEdAnimBrowser::OnAllAnimsRClick( wxMouseEvent &event )
{
	OpenAllAnimsContextMenu();
}

void CEdAnimBrowser::OnAnimSetsRClick( wxMouseEvent &event )
{
	OpenAnimSetsContextMenu();
}

void CEdAnimBrowser::OnAnimsRClick( wxMouseEvent &event )
{
	OpenAnimsContextMenu();
}

void CEdAnimBrowser::OpenAllAnimsContextMenu()
{
	if ( m_mode == ABM_Normal_Animset )
	{
		String itemAnim = GetSelectedAnimation();
		if ( !itemAnim.Empty() )
		{
			wxMenu menu;

			if ( m_extraAnimInfo == I_Streaming )
			{
				menu.Append( ID_STREAMING_TYPE_1, TXT("Standard") );
				menu.Connect( ID_STREAMING_TYPE_1, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAnimBrowser::OnAnimStreamingTypeChanged ), NULL, this);
				menu.Append( ID_STREAMING_TYPE_2, TXT("Prestreamed") );
				menu.Connect( ID_STREAMING_TYPE_2, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAnimBrowser::OnAnimStreamingTypeChanged ), NULL, this);
				menu.Append( ID_STREAMING_TYPE_3, TXT("Persistent") );
				menu.Connect( ID_STREAMING_TYPE_3, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAnimBrowser::OnAnimStreamingTypeChanged ), NULL, this);
				menu.AppendSeparator();
			}

			menu.Append( ID_REMOVE_ANIM, TXT("Remove") );
			menu.Connect( ID_REMOVE_ANIM, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(CEdAnimBrowser::OnRemoveAnimationFromAll), NULL, this);

			CSkeletalAnimationSetEntry* entry = GetSelectedAnimationEntry();
			if ( entry )
			{
				CSkeletalAnimationSet* animSet = entry->GetAnimSet();
				if ( animSet && entry->GetAnimation() )
				{
					menu.Append( ID_SYNC_ANIM_EVENTS_TO, TXT("Sync events") );
					menu.Connect( ID_SYNC_ANIM_EVENTS_TO, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(CEdAnimBrowser::OnSyncEventsTo), NULL, this);
					menu.AppendSeparator();

					if ( entry->GetAnimation()->IsA< CVirtualSkeletalAnimation >() )
					{
						menu.Append( ID_EDIT_VIRTUAL_ANIM, TXT("Edit virtual animation...") );
						menu.Connect( ID_EDIT_VIRTUAL_ANIM, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(CEdAnimBrowser::OnEditVirtualAnimation ), NULL, this);
						menu.AppendSeparator();
					}

					menu.Append( ID_RECOMPRESS_ANIMATION, TXT("Recompress") );
					menu.Connect( ID_RECOMPRESS_ANIMATION, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(CEdAnimBrowser::OnRecompressAnimation), NULL, this);
					menu.Append( ID_RECOMPRESS_ANIMATION_CHOOSE, TXT("Recompress (choose preset)") );
					menu.Connect( ID_RECOMPRESS_ANIMATION_CHOOSE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(CEdAnimBrowser::OnRecompressAnimationChoosePreset), NULL, this);
					if ( !entry->GetAnimation()->GetImportFile().Empty() )
					{
						menu.Append( ID_REIMPORT_NORMAL_ANIMATION, TXT("Reimport as simple/plain animation") );
						menu.Connect( ID_REIMPORT_NORMAL_ANIMATION, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(CEdAnimBrowser::OnReimportNormalAnimation), NULL, this);
						menu.Append( ID_REIMPORT_ADD_3_ANIMATION, TXT("Reimport as additive (skeleton)") );
						menu.Connect( ID_REIMPORT_ADD_3_ANIMATION, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(CEdAnimBrowser::OnReimportAddAnimation3), NULL, this);
						menu.Append( ID_REIMPORT_ADD_1_ANIMATION, TXT("Reimport as additive I (Local)") );
						menu.Connect( ID_REIMPORT_ADD_1_ANIMATION, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(CEdAnimBrowser::OnReimportAddAnimation1), NULL, this);
						menu.Append( ID_REIMPORT_ADD_2_ANIMATION, TXT("Reimport as additive II (Ref)") );
						menu.Connect( ID_REIMPORT_ADD_2_ANIMATION, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(CEdAnimBrowser::OnReimportAddAnimation2), NULL, this);
						menu.Append( ID_REIMPORT_ADD_4_ANIMATION, TXT("Reimport as additive (animation)") );
						menu.Connect( ID_REIMPORT_ADD_4_ANIMATION, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(CEdAnimBrowser::OnReimportAddAnimation4), NULL, this);
						menu.Append( ID_REIMPORT_MS_ANIMATION, TXT("Reimport as MS") );
						menu.Connect( ID_REIMPORT_MS_ANIMATION, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(CEdAnimBrowser::OnReimportMSAnimation), NULL, this);
						menu.Append( ID_REIMPORT_ANIMATION, TXT("Reimport (keep type)") );
						menu.Connect( ID_REIMPORT_ANIMATION, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(CEdAnimBrowser::OnReimportAnimation), NULL, this);
					}
					menu.AppendSeparator();

					menu.Append( ID_EXPORT_ANIMATION, TXT("Export animation") );
					menu.Connect( ID_EXPORT_ANIMATION, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(CEdAnimBrowser::OnExportAnimation), NULL, this);
					menu.Append( ID_EXPORT_ANIMATION_WITHOUT_TRAJECTORY, TXT("Export animation (without trajectory)") );
					menu.Connect( ID_EXPORT_ANIMATION_WITHOUT_TRAJECTORY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(CEdAnimBrowser::OnExportAnimationWithoutTrajectory), NULL, this);
					menu.AppendSeparator();

					menu.Append( ID_CONVERT_TO_ADD_1, TXT("Duplicate as additive I (Local)") );
					menu.Connect( ID_CONVERT_TO_ADD_1, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(CEdAnimBrowser::OnDuplicateAsAdd1 ), NULL, this);
					menu.Append( ID_CONVERT_TO_ADD_2, TXT("Duplicate as additive I (Ref)") );
					menu.Connect( ID_CONVERT_TO_ADD_2, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(CEdAnimBrowser::OnDuplicateAsAdd2 ), NULL, this);
					menu.AppendSeparator();

					menu.Append( ID_RENAME_ANIMATION, TXT("Rename") );
					menu.Connect( ID_RENAME_ANIMATION, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(CEdAnimBrowser::OnRenameAnimation), NULL, this);
					menu.Append( ID_GEN_MEM_REP_ANIM, TXT("Generate memory report") );
					menu.Connect( ID_GEN_MEM_REP_ANIM, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAnimBrowser::OnGenMemRepAnim ), NULL, this);

					menu.AppendSeparator();

					menu.Append( ID_REC_COMPRESSED_MEX, TXT("Recreate motion extraction") );
					menu.Connect( ID_REC_COMPRESSED_MEX, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAnimBrowser::OnRecreateCompressedMotionExtraction ), NULL, this);
					menu.Append( ID_REC_BBOX, TXT("Recreate bounding box") );
					menu.Connect( ID_REC_BBOX, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAnimBrowser::OnRecreateBBox ), NULL, this);
					menu.Append( ID_RECREATE_COMPR_POSES, TXT("Recreate compressed pose") );
					menu.Connect( ID_RECREATE_COMPR_POSES, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAnimBrowser::OnRecreateComprPose ), NULL, this);

					menu.AppendSeparator();

					if ( wxTheClipboard->Open() )
					{
						CClipboardData data( TXT("TimelineEvent") );
						if ( wxTheClipboard->IsSupported( data.GetDataFormat() ) )
						{		
							menu.Append( ID_PASTE_EVENTS, TXT("Paste events to selected animations") );
							menu.Connect( ID_PASTE_EVENTS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAnimBrowser::OnPasteTimelineEventsForAnimations ), NULL, this);
							menu.Append( ID_PASTERELATIVETOOTHER_EVENTS, TXT("Paste events to selected animations, relative to event") );
							menu.Connect( ID_PASTERELATIVETOOTHER_EVENTS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAnimBrowser::OnPasteTimelineEventsRelativeToOtherEventForAnimations ), NULL, this);
						} 
						wxTheClipboard->Close();
					}

					menu.Append( ID_FIND_AND_REMOVE_EVENTS, TXT("Find and remove events") );
					menu.Connect( ID_FIND_AND_REMOVE_EVENTS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAnimBrowser::OnFindAndRemoveEventsForAnimations ), NULL, this);

					menu.Append( ID_FIND_AND_MOVE_EVENTS, TXT("Find and move events") );
					menu.Connect( ID_FIND_AND_MOVE_EVENTS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAnimBrowser::OnFindAndMoveEventsForAnimations ), NULL, this);

					menu.Append( ID_FIND_AND_EDIT_EVENTS, TXT("Find and edit events") );
					menu.Connect( ID_FIND_AND_EDIT_EVENTS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAnimBrowser::OnFindAndEditEventsForAnimations ), NULL, this);
					
					menu.AppendSeparator();

					menu.Append( ID_ANIM_PLOT, TXT("Plot") );
					menu.Connect( ID_ANIM_PLOT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAnimBrowser::OnAnimPlot ), NULL, this);

					menu.Append( ID_ANIM_MS_PLOT, TXT("Plot MS") );
					menu.Connect( ID_ANIM_MS_PLOT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAnimBrowser::OnAnimMSPlot ), NULL, this);

					menu.Append( ID_MIMIC_ANIM_PLOT, TXT("Plot mimic") );
					menu.Connect( ID_MIMIC_ANIM_PLOT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAnimBrowser::OnMimicAnimPlot ), NULL, this);

					menu.Append( ID_ANIM_MOTION_COMPR, TXT("Compress motion") );
					menu.Connect( ID_ANIM_MOTION_COMPR, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAnimBrowser::OnAnimCompressMotion ), NULL, this);

					menu.Append( ID_CALC_BONE_MS, TXT("Calc bone MS") );
					menu.Connect( ID_CALC_BONE_MS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAnimBrowser::OnCalcBoneMS ), NULL, this);

					menu.Append( ID_CALC_BONE_WS, TXT("Calc bone WS") );
					menu.Connect( ID_CALC_BONE_WS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAnimBrowser::OnCalcBoneWS ), NULL, this);

					menu.Append( ID_CALC_BONE_WITH_MOTION_MS, TXT("Calc bone MS with motion") );
					menu.Connect( ID_CALC_BONE_WITH_MOTION_MS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAnimBrowser::OnCalcBoneWithMotionMS ), NULL, this);

					menu.Append( ID_CALC_BONE_WITH_MOTION_WS, TXT("Calc bone WS with motion") );
					menu.Connect( ID_CALC_BONE_WITH_MOTION_WS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAnimBrowser::OnCalcBoneWithMotionWS ), NULL, this);

					menu.AppendSeparator();
					
					menu.Append( ID_COPY_ANIM, TXT("Copy animation") );
					menu.Connect( ID_COPY_ANIM, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAnimBrowser::OnAnimCopy ), NULL, this);
				}
			}

			menu.AppendSeparator();
			menu.Append( ID_TAKE_SCREENSHOT, TXT("Save as bitmap sequence") );
			menu.Connect( ID_TAKE_SCREENSHOT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAnimBrowser::OnTakeScreenshot ), NULL, this ); 
			
			{
				menu.AppendSeparator();

				wxMenu* subMenu = new wxMenu();
				subMenu->Append( ID_ADD_DEBUG_POSE_START, TXT("Add first frame") );
				subMenu->Append( ID_ADD_DEBUG_POSE_END, TXT("Add last frame") );
				subMenu->Append( ID_ADD_DEBUG_POSE_TIME, TXT("Add time frame") );
				subMenu->Append( ID_REMOVE_DEBUG_POSE, TXT("Remove all") );

				menu.AppendSubMenu( subMenu, wxT("Debug poses") );
			}

			PopupMenu( &menu );
		}
		else
		{
			CSkeletalAnimationSet* animSet = GetSelectedAnimSet();
			if ( animSet )
			{
				wxMenu menu;

				if ( animSet->IsSelfContained() )
				{
					menu.Append( ID_RECOMPRESS_ANIMATIONS, TXT("Recompress all") );
					menu.Connect( ID_RECOMPRESS_ANIMATIONS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(CEdAnimBrowser::OnRecompressAnimations), NULL, this);
					menu.Append( ID_RECOMPRESS_ANIMATIONS_CHOOSE, TXT("Recompress all (choose preset)") );
					menu.Connect( ID_RECOMPRESS_ANIMATIONS_CHOOSE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(CEdAnimBrowser::OnRecompressAnimationsChoosePreset), NULL, this);
					menu.Append( ID_REIMPORT_ANIMATIONS, TXT("Reimport all") );
					menu.Connect( ID_REIMPORT_ANIMATIONS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(CEdAnimBrowser::OnReimportAnimations), NULL, this);
					menu.Append( ID_IMPORT_NORMAL_ANIMATIONS, TXT("Import simple/plain animations") );
					menu.Connect( ID_IMPORT_NORMAL_ANIMATIONS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(CEdAnimBrowser::OnImportNormalAnimations), NULL, this);
					menu.Append( ID_IMPORT_ADD_3_ANIMATIONS, TXT("Import additive animations (skeleton)") );
					menu.Connect( ID_IMPORT_ADD_3_ANIMATIONS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(CEdAnimBrowser::OnImportAddAnimations3), NULL, this);
					menu.Append( ID_IMPORT_ADD_1_ANIMATIONS, TXT("Import additive animations I (Local)") );
					menu.Connect( ID_IMPORT_ADD_1_ANIMATIONS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(CEdAnimBrowser::OnImportAddAnimations1), NULL, this);
					menu.Append( ID_IMPORT_ADD_2_ANIMATIONS, TXT("Import additive animations II (Ref)") );
					menu.Connect( ID_IMPORT_ADD_2_ANIMATIONS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(CEdAnimBrowser::OnImportAddAnimations2), NULL, this);
					menu.Append( ID_IMPORT_MS_ANIMATIONS, TXT("Import MS animations") );
					menu.Connect( ID_IMPORT_MS_ANIMATIONS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(CEdAnimBrowser::OnImportMSAnimations), NULL, this);
					menu.Append( ID_IMPORT_ADD_4_ANIMATIONS, TXT("Import additive animations (animation)") );
					menu.Connect( ID_IMPORT_ADD_4_ANIMATIONS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(CEdAnimBrowser::OnImportAddAnimations4), NULL, this);
					menu.Append( ID_IMPORT_ANIMATIONS, TXT("Import animations (keep type)") );
					menu.Connect( ID_IMPORT_ANIMATIONS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(CEdAnimBrowser::OnImportAnimations), NULL, this);
					menu.Append( ID_SORT_ANIMSET, TXT("Sort") );
					menu.Connect( ID_SORT_ANIMSET, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(CEdAnimBrowser::OnSortAnimset), NULL, this);
					menu.Append( ID_COPY_TO_CLIPBOARD, TXT("Copy to clipboard") );
					menu.Connect( ID_COPY_TO_CLIPBOARD, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(CEdAnimBrowser::OnCopyAllAnimationsToClipboard), NULL, this);
					menu.Append( ID_GENERATE_BB_ALL, TXT("Generate bounding box for all anims") );
					menu.Connect( ID_GENERATE_BB_ALL, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAnimBrowser::OnGenerateBoundingBoxForAll ), NULL, this);
					menu.Append( ID_GEN_MEM_REP_SET, TXT("Generate memory report") );
					menu.Connect( ID_GEN_MEM_REP_SET, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAnimBrowser::OnGenMemRepSet ), NULL, this);

					#ifdef _DEBUG
						menu.AppendSeparator();
						menu.Append( ID_DO_NOT_CLICK, TXT("Do not click") );
						menu.Connect( ID_DO_NOT_CLICK, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAnimBrowser::OnDoNotClick ), NULL, this);
					#endif

					menu.AppendSeparator();

					menu.Append( ID_CHECK_PART_STREAMABLE_STATS, TXT("Check partially streamable stats") );
					menu.Connect( ID_CHECK_PART_STREAMABLE_STATS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAnimBrowser::OnCheckPartStreambleStats ), NULL, this);
					menu.Append( ID_ALL_CHECK_CORRUPTED_ANIMS, TXT("Check corrupted animations") );
					menu.Connect( ID_ALL_CHECK_CORRUPTED_ANIMS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAnimBrowser::OnCheckCorruptedAnimations ), NULL, this);
					menu.Append( ID_ALL_CHECK_CORRUPTED_ANIMS_ALL, TXT("Check corrupted animations (all files in depot)") );
					menu.Connect( ID_ALL_CHECK_CORRUPTED_ANIMS_ALL, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAnimBrowser::OnCheckAllCorruptedAnimations ), NULL, this);
					menu.Append( ID_ALL_REC_COMPRESSED_MEX, TXT("Recreate motion extraction") );
					menu.Connect( ID_ALL_REC_COMPRESSED_MEX, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAnimBrowser::OnRecreateAllCompressedMotionExtraction ), NULL, this);
					menu.Append( ID_ALL_REC_BBOX, TXT("Recreate bounding box") );
					menu.Connect( ID_ALL_REC_BBOX, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAnimBrowser::OnRecreateAllBBox ), NULL, this);
					menu.Append( ID_RECREATE_COMPR_POSE, TXT("Recreate compressed poses") );
					menu.Connect( ID_RECREATE_COMPR_POSE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAnimBrowser::OnRecreateComprPoses ), NULL, this);

					if ( wxTheClipboard->Open() )
					{
						CClipboardData data( TXT("Anims") );
						if ( wxTheClipboard->IsSupported( data.GetDataFormat() ) )
						{		
							menu.AppendSeparator();

							menu.Append( ID_PASTE_ANIM, TXT("Paste animation to set") );
							menu.Connect( ID_PASTE_ANIM, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAnimBrowser::OnAnimPaste ), NULL, this);
						} 
						wxTheClipboard->Close();
					}

					menu.AppendSeparator();
					if ( wxTheClipboard->Open() )
					{
						CClipboardData data( TXT("TimelineEvent") );
						if ( wxTheClipboard->IsSupported( data.GetDataFormat() ) )
						{		
							menu.Append( ID_PASTE_EVENTS, TXT("Paste events to all animations") );
							menu.Connect( ID_PASTE_EVENTS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAnimBrowser::OnPasteTimelineEventsForAnimset ), NULL, this);
							menu.Append( ID_PASTERELATIVETOOTHER_EVENTS, TXT("Paste events to all animations, relative to event") );
							menu.Connect( ID_PASTERELATIVETOOTHER_EVENTS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAnimBrowser::OnPasteTimelineEventsRelativeToOtherEventForAnimset ), NULL, this);
						} 
						wxTheClipboard->Close();
					}

					menu.Append( ID_FIND_AND_REMOVE_EVENTS, TXT("Find and remove events") );
					menu.Connect( ID_FIND_AND_REMOVE_EVENTS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAnimBrowser::OnFindAndRemoveEventsForAnimset ), NULL, this);

					menu.Append( ID_FIND_AND_MOVE_EVENTS, TXT("Find and move events") );
					menu.Connect( ID_FIND_AND_MOVE_EVENTS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAnimBrowser::OnFindAndMoveEventsForAnimset ), NULL, this);
					
					menu.Append( ID_FIND_AND_EDIT_EVENTS, TXT("Find and edit events") );
					menu.Connect( ID_FIND_AND_EDIT_EVENTS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAnimBrowser::OnFindAndEditEventsForAnimset ), NULL, this);
					
					menu.AppendSeparator();
					menu.Append( ID_CREATE_VIRTUAL_ANIM, TXT("Create new virtual animation...") );
					menu.Connect( ID_CREATE_VIRTUAL_ANIM, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAnimBrowser::OnCreateVirtualAnimation ), NULL, this);
				}
				else
				{
					menu.Append( ID_SUCK_ANIMATIONS, TXT("Suck animations to set") );
					menu.Connect( ID_SUCK_ANIMATIONS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(CEdAnimBrowser::OnSuckAnimaitons), NULL, this);
				}

				PopupMenu( &menu );
			}
		}
	}
	else if ( m_mode == ABM_Select )
	{
		String itemAnim = GetSelectedAnimation();
		if ( !itemAnim.Empty() )
		{
			wxMenu menu;

			wxMenu* subMenu = new wxMenu();
			subMenu->Append( ID_ADD_DEBUG_POSE_START, TXT("Add first frame") );
			subMenu->Append( ID_ADD_DEBUG_POSE_END, TXT("Add last frame") );
			subMenu->Append( ID_ADD_DEBUG_POSE_TIME, TXT("Add time frame") );
			subMenu->Append( ID_REMOVE_DEBUG_POSE, TXT("Remove all") );

			menu.AppendSubMenu( subMenu, wxT("Debug poses") );

			PopupMenu( &menu );
		}
	}
}

void CEdAnimBrowser::OpenAnimSetsContextMenu()
{

}

void CEdAnimBrowser::OpenAnimsContextMenu()
{
	if (m_mode == ABM_Normal_Animset && m_selectedAnimation != NULL )
	{
		wxMenu menu;
		menu.Append( ID_REMOVE_ANIM, TXT("Remove") );
		menu.Connect( ID_REMOVE_ANIM, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(CEdAnimBrowser::OnRemoveAnimationFromAnimList), NULL, this);
		PopupMenu( &menu );
	}
}

void CEdAnimBrowser::OnRemoveAnimationFromAll( wxCommandEvent &event )
{
	ASSERT( m_selectedAnimSet != NULL );

	wxArrayTreeItemIds selections;
	m_animsList->GetSelections( selections );

	// has to be placed before messing with the animation list
	SelectAnimation( NULL );

	for ( wxTreeItemId item : selections )
	{
		CName animName( m_animsList->GetItemText( item ) );
		m_selectedAnimSet->RemoveAnimation( m_selectedAnimSet->FindAnimation( animName ) );
	}

	RefreshPage();
}

void CEdAnimBrowser::OnRemoveAnimationFromAnimList( wxCommandEvent &event )
{
	ASSERT( m_selectedAnimSet != NULL );

	wxArrayInt selections;
	m_animSetAnimationList->GetSelections( selections );

	// has to be placed before messing with the animation list
	SelectAnimation( NULL );

	for ( int item : selections )
	{
		CName animName( m_animSetAnimationList->GetString( item ) );
		m_selectedAnimSet->RemoveAnimation( m_selectedAnimSet->FindAnimation( animName ) );
	}

	RefreshPage();
}

void CEdAnimBrowser::OnSaveAnimation( wxCommandEvent &event )
{	
	if (m_mode == ABM_Normal_Animset || ABM_Normal_Anim) 
	{
		CSkeletalAnimationSet* set = SafeCast< CSkeletalAnimationSet >( m_resource );
		OnSaveAnimset( set );

		m_resource->Save();
	}
	else
	{
		TSkeletalAnimationSetsArray animSets;
		GetAnimsets(animSets);

		for( auto it = animSets.Begin(), end = animSets.End(); it != end; ++it )
		{
			if ( ( *it )->GetFile()->IsModified() )
			{
				( *it )->Save();
			}
		}

		CResource* selectedAnimSet = m_timeline->GetSelectedResource();
		if( selectedAnimSet != NULL )
		{
			selectedAnimSet->Save();
		}
	}

	if( m_mode == ABM_Normal_Animset )
	{
		CSkeletalAnimationSet* animSet = Cast< CSkeletalAnimationSet >( m_resource );
		ASSERT( animSet != NULL );

		// Save all external files if modified
		TDynArray< THandle< CExtAnimEventsFile > >& files = animSet->GetEventsFiles();
		for( auto fileIter = files.Begin(); fileIter != files.End(); ++fileIter )
		{
			CExtAnimEventsFile* file = ( *fileIter ).Get();
			if( file && file->IsModified() )
			{
				file->Save();
			}
		}
	}
}

void CEdAnimBrowser::OnRestoreAnimation( wxCommandEvent &event )
{
	CSkeletalAnimationSet* set = nullptr;

	if (m_mode == ABM_Normal_Animset || ABM_Normal_Anim) 
	{
		set = SafeCast< CSkeletalAnimationSet >( m_resource );
	}
	else
	{
		CResource* selectedAnimSet = m_timeline->GetSelectedResource();
		set = SafeCast< CSkeletalAnimationSet >( selectedAnimSet );
	}

	if ( set )
	{
		TDynArray< CSkeletalAnimationSetEntry* > animationEntries;
		set->GetAnimations( animationEntries );
		GFeedback->BeginTask( TXT( "Restoring animations..." ), false );
		for ( Uint32 i = 0; i < animationEntries.Size(); ++i )
		{
			GFeedback->UpdateTaskProgress( i, animationEntries.Size() );
			CSkeletalAnimation* anim = animationEntries[i]->GetAnimation();
			if ( anim )
			{
				anim->RestoreAnimation();
			}
		}
		GFeedback->EndTask();
	}
}

void CEdAnimBrowser::OnMenuUndo( wxCommandEvent& event )
{
	if ( m_playedAnimation )
	{
		m_playedAnimation->Pause();
		m_playedAnimation->SetTime( 0 );
	}

	m_undoManager->Undo();
}

void CEdAnimBrowser::OnMenuRedo( wxCommandEvent& event )
{
	if ( m_playedAnimation )
	{
		m_playedAnimation->Pause();
		m_playedAnimation->SetTime( 0 );
	}

	m_undoManager->Redo();
}

void CEdAnimBrowser::OnMenuReloadEntity( wxCommandEvent& event )
{
	OnReloadEntity();
}

void CEdAnimBrowser::OnReloadEntity()
{
	if ( m_entityTemplate && m_selectedAnimation )
	{
		LoadEntity( m_entityTemplate->GetFile()->GetDepotPath(), m_selectedAnimation->GetName().AsString() );
	}
}

/*void CEdAnimBrowser::OnLoopOverflow( Float deltaTime, Uint32 overflows )
{
	if ( IsLoopingEnabled() )
	{
		// do nothing
	}
	else
	{
		// pause animation
		m_playedAnimation->Pause();
		m_playedAnimation->SetTime( 0.0f );
		UpdatePlayPauseIcon();
	}
}

void CEdAnimBrowser::OnLoopUnderflow( Float deltaTime, Uint32 underflows )
{
	if ( IsLoopingEnabled() )
	{
		// do nothing
	}
	else
	{
		m_playedAnimation->Pause();
		m_playedAnimation->SetTime( 0.0f );
		UpdatePlayPauseIcon();
	}
}*/

void CEdAnimBrowser::OnAnimationFinished( const CPlayedSkeletalAnimation* animation )
{
	ASSERT( m_playedAnimation->m_playedAnimation == animation );

	if ( IsLoopingEnabled() )
	{
		// do nothing
	}
	else
	{
		// pause animation
		m_playedAnimation->Pause();
		m_playedAnimation->SetTime( 0.0f );
		RefreshAnimationForCustomPages();

		m_refershIconsRequest.Increment();
	}
}

void CEdAnimBrowser::OnToggleBBox( wxCommandEvent& event )
{
	m_previewPanel->ToggleBBox();
}

CGatheredResource resAnimFloor( TXT("engine\\meshes\\editor\\plane.w2mesh"), 0 );

void CEdAnimBrowser::OnToggleFloor( wxCommandEvent& event )
{
	wxToolBar* toolbar = XRCCTRL( *this, "playbackControlToolbar", wxToolBar );
	m_showFloor = toolbar->GetToolState( XRCID("floorButton") );
	if ( m_showFloor )
	{
		if ( !m_floor )
		{
			m_floor = m_previewPanel->GetWorld()->GetDynamicLayer()->CreateEntitySync( EntitySpawnInfo() );

			CMeshComponent* component = Cast< CMeshComponent >( m_floor->CreateComponent( ClassID< CMeshComponent >(), SComponentSpawnInfo() ) );
			CMesh* mesh = resAnimFloor.LoadAndGet< CMesh >();
			component->SetResource( mesh );
		}

		if ( m_floor )
		{
			m_floor->SetHideInGame( false );

			ReselectCurrentAnimation();
		}
	}
	else
	{
		if ( m_floor )
		{
			m_floor->SetHideInGame( true );
		}
	}
}

void CEdAnimBrowser::OnTogglePlayerCameraMode( wxCommandEvent& event )
{
	wxToolBar* toolbar = XRCCTRL( *this, "playbackControlToolbar", wxToolBar );
	m_previewPanel->SetUsePlayerCamera( toolbar->GetToolState( XRCID("cameraButton") ) );
}

void CEdAnimBrowser::OnTogglePlayerMotionTrack( wxCommandEvent& event )
{
	wxToolBar* toolbar = XRCCTRL( *this, "playbackControlToolbar", wxToolBar );

	UseMotionExtraction( toolbar->GetToolState( XRCID("movementButton") ) );
}

void CEdAnimBrowser::OnToggleExtractTrajectory( wxCommandEvent& event )
{
	wxToolBar* toolbar = XRCCTRL( *this, "playbackControlToolbar", wxToolBar );

	UseTrajectoryExtraction( toolbar->GetToolState( XRCID("exTrajButton") ) );
}

void CEdAnimBrowser::OnDisplayItem( wxCommandEvent& event )
{
	if ( !m_itemDialog )
	{
		m_itemDialog = new CEdDisplayItemDialog( this, this );
	}
	
	m_itemDialog->Show();
}

void CEdAnimBrowser::OnDisplayItemUndo( wxCommandEvent& event )
{
	UndoItemDisplay();
}

void CEdAnimBrowser::OnCompressedPose( wxCommandEvent& event )
{
	m_forceCompressedPose = event.IsChecked();
}

void CEdAnimBrowser::RefreshCompressedPoseWidgets()
{
	FillCompressedPoseChoice();

	Int32 selIndex = 0;
	while ( CSkeletalAnimationSetEntry* sel = GetSelectedAnimationEntry( selIndex ++ ) )
	{
		if ( sel && sel->GetAnimation() )
		{
			CSkeletalAnimation* anim = sel->GetAnimation();
			const CName& name = anim->GetCompressedPoseName();
		
			wxChoice* choice = XRCCTRL( *this, "choicePoseComprName", wxChoice );
			VERIFY( choice->SetStringSelection( name.AsString().AsChar() ) );

			wxTextCtrl* edit = XRCCTRL( *this, "editPoseComprInfo", wxTextCtrl );

			CSkeletalAnimationSet* set = sel->GetAnimSet();
			const SCompressedPoseInfo* info = set->FindCompressedPoseInfo( name );
			if ( info )
			{
				wxString str = wxString::Format( wxT("%1.1fs, %s"), info->m_time, info->m_animation.AsString().AsChar() );
				edit->SetValue( str );
			}
			else
			{
				edit->SetValue( wxT("<none>") );
			}
		}
	}
}

void CEdAnimBrowser::FillCompressedPoseChoice()
{
	wxChoice* choice = XRCCTRL( *this, "choicePoseComprName", wxChoice );
	choice->Freeze();
	choice->Clear();
	choice->AppendString( CName::NONE.AsString().AsChar() );

	Int32 selIndex = 0;
	while ( CSkeletalAnimationSetEntry* sel = GetSelectedAnimationEntry( selIndex ++ ) )
	{
		if ( sel )
		{
			CSkeletalAnimationSet* set = sel->GetAnimSet();
			if ( set )
			{
				Uint32 size = set->GetNumCompressedPoses();
				for ( Uint32 i=0; i<size; ++i )
				{
					const CName& name = set->FindCompressedPoseName( i );
					choice->AppendString( name.AsString().AsChar() );
				}
			}
		}
	}

	choice->Thaw();
}

void CEdAnimBrowser::SelectCompressedPose( const String& poseNameStr )
{
	wxChoice* choice = XRCCTRL( *this, "choicePoseComprName", wxChoice );
	VERIFY( choice->SetStringSelection( poseNameStr.AsChar() ) );

	wxTextCtrl* edit = XRCCTRL( *this, "editPoseComprInfo", wxTextCtrl );
	edit->SetValue( poseNameStr.AsChar() );

	Int32 selIndex = 0;
	while ( CSkeletalAnimationSetEntry* animEntry = GetSelectedAnimationEntry( selIndex ++ ) )
	{
		if ( animEntry && animEntry->GetAnimation() )
		{
			CSkeletalAnimation* anim = animEntry->GetAnimation();
			CName poseName( poseNameStr );
			anim->SetCompressedPoseName( poseName );

			CSkeletalAnimationSet* set = animEntry->GetAnimSet();
			const SCompressedPoseInfo* info = set->FindCompressedPoseInfo( poseName );
			if ( info )
			{
				wxString str = wxString::Format( wxT("%1.1fs, %s"), info->m_time, info->m_animation.AsString().AsChar() );
				edit->SetValue( str );
			}
			else
			{
				ASSERT( info );
				edit->SetValue( wxT("<none>") );
			}

			RecreateCloneWithCompressedPose();
		}
	}
}

void CEdAnimBrowser::DeselectCompressedPose()
{
	// Write <none>
	wxChoice* choice = XRCCTRL( *this, "choicePoseComprName", wxChoice );
	
	choice->Freeze();
	choice->Clear();
	choice->AppendString( CName::NONE.AsString().AsChar() );
	choice->Thaw();
	choice->SetSelection( 0 );

	wxTextCtrl* edit = XRCCTRL( *this, "editPoseComprInfo", wxTextCtrl );
	edit->SetValue( wxT("<none>") );

	Int32 selIndex = 0;
	while ( CSkeletalAnimationSetEntry* animEntry = GetSelectedAnimationEntry( selIndex ++ ) )
	{
		if ( animEntry && animEntry->GetAnimation() )
		{
			CSkeletalAnimation* anim = animEntry->GetAnimation();
			anim->SetCompressedPoseName( CName::NONE );
		}
	}
}

String CEdAnimBrowser::GetSelectedCompressedPose() const
{
	wxChoice* choice = XRCCTRL( *this, "choicePoseComprName", wxChoice );
	String temp = choice->GetStringSelection().wx_str();
	return temp;
}

void CEdAnimBrowser::OnSelectCompressedPose( wxCommandEvent& event )
{
	wxChoice* choice = XRCCTRL( *this, "choicePoseComprName", wxChoice );
	String temp = choice->GetStringSelection().wx_str();
	SelectCompressedPose( temp );
	RefreshPage();
}

void CEdAnimBrowser::RefreshSecondToolbar()
{
	wxCommandEvent fake;
	OnEditCompressedPose( fake );
}

void CEdAnimBrowser::OnEditCompressedPose( wxCommandEvent& event )
{
	wxToolBar* tb = XRCCTRL( *this, "secondControlToolbar", wxToolBar );
	Bool state = tb->GetToolState( XRCID( "poseComprEdit" ) );
	
	{
		wxChoice* choice = XRCCTRL( *this, "choicePoseComprName", wxChoice );
		choice->Enable( state );
		tb->EnableTool( XRCID( "poseComprAdd" ), state );
		tb->EnableTool( XRCID( "poseComprRemove" ), state );
		tb->EnableTool( XRCID( "poseComprPrev" ), state );
		tb->EnableTool( XRCID( "poseComprNext" ), state );
	}
}

void CEdAnimBrowser::OnAddCompressedPose( wxCommandEvent& event )
{
	String val;

	if ( !m_playedAnimation )
	{
		wxMessageBox( wxT("Select animation"), wxT("AnimBrowser") );
		return;
	}

	CSkeletalAnimationSetEntry* selEntry = GetSelectedAnimationEntry();
	if ( selEntry )
	{
		if ( selEntry->GetName() != m_playedAnimation->GetName() )
		{
			ASSERT( selEntry->GetName() != m_playedAnimation->GetName()  );
			return;
		}
	}
	else
	{
		ASSERT( selEntry );
		return;
	}

	CSkeletalAnimationSet* set = selEntry->GetAnimSet();
	ASSERT( set );

	const CSkeleton* skeleton = set->GetSkeleton();
	if ( !skeleton )
	{
		wxMessageBox( wxT("Animset don't have skeleton"), wxT("AnimBrowser") );
		return;
	}

	if ( !skeleton->GetPoseCompression() )
	{
		wxString msg = wxString::Format( wxT("Skeleton's pose compression is empty. \nPlease open resource file \n'%s'\nand select pose compression property"), skeleton->GetDepotPath().AsChar() );
		wxMessageBox( msg, wxT("AnimBrowser") );
		return;
	}

	const CName& animName = m_playedAnimation->GetName();
	const Float animTime = m_playedAnimation->GetTime();

	String msg = String::Printf( TXT("Write new pose name. Animation: '%s', Time: '%1.1f'"), animName.AsString().AsChar(), animTime );

	if ( InputBox( this, TXT("AnimBrowser"), msg, val ) )
	{
		if ( !val.Empty() )
		{
			CName poseName( val );

			if ( set->FindCompressedPoseHandle( poseName ) != -1 )
			{
				wxString msg = wxString::Format( wxT("Pose name '%s' already exists"), poseName.AsString().AsChar() );
				wxMessageBox( msg, wxT("AnimBrowser") );
				return;
			}

			if ( set->AddCompressedPose( poseName, animName, animTime ) )
			{
				RefreshCompressedPoseWidgets();
				SelectCompressedPose( poseName.AsString() );
				RefreshPage();
			}
			else
			{
				wxMessageBox( wxT("Fail!"), wxT("AnimBrowser") );
			}
		}
	}
}

void CEdAnimBrowser::OnRemoveCompressedPose( wxCommandEvent& event )
{
	String poseName = GetSelectedCompressedPose();
	if ( poseName == CName::NONE.AsString().AsChar() )
	{
		wxMessageBox( wxT("This is default pose"), wxT("AnimBrowser") );
		return;
	}

	if ( !YesNo( TXT("Sure to delete '%s' pose?"), poseName.AsChar() ) )
	{
		return;
	}


	CSkeletalAnimationSetEntry* selEntry = GetSelectedAnimationEntry();
	if ( selEntry && selEntry->GetAnimation() )
	{
		CSkeletalAnimation* anim = selEntry->GetAnimation();
		anim->SetCompressedPoseName( CName::NONE );

		CSkeletalAnimationSet* set = selEntry->GetAnimSet();
		ASSERT( set );

		if ( !set->RemoveCompressedPose( CName( poseName ) ) )
		{
			wxMessageBox( wxT("Fail!"), wxT("AnimBrowser") );
		}

		RefreshCompressedPoseWidgets();
		RefreshPage();
	}
}

void CEdAnimBrowser::OnNextCompressedPose( wxCommandEvent& event )
{
	CSkeletalAnimationSetEntry* selEntry = GetSelectedAnimationEntry();
	if ( selEntry )
	{
		CSkeletalAnimationSet* set = selEntry->GetAnimSet();
		if ( set )
		{
			Int32 size = (Int32)set->GetNumCompressedPoses();

			String currPoseName = GetSelectedCompressedPose();
			if ( !currPoseName.Empty() )
			{
				CompressedPoseHandle curr = set->FindCompressedPoseHandle( CName( currPoseName ) );
				if ( curr != -1 )
				{
					if ( curr + 1 < size )
					{
						curr += 1;
						
						const CName& newPoseName = set->FindCompressedPoseName( curr );

						SelectCompressedPose( newPoseName.AsString() );
						RefreshPage();

						return;
					}
				}
			}

			// Select first
			if ( size > 0 )
			{
				const CName& newPoseName = set->FindCompressedPoseName( size - 1 );
				SelectCompressedPose( newPoseName.AsString() );
				RefreshPage();
			}
			else
			{
				DeselectCompressedPose();
				RefreshPage();
			}
		}
	}
}

void CEdAnimBrowser::OnPrevCompressedPose( wxCommandEvent& event )
{
	CSkeletalAnimationSetEntry* selEntry = GetSelectedAnimationEntry();
	if ( selEntry )
	{
		CSkeletalAnimationSet* set = selEntry->GetAnimSet();
		if ( set )
		{
			Int32 size = (Int32)set->GetNumCompressedPoses();

			String currPoseName = GetSelectedCompressedPose();
			if ( !currPoseName.Empty() )
			{
				CompressedPoseHandle curr = set->FindCompressedPoseHandle( CName( currPoseName ) );
				if ( curr != -1 )
				{
					if ( curr - 1 > 0 && curr - 1 < size )
					{
						curr -= 1;

						const CName& newPoseName = set->FindCompressedPoseName( curr );

						SelectCompressedPose( newPoseName.AsString() );
						RefreshPage();

						return;
					}
				}
			}

			// Select first
			if ( size > 0 )
			{
				const CName& newPoseName = set->FindCompressedPoseName( 0 );
				SelectCompressedPose( newPoseName.AsString() );
				RefreshPage();
			}
			else
			{
				DeselectCompressedPose();
				RefreshPage();
			}
		}
	}
}

void CEdAnimBrowser::OnCloneWithCompressedPose( wxCommandEvent& event )
{
	RecreateCloneWithCompressedPose();
}

void CEdAnimBrowser::OnCloneFlipPose( wxCommandEvent& event )
{
	Float posFromAss = 10000.f;

	if ( m_entity->GetPosition().Mag3() > posFromAss )
	{
		m_entity->SetPosition( Vector::ZERO_3D_POINT );
		m_previewPanel->SetOffsetForClone( CLONE_COMPRESSED_POSE, posFromAss );
	}
	else
	{
		m_entity->SetPosition( Vector( posFromAss, posFromAss, posFromAss ) );
		OnCloneWithCompressedPoseOffset( event );
	}
}

void CEdAnimBrowser::OnCloneWithCompressedPoseColor( wxCommandEvent& event )
{
	if ( m_previewPanel->HasClone( CLONE_COMPRESSED_POSE ) )
	{
		CEdEntityClone::ECloneColor color;

		wxChoice* choice = XRCCTRL( *this, "comprPoseCloneColor", wxChoice );
		Int32 sel = choice->GetSelection();
		if ( sel != -1 )
		{
			switch( sel )
			{
			case 0: color = CEdEntityClone::CC_Original; break;
			case 1: color = CEdEntityClone::CC_Default; break;
			case 2: color = CEdEntityClone::CC_Red; break;
			case 3: color = CEdEntityClone::CC_Green; break;
			}

			m_previewPanel->SetCloneColor( CLONE_COMPRESSED_POSE, color );
		}
	}
}

void CEdAnimBrowser::OnCloneWithCompressedPoseOffset( wxCommandEvent& event )
{
	if ( m_previewPanel->HasClone( CLONE_COMPRESSED_POSE ) )
	{
		wxSlider* slider = XRCCTRL( *this, "comprPoseCloneSlider", wxSlider );
		m_previewPanel->SetOffsetForClone( CLONE_COMPRESSED_POSE, -(Float)slider->GetValue() / 50.f );
	}
}

namespace
{
	Color RandColor()
	{
		static Uint32 colorMask = 1;
		colorMask = (colorMask + 1) % 7;	
		if ( !colorMask ) colorMask = 1;

		Color color = Color( colorMask & 1 ? 255 : 0, 
			colorMask & 2 ? 255 : 0,
			colorMask & 4 ? 255 : 0 );

		if ( color == Color( 0, 0, 255 ) )
		{
			color = Color( 128, 128, 255 );
		}

		return color;
	}
}

Bool CEdAnimBrowser::CreateDebugPose( Float time, const CSkeletalAnimationSetEntry* anim, CEdAnimBrowser::DebugPose& outPose ) const
{
	const CSkeleton* sk( nullptr );
	Int32 trajectoryBone = 1;

	if ( m_animatedComponent )
	{
		sk = m_animatedComponent->GetSkeleton();
		trajectoryBone = m_animatedComponent->GetTrajectoryBone();
	}
	else if ( const CSkeletalAnimationSet* set = anim->GetAnimSet() )
	{
		sk = set->GetSkeleton();
	}

	if ( sk )
	{
		const Uint32 numBones = sk->GetBonesNum();
		const Uint32 numTracks = sk->GetTracksNum();
		SBehaviorGraphOutput pose;
		pose.Init( numBones, numTracks );

		if ( anim->GetAnimation()->Sample( time, pose.m_numBones, pose.m_numFloatTracks, pose.m_outputPose, pose.m_floatTracks ) )
		{
			outPose.m_skeleton = sk;
			outPose.m_color = RandColor();
			outPose.m_name = String::Printf( TXT("%s - %1.2f"), anim->GetName().AsChar(), time );
			outPose.m_id = anim;

			pose.ExtractTrajectory( sk, trajectoryBone );

			outPose.m_pose.Resize( numBones );
			pose.GetBonesModelSpace( sk, outPose.m_pose );

			return true;
		}
	}

	return false;
}

void CEdAnimBrowser::OnAddDebugPoseStart( wxCommandEvent& event )
{
	if ( const CSkeletalAnimationSetEntry* anim = GetSelectedAnimationEntry() )
	{
		const Float time = 0.f;

		DebugPose pose;
		if ( CreateDebugPose( time, anim, pose ) )
		{
			m_debugPoses.PushBack( pose );
		}
	}
}

void CEdAnimBrowser::OnAddDebugPoseEnd( wxCommandEvent& event )
{
	if ( const CSkeletalAnimationSetEntry* anim = GetSelectedAnimationEntry() )
	{
		const Float time = anim->GetDuration();

		DebugPose pose;
		if ( CreateDebugPose( time, anim, pose ) )
		{
			m_debugPoses.PushBack( pose );
		}
	}
}

void CEdAnimBrowser::OnAddDebugPoseTime( wxCommandEvent& event )
{
	if ( const CSkeletalAnimationSetEntry* anim = GetSelectedAnimationEntry() )
	{
		String valStr( TXT("0.0") );
		if ( InputBox( this, TXT("Anim Browser Poses"), TXT("Animation time:"), valStr ) )
		{
			Float time( 0.f );
			if ( FromString( valStr, time ) )
			{
				DebugPose pose;
				if ( CreateDebugPose( time, anim, pose ) )
				{
					m_debugPoses.PushBack( pose );
				}
			}
		}
	}
}

void CEdAnimBrowser::OnRemoveDebugPose( wxCommandEvent& event )
{
	if ( const CSkeletalAnimationSetEntry* anim = GetSelectedAnimationEntry() )
	{
		const Int32 numPoses = m_debugPoses.SizeInt();
		for ( Int32 i=numPoses-1; i>=0; --i )
		{
			if ( m_debugPoses[ i ].m_id == anim )
			{
				m_debugPoses.RemoveAtFast( i );
			}
		}
	}
}

void CEdAnimBrowser::RecreateCloneWithCompressedPose()
{
	if ( !m_animatedComponent || !m_playedAnimation || !m_playedAnimation->m_playedAnimation )
	{
		return;
	}

	wxToolBar* tb = XRCCTRL( *this, "secondControlToolbar", wxToolBar );
	Bool shouldCreateClone = tb->GetToolState( XRCID( "comprPoseClone" ) );

	Bool hasClone = m_previewPanel->HasClone( CLONE_COMPRESSED_POSE );

	if ( shouldCreateClone && !hasClone )
	{
		if ( !m_previewPanel->CreateClone( CLONE_COMPRESSED_POSE, m_animatedComponent->GetEntity() ) )
		{
			static Bool msgType = false;

			if ( msgType )
			{
				wxMessageBox( wxT("Not enough crystals") );
			}
			else
			{
				wxMessageBox( wxT("Not enough vespene gas") );
			}

			msgType = !msgType;
		}

		m_previewPanel->SyncCloneTo( CLONE_COMPRESSED_POSE, m_playedAnimation->m_playedAnimation, true );
	}
	else if ( shouldCreateClone && hasClone && m_playedAnimation->m_playedAnimation )
	{
		m_previewPanel->SyncCloneTo( CLONE_COMPRESSED_POSE, m_playedAnimation->m_playedAnimation, true );
	}
	else if ( !shouldCreateClone && hasClone )
	{
		m_previewPanel->DestroyClone( CLONE_COMPRESSED_POSE );
	}
}

void CEdAnimBrowser::OnMotionExTypeChanged( wxCommandEvent& event )
{
	if ( event.GetId() == XRCID( "exCompressed" ) )
	{
		m_motionExType = event.IsChecked();
	}
	else
	{
		m_motionExType = !event.IsChecked();
	}

	UpdateMotionExTypeIcon();
	UpdateMotionExTypeForAnim();
}

void CEdAnimBrowser::OnMimicCor( wxCommandEvent& event )
{
	if ( m_animatedComponent )
	{
		if ( IActorInterface* a = m_animatedComponent->GetEntity()->QueryActorInterface() )
		{
			if ( a->HasMimic() )
			{
				a->MimicOff();
			}
			else
			{
				a->MimicOn();
			}
		}
	}
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

void CEdAnimBrowser::OnTPoseCor( wxCommandEvent& event )
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

CDirectory* CEdAnimBrowser::FindParentDir(CDirectory* childDir, const String& parentDirName)
{
	ASSERT( childDir );

	Uint32 depth = 0;
	CDirectory* dir = childDir;

	if (dir->GetName()==parentDirName) return dir;

	while (depth<MAX_DIR_SEARCH_DEPTH)
	{
		if (dir->GetParent())
		{
			dir = dir->GetParent();

			if (dir->GetName()==parentDirName)
			{
				return dir;
			}

			depth++;
		}
		else
		{
			break;
		}
	}

	return NULL;
}

Bool CEdAnimBrowser::FindEntity(String &entName, const String &filePlace)
{
	TDynArray<String> files;
	files.PushBack( DEFAULT_ENTITY_NAME );

	CResource* entResource = ClassID< CEntityTemplate >()->GetDefaultObject<CResource>();
	String entExtension = String( entResource->GetExtension() ).ToLower();

	GFileManager->FindFiles( filePlace , TXT("*.")+entExtension, files, false);

	if (files.Empty())
	{
		return false;
	}
	else
	{
		for (Uint32 i=0; i<files.Size(); i++)
		{
			GDepot->ConvertToLocalPath(files[i], files[i]);
		}
		entName = InputComboBox(NULL, TXT("Select entity"), TXT("Select entity for animation"), DEFAULT_ENTITY_NAME, files);
		return true;
	}
}

void CEdAnimBrowser::AddAnimations( TDynArray<CSkeletalAnimation*> anims )
{
	// Only for animset mode
	if (m_mode!=ABM_Normal_Anim && m_selectedAnimSet && !anims.Empty())
	{
		Int32 changeAll = -1;

		for (Uint32 i=0; i<anims.Size(); i++)
		{
			CSkeletalAnimationSetEntry* animEntry = m_selectedAnimSet->FindAnimation(anims[i]->GetName());

			if (!animEntry)
			{
				// Add
				m_selectedAnimSet->AddAnimation(anims[i]);
			}
			else
			{
				if (changeAll == -1)
				{
					// Ask for all
					TDynArray<String> options;
					options.PushBack(TXT("Overide"));
					options.PushBack(TXT("Override all"));
					options.PushBack(TXT("Add"));
					options.PushBack(TXT("Add all"));

					wxString message = wxString::Format(wxT("Do you want to override animation %s?"), anims[i]->GetName().AsString().AsChar());
		
					CEdMultiChoiceDlg dlg(this, wxT("Question"), message, options);
					Int32 choice = dlg.ShowModal();

					if (choice == 1) changeAll = 1;
					else if (choice == 3) changeAll = 2;
					else changeAll = -1;

					if (choice == 0 || choice == 1) 
						m_selectedAnimSet->ChangeAnimation(animEntry->GetAnimation(), anims[i]);
					else
						m_selectedAnimSet->AddAnimation(anims[i]);

				}
				else if (changeAll == 1)
				{
					// Do not ask
					m_selectedAnimSet->ChangeAnimation(animEntry->GetAnimation(), anims[i]);
				}
				else if (changeAll == 2)
				{
					// Add
					m_selectedAnimSet->AddAnimation(anims[i]);
				}
			}
		}
		
		RefreshPage();
	}
}

void CEdAnimBrowser::DispatchEditorEvent( const CName& name, IEdEventData* data )
{
	if ( name == CNAME( FileReloadConfirm ) )
	{
		CResource* res = GetEventData< CResource* >( data );
		if ( res == m_resource )
		{
			SEvents::GetInstance().QueueEvent( CNAME( FileReloadToConfirm ), CreateEventData( CReloadFileInfo( res, NULL, GetTitle().wc_str() ) ) );
		}
	}
	else if ( name == CNAME( FileReload ) )
	{
		const CReloadFileInfo& reloadInfo = GetEventData< CReloadFileInfo >( data );
		if ( reloadInfo.m_newResource->IsA< CEntityTemplate >() )
		{
			CEntityTemplate* oldTemplate = (CEntityTemplate*)( reloadInfo.m_oldResource );
			CEntityTemplate* newTemplate = (CEntityTemplate*)( reloadInfo.m_newResource );
			if ( oldTemplate == m_entityTemplate )
			{
				LoadEntity( newTemplate->GetFile()->GetDepotPath(), m_selectedAnimation->GetAnimation()->GetName().AsString() );
				SelectAnimation( m_selectedAnimation->GetAnimation()->GetName().AsString() );

				wxTheFrame->GetAssetBrowser()->OnEditorReload( m_resource, this );
			}
		}
		else if (reloadInfo.m_newResource->IsA<CSkeletalAnimationSet>())
		{
			if (!m_mode == ABM_Normal_Animset) return;

			CSkeletalAnimationSet* oldSet = (CSkeletalAnimationSet*)(reloadInfo.m_oldResource);
			CSkeletalAnimationSet* newSet = (CSkeletalAnimationSet*)(reloadInfo.m_newResource);

			ASSERT(m_selectedAnimSet != newSet );

			// Update
			if (oldSet == m_selectedAnimSet)
			{
				m_selectedAnimSet = newSet;

				m_resource = m_selectedAnimSet;
	
				String animName;
				CSkeletalAnimationSetEntry* anim = m_selectedAnimSet->FindAnimation( m_selectedAnimation->GetAnimation()->GetName() );
				if (!anim && m_selectedAnimSet->GetNumAnimations())
				{
					TDynArray< CSkeletalAnimationSetEntry* > anims;
					//m_selectedAnimSet->GetAnimations(anims);
					GetAnimationsFromAnimset(m_selectedAnimSet, anims);

					for (Uint32 i=0; i<m_selectedAnimSet->GetNumAnimations(); i++)
					{
						if (anims[i]) 
						{
							animName = anims[i]->GetAnimation()->GetName().AsString();
							break;
						}
					}

					ASSERT(animName!=String::EMPTY && TXT("Animset is empty or damaged"));
				}
				else
					animName = anim->GetAnimation()->GetName().AsString();

				LoadEntity( m_entityTemplate->GetFile()->GetDepotPath(), animName);
				SelectAnimation( animName );

				wxTheFrame->GetAssetBrowser()->OnEditorReload(m_resource, this);
			}
		}

		RefreshPage();
	}
	else if ( name == TXT("EditorPropertyPostChange") )
	{
		const CEdPropertiesPage::SPropertyEventData& eventData = GetEventData< CEdPropertiesPage::SPropertyEventData >( data );
		if ( eventData.m_page == m_animProperties )
		{
			RefreshPage();
		}
	}
}

void CEdAnimBrowser::OnTakeScreenshot( wxCommandEvent& event )
{
	if( m_playedAnimation->m_playedAnimation )
	{
		m_previewPanel->TakeScreenshotsFromAnim( m_playedAnimation->m_playedAnimation );
	}
}

void CEdAnimBrowser::OnCharHook( wxKeyEvent &event )
{
	switch(event.GetKeyCode())
	{
	case 'R':
		if( event.GetEventObject()->GetClassInfo()->IsKindOf( wxCLASSINFO( wxTextCtrl ) ) == false )
		{
			if (event.ControlDown())
			{
				// on SPACE play/pause animation
				OnReloadEntity();
				return;
			}
		}
		break;
	case WXK_SPACE:
		if( event.GetEventObject()->GetClassInfo()->IsKindOf( wxCLASSINFO( wxTextCtrl ) ) == false )
		{
			// on SPACE play/pause animation
			PlayPauseAnimation();
			return;
		}
		break;
	case WXK_HOME:
		// on Ctrl+HOME reset the animation
		if ( event.CmdDown() )
		{
			ResetAnimation();
			return;
		}
		break;
	case WXK_END:
		// on Ctrl+END go to the animation end
		if ( event.CmdDown() )
		{
			ResetAnimation( true );
			return;
		}
		break;
	}
	event.Skip();
}

void CEdAnimBrowser::OnGenerateBoundingBox( wxCommandEvent& event )
{
	if ( m_selectedAnimation && m_selectedAnimation->GetAnimation() )
	{
		GFeedback->BeginTask( TXT("Generate bounding box"), false );

		m_selectedAnimation->GetAnimation()->GenerateBoundingBox( m_animatedComponent );

		GFeedback->EndTask();
	}
}

void CEdAnimBrowser::OnGenerateBoundingBoxForAll( wxCommandEvent& event )
{
	// Get the target skeletal animation set
	CSkeletalAnimationSet* animSet = GetSelectedAnimSet();
	if ( animSet )
	{
		TDynArray< CSkeletalAnimationSetEntry* > animations;
		animSet->GetAnimations( animations );

		if ( animations.Size() == 0 )
		{
			return;
		}

		GFeedback->BeginTask( TXT("Generate bounding box for animations"), true );

		for ( Uint32 i=0; i<animations.Size(); ++i )
		{
			// Cancel task
			if ( GFeedback->IsTaskCanceled() )
			{
				break;
			}

			GFeedback->UpdateTaskInfo( TXT("Animation '%s'..."), animations[i]->GetName().AsString().AsChar() );
			GFeedback->UpdateTaskProgress( i, animations.Size() );

			if ( animations[i]->GetAnimation() )
			{
				animations[i]->GetAnimation()->GenerateBoundingBox( m_animatedComponent );
			}
		}

		GFeedback->EndTask();		
	}

	RefreshPage();
}

void CEdAnimBrowser::OnTimeMultiplierChanged( wxScrollEvent& event )
{
	wxSlider* slider = XRCCTRL( *this, "speedMulitplier", wxSlider );

	Int32 value = slider->GetValue();
	Int32 minVal = slider->GetMin();
	Int32 maxVal = slider->GetMax();

	Float range = (Float)( maxVal - minVal );
	if ( range <= 0.0f )
		range = 1.0f;

	m_previewPanel->SetTimeMultiplier( (Float)value / range );
}

void CEdAnimBrowser::OnTimelineResized( wxCommandEvent& event )
{
	/* Little hack for refreshing window layout.
	After 4 hours of strumbling, could not make this better */

	static Int32 smallAdder = 1;
	smallAdder *= -1;
	m_timelinePanel->SetSize( m_timelinePanel->GetSize().GetX(),
		m_timelinePanel->GetSize().GetY() + smallAdder );
}

CActor* CEdAnimBrowser::GetActorEntity()
{
	if ( !m_entity )
	{
		return NULL;
	}
	CActor* actor = Cast< CActor >( m_entity );
	if ( !actor )
	{
		return NULL;
	}

	return actor;
}

namespace
{
	CExtAnimEvent* FindEventByName( CSkeletalAnimationSetEntry* anim, CName eventName )
	{
		TDynArray< CExtAnimEvent* > temp;
		anim->GetAllEvents( temp );

		for ( Uint32 i=0; i<temp.Size(); ++i )
		{
			if ( temp[ i ] && temp[ i ]->GetEventName() == eventName )
			{
				return temp[ i ];
			}
		}
		return NULL;
	}

	void SyncEventsTo( CSkeletalAnimationSetEntry* animWithEvtsToSync, CExtAnimEvent* evt )
	{

	}
}

void CEdAnimBrowser::OnSyncEventsTo( wxCommandEvent& event )
{
	TDynArray< CSkeletalAnimationSetEntry* > animsToChange;
	TDynArray< String > animsToChangeNames;

	if ( m_selectedAnimation )
	{
		String selAnimName = m_selectedAnimation->GetName().AsString();

		static String animTemplateName = selAnimName.StringBefore( TXT("_"), true );
		if ( !InputBox( this, TXT("Animation prefix name template"), TXT("Write animation prefix name tamplate. All animations with this prefix will be found."), animTemplateName, false ) )
		{
			return;
		}

		CSkeletalAnimationSet* set = m_selectedAnimation->GetAnimSet();
		if ( set )
		{
			const TDynArray< CSkeletalAnimationSetEntry* >& anims = set->GetAnimations();

			const Uint32 size = anims.Size();
			for ( Uint32 i=0; i<size; ++i )
			{
				if ( anims[ i ] )
				{
					String animName = anims[ i ]->GetName().AsString();

					if ( animName.BeginsWith( animTemplateName ) )
					{
						animsToChange.PushBack( anims[ i ] );
						animsToChangeNames.PushBack( animName );
					}
				}
			}
		}
	}

	if ( animsToChange.Size() > 0 )
	{
		static Bool fistTime = true;

		static TDynArray< Bool > animToChangeAnswers( animsToChangeNames.Size() );
		if ( fistTime )
		{
			for ( Uint32 j=0; j<animToChangeAnswers.Size(); ++j )
			{
				animToChangeAnswers[ j ] = true;
			}
			fistTime = false;
		}

		animToChangeAnswers.Resize( animsToChangeNames.Size() );

		if ( MultiBoolDialog( this, TXT("Select animations"), animsToChangeNames, animToChangeAnswers ) )
		{
			static TDynArray< String > selectedEventNames;
			static TDynArray< Bool > selectedEventAnswers;

			TDynArray< CExtAnimEvent* > evts;
			m_selectedAnimation->GetAllEvents( evts );

			for ( Uint32 j=0; j<evts.Size(); ++j )
			{
				if ( evts[ j ] )
				{
					selectedEventNames.PushBackUnique( evts[ j ]->GetEventName().AsString() );
				}
			}

			selectedEventAnswers.Resize( selectedEventNames.Size() );

			if ( MultiBoolDialog( this, TXT("Select events"), selectedEventNames, selectedEventAnswers ) )
			{
				for ( Uint32 j=0; j<selectedEventAnswers.Size(); ++j )
				{
					if ( selectedEventAnswers[ j ] )
					{
						CName eventToSync = CName( selectedEventNames[ j ] );
						
						CExtAnimEvent* parentEvent = FindEventByName( m_selectedAnimation, eventToSync );
						if ( !parentEvent )
						{
							continue;
						}

						const Bool parentIsDuration = IsType< CExtAnimDurationEvent>( parentEvent );
						const Float parentDuration = static_cast< CExtAnimDurationEvent* >( parentEvent )->GetDuration(); 

						for ( Uint32 k=0; k<animToChangeAnswers.Size(); ++k )
						{
							if ( animToChangeAnswers[ k ] )
							{
								CSkeletalAnimationSetEntry* animToChange = animsToChange[ k ];

								CExtAnimEvent* childEvent = FindEventByName( animToChange, eventToSync );
								if ( childEvent && parentEvent->GetClass() == childEvent->GetClass() )
								{
									childEvent->SetStartTime( parentEvent->GetStartTime() );

									if ( parentIsDuration && IsType< CExtAnimDurationEvent>( childEvent ) )
									{
										static_cast< CExtAnimDurationEvent* >( childEvent )->SetDuration( parentDuration );
									}
								}
							}
						}
					}
				}
			}
		}
	}
}

void CEdAnimBrowser::OnCopyAllAnimationsToClipboard( wxCommandEvent& event )
{
	CSkeletalAnimationSet* set = GetSelectedAnimSet();
	if( set )
	{
		String allAnims;

		const TDynArray<CSkeletalAnimationSetEntry*> anims = set->GetAnimations();
		const TDynArray<CSkeletalAnimationSetEntry*>::const_iterator end = anims.End();
		for( TDynArray<CSkeletalAnimationSetEntry*>::const_iterator it = anims.Begin(); it != end; ++it )
		{
			if ( !allAnims.Empty() )
			{
				allAnims += TXT("\n");
			}

			allAnims += (*it)->GetName().AsString();
		}

		if ( GClipboard )
		{
			GClipboard->Copy(allAnims);
		}
	}
}

void CEdAnimBrowser::OnToggleGhosts( wxCommandEvent& event )
{
	if( event.IsChecked() )
	{
		m_previewPanel->ShowGhosts( m_ghostCount, m_ghostType );
	}
	else
	{
		m_previewPanel->HideGhosts();
	}
}

void CEdAnimBrowser::OnConfigureGhosts( wxCommandEvent& event )
{
	CEdGhostConfigDialog* dialog = new CEdGhostConfigDialog( this, m_ghostCount, m_ghostType );
	dialog->Bind( wxEVT_GHOSTCONFIGUPDATED, &CEdAnimBrowser::OnConfigureGhostsOK, this );

	dialog->Show();
}

void CEdAnimBrowser::OnConfigureGhostsOK( wxCommandEvent& event )
{
	CEdGhostConfigDialog* dialog = wxStaticCast( event.GetEventObject(), CEdGhostConfigDialog );

	m_ghostCount = dialog->GetCount();
	m_ghostType = dialog->GetType();

	if( m_previewPanel->HasGhosts() )
	{
		m_previewPanel->ShowGhosts( m_ghostCount, m_ghostType );
	}
}

bool CEdAnimBrowser::ShouldBeRestarted() const
{
	return m_resettingConfig;
}

void CEdAnimBrowser::OnZoomExtentsPreview( wxCommandEvent& event )
{
	m_previewPanel->ShowZoomExtents( m_entity->CalcBoundingBox() );
}

//////////////////////////////////////////////////////////////////////////

CEdAnimBrowser::PlayedAnimation::PlayedAnimation()
	: m_bodyMimicFlag( true )
	, m_playedAnimation( nullptr )
{

}

void CEdAnimBrowser::PlayedAnimation::Set( CAnimatedComponent* ac, CSkeletalAnimationSetEntry* aniamtion, IPlayedAnimationListener* l ) 
{
	if ( IsBody() )
	{
		m_playedAnimation = ac->GetAnimatedSkeleton()->PlayAnimation( aniamtion );
		if ( m_playedAnimation )
		{
			m_playedAnimation->AddAnimationListener( l );
		}
	}
	else
	{
		if ( ac && !m_slot.IsValid() )
		{
			ac->GetBehaviorStack()->GetSlot( CName( TXT( "MIXER_SLOT_OVERRIDE" ) ), m_slot, false );
		}
		
		if ( m_slot.IsValid() )
		{
			SAnimationFullState animA;
			animA.m_state.m_animation = aniamtion->GetName();
			SAnimationFullState animB;
			animB.m_state.m_animation = aniamtion->GetName();
			m_slot.SetIdleAnimationToSample( animA, animB, 1.f );
		}
	}
}

void CEdAnimBrowser::PlayedAnimation::Reset()
{
	m_playedAnimation = nullptr;
}

Bool CEdAnimBrowser::PlayedAnimation::IsBody() const
{
	return m_bodyMimicFlag;
}

Bool CEdAnimBrowser::PlayedAnimation::IsPaused() const 
{ 
	if ( IsBody() )
	{
		if ( m_playedAnimation )
		{
			return m_playedAnimation->IsPaused();
		}
	}

	return false; 
}

void CEdAnimBrowser::PlayedAnimation::Pause() 
{
	if ( IsBody() )
	{
		if ( m_playedAnimation )
		{
			m_playedAnimation->Pause();
		}
	}
}

void CEdAnimBrowser::PlayedAnimation::Unpause()
{
	if ( IsBody() )
	{
		if ( m_playedAnimation )
		{
			m_playedAnimation->Unpause();
		}
	}
}

Bool CEdAnimBrowser::PlayedAnimation::IsEqual( const CSkeletalAnimationSetEntry* rhs ) const 
{ 
	if ( IsBody() )
	{
		if ( m_playedAnimation )
		{
			return m_playedAnimation->IsEqual( rhs );
		}
	}
	else if ( m_slot.IsValid() )
	{
		const SAnimationFullState* state = m_slot.GetIdleA();
		return state && state->GetAnimation() == rhs;
	}

	return false; 
}

Bool CEdAnimBrowser::PlayedAnimation::IsValid() const 
{ 
	if ( IsBody() )
	{
		if ( m_playedAnimation )
		{
			return m_playedAnimation->IsValid();
		}
	}
	else
	{
		return m_slot.IsValid();
	}

	return false; 
}

Float CEdAnimBrowser::PlayedAnimation::GetTime() const 
{ 
	if ( IsBody() )
	{
		if ( m_playedAnimation )
		{
			return m_playedAnimation->GetTime();
		}
	}
	else if ( m_slot.IsValid() )
	{
		const SAnimationFullState* state = m_slot.GetIdleA();
		return state ? state->m_state.m_currTime : 0.f;
	}

	return 0.f; 
}

void CEdAnimBrowser::PlayedAnimation::SetTime( Float t ) 
{
	if ( IsBody() )
	{
		if ( m_playedAnimation )
		{
			m_playedAnimation->SetTime( t );
		}
	}
	else if ( m_slot.IsValid() )
	{
		//const SAnimationFullState& state = m_slot.GetIdleA();
		//return state.m_state.m_currTime;
	}
}

Float CEdAnimBrowser::PlayedAnimation::GetDuration() const 
{ 
	if ( IsBody() )
	{
		if ( m_playedAnimation )
		{
			return m_playedAnimation->GetDuration();
		}
	}
	else if ( m_slot.IsValid() )
	{
		const SAnimationFullState* state = m_slot.GetIdleA();
		return state && state->GetAnimation() ? state->GetAnimation()->GetDuration() : 1.f;
	}

	return 1.f; 
}

CName CEdAnimBrowser::PlayedAnimation::GetName() const 
{ 
	if ( IsBody() )
	{
		if ( m_playedAnimation )
		{
			return m_playedAnimation->GetName();
		}
	}
	else if ( m_slot.IsValid() )
	{
		const SAnimationFullState* state = m_slot.GetIdleA();
		return state && state->GetAnimation() ? state->GetAnimation()->GetName() : CName::NONE;
	}

	return CName::NONE; 
}

const CSkeletalAnimationSetEntry* CEdAnimBrowser::PlayedAnimation::GetAnimationEntry() 
{ 
	if ( IsBody() )
	{
		if ( m_playedAnimation )
		{
			return m_playedAnimation->GetAnimationEntry();
		}
	}
	else if ( m_slot.IsValid() )
	{
		const SAnimationFullState* state = m_slot.GetIdleA();
		return state ? state->GetAnimation() : nullptr;
	}

	return nullptr; 
}

void CEdAnimBrowser::PlayedAnimation::ForceCompressedPose() 
{
	if ( IsBody() )
	{
		if ( m_playedAnimation )
		{
			m_playedAnimation->ForceCompressedPose();
		}
	}
}

void CEdAnimBrowser::PlayedAnimation::SetMimic( CMimicComponent* mimicComponent ) 
{ 
	m_bodyMimicFlag = false;

	if ( mimicComponent->GetBehaviorStack() )
	{
		mimicComponent->GetBehaviorStack()->GetSlot( CName( TXT( "MIXER_SLOT_OVERRIDE" ) ), m_slot, false );
	}
}


//////////////////////////////////////////////////////////////////////////

Float PoseComparator::CalcSimilarity( const SBehaviorGraphOutput* poseA, const SBehaviorGraphOutput* poseB ) const
{
	Uint32 size = Min< Uint32 >( poseA->m_numBones, poseB->m_numBones );
	for ( Uint32 i=0; i<size; ++i )
	{
		const AnimQsTransform& boneA = poseA->m_outputPose[ i ];
		const AnimQsTransform& boneB = poseB->m_outputPose[ i ];

#ifdef USE_HAVOK_ANIMATION
		Float distPos = boneA.m_translation.distanceTo3( boneB.m_translation );

		hkVector4 rotA = boneA.m_rotation.m_vec;
		hkVector4 rotB = boneB.m_rotation.m_vec;

		rotA.normalize4();
		rotB.normalize4();

		Float rotDot = rotA.dot4( rotB );
		if ( rotDot < 0.0f )
		{
			rotA.mul4(-1.0f);
			rotDot = rotA.dot4( rotB );
		}
#else
		Float distPos = boneA.Translation.DistanceTo( boneB.Translation );

		RedVector4 rotA = boneA.Rotation.Quat;
		RedVector4 rotB = boneB.Rotation.Quat;

		rotA.Normalize4();
		rotB.Normalize4();

		Float rotDot = Dot(rotA, rotB );
		if ( rotDot < 0.0f )
		{
			SetMul( rotA, -1.0f );
			rotDot = Dot(rotA, rotB );
		}
#endif
		//Float distRot = rotDot;

		//...
	}

	return 0.f;
}

Float PoseComparator::CalcSimilarity( const CSkeleton* skeleton, const CSkeletalAnimation* animA, Float timeA, const CSkeletalAnimation* animB, Float timeB ) const
{
	CPoseHandle poseA = skeleton->GetPoseProvider()->AcquirePose();
	CPoseHandle poseB = skeleton->GetPoseProvider()->AcquirePose();

	animA->Sample( timeA, poseA->m_numBones, poseA->m_numFloatTracks, poseA->m_outputPose, poseA->m_floatTracks );
	animA->Sample( timeB, poseB->m_numBones, poseB->m_numFloatTracks, poseB->m_outputPose, poseB->m_floatTracks );

	Float ret = CalcSimilarity( poseA.Get(), poseB.Get() );

	return ret;
}

#undef for_each
#undef for_each_ptr

#ifdef DEBUG_ANIMS
#pragma optimize("",on)
#endif
