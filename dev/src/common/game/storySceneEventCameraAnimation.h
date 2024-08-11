/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "storySceneEventAnimClip.h"

class CStorySceneEventCameraAnim : public CStorySceneEventAnimClip
{
	DECLARE_SCENE_EVENT_CLASS( CStorySceneEventCameraAnim, CStorySceneEventAnimClip )

private:
	CName	m_animationName;
	Bool	m_isIdle;

public:
	CStorySceneEventCameraAnim();
	CStorySceneEventCameraAnim( const String& eventName, CStorySceneElement* sceneElement, Float startTime, const CName& actor, const String& trackName );

	// compiler generated cctor is ok

	virtual CStorySceneEventCameraAnim* Clone() const override;

	RED_INLINE Bool IsIdle() const { return m_isIdle; }

	RED_INLINE void SetIsIdle( Bool idle ) { m_isIdle = idle; }
	RED_INLINE void SetAnimName( CName name ) { m_animationName = name; }

protected:
	virtual const CName& GetAnimationName() const override { return m_animationName; }
	virtual const CAnimatedComponent* GetAnimatedComponentForActor( const CStoryScenePlayer* scenePlayer ) const override;

	virtual void AddEventToCollector( CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, SAnimationState& dialogAnimationState, const SStorySceneEventTimeInfo& timeInfo, Float blendWeight ) const;
	virtual void RemoveEventFromCollector( CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;
};

BEGIN_CLASS_RTTI( CStorySceneEventCameraAnim )
	PARENT_CLASS( CStorySceneEventAnimClip )
	PROPERTY_CUSTOM_EDIT( m_animationName, TXT( "Mimics animation name" ), TXT( "DialogCameraAnimationSelection" ) )
	PROPERTY_EDIT( m_isIdle, TXT("") )
END_CLASS_RTTI()
