/*
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "extAnimCutsceneEnvironmentEvent.h"
#include "cutsceneDebug.h"
#include "game.h"
#include "cameraComponent.h"
#include "environmentManager.h"
#include "environmentComponentArea.h"
#include "lightComponent.h"
#include "worldIterators.h"
#include "componentIterator.h"
#include "tagManager.h"
#include "entity.h"

#ifdef DEBUG_CUTSCENES
#pragma optimize("",off)
#endif

IMPLEMENT_ENGINE_CLASS( CExtAnimCutsceneEnvironmentEvent );

CExtAnimCutsceneEnvironmentEvent::CExtAnimCutsceneEnvironmentEvent()
   : CExtAnimEvent()
   , m_stabilizeBlending( true )
   , m_instantEyeAdaptation( true )
   , m_instantDissolve( true )
   , m_forceSetupLocalEnvironments( false )
   , m_forceSetupGlobalEnvironments( false )
   , m_forceNoOtherEnvironments( false )
   , m_environmentName( String::EMPTY )
   , m_environmentActivate( true )
   , m_wasActiveBefore( false )
{
	m_reportToScript = false;
}

CExtAnimCutsceneEnvironmentEvent::CExtAnimCutsceneEnvironmentEvent( const CName& eventName,
		   const CName& animationName, Float startTime, const String& trackName,
		   Bool stabilizeBlending, Bool instantEyeAdaptation, Bool instantDissolve,
		   Bool forceSetupLocalEnvironments, Bool forceSetupGlobalEnvironments,
		   const String& environmentName, Bool environmentActivate )
	: CExtAnimEvent( eventName, animationName, startTime, trackName )
	, m_stabilizeBlending( stabilizeBlending )
	, m_instantEyeAdaptation( instantEyeAdaptation )
	, m_instantDissolve( instantDissolve )
	, m_forceSetupLocalEnvironments( forceSetupLocalEnvironments )
	, m_forceSetupGlobalEnvironments( forceSetupGlobalEnvironments )
	, m_environmentName( environmentName )
	, m_environmentActivate( environmentActivate )
{
	m_reportToScript = false;
}

CExtAnimCutsceneEnvironmentEvent::~CExtAnimCutsceneEnvironmentEvent()
{

}

CAreaEnvironmentComponent* CExtAnimCutsceneEnvironmentEvent::FindAreaEnvironment() const
{
	THandle< CWorld > world = GGame ? GGame->GetActiveWorld() : nullptr;
	if ( m_environmentName.Empty() || nullptr == world.Get() )
	{
		return NULL;
	}

	for ( WorldAttachedComponentsIterator it( world ); it; ++it )
	{
		CAreaEnvironmentComponent *env = Cast< CAreaEnvironmentComponent > ( *it );
		if ( env && env->GetEntity() != NULL && env->GetEntity()->GetName() == m_environmentName )
		{
			return env;
		}
	}

	return NULL;
}

void CExtAnimCutsceneEnvironmentEvent::Process( const CAnimationEventFired& info, CAnimatedComponent* component ) const
{
	ASSERT( !component );

	THandle< CWorld > world = GGame ? GGame->GetActiveWorld() : nullptr;
	CEnvironmentManager *envManager = world ? world->GetEnvironmentManager() : NULL;
	if ( NULL == envManager )
	{
		return;
	}

	CAreaEnvironmentComponent *areaEnv = FindAreaEnvironment();
	if ( areaEnv )
	{
		m_wasActiveBefore = areaEnv->IsActive();
		areaEnv->SetActivated( m_environmentActivate );
	}

	if ( m_forceNoOtherEnvironments )
	{
		//envManager->DisableEnvChanges();
	}

	if ( !m_forceNoOtherEnvironments && (m_forceSetupLocalEnvironments || m_forceSetupGlobalEnvironments) )
	{
		envManager->SetupAreaEnvironmentsForPosition( world->GetCameraDirector()->GetCameraPosition(), m_forceSetupLocalEnvironments, m_forceSetupGlobalEnvironments );
	}

	if ( m_stabilizeBlending )
	{
		// this isn't needed anymore, blending is now based on camera position, not time
		//envManager->StabilizeEnvironmentBlending();
	}

	if ( m_instantEyeAdaptation )
	{
		envManager->SetInstantAdaptationTrigger();
	}

	if ( m_instantDissolve )
	{
		envManager->SetInstantDissolveTrigger();
	}
}

void CExtAnimCutsceneEnvironmentEvent::OnCutsceneFinish() const
{
	THandle< CWorld > world = GGame ? GGame->GetActiveWorld() : nullptr;
	CEnvironmentManager *envManager = world ? world->GetEnvironmentManager() : NULL;

	if ( !m_wasActiveBefore )
	{
		CAreaEnvironmentComponent *areaEnv = FindAreaEnvironment();
		if ( areaEnv )
		{
			areaEnv->Deactivate();
			
			if ( envManager )
			{
				if ( m_stabilizeBlending )
				{
					// not needed anymore, blending is based on position not time
					//envManager->StabilizeEnvironmentBlending();
				}

				if ( m_instantEyeAdaptation )
				{
					envManager->SetInstantAdaptationTrigger();
				}

				if ( m_instantDissolve )
				{
					envManager->SetInstantDissolveTrigger();
				}
			}
		}
	}

	if ( m_forceNoOtherEnvironments )
	{
		if ( envManager )
		{
			//envManager->EnableEnvChanges();
		}
	}
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CExtAnimCutsceneLightEvent );

CExtAnimCutsceneLightEvent::CExtAnimCutsceneLightEvent()
	: m_color( Color::WHITE )
	, m_radius( 5.0f )
	, m_brightness( 1.0f )
	, m_isEnabled( true )
{
	m_reportToScript = false;
}

CExtAnimCutsceneLightEvent::CExtAnimCutsceneLightEvent( const CName& eventName, const CName& animationName, Float startTime, const String& trackName )
	: CExtAnimEvent( eventName, animationName, startTime, trackName )
	, m_color( Color::WHITE )
	, m_radius( 5.0f )
	, m_brightness( 1.0f )
	, m_isEnabled( true )
{
	m_reportToScript = false;
}

void CExtAnimCutsceneLightEvent::Process( const CAnimationEventFired& info, CAnimatedComponent* component ) const
{
	ASSERT( !component );

	CWorld* world = GGame ? GGame->GetActiveWorld() : nullptr;
	if ( world && world->GetTagManager() )
	{
		TDynArray< CEntity* > entities;
		world->GetTagManager()->CollectTaggedEntities( m_tag, entities, BCTO_MatchAll );

		const Uint32 num = entities.Size();
		if ( num > 0 )
		{
			const CExtAnimCutsceneLightEvent* refObject = CExtAnimCutsceneLightEvent::GetStaticClass()->GetDefaultObject< CExtAnimCutsceneLightEvent >();

			const Bool isColorSet = refObject->m_color.ToUint32() != m_color.ToUint32();
			const Bool isEnableSet = refObject->m_isEnabled != m_isEnabled;
			const Bool isRadiusSet = refObject->m_radius != m_radius;
			const Bool isBrightnessSet =  refObject->m_brightness != m_brightness;
			const Bool isFlickeringSet = 
				refObject->m_lightFlickering.m_flickerPeriod != m_lightFlickering.m_flickerPeriod &&
				refObject->m_lightFlickering.m_flickerStrength != m_lightFlickering.m_flickerStrength &&				
				refObject->m_lightFlickering.m_positionOffset != m_lightFlickering.m_positionOffset;

			for ( Uint32 i=0; i<num; ++i )
			{
				CEntity* e = entities[ i ];

				for ( ComponentIterator< CLightComponent > it( e ); it; ++it )
				{
					CLightComponent* lc = *it;

					if ( isEnableSet )
					{
						lc->SetEnabled( m_isEnabled );
					}

					if ( isColorSet )
					{
						lc->SetColor( m_color );
					}

					if ( isRadiusSet )
					{
						lc->SetRadius( m_radius );
					}

					if ( isBrightnessSet )
					{
						lc->SetBrightness( m_brightness );
					}

					if ( isFlickeringSet )
					{
						lc->SetLightFlickering( m_lightFlickering );
					}
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CExtAnimCutsceneBokehDofEvent );

CExtAnimCutsceneBokehDofEvent::CExtAnimCutsceneBokehDofEvent()
{
	m_reportToScript = false;
}

CExtAnimCutsceneBokehDofEvent::CExtAnimCutsceneBokehDofEvent( const CName& eventName, const CName& animationName, Float startTime, const String& trackName )
	: CExtAnimEvent( eventName, animationName, startTime, trackName )
{
	m_reportToScript = false;
	m_bokehDofParams.m_enabled = true;
}

void CExtAnimCutsceneBokehDofEvent::ProcessEx( const CAnimationEventFired& info, CCameraComponent* component, Float currTime ) const
{
	if ( component )
	{
		component->SetBokehDofParams( m_bokehDofParams );
	}
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CExtAnimCutsceneBokehDofBlendEvent );

CExtAnimCutsceneBokehDofBlendEvent::CExtAnimCutsceneBokehDofBlendEvent()
{
	m_reportToScript = false;
}

CExtAnimCutsceneBokehDofBlendEvent::CExtAnimCutsceneBokehDofBlendEvent( const CName& eventName, const CName& animationName, Float startTime, const String& trackName )
	: CExtAnimDurationEvent( eventName, animationName, startTime, 1.f, trackName )
{
	m_reportToScript = false;
	m_bokehDofParamsStart.m_enabled = true;
	m_bokehDofParamsEnd.m_enabled = true;
}

void CExtAnimCutsceneBokehDofBlendEvent::ProcessEx( const CAnimationEventFired& info, CCameraComponent* component, Float currTime ) const
{
	if ( component )
	{
		SDofParams param;

		const Float duration = info.m_extEvent->GetEndTimeWithoutClamp() - info.m_extEvent->GetStartTime();

		Float w = duration > 0.f ? ( currTime - info.m_extEvent->GetStartTime() ) / duration : 1.f;
		ASSERT( w >= 0.f && w <= 1.f );
		w = Clamp( w, 0.f, 1.f );

		SBokehDofParams bokehDofParamsResult;
		bokehDofParamsResult.m_enabled = m_bokehDofParamsStart.m_enabled;
		bokehDofParamsResult.m_bokehSizeMuliplier = Lerp( w, m_bokehDofParamsStart.m_bokehSizeMuliplier, m_bokehDofParamsEnd.m_bokehSizeMuliplier );
		bokehDofParamsResult.m_planeInFocus = Lerp( w, m_bokehDofParamsStart.m_planeInFocus, m_bokehDofParamsEnd.m_planeInFocus );
		bokehDofParamsResult.m_fStops = m_bokehDofParamsStart.m_fStops;	// cannot blend this parameter!!!

		component->SetBokehDofParams( bokehDofParamsResult );
	}
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CExtAnimCutsceneSetClippingPlanesEvent );

CExtAnimCutsceneSetClippingPlanesEvent::CExtAnimCutsceneSetClippingPlanesEvent()
	: m_nearPlaneDistance( NP_CustomValue )
	, m_farPlaneDistance( FP_CustomValue )
{
	m_reportToScript = false;
}

CExtAnimCutsceneSetClippingPlanesEvent::CExtAnimCutsceneSetClippingPlanesEvent( const CName& eventName, const CName& animationName, Float startTime, const String& trackName )
	: CExtAnimEvent( eventName, animationName, startTime, trackName )
	, m_nearPlaneDistance( NP_CustomValue )
	, m_farPlaneDistance( FP_CustomValue )
{
	m_reportToScript = false;
}

void CExtAnimCutsceneSetClippingPlanesEvent::ProcessEx( const CAnimationEventFired& info, CCameraComponent* component, Float currTime ) const
{
	if( component != nullptr )
	{
		component->SetCustomClippingPlanes( m_customPlaneDistance );
		component->SetNearPlane( m_nearPlaneDistance );
		component->SetFarPlane( m_farPlaneDistance );
	}
}

#ifdef DEBUG_CUTSCENES
#pragma optimize("",off)
#endif
