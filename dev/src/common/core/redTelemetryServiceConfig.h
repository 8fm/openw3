#pragma once

#include "../../../internal/telemetry/telemetryInclude.h"

#include "settings.h"
#include "string.h"

class SRedTelemetryServiceConfig
{
public:
	SRedTelemetryServiceConfig()
		: useDefaultPlayerConfig( false )
	{
		// NOP
		this->serverPort	= 3000;
		this->dumpFileName	= nullptr;
#ifdef RED_PLATFORM_WINPC
		this->platform = Telemetry::P_PC;
#endif // RED_PLATFORM_WINPC
#ifdef RED_PLATFORM_DURANGO
		this->platform = Telemetry::P_XBONE;
#endif // RED_PLATFORM_DURANGO
#ifdef RED_PLATFORM_ORBIS
		this->platform = Telemetry::P_PS4;
#endif // RED_PLATFORM_ORBIS
		this->userCategory = Telemetry::UC_NONE;
		
	}

	SRedTelemetryServiceConfig( const String& language )
		: useDefaultPlayerConfig( true )
	{
		this->serverPort	= 3000;
		this->language		= language;
		this->dumpFileName	= nullptr;
#ifdef RED_PLATFORM_WINPC
		this->platform = Telemetry::P_PC;
#endif // RED_PLATFORM_WINPC
#ifdef RED_PLATFORM_DURANGO
		this->platform = Telemetry::P_XBONE;
#endif // RED_PLATFORM_DURANGO
#ifdef RED_PLATFORM_ORBIS
		this->platform = Telemetry::P_PS4;
#endif // RED_PLATFORM_ORBIS
		this->userCategory = Telemetry::UC_NONE;
	}

	// Deprecated
	SRedTelemetryServiceConfig( const String& deviceId, const String& language )
		: useDefaultPlayerConfig( false )
	{
		this->serverPort	= 3000;
		this->deviceId		= deviceId;
		this->language		= language;
		this->dumpFileName	= nullptr;
#ifdef RED_PLATFORM_WINPC
		this->platform = Telemetry::P_PC;
#endif // RED_PLATFORM_WINPC
#ifdef RED_PLATFORM_DURANGO
		this->platform = Telemetry::P_XBONE;
#endif // RED_PLATFORM_DURANGO
#ifdef RED_PLATFORM_ORBIS
		this->platform = Telemetry::P_PS4;
#endif // RED_PLATFORM_ORBIS
		this->userCategory = Telemetry::UC_NONE;
	}

	Bool LoadConfig( const String& fileName );

#if !defined ( NO_TELEMETRY_DEBUG )
	Bool SaveConfig( const String& fileName ) const;
#endif

	const Char* GetDumpFileName() const;
	
private:

	void SetUserBasedOnString( const String& user );
	void UserToString( String& user ) const;

	void SetPlatformBasedOnString( const String& platform );
	void PlatformToString( String& platform ) const;

public:
	Bool						useDefaultPlayerConfig;
	Int32						gameVersion;
	Int32						gameId;

	Int32						curlTimeout;

	Telemetry::EUserCategory	userCategory;
	String						deviceId;
	String						language;
	Telemetry::EPlatform		platform;

	Int32						batchCount;
	Int32						batchSeconds;

	String						serverHost;
	Uint32						serverPort;

	String						googleTrackingID;

private:
	String						dumpFileName;
	static const Char*			s_defaultDumpFileName; 
};