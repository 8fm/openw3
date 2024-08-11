#pragma once

#include "../../common/core/redTelemetryServicesManager.h"

class CRedTelemetryService
{
public:

	CRedTelemetryService( const Char* serviceName, Telemetry::EBackendTelemetry backendName );
	~CRedTelemetryService();

	IRedTelemetryServiceInterface* GetInterface();

	void LoadConfig( const String& fileName );

	//! If one of below methods return FALSE it mean that previous call was not yet executed
	Bool StartCollecting();
	Bool StartSession( const String& playerId, const String& parentSessionId );
	Bool StopSession();
	Bool StopCollecting();

	void SetImmediatePost( Bool val );

	void ManualShutDownService();

private:

	IRedTelemetryServiceInterface*		m_interface;
	String								m_serviceName;
	Telemetry::EBackendTelemetry		m_backendName;
};

