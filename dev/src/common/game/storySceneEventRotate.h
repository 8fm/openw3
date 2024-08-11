/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#pragma once

class CStoryScenePlayer;

class CStorySceneEventRotate : public CStorySceneEvent
{
	DECLARE_SCENE_EVENT_CLASS( CStorySceneEventRotate, CStorySceneEvent )

private:
	CName		m_actor;
	Float		m_angle;
	Bool		m_toCamera;
	Bool		m_instant;

	Bool		m_absoluteAngle;

public:
	static const CName BEH_STEP_EVENT;
	static const Char* BEH_STEP_ANGLE;

public:
	CStorySceneEventRotate();
	CStorySceneEventRotate( const String& eventName, CStorySceneElement* sceneElement, Float startTime, const CName& actor, const String& trackName );

	// compiler generated cctor is ok

	virtual CStorySceneEventRotate* Clone() const override;

public:
	virtual void OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;

public:
	RED_INLINE const CName& GetActor() const { return m_actor; }
	RED_INLINE Float GetAngle() const { return m_angle; }
	RED_INLINE Bool GetToCamera() const { return m_toCamera; }
	RED_INLINE Bool IsInstant() const { return m_instant; }
	RED_INLINE Bool IsAbsolute() const { return m_absoluteAngle; }

	void SetActor( const CName& actor );
};

BEGIN_CLASS_RTTI( CStorySceneEventRotate )
	PARENT_CLASS( CStorySceneEvent )
	PROPERTY_CUSTOM_EDIT( m_actor, TXT( "Actor" ), TXT( "DialogVoiceTag" ) )
	PROPERTY_EDIT( m_angle, TXT( "Rotation angle ( -180, 180 )" ) )
	PROPERTY_EDIT( m_absoluteAngle, TXT( "Is angle value absolute or incremental" ) )
	PROPERTY_EDIT( m_toCamera, TXT( "Should actor rotate to camera" ) )
	PROPERTY_EDIT( m_instant, TXT( "Should rotation be animated or instant" ) )
END_CLASS_RTTI()
