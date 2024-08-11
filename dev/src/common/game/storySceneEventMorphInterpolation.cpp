#include "build.h"
#include "storySceneEventMorphInterpolation.h"
#include "storySceneEventsCollector.h"
#include "storySceneEventEquipItem.h"
#include "storyScenePlayer.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "gameTypeRegistry.h"

// =================================================================================================
namespace StoryScene {
	// =================================================================================================

	EInterpolationChannelType InterpolationTraits< CStorySceneMorphEvent >::GetChannelType( Uint32 channel )
	{
		// all channels are normal
		return EInterpolationChannelType::ICT_Normal;
	}

	/*
	Returns default interpolation method for specified channel.

	Later this can be changed for each key on each channel.
	*/
	EInterpolationMethod InterpolationTraits< CStorySceneMorphEvent >::GetDefaultInterpolationMethod( Uint32 channel )
	{
		return EInterpolationMethod::IM_Bezier;
	}

	/*
	Gets volatile channels of specified key event.

	\param keyEvent Key event whose channels to get.
	\param outChannels (out) Array of floats to which to store channel values. Size of this array is equal to number of defined channels.
	*/
	void InterpolationTraits< CStorySceneMorphEvent >::GetChannels( const CStorySceneMorphEvent& keyEvent, ChannelArray& outChannels, VolatileChannels, CSceneEventFunctionSimpleArgs& args )
	{
		// no volatile channels defined
	}

	/*
	Gets static channels of specified key event.

	\param keyEvent Key event whose channels to get.
	\param outChannels (out) Array of floats to which to store channel values. Size of this array is equal to number of defined channels.
	*/
	void InterpolationTraits< CStorySceneMorphEvent >::GetChannels( const CStorySceneMorphEvent& keyEvent, ChannelArray& outChannels, StaticChannels, CSceneEventFunctionSimpleArgs& args )
	{
		outChannels[ Channel::weight ] = keyEvent.GetWeight();
	}

	/*
	Returns whether key should be volatile or not by default.
	*/
	Bool InterpolationTraits< CStorySceneMorphEvent >::GetDefaultVolatilityState( const CStorySceneMorphEvent& keyEvent )
	{
		return false;
	}

	/*


	\param interpolateEvent Story scene interpolate event.
	\param sourceKey Source key + interpolation result = resulting object stat.
	\param interpolationResult Interpolation result, i.e. values of all channels. It's guaranteed to have numChannels elements.
	\param args Original args passed to interpolate event OnProcess().
	*/
	void InterpolationTraits< CStorySceneMorphEvent >::OnProcess( const CStorySceneEvent* interpolationEvent, const CStorySceneMorphEvent& sourceKeyEvent, const ChannelArray& interpolationResult, CSceneEventFunctionArgs& args )
	{
		ASSERT( interpolationEvent );

		if( sourceKeyEvent.GetSubject() != CName::NONE )
		{
			StorySceneEventsCollector::ActorMorph evt( interpolationEvent, sourceKeyEvent.GetSubject() );
			evt.m_eventTimeAbs = args.m_timeInfo.m_timeAbs;
			evt.m_eventTimeLocal = args.m_timeInfo.m_timeLocal;
			evt.m_weight = interpolationResult[ Channel::weight ];
			evt.m_morphComponentId = sourceKeyEvent.GetMorphComponentId();
			args.m_collector.AddEvent( evt );
		}
	}

	// =================================================================================================
} // namespace StoryScene
// =================================================================================================

RED_SCENE_INTERPOLATION_EVENT_GENERATE_CLASS_IMPL( CStorySceneEventMorphInterpolation, CStorySceneMorphEvent )
