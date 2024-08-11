/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "storySceneEvent.h"
#include "storySceneEventCurveAnimation.h"
#include "storySceneIncludes.h"

class CStorySceneEventOverridePlacement : public CStorySceneEvent
{
	DECLARE_SCENE_EVENT_CLASS( CStorySceneEventOverridePlacement, CStorySceneEvent );

protected:
	CName				m_actorName;
	EngineTransform		m_placement;

	EDialogResetClothAndDanglesType m_resetCloth;

public:
	CStorySceneEventOverridePlacement();
	CStorySceneEventOverridePlacement( const String& eventName, CStorySceneElement* sceneElement, Float startTime, const String& trackName, const CName& actorName, const EngineTransform& defaultPlacement );

	// compiler generated cctor is ok

	virtual CStorySceneEventOverridePlacement* Clone() const override;

public:
	virtual void OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;

	RED_INLINE EngineTransform& GetTransformRef() { return m_placement; }
	RED_INLINE const EngineTransform& GetTransformRef() const { return m_placement; }

	virtual CName GetSubject() const override { return m_actorName; }

protected:
	virtual void OnInterpolationEventGUIDSet() override;

#ifndef NO_EDITOR
public:
	RED_INLINE void SetTransform( const EngineTransform& transform ) { m_placement = transform; }
#endif
};

BEGIN_CLASS_RTTI( CStorySceneEventOverridePlacement );
	PARENT_CLASS( CStorySceneEvent );
	PROPERTY_CUSTOM_EDIT( m_actorName, TXT( "actor name" ), TXT( "DialogVoiceTag" ) )
	PROPERTY_EDIT( m_placement, TXT( "overriden actor placement" ) );
	PROPERTY_EDIT( m_resetCloth, TXT( "" ) );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CStorySceneEventOverridePlacementDuration : public CStorySceneEventCurveAnimation
{
	DECLARE_SCENE_EVENT_CLASS( CStorySceneEventOverridePlacementDuration, CStorySceneEventCurveAnimation );

protected:
	CName				m_actorName;

public:
	CStorySceneEventOverridePlacementDuration();
	CStorySceneEventOverridePlacementDuration( const String& eventName, CStorySceneElement* sceneElement,
		Float startTime, const String& trackName, const CName& actorName, const EngineTransform& defaultPlacement );

	// compiler generated cctor is ok

	virtual CStorySceneEventOverridePlacementDuration* Clone() const override;

public:
	virtual void OnProcess( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;
	virtual void OnEnd( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;

	virtual CName GetSubject() const override { return m_actorName; }
};

BEGIN_CLASS_RTTI( CStorySceneEventOverridePlacementDuration );
	PARENT_CLASS( CStorySceneEventCurveAnimation );
	PROPERTY_CUSTOM_EDIT( m_actorName, TXT( "actor name" ), TXT( "DialogVoiceTag" ) )
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CStorySceneEventWalk : public CStorySceneEventCurveAnimation
{
	DECLARE_SCENE_EVENT_CLASS( CStorySceneEventWalk, CStorySceneEventCurveAnimation );

protected:
	CName		m_actorName;

	CName		m_animationStartName;
	CName		m_animationLoopName;
	CName		m_animationStopName;

public:
	CStorySceneEventWalk();
	CStorySceneEventWalk( const String& eventName, CStorySceneElement* sceneElement,
		Float startTime, const String& trackName, const CName& actorName, const EngineTransform& defaultPlacement );

	// compiler generated cctor is ok

	virtual CStorySceneEventWalk* Clone() const override;

public:
	virtual void OnProcess( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;
	virtual void OnEnd( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;

	virtual CName GetSubject() const override { return m_actorName; }
};

BEGIN_CLASS_RTTI( CStorySceneEventWalk );
	PARENT_CLASS( CStorySceneEventCurveAnimation );
	PROPERTY_CUSTOM_EDIT( m_actorName, TXT( "actor name" ), TXT( "DialogVoiceTag" ) );
	PROPERTY_CUSTOM_EDIT( m_animationStartName, TXT("Animation name"), TXT("DialogAnimationSelection") );
	PROPERTY_CUSTOM_EDIT( m_animationLoopName, TXT("Animation name"), TXT("DialogAnimationSelection") );
	PROPERTY_CUSTOM_EDIT( m_animationStopName, TXT("Animation name"), TXT("DialogAnimationSelection") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

#include "storySceneEventBlend.h"

namespace DialogTimelineItems
{
	class CTimelineItemOverridePlacementBlend;
};

class CStorySceneOverridePlacementBlend : public CStorySceneEventCurveBlend
{
	DECLARE_SCENE_EVENT_CLASS( CStorySceneOverridePlacementBlend, CStorySceneEventCurveBlend );

protected:
	CName		m_actorName;

	CName		m_animationStartName;
	CName		m_animationLoopName;
	CName		m_animationStopName;

public:
	CStorySceneOverridePlacementBlend();
	CStorySceneOverridePlacementBlend( const String& eventName, CStorySceneElement* sceneElement,
		Float startTime, const String& trackName, const CName& actorName );

	// compiler generated cctor is ok

	virtual CStorySceneOverridePlacementBlend* Clone() const override;

	// Overridden from CStorySceneEventDuration
	virtual void OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;
	virtual void OnEnd( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;
	virtual void OnProcess( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;

	// Overridden from CStorySceneEventBlend
	virtual Uint32 GetNumExtraCurveDataStreams() const override;
	virtual void GetKeyDataFromEvent( CStorySceneEvent* keyEvent, CurveKeyData& result ) const override;
	virtual void SetKeyDataToEvent( CStorySceneEvent* keyEvent, const CurveKeyData& data ) const override;

	Bool GetTransformAt( Float localTime, EngineTransform& result ) const;

	virtual CName GetSubject() const override { return m_actorName; }

protected:
	friend class DialogTimelineItems::CTimelineItemOverridePlacementBlend;
};

BEGIN_CLASS_RTTI( CStorySceneOverridePlacementBlend );
	PARENT_CLASS( CStorySceneEventCurveBlend );
	PROPERTY_CUSTOM_EDIT( m_actorName, TXT( "actor name" ), TXT( "DialogVoiceTag" ) );
	PROPERTY_CUSTOM_EDIT( m_animationStartName, TXT("Animation name"), TXT("DialogAnimationSelection") );
	PROPERTY_CUSTOM_EDIT( m_animationLoopName, TXT("Animation name"), TXT("DialogAnimationSelection") );
	PROPERTY_CUSTOM_EDIT( m_animationStopName, TXT("Animation name"), TXT("DialogAnimationSelection") );
END_CLASS_RTTI();
