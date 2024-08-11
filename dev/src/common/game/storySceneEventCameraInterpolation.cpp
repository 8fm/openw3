// Copyright © 2014 CD Projekt Red. All Rights Reserved.

#include "build.h"
#include "storySceneEventCameraInterpolation.h"
#include "storySceneEventsCollector.h"
#include "storyScenePlayer.h"
#include "storySceneEventCamera.h"
#include "../core/instanceDataLayoutCompiler.h"

// =================================================================================================
namespace {
// =================================================================================================

// functions in this namespace
void CommitChannels( const Float* channels, StorySceneCameraDefinition& camDef );

/*

\param channels Channels that are to be committed to object.
\param cam Object that is to be updated.

TODO: CStorySceneEventCustomCamera VS StorySceneCameraDefinition

TODO: think about passing channels as Float[Traits::numChannels]
*/
void CommitChannels( const Float* channels, StorySceneCameraDefinition& camDef )
{
	typedef StoryScene::InterpolationTraits< CStorySceneEventCamera > Traits;

	Vector t( channels[ Traits::Channel::posX ], channels[ Traits::Channel::posY ], channels[ Traits::Channel::posZ ], 1.0f );
	camDef.m_cameraTransform.SetPosition( t );

	EulerAngles r( channels[ Traits::Channel::rotY ], channels[ Traits::Channel::rotX ], channels[ Traits::Channel::rotZ ] );
	camDef.m_cameraTransform.SetRotation( r );

	camDef.m_cameraFov = channels[ Traits::Channel::fov ];
	camDef.m_cameraZoom = channels[ Traits::Channel::zoom ];

	camDef.m_dofBlurDistFar = channels[ Traits::Channel::dofBlurDistFar ];
	camDef.m_dofBlurDistNear = channels[ Traits::Channel::dofBlurDistNear ];
	camDef.m_dofFocusDistFar = channels[ Traits::Channel::dofFocusDistFar ];
	camDef.m_dofFocusDistNear = channels[ Traits::Channel::dofFocusDistNear ];
	camDef.m_dofIntensity = channels[ Traits::Channel::dofIntensity ];
	camDef.m_bokehDofParams.m_planeInFocus = channels[ Traits::Channel::planeInFocus ];
	camDef.m_bokehDofParams.m_bokehSizeMuliplier = channels[ Traits::Channel::bokehSizeMultiplier ];
}

// =================================================================================================
} // unnamed namespace
// =================================================================================================

// =================================================================================================
namespace StoryScene {
// =================================================================================================

/*

Note - if all channels are normal then we can just do "return Channeltype::normal".
*/
EInterpolationChannelType InterpolationTraits< CStorySceneEventCamera >::GetChannelType( Uint32 channel )
{
	switch( channel )
	{
		case rotX: return EInterpolationChannelType::ICT_AngleInDegrees;
		case rotY: return EInterpolationChannelType::ICT_AngleInDegrees;
		case rotZ: return EInterpolationChannelType::ICT_AngleInDegrees;
		default: return EInterpolationChannelType::ICT_Normal;
	}
}

/*
Returns default interpolation method for specified channel.

Later this can be changed for each key on each channel.
*/
EInterpolationMethod InterpolationTraits< CStorySceneEventCamera >::GetDefaultInterpolationMethod( Uint32 channel )
{
	switch( channel )
	{
		case posX: return EInterpolationMethod::IM_Bezier;
		default: return EInterpolationMethod::IM_Bezier;
	}
}

/*
Gets volatile channels of specified key event.

\param keyEvent Key event whose channels to get.
\param outChannels (out) Array of floats to which to store channel values. Size of this array is equal to number of defined channels.
*/
void InterpolationTraits< CStorySceneEventCamera >::GetChannels( const CStorySceneEventCamera& keyEvent, ChannelArray& outChannels, VolatileChannels, CSceneEventFunctionSimpleArgs& args )
{
	if ( const StorySceneCameraDefinition* cam = keyEvent.GetCameraDefinition( &args ) )
	{
		const StorySceneCameraDefinition& camDef = *cam;

		const Vector& t = camDef.m_cameraTransform.GetPosition();
		outChannels[ Channel::posX ] = t.X;
		outChannels[ Channel::posY ] = t.Y;
		outChannels[ Channel::posZ ] = t.Z;

		const EulerAngles& r = camDef.m_cameraTransform.GetRotation();
		outChannels[ Channel::rotX ] = r.Pitch;
		outChannels[ Channel::rotY ] = r.Roll;
		outChannels[ Channel::rotZ ] = r.Yaw;
	}
}

/*
Gets static channels of specified key event.

\param keyEvent Key event whose channels to get.
\param outChannels (out) Array of floats to which to store channel values. Size of this array is equal to number of defined channels.
*/
void InterpolationTraits< CStorySceneEventCamera >::GetChannels( const CStorySceneEventCamera& keyEvent, ChannelArray& outChannels, StaticChannels, CSceneEventFunctionSimpleArgs& args )
{
	if( const StorySceneCameraDefinition* cam = keyEvent.GetCameraDefinition( &args ) )
	{
		const StorySceneCameraDefinition& camDef = *cam;

		outChannels[ Channel::fov ] = camDef.m_cameraFov;
		outChannels[ Channel::zoom ] = camDef.m_cameraZoom;

		outChannels[ Channel::dofBlurDistFar ] = camDef.m_dofBlurDistFar;
		outChannels[ Channel::dofBlurDistNear ] = camDef.m_dofBlurDistNear;
		outChannels[ Channel::dofFocusDistFar ] = camDef.m_dofFocusDistFar;
		outChannels[ Channel::dofFocusDistNear ] = camDef.m_dofFocusDistNear;
		outChannels[ Channel::dofIntensity ] = camDef.m_dofIntensity;

		outChannels[ Channel::planeInFocus ] = camDef.m_bokehDofParams.m_planeInFocus;
		outChannels[ Channel::bokehSizeMultiplier ] = camDef.m_bokehDofParams.m_bokehSizeMuliplier;
	}
}

/*
Returns whether key should be volatile or not by default.
*/
Bool InterpolationTraits< CStorySceneEventCamera >::GetDefaultVolatilityState( const CStorySceneEventCamera& keyEvent )
{
	return keyEvent.IsVolatileEvent();
}

/*


\param interpolateEvent Story scene interpolate event.
\param sourceKey Source key + interpolation result = resulting object stat.
\param interpolationResult Interpolation result, i.e. values of all channels. It's guaranteed to have numChannels elements.
\param args Original args passed to interpolate event OnProcess().
*/
void InterpolationTraits< CStorySceneEventCamera >::OnProcess( const CStorySceneEvent* interpolationEvent, const CStorySceneEventCamera& sourceKeyEvent, const ChannelArray& interpolationResult, CSceneEventFunctionArgs& args )
{
	ASSERT( interpolationEvent );

	StorySceneEventsCollector::CameraBlend cameraBlendEvent( interpolationEvent );
	cameraBlendEvent.m_eventTimeAbs = args.m_timeInfo.m_timeAbs;
	cameraBlendEvent.m_eventTimeLocal = args.m_timeInfo.m_timeLocal;

	// combine sourceKeyEvent state and interpolation result to obtain final result
	if( const StorySceneCameraDefinition* cam = sourceKeyEvent.GetCameraDefinition() )
	{
		cameraBlendEvent.m_currentCameraState = *cam;
		CommitChannels( interpolationResult, cameraBlendEvent.m_currentCameraState );
	}
	

	args.m_collector.AddEvent( cameraBlendEvent );
}

// =================================================================================================
} // namespace StoryScene
// =================================================================================================

RED_SCENE_INTERPOLATION_EVENT_GENERATE_CLASS_IMPL( CStorySceneEventCameraInterpolation, CStorySceneEventCamera )
