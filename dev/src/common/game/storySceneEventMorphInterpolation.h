// Copyright © 2014 CD Projekt Red. All Rights Reserved.

#pragma once

#include "interpolationEventClassGenerator.h"

// forward declaration of key class is sufficient
class CStorySceneDisablePhysicsClothEvent;

// =================================================================================================
namespace StoryScene {
	// =================================================================================================

	template <>
	class InterpolationTraits< CStorySceneMorphEvent >
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

		static void GetChannels( const CStorySceneMorphEvent& keyEvent, ChannelArray& outChannels, VolatileChannels, CSceneEventFunctionSimpleArgs& args );
		static void GetChannels( const CStorySceneMorphEvent& keyEvent, ChannelArray& outChannels, StaticChannels, CSceneEventFunctionSimpleArgs& args );

		static EInterpolationChannelType GetChannelType( Uint32 channel );
		static EInterpolationMethod GetDefaultInterpolationMethod( Uint32 channel );
		static Bool GetDefaultVolatilityState( const CStorySceneMorphEvent& keyEvent );

		static void OnProcess( const CStorySceneEvent* interpolationEvent, const CStorySceneMorphEvent& sourceKeyEvent, const ChannelArray& interpolationResult, CSceneEventFunctionArgs& args );
	};

	// =================================================================================================
} // namespace StoryScene
// =================================================================================================

RED_SCENE_INTERPOLATION_EVENT_GENERATE_CLASS_HEADER( CStorySceneEventMorphInterpolation, CStorySceneMorphEvent )
