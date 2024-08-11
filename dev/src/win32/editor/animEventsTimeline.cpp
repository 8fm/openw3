/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "animEventsTimeline.h"
#include "../../common/game/extAnimItemEvents.h"
#include "../../common/game/extAnimFootstepEvent.h"
#include "../../common/game/extAnimProjectileEvent.h"
#include "../../common/game/extAnimGameplayMimicEvent.h"
#include "../../common/game/extAnimLocationAdjustmentEvent.h"
#include "../../common/game/extAnimMaterialBasedFxEvent.h"
#include "../../common/game/extAnimRotationAdjustmentEvent.h"
#include "../../common/game/extAnimRotationAdjustmentLocationBasedEvent.h"
#include "../../common/game/extAnimOnSlopeEvent.h"
#include "../../common/game/expEvents.h"
#include "../../common/game/sceneAnimEvents.h"
#include "../../games/r4/animAttackEvent.h"
#include "../../common/game/animEffectEvent.h"
#include "../../games/r4/preAttackEvent.h"
#include "animEventsTimelineItems.inl"
#include "undoAnimEventsEditor.h"

#include "../../common/engine/extAnimScriptEvent.h"
#include "../../common/engine/extAnimRaiseEventEvent.h"
#include "../../common/engine/extAnimMorphEvent.h"
#include "../../common/engine/extAnimCutsceneDialogEvent.h"
#include "../../common/engine/extAnimExplorationEvent.h"
#include "../../common/engine/extAnimBehaviorEvents.h"

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

// =================================================================================================
} // unnamed namespace
// =================================================================================================

enum ETrackEvents
{
	ID_NEW_SIMPLE_EVENT = 4001,
	ID_NEW_SOUND_EVENT,
	ID_NEW_DURATION_EVENT,
	ID_NEW_EXPLORATION_EVENT,
	ID_NEW_MORPH_EVENT,
	ID_NEW_FOOTSTEP_EVENT,
	ID_NEW_ITEM_EVENT,
	ID_NEW_ITEM_EFFECT_EVENT,
	ID_NEW_ITEM_EFFECT_DURATION_EVENT,
	ID_NEW_LOOK_AT_EVENT,
	ID_NEW_SCENE_DIALOG_DISABLE_LOOKAT,
	ID_NEW_SCENE_DIALOG_EXPRESSION_POINT,
	ID_NEW_SCENE_DIALOG_EXPRESSION_DURATION,
	ID_NEW_COMBO_EVENT,
	ID_NEW_HIT_EVENT,
	ID_NEW_MAGIC_EVENT,
	ID_NEW_LOCATION_ADJUSTMENT_EVENT,
	ID_NEW_ROTATION_ADJUSTMENT_EVENT,
	ID_NEW_ROTATION_ADJUSTMENT_LOCATION_BASED_EVENT,
	ID_NEW_ON_SLOPE_EVENT_ON_SLOPE_TRACK,
	ID_NEW_COMBAT_EVENT,
	ID_NEW_EFFECT_EVENT,
	ID_NEW_EFFECT_DURATION_EVENT,
	ID_NEW_RAISE_EVENT_EVENT,
	ID_SHOW_ALL_TRACKS,
	ID_NEW_ANIMATE_ITEM_EVENT,
	ID_NEW_ITEM_BEHAVIOR_EVENT,
	ID_NEW_ITEM_DROP_EVENT,
	ID_NEW_ITEM_SYNC_EVENT,
	ID_NEW_ITEM_SYNC_DURATION_EVENT,
	ID_NEW_ITEM_SYNC_WITH_CORR_EVENT,
	ID_NEW_GAMEPLAY_MIMIC_STATE_EVENT,
	ID_NEW_PRE_ATTACK_EVENT,
	ID_EXP_SYNC_EVENT,
	ID_EXP_SLIDE_EVENT,
	ID_SCRIPT_ENUM_EVENT,
	ID_SCRIPT_SLIDE_TO_TARGET_EVENT,
	ID_SCRIPT_MULTI_VALUE_EVENT,
	ID_SCRIPT_MULTI_VALUE_SIMPLE_EVENT,
	ID_NEW_MATERIAL_BASED_FX_EVENT,
	ID_NEW_FORCED_LOGICAL_FOOTSTEP_EVENT,
};


CEdAnimEventsTimeline::CEdAnimEventsTimeline( wxPanel* parentPanel )
	: CEdTimeline( parentPanel, nullptr, CanvasType::gdiplus )
	, m_animationName( CName::NONE )
	, m_eventsContainer( NULL )
{
	RegisterDrawGroupTracksPinned(new TimelineImpl::CDrawGroupTracks(*this, &m_pinnedBuffer));
	RegisterDrawGroupTracksDefault(new TimelineImpl::CDrawGroupTracks(*this, &m_defaultBuffer));
	RegisterDrawGroupTimebar(new TimelineImpl::CDrawGroupTimebar(*this, &m_timebarBuffer));
	RegisterDrawGroupVolatile(new TimelineImpl::CDrawGroupVolatile(*this, &m_volatileBuffer));
}

CEdAnimEventsTimeline::~CEdAnimEventsTimeline()
{}

void CEdAnimEventsTimeline::FillTrackMenu( const String& trackName, wxMenu* menu )
{
	menu->Append( ID_NEW_SIMPLE_EVENT, wxT( "Add simple event" ) );
	menu->Connect( ID_NEW_SIMPLE_EVENT, wxEVT_COMMAND_MENU_SELECTED,
		wxCommandEventHandler( CEdAnimEventsTimeline::OnNewEvent ), NULL, this );

	menu->Append( ID_NEW_DURATION_EVENT, wxT( "Add duration event" ) );
	menu->Connect( ID_NEW_DURATION_EVENT, wxEVT_COMMAND_MENU_SELECTED,
		wxCommandEventHandler( CEdAnimEventsTimeline::OnNewEvent ), NULL, this );

	menu->Append( ID_NEW_FOOTSTEP_EVENT, wxT( "Add footstep event" ) );
	menu->Connect( ID_NEW_FOOTSTEP_EVENT, wxEVT_COMMAND_MENU_SELECTED,
		wxCommandEventHandler( CEdAnimEventsTimeline::OnNewEvent ), NULL, this );

	menu->Append( ID_NEW_FORCED_LOGICAL_FOOTSTEP_EVENT, wxT( "Add forced logical footstep event" ) );
	menu->Connect( ID_NEW_FORCED_LOGICAL_FOOTSTEP_EVENT, wxEVT_COMMAND_MENU_SELECTED,
		wxCommandEventHandler( CEdAnimEventsTimeline::OnNewEvent ), NULL, this );

	menu->Append( ID_NEW_MATERIAL_BASED_FX_EVENT, wxT( "Add material based FX event" ) );
	menu->Connect( ID_NEW_MATERIAL_BASED_FX_EVENT, wxEVT_COMMAND_MENU_SELECTED,
		wxCommandEventHandler( CEdAnimEventsTimeline::OnNewEvent ), NULL, this );

	menu->Append( ID_NEW_SOUND_EVENT, wxT( "Add sound event" ) );
	menu->Connect( ID_NEW_SOUND_EVENT, wxEVT_COMMAND_MENU_SELECTED,
		wxCommandEventHandler( CEdAnimEventsTimeline::OnNewEvent ), NULL, this );
	
	menu->Append( ID_NEW_MORPH_EVENT, wxT( "Add morph event" ) );
	menu->Connect( ID_NEW_MORPH_EVENT, wxEVT_COMMAND_MENU_SELECTED,
		wxCommandEventHandler( CEdAnimEventsTimeline::OnNewEvent ), NULL, this );

	menu->Append( ID_NEW_LOOK_AT_EVENT, wxT( "Add look at event" ) );
	menu->Connect( ID_NEW_LOOK_AT_EVENT, wxEVT_COMMAND_MENU_SELECTED,
		wxCommandEventHandler( CEdAnimEventsTimeline::OnNewEvent ), NULL, this );

	menu->Append( ID_NEW_COMBO_EVENT, wxT( "Add combo event" ) );
	menu->Connect( ID_NEW_COMBO_EVENT, wxEVT_COMMAND_MENU_SELECTED,
		wxCommandEventHandler( CEdAnimEventsTimeline::OnNewEvent ), NULL, this );

	menu->Append( ID_NEW_MAGIC_EVENT, wxT( "Add magic event" ) );
	menu->Connect( ID_NEW_MAGIC_EVENT, wxEVT_COMMAND_MENU_SELECTED,
		wxCommandEventHandler( CEdAnimEventsTimeline::OnNewEvent ), NULL, this );

	menu->Append( ID_NEW_PRE_ATTACK_EVENT, wxT( "Add pre attack event" ) );
	menu->Connect( ID_NEW_PRE_ATTACK_EVENT, wxEVT_COMMAND_MENU_SELECTED,
		wxCommandEventHandler( CEdAnimEventsTimeline::OnNewEvent ), NULL, this );

	menu->Append( ID_NEW_COMBAT_EVENT, wxT( "Add combat event" ) );
	menu->Connect( ID_NEW_COMBAT_EVENT, wxEVT_COMMAND_MENU_SELECTED,
		wxCommandEventHandler( CEdAnimEventsTimeline::OnNewEvent ), NULL, this );

    menu->Append( ID_NEW_EFFECT_EVENT, wxT( "Add effect event" ) );
    menu->Connect( ID_NEW_EFFECT_EVENT, wxEVT_COMMAND_MENU_SELECTED,
        wxCommandEventHandler( CEdAnimEventsTimeline::OnNewEvent ), NULL, this );
    
	menu->Append( ID_NEW_EFFECT_DURATION_EVENT, wxT( "Add effect duration event" ) );
	menu->Connect( ID_NEW_EFFECT_DURATION_EVENT, wxEVT_COMMAND_MENU_SELECTED,
		wxCommandEventHandler( CEdAnimEventsTimeline::OnNewEvent ), NULL, this );
	
	menu->Append( ID_NEW_RAISE_EVENT_EVENT, wxT( "Add 'raise event' event" ) );
	menu->Connect( ID_NEW_RAISE_EVENT_EVENT, wxEVT_COMMAND_MENU_SELECTED,
		wxCommandEventHandler( CEdAnimEventsTimeline::OnNewEvent ), NULL, this );

	{
		wxMenu* subMenu = new wxMenu();

		subMenu->Append( ID_NEW_SCENE_DIALOG_DISABLE_LOOKAT, wxT( "Add disable dialog lookat event" ) );
		menu->Connect( ID_NEW_SCENE_DIALOG_DISABLE_LOOKAT, wxEVT_COMMAND_MENU_SELECTED,
			wxCommandEventHandler( CEdAnimEventsTimeline::OnNewEvent ), NULL, this );

		subMenu->Append( ID_NEW_SCENE_DIALOG_EXPRESSION_POINT , wxT( "Animation expression point" ) );
		menu->Connect( ID_NEW_SCENE_DIALOG_EXPRESSION_POINT, wxEVT_COMMAND_MENU_SELECTED,
			wxCommandEventHandler( CEdAnimEventsTimeline::OnNewEvent ), NULL, this );

		subMenu->Append( ID_NEW_SCENE_DIALOG_EXPRESSION_DURATION , wxT( "Animation expression duration" ) );
		menu->Connect( ID_NEW_SCENE_DIALOG_EXPRESSION_DURATION, wxEVT_COMMAND_MENU_SELECTED,
			wxCommandEventHandler( CEdAnimEventsTimeline::OnNewEvent ), NULL, this );

		menu->Append( wxID_ANY, wxT("Dialog"), subMenu );
	}
	

	{
		wxMenu* subMenu = new wxMenu();

		subMenu->Append( ID_NEW_LOCATION_ADJUSTMENT_EVENT, wxT( "Add location adjustment (target loc) event" ) );
		menu->Connect( ID_NEW_LOCATION_ADJUSTMENT_EVENT, wxEVT_COMMAND_MENU_SELECTED,
			wxCommandEventHandler( CEdAnimEventsTimeline::OnNewEvent ), NULL, this );

		subMenu->Append( ID_NEW_ROTATION_ADJUSTMENT_EVENT, wxT( "Add rotation adjustment (target dir) event" ) );
		menu->Connect( ID_NEW_ROTATION_ADJUSTMENT_EVENT, wxEVT_COMMAND_MENU_SELECTED,
			wxCommandEventHandler( CEdAnimEventsTimeline::OnNewEvent ), NULL, this );

		subMenu->Append( ID_NEW_ROTATION_ADJUSTMENT_LOCATION_BASED_EVENT, wxT( "Add rotation adjustment (location based) event" ) );
		menu->Connect( ID_NEW_ROTATION_ADJUSTMENT_LOCATION_BASED_EVENT, wxEVT_COMMAND_MENU_SELECTED,
			wxCommandEventHandler( CEdAnimEventsTimeline::OnNewEvent ), NULL, this );

		subMenu->Append( ID_NEW_ON_SLOPE_EVENT_ON_SLOPE_TRACK, wxT( "Add \"on slope\" event on \"OnSlope\" track" ) );
		menu->Connect( ID_NEW_ON_SLOPE_EVENT_ON_SLOPE_TRACK, wxEVT_COMMAND_MENU_SELECTED,
			wxCommandEventHandler( CEdAnimEventsTimeline::OnNewEvent ), NULL, this );

		menu->Append( wxID_ANY, wxT("Movement"), subMenu );
	}

	{
		wxMenu* subMenu = new wxMenu();

		subMenu->Append( ID_NEW_ITEM_EVENT, wxT( "Add item event" ) );
		menu->Connect( ID_NEW_ITEM_EVENT, wxEVT_COMMAND_MENU_SELECTED,
			wxCommandEventHandler( CEdAnimEventsTimeline::OnNewEvent ), NULL, this );

		subMenu->Append( ID_NEW_ITEM_EFFECT_EVENT, wxT( "Add item effect event" ) );
		menu->Connect( ID_NEW_ITEM_EFFECT_EVENT, wxEVT_COMMAND_MENU_SELECTED,
			wxCommandEventHandler( CEdAnimEventsTimeline::OnNewEvent ), NULL, this );

		subMenu->Append( ID_NEW_ITEM_EFFECT_DURATION_EVENT, wxT( "Add item effect duration event" ) );
		menu->Connect( ID_NEW_ITEM_EFFECT_DURATION_EVENT, wxEVT_COMMAND_MENU_SELECTED,
			wxCommandEventHandler( CEdAnimEventsTimeline::OnNewEvent ), NULL, this );

		subMenu->Append( ID_NEW_ANIMATE_ITEM_EVENT, wxT( "Add item animation event" ) );
		menu->Connect( ID_NEW_ANIMATE_ITEM_EVENT, wxEVT_COMMAND_MENU_SELECTED,
			wxCommandEventHandler( CEdAnimEventsTimeline::OnNewEvent ), NULL, this );

		subMenu->Append( ID_NEW_ITEM_BEHAVIOR_EVENT, wxT( "Add item behavior event" ) );
		menu->Connect( ID_NEW_ITEM_BEHAVIOR_EVENT, wxEVT_COMMAND_MENU_SELECTED,
			wxCommandEventHandler( CEdAnimEventsTimeline::OnNewEvent ), NULL, this );

		subMenu->Append( ID_NEW_ITEM_DROP_EVENT, wxT( "Add item drop event" ) );
		menu->Connect( ID_NEW_ITEM_DROP_EVENT, wxEVT_COMMAND_MENU_SELECTED,
			wxCommandEventHandler( CEdAnimEventsTimeline::OnNewEvent ), NULL, this );

		subMenu->Append( ID_NEW_ITEM_SYNC_EVENT, wxT( "Add item sync event" ) );
		menu->Connect( ID_NEW_ITEM_SYNC_EVENT, wxEVT_COMMAND_MENU_SELECTED,
			wxCommandEventHandler( CEdAnimEventsTimeline::OnNewEvent ), NULL, this );

		subMenu->Append( ID_NEW_ITEM_SYNC_DURATION_EVENT, wxT( "Add item sync duration event" ) );
		menu->Connect( ID_NEW_ITEM_SYNC_DURATION_EVENT, wxEVT_COMMAND_MENU_SELECTED,
			wxCommandEventHandler( CEdAnimEventsTimeline::OnNewEvent ), NULL, this );

		subMenu->Append( ID_NEW_ITEM_SYNC_WITH_CORR_EVENT, wxT( "Add item sync event with correction" ) );
		menu->Connect( ID_NEW_ITEM_SYNC_WITH_CORR_EVENT, wxEVT_COMMAND_MENU_SELECTED,
			wxCommandEventHandler( CEdAnimEventsTimeline::OnNewEvent ), NULL, this );

		menu->Append( wxID_ANY, wxT("Items"), subMenu );
	}

	menu->Append( ID_NEW_GAMEPLAY_MIMIC_STATE_EVENT, wxT( "Add gameplay mimic state event" ) );
	menu->Connect( ID_NEW_GAMEPLAY_MIMIC_STATE_EVENT, wxEVT_COMMAND_MENU_SELECTED,
		wxCommandEventHandler( CEdAnimEventsTimeline::OnNewEvent ), NULL, this );

	{
		wxMenu* subMenu = new wxMenu();

		subMenu->Append( ID_EXP_SYNC_EVENT, wxT( "Add sync event" ) );
		menu->Connect( ID_EXP_SYNC_EVENT, wxEVT_COMMAND_MENU_SELECTED,
			wxCommandEventHandler( CEdAnimEventsTimeline::OnNewEvent ), NULL, this );

		subMenu->Append( ID_EXP_SLIDE_EVENT, wxT( "Add slide event" ) );
		menu->Connect( ID_EXP_SLIDE_EVENT, wxEVT_COMMAND_MENU_SELECTED,
			wxCommandEventHandler( CEdAnimEventsTimeline::OnNewEvent ), NULL, this );

		menu->Append( wxID_ANY, wxT("Explorations"), subMenu );
	}

	{
		wxMenu* subMenu = new wxMenu();

		subMenu->Append( ID_SCRIPT_SLIDE_TO_TARGET_EVENT, wxT( "Add 'Slide To Target' duration event" ) );
		menu->Connect( ID_SCRIPT_SLIDE_TO_TARGET_EVENT, wxEVT_COMMAND_MENU_SELECTED,
			wxCommandEventHandler( CEdAnimEventsTimeline::OnNewEvent ), NULL, this );

		subMenu->Append( ID_SCRIPT_ENUM_EVENT, wxT( "Add 'Enum' duration event" ) );
		menu->Connect( ID_SCRIPT_ENUM_EVENT, wxEVT_COMMAND_MENU_SELECTED,
			wxCommandEventHandler( CEdAnimEventsTimeline::OnNewEvent ), NULL, this );

		subMenu->Append( ID_SCRIPT_MULTI_VALUE_SIMPLE_EVENT, wxT( "Add 'Multi value' event" ) );
		menu->Connect( ID_SCRIPT_MULTI_VALUE_SIMPLE_EVENT, wxEVT_COMMAND_MENU_SELECTED,
			wxCommandEventHandler( CEdAnimEventsTimeline::OnNewEvent ), NULL, this );

		subMenu->Append( ID_SCRIPT_MULTI_VALUE_EVENT, wxT( "Add 'Multi-value' duration event" ) );
		menu->Connect( ID_SCRIPT_MULTI_VALUE_EVENT, wxEVT_COMMAND_MENU_SELECTED,
			wxCommandEventHandler( CEdAnimEventsTimeline::OnNewEvent ), NULL, this );

		menu->Append( wxID_ANY, wxT("Custom Script Events"), subMenu );
	}
}

void CEdAnimEventsTimeline::FillCanvasMenu( wxMenu* menu )
{
	//wxMenuItem* item = menu->AppendCheckItem( ID_SHOW_ALL_TRACKS, wxT( "Show all tracks" ) );
	//item->Check( m_showAllTracks );
	//menu->Connect( ID_SHOW_ALL_TRACKS, wxEVT_COMMAND_MENU_SELECTED,
	//	wxCommandEventHandler( CEdAnimEventsTimeline::OnShowAllTracksToggled ), NULL, this );
}

void CEdAnimEventsTimeline::SetAnimation( const CName& animationName, Float animationDuration,
	const TDynArray< IEventsContainer* >& eventContainers )
{ 
	ClearItems();

	if ( animationDuration < 0.0f )
	{
		// get duration automatically
		for( Uint32 i = 1; i < eventContainers.Size(); ++i )
		{
			IEventsContainer* container = eventContainers[ i ];
			if ( container )
			{
				TDynArray< CExtAnimEvent* > events;
				container->GetEventsForAnimation( m_animationName, events );
				for( TDynArray< CExtAnimEvent* >::iterator eventIter = events.Begin();
					eventIter != events.End(); ++eventIter )
				{
					CExtAnimEvent* event = *eventIter;
					ASSERT( event != NULL );

					animationDuration = Max( animationDuration, event->GetStartTime() + 1.0f );
				}
			}
		}
	}

	m_animationName = animationName;
	m_eventsContainer = NULL;
	m_activeRangeDuration = animationDuration;
	m_verticalOffset = 0;

	SetTimeLimits( 0.0f, animationDuration );

	if( m_animationName.Empty() || eventContainers.Size() == 0 )
	{
		RecreateTracks();
		CalculateNewGrid();
		NotifyBufferIsInvalid();
		return;
	}

	// Main events file
	m_eventsContainer = eventContainers[ 0 ];

	// Create timeline objects
	TDynArray< CExtAnimEvent* > events;
	m_eventsContainer->GetEventsForAnimation( m_animationName, events );
	for( TDynArray< CExtAnimEvent* >::iterator eventIter = events.Begin();
		eventIter != events.End(); ++eventIter )
	{
		CExtAnimEvent* event = *eventIter;
		ASSERT( event != NULL );

		AddItem( CreateTimelineItem( event, m_eventsContainer ) );
	}

	// Create ghost objects from other files
	for( Uint32 i = 1; i < eventContainers.Size(); ++i )
	{
		IEventsContainer* container = eventContainers[ i ];
		if ( container )
		{
			TDynArray< CExtAnimEvent* > events;
			container->GetEventsForAnimation( m_animationName, events );
			for( TDynArray< CExtAnimEvent* >::iterator eventIter = events.Begin();
				eventIter != events.End(); ++eventIter )
			{
				CExtAnimEvent* event = *eventIter;
				ASSERT( event != NULL );

				// Create ghost item
				ITimelineItem* item = new CTimelineItemGhostEvent( CreateTimelineItem( event, container ), container );
				AddItem( item );
			}
		}
	}

	RecreateTracks();

	// Center timeline
	m_visibleRangeDuration = m_activeRangeDuration * 1.2f;
	CenterPosition( m_activeRangeDuration / 2.2f );

	CalculateNewGrid();
	NotifyBufferIsInvalid();
}

void CEdAnimEventsTimeline::OnNewEvent( wxCommandEvent& event )
{
	ASSERT( m_selectedTrack < ( Int32 ) m_tracks.Size() );
	ASSERT( m_eventsContainer != NULL );

	// Create new event where the user clicked
	CExtAnimEvent* newEvent = NULL;
	Bool forcedDuration = false;

	switch( event.GetId() )
	{
	case ID_NEW_SIMPLE_EVENT:
		newEvent = new CExtAnimEvent( 
			CName( m_tracks[ m_selectedTrack ]->m_name ),
			m_animationName, m_cursorTimePos, m_tracks[ m_selectedTrack ]->m_name );
		break;
	case ID_NEW_SOUND_EVENT:
		newEvent = new CExtAnimSoundEvent( 
			CName( m_tracks[ m_selectedTrack ]->m_name ),
			m_animationName, m_cursorTimePos, m_tracks[ m_selectedTrack ]->m_name );
		break;
	case ID_NEW_FOOTSTEP_EVENT:
		newEvent = new CExtAnimFootstepEvent( 
			CName( m_tracks[ m_selectedTrack ]->m_name ),
			m_animationName, m_cursorTimePos, m_tracks[ m_selectedTrack ]->m_name );
		break;
	case ID_NEW_FORCED_LOGICAL_FOOTSTEP_EVENT: //this is so sad..
		newEvent = new CExtForcedLogicalFootstepAnimEvent( 
			CName( m_tracks[ m_selectedTrack ]->m_name ),
			m_animationName, m_cursorTimePos, m_tracks[ m_selectedTrack ]->m_name );
		break;
	case ID_NEW_MATERIAL_BASED_FX_EVENT:
		newEvent = new CExtAnimMaterialBasedFxEvent( 
			CName( m_tracks[ m_selectedTrack ]->m_name ),
			m_animationName, m_cursorTimePos, m_tracks[ m_selectedTrack ]->m_name );
		break;
	case ID_NEW_DURATION_EVENT:
		newEvent = new CExtAnimDurationEvent( 
			CName( m_tracks[ m_selectedTrack ]->m_name ),
			m_animationName, m_cursorTimePos, 0.1f,
			m_tracks[ m_selectedTrack ]->m_name );
		break;
	case ID_NEW_MORPH_EVENT:
		newEvent = new CExtAnimMorphEvent( 
			CName( m_tracks[ m_selectedTrack ]->m_name ),
			m_animationName, m_cursorTimePos, 0.1f,
			m_tracks[ m_selectedTrack ]->m_name );
		break;
	case ID_NEW_EXPLORATION_EVENT:
		newEvent = new CExtAnimExplorationEvent( 
			CName( m_tracks[ m_selectedTrack ]->m_name ),
			m_animationName, m_cursorTimePos, 0.1f,
			m_tracks[ m_selectedTrack ]->m_name );
		break;
	case ID_NEW_ITEM_EVENT:
		newEvent = new CExtAnimItemEvent( 
			CName( m_tracks[ m_selectedTrack ]->m_name ),
			m_animationName, m_cursorTimePos,
			m_tracks[ m_selectedTrack ]->m_name );
		break;
	case ID_NEW_ITEM_EFFECT_EVENT:
		newEvent = new CExtAnimItemEffectEvent(
			CName( m_tracks[ m_selectedTrack ]->m_name ),
			m_animationName, m_cursorTimePos,
			m_tracks[ m_selectedTrack ]->m_name );
		break;
	case ID_NEW_ITEM_EFFECT_DURATION_EVENT:
		newEvent = new CExtAnimItemEffectDurationEvent(
			CName( m_tracks[ m_selectedTrack ]->m_name ),
			m_animationName, m_cursorTimePos, 0.1f,
			m_tracks[ m_selectedTrack ]->m_name );
		break;		
	case ID_NEW_COMBO_EVENT:
		newEvent = new CExtAnimComboEvent( 
			CName( m_tracks[ m_selectedTrack ]->m_name ),
			m_animationName, m_cursorTimePos, 0.1f,
			m_tracks[ m_selectedTrack ]->m_name );
		break;
	case ID_NEW_HIT_EVENT:
		newEvent = new CExtAnimHitEvent( 
			CName( m_tracks[ m_selectedTrack ]->m_name ),
			m_animationName, m_cursorTimePos,
			m_tracks[ m_selectedTrack ]->m_name );
		break;
	case ID_NEW_LOOK_AT_EVENT:
		newEvent = new CExtAnimLookAtEvent( 
			CName( m_tracks[ m_selectedTrack ]->m_name ),
			m_animationName, m_cursorTimePos, 0.1f,
			m_tracks[ m_selectedTrack ]->m_name );
		break;
	case ID_NEW_SCENE_DIALOG_DISABLE_LOOKAT:
		newEvent = new CExtAnimDisableDialogLookatEvent( 
			CName( m_tracks[ m_selectedTrack ]->m_name ),
			m_animationName, m_cursorTimePos, 0.1f,
			m_tracks[ m_selectedTrack ]->m_name );
		break;	
	case ID_NEW_SCENE_DIALOG_EXPRESSION_POINT:
		newEvent = new CExtAnimDialogKeyPoseMarker( 
			CName( m_tracks[ m_selectedTrack ]->m_name ),
			m_animationName, m_cursorTimePos, 
			m_tracks[ m_selectedTrack ]->m_name );
		break;	
	case ID_NEW_SCENE_DIALOG_EXPRESSION_DURATION:
		newEvent = new CExtAnimDialogKeyPoseDuration( 
			CName( m_tracks[ m_selectedTrack ]->m_name ),
			m_animationName, m_cursorTimePos, 
			m_tracks[ m_selectedTrack ]->m_name );
		break;	
	case ID_NEW_MAGIC_EVENT:
		newEvent = new CExtAnimProjectileEvent( 
			CName( m_tracks[ m_selectedTrack ]->m_name ),
			m_animationName, m_cursorTimePos,
			m_tracks[ m_selectedTrack ]->m_name );
		break;
	case ID_NEW_COMBAT_EVENT:
		newEvent = new CExtAnimAttackEvent( 
			CName( m_tracks[ m_selectedTrack ]->m_name ),
			m_animationName, m_cursorTimePos,
			m_tracks[ m_selectedTrack ]->m_name );
        break;
    case ID_NEW_EFFECT_EVENT:
        newEvent = new CExtAnimEffectEvent(
            CName( m_tracks[ m_selectedTrack ]->m_name ),
            m_animationName, m_cursorTimePos,
            m_tracks[ m_selectedTrack ]->m_name );
        break;
	case ID_NEW_EFFECT_DURATION_EVENT:
		newEvent = new CExtAnimEffectDurationEvent(
			CName( m_tracks[ m_selectedTrack ]->m_name ),
			m_animationName, m_cursorTimePos, 0.1f,
			m_tracks[ m_selectedTrack ]->m_name );
		break;
	case ID_NEW_RAISE_EVENT_EVENT:
		newEvent = new CExtAnimRaiseEventEvent(
			CName( m_tracks[ m_selectedTrack ]->m_name ),
			m_animationName, m_cursorTimePos,
			m_tracks[ m_selectedTrack ]->m_name );
		break;
	case ID_NEW_LOCATION_ADJUSTMENT_EVENT:
		newEvent = new CExtAnimLocationAdjustmentEvent( 
			CName( m_tracks[ m_selectedTrack ]->m_name ),
			m_animationName, m_cursorTimePos, 0.1f,
			m_tracks[ m_selectedTrack ]->m_name );
		break;
	case ID_NEW_ROTATION_ADJUSTMENT_EVENT:
		newEvent = new CExtAnimRotationAdjustmentEvent( 
			CName( m_tracks[ m_selectedTrack ]->m_name ),
			m_animationName, m_cursorTimePos, 0.1f,
			m_tracks[ m_selectedTrack ]->m_name );
		break;
	case ID_NEW_ROTATION_ADJUSTMENT_LOCATION_BASED_EVENT:
		newEvent = new CExtAnimRotationAdjustmentLocationBasedEvent(
			CName( m_tracks[ m_selectedTrack ]->m_name ),
			m_animationName, m_cursorTimePos, 0.1f,
			m_tracks[ m_selectedTrack ]->m_name );
		break;
	case ID_NEW_ON_SLOPE_EVENT_ON_SLOPE_TRACK:
		AddTrack( CExtAnimOnSlopeEvent::GetDefaultTrackName() );
		newEvent = new CExtAnimOnSlopeEvent(
			CName( CExtAnimOnSlopeEvent::GetDefaultTrackName() ),
			m_animationName, 0.0f, m_activeRangeDuration,
			CExtAnimOnSlopeEvent::GetDefaultTrackName() );
		forcedDuration = true;
		break;
	case ID_NEW_ANIMATE_ITEM_EVENT:
		newEvent = new CExtAnimItemAnimationEvent( 
			CName( m_tracks[ m_selectedTrack ]->m_name ),
			m_animationName, m_cursorTimePos,
			m_tracks[ m_selectedTrack ]->m_name );
		break;
	case ID_NEW_ITEM_BEHAVIOR_EVENT:
		newEvent = new CExtAnimItemBehaviorEvent( 
			CName( m_tracks[ m_selectedTrack ]->m_name ),
			m_animationName, m_cursorTimePos,
			m_tracks[ m_selectedTrack ]->m_name );
		break;
	case ID_NEW_ITEM_DROP_EVENT:
		newEvent = new CExtAnimDropItemEvent( 
			CName( m_tracks[ m_selectedTrack ]->m_name ),
			m_animationName, m_cursorTimePos,
			m_tracks[ m_selectedTrack ]->m_name );
		break;
	case ID_NEW_ITEM_SYNC_EVENT:
		newEvent = new CExtAnimItemSyncEvent( 
			CName( m_tracks[ m_selectedTrack ]->m_name ),
			m_animationName, m_cursorTimePos,
			m_tracks[ m_selectedTrack ]->m_name );
		break;
	case ID_NEW_ITEM_SYNC_DURATION_EVENT:
		newEvent = new CExtAnimItemSyncDurationEvent( 
			CName( m_tracks[ m_selectedTrack ]->m_name ),
			m_animationName, m_cursorTimePos, 0.1f,
			m_tracks[ m_selectedTrack ]->m_name );
		break;
	case ID_NEW_ITEM_SYNC_WITH_CORR_EVENT:
		newEvent = new CExtAnimItemSyncWithCorrectionEvent( 
			CName( m_tracks[ m_selectedTrack ]->m_name ),
			m_animationName, m_cursorTimePos, 0.1f,
			m_tracks[ m_selectedTrack ]->m_name );
		break;
	case ID_NEW_GAMEPLAY_MIMIC_STATE_EVENT:
		newEvent = new CExtAnimGameplayMimicEvent( 
			CName( m_tracks[ m_selectedTrack ]->m_name ),
			m_animationName, m_cursorTimePos, 0.1f,
			m_tracks[ m_selectedTrack ]->m_name );
		break;
	case ID_EXP_SYNC_EVENT:
		newEvent = new CExpSyncEvent( 
			CName( m_tracks[ m_selectedTrack ]->m_name ),
			m_animationName, m_cursorTimePos,
			m_tracks[ m_selectedTrack ]->m_name );
		break;
	case ID_EXP_SLIDE_EVENT:
		newEvent = new CExpSlideEvent( 
			CName( m_tracks[ m_selectedTrack ]->m_name ),
			m_animationName, m_cursorTimePos, 0.1f,
			m_tracks[ m_selectedTrack ]->m_name );
		break;
	case ID_SCRIPT_ENUM_EVENT:
		newEvent = new CEASEnumEvent(
			CName( m_tracks[ m_selectedTrack ]->m_name ),
			m_animationName, m_cursorTimePos, 0.1f,
			m_tracks[ m_selectedTrack ]->m_name );
		break;
	case ID_SCRIPT_SLIDE_TO_TARGET_EVENT:
		newEvent = new CEASSlideToTargetEvent(
			CName( m_tracks[ m_selectedTrack ]->m_name ),
			m_animationName, m_cursorTimePos, 0.1f,
			m_tracks[ m_selectedTrack ]->m_name );
		break;
	case ID_SCRIPT_MULTI_VALUE_EVENT:
		newEvent = new CEASMultiValueEvent(
			CName( m_tracks[ m_selectedTrack ]->m_name ),
			m_animationName, m_cursorTimePos, 0.1f,
			m_tracks[ m_selectedTrack ]->m_name );
		break;
	case ID_SCRIPT_MULTI_VALUE_SIMPLE_EVENT:
		newEvent = new CEASMultiValueSimpleEvent(
			CName( m_tracks[ m_selectedTrack ]->m_name ),
			m_animationName, m_cursorTimePos,
			m_tracks[ m_selectedTrack ]->m_name );
		break;
	case ID_NEW_PRE_ATTACK_EVENT:
		newEvent = new CPreAttackEvent(
			CName( m_tracks[ m_selectedTrack ]->m_name ),
			m_animationName, m_cursorTimePos, 0.1f,
			m_tracks[ m_selectedTrack ]->m_name );
		break;
	default:
		ASSERT( 0 && "Unknown command" );
		return;
	};

	m_eventsContainer->AddEvent( newEvent );

	ITimelineItem* item = CreateTimelineItem( newEvent, m_eventsContainer );

	if ( m_undoManager )
	{
 		CUndoTimelineItemExistance::PrepareCreationStep( *m_undoManager, this, item );
 		CUndoTimelineItemExistance::FinalizeStep( *m_undoManager );
	}

	AddItem( item );

	if( item->IsDuration() && ! forcedDuration )
	{
		SetDurationEventLength( item );
	}

	m_selectedItems.PushBackUnique( item );

	SelectionChanged();

	//RecreateTracks();
}

void CEdAnimEventsTimeline::OnShowAllTracksToggled( wxCommandEvent& event )
{
	RecreateTracks();
}

CExtAnimEvent* CEdAnimEventsTimeline::DeserializeAsEvent( IFile& reader )
{
	// Read type
	CName eventType;
	reader << eventType;

	// Find type in RTTI
	CClass* theClass = SRTTI::GetInstance().FindClass( eventType );
	ASSERT( theClass != NULL );

	// Create object
	CExtAnimEvent* animEvent = theClass->CreateObject< CExtAnimEvent >() ;
	ASSERT( animEvent != NULL );

	// Serialize object
	theClass->Serialize( reader, animEvent );

	return animEvent;
}

void CEdAnimEventsTimeline::SerializeEvent( CExtAnimEvent* event, IFile& writer )
{
	// Serialize type
	CName eventType = event->GetClass()->GetName();
	writer << eventType;
	event->GetClass()->Serialize( writer, event );
}

ITimelineItem* CEdAnimEventsTimeline::DeserializeItem( IFile& reader )
{
	ASSERT( m_eventsContainer != NULL );

	CExtAnimEvent* animEvent = DeserializeAsEvent( reader );

	animEvent->SetAnimationName( m_animationName );
	m_eventsContainer->AddEvent( animEvent );

	return CreateTimelineItem( animEvent, m_eventsContainer );
}

void CEdAnimEventsTimeline::SerializeItem( ITimelineItem* item, IFile& writer )
{
	if ( CTimelineItemEvent* itemEvent = dynamic_cast< CTimelineItemEvent* >( item ) )
	{
		CExtAnimEvent* event = itemEvent->GetEvent();
		ASSERT( event != NULL );

		SerializeEvent( event, writer );
	}
}

CEdAnimEventsTimeline::CTimelineItemEvent* CEdAnimEventsTimeline::CreateTimelineItem( CExtAnimEvent* event, const IEventsContainer* container )
{
	CEdAnimEventsTimeline::CTimelineItemEvent* retItem = NULL;
	if( IsType< CExtAnimFootstepEvent >( event ) )
	{
		retItem = new CTimelineItemFootstep( Cast< CExtAnimFootstepEvent >(  event ) );
	}
	else if( IsType< CExtAnimComboEvent >( event ) )
	{
		retItem = new CTimelineItemCombo( Cast< CExtAnimComboEvent >(  event ) );
	}
	else if( IsType< CExtAnimLookAtEvent >( event ) )
	{
		retItem = new CTimelineItemLookAt( Cast< CExtAnimLookAtEvent >(  event ) );
	}
	else if( IsType< CExtAnimHitEvent >( event ) )
	{
		retItem = new CTimelineItemHit( Cast< CExtAnimHitEvent >(  event ) );
	}
	else if( IsType< CExtAnimItemEvent >( event ) )
	{
		retItem = new CTimelineItemItem( Cast< CExtAnimItemEvent >(  event ) );
	}
	else if( IsType< CExtAnimItemEffectEvent >( event ) )
	{
		retItem = new CTimelineItemItemEffect( Cast< CExtAnimItemEffectEvent >(  event ) );
	}
	else if( IsType< CExtAnimItemAnimationEvent >( event ) )
	{
		retItem = new CTimelineItemItemAnimation( Cast< CExtAnimItemAnimationEvent >(  event ) );
	}
	else if( IsType< CExtAnimItemBehaviorEvent >( event ) )
	{
		retItem = new CTimelineItemItemBehavior( Cast< CExtAnimItemBehaviorEvent >(  event ) );
	}
	else if( IsType< CExtAnimDropItemEvent >( event ) )
	{
		retItem = new CTimelineItemItemDrop( Cast< CExtAnimDropItemEvent >(  event ) );
	}
	else if( IsType< CExtAnimExplorationEvent >( event ) )
	{
		retItem = new CTimelineItemExploration( Cast< CExtAnimExplorationEvent >(  event ) );
	}
	else if( IsType< CExtAnimMorphEvent >( event ) )
	{
		retItem = new CTimelineItemMorphing( Cast< CExtAnimMorphEvent >(  event ) );
	}
	else if( IsType< CExtAnimProjectileEvent >( event ) )
	{
		retItem = new CTimelineItemProjectile( Cast< CExtAnimProjectileEvent >(  event ) );
	}
	else if( IsType< CExtAnimSoundEvent >( event ) )
	{
		retItem = new CTimelineItemSound( Cast< CExtAnimSoundEvent >(  event ) );
	}
	else if( IsType< CExtAnimEvent >( event ) )
	{
		retItem = new CTimelineItemAnimation( Cast< CExtAnimEvent >(  event ) );
	}
	else if( IsType< CExtAnimItemSyncEvent >( event ) )
	{
		retItem = new CTimelineItemSyncEvent( Cast< CExtAnimItemSyncEvent >(  event ) );
	}
	else if( IsType< CExtAnimItemSyncDurationEvent >( event ) )
	{
		retItem = new CTimelineItemSyncEvent( Cast< CExtAnimItemSyncDurationEvent >(  event ) );
	}
	else if( IsType< CExtAnimItemSyncWithCorrectionEvent >( event ) )
	{
		retItem = new CTimelineItemSyncEvent( Cast< CExtAnimItemSyncWithCorrectionEvent >(  event ) );
	}
	else
	{
		ASSERT( 0 && "Invalid event type" );
		return NULL;
		// return new CTimelineItemEvent( event );
	}
	retItem->SetContainer( container, container == m_eventsContainer );
	return retItem;
}

CExtAnimEvent* CEdAnimEventsTimeline::GetEvent( int n ) const
{
	CTimelineItemEvent* item = dynamic_cast<CTimelineItemEvent*>(m_items[n]);
	return item ? item->GetEvent() : NULL;
}

CExtAnimEvent* CEdAnimEventsTimeline::GetSelectedEvent( int n ) const
{
	CTimelineItemEvent* item = dynamic_cast<CTimelineItemEvent*>(m_selectedItems[n]);
	return item ? item->GetEvent() : NULL;
}

void CEdAnimEventsTimeline::RemoveItemImpl( ITimelineItem* item )
{
	if ( CTimelineItemEvent* itemEvent = dynamic_cast< CTimelineItemEvent* >( item ) )
	{
		ASSERT( itemEvent->GetEvent() != NULL );
		m_eventsContainer->RemoveEvent( itemEvent->GetEvent() );
	}
}

void CEdAnimEventsTimeline::StoreLayout()
{
	// empty - this functionality is not supported
}

void CEdAnimEventsTimeline::RestoreLayout()
{
	// empty - this functionality is not supported
}

void CEdAnimEventsTimeline::EditProperties( ITimelineItem* item, CEdPropertiesPage& propertiesPage ) const
{	
	if( item == NULL )
	{
		return;
	}

	CExtAnimEvent* event = static_cast< CTimelineItemEvent* >( item )->GetEvent();
	ASSERT( event != NULL );

	propertiesPage.SetObject( event );
}

void CEdAnimEventsTimeline::ItemPropertyChanged( TDynArray< ITimelineItem* >& items, const CName& propertyName )
{
	const Uint32 size = items.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		if ( items[ i ] )
		{
			CExtAnimEvent* event = static_cast< CTimelineItemEvent* >( items[ i ] )->GetEvent();
			if ( event )
			{
				event->OnPropertyPostChanged( propertyName );
			}
			else
			{
				ASSERT( event );
			}
		}
		else
		{
			ASSERT( items[ i ] );
		}
	}
}
