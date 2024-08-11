/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "storySceneEventOverridePlacement.h"
#include "storyScenePlayer.h"
#include "storySceneDirector.h"

#ifdef DEBUG_SCENES
#pragma optimize("",off)
#endif

IMPLEMENT_ENGINE_CLASS( CStorySceneEventOverridePlacement );

CStorySceneEventOverridePlacement::CStorySceneEventOverridePlacement()
	: CStorySceneEvent()
	, m_resetCloth( DRCDT_Reset )
{

}

CStorySceneEventOverridePlacement::CStorySceneEventOverridePlacement( const String& eventName, CStorySceneElement* sceneElement, Float startTime, const String& trackName, const CName& actorName, const EngineTransform& defaultPlacement )
	: CStorySceneEvent( eventName, sceneElement, startTime, trackName )
	, m_actorName( actorName )
	, m_placement( defaultPlacement )
	, m_resetCloth( DRCDT_Reset )
{

}

CStorySceneEventOverridePlacement* CStorySceneEventOverridePlacement::Clone() const
{
	return new CStorySceneEventOverridePlacement( *this );
}

void CStorySceneEventOverridePlacement::OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	TBaseClass::OnStart( data, scenePlayer, collector, timeInfo );

	if ( m_actorName )
	{
		StorySceneEventsCollector::ActorPlacement evt( this, m_actorName );
		evt.m_eventTimeAbs = timeInfo.m_timeAbs;
		evt.m_eventTimeLocal = timeInfo.m_timeLocal;
		evt.m_placementSS = m_placement;

		// TODO
		evt.m_sceneTransformWS = scenePlayer->GetSceneDirector()->GetCurrentScenePlacement();

		collector.AddEvent( evt );

		if ( !IsInterpolationEventKey() && m_resetCloth != DRCDT_None )
		{
			StorySceneEventsCollector::ActorResetClothAndDangles cEvt( this, m_actorName );
			cEvt.m_eventTimeAbs = timeInfo.m_timeAbs;
			cEvt.m_eventTimeLocal = timeInfo.m_timeLocal;
			cEvt.m_forceRelaxedState = m_resetCloth == DRCDT_ResetAndRelax;
			cEvt.m_dangle = true;

			collector.AddEvent( cEvt );
		}
	}
}

void CStorySceneEventOverridePlacement::OnInterpolationEventGUIDSet()
{
	m_resetCloth = DRCDT_None;
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CStorySceneEventOverridePlacementDuration );

CStorySceneEventOverridePlacementDuration::CStorySceneEventOverridePlacementDuration()
	: CStorySceneEventCurveAnimation()
{
	
}

CStorySceneEventOverridePlacementDuration::CStorySceneEventOverridePlacementDuration( const String& eventName, CStorySceneElement* sceneElement, Float startTime, const String& trackName, const CName& actor, const EngineTransform& defaultPlacement )
	: CStorySceneEventCurveAnimation( eventName, sceneElement, startTime, actor, trackName, defaultPlacement )
	, m_actorName( actor )
{
	
}

CStorySceneEventOverridePlacementDuration* CStorySceneEventOverridePlacementDuration::Clone() const
{
	return new CStorySceneEventOverridePlacementDuration( *this );
}

void CStorySceneEventOverridePlacementDuration::OnProcess( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	TBaseClass::OnProcess( data, scenePlayer, collector, timeInfo );

	EngineTransform transform;

	{
		SMultiCurve* curve = const_cast<SMultiCurve*>( &m_curve );
		curve->SetParent( scenePlayer );
		m_curve.GetRootTransform( timeInfo.m_timeLocal / GetInstanceDuration( data ), transform );
	}

	{
		StorySceneEventsCollector::ActorPlacement evt( this, m_actorName );
		evt.m_eventTimeAbs = timeInfo.m_timeAbs;
		evt.m_eventTimeLocal = timeInfo.m_timeLocal;
		evt.m_placementSS = transform;

		// TODO
		evt.m_sceneTransformWS = scenePlayer->GetSceneDirector()->GetCurrentScenePlacement();

		collector.AddEvent( evt );
	}
}

void CStorySceneEventOverridePlacementDuration::OnEnd( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	EngineTransform transform;

	{
		SMultiCurve* curve = const_cast<SMultiCurve*>( &m_curve );
		curve->SetParent( scenePlayer );
		m_curve.GetRootTransform( 1.0f, transform );
	}

	{
		StorySceneEventsCollector::ActorPlacement evt( this, m_actorName );
		evt.m_eventTimeAbs = timeInfo.m_timeAbs;
		evt.m_eventTimeLocal = timeInfo.m_timeLocal;
		evt.m_placementSS = transform;

		// TODO
		evt.m_sceneTransformWS = scenePlayer->GetSceneDirector()->GetCurrentScenePlacement();

		collector.AddEvent( evt );
	}

	TBaseClass::OnEnd( data, scenePlayer, collector, timeInfo );
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CStorySceneEventWalk );

CStorySceneEventWalk::CStorySceneEventWalk()
	: CStorySceneEventCurveAnimation()
{

}

CStorySceneEventWalk::CStorySceneEventWalk( const String& eventName, CStorySceneElement* sceneElement, Float startTime, const String& trackName, const CName& actor, const EngineTransform& defaultPlacement )
	: CStorySceneEventCurveAnimation( eventName, sceneElement, startTime, actor, trackName, defaultPlacement )
	, m_actorName( actor )
{

}

CStorySceneEventWalk* CStorySceneEventWalk::Clone() const
{
	return new CStorySceneEventWalk( *this );
}

void CStorySceneEventWalk::OnProcess( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	TBaseClass::OnProcess( data, scenePlayer, collector, timeInfo );

	EngineTransform transform;

	{
		SMultiCurve* curve = const_cast<SMultiCurve*>( &m_curve );
		curve->SetParent( scenePlayer );
		m_curve.GetRootTransform( timeInfo.m_timeLocal / GetInstanceDuration( data ), transform );
	}

	{
		StorySceneEventsCollector::ActorPlacement evt( this, m_actorName );
		evt.m_eventTimeAbs = timeInfo.m_timeAbs;
		evt.m_eventTimeLocal = timeInfo.m_timeLocal;
		evt.m_placementSS = transform;

		// TODO
		evt.m_sceneTransformWS = scenePlayer->GetSceneDirector()->GetCurrentScenePlacement();

		collector.AddEvent( evt );
	}
}

void CStorySceneEventWalk::OnEnd( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	EngineTransform transform;

	{
		SMultiCurve* curve = const_cast<SMultiCurve*>( &m_curve );
		curve->SetParent( scenePlayer );
		m_curve.GetRootTransform( 1.0f, transform );
	}

	{
		StorySceneEventsCollector::ActorPlacement evt( this, m_actorName );
		evt.m_eventTimeAbs = timeInfo.m_timeAbs;
		evt.m_eventTimeLocal = timeInfo.m_timeLocal;
		evt.m_placementSS = transform;

		// TODO
		evt.m_sceneTransformWS = scenePlayer->GetSceneDirector()->GetCurrentScenePlacement();

		collector.AddEvent( evt );
	}

	TBaseClass::OnEnd( data, scenePlayer, collector, timeInfo );
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CStorySceneOverridePlacementBlend );

CStorySceneOverridePlacementBlend::CStorySceneOverridePlacementBlend()
{
}

CStorySceneOverridePlacementBlend::CStorySceneOverridePlacementBlend( const String& eventName, CStorySceneElement* sceneElement,
								  Float startTime, const String& trackName, const CName& actorName )
	: CStorySceneEventCurveBlend( eventName, sceneElement, trackName, 0.0f, 1.0f )
	, m_actorName( actorName )
{
}

CStorySceneOverridePlacementBlend* CStorySceneOverridePlacementBlend::Clone() const 
{
	return new CStorySceneOverridePlacementBlend( *this );
}

void CStorySceneOverridePlacementBlend::OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	TBaseClass::OnStart( data, scenePlayer, collector, timeInfo );

	StorySceneEventsCollector::ActorPlacement evt( this, m_actorName );
	evt.m_eventTimeAbs = timeInfo.m_timeAbs;
	evt.m_eventTimeLocal = timeInfo.m_timeLocal;
	evt.m_sceneTransformWS = scenePlayer->GetSceneDirector()->GetCurrentScenePlacement(); // TODO
	if ( GetTransformAt( 0.0f, evt.m_placementSS ) )
	{
		collector.AddEvent( evt );
	}
}

void CStorySceneOverridePlacementBlend::OnEnd( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	StorySceneEventsCollector::ActorPlacement evt( this, m_actorName );
	evt.m_eventTimeAbs = timeInfo.m_timeAbs;
	evt.m_eventTimeLocal = timeInfo.m_timeLocal;
	evt.m_sceneTransformWS = scenePlayer->GetSceneDirector()->GetCurrentScenePlacement(); // TODO
	if ( GetTransformAt( GetInstanceDuration( data ), evt.m_placementSS ) )
	{
		collector.AddEvent( evt );
	}

	TBaseClass::OnEnd( data, scenePlayer, collector, timeInfo );
}

void CStorySceneOverridePlacementBlend::OnProcess( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	TBaseClass::OnProcess( data, scenePlayer, collector, timeInfo );

	StorySceneEventsCollector::ActorPlacement evt( this, m_actorName );
	evt.m_eventTimeAbs = timeInfo.m_timeAbs;
	evt.m_eventTimeLocal = timeInfo.m_timeLocal;
	evt.m_sceneTransformWS = scenePlayer->GetSceneDirector()->GetCurrentScenePlacement(); // TODO
	if ( GetTransformAt( timeInfo.m_timeLocal, evt.m_placementSS ) )
	{
		collector.AddEvent( evt );
	}
}

Bool CStorySceneOverridePlacementBlend::GetTransformAt( Float localTime, EngineTransform& result ) const
{
	if ( m_curve.Size() < 2 )
	{
		return false;
	}

	m_curve.GetRootTransform( localTime, result );
	return true;
}

Uint32 CStorySceneOverridePlacementBlend::GetNumExtraCurveDataStreams() const
{
	return 0;
}

void CStorySceneOverridePlacementBlend::GetKeyDataFromEvent( CStorySceneEvent* keyEvent, CurveKeyData& result ) const
{
#ifndef NO_EDITOR
	CStorySceneEventOverridePlacement* placement = Cast< CStorySceneEventOverridePlacement >( keyEvent );
	ASSERT( placement );
	result.m_transform = placement->GetTransformRef();
#else
	RED_UNUSED( keyEvent );
	RED_UNUSED( result );
	RED_HALT( "GetKeyDataFromEvent compiled out! Please look at this" );
#endif
}

void CStorySceneOverridePlacementBlend::SetKeyDataToEvent( CStorySceneEvent* keyEvent, const CurveKeyData& data ) const
{
#ifndef NO_EDITOR
	CStorySceneEventOverridePlacement* placement = Cast< CStorySceneEventOverridePlacement >( keyEvent );
	ASSERT( placement );
	placement->SetTransform( data.m_transform );
#else
	RED_UNUSED( keyEvent );
	RED_UNUSED( data );
	RED_HALT( "SetKeyDataToEvent compiled out! Please look at this" );
#endif
}

//////////////////////////////////////////////////////////////////////////

#ifdef DEBUG_SCENES
#pragma optimize("",on)
#endif
