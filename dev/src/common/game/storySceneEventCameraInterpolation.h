// Copyright © 2014 CD Projekt Red. All Rights Reserved.

#pragma once

#include "interpolationEventClassGenerator.h"

// forward declaration of key class is sufficient
class CStorySceneEventCamera;

// =================================================================================================
namespace StoryScene {
// =================================================================================================

template <>
class InterpolationTraits< CStorySceneEventCamera >
{
public:

	enum Channel
	{
		posX,
		posY,
		posZ,
		rotX,
		rotY,
		rotZ,
		fov,
		zoom,
		dofBlurDistFar,
		dofBlurDistNear,
		dofFocusDistFar,
		dofFocusDistNear,
		dofIntensity,
		planeInFocus,
		bokehSizeMultiplier,

		numChannels		// special
	};

	typedef Float ChannelArray[ numChannels ];

	// define channel ranges
	typedef InterpolationChannelRange< Channel::posX, Channel::rotZ > VolatileChannels;
	typedef InterpolationChannelRange< Channel::fov, Channel::bokehSizeMultiplier > StaticChannels;

	static void GetChannels( const CStorySceneEventCamera& keyEvent, ChannelArray& outChannels, VolatileChannels, CSceneEventFunctionSimpleArgs& args );
	static void GetChannels( const CStorySceneEventCamera& keyEvent, ChannelArray& outChannels, StaticChannels, CSceneEventFunctionSimpleArgs& args );

	static EInterpolationChannelType GetChannelType( Uint32 channel );
	static EInterpolationMethod GetDefaultInterpolationMethod( Uint32 channel );
	static Bool GetDefaultVolatilityState( const CStorySceneEventCamera& keyEvent );

	static void OnProcess( const CStorySceneEvent* interpolationEvent, const CStorySceneEventCamera& sourceKeyEvent, const ChannelArray& interpolationResult, CSceneEventFunctionArgs& args );
};

// =================================================================================================
} // namespace StoryScene
// =================================================================================================

RED_SCENE_INTERPOLATION_EVENT_GENERATE_CLASS_HEADER( CStorySceneEventCameraInterpolation, CStorySceneEventCamera )
