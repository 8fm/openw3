/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "storySceneEvent.h"
#include "storyScenePlayer.h"
#include "storySceneDebugger.h"
#include "storySceneEventsCollector.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "../engine/localizationManager.h"

#ifdef DEBUG_SCENES_2
#pragma optimize("",off)
#endif

CSceneEventFunctionSimpleArgs::CSceneEventFunctionSimpleArgs( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer )
	: m_data( data )
	, m_scenePlayer( scenePlayer )
{}

CSceneEventFunctionSimpleArgs::CSceneEventFunctionSimpleArgs( CSceneEventFunctionArgs& args )
	: m_data( args.m_data )
	, m_scenePlayer( args.m_scenePlayer )
{}

/*
Ctor.
*/
CSceneEventFunctionArgs::CSceneEventFunctionArgs( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo )
: m_data( data )
, m_scenePlayer( scenePlayer )
, m_collector( collector )
, m_timeInfo( timeInfo )
, m_destEvent( nullptr )
{}

IMPLEMENT_ENGINE_CLASS( CStorySceneEvent );

CStorySceneEvent::CStorySceneEvent()
   : m_eventName()
   , m_sceneElement( nullptr )
   , m_startPosition( 0.0f )
   , m_trackName()
   , m_GUID( CGUID::Create() )
   , m_linkParentTimeOffset( 0.f )
   , m_isMuted( false )
   , m_contexID( 0 )
{
}

CStorySceneEvent::CStorySceneEvent( const String& eventName, CStorySceneElement* sceneElement, Float startPosition, const String& trackName )
	 : m_eventName( eventName )
	 , m_sceneElement( sceneElement )
	 , m_startPosition( startPosition )
	 , m_trackName( trackName )
	 , m_GUID( CGUID::Create() )
	 , m_linkParentTimeOffset( 0.f )
	 , m_isMuted( false )
	 , m_contexID( 0 )
{
}

/*
Cctor.

\param other Event that is to be copied.

Note that:
1. Copy will be assigned unique GUID.
2. Copy will not be associated with any interpolation/blend event. However, interpolation/blend event
   copy will still reference the same key events that are referenced by original interpolation/blend
   event. You will most likely want to copy all those key events and associate them with copy.
3. Copy will not be linked to any event nor any events will be linked to copy.
*/
CStorySceneEvent::CStorySceneEvent( const CStorySceneEvent& other )
: m_eventName( other.m_eventName )
, m_sceneElement( other.m_sceneElement )
, m_startPosition( other.m_startPosition )
, m_trackName( other.m_trackName )
, m_isMuted( other.m_isMuted )
, m_contexID( other.m_contexID )
, m_GUID( CGUID::Create() )
, m_linkParentTimeOffset( 0.0f )
#ifndef NO_EDITOR
, m_debugString( other.m_debugString )
#endif // NO_EDITOR
{}

CStorySceneEvent::~CStorySceneEvent()
{
}

/*
Clones event.

See CStorySceneEvent copy constructor documentation for some important info about event copies.
*/
CStorySceneEvent* CStorySceneEvent::Clone() const
{
	// This function should be pure virtual but it's not due to limitations of our RTTI.
	// All concrete classes deriving from CStorySceneEvent have to override this function.
	RED_FATAL( "CStorySceneEvent::Clone(): call to a function that would be pure virtual (but it's not due to limitations of our RTTI)." );
	return nullptr;
}

void CStorySceneEvent::RegenerateGUID()
{
	m_GUID = CGUID::Create();
}

/*
Notifies event about GUID change so it can update its GUID or its references to other events.
*/
void CStorySceneEvent::OnGuidChanged( CGUID oldGuid, CGUID newGuid )
{
	if( m_GUID == oldGuid )
	{
		m_GUID = newGuid;
	}
	else if( m_interpolationEventGUID == oldGuid )
	{
		m_interpolationEventGUID = newGuid;
	}
	else if( m_blendParentGUID == oldGuid )
	{
		m_blendParentGUID = newGuid;
	}
	else if( m_linkParentGUID == oldGuid )
	{
		m_linkParentGUID = newGuid;
	}

	#ifndef NO_EDITOR

		for( Uint32 i = 0, numChildren = m_linkChildrenGUID.Size(); i < numChildren; ++i )
		{
			if( m_linkChildrenGUID[ i ] == oldGuid )
			{
				m_linkChildrenGUID[ i ] = newGuid;
			}
		}

	#endif // !NO_EDITOR
}

void CStorySceneEvent::Serialize( IFile& file )
{
	if ( file.IsWriter() && m_GUID.IsZero() )
	{
		m_GUID = CGUID::Create();
	}

	GetClass()->Serialize( file, this );

	OnSerialize( file );

	if ( file.IsReader() && m_GUID.IsZero() )
	{
		m_GUID = CGUID::Create();
	}
}

String CStorySceneEvent::GetName() const
{
	return String::Printf( TXT("%s - %s; %1.2f"), GetClass()->GetName().AsString().AsChar(), GetEventName().AsChar(), GetStartPosition() );
}

void CStorySceneEvent::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	compiler << i_valid;
	compiler << i_started;
	compiler << i_finished;
	compiler << i_startTimeMS;
	compiler << i_durationMS;
	compiler << i_scalingFactor;
}

void CStorySceneEvent::OnInitInstance( CStorySceneInstanceBuffer& data ) const
{
	data[ i_valid ] = false;
	data[ i_started ] = false;
	data[ i_finished ] = false;
	data[ i_startTimeMS ] = 0.f;
	data[ i_durationMS ] = 0.f;
	data[ i_scalingFactor ] = 1.0f;
}

void CStorySceneEvent::OnReleaseInstance( CStorySceneInstanceBuffer& data ) const
{
	//data[ i_valid ] = false;
}

void CStorySceneEvent::GetEventInstanceData( const CStorySceneInstanceBuffer& data, Float& s, Float& e, Float& d ) const
{
	SCENE_ASSERT( data[ i_valid ] );
	
	s = data[ i_startTimeMS ];
	d = data[ i_durationMS ];
	
	e = s + d;
}

/*
Returns whether event instance data is set.
*/
Bool CStorySceneEvent::IsEventInstanceDataSet( const CStorySceneInstanceBuffer& instanceBuffer ) const
{
	return instanceBuffer[ i_valid ];
}

void CStorySceneEvent::MarkInstanceDataSet( CStorySceneInstanceBuffer& instanceBuffer ) const
{
	instanceBuffer[ i_valid ] = true;
}

void CStorySceneEvent::SetInstanceStartTime( CStorySceneInstanceBuffer& instanceBuffer, Float startTime ) const
{
	SCENE_ASSERT( startTime >= 0.f );
	instanceBuffer[ i_startTimeMS ] = startTime;
}

Float CStorySceneEvent::GetInstanceStartTime( const CStorySceneInstanceBuffer& instanceBuffer ) const
{
	return instanceBuffer[ i_startTimeMS ];
}

void CStorySceneEvent::SetInstanceScalingFactor( CStorySceneInstanceBuffer& instanceBuffer, Float scalingFactor ) const
{
	SCENE_ASSERT( scalingFactor >= 0.f );
	instanceBuffer[ i_scalingFactor ] = scalingFactor;
}

Float CStorySceneEvent::GetInstanceScalingFactor( const CStorySceneInstanceBuffer& instanceBuffer ) const
{
	return instanceBuffer[ i_scalingFactor ];
}

void CStorySceneEvent::SetInstanceDuration( CStorySceneInstanceBuffer& instanceBuffer, Float duration ) const
{
	SCENE_ASSERT( duration >= 0.f );
	instanceBuffer[ i_durationMS ] = duration;
}

Float CStorySceneEvent::GetInstanceDuration( const CStorySceneInstanceBuffer& instanceBuffer ) const
{
	return instanceBuffer[ i_durationMS ];
}

void CStorySceneEvent::EventInit( CStorySceneInstanceBuffer& data, const CStoryScenePlayer* scenePlayer ) const 
{
	data[ i_started ] = false;
	data[ i_finished ] = false;

	if ( scenePlayer->GetSceneDebugger() )
	{
		scenePlayer->GetSceneDebugger()->EventInit( this );
	}

	OnInit( data, const_cast< CStoryScenePlayer* >( scenePlayer ) );
}

void CStorySceneEvent::EventDeinit( CStorySceneInstanceBuffer& data, const CStoryScenePlayer* scenePlayer ) const 
{
	if ( scenePlayer->GetSceneDebugger() )
	{
		scenePlayer->GetSceneDebugger()->EventDeinit( this );
	}

	OnDeinit( data, const_cast< CStoryScenePlayer* >( scenePlayer ) );
}

void CStorySceneEvent::EventStart( CStorySceneInstanceBuffer& data, const CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo )	const 
{
	RED_FATAL_ASSERT( !data[ i_started ], "CStorySceneEvent: EventStart() called for event that has already started." );
	RED_FATAL_ASSERT( !data[ i_finished ], "CStorySceneEvent: EventStart() called for event that has already finished." );

	if ( scenePlayer->GetSceneDebugger() )
	{
		scenePlayer->GetSceneDebugger()->EventStart( this );
	}

	OnStart( data, const_cast< CStoryScenePlayer* >( scenePlayer ), collector, timeInfo );

	data[ i_started ] = true;
}

void CStorySceneEvent::EventProcess( CStorySceneInstanceBuffer& data, const CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	RED_FATAL_ASSERT( data[ i_started ], "CStorySceneEvent: EventProcess() called for event that has not yet started." );
	RED_FATAL_ASSERT( !data[ i_finished ], "CStorySceneEvent: EventProcess() called for event that has already finished." );

	if ( scenePlayer->GetSceneDebugger() )
	{
		scenePlayer->GetSceneDebugger()->EventProcess( this, timeInfo.m_timeLocal, timeInfo.m_progress, timeInfo.m_timeDelta );
	}

	OnProcess( data, const_cast< CStoryScenePlayer* >( scenePlayer ), collector, timeInfo );
}

void CStorySceneEvent::EventEnd( CStorySceneInstanceBuffer& data, const CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo )	const 
{
	RED_FATAL_ASSERT( data[ i_started ], "CStorySceneEvent: EventEnd() called for event that has not yet started." );
	RED_FATAL_ASSERT( !data[ i_finished ], "CStorySceneEvent: EventEnd() called for event that has already finished." );

	if ( scenePlayer->GetSceneDebugger() )
	{
		scenePlayer->GetSceneDebugger()->EventEnd( this );
	}

	OnEnd( data, const_cast< CStoryScenePlayer* >( scenePlayer ), collector, timeInfo );

	data[ i_finished ] = true;
}

void CStorySceneEvent::SetStartPosition( Float startPosition )
{
	SCENE_ASSERT( startPosition >= 0.f );
	m_startPosition = startPosition;
}

/*
Bakes event scale.

Note that this function doesn't change scaling factor currently used by event
so most of the time scaling factor should be reset after BakeScale() is called.
*/
void CStorySceneEvent::BakeScale( Float scalingFactor )
{
	DoBakeScaleImpl( scalingFactor );
}

void CStorySceneEvent::DoBakeScaleImpl( Float scalingFactor )
{
	// nothing
}

#ifdef DEBUG_SCENES_2
#pragma optimize("",on)
#endif
