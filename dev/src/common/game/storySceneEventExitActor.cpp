
#include "build.h"

#include "../engine/behaviorGraphStack.h"
#include "../engine/behaviorGraphAnimationManualSlot.h"
#include "storySceneEvent.h"
#include "storySceneEventExitActor.h"
#include "storyScenePlayer.h"
#include "sceneLog.h"
#include "storySceneDirector.h"

#ifdef DEBUG_SCENES
#pragma optimize("",off)
#endif

IMPLEMENT_ENGINE_CLASS( CStorySceneEventExitActor );

CStorySceneEventExitActor::CStorySceneEventExitActor()
	 : CStorySceneEventAnimClip()
{
	m_blendOut = 0.0f;
}

CStorySceneEventExitActor::CStorySceneEventExitActor( const String& eventName,  CStorySceneElement* sceneElement, Float startTime, const CName& actor, const CName& behEvent, const String& trackName )
   : CStorySceneEventAnimClip( eventName, sceneElement, startTime, actor, trackName )

{
	m_blendOut = 0.0f;
}

CStorySceneEventExitActor* CStorySceneEventExitActor::Clone() const
{
	return new CStorySceneEventExitActor( *this );
}

void CStorySceneEventExitActor::OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	TBaseClass::OnStart( data, scenePlayer, collector, timeInfo );

	// TODO lookaty
	CActor* actor = scenePlayer->GetMappedActor( m_actor );
	if( actor == NULL )
	{
		SCENE_WARN( TXT( "Cannot find actor '%ls' for story event '%ls'" ), m_actor.AsString().AsChar(), m_eventName.AsChar() );
		return;
	}

	actor->DisableDialogsLookAts( 1.f );
}

void CStorySceneEventExitActor::OnEnd( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	TBaseClass::OnEnd( data, scenePlayer, collector, timeInfo );

	if ( m_actor )
	{
		StorySceneEventsCollector::ActorVisibility evt( this, m_actor );
		evt.m_eventTimeAbs = timeInfo.m_timeAbs;
		evt.m_eventTimeLocal = timeInfo.m_timeLocal;

		evt.m_showHide = false;

		collector.AddEvent( evt );
	}
}

void CStorySceneEventExitActor::OnAddExtraDataToEvent( StorySceneEventsCollector::BodyAnimation& event ) const
{
	//event.m_useMotion = true;
	event.m_useFakeMotion = true;
}

#ifndef NO_EDITOR
void CStorySceneEventExitActor::OnPreviewPropertyChanged( const CStoryScenePlayer* previewPlayer, const CName& propertyName )
{
	TBaseClass::OnPreviewPropertyChanged( previewPlayer, propertyName );

	m_eventName = m_behEvent.AsString();
}
#endif

#ifdef DEBUG_SCENES
#pragma optimize("",on)
#endif
