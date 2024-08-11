/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#ifndef NO_MARKER_SYSTEMS

#include "poiSystem.h"
#include "reviewSystem.h"
#include "stickersSystem.h"
#include "entity.h"
#include "bitmapTexture.h"

namespace
{
	const Double GThreadIdleTime = 1.0;	// seconds
}

// Marker system manager background update implementation
CMarkersSystemThread::CMarkersSystemThread( IMarkerSystemInterface* systemToUpdate ) 
	: CThread( "MarkersSystemUpdateThread" )
	, m_systemToUpdate( systemToUpdate )
{ 
	m_lastUpdate = Red::System::Clock::GetInstance().GetTimer().GetSeconds();
}

CMarkersSystemThread::~CMarkersSystemThread() 
{ 
	/*intentionally empty*/ 
}

void CMarkersSystemThread::ThreadFunc()
{
	do
	{
		Double actuallyTime = Red::System::Clock::GetInstance().GetTimer().GetSeconds();

		if( ( actuallyTime - m_lastUpdate ) > GThreadIdleTime )
		{
			 m_systemToUpdate->BackgroundUpdate();
			 m_lastUpdate = Red::System::Clock::GetInstance().GetTimer().GetSeconds();
		}
		
		Red::Threads::SleepOnCurrentThread( 1 );

	}while( GIsClosing == false );
}

// Marker system manager implementation
CMarkersSystem::CMarkersSystem() 
	: m_systemType( MST_MarkerManager )
	, m_systemsAreOn( false )
{
	/* intentionally empty */
}

CMarkersSystem::~CMarkersSystem()
{
	/* intentionally empty */
}

void CMarkersSystem::Initialize()
{
	CreateSubsystems();

	m_thread = new CMarkersSystemThread( this );
	m_thread->InitThread();
}

void CMarkersSystem::Tick( Float timeDelta )
{
	for( auto it = m_systems.Begin(); it != m_systems.End(); ++it )
	{
		IMarkerSystemInterface* system = it->m_second;
		system->Tick( timeDelta );
	}
}

void CMarkersSystem::Shutdown()
{
	// kill process
	if( m_thread != nullptr )
	{
		m_thread->JoinThread();
		delete m_thread;
		m_thread = nullptr;
	}

	for( MarkerSystemsMap::iterator it = m_systems.Begin(); it != m_systems.End(); ++it )
	{
		IMarkerSystemInterface* system = it->m_second;
		system->Shutdown();
		delete system;
	}
}

void CMarkersSystem::RegisterMarkerSystem( enum EMarkerSystemType systemType, IMarkerSystemInterface* newSystem )
{
	if( newSystem != nullptr )
	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_updateGuard );
		m_systems.Insert( systemType, newSystem );
		for( TDynArray< IMarkerSystemListener* >::const_iterator it = m_listeners.Begin(); it != m_listeners.End(); ++it )
		{
			( *it )->ProcessMessage( MSM_SystemRegistered, systemType, newSystem );
		}
	}
}

void CMarkersSystem::BackgroundUpdate()
{
	if( IsActive() == true )
	{		
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_updateGuard );
		// update
		for( MarkerSystemsMap::iterator it = m_systems.Begin(); it != m_systems.End(); ++it )
		{
			IMarkerSystemInterface* system = it->m_second;
			if( system->CanBeUpdated() == true )
			{
				system->BackgroundUpdate();
			}
		}
	}
}

bool CMarkersSystem::WaitingForEntity() const
{
	for( MarkerSystemsMap::const_iterator it = m_systems.Begin(); it != m_systems.End(); ++it )
	{
		IMarkerSystemInterface* system = it->m_second;
		if( system->WaitingForEntity() == true )
		{
			return true;
		}
	}
	return false;
}

void CMarkersSystem::SetNewEntity(CEntity* newEntity)
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_updateGuard );
	for( MarkerSystemsMap::iterator it = m_systems.Begin(); it != m_systems.End(); ++it )
	{
		IMarkerSystemInterface* system = it->m_second;
		if( system->WaitingForEntity() == true )
		{
			system->SetNewEntity(newEntity);
			return;
		}
	}
}

void CMarkersSystem::WaitForEntity()
{
	/*intentionally empty*/
}

void CMarkersSystem::Connect()
{
	/* intentionally empty */
}

Bool CMarkersSystem::IsActive() const
{
	return m_systemsAreOn;
}

void CMarkersSystem::TurnOnSystems()
{
	m_systemsAreOn = true;
}

void CMarkersSystem::TurnOffSystems()
{
	m_systemsAreOn = false;
}

void CMarkersSystem::RegisterListener( enum EMarkerSystemType systemType, IMarkerSystemListener* listener )
{
	IMarkerSystemInterface** systemPtr = m_systems.FindPtr( systemType );
	if( systemPtr != nullptr )
	{
		IMarkerSystemInterface* system = ( *systemPtr );
		system->RegisterListener( systemType, listener );
		listener->ProcessMessage( MSM_SystemRegistered, systemType, system );
	}
	m_listeners.PushBackUnique( listener );
}

void CMarkersSystem::UnregisterListener( enum EMarkerSystemType systemType, IMarkerSystemListener* listener )
{
	IMarkerSystemInterface** systemPtr = m_systems.FindPtr( systemType );
	if( systemPtr != nullptr )
	{
		IMarkerSystemInterface* system = ( *systemPtr );
		system->UnregisterListener( systemType, listener );
	}
	m_listeners.Remove( listener );
}

void CMarkersSystem::CreateSubsystems()
{
	IMarkerSystemInterface* subsystems = nullptr;

	// create review marker system
	subsystems = new CReviewSystem();
	subsystems->Initialize();
	RegisterMarkerSystem( MST_Review, subsystems );

	// create poi marker system
	subsystems = new CPoiSystem();
	subsystems->Initialize();
	RegisterMarkerSystem( MST_POI, subsystems );

	// create sticker marker system
	subsystems = new CStickersSystem();
	subsystems->Initialize();
	RegisterMarkerSystem( MST_Sticker, subsystems );
}

EMarkerSystemType CMarkersSystem::GetSystemType() const
{
	return m_systemType;
}

IMarkerSystemInterface* CMarkersSystem::GetSystem( enum EMarkerSystemType systemType )
{
	IMarkerSystemInterface** systemPtr = m_systems.FindPtr( systemType );
	if( systemPtr != nullptr )
	{
		return ( *systemPtr );
	}
	return nullptr;
}

void CMarkersSystem::SendRequest( enum EMarkerSystemRequestType request )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_updateGuard );
	for( MarkerSystemsMap::iterator it = m_systems.Begin(); it != m_systems.End(); ++it )
	{
		IMarkerSystemInterface* system = it->m_second;
		system->SendRequest( request );
	}
}

#endif	// NO_MARKER_SYSTEMS
