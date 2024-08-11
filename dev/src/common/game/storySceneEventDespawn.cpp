/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "storySceneEventDespawn.h"
#include "storyScenePlayer.h"
#include "storySceneEventsCollector_events.h"
#include "../../common/engine/weatherManager.h"

#ifdef DEBUG_SCENES_2
#pragma optimize("",off)
#endif

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CStorySceneEventDespawn );

CStorySceneEventDespawn::CStorySceneEventDespawn()
	: CStorySceneEvent()
	, m_actor()
{

}

CStorySceneEventDespawn::CStorySceneEventDespawn( const String& eventName, CStorySceneElement* sceneElement,
		Float startTime, const CName& actor, const String& trackName)
	: CStorySceneEvent( eventName, sceneElement, startTime, trackName )
	, m_actor( actor )
{

}

CStorySceneEventDespawn* CStorySceneEventDespawn::Clone() const
{
	return new CStorySceneEventDespawn( *this );
}

void CStorySceneEventDespawn::OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer , CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	// DIALOG_TOMSIN_TODO - tu nie moze byc destroy, tylko hideingame a destroye na koncu sceny!
	/*CActor* actor = scenePlayer->GetMappedActor( m_actor, false );
	if( actor == NULL )
	{
		SCENE_WARN( TXT( "Cannot find actor '%ls' for story event '%ls'" ), m_actor.AsString().AsChar(), m_eventName.AsChar() );
		return;
	}

	scenePlayer->RemoveActorFromScene( m_actor );

	actor->Destroy();*/
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CStorySceneEventVisibility );

CStorySceneEventVisibility::CStorySceneEventVisibility()
	: CStorySceneEvent()
	, m_actor()
	, m_showHideFlag( true )
{

}

CStorySceneEventVisibility::CStorySceneEventVisibility( const String& eventName, CStorySceneElement* sceneElement,
		Float startTime, const CName& actor, const String& trackName )
	: CStorySceneEvent( eventName, sceneElement, startTime, trackName )
	, m_actor( actor )
	, m_showHideFlag( true )
{

}

CStorySceneEventVisibility* CStorySceneEventVisibility::Clone() const
{
	return new CStorySceneEventVisibility( *this );
}

void CStorySceneEventVisibility::OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer , CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	if ( m_actor )
	{
		StorySceneEventsCollector::ActorVisibility evt( this, m_actor );
		evt.m_eventTimeLocal = timeInfo.m_timeLocal;
		evt.m_eventTimeAbs = timeInfo.m_timeAbs;
		evt.m_showHide = m_showHideFlag;
		collector.AddEvent( evt );
	}
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CStorySceneEventApplyAppearance );

CStorySceneEventApplyAppearance::CStorySceneEventApplyAppearance()
{

}

CStorySceneEventApplyAppearance::CStorySceneEventApplyAppearance( const String& eventName, CStorySceneElement* sceneElement, Float startTime, const CName& actor, const String& trackName )
	: CStorySceneEvent( eventName, sceneElement, startTime, trackName )
	, m_actor( actor )
{

}

void CStorySceneEventApplyAppearance::OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer , CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	if ( m_actor && m_appearance )
	{
		StorySceneEventsCollector::ActorApplyAppearance evt( this, m_actor );
		evt.m_eventTimeLocal = timeInfo.m_timeLocal;
		evt.m_eventTimeAbs = timeInfo.m_timeAbs;
		evt.m_appearance = m_appearance;
		collector.AddEvent( evt );
	}
}

CStorySceneEventApplyAppearance* CStorySceneEventApplyAppearance::Clone() const 
{
	return new CStorySceneEventApplyAppearance( *this );
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CStorySceneEventPropVisibility );

void CStorySceneEventPropVisibility::OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const 
{
	if ( m_propID )
	{
		StorySceneEventsCollector::PropVisibility evt( this, m_propID );
		evt.m_eventTimeLocal = timeInfo.m_timeLocal;
		evt.m_eventTimeAbs = timeInfo.m_timeAbs;

		evt.m_showHide = m_showHideFlag;
		collector.AddEvent( evt );
	}
}

CStorySceneEventPropVisibility::CStorySceneEventPropVisibility( const String& eventName, CStorySceneElement* sceneElement, Float startTime, const CName& id, const String& trackName )
	: CStorySceneEvent( eventName, sceneElement, startTime, trackName )
	, m_showHideFlag( true )
{
	if( CStorySceneSection* section = sceneElement->GetSection() )
	{
		if( CStoryScene* scene =  section->GetScene() )
		{
			if( scene->GetPropDefinition( id ) )
			{
				m_propID = id;
			}
		}
	}	
}

CStorySceneEventPropVisibility::CStorySceneEventPropVisibility() 
	: CStorySceneEvent() 
	, m_showHideFlag( true )
{

}

CStorySceneEventPropVisibility* CStorySceneEventPropVisibility::Clone() const
{
	return new CStorySceneEventPropVisibility( *this );
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CStorySceneEventWeatherChange );

CStorySceneEventWeatherChange::CStorySceneEventWeatherChange()
	: CStorySceneEvent()
	, m_blendTime( 70.f )
{

}

CStorySceneEventWeatherChange::CStorySceneEventWeatherChange( const String& eventName, CStorySceneElement* sceneElement, Float startTime, const CName& actor, const String& trackName )
	: CStorySceneEvent( eventName, sceneElement, startTime, trackName )
	, m_blendTime( 70.f )
{

}

CStorySceneEventWeatherChange* CStorySceneEventWeatherChange::Clone() const
{
	return new CStorySceneEventWeatherChange( *this );
}

void CStorySceneEventWeatherChange::OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer , CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	CWorld* world = scenePlayer->GetLayer()->GetWorld();
	CWeatherManager* weatherManager = world && world->GetEnvironmentManager() ? world->GetEnvironmentManager()->GetWeatherManager() : nullptr;
	if ( weatherManager )
	{
		const SWeatherCondition* weatherCondition = weatherManager->FindWeatherCondition( m_weatherName, nullptr );
		if ( weatherCondition )
		{
			weatherManager->SetBlendTime( m_blendTime );
			weatherManager->RequestWeatherChangeTo( m_weatherName );
		}
	}
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CStorySceneEventMimicLod );

CStorySceneEventMimicLod::CStorySceneEventMimicLod()
{

}

CStorySceneEventMimicLod::CStorySceneEventMimicLod( const String& eventName, CStorySceneElement* sceneElement, Float startTime, const CName& actor, const String& trackName )
	: CStorySceneEvent( eventName, sceneElement, startTime, trackName )
	, m_actor( actor )
{

}

void CStorySceneEventMimicLod::OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer , CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	if ( m_actor )
	{
		StorySceneEventsCollector::ActorMimicLod evt( this, m_actor );
		evt.m_eventTimeLocal = timeInfo.m_timeLocal;
		evt.m_eventTimeAbs = timeInfo.m_timeAbs;
		evt.m_setMimicOn = m_setMimicOn;
		collector.AddEvent( evt );
	}
}

CStorySceneEventMimicLod* CStorySceneEventMimicLod::Clone() const 
{
	return new CStorySceneEventMimicLod( *this );
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CStorySceneEventUseHiresShadows );

CStorySceneEventUseHiresShadows::CStorySceneEventUseHiresShadows()
{

}

CStorySceneEventUseHiresShadows::CStorySceneEventUseHiresShadows( const String& eventName, CStorySceneElement* sceneElement, Float startTime, const CName& actor, const String& trackName )
	: CStorySceneEvent( eventName, sceneElement, startTime, trackName )
	, m_actor( actor )
{

}

void CStorySceneEventUseHiresShadows::OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer , CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	if ( m_actor )
	{
		StorySceneEventsCollector::ActorUseHiresShadows evt( this, m_actor );
		evt.m_eventTimeLocal = timeInfo.m_timeLocal;
		evt.m_eventTimeAbs = timeInfo.m_timeAbs;
		evt.m_useHiresShadows = m_useHiresShadows;
		collector.AddEvent( evt );
	}
}

CStorySceneEventUseHiresShadows* CStorySceneEventUseHiresShadows::Clone() const 
{
	return new CStorySceneEventUseHiresShadows( *this );
}

//////////////////////////////////////////////////////////////////////////

#ifdef DEBUG_SCENES_2
#pragma optimize("",on)
#endif
