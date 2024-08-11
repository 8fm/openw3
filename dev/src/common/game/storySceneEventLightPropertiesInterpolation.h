// Copyright © 2014 CD Projekt Red. All Rights Reserved.

#pragma once

#include "interpolationEventClassGenerator.h"

// forward declaration of key class is sufficient
class CStorySceneEventLightProperties;

// =================================================================================================
namespace StoryScene {
	// =================================================================================================

	template <>
	class InterpolationTraits< CStorySceneEventLightProperties >
	{
	public:

		enum Channel
		{
			enabled,

			colorR,
			colorG,
			colorB,

			radius,
			brightness,
			attenuation,

			ambientLevel,
			marginFactor,

			innerAngle,
			outerAngle, 
			softness,

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
		typedef InterpolationChannelRange< Channel::posX, Channel::rotYaw > VolatileChannels;
		typedef InterpolationChannelRange< Channel::colorR, Channel::softness > StaticChannels;

		static void GetChannels( const CStorySceneEventLightProperties& keyEvent, ChannelArray& outChannels, VolatileChannels, CSceneEventFunctionSimpleArgs& args );
		static void GetChannels( const CStorySceneEventLightProperties& keyEvent, ChannelArray& outChannels, StaticChannels, CSceneEventFunctionSimpleArgs& args );

		static EInterpolationChannelType GetChannelType( Uint32 channel );
		static EInterpolationMethod GetDefaultInterpolationMethod( Uint32 channel );
		static Bool GetDefaultVolatilityState( const CStorySceneEventLightProperties& keyEvent );

		static void OnProcess( const CStorySceneEvent* interpolationEvent, const CStorySceneEventLightProperties& sourceKeyEvent, const ChannelArray& interpolationResult, CSceneEventFunctionArgs& args );
	};

	// =================================================================================================
} // namespace StoryScene
// =================================================================================================

RED_SCENE_INTERPOLATION_EVENT_GENERATE_CLASS_HEADER( CStorySceneEventLightPropertiesInterpolation, CStorySceneEventLightProperties )
