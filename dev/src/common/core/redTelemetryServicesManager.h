#pragma once

#include "redTelemetryServiceInterface.h"
#include "../../../internal/telemetry/telemetryInclude.h"
#include "string.h"
#include "hashmap.h"

class CTask;

struct SRedTelemetryServiceHandler
{
	String						m_name;
	IRedTelemetryServiceInterface*	m_interface;
	CTask*						m_job;
	Double						m_lastUpdateTime;
	Int32						m_updateInterval;
};
//! This class serves as access point for RedTelemetryServices interface.
//! It spawns jobs to update RedTelemetryServices in background
class CRedTelemetryServicesManager
{
public:
	CRedTelemetryServicesManager();
	~CRedTelemetryServicesManager();

	Bool AddService( const String& serviceName );
	Bool LoadConfForService( const String& serviceName, const Char* pathToConfig , Telemetry::EBackendTelemetry backendName );
	Bool RemoveService( const String& serviceName, Bool withoutFlush = false );
	
	IRedTelemetryServiceInterface* GetService( const String& serviceName ); 

	Bool StartCollecting( const String& serviceName );
	Bool StartSession( const String& serviceName, const String& playerId, const String& parentSessionId );
	void Update();
	Bool StopSession( const String& serviceName );
	Bool StopCollecting( const String& serviceName );

	Bool RemoveSessionTag( const String& serviceName, const String& tag );
	void RemoveAllSessionTags( const String& serviceName );
	void AddSessionTag( const String& serviceName, const String& tag );

private:

	void StartUpdateJob( SRedTelemetryServiceHandler* serviceHandler ) const;
	void CancelUpdateJob( SRedTelemetryServiceHandler* serviceHandler ) const;
	Bool FlushService( SRedTelemetryServiceHandler* serviceHandler );

private:

	Red::System::Timer					m_timer;

	THashMap< String, SRedTelemetryServiceHandler* >		m_handlers;
};
typedef TSingleton< CRedTelemetryServicesManager, TDefaultLifetime, TCreateUsingNew > SRedTelemetryServicesManager;

