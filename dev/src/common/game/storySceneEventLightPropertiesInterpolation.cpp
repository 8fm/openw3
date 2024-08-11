// Copyright © 2014 CD Projekt Red. All Rights Reserved.

#include "build.h"
#include "storySceneEventLightPropertiesInterpolation.h"
#include "storySceneEventsCollector.h"
#include "storyScenePlayer.h"
#include "storySceneEventLightProperties.h"
#include "storySceneItems.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "storySceneUtils.h"

// =================================================================================================
namespace StoryScene {
	// =================================================================================================

	EInterpolationChannelType InterpolationTraits< CStorySceneEventLightProperties >::GetChannelType( Uint32 channel )
	{
		// all channels are normal
		return EInterpolationChannelType::ICT_Normal;
	}

	/*
	Returns default interpolation method for specified channel.

	Later this can be changed for each key on each channel.
	*/
	EInterpolationMethod InterpolationTraits< CStorySceneEventLightProperties >::GetDefaultInterpolationMethod( Uint32 channel )
	{
		return EInterpolationMethod::IM_Bezier;
	}

	/*
	Gets volatile channels of specified key event.

	\param keyEvent Key event whose channels to get.
	\param outChannels (out) Array of floats to which to store channel values. Size of this array is equal to number of defined channels.
	*/
	void InterpolationTraits< CStorySceneEventLightProperties >::GetChannels( const CStorySceneEventLightProperties& keyEvent, ChannelArray& outChannels, VolatileChannels, CSceneEventFunctionSimpleArgs& args )
	{
		EngineTransform transform = keyEvent.m_placement;
		if ( keyEvent.IsAttached() )
		{
			const CEntity* ent = args.m_scenePlayer->GetSceneActorEntity( keyEvent.GetAttachmentActor() );
			if( !ent )
			{
				ent = args.m_scenePlayer->GetScenePropEntity( keyEvent.GetAttachmentActor() );
			}
			if( !ent )
			{
				ent = args.m_scenePlayer->GetSceneEffectEntity( keyEvent.GetAttachmentActor() );
			}
			if ( ent && ent->GetRootAnimatedComponent() )
			{
				Matrix local;
				Matrix l2w = StorySceneUtils::CalcL2WForAttachedObject( ent->GetRootAnimatedComponent(), keyEvent.GetAttachmentBone(), keyEvent.GetAttachmentFlags() );
				transform.CalcLocalToWorld( local );
				transform = local * l2w;
			}			
		}

		if ( keyEvent.m_useGlobalCoords )
		{
			transform = StorySceneUtils::CalcSSFromWS( transform, args.m_scenePlayer->GetSceneDirector()->GetCurrentScenePlacement() );
		}


		Vector position = transform.GetPosition();
		EulerAngles rotation = transform.GetRotation();

		if( keyEvent.IsTracking() )
		{
			Vector outPos;
			EulerAngles outRot;
			if( StorySceneUtils::CalcTrackedPosition( keyEvent.m_lightTracker, args.m_scenePlayer, outPos, outRot ) )
			{
				position += outPos;
				rotation = outRot;
			}
		}		

		outChannels[ Channel::posX ] = position.X;
		outChannels[ Channel::posY ] = position.Y;
		outChannels[ Channel::posZ ] = position.Z;

		outChannels[ Channel::rotRoll ] = rotation.Roll;
		outChannels[ Channel::rotPitch ] = rotation.Pitch;
		outChannels[ Channel::rotYaw ] = rotation.Yaw;
	}

	/*
	Gets static channels of specified key event.

	\param keyEvent Key event whose channels to get.
	\param outChannels (out) Array of floats to which to store channel values. Size of this array is equal to number of defined channels.
	*/
	 

	void InterpolationTraits< CStorySceneEventLightProperties >::GetChannels( const CStorySceneEventLightProperties& keyEvent, ChannelArray& outChannels, StaticChannels, CSceneEventFunctionSimpleArgs& args )
	{
		ASSERT( args.m_scenePlayer );
		Color color = keyEvent.GetColor( args.m_scenePlayer );
		const SStorySceneSpotLightProperties& slp = keyEvent.m_spotLightProperties;

		outChannels[ Channel::enabled ] = keyEvent.GetEnabled() ? 1.0f : 0.0f;

		outChannels[ Channel::colorR ] = color.R;
		outChannels[ Channel::colorG ] = color.G;
		outChannels[ Channel::colorB ] = color.B;		

		CWorld* world = args.m_scenePlayer->GetLayer()->GetWorld();

		outChannels[ Channel::attenuation ] = keyEvent.GetAttenuation( world ) ;
		outChannels[ Channel::radius ] = keyEvent.GetRadius( world ); 
		outChannels[ Channel::brightness ] = keyEvent.GetBrightness( world );

		outChannels[ Channel::innerAngle ] = slp.m_innerAngle;
		outChannels[ Channel::outerAngle ] = slp.m_outerAngle;
		outChannels[ Channel::softness ] = slp.m_softness;

		outChannels[ Channel::ambientLevel ] = keyEvent.m_dimmerProperties.GetAmbientLevel( world );
		outChannels[ Channel::marginFactor ] = keyEvent.m_dimmerProperties.GetMarginFactor( world );
	}

	/*
	Returns whether key should be volatile or not by default.
	*/
	Bool InterpolationTraits< CStorySceneEventLightProperties >::GetDefaultVolatilityState( const CStorySceneEventLightProperties& keyEvent )
	{
		return  keyEvent.IsAttached();
	}

	/*
	\param interpolateEvent Story scene interpolate event.
	\param sourceKey Source key + interpolation result = resulting object stat.
	\param interpolationResult Interpolation result, i.e. values of all channels. It's guaranteed to have numChannels elements.
	\param args Original args passed to interpolate event OnProcess().
	*/
	void InterpolationTraits< CStorySceneEventLightProperties >::OnProcess( const CStorySceneEvent* interpolationEvent, const CStorySceneEventLightProperties& sourceKeyEvent, const ChannelArray& interpolationResult, CSceneEventFunctionArgs& args )
	{
		SCENE_ASSERT( interpolationEvent );

		if ( sourceKeyEvent.GetSubject() )
		{
			// NOTE: in a perfect world we would also check the destination key, not only the source key here.
			// Since the light property event's default values are -1.0f it can lead to some strange interpolations
			// but these should be pretty obvious in the editor.

			StorySceneEventsCollector::LightProperty evt( interpolationEvent, sourceKeyEvent.GetSubject() );
			
			const Color& sourceColor = sourceKeyEvent.m_color;
			const SStorySceneSpotLightProperties& spotProps = sourceKeyEvent.m_spotLightProperties;

			evt.m_eventTimeAbs = args.m_timeInfo.m_timeAbs;
			evt.m_eventTimeLocal = args.m_timeInfo.m_timeLocal;

			evt.m_enabled = interpolationResult[ Channel::enabled ] > 0.f;

			/*if ( sourceColor.R != 0 ||
				sourceColor.G != 0 ||
				sourceColor.B != 0 )
			{
				evt.m_color = Color( (Uint8)interpolationResult[ Channel::colorR ], 
					(Uint8)interpolationResult[ Channel::colorG ], 
					(Uint8)interpolationResult[ Channel::colorB ] );
			}
			else
			{
				evt.m_color = sourceColor;
			}*/

			evt.m_color = Color( (Uint8)interpolationResult[ Channel::colorR ], (Uint8)interpolationResult[ Channel::colorG ], (Uint8)interpolationResult[ Channel::colorB ] );

			evt.m_radius = interpolationResult[ Channel::radius ];
			evt.m_attenuation = interpolationResult[ Channel::attenuation ];
			evt.m_brightness = interpolationResult[ Channel::brightness ] * interpolationResult[ Channel::enabled ];

			evt.m_innerAngle = spotProps.m_innerAngle >= 0.0f ? interpolationResult[ Channel::innerAngle ] : spotProps.m_innerAngle;
			evt.m_innerAngle = spotProps.m_outerAngle >= 0.0f ? interpolationResult[ Channel::outerAngle ] : spotProps.m_outerAngle;
			evt.m_innerAngle = spotProps.m_softness >= 0.0f ? interpolationResult[ Channel::softness ] : spotProps.m_softness;

			Vector pos( Vector::ZERO_3D_POINT );
			EulerAngles rot( EulerAngles::ZEROS );

			pos.X = interpolationResult[ Channel::posX ];
			pos.Y = interpolationResult[ Channel::posY ];
			pos.Z = interpolationResult[ Channel::posZ ];

			rot.Roll = interpolationResult[ Channel::rotRoll ];
			rot.Pitch = interpolationResult[ Channel::rotPitch ];
			rot.Yaw = interpolationResult[ Channel::rotYaw ];

			evt.m_ambientLevel = interpolationResult[ Channel::ambientLevel ];
			evt.m_marginFactor = interpolationResult[ Channel::marginFactor ];

			evt.m_placementSS.SetPosition( pos );
			evt.m_placementSS.SetRotation( rot );
			evt.m_sceneTransformWS = args.m_scenePlayer->GetSceneDirector()->GetCurrentScenePlacement();

			args.m_collector.AddEvent( evt );
		}
	}

	// =================================================================================================
} // namespace StoryScene
// =================================================================================================

RED_SCENE_INTERPOLATION_EVENT_GENERATE_CLASS_IMPL( CStorySceneEventLightPropertiesInterpolation, CStorySceneEventLightProperties )
