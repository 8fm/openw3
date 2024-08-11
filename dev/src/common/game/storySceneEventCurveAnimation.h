/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "storySceneEventDuration.h"

namespace DialogTimelineItems
{
	class CTimelineItemCurveAnimation;
};

class CStorySceneEventCurveAnimation : public CStorySceneEventDuration
{
	DECLARE_SCENE_EVENT_CLASS( CStorySceneEventCurveAnimation, CStorySceneEventDuration );

protected:
	SMultiCurve	m_curve;

public:
	CStorySceneEventCurveAnimation();
	CStorySceneEventCurveAnimation( const String& eventName, CStorySceneElement* sceneElement, Float startTime, const CName& actor, const String& trackName, const EngineTransform& defaultPlacement );

	// compiler generated cctor is ok

	virtual CStorySceneEventCurveAnimation* Clone() const override;

public:
	virtual void OnInit( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer ) const override;
	virtual void OnDeinit( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer ) const override;

	virtual void OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;
	virtual void OnProcess( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;
	virtual void OnEnd( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;

protected:
	void InitCurve();

	friend class DialogTimelineItems::CTimelineItemCurveAnimation;
};

BEGIN_CLASS_RTTI( CStorySceneEventCurveAnimation );
	PARENT_CLASS( CStorySceneEventDuration );
	PROPERTY_CUSTOM_EDIT( m_curve, TXT( "Animation curve" ), TXT("MultiCurveEditor") );
END_CLASS_RTTI();
