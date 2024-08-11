#pragma once
#include "interpolationEventClassGenerator.h"

class CStorySceneEventCameraLight;


namespace StoryScene {
	
	namespace CameraLightBlend
	{
		const SCameraLightsModifiersSetup& BlendCameraLight( const SCameraLightsModifiersSetup& srcData, const  SCameraLightsModifiersSetup& dstData, Float progress, Float dayTime );
	}

template <>
class InterpolationTraits< CStorySceneEventCameraLight >
{
public:

	enum Channel
	{
		gameplayLightWeight,
		sceneLightWeight,
		numChannels
	};

	typedef Float ChannelArray[ numChannels ];

	// define channel ranges
	typedef InterpolationChannelRangeNone VolatileChannels;
	typedef InterpolationChannelRange< Channel::gameplayLightWeight, Channel::sceneLightWeight > StaticChannels;

	static void GetChannels( const CStorySceneEventCameraLight& keyEvent, ChannelArray& outChannels, VolatileChannels, CSceneEventFunctionSimpleArgs& args );
	static void GetChannels( const CStorySceneEventCameraLight& keyEvent, ChannelArray& outChannels, StaticChannels, CSceneEventFunctionSimpleArgs& args );

	static EInterpolationChannelType GetChannelType( Uint32 channel );
	static EInterpolationMethod GetDefaultInterpolationMethod( Uint32 channel );
	static Bool GetDefaultVolatilityState( const CStorySceneEventCameraLight& keyEvent );

	static void OnProcess( const CStorySceneEvent* interpolationEvent, const CStorySceneEventCameraLight& sourceKeyEvent, const ChannelArray& interpolationResult, CSceneEventFunctionArgs& args );
};

} 

RED_SCENE_INTERPOLATION_EVENT_GENERATE_CLASS_HEADER( CStorySceneEventCameraLightInterpolation, CStorySceneEventCameraLight )
