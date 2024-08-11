/**
* Copyright c 2012 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#include "../engine/behaviorGraphStack.h"

#include "storySceneEventCameraBlend.h"
#include "storyScenePlayer.h"
#include "storySceneDirector.h"
#include "storySceneEventsCollector_events.h"
#include "storySceneEventCustomCamera.h"
#include "storySceneEventCustomCameraInstance.h"
#include "storySceneEventCamera.h"
#include "../core/instanceDataLayoutCompiler.h"

#ifdef DEBUG_SCENES_2
#pragma optimize("",off)
#endif

IMPLEMENT_RTTI_ENUM( ECameraInterpolation );

IMPLEMENT_ENGINE_CLASS( SStorySceneCameraBlendKey );
IMPLEMENT_ENGINE_CLASS( CStorySceneEventCameraBlend );

CStorySceneEventCameraBlend::CStorySceneEventCameraBlend()
	: CStorySceneEventDuration()
	, m_cameraName( String::EMPTY )
	, m_interpolationType( BEZIER )
{

}

CStorySceneEventCameraBlend::CStorySceneEventCameraBlend( const String& eventName, CStorySceneElement* sceneElement, 
	const String& trackName, Float startTime, Float duration )
	: CStorySceneEventDuration( eventName, sceneElement, startTime, duration, trackName )
	, m_cameraName( String::EMPTY )
	, m_interpolationType( BEZIER )
{

}

CStorySceneEventCameraBlend* CStorySceneEventCameraBlend::Clone() const 
{
	return new CStorySceneEventCameraBlend( *this );
}

void CStorySceneEventCameraBlend::OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	TBaseClass::OnStart( data, scenePlayer, collector, timeInfo );

	if ( !m_blendKeys.Empty() && !scenePlayer->IsGameplayNow() )
	{
		StorySceneEventsCollector::CameraBlend event( this );
		event.m_eventTimeAbs = timeInfo.m_timeAbs;
		event.m_eventTimeLocal = timeInfo.m_timeLocal;
		event.m_currentCameraState = m_blendKeys[ 0 ].m_cameraDefinition;

		collector.AddEvent( event );
	}
}

void CStorySceneEventCameraBlend::OnEnd( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	if ( !m_blendKeys.Empty() && !scenePlayer->IsGameplayNow() )
	{
		StorySceneEventsCollector::CameraBlend event( this );
		event.m_eventTimeAbs = timeInfo.m_timeAbs;
		event.m_eventTimeLocal = timeInfo.m_timeLocal;
		event.m_currentCameraState = m_blendKeys.Back().m_cameraDefinition;

		collector.AddEvent( event );
	}

	TBaseClass::OnEnd( data, scenePlayer, collector, timeInfo );
}

void CStorySceneEventCameraBlend::OnProcess( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	TBaseClass::OnProcess( data, scenePlayer, collector, timeInfo );

	if ( !scenePlayer->IsGameplayNow() )
	{
		Float eventAlpha = timeInfo.m_timeLocal /  data[ i_durationMS ];

		for( Uint32 i = 1; i < m_blendKeys.Size(); ++i )
		{
			const SStorySceneCameraBlendKey& nextBlendKey = m_blendKeys[ i ];
			const SStorySceneCameraBlendKey& lastBlendKey = m_blendKeys[ i - 1 ];

			const Float scalingFactor = GetInstanceScalingFactor( data );

			if ( nextBlendKey.m_time * scalingFactor > eventAlpha )
			{
				const Float keyAlpha = ( eventAlpha - lastBlendKey.m_time * scalingFactor ) / ( nextBlendKey.m_time * scalingFactor - lastBlendKey.m_time * scalingFactor );
				SCENE_ASSERT( keyAlpha >= 0.f && keyAlpha <= 1.f );

				StorySceneEventsCollector::CameraBlend event( this );
				event.m_eventTimeAbs = timeInfo.m_timeAbs;
				event.m_eventTimeLocal = timeInfo.m_timeLocal;

				const Float weight = ( m_interpolationType == BEZIER ) ? BehaviorUtils::BezierInterpolation( keyAlpha ) : keyAlpha; // TODO: Don't use reference to behavior; think about modifiable curves

				InterpolateCameraDefinitions( lastBlendKey.m_cameraDefinition, nextBlendKey.m_cameraDefinition, weight, event.m_currentCameraState );

				if ( m_blendKeys.Size() > 0)
				{
					event.m_currentCameraState.m_sourceSlotName = m_blendKeys[m_blendKeys.Size()-1].m_cameraDefinition.m_sourceSlotName;
					event.m_currentCameraState.m_targetSlotName = m_blendKeys[m_blendKeys.Size()-1].m_cameraDefinition.m_targetSlotName;
				}

				collector.AddEvent( event );

				break;
			}
		}
	}
}

/*
Performs linear interpolation between two camera definitions.
*/
void CStorySceneEventCameraBlend::InterpolateCameraDefinitions( const StorySceneCameraDefinition& from, const StorySceneCameraDefinition& to, Float weight, StorySceneCameraDefinition& result )
{
	result.m_cameraTransform.SetPosition( from.m_cameraTransform.GetPosition() * ( 1.0f - weight ) + to.m_cameraTransform.GetPosition() * ( weight ) );
#ifdef USE_HAVOK_ANIMATION
	result.m_cameraTransform.SetRotation( InterpolateEulerAngles( from.m_cameraTransform.GetRotation(), to.m_cameraTransform.GetRotation(), weight ) );
#else
	RedEulerAngles angle = RedEulerAngles::InterpolateEulerAngles( reinterpret_cast< const RedEulerAngles& >( from.m_cameraTransform.GetRotation() ), reinterpret_cast< const RedEulerAngles& >( to.m_cameraTransform.GetRotation() ), weight );
	result.m_cameraTransform.SetRotation( reinterpret_cast< const EulerAngles& >( angle ) );
#endif
	//result.m_cameraTransform.SetRotation( from.m_cameraTransform.GetRotation() * ( 1.0f - weight ) + to.m_cameraTransform.GetRotation() * weight );
	result.m_cameraFov = from.m_cameraFov * ( 1.0f - weight ) + to.m_cameraFov * weight;
	result.m_cameraZoom = from.m_cameraZoom * ( 1.0f - weight ) + to.m_cameraZoom * weight;
	result.m_dofBlurDistFar = from.m_dofBlurDistFar * ( 1.0f - weight ) + to.m_dofBlurDistFar * weight;
	result.m_dofBlurDistNear = from.m_dofBlurDistNear * ( 1.0f - weight ) + to.m_dofBlurDistNear * weight;
	result.m_dofFocusDistFar = from.m_dofFocusDistFar * ( 1.0f - weight ) + to.m_dofFocusDistFar * weight;
	result.m_dofFocusDistNear = from.m_dofFocusDistNear * ( 1.0f - weight ) + to.m_dofFocusDistNear * weight;
	result.m_dofIntensity = from.m_dofIntensity * ( 1.0f - weight ) + to.m_dofIntensity * weight;

	result.m_targetEyesLS = Vector::ZEROS;
	result.m_sourceEyesHeigth = 0.f;
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CStorySceneCameraBlendEvent );

CStorySceneCameraBlendEvent::CStorySceneCameraBlendEvent()
	: m_firstPointOfInterpolation( 0.5f )
	, m_lastPointOfInterpolation( 0.5f )
	, m_firstPartInterpolation( BEZIER )
	, m_lastPartInterpolation( BEZIER )
{

}

CStorySceneCameraBlendEvent::CStorySceneCameraBlendEvent( const String& eventName, CStorySceneElement* sceneElement, const String& trackName )
	: CStorySceneEventBlend( eventName, sceneElement, trackName, 0.0f, 1.0f )
	, m_firstPointOfInterpolation( 0.5f )
	, m_lastPointOfInterpolation( 0.5f )
	, m_firstPartInterpolation( BEZIER )
	, m_lastPartInterpolation( BEZIER )
{

}

/*
Cctor.

Compiler generated cctor would also copy instance vars - we don't want that.
*/
CStorySceneCameraBlendEvent::CStorySceneCameraBlendEvent( const CStorySceneCameraBlendEvent& other )
: CStorySceneEventBlend( other )
, m_firstPointOfInterpolation( other.m_firstPointOfInterpolation )
, m_lastPointOfInterpolation( other.m_lastPointOfInterpolation )
, m_firstPartInterpolation( other.m_firstPartInterpolation )
, m_lastPartInterpolation( other.m_lastPartInterpolation )
{}

CStorySceneCameraBlendEvent* CStorySceneCameraBlendEvent::Clone() const
{
	return new CStorySceneCameraBlendEvent( *this );
}

void CStorySceneCameraBlendEvent::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_cachedIndices;
	compiler << i_cachedTimes;
}

void CStorySceneCameraBlendEvent::OnProcess( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	TBaseClass::OnProcess( data, scenePlayer, collector, timeInfo );

	if ( !scenePlayer->IsInGameplay() )
	{
		const Float time = timeInfo.m_timeAbs;

		const TDynArray< Float >& times = data[ i_cachedTimes ];
		const TDynArray< Int32 >& indices = data[ i_cachedIndices ];

		//++ DIALOG_TOMSIN_TODO - remove this lazy initialization
		if ( times.Size() == 0 )
		{
			LazyInitialization( data, scenePlayer );
		}
		//--

		SCENE_ASSERT( times.Size() == indices.Size() );

		const Int32 num = times.SizeInt();
		for ( Int32 i=1; i<num; ++i )
		{
			const Float currTime = times[ i ];
			if ( currTime >= time )
			{
				const Float prevTime = times[ i-1 ];

				SCENE_ASSERT( prevTime <= currTime );

				const Int32 prevIndex = indices[ i-1 ];
				const Int32 currIndex = indices[ i ];

				SCENE_ASSERT( prevIndex != -1 );
				SCENE_ASSERT( currIndex != -1 );

				const CStorySceneEvent* prevEvt = scenePlayer->FindEventByIndex( prevIndex );
				const CStorySceneEvent* currEvt = scenePlayer->FindEventByIndex( currIndex );

				SCENE_ASSERT( prevEvt );
				SCENE_ASSERT( currEvt );

				if ( prevEvt && currEvt )
				{
					const CStorySceneEventCamera* prevCam = Cast< const CStorySceneEventCamera >( prevEvt );
					const CStorySceneEventCamera* currCam = Cast< const CStorySceneEventCamera >( currEvt );

					SCENE_ASSERT( prevCam );
					SCENE_ASSERT( currCam );

					if ( prevCam && currCam )
					{
						const Float blendWeight = ( time - prevTime ) / ( currTime - prevTime );
						SCENE_ASSERT( blendWeight >= 0.f && blendWeight <= 1.f );

						const StorySceneCameraDefinition* prevCamDef = prevCam->GetCameraDefinition();
						const StorySceneCameraDefinition* currCamDef = currCam->GetCameraDefinition();

						StorySceneEventsCollector::CameraBlend event( this );
						event.m_eventTimeAbs = timeInfo.m_timeAbs;
						event.m_eventTimeLocal = timeInfo.m_timeLocal;

						Float finalWeight = blendWeight;
						if ( num == 1 )
						{
							// One part blend
							if ( blendWeight < m_firstPointOfInterpolation && m_firstPartInterpolation == BEZIER )
							{
								finalWeight = BehaviorUtils::BezierInterpolation( blendWeight );
							}
							else if ( blendWeight >= m_firstPointOfInterpolation && m_lastPartInterpolation == BEZIER )
							{
								finalWeight = BehaviorUtils::BezierInterpolation( blendWeight );
							}
						}
						else if ( i == 1 && m_firstPartInterpolation == BEZIER && blendWeight < m_firstPointOfInterpolation )
						{
							// First part
							finalWeight = BehaviorUtils::BezierInterpolation( blendWeight );
						}
						else if ( i == num-1 && m_lastPartInterpolation == BEZIER && blendWeight >= m_lastPointOfInterpolation )
						{
							// Last part
							finalWeight = BehaviorUtils::BezierInterpolation( blendWeight );
						}

						SCENE_ASSERT( finalWeight >= 0.f && finalWeight <= 1.f );

						CStorySceneEventCameraBlend::InterpolateCameraDefinitions( *prevCamDef, *currCamDef, finalWeight, event.m_currentCameraState );

						collector.AddEvent( event );
					}
				}

				break;
			}
		}
	}
}

void CStorySceneCameraBlendEvent::InterpolateCameraDefinitions( const StorySceneCameraDefinition& from, const StorySceneCameraDefinition& to, Float weight, StorySceneCameraDefinition& result )
{
	result.m_cameraTransform.SetPosition( from.m_cameraTransform.GetPosition() * ( 1.0f - weight ) + to.m_cameraTransform.GetPosition() * ( weight ) );
#ifdef USE_HAVOK_ANIMATION
	result.m_cameraTransform.SetRotation( InterpolateEulerAngles( from.m_cameraTransform.GetRotation(), to.m_cameraTransform.GetRotation(), weight ) );
#else
	RedEulerAngles angle = RedEulerAngles::InterpolateEulerAngles( reinterpret_cast< const RedEulerAngles& >( from.m_cameraTransform.GetRotation() ), reinterpret_cast< const RedEulerAngles& >( to.m_cameraTransform.GetRotation() ), weight );
	result.m_cameraTransform.SetRotation( reinterpret_cast< const EulerAngles& >( angle ) );
#endif
	//result.m_cameraTransform.SetRotation( from.m_cameraTransform.GetRotation() * ( 1.0f - weight ) + to.m_cameraTransform.GetRotation() * weight );
	result.m_cameraFov = from.m_cameraFov * ( 1.0f - weight ) + to.m_cameraFov * weight;
	result.m_cameraZoom = from.m_cameraZoom * ( 1.0f - weight ) + to.m_cameraZoom * weight;
	result.m_dofBlurDistFar = from.m_dofBlurDistFar * ( 1.0f - weight ) + to.m_dofBlurDistFar * weight;
	result.m_dofBlurDistNear = from.m_dofBlurDistNear * ( 1.0f - weight ) + to.m_dofBlurDistNear * weight;
	result.m_dofFocusDistFar = from.m_dofFocusDistFar * ( 1.0f - weight ) + to.m_dofFocusDistFar * weight;
	result.m_dofFocusDistNear = from.m_dofFocusDistNear * ( 1.0f - weight ) + to.m_dofFocusDistNear * weight;
	result.m_dofIntensity = from.m_dofIntensity * ( 1.0f - weight ) + to.m_dofIntensity * weight;

	result.m_targetEyesLS = from.m_targetEyesLS * ( 1.0f - weight ) + to.m_targetEyesLS * weight;
	result.m_sourceEyesHeigth = from.m_sourceEyesHeigth * ( 1.0f - weight ) + to.m_sourceEyesHeigth * weight;
}

void CStorySceneCameraBlendEvent::LazyInitialization( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer ) const
{
	TDynArray< Float >& timesRef = data[ i_cachedTimes ];
	TDynArray< Int32 >& indicesRef = data[ i_cachedIndices ];

	const Uint32 numKyes = m_keys.Size();

	timesRef.Reserve( numKyes );
	indicesRef.Reserve( numKyes );

	for ( Uint32 i=0; i<numKyes; ++i )
	{
		Int32 index = -1;
		const CStorySceneEvent* evt = scenePlayer->FindEventByGUID( m_keys[ i ], &index );
		if ( evt )
		{
			SCENE_ASSERT( evt->GetClass()->IsA< CStorySceneEventCamera >() );
			SCENE_ASSERT( index != -1 );

			if ( evt->GetClass()->IsA< CStorySceneEventCamera >() )
			{
				Float s = 0.f;
				Float e = 0.f;
				Float d = 0.f;

				evt->GetEventInstanceData( data, s, e, d );

				timesRef.PushBack( s );
				indicesRef.PushBack( index );
			}
		}
	}
}

#ifdef DEBUG_SCENES_2
#pragma optimize("",on)
#endif
