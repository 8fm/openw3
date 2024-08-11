/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "dialogTimeline.h"

#include "../../common/core/depot.h"
#include "../../common/core/memoryFileWriter.h"
#include "../../common/core/memoryFileReader.h"

#include "../../common/game/storySceneLine.h"
#include "../../common/game/storySceneCutscene.h"
#include "../../common/game/storySceneScriptLine.h"
#include "../../common/game/storyScenePauseElement.h"
#include "../../common/game/storySceneBlockingElement.h"
#include "../../common/game/storySceneEvent.h"
#include "../../common/game/storySceneEventDuration.h"
#include "../../common/game/storySceneEventAnimation.h"
#include "../../common/game/storySceneEventEnterActor.h"
#include "../../common/game/storySceneEventExitActor.h"
#include "../../common/game/storySceneEventLookat.h"
#include "../../common/game/storySceneEventDespawn.h"
#include "../../common/game/storySceneEventFade.h"
#include "../../common/game/storySceneEventMimics.h"
#include "../../common/game/storySceneEventMimicsAnim.h"
#include "../../common/game/storySceneEventSound.h"
#include "../../common/game/storySceneEventHitSound.h"
#include "../../common/game/storySceneEventRotate.h"
#include "../../common/game/storySceneEventCustomCamera.h"
#include "../../common/game/storySceneEventCustomCameraInstance.h"
#include "../../common/game/storySceneEventCameraBlend.h"
#include "../../common/game/storySceneEventChangePose.h"
#include "../../common/game/storySceneEventEquipItem.h"
#include "../../common/game/storySceneSection.h"
#include "../../common/game/storySceneComment.h"
#include "../../common/game/storyScene.h"
#include "../../common/game/actor.h"
#include "../../common/game/storySceneCutsceneSection.h"
#include "../../common/game/storySceneEventMimicPose.h"
#include "../../common/game/storySceneEventMimicFilter.h"
#include "../../common/game/storySceneEventOverridePlacement.h"
#include "../../common/game/storySceneEventGroup.h"
#include "../../common/game/storySceneEventEnhancedCameraBlend.h"
#include "../../common/game/storySceneEventInterpolation.h"
#include "../../common/game/storySceneItems.h"
#include "../../common/game/storySceneEventLightProperties.h"
#include "../../common/game/storySceneVideo.h"
#include "../../common/game/StorySceneEventCameraLight.h"
#include "../../common/game/storySceneEventScenePropPlacement.h"
#include "../../common/engine/localizationManager.h"

#include "dialogEditorPage.h"
#include "dialogPreview.h"
#include "dialogEditorActions.h"
#include "undoTimeLine.h"
#include "timelineImpl/drawing.h"
#include "timelineImpl/drawGroupDialogTracks.h"
#include "dialogEditorUtils.h"
#include "dialogTimeline_items.h"
#include "dialogTimeline_includes.h"
#include "dialogTimelineItemInterpolationEvent.h"

#ifdef DEBUG_SCENES_2
#pragma optimize("",off)
#endif

using namespace DialogTimelineItems;

const String	CEdDialogTimeline::TIMELINE_DEFAULT_CAMERA_TRACK( TXT( "_Camera" ) );
const String	CEdDialogTimeline::CAMERA_LIGHTS_TRACK( TXT( "Camera lights" ) );
const String	CEdDialogTimeline::PROP_TRACK( TXT( "PROPS" ) );
const String	CEdDialogTimeline::EFFECT_TRACK( TXT( "EFFECTS" ) );
const String	CEdDialogTimeline::LIGHT_TRACK( TXT( "LIGHTS" ) );
const String	CEdDialogTimeline::CUTSCENE_LINES_TRACK( TXT( "CUTSCENE LINES" ) );
const Int32		CEdDialogTimeline::TIMELINE_PADLOCK_BTN_SIZE = 16;

wxBEGIN_EVENT_TABLE( CEdDialogTimeline, CEdTimeline )
	EVT_MOUSEWHEEL( CEdDialogTimeline::OnMouseWheel )
	EVT_MENU( wxID_STORYSCENEEDITOR_CUT_CLIP, CEdDialogTimeline::OnCutAnimClip )
	EVT_MENU( wxID_STORYSCENEEDITOR_SET_CLIPPING_FRONT, CEdDialogTimeline::OnSetClippingFront )
	EVT_MENU( wxID_STORYSCENEEDITOR_SET_CLIPPING_END, CEdDialogTimeline::OnSetClippingBack )
	EVT_MENU( wxID_STORYSCENEEDITOR_ALIGN_EVENTS_LEFT, CEdDialogTimeline::OnAlignStart )
	EVT_MENU( wxID_STORYSCENEEDITOR_ALIGN_EVENTS_RIGHT, CEdDialogTimeline::OnAlignEnd )
	EVT_MENU( wxID_STORYSCENEEDITOR_ALIGN_EVENTS_TO_CAMERA, CEdDialogTimeline::OnAlignToCamera )
	EVT_MENU( ID_ADD_PAUSE_BEFORE, CEdDialogTimeline::OnCreatePause )
	EVT_MENU( ID_ADD_PAUSE_AFTER, CEdDialogTimeline::OnCreatePause )
	EVT_MENU( ID_NORMAL_EVENT, CEdDialogTimeline::OnNormalEventCheckedChanged )
	EVT_MENU( ID_MARK_EVENT, CEdDialogTimeline::OnMarkEventCheckedChanged )
	EVT_MENU( ID_LOCK_EVENT, CEdDialogTimeline::OnLockEventCheckedChanged )
	EVT_MENU( ID_ADD_PAUSE_5, CEdDialogTimeline::OnCreatePause )
	EVT_MENU( ID_ADD_PAUSE_15, CEdDialogTimeline::OnCreatePause )
	EVT_MENU( ID_ADD_PAUSE_30, CEdDialogTimeline::OnCreatePause )
	EVT_MENU( ID_ADD_CAMERA_EVENT, CEdDialogTimeline::OnNewEvent )
	EVT_MENU( ID_ADD_CUSTOM_CAMERA_INSTANCE, CEdDialogTimeline::OnNewEvent )
	EVT_MENU( ID_ADD_ENHANCED_BLEND_INTERPOLATED_CAMERA_EVENT, CEdDialogTimeline::OnNewEvent )
	EVT_MENU( ID_ADD_ANIMATION_EVENT, CEdDialogTimeline::OnNewEvent )
	EVT_MENU( ID_ADD_CHANGE_ACTOR_GAME_STATE_EVENT, CEdDialogTimeline::OnNewEvent )
	EVT_MENU( ID_ADD_ADDITIVE_ANIMATION_EVENT, CEdDialogTimeline::OnNewEvent )
	EVT_MENU( ID_ADD_OVERRIDE_ANIMATION_EVENT, CEdDialogTimeline::OnNewEvent )
	EVT_MENU( ID_ADD_ENTER_ACTOR_EVENT, CEdDialogTimeline::OnNewEvent )
	EVT_MENU( ID_ADD_EXIT_ACTOR_EVENT, CEdDialogTimeline::OnNewEvent )
	EVT_MENU( ID_ADD_CHANGE_POSE, CEdDialogTimeline::OnNewEvent )
	EVT_MENU( ID_ADD_LOOKAT_EVENT, CEdDialogTimeline::OnNewEvent )
	EVT_MENU( ID_ADD_LOOKAT_DURATION_EVENT, CEdDialogTimeline::OnNewEvent )
	EVT_MENU( ID_ADD_LOOKAT_GAMEPLAY_EVENT, CEdDialogTimeline::OnNewEvent )
	EVT_MENU( ID_ADD_EQUIP_ITEM_EVENT, CEdDialogTimeline::OnNewEvent )
	EVT_MENU( ID_ADD_MORPH_EVENT, CEdDialogTimeline::OnNewEvent )
	EVT_MENU( ID_ADD_D_PX_CLOTH_EVENT, CEdDialogTimeline::OnNewEvent )
	EVT_MENU( ID_ADD_DANGLES_SHAKE_EVENT, CEdDialogTimeline::OnNewEvent )
	EVT_MENU( ID_ADD_D_DANGLE_EVENT, CEdDialogTimeline::OnNewEvent )
	EVT_MENU( ID_ADD_RESET_DANGLE_EVENT, CEdDialogTimeline::OnNewEvent )
	EVT_MENU( ID_ADD_VISIBILITY_EVENT, CEdDialogTimeline::OnNewEvent )
	EVT_MENU( ID_ADD_EVENT_LODOVERRIDE, CEdDialogTimeline::OnNewEvent )
	EVT_MENU( ID_ADD_APPEARANCE_EVENT, CEdDialogTimeline::OnNewEvent )
	EVT_MENU( ID_ADD_HIRES_SHADOWS_EVENT, CEdDialogTimeline::OnNewEvent )
	EVT_MENU( ID_ADD_MIMIC_LOD_EVENT, CEdDialogTimeline::OnNewEvent )
	EVT_MENU( ID_ADD_PROP_VISIBILITY_EVENT, CEdDialogTimeline::OnNewEvent )	
	//EVT_MENU( ID_ADD_ROTATE_TO_CAMERA_EVENT, CEdDialogTimeline::OnNewEvent )
	EVT_MENU( ID_ADD_PLACEMENT_OVERRIDE_EVENT, CEdDialogTimeline::OnNewEvent )
	EVT_MENU( ID_ADD_PLACEMENT_OVERRIDE_EVENT_DURATION, CEdDialogTimeline::OnNewEvent )
	EVT_MENU( ID_ADD_PLACEMENT_OVERRIDE_BLEND_INTERPOLATED_EVENT, CEdDialogTimeline::OnNewEvent )
	EVT_MENU( ID_ADD_PLACEMENT_OVERRIDE_BLEND, CEdDialogTimeline::OnAddPlacementOverrideBlend )
	EVT_MENU( ID_ADD_SCENE_PROP_PLACEMENT_EVENT, CEdDialogTimeline::OnNewEvent )
	EVT_MENU( ID_ADD_WORLD_PROP_PLACEMENT_EVENT, CEdDialogTimeline::OnNewEvent )
	EVT_MENU( ID_ADD_LIGHT_PROPERTIES_EVENT, CEdDialogTimeline::OnNewEvent )
	EVT_MENU( ID_ADD_PLACEMENT_WALK, CEdDialogTimeline::OnNewEvent )
	EVT_MENU( ID_ADD_ACTOR_EFFECT_ITEM_EVENT, CEdDialogTimeline::OnNewEvent )
	EVT_MENU( ID_ADD_PROP_EFFECT_ITEM_EVENT, CEdDialogTimeline::OnNewEvent )
	EVT_MENU( ID_ADD_ACTOR_EFFECT_DURATION_ITEM_EVENT, CEdDialogTimeline::OnNewEvent )
	EVT_MENU( ID_ADD_CURVE_ANIMATION_EVENT, CEdDialogTimeline::OnNewEvent )
	EVT_MENU( ID_ADD_MIMICS_ANIMATION_EVENT, CEdDialogTimeline::OnNewEvent )
	EVT_MENU( ID_ADD_CAMERA_ANIMATION_EVENT, CEdDialogTimeline::OnNewEvent )
	EVT_MENU( ID_ADD_GAMEPLAY_CAMERA_EVENT, CEdDialogTimeline::OnNewEvent )
	EVT_MENU( ID_ADD_START_BLEND_TO_GAMEPLAY_CAMERA_EVENT, CEdDialogTimeline::OnNewEvent )
	EVT_MENU( ID_ADD_MIMICS_EVENT, CEdDialogTimeline::OnNewEvent )
	EVT_MENU( ID_ADD_MIMICS_POSE_EVENT, CEdDialogTimeline::OnNewEvent )
	EVT_MENU( ID_ADD_MIMICS_FILTER_EVENT, CEdDialogTimeline::OnNewEvent )
	EVT_MENU( ID_ADD_FADE_EVENT, CEdDialogTimeline::OnNewEvent )
	EVT_MENU( ID_ADD_WEATHER_EVENT, CEdDialogTimeline::OnNewEvent )
	EVT_MENU( ID_ADD_SURFACE_EFFECT_EVENT, CEdDialogTimeline::OnNewEvent )
	EVT_MENU( ID_ADD_ATTACH_PROP_TO_BONE, CEdDialogTimeline::OnNewEvent )
	EVT_MENU( ID_ADD_ADDFACT_EVENT, CEdDialogTimeline::OnNewEvent )	
	EVT_MENU( ID_ADD_CAMERALIGHT_EVENT, CEdDialogTimeline::OnNewEvent )		
	EVT_MENU( ID_ADD_OPENDOOR_EVENT, CEdDialogTimeline::OnNewEvent )	
	EVT_MENU( ID_ADD_SOUND_EVENT, CEdDialogTimeline::OnNewEvent )
	EVT_MENU( ID_ADD_HIT_SOUND_EVENT, CEdDialogTimeline::OnNewEvent )
	EVT_MENU( ID_ADD_DIALOG_LINE_EVENT, CEdDialogTimeline::OnNewEvent )
	EVT_MENU( ID_ADD_TO_CUSTOM_CAMERA_LIST, CEdDialogTimeline::OnAddToCustomCameraList )
	EVT_MENU( ID_ADD_TO_CAMERA_ENHANCED_BLEND, CEdDialogTimeline::OnAddToCameraEnhancedBlend )
	EVT_MENU( ID_REMOVE_FROM_CAMERA_ENHANCED_BLEND, CEdDialogTimeline::OnRemoveFromCameraEnhancedBlend )
	EVT_MENU( ID_DETACH_FROM_CUSTOM_CAMERA_LIST, CEdDialogTimeline::OnDetachFromCustomCameraList )
	EVT_MENU( ID_BLEND_CAMERA_EVENT_PLOT_POSITION, CEdDialogTimeline::OnCameraBlendNewPlot )
	EVT_MENU( ID_BLEND_CAMERA_EVENT_PLOT_ROTATION, CEdDialogTimeline::OnCameraBlendNewPlot )
	EVT_MENU( ID_REMOVE_FROM_CAMERA_BLEND, CEdDialogTimeline::OnRemoveFromCameraBlendNew )
	EVT_MENU( ID_REMOVE_CAMERA_BLEND, CEdDialogTimeline::OnRemoveCameraBlend )
	EVT_MENU( ID_ADD_ENHANCED_CAMERA_BLEND, CEdDialogTimeline::OnCreateEnhancedCameraBlend )
	EVT_MENU( ID_GROUP_EVENTS, CEdDialogTimeline::OnGroupEvents )
	EVT_MENU( ID_UNGROUP_EVENTS, CEdDialogTimeline::OnUngroupEvents )
	EVT_MENU( ID_LINK_EVENT, CEdDialogTimeline::OnLinkEvent )
	EVT_MENU( ID_UNLINK_EVENT, CEdDialogTimeline::OnUnlinkEvent )
	EVT_MENU( ID_MUTE_EVENT, CEdDialogTimeline::OnMuteEvent )
	EVT_MENU( ID_GOTO_MARKER, CEdDialogTimeline::OnGoToMarker )
	EVT_MENU( ID_OVERRIDE_PLACEMENT_COPY_TO_DIALOGSLOT, CEdDialogTimeline::OnCopyTransformToDialogsetSlotFromPlacementEvent )
	EVT_MENU( ID_LIGHT_GO_TO_CENTER_SS, CEdDialogTimeline::OnCopyLightGoToCenterSS )
	EVT_MENU( ID_ALIGN_END_TO_SECTIONS_END, CEdDialogTimeline::OnAlignEndToSectionsEnd )
	EVT_MENU( ID_ALIGN_STARTS, CEdDialogTimeline::OnAlignStart )
	EVT_MENU( ID_ALIGN_ENDS, CEdDialogTimeline::OnAlignEnd )
	EVT_MENU( ID_ALIGN_START_TO_END, CEdDialogTimeline::OnAlignStartToEnd )
	EVT_MENU( ID_SET_AS_BACKGROUND_LINE, CEdDialogTimeline::OnSetAsBackgroundLine )
	EVT_MENU( ID_SET_AS_NONBACKGROUND_LINE, CEdDialogTimeline::OnSetAsNonBackgroundLine )
	EVT_MENU( ID_TOGGLE_TIME_PAUSE, CEdDialogTimeline::OnToggleTimePause )
	EVT_MENU( ID_ADD_POSE_KEY_EVENT, CEdDialogTimeline::OnNewEvent )
	EVT_MENU( ID_LIMIT_MIN_SET, CEdDialogTimeline::OnLimitMinSet )
	EVT_MENU( ID_LIMIT_MAX_SET, CEdDialogTimeline::OnLimitMaxSet )
	EVT_MENU( ID_LIMIT_MIN_RESET, CEdDialogTimeline::OnLimitMinReset )
	EVT_MENU( ID_LIMIT_MAX_RESET, CEdDialogTimeline::OnLimitMaxReset )
	EVT_MENU( ID_ARROW_LEFT, CEdDialogTimeline::OnArrowLeft )
	EVT_MENU( ID_ARROW_RIGHT, CEdDialogTimeline::OnArrowRight )
	EVT_MENU( ID_ARROW_UP, CEdDialogTimeline::OnArrowUp )
	EVT_MENU( ID_ARROW_DOWN, CEdDialogTimeline::OnArrowDown )
	EVT_MENU( ID_CREATE_PLACEMENT_FOR_ANIM_PEVLIS, CEdDialogTimeline::OnCreatePlacementForAnimPelvis )
	EVT_MENU( ID_SPLIT_ANIMATION_EVENT, CEdDialogTimeline::OnSplitAnimationEvent )
	EVT_MENU( ID_COPY_CAM_LIGHTS, CEdDialogTimeline::OnCopyLightFromCamera )
	EVT_MENU( ID_PASTE_CAM_LIGHTS, CEdDialogTimeline::OnPasteLightToCamera )
	EVT_MENU( ID_TOGGLE_ENTITY_HELPER_IS_INTERACTIVE, CEdDialogTimeline::OnToggleEntityHelperIsInteractive )

	EVT_MENU( ID_INTERPOLATION_EVENT_CREATE, CEdDialogTimeline::OnInterpolationEventCreate )
	EVT_MENU( ID_INTERPOLATION_EVENT_DESTROY, CEdDialogTimeline::OnInterpolationEventDestroy )
	EVT_MENU( ID_INTERPOLATION_EVENT_ATTACH_KEY, CEdDialogTimeline::OnInterpolationEventAttachKey )
	EVT_MENU( ID_INTERPOLATION_EVENT_DETACH_KEY, CEdDialogTimeline::OnInterpolationEventDetachKey )
	EVT_MENU( ID_INTERPOLATION_EVENT_PLOT, CEdDialogTimeline::OnInterpolationEventPlot )

	EVT_MENU( ID_ADD_DEBUG_COMMENT_EVENT, CEdDialogTimeline::OnDebugCommentEventCreate )
	EVT_MENU( ID_SET_CURRENT_TIME_ZERO, CEdDialogTimeline::OnSetCurrentTime )
	EVT_MENU( ID_SET_CURRENT_TIME_END, CEdDialogTimeline::OnSetCurrentTime )

wxEND_EVENT_TABLE()

// =================================================================================================
namespace {
// =================================================================================================

CanvasType CanvasTypeSwitcher()
{
	static CanvasType nextCanvasType = CanvasType::gdi;

	CanvasType retCanvasType = nextCanvasType;
	if(nextCanvasType == CanvasType::gdi)
	{
		nextCanvasType = CanvasType::gdiplus;
	}
	else
	{
		nextCanvasType = CanvasType::gdi;
	}

	return retCanvasType;
}

/*
Returns whether timeline item is scene element.

\param item Timeline item to check. Must not be 0.
*/
Bool IsElement(ITimelineItem* item)
{
	ASSERT( item );
	return dynamic_cast< CTimelineItemBlocking* >( item ) != 0;
}

/*
Returns whether timeline item is event.

\param item Timeline item to check. Must not be 0.
*/
Bool IsEvent(ITimelineItem* item)
{
	ASSERT( item );
	return dynamic_cast< CTimelineItemEvent* >( item ) != 0;
}

// =================================================================================================
} // unnamed namespace
// =================================================================================================


CEdDialogTimeline::CEdDialogTimeline( wxPanel* parentPanel, CEdSceneEditor* parentSceneScriptEditor, CEdPropertiesPage* propertiesPage )
	: CEdTimeline( parentPanel, propertiesPage, CanvasType::gdi )
	, m_mediator( parentSceneScriptEditor )
	, m_section( NULL )
	, m_debugItems( false )
	, m_showVoiceMarkers( false )
	, m_eventsLinker( this )
{
	SetWindowStyleFlag( wxWANTS_CHARS );

	const Uint32 numAccelerators = 24;
	wxAcceleratorEntry shortcutsEntries[ 24 ];
	shortcutsEntries[ 0  ].Set( wxACCEL_SHIFT,	'c',		wxID_STORYSCENEEDITOR_CUT_CLIP );
	shortcutsEntries[ 1  ].Set( wxACCEL_SHIFT,	'i',		wxID_STORYSCENEEDITOR_SET_CLIPPING_FRONT );
	shortcutsEntries[ 2  ].Set( wxACCEL_SHIFT,	'o',		wxID_STORYSCENEEDITOR_SET_CLIPPING_END );
	shortcutsEntries[ 3  ].Set( wxACCEL_ALT,	'a',		wxID_STORYSCENEEDITOR_ALIGN_EVENTS_LEFT );
	shortcutsEntries[ 4  ].Set( wxACCEL_ALT,	'd',		wxID_STORYSCENEEDITOR_ALIGN_EVENTS_RIGHT );
	shortcutsEntries[ 5  ].Set( wxACCEL_ALT,	'w',		wxID_STORYSCENEEDITOR_ALIGN_EVENTS_TO_CAMERA );
	shortcutsEntries[ 6  ].Set( wxACCEL_NORMAL,	'l',		ID_LINK_EVENT );
	shortcutsEntries[ 7  ].Set( wxACCEL_NORMAL,	'u',		ID_UNLINK_EVENT );
	shortcutsEntries[ 8  ].Set( wxACCEL_NORMAL,	'p',		ID_TOGGLE_TIME_PAUSE );
	shortcutsEntries[ 9  ].Set( wxACCEL_NORMAL,	'm',		ID_MUTE_EVENT );
	shortcutsEntries[ 10 ].Set( wxACCEL_NORMAL,	'r',		ID_SPLIT_ANIMATION_EVENT );
	shortcutsEntries[ 11 ].Set( wxACCEL_NORMAL,	'n',		ID_GOTO_MARKER );
	shortcutsEntries[ 12 ].Set( wxACCEL_NORMAL,	'c',		ID_ADD_CAMERA_EVENT );
	shortcutsEntries[ 13 ].Set( wxACCEL_NORMAL,	'[',		ID_LIMIT_MIN_SET );
	shortcutsEntries[ 14 ].Set( wxACCEL_NORMAL,	']',		ID_LIMIT_MAX_SET );
	shortcutsEntries[ 15 ].Set( wxACCEL_CTRL,	'p',		ID_ADD_CHANGE_POSE );
	shortcutsEntries[ 16 ].Set( wxACCEL_CTRL,	'[',		ID_LIMIT_MIN_RESET );
	shortcutsEntries[ 17 ].Set( wxACCEL_CTRL,	']',		ID_LIMIT_MAX_RESET );
	shortcutsEntries[ 18 ].Set( wxACCEL_NORMAL,	WXK_LEFT,	ID_ARROW_LEFT );
	shortcutsEntries[ 19 ].Set( wxACCEL_NORMAL,	WXK_RIGHT,	ID_ARROW_RIGHT );
	shortcutsEntries[ 20 ].Set( wxACCEL_NORMAL,	WXK_UP,		ID_ARROW_UP );
	shortcutsEntries[ 21 ].Set( wxACCEL_NORMAL,	WXK_DOWN,	ID_ARROW_DOWN );
	shortcutsEntries[ 22 ].Set( wxACCEL_NORMAL,	WXK_HOME,	ID_SET_CURRENT_TIME_ZERO );
	shortcutsEntries[ 23 ].Set( wxACCEL_NORMAL,	WXK_END,	ID_SET_CURRENT_TIME_END );

	wxAcceleratorTable shortcutsTable( numAccelerators, shortcutsEntries );
	SetAcceleratorTable( shortcutsTable );

	m_timelineToolbar = XRCCTRL( *parentSceneScriptEditor, "DialogTimelineToolbar", wxToolBar );
	m_timelinePlayTool = XRCID( "PlayButton" );
	m_timelinePlayIcon = SEdResources::GetInstance().LoadBitmap( TEXT( "IMG_CONTROL_PLAY" ) );
	m_timelinePauseIcon = SEdResources::GetInstance().LoadBitmap( TEXT( "IMG_CONTROL_PAUSE" ) );

	m_lockedBitmap = ConvertToGDI( SEdResources::GetInstance().LoadBitmap( TXT( "IMG_LOCKED_PADLOCK" ) ) );
	m_unlockedBitmap = ConvertToGDI( SEdResources::GetInstance().LoadBitmap( TXT("IMG_UNLOCKED_PADLOCK") ) );

	RegisterDrawGroupTracksPinned(new DialogTimelineImpl::CDrawGroupDialogTracks(*this, &m_pinnedBuffer));
	RegisterDrawGroupTracksDefault(new DialogTimelineImpl::CDrawGroupDialogTracks(*this, &m_defaultBuffer));
	RegisterDrawGroupTimebar(new TimelineImpl::CDrawGroupTimebar(*this, &m_timebarBuffer));
	RegisterDrawGroupVolatile(new TimelineImpl::CDrawGroupVolatile(*this, &m_volatileBuffer));

	TimeLimitsEnabled( true );


	TDynArray<CClass*> classes;	
	SRTTI::GetInstance().EnumClasses( CStorySceneEvent::GetStaticClass(), classes );

	Int32 Id = ID_ADD_GENERIC_EVENT;

	for ( CClass* cls : classes )
	{
		CStorySceneEvent* defObj = cls->GetDefaultObject<CStorySceneEvent>();
		if( defObj->GenericEventCreation() )
		{
			m_genericObjsData.PushBack( MakePair( ++Id, defObj->GetGenericCreationData() ) );			
		}		
	}
}

CEdDialogTimeline::~CEdDialogTimeline()
{}

void CEdDialogTimeline::PaintCanvas( Int32 width, Int32 height )
{
	static Bool PRINT = true;
	if ( PRINT )
	{
		CEdTimeline::PaintCanvas( width, height );

		m_eventsLinker.PrintCanvas( m_cursorPos );
	}
}

const CAnimatedComponent* CEdDialogTimeline::GetBodyComponent( const CName& actor ) const
{
	return m_mediator->OnTimeline_GetBodyComponentForActor( actor );
}

const CAnimatedComponent* CEdDialogTimeline::GetHeadComponent( const CName& actor ) const
{
	return m_mediator->OnTimeline_GetMimicComponentForActor( actor );
}


void CEdDialogTimeline::RemoveItemImpl( ITimelineItem* item )
{
	item->OnDeleted();

	if ( CTimelineItemEvent* eventItem = dynamic_cast< CTimelineItemEvent* >( item ) )
	{
		m_mediator->OnTimeline_RemoveEvent( m_section, eventItem->GetEvent() );
	}
	else if( CTimelineItemBlocking* elementItem = dynamic_cast< CTimelineItemBlocking* >( item ) )
	{
		m_mediator->OnTimeline_RemoveElement( m_section, elementItem->GetElement() );
	}
}

void CEdDialogTimeline::OnTrackSelectionChanged()
{
	String trackName;

	if ( m_selectedTrack != -1 )
	{
		trackName = m_tracks[ m_selectedTrack ]->m_name;
	}

	m_mediator->OnTimeline_TrackSelectionChanged( trackName );
}

Bool CEdDialogTimeline::UseLocColors() const
{
	return m_mediator->OnTimeline_UseLocColors();
}

wxColor CEdDialogTimeline::FindLocColor( Uint32 stringId ) const
{
	return m_mediator->OnTimeline_FindLocColor( stringId );
}

const IStorySceneElementInstanceData* CEdDialogTimeline::FindElementInstance( const CStorySceneElement* element ) const
{
	return m_mediator->OnTimeline_FindElementInstance( element );
}

Float CEdDialogTimeline::GetEventInstanceStartTime( const CStorySceneEvent& e ) const
{
	return m_mediator->OnTimeline_GetEventInstanceStartTime( e );
}

void CEdDialogTimeline::SetEventInstanceStartTime( const CStorySceneEvent& e, Float startTime ) const
{
	m_mediator->OnTimeline_SetEventInstanceStartTime( e, startTime );
}

Float CEdDialogTimeline::GetEventInstanceDuration( const CStorySceneEvent& e ) const
{
	return m_mediator->OnTimeline_GetEventInstanceDuration( e );
}

void CEdDialogTimeline::SetEventInstanceDuration( const CStorySceneEvent& e, Float duration ) const
{
	m_mediator->OnTimeline_SetEventInstanceDuration( e, duration );
}

Float CEdDialogTimeline::GetEventInstanceScalingFactor( const CStorySceneEvent& e ) const
{
	return m_mediator->OnTimeline_GetEventScalingFactor( e );
}

void CEdDialogTimeline::LoadTracksStateFromConfig( const String& sceneDepotPath )
{
	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
	Bool saveTracks = false;
	{
		CConfigurationScopedPathSetter pathSetter( config, TXT("/SceneEditorTracks") );
		saveTracks = ( config.Read( TXT("saveState"), 0 ) == 1 );
	}

	if ( !saveTracks )
	{
		return;
	}

	const String path = String::Printf( TXT("/SceneEditorTracks/%s/%s"), sceneDepotPath.AsChar(), m_section->GetName().AsChar() );

	Bool expanded = true, hidden = false;
	for ( Track* track : m_tracks )
	{
		if ( !track->m_isGroup )
		{
			continue;
		}

		CConfigurationScopedPathSetter pathSetter( config, String::Printf( TXT("%s/%s"), path.AsChar(), track->m_name.AsChar() ) );
		String configVal = config.Read( TXT("expanded"), String::EMPTY );
		if ( !configVal.Empty() )
		{
			if ( FromString( configVal, expanded ) )
			{
				if ( expanded )
				{
					ExpandTrackGroup( track );
				}
				else
				{
					CollapseTrackGroup( track );
				}
			}
		}
	}
	UpdateLayout();
}

void CEdDialogTimeline::SaveTracksStateToConfig( const String& sceneDepotPath )
{
	// if we don't have any section open, we don't have anything to save
	if ( m_section == nullptr )
	{
		return;
	}

	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
	Bool saveTracks = false;
	{
		CConfigurationScopedPathSetter pathSetter( config, TXT("/SceneEditorTracks") );
		saveTracks = ( config.Read( TXT("saveState"), 0 ) == 1 );
	}

	if ( !saveTracks )
	{
		return;
	}

	const String path = String::Printf( TXT("/SceneEditorTracks/%s/%s"), sceneDepotPath.AsChar(), m_section->GetName().AsChar() );

	for ( const Track* track : m_tracks )
	{
		if ( track->m_isGroup )
		{
			CConfigurationScopedPathSetter pathSetter( config, String::Printf( TXT("%s/%s"), path.AsChar(), track->m_name.AsChar() ) );
			config.Write( TXT("expanded"), ToString( track->m_isExpanded ) );
		}
	}
}

Float CEdDialogTimeline::GetAnimationDurationFromEvent( const CStorySceneEventAnimClip& animClip ) const
{
	return m_mediator->OnTimeline_GetAnimationDurationFromEvent( &animClip );
}

void CEdDialogTimeline::RequestRebuild()
{
	m_mediator->OnTimeline_RequestRebuild();
}

void CEdDialogTimeline::OnItemPropertyChanged( wxCommandEvent& event )
{
	CEdTimeline::OnItemPropertyChanged( event );

	// this is needed since the timeline class registers the wxEVT_COMMAND_PROPERTY_CHANGED event 
	// for the properties grid, and unfortunately we can only register 1 listener per event.
	m_mediator->OnTimeline_OnItemPropertyChanged( event );
}

void CEdDialogTimeline::ItemPropertyChanged( TDynArray< ITimelineItem* >& items, const CName& propertyName )
{
	const Uint32 num = items.Size();
	if( num == 0 )
	{
		return;
	}

	for( Uint32 i=0; i<num; ++i )
	{
		ITimelineItem* item = items[ i ];
		if ( item )
		{
			if ( CTimelineItemEvent* sceneEventItem = dynamic_cast< CTimelineItemEvent* >( item ) )
			{
				if ( CStorySceneEvent* sceneEvent = sceneEventItem->GetEvent() )
				{
					m_mediator->OnTimeline_EventChanged( m_section, sceneEvent, propertyName );
				}
			}		
			item->UpdatePresentation();
		}
	}

	NotifyBufferIsInvalid();

	if ( propertyName == TXT("startPosition") )
	{
		OnCheckAutoLink();
	}
}

void CEdDialogTimeline::SetSelection( TDynArray< ITimelineItem* >& items, Bool goToEvent )
{
	if ( m_isSetSelectionLocked )
	{
		return;
	}

	CEdTimeline::SetSelection( items );

	// Update highlight always - this way the de-selection will be handled
	for( TDynArray< CTimelineItemBlocking* >::iterator elemIter = m_elements.Begin();
		elemIter != m_elements.End(); ++elemIter )
	{
		( *elemIter )->SetHighlight( false, 0.0f );
	}

	if( ! IsOneSelected() )
	{
		m_mediator->OnTimeline_SelectionChanged( m_section, nullptr );
		return;
	}

	ITimelineItem* item = m_selectedItems[ 0 ];

	if( CTimelineItemEvent* eventItem = dynamic_cast< CTimelineItemEvent* >( item ) )
	{
		m_mediator->OnTimeline_LockCamera( true ); // Prevent camera jumping to newly selected camera event
		m_mediator->OnTimeline_SelectionChanged( m_section, eventItem, goToEvent );
		m_mediator->OnTimeline_LockCamera( false );

		// Highlight element item
		ASSERT( eventItem->GetElementItem() );

		eventItem->GetElementItem()->SetHighlight( true, eventItem->GetEvent()->GetStartPosition() );
	}
}

void CEdDialogTimeline::SelectionChanged()
{
	LockSetSelection( true );

	// Reset selection in preview (consecutive notification of items - below - will refresh selection in preview)

	CSelectionManager* selectionManager = m_mediator->GetWorld()->GetSelectionManager();
	selectionManager->DeselectAll();

	// Notify items of being selected

	for ( auto it = m_selectedItems.Begin(); it != m_selectedItems.End(); ++it )
	{
		if ( CTimelineItemEvent* item = dynamic_cast< CTimelineItemEvent* >( *it ) )
		{
			item->OnSelected();
		}
	}

	// Update highlight always - this way the de-selection will be handled
	for( TDynArray< CTimelineItemBlocking* >::iterator elemIter = m_elements.Begin();
		elemIter != m_elements.End(); ++elemIter )
	{
		( *elemIter )->SetHighlight( false, 0.0f );
	}

	if( ! IsOneSelected() )
	{
		m_mediator->OnTimeline_SelectionChanged( m_section, nullptr );
	}
	else
	{
		ITimelineItem* item = m_selectedItems[ 0 ];

		if( CTimelineItemBlocking* elementItem = dynamic_cast< CTimelineItemBlocking* >( item ) )
		{
			m_mediator->OnTimeline_SelectionChanged( m_section, item, false );
		}
		else if( CTimelineItemEvent* eventItem = dynamic_cast< CTimelineItemEvent* >( item ) )
		{
			Bool goToEvent = !RIM_IS_KEY_DOWN( IK_Alt ) && !m_midlleMouseDown;
			m_mediator->OnTimeline_SelectionChanged( m_section, eventItem, goToEvent );

			// Highlight element item
			ASSERT( eventItem->GetElementItem() );

			eventItem->GetElementItem()->SetHighlight( true, eventItem->GetEvent()->GetStartPosition() );
		}
	}

	LockSetSelection( false );

	m_eventsLinker.OnSelectionChanged( m_selectedItems );
}

void CEdDialogTimeline::EditProperties( ITimelineItem* item, CEdPropertiesPage& propertiesPage ) const
{
	if( item == NULL )
	{
		propertiesPage.SetNoObject();
		return;
	}

	propertiesPage.SetReadOnly( !item->IsEditable() );

	if( CTimelineItemBlockingEvent* blockingEvent = dynamic_cast< CTimelineItemBlockingEvent* >( item ) )
	{
		CStorySceneBlockingElement* blockingElement = Cast< CStorySceneBlockingElement >( blockingEvent->GetElement() );

		ASSERT( blockingElement->GetEvent() != NULL );
		propertiesPage.SetObject( blockingElement->GetEvent() );
	}
	else if( CTimelineItemBlocking* lineItem = dynamic_cast< CTimelineItemBlocking* >( item ) )
	{
		propertiesPage.SetObject( lineItem->GetElement() );
	}
	else if( CTimelineItemEvent* eventItem = dynamic_cast< CTimelineItemEvent* >( item ) )
	{
		propertiesPage.SetObject( eventItem->GetEvent() );
	}
}

void CEdDialogTimeline::RefreshPlayPauseIcon( Bool flag )
{
	m_timelineToolbar->SetToolNormalBitmap( m_timelinePlayTool, !flag ? m_timelinePauseIcon : m_timelinePlayIcon );
}

void CEdDialogTimeline::SetSection( const CStorySceneSection* section )
{
	if ( m_section == section )
	{
		return;
	}

	StoreLayout();

	m_section = section;

	m_selectedItems.Clear();

	ClearItems();
	m_elements.Clear();
	m_activeRangeDuration = FillWithSectionContents( section );

	SetTimeLimits( 0.0f, m_activeRangeDuration );

	RecreateTracks();
}

void CEdDialogTimeline::OnItemInternalChanged( CTimelineItemBlocking* item )
{
	
}

void CEdDialogTimeline::OnPauseItemDurationChange( CTimelineItemBlocking* item, Float durationChange )
{
	// DIALOG_TOMSIN_TODO - zmiana duration pasuy znie zmienia eventow (ich abs time postition)
	OnBlockingItemDurationChange( item, durationChange );
}

void CEdDialogTimeline::OnBlockingItemDurationChange( CTimelineItemBlocking* item, Float durationChange )
{
	Int32 itemIndex = m_elements.GetIndex( item );

	if ( itemIndex == -1 )
	{
		return;
	}

	// Update start times of next scene elements
	for( Uint32 i = itemIndex + 1; i < m_elements.Size(); ++i )
	{
		m_elements[ i ]->SetStart( m_elements[ i ]->GetStart() + durationChange, true );
		//m_mediator->OnTimeline_EventChanged( m_section, m_elements[ i ]->GetEvent() );
	}

	// Update total scene section time
	m_activeRangeDuration += durationChange;
}

void CEdDialogTimeline::RecreateTracks()
{
	if( m_section == NULL )
	{
		return;
	}

	// Combatibility conversion if track names to properly track support track groups
	for( TDynArray< ITimelineItem* >::iterator itemIter = m_items.Begin();
		itemIter != m_items.End(); ++itemIter )
	{
		ITimelineItem* item = *itemIter;

		if ( item == NULL )
		{
			continue;
		}

		String newTrackName = item->GetTrackName();
		if ( newTrackName == TXT( ".Camera" ) )
		{
			newTrackName = TIMELINE_DEFAULT_CAMERA_TRACK;
			item->SetTrackName( newTrackName );
			m_mediator->HACK_GetStoryScene()->MarkModified();
		}
		else if ( newTrackName.ContainsCharacter( TXT( '\n' ) ) == true )
		{
			newTrackName.ReplaceAll( TXT( "\n" ), TXT( "." ) );
			item->SetTrackName( newTrackName );
			m_mediator->HACK_GetStoryScene()->MarkModified();
		}
	}

	// Run base method
	CEdTimeline::RecreateTracks();
	if( Cast< CStorySceneCutsceneSection>( m_section ) )
	{
		Int32 csTrack = GetTrackIndex( CUTSCENE_LINES_TRACK );
		if ( csTrack != -1 )
		{
			Track* track = m_tracks[csTrack];
			m_defaultGroup->RemoveTrack(track);
			m_pinnedGroup->AddTrack(track);		
		}
	}
	
	AddTrack( CAMERA_LIGHTS_TRACK );

	AddTrack(TIMELINE_DEFAULT_CAMERA_TRACK);
	Track* cameraTrack = m_tracks[ GetTrackIndex( TIMELINE_DEFAULT_CAMERA_TRACK ) ];
	m_defaultGroup->RemoveTrack(cameraTrack);
	m_pinnedGroup->AddTrack(cameraTrack);
	
		
	TDynArray< CName > voiceTags;
	m_mediator->OnTimeline_GetVoiceTagsForCurrentSetting( voiceTags );
	
	for(auto i = voiceTags.Begin() ; i != voiceTags.End() ; i++)
	{
		String groupPrefix = (*i).AsString();
		groupPrefix += GROUP_SEPARATOR;

		AddTrack( groupPrefix + TXT("additives") );
		AddTrack( groupPrefix + TXT("animations") );
		AddTrack( groupPrefix + TXT("lookats") );
		AddTrack( groupPrefix + TXT("mimics") );
		AddTrack( groupPrefix + TXT("mimics2") );
		AddTrack( groupPrefix + TXT("placement") );
		AddTrack( groupPrefix + TXT("ik") );
	}

	// add prop tracks
	const TDynArray< CStorySceneProp* >& propDefs = m_mediator->HACK_GetStoryScene()->GetScenePropDefinitions(); // TODO - don't use direct calls
	for( Uint32 i=0; i<propDefs.Size(); ++i )
	{
		const CStorySceneProp* def = propDefs[ i ];
		if( !def || def->m_id == CName::NONE )
		{
			continue;
		}

		String trackName = PROP_TRACK;
		trackName += GROUP_SEPARATOR;
		trackName += def->m_id.AsString();
		AddTrack( trackName );
	}

	// add effect tracks
	const TDynArray< CStorySceneEffect* >& fxDefs = m_mediator->HACK_GetStoryScene()->GetSceneEffectDefinitions(); // TODO - don't use direct calls
	for( Uint32 i=0; i<fxDefs.Size(); ++i )
	{
		const CStorySceneEffect* def = fxDefs[ i ];
		if( !def || def->m_id == CName::NONE )
		{
			continue;
		}

		String trackName = EFFECT_TRACK;
		trackName += GROUP_SEPARATOR;
		trackName += def->m_id.AsString();
		AddTrack( trackName );
	}

	// add lights tracks
	const TDynArray< CStorySceneLight* >& lightDefs = m_mediator->HACK_GetStoryScene()->GetSceneLightDefinitions(); // TODO - don't use direct calls
	for( Uint32 i=0; i<lightDefs.Size(); ++i )
	{
		const CStorySceneLight* def = lightDefs[ i ];
		if( !def || def->m_id == CName::NONE )
		{
			continue;
		}

		String trackName = LIGHT_TRACK;
		trackName += GROUP_SEPARATOR;
		trackName += def->m_id.AsString();
		AddTrack( trackName );
	}

	SortTracks();
	RestoreLayout();
}

void CEdDialogTimeline::SortTracks()
{
	CEdTimeline::SortTracks();

	Int32 defaultTrackIndex = GetTrackIndex( TIMELINE_DEFAULT_TRACK_NAME );
	if ( defaultTrackIndex != -1 && defaultTrackIndex != 0 )
	{
		Track* defaultTrack = m_tracks[ defaultTrackIndex ];
		m_tracks.Remove( defaultTrack );
		m_tracks.Insert( 0, defaultTrack );
	}

	Int32 cameraTrackIndex = GetTrackIndex( TIMELINE_DEFAULT_CAMERA_TRACK );
	if ( cameraTrackIndex != -1 && cameraTrackIndex != 1 )
	{
		Track* cameraTrack = m_tracks[ cameraTrackIndex ];
		m_tracks.Remove( cameraTrack );
		m_tracks.Insert( 1, cameraTrack );
	}
}

/*
Creates track object of appropriate dynamic type.

For CEdDialogTimeline created track object is of DialogTrack class.
*/
DialogTrack* CEdDialogTimeline::CreateTrack( String name, Uint32 depth, bool isGroup ) const
{
	return new DialogTrack( name, depth, isGroup );
}

void CEdDialogTimeline::RefreshSection()
{
	TDynArray< CStorySceneEvent* > marked;
	TDynArray< CStorySceneEvent* > locked;
	GetMarkedLockedEvents( marked, locked );

	TDynArray< String >	lockedTracks;
	GetLockedTracks( lockedTracks );

	// store current layout (SetSection() would do it but there's this "m_section = NULL" hack few lines below)
	StoreLayout();

	// TODO
	const CStorySceneSection* temp = m_section;
	m_section = NULL;
	SetSection( temp );

	for ( Uint32 i = 0; i < m_tracks.Size(); i++ ) 
	{
		DialogTrack* track = dynamic_cast< DialogTrack* >( m_tracks[i] );
		if ( track && lockedTracks.Exist( track->m_name ) )
		{
			track->m_isLocked = true;
		}	
	}

	for ( Uint32 i = 0; i < m_items.Size(); i++  )
	{
		CTimelineItemEvent* item = dynamic_cast<CTimelineItemEvent*>( m_items[i] );
		if( item && item->GetEvent() )
		{
			if( marked.Exist( item->GetEvent() ) )
			{
				item->SetState( DialogTimelineItems::EVENTSTATE_MARKED );
			}
			if( locked.Exist( item->GetEvent() ) )
			{
				item->SetState( DialogTimelineItems::EVENTSTATE_LOCKED );
			}			
		}
	}

	NotifyBufferIsInvalid();
}

void CEdDialogTimeline::SerializeItem( ITimelineItem* item, IFile& writer )
{
	CStorySceneEvent* event = static_cast< CTimelineItemEvent* >( item )->GetEvent();
	ASSERT( event != NULL );

	CName eventType = event->GetClass()->GetName();
	writer << eventType;
	Float absStartTime = item->GetStart();
	writer << absStartTime;
	event->Serialize( writer );
}

/*

See DeserializeEvent() docs for more info on "DeserializeItem() vs DeserializeEvent()".
*/
ITimelineItem* CEdDialogTimeline::DeserializeItem( IFile& reader )
{
	ASSERT( m_section != NULL );

	// Read type
	CName eventType;
	reader << eventType;

	// Read abs start time
	Float absStartTime = 0.0f;
	reader << absStartTime;

	// Find type in RTTI
	CClass* theClass = SRTTI::GetInstance().FindClass( eventType );
	ASSERT( theClass != NULL );

	// Create object
	CStorySceneEvent* storyEvent = theClass->CreateObject< CStorySceneEvent >() ;
	ASSERT( storyEvent != NULL );

	// Serialize object
	storyEvent->Serialize( reader );

	// Interpolation and blend events cannot be currently copied/pasted. CTimelineItemBlend::IsCopyable()
	// and CTimelineItemInterpolationEvent::IsCopyable() return false so we shouldn't encounter them here.
	RED_FATAL_ASSERT( !IsType< CStorySceneEventInterpolation >( storyEvent ), "CEdDialogTimeline::DeserializeItem(): encountered interpolation event." );
	RED_FATAL_ASSERT( !IsType< CStorySceneEventBlend >( storyEvent ), "CEdDialogTimeline::DeserializeItem(): encountered blend event." );

	// Copy stops being blend/interpolation key (it it was one).
	storyEvent->SetBlendParentGUID( CGUID::ZERO );
	storyEvent->SetInterpolationEventGUID( CGUID::ZERO );

	// Copy stops being linked original link parent (if it was linked).
	// No events are linked to copy so copy shouldn't list them as link children.
	storyEvent->ResetLinkParent();
	storyEvent->RemoveAllLinkChildren();

	storyEvent->RegenerateGUID();

	// Find scene element with which to associate the story event and timeline item.
	// If no scene element corresponds to absStartTime then we just take first element
	// as we have to specify some element when creating timeline event item. Note that
	// it's not nice to do such things in DeserializeItem() and we should change this
	// eventually. Example situation in which we won't find scene element that corresponds
	// to absStartTime: 1. copy an event placed at the of a section, 2. delete any pause
	// element, 3. paste event anywhere.
	CTimelineItemBlocking* element = FindSceneElement( absStartTime );
	if( !element )
	{
		element = m_elements[ 0 ];
	}

	storyEvent->SetSceneElement( element->GetElement() );

	m_mediator->OnTimeline_AddEvent( m_section, storyEvent, GetSectionVariantId(), false );

	// Rebuild player to recreate playing plan as some code accesses event instance
	// data before rebuild request (issued by OnTimeline_AddEvent()) is processed.
	m_mediator->OnTimeline_RebuildPlayer();

	ITimelineItem* item = CreateTimelineItemEvent( storyEvent, element );
	item->SetStart( absStartTime, true );

	return item;
}

/*
Deserializes event.

DeserializeEvent() vs DeserializeItem()
DeserializeItem() is doing too much - it deserializes event, then it adds it to story scene section,
connects the event with scene element and finally it creates timeline item. This is bad as in various
cases we want these steps to be done in different order and we want to perform some additional operations
between some steps (e.g. regenerating pasted event GUID before it's added to story scene section).
DeserializeEvent() does only one thing - it deserializes and event. The rest is to be done by the caller.
Using DeserializeEvent() is preferable. DeserializedItem() is going to be gradually phased out.
*/
CStorySceneEvent* CEdDialogTimeline::DeserializeEvent( IFile& reader, Float& outAbsStartTime )
{
	// Read type
	CName eventType;
	reader << eventType;

	// Read abs start time
	reader << outAbsStartTime;

	// Find type in RTTI
	CClass* theClass = SRTTI::GetInstance().FindClass( eventType );
	ASSERT( theClass != NULL );

	// Create object
	CStorySceneEvent* storyEvent = theClass->CreateObject< CStorySceneEvent >() ;
	ASSERT( storyEvent != NULL );

	// Serialize object
	storyEvent->Serialize( reader );

	return storyEvent;
}

Bool CEdDialogTimeline::CalculateEventItemRect( const ITimelineItem* item, wxRect& rect )
{
	ASSERT( item );

	Track* track = GetItemTrack( item );
	TimelineImpl::CDrawGroupTracks* drwGrp = GetTrackDrawGroup( track );
	if ( drwGrp->GetDispTrackIndex( track ) )
	{
		// compute rect x and width
		const Int32 startPos = CalculatePixelPos( item->GetStart() );
		const Int32 endPos = CalculatePixelPos( Min( item->GetStart() + item->GetDuration(), GetActiveRangeDuration() ) );
		rect.x = startPos;
		rect.width = endPos - startPos;

		// compute rect y and height
		wxPoint trackLocalPos = drwGrp->GetDispTrackLocalPos( track );
		Int32 trackHeight  = drwGrp->GetTrackHeight(track);
		Int32 itemHeight   = drwGrp->GetItemHeight(track);
		Int32 itemPosY = trackLocalPos.y + trackHeight - itemHeight - 2;
		rect.y = itemPosY;
		rect.height = itemHeight;

		// event rect computed
		return true;
	}

	// couldn't compute event rect as event is on a track that is not displayed
	return false;
}

void CEdDialogTimeline::OnLeftMouseDoubleClick( Int32 x, Int32 y )
{
	if ( !IsTimelineEditingEnabled() )
	{
		// Creating events and modifying existing ones is only allowed if section is approved.
		return;
	}

	if ( m_selectedTrack >= 0 && x < TIMELINE_TRACK_BTN_WIDTH )
	{
		ASSERT( m_selectedTrack < ( Int32 ) m_tracks.Size() );

		String trackName = m_tracks[ m_selectedTrack ]->m_name;

		String actorNameStr = trackName.StringBefore( TXT(".") );
		if ( actorNameStr.Empty() && trackName != TIMELINE_DEFAULT_CAMERA_TRACK && trackName != CAMERA_LIGHTS_TRACK )
		{
			return;
		}

		CName actorName( actorNameStr );

		CTimelineItemBlocking* selected = FindSceneElement( m_timeSelectorTimePos );
		if ( !selected )
		{
			return;
		}

		Float position = ( m_timeSelectorTimePos - selected->GetStart() ) / selected->GetDuration();

		size_t index = 0;

		if( actorNameStr == PROP_TRACK )
		{
			String propNameStr = trackName.StringAfter( GROUP_SEPARATOR_STR );
			CName propName = CName( propNameStr );

			CreateEvent( ID_ADD_SCENE_PROP_PLACEMENT_EVENT, trackName, position, propName, selected );
		}
		if( actorNameStr == LIGHT_TRACK )
		{
			String lightNameStr = trackName.StringAfter( GROUP_SEPARATOR_STR );
			CName lightName = CName( lightNameStr );

			CreateEvent( ID_ADD_LIGHT_PROPERTIES_EVENT, trackName, position, lightName, selected );
		}
		else if ( trackName.FindSubstring( TXT("animations"), index ) )
		{
			TDynArray< CName > extraData;
			bool unfiltered = RIM_IS_KEY_DOWN( IK_Shift );
			ShowMenuWithAnimationBodyExtraData( actorName, extraData, unfiltered );
			if ( extraData.Empty() )
			{
				CreateEvent( ID_ADD_ANIMATION_EVENT, trackName, position, actorName, selected );
			}
			else
			{
				CreateEvent( ID_ADD_ANIMATION_EVENT, trackName, position, actorName, selected, &extraData );
			}
		}
		else if ( trackName.FindSubstring( TIMELINE_DEFAULT_CAMERA_TRACK, index ) )
		{
			if( m_section->IsA< CStorySceneCutsceneSection >() )
			{
				wxMessageBox( TXT( "Camera event can't be created in cutscene section as camera is controlled by cutscene itself." ) );
			}
			else
			{
				CreateEvent( ID_ADD_CAMERA_EVENT, trackName, position, actorName, selected );
			}
		}
		else if ( trackName.FindSubstring( TXT("lookats"), index ) )
		{
			if ( wxIsCtrlDown() )
			{
				CreateEvent( ID_ADD_LOOKAT_DURATION_EVENT, trackName, position, actorName, selected );
			}
			else if ( wxIsAltDown() )
			{
				CreateEvent( ID_ADD_LOOKAT_GAMEPLAY_EVENT, trackName, position, actorName, selected );
			}
			else
			{
				CreateEvent( ID_ADD_LOOKAT_EVENT, trackName, position, actorName, selected );
			}
		}
		else if ( trackName.FindSubstring( TXT("mimics"), index ) || trackName.FindSubstring( TXT("mimics2"), index ) )
		{
			if ( GetKeyboardModifiers() && wxIsCtrlDown() )
			{
				CreateEvent( ID_ADD_MIMICS_EVENT, trackName, position, actorName, selected );
			}
			else
			{
				TDynArray< CName > extraData;
				ShowMenuWithAnimationMimicsExtraData( actorName, extraData );

				if ( extraData.Empty() )
				{
					CreateEvent( ID_ADD_MIMICS_ANIMATION_EVENT, trackName, position, actorName, selected );
				}
				else
				{
					CreateEvent( ID_ADD_MIMICS_ANIMATION_EVENT, trackName, position, actorName, selected, &extraData );
				}
			}
			
		}
		else if ( trackName.FindSubstring( TXT("additives"), index ) )
		{
			CreateEvent( ID_ADD_ADDITIVE_ANIMATION_EVENT, trackName, position, actorName, selected );
		}
		else if ( trackName.FindSubstring( TXT("placement"), index ) )
		{
			CreateEvent( ID_ADD_PLACEMENT_OVERRIDE_EVENT, trackName, position, actorName, selected );
		}
		else if ( trackName.FindSubstring( TXT("ik"), index ) )
		{
			CreateEvent( ID_ADD_POSE_KEY_EVENT, trackName, position, actorName, selected );
		}
		else if( trackName == CAMERA_LIGHTS_TRACK )
		{
			CreateEvent( ID_ADD_CAMERALIGHT_EVENT, trackName, position, actorName, selected );
		}
	}
	else if ( IsOneSelected() )
	{
		ITimelineItem* item = m_selectedItems[ 0 ];

		if( CTimelineItemEvent* eventItem = dynamic_cast< CTimelineItemEvent* >( item ) )
		{
			CStorySceneEvent* evt = eventItem->GetEvent();

			if ( IsType< CStorySceneEventAnimation >( evt ) )
			{
				CStorySceneEventAnimation* animEvent = static_cast< CStorySceneEventAnimation* >( evt );
				ShowMenuForAnimationData( animEvent, wxIsCtrlDown(), wxIsShiftDown() );
			}
			else if ( IsType< CStorySceneEventMimicsAnim >( evt ) )
			{
				CStorySceneEventMimicsAnim* animEvent = static_cast< CStorySceneEventMimicsAnim* >( evt );
				ShowMenuForAnimationData( animEvent, wxIsCtrlDown() );
			}
			else if ( IsType< CStorySceneEventChangePose >( evt ) )
			{
				CStorySceneEventChangePose* animEvent = static_cast< CStorySceneEventChangePose* >( evt );
				ShowMenuForAnimationData( animEvent, wxIsCtrlDown() );
			}
		}
	}
}

void CEdDialogTimeline::OnStateSelected( wxPoint globalPos )
{

}

void CEdDialogTimeline::OnStateChanged( CEdTimeline::States prevState, CEdTimeline::States newState )
{
	if ( prevState == STATE_DRAGGING || prevState == STATE_RESIZING )
	{
		OnCheckAutoLink();

		m_mediator->OnTimeline_StateChanged();
	}
}

void CEdDialogTimeline::OnTrackDoubleClicked()
{
	
}

void CEdDialogTimeline::OnCheckAutoLink()
{
	const Float THR = 0.001f;

	const Uint32 num = m_items.Size();
	for ( Uint32 i=0; i<num; ++i )
	{
		if ( CTimelineItemEvent* itemEvt = dynamic_cast< CTimelineItemEvent* >( m_items[ i ] ) )
		{
			if ( !itemEvt->GetEvent()->HasLinkParent() && itemEvt->AutoLinkToThisEvent() )
			{
				const Float parentItemPos = itemEvt->GetStart();

				for ( Uint32 j=0; j<num; ++j )
				{
					if ( CTimelineItemEvent* childItemEvt = dynamic_cast< CTimelineItemEvent* >( m_items[ j ] ) )
					{
						if ( childItemEvt != itemEvt && !childItemEvt->GetEvent()->HasLinkParent() && !childItemEvt->AutoLinkToThisEvent() && !IsInterpolationEvent( childItemEvt ) )
						{
							const Float itemPosStart = childItemEvt->GetStart();
							const Float itemPosEnd = childItemEvt->GetStart() + childItemEvt->GetDuration();
							if ( MAbs( itemPosStart - parentItemPos ) < THR )
							{
								m_eventsLinker.LinkEvent( itemEvt, childItemEvt );
							}
							else if ( MAbs( itemPosEnd - parentItemPos ) < THR )
							{
								m_eventsLinker.LinkEvent( itemEvt, childItemEvt );
							}
						}
					}
				}
			}
		}
	}
}

void CEdDialogTimeline::GatherAllEventItems( TDynArray< CTimelineItemEvent* >& eventItems ) const
{
	for ( TDynArray< ITimelineItem* >::const_iterator itemIter = m_items.Begin();
		itemIter != m_items.End(); ++itemIter )
	{
		CTimelineItemEvent* dialogEventItem = dynamic_cast< CTimelineItemEvent* >( *itemIter );
		if ( dialogEventItem != NULL )
		{
			CTimelineItemEventGroup* groupEventItem = dynamic_cast< CTimelineItemEventGroup* >( dialogEventItem );
			if ( groupEventItem != NULL )
			{
				// Look within groups
				const TDynArray< CTimelineItemEvent* >& embeddedItems = groupEventItem->GetEmbeddedItems();
				for ( TDynArray< CTimelineItemEvent* >::const_iterator embeddedIter = embeddedItems.Begin();
					embeddedIter != embeddedItems.End(); ++embeddedIter )
				{
					eventItems.PushBack( *embeddedIter );
				}
			}
			else
			{
				eventItems.PushBack( dialogEventItem );
			}
		}
	}
}

/*
Gets events (from current section variant) that are associated with specified element.

\param element Element. Must not be 0.
\param events (out) Container to which element events are to be stored. Not cleared before use.
\return Number of element events (number of events pushed to container).
*/
Uint32 CEdDialogTimeline::GetElementEvents( const DialogTimelineItems::CTimelineItemBlocking* element, TDynArray< DialogTimelineItems::CTimelineItemEvent* >& events ) const
{
	Uint32 numElementEvents = 0;

	Uint32 numItems = m_items.Size();
	for( Uint32 i = 0; i < numItems; ++i )
	{
		ITimelineItem* item = m_items[ i ];
		if( IsEvent( item ) )
		{
			CTimelineItemEvent* ev = static_cast< CTimelineItemEvent* >( item );
			if ( ev->GetElementItem() == element )
			{
				events.PushBack( ev );
				++numElementEvents;
			}
		}
	}

	return numElementEvents;
}

void CEdDialogTimeline::CancelSelection()
{
	m_selectedItems.Clear();
	SelectionChanged();
}

void CEdDialogTimeline::SelectOnlyOneItem( const CStorySceneEvent* e )
{
	CancelSelection();

	const Uint32 numItems = m_items.Size();
	for( Uint32 i = 0; i < numItems; ++i )
	{
		ITimelineItem* item = m_items[ i ];
		if( IsEvent( item ) )
		{
			CTimelineItemEvent* ev = static_cast< CTimelineItemEvent* >( item );
			if ( ev->GetEvent() == e )
			{
				m_selectedItems.PushBack( ev );
			}
		}
	}

	SelectionChanged();
}

void CEdDialogTimeline::ToggleDebugItems()
{
	m_debugItems = !m_debugItems;
}

void CEdDialogTimeline::ToggleCameraTool( Bool previewMode, Bool freeMode, Bool editMode )
{
	m_timelineToolbar->ToggleTool( XRCID( "PreviewModeButton"), previewMode );
	m_timelineToolbar->ToggleTool( XRCID( "FreeCameraModeButton"), freeMode );
	m_timelineToolbar->ToggleTool( XRCID( "EditModeButton"), editMode );
}

Float CEdDialogTimeline::FillWithSectionContents( const CStorySceneSection* section )
{
	m_eventsLinker.Reset();

	if ( !section )
	{
		return 1.f;
	}

	const Uint32 numOfElements = section->GetNumberOfElements();

	// DIALOG_TOMSIN_TODO
	// Calculate elements lengths
	Float totalDuration = 0.0f;//         start, duration
	THashMap< CStorySceneElement*, TPair< Float, Float > > elements;
	for( Uint32 i = 0; i < numOfElements; ++i )
	{
		const CStorySceneElement* element = m_section->GetElement( i );
		ASSERT( element != NULL );

		CTimelineItemBlocking* blockingItem = CreateBlockingTimelineItem( const_cast< CStorySceneElement* >( element ), totalDuration, 0 );
		if ( blockingItem != NULL )
		{
			totalDuration += blockingItem->GetDuration();
			AddItem( blockingItem );
			m_elements.PushBack( blockingItem );
		}
	}

	// DIALOG_TOMSIN_TODO
	// Include choice
	const CStorySceneChoice* choice = section->GetChoice();
	if ( choice != NULL )
	{
		CTimelineItemBlocking* blockingItem = CreateBlockingTimelineItem( const_cast< CStorySceneChoice*>( choice ), totalDuration, 0 );
		if ( blockingItem != NULL )
		{
			totalDuration += blockingItem->GetDuration();
			AddItem( blockingItem );
			m_elements.PushBack( blockingItem );
		}
	}

	const Uint32 currentLocaleId = SLocalizationManager::GetInstance().GetCurrentLocaleId();
	const CStorySceneSectionVariantId sectionVariantId = m_section->GetVariantUsedByLocale( currentLocaleId );

	// Create timeline event for each scene event.
	const TDynArray< CGUID >& evGuids = m_section->GetEvents( sectionVariantId );
	for ( auto evGuid : evGuids )
	{
		const CStorySceneEvent* sceneEvent = m_section->GetEvent( evGuid );
		ASSERT( sceneEvent );

		// Find timeline element item with which new timeline event item is to be associated.
		CTimelineItemBlocking* elementItem = NULL;
		for ( auto timelineElementIt = m_elements.Begin(), timelineElementEnd = m_elements.End(); timelineElementIt != timelineElementEnd; ++timelineElementIt )
		{
			if( ( *timelineElementIt )->GetElement() == sceneEvent->GetSceneElement() )
			{
				elementItem = *timelineElementIt;
				break;
			}
		}

		ASSERT( elementItem != NULL );
		if ( elementItem != NULL )
		{
			AddItem( CreateTimelineItemEvent( const_cast< CStorySceneEvent* >( sceneEvent ), elementItem ) );
		}
	}

	// Initialize event items associated with interpolation events (it's safe to do this here
	// as we can guarantee that all items associated with all key events are already created).
	TDynArray< ITimelineItem* > keys; // TODO: CTimelineItemEvent* (when we change CTimelineItemInterpolationEvent::m_keys type)
	for( auto evGuid : evGuids )
	{
		if( const CStorySceneEventInterpolation* scInterpolationEv = Cast< const CStorySceneEventInterpolation >( m_section->GetEvent( evGuid ) ) )
		{
			// Item can be null because of section's context
			if ( CTimelineItemInterpolationEvent* tiInterpolationEv = dynamic_cast< CTimelineItemInterpolationEvent* >( FindItemEvent( scInterpolationEv ) ) )
			{
				tiInterpolationEv->Reinitialize();
			}
		}
	}

	if ( section->IsA< CStorySceneCutsceneSection >() )
	{
		const CStorySceneCutsceneSection* csSection = static_cast< const CStorySceneCutsceneSection* >( section );
		const CCutsceneTemplate* cs = csSection->GetCsTemplate();
		if ( cs && !csSection->GetChoice() )
		{
			SCENE_ASSERT( cs->GetDuration() == totalDuration );
		}
	}

	return totalDuration;
}

void CEdDialogTimeline::ShowItemMenu()
{
	if ( m_selectedItems.Empty() == true )
	{
		return;
	}

	wxMenu* menu = PrepareItemMenu();

	Bool anyNormal = false;
	Bool anyMarked = false;
	Bool anyLocked = false;

	for ( Uint32 i = 0; i < m_selectedItems.Size(); ++i )
	{
		anyNormal = anyNormal || ( m_selectedItems[i] ? m_selectedItems[i]->GetState() == EVENTSTATE_NORMAL : false );
		anyMarked = anyMarked || ( m_selectedItems[i] ? m_selectedItems[i]->GetState() == EVENTSTATE_MARKED : false );
		anyLocked = anyLocked || ( m_selectedItems[i] ? m_selectedItems[i]->GetState() == EVENTSTATE_LOCKED : false );
	}

	ITimelineItem* eventItem = m_selectedItems[ 0 ];
	if ( eventItem )
	{
		CTimelineItemLineElement* dialogLine = dynamic_cast< CTimelineItemLineElement* >( m_selectedItems[ 0 ] );
		if( dialogLine )
		{
			if( !dialogLine->IsBackgroundLine() )
			{
				menu->Append( ID_SET_AS_BACKGROUND_LINE, wxT( "Set as background line" ) );
			}
			else
			{
				menu->Append( ID_SET_AS_NONBACKGROUND_LINE, wxT( "Set as non-background line" ) );
			}
		}

		CTimelineItemBlocking* element = dynamic_cast< CTimelineItemBlocking* >( m_selectedItems[ 0 ] );
		if( element ) 
		{
			menu->Append( ID_ADD_PAUSE_BEFORE, wxT( "Add pause before" ) );
			menu->Append( ID_ADD_PAUSE_AFTER, wxT( "Add pause after" ) );
			menu->AppendSeparator();
		}
		
		menu->Append( ID_NORMAL_EVENT, wxT( "Normal" ), wxEmptyString, wxITEM_CHECK )->Check( anyNormal );
		menu->Append( ID_MARK_EVENT, wxT( "Marked" ), wxEmptyString, wxITEM_CHECK )->Check( anyMarked );
		menu->Append( ID_LOCK_EVENT, wxT( "Locked" ), wxEmptyString, wxITEM_CHECK )->Check( anyLocked );
	}

	PopupMenu( menu );

	delete menu;
}

void CEdDialogTimeline::OnMouseWheel( wxMouseEvent& event )
{
	if ( event.ControlDown() == true && event.ShiftDown() == true )
	{
		Float clipRollOffset = event.GetWheelRotation() / 1000.0f;
		PerformClipRoll( clipRollOffset );

		event.Skip( false );
		return;
	}
	event.Skip( true );
}

void CEdDialogTimeline::AddGenericEventsEntries( TDynArray< TPair< Int32, SEtorySceneEventGenericCreationData* > >& genericObjs, wxMenu* menu, const String& category )
{
	for ( Int32 i = genericObjs.SizeInt() - 1; i >= 0; --i )
	{
		const SEtorySceneEventGenericCreationData* data = genericObjs[i].m_second;
		if ( data->m_menuCategory == category )
		{				
			menu->Append( genericObjs[i].m_first, data->m_menuEntry.AsChar()  );
			Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdDialogTimeline::OnNewEvent, this, genericObjs[i].m_first );
			genericObjs.RemoveAtFast( i );
		}
	}
}

Bool CEdDialogTimeline::IsCanvasMenuEnabled() const
{
	// If section is not approved then canvas menu is disabled
	// to disallow deleting, pasting, linking events etc.
	return IsTimelineEditingEnabled();
}

Bool CEdDialogTimeline::IsTrackMenuEnabled() const
{
	// If section is not approved then track menu is disabled
	// to disallow creation of new events.
	return IsTimelineEditingEnabled();
}

Bool CEdDialogTimeline::IsTrackButtonMenuEnabled() const
{
	// If section is not approved then track button menu is disabled
	// to disallow creating, deleting and renaming tracks.
	return IsTimelineEditingEnabled();
}

Bool CEdDialogTimeline::IsItemMenuEnabled() const
{
	// If section is not approved then item menu is disabled
	// to disallow some operations that modify event.
	return IsTimelineEditingEnabled();
}

void CEdDialogTimeline::FillTrackMenu( const String& name, wxMenu* menu )
{
	TDynArray< TPair< Int32, SEtorySceneEventGenericCreationData* > >	genericObjs = m_genericObjsData;

	if( m_cursorTimePos < 0.0f || m_cursorTimePos > m_activeRangeDuration)
	{
		// pause submenu
		wxString subMenuName = wxT( "Create pause" );
		wxMenu* pauseSubMenu = new wxMenu();
		pauseSubMenu->Append( ID_ADD_PAUSE_5, wxT( "5s" ) );
		pauseSubMenu->Append( ID_ADD_PAUSE_15, wxT( "15s" ) );
		pauseSubMenu->Append( ID_ADD_PAUSE_30, wxT( "30s" ) );
		menu->AppendSubMenu( pauseSubMenu, subMenuName );
	}
	else
	{
		// Create "Add camera" menu. Don't create it if section is a cutscene section as in that case
		// we don't want user to create any cameras - it's the cutscene itself that controls cameras.
		if( !m_section->IsA< CStorySceneCutsceneSection >() )
		{
			// camera submenu
			wxMenu* cameraSubMenu = new wxMenu();
			wxString camSubMenuName = wxT( "Add camera" );
			cameraSubMenu->Append( ID_ADD_CAMERA_EVENT, wxT( "from view" ) );
			cameraSubMenu->Append( ID_ADD_CUSTOM_CAMERA_INSTANCE, wxT( "from list" ) );
			cameraSubMenu->Append( ID_ADD_CAMERA_ANIMATION_EVENT, wxT( "animation" ) );
			cameraSubMenu->Append( ID_ADD_GAMEPLAY_CAMERA_EVENT, wxT( "gameplay" ) );
			cameraSubMenu->Append( ID_ADD_START_BLEND_TO_GAMEPLAY_CAMERA_EVENT, wxT( "start-blend-to-gameplay" ) );
			if ( FindDurationBlendItemByEventClass( ClassID< CStorySceneEventEnhancedCameraBlend >(), m_timeSelectorTimePos ) )
			{
				cameraSubMenu->Append( ID_ADD_ENHANCED_BLEND_INTERPOLATED_CAMERA_EVENT, wxT( "blend interpolated" ) );
			}
			AddGenericEventsEntries( genericObjs, cameraSubMenu, camSubMenuName.wc_str() );
			menu->AppendSubMenu( cameraSubMenu, wxT( "Add camera" ) );
		}

		// actor submenu
		wxMenu* actorSubMenu = new wxMenu();
		wxString actorSubMenuName = wxT( "Add actor event" );
		actorSubMenu->Append( ID_ADD_ANIMATION_EVENT, wxT( "animation" ) );
		actorSubMenu->Append( ID_ADD_ADDITIVE_ANIMATION_EVENT, wxT( "additive animation" ) );
		actorSubMenu->Append( ID_ADD_OVERRIDE_ANIMATION_EVENT, wxT( "override animation" ) );
		actorSubMenu->Append( ID_ADD_ENTER_ACTOR_EVENT, wxT( "enter" ) );
		actorSubMenu->Append( ID_ADD_EXIT_ACTOR_EVENT, wxT( "exit" ) );
		actorSubMenu->Append( ID_ADD_CHANGE_POSE, wxT( "change pose" ) );
		actorSubMenu->Append( ID_ADD_LOOKAT_DURATION_EVENT, wxT( "look at (point cloud)" ) );
		actorSubMenu->Append( ID_ADD_LOOKAT_EVENT, wxT( "look at tick (old)" ) );
		actorSubMenu->Append( ID_ADD_LOOKAT_GAMEPLAY_EVENT, wxT( "look at (gameplay)" ) );
		actorSubMenu->Append( ID_ADD_POSE_KEY_EVENT, wxT( "pose key" ) );
		actorSubMenu->Append( ID_ADD_VISIBILITY_EVENT, wxT( "visibility" ) );
		actorSubMenu->Append( ID_ADD_EVENT_LODOVERRIDE, wxT( "LOD override" ) );
		actorSubMenu->Append( ID_ADD_APPEARANCE_EVENT, wxT( "appearance" ) );
		//actorSubMenu->Append( ID_ADD_ROTATE_TO_CAMERA_EVENT, wxT( "rotate" ) );
		actorSubMenu->Append( ID_ADD_PLACEMENT_OVERRIDE_EVENT, wxT( "placement tick" ) );
		actorSubMenu->Append( ID_ADD_PLACEMENT_OVERRIDE_EVENT_DURATION, wxT( "placement duration" ) );
		if ( FindDurationBlendItemByEventClass( ClassID< CStorySceneOverridePlacementBlend >(), m_timeSelectorTimePos ) )
		{
			actorSubMenu->Append( ID_ADD_PLACEMENT_OVERRIDE_BLEND_INTERPOLATED_EVENT, wxT( "placement tick (blend interpolated)" ) );
		}
		actorSubMenu->Append( ID_ADD_PLACEMENT_WALK, wxT( "walk (deprecated, use 'placement duration')" ) );
		actorSubMenu->Append( ID_ADD_EQUIP_ITEM_EVENT, wxT( "equip item" ) );
		actorSubMenu->Append( ID_ADD_MORPH_EVENT, wxT( "morph" ) );
		actorSubMenu->Append( ID_ADD_D_PX_CLOTH_EVENT, wxT( "disable physics cloth" ) );
		actorSubMenu->Append( ID_ADD_D_DANGLE_EVENT, wxT( "disable dangle" ) );
		actorSubMenu->Append( ID_ADD_DANGLES_SHAKE_EVENT, wxT( "dangles shake" ) );
		actorSubMenu->Append( ID_ADD_RESET_DANGLE_EVENT, wxT( "reset cloth and dangles" ) );
		actorSubMenu->Append( ID_ADD_ACTOR_EFFECT_ITEM_EVENT, wxT( "effect" ) );
		actorSubMenu->Append( ID_ADD_ACTOR_EFFECT_DURATION_ITEM_EVENT, wxT( "effect duration" ) );
		actorSubMenu->Append( ID_ADD_CURVE_ANIMATION_EVENT, wxT( "curve animation" ) );
		actorSubMenu->Append( ID_ADD_CHANGE_ACTOR_GAME_STATE_EVENT, wxT("change game state") );
		actorSubMenu->Append( ID_ADD_HIRES_SHADOWS_EVENT, wxT("hires shadows") );
		actorSubMenu->Append( ID_ADD_MIMIC_LOD_EVENT, wxT("mimic lod") );

		AddGenericEventsEntries( genericObjs, actorSubMenu, actorSubMenuName.wc_str() );
		menu->AppendSubMenu( actorSubMenu, actorSubMenuName );

		// mimic submenu
		wxMenu* mimicSubMenu = new wxMenu();
		wxString mimicSubMenuName = wxT( "Add mimic event" );
		mimicSubMenu->Append( ID_ADD_MIMICS_ANIMATION_EVENT, wxT( "animation" ) );
		mimicSubMenu->Append( ID_ADD_MIMICS_EVENT, wxT( "change idle" ) );
		mimicSubMenu->Append( ID_ADD_MIMICS_POSE_EVENT, wxT( "add pose" ) );
		mimicSubMenu->Append( ID_ADD_MIMICS_FILTER_EVENT, wxT( "add filter" ) );
		mimicSubMenu->Append( ID_ADD_POSE_KEY_EVENT, wxT("add key") );
		AddGenericEventsEntries( genericObjs, mimicSubMenu, mimicSubMenuName.wc_str() );
		menu->AppendSubMenu( mimicSubMenu, mimicSubMenuName );

		// prop submenu
		wxMenu* propSubMenu = new wxMenu();
		wxString propSubMenuName = wxT( "Add prop event" );
		propSubMenu->Append( ID_ADD_SCENE_PROP_PLACEMENT_EVENT, wxT( "Scene Placement" ) );
		//propSubMenu->Append( ID_ADD_WORLD_PROP_PLACEMENT_EVENT, wxT( "World Placement" ) );
		propSubMenu->Append( ID_ADD_ATTACH_PROP_TO_BONE, wxT( "Attach prop to actor slot" ) );
		propSubMenu->Append( ID_ADD_PROP_EFFECT_ITEM_EVENT, wxT( "Effect" ) );
		propSubMenu->Append( ID_ADD_PROP_VISIBILITY_EVENT, wxT( "Visibility" ) );
		propSubMenu->Append( ID_ADD_LOOKAT_GAMEPLAY_EVENT, wxT( "Look at (gameplay)" ) );
		AddGenericEventsEntries( genericObjs, propSubMenu, propSubMenuName.wc_str() );
		menu->AppendSubMenu( propSubMenu, propSubMenuName );

		// light submenu
		wxMenu* lightSubMenu = new wxMenu();
		wxString lightSubMenuName = wxT( "Add light event" ); 
		lightSubMenu->Append( ID_ADD_LIGHT_PROPERTIES_EVENT, wxT( "Light Property Event" ) );
		AddGenericEventsEntries( genericObjs, lightSubMenu, lightSubMenuName.wc_str() );
		menu->AppendSubMenu( lightSubMenu, lightSubMenuName );

		// misc submenu
		wxMenu* miscSubMenu = new wxMenu();
		wxString miscSubMenuName = wxT( "Add misc event" ); 
		miscSubMenu->Append( ID_ADD_FADE_EVENT, wxT( "fade" ) );
		miscSubMenu->Append( ID_ADD_SOUND_EVENT, wxT( "sound" ) );
		miscSubMenu->Append( ID_ADD_HIT_SOUND_EVENT, wxT( "hit sound" ) );
		miscSubMenu->Append( ID_ADD_DIALOG_LINE_EVENT, wxT( "dialog line" ) );
		miscSubMenu->Append( ID_ADD_WEATHER_EVENT, wxT( "weather change" ) );
		miscSubMenu->Append( ID_ADD_SURFACE_EFFECT_EVENT, wxT( "Surface effect" ) );	
		miscSubMenu->Append( ID_ADD_ADDFACT_EVENT, wxT( "Add fact event" ) );
		miscSubMenu->Append( ID_ADD_OPENDOOR_EVENT, wxT( "Open door event" ) );
		miscSubMenu->Append( ID_ADD_DEBUG_COMMENT_EVENT, wxT( "debug comment" ) );	
		miscSubMenu->Append( ID_ADD_CAMERALIGHT_EVENT, wxT( "Camera light" ) );
		AddGenericEventsEntries( genericObjs, miscSubMenu, miscSubMenuName.wc_str() );
		menu->AppendSubMenu( miscSubMenu, miscSubMenuName );

		while ( genericObjs.Size() > 0 )
		{
			wxMenu* dynamicSubMenu = new wxMenu();
			String dynamicMenuCat = genericObjs.Back().m_second->m_menuCategory;
			AddGenericEventsEntries( genericObjs, dynamicSubMenu, dynamicMenuCat );
			menu->AppendSubMenu( dynamicSubMenu, dynamicMenuCat.AsChar() );
		}
	}
}

void CEdDialogTimeline::FillItemMenu( ITimelineItem* item, wxMenu* menu, Bool& addDefaults )
{
	addDefaults = true;

	// Check if event is clicked
	CTimelineItemEvent* eventItem = dynamic_cast< CTimelineItemEvent* >( item );
	if( eventItem != NULL )
	{
		if ( IsOneSelected() )
		{
			// Check if clicked in custom camera event
			CStorySceneEventCustomCamera* customCameraEvent = Cast< CStorySceneEventCustomCamera >( eventItem->GetEvent() );
			CStorySceneEventCustomCameraInstance* customCameraEventInstance = Cast< CStorySceneEventCustomCameraInstance >( eventItem->GetEvent() );
			const Bool isCameraEvt = customCameraEvent || customCameraEventInstance;
			if ( isCameraEvt )
			{
				if ( eventItem->GetEvent()->HasBlendParent() )
				{
					CStorySceneEvent* parentEvt = FindEvent( eventItem->GetEvent()->GetBlendParentGUID() );
					SCENE_ASSERT( parentEvt );
					if ( parentEvt )
					{
						if ( CStorySceneEventEnhancedCameraBlend* evt = Cast< CStorySceneEventEnhancedCameraBlend >( parentEvt ) )
						{
							if ( evt->GetNumberOfKeys() > 2 ) // Blend requires minimum of 2 keys
							{
								menu->Append( ID_REMOVE_FROM_CAMERA_ENHANCED_BLEND, wxT( "Remove from camera ENHANCED blend" ) );
							}
						}
						else if ( parentEvt->GetClass()->IsA< CStorySceneCameraBlendEvent >() )
						{
							menu->Append( ID_REMOVE_FROM_CAMERA_BLEND, wxT( "Remove from camera blend" ) );
						}
					}
				}
				else
				{
					if ( FindDurationBlendItemByEventClass( ClassID< CStorySceneEventEnhancedCameraBlend >(), eventItem->GetStart() ) )
					{
						menu->Append( ID_ADD_TO_CAMERA_ENHANCED_BLEND, wxT( "Add to camera EHNANCED blend" ) );
					}
				}
			}

			if( eventItem->GetEvent()->IsInterpolationEventKey() )
			{
				menu->Append( ID_INTERPOLATION_EVENT_DETACH_KEY, wxT( "Detach key from interpolation event" ) );
			}

			if( IsInterpolationEvent( eventItem) )
			{
				menu->Append( ID_INTERPOLATION_EVENT_PLOT, wxT( "Plot interpolation event" ) );
			}

			if( customCameraEvent != NULL )
			{
				menu->Append( ID_ADD_TO_CUSTOM_CAMERA_LIST, wxT( "Add to the list" ) );
			}
			
			if( customCameraEventInstance != NULL )
			{
				menu->Append( ID_DETACH_FROM_CUSTOM_CAMERA_LIST, wxT( "Detach from the list" ) );
			}

			if ( eventItem->GetEvent()->GetClass()->IsA< CStorySceneEventCameraBlend >() == true )
			{
				menu->Append( ID_REMOVE_CAMERA_BLEND, wxT( "Remove blend (old)" ) );
			}

			if ( eventItem->GetEvent()->GetClass()->IsA< CStorySceneCameraBlendEvent >() == true )
			{
				menu->AppendSeparator();
				wxMenu* subMenu = new wxMenu();
				subMenu->Append( ID_BLEND_CAMERA_EVENT_PLOT_POSITION, wxT( "Position" ) );
				subMenu->Append( ID_BLEND_CAMERA_EVENT_PLOT_ROTATION, wxT( "Rotation" ) );
				menu->AppendSubMenu( subMenu, wxT( "Plot" ) );
			}

			if ( eventItem->GetEvent()->GetClass()->IsA< CStorySceneEventGroup >() == true )
			{
				menu->Append( ID_UNGROUP_EVENTS, wxT( "Ungroup events" ) );
			}

			if ( CTimelineItemAnimClip* aitem = dynamic_cast< CTimelineItemAnimClip* >( item ) )
			{
				if ( aitem->GetKeyPoseMarkers().Size() > 0 )
				{
					menu->Append( ID_GOTO_MARKER, wxT( "Go to key pose marker ('n'-key)" ) );
				}
			}

			if ( eventItem->GetEvent()->HasLinkParent() )
			{
				menu->Append( ID_UNLINK_EVENT, wxT( "Unlink event ('u'-key)" ) );
			}
			else if ( eventItem->AutoLinkToThisEvent() )
			{
				menu->Append( ID_UNLINK_EVENT, wxT( "Unlink all children events from this event ('u'-key)" ) );
			}
			else
			{
				menu->Append( ID_LINK_EVENT, wxT( "Link event ('l'-key)" ) );
			}

			if ( eventItem->GetEvent()->IsMuted() )
			{
				menu->Append( ID_MUTE_EVENT, wxT( "Unmute event ('m'-key)" ) );
			}
			else
			{
				menu->Append( ID_MUTE_EVENT, wxT( "Mute event ('m'-key)" ) );
			}

			if ( CStorySceneEventOverridePlacement* evt = Cast< CStorySceneEventOverridePlacement >( eventItem->GetEvent() ) )
			{
				menu->Append( ID_OVERRIDE_PLACEMENT_COPY_TO_DIALOGSLOT, wxT( "Copy transform to dialogset slot" ) );
			}
			else if ( CStorySceneEventLightProperties* evt = Cast< CStorySceneEventLightProperties >( eventItem->GetEvent() ) )
			{
				menu->AppendSeparator();
				menu->Append( ID_LIGHT_COPY_TRANSFORM_FROM_DEF, wxT( "Copy transform from light definition to event" ) );
				menu->Append( ID_LIGHT_GO_TO_CENTER_SS, wxT( "Move light to center of screen" ) );
			}

			if ( eventItem->GetEvent()->GetClass()->IsA< CStorySceneEventAnimation >() )
			{
				menu->AppendSeparator();
				wxMenu* subMenu = new wxMenu();

				subMenu->Append( ID_CREATE_PLACEMENT_FOR_ANIM_PEVLIS, wxT( "Create actor placement for animation's pelvis bone" ) );

				menu->AppendSubMenu( subMenu, wxT( "Advanced" ) );
			}
			
			if ( GetEventInstanceDuration( *eventItem->GetEvent() ) > 0.f ) // We have only one option here so this if is ok
			{
				menu->AppendSeparator();
				wxMenu* subMenu = new wxMenu();

				subMenu->Append( ID_ALIGN_END_TO_SECTIONS_END, wxT( "Align end to section's end" ) );

				menu->AppendSubMenu( subMenu, wxT( "Align" ) );
			}

			if( IsType< CStorySceneEventAnimClip >( eventItem->GetEvent() ) && eventItem->GetStart() < m_timeSelectorTimePos && m_timeSelectorTimePos < eventItem->GetEnd() )
			{
				menu->Append( ID_SPLIT_ANIMATION_EVENT, "Razor ('r'-key)", "Split animation event at time cursor" );
			}

			if ( isCameraEvt )
			{
				menu->AppendSeparator();
				wxMenu* subMenu = new wxMenu();
				subMenu->Append( ID_COPY_CAM_LIGHTS, wxT( "Copy light events for this camera" ) );
				subMenu->Append( ID_PASTE_CAM_LIGHTS, wxT( "Paste light events here" ) );
				menu->AppendSubMenu( subMenu, wxT( "Lights" ) );
			}

			if ( IsType< CStorySceneEventScenePropPlacement >( eventItem->GetEvent() ) )
			{
				menu->Append( ID_TOGGLE_ENTITY_HELPER_IS_INTERACTIVE, "Toggle interactive" );
			}
		}
		else
		{
			if( CanCreateInterpolationEvent( m_selectedItems ) )
			{
				menu->Append( ID_INTERPOLATION_EVENT_CREATE, wxT( "Create interpolation event" ) );
			}

			if( Cast< CStorySceneEventCustomCamera >( eventItem->GetEvent() ) || Cast< CStorySceneEventCustomCameraInstance >( eventItem->GetEvent() ) )
			{
				menu->Append( ID_ADD_ENHANCED_CAMERA_BLEND, wxT( "Create enhanced blend" ) );
			}

			ITimelineItem* tiInterpolateEvent = nullptr;
			TDynArray< ITimelineItem* > tiKeys;
			if( GetSelectedItemCount() >= 2 && GetInterpolateEventAndKeys( m_selectedItems, tiInterpolateEvent, tiKeys) && !tiKeys.Empty() )
			{
				menu->Append( ID_INTERPOLATION_EVENT_ATTACH_KEY, wxT( "Attach keys to interpolation event" ) );
			}

			if( Cast< CStorySceneEventOverridePlacement >( eventItem->GetEvent() ) )
			{
				menu->Append( ID_ADD_PLACEMENT_OVERRIDE_BLEND, wxT( "Create override placement blend" ) );
			}

			menu->Append( ID_ALIGN_STARTS, wxT( "Align starts" ) );
			menu->Append( ID_ALIGN_ENDS, wxT( "Align ends" ) );
			menu->Append( ID_ALIGN_START_TO_END, wxT( "Align start to end" ) );

			menu->Enable( ID_ALIGN_START_TO_END, m_selectedItems.Size() == 2 );
		}
	}
}

void CEdDialogTimeline::OnNormalEventCheckedChanged( wxCommandEvent& event )
{
	Bool normal = event.IsChecked();
	for ( Uint32 i = 0; i < m_selectedItems.Size(); ++i )
	{
		ITimelineItem* eventItem = m_selectedItems[ i ];
		if ( eventItem )
		{
			eventItem->SetState( ( Int32 )EVENTSTATE_NORMAL );
		}
	}
}

void CEdDialogTimeline::OnMarkEventCheckedChanged( wxCommandEvent& event )
{
	Bool marked = event.IsChecked();
	for ( Uint32 i = 0; i < m_selectedItems.Size(); ++i )
	{
		ITimelineItem* eventItem = m_selectedItems[ i ];
		if ( eventItem )
		{
			eventItem->SetState( ( Int32 )EVENTSTATE_MARKED );
		}
	}
}

void CEdDialogTimeline::OnLockEventCheckedChanged( wxCommandEvent& event )
{
	Bool locked = event.IsChecked();
	for ( Uint32 i = 0; i < m_selectedItems.Size(); ++i )
	{
		ITimelineItem* eventItem = m_selectedItems[ i ];
		if ( eventItem )
		{
			eventItem->SetState( ( Int32 )EVENTSTATE_LOCKED );
		}
	}
}

void CEdDialogTimeline::OnDrawItem( const ITimelineItem* item, const wxRect& rect )
{
	/*if ( m_debugItems )
	{
		const CTimelineItemEvent* itemEvent = dynamic_cast< const CTimelineItemEvent* >( item );
		if ( itemEvent )
		{
			Float startTimeB = 0.f;
			Float durationB = 1.f;

			m_mediator->OnTimeline_GetEventTime( itemEvent->GetEvent(), startTimeB, durationB );

			Float startTimeA = item->GetStart();
			Float durationA = item->GetDuration();

			Int32 startPosA = ToPixelCoords( startTimeA );
			Int32 endPosA = ToPixelCoords( Min( startTimeA + durationA, m_totalDuration ) );

			Int32 startPosB = ToPixelCoords( startTimeB );
			Int32 endPosB = ToPixelCoords( Min( startTimeB + durationB, m_totalDuration ) );

			if ( MAbs( startTimeA - startTimeB ) > 0.01f || MAbs( durationA - durationB ) > 0.01f )
			{
				Track* track = m_tracks[ GetTrackIndex( item->GetTrackName() ) ] ;

				TimelineImpl::CDrawGroupTracks* drwGrp = GetTrackDrawGroup( track );
				wxPoint trackPos = drwGrp->GetDispTrackLocalPos( track );

				drwGrp->GetDrawBuffer()->DrawCircleCentered( wxPoint( startPosA, trackPos.y + 6 ), 6, wxColor( 255, 0, 0 ) );
				if ( startPosA != endPosA )
				{
					drwGrp->GetDrawBuffer()->DrawCircleCentered( wxPoint( endPosA, trackPos.y + 6 ), 6, wxColor( 255, 0, 0 ) );
				}

				drwGrp->GetDrawBuffer()->DrawCircleCentered( wxPoint( startPosB, trackPos.y + 6 ), 6, wxColor( 255, 255, 0 ) );
				if ( startPosB != endPosB )
				{
					drwGrp->GetDrawBuffer()->DrawCircleCentered( wxPoint( endPosB, trackPos.y + 6 ), 6, wxColor( 255, 255, 0 ) );
				}
			}
		}
	}*/

	// rect is in local space but events linker wants global rect
	wxRect globalRect = GetTrackDrawGroup( GetItemTrack( item ) )->GetGlobalRect( rect );

	m_eventsLinker.OnDrawItem( item, globalRect );
}

String CEdDialogTimeline::GetActorTrackName( const CName& actorId )
{
	// NOTE: Not a very pretty function... but the whole CName IDs for 
	// actors, props, effects & lights etc. should be combined into
	// 1 system or THashMap<> instead of this multiple array crap

	if( CStoryScene* scene = m_mediator->HACK_GetStoryScene() )
	{
		// a prop?
		const TDynArray< CStorySceneProp* >& propDefs = scene->GetScenePropDefinitions();
		for( Uint32 i=0; i<propDefs.Size(); ++i )
		{
			const CStorySceneProp* def = propDefs[ i ];
			if( propDefs[ i ] && propDefs[ i ]->m_id == actorId )
			{
				return PROP_TRACK;
			}
		}

		// an effect?
		const TDynArray< CStorySceneEffect* >& fxDefs = scene->GetSceneEffectDefinitions();
		for( Uint32 i=0; i<fxDefs.Size(); ++i )
		{
			if( fxDefs[ i ] && fxDefs[ i ]->m_id == actorId )
			{
				return EFFECT_TRACK;
			}
		}

		// a light?
		const TDynArray< CStorySceneLight* >& lightDefs = scene->GetSceneLightDefinitions();
		for( Uint32 i=0; i<lightDefs.Size(); ++i )
		{
			if( lightDefs[ i ] && lightDefs[ i ]->m_id == actorId )
			{
				return LIGHT_TRACK;
			}
		}
	}

	return String::EMPTY;
}

/*
Gets list of interpolation events that would be invalidated if specified events were deleted.
*/
void CEdDialogTimeline::GetInterpolationEventsInvalidatedByDeletion( const TDynArray< CTimelineItemEvent* >& events, TDynArray< CTimelineItemEvent* >& outInterpolationEvents )
{
	// find all events that are keys of interpolation events
	TDynArray< CTimelineItemEvent* > keys;
	for( Uint32 i = 0, numEvents = events.Size(); i < numEvents; ++i )
	{
		if( events[ i ]->GetEvent()->IsInterpolationEventKey() )
		{
			keys.PushBack( events[ i ] );
		}
	}

	if( !keys.Empty() )
	{
		// group keys by interpolation event to which they belong
		auto comp = [] ( const CTimelineItemEvent* a, const CTimelineItemEvent* b ) { return a->GetEvent()->GetInterpolationEventGUID() < b->GetEvent()->GetInterpolationEventGUID(); };
		Sort( keys.Begin(), keys.End(), comp );

		for( Uint32 i = 0, numKeys = keys.Size(); i < numKeys; )
		{
			CGUID interpolationEventGuid = keys[ i ]->GetEvent()->GetInterpolationEventGUID();

			// count number of keys in this group
			Uint32 j = i + 1;
			for( ; j < numKeys && keys[ j ]->GetEvent()->GetInterpolationEventGUID() == interpolationEventGuid; ++j );
			Uint32 numKeysInGroup = j - i;
			i += numKeysInGroup;

			// check if deleting all keys in this group would make interpolation event invalid (it's invalid if it has less than two keys)
			CTimelineItemInterpolationEvent* interpolationEvent = static_cast< CTimelineItemInterpolationEvent* >( FindItemEvent( interpolationEventGuid ) );
			if( interpolationEvent->GetNumKeys() - numKeysInGroup < 2 )
			{
				// add to result
				outInterpolationEvents.PushBackUnique( interpolationEvent );
			}
		}
	}
}

/*
Gets list of blend events that would be invalidated if specified events were deleted.
*/
void CEdDialogTimeline::GetBlendEventsInvalidatedByDeletion( const TDynArray< CTimelineItemEvent* >& events, TDynArray< CTimelineItemEvent* >& outBlendEvents )
{
	// find all events that are keys of blend events
	TDynArray< CTimelineItemEvent* > keys;
	for( Uint32 i = 0, numEvents = events.Size(); i < numEvents; ++i )
	{
		if( events[ i ]->GetEvent()->HasBlendParent() )
		{
			keys.PushBack( events[ i ] );
		}
	}

	if( !keys.Empty() )
	{
		// group keys by blend event to which they belong
		auto comp = [] ( const CTimelineItemEvent* a, const CTimelineItemEvent* b ) { return a->GetEvent()->GetBlendParentGUID() < b->GetEvent()->GetBlendParentGUID(); };
		Sort( keys.Begin(), keys.End(), comp );

		for( Uint32 i = 0, numKeys = keys.Size(); i < numKeys; )
		{
			CGUID blendEventGUID = keys[ i ]->GetEvent()->GetBlendParentGUID();

			// count number of keys in this group
			Uint32 j = i + 1;
			for( ; j < numKeys && keys[ j ]->GetEvent()->GetBlendParentGUID() == blendEventGUID; ++j );
			Uint32 numKeysInGroup = j - i;
			i += numKeysInGroup;

			// check if deleting all keys in this group would make blend event invalid (it's invalid if it has less than two keys)
			CTimelineItemBlend* blendEvent = static_cast< CTimelineItemBlend* >( FindItemEvent( blendEventGUID ) );
			if( blendEvent->GetItemsCount() - numKeysInGroup < 2 )
			{
				// add to result
				outBlendEvents.PushBackUnique( blendEvent );
			}
		}
	}
}

void CEdDialogTimeline::RemoveItems( const TDynArray< ITimelineItem* > items )
{
	TDynArray< CTimelineItemBlocking* > elements;						// list of items that are elements
	TDynArray< CTimelineItemEvent* > events;							// list of items that are events

	// separate items that are elements from items that are events
	for( Uint32 i = 0, numItems = items.Size(); i < numItems; ++i )
	{
		ITimelineItem* item = items[ i ];
		if( IsElement( item ) )
		{
			elements.PushBack( static_cast< CTimelineItemBlocking* >( item ) );
		}
		else
		{
			events.PushBack( static_cast< CTimelineItemEvent* >( item ) );
		}
	}

	// include events that should be deleted because their element is deleted
	Uint32 numElements = elements.Size();
	for( Uint32 i = 0; i < numElements; ++i )
	{
		TDynArray< DialogTimelineItems::CTimelineItemEvent* > elementEvents;
		Uint32 numElementEvents = GetElementEvents( elements[ i ], elementEvents );
		events.PushBackUnique( elementEvents );
	}

	// get interpolation and blend events that should be deleted because they would become invalid
	TDynArray< CTimelineItemEvent* > interpolationEvents;
	TDynArray< CTimelineItemEvent* > blendEvents;
	GetInterpolationEventsInvalidatedByDeletion( events, interpolationEvents );
	GetBlendEventsInvalidatedByDeletion( events, blendEvents );

	// separate all events that are to be deleted into three groups:
	// 1. interpolation events
	// 2. blend events
	// 3. rest of events
	TDynArray< CTimelineItemEvent* > otherEvents;
	for( Uint32 i = 0, numEvents = events.Size(); i < numEvents; ++i )
	{
		if( IsInterpolationEvent( events[ i ] ) )
		{
			interpolationEvents.PushBackUnique( events[ i ] );
		}
		else if( dynamic_cast< CTimelineItemBlend* >( events[ i ] ) )
		{
			blendEvents.PushBackUnique( events[ i ] );
		}
		else
		{
			otherEvents.PushBack( events[ i ] ); // no need to use PushBackUnique() here
		}
	}

	// create final list of events that are to be deleted - order is important
	TDynArray< CTimelineItemEvent* > deletionList;
	deletionList.Reserve( interpolationEvents.Size() + blendEvents.Size() + otherEvents.Size() );
	deletionList.PushBack( interpolationEvents );
	deletionList.PushBack( blendEvents );
	deletionList.PushBack( otherEvents );

	// remove events
	for( Uint32 i = 0, numEvents = deletionList.Size(); i < numEvents; ++i )
	{
		ITimelineItem* item = deletionList[ i ];
		RemoveItem( item );
		delete item;
	}

	// remove elements
	for( Uint32 i = 0; i < numElements; ++i )
	{
		CTimelineItemBlocking* item = elements[ 0 ];
		RemoveItem( item );
		m_elements.Remove( item );
		delete item;
	}
}

void CEdDialogTimeline::OnDeleteItem( wxCommandEvent& event )
{
	ASSERT( IsAnySelected() );

	// Hide properties page
	EditProperties( NULL, *m_propertiesPage );
	m_propertiesPage->Show( false );

	TDynArray< ITimelineItem* > removableItems;

	// get list of items that can be removed
	for ( Uint32 i = 0; i < m_selectedItems.Size(); ++i )
	{
		if ( m_selectedItems[i]->IsRemovable() )
		{
			removableItems.PushBack( m_selectedItems[i] );
		}
	}

	if ( removableItems.Empty() && !m_selectedItems.Empty() )
	{
		wxMessageBox( TXT( "Selected items cannot be removed" ) );
		return;
	}

	// Ask if user is sure
	if( ! YesNo( TXT( "Do you really want to delete these items?" ) ) )
	{
		return;
	}

	RemoveItems( removableItems );

	m_selectedItems.Clear();
	SelectionChanged();
}

void CEdDialogTimeline::GetDisableDialogLookatEventsPositions( CName actorName, CName animName, TDynArray< TPair< Float, Float > >& dialogAnimEvents, Bool mimic ) const
{
	if ( m_mediator )
	{
		m_mediator->OnTimeline_GetDisableDialogLookatEventsPositions( actorName, animName, dialogAnimEvents, mimic );
	}
}

void CEdDialogTimeline::GetKeyPoseMarkersEventsPositions( CName actorName, CName animName, TDynArray< Float >& dialogAnimEvents, Bool mimic ) const
{
	if ( m_mediator )
	{
		m_mediator->OnTimeline_GetKeyPoseMarkersEventsPositions( actorName, animName, dialogAnimEvents, mimic );
	}
}

void CEdDialogTimeline::GetKeyPoseDurationsEventsPositions( CName actorName, CName animName, TDynArray< TPair< Float, Float > >& dialogAnimEventsTrans, TDynArray< TPair< Float, Float > >& dialogAnimEventsPoses, Bool mimic ) const
{
	if ( m_mediator )
	{
		m_mediator->OnTimeline_GetKeyPoseDurationsEventsPositions( actorName, animName, dialogAnimEventsTrans, dialogAnimEventsPoses, mimic );
	}
}

/*
Returns CName denoting actor associated with specified track.

\param trackName Track name for which to find associated actor. May include group name.
\return CName denoting actor associated with specified track or CName::NONE if no actor is associated with specified track.
*/
CName CEdDialogTimeline::GetTrackActor( const String& trackName ) const
{
	CName actor = CName::NONE;

	size_t separatorPos;
	if ( trackName.FindCharacter( GROUP_SEPARATOR, separatorPos ) )
	{
		actor = CName( trackName.LeftString( separatorPos ) );
	}
	else
	{
		actor = CName( trackName );
	}

	// check if this is really an actor name
	if ( m_mediator->GetSceneActor( actor ) == NULL )
	{
		// then check if it is an entity name (prop, effect, light, etc.)
		// TODO: should be cleaned when actors, props, effects etc. are merged
		if ( m_mediator->GetSceneEntity( actor ) == NULL )
		{
			actor = CName::NONE;
		}
	}

	return actor;
}


void CEdDialogTimeline::GetMarkedLockedEvents( TDynArray< CStorySceneEvent*>& marked, TDynArray< CStorySceneEvent*>& locked )
{
	for( Uint32 i = 0; i < m_items.Size(); ++i )
	{
		CTimelineItemEvent* item = dynamic_cast<CTimelineItemEvent*>( m_items[i] );
		
		if( item && item->GetEvent() )
		{
			if( item->GetState() == DialogTimelineItems::EVENTSTATE_MARKED )
			{
				marked.PushBack( item->GetEvent() );
			}
			if( item->GetState() == DialogTimelineItems::EVENTSTATE_LOCKED  )
			{
				locked.PushBack( item->GetEvent() );
			}			
		}

	}
}

Bool CEdDialogTimeline::GetVoiceDataPositions( CStorySceneLine* lineElement, TDynArray< TPair< Float, Float > >* voiceMaxAmpRelPos, Float* voiceStartRelPos, Float* voiceEndRelPos )
{
	return m_showVoiceMarkers && m_mediator->OnTimeline_GetVoiceDataPositions( lineElement, voiceMaxAmpRelPos, voiceStartRelPos, voiceEndRelPos );
}

void CEdDialogTimeline::GetSelectedEvents( TDynArray<CStorySceneEvent*>& events )
{
	Uint32 size = m_selectedItems.Size();
	for ( Uint32 i = 0; i < size; ++i )
	{
		CTimelineItemEvent* item = dynamic_cast< CTimelineItemEvent* >( m_selectedItems[i] ); 
		if ( item )
		{
			events.PushBack( item->GetEvent() );
		}
	}
}

CWorld* CEdDialogTimeline::GetPreviewWorld() const
{
	return m_mediator->OnTimeline_GetWorld();
}

EScenePreviewCameraMode CEdDialogTimeline::GetCameraMode()
{
	return m_mediator->OnPreview_GetCameraMode();
}

void CEdDialogTimeline::SetShowVoiceMarkers( Bool showVoiceMarkers )
{
	m_showVoiceMarkers = showVoiceMarkers;
}

Bool CEdDialogTimeline::GetShowVoiceMarkers()
{
	return m_showVoiceMarkers;
}

void CEdDialogTimeline::DrawLink( const wxRect& start, const wxRect& end, const wxColor& c, Float progress )
{
	TimelineImpl::CDrawGroupVolatile* drw = GetDrawGroupVolatile();

	drw->GetDrawBuffer()->DrawLine( start.GetX(), start.GetY(), end.GetX(), end.GetY(), c );
}

Bool CEdDialogTimeline::IsTrackLocked( const String& trackName ) const
{
	Int32 i = GetTrackIndex( trackName );
	if ( i > 0 )
	{
		DialogTrack* track = dynamic_cast< DialogTrack* >( m_tracks[i] );
		if ( track )
		{
			return track->m_isLocked;
		}
	}
	return false;
}

void CEdDialogTimeline::GetLockedTracks( TDynArray<String>& tracks ) const
{
	for ( Uint32 i = 0; i < m_tracks.Size(); i++ ) 
	{
		DialogTrack* track = dynamic_cast< DialogTrack* >( m_tracks[i] );
		if ( track && track->m_isLocked )
		{
			tracks.PushBack( m_tracks[i]->m_name );
		}	
	}
}

CName CEdDialogTimeline::GetSelectedEntityName() const
{
	if ( m_selectedTrack >= 0 )
	{
		ASSERT( m_selectedTrack < ( Int32 ) m_tracks.Size() );
		const String trackName = m_tracks[ m_selectedTrack ]->m_name;
		const String actorNameStr = trackName.StringBefore( TXT(".") );

		return CName( actorNameStr );
	}

	return CName::NONE;
}

wxDragResult CEdDialogTimeline::OnDragOver( wxCoord x, wxCoord y, wxDragResult def )
{
	// TODO - if something
	//{
	//	return wxDragCancel;
	//}

	wxTextDataObject* textData = GetDraggedDataObject();
	if ( !textData )
	{
		return wxDragNone;
	}

	String text = textData->GetText().c_str();

	size_t cutPlace;
	if ( !text.FindCharacter( ';', cutPlace ) )
	{
		return wxDragNone;
	}

	String path = text.LeftString( cutPlace );
	CName animationName = CName( text.RightString( text.GetLength() - cutPlace - 1 ) );

	ResourceLoadingContext context;
	CSkeletalAnimationSet* set = LoadResource< CSkeletalAnimationSet >( path, context );
	if ( set )
	{
		CSkeletalAnimationSetEntry* entry = set->FindAnimation( animationName );
		if ( entry )
		{
			wxPoint globalPos( x, y );

			TDynArray< ITimelineItem* > itemsAtPos;
			const Uint32 numItemsAtPos = GetItemsAt(globalPos, itemsAtPos);
			if ( numItemsAtPos == 1 )
			{
				CTimelineItemEvent* item = dynamic_cast< CTimelineItemEvent* >( itemsAtPos[ 0 ] );
				if ( item )
				{
					CStorySceneEvent* e = item->GetEvent();
					if ( e )
					{
						if ( ISSDragAndDropBodyAnimInterface* inter = e->QueryInterface< ISSDragAndDropBodyAnimInterface >() )
						{
							//ISSAnimClipInterface* ii = e->QueryInterface< ISSAnimClipInterface >();
							//SCENE_ASSERT( ii );
							//...
						}
						else if ( ISSDragAndDropMimicsAnimInterface* inter = e->QueryInterface< ISSDragAndDropMimicsAnimInterface >() )
						{
							//ISSAnimClipInterface* ii = e->QueryInterface< ISSAnimClipInterface >();
							//SCENE_ASSERT( ii );
						}
					}
				}
			}

			return wxDragCopy;
		}
	}

	return wxDragNone;
}

Bool CEdDialogTimeline::OnDropText( wxCoord x, wxCoord y, String &text )
{
	// TODO - if something
	//{
	//	return wxDragCancel;
	//}

	size_t cutPlace;
	if ( !text.FindCharacter( ';', cutPlace ) )
	{
		return false;
	}

	String path = text.LeftString( cutPlace );
	CName animationName = CName( text.RightString( text.GetLength() - cutPlace - 1 ) );

	ResourceLoadingContext context;
	CSkeletalAnimationSet* set = LoadResource< CSkeletalAnimationSet >( path, context );
	if ( set )
	{
		CSkeletalAnimationSetEntry* entry = set->FindAnimation( animationName );
		if ( entry )
		{
			wxPoint globalPos( x, y );

			TDynArray< ITimelineItem* > itemsAtPos;
			const Uint32 numItemsAtPos = GetItemsAt(globalPos, itemsAtPos);
			if ( numItemsAtPos == 1 )
			{
				CTimelineItemEvent* item = dynamic_cast< CTimelineItemEvent* >( itemsAtPos[ 0 ] );
				if ( item )
				{
					CStorySceneEvent* e = item->GetEvent();
					if ( e )
					{
						if ( ISSDragAndDropBodyAnimInterface* inter = e->QueryInterface< ISSDragAndDropBodyAnimInterface >() )
						{
							TDynArray< CName > extraData;
							StorySceneEditorUtils::FindBodyAnimationData( animationName, extraData );

							inter->Interface_SetDragAndDropBodyAnimation( extraData, entry->GetDuration() );

							item->UpdatePresentation();
						}
						else if ( ISSDragAndDropMimicsAnimInterface* inter = e->QueryInterface< ISSDragAndDropMimicsAnimInterface >() )
						{
							TDynArray< CName > extraData;
							StorySceneEditorUtils::FindMimicsAnimationData( animationName, extraData );

							inter->Interface_SetDragAndDropMimicsAnimation( extraData, entry->GetDuration() );

							item->UpdatePresentation();
						}

						return true;
					}
				}

				return false;
			}

			Track* t = GetTrackAt( globalPos );
			if ( !t )
			{
				return false;
			}

			const String& trackName = t->m_name;

			String actorNameStr = trackName.StringBefore( TXT(".") );
			if ( actorNameStr.Empty() )
			{
				return false;
			}

			CName actorName( actorNameStr );

			const Float time = CalculateTimePos( x );

			CTimelineItemBlocking* selected = FindSceneElement( time );
			if ( !selected )
			{
				return false;
			}

			Float position = ( time - selected->GetStart() ) / selected->GetDuration();

			size_t index = 0;
			if ( trackName.FindSubstring( TXT("animations"), index ) )
			{
				TDynArray< CName > extraData;
				StorySceneEditorUtils::FindBodyAnimationData( animationName, extraData );

				CreateEvent( ID_ADD_ANIMATION_EVENT, trackName, position, actorName, selected, &extraData );
			}
			else if ( trackName.FindSubstring( TXT("mimics"), index ) || trackName.FindSubstring( TXT("mimics2"), index ) )
			{
				TDynArray< CName > extraData;
				StorySceneEditorUtils::FindMimicsAnimationData( animationName, extraData );

				CreateEvent( ID_ADD_MIMICS_ANIMATION_EVENT, trackName, position, actorName, selected, &extraData );
			}

			return true;
		}
	}

	return false;
}

/*
Converts old camera blends to interpolation events.

\return True - all old camera blends were successfully converted. False - at least one camera blend couldn't be converted.

Old camera blends:
a) CStorySceneEventCameraBlend - oldest type of camera blend, we'll call it version 1.
   Accompanied by CTimelineItemCameraBlend timeline item.
b) CStorySceneCameraBlendEvent - this replaced CStorySceneEventCameraBlend. Well call it version 2.
   Accompanied by CTimelineItemCameraBlendEvent.
*/
Bool CEdDialogTimeline::ConvertOldCameraBlendsToInterpolationEvents()
{
	// Can't run conversion if section is not approved.
	ASSERT( IsTimelineEditingEnabled() );

	TDynArray< CTimelineItemCameraBlend* > tiBlendsV1;
	TDynArray< CTimelineItemCameraBlendEvent* > tiBlendsV2;

	// get list of camera blends that are to be converted
	for( auto itItem = m_items.Begin(), endItems = m_items.End(); itItem != endItems; ++itItem )
	{
		if( CTimelineItemCameraBlend* blendV1 = dynamic_cast< CTimelineItemCameraBlend* >( *itItem ) )
		{
			tiBlendsV1.PushBack( blendV1 );
		}
		else if( CTimelineItemCameraBlendEvent* blendV2 = dynamic_cast< CTimelineItemCameraBlendEvent* >( *itItem ) )
		{
			tiBlendsV2.PushBack( blendV2 );
		}
	}

	Bool allBlendsConverted = true;

	// convert v1 type camera blends
	for( auto itBlendV1 = tiBlendsV1.Begin(), endBlendsV1 = tiBlendsV1.End(); itBlendV1 != endBlendsV1; ++itBlendV1 )
	{
		CTimelineItemInterpolationEvent* result = ConvertOldCameraBlendToInterpolationEvent( *itBlendV1 );
		if( !result )
		{
			allBlendsConverted = false;
		}
	}

	// convert v2 type camera blends
	for( auto itBlendV2 = tiBlendsV2.Begin(), endBlends = tiBlendsV2.End(); itBlendV2 != endBlends; ++itBlendV2 )
	{
		CTimelineItemInterpolationEvent* result = ConvertOldCameraBlendToInterpolationEvent( *itBlendV2 );
		if( !result )
		{
			allBlendsConverted = false;
		}
	}

	return allBlendsConverted;
}

/*
Converts v1 type camera blend to interpolation event.

\param tiBlend Camera blend event that is to be converted to interpolation event. It's deleted after
successful conversion.
\return Resulting camera interpolation event or nullptr if camera blend event couldn't be converted.
Possible reason of failure: camera blend is wrapped.
*/
CTimelineItemInterpolationEvent* CEdDialogTimeline::ConvertOldCameraBlendToInterpolationEvent( CTimelineItemCameraBlend* tiBlend )
{
	// Can't run conversion if section is not approved.
	ASSERT( IsTimelineEditingEnabled() );

	CStorySceneEventCameraBlend* scBlend = static_cast< CStorySceneEventCameraBlend* >( tiBlend->GetEvent() );
	const TDynArray< SStorySceneCameraBlendKey >& blendKeys = scBlend->GetBlendKeys();

	// Check whether any of blend keys fall out of timeline range. In such case we're unable to create
	// interpolation event. We could wrap position of a key but then resulting interpolation event would
	// give completely different results from those of source blend event. We could also move source blend
	// so that it stops being wrapped but only the user can decide whether this is desirable.
	for( Uint32 iBlendKey = 0, numBlendKeys = blendKeys.Size(); iBlendKey < numBlendKeys; ++iBlendKey )
	{
		const Float scBlendStartTime = m_mediator->OnTimeline_GetEventInstanceStartTime( *scBlend );
		const Float scBlendDuration = m_mediator->OnTimeline_GetEventInstanceDuration( *scBlend );
		const Float absStartTime = scBlendStartTime + blendKeys[ iBlendKey ].m_time * scBlendDuration;
		
		if( absStartTime >= m_activeRangeDuration )
		{
			return nullptr;
		}
	}

	// Create camera event for each blend key. Those events will be interpolation event keys.
	TDynArray< ITimelineItem* > tiKeys;
	for( Uint32 iBlendKey = 0, numBlendKeys = blendKeys.Size(); iBlendKey < numBlendKeys; ++iBlendKey )
	{
		const Float scBlendStartTime = m_mediator->OnTimeline_GetEventInstanceStartTime( *scBlend );
		const Float scBlendDuration = m_mediator->OnTimeline_GetEventInstanceDuration( *scBlend );
		const Float absStartTime = scBlendStartTime + blendKeys[ iBlendKey ].m_time * scBlendDuration;
		
		const StorySceneCameraDefinition& camDef = blendKeys[ iBlendKey ].m_cameraDefinition;

		CTimelineItemEvent* cameraEvent = CreateCustomCameraEvent( camDef, nullptr, absStartTime );
		tiKeys.PushBack( cameraEvent );
	}

	// Get ease in and ease out style.
	EInterpolationEasingStyle easingStyle = EInterpolationEasingStyle::IES_Smooth; // equivalent of ECameraInterpolation::BEZIER
	if( scBlend->GetInterpolationType() ==  ECameraInterpolation::LINEAR )
	{
		easingStyle = EInterpolationEasingStyle::IES_Rapid;
	}

	// Delete old blend.
	RemoveItem( tiBlend );
	delete tiBlend;

	// Create interpolation event.
	CTimelineItemInterpolationEvent* tiInterpolationEvent = InterpolationEventCreate( tiKeys );
	ASSERT( tiInterpolationEvent );

	// Set ease in and ease out.
	CStorySceneEventInterpolation* scInterpolationEvent = tiInterpolationEvent->GetSceneInterpolationEvent();
	scInterpolationEvent->SetEaseInStyle( easingStyle );
	scInterpolationEvent->SetEaseInParameter( 1.0f / 3.0f );
	scInterpolationEvent->SetEaseOutStyle( easingStyle );
	scInterpolationEvent->SetEaseOutParameter( 1.0f / 3.0f );

	return tiInterpolationEvent;
}

/*
Converts v2 type camera blend to interpolation event.

\param tiBlend Camera blend event that is to be converted to interpolation event. It's deleted after
successful conversion.
\return Resulting camera interpolation event or nullptr if camera blend event couldn't be converted.
*/
CTimelineItemInterpolationEvent* CEdDialogTimeline::ConvertOldCameraBlendToInterpolationEvent( CTimelineItemCameraBlendEvent* tiBlend )
{
	// Can't run conversion if section is not approved.
	ASSERT( IsTimelineEditingEnabled() );

	CStorySceneCameraBlendEvent* scBlend = static_cast< CStorySceneCameraBlendEvent* >( tiBlend->GetEvent() );

	// get list of key items
	TDynArray< ITimelineItem* > tiKeys;
	for( Uint32 iKey = 0, numKeys = scBlend->GetNumberOfKeys(); iKey < numKeys; ++iKey )
	{
		CGUID guid = scBlend->GetKey( iKey );
		CTimelineItemEvent* item = FindItemEvent( guid );
		tiKeys.PushBack( item );
	}

	// get ease in style and parameter
	EInterpolationEasingStyle easeInStyle = EInterpolationEasingStyle::IES_Smooth;  // equivalent of ECameraInterpolation::BEZIER
	if( scBlend->GetFirstPartInterpolationType() == ECameraInterpolation::LINEAR )
	{
		easeInStyle = EInterpolationEasingStyle::IES_Rapid;
	}
	// this gives better results than using scBlend->GetFirstPointOfInterpolation() value
	Float easeInParameter = 1.0f / 3.0f;

	// get ease out style and parameter
	EInterpolationEasingStyle easeOutStyle = EInterpolationEasingStyle::IES_Smooth; // equivalent of ECameraInterpolation::BEZIER
	if( scBlend->GetLastPartInterpolationType() == ECameraInterpolation::LINEAR )
	{
		easeOutStyle = EInterpolationEasingStyle::IES_Rapid;
	}
	// this gives better results than using scBlend->GetLastPointOfInterpolation() value
	Float easeOutParameter = 1.0f / 3.0f;

	// Delete old blend.
	RemoveItem( tiBlend );
	delete tiBlend;

	// Create interpolation event.
	CTimelineItemInterpolationEvent* tiInterpolationEvent = InterpolationEventCreate( tiKeys );
	ASSERT( tiInterpolationEvent );

	// Set ease in and ease out.
	CStorySceneEventInterpolation* scInterpolationEvent = tiInterpolationEvent->GetSceneInterpolationEvent();
	scInterpolationEvent->SetEaseInStyle( easeInStyle );
	scInterpolationEvent->SetEaseInParameter( easeInParameter );
	scInterpolationEvent->SetEaseOutStyle( easeOutStyle );
	scInterpolationEvent->SetEaseOutParameter( easeOutParameter );

	return tiInterpolationEvent;
}

void CEdDialogTimeline::StoreLayout()
{
	if( m_section )
	{
		// store layout
		CTimelineLayout layout;
		GetLayout( layout );
		m_mediator->StoreTimelineLayout( m_section->GetSectionId(), layout );
	}
}

void CEdDialogTimeline::RestoreLayout()
{
	// apply layout
	if( m_section )
	{
		if( const CTimelineLayout* layout = m_mediator->GetTimelineLayout( m_section->GetSectionId() ) )
		{
			ApplyLayout( *layout );
		}
		else
		{
			CTimelineLayout defaultLayout;
			defaultLayout.m_verticalOffset = 0.0f;
			defaultLayout.m_visibleRangeDuration = 1.2f * m_activeRangeDuration;
			defaultLayout.m_activeRangeTimeOffset = defaultLayout.m_visibleRangeDuration / 2 - ( m_activeRangeDuration / 2.2f );
			defaultLayout.m_currentGrid = 5.0f;
			defaultLayout.m_vertScale = 1.0f;

			ApplyLayout( defaultLayout );
		}
	}

	UpdateLayout();
	NotifyBufferIsInvalid();
}

Bool CEdDialogTimeline::IsTimelineEditingEnabled() const
{
	return m_mediator->OnTimeline_IsTimelineEditingEnabled();
}

void CEdDialogTimeline::HandleRepositionLeftEdge( ITimelineItem* item, Float newTimePos, TimelineKeyModifiers keyModifiers )
{
	if( !dynamic_cast< CTimelineItemPause* >( item ) )
	{
		item->SetLeftEdge( newTimePos, keyModifiers );
	}
	else // CTimelineItemPause
	{
		// When we have many variants, we only allow to resize pauses in such a way that all events keep their relative position.
		// This way we don't have to bother with updating events from other variants which would be pretty cumbersome since much
		// of this logic is implemented by timeline items themselves and we have timeline items for currently chosen variant only.
		// Of course, the main problem is that pause elements (as other scene elements) are currently shared between all variants.
		// This will change in future but where not there yet.
		if( m_section->GetNumVariants() > 1 && !( keyModifiers.ctrl && !keyModifiers.alt ) )
		{
			return;
		}

		CEdTimeline::HandleRepositionLeftEdge( item, newTimePos, keyModifiers );
	}
}

void CEdDialogTimeline::HandleRepositionRightEdge( ITimelineItem* item, Float newTimePos, TimelineKeyModifiers keyModifiers )
{
	if( !dynamic_cast< CTimelineItemPause* >( item ) )
	{
		item->SetRightEdge( newTimePos, keyModifiers );
	}
	else // CTimelineItemPause
	{
		// When we have many variants, we only allow to resize pauses in such a way that all events keep their relative position.
		// This way we don't have to bother with updating events from other variants which would be pretty cumbersome since much
		// of this logic is implemented by timeline items themselves and we have timeline items for currently chosen variant only.
		// Of course, the main problem is that pause elements (as other scene elements) are currently shared between all variants.
		// This will change in future but where not there yet.
		if( m_section->GetNumVariants() > 1 && !( keyModifiers.ctrl && !keyModifiers.alt ) )
		{
			return;
		}

		CEdTimeline::HandleRepositionRightEdge( item, newTimePos, keyModifiers );
	}
}

Int32 CEdDialogTimeline::GetIdOfGenericObject( const CClass* cls ) const
{
	for( const TPair< Int32, SEtorySceneEventGenericCreationData* >& entry : m_genericObjsData )
	{
		if( entry.m_second->m_eventClass == cls )
		{
			return entry.m_first;
		}
	}
	return -1;
}

CStorySceneSectionVariantId CEdDialogTimeline::GetSectionVariantId() const
{
	// TODO: Currently this is how we get which variant is currently displayed. This is also
	// how we do it in other places. This is not nice. Scene Editor should remember somewhere
	// which variant is displayed and it should pass it to any functions that need it.
	// Unfortunately this is harder than it seems due to loads of crappy code.
	const Uint32 currentLocaleId = SLocalizationManager::GetInstance().GetCurrentLocaleId();
	const CStorySceneSectionVariantId sectionVariantId = m_section->GetVariantUsedByLocale( currentLocaleId );
	return sectionVariantId;
}

#ifdef DEBUG_SCENES_2
#pragma optimize("",on)
#endif
