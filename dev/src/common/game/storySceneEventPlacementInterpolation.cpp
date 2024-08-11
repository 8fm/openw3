// Copyright © 2014 CD Projekt Red. All Rights Reserved.

#include "build.h"
#include "storySceneEventPlacementInterpolation.h"
#include "storySceneEventsCollector.h"
#include "storyScenePlayer.h"
#include "storySceneEventOverridePlacement.h"
#include "../core/instanceDataLayoutCompiler.h"

// =================================================================================================
namespace StoryScene {
// =================================================================================================

EInterpolationChannelType InterpolationTraits< CStorySceneEventOverridePlacement >::GetChannelType( Uint32 channel )
{	
	// all channels are normal
	switch( channel )
	{
	case rotYaw:
	case rotPitch:
	case rotRoll: return EInterpolationChannelType::ICT_AngleInDegrees;
	default: return EInterpolationChannelType::ICT_Normal;
	}
}

/*
Returns default interpolation method for specified channel.

Later this can be changed for each key on each channel.
*/
EInterpolationMethod InterpolationTraits< CStorySceneEventOverridePlacement >::GetDefaultInterpolationMethod( Uint32 channel )
{
	return EInterpolationMethod::IM_Bezier;
}

/*
Gets volatile channels of specified key event.

\param keyEvent Key event whose channels to get.
\param outChannels (out) Array of floats to which to store channel values. Size of this array is equal to number of defined channels.
*/
void InterpolationTraits< CStorySceneEventOverridePlacement >::GetChannels( const CStorySceneEventOverridePlacement& keyEvent, ChannelArray& outChannels, VolatileChannels, CSceneEventFunctionSimpleArgs& args )
{
	// no volatile channels defined
}

/*
Gets static channels of specified key event.

\param keyEvent Key event whose channels to get.
\param outChannels (out) Array of floats to which to store channel values. Size of this array is equal to number of defined channels.
*/
void InterpolationTraits< CStorySceneEventOverridePlacement >::GetChannels( const CStorySceneEventOverridePlacement& keyEvent, ChannelArray& outChannels, StaticChannels, CSceneEventFunctionSimpleArgs& args )
{
	Vector t = keyEvent.GetTransformRef().GetPosition();
	EulerAngles r = keyEvent.GetTransformRef().GetRotation();

	outChannels[ Channel::posX ] = t.X;
	outChannels[ Channel::posY ] = t.Y;
	outChannels[ Channel::posZ ] = t.Z;

	outChannels[ Channel::rotYaw ] = r.Yaw;
	outChannels[ Channel::rotPitch ] = r.Pitch;
	outChannels[ Channel::rotRoll ] = r.Roll;
}

/*
Returns whether key should be volatile or not by default.
*/
Bool InterpolationTraits< CStorySceneEventOverridePlacement >::GetDefaultVolatilityState( const CStorySceneEventOverridePlacement& keyEvent )
{
	return false;
}

/*


\param interpolateEvent Story scene interpolate event.
\param sourceKey Source key + interpolation result = resulting object stat.
\param interpolationResult Interpolation result, i.e. values of all channels. It's guaranteed to have numChannels elements.
\param args Original args passed to interpolate event OnProcess().
*/
void InterpolationTraits< CStorySceneEventOverridePlacement >::OnProcess( const CStorySceneEvent* interpolationEvent, const CStorySceneEventOverridePlacement& sourceKeyEvent, const ChannelArray& interpolationResult, CSceneEventFunctionArgs& args )
{
	ASSERT( interpolationEvent );

	StorySceneEventsCollector::ActorPlacement evt( interpolationEvent, sourceKeyEvent.GetSubject() );
	evt.m_eventTimeAbs = args.m_timeInfo.m_timeAbs;
	evt.m_eventTimeLocal = args.m_timeInfo.m_timeLocal;
	evt.m_sceneTransformWS = args.m_scenePlayer->GetSceneDirector()->GetCurrentScenePlacement(); // TODO

	// combine sourceKeyEvent state and interpolation result to obtain final result
	evt.m_placementSS = sourceKeyEvent.GetTransformRef();
	Vector interpolatedPosition( interpolationResult[ Channel::posX ], interpolationResult[ Channel::posY ], interpolationResult[ Channel::posZ ] );
	EulerAngles interpolatedRotation( interpolationResult[ Channel::rotRoll ], interpolationResult[ Channel::rotPitch ], interpolationResult[ Channel::rotYaw ] );
	evt.m_placementSS.SetPosition( interpolatedPosition );
	evt.m_placementSS.SetRotation( interpolatedRotation );
	args.m_collector.AddEvent( evt );
}

// =================================================================================================
} // namespace StoryScene
// =================================================================================================

RED_SCENE_INTERPOLATION_EVENT_GENERATE_CLASS_IMPL( CStorySceneEventPlacementInterpolation, CStorySceneEventOverridePlacement )
