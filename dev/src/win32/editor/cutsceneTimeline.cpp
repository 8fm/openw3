/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "cutsceneTimeline.h"
#include "../../common/game/extAnimItemEvents.h"
#include "../../common/game/extAnimFootstepEvent.h"
#include "../../common/game/extAnimCutsceneQuestEvent.h"
#include "../../common/engine/extAnimCutsceneSoundEvent.h"
#include "../../common/engine/extAnimCutsceneFadeEvent.h"
#include "../../common/engine/extAnimMorphEvent.h"
#include "../../common/engine/animationBufferMultipart.h"

#include "cutsceneTimelineItems.inl"
#include "../../games/r4/customCSEvents.h"
#include "../../common/core/diskFile.h"
#include "../../common/engine/extAnimCutsceneActorEffect.h"
#include "../../common/engine/extAnimCutsceneEffectEvent.h"
#include "../../common/engine/extAnimCutsceneEnvironmentEvent.h"

enum ETrackEvents
{
	ID_NEW_SIMPLE_EVENT = 4001,
	ID_NEW_SOUND_EVENT,
	ID_NEW_DURATION_EVENT,
	ID_NEW_EXPLORATION_EVENT,
	ID_NEW_CUTSCENE_ACTOR_EFFECT_EVENT,
	ID_NEW_CUTSCENE_BODY_PART_EVENT,
	ID_NEW_RESET_CLOTH_EVENT,
	ID_NEW_DISABLE_CLOTH_EVENT,
	ID_NEW_DISABLE_DANGLE_EVENT,
	ID_NEW_CUTSCENE_DIALOG_EVENT,
	ID_NEW_CUTSCENE_EFFECT_EVENT,
	ID_NEW_CUTSCENE_SLOWMO_EVENT,
	ID_NEW_CUTSCENE_WIND_EVENT,
	ID_NEW_CUTSCENE_ENVIRONMENT_EVENT,
	ID_NEW_CUTSCENE_LIGHT_EVENT,
	ID_NEW_CUTSCENE_DOF_EVENT,
	ID_NEW_CUTSCENE_DOF_BLEND_EVENT,
	ID_NEW_CUTSCENE_SET_CLIPPING_PLANES_EVENT,
	ID_NEW_FOOTSTEP_EVENT,
	ID_NEW_ITEM_EVENT,
	ID_NEW_ITEM_EFFECT_EVENT,
	ID_NEW_ITEM_EFFECT_DURATION_EVENT,
	ID_NEW_COMBO_EVENT,
	ID_NEW_MUSIC_EVENT,
	ID_NEW_CUTSCENE_QUEST_EVENT,
	ID_NEW_FADE_EVENT,
	ID_NEW_SURFACE_EFFECT,
	ID_NEW_HIDE_ENTITY_EVENT,
	ID_NEW_TOGGLE_TERRAIN_EVENT,
	ID_NEW_CUTSCENE_BREAK_EVENT,
	ID_NEW_CUTSCENE_AMB_EVENT,
	ID_SHOW_ALL_TRACKS,
	ID_CREATE_MUSIC_TRACK,
	ID_NEW_REATTACH_ITEM_EVENT,
	ID_NEW_MORPH_EVENT,

	ID_CS_PAUSE,
	ID_CS_TO_GAMEPLAY
};

CCutsceneTemplate* CEdCutsceneTimeline::m_currentCutscene = NULL;
CName CEdCutsceneTimeline::m_currentEntity;

wxBEGIN_EVENT_TABLE( CEdCutsceneTimeline, CEdTimeline )
	EVT_MENU( ID_CS_PAUSE, CEdCutsceneTimeline::OnTogglePause )
	EVT_MENU( ID_CS_TO_GAMEPLAY, CEdCutsceneTimeline::OnCsToGameplay )
wxEND_EVENT_TABLE()

CEdCutsceneTimeline::CEdCutsceneTimeline( CEdCutsceneEditor* ed, wxPanel* parentPanel, CEdPropertiesPage* propertyPage )
	: CEdTimeline( parentPanel, propertyPage, CanvasType::gdiplus, TXT("Camera") )
	, m_trackName( CName::NONE )
	, m_eventsSource( NULL )
	, m_editor( ed )
{
	{
		SetWindowStyleFlag( wxWANTS_CHARS );

		const Uint32 numAccelerators = 2;
		wxAcceleratorEntry shortcutsEntries[ 2 ];
		shortcutsEntries[ 0 ].Set( wxACCEL_NORMAL,	WXK_SPACE, ID_CS_PAUSE );
		shortcutsEntries[ 1 ].Set( wxACCEL_NORMAL,	'g', ID_CS_TO_GAMEPLAY );

		wxAcceleratorTable shortcutsTable( numAccelerators, shortcutsEntries );
		SetAcceleratorTable( shortcutsTable );
	}

	RegisterDrawGroupTracksPinned(new TimelineImpl::CDrawGroupTracks(*this, &m_pinnedBuffer));
	RegisterDrawGroupTracksDefault(new TimelineImpl::CDrawGroupTracks(*this, &m_defaultBuffer));
	RegisterDrawGroupTimebar(new TimelineImpl::CDrawGroupTimebar(*this, &m_timebarBuffer));
	RegisterDrawGroupVolatile(new TimelineImpl::CDrawGroupVolatile(*this, &m_volatileBuffer));
}

CEdCutsceneTimeline::~CEdCutsceneTimeline()
{}

void CEdCutsceneTimeline::FillCanvasMenu( wxMenu* menu )
{
}

void CEdCutsceneTimeline::FillTrackMenu( const String& trackName, wxMenu* menu )
{
	if( m_trackName == CCutsceneTemplate::CUTSCENE_ANIMATION_NAME )
	{
		menu->Append( ID_NEW_SOUND_EVENT, wxT( "Add sound event" ) );
		menu->Connect( ID_NEW_SOUND_EVENT, wxEVT_COMMAND_MENU_SELECTED,
			wxCommandEventHandler( CEdCutsceneTimeline::OnNewEvent ), NULL, this );

		menu->Append( ID_NEW_MUSIC_EVENT, wxT( "Add music event" ) );
		menu->Connect( ID_NEW_MUSIC_EVENT, wxEVT_COMMAND_MENU_SELECTED,
			wxCommandEventHandler( CEdCutsceneTimeline::OnNewEvent ), NULL, this );

		menu->Append( ID_NEW_CUTSCENE_DOF_EVENT, wxT( "Add bokeh DOF event" ) );
		menu->Connect( ID_NEW_CUTSCENE_DOF_EVENT, wxEVT_COMMAND_MENU_SELECTED,
			wxCommandEventHandler( CEdCutsceneTimeline::OnNewEvent ), NULL, this );

		menu->Append( ID_NEW_CUTSCENE_DOF_BLEND_EVENT, wxT( "Add bokeh DOF blend event" ) );
		menu->Connect( ID_NEW_CUTSCENE_DOF_BLEND_EVENT, wxEVT_COMMAND_MENU_SELECTED,
			wxCommandEventHandler( CEdCutsceneTimeline::OnNewEvent ), NULL, this );

		menu->Append( ID_NEW_CUTSCENE_SET_CLIPPING_PLANES_EVENT, wxT( "Add clipping planes event" ) );
		menu->Connect( ID_NEW_CUTSCENE_SET_CLIPPING_PLANES_EVENT, wxEVT_COMMAND_MENU_SELECTED,
			wxCommandEventHandler( CEdCutsceneTimeline::OnNewEvent ), NULL, this );

		menu->Append( ID_NEW_CUTSCENE_AMB_EVENT, wxT( "Add ambients event" ) );
		menu->Connect( ID_NEW_CUTSCENE_AMB_EVENT, wxEVT_COMMAND_MENU_SELECTED,
			wxCommandEventHandler( CEdCutsceneTimeline::OnNewEvent ), NULL, this );

		menu->Append( ID_NEW_CUTSCENE_DIALOG_EVENT, wxT( "Add dialog event" ) );
		menu->Connect( ID_NEW_CUTSCENE_DIALOG_EVENT, wxEVT_COMMAND_MENU_SELECTED,
			wxCommandEventHandler( CEdCutsceneTimeline::OnNewEvent ), NULL, this );

		menu->Append( ID_NEW_CUTSCENE_EFFECT_EVENT, wxT( "Add effect event" ) );
		menu->Connect( ID_NEW_CUTSCENE_EFFECT_EVENT, wxEVT_COMMAND_MENU_SELECTED,
			wxCommandEventHandler( CEdCutsceneTimeline::OnNewEvent ), NULL, this );

		menu->Append( ID_NEW_CUTSCENE_SLOWMO_EVENT, wxT( "Add slow mo event" ) );
		menu->Connect( ID_NEW_CUTSCENE_SLOWMO_EVENT, wxEVT_COMMAND_MENU_SELECTED,
			wxCommandEventHandler( CEdCutsceneTimeline::OnNewEvent ), NULL, this );

		menu->Append( ID_NEW_CUTSCENE_WIND_EVENT, wxT( "Add wind event" ) );
		menu->Connect( ID_NEW_CUTSCENE_WIND_EVENT, wxEVT_COMMAND_MENU_SELECTED,
			wxCommandEventHandler( CEdCutsceneTimeline::OnNewEvent ), NULL, this );

		menu->Append( ID_NEW_CUTSCENE_ENVIRONMENT_EVENT, wxT( "Add environment event" ) );
		menu->Connect( ID_NEW_CUTSCENE_ENVIRONMENT_EVENT, wxEVT_COMMAND_MENU_SELECTED,
			wxCommandEventHandler( CEdCutsceneTimeline::OnNewEvent ), NULL, this );

		menu->Append( ID_NEW_CUTSCENE_LIGHT_EVENT, wxT( "Add light event" ) );
		menu->Connect( ID_NEW_CUTSCENE_LIGHT_EVENT, wxEVT_COMMAND_MENU_SELECTED,
			wxCommandEventHandler( CEdCutsceneTimeline::OnNewEvent ), NULL, this );

		menu->Append( ID_NEW_CUTSCENE_QUEST_EVENT, wxT( "Add quest event" ) );
		menu->Connect( ID_NEW_CUTSCENE_QUEST_EVENT, wxEVT_COMMAND_MENU_SELECTED,
			wxCommandEventHandler( CEdCutsceneTimeline::OnNewEvent ), NULL, this );

		menu->Append( ID_NEW_FADE_EVENT, wxT( "Add fade event" ) );
		menu->Connect( ID_NEW_FADE_EVENT, wxEVT_COMMAND_MENU_SELECTED,
			wxCommandEventHandler( CEdCutsceneTimeline::OnNewEvent ), NULL, this );

		menu->Append( ID_NEW_SURFACE_EFFECT, wxT( "Add surface effect" ) );
		menu->Connect( ID_NEW_SURFACE_EFFECT, wxEVT_COMMAND_MENU_SELECTED,
			wxCommandEventHandler( CEdCutsceneTimeline::OnNewEvent ), NULL, this );

		menu->Append( ID_NEW_HIDE_ENTITY_EVENT, wxT( "Add hide entity event" ) );
		menu->Connect( ID_NEW_HIDE_ENTITY_EVENT, wxEVT_COMMAND_MENU_SELECTED,
			wxCommandEventHandler( CEdCutsceneTimeline::OnNewEvent ), NULL, this );

		menu->Append( ID_NEW_TOGGLE_TERRAIN_EVENT, wxT( "Toggle terrain event" ) );
		menu->Connect( ID_NEW_TOGGLE_TERRAIN_EVENT, wxEVT_COMMAND_MENU_SELECTED,
			wxCommandEventHandler( CEdCutsceneTimeline::OnNewEvent ), NULL, this );

		menu->Append( ID_NEW_CUTSCENE_BREAK_EVENT, wxT( "Break" ) );
		menu->Connect( ID_NEW_CUTSCENE_BREAK_EVENT, wxEVT_COMMAND_MENU_SELECTED,
			wxCommandEventHandler( CEdCutsceneTimeline::OnNewEvent ), NULL, this );
	}
	else
	{
		menu->Append( ID_NEW_SIMPLE_EVENT, wxT( "Add simple event" ) );
		menu->Connect( ID_NEW_SIMPLE_EVENT, wxEVT_COMMAND_MENU_SELECTED,
			wxCommandEventHandler( CEdCutsceneTimeline::OnNewEvent ), NULL, this );

		menu->Append( ID_NEW_DURATION_EVENT, wxT( "Add duration event" ) );
		menu->Connect( ID_NEW_DURATION_EVENT, wxEVT_COMMAND_MENU_SELECTED,
			wxCommandEventHandler( CEdCutsceneTimeline::OnNewEvent ), NULL, this );

		menu->Append( ID_NEW_ITEM_EVENT, wxT( "Add item event" ) );
		menu->Connect( ID_NEW_ITEM_EVENT, wxEVT_COMMAND_MENU_SELECTED,
			wxCommandEventHandler( CEdCutsceneTimeline::OnNewEvent ), NULL, this );

		menu->Append( ID_NEW_ITEM_EFFECT_EVENT, wxT( "Add item effect event" ) );
		menu->Connect( ID_NEW_ITEM_EFFECT_EVENT, wxEVT_COMMAND_MENU_SELECTED,
			wxCommandEventHandler( CEdCutsceneTimeline::OnNewEvent ), NULL, this );

		menu->Append( ID_NEW_ITEM_EFFECT_DURATION_EVENT, wxT( "Add item effect duration event" ) );
		menu->Connect( ID_NEW_ITEM_EFFECT_DURATION_EVENT, wxEVT_COMMAND_MENU_SELECTED,
			wxCommandEventHandler( CEdCutsceneTimeline::OnNewEvent ), NULL, this );

		menu->Append( ID_NEW_CUTSCENE_ACTOR_EFFECT_EVENT, wxT( "Add actor effect event" ) );
		menu->Connect( ID_NEW_CUTSCENE_ACTOR_EFFECT_EVENT, wxEVT_COMMAND_MENU_SELECTED,
			wxCommandEventHandler( CEdCutsceneTimeline::OnNewEvent ), NULL, this );

		menu->Append( ID_NEW_CUTSCENE_BODY_PART_EVENT, wxT( "Add body part event" ) );
		menu->Connect( ID_NEW_CUTSCENE_BODY_PART_EVENT, wxEVT_COMMAND_MENU_SELECTED,
			wxCommandEventHandler( CEdCutsceneTimeline::OnNewEvent ), NULL, this );

		menu->Append( ID_NEW_RESET_CLOTH_EVENT, wxT( "Add reset cloth and dangles" ) );
		menu->Connect( ID_NEW_RESET_CLOTH_EVENT, wxEVT_COMMAND_MENU_SELECTED,
			wxCommandEventHandler( CEdCutsceneTimeline::OnNewEvent ), NULL, this );

		menu->Append( ID_NEW_DISABLE_CLOTH_EVENT, wxT( "Add disable cloth" ) );
		menu->Connect( ID_NEW_DISABLE_CLOTH_EVENT, wxEVT_COMMAND_MENU_SELECTED,
			wxCommandEventHandler( CEdCutsceneTimeline::OnNewEvent ), NULL, this );

		menu->Append( ID_NEW_DISABLE_DANGLE_EVENT, wxT( "Add disable dangle" ) );
		menu->Connect( ID_NEW_DISABLE_DANGLE_EVENT, wxEVT_COMMAND_MENU_SELECTED,
			wxCommandEventHandler( CEdCutsceneTimeline::OnNewEvent ), NULL, this );

		menu->Append( ID_NEW_SOUND_EVENT, wxT( "Add sound event" ) );
		menu->Connect( ID_NEW_SOUND_EVENT, wxEVT_COMMAND_MENU_SELECTED,
			wxCommandEventHandler( CEdCutsceneTimeline::OnNewEvent ), NULL, this );

		menu->Append( ID_NEW_FOOTSTEP_EVENT, wxT( "Add footstep event" ) );
		menu->Connect( ID_NEW_FOOTSTEP_EVENT, wxEVT_COMMAND_MENU_SELECTED,
			wxCommandEventHandler( CEdCutsceneTimeline::OnNewEvent ), NULL, this );

		menu->Append( ID_NEW_REATTACH_ITEM_EVENT, wxT( "Add reattach item event" ) );
		menu->Connect( ID_NEW_REATTACH_ITEM_EVENT, wxEVT_COMMAND_MENU_SELECTED,
			wxCommandEventHandler( CEdCutsceneTimeline::OnNewEvent ), NULL, this );

		menu->Append( ID_NEW_MORPH_EVENT, wxT( "Add morph event" ) );
		menu->Connect( ID_NEW_MORPH_EVENT, wxEVT_COMMAND_MENU_SELECTED,
			wxCommandEventHandler( CEdCutsceneTimeline::OnNewEvent ), NULL, this );
	}
}

void CEdCutsceneTimeline::SetTrack( const CName& trackName, Float trackDuration, IEventsContainer* eventsSource, const CSkeletalAnimationSetEntry* animation, CCutsceneTemplate* editedCutscene )
{ 
	// For property editors only!
	m_currentCutscene = editedCutscene;
	m_currentEntity = trackName;

	ClearItems();

	m_trackName = trackName;
	m_eventsSource = eventsSource;
	m_activeRangeDuration = trackDuration;
	m_verticalOffset = 0;

	if( m_trackName.Empty() || m_eventsSource == NULL )
	{
		Refresh();
		return;
	}

	if ( m_currentCutscene )
	{
		TDynArray<Float> cuts = m_currentCutscene->GetCameraCuts();

		if ( cuts.Size() > 0 )
		{
			for( Uint32 i = 1; i < cuts.Size(); i++ ) 
			{
				AddItem( new CTimelineItemBlocking( cuts[i-1], cuts[i] - cuts[i-1] ) );
			}
			AddItem( new CTimelineItemBlocking( cuts.Back(), m_currentCutscene->GetDuration() - cuts.Back() ) );
		}
	}

	// Create timeline objects
	TDynArray< CExtAnimEvent* > events;
	m_eventsSource->GetEventsForAnimation( m_trackName, events );
	for( TDynArray< CExtAnimEvent* >::iterator eventIter = events.Begin();
		eventIter != events.End(); ++eventIter )
	{
		CExtAnimEvent* event = *eventIter;
		ASSERT( event != NULL );

		AddItem( CreateTimelineItem( event ) );
	}

	RecreateTracks();

	// Center timeline
	m_visibleRangeDuration = m_activeRangeDuration * 2;
	CalculateNewGrid();
	CenterPosition( m_activeRangeDuration / 2 );
}

void CEdCutsceneTimeline::OnTogglePause( wxCommandEvent& event )
{
	m_editor->OnPlayPause();
}

void CEdCutsceneTimeline::OnCsToGameplay( wxCommandEvent& event )
{
	m_editor->CsToGameplayRequest();
}

void CEdCutsceneTimeline::OnNewEvent( wxCommandEvent& event )
{
	ASSERT( m_selectedTrack < ( Int32 ) m_tracks.Size() );
	ASSERT( m_eventsSource != NULL );

	// Create new event where the user clicked
	CExtAnimEvent* newEvent = NULL;
	
	switch( event.GetId() )
	{
	case ID_NEW_SIMPLE_EVENT:
		newEvent = new CExtAnimEvent( 
			CName( m_tracks[ m_selectedTrack ]->m_name ),
			m_trackName, m_cursorTimePos, m_tracks[ m_selectedTrack ]->m_name );
		break;
	case ID_NEW_SOUND_EVENT:
		newEvent = new CExtAnimCutsceneSoundEvent( 
			CName( m_tracks[ m_selectedTrack ]->m_name ),
			m_trackName, m_cursorTimePos, m_tracks[ m_selectedTrack ]->m_name );
		break;
	case ID_NEW_FOOTSTEP_EVENT:
		newEvent = new CExtAnimFootstepEvent( 
			CName( m_tracks[ m_selectedTrack ]->m_name ),
			m_trackName, m_cursorTimePos, m_tracks[ m_selectedTrack ]->m_name );
		break;
	case ID_NEW_CUTSCENE_ACTOR_EFFECT_EVENT:
		newEvent = new CExtAnimCutsceneActorEffect( 
			CName( m_tracks[ m_selectedTrack ]->m_name ),
			m_trackName, m_cursorTimePos, 0.0f, CName::NONE,
			m_tracks[ m_selectedTrack ]->m_name );
		break;
	case ID_NEW_CUTSCENE_DIALOG_EVENT:
		newEvent = new CExtAnimCutsceneDialogEvent( 
			CName( m_tracks[ m_selectedTrack ]->m_name ),
			m_trackName, m_cursorTimePos, m_tracks[ m_selectedTrack ]->m_name );
		break;
	case ID_NEW_CUTSCENE_BREAK_EVENT:
		newEvent = new CExtAnimCutsceneBreakEvent( 
			CName( m_tracks[ m_selectedTrack ]->m_name ),
			m_trackName, m_cursorTimePos, m_tracks[ m_selectedTrack ]->m_name );
		break;
	case ID_NEW_CUTSCENE_EFFECT_EVENT:
		newEvent = new CExtAnimCutsceneEffectEvent( 
			CName( m_tracks[ m_selectedTrack ]->m_name ),
			m_trackName, m_cursorTimePos, 0.0f, m_tracks[ m_selectedTrack ]->m_name );
		break;
	case ID_NEW_CUTSCENE_SLOWMO_EVENT:
		newEvent = new CExtAnimCutsceneSlowMoEvent( 
			CName( m_tracks[ m_selectedTrack ]->m_name ),
			m_trackName, m_cursorTimePos, 0.0f, m_tracks[ m_selectedTrack ]->m_name );
		break;
	case ID_NEW_CUTSCENE_WIND_EVENT:
		newEvent = new CExtAnimCutsceneWindEvent( 
			CName( m_tracks[ m_selectedTrack ]->m_name ),
			m_trackName, m_cursorTimePos, 0.0f, m_tracks[ m_selectedTrack ]->m_name );
		break;
	case ID_NEW_CUTSCENE_ENVIRONMENT_EVENT:
		newEvent = new CExtAnimCutsceneEnvironmentEvent( 
			CName( m_tracks[ m_selectedTrack ]->m_name ),
			m_trackName, m_cursorTimePos, m_tracks[ m_selectedTrack ]->m_name,
			false, false, false, false, false, String(), true );
		break;
	case ID_NEW_CUTSCENE_LIGHT_EVENT:
		newEvent = new CExtAnimCutsceneLightEvent( 
			CName( m_tracks[ m_selectedTrack ]->m_name ),
			m_trackName, m_cursorTimePos, m_tracks[ m_selectedTrack ]->m_name );
		break;
	case ID_NEW_CUTSCENE_DOF_EVENT:
		newEvent = new CExtAnimCutsceneBokehDofEvent( 
			CName( m_tracks[ m_selectedTrack ]->m_name ),
			m_trackName, m_cursorTimePos, m_tracks[ m_selectedTrack ]->m_name );
		break;
	case ID_NEW_CUTSCENE_DOF_BLEND_EVENT:
		newEvent = new CExtAnimCutsceneBokehDofBlendEvent( 
			CName( m_tracks[ m_selectedTrack ]->m_name ),
			m_trackName, m_cursorTimePos, m_tracks[ m_selectedTrack ]->m_name );
		break;
	case ID_NEW_CUTSCENE_SET_CLIPPING_PLANES_EVENT:
		newEvent = new CExtAnimCutsceneSetClippingPlanesEvent( 
			CName( m_tracks[ m_selectedTrack ]->m_name ),
			m_trackName, m_cursorTimePos, m_tracks[ m_selectedTrack ]->m_name );
		break;
	case ID_NEW_DURATION_EVENT:
		newEvent = new CExtAnimDurationEvent( 
			CName( m_tracks[ m_selectedTrack ]->m_name ),
			m_trackName, m_cursorTimePos, GetCurrentGrid(),
			m_tracks[ m_selectedTrack ]->m_name );
		break;
	case ID_NEW_ITEM_EVENT:
		newEvent = new CExtAnimItemEvent( 
			CName( m_tracks[ m_selectedTrack ]->m_name ),
			m_trackName, m_cursorTimePos,
			m_tracks[ m_selectedTrack ]->m_name );
		break;
	case ID_NEW_REATTACH_ITEM_EVENT:
		newEvent = new CExtAnimReattachItemEvent( 
			CName( m_tracks[ m_selectedTrack ]->m_name ),
			m_trackName, m_cursorTimePos, GetCurrentGrid(),
			m_tracks[ m_selectedTrack ]->m_name );
		break;
	case ID_NEW_ITEM_EFFECT_EVENT:
		newEvent = new CExtAnimItemEffectEvent(
			CName( m_tracks[ m_selectedTrack ]->m_name ),
			m_trackName, m_cursorTimePos,
			m_tracks[ m_selectedTrack ]->m_name );
		break;
	case ID_NEW_ITEM_EFFECT_DURATION_EVENT:
		newEvent = new CExtAnimItemEffectDurationEvent(
			CName( m_tracks[ m_selectedTrack ]->m_name ),
			m_trackName, m_cursorTimePos, 0.1f,
			m_tracks[ m_selectedTrack ]->m_name );
		break;
	case ID_NEW_MORPH_EVENT:
		newEvent = new CExtAnimMorphEvent(
			CName( m_tracks[ m_selectedTrack ]->m_name ),
			m_trackName, m_cursorTimePos, 1.f,
			m_tracks[ m_selectedTrack ]->m_name );
		break;
	//case ID_NEW_COMBO_EVENT:
	//	newEvent = new CExtAnimComboEvent( 
	//		CName( m_tracks[ m_selectedTrack ] ),
	//		m_trackName, m_mousePosition, GetCurrentGrid(),
	//		m_tracks[ m_selectedTrack ] );
	//	break;
	case ID_NEW_CUTSCENE_BODY_PART_EVENT:
		newEvent = new CExtAnimCutsceneBodyPartEvent(
			CName( m_tracks[ m_selectedTrack ]->m_name ),
			m_trackName, m_cursorTimePos,
			m_tracks[ m_selectedTrack ]->m_name );
		break;
	case ID_NEW_RESET_CLOTH_EVENT:
		newEvent = new CExtAnimCutsceneResetClothAndDangleEvent(
			CName( m_tracks[ m_selectedTrack ]->m_name ),
			m_trackName, m_cursorTimePos,
			m_tracks[ m_selectedTrack ]->m_name );
		break;
	case ID_NEW_DISABLE_CLOTH_EVENT:
		newEvent = new CExtAnimCutsceneDisableClothEvent(
			CName( m_tracks[ m_selectedTrack ]->m_name ),
			m_trackName, m_cursorTimePos,
			m_tracks[ m_selectedTrack ]->m_name );
		break;
	case ID_NEW_DISABLE_DANGLE_EVENT:
		newEvent = new CExtAnimCutsceneDisableDangleEvent(
			CName( m_tracks[ m_selectedTrack ]->m_name ),
			m_trackName, m_cursorTimePos,
			m_tracks[ m_selectedTrack ]->m_name );
		break;
	case ID_NEW_CUTSCENE_QUEST_EVENT:
		{
			// Extract cutscene name
			String fileName = m_eventsSource->GetParentResource()->GetFile()->GetFileName();

			Uint32 extSize = Red::System::StringLength( TXT( ".w2cutscene" ) );

			// Cut extension
			String cutsceneName( fileName.LeftString( fileName.Size() - 1 - extSize ) );

			newEvent = new CExtAnimCutsceneQuestEvent( CName( m_tracks[ m_selectedTrack ]->m_name ), m_trackName, m_cursorTimePos, m_tracks[ m_selectedTrack ]->m_name, cutsceneName );
			break;
		}
	case ID_NEW_FADE_EVENT:
		newEvent = new CExtAnimCutsceneFadeEvent( CName( m_tracks[ m_selectedTrack ]->m_name ), m_trackName, m_cursorTimePos, m_tracks[ m_selectedTrack ]->m_name );
		break;

	case ID_NEW_SURFACE_EFFECT:
		newEvent = new CExtAnimCutsceneSurfaceEffect( CName( m_tracks[ m_selectedTrack ]->m_name ), m_trackName, m_cursorTimePos, m_tracks[ m_selectedTrack ]->m_name );
		break;

	case ID_NEW_HIDE_ENTITY_EVENT:
		newEvent = new CExtAnimCutsceneHideEntityEvent( CName( m_tracks[ m_selectedTrack ]->m_name ), m_trackName, m_cursorTimePos, m_tracks[ m_selectedTrack ]->m_name );
		break;

	case ID_NEW_TOGGLE_TERRAIN_EVENT:
		newEvent = new CExtAnimCutsceneHideTerrainEvent( CName( m_tracks[ m_selectedTrack ]->m_name ), m_trackName, m_cursorTimePos, 0.0f, m_tracks[ m_selectedTrack ]->m_name );
		break;

	default:
		ASSERT( 0 && "Unknown command" );
		return;
	};

	m_eventsSource->AddEvent( newEvent );

	ITimelineItem* item = CreateTimelineItem( newEvent );
	AddItem( item );

	if( item->IsDuration() )
	{
		SetDurationEventLength( item );
	}

	m_selectedItems.PushBackUnique( item );

	SelectionChanged();
}

ITimelineItem* CEdCutsceneTimeline::DeserializeItem( IFile& reader )
{
	ASSERT( m_eventsSource != NULL );
	
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

	animEvent->SetAnimationName( m_trackName );
	m_eventsSource->AddEvent( animEvent );

	return CreateTimelineItem( animEvent );
}

void CEdCutsceneTimeline::SerializeItem( ITimelineItem* item, IFile& writer )
{
	CExtAnimEvent* event = static_cast< CTimelineItemEvent* >( item )->GetEvent();
	ASSERT( event != NULL );

	// Serialize type
	CName eventType = event->GetClass()->GetName();
	writer << eventType;
	event->GetClass()->Serialize( writer, event );
}

ITimelineItem* CEdCutsceneTimeline::CreateTimelineItem( CExtAnimEvent* event )
{
	if( IsType< CExtAnimCutsceneEffectEvent >( event ) )
	{
		return new CTimelineItemEffect( Cast< CExtAnimCutsceneEffectEvent >(  event ) );
	}
	else if( IsType< CExtAnimCutsceneBodyPartEvent >( event ) )
	{
		return new CTimelineItemBodypart( Cast< CExtAnimCutsceneBodyPartEvent >(  event ) );
	}
	else if( IsType< CExtAnimCutsceneResetClothAndDangleEvent >( event ) )
	{
		return new CTimelineItemResetCloth( Cast< CExtAnimCutsceneResetClothAndDangleEvent >(  event ) );
	}
	else if( IsType< CExtAnimCutsceneDisableClothEvent >( event ) )
	{
		return new CTimelineItemDisableCloth( Cast< CExtAnimCutsceneDisableClothEvent >(  event ) );
	}
	else if( IsType< CExtAnimCutsceneDisableDangleEvent >( event ) )
	{
		return new CTimelineItemDisableDangle( Cast< CExtAnimCutsceneDisableDangleEvent >(  event ) );
	}
	else if( IsType< CExtAnimCutsceneDialogEvent >( event ) )
	{
		return new CTimelineItemDialog( Cast< CExtAnimCutsceneDialogEvent >(  event ) );
	}
	else if( IsType< CExtAnimCutsceneEnvironmentEvent >( event ) )
	{
		return new CTimelineItemEnvironment( Cast< CExtAnimCutsceneEnvironmentEvent >(  event ) );
	}
	else if( IsType< CExtAnimCutsceneLightEvent >( event ) )
	{
		return new CTimelineItemEnvironment( Cast< CExtAnimCutsceneLightEvent >(  event ) );
	}
	else if( IsType< CExtAnimCutsceneBokehDofEvent >( event ) || IsType< CExtAnimCutsceneBokehDofBlendEvent >( event ) )
	{
		return new CTimelineItemDof( event );
	}
	else if( IsType< CExtAnimCutsceneSetClippingPlanesEvent> ( event ) )
	{
		return new CTimelineItemClippingPlanes( event );
	}
	else if( IsType< CExtAnimFootstepEvent >( event ) )
	{
		return new CTimelineItemFootstep( Cast< CExtAnimFootstepEvent >(  event ) );
	}
	else if( IsType< CExtAnimCutsceneSoundEvent >( event ) )
	{
		return new CTimelineItemSound( Cast< CExtAnimCutsceneSoundEvent >(  event ) );
	}
	else if( IsType< CExtAnimCutsceneActorEffect >( event ) )
	{
		return new CTimelineItemEffect( Cast< CExtAnimCutsceneActorEffect >(  event ) );
	}
	else if( IsType< CExtAnimCutsceneQuestEvent >( event ) )
	{
		return new CTimelineItemQuest( Cast< CExtAnimCutsceneQuestEvent >(  event ) );
	}
	else if( IsType< CExtAnimCutsceneFadeEvent >( event ) )
	{
		return new CTimelineItemFade( Cast< CExtAnimCutsceneFadeEvent >(  event ) );
	}
	else if( IsType< CExtAnimCutsceneSurfaceEffect >( event ) )
	{
		return new CTimelineItemEffect( Cast< CExtAnimCutsceneSurfaceEffect >(  event ) );
	}
	else if( IsType< CExtAnimCutsceneHideEntityEvent >( event ) )
	{
		return new CTimelineItemMorphing( Cast< CExtAnimCutsceneHideEntityEvent >(  event ) );
	}
	else if( IsType< CExtAnimItemEvent >( event ) )
	{
		return new CTimelineItemItem( Cast< CExtAnimItemEvent >(  event ) );
	}
	else if( IsType< CExtAnimReattachItemEvent >( event ) )
	{
		return new CTimelineItemReattachItem( Cast< CExtAnimReattachItemEvent >(  event ) );
	}
	else if( IsType< CExtAnimMorphEvent >( event ) )
	{
		return new CTimelineItemMorphing( Cast< CExtAnimMorphEvent >(  event ) );
	}
	else if( IsType< CExtAnimCutsceneSlowMoEvent >( event ) )
	{
		return new CTimelineItemSlowMo( Cast< CExtAnimCutsceneSlowMoEvent >(  event ) );
	}
	else
	{
		return new CTimelineItemAnimation( event );
	}
}

void CEdCutsceneTimeline::RemoveItemImpl( ITimelineItem* item )
{
	CExtAnimEvent* event = static_cast< CTimelineItemEvent* >( item )->GetEvent();
	ASSERT( event != NULL );

	m_eventsSource->RemoveEvent( event );
}

void CEdCutsceneTimeline::StoreLayout()
{
	// empty - this functionality is not supported
}

void CEdCutsceneTimeline::RestoreLayout()
{
	// empty - this functionality is not supported
}

void CEdCutsceneTimeline::EditProperties( ITimelineItem* item, CEdPropertiesPage& propertiesPage ) const
{	
	if( item == NULL )
	{
		return;
	}

	if ( CTimelineItemEvent* evtItem = dynamic_cast< CTimelineItemEvent* >( item ) )
	{
		CExtAnimEvent* event = evtItem->GetEvent();
		ASSERT( event );

		propertiesPage.SetObject( event );
	}
}

Bool CEdCutsceneTimeline::SelectEvent( const CName& eventName )
{
	m_selectedItems.Clear();

	if( m_trackName.Empty() || m_eventsSource == NULL )
	{
		return false;
	}

	for( TDynArray< ITimelineItem* >::const_iterator itemIter = m_items.Begin();
		itemIter != m_items.End(); ++itemIter )
	{	
		CTimelineItemEvent* event = dynamic_cast<CTimelineItemEvent*>(*itemIter);
		//ASSERT( event != NULL );

		if( event != nullptr )
		{
			if ( event->GetEvent()->GetEventName() == eventName )
			{
				m_selectedItems.PushBackUnique( *itemIter );
				SelectionChanged();
				NotifyBufferIsInvalid();
				return true;
			}
		}
		
	}

	return false;
}

CCutsceneTemplate* CEdCutsceneTimeline::GetEditedCutscene()
{
	return m_currentCutscene;
}

CName CEdCutsceneTimeline::GetCurrentEntity()
{
	return m_currentEntity;
}
