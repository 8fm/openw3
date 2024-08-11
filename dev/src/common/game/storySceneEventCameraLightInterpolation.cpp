#include "build.h"
#include "storySceneEventCameraLightInterpolation.h"
#include "storySceneEventsCollector_events.h"
#include "storySceneEventsCollector.h"
#include "StorySceneEventCameraLight.h"
#include "gameTypeRegistry.h"
#include "storySceneSection.h"
#include "storySceneElement.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "storyScenePlayer.h"
#include "../engine/environmentManager.h"

namespace StoryScene 
{
	EInterpolationChannelType InterpolationTraits< CStorySceneEventCameraLight >::GetChannelType( Uint32 channel )
	{	
		return EInterpolationChannelType::ICT_Normal;
	}

	EInterpolationMethod InterpolationTraits< CStorySceneEventCameraLight >::GetDefaultInterpolationMethod( Uint32 channel )
	{
		return EInterpolationMethod::IM_Bezier;
	}

	void InterpolationTraits< CStorySceneEventCameraLight >::GetChannels( const CStorySceneEventCameraLight& keyEvent, ChannelArray& outChannels, VolatileChannels, CSceneEventFunctionSimpleArgs& args )
	{
	}

	void InterpolationTraits< CStorySceneEventCameraLight >::GetChannels( const CStorySceneEventCameraLight& keyEvent, ChannelArray& outChannels, StaticChannels, CSceneEventFunctionSimpleArgs& args )
	{
		outChannels[ Channel::gameplayLightWeight ] =	keyEvent.m_cameralightType == ECLT_Gameplay;
		outChannels[ Channel::sceneLightWeight ]	=	keyEvent.m_cameralightType == ECLT_Scene;
	}

	Bool InterpolationTraits< CStorySceneEventCameraLight >::GetDefaultVolatilityState( const CStorySceneEventCameraLight& keyEvent )
	{
		return false;
	}

	namespace
	{
		Float GetWeight( ECameraLightModType cameralightType, const InterpolationTraits< CStorySceneEventCameraLight >::ChannelArray& interpolationResult )
		{
			InterpolationTraits< CStorySceneEventCameraLight >::Channel channel;
			switch( cameralightType )
			{
			case ECLT_Gameplay:
				channel = InterpolationTraits< CStorySceneEventCameraLight >::gameplayLightWeight;
				break;
			case ECLT_Scene:
				channel = InterpolationTraits< CStorySceneEventCameraLight >::sceneLightWeight;
				break;
			default:
				channel = InterpolationTraits< CStorySceneEventCameraLight >::sceneLightWeight;
				break;
			}

			return interpolationResult[ channel ]; 
		}

		void ProcessKey( SCameraLightsModifiersSetup& setup, const CStorySceneEventCameraLight* sourceKeyEvent, Float dayTime )
		{
			SCameraLightModifiers& mod0 = setup.m_modifiersByType[sourceKeyEvent->m_cameralightType].m_lightModifier0;
			SCameraLightModifiers& mod1 = setup.m_modifiersByType[sourceKeyEvent->m_cameralightType].m_lightModifier1;

			CStorySceneEventCameraLight::CopyLightData( mod0, sourceKeyEvent->m_lightMod1, dayTime );
			CStorySceneEventCameraLight::CopyLightData( mod1, sourceKeyEvent->m_lightMod2, dayTime );
		}

		void ProcessKeyBlend( SCameraLightModifiers& mod, const SCameraLightModifiers& sourceMod, const SCameraLightModifiers& destinationMod, Float progress )
		{
#define INTERPOLATE_MOD_VALUE( x ) mod.x = sourceMod.x * progrInv + destinationMod.x * progress;
			Float progrInv = 1.f - progress;
			INTERPOLATE_MOD_VALUE( brightnessScale )
			INTERPOLATE_MOD_VALUE( radiusScale )
			INTERPOLATE_MOD_VALUE( offsetFront )
			INTERPOLATE_MOD_VALUE( offsetRight )
			INTERPOLATE_MOD_VALUE( offsetUp )		

			INTERPOLATE_MOD_VALUE( attenuationOverrideAmount )		
			if ( sourceMod.attenuationOverrideAmount > 0.f == destinationMod.attenuationOverrideAmount > 0.f )
			{
				INTERPOLATE_MOD_VALUE( attenuationOverride )
			}
			else if( sourceMod.attenuationOverrideAmount > 0.f )
			{
				mod.attenuationOverride = sourceMod.attenuationOverride;
			}
			else
			{
				mod.attenuationOverride = destinationMod.attenuationOverride;
			}
			INTERPOLATE_MOD_VALUE( colorOverrideAmount )
			if ( sourceMod.colorOverrideAmount > 0.f == destinationMod.colorOverrideAmount > 0.f )
			{
				INTERPOLATE_MOD_VALUE( colorOverrideR );
				INTERPOLATE_MOD_VALUE( colorOverrideG );
				INTERPOLATE_MOD_VALUE( colorOverrideB );
			}
			else if( sourceMod.colorOverrideAmount > 0.f )
			{
				mod.colorOverrideR	= sourceMod.colorOverrideR;
				mod.colorOverrideG	= sourceMod.colorOverrideG;
				mod.colorOverrideB	= sourceMod.colorOverrideB;
			}
			else
			{
				mod.colorOverrideR	= destinationMod.colorOverrideR;
				mod.colorOverrideG	= destinationMod.colorOverrideG;
				mod.colorOverrideB	= destinationMod.colorOverrideB;					
			}
#undef INTERPOLATE_MOD_VALUE
		}

		void ProcessModifierKey( SCameraLightModifiers& outLightModifier, const SCameraLightModifiers& srcLightModifier, const SCameraLightModifiers& dstLightModifier, Float progress )
		{
			if( srcLightModifier.brightnessScale > 0.f == dstLightModifier.brightnessScale > 0.f )
			{
				ProcessKeyBlend( outLightModifier, srcLightModifier, dstLightModifier, progress );
			}
			else if( srcLightModifier.brightnessScale > 0.f )
			{
				outLightModifier = srcLightModifier;
				Float srcWeight = 1.f - progress;
				outLightModifier.brightnessScale *= srcWeight;
			}
			else
			{
				outLightModifier = dstLightModifier;
				Float dstWeight = progress;
				outLightModifier.brightnessScale *= dstWeight;
			}
		}

	}

	namespace CameraLightBlend
	{
		void BlendCameraLight( SCameraLightsModifiersSetup& outData, const SCameraLightsModifiersSetup& srcData, const  SCameraLightsModifiersSetup& dstData, Float progress, Float dayTime )
		{
			for ( Uint32 i = 0; i < ECLT_MAX; i++ )
			{
				ECameraLightModType lightType = ECameraLightModType(i);
				ProcessModifierKey( outData.m_modifiersByType[lightType].m_lightModifier0, srcData.m_modifiersByType[lightType].m_lightModifier0, dstData.m_modifiersByType[lightType].m_lightModifier0, progress );
				ProcessModifierKey( outData.m_modifiersByType[lightType].m_lightModifier1, srcData.m_modifiersByType[lightType].m_lightModifier1, dstData.m_modifiersByType[lightType].m_lightModifier1, progress );
			}
		}
	}


	void InterpolationTraits< CStorySceneEventCameraLight >::OnProcess( const CStorySceneEvent* interpolationEvent, const CStorySceneEventCameraLight& sourceKeyEvent, const ChannelArray& interpolationResult, CSceneEventFunctionArgs& args )
	{
		ASSERT( args.m_destEvent )
		const CStorySceneEventCameraLight& destinationEvent = *Cast< const CStorySceneEventCameraLight >( args.m_destEvent );

		StorySceneEventsCollector::CameraLightProp evt;
		evt.m_event = interpolationEvent;

		SCameraLightsModifiersSetup& setup = evt.cameraSetup;
		setup.SetModifiersIdentity( false );		

		Float dayTime = 0.f;
		if ( CWorld* world = args.m_scenePlayer->GetLayer()->GetWorld() )
		{
			if( CEnvironmentManager* envMgr = world->GetEnvironmentManager() )
			{
				dayTime = envMgr->GetCurrentGameTime().ToFloat() / (24.f * 60.f * 60.f);
			}
		}

		SCameraLightsModifiersSetup srcData, dstData;
		srcData.SetModifiersIdentity( false );
		dstData.SetModifiersIdentity( false );
		ProcessKey( srcData, &sourceKeyEvent, dayTime );
		ProcessKey( dstData, &destinationEvent, dayTime );		

		Float keyAtime = sourceKeyEvent.GetInstanceStartTime( args.m_data );
		Float keyBtime = destinationEvent.GetInstanceStartTime( args.m_data );
		Float progress = ( args.m_timeInfo.m_timeAbs - keyAtime )/( keyBtime - keyAtime );

		CameraLightBlend::BlendCameraLight( setup, srcData, dstData, progress, dayTime );
		args.m_collector.AddEvent( evt );
	}

}// namespace StoryScene;


RED_SCENE_INTERPOLATION_EVENT_GENERATE_CLASS_IMPL( CStorySceneEventCameraLightInterpolation, CStorySceneEventCameraLight )
