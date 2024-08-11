#pragma once

#include "../../../internal/telemetry/telemetryInclude.h"
#include "string.h"

typedef void ( *StatsFunc ) ( String& );

class SRedTelemetryServiceConfig;

//! This interface serves only as a wrapper to allow compilation for XBOX/PS4
//! without conditional compilation
//! Probably it will be changed/removed after RedTelemetry lib will be compiled on all platforms
class IRedTelemetryServiceInterface
{
public:

	IRedTelemetryServiceInterface() {}
	virtual ~IRedTelemetryServiceInterface() {}

	//! Those methods are lightweight, they only puts event to lock-less queue
	virtual void Log	( const String& name, const String& category ) { RED_UNUSED( category ); RED_UNUSED( name ); }
	virtual void LogL	( const String& name, const String& category, const String& label ) { RED_UNUSED( category ); RED_UNUSED( name ); RED_UNUSED( label ); }
	virtual void LogV	( const String& name, const String& category, const String& value ) { RED_UNUSED( category ); RED_UNUSED( name ); RED_UNUSED( value ); }
	virtual void LogV	( const String& name, const String& category, Int32 value ) { RED_UNUSED( category ); RED_UNUSED( name ); RED_UNUSED( value ); }
	virtual void LogV_WS( const String& name, const String& category, const String& value ) { RED_UNUSED( category ); RED_UNUSED( name ); RED_UNUSED( value ); }
	virtual void LogVL	( const String& name, const String& category, const String& value, const String& label ) { RED_UNUSED( category ); RED_UNUSED( name ); RED_UNUSED( label ), RED_UNUSED( value ); }
	virtual void LogVL	( const String& name, const String& category, Int32 value, const String& label ) { RED_UNUSED( category ); RED_UNUSED( name ); RED_UNUSED( label ), RED_UNUSED( value ); }
	virtual void LogVL_WS( const String& name, const String& category, const String& value, const String& label ) { RED_UNUSED( category ); RED_UNUSED( name ); RED_UNUSED( label ), RED_UNUSED( value ); }

	virtual void LogEx	( const String& ex ) { RED_UNUSED( ex ); }

	virtual void SetCommonStatValue( const String& name, Int32 value ) { RED_UNUSED( name ); RED_UNUSED( value ); }
	virtual void SetCommonStatValue( const String& name, Float value ) { RED_UNUSED( name ); RED_UNUSED( value ); }
	virtual void SetCommonStatValue( const String& name, const String& value ) { RED_UNUSED( name ); RED_UNUSED( value ); }

	virtual Bool RemoveSessionTag( const String& tag ){ RED_UNUSED( tag ); return false; }
	virtual void RemoveAllSessionTags(){}
	virtual void AddSessionTag( const String& tag ){ RED_UNUSED( tag ); }

	//! Check if service is processing any request at the moment
	virtual Bool IsTransferInProgress() const { return false; }

	virtual Bool IsServiceInitialized() const { return true; }

	virtual void SetExternalSessionId( Telemetry::EBackendTelemetry backendName, const String& extSessionId ){ RED_UNUSED( backendName ); RED_UNUSED( extSessionId ); }

	virtual const String& GetSessionId( Telemetry::EBackendTelemetry backendName ){ RED_UNUSED( backendName ); return s_emptySessionName; }

	// return current timer value (can be used to link different data streams)
	virtual void GetTime( double& time, unsigned long long& qpf, unsigned long long& qpc ) { RED_UNUSED( time ); RED_UNUSED( qpf ); RED_UNUSED( qpc ); }

	virtual void SetImmediatePost( bool val ){ RED_UNUSED( val ); }
	virtual Bool GetImmediatePost(){ return false; }

public:

	//! Pass config struct here to change default service config
	virtual void Configure( const SRedTelemetryServiceConfig& , Telemetry::EBackendTelemetry ){}

	//! This method effectively begins event collecting. No event will be logged before calling this method
	//! This method will request saved data ( CTelemetry::DataSize, CTelemetry::ReadDataToBuffer, CTelemetry::DeleteCachedData )
	//! Prepare cached data in memory before calling this method
	//! If method return FALSE  previous StartCollecting call was not yet executed
	virtual Bool StartCollecting() { return true; }

	//! This method starts collecting session. 
	//! Not all telemetry implementations respect sessions (for now only implementations base on HTTP respect sessions)
	//! Before this method StartCollecting must be called
	//! If method return FALSE  previous StartSession call was not yet executed
	virtual Bool StartSession( const String& playerId, const String& parentSessionId ) { RED_UNUSED( playerId ); RED_UNUSED( parentSessionId ); return true; }

	//! This method should be called from another ( not main ) thread
	//! It can take significant amount of time ( call async HTTPS request / flush data to local storage on exit )
	//! Calling this method can trigger request to save data ( CTelemetry::AppendDataToLocalStorage )
	//! @param immediatePost - Discard internal batching system - post gathered data immediately
	virtual void Update( Bool immediatePost ) { RED_UNUSED( immediatePost ); }

	//! This method stops collecting session. 
	//! If method return FALSE  previous StopSession call was not yet executed
	virtual Bool StopSession() { return true; }

	//! This method is lightweight, it only changes state
	//! If method return FALSE  previous StopCollecting call was not yet executed
	virtual Bool StopCollecting() { return true; }

	static String s_emptySessionName;
};
