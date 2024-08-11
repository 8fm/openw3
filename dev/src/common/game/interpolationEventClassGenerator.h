// Copyright © 2014 CD Projekt Red. All Rights Reserved.

#pragma once

#include "interpolationBasics.h"
#include "interpolationDriver.h"
#include "storySceneEventInterpolation.h"

/*
This file contains all the machinery needed to generate new interpolation event class.
Two macros are defined in this file:

1. RED_SCENE_INTERPOLATION_EVENT_GENERATE_CLASS_HEADER( InterpolationEventClass, KeyEventClass )
2. RED_SCENE_INTERPOLATION_EVENT_GENERATE_CLASS_IMPL( InterpolationEventClass, KeyEventClass )

First macro is to be used in header file. Second one - in implementation file.

InterpolationEventClass is a name of interpolation event class that will be generated. This class
will inherit from CStorySceneEventInterpolation. KeyEventClass is a name of key event class that
will be interpolated by InterpolationEventClass. This class must already exist.

See storySceneEventCameraInterpolation.h/cpp or storySceneEventPlacementInterpolation.h/cpp
for examples of generated interpolation event classes.

In short, this is what you have to do to create new interpolation event class.
1. Create InterpolationTraits< InterpolationEventClass > - this describes KeyEventClass specific interpolation traits.
2. Use RED_SCENE_INTERPOLATION_EVENT_GENERATE_CLASS_HEADER( InterpolationEventClass, KeyEventClass ) in header file.
3. RED_SCENE_INTERPOLATION_EVENT_GENERATE_CLASS_IMPL( InterpolationEventClass, KeyEventClass ) in implementation file.
4. Register InterpolationEventClass and InterpolationEventClass##Key in gameTypeRegistry.h.
5. Enjoy :)
*/

/*
Generates InterpolationEventClass and InterpolationEventClass##Key classes - this macro is to be used in header file.

\param InterpolationEventClass Name of interpolation event class that will be generated. This class
will inherit from CStorySceneEventInterpolation.
\param KeyEventClass Name of key event class that will be interpolated by InterpolationEventClass.
This class must already exist.
*/
#define RED_SCENE_INTERPOLATION_EVENT_GENERATE_CLASS_HEADER( InterpolationEventClass, KeyEventClass )																											\
/* Describes interpolation key.																			*/																										\
/*																										*/																										\
/* This should be InterpolationEventClass::CInterpolationKey class but our RTTI won't let us do this.	*/																										\
class InterpolationEventClass##Key																																												\
{																																																				\
public:																																																			\
	typedef StoryScene::InterpolationTraits< KeyEventClass > Traits;																																			\
																																																				\
	Bezier2dHandle m_bezierHandles[ Traits::numChannels ];					/* Key Bezier handle for each channel. */																							\
	Uint32 m_interpolationTypes[ Traits::numChannels ];						/* Determines which interpolation method to use for each channel */																	\
																			/* when interpolating between this key and the next one. */																			\
																			/* TODO: This isn't used yet - see CStorySceneEventInterpolation::m_interpolationMethod. */											\
																			/* TODO: make this enum InterpolationTypes. */																						\
	Bool m_volatile;														/* Indicates whether key is static or volatile. */																					\
																																																				\
	DECLARE_RTTI_SIMPLE_CLASS( InterpolationEventClass##Key )																																					\
};																																																				\
																																																				\
BEGIN_CLASS_RTTI( InterpolationEventClass##Key )																																								\
	PROPERTY( m_bezierHandles )																																													\
	PROPERTY( m_interpolationTypes )																																											\
	PROPERTY( m_volatile )																																														\
END_CLASS_RTTI();																																																\
																																																				\
/* Interpolation event that can interpolate keys of specified type.										*/																										\
class InterpolationEventClass : public CStorySceneEventInterpolation																																			\
{																																																				\
public:																																																			\
	typedef StoryScene::InterpolationTraits< KeyEventClass > Traits;																																			\
	typedef KeyEventClass KeyEvent;																																												\
	typedef InterpolationEventClass##Key Key;																																									\
	typedef StoryScene::InterpolationDriver< InterpolationEventClass > Driver;																																	\
																																																				\
	friend Driver;																																																\
																																																				\
	InterpolationEventClass();																																													\
	InterpolationEventClass( const CreationArgs& creationArgs );																																				\
	InterpolationEventClass( const InterpolationEventClass& other );																																			\
	virtual ~InterpolationEventClass() override;																																								\
																																																				\
	virtual InterpolationEventClass* Clone() const override;																																					\
																																																				\
	virtual Bool Initialize(  const CreationArgs& creationArgs ) override;																																		\
																																																				\
	virtual Uint32 GetNumKeys() const override;																																									\
	const Key& GetKey( Uint32 keyIndex ) const;																																									\
	Key& GetKey( Uint32 keyIndex );																																												\
																																																				\
	Uint32 CountVolatileKeys() const;																																											\
																																																				\
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler ) override;																															\
	virtual void OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;					\
	virtual void OnProcess( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;					\
																																																				\
private:																																																		\
	virtual void ImplOnKeyChanged( CGUID keyGuid ) override;																																					\
	virtual void ImplAttachKey( CStorySceneEvent* keyEvent ) override;																																			\
	virtual void ImplDetachKey( CStorySceneEvent* keyEvent ) override;																																			\
	virtual void ImplSampleData( Float timeStep, TDynArray< TDynArray< TPair< Vector2, Vector > > >& outData, CSceneEventFunctionSimpleArgs& args ) const override;												\
	virtual void ImplRecalculateBezierHandles( CSceneEventFunctionSimpleArgs& args ) override;																													\
																																																				\
	TDynArray< Key > m_keys;																		/* List of keys.									*/														\
	TInstanceVar< TDynArray< Float > > i_cachedKeyStartTimes;																																					\
	TInstanceVar< TDynArray< Float > > i_channelStorage;											/* Storage for values of all channels of all keys.	*/														\
																									/* Format: key 0 channels, key 1 channels, ..		*/														\
																																																				\
	DECLARE_SCENE_EVENT_CLASS( InterpolationEventClass, CStorySceneEventInterpolation )																															\
};																																																				\
																																																				\
BEGIN_CLASS_RTTI( InterpolationEventClass )																																										\
	PARENT_CLASS( CStorySceneEventInterpolation )																																								\
	PROPERTY( m_keys )																																															\
END_CLASS_RTTI()																																																\
																																																				\
RED_INLINE const InterpolationEventClass::Key& InterpolationEventClass::GetKey( Uint32 keyIndex ) const																											\
{																																																				\
	ASSERT( keyIndex < m_keys.Size() );																																											\
	return m_keys[ keyIndex ];																																													\
}																																																				\
																																																				\
RED_INLINE InterpolationEventClass::Key& InterpolationEventClass::GetKey( Uint32 keyIndex )																														\
{																																																				\
	ASSERT( keyIndex < m_keys.Size() );																																											\
	return m_keys[ keyIndex ];																																													\
}

/*
Generates InterpolationEventClass and InterpolationEventClass##Key classes - this macro is to be used in implementation file.

\param InterpolationEventClass Name of interpolation event class that will be generated. This class
will inherit from CStorySceneEventInterpolation.
\param KeyEventClass Name of key event class that will be interpolated by InterpolationEventClass.
This class must already exist.
*/
#define RED_SCENE_INTERPOLATION_EVENT_GENERATE_CLASS_IMPL( InterpolationEventClass, KeyEventClass )																												\
IMPLEMENT_ENGINE_CLASS( InterpolationEventClass##Key  );																																						\
IMPLEMENT_ENGINE_CLASS( InterpolationEventClass  );																																								\
																																																				\
/* Ctor. */																																																		\
InterpolationEventClass::InterpolationEventClass()																																								\
{}																																																				\
																																																				\
/* Ctor.																		*/																																\
/*																				*/																																\
/*\param creationArgs Creation args. See CreationArgs docs for requirements.	*/																																\
InterpolationEventClass::InterpolationEventClass( const CreationArgs& creationArgs )																															\
: CStorySceneEventInterpolation( creationArgs.m_eventName, creationArgs.m_keyEvents[ 0 ]->GetSceneElement(), creationArgs.m_keyEvents[ 0 ]->GetStartPosition(), creationArgs.m_keyEvents[ 0 ]->GetTrackName() )	\
{																																																				\
	for( Uint32 iKey = 0, numKeys = creationArgs.m_keyEvents.Size(); iKey < numKeys; ++iKey )																													\
	{																																																			\
		AttachKey( creationArgs.m_keyEvents[ iKey ] );																																							\
	}																																																			\
}																																																				\
																																																				\
/* Cctor.																									*/																									\
/*																											*/																									\
/* Compiler generated cctor would also copy instance vars - we don't want that.								*/																									\
/*																											*/																									\
/* Resulting interpolation event will reference the same key events as original interpolation event so you	*/																									\
/* will most likely want to copy all those key events and associate them with new interpolation event.		*/																									\
InterpolationEventClass::InterpolationEventClass( const InterpolationEventClass& other )																														\
: CStorySceneEventInterpolation( other )																																										\
, m_keys( other.m_keys )																																														\
{}																																																				\
																																																				\
/* Dtor. */																																																		\
InterpolationEventClass::~InterpolationEventClass()																																								\
{}																																																				\
																																																				\
/* Clones interpolation event.																				*/																									\
/*																											*/																									\
/* Resulting interpolation event will reference the same key events as original interpolation event so you	*/																									\
/* will most likely want to Clone() all those key events and associate them with new interpolation event.	*/																									\
InterpolationEventClass* InterpolationEventClass::Clone() const																																					\
{																																																				\
	return new InterpolationEventClass( *this );																																								\
}																																																				\
																																																				\
/* TODO: explain when Initialize() should be called or better - refactor some code so it's more intuitive. */																									\
Bool InterpolationEventClass::Initialize(  const CreationArgs& creationArgs )																																	\
{																																																				\
	m_eventName = creationArgs.m_eventName;																																										\
	m_sceneElement = creationArgs.m_keyEvents[ 0 ]->GetSceneElement();																																			\
	m_startPosition = creationArgs.m_keyEvents[ 0 ]->GetStartPosition();																																		\
	m_trackName = creationArgs.m_keyEvents[ 0 ]->GetTrackName();																																				\
																																																				\
	for( Uint32 iKey = 0, numKeys = creationArgs.m_keyEvents.Size(); iKey < numKeys; ++iKey )																													\
	{																																																			\
		AttachKey( creationArgs.m_keyEvents[ iKey ]  );																																							\
	}																																																			\
																																																				\
	return true;																																																\
}																																																				\
																																																				\
Uint32 InterpolationEventClass::GetNumKeys() const																																								\
{																																																				\
	return m_keys.Size();																																														\
}																																																				\
																																																				\
Uint32 InterpolationEventClass::CountVolatileKeys() const																																						\
{																																																				\
	Uint32 numVolatileKeys = 0;																																													\
	for( Uint32 iKey = 0, numKeys = m_keys.Size(); iKey < numKeys; ++iKey )																																		\
	{																																																			\
		numVolatileKeys += m_keys[ iKey ].m_volatile;																																							\
	}																																																			\
																																																				\
	return numVolatileKeys;																																														\
}																																																				\
																																																				\
void InterpolationEventClass::ImplOnKeyChanged( CGUID keyGuid )																																					\
{																																																				\
	Driver::OnKeyChanged( *this, keyGuid );																																										\
}																																																				\
																																																				\
void InterpolationEventClass::ImplAttachKey( CStorySceneEvent* keyEvent )																																		\
{																																																				\
	Driver::AttachKey( *this, static_cast< KeyEvent* >( keyEvent ) );																																			\
}																																																				\
																																																				\
void InterpolationEventClass::ImplDetachKey( CStorySceneEvent* keyEvent )																																		\
{																																																				\
	Driver::DetachKey( *this, static_cast< KeyEvent* >( keyEvent ) );																																			\
}																																																				\
																																																				\
/*\param outData (out) Storage for output data. outData[ ch ] will contain samples for channel ch.				*/																								\
/* Curve samples spaced by timeStep are followed by curve samples for each key point.							*/																								\
/* Each sample is ( Vector2: point on curve, Vector: Bezier handle tangents ).									*/																								\
/* Bezier handle tangents are defined for key point samples only. For the rest of samples zero vector is used.	*/																								\
void InterpolationEventClass::ImplSampleData( Float timeStep, TDynArray< TDynArray< TPair< Vector2, Vector > > >& outData, CSceneEventFunctionSimpleArgs& args ) const											\
{																																																				\
	Driver::SampleData( *this, timeStep, outData, args );																																						\
}																																																				\
																																																				\
void InterpolationEventClass::ImplRecalculateBezierHandles( CSceneEventFunctionSimpleArgs& args )																												\
{																																																				\
	StoryScene::RecalculateBezierHandles( *this, Traits::VolatileChannels(), args );																															\
	StoryScene::RecalculateBezierHandles( *this, Traits::StaticChannels(), args );																																\
}																																																				\
																																																				\
void InterpolationEventClass::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )																															\
{																																																				\
	TBaseClass::OnBuildDataLayout( compiler );																																									\
																																																				\
	compiler << i_channelStorage;																																												\
	compiler << i_cachedKeyStartTimes;																																											\
}																																																				\
																																																				\
void InterpolationEventClass::OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const					\
{																																																				\
	TBaseClass::OnStart( data, scenePlayer, collector, timeInfo );																																				\
																																																				\
	CSceneEventFunctionArgs args( data, scenePlayer, collector, timeInfo );																																		\
	Driver::OnStart( *this, args );																																												\
}																																																				\
																																																				\
void InterpolationEventClass::OnProcess( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const				\
{																																																				\
	TBaseClass::OnProcess( data, scenePlayer, collector, timeInfo );																																			\
																																																				\
	CSceneEventFunctionArgs args( data, scenePlayer, collector, timeInfo );																																		\
	Driver::OnProcess( *this, args );																																											\
}
