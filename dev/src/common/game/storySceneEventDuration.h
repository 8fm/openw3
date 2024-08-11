/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "storySceneEvent.h"

class CStorySceneEventDuration : public CStorySceneEvent
{
	DECLARE_SCENE_EVENT_CLASS( CStorySceneEventDuration, CStorySceneEvent )

protected:
	Float					m_duration;						// Duration, in seconds.

protected:
	TInstanceVar< Bool >	i_running;

public:
	CStorySceneEventDuration();
	CStorySceneEventDuration( const String& eventName, CStorySceneElement* sceneElement, Float startTime, Float duration, const String& trackName );
	CStorySceneEventDuration( const CStorySceneEventDuration& other );

public:
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler ) override;
	virtual void OnInitInstance( CStorySceneInstanceBuffer& data ) const override;
	virtual void OnReleaseInstance( CStorySceneInstanceBuffer& data ) const override;

	virtual void OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;
	virtual void OnProcess( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;
	virtual void OnEnd( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;

public:
	virtual void SetDuration( Float duration );
	Float GetDurationProperty() const;

protected:
	void DoBakeScaleImpl( Float scalingFactor ) override;
};

BEGIN_CLASS_RTTI( CStorySceneEventDuration )
	PARENT_CLASS( CStorySceneEvent )
	PROPERTY_EDIT( m_duration, TXT( "Duration (without scaling)" ) )
END_CLASS_RTTI()

RED_INLINE Float CStorySceneEventDuration::GetDurationProperty() const
{
	return m_duration;
}
