/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "redTelemetryService.h"

//////////////////////////////////////////////////////////////////////////
//! This class is a singleton, so its creation is thread-safe
CRedTelemetryService::CRedTelemetryService( const Char* serviceName, Telemetry::EBackendTelemetry backendName )
{
	m_serviceName = serviceName;
	SRedTelemetryServicesManager::GetInstance().AddService( m_serviceName );
	m_backendName = backendName;
	m_interface = nullptr;
}

//////////////////////////////////////////////////////////////////////////
CRedTelemetryService::~CRedTelemetryService()
{
	SRedTelemetryServicesManager::GetInstance().RemoveService( m_serviceName, true );
}

//////////////////////////////////////////////////////////////////////////
void CRedTelemetryService::ManualShutDownService()
{
	SRedTelemetryServicesManager::GetInstance().RemoveService( m_serviceName, false );
}

//////////////////////////////////////////////////////////////////////////
IRedTelemetryServiceInterface* CRedTelemetryService::GetInterface()
{
	return 	m_interface;
}

//////////////////////////////////////////////////////////////////////////
void CRedTelemetryService::LoadConfig( const String& fileName )
{
	if( SRedTelemetryServicesManager::GetInstance().LoadConfForService( m_serviceName , fileName.AsChar(), m_backendName ) )
	{
		m_interface = SRedTelemetryServicesManager::GetInstance().GetService( m_serviceName );
	}
}

//////////////////////////////////////////////////////////////////////////
Bool CRedTelemetryService::StartCollecting( )
{
	RED_ASSERT( m_interface, TXT( "CRedTelemetryService cannot be retreived - instance not created" ) );
	if( m_interface )
	{
		return SRedTelemetryServicesManager::GetInstance().StartCollecting( m_serviceName );
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////
Bool CRedTelemetryService::StartSession( const String& playerId, const String& parentSessionId)
{
	RED_ASSERT( m_interface, TXT( "CRedTelemetryService cannot be retreived - instance not created" ) );
	if( m_interface )
	{
		return SRedTelemetryServicesManager::GetInstance().StartSession( m_serviceName, playerId, parentSessionId );
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////
Bool CRedTelemetryService::StopSession()
{
	RED_ASSERT( m_interface, TXT( "CRedTelemetryService cannot be retreived - instance not created" ) );
	if( m_interface )
	{
		return SRedTelemetryServicesManager::GetInstance().StopSession( m_serviceName );
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////
Bool CRedTelemetryService::StopCollecting()
{
	RED_ASSERT( m_interface, TXT( "CRedTelemetryService cannot be retreived - instance not created" ) );
	if( m_interface )
	{
		return SRedTelemetryServicesManager::GetInstance().StopCollecting( m_serviceName );
	}
	return false;
}

void CRedTelemetryService::SetImmediatePost( Bool val )
{
	RED_ASSERT( m_interface, TXT( "CRedTelemetryService cannot be retreived - instance not created" ) );
	if( m_interface )
	{
		m_interface->SetImmediatePost( val );
	}
}