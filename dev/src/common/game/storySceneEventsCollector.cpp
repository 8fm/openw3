
#include "build.h"
#include "storySceneEventsCollector.h"
#include "storySceneDebugger.h"

#ifdef DEBUG_SCENES_2
#pragma optimize("",off)
#endif

CStorySceneEventsCollector::CStorySceneEventsCollector()
	: m_empty( true )
{
	Clear();
}

void CStorySceneEventsCollector::Clear()
{
	m_bodyAnimations.Clear();
	m_mimicsAnimations.Clear();
	m_mimicsPoses.Clear();
	m_bodyPoses.Clear();
	m_actorChangeState.Clear();
	m_propVisibility.Clear();

	m_actorVisibility.Clear();
	m_actorItems.Clear();
	m_actorLookAts.Clear();
	m_actorLookAtTicks.Clear();
	m_actorAppearances.Clear();
	m_actorHiresShadows.Clear();
	m_actorMimicLod.Clear();

	m_actorItemVisibility.Clear();

	m_actorPlacement.Clear();
	m_actorMotion.Clear();

	m_actorChangeGameState.Clear();

	m_propPlacements.Clear();
	m_lightProperties.Clear();

	m_cameraShot.Reset();
	m_cameraBlend.Reset();
	m_cameraStartBlendToGameplay.Reset();

	m_playDialogLineEvents.Clear();
	m_attachPropToBoneEvents.Clear();
	m_playEffectEvents.Clear();

	m_fade.Reset();
	m_syncItemInfo.Reset();

	m_displayDebugCommentEvents.Clear();
	m_hideDebugCommentEvents.Clear();
	m_doorChangeStates.Clear();

	m_empty = true;
}

Bool CStorySceneEventsCollector::Empty() const
{
	return m_empty;
}

//////////////////////////////////////////////////////////////////////////

void CStorySceneEventsCollector::AddEvent( const StorySceneEventsCollector::BodyAnimation& event )
{
	SCENE_ASSERT( event.m_eventTimeAbs >= 0.f );
	SCENE_ASSERT( event.m_eventTimeLocal >= 0.f );

	m_empty = false;
	AddOnlyOneById( m_bodyAnimations, event );
}

void CStorySceneEventsCollector::AddEvent( const StorySceneEventsCollector::VideoOverlay& event )
{
	m_empty = false;
	AddOnlyOneById( m_videoOverlays, event );
}

void CStorySceneEventsCollector::AddEvent(const StorySceneEventsCollector::SyncItemInfo& event)
{
	m_empty = false;
	m_syncItemInfo.Set( event );
}

void CStorySceneEventsCollector::RemoveEvent( const StorySceneEventsCollector::BodyAnimation& event )
{
	RemoveById( m_bodyAnimations, event );
}

void CStorySceneEventsCollector::AddEvent( const StorySceneEventsCollector::MimicsAnimation& event )
{
	SCENE_ASSERT( event.m_eventTimeAbs >= 0.f );
	SCENE_ASSERT( event.m_eventTimeLocal >= 0.f );

	m_empty = false;
	AddOnlyOneById( m_mimicsAnimations, event );
}

void CStorySceneEventsCollector::RemoveEvent( const StorySceneEventsCollector::MimicsAnimation& event )
{
	RemoveById( m_mimicsAnimations, event );
}

void CStorySceneEventsCollector::AddEvent( const StorySceneEventsCollector::MimicPose& event )
{
	m_empty = false;
	AddOnlyOneByActor( m_mimicsPoses, event );
}

void CStorySceneEventsCollector::AddEvent( const StorySceneEventsCollector::BodyPose& event )
{
	SCENE_ASSERT( event.m_eventTimeAbs >= 0.f );
	SCENE_ASSERT( event.m_eventTimeLocal >= 0.f );

	m_empty = false;
	AddOnlyOneById( m_bodyPoses, event );
}

void CStorySceneEventsCollector::RemoveEvent( const StorySceneEventsCollector::MimicPose& event )
{
	RemoveById( m_mimicsPoses, event );
}

void CStorySceneEventsCollector::RemoveEvent( const StorySceneEventsCollector::BodyPose& event )
{
	RemoveById( m_bodyPoses, event );
}

void CStorySceneEventsCollector::AddEvent( const StorySceneEventsCollector::ActorPlacement& event )
{
	SCENE_ASSERT( event.m_eventTimeAbs >= 0.f );
	SCENE_ASSERT( event.m_eventTimeLocal >= 0.f );

	m_empty = false;
	AddOnlyOneByActor( m_actorPlacement, event );
}

void CStorySceneEventsCollector::AddEvent( const StorySceneEventsCollector::ActorMotion& event )
{
	SCENE_ASSERT( event.m_eventTimeAbs >= 0.f );
	SCENE_ASSERT( event.m_eventTimeLocal >= 0.f );

	m_empty = false;
	AddOnlyOneById( m_actorMotion, event );
}

//////////////////////////////////////////////////////////////////////////

void CStorySceneEventsCollector::AddEvent( const StorySceneEventsCollector::CameraShot& event )
{
	SCENE_ASSERT( event.m_eventTimeAbs >= 0.f );
	SCENE_ASSERT( event.m_eventTimeLocal >= 0.f );

	m_empty = false;
	m_cameraShot.Set( event );
}

void CStorySceneEventsCollector::AddEvent( const StorySceneEventsCollector::CameraBlend& event )
{
	SCENE_ASSERT( event.m_eventTimeAbs >= 0.f );
	SCENE_ASSERT( event.m_eventTimeLocal >= 0.f );

	m_empty = false;
	m_cameraBlend.Set( event );
}

void CStorySceneEventsCollector::AddEvent( const StorySceneEventsCollector::CameraAnimation& event )
{
	SCENE_ASSERT( event.m_eventTimeAbs >= 0.f );
	SCENE_ASSERT( event.m_eventTimeLocal >= 0.f );

	m_empty = false;
	AddOnlyOneById( m_cameraAnimations, event );
}

void CStorySceneEventsCollector::RemoveEvent( const StorySceneEventsCollector::CameraAnimation& event )
{
	RemoveById( m_cameraAnimations, event );
}

void CStorySceneEventsCollector::AddEvent( const StorySceneEventsCollector::CameraStartBlendToGameplay& event )
{
	SCENE_ASSERT( event.m_eventTimeAbs >= 0.f );
	SCENE_ASSERT( event.m_eventTimeLocal >= 0.f );

	m_empty = false;
	m_cameraStartBlendToGameplay.Set( event );
}

void CStorySceneEventsCollector::AddEvent( const StorySceneEventsCollector::Fade& event )
{
	SCENE_ASSERT( event.m_eventTimeAbs >= 0.f );
	SCENE_ASSERT( event.m_eventTimeLocal >= 0.f );

	m_empty = false;
	m_fade.Set( event );
}

//////////////////////////////////////////////////////////////////////////

void CStorySceneEventsCollector::AddEvent( const StorySceneEventsCollector::ActorChangeGameState& event )
{
	SCENE_ASSERT( event.m_eventTimeAbs >= 0.f );
	SCENE_ASSERT( event.m_eventTimeLocal >= 0.f );

	AddOnlyOneByActor( m_actorChangeGameState, event );
}

//////////////////////////////////////////////////////////////////////////

void CStorySceneEventsCollector::AddEvent( const StorySceneEventsCollector::ActorChangeState& event )
{
	SCENE_ASSERT( event.m_eventTimeAbs >= 0.f );
	SCENE_ASSERT( event.m_eventTimeLocal >= 0.f );

	AddOnlyOneById( m_actorChangeState, event );
}

void CStorySceneEventsCollector::AddEvent( const StorySceneEventsCollector::ActorVisibility& event )
{
	SCENE_ASSERT( event.m_eventTimeAbs >= 0.f );
	SCENE_ASSERT( event.m_eventTimeLocal >= 0.f );

	AddOnlyOneByActor( m_actorVisibility, event );
}

void CStorySceneEventsCollector::AddEvent( const StorySceneEventsCollector::ActorApplyAppearance& event )
{
	SCENE_ASSERT( event.m_eventTimeAbs >= 0.f );
	SCENE_ASSERT( event.m_eventTimeLocal >= 0.f );

	AddOnlyOneByActor( m_actorAppearances, event );
}

void CStorySceneEventsCollector::AddEvent( const StorySceneEventsCollector::ActorUseHiresShadows& event )
{
	SCENE_ASSERT( event.m_eventTimeAbs >= 0.f );
	SCENE_ASSERT( event.m_eventTimeLocal >= 0.f );

	AddOnlyOneByActor( m_actorHiresShadows, event );
}

void CStorySceneEventsCollector::AddEvent( const StorySceneEventsCollector::ActorMimicLod& event )
{
	SCENE_ASSERT( event.m_eventTimeAbs >= 0.f );
	SCENE_ASSERT( event.m_eventTimeLocal >= 0.f );

	AddOnlyOneByActor( m_actorMimicLod, event );
}

void CStorySceneEventsCollector::AddEvent( const StorySceneEventsCollector::ActorLodOverride& event )
{
	SCENE_ASSERT( event.m_eventTimeAbs >= 0.f );
	SCENE_ASSERT( event.m_eventTimeLocal >= 0.f );

	AddOnlyOneByActor( m_actorLodOverrides, event );
}

void CStorySceneEventsCollector::AddEvent( const StorySceneEventsCollector::ActorItem& event )
{
	SCENE_ASSERT( event.m_eventTimeAbs >= 0.f );
	SCENE_ASSERT( event.m_eventTimeLocal >= 0.f );

	AddOnlyOneByActor( m_actorItems, event );
}

void CStorySceneEventsCollector::AddEvent( const StorySceneEventsCollector::ActorItemVisibility& event )
{
	SCENE_ASSERT( event.m_eventTimeAbs >= 0.f );
	SCENE_ASSERT( event.m_eventTimeLocal >= 0.f );

	const Uint32 size = m_actorItemVisibility.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		const StorySceneEventsCollector::ActorItemVisibility& evtIt = m_actorItemVisibility[ i ];

		if ( evtIt.m_actorId == event.m_actorId && evtIt.m_item == event.m_item )
		{
			SCENE_ASSERT( event.m_eventTimeAbs >= evtIt.m_eventTimeAbs );
			m_actorItemVisibility.RemoveAtFast( i );
			break;
		}
	}
	InsertEventByTime( m_actorItemVisibility, event );
}

void CStorySceneEventsCollector::AddEvent( const StorySceneEventsCollector::TimeMultiplier& event )
{
	SCENE_ASSERT( event.m_eventTimeAbs >= 0.f );
	SCENE_ASSERT( event.m_eventTimeLocal >= 0.f );

	AddOnlyOneById( m_timeMultiplier, event );
}

void CStorySceneEventsCollector::AddEvent( const StorySceneEventsCollector::ActorMorph& event )
{
	SCENE_ASSERT( event.m_eventTimeAbs >= 0.f );
	SCENE_ASSERT( event.m_eventTimeLocal >= 0.f );

	AddOnlyOneById( m_actorMorph, event );
}

void CStorySceneEventsCollector::AddEvent( const StorySceneEventsCollector::ActorDisablePhysicsCloth& event )
{
	SCENE_ASSERT( event.m_eventTimeAbs >= 0.f );
	SCENE_ASSERT( event.m_eventTimeLocal >= 0.f );

	AddOnlyOneByActor( m_actorDisablePhysicsCloth, event );
}

void CStorySceneEventsCollector::AddEvent( const StorySceneEventsCollector::ActorDisableDangle& event )
{
	SCENE_ASSERT( event.m_eventTimeAbs >= 0.f );
	SCENE_ASSERT( event.m_eventTimeLocal >= 0.f );

	AddOnlyOneByActor( m_actorDisableDangle, event );
}

void CStorySceneEventsCollector::AddEvent( const StorySceneEventsCollector::ActorDanglesShake& event )
{
	SCENE_ASSERT( event.m_eventTimeAbs >= 0.f );
	SCENE_ASSERT( event.m_eventTimeLocal >= 0.f );

	AddOnlyOneByActor( m_actorDanglesShakes, event );
}

void CStorySceneEventsCollector::AddEvent( const StorySceneEventsCollector::ActorResetClothAndDangles& event )
{
	SCENE_ASSERT( event.m_eventTimeAbs >= 0.f );
	SCENE_ASSERT( event.m_eventTimeLocal >= 0.f );

	MergeOnlyOneByActor( m_actorResetClothAndDangles, event );
}

void CStorySceneEventsCollector::AddEvent( const StorySceneEventsCollector::PlayDialogLine& event )
{
	SCENE_ASSERT( event.m_eventTimeAbs >= 0.f );
	SCENE_ASSERT( event.m_eventTimeLocal >= 0.f );

	AddOnlyOneByActor( m_playDialogLineEvents, event );
}

void CStorySceneEventsCollector::AddEvent( const StorySceneEventsCollector::ActorLookAt& event )
{
	SCENE_ASSERT( event.m_eventTimeAbs >= 0.f );
	SCENE_ASSERT( event.m_eventTimeLocal >= 0.f );

	AddOnlyOneById( m_actorLookAts, event );
}

void CStorySceneEventsCollector::AddEvent( const StorySceneEventsCollector::ActorLookAtTick& event )
{
	SCENE_ASSERT( event.m_eventTimeAbs >= 0.f );
	SCENE_ASSERT( event.m_eventTimeLocal >= 0.f );

	AddOnlyOneByActor( m_actorLookAtTicks, event );
}

void CStorySceneEventsCollector::AddEvent( const StorySceneEventsCollector::ActorGameplayLookAt& event )
{
	SCENE_ASSERT( event.m_eventTimeAbs >= 0.f );
	SCENE_ASSERT( event.m_eventTimeLocal >= 0.f );

	AddOnlyOneByActor( m_actorLookAtGameplay, event );
}

void CStorySceneEventsCollector::AddEvent( const StorySceneEventsCollector::ActorPrepareToTalk& event )
{
	SCENE_ASSERT( event.m_eventTimeAbs >= 0.f );
	SCENE_ASSERT( event.m_eventTimeLocal >= 0.f );

	const Uint32 num = m_actorPrepareToTalk.Size();
	for ( Uint32 i=0; i<num; ++i )
	{
		SCENE_ASSERT( m_actorPrepareToTalk[i].m_actorId != event.m_actorId );
	}

	AddOnlyOneByActor( m_actorPrepareToTalk, event );
}

void CStorySceneEventsCollector::AddEvent( const StorySceneEventsCollector::ActorFinishTalk& event )
{
	SCENE_ASSERT( event.m_eventTimeAbs >= 0.f );
	SCENE_ASSERT( event.m_eventTimeLocal >= 0.f );

	const Uint32 num = m_actorFinishTalk.Size();
	for ( Uint32 i=0; i<num; ++i )
	{
		SCENE_ASSERT( m_actorFinishTalk[i].m_actorId != event.m_actorId );
	}

	AddOnlyOneByActor( m_actorFinishTalk, event );
}

//////////////////////////////////////////////////////////////////////////

void CStorySceneEventsCollector::AddEvent( const StorySceneEventsCollector::DisplayDebugComment& event )
{
	SCENE_ASSERT( event.m_eventTimeAbs >= 0.f );
	SCENE_ASSERT( event.m_eventTimeLocal >= 0.f );

	AddOnlyOneById( m_displayDebugCommentEvents, event );
}

void CStorySceneEventsCollector::AddEvent( const StorySceneEventsCollector::HideDebugComment& event )
{
	SCENE_ASSERT( event.m_eventTimeAbs >= 0.f );
	SCENE_ASSERT( event.m_eventTimeLocal >= 0.f );

	AddOnlyOneById( m_hideDebugCommentEvents, event );
}

//////////////////////////////////////////////////////////////////////////

void CStorySceneEventsCollector::AddEvent( const StorySceneEventsCollector::PropPlacement& event )
{
	SCENE_ASSERT( event.m_eventTimeAbs >= 0.f );
	SCENE_ASSERT( event.m_eventTimeLocal >= 0.f );

	AddOnlyOneByActor( m_propPlacements, event );
}

void CStorySceneEventsCollector::AddEvent( const StorySceneEventsCollector::AttachPropToBone& event )
{
	SCENE_ASSERT( event.m_eventTimeAbs >= 0.f );
	SCENE_ASSERT( event.m_eventTimeLocal >= 0.f );

	AddOnlyOneByActor( m_attachPropToBoneEvents, event );
}

void CStorySceneEventsCollector::AddEvent( const StorySceneEventsCollector::PropVisibility& event )
{
	SCENE_ASSERT( event.m_eventTimeAbs >= 0.f );
	SCENE_ASSERT( event.m_eventTimeLocal >= 0.f );

	AddOnlyOneByActor( m_propVisibility, event );
}

void CStorySceneEventsCollector::AddEvent( const StorySceneEventsCollector::LightProperty& event )
{
	SCENE_ASSERT( event.m_eventTimeAbs >= 0.f );
	SCENE_ASSERT( event.m_eventTimeLocal >= 0.f );

	AddOnlyOneById( m_lightProperties, event );
}

void CStorySceneEventsCollector::AddEvent( const StorySceneEventsCollector::CameraLightProp& event )
{
	AddOnlyOneById( m_cameralightProperties, event );
}

void CStorySceneEventsCollector::AddEvent( const StorySceneEventsCollector::CameraPrefetch& event )
{
	AddOnlyOneById( m_cameraPrefetches, event );
}


void CStorySceneEventsCollector::AddEvent( const StorySceneEventsCollector::DoorChangeState& event )
{
	AddOnlyOneById( m_doorChangeStates, event );
}

void CStorySceneEventsCollector::AddEvent( const StorySceneEventsCollector::EnvChange& event )
{
	AddOnlyOneById( m_envChanges, event );
}

void CStorySceneEventsCollector::AddEvent( const StorySceneEventsCollector::PlayEffect& event )
{
	SCENE_ASSERT( event.m_eventTimeAbs >= 0.f );
	SCENE_ASSERT( event.m_eventTimeLocal >= 0.f );

	m_empty = false;
	const Uint32 size = m_playEffectEvents.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		const StorySceneEventsCollector::PlayEffect& evtIt = m_playEffectEvents[ i ];
		if ( evtIt.m_actorId == event.m_actorId && evtIt.m_effectName == event.m_effectName )
		{
			SCENE_ASSERT( event.m_eventTimeAbs >= evtIt.m_eventTimeAbs );
			m_playEffectEvents.RemoveAtFast( i );
			break;
		}
	}
	InsertEventByTime( m_playEffectEvents, event );
}

//////////////////////////////////////////////////////////////////////////

#ifdef DEBUG_SCENES_2
#pragma optimize("",on)
#endif
