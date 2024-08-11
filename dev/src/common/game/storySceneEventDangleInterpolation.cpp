#include "build.h"
#include "storySceneEventDangleInterpolation.h"
#include "storySceneEventsCollector.h"
#include "storyScenePlayer.h"
#include "storySceneEventDangle.h"
#include "../core/instanceDataLayoutCompiler.h"

// =================================================================================================
namespace StoryScene {
	// =================================================================================================

	EInterpolationChannelType InterpolationTraits< CStorySceneDisableDangleEvent >::GetChannelType( Uint32 channel )
	{
		// all channels are normal
		return EInterpolationChannelType::ICT_Normal;
	}

	/*
	Returns default interpolation method for specified channel.

	Later this can be changed for each key on each channel.
	*/
	EInterpolationMethod InterpolationTraits< CStorySceneDisableDangleEvent >::GetDefaultInterpolationMethod( Uint32 channel )
	{
		return EInterpolationMethod::IM_Bezier;
	}

	/*
	Gets volatile channels of specified key event.

	\param keyEvent Key event whose channels to get.
	\param outChannels (out) Array of floats to which to store channel values. Size of this array is equal to number of defined channels.
	*/
	void InterpolationTraits< CStorySceneDisableDangleEvent >::GetChannels( const CStorySceneDisableDangleEvent& keyEvent, ChannelArray& outChannels, VolatileChannels, CSceneEventFunctionSimpleArgs& args )
	{
		// no volatile channels defined
	}

	/*
	Gets static channels of specified key event.

	\param keyEvent Key event whose channels to get.
	\param outChannels (out) Array of floats to which to store channel values. Size of this array is equal to number of defined channels.
	*/
	void InterpolationTraits< CStorySceneDisableDangleEvent >::GetChannels( const CStorySceneDisableDangleEvent& keyEvent, ChannelArray& outChannels, StaticChannels, CSceneEventFunctionSimpleArgs& args )
	{
		outChannels[ Channel::weight ] = keyEvent.GetWeight();
	}

	/*
	Returns whether key should be volatile or not by default.
	*/
	Bool InterpolationTraits< CStorySceneDisableDangleEvent >::GetDefaultVolatilityState( const CStorySceneDisableDangleEvent& keyEvent )
	{
		return false;
	}

	/*


	\param interpolateEvent Story scene interpolate event.
	\param sourceKey Source key + interpolation result = resulting object stat.
	\param interpolationResult Interpolation result, i.e. values of all channels. It's guaranteed to have numChannels elements.
	\param args Original args passed to interpolate event OnProcess().
	*/
	void InterpolationTraits< CStorySceneDisableDangleEvent >::OnProcess( const CStorySceneEvent* interpolationEvent, const CStorySceneDisableDangleEvent& sourceKeyEvent, const ChannelArray& interpolationResult, CSceneEventFunctionArgs& args )
	{
		ASSERT( interpolationEvent );

		if( sourceKeyEvent.GetSubject() != CName::NONE )
		{
			StorySceneEventsCollector::ActorDisableDangle evt( interpolationEvent, sourceKeyEvent.GetSubject() );
			evt.m_eventTimeAbs = args.m_timeInfo.m_timeAbs;
			evt.m_eventTimeLocal = args.m_timeInfo.m_timeLocal;
			evt.m_weight = interpolationResult[ Channel::weight ];

			args.m_collector.AddEvent( evt );
		}
	}

	//////////////////////////////////////////////////////////////////////////

	EInterpolationChannelType InterpolationTraits< CStorySceneDanglesShakeEvent >::GetChannelType( Uint32 channel )
	{
		// all channels are normal
		return EInterpolationChannelType::ICT_Normal;
	}

	/*
	Returns default interpolation method for specified channel.

	Later this can be changed for each key on each channel.
	*/
	EInterpolationMethod InterpolationTraits< CStorySceneDanglesShakeEvent >::GetDefaultInterpolationMethod( Uint32 channel )
	{
		return EInterpolationMethod::IM_Bezier;
	}

	/*
	Gets volatile channels of specified key event.

	\param keyEvent Key event whose channels to get.
	\param outChannels (out) Array of floats to which to store channel values. Size of this array is equal to number of defined channels.
	*/
	void InterpolationTraits< CStorySceneDanglesShakeEvent >::GetChannels( const CStorySceneDanglesShakeEvent& keyEvent, ChannelArray& outChannels, VolatileChannels, CSceneEventFunctionSimpleArgs& args )
	{
		// no volatile channels defined
	}

	/*
	Gets static channels of specified key event.

	\param keyEvent Key event whose channels to get.
	\param outChannels (out) Array of floats to which to store channel values. Size of this array is equal to number of defined channels.
	*/
	void InterpolationTraits< CStorySceneDanglesShakeEvent >::GetChannels( const CStorySceneDanglesShakeEvent& keyEvent, ChannelArray& outChannels, StaticChannels, CSceneEventFunctionSimpleArgs& args )
	{
		outChannels[ Channel::weight ] = keyEvent.GetFactor();
	}

	/*
	Returns whether key should be volatile or not by default.
	*/
	Bool InterpolationTraits< CStorySceneDanglesShakeEvent >::GetDefaultVolatilityState( const CStorySceneDanglesShakeEvent& keyEvent )
	{
		return false;
	}

	/*


	\param interpolateEvent Story scene interpolate event.
	\param sourceKey Source key + interpolation result = resulting object stat.
	\param interpolationResult Interpolation result, i.e. values of all channels. It's guaranteed to have numChannels elements.
	\param args Original args passed to interpolate event OnProcess().
	*/
	void InterpolationTraits< CStorySceneDanglesShakeEvent >::OnProcess( const CStorySceneEvent* interpolationEvent, const CStorySceneDanglesShakeEvent& sourceKeyEvent, const ChannelArray& interpolationResult, CSceneEventFunctionArgs& args )
	{
		ASSERT( interpolationEvent );

		if( sourceKeyEvent.GetSubject() != CName::NONE )
		{
			StorySceneEventsCollector::ActorDanglesShake evt( interpolationEvent, sourceKeyEvent.GetSubject() );
			evt.m_eventTimeAbs = args.m_timeInfo.m_timeAbs;
			evt.m_eventTimeLocal = args.m_timeInfo.m_timeLocal;
			evt.m_factor = interpolationResult[ Channel::weight ];

			args.m_collector.AddEvent( evt );
		}
	}

	// =================================================================================================
} // namespace StoryScene
// =================================================================================================

RED_SCENE_INTERPOLATION_EVENT_GENERATE_CLASS_IMPL( CStorySceneEventDangleDisablingInterpolation, CStorySceneDisableDangleEvent )
RED_SCENE_INTERPOLATION_EVENT_GENERATE_CLASS_IMPL( CStorySceneDanglesShakeEventInterpolation, CStorySceneDanglesShakeEvent )
