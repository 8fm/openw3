// Copyright © 2014 CD Projekt Red. All Rights Reserved.

#pragma once

#include "interpolationEventClassGenerator.h"

// forward declaration of key class is sufficient
class CStorySceneEventOverridePlacement;

// =================================================================================================
namespace StoryScene {
// =================================================================================================

template <>
class InterpolationTraits< CStorySceneEventOverridePlacement >
{
public:

	enum Channel
	{
		posX,
		posY,
		posZ,
		rotPitch,
		rotYaw,
		rotRoll,
		numChannels
	};

	typedef Float ChannelArray[ numChannels ];
	
	// define channel ranges
	typedef InterpolationChannelRangeNone VolatileChannels;
	typedef InterpolationChannelRange< Channel::posX, Channel::rotRoll > StaticChannels;

	static void GetChannels( const CStorySceneEventOverridePlacement& keyEvent, ChannelArray& outChannels, VolatileChannels, CSceneEventFunctionSimpleArgs& args );
	static void GetChannels( const CStorySceneEventOverridePlacement& keyEvent, ChannelArray& outChannels, StaticChannels, CSceneEventFunctionSimpleArgs& args );

	static EInterpolationChannelType GetChannelType( Uint32 channel );
	static EInterpolationMethod GetDefaultInterpolationMethod( Uint32 channel );
	static Bool GetDefaultVolatilityState( const CStorySceneEventOverridePlacement& keyEvent );

	static void OnProcess( const CStorySceneEvent* interpolationEvent, const CStorySceneEventOverridePlacement& sourceKeyEvent, const ChannelArray& interpolationResult, CSceneEventFunctionArgs& args );
};

// =================================================================================================
} // namespace StoryScene
// =================================================================================================

RED_SCENE_INTERPOLATION_EVENT_GENERATE_CLASS_HEADER( CStorySceneEventPlacementInterpolation, CStorySceneEventOverridePlacement )
