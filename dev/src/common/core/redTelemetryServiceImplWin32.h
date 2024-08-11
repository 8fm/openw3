#pragma once

#include "../../../internal/telemetry/telemetryInclude.h"

#include "settings.h"
#include "redTelemetryServiceConfig.h"
#include "redTelemetryServiceInterface.h"
#include "string.h"
#include "pair.h"

typedef TPair< String, String > TelemetryParam;

class CRedTelemetryServiceImplWin32 : public IRedTelemetryServiceInterface, public Telemetry::ITelemetryManagerDelegate
{
public:

	CRedTelemetryServiceImplWin32();
	~CRedTelemetryServiceImplWin32();

	virtual void Log	( const String& name, const String& category );
	virtual void LogL	( const String& name, const String& category, const String& label );
	virtual void LogV	( const String& name, const String& category, const String& value );
	virtual void LogV	( const String& name, const String& category, Int32 value );
	virtual void LogV_WS( const String& name, const String& category, const String& value );
	virtual void LogVL	( const String& name, const String& category, const String& value, const String& label );
	virtual void LogVL	( const String& name, const String& category, Int32 value, const String& label );	
	virtual void LogVL_WS( const String& name, const String& category, const String& value, const String& label );

	virtual void LogEx	( const String& ex );

	virtual void SetCommonStatValue( const String& name, Int32 value );
	virtual void SetCommonStatValue( const String& name, Float value );
	virtual void SetCommonStatValue( const String& name, const String& value );

	virtual Bool RemoveSessionTag( const String& tag );
	virtual void RemoveAllSessionTags();
	virtual void AddSessionTag( const String& tag );
	
	Bool IsServiceInitialized() const { return m_libInterface != NULL; }

	Bool IsTransferInProgress() const;

	virtual void SetExternalSessionId( Telemetry::EBackendTelemetry backendName, const String& extSessionId );

	virtual const String& GetSessionId( Telemetry::EBackendTelemetry backendName );
		
	virtual void GetTime( double& time, unsigned long long& qpf, unsigned long long& qpc );

	virtual void SetImmediatePost( bool val );
	virtual Bool GetImmediatePost();

private:
	virtual void Configure( const SRedTelemetryServiceConfig& config, Telemetry::EBackendTelemetry backendName );

	virtual Bool StartCollecting();
	virtual Bool StartSession( const String& playerId, const String& parentSessionId );
	virtual void Update( Bool immediatePost );
	virtual Bool StopSession();
	virtual Bool StopCollecting();

	IFile* GetDumpedDataHandler();
public: // Telemetry delegate

	bool AppendDataToLocalStorage( const void* const dataBuffer, unsigned int bufferSize );

	unsigned int DataSize();
	bool ReadDataToBuffer( void* dataBuffer );
	void DeleteCachedData();

private:

	SRedTelemetryServiceConfig				m_config;
	Telemetry::CTelemetryInterfaceManager*	m_libInterface;
	IFile*									m_dumpedData;
	String									m_sessionIdRedString;
	TDynArray<String>						m_sessionTags;	
	Bool									m_immediatePost;	
};
