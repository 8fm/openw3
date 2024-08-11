/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "storySceneEvent.h"
#include "storySceneEventDuration.h"
#include "../core/instanceDataLayoutCompiler.h"

#ifdef DEBUG_SCENES
#pragma optimize("",off)
#endif

IMPLEMENT_ENGINE_CLASS( CStorySceneEventDuration );

CStorySceneEventDuration::CStorySceneEventDuration()
   : CStorySceneEvent()
   , m_duration( 0.1f )
{

}

CStorySceneEventDuration::CStorySceneEventDuration( const String& eventName, 
		CStorySceneElement* sceneElement, Float startTime, Float duration, const String& trackName )
	: CStorySceneEvent( eventName, sceneElement, startTime, trackName )
	, m_duration( duration )
{

}

/*
Cctor.

Compiler generated cctor would also copy instance vars - we don't want that.
*/
CStorySceneEventDuration::CStorySceneEventDuration( const CStorySceneEventDuration& other )
: CStorySceneEvent( other )
, m_duration( other.m_duration )
{}

void CStorySceneEventDuration::DoBakeScaleImpl( Float scalingFactor )
{
	CStorySceneEvent::DoBakeScaleImpl( scalingFactor );
	m_duration *= scalingFactor;
}

/*
Note it doesn't include scaling.
*/
void  CStorySceneEventDuration::SetDuration( Float duration )
{
	m_duration = duration;
}

void CStorySceneEventDuration::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_running;
}

void CStorySceneEventDuration::OnInitInstance( CStorySceneInstanceBuffer& data ) const
{
	TBaseClass::OnInitInstance( data );

	data[ i_running ] = false;
}

void CStorySceneEventDuration::OnReleaseInstance( CStorySceneInstanceBuffer& data ) const
{
	TBaseClass::OnReleaseInstance( data );
}

void CStorySceneEventDuration::OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	TBaseClass::OnStart( data, scenePlayer, collector, timeInfo );

	data[ i_running ] = true;
}

void CStorySceneEventDuration::OnProcess( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	//SCENE_ASSERT( data[ i_running ] );

	TBaseClass::OnProcess( data, scenePlayer, collector, timeInfo );
}

void CStorySceneEventDuration::OnEnd( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	TBaseClass::OnEnd( data, scenePlayer, collector, timeInfo );

	data[ i_running ] = false;
}

#ifdef DEBUG_SCENES
#pragma optimize("",on)
#endif
