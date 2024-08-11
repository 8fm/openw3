// Copyright © 2014 CD Projekt Red. All Rights Reserved.

#include "build.h"
#include "storySceneEventClothDisablingInterpolation.h"
#include "storySceneEventsCollector.h"
#include "storyScenePlayer.h"
#include "storySceneEventCloth.h"
#include "../core/instanceDataLayoutCompiler.h"

// =================================================================================================
namespace StoryScene {
// =================================================================================================

EInterpolationChannelType InterpolationTraits< CStorySceneDisablePhysicsClothEvent >::GetChannelType( Uint32 channel )
{
	// all channels are normal
	return EInterpolationChannelType::ICT_Normal;
}

/*
Returns default interpolation method for specified channel.

Later this can be changed for each key on each channel.
*/
EInterpolationMethod InterpolationTraits< CStorySceneDisablePhysicsClothEvent >::GetDefaultInterpolationMethod( Uint32 channel )
{
	return EInterpolationMethod::IM_Bezier;
}

/*
Gets volatile channels of specified key event.

\param keyEvent Key event whose channels to get.
\param outChannels (out) Array of floats to which to store channel values. Size of this array is equal to number of defined channels.
*/
void InterpolationTraits< CStorySceneDisablePhysicsClothEvent >::GetChannels( const CStorySceneDisablePhysicsClothEvent& keyEvent, ChannelArray& outChannels, VolatileChannels, CSceneEventFunctionSimpleArgs& args )
{
	// no volatile channels defined
}

/*
Gets static channels of specified key event.

\param keyEvent Key event whose channels to get.
\param outChannels (out) Array of floats to which to store channel values. Size of this array is equal to number of defined channels.
*/
void InterpolationTraits< CStorySceneDisablePhysicsClothEvent >::GetChannels( const CStorySceneDisablePhysicsClothEvent& keyEvent, ChannelArray& outChannels, StaticChannels, CSceneEventFunctionSimpleArgs& args )
{
	outChannels[ Channel::weight ] = keyEvent.GetWeight();
}

/*
Returns whether key should be volatile or not by default.
*/
Bool InterpolationTraits< CStorySceneDisablePhysicsClothEvent >::GetDefaultVolatilityState( const CStorySceneDisablePhysicsClothEvent& keyEvent )
{
	return false;
}

/*


\param interpolateEvent Story scene interpolate event.
\param sourceKey Source key + interpolation result = resulting object stat.
\param interpolationResult Interpolation result, i.e. values of all channels. It's guaranteed to have numChannels elements.
\param args Original args passed to interpolate event OnProcess().
*/
void InterpolationTraits< CStorySceneDisablePhysicsClothEvent >::OnProcess( const CStorySceneEvent* interpolationEvent, const CStorySceneDisablePhysicsClothEvent& sourceKeyEvent, const ChannelArray& interpolationResult, CSceneEventFunctionArgs& args )
{
	ASSERT( interpolationEvent );

	if( sourceKeyEvent.GetSubject() != CName::NONE )
	{
		StorySceneEventsCollector::ActorDisablePhysicsCloth evt( interpolationEvent, sourceKeyEvent.GetSubject() );
		evt.m_eventTimeAbs = args.m_timeInfo.m_timeAbs;
		evt.m_eventTimeLocal = args.m_timeInfo.m_timeLocal;
		evt.m_weight = interpolationResult[ Channel::weight ];

		args.m_collector.AddEvent( evt );
	}
}

// =================================================================================================
} // namespace StoryScene
// =================================================================================================

RED_SCENE_INTERPOLATION_EVENT_GENERATE_CLASS_IMPL( CStorySceneEventClothDisablingInterpolation, CStorySceneDisablePhysicsClothEvent )
