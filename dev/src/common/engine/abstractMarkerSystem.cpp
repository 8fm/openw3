/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#ifndef NO_MARKER_SYSTEMS

#include "abstractMarkerSystem.h"
#include "../redThreads/redThreadsThread.h"

AbstractMarkerSystem::AbstractMarkerSystem( enum EMarkerSystemType systemType )
	: m_systemType( systemType )
	, m_updateLocked( false )
{
	/* intentionally empty */
}

AbstractMarkerSystem::~AbstractMarkerSystem()
{
	/* intentionally empty */
}

void AbstractMarkerSystem::SendMessage( enum EMarkerSystemMessage message )
{
	for( TDynArray< IMarkerSystemListener* >::const_iterator it = m_listeners.Begin(); it != m_listeners.End(); ++it )
	{
		( *it )->ProcessMessage( message, m_systemType, this );
	}
}

Bool AbstractMarkerSystem::CanBeUpdated()
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_updateMutex );
	return ( m_updateLocked == false );
}

void AbstractMarkerSystem::SendRequest( enum EMarkerSystemRequestType request )
{
	if( m_requestsQueue.Empty() == false && m_requestsQueue.Back() == request )
	{
		return;
	}
	m_requestsQueue.Push( request );
}

EMarkerSystemType AbstractMarkerSystem::GetSystemType() const
{
	return m_systemType;
}

void AbstractMarkerSystem::WaitForEntity()
{
	m_waitingForEntity = true;
}

Bool AbstractMarkerSystem::WaitingForEntity() const
{
	return m_waitingForEntity;
}

void AbstractMarkerSystem::RegisterListener( enum EMarkerSystemType systemType, IMarkerSystemListener* listener )
{
	if( systemType == m_systemType )
	{
		m_listeners.PushBackUnique( listener );
	}
}

void AbstractMarkerSystem::UnregisterListener( enum EMarkerSystemType systemType, IMarkerSystemListener* listener )
{
	if( systemType == m_systemType )
	{
		m_listeners.Remove( listener );
	}
}

void AbstractMarkerSystem::LockUpdate()
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_updateMutex );
	m_updateLocked = true;
}

void AbstractMarkerSystem::UnlockUpdate()
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_updateMutex );
	m_updateLocked = false;
}

#endif	// NO_MARKER_SYSTEMS
