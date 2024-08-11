/*
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "extAnimCutsceneEffectEvent.h"
#include "cutsceneDebug.h"
#include "cutsceneInstance.h"
#include "weatherManager.h"
#include "renderCommands.h"

#ifdef DEBUG_CUTSCENES
#pragma optimize("",off)
#endif

IMPLEMENT_ENGINE_CLASS( CExtAnimCutsceneEffectEvent );

CExtAnimCutsceneEffectEvent::CExtAnimCutsceneEffectEvent()
	 : CExtAnimDurationEvent()
	 , m_effect( CName::NONE )
	 , m_tag()
	 , m_spawnPosMS( Vector::ZEROS )
	 , m_spawnRotMS( EulerAngles::ZEROS )
	 , m_template()
{
	m_reportToScript = false;
}

CExtAnimCutsceneEffectEvent::CExtAnimCutsceneEffectEvent( const CName& eventName,
		 const CName& animationName, Float startTime, Float duration, const String& trackName
		 )
	: CExtAnimDurationEvent( eventName, animationName, startTime, duration, trackName )
	, m_effect( CName::NONE )
	, m_tag()
	, m_spawnPosMS( Vector::ZEROS )
	, m_spawnRotMS( EulerAngles::ZEROS )
	, m_template()
{
	m_reportToScript = false;
}

CExtAnimCutsceneEffectEvent::~CExtAnimCutsceneEffectEvent()
{

}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CExtAnimCutsceneSlowMoEvent );

CExtAnimCutsceneSlowMoEvent::CExtAnimCutsceneSlowMoEvent()
	: CExtAnimCutsceneDurationEvent()
	, m_factor( 1.f )
	, m_useWeightCurve( false )
	, m_enabled( true )
{
	
}

CExtAnimCutsceneSlowMoEvent::CExtAnimCutsceneSlowMoEvent( const CName& eventName, const CName& animationName, Float startTime, Float duration, const String& trackName )
	: CExtAnimCutsceneDurationEvent( eventName, animationName, startTime, duration, trackName )
	, m_factor( 1.f )
	, m_useWeightCurve( false )
	, m_enabled( true )
{
	m_weightCurve.Clear();
	m_weightCurve.AddPoint( 0.f, 0.f );
	m_weightCurve.AddPoint( 1.f, 1.f );
}

void CExtAnimCutsceneSlowMoEvent::StartEx( const CAnimationEventFired& info, CCutsceneInstance* cs ) const
{
	if ( m_enabled )
	{
		cs->StartSlowMoBlending( m_factor );
	}
}

void CExtAnimCutsceneSlowMoEvent::StopEx( const CAnimationEventFired& info, CCutsceneInstance* cs ) const
{
	if ( m_enabled )
	{
		cs->FinishSlowMoBlending( m_factor );
	}
}

void CExtAnimCutsceneSlowMoEvent::ProcessEx( const CAnimationEventFired& info, CCutsceneInstance* cs ) const
{
	if ( m_enabled )
	{
		const Float evtProgress = info.GetProgress();
		const Float w = m_useWeightCurve ? Clamp( m_weightCurve.GetFloatValue( evtProgress ), 0.f, 1.f ) : evtProgress;

		cs->ProcessSlowMoBlending( m_factor, w );
	}
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CExtAnimCutsceneWindEvent );

CExtAnimCutsceneWindEvent::CExtAnimCutsceneWindEvent()
	: CExtAnimCutsceneDurationEvent()
	, m_factor( 1.f )
	, m_useWeightCurve( false )
	, m_enabled( true )
{

}

CExtAnimCutsceneWindEvent::CExtAnimCutsceneWindEvent( const CName& eventName, const CName& animationName, Float startTime, Float duration, const String& trackName )
	: CExtAnimCutsceneDurationEvent( eventName, animationName, startTime, duration, trackName )
	, m_factor( 1.f )
	, m_useWeightCurve( false )
	, m_enabled( true )
{
	m_weightCurve.Clear();
	m_weightCurve.AddPoint( 0.f, 0.f );
	m_weightCurve.AddPoint( 1.f, 1.f );
}

void CExtAnimCutsceneWindEvent::StartEx( const CAnimationEventFired& info, CCutsceneInstance* cs ) const
{
	if ( m_enabled )
	{
		SetWindFactorEventOverride( 1.0f );
	}
}

void CExtAnimCutsceneWindEvent::StopEx( const CAnimationEventFired& info, CCutsceneInstance* cs ) const
{
	if ( m_enabled )
	{
		SetWindFactorEventOverride( 1.0f );
	}
}

void CExtAnimCutsceneWindEvent::ProcessEx( const CAnimationEventFired& info, CCutsceneInstance* cs ) const
{
	if ( m_enabled )
	{
		const Float evtProgress = info.GetProgress();
		const Float w = m_useWeightCurve ? m_weightCurve.GetFloatValue( evtProgress ) : m_factor;
		SetWindFactorEventOverride( w );
	}
}

void CExtAnimCutsceneWindEvent::SetWindFactorEventOverride( Float v ) const
{
	if( GGame )
	{
		CWorld* world = GGame->GetActiveWorld();
		if( world )
		{
			CEnvironmentManager* envm = world->GetEnvironmentManager();
			if( envm )
			{
				CWeatherManager* wmen = envm->GetWeatherManager();
				wmen->SetWindFactorEventOverride( v );
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CExtAnimCutsceneHideTerrainEvent );

CExtAnimCutsceneHideTerrainEvent::CExtAnimCutsceneHideTerrainEvent()
	: CExtAnimCutsceneDurationEvent()
{

}

CExtAnimCutsceneHideTerrainEvent::CExtAnimCutsceneHideTerrainEvent( const CName& eventName, const CName& animationName, Float startTime, Float duration, const String& trackName )
	: CExtAnimCutsceneDurationEvent( eventName, animationName, startTime, duration, trackName )
{

}

void CExtAnimCutsceneHideTerrainEvent::StartEx( const CAnimationEventFired& info, CCutsceneInstance* cs ) const
{
	if( GGame )
	{
		CWorld* world = GGame->GetActiveWorld();
		if( world && world->GetTerrain() )
		{
			( new CRenderCommand_SetupEnvironmentElementsVisibility( world->GetRenderSceneEx(), false, true, true ) )->Commit();
		}
	}
}

void CExtAnimCutsceneHideTerrainEvent::StopEx( const CAnimationEventFired& info, CCutsceneInstance* cs ) const
{
	if( GGame )
	{
		CWorld* world = GGame->GetActiveWorld();
		if( world && world->GetTerrain() )
		{
			( new CRenderCommand_SetupEnvironmentElementsVisibility( world->GetRenderSceneEx(), true, true, true ) )->Commit();
		}
	}
}

void CExtAnimCutsceneHideTerrainEvent::ProcessEx( const CAnimationEventFired& info, CCutsceneInstance* cs ) const
{

}

#ifdef DEBUG_CUTSCENES
#pragma optimize("",on)
#endif
