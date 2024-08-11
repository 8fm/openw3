// Copyright © 2014 CD Projekt Red. All Rights Reserved.

#include "build.h"
#include "storySceneEventPropPlacementInterpolation.h"
#include "storySceneEventsCollector.h"
#include "storyScenePlayer.h"
#include "storySceneEventScenePropPlacement.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "storySceneDirector.h"

// =================================================================================================
namespace StoryScene {
// =================================================================================================

EInterpolationChannelType InterpolationTraits< CStorySceneEventScenePropPlacement >::GetChannelType( Uint32 channel )
{
	// all channels are normal
	return EInterpolationChannelType::ICT_Normal;
}

/*
Returns default interpolation method for specified channel.

Later this can be changed for each key on each channel.
*/
EInterpolationMethod InterpolationTraits< CStorySceneEventScenePropPlacement >::GetDefaultInterpolationMethod( Uint32 channel )
{
	return EInterpolationMethod::IM_Linear;
}

/*
Gets volatile channels of specified key event.

\param keyEvent Key event whose channels to get.
\param outChannels (out) Array of floats to which to store channel values. Size of this array is equal to number of defined channels.
*/
void InterpolationTraits< CStorySceneEventScenePropPlacement >::GetChannels( const CStorySceneEventScenePropPlacement& keyEvent, ChannelArray& outChannels, VolatileChannels, CSceneEventFunctionSimpleArgs& args )
{
	// no volatile channels defined
}

/*
Gets static channels of specified key event.

\param keyEvent Key event whose channels to get.
\param outChannels (out) Array of floats to which to store channel values. Size of this array is equal to number of defined channels.
*/
void InterpolationTraits< CStorySceneEventScenePropPlacement >::GetChannels( const CStorySceneEventScenePropPlacement& keyEvent, ChannelArray& outChannels, StaticChannels, CSceneEventFunctionSimpleArgs& args )
{
	Vector t = keyEvent.GetTransformRef().GetPosition();
	EulerAngles r = keyEvent.GetTransformRef().GetRotation();
	
	Uint32 rotCyclesPitch = 0;
	Uint32 rotCyclesRoll = 0;
	Uint32 rotCyclesYaw = 0;

	keyEvent.GetRotationCycles( rotCyclesPitch, rotCyclesRoll, rotCyclesYaw );

	outChannels[ Channel::posX ] = t.X;
	outChannels[ Channel::posY ] = t.Y;
	outChannels[ Channel::posZ ] = t.Z;

	outChannels[ Channel::rotRoll ] = r.Roll + (Float)rotCyclesRoll*360.f;
	outChannels[ Channel::rotPitch ] = r.Pitch + (Float)rotCyclesPitch*360.f;
	outChannels[ Channel::rotYaw ] = r.Yaw + (Float)rotCyclesYaw*360.f;
}

/*
Returns whether key should be volatile or not by default.
*/
Bool InterpolationTraits< CStorySceneEventScenePropPlacement >::GetDefaultVolatilityState( const CStorySceneEventScenePropPlacement& keyEvent )
{
	return false;
}

/*
\param interpolateEvent Story scene interpolate event.
\param sourceKey Source key + interpolation result = resulting object stat.
\param interpolationResult Interpolation result, i.e. values of all channels. It's guaranteed to have numChannels elements.
\param args Original args passed to interpolate event OnProcess().
*/
void InterpolationTraits< CStorySceneEventScenePropPlacement >::OnProcess( const CStorySceneEvent* interpolationEvent, const CStorySceneEventScenePropPlacement& sourceKeyEvent, const ChannelArray& interpolationResult, CSceneEventFunctionArgs& args )
{
	SCENE_ASSERT( interpolationEvent );
	SCENE_ASSERT( args.m_scenePlayer )
	// TODO: clean up after refactoring

	if( sourceKeyEvent.GetSubject() )
	{
		StorySceneEventsCollector::PropPlacement evt( interpolationEvent, sourceKeyEvent.GetSubject() );

		EngineTransform propLSTransf;
		propLSTransf.SetPosition( Vector( interpolationResult[ Channel::posX ], interpolationResult[ Channel::posY ], interpolationResult[ Channel::posZ ] ) );			// For some reason the interpolationResult is corrupted here
		propLSTransf.SetRotation( EulerAngles( interpolationResult[ Channel::rotRoll ], interpolationResult[ Channel::rotPitch ], interpolationResult[ Channel::rotYaw ] ) );

		evt.m_eventTimeAbs = args.m_timeInfo.m_timeAbs;
		evt.m_eventTimeLocal = args.m_timeInfo.m_timeLocal;

		Matrix sceneL2W, propLS;
		args.m_scenePlayer->GetSceneDirector()->GetCurrentScenePlacement().CalcLocalToWorld( sceneL2W );
		propLSTransf.CalcLocalToWorld( propLS );					

		evt.m_placementWS = EngineTransform( propLS * sceneL2W );
		args.m_collector.AddEvent( evt );
	}
}

// =================================================================================================
} // namespace StoryScene
// =================================================================================================

RED_SCENE_INTERPOLATION_EVENT_GENERATE_CLASS_IMPL( CStorySceneEventPropPlacementInterpolation, CStorySceneEventScenePropPlacement )
