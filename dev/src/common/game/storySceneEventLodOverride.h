// Copyright © 2016 CD Projekt Red. All Rights Reserved.

#pragma once

#include "storySceneEvent.h"

class CStorySceneEventLodOverride : public CStorySceneEvent
{
public:
	CStorySceneEventLodOverride();
	CStorySceneEventLodOverride( const String& eventName, CStorySceneElement* sceneElement, Float startTime, CName actor, const String& trackName );
	virtual ~CStorySceneEventLodOverride() override;

	// compiler generated cctor is ok

	virtual CStorySceneEventLodOverride* Clone() const override;

	virtual void OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;

private:
	CName m_actor;											// Target actor, specified using voicetag. Only scene actors can be targeted this way.
	TagList m_actorsByTag;									// Target actors, specified using tags - all actors with any of those tags will become targets.
															// Both scene and non-scene actors can be targeted this way.

	Bool m_forceHighestLod;									// Controls whether highest LOD is forced or not.
	Bool m_disableAutoHide;									// Controls whether autohide is disabled or not.

	DECLARE_SCENE_EVENT_CLASS( CStorySceneEventLodOverride, CStorySceneEvent )
};

BEGIN_CLASS_RTTI( CStorySceneEventLodOverride )
	PARENT_CLASS( CStorySceneEvent )
	PROPERTY_CUSTOM_EDIT( m_actor, TXT( "Target actor, specified using voicetag. Only scene actors can be targeted this way." ), TXT( "DialogVoiceTag" ) )
	PROPERTY_EDIT( m_actorsByTag, TXT( "Target actors, specified  using tags - all actors with any of those tags will become targets. Both scene and non-scene actors can be targeted this way." ) );
	PROPERTY_EDIT( m_forceHighestLod, TXT( "True - always use highest LOD.\nFalse - let LOD system manage LOD level (default)." ) )
	PROPERTY_EDIT( m_disableAutoHide, TXT( "True - don't hide actor even if it's far away from camera.\nFalse - hide actor if it's far away from camera (default)" ) )
END_CLASS_RTTI()
