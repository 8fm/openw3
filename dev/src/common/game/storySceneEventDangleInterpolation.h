#pragma once

#include "interpolationEventClassGenerator.h"

// forward declaration of key class is sufficient
class CStorySceneDisableDangleEvent;
class CStorySceneDanglesShakeEvent;

// =================================================================================================
namespace StoryScene {
	// =================================================================================================

	template <>
	class InterpolationTraits< CStorySceneDisableDangleEvent >
	{
	public:

		enum Channel
		{
			weight,
			numChannels		// special
		};

		typedef Float ChannelArray[ numChannels ];

		// define channel ranges
		typedef InterpolationChannelRangeNone VolatileChannels;
		typedef InterpolationChannelRange< Channel::weight, Channel::weight > StaticChannels;

		static void GetChannels( const CStorySceneDisableDangleEvent& keyEvent, ChannelArray& outChannels, VolatileChannels, CSceneEventFunctionSimpleArgs& args );
		static void GetChannels( const CStorySceneDisableDangleEvent& keyEvent, ChannelArray& outChannels, StaticChannels, CSceneEventFunctionSimpleArgs& args );

		static EInterpolationChannelType GetChannelType( Uint32 channel );
		static EInterpolationMethod GetDefaultInterpolationMethod( Uint32 channel );
		static Bool GetDefaultVolatilityState( const CStorySceneDisableDangleEvent& keyEvent );

		static void OnProcess( const CStorySceneEvent* interpolationEvent, const CStorySceneDisableDangleEvent& sourceKeyEvent, const ChannelArray& interpolationResult, CSceneEventFunctionArgs& args );
	};

	//////////////////////////////////////////////////////////////////////////

	template <>
	class InterpolationTraits< CStorySceneDanglesShakeEvent >
	{
	public:

		enum Channel
		{
			weight,
			numChannels		// special
		};

		typedef Float ChannelArray[ numChannels ];

		// define channel ranges
		typedef InterpolationChannelRangeNone VolatileChannels;
		typedef InterpolationChannelRange< Channel::weight, Channel::weight > StaticChannels;

		static void GetChannels( const CStorySceneDanglesShakeEvent& keyEvent, ChannelArray& outChannels, VolatileChannels, CSceneEventFunctionSimpleArgs& args );
		static void GetChannels( const CStorySceneDanglesShakeEvent& keyEvent, ChannelArray& outChannels, StaticChannels, CSceneEventFunctionSimpleArgs& args );

		static EInterpolationChannelType GetChannelType( Uint32 channel );
		static EInterpolationMethod GetDefaultInterpolationMethod( Uint32 channel );
		static Bool GetDefaultVolatilityState( const CStorySceneDanglesShakeEvent& keyEvent );

		static void OnProcess( const CStorySceneEvent* interpolationEvent, const CStorySceneDanglesShakeEvent& sourceKeyEvent, const ChannelArray& interpolationResult, CSceneEventFunctionArgs& args );
	};

	// =================================================================================================
} // namespace StoryScene
// =================================================================================================

RED_SCENE_INTERPOLATION_EVENT_GENERATE_CLASS_HEADER( CStorySceneEventDangleDisablingInterpolation, CStorySceneDisableDangleEvent )
RED_SCENE_INTERPOLATION_EVENT_GENERATE_CLASS_HEADER( CStorySceneDanglesShakeEventInterpolation, CStorySceneDanglesShakeEvent )
