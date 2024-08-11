/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "redTelemetryServicesManager.h"
#include "redTelemetryServiceUpdateJob.h"
#include "redTelemetryServiceImplWin32.h"
#include "redTelemetryServiceDDI.h"

#include "taskManager.h"
#include "profiler.h"

//////////////////////////////////////////////////////////////////////////
CRedTelemetryServicesManager::CRedTelemetryServicesManager()
{

}

//////////////////////////////////////////////////////////////////////////
CRedTelemetryServicesManager::~CRedTelemetryServicesManager()
{
	for ( THashMap< String, SRedTelemetryServiceHandler* >::iterator it=m_handlers.Begin(); it!=m_handlers.End(); ++it )
	{
		if( it->m_second->m_interface )
		{
			delete it->m_second->m_interface;
		}
		delete it->m_second;
	}
	m_handlers.Clear();
}

//////////////////////////////////////////////////////////////////////////

void CRedTelemetryServicesManager::Update()
{
	Double seconds = m_timer.GetSeconds();

	for ( THashMap< String, SRedTelemetryServiceHandler* >::iterator it=m_handlers.Begin(); it!=m_handlers.End(); ++it )
	{
		if( it->m_second->m_interface->GetImmediatePost() == false )
		{
			if ( it->m_second->m_job && it->m_second->m_job->IsFinished() )
			{
				it->m_second->m_job->Release();
				it->m_second->m_job = nullptr;
			}	
			else if( !it->m_second->m_job )
			{
				Bool transferInProgress = it->m_second->m_interface->IsTransferInProgress();
				if(transferInProgress || (seconds - it->m_second->m_lastUpdateTime) > it->m_second->m_updateInterval)
				{
					StartUpdateJob( it->m_second );
					it->m_second->m_lastUpdateTime = seconds;
				}
			}
		}
			
	}
}

//////////////////////////////////////////////////////////////////////////

void CRedTelemetryServicesManager::StartUpdateJob( SRedTelemetryServiceHandler* handler ) const
{
	if( handler != nullptr && handler->m_interface != nullptr )
	{
		handler->m_job = new ( CTask::Root ) CRedTelemetryServiceUpdateJob( handler->m_interface );
		GTaskManager->Issue( *handler->m_job );
	}
}

//////////////////////////////////////////////////////////////////////////
void CRedTelemetryServicesManager::CancelUpdateJob( SRedTelemetryServiceHandler* handler ) const
{
	if( handler != nullptr )
	{
		if( handler->m_job ) //FlushService
		{
			handler->m_job->TryCancel();
			handler->m_job->Release();
			handler->m_job = nullptr;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
Bool CRedTelemetryServicesManager::FlushService( SRedTelemetryServiceHandler* handler )
{
	if( handler != nullptr )
	{
		RED_ASSERT( handler->m_interface, TXT( "RedTelemetryService cannot be retreived - instance not created" ) );
		if( handler->m_interface ) //FlushService
		{
			handler->m_interface->StopCollecting();
			handler->m_interface->Update( true );
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////
Bool CRedTelemetryServicesManager::AddService( const String& name )
{
	if( name.Empty() )
	{
		return false;
	}
	if( m_handlers.KeyExist( name ) == true )
	{
		return false;
	}
	SRedTelemetryServiceHandler *handler = new SRedTelemetryServiceHandler();
	handler->m_name = name;
	if( name == CRedTelemetryServiceDDI::s_serviceName )
	{
		handler->m_interface = new CRedTelemetryServiceDDI();	
	}
	else
	{
	//! temporary only for WIN32 platform. Telemetry will be available for all supported platforms
#if defined( RED_PLATFORM_WIN32 ) || defined ( RED_PLATFORM_DURANGO ) || defined( RED_PLATFORM_WIN64 )
		handler->m_interface = new CRedTelemetryServiceImplWin32();		
#else
		handler->m_interface = new IRedTelemetryServiceInterface();
#endif
	}

	RED_ASSERT( handler->m_interface != nullptr && handler->m_interface->IsServiceInitialized(), TXT( "RedTelemetryService system not created" ) );

	if( handler->m_interface && !handler->m_interface->IsServiceInitialized() )
	{
		delete handler->m_interface;
		handler->m_interface = new IRedTelemetryServiceInterface();
	}

	handler->m_job = nullptr;
	handler->m_updateInterval = 60;

	// init timer
	m_timer.GetSeconds( handler->m_lastUpdateTime );

	m_handlers.Insert( name, handler);

	return true;
}

//////////////////////////////////////////////////////////////////////////
Bool CRedTelemetryServicesManager::LoadConfForService( const String& name, const Char* pathToConfig, Telemetry::EBackendTelemetry backendName )
{
	THashMap< String, SRedTelemetryServiceHandler* >::iterator iterHandler = m_handlers.Find( name );

	if( iterHandler != m_handlers.End() )
	{
		// TODO: Setup player name and device id
#ifdef RED_PLATFORM_DURANGO
		SRedTelemetryServiceConfig tconfig( TXT( "XB1" ), TXT( "pl" ) );
#else
		SRedTelemetryServiceConfig tconfig( TXT( "pl" ) );
#endif // RED_PLATFORM_DURANGO

		

		Bool configLoaded = tconfig.LoadConfig( pathToConfig );

		RED_ASSERT( configLoaded, TXT( "Cannot load config for RedTelemetryService (bin/r4config/TelemetryServiceConfig.ini).") );
		RED_ASSERT( tconfig.gameId != 0, TXT( "RedTelemetryService was not configured properly. GameId == 0" ) );

		if( iterHandler->m_second->m_interface && configLoaded )
		{
			iterHandler->m_second->m_interface->Configure( tconfig, backendName );
			iterHandler->m_second->m_updateInterval = tconfig.batchSeconds;
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////
Bool CRedTelemetryServicesManager::RemoveService( const String& name, Bool withoutFlush /*=FALSE*/ )
{
	THashMap< String, SRedTelemetryServiceHandler* >::iterator iterHandler = m_handlers.Find( name );

	if( iterHandler != m_handlers.End() )
	{
		//Stop job
		CancelUpdateJob( iterHandler->m_second );
		if( withoutFlush == false )
		{
			FlushService( iterHandler->m_second );
		}

		m_handlers.Erase( name );
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////
IRedTelemetryServiceInterface* CRedTelemetryServicesManager::GetService( const String& name )
{
	THashMap< String, SRedTelemetryServiceHandler* >::iterator iterHandler = m_handlers.Find( name );

	if( iterHandler != m_handlers.End() )
	{
		if( iterHandler->m_second != nullptr )
		{
			return iterHandler->m_second->m_interface;
		}		
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////
Bool CRedTelemetryServicesManager::StartCollecting( const String& name )
{
	THashMap< String, SRedTelemetryServiceHandler* >::iterator iterHandler = m_handlers.Find( name );

	if( iterHandler != m_handlers.End() )
	{
		RED_ASSERT( iterHandler->m_second->m_interface, TXT( "RedTelemetryService cannot be retreived - instance not created" ) );
		if( iterHandler->m_second->m_interface ) //FlushService
		{
			iterHandler->m_second->m_interface->StartCollecting();
		}
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////
Bool CRedTelemetryServicesManager::StartSession( const String& name, const String& playerId, const String& parentSessionId )
{
	THashMap< String, SRedTelemetryServiceHandler* >::iterator iterHandler = m_handlers.Find( name );

	if( iterHandler != m_handlers.End() )
	{
		RED_ASSERT( iterHandler->m_second->m_interface, TXT( "RedTelemetryService cannot be retreived - instance not created" ) );
		if( iterHandler->m_second->m_interface ) //FlushService
		{
			return iterHandler->m_second->m_interface->StartSession( playerId, parentSessionId );
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////
Bool CRedTelemetryServicesManager::StopSession( const String& name )
{
	THashMap< String, SRedTelemetryServiceHandler* >::iterator iterHandler = m_handlers.Find( name );

	if( iterHandler != m_handlers.End() )
	{
		RED_ASSERT( iterHandler->m_second->m_interface, TXT( "RedTelemetryService cannot be retreived - instance not created" ) );
		if( iterHandler->m_second->m_interface ) //FlushService
		{
			return iterHandler->m_second->m_interface->StopSession();
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////
Bool CRedTelemetryServicesManager::StopCollecting( const String& name )
{
	THashMap< String, SRedTelemetryServiceHandler* >::iterator iterHandler = m_handlers.Find( name );

	if( iterHandler != m_handlers.End() )
	{
		 CancelUpdateJob( iterHandler->m_second );		
		 if( FlushService( iterHandler->m_second ) )
		 {
			 return true;
		 }
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////
Bool CRedTelemetryServicesManager::RemoveSessionTag( const String& serviceName, const String& tag )
{
	THashMap< String, SRedTelemetryServiceHandler* >::iterator iterHandler = m_handlers.Find( serviceName );

	if( iterHandler != m_handlers.End() )
	{
		RED_ASSERT( iterHandler->m_second->m_interface, TXT( "RedTelemetryService cannot be retreived - instance not created" ) );
		if( iterHandler->m_second->m_interface )
		{
			return iterHandler->m_second->m_interface->RemoveSessionTag(tag);
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////
void CRedTelemetryServicesManager::RemoveAllSessionTags( const String& serviceName )
{
	THashMap< String, SRedTelemetryServiceHandler* >::iterator iterHandler = m_handlers.Find( serviceName );

	if( iterHandler != m_handlers.End() )
	{
		RED_ASSERT( iterHandler->m_second->m_interface, TXT( "RedTelemetryService cannot be retreived - instance not created" ) );
		if( iterHandler->m_second->m_interface )
		{
			iterHandler->m_second->m_interface->RemoveAllSessionTags();
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CRedTelemetryServicesManager::AddSessionTag( const String& serviceName, const String& tag )
{
	THashMap< String, SRedTelemetryServiceHandler* >::iterator iterHandler = m_handlers.Find( serviceName );

	if( iterHandler != m_handlers.End() )
	{
		RED_ASSERT( iterHandler->m_second->m_interface, TXT( "RedTelemetryService cannot be retreived - instance not created" ) );
		if( iterHandler->m_second->m_interface )
		{
			iterHandler->m_second->m_interface->AddSessionTag( tag );
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CRedTelemetryServiceUpdateJob::Run()
{
	PC_SCOPE_PIX( CRedTelemetryServiceUpdateJob );

	RED_ASSERT( m_redTelemetryServiceInterface, TXT( "m_redTelemetryServiceInterface == NULL" ) );

	Bool shouldPostNewData = !m_redTelemetryServiceInterface->IsTransferInProgress();
	if( m_redTelemetryServiceInterface )
	{
		m_redTelemetryServiceInterface->Update( shouldPostNewData );
	}
}