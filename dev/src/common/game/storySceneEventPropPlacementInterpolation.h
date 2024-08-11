// Copyright © 2014 CD Projekt Red. All Rights Reserved.

#pragma once

#include "interpolationEventClassGenerator.h"

// forward declaration of key class is sufficient
class CStorySceneEventWorldPropPlacement;

// =================================================================================================
namespace StoryScene {
// =================================================================================================

template <>
class InterpolationTraits< CStorySceneEventScenePropPlacement >
{
public:

	enum Channel
	{
		posX,
		posY,
		posZ,

		rotRoll,
		rotPitch,
		rotYaw,

		numChannels
	};

	typedef Float ChannelArray[ numChannels ];

	// define channel ranges
	typedef InterpolationChannelRangeNone VolatileChannels;
	typedef InterpolationChannelRange< Channel::posX, Channel::rotYaw > StaticChannels;

	static void GetChannels( const CStorySceneEventScenePropPlacement& keyEvent, ChannelArray& outChannels, VolatileChannels, CSceneEventFunctionSimpleArgs& args );
	static void GetChannels( const CStorySceneEventScenePropPlacement& keyEvent, ChannelArray& outChannels, StaticChannels, CSceneEventFunctionSimpleArgs& args );

	static EInterpolationChannelType GetChannelType( Uint32 channel );
	static EInterpolationMethod GetDefaultInterpolationMethod( Uint32 channel );
	static Bool GetDefaultVolatilityState( const CStorySceneEventScenePropPlacement& keyEvent );

	static void OnProcess( const CStorySceneEvent* interpolationEvent, const CStorySceneEventScenePropPlacement& sourceKeyEvent, const ChannelArray& interpolationResult, CSceneEventFunctionArgs& args );
};

// =================================================================================================
} // namespace StoryScene
// =================================================================================================

RED_SCENE_INTERPOLATION_EVENT_GENERATE_CLASS_HEADER( CStorySceneEventPropPlacementInterpolation, CStorySceneEventScenePropPlacement )
