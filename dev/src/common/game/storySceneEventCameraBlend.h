/**
* Copyright c 2012 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "storySceneEventBlend.h"
#include "storySceneDialogset.h"

class CStorySceneEventCustomCamera;

//////////////////////////////////////////////////////////////////////////

enum ECameraInterpolation
{
	LINEAR,
	BEZIER
};

BEGIN_ENUM_RTTI( ECameraInterpolation );
	ENUM_OPTION( LINEAR );
	ENUM_OPTION( BEZIER );
END_ENUM_RTTI();

//////////////////////////////////////////////////////////////////////////

/*
Describes blend key. Used only by old style camera blends (CStorySceneEventCameraBlend).
*/
struct SStorySceneCameraBlendKey
{
	DECLARE_RTTI_STRUCT( SStorySceneCameraBlendKey );

	Float						m_time;
	StorySceneCameraDefinition	m_cameraDefinition;

	SStorySceneCameraBlendKey() 
	{
	}

	SStorySceneCameraBlendKey( Float time, const StorySceneCameraDefinition& cameraDefinition )
		: m_time( time )
		, m_cameraDefinition( cameraDefinition )
	{
	}
};

BEGIN_CLASS_RTTI( SStorySceneCameraBlendKey )
	PROPERTY_EDIT( m_time, TXT( "Time" ) );
	PROPERTY_INLINED( m_cameraDefinition, TXT( "Camera definition") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

/*
Old camera blends.

This class is obsoleted by CStorySceneCameraBlendEvent.
*/
class CStorySceneEventCameraBlend : public CStorySceneEventDuration
{
	DECLARE_SCENE_EVENT_CLASS( CStorySceneEventCameraBlend, CStorySceneEventDuration )

protected:
	String									m_cameraName;
	TDynArray< SStorySceneCameraBlendKey >	m_blendKeys;
	ECameraInterpolation					m_interpolationType;

public:
	CStorySceneEventCameraBlend();
	CStorySceneEventCameraBlend( const String& eventName, CStorySceneElement* sceneElement, const String& trackName, Float startTime, Float duration = 2.0f );

	// compiler generated cctor is ok

	virtual CStorySceneEventCameraBlend* Clone() const override;

public:
	virtual void OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;
	virtual void OnEnd( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;
	virtual void OnProcess( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;

public:
	RED_INLINE void SetCameraName( const String& cameraName ) { m_cameraName = cameraName; }
	RED_INLINE Bool IsValid() const { return m_blendKeys.Size() >= 2; }

	RED_INLINE void AddBlendKey( Float time, const StorySceneCameraDefinition& cameraDefinition ) { m_blendKeys.PushBack( SStorySceneCameraBlendKey( time, cameraDefinition ) ); }
	RED_INLINE const TDynArray< SStorySceneCameraBlendKey >& GetBlendKeys() const { return m_blendKeys; }

public:
	static void InterpolateCameraDefinitions( const StorySceneCameraDefinition& from, const StorySceneCameraDefinition& to, Float weight, StorySceneCameraDefinition& result );

#ifndef NO_EDITOR
public:
	ECameraInterpolation GetInterpolationType() const { return m_interpolationType; }
#endif
};

BEGIN_CLASS_RTTI( CStorySceneEventCameraBlend )
	PARENT_CLASS( CStorySceneEventDuration )
	//PROPERTY_EDIT( m_cameraName, TXT( "Camera definition name" ) )
	PROPERTY_INLINED( m_blendKeys, TXT( "Camera blending keys" ) );
	PROPERTY_EDIT( m_interpolationType, TXT( "Camera interpolation method" ) );
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////

/*
New camera blends

This class replaces old camera blends (CStorySceneEventCameraBlend).
*/
class CStorySceneCameraBlendEvent : public CStorySceneEventBlend
{
	DECLARE_SCENE_EVENT_CLASS( CStorySceneCameraBlendEvent, CStorySceneEventBlend )

protected:
	Float								m_firstPointOfInterpolation;
	Float								m_lastPointOfInterpolation;

	ECameraInterpolation				m_firstPartInterpolation;
	ECameraInterpolation				m_lastPartInterpolation;

protected:
	TInstanceVar< TDynArray< Float > >	i_cachedTimes;
	TInstanceVar< TDynArray< Int32 > >	i_cachedIndices;

public:
	CStorySceneCameraBlendEvent();
	CStorySceneCameraBlendEvent( const String& eventName, CStorySceneElement* sceneElement, const String& trackName );
	CStorySceneCameraBlendEvent( const CStorySceneCameraBlendEvent& other );

	virtual CStorySceneCameraBlendEvent* Clone() const override;

public:
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler ) override;

	virtual void OnProcess( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;

#ifndef NO_EDITOR
public:
	Float GetFirstPointOfInterpolation() const { return m_firstPointOfInterpolation; }
	Float GetLastPointOfInterpolation() const { return m_lastPointOfInterpolation; }

	void SetInterpolationType( ECameraInterpolation firstPart, ECameraInterpolation lastPart ) { m_firstPartInterpolation = firstPart; m_lastPartInterpolation = lastPart; }
	ECameraInterpolation GetFirstPartInterpolationType() const { return m_firstPartInterpolation; }
	ECameraInterpolation GetLastPartInterpolationType() const { return m_lastPartInterpolation; }
#endif

private:
	void InterpolateCameraDefinitions( const StorySceneCameraDefinition& from, const StorySceneCameraDefinition& to, Float weight, StorySceneCameraDefinition& result );

	void LazyInitialization( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer ) const;
};

BEGIN_CLASS_RTTI( CStorySceneCameraBlendEvent )
	PARENT_CLASS( CStorySceneEventBlend )
	PROPERTY_EDIT_RANGE( m_firstPointOfInterpolation, TXT("[0,1]"), 0.f, 1.f );
	PROPERTY_EDIT_RANGE( m_lastPointOfInterpolation, TXT("[0,1]"), 0.f, 1.f );
	PROPERTY_EDIT( m_firstPartInterpolation, TXT("") );
	PROPERTY_EDIT( m_lastPartInterpolation, TXT("") );
END_CLASS_RTTI()
