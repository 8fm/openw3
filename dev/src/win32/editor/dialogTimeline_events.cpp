#include "build.h"
#include "timeline.h"
#include "dialogTimeline.h"
#include "dialogEditorPage.h"
#include "dialogPreview.h"
#include "dialogEditorActions.h"
#include "undoTimeLine.h"
#include "poppedUp.h"
#include "itemSelectionDialogs/itemSelectorDialogBase.h"

#include "../../common/game/storySceneLine.h"
#include "../../common/game/storySceneCutscene.h"
#include "../../common/game/storySceneScriptLine.h"
#include "../../common/game/storyScenePauseElement.h"
#include "../../common/game/storySceneBlockingElement.h"
#include "../../common/game/storySceneEvent.h"
#include "../../common/game/storySceneEventChangeActorGameState.h"
#include "../../common/game/storySceneEventDuration.h"
#include "../../common/game/storySceneEventAnimation.h"
#include "../../common/game/storySceneEventCurveAnimation.h"
#include "../../common/game/storySceneEventEnterActor.h"
#include "../../common/game/storySceneEventExitActor.h"
#include "../../common/game/storySceneEventLookat.h"
#include "../../common/game/storySceneEventDespawn.h"
#include "../../common/game/storySceneEventLodOverride.h"
#include "../../common/game/storySceneEventFade.h"
#include "../../common/game/storySceneEventMimics.h"
#include "../../common/game/storySceneEventMimicsAnim.h"
#include "../../common/game/storySceneEventSound.h"
#include "../../common/game/storySceneEventHitSound.h"
#include "../../common/game/storySceneEventRotate.h"
#include "../../common/game/storySceneEventCustomCamera.h"
#include "../../common/game/storySceneEventGameplayCamera.h"
#include "../../common/game/storySceneEventStartBlendToGameplayCamera.h"
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
#include "../../common/game/storySceneEventControlRig.h"
#include "../../common/game/storySceneEventGroup.h"
#include "../../common/game/storySceneEffectEvent.h"
#include "../../common/game/storySceneEventDialogLine.h"
#include "../../common/game/storySceneEventScenePropPlacement.h"
#include "../../common/game/storySceneEventWorldPropPlacement.h"
#include "../../common/game/storySceneEventLightProperties.h"
#include "../../common/game/storySceneEventEnhancedCameraBlend.h"
#include "../../common/game/storySceneEventPoseKey.h"
#include "../../common/game/storySceneEventInterpolation.h"
#include "../../common/game/storySceneEventDebugComment.h"
#include "../../common/game/storySceneEventCameraAnimation.h"
#include "../../common/game/storySceneEventCloth.h"
#include "../../common/game/storySceneEventDangle.h"
#include "../../common/game/storySceneUtils.h"
#include "../../common/game/storySceneEventHitSound.h"

#include "dialogTimeline_includes.h"
#include "dialogTimeline_items.h"
#include "dialogTimelineItemInterpolationEvent.h"
#include "dialogTimelineItemDebugCommentEvent.h"

#ifdef DEBUG_SCENES_2
#pragma optimize("",off)
#endif

using namespace DialogTimelineItems;

#include "../../common/game/storySceneVideo.h"
#include "../../games/r4/customSceneEvents.h"
#include "dialogTimeline_items.h"
#include "../../common/game/storySceneAddFactEvent.h"
#include "../../common/game/storySceneEventAttachPropToBone.h"
#include "../../common/engine/localizationManager.h"
#include "../../common/engine/skeletalAnimationContainer.h"
#include "../../common/game/storySceneEventOpenDoor.h"
#include "../../common/game/storySceneEventLightProperties.h"
#include "../../common/game/storySceneEventCsCamera.h"

#include "../../common/core/feedback.h"
#include "../../common/game/StorySceneEventCameraLight.h"

/*
Finds scene element item at specified time.

\return Scene element item at specified time or 0 if no scene element item was found.
*/
CTimelineItemBlocking* CEdDialogTimeline::FindSceneElement( Float time ) const
{
	CTimelineItemBlocking* foundItem = nullptr;

	for( auto it = m_elements.Begin(), end = m_elements.End(); it != end; ++it )
	{
		Float startTime = ( *it )->GetStart();
		Float endTime = startTime + ( *it )->GetDuration();

		if( startTime <= time && time < endTime )
		{
			foundItem = *it;
			break;
		}
	}

	return foundItem;
}

/*
Finds timeline event associated with story scene event.

\param e Story scene event for which to find associated timeline event.
\return Timeline event associated with story scene event or nullptr if there's no such timeline event.
*/
const CTimelineItemEvent* CEdDialogTimeline::FindItemEvent( const CStorySceneEvent* e ) const
{
	for ( auto it = m_items.Begin(), end = m_items.End(); it != end; ++it )
	{
		CTimelineItemEvent* item = dynamic_cast<CTimelineItemEvent*>( *it );
		if ( item && item->GetEvent() == e )
		{
			return item;
		}
	}
	return NULL;
}

CTimelineItemEvent* CEdDialogTimeline::FindItemEvent( const CStorySceneEvent* e )
{
	// call const overload of this function
	return const_cast< CTimelineItemEvent* >( static_cast< const CEdDialogTimeline* >( this )->FindItemEvent( e ) );
}

const CTimelineItemEvent* CEdDialogTimeline::FindItemEvent( const CGUID& eventGUID ) const
{
	for ( auto it = m_items.Begin(), end = m_items.End(); it != end; ++it )
	{
		CTimelineItemEvent* item = dynamic_cast<CTimelineItemEvent*>( *it );
		if ( item && item->GetEvent()->GetGUID() == eventGUID )
		{
			return item;
		}
	}
	return NULL;
}

CTimelineItemEvent* CEdDialogTimeline::FindItemEvent( const CGUID& eventGUID )
{
	// call const overload of this function
	return const_cast< CTimelineItemEvent* >( static_cast< const CEdDialogTimeline* >( this )->FindItemEvent( eventGUID ) );
}

CStorySceneEvent* CEdDialogTimeline::FindEvent( const CGUID& eventGUID )
{
	CTimelineItemEvent* item = FindItemEvent( eventGUID );
	return item ? item->GetEvent() : nullptr;
}

DialogTimelineItems::CTimelineItemBlend* CEdDialogTimeline::FindDurationBlendItemByEventClass( CClass* eventClass, Float position )
{
	SCENE_ASSERT( eventClass->IsA< CStorySceneEventBlend >() );

	for ( auto it = m_items.Begin(), end = m_items.End(); it != end; ++it )
	{
		CTimelineItemBlend* item = dynamic_cast<CTimelineItemBlend*>( *it );
		if ( !item )
		{
			continue;
		}
		if ( !item->GetEvent()->GetClass()->IsA( eventClass ) )
		{
			continue;
		}
		if ( item->GetStart() <= position && position <= item->GetEnd() )
		{
			return item;
		}
	}
	return NULL;
}

void CEdDialogTimeline::OnNewEvent( wxCommandEvent& event )
{
	// Get selected track name
	const String& trackName = m_tracks[ m_selectedTrack ]->m_name;

	// Find to which scene element should be added event
	CTimelineItemBlocking* selected = FindSceneElement( m_cursorTimePos );
	if ( !selected )
	{
		return;
	}

	// Calculate position
	const Float position = ( m_cursorTimePos - selected->GetStart() ) / selected->GetDuration();

	CreateEvent( event.GetId(), trackName, position, GetTrackActor(trackName), selected );
}

CStorySceneEvent* CEdDialogTimeline::CreateEvent( const CClass* c, Float absTime, const CName& actorId, const void* extraData )
{
	CStorySceneEvent* e = nullptr;

	CTimelineItemBlocking* selected = FindSceneElement( absTime );
	if ( !selected )
	{
		return e;
	}

	const Float position = ( absTime - selected->GetStart() ) / selected->GetDuration();
	String actorTrack = actorId.AsString() + TXT(".");

	if ( c->IsA< CStorySceneEventOverridePlacement >() )
	{
		actorTrack += TXT("placement");
		e = CreateEvent( ID_ADD_PLACEMENT_OVERRIDE_EVENT, actorTrack, position, actorId, selected, extraData );
	}
	else if ( c->IsA< CStorySceneEventScenePropPlacement >() )
	{
		actorTrack = GetActorTrackName( actorId );
		actorTrack += GROUP_SEPARATOR;
		actorTrack += actorId.AsString();
		e = CreateEvent( ID_ADD_SCENE_PROP_PLACEMENT_EVENT, actorTrack, position, actorId, selected, extraData );
	}
	else if ( c->IsA< CStorySceneEventWorldPropPlacement >() )
	{
		actorTrack = PROP_TRACK;
		actorTrack += GROUP_SEPARATOR;
		actorTrack += actorId.AsString();
		e = CreateEvent( ID_ADD_WORLD_PROP_PLACEMENT_EVENT, actorTrack, position, actorId, selected, extraData );
	}
	else if ( c->IsA< CStorySceneEventLightProperties >() )
	{
		actorTrack = LIGHT_TRACK;
		actorTrack += GROUP_SEPARATOR;
		actorTrack += actorId.AsString();
		e = CreateEvent( ID_ADD_LIGHT_PROPERTIES_EVENT, actorTrack, position, actorId, selected, extraData );
	}
	else if ( c->IsA< CStorySceneEventPoseKey >() )
	{
		actorTrack += TXT("ik");
		e = CreateEvent( ID_ADD_POSE_KEY_EVENT, actorTrack, position, actorId, selected, extraData );
	}

	return e;
}

CStorySceneEvent* CEdDialogTimeline::CreateEvent( Int32 eventId, const String& trackName, Float position, CName actor, CTimelineItemBlocking* selected, const void* extraData )
{
	CStorySceneEvent* storyEvent = NULL;

	if( eventId >= ID_ADD_GENERIC_EVENT )
	{
		for( TPair< Int32, SEtorySceneEventGenericCreationData* >& iter : m_genericObjsData )
		{
			if( eventId == iter.m_first )
			{
				SEtorySceneEventGenericCreationData::SGenericCreationArgs args( String( TXT( "NewEvent" ) ), selected->GetElement(), position, actor, trackName );
				storyEvent = iter.m_second->CreateEvent( args );
				break;
			}
		}
	}
	else
	{
		switch( eventId )
		{
		case ID_ADD_ANIMATION_EVENT:
			{
				CStorySceneEventAnimation* se = new CStorySceneEventAnimation( TXT( "NewEvent" ),
					selected->GetElement(), position, actor, String(), trackName );

				if ( extraData )
				{
					const TDynArray< CName >* data = static_cast< const TDynArray< CName >* >( extraData );
					se->SetAnimationState( *data );

					const Float duration = m_mediator->OnTimeline_GetBodyAnimationDuration( actor, se->GetAnimationName() );
					se->RefreshDuration( duration );
				}
				else
				{
					SStorySceneActorAnimationState state;
					m_mediator->OnTimeline_GetCurrentActorAnimationState( actor, state );
					se->SuckDataFromActorState( state );
				}

				storyEvent = se;
				break;
			}
		case ID_ADD_CHANGE_ACTOR_GAME_STATE_EVENT:
			storyEvent = new CStorySceneEventChangeActorGameState( TXT( "NewEvent" ), selected->GetElement(), position, actor, trackName );
			break;
		case ID_ADD_ADDITIVE_ANIMATION_EVENT:
			storyEvent = new CStorySceneEventAdditiveAnimation( TXT( "NewEvent" ), selected->GetElement(), position, actor, String(), trackName );
			break;
		case ID_ADD_OVERRIDE_ANIMATION_EVENT:
			storyEvent = new CStorySceneEventOverrideAnimation( TXT( "NewEvent" ), selected->GetElement(), position, actor, String(), trackName );
			break;
		case ID_ADD_ENTER_ACTOR_EVENT:
			storyEvent = new CStorySceneEventEnterActor( TXT( "Enters" ), selected->GetElement(), position, actor, CName( TXT( "auto" ) ), trackName );
			break;
		case ID_ADD_EXIT_ACTOR_EVENT:
			storyEvent = new CStorySceneEventExitActor( TXT( "Exits" ), selected->GetElement(), position, actor, CName( TXT( "auto" ) ), trackName );
			break;
		case ID_ADD_CAMERA_EVENT:
			if ( m_mediator->OnTimeline_CreateCustomCameraFromView() )
			{
				if ( m_undoManager )
				{
					SCENE_ASSERT ( !m_items.Empty() );
					SCENE_ASSERT ( dynamic_cast< CTimelineItemEvent* >( m_items.Back() ) );
					CUndoTimelineItemExistance::PrepareCreationStep( *m_undoManager, this, m_items.Back() );
					CUndoTimelineItemExistance::FinalizeStep( *m_undoManager );
				}
			}
			return nullptr;
		case ID_ADD_ENHANCED_BLEND_INTERPOLATED_CAMERA_EVENT:
			if ( m_mediator->OnTimeline_CreateCustomCameraEnhancedBlendInterpolated() )
			{
				if ( m_undoManager )
				{
					SCENE_ASSERT ( !m_items.Empty() );
					SCENE_ASSERT ( dynamic_cast< CTimelineItemEvent* >( m_items.Back() ) );
					CUndoTimelineItemExistance::PrepareCreationStep( *m_undoManager, this, m_items.Back() );
					CUndoTimelineItemExistance::FinalizeStep( *m_undoManager );
				}
			}
			return nullptr;
		case ID_ADD_LOOKAT_EVENT:
			{
				CName targetName = m_mediator->OnTimeline_GetPrevSpeakerName( selected->GetElement() );
				CStorySceneEventLookAt* e = new CStorySceneEventLookAt( TXT( "Lookat" ),
					selected->GetElement(), position, actor, targetName, true, trackName );
				storyEvent = e;
				break;
			}
		case ID_ADD_LOOKAT_DURATION_EVENT:
			{
				CName targetName = m_mediator->OnTimeline_GetPrevSpeakerName( selected->GetElement() );
				CStorySceneEventLookAtDuration* e = new CStorySceneEventLookAtDuration( TXT( "Lookat" ),
					selected->GetElement(), position, actor, targetName, true, trackName );
				storyEvent = e;
				break;
			}
		case ID_ADD_LOOKAT_GAMEPLAY_EVENT:
			{
				CName targetName = m_mediator->OnTimeline_GetPrevSpeakerName( selected->GetElement() );
				CStorySceneEventGameplayLookAt* e = new CStorySceneEventGameplayLookAt( TXT( "Lookat" ),
					selected->GetElement(), position, actor, targetName, true, trackName );
				storyEvent = e;
				break;
			}
		case ID_ADD_CUSTOM_CAMERA_INSTANCE:
			{
				CStorySceneEventCustomCameraInstance * camEvent = new CStorySceneEventCustomCameraInstance( TXT( "CustomCameraInstance" ), selected->GetElement(), position, trackName );
				wxListCtrl * camPage = m_mediator->HACK_GetCustomCamerasList();
				long item = camPage->GetNextItem(-1,wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
				if( item != -1 )
				{
					camEvent->SetCustomCameraName( CName( camPage->GetItemText( item ) ) );
				}
				storyEvent = camEvent;
				break;
			}
		case ID_ADD_GAMEPLAY_CAMERA_EVENT:
			storyEvent = new CStorySceneEventGameplayCamera( TXT( "GameplayCamera" ), selected->GetElement(), position, trackName );
			break;
		case ID_ADD_START_BLEND_TO_GAMEPLAY_CAMERA_EVENT:
			{
				StorySceneCameraDefinition cameraDef;
				m_mediator->OnTimeline_SaveCameraToDefinition( &cameraDef );

				storyEvent = new CStorySceneEventStartBlendToGameplayCamera( TXT( "StartBlendToGameplayCamera" ), selected->GetElement(), position, cameraDef, trackName );
				break;
			}
		case ID_ADD_VISIBILITY_EVENT:
			storyEvent = new CStorySceneEventVisibility( TXT( "Visibility" ), selected->GetElement(), position, actor, trackName );
			break;
		case ID_ADD_EVENT_LODOVERRIDE:
			{
				storyEvent = new CStorySceneEventLodOverride( TXT( "LOD override" ), selected->GetElement(), position, actor, trackName );
				break;
			}
		case ID_ADD_APPEARANCE_EVENT:
			storyEvent = new CStorySceneEventApplyAppearance( TXT( "Appearance" ), selected->GetElement(), position, actor, trackName );
			break;
		case ID_ADD_HIRES_SHADOWS_EVENT:
			storyEvent = new CStorySceneEventUseHiresShadows( TXT( "HiresShadows" ), selected->GetElement(), position, actor, trackName );
			break;
		case ID_ADD_MIMIC_LOD_EVENT:
			storyEvent = new CStorySceneEventMimicLod( TXT( "MimicLod" ), selected->GetElement(), position, actor, trackName );
			break;
		case ID_ADD_PROP_VISIBILITY_EVENT:
			storyEvent = new CStorySceneEventPropVisibility( TXT( "Visibility" ), selected->GetElement(), position, actor, trackName );
			break;		
		case ID_ADD_FADE_EVENT:
			storyEvent = new CStorySceneEventFade( TXT( "Fade" ), selected->GetElement(), position, true, trackName );
			break;
		case ID_ADD_WEATHER_EVENT:
			storyEvent = new CStorySceneEventWeatherChange( TXT( "Weather" ), selected->GetElement(), position, actor, trackName );
			break;
		case ID_ADD_SURFACE_EFFECT_EVENT:
			storyEvent = new CStorySceneEventSurfaceEffect( TXT( "Surface effect" ), selected->GetElement(), position, actor, trackName );
			break;
		case ID_ADD_ATTACH_PROP_TO_BONE:
			storyEvent = new CStorySceneEventAttachPropToSlot( TXT( "Attach prop to slot"), selected->GetElement(), position, actor, trackName );
			break;
		case ID_ADD_ADDFACT_EVENT:
			storyEvent = new CStorySceneAddFactEvent( TXT( "Add fact" ), selected->GetElement(), position, actor, trackName );
			break;
		case ID_ADD_CAMERALIGHT_EVENT:
			storyEvent = new CStorySceneEventCameraLight( TXT( "Camera Light" ), selected->GetElement(), position, actor, trackName );
			break;
		case ID_ADD_OPENDOOR_EVENT:
			storyEvent = new CStorySceneEventOpenDoor( TXT( "Open door" ), selected->GetElement(), position, trackName );
			break;		
		case ID_ADD_MIMICS_EVENT:
			{
				CStorySceneEventMimics* mimicEvent = new CStorySceneEventMimics( TXT( "Mimics" ), selected->GetElement(), position, actor, trackName );
				TDynArray<CName>	data;
				Float				poseWeight = mimicEvent->GetMimicPoseWeight();
				m_mediator->OnTimeline_GetMimicIdleData( data, poseWeight, actor );
				mimicEvent->SetMimicData( data  );
				mimicEvent->SetMimicPoseWeight( poseWeight );
				storyEvent = mimicEvent;
				break;
			}
		case ID_ADD_MIMICS_ANIMATION_EVENT:
			{
				CStorySceneEventMimicsAnim* se = new CStorySceneEventMimicsAnim( TXT( "Mimics_Anim" ), selected->GetElement(), position, actor, trackName );

				if ( extraData )
				{
					const TDynArray< CName >* data = static_cast< const TDynArray< CName >* >( extraData );
					se->SetAnimationState( *data );

					const Float duration = m_mediator->OnTimeline_GetMimicAnimationDuration( actor, se->GetAnimationName() );
					se->RefreshDuration( duration );
				}

				storyEvent = se;
				break;
			}
		case ID_ADD_CAMERA_ANIMATION_EVENT:
			storyEvent = new CStorySceneEventCameraAnim( TXT( "Camera_Anim" ), selected->GetElement(), position, actor, trackName );
			break;
		case ID_ADD_SOUND_EVENT:
			storyEvent = new CStorySceneEventSound( TXT( "Sound" ), selected->GetElement(), position, actor, trackName );
			break;
		case ID_ADD_HIT_SOUND_EVENT:
			storyEvent = new CStorySceneEventHitSound( TXT( "Hit Sound" ), selected->GetElement(), position, actor, trackName );
			break;
		case ID_ADD_DIALOG_LINE_EVENT:
			storyEvent = new CStorySceneEventDialogLine( TXT( "Dialog line" ), selected->GetElement(), position, trackName, nullptr );
			break;
		case ID_ADD_CHANGE_POSE:
			storyEvent = new CStorySceneEventChangePose( TXT( "Changing pose" ), selected->GetElement(), position, actor, trackName );
			break;
		case ID_ADD_POSE_KEY_EVENT:
			storyEvent = new CStorySceneEventPoseKey( TXT( "Add pose key" ), selected->GetElement(), position, actor, trackName );
			break;
		case ID_ADD_MIMICS_POSE_EVENT:
			storyEvent = new CStorySceneEventMimicsPose( TXT( "Changing mimic pose" ), selected->GetElement(), position, actor, trackName );
			break;
		case ID_ADD_MIMICS_FILTER_EVENT:
			storyEvent = new CStorySceneEventMimicsFilter( TXT( "Changing mimic filter" ), selected->GetElement(), position, actor, trackName );
			break;
		case ID_ADD_EQUIP_ITEM_EVENT:
			storyEvent = new CStorySceneEventEquipItem( TXT( "Equip item event" ), selected->GetElement(), position, actor, trackName );
			break;
		case ID_ADD_MORPH_EVENT:
			storyEvent = new CStorySceneMorphEvent( TXT( "Morph" ), selected->GetElement(), position, actor, trackName );
			break;
		case ID_ADD_D_PX_CLOTH_EVENT:
			storyEvent = new CStorySceneDisablePhysicsClothEvent( TXT( "Disable physics cloth" ), selected->GetElement(), position, actor, trackName );
			break;
		case ID_ADD_DANGLES_SHAKE_EVENT:
			storyEvent = new CStorySceneDanglesShakeEvent( TXT( "Dangles shake" ), selected->GetElement(), position, actor, trackName );
			break;
		case ID_ADD_D_DANGLE_EVENT:
			storyEvent = new CStorySceneDisableDangleEvent( TXT( "Disable dangle" ), selected->GetElement(), position, actor, trackName );
			break;
		case ID_ADD_RESET_DANGLE_EVENT:
			storyEvent = new CStorySceneResetClothAndDanglesEvent( TXT( "Reset cloth and dangles" ), selected->GetElement(), position, actor, trackName );
			break;
		case ID_ADD_ACTOR_EFFECT_ITEM_EVENT:
			storyEvent = new CStorySceneActorEffectEvent( TXT( "Actor effect" ), selected->GetElement(), position, actor, trackName );
			break;
		case ID_ADD_PROP_EFFECT_ITEM_EVENT:
			storyEvent = new CStoryScenePropEffectEvent( TXT( "Prop effect" ), selected->GetElement(), position, actor, trackName );
			break;
		case ID_ADD_ACTOR_EFFECT_DURATION_ITEM_EVENT:
			storyEvent = new CStorySceneActorEffectEventDuration( TXT( "Actor effect duration" ), selected->GetElement(), position, actor, trackName );
			break;
		case ID_ADD_CURVE_ANIMATION_EVENT:
			{
				EngineTransform defaultPlacement;
				m_mediator->OnTimeline_GetActorPlacementLS( actor, defaultPlacement );

				storyEvent = new CStorySceneEventCurveAnimation( TXT( "Curve animation" ),
					selected->GetElement(), position, actor, trackName, defaultPlacement );
				break;
			}
		case  ID_ADD_SCENE_PROP_PLACEMENT_EVENT:
			{
				EngineTransform placement;
				m_mediator->OnTimeline_GetActorPlacementLS( actor, placement );
				storyEvent = new CStorySceneEventScenePropPlacement( TXT( "Scene prop placement event" ), selected->GetElement(), position, trackName, actor, placement );
				break;
			}
		case  ID_ADD_WORLD_PROP_PLACEMENT_EVENT:
			storyEvent = new CStorySceneEventWorldPropPlacement( TXT( "World prop placement event" ), selected->GetElement(), position, actor, trackName );
			break;
		case  ID_ADD_LIGHT_PROPERTIES_EVENT:
			{			
				String trackPrefix = LIGHT_TRACK + GROUP_SEPARATOR_STR;
				String lightNameStr = trackName.StringAfter( trackPrefix );
				CName lightName = lightNameStr.Empty() ? CName::NONE : CName( lightNameStr );
				CStorySceneEventLightProperties* lightEvt = new CStorySceneEventLightProperties( TXT( "Light properties event" ), selected->GetElement(), position, lightName, trackName );	
				SStorySceneAttachmentInfo attachmentInfo;
				m_mediator->OnTimeline_GetCurrentLightState( lightName, attachmentInfo, lightEvt->GetTransformRef() );
				lightEvt->SetAttachmentInfo( attachmentInfo );
				storyEvent = lightEvt;
				break;
			}
		case  ID_ADD_PLACEMENT_OVERRIDE_EVENT:
			{
				EngineTransform defaultPlacement;

				if ( extraData )
				{
					const EngineTransform* data = static_cast< const EngineTransform* >( extraData );
					defaultPlacement = *data;
				}
				else
				{
					m_mediator->OnTimeline_GetActorPlacementLS( actor, defaultPlacement );
				}

				storyEvent = new CStorySceneEventOverridePlacement( TXT( "Placement event" ), selected->GetElement(), position, trackName, actor, defaultPlacement );
				break;
			}
		case ID_ADD_PLACEMENT_OVERRIDE_EVENT_DURATION:
			{
				EngineTransform defaultPlacement;
				m_mediator->OnTimeline_GetActorPlacementLS( actor, defaultPlacement );

				storyEvent = new CStorySceneEventOverridePlacementDuration( TXT( "Placement duration event" ), selected->GetElement(), position, trackName, actor, defaultPlacement );
				break;
			}
		case ID_ADD_PLACEMENT_OVERRIDE_BLEND_INTERPOLATED_EVENT:
			if ( m_mediator->OnTimeline_CreatePlacementOverrideBlendInterpolated() )
			{
				if ( m_undoManager )
				{
					SCENE_ASSERT ( !m_items.Empty() );
					SCENE_ASSERT ( dynamic_cast< CTimelineItemEvent* >( m_items.Back() ) );
					CUndoTimelineItemExistance::PrepareCreationStep( *m_undoManager, this, m_items.Back() );
					CUndoTimelineItemExistance::FinalizeStep( *m_undoManager );
				}
			}
			return nullptr;
		case ID_ADD_PLACEMENT_WALK:
			{
				EngineTransform defaultPlacement;
				m_mediator->OnTimeline_GetActorPlacementLS( actor, defaultPlacement );

				storyEvent = new CStorySceneEventWalk( TXT( "Walk" ), selected->GetElement(), position, trackName, actor, defaultPlacement );
				break;
			}
		}
	}

	SCENE_ASSERT( storyEvent );

	m_mediator->OnTimeline_AddEvent( m_section, storyEvent, GetSectionVariantId() );

	// Rebuild player to recreate playing plan as some code accesses event instance
	// data before rebuild request (issued by OnTimeline_AddEvent()) is processed.
	m_mediator->OnTimeline_RebuildPlayer();

	CTimelineItemEvent* timelineItemEvent = CreateTimelineItemEvent( storyEvent, selected );
	SCENE_ASSERT( timelineItemEvent );

	if ( m_undoManager )
	{
		CUndoTimelineItemExistance::PrepareCreationStep( *m_undoManager, this, timelineItemEvent );
		CUndoTimelineItemExistance::FinalizeStep( *m_undoManager );
	}

	AddItem( timelineItemEvent );

	return storyEvent;
}

CTimelineItemEvent* CEdDialogTimeline::CreateTimelineItemEvent( CStorySceneEvent* event, CTimelineItemBlocking* elementItem )
{
	SCENE_ASSERT( event != NULL );
	SCENE_ASSERT( elementItem != NULL );

	if ( event->GenericEventCreation() )
	{
		return new CTimelineItemDefault( this, const_cast< CStorySceneEvent* >( event ), elementItem, m_elements, event->GetGenericCreationData()->m_timelineIconName, event->GetClass() );
	}
	if ( IsType< CStorySceneEventEnterActor >( event ) || IsType< CStorySceneEventExitActor >( event ) )
	{
		return new CTimelineItemEnterExit( this, const_cast< CStorySceneEvent* >( event ), elementItem, m_elements );
	}
	else if ( IsType< CStorySceneEventChangeActorGameState >( event ) )
	{
		return new CTimelineItemChangeActorGameState( this, const_cast< CStorySceneEvent* >( event ), elementItem, m_elements );
	}
	else if ( IsType< CStorySceneEventLookAt >( event ) )
	{
		return new CTimelineItemLookat( this, const_cast< CStorySceneEvent* >( event ), elementItem, m_elements );
	}
	else if ( IsType< CStorySceneEventLookAtDuration >( event ) )
	{
		return new CTimelineItemLookatDuration( this, const_cast< CStorySceneEvent* >( event ), elementItem, m_elements );
	}
	else if ( IsType< CStorySceneEventGameplayLookAt >( event ) )
	{
		return new CTimelineItemLookatGameplay( this, const_cast< CStorySceneEvent* >( event ), elementItem, m_elements );
	}
	else if ( IsType< CStorySceneEventFade >( event ) )
	{
		return new CTimelineItemFade( this, const_cast< CStorySceneEvent* >( event ), elementItem, m_elements );
	}
	else if ( IsType< CStorySceneEventVisibility >( event ) || IsType< CStorySceneEventPropVisibility >( event ) )
	{
		return new CTimelineItemVisibility( this, const_cast< CStorySceneEvent* >( event ), elementItem, m_elements );
	}
	else if( IsType< CStorySceneEventLodOverride >( event ) )
	{
		return new CTimelineItemLodOverride( this, static_cast< CStorySceneEventLodOverride* >( event ), elementItem, m_elements );
	}
	else if ( IsType< CStorySceneEventApplyAppearance >( event ) )
	{
		return new CTimelineItemAppearance( this, const_cast< CStorySceneEvent* >( event ), elementItem, m_elements );
	}
	else if ( IsType< CStorySceneEventUseHiresShadows >( event ) )
	{
		return new CTimelineItemUseHiresShadows( this, const_cast< CStorySceneEvent* >( event ), elementItem, m_elements );
	}
	else if ( IsType< CStorySceneEventMimicLod >( event ) )
	{
		return new CTimelineItemMimicLod( this, const_cast< CStorySceneEvent* >( event ), elementItem, m_elements );
	}
	else if ( IsType< CStorySceneEventGameplayCamera >( event ) )
	{
		return new CTimelineItemGameplayCamera( this, const_cast< CStorySceneEvent* >( event ), elementItem, m_elements );
	}
	else if ( IsType< CStorySceneEventStartBlendToGameplayCamera >( event ) )
	{
		return new CTimelineItemStartBlendToGameplayCamera( this, const_cast< CStorySceneEvent* >( event ), elementItem, m_elements );
	}
	else if ( IsType< CStorySceneEventCustomCamera >( event ) )
	{
		return new CTimelineItemCustomCamera( this, const_cast< CStorySceneEvent* >( event ), elementItem, m_elements );
	}
	else if ( IsType< CStorySceneEventCustomCameraInstance >( event ) )
	{
		return new CTimelineItemCustomCameraInstance( this, const_cast< CStorySceneEvent* >( event ), elementItem, m_elements );
	}
	else if ( IsType< CStorySceneEventAnimation >( event ) )
	{
		return new CTimelineItemAnimation( this, const_cast< CStorySceneEvent* >( event ), elementItem, m_elements );
	}
	else if ( IsType< CStorySceneEventCurveAnimation >( event ) )
	{
		return new CTimelineItemCurveAnimation( this, const_cast< CStorySceneEvent* >( event ), elementItem, m_elements );
	}
	else if ( IsType< CStorySceneEventWalk >( event ) )
	{
		return new CTimelineItemCurveAnimation( this, const_cast< CStorySceneEvent* >( event ), elementItem, m_elements );
	}
	else if ( IsType< CStorySceneEventRotate >( event ) )
	{
		return new CTimelineItemRotate( this, const_cast< CStorySceneEvent* >( event ), elementItem, m_elements );
	}
	else if ( IsType< CStorySceneEventMimics >( event ) )
	{
		return new CTimelineItemMimicsDuration( this, const_cast< CStorySceneEvent* >( event ), elementItem, m_elements );
	}
	else if ( IsType< CStorySceneEventMimicsPose >( event ) )
	{
		return new CTimelineItemMimicsPose( this, const_cast< CStorySceneEvent* >( event ), elementItem, m_elements );
	}
	else if ( IsType< CStorySceneEventMimicsFilter >( event ) )
	{
		return new CTimelineItemMimicsFilter( this, const_cast< CStorySceneEvent* >( event ), elementItem, m_elements );
	}
	else if ( IsType< CStorySceneEventPoseKey >( event ) )
	{
		return new CTimelineItemPoseKey( this, const_cast< CStorySceneEvent* >( event ), elementItem, m_elements );
	}
	else if ( IsType< CStorySceneEventMimicsAnim >( event ) )
	{
		return new CTimelineItemMimicAnimClip( this, event, elementItem, m_elements );
	}
	else if ( IsType< CStorySceneEventCameraAnim >( event ) )
	{
		return new CTimelineItemCameraAnimClip( this, event, elementItem, m_elements );
	}
	else if ( IsType< CStorySceneEventSound >( event ) )
	{
		return new CTimelineItemSound( this, const_cast< CStorySceneEvent* >( event ), elementItem, m_elements );
	}
	else if ( IsType< CStorySceneEventHitSound >( event ) )
	{
		return new CTimelineItemSound( this, const_cast< CStorySceneEvent* >( event ), elementItem, m_elements );
	}
	else if ( IsType< CStorySceneEventCameraBlend >( event ) )
	{
		return new CTimelineItemCameraBlend( this, const_cast< CStorySceneEvent* >( event ), elementItem, m_elements );
	}
	else if ( IsType< CStorySceneEventEnhancedCameraBlend >( event ) )
	{
		return new CTimelineItemEnhancedCameraBlend( this, const_cast< CStorySceneEvent* >( event ), elementItem, m_elements );
	}
	else if ( IsType< CStorySceneCameraBlendEvent >( event ) )
	{
		return new CTimelineItemCameraBlendEvent( this, const_cast< CStorySceneEvent* >( event ), elementItem, m_elements );
	}
	else if ( IsType< CStorySceneOverridePlacementBlend >( event ) )
	{
		return new CTimelineItemOverridePlacementBlend( this, const_cast< CStorySceneEvent* >( event ), elementItem, m_elements );
	}
	else if ( IsType< CStorySceneEventChangePose >( event ) )
	{
		return new CTimelineItemPoseChange( this, const_cast< CStorySceneEvent* >( event ), elementItem, m_elements );
	}
	else if ( IsType< CStorySceneEventEquipItem >( event ) )
	{
		return new CTimelineItemEquipItem( this, const_cast< CStorySceneEvent* >( event ), elementItem, m_elements );
	}
	else if ( IsType< CStorySceneMorphEvent >( event ) )
	{
		return new CTimelineItemMorphItem( this, const_cast< CStorySceneEvent* >( event ), elementItem, m_elements );
	}
	else if ( IsType< CStorySceneDisablePhysicsClothEvent >( event ) )
	{
		return new CTimelineItemDisablePhysicsCloth( this, const_cast< CStorySceneEvent* >( event ), elementItem, m_elements );
	}
	else if ( IsType< CStorySceneDanglesShakeEvent >( event ) )
	{
		return new CTimelineItemDanglesShake( this, const_cast< CStorySceneEvent* >( event ), elementItem, m_elements );
	}
	else if ( IsType< CStorySceneDisableDangleEvent >( event ) )
	{
		return new CTimelineItemDisableDangle( this, const_cast< CStorySceneEvent* >( event ), elementItem, m_elements );
	}
	else if ( IsType< CStorySceneResetClothAndDanglesEvent >( event ) )
	{
		return new CTimelineItemResetClothAndDangles( this, const_cast< CStorySceneEvent* >( event ), elementItem, m_elements );
	}
	else if ( IsType< CStorySceneActorEffectEvent >( event ) ||  IsType< CStoryScenePropEffectEvent >( event ) )
	{
		return new CTimelineItemEffect( this, const_cast< CStorySceneEvent* >( event ), elementItem, m_elements );
	}
	else if ( IsType< CStorySceneActorEffectEventDuration >( event ) )
	{
		return new CTimelineItemEffect( this, const_cast< CStorySceneEvent* >( event ), elementItem, m_elements );
	}
	else if ( IsType< CStorySceneEventScenePropPlacement >( event ) )
	{
		return new CTimelineItemScenePropPlacement( this, const_cast< CStorySceneEvent* >( event ), elementItem, m_elements );
	}
	else if ( IsType< CStorySceneEventWorldPropPlacement >( event ) )
	{
		return new CTimelineItemScenePropPlacement( this, const_cast< CStorySceneEvent* >( event ), elementItem, m_elements );
	}
	else if ( IsType< CStorySceneEventLightProperties >( event ) )
	{
		return new CTimelineItemLightProperty( this, const_cast< CStorySceneEvent* >( event ), elementItem, m_elements );		
	}
	else if ( IsType< CStorySceneEventOverridePlacement >( event ) )
	{
		return new CTimelineItemOverridePlacement( this, const_cast< CStorySceneEvent* >( event ), elementItem, m_elements );
	}
	else if ( IsType< CStorySceneEventOverridePlacementDuration >( event ) )
	{
		return new CTimelineItemOverridePlacementDuration( this, const_cast< CStorySceneEvent* >( event ), elementItem, m_elements );
	}
	else if ( IsType< CStorySceneEventSurfaceEffect >( event )  )
	{
		return new CTimelineItemSurfaceEffect( this, const_cast< CStorySceneEvent* >( event ), elementItem, m_elements );
	}
	else if ( IsType< CStorySceneEventAttachPropToSlot >( event )  )
	{
		return new CTimelineItemAttachToBone( this, const_cast< CStorySceneEvent* >( event ), elementItem, m_elements );
	}
	else if ( IsType< CStorySceneEventCsCamera >( event )  )
	{
		return new CTimelineItemCsCamera( this, const_cast< CStorySceneEvent* >( event ), elementItem, m_elements );
	}
	else if ( IsType< CStorySceneEventWeatherChange >( event ) )
	{
		return new CTimelineItemWeatherChange( this, const_cast< CStorySceneEvent* >( event ), elementItem, m_elements );
	}
	else if ( IsType< CStorySceneEventDialogLine >( event ) )
	{
		return new CTimelineItemLineEvent( this, const_cast< CStorySceneEvent* >( event ), elementItem, m_elements );
	}
	else if ( IsType< CStorySceneEventGroup >( event ) )
	{
		CStorySceneEventGroup* groupEvent = Cast< CStorySceneEventGroup >( event );
		CTimelineItemEventGroup* groupItem = new CTimelineItemEventGroup( this, event, elementItem, m_elements );
		const TDynArray< SStorySceneEventGroupEntry >& sceneGroupEntries = groupEvent->GetEvents();
		for ( TDynArray< SStorySceneEventGroupEntry >::const_iterator groupEntryIter = sceneGroupEntries.Begin();
			groupEntryIter != sceneGroupEntries.End(); ++groupEntryIter )
		{
			groupItem->AddItem( CreateTimelineItemEvent( groupEntryIter->m_event, elementItem ), groupEntryIter->m_time );
		}
		return groupItem;
	}
	else if ( IsType< CStorySceneAddFactEvent >( event )  )
	{
		return new CTimelineItemDefault( this, const_cast< CStorySceneEvent* >( event ), elementItem, m_elements, TXT("IMG_DIALOG_BOOK"), CStorySceneAddFactEvent::GetStaticClass() );
	}
	else if ( IsType< CStorySceneEventCameraLight >( event )  )
	{
		return new CTimelineItemCameraLight( this, const_cast< CStorySceneEvent* >( event ), elementItem, m_elements );
	}
	else if ( IsType< CStorySceneEventOpenDoor >( event )  )
	{
		return new CTimelineItemDefault( this, const_cast< CStorySceneEvent* >( event ), elementItem, m_elements, TXT("IMG_DIALOG_EQUIP_ITEM"), CStorySceneEventOpenDoor::GetStaticClass() );
	}
	else if ( IsType< CStorySceneEventInterpolation >( event ) )
	{
		return new CTimelineItemInterpolationEvent( this, static_cast< CStorySceneEventInterpolation* >( event ), elementItem, m_elements );

		// Note that caller should call CTimelineItemInterpolationEvent::Reinitialize() at some point.
		// We can't do this here as Reinitialize() requires that timeline items for all keys exist and
		// here this may not be the case yet.
	}
	else if( IsType< CStorySceneEventDebugComment >( event ) )
	{
		return new CTimelineItemDebugCommentEvent( this, static_cast< CStorySceneEventDebugComment* >( event ), elementItem, m_elements );
	}
	else
	{
		SCENE_ASSERT( 0 && "Invalid event type" );
		return NULL;
	}
}

CTimelineItemBlocking* CEdDialogTimeline::CreateBlockingTimelineItem( CStorySceneElement* element, Float startTime, Uint32 level )
{
	if ( !element->IsPlayable() )
	{
		return nullptr;
	}

	CTimelineItemBlocking* blockingItem = NULL;

	const IStorySceneElementInstanceData* elementData = m_mediator->OnTimeline_FindElementInstance( element );
	const Float duration = elementData ? elementData->GetDuration() : element->CalculateDuration( SLocalizationManager::GetInstance().GetCurrentLocale() );

	if( element->IsA< CStorySceneLine >() )
	{
		CStorySceneLine* line = Cast< CStorySceneLine >( element );
		blockingItem = new CTimelineItemLineElement( this, line, startTime, duration, level );
	}
	else if( element->IsA< CStoryScenePauseElement >() )
	{
		CStoryScenePauseElement* pause = Cast< CStoryScenePauseElement >( element );

		// Create timeline object
		blockingItem = new CTimelineItemPause( pause, startTime, duration, this, level );
	}
	else if( element->IsA< CStorySceneBlockingElement >() )
	{
		CStorySceneBlockingElement* blocking = Cast< CStorySceneBlockingElement >( element );

		// Create timeline object
		blockingItem = new CTimelineItemBlockingEvent( blocking, startTime, duration, level );
	}
	else if( element->IsA< CStorySceneCutscenePlayer >() )
	{
		CStorySceneCutscenePlayer* player = Cast< CStorySceneCutscenePlayer >( element );

		// Create timeline object
		blockingItem = new CTimelineItemCutscene( player, startTime, duration, level );
	}
	else if( element->IsA< CStorySceneScriptLine >() )
	{
		CStorySceneScriptLine* scriptLine = Cast< CStorySceneScriptLine >( element );

		// Create timeline object
		blockingItem = new CTimelineItemScriptLine( scriptLine, startTime, duration, level );
	}
	else if ( element->IsA< CStorySceneChoice >() )
	{
		CStorySceneChoice* choice = Cast< CStorySceneChoice >( element );

		blockingItem = new CTimelineItemChoice( choice, startTime, duration, this, level );
	}
	else if( element->IsA< CStorySceneComment >() )
	{
		// Ignore comments
	}
	else if ( element->IsA< CStorySceneVideoElement >() )
	{
		// Ignore video element for now
	}
	else
	{
		SCENE_ASSERT( ! "Unknown story scene element!" );
	}

	//if ( blockingItem != NULL )
	//{
	//	m_items.PushBack( blockingItem );
	//	m_elements.PushBack( blockingItem );
	//}

	return blockingItem;
}

void CEdDialogTimeline::CreateCustomCameraInstance( CName cameraName )
{
	// Find scene element with which new event should be associated.
	CTimelineItemBlocking* selected = FindSceneElement( m_timeSelectorTimePos );
	if( selected == NULL )
	{
		return;
	}

	// Calculate position
	Float position = ( m_timeSelectorTimePos - selected->GetStart() ) / selected->GetDuration();

	CStorySceneEventCustomCameraInstance* storyEvent = new CStorySceneEventCustomCameraInstance( TXT( "CustomCamera" ),
		selected->GetElement(), position, TIMELINE_DEFAULT_CAMERA_TRACK );
	storyEvent->SetCustomCameraName( cameraName );

	m_mediator->OnTimeline_AddEvent( m_section, storyEvent, GetSectionVariantId() );

	// Rebuild player to recreate playing plan as some code accesses event instance
	// data before rebuild request (issued by OnTimeline_AddEvent()) is processed.
	m_mediator->OnTimeline_RebuildPlayer();

	AddItem( CreateTimelineItemEvent( storyEvent, selected ) );
}

CTimelineItemEvent* CEdDialogTimeline::CreateCustomCameraEvent( const StorySceneCameraDefinition & definition, DialogTimelineItems::CTimelineItemBlend* blend, Float absTime )
{
	return 
		CreateCustomCameraEvent( definition.m_cameraTransform.GetPosition(),				definition.m_cameraTransform.GetRotation(),
		definition.m_cameraZoom,	definition.m_cameraFov,			definition.m_dofFocusDistFar,	definition.m_dofBlurDistFar,
		definition.m_dofIntensity, definition.m_dofFocusDistNear,	definition.m_dofBlurDistNear, definition.m_dof, blend, absTime );
}

CTimelineItemEvent* CEdDialogTimeline::CreateCustomCameraEvent( const Vector& cameraPosition, 
	const EulerAngles& cameraRotation, Float cameraZoom, Float cameraFov,
	Float dofFocusDistFar, Float dofBlurDistFar,
	Float dofIntensity, Float dofFocusDistNear, Float dofBlurDistNear,
	const ApertureDofParams& apertureDofParams,
	DialogTimelineItems::CTimelineItemBlend* blend, Float absTime )
{
	if ( absTime < 0.f )
	{
		absTime = m_timeSelectorTimePos;
	}

	// Find scene element with which new event should be associated.
	CTimelineItemBlocking* selected = FindSceneElement( absTime );
	if( selected == NULL )
	{
		return NULL;
	}

	// Calculate position
	Float position = ( absTime - selected->GetStart() ) / selected->GetDuration();

	CStorySceneEvent* storyEvent = new CStorySceneEventCustomCamera( TXT( "CustomCamera" ),
		selected->GetElement(), position, cameraPosition, cameraRotation, cameraZoom,
		cameraFov, dofFocusDistFar, dofBlurDistFar, dofIntensity, dofFocusDistNear, dofBlurDistNear,
		apertureDofParams, TIMELINE_DEFAULT_CAMERA_TRACK );

	m_mediator->OnTimeline_AddEvent( m_section, storyEvent, GetSectionVariantId() );

	// Rebuild player to recreate playing plan as some code accesses event instance
	// data before rebuild request (issued by OnTimeline_AddEvent()) is processed.
	m_mediator->OnTimeline_RebuildPlayer();

	CTimelineItemEvent* timelineItemEvent = CreateTimelineItemEvent( storyEvent, selected );
	SCENE_ASSERT( timelineItemEvent );

	AddItem( timelineItemEvent );

	if ( blend )
	{
		blend->AddChild( timelineItemEvent );
	}

	return timelineItemEvent;
}

CTimelineItemEvent* CEdDialogTimeline::CreateOverridePlacementEvent( const String& trackName, const CName& actorName, const EngineTransform* transform, DialogTimelineItems::CTimelineItemBlend* blend )
{
	// Find scene element with which new event should be associated.
	CTimelineItemBlocking* selected = FindSceneElement( m_timeSelectorTimePos );
	if( selected == NULL )
	{
		return NULL;
	}

	// Calculate position
	Float position = ( m_timeSelectorTimePos - selected->GetStart() ) / selected->GetDuration();

	EngineTransform defaultPlacement;
	m_mediator->OnTimeline_GetActorPlacementLS( actorName, defaultPlacement );

	CStorySceneEvent* storyEvent = new CStorySceneEventOverridePlacement( TXT( "Placement event" ),
		selected->GetElement(), position, trackName, actorName, transform ? *transform : defaultPlacement );

	if ( CTimelineItemEvent* timelineItemEvent = CreateTimelineItemEvent( storyEvent, selected ) )
	{
		m_items.PushBack( timelineItemEvent );
		m_mediator->OnTimeline_AddEvent( m_section, storyEvent, GetSectionVariantId() );

		if ( blend )	
		{
			blend->AddChild( timelineItemEvent );
		}
		return timelineItemEvent;
	}
	else
	{
		return NULL;
	}
}

Bool CEdDialogTimeline::IsCameraEventItem( const ITimelineItem* item )
{
	return IsType< CStorySceneEventCamera >( static_cast< const CTimelineItemEvent* >( item )->GetEvent() );
}

CTimelineItemEvent* CEdDialogTimeline::DetachFromList( CTimelineItemEvent* camInst )
{
	CStorySceneEventCustomCameraInstance* camera = Cast<CStorySceneEventCustomCameraInstance>( camInst->GetEvent() ); 

	if ( camera && camera->GetCameraDefinition() )
	{
		if( StorySceneCameraDefinition* cam = camera->GetCameraDefinition() )
		{
			Float pos = camInst->GetStart();
			RemoveItem( camInst );
			delete camInst;
			return CreateCustomCameraEvent( *cam, nullptr, pos );
		}		
	}
	return nullptr;
}


void CEdDialogTimeline::OnAddToCameraEnhancedBlend( wxCommandEvent& event )
{
	if ( !IsOneSelected() || !IsCameraEventItem( m_selectedItems[ 0 ] ) )
	{
		return;
	}

	CTimelineItemEvent* item = static_cast<CTimelineItemEvent*>( m_selectedItems[ 0 ] ); 
	CTimelineItemCustomCameraInstance* inst = dynamic_cast<CTimelineItemCustomCameraInstance*>( item );
	if( inst && !inst->GetEvent()->HasBlendParent() )
	{
		item = DetachFromList( inst );
	}

	CTimelineItemBlend* blendItem = FindDurationBlendItemByEventClass( ClassID< CStorySceneEventEnhancedCameraBlend >(), item->GetStart() );
	if ( blendItem )
	{
		blendItem->AddChild( item );
	}
}

void CEdDialogTimeline::OnRemoveFromCameraEnhancedBlend( wxCommandEvent& event )
{
	if ( !IsOneSelected() || !IsCameraEventItem( m_selectedItems[ 0 ] ) )
	{
		return;
	}
	CTimelineItemEvent* item = static_cast<CTimelineItemEvent*>( m_selectedItems[ 0 ] );
	CTimelineItemBlend* blendItem = item->GetBlendParent();
	if ( blendItem )
	{
		blendItem->RemoveChild( item );
		if( blendItem->GetItemsCount() == 0 )
		{
			RemoveItem( blendItem );
			m_selectedItems.RemoveAt( 0 );
			SelectionChanged();
		}
	}
}

void CEdDialogTimeline::OnRemoveFromCameraBlendNew( wxCommandEvent& event )
{
	if ( !IsOneSelected() || !IsCameraEventItem( m_selectedItems[ 0 ] ) )
	{
		return;
	}
	CTimelineItemEvent* item = static_cast<CTimelineItemEvent*>( m_selectedItems[ 0 ] );
	CTimelineItemBlend* blendItem = item->GetBlendParent();
	if ( blendItem )
	{
		blendItem->RemoveChild( item );		
		if( blendItem->GetItemsCount() == 0 )
		{
			RemoveItem( blendItem );
			m_selectedItems.RemoveAt( 0 );
			SelectionChanged();
		}
	}
}

void CEdDialogTimeline::OnAddToCustomCameraList( wxCommandEvent& event )
{
	if( ! IsOneSelected() ) 
	{
		return;
	}

	CTimelineItemEvent* item = dynamic_cast< CTimelineItemEvent* >( m_selectedItems[ 0 ] );
	if( item == NULL )
	{
		return;
	}

	CStorySceneEventCustomCamera* customCamera = Cast< CStorySceneEventCustomCamera >( item->GetEvent() );
	if( customCamera == NULL )
	{
		return;
	}

	// Check if camera with this name is not already added
	StorySceneCameraDefinition* cam = customCamera->GetCameraDefinition();
	if( m_mediator->HACK_GetStoryScene()->GetCameraDefinition( cam->m_cameraName ) != NULL )
	{
		wxMessageBox( TXT( "There is already camera with this name on the list" ) );
		return;
	}

	// Get basic parameters
	Float startPosition = customCamera->GetStartPosition();
	CStorySceneElement* element = customCamera->GetSceneElement();
	String name( cam->m_cameraName.AsString() );
	String trackName( customCamera->GetTrackName() );

	// Add to the custom camera list
	StorySceneCameraDefinition cameraDefinition = *(cam);
	//cameraDefinition.m_cameraName = CName( customCamera->GetEventName() );
	if( m_section )
	{	
		cameraDefinition.ParseCameraParams();
	}
	m_mediator->HACK_GetStoryScene()->AddCameraDefinition( cameraDefinition );

	// Add instance
	CStorySceneEventCustomCameraInstance* storyEvent = new CStorySceneEventCustomCameraInstance( customCamera->GetEventName() + TXT( "_instance" ), element, startPosition, trackName );
	storyEvent->SetCustomCameraName( CName( name ) );

	m_mediator->OnTimeline_AddEvent( m_section, storyEvent, GetSectionVariantId() );

	// Rebuild player to recreate playing plan as some code accesses event instance
	// data before rebuild request (issued by OnTimeline_AddEvent()) is processed.
	m_mediator->OnTimeline_RebuildPlayer();

	CTimelineItemEvent* newItem = CreateTimelineItemEvent( storyEvent, item->GetElementItem() );
	AddItem( newItem );

	if( customCamera->IsInterpolationEventKey() )
	{
		CTimelineItemInterpolationEvent* tiIterpolationEvent = static_cast< CTimelineItemInterpolationEvent* >( FindItemEvent( customCamera->GetInterpolationEventGUID() ) );
		InterpolationEventAttachKey( tiIterpolationEvent, newItem );
		InterpolationEventDetachKey( tiIterpolationEvent, item );
	}

	// Remove from timeline
	RemoveItem( item );
	delete item;

	// Update selection
	m_selectedItems.Clear();
	m_selectedItems.PushBack( newItem );
	SelectionChanged();
}

void CEdDialogTimeline::OnDetachFromCustomCameraList( wxCommandEvent& event )
{
	if( ! IsOneSelected() )
	{
		return;
	}

	CTimelineItemEvent* item = dynamic_cast< CTimelineItemEvent* >( m_selectedItems[ 0 ] );
	if( item == NULL )
	{
		return;
	}

	CStorySceneEventCustomCameraInstance* customCamera = Cast< CStorySceneEventCustomCameraInstance >( item->GetEvent() );
	if( customCamera == NULL )
	{
		return;
	}

	// Get camera event
	const StorySceneCameraDefinition* cameraDefinition = m_mediator->HACK_GetStoryScene()->GetCameraDefinition( customCamera->GetCustomCameraName() );
	if( cameraDefinition == NULL )
	{
		wxMessageBox( TXT( "Invalid custom camera name" ) );
		return;
	}

	// Get basic parameters
	Float startPosition = customCamera->GetStartPosition();
	CStorySceneElement* element = customCamera->GetSceneElement();
	String trackName( customCamera->GetTrackName() );

	// Add custom camera event
	CStorySceneEvent* storyEvent = new CStorySceneEventCustomCamera(
		cameraDefinition->m_cameraName.AsString(), 
		element, 
		startPosition, 
		cameraDefinition->m_cameraTransform.GetPosition(), 
		cameraDefinition->m_cameraTransform.GetRotation(),
		cameraDefinition->m_cameraZoom,
		cameraDefinition->m_cameraFov,
		cameraDefinition->m_dofFocusDistFar,
		cameraDefinition->m_dofBlurDistFar,
		cameraDefinition->m_dofIntensity,
		cameraDefinition->m_dofFocusDistNear,
		cameraDefinition->m_dofBlurDistNear,
		cameraDefinition->m_dof,
		trackName );

	storyEvent->SetStartPosition( startPosition );
	storyEvent->SetTrackName( trackName );
	storyEvent->SetSceneElement( element );

	m_mediator->OnTimeline_AddEvent( m_section, storyEvent, GetSectionVariantId() );
	CTimelineItemEvent* newItem = CreateTimelineItemEvent( storyEvent, item->GetElementItem() );
	AddItem( newItem );

	if( customCamera->IsInterpolationEventKey() )
	{
		CTimelineItemInterpolationEvent* tiIterpolationEvent = static_cast< CTimelineItemInterpolationEvent* >( FindItemEvent( customCamera->GetInterpolationEventGUID() ) );
		InterpolationEventAttachKey( tiIterpolationEvent, newItem );
		InterpolationEventDetachKey( tiIterpolationEvent, item );
	}

	// Remove from timeline
	RemoveItem( item );
	delete item;

	CancelSelection();
}

void CEdDialogTimeline::OnCreateEnhancedCameraBlend( wxCommandEvent& event )
{
	if ( m_selectedItems.Empty() )
	{
		return;
	}

	TDynArray< ITimelineItem* > items = m_selectedItems;
	for( Uint32 i = 0; i <  items.Size(); i++ )
	{
		CTimelineItemCustomCameraInstance* inst = dynamic_cast<CTimelineItemCustomCameraInstance*>( items[i] );
		if( inst && !inst->GetEvent()->HasBlendParent() )
		{
			items[i] = DetachFromList( inst );
		}
	}

	// Create blend event

	::Sort( items.Begin(), items.End(), TimelineItemStartSorter() );

	CTimelineItemEvent* firstEvent = dynamic_cast< CTimelineItemEvent* >( items[0] );
	CStorySceneEventEnhancedCameraBlend* blendEvent = new CStorySceneEventEnhancedCameraBlend( TXT("camera ENHANCED blend"), firstEvent->GetEvent()->GetSceneElement(), firstEvent->GetEvent()->GetTrackName() );
	if ( !blendEvent )
	{
		return;
	}
	CTimelineItemEnhancedCameraBlend* blendItem = (CTimelineItemEnhancedCameraBlend*) CreateTimelineItemEvent( blendEvent, firstEvent->GetElementItem() );
	if ( !blendItem )
	{
		delete blendEvent;
		return;
	}
	if ( !blendItem->Initialize( items ) )
	{
		delete blendItem;
		return;
	}

	// Add timeline item for the event

	AddItem( blendItem );

	// Update selection

	m_selectedItems.Clear();
	m_selectedItems.PushBack( blendItem );
	SelectionChanged();

	// Refresh player and event state

	m_mediator->OnTimeline_AddEvent( m_section, blendItem->GetEvent(), GetSectionVariantId() );

	m_mediator->OnTimeline_RebuildPlayer();
}

/*
Removes old style camera blend (CStorySceneEventCameraBlend).

For each blend key, new camera event is created. Then, blend event is removed.
*/
void CEdDialogTimeline::OnRemoveCameraBlend( wxCommandEvent& event )
{
	if ( m_selectedItems.Empty() == true )
	{
		return;
	}

	TDynArray< ITimelineItem* > obsoleteItems;
	TDynArray< ITimelineItem* > newItems;

	TDynArray< CStorySceneEvent* > storyEvents;

	for ( Uint32 i = 0; i < m_selectedItems.Size(); ++i )
	{
		CTimelineItemEvent* item = dynamic_cast< CTimelineItemEvent* >( m_selectedItems[ i ] );
		if ( item == NULL )
		{
			continue;
		}

		CStorySceneEventCameraBlend* cameraBlendEvent = Cast< CStorySceneEventCameraBlend >( item->GetEvent() );
		if ( cameraBlendEvent == NULL )
		{
			continue;
		}

		const TDynArray< SStorySceneCameraBlendKey >& blendKeys = cameraBlendEvent->GetBlendKeys();
		for ( Uint32 j = 0; j < blendKeys.Size(); ++j )
		{
			Float keyTime = blendKeys[ j ].m_time;
			StorySceneCameraDefinition cameraDefinition = blendKeys[ j ].m_cameraDefinition;

			Float newEventAbsoluteStart = item->GetStart() + keyTime * GetEventInstanceDuration( *cameraBlendEvent );

			CTimelineItemBlocking* parentBlockingElement = NULL;
			for( TDynArray< CTimelineItemBlocking* >::iterator itemIter = m_elements.Begin();
				itemIter != m_elements.End(); ++itemIter )
			{
				Float elementStart = ( *itemIter )->GetStart();
				Float elementEnd = ( *itemIter )->GetStart() + ( *itemIter )->GetDuration();
				if( newEventAbsoluteStart >= elementStart && newEventAbsoluteStart <= elementEnd )
				{
					parentBlockingElement = *itemIter;
					break;
				}
			}

			if( parentBlockingElement == NULL )
			{
				return;
			}

			// Calculate position
			Float position = ( newEventAbsoluteStart - parentBlockingElement->GetStart() ) / parentBlockingElement->GetDuration();

			const StorySceneCameraDefinition* camDef = m_mediator->OnTimeline_GetCameraDefinition( cameraDefinition.m_cameraName );
			EngineTransform adjusted;
			CStorySceneEvent* storyEvent = nullptr;		
			if( camDef && 
				( 
					cameraDefinition.m_cameraTransform == camDef->m_cameraTransform || 
					(
						m_mediator->OnTimeline_AdjustCameraForActorHeight( *camDef, &adjusted ) &&
						Vector::Near2( cameraDefinition.m_cameraTransform.GetPosition(), adjusted.GetPosition() ) &&
						cameraDefinition.m_cameraTransform.GetRotation().AlmostEquals( adjusted.GetRotation() ) 
					)																								
				)  
			  )
			{
				CStorySceneEventCustomCameraInstance* camInstance = new CStorySceneEventCustomCameraInstance( String::EMPTY, parentBlockingElement->GetElement(), position, TIMELINE_DEFAULT_CAMERA_TRACK );
				camInstance->SetCustomCameraName( cameraDefinition.m_cameraName );
				storyEvent = camInstance;
			}
			else
			{
				storyEvent = new CStorySceneEventCustomCamera( TXT( "CustomCamera" ),
					parentBlockingElement->GetElement(), position, 
					cameraDefinition.m_cameraTransform.GetPosition(), 
					cameraDefinition.m_cameraTransform.GetRotation(), 
					cameraDefinition.m_cameraZoom,
					cameraDefinition.m_cameraFov, 
					cameraDefinition.m_dofFocusDistFar, 
					cameraDefinition.m_dofBlurDistFar, 
					cameraDefinition.m_dofIntensity, 
					cameraDefinition.m_dofFocusDistNear, 
					cameraDefinition.m_dofBlurDistNear,
					cameraDefinition.m_dof,
					TIMELINE_DEFAULT_CAMERA_TRACK );
			}

			ITimelineItem* newItem = CreateTimelineItemEvent( storyEvent, parentBlockingElement );

			newItems.PushBack( newItem );
			AddItem( newItem );

			storyEvents.PushBack( storyEvent );
		}

		obsoleteItems.PushBack( item );
	}

	while( obsoleteItems.Empty() == false )
	{
		ITimelineItem* obsoleteItem = obsoleteItems.PopBack();
		RemoveItem( obsoleteItem );
		delete obsoleteItem;
	}

	m_selectedItems.Clear();
	for ( Uint32 k = 0; k < newItems.Size(); ++k )
	{
		m_selectedItems.PushBack( newItems[ k ] );
	}

	SelectionChanged();

	for ( Uint32 i=0; i<storyEvents.Size(); ++i )
	{
		m_mediator->OnTimeline_AddEvent( m_section, storyEvents[ i ], GetSectionVariantId() );
	}
}


void CEdDialogTimeline::OnCameraBlendNewPlot( wxCommandEvent& event )
{
	if ( !IsOneSelected() )
	{
		return;
	}

	CTimelineItemCameraBlendEvent* item = dynamic_cast< CTimelineItemCameraBlendEvent* >( m_selectedItems[ 0 ] );
	if ( !item )
	{
		return;
	}

	const CStorySceneEvent* e = item->GetEvent();
	if ( e->GetClass()->IsA< CStorySceneCameraBlendEvent >() )
	{
		const CStorySceneCameraBlendEvent* camEvt = static_cast< const CStorySceneCameraBlendEvent* >( e );

		TDynArray< String > tracks;

		if ( event.GetId() == ID_BLEND_CAMERA_EVENT_PLOT_POSITION )
		{
			tracks.PushBack( TXT("Position.X") );
			tracks.PushBack( TXT("Position.Y") );
			tracks.PushBack( TXT("Position.Z") );

			tracks.PushBack( TXT("Key.Position.X") );
			tracks.PushBack( TXT("Key.Position.Y") );
			tracks.PushBack( TXT("Key.Position.Z") );
		}
		else if ( event.GetId() == ID_BLEND_CAMERA_EVENT_PLOT_ROTATION )
		{
			tracks.PushBack( TXT("Rotation.X") );
			tracks.PushBack( TXT("Rotation.Y") );
			tracks.PushBack( TXT("Rotation.Z") );

			tracks.PushBack( TXT("Key.Rotation.X") );
			tracks.PushBack( TXT("Key.Rotation.Y") );
			tracks.PushBack( TXT("Key.Rotation.Z") );
		}
		
		m_mediator->OnTimeline_CameraBlendNewPlot( camEvt, tracks );
	}
}

void CEdDialogTimeline::OnAddPlacementOverrideBlend( wxCommandEvent& event )
{
	if ( m_selectedItems.Empty() )
	{
		return;
	}

	// Create blend event

	::Sort( m_selectedItems.Begin(), m_selectedItems.End(), TimelineItemStartSorter() );

	CTimelineItemEvent* firstEvent = dynamic_cast< CTimelineItemEvent* >( m_selectedItems[0] );
	CStorySceneOverridePlacementBlend* blendEvent = new CStorySceneOverridePlacementBlend( TXT("placement override blend"), firstEvent->GetEvent()->GetSceneElement(), 0.0f, firstEvent->GetEvent()->GetTrackName(), firstEvent->GetEvent()->GetSubject() );
	if ( !blendEvent )
	{
		return;
	}

	m_mediator->OnTimeline_AddEvent( m_section, blendEvent, GetSectionVariantId() );

	CTimelineItemOverridePlacementBlend* blendItem = (CTimelineItemOverridePlacementBlend*) CreateTimelineItemEvent( blendEvent, firstEvent->GetElementItem() );
	if ( !blendItem )
	{
		delete blendEvent;
		return;
	}
	if ( !blendItem->Initialize( m_selectedItems ) )
	{
		delete blendItem;
		return;
	}

	m_mediator->OnTimeline_RebuildPlayer();

	// Add timeline item for the event

	m_items.PushBack( blendItem );

	// Update selection

	m_selectedItems.Clear();
	m_selectedItems.PushBack( blendItem );
	SelectionChanged();
}

void CEdDialogTimeline::OnCopyTransformToDialogsetSlotFromPlacementEvent( wxCommandEvent& event )
{
	if ( IsOneSelected() )
	{
		if ( CTimelineItemEvent* item = dynamic_cast< CTimelineItemEvent* >( m_selectedItems[ 0 ] ) )
		{
			if ( CStorySceneEventOverridePlacement* evt = Cast< CStorySceneEventOverridePlacement >( item->GetEvent() ) )
			{
				if ( CStorySceneDialogsetSlot* slot = m_mediator->OnTimeline_GetAndChangeActorDialogsetSlot( evt->GetSubject() ) )
				{
					slot->GetSlotPlacementRef() = evt->GetTransformRef();
				}
			}
		}
	}
}

void CEdDialogTimeline::OnCopyLightGoToCenterSS( wxCommandEvent& event )
{
	if ( IsOneSelected() )
	{
		if ( CTimelineItemEvent* item = dynamic_cast< CTimelineItemEvent* >( m_selectedItems[ 0 ] ) )
		{
			if ( CStorySceneEventLightProperties* evt = Cast< CStorySceneEventLightProperties >( item->GetEvent() ) )
			{
				EngineTransform& trans = evt->GetTransformRef();

				const CStorySceneLight* light = m_mediator->OnTimeline_GetLightDefinition( evt->GetSubject() );			
				if ( evt->IsAttached() )
				{
					if ( const CAnimatedComponent* ac = m_mediator->OnTimeline_GetBodyComponentForActor( evt->GetAttachmentActor() ) )
					{
						Int32 bone = ac->FindBoneByName( evt->GetAttachmentBone() );
						Matrix worldToLocal = ac->GetBoneMatrixWorldSpace( bone ).FullInverted();
						const Vector posWS = m_mediator->OnTimeline_CalcLightSpawnPositionWS();
						trans = worldToLocal.TransformPoint( posWS );
					}	
				}
				else
				{
					if ( evt->UseGlobalCoords() )
					{
						trans = m_mediator->OnTimeline_CalcLightSpawnPositionWS();
					}
					else
					{
						trans = m_mediator->OnTimeline_CalcLightSpawnPositionSS();
					}					
				}									
				m_mediator->OnTimeline_RefreshPlayer();
			}
		}
	}
}

void CEdDialogTimeline::OnToggleTimePause( wxCommandEvent& event )
{
	m_mediator->OnTimeline_ToggleTimePause();
}

void CEdDialogTimeline::OnLimitMinSet( wxCommandEvent& event )
{
	SetTimeLimitMin( m_timeSelectorTimePos );
}

void CEdDialogTimeline::OnLimitMaxSet( wxCommandEvent& event )
{
	SetTimeLimitMax( m_timeSelectorTimePos );
}

void CEdDialogTimeline::OnLimitMinReset( wxCommandEvent& event )
{
	SetTimeLimitMin( 0.f );
}

void CEdDialogTimeline::OnLimitMaxReset( wxCommandEvent& event )
{
	SetTimeLimitMax( m_activeRangeDuration );
}

void CEdDialogTimeline::OnArrowLeft( wxCommandEvent& event )
{
	GoToPrevFrame();
}

void CEdDialogTimeline::OnArrowRight( wxCommandEvent& event )
{
	GoToNextFrame();
}

void CEdDialogTimeline::OnArrowUp( wxCommandEvent& event )
{
	GoToNextCameraEvent();
}

void CEdDialogTimeline::OnArrowDown( wxCommandEvent& event )
{
	GoToPrevCameraEvent();
}

void CEdDialogTimeline::CreateSelectedAnimationPelvisPlacement()
{
	if ( IsOneSelected() )
	{
		if ( CTimelineItemAnimClip* item = dynamic_cast< CTimelineItemAnimClip* >( m_selectedItems[ 0 ] ) )
		{
			const Float time = item->GetStart() + item->GetDuration();

			CTimelineItemBlocking* selected = FindSceneElement( m_timeSelectorTimePos );
			const CName actorId = GetSelectedEntityName();

			SCENE_ASSERT( MAbs( time - m_timeSelectorTimePos ) < 0.01f )

			if ( actorId && selected )
			{
				EngineTransform actorTransform;
				if ( m_mediator->OnTimeline_GetActorPelvisPlacementLS( actorId, actorTransform ) )
				{
					EngineTransform scenePlacement = m_mediator->OnTimeline_GetScenePlacement();
					Matrix ret = StorySceneUtils::CalcWSFromSS( actorTransform, scenePlacement );
					actorTransform.Init( ret );

					const Float absTime = time;
					const void* extraData = &actorTransform;

					CreateEvent( ClassID< CStorySceneEventOverridePlacement >(), absTime, actorId, extraData );
				}
			}
		}
	}
}

void CEdDialogTimeline::OnCreatePlacementForAnimPelvis( wxCommandEvent& event )
{
	if ( IsOneSelected() )
	{
		if ( CTimelineItemAnimClip* item = dynamic_cast< CTimelineItemAnimClip* >( m_selectedItems[ 0 ] ) )
		{
			const Float time = item->GetStart() + item->GetDuration();

			m_mediator->OnTimeline_GoToTime( time );

			m_mediator->RunLaterOnce( [ this ](){ CreateSelectedAnimationPelvisPlacement(); } );
		}
	}
}

void CEdDialogTimeline::OnGroupEvents( wxCommandEvent& event )
{
	if ( m_selectedItems.Empty() == true )
	{
		return;
	}

	// Sort selected items by time
	::Sort( m_selectedItems.Begin(), m_selectedItems.End(), TimelineItemStartSorter() );

	CTimelineItemEvent* firstItem = dynamic_cast< CTimelineItemEvent* >( m_selectedItems[ 0 ] );
	CTimelineItemEvent* lastItem = dynamic_cast< CTimelineItemEvent* >( m_selectedItems.Back() );

	if ( firstItem == NULL || lastItem == NULL )
	{
		return;
	}

	// Get basic parameters
	Float startPosition = firstItem->GetEvent()->GetStartPosition();
	CStorySceneElement* element = firstItem->GetEvent()->GetSceneElement();
	//String trackName( firstItem->GetEvent()->GetTrackName() );

	CStorySceneEventGroup* groupEvent = new CStorySceneEventGroup( TXT( "event group" ), element, TXT( "_GROUPS" ), startPosition, 1.0f );

	TDynArray< CTimelineItemEvent* > obsoleteItems;
	Float startTime = firstItem->GetStart();
	Float endTime = startTime;

	for ( Uint32 i = 0; i < m_selectedItems.Size(); ++i )
	{
		CTimelineItemEvent* item = dynamic_cast< CTimelineItemEvent* >( m_selectedItems[ i ] );
		if ( item == NULL )
		{
			continue;
		}

		if ( item->GetEvent()->HasBlendParent() )
		{
			continue; // Event already belongs to blend
		}

		Float keyTime = ::Max< Float >( 0.0f, ( item->GetStart() - firstItem->GetStart() ) );

		groupEvent->AddEvent( item->GetEvent(), keyTime );
		obsoleteItems.PushBack( item );

		endTime = ::Max< Float >( item->GetStart() + item->GetDuration(), endTime );
	}


	groupEvent->SetDuration( ::Min< Float >( endTime, GetActiveRangeDuration() ) - startTime );

	ITimelineItem* groupItem = CreateTimelineItemEvent( groupEvent, firstItem->GetElementItem() );
	AddItem( groupItem );

	// Remove from timeline
	while( obsoleteItems.Empty() == false )
	{
		CTimelineItemEvent* obsoleteItem = obsoleteItems.PopBack();
		RemoveItem( obsoleteItem );
		delete obsoleteItem;
	}

	RecreateTracks();

	// Update selection
	m_selectedItems.Clear();
	m_selectedItems.PushBack( groupItem );
	SelectionChanged();

	m_mediator->OnTimeline_AddEvent( m_section, groupEvent, GetSectionVariantId() );
}

void CEdDialogTimeline::OnUngroupEvents( wxCommandEvent& event )
{
	if ( m_selectedItems.Empty() == true )
	{
		return;
	}

	TDynArray< ITimelineItem* > obsoleteItems;
	TDynArray< ITimelineItem* > newItems;

	for ( Uint32 i = 0; i < m_selectedItems.Size(); ++i )
	{
		CTimelineItemEventGroup* item = dynamic_cast< CTimelineItemEventGroup* >( m_selectedItems[ i ] );
		if ( item == NULL )
		{
			continue;
		}

		CStorySceneEventGroup* groupEvent = Cast< CStorySceneEventGroup >( item->GetEvent() );
		if ( groupEvent == NULL )
		{
			continue;
		}

		const TDynArray< SStorySceneEventGroupEntry >& blendKeys = groupEvent->GetEvents();
		for ( Uint32 j = 0; j < blendKeys.Size(); ++j )
		{
			Float eventTime = blendKeys[ j ].m_time;
			CStorySceneEvent* event = blendKeys[ j ].m_event;

			Float newEventAbsoluteStart = item->GetStart() + eventTime;

			CTimelineItemBlocking* parentBlockingElement = FindSceneElement( newEventAbsoluteStart );
			if( parentBlockingElement == NULL )
			{
				return;
			}

			// Calculate position
			Float position = ( newEventAbsoluteStart - parentBlockingElement->GetStart() ) / parentBlockingElement->GetDuration();
			event->SetStartPosition( position );

			ITimelineItem* newItem = CreateTimelineItemEvent( event, parentBlockingElement );

			newItems.PushBack( newItem );
			AddItem( newItem );

			m_mediator->OnTimeline_AddEvent( m_section, event, GetSectionVariantId() );
		}

		obsoleteItems.PushBack( item );
	}

	while( obsoleteItems.Empty() == false )
	{
		ITimelineItem* obsoleteItem = obsoleteItems.PopBack();
		RemoveItem( obsoleteItem );
		delete obsoleteItem;
	}

	m_selectedItems.Clear();
	for ( Uint32 k = 0; k < newItems.Size(); ++k )
	{
		m_selectedItems.PushBack( newItems[ k ] );
	}

	SelectionChanged();
}

void CEdDialogTimeline::OnLinkEvent( wxCommandEvent& event )
{
	if( !IsTimelineEditingEnabled() )
	{
		GFeedback->ShowMsg( TXT( "Scene Editor" ), TXT( "Link - operation not allowed in unapproved section." ) );
		return;
	}

	if ( !IsOneSelected() )
	{
		return;
	}

	CTimelineItemEvent* item = dynamic_cast< CTimelineItemEvent* >( m_selectedItems[ 0 ] );
	if ( !item )
	{
		return;
	}
	
	m_eventsLinker.StartLinkingProcess( item );
}

void CEdDialogTimeline::OnUnlinkEvent( wxCommandEvent& event )
{
	if( !IsTimelineEditingEnabled() )
	{
		GFeedback->ShowMsg( TXT( "Scene Editor" ), TXT( "Unlink - operation not allowed as timeline editing is disabled." ) );
		return;
	}

	if ( IsOneSelected() )
	{
		CTimelineItemEvent* item = dynamic_cast< CTimelineItemEvent* >( m_selectedItems[ 0 ] );
		if ( item && item->AutoLinkToThisEvent() )
		{
			m_eventsLinker.UnlinkAllChildrenEvents( item );
			return;
		}
	}

	for ( Uint32 i = 0; i < m_selectedItems.Size(); ++i )
	{
		CTimelineItemEvent* item = dynamic_cast< CTimelineItemEvent* >( m_selectedItems[ i ] );
		m_eventsLinker.UnlinkEvent( item );
	}
}

void CEdDialogTimeline::OnMuteEvent( wxCommandEvent& event )
{
	if( !IsTimelineEditingEnabled() )
	{
		GFeedback->ShowMsg( TXT( "Scene Editor" ), TXT( "Mute/unmute - operation not allowed in unapproved section." ) );
		return;
	}

	for ( Uint32 i = 0; i < m_selectedItems.Size(); ++i )
	{
		CTimelineItemEvent* item = dynamic_cast< CTimelineItemEvent* >( m_selectedItems[ i ] );
		CStorySceneEvent* e = item->GetEvent();

		e->Mute( !e->IsMuted() );
	}
}

void CEdDialogTimeline::OnGoToMarker( wxCommandEvent& event )
{
	if ( m_selectedItems.Size() > 0 )
	{
		if ( CTimelineItemAnimClip* item = dynamic_cast< CTimelineItemAnimClip* >( m_selectedItems[ 0 ] ) )
		{
			const Float currPos = GetCurrentPosition();

			Float best = FLT_MAX;
			Float time = -1.f;

			const TDynArray< Float >& markers = item->GetKeyPoseMarkers();
			for ( Uint32 i=0; i<markers.Size(); ++i )
			{
				const Float m = item->GetStart() + item->GetDuration() * markers[ i ];

				const Float dist = MAbs( m - currPos );
				if ( dist < best )
				{
					best = dist;
					time = m;
				}
			}

			if ( time > 0.f )
			{
				m_mediator->OnTimeline_GoToTime( time );
			}
		}
	}
}

void CEdDialogTimeline::OnCopyLightFromCamera( wxCommandEvent& event )
{
	if ( IsOneSelected() )
	{
		if ( CTimelineItemEvent* selItem = dynamic_cast< CTimelineItemEvent* >( m_selectedItems[ 0 ] ) )
		{
			TDynArray< ITimelineItem* > titems;
			CollectItemsAt( selItem->GetStart(), titems );

			TDynArray< ITimelineItem* > evtToCopy;

			for ( Uint32 i=0; i<titems.Size(); ++i )
			{
				if ( CTimelineItemEvent* item = dynamic_cast< CTimelineItemEvent* >( titems[ i ] ) )
				{
					if ( CStorySceneEvent* evt = item->GetEvent() )
					{
						if ( evt->GetClass()->IsA< CStorySceneEventLightProperties >() )
						{
							evtToCopy.PushBack( item );
						}
					}
				}
			}

			if ( evtToCopy.Size() > 0 )
			{
				CopyItems( evtToCopy, false );
			}
		}
	}
}

void CEdDialogTimeline::OnPasteLightToCamera( wxCommandEvent& event )
{
	if ( IsOneSelected() )
	{
		if ( CTimelineItemEvent* item = dynamic_cast< CTimelineItemEvent* >( m_selectedItems[ 0 ] ) )
		{
			const Float time = item->GetStart();

			StoreLayout();

			TDynArray< ITimelineItem* > pastedItems;
			PasteItems( pastedItems, PIM_AllToOneCustomTime, time, false );

			RestoreLayout();
		}
	}
}

void CEdDialogTimeline::OnToggleEntityHelperIsInteractive( wxCommandEvent& event )
{
	m_mediator->OnTimeline_ToggleIsInteractiveEntityHelper();
}

void CEdDialogTimeline::OnCreatePause( wxCommandEvent& event )
{
	SCENE_ASSERT( m_section );

	CStoryScenePauseElement* pause = CreateObject< CStoryScenePauseElement >( const_cast< CStorySceneSection*>( m_section ) );
	Uint32 placement = 0;
	switch( event.GetId() )
	{
	case ID_ADD_PAUSE_5:
		pause->SetDuration( 5.0f );
		break;
	case ID_ADD_PAUSE_15:
		pause->SetDuration( 15.0f );
		break;
	case ID_ADD_PAUSE_30:
		pause->SetDuration( 30.0f );
		break;
	case ID_ADD_PAUSE_AFTER:
	case ID_ADD_PAUSE_BEFORE:
		{
			CTimelineItemBlocking* element = dynamic_cast< CTimelineItemBlocking* >( m_selectedItems[ 0 ] );		
			if( ! IsOneSelected() || ! m_selectedItems[ 0 ] || !element )
			{
				return;
			}			
			for( Uint32 i = 0; i < m_section->GetNumberOfElements(); ++i )
			{
				if( m_section->GetElement( i ) == element->GetElement() )
				{
					placement = event.GetId() == ID_ADD_PAUSE_BEFORE ? i : i+1;
					placement = Clamp< Uint32 >( placement, 0, m_section->GetNumberOfElements() );
					break;
				}
			}
		} 
		// no break def value for duration
	default:
		pause->SetDuration( 1.0f );
		break;
	}

	m_mediator->OnTimeline_AddSceneElement( m_section, pause, placement );

	// Approve pause duration in all variants. Pauses are auto-approved since they are always explicitly added be the user.
	// Note that pause duration is the same in all locales but we still have to pass locale arg when getting its duration.
	const Uint32 pauseDuration = pause->CalculateDuration( SLocalizationManager::GetInstance().GetCurrentLocale() );
	TDynArray< CStorySceneSectionVariantId > variants;
	variants.Reserve( m_section->GetNumVariants() );
	m_section->EnumerateVariants( variants );
	for( const CStorySceneSectionVariantId variant : variants )
	{
		m_mediator->OnTimeline_ApproveElementDuration( m_section, variant, pause, pauseDuration );
	}
}

Bool CEdDialogTimeline::ShowMenuForDialogsetBodyData( const CName& actorName, SStorySceneActorAnimationState* state ) const
{
	CStorySceneDialogsetSlot* slot = m_mediator->OnTimeline_GetAndChangeActorDialogsetSlot( actorName );
	if ( slot )
	{
		CName status = slot->GetBodyFilterStatus();
		CName emoState = slot->GetBodyFilterEmotionalState();
		CName pose = slot->GetBodyFilterPoseName();

		Bool ret = ShowMenuForActorsBodyFilterData( status, emoState, pose );
		if ( ret )
		{
			slot->SetBodyFilterStatus( status );
			slot->SetBodyFilterEmotionalState( emoState );
			slot->SetBodyFilterPoseName( pose );

			if ( state )
			{
				state->m_status = status;
				state->m_emotionalState = emoState;
				state->m_poseType = pose;
			}
		}

		return ret;
	}
	return false;
}


namespace
{
	void BuildUnfilteredAnimListDialog( const CSkeletalAnimationContainer& cont, const CStorySceneAnimationList& list, CEdItemSelectorDialog< const CName >& dlg )
	{
		for ( CStorySceneAnimationList::AllBodyAnimationsIterator it( list ); it; ++it )
		{
			String path = 
				it.GetTypeName().AsString() + TXT("|") + 
				it.GetStatus().AsString()   + TXT("|") + 
				it.GetEmoState().AsString() + TXT("|") + 
				it.GetPose().AsString()     + TXT("|") + 
				(*it).m_friendlyName;

			if ( cont.HasAnimation( (*it).m_animationName ) )
			{
				dlg.AddItem( path, &(*it).m_animationName, true );
			}
		}
	}
}

Bool CEdDialogTimeline::ShowMenuForAnimationData( CStorySceneEventAnimation* animationEvt, Bool selectAllFilters, Bool unfiltered ) const
{
	if ( animationEvt->GetActor() == CName::NONE )
	{
		return false;
	}

	// Copy slot->animation
	CStorySceneDialogsetSlot* slot = m_mediator->OnTimeline_GetAndChangeActorDialogsetSlot( animationEvt->GetActor() );
	if ( slot )
	{
		const CName slotStatus = slot->GetBodyFilterStatus();
		const CName slotEmoState = slot->GetBodyFilterEmotionalState();
		const CName slotPose = slot->GetBodyFilterPoseName();

		const CName status = animationEvt->GetBodyFilterStatus();
		const CName emoState = animationEvt->GetBodyFilterEmotionalState();
		const CName pose = animationEvt->GetBodyFilterPoseName();

		if ( !status )
		{
			animationEvt->SetBodyFilterStatus( slotStatus );
		}
		if ( !emoState  )
		{
			animationEvt->SetBodyFilterEmotionalState( slotEmoState );
		}
		if ( !pose )
		{
			animationEvt->SetBodyFilterPoseName( slotPose );
		} 
	}


	// Fill animation's data manually
	CName status = animationEvt->GetBodyFilterStatus();
	CName emoState = animationEvt->GetBodyFilterEmotionalState();
	CName pose = animationEvt->GetBodyFilterPoseName();
	CName type = animationEvt->GetBodyFilterTypeName();

	Bool ret = unfiltered || ShowMenuForActorsBodyFilterData( status, emoState, pose, &type, selectAllFilters );
	if ( ret )
	{
		animationEvt->SetBodyFilterStatus( status );
		animationEvt->SetBodyFilterEmotionalState( emoState );
		animationEvt->SetBodyFilterPoseName( pose );
		animationEvt->SetBodyFilterTypeName( type );

		// If slot is empty then copy animation->slot
		const Bool blockSlotOverride = IsType< CStorySceneEventAdditiveAnimation >( animationEvt );
		CStorySceneDialogsetSlot* slot = m_mediator->OnTimeline_GetAndChangeActorDialogsetSlot( animationEvt->GetActor() );
		if ( slot && !blockSlotOverride )
		{
			const CName slotStatus = slot->GetBodyFilterStatus();
			const CName slotEmoState = slot->GetBodyFilterEmotionalState();
			const CName slotPose = slot->GetBodyFilterPoseName();

			if ( !slotStatus && slotStatus != status )
			{
				slot->SetBodyFilterStatus( status );
			}
			if ( !slotEmoState && slotEmoState != emoState )
			{
				slot->SetBodyFilterEmotionalState( emoState );
			}
			if ( !slotPose && slotPose != pose )
			{
				slot->SetBodyFilterPoseName( pose );
			}
		}

		CEdItemSelectorDialog< const CName > ed( nullptr );

		const CAnimatedComponent* ac = m_mediator->OnTimeline_GetBodyComponentForActor( animationEvt->GetActor() );
		if ( ac && ac->GetAnimationContainer() )
		{
			const CSkeletalAnimationContainer* cont = ac->GetAnimationContainer();

			const CStorySceneAnimationList& list = GCommonGame->GetSystem< CStorySceneSystem >()->GetAnimationList();

			wxPoint pos = wxGetMousePosition();

			if ( unfiltered )
			{
				BuildUnfilteredAnimListDialog( *cont, list, ed );
			}
			else
			{
				for ( CStorySceneAnimationList::BodyAnimationIterator it( list, animationEvt->GetBodyFilterStatus(), animationEvt->GetBodyFilterEmotionalState()
					, animationEvt->GetBodyFilterPoseName(), animationEvt->GetBodyFilterTypeName() ); it; ++it )
				{
					if ( cont->HasAnimation( (*it).m_animationName ) )
					{
						Bool selected = ( (*it).m_friendlyName == animationEvt->GetAnimationFriendlyName() );
						ed.AddItem( (*it).m_friendlyName, &(*it).m_animationName, true, -1, selected );
					}
				}
			}

			if ( const CName* selected = ed.Execute() )
			{
				const String selectedAnimStr = ed.FindNameForData( selected );
				CName selectedAnimName( selectedAnimStr );

				TDynArray< CName > extraData;
				extraData.PushBack( animationEvt->GetBodyFilterStatus() );
				extraData.PushBack( animationEvt->GetBodyFilterEmotionalState() );
				extraData.PushBack( animationEvt->GetBodyFilterPoseName() );
				extraData.PushBack( animationEvt->GetBodyFilterTypeName() );
				extraData.PushBack( selectedAnimName );
				extraData.PushBack( *selected );

				animationEvt->SetAnimationState( extraData );

				const Float duration = m_mediator->OnTimeline_GetBodyAnimationDuration( animationEvt->GetActor(), animationEvt->GetAnimationName() );
				animationEvt->RefreshDuration( duration );

				ret = true;
			}
		}
	}

	return ret;
}

Bool CEdDialogTimeline::ShowMenuForAnimationData( CStorySceneEventChangePose* animationEvt, Bool selectAllFilters ) const
{
	if ( animationEvt->GetActor() == CName::NONE )
	{
		return false;
	}

	CName status = animationEvt->GetBodyFilterStatus();
	CName emoState = animationEvt->GetBodyFilterEmotionalState();
	CName pose = animationEvt->GetBodyFilterPoseName();
	Bool ret = ShowMenuForActorsBodyFilterData( status, emoState, pose, nullptr, selectAllFilters );
	if ( ret )
	{
		TDynArray< CName > state(3);
		state[ 0 ] = status;
		state[ 1 ] = emoState;
		state[ 2 ] = pose;
		animationEvt->SetAnimationState( state );

		SStorySceneActorAnimationState currState;
		SStorySceneActorAnimationState destState( status, emoState, pose );
		if ( m_mediator->OnDialogAnimationFilter_GetPreviousActorAnimationState( animationEvt->GetActor(), currState ) )
		{
			TDynArray< CName > transitions;
			GCommonGame->GetSystem< CStorySceneSystem >()->GetAnimationList().FindBodyTransitions( currState, destState, transitions );
			
			CEdItemSelectorDialog< const CName > ed( nullptr );

			for ( Uint32 i=0; i<transitions.Size(); ++i )
			{
				Bool selected = ( transitions[ i ] == animationEvt->GetAnimationName() );
				ed.AddItem( transitions[ i ].AsString(), &transitions[ i ], true, -1, selected );
			}

			if ( const CName* selected = ed.Execute() )
			{
				animationEvt->SetAnimationName( *selected );
				const Float duration = m_mediator->OnTimeline_GetBodyAnimationDuration( animationEvt->GetActor(), animationEvt->GetAnimationName() );
				animationEvt->RefreshDuration( duration );
			}
		}		
	}
	return true;
}

Bool CEdDialogTimeline::ShowMenuForAnimationData( CStorySceneEventMimicsAnim* animationEvt, Bool selectAllFilters ) const
{
	if ( animationEvt->GetActor() == CName::NONE )
	{
		return false;
	}

	// Fill animation's data manually
	CName actionFilter = animationEvt->GetMimicsActionFilter();

	Bool ret = ShowMenuForActorsMimicsFilterData( actionFilter, selectAllFilters );
	if ( ret )
	{
		animationEvt->SetMimicksActionFilter( actionFilter );

		CStorySceneDialogsetSlot* slot = m_mediator->OnTimeline_GetAndChangeActorDialogsetSlot( animationEvt->GetActor() );

		const CAnimatedComponent* ac = m_mediator->OnTimeline_GetMimicComponentForActor( animationEvt->GetActor() );
		if ( ac && ac->GetAnimationContainer() )
		{
			const CSkeletalAnimationContainer* cont = ac->GetAnimationContainer();

			const CStorySceneAnimationList& list = GCommonGame->GetSystem< CStorySceneSystem >()->GetAnimationList();

			CEdItemSelectorDialog< const CName > ed( nullptr );

			for ( CStorySceneAnimationList::MimicsAnimationIteratorByAction it( list, actionFilter ); it; ++it )
			{
				if ( cont->HasAnimation( (*it).m_animationName ) )
				{
					Bool selected = ( (*it).m_friendlyName == animationEvt->GetAnimationFriendlyName() );
					ed.AddItem( (*it).m_friendlyName, &(*it).m_animationName, true, -1, selected );
				}
			}

			if ( const CName* selected = ed.Execute() )
			{
				const String selectedAnimStr = ed.FindNameForData( selected );
 				CName selectedAnimName( selectedAnimStr );

				TDynArray< CName > extraData;
				extraData.PushBack( actionFilter );
				extraData.PushBack( selectedAnimName );
				extraData.PushBack( *selected );

				animationEvt->SetAnimationState( extraData );

				const Float duration = m_mediator->OnTimeline_GetMimicAnimationDuration( animationEvt->GetActor(), animationEvt->GetAnimationName() );
				animationEvt->RefreshDuration( duration );
			}
		}
	}

	return ret;
}

Bool CEdDialogTimeline::ShowMenuForActorsMimicsFilterData( CName& inOutEmoState, Bool selectAllFilters ) const
{
	const CStorySceneAnimationList& list = GCommonGame->GetSystem< CStorySceneSystem >()->GetAnimationList();

	if ( inOutEmoState == CName::NONE || selectAllFilters )
	{
		CEdItemSelectorDialog< const CName > ed( nullptr );

		for ( CStorySceneAnimationList::ActionTypeMimicsIterator it( list ); it; ++it )
		{
			Bool selected = ( *it == inOutEmoState );
			ed.AddItem( (*it).AsString(), &(*it), true, -1, selected );
		}

		if ( !ed.IsEmpty() )
		{
			if ( const CName* selected = ed.Execute() )
			{
				inOutEmoState = *selected;
			}
		}
	}

	return inOutEmoState != CName::NONE;
}

Bool CEdDialogTimeline::ShowMenuForActorsBodyFilterData( CName& inOutStatus, CName& inOutEmoState, CName& inOutPose, CName* inOutType, Bool selectAllFilters ) const
{
	const CStorySceneAnimationList& list = GCommonGame->GetSystem< CStorySceneSystem >()->GetAnimationList();

	wxPoint pos = wxGetMousePosition(); // to keep the window in the same spot during successive selections

	if ( inOutStatus == CName::NONE || selectAllFilters )
	{
		CEdItemSelectorDialog< const CName > ed( nullptr );

		for ( CStorySceneAnimationList::StatusBodyIterator it( list ); it; ++it )
		{
			Bool selected = ( *it == inOutStatus );
			ed.AddItem( (*it).AsString(), &(*it), true, -1, selected );
		}

		if ( !ed.IsEmpty() )
		{
			if ( const CName* selected = ed.Execute( &pos ) )
			{
				inOutStatus = *selected;
			}
			else
			{
				return false;
			}
		}
	}

	const CName& aStatus = inOutStatus;
	if ( aStatus != CName::NONE )
	{
		if ( inOutEmoState == CName::NONE || selectAllFilters )
		{
			CEdItemSelectorDialog< const CName > ed( nullptr );

			for ( CStorySceneAnimationList::EmotionalStatesBodyIterator it( list, aStatus ); it; ++it )
			{
				Bool selected = ( *it == inOutEmoState );
				ed.AddItem( (*it).AsString(), &(*it), true, -1, selected );
			}

			if ( !ed.IsEmpty() )
			{
				if ( const CName* selected = ed.Execute( &pos ) )
				{
					inOutEmoState = *selected;
				}
				else
				{
					return false;
				}
			}
		}

		const CName& aEmoState = inOutEmoState;
		if ( aEmoState != CName::NONE )
		{
			if ( inOutPose == CName::NONE || selectAllFilters )
			{
				CEdItemSelectorDialog< const CName > ed( nullptr );

				for ( CStorySceneAnimationList::PoseBodyIterator it( list, aStatus, aEmoState ); it; ++it )
				{
					Bool selected = ( *it == inOutPose );
					ed.AddItem( (*it).AsString(), &(*it), true, -1, selected );
				}

				if ( !ed.IsEmpty() )
				{
					if ( const CName* selected = ed.Execute( &pos ) )
					{
						inOutPose = *selected;
					}
					else
					{
						return false;
					}
				}
			}

			const CName& aPose = inOutPose;
			if ( inOutType && aPose != CName::NONE )
			{
				if ( *inOutType == CName::NONE || selectAllFilters )
				{
					CEdItemSelectorDialog< const CName > ed( nullptr );

					for ( CStorySceneAnimationList::TypeBodyIterator it( list, aStatus, aEmoState, aPose ); it; ++it )
					{
						Bool selected = ( *it == *inOutType );
						ed.AddItem( (*it).AsString(), &(*it), true, -1, selected );
					}

					if ( !ed.IsEmpty() )
					{
						if ( const CName* selected = ed.Execute( &pos ) )
						{
							*inOutType = *selected;
						}
						else
						{
							return false;
						}
					}
				}
			}
		}
	}

	return inOutPose != CName::NONE && inOutEmoState != CName::NONE && inOutStatus != CName::NONE && ( inOutType ? *inOutType != CName::NONE : true );
}

void CEdDialogTimeline::ShowMenuWithAnimationMimicsExtraData( const CName& actorName, TDynArray< CName >& extraData ) const
{
	const CAnimatedComponent* ac = m_mediator->OnTimeline_GetMimicComponentForActor( actorName );
	if ( ac && ac->GetAnimationContainer() )
	{
		const CSkeletalAnimationContainer* cont = ac->GetAnimationContainer();

		const CStorySceneAnimationList& list = GCommonGame->GetSystem< CStorySceneSystem >()->GetAnimationList();

		CEdItemSelectorDialog< const CName > edActions( nullptr );

		for ( CStorySceneAnimationList::ActionTypeMimicsIterator it( list ); it; ++it )
		{
			edActions.AddItem( (*it).AsString(), &(*it), true );
		}

		if ( const CName* selectedAction = edActions.Execute() )
		{
			CEdItemSelectorDialog< const CName > edAnims( nullptr );

			for ( CStorySceneAnimationList::MimicsAnimationIteratorByAction it( list, *selectedAction ); it; ++it )
			{
				edAnims.AddItem( (*it).m_friendlyName, &(*it).m_animationName, true );
			}

			if ( const CName* selectedAnim = edAnims.Execute() )
			{
				const String selectedAnimStr = edAnims.FindNameForData( selectedAnim );
 				CName selectedAnimName( selectedAnimStr );

				extraData.PushBack( *selectedAction );
				extraData.PushBack( selectedAnimName );
				extraData.PushBack( *selectedAnim );
			}
		}
	}
}

Bool CEdDialogTimeline::ShowMenuWithAnimationBodyExtraData( const CName& actorName, TDynArray< CName >& extraData, bool unfiltered ) const
{
	const CAnimatedComponent* ac = m_mediator->OnTimeline_GetBodyComponentForActor( actorName );

	if ( !ac || !ac->GetAnimationContainer() )
	{
		return false;
	}

	const CSkeletalAnimationContainer* cont = ac->GetAnimationContainer();

	const CStorySceneAnimationList& list = GCommonGame->GetSystem< CStorySceneSystem >()->GetAnimationList();

	wxPoint pos = wxGetMousePosition(); // to keep the window in the same spot during successive selections
	CEdItemSelectorDialog< const CName > edAnims( nullptr );

	if ( unfiltered )
	{
		// == show the full, hierarchical list of animations ==
		BuildUnfilteredAnimListDialog( *cont, list, edAnims );
	}
	else
	{
		// == filter animations by the current actor's state ==

		SStorySceneActorAnimationState state;
		Bool ret = m_mediator->OnTimeline_GetCurrentActorAnimationState( actorName, state ) && state.IsBodySet();

		if ( !ret )
		{
			ret = ShowMenuForDialogsetBodyData( actorName, &state );
		}

		CEdItemSelectorDialog< const CName > edType( nullptr );

		for ( CStorySceneAnimationList::TypeBodyIterator it( list, state.m_status, state.m_emotionalState, state.m_poseType ); it; ++it )
		{
			Bool selected = ( *it == CStorySceneAnimationList::GESTURE_KEYWORD );
			edType.AddItem( (*it).AsString(), &(*it), true, -1, selected );
		}

		if ( !edType.IsEmpty() )
		{
			if ( const CName* selectedType = edType.Execute( &pos ) )
			{
				for ( CStorySceneAnimationList::BodyAnimationIterator it( list, state.m_status, state.m_emotionalState, state.m_poseType, *selectedType ); it; ++it )
				{
					if ( cont->HasAnimation( (*it).m_animationName ) )
					{
						edAnims.AddItem( (*it).m_friendlyName, &(*it).m_animationName, true );
					}
				}
			}
			else
			{
				return false;
			}
		}
		else
		{
			return false;
		}
	}

	if ( const CName* selectedAnim = edAnims.Execute( &pos ) )
	{
		SStorySceneActorAnimationState state;
		CName animType;
		if ( list.FindStateByAnimation( *selectedAnim, state, &animType ) )
		{
			const String selectedAnimStr = edAnims.FindNameForData( selectedAnim );
			CName selectedAnimName( selectedAnimStr );

			extraData.PushBack( state.m_status );
			extraData.PushBack( state.m_emotionalState );
			extraData.PushBack( state.m_poseType );
			extraData.PushBack( animType );
			extraData.PushBack( selectedAnimName );
			extraData.PushBack( *selectedAnim );

			return true;
		}
	}

	return false;
}

/*
Handles "Set as background line" menu item.
*/
void CEdDialogTimeline::OnSetAsBackgroundLine( wxCommandEvent& event )
{
	if( GetSelectedItemCount() == 1 )
	{
		CTimelineItemLineElement* lineItem = dynamic_cast< CTimelineItemLineElement* >( GetSelectedItem( 0 ) );
		SCENE_ASSERT( lineItem ); // We should not be here if selected item is not CTimelineItemLineElement.
		SCENE_ASSERT( !lineItem->IsBackgroundLine() );

		Bool proceed = true;

		// Delete all events associated with dialog line.
		TDynArray< DialogTimelineItems::CTimelineItemEvent* > evts;
		Uint32 numEvts = GetElementEvents( lineItem, evts );
		if( numEvts > 0 )
		{
			proceed = YesNo( TXT( "All events associated with this dialog line will be deleted. Do you want to proceed?" ) );

			if( proceed )
			{
				Bool allEvtsAreRemovable = true;
				for( Uint32 i = 0; i < numEvts; ++i )
				{
					if( !evts[ i ]->IsRemovable() )
					{
						allEvtsAreRemovable = false;
						break;
					}
				}

				if( allEvtsAreRemovable )
				{
					for( Uint32 i = 0; i < numEvts; ++i )
					{
						ITimelineItem* item = evts[ i ];
						RemoveItem( item );
						delete item;
					}
				}
				else
				{
					wxMessageBox( TXT("Some events associated with this dialog are not removable. Operation cancelled." ) );
					proceed = false;
				}
			}
		}

		if( proceed )
		{
			CStorySceneLine* sceneLine = Cast< CStorySceneLine >( lineItem->GetElement() );
			m_mediator->OnTimeline_SetAsBackgroundLine( sceneLine, true );
		}
		// else
		  // don't set the line as a background line because user didn't want to delete
		  // events associated with the line or some of those events were not removable
	}
}

/*
Handles "Set as non-background line" menu item.
*/
void CEdDialogTimeline::OnSetAsNonBackgroundLine( wxCommandEvent& event )
{
	if( GetSelectedItemCount() == 1 )
	{
		CTimelineItemLineElement* lineItem = dynamic_cast< CTimelineItemLineElement* >( GetSelectedItem( 0 ) );
		SCENE_ASSERT( lineItem ); // We should not be here if selected item is not CTimelineItemLineElement.
		SCENE_ASSERT( lineItem->IsBackgroundLine() );

		CStorySceneLine* sceneLine = Cast< CStorySceneLine >( lineItem->GetElement() );
		m_mediator->OnTimeline_SetAsBackgroundLine( sceneLine, false );
	}
}

void CEdDialogTimeline::GoToPrevCameraEvent()
{
	const Float currTime = m_timeSelectorTimePos;
	Float bestTime = -1.f;
	Float bestDiff = FLT_MAX;

	const Uint32 num = m_items.Size();
	for ( Uint32 i=0; i<num; ++i )
	{
		Float itemTime = -1.f;

		if ( CTimelineItemCustomCamera* item = dynamic_cast< CTimelineItemCustomCamera* >( m_items[ i ] ) )
		{
			itemTime = item->GetStart();
		}
		else if ( CTimelineItemCustomCameraInstance* item = dynamic_cast< CTimelineItemCustomCameraInstance* >( m_items[ i ] ) )
		{
			itemTime = item->GetStart();
		}

		if ( itemTime >= 0.f )
		{
			const Float diff = itemTime - currTime;
			if ( diff < -0.002f && MAbs( diff ) < bestDiff )
			{
				bestDiff = MAbs( diff );
				bestTime = itemTime;
			}
		}
	}

	if ( bestTime >= 0.f )
	{
		m_mediator->OnTimeline_GoToTime( bestTime );
	}
}

void CEdDialogTimeline::GoToNextCameraEvent()
{
	const Float currTime = m_timeSelectorTimePos;
	Float bestTime = -1.f;
	Float bestDiff = FLT_MAX;

	const Uint32 num = m_items.Size();
	for ( Uint32 i=0; i<num; ++i )
	{
		Float itemTime = -1.f;

		if ( CTimelineItemCustomCamera* item = dynamic_cast< CTimelineItemCustomCamera* >( m_items[ i ] ) )
		{
			itemTime = item->GetStart();
		}
		else if ( CTimelineItemCustomCameraInstance* item = dynamic_cast< CTimelineItemCustomCameraInstance* >( m_items[ i ] ) )
		{
			itemTime = item->GetStart();
		}

		if ( itemTime >= 0.f )
		{
			const Float diff = itemTime - currTime;
			if ( diff > 0.002f && diff < bestDiff )
			{
				bestDiff = diff;
				bestTime = itemTime;
			}
		}
	}

	if ( bestTime >= 0.f )
	{
		m_mediator->OnTimeline_GoToTime( bestTime );
	}
}

void CEdDialogTimeline::GoToPrevFrame()
{
	Float nextTime =  m_timeSelectorTimePos - 1.f/30.f;
	if ( nextTime > m_timeLimitMax )
	{
		nextTime = m_timeLimitMin;
	}
	if ( nextTime < m_timeLimitMin )
	{
		nextTime = m_timeLimitMax;
	}
	m_mediator->OnTimeline_GoToTime(nextTime );
}

void CEdDialogTimeline::GoToNextFrame()
{
	Float nextTime =  m_timeSelectorTimePos + 1.f/30.f;
	if ( nextTime > m_timeLimitMax )
	{
		nextTime = m_timeLimitMin;
	}
	if ( nextTime < m_timeLimitMin )
	{
		nextTime = m_timeLimitMax;
	}
	m_mediator->OnTimeline_GoToTime( nextTime );
}

/*
Creates interpolation event.
*/
void CEdDialogTimeline::OnInterpolationEventCreate( wxCommandEvent& event )
{
	// Before giving the user possibility to create interpolation event, we check if selected
	// items can be used to do this. Here we can be sure that they can. Still - we assert this.
	SCENE_ASSERT( CanCreateInterpolationEvent( m_selectedItems ) );

	CTimelineItemInterpolationEvent* tiInterpolateCameras = InterpolationEventCreate( m_selectedItems );

	// Select new item.
	m_selectedItems.Clear();
	m_selectedItems.PushBack( tiInterpolateCameras );
	SelectionChanged();
}

void CEdDialogTimeline::OnDebugCommentEventCreate( wxCommandEvent& event )
{
	CTimelineItemBlocking* tiElement = FindSceneElement( m_cursorTimePos );
	const Float startPos = ( m_cursorTimePos - tiElement->GetStart() ) / tiElement->GetDuration();
	const String& trackName = m_tracks[ m_selectedTrack ]->m_name;

	CTimelineItemDebugCommentEvent* tiDebugComment = DebugCommentEventCreate( tiElement, startPos, trackName );

	// TODO: do it for all other items also!
	// Select new item.
	m_selectedItems.Clear();
	m_selectedItems.PushBack( tiDebugComment );
	SelectionChanged();
}

void CEdDialogTimeline::OnSetCurrentTime( wxCommandEvent& event )
{
	if ( event.GetId() == ID_SET_CURRENT_TIME_ZERO )
	{
		m_mediator->OnTimeline_GoToTime( 0.f );
	}
	else if ( event.GetId() == ID_SET_CURRENT_TIME_END )
	{
		m_mediator->OnTimeline_GoToTime( m_timeLimitMax );
	}
}

CTimelineItemDebugCommentEvent* CEdDialogTimeline::DebugCommentEventCreate( CTimelineItemBlocking* tiElement, Float startPos, const String& trackName )
{
	CStorySceneEventDebugComment* scDebugCommentEvent = new CStorySceneEventDebugComment( TXT( "Debug comment event" ), tiElement->GetElement(), startPos, trackName );
	m_mediator->OnTimeline_AddEvent( m_section, scDebugCommentEvent, GetSectionVariantId() );

	// Rebuild player to recreate playing plan as some code accesses event instance
	// data before rebuild request (issued by OnTimeline_AddEvent()) is processed.
	m_mediator->OnTimeline_RebuildPlayer();

	CTimelineItemDebugCommentEvent* tiDebugCommentEvent = new CTimelineItemDebugCommentEvent( this, scDebugCommentEvent, tiElement, m_elements );
	AddItem( tiDebugCommentEvent );

	return tiDebugCommentEvent;
}

/*
Creates interpolation event

\param keys Interpolation event keys. There must by at least tow keys and they must satisfy CanCreateInterpolationEvent( keys ) == true.
\return Interpolation event item (item is already added to timeline, corresponding story scene event is already added to scene).
*/
CTimelineItemInterpolationEvent* CEdDialogTimeline::InterpolationEventCreate( const TDynArray< ITimelineItem* >& keys )
{
	// TODO: all keys are CTimelineItemEvents..

	SCENE_ASSERT( CanCreateInterpolationEvent( keys ) );

	// sort keys by time
	TDynArray< ITimelineItem* > sortedKeys = keys;
	::Sort( sortedKeys.Begin(), sortedKeys.End(), TimelineItemStartSorter() );

	// prepare creation args
	CStorySceneEventInterpolation::CreationArgs creationArgs;
	creationArgs.m_eventName = TXT( "interpolation event" );
	for( auto itKey = sortedKeys.Begin(), endKeys = sortedKeys.End(); itKey != endKeys; ++itKey )
	{
		CTimelineItemEvent* tiKeyEvent = static_cast< CTimelineItemEvent* >( *itKey );
		CStorySceneEvent* scKeyEvent = tiKeyEvent->GetEvent();
		creationArgs.m_keyEvents.PushBack( scKeyEvent );
	}

	CTimelineItemEvent* tiFirstKeyEvent = static_cast< CTimelineItemEvent* >( sortedKeys[ 0 ] );

	// create interpolation event of appropriate type
	CStorySceneEventInterpolation* scInterpolationEvent = tiFirstKeyEvent->CreateInterpolationEvent();
	SCENE_ASSERT( scInterpolationEvent ); // if this fires then we shouldn't be here at all
	scInterpolationEvent->Initialize( creationArgs );
	m_mediator->OnTimeline_AddEvent( m_section, scInterpolationEvent, GetSectionVariantId() );

	// Rebuild player to recreate playing plan as some code accesses event instance
	// data before rebuild request (issued by OnTimeline_AddEvent()) is processed.
	m_mediator->OnTimeline_RebuildPlayer();

	// create timeline item for interpolation event
	CTimelineItemInterpolationEvent* tiInterpolationEvent = new CTimelineItemInterpolationEvent( this, scInterpolationEvent, tiFirstKeyEvent->GetElementItem(), m_elements );
	tiInterpolationEvent->Reinitialize();
	AddItem( tiInterpolationEvent );

	return tiInterpolationEvent;
}

/*
TODO: Remove this function when we're sure we don't need it.
*/
void CEdDialogTimeline::OnInterpolationEventDestroy( wxCommandEvent& event )
{
	// Currently this function is not used. We destroy interpolate event just like any other event.
	// Keys are not destroyed. TODO: The only missing part is that we should notify keys that they
	// are no longer attached to interpolate event. This can be don in ITimelineItem::OnDeleted()
	// which is called before item is removed.
	SCENE_ASSERT( false );
}

void CEdDialogTimeline::OnInterpolationEventAttachKey( wxCommandEvent& event )
{
	SCENE_ASSERT( GetSelectedItemCount() >= 2 );

	// Check selection list. We have to have exactly one interpolate event and at least one key event compatible with selected interpolate event.

	ITimelineItem* tiInterpolationEventItem = nullptr;
	TDynArray< ITimelineItem* > tiKeys;
	Bool allOk = GetInterpolateEventAndKeys( m_selectedItems, tiInterpolationEventItem, tiKeys );

	// this function should not be called otherwise
	SCENE_ASSERT( allOk && !tiKeys.Empty() );

	// this is our interpolate event
	DialogTimelineItems::CTimelineItemInterpolationEvent* tiInterpolationEvent = dynamic_cast< DialogTimelineItems::CTimelineItemInterpolationEvent* >( tiInterpolationEventItem );
	SCENE_ASSERT( tiInterpolationEvent );
	CStorySceneEventInterpolation* scInterpolationEvent = tiInterpolationEvent->GetSceneInterpolationEvent();

	for( auto itKey = tiKeys.Begin(), endKeys = tiKeys.End(); itKey != endKeys; ++itKey )
	{
		DialogTimelineItems::CTimelineItemEvent* tiKey = dynamic_cast< DialogTimelineItems::CTimelineItemEvent* >( *itKey );
		SCENE_ASSERT( tiKey );

		// TODO: remove key from old interpolate event.. hm.. here or in InterpolateEventAttachkey()
		InterpolationEventAttachKey( tiInterpolationEvent, tiKey );
	}

	// TODO: really needed?
	m_mediator->OnTimeline_EventChanged( m_section, scInterpolationEvent );
}

/*
Attaches key event to interpolation event.

\param tiInterpolationEvent Interpolation event item.
\param tiKeyEvent Key event item that is to be attached to interpolation event.
*/
void CEdDialogTimeline::InterpolationEventAttachKey( DialogTimelineItems::CTimelineItemInterpolationEvent* tiInterpolationEvent, DialogTimelineItems::CTimelineItemEvent* tiKeyEvent )
{
	CStorySceneEventInterpolation* scInterpolationEvent = tiInterpolationEvent->GetSceneInterpolationEvent();
	CStorySceneEvent* scKeyEvent = tiKeyEvent->GetEvent();

	scInterpolationEvent->AttachKey( scKeyEvent );
	tiInterpolationEvent->Reinitialize();
}

void CEdDialogTimeline::OnInterpolationEventDetachKey( wxCommandEvent& event )
{
	// TODO: don't allow detaching from interpolation event if it has only two keys

	// TODO: allow detaching many keys at once
	SCENE_ASSERT( GetSelectedItemCount() == 1 );

	// get key event
	DialogTimelineItems::CTimelineItemEvent* tiKeyEvent = dynamic_cast< DialogTimelineItems::CTimelineItemEvent* >( m_selectedItems[ 0 ] );
	SCENE_ASSERT( tiKeyEvent );
	CStorySceneEvent* scKeyEvent = tiKeyEvent->GetEvent();
	SCENE_ASSERT( scKeyEvent->IsInterpolationEventKey() );

	// get interpolation event
	CGUID interpolationEventGuid = scKeyEvent->GetInterpolationEventGUID();
	CTimelineItemInterpolationEvent* tiInterpolationEvent = static_cast< CTimelineItemInterpolationEvent* >( FindItemEvent( interpolationEventGuid ) );
	SCENE_ASSERT( tiInterpolationEvent );
	CStorySceneEventInterpolation* scInterpolationEvent = tiInterpolationEvent->GetSceneInterpolationEvent();

	InterpolationEventDetachKey( tiInterpolationEvent, tiKeyEvent );

	// TODO: really needed?
	m_mediator->OnTimeline_EventChanged( m_section, scInterpolationEvent );
}

/*
Detaches key event from interpolation event.

\param tiInterpolationEvent Interpolation event from which to detach key event. Must have more
that two keys (otherwise detaching key would leave interpolation event in invalid state).
\param tiKeyEvent Key event to detach.
*/
void CEdDialogTimeline::InterpolationEventDetachKey( DialogTimelineItems::CTimelineItemInterpolationEvent* tiInterpolationEvent, DialogTimelineItems::CTimelineItemEvent* tiKeyEvent )
{
	// We can't detach key from interpolation event if it has two keys only.
	SCENE_ASSERT( tiInterpolationEvent->GetNumKeys() > 2 );

	CStorySceneEventInterpolation* scInterpolationEvent = tiInterpolationEvent->GetSceneInterpolationEvent();
	CStorySceneEvent* scKeyEvent = tiKeyEvent->GetEvent();

	scInterpolationEvent->DetachKey( scKeyEvent );
	tiInterpolationEvent->Reinitialize();
}

void CEdDialogTimeline::OnInterpolationEventPlot( wxCommandEvent& event )
{
	SCENE_ASSERT( IsOneSelected() );
	SCENE_ASSERT( IsInterpolationEvent( m_selectedItems[ 0 ] ) );

	// TODO: get proper channel names
	// channel names
	TDynArray< String > tracks;
	tracks.PushBack( TXT("a") );
	tracks.PushBack( TXT("b") );
	tracks.PushBack( TXT("c") );
	tracks.PushBack( TXT("d") );
	tracks.PushBack( TXT("e") );
	tracks.PushBack( TXT("f") );
	tracks.PushBack( TXT("g") );
	tracks.PushBack( TXT("h") );
	tracks.PushBack( TXT("i") );
	tracks.PushBack( TXT("j") );
	tracks.PushBack( TXT("k") );
	tracks.PushBack( TXT("l") );
	tracks.PushBack( TXT("m") );

	const CTimelineItemInterpolationEvent* tiInterpolationEvent = dynamic_cast< const CTimelineItemInterpolationEvent* >( m_selectedItems[ 0 ] );
	const CStorySceneEventInterpolation* scInterpolationEvent = static_cast< const CStorySceneEventInterpolation* >( tiInterpolationEvent->GetEvent() );

	m_mediator->OnTimeline_InterpolationEventPlot( scInterpolationEvent, tracks );
}

/*
Splits animation event at time cursor.
*/
void CEdDialogTimeline::OnSplitAnimationEvent( wxCommandEvent& event )
{
	if( !IsTimelineEditingEnabled() )
	{
		GFeedback->ShowMsg( TXT( "Scene Editor" ), TXT( "Razor - operation not allowed in unapproved section." ) );
		return;
	}

	SCENE_ASSERT( IsOneSelected() );

	SCENE_ASSERT( dynamic_cast< CTimelineItemAnimClip* >( m_selectedItems[ 0 ] ) ); // otherwise OnSplitAnimationEvent() should not be called
	CTimelineItemAnimClip* tiAnimClip = dynamic_cast< CTimelineItemAnimClip* >( m_selectedItems[ 0 ] );
	if ( !tiAnimClip )
	{
		return;
	}

	// create second animation event
	CStorySceneEventAnimClip* scAnimClip2 = static_cast< CStorySceneEventAnimClip* >( tiAnimClip->GetEvent()->Clone() );
	m_mediator->OnTimeline_AddEvent( m_section, scAnimClip2, GetSectionVariantId() );

	// Rebuild player to recreate playing plan as some code accesses event instance
	// data before rebuild request (issued by OnTimeline_AddEvent()) is processed.
	m_mediator->OnTimeline_RebuildPlayer();

	// add second animation event to timeline
	CTimelineItemAnimClip* tiAnimClip2 = static_cast< CTimelineItemAnimClip* >( CreateTimelineItemEvent( scAnimClip2, tiAnimClip->GetElementItem() ) );
	AddItem( tiAnimClip2 );

	// TODO: CTimelineItemAnimClip should have SetClipFront() and SetClipEnd() functions that we should use here
	// instead of SetRightEdge() and SetLeftEdge() that should be used from UI only. Of course, SetLeftEdge() and
	// SetRightEdge() will internally use SetClipFront() and SetClipEnd().
	// adjust parameters of both animation events
	TimelineKeyModifiers keyModifiers = { false, false, false };
	tiAnimClip->SetRightEdge( m_timeSelectorTimePos, keyModifiers );
	tiAnimClip2->SetLeftEdge( m_timeSelectorTimePos, keyModifiers );
}

#ifdef DEBUG_SCENES_2
#pragma optimize("",on)
#endif
