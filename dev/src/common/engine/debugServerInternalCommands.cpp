/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
* DrUiD
*/

#include "build.h"


#ifndef NO_DEBUG_SERVER

//////////////////////////////////////////////////////////////////////////
// headers
#include "debugServerManager.h"
#include "debugServerInternalCommands.h"
#include "debugServerHelpers.h"
#include "../core/redTelemetryServicesManager.h"
#include "../core/version.h"
#include "../core/memoryFileWriter.h"
#include "scriptInvoker.h"


//////////////////////////////////////////////////////////////////////////
// implementations

//////////////////////////////////////////////////////////////////////////
//
// areWeConnected
Uint32 CDebugServerCommandAreWeConnected::ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data )
{
	RED_UNUSED( owner );

	// set as connected
	const AnsiChar* channelName = UNICODE_TO_ANSI( data[1].AsChar() );
	Bool debugServer = true;
	if ( Red::System::StringCompare( channelName, RED_NET_CHANNEL_RES_SERVER ) == 0 )
	{
		debugServer = false;
	}
	DBGSRV_CALL( SetConnected( !debugServer ) );

	// init
	Red::Network::ChannelPacket packet( channelName );
	packet.WriteString( "yes" );

	// fill data
	/*1*/packet.WriteString( UNICODE_TO_ANSI( ToString( DebugServerAPI ).AsChar() ) );
	/*2*/packet.WriteString( DBGSRV_CALL( GetName() ) );
	/*3.11*/packet.WriteString( UNICODE_TO_ANSI( String( APP_VERSION_NUMBER ).AsChar() ) );
	/*4.11*/packet.WriteString( UNICODE_TO_ANSI( String( APP_DATE ).AsChar() ) );

	// send
	Red::Network::Manager::GetInstance()->Send( channelName, packet );

	// send is attached
	if ( debugServer && DBGSRV_CALL( IsAttached() ) )
	{
		DBGSRV_CALL( SendWorldAttached() );
	}

	// send is game running
	if ( debugServer && DBGSRV_CALL( IsGameRunning() ) )
	{
		DBGSRV_CALL( SendGameStarted() );
	}

	// processed
	return 1;	
};
//////////////////////////////////////////////////////////////////////////
//
// disconnect
Uint32 CDebugServerCommandDisconnect::ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data )
{
	RED_UNUSED( owner );

	// set as disconnect
	const AnsiChar* channelName = UNICODE_TO_ANSI( data[0].AsChar() );
	Bool debugServer = true;
	if ( Red::System::StringCompare( channelName, RED_NET_CHANNEL_RES_SERVER ) == 0 )
	{
		debugServer = false;
	}
	DBGSRV_CALL( SetDisconnected( debugServer ) );

	// processed
	return 1;
};
//////////////////////////////////////////////////////////////////////////
//
// enable frame time 
Uint32 CDebugServerCommandEnableFrameTime::ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data )
{
	RED_UNUSED( owner );

	// parse request
	const Char* str = data[0].AsChar();
	Bool enable = false;
	GParseBool( str, enable );
	DBGSRV_CALL( EnableFrameTimeLogging( enable ) );

	// processed
	return 1;
};

class ConsoleExecResultSender : public Red::System::Log::OutputDevice
{
public:
	ConsoleExecResultSender( const String& command )
	{
		const Bool result = CScriptInvoker::Parse( command );
		Red::Network::ChannelPacket packet( RED_NET_CHANNEL_DEBUG_SERVER );
		packet.WriteString( "consoleExecResult" );
		packet.WriteString( result ? "succeed" : "invalid command" );
		DBGSRV().Send( RED_NET_CHANNEL_DEBUG_SERVER, packet );
	}

	void Write( const Red::System::Log::Message& message ) override
	{
		Red::Network::ChannelPacket packet( RED_NET_CHANNEL_DEBUG_SERVER );
		packet.WriteString( "consoleExecResult" );
		packet.WriteString( UNICODE_TO_ANSI( message.text ) );
		DBGSRV().Send( RED_NET_CHANNEL_DEBUG_SERVER, packet );
	}
};

//////////////////////////////////////////////////////////////////////////
//
// consoleExec
Uint32 CDebugServerCommandConsoleExec::ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data )
{
	RED_UNUSED( owner );

	if ( !GGame->GetActiveWorld() || data.Empty() )
		return 0;

	ConsoleExecResultSender consoleSender( data[0] );
	return 0;
}
//////////////////////////////////////////////////////////////////////////
//
// startPropsTrace
Uint32 CDebugServerCommandStartPropsTrace::ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data )
{
	RED_UNUSED( owner );

	DBGSRV_CALL( StartPropsTrace() );

	// processed
	return 1;
};
//////////////////////////////////////////////////////////////////////////
//
// finishPropsTrace
Uint32 CDebugServerCommandFinishPropsTrace::ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data )
{
	RED_UNUSED( owner );

	DBGSRV_CALL( FinishPropsTrace() );

	// processed
	return 1;
};


//////////////////////////////////////////////////////////////////////////
//
// initProfile
Uint32 CDebugServerCommandInitProfile::ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data )
{
	RED_UNUSED( owner );

	Red::Network::ChannelPacket packet( RED_NET_CHANNEL_DEBUG_SERVER );
	/*1*/packet.WriteString( "initProfileResult" );
	Bool init = false;

	#ifdef NEW_PROFILER_ENABLED
	if ( data.Size() > 1 )
	{
		Uint32 mem = 0;
		const Char* str = data[0].AsChar();
		if ( GParseInteger( str, mem ) && mem > 0 )
		{
			PROFILER_Init( mem );
			init = true;
		}
	}
	#endif

	/*2*/packet.WriteString( init ? "1" : "0" );

	// send
	DBGSRV().Send( RED_NET_CHANNEL_DEBUG_SERVER, packet );

	// processed
	return 1;
};
//////////////////////////////////////////////////////////////////////////
//
// startProfile
Uint32 CDebugServerCommandStartProfile::ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data )
{
	RED_UNUSED( owner );
	
	Red::Network::ChannelPacket packet( RED_NET_CHANNEL_DEBUG_SERVER );
	/*1*/packet.WriteString( "startProfileResult" );

	#ifdef NEW_PROFILER_ENABLED
	PROFILER_Start();
	/*2*/packet.WriteString( "1" );
	#else
	/*2*/packet.WriteString( "0" );
	#endif

	// send
	DBGSRV().Send( RED_NET_CHANNEL_DEBUG_SERVER, packet );

	// processed
	return 1;
};
//////////////////////////////////////////////////////////////////////////
//
// stopProfile
Uint32 CDebugServerCommandStopProfile::ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data )
{
	RED_UNUSED( owner );

	Red::Network::ChannelPacket packet( RED_NET_CHANNEL_DEBUG_SERVER );
	/*1*/packet.WriteString( "stopProfileResult" );

	#ifdef NEW_PROFILER_ENABLED
	PROFILER_Stop();
	/*2*/packet.WriteString( "1" );
	#else
	/*2*/packet.WriteString( "0" );
	#endif

	// send
	DBGSRV().Send( RED_NET_CHANNEL_DEBUG_SERVER, packet );

	// processed
	return 1;
};
//////////////////////////////////////////////////////////////////////////
//
// storeProfile
Uint32 CDebugServerCommandStoreProfile::ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data )
{
	RED_UNUSED( owner );

	#ifdef NEW_PROFILER_ENABLED
	String sessionId;
	Uint64 sessionQpc = 0;
	Uint64 sessionQpf = 0;

	PROFILER_Store( String::EMPTY, sessionId, sessionQpc, sessionQpf );

	Red::Network::ChannelPacket packet( RED_NET_CHANNEL_DEBUG_SERVER );
	/*1*/packet.WriteString( "storeProfileResult" );
	/*2*/packet.WriteString( UNICODE_TO_ANSI( PROFILER_GetLastStorePath().AsChar() ) );

	// send
	DBGSRV().Send( RED_NET_CHANNEL_DEBUG_SERVER, packet );
	#endif

	// processed
	return 1;
};
//////////////////////////////////////////////////////////////////////////
//
// sendProfile
const Uint32 buffSize = 60000;
Uint32 CDebugServerCommandSendProfile::ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data )
{
	RED_UNUSED( owner );

	#ifdef NEW_PROFILER_ENABLED
	// stop
	PROFILER_Stop();

	// fill buffer
	TDynArray< Uint8 > buffer;
	CMemoryFileWriter writer( buffer );
	PROFILER_StoreToMem( &writer );

	// sending data
	Uint32 packets = buffer.Size()/buffSize;
	Uint32 packetsRest = buffer.Size()%buffSize;

	Red::Network::ChannelPacket packet( RED_NET_CHANNEL_DEBUG_SERVER );
	for ( Uint32 no = 0; no < packets; ++no )
	{
		packet.Clear( RED_NET_CHANNEL_DEBUG_SERVER );
		/*1*/packet.WriteString( "sendProfileResult" );
		/*2*/packet.WriteArray( (Uint8*)buffer.Data()+no*buffSize, buffSize );
		DBGSRV().Send( RED_NET_CHANNEL_DEBUG_SERVER, packet );
	}

	// sending rest
	if ( packetsRest != 0 )
	{
		packet.Clear( RED_NET_CHANNEL_DEBUG_SERVER );
		/*1*/packet.WriteString( "sendProfileResult" );
		/*2*/packet.WriteArray( (Uint8*)buffer.Data()+packets*buffSize, packetsRest );
		DBGSRV().Send( RED_NET_CHANNEL_DEBUG_SERVER, packet );
	}

	// eof
	packet.Clear( RED_NET_CHANNEL_DEBUG_SERVER );
	/*1*/packet.WriteString( "sendProfileResult" );
	/*2*/packet.WriteString( "eof" );
	DBGSRV().Send( RED_NET_CHANNEL_DEBUG_SERVER, packet );

	// processed
	return packets + !!packetsRest + 1;
	#else
	return 0;
	#endif
};
//////////////////////////////////////////////////////////////////////////
//
// statusProfile
Uint32 CDebugServerCommandStatusProfile::ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data )
{
	RED_UNUSED( owner );

	#ifdef NEW_PROFILER_ENABLED
	// init
	Red::Network::ChannelPacket packet( RED_NET_CHANNEL_DEBUG_SERVER );
	/*1*/packet.WriteString( "statusProfileResult" );

	// status
	const Float usageFactor = PROFILER_GetBufferUsage();
	if ( PROFILER_IsRecording() || usageFactor > 0.99f )
	{
		/*2*/packet.WriteString( UNICODE_TO_ANSI( ToString( usageFactor*100.0f ).AsChar() ) );
	}
	else
	{
		/*2*/packet.WriteString( "0.0" );
	}

	// send
	DBGSRV().Send( RED_NET_CHANNEL_DEBUG_SERVER, packet );
	#endif

	// processed
	return 1;
};
//////////////////////////////////////////////////////////////////////////
//
// toggleScriptsProfile
Uint32 CDebugServerCommandToggleScriptsProfile::ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data )
{
	RED_UNUSED( owner );

	#ifdef NEW_PROFILER_ENABLED
	// init
	Int32 result = 0;

	// toggle state
	#ifdef NEW_PROFILER_ENABLED
		const Bool enable = SScriptProfilerManager::GetInstance().IsEnableProfileFunctionCalls();
		SScriptProfilerManager::GetInstance().EnableProfileFunctionCalls( !enable );
		result = !enable;
	#else
		result = -1;
	#endif

	// send result
	Red::Network::ChannelPacket packet( RED_NET_CHANNEL_DEBUG_SERVER );
	/*1*/packet.WriteString( "toggleScriptsProfileResult" );
	/*2*/packet.WriteString( UNICODE_TO_ANSI( ToString( result ).AsChar() ) );

	// send
	DBGSRV().Send( RED_NET_CHANNEL_DEBUG_SERVER, packet );
	#endif

	// processed
	return 1;
};

#endif


//////////////////////////////////////////////////////////////////////////
// EOF
