/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
* DrUiD
*/

#include "build.h"


#ifndef NO_DEBUG_SERVER

//////////////////////////////////////////////////////////////////////////
// headers
#include "debugServerManager.h"
#include "debugServerHelpers.h"
#include "debugServerPlugin.h"
#include "debugServerInternalPlugin.h"

#include "baseengine.h"
#include "renderer.h"


//////////////////////////////////////////////////////////////////////////
// consts
const Double DebugServerSendStatsTime = 1.0;	// once per second


//////////////////////////////////////////////////////////////////////////
// implementations

//////////////////////////////////////////////////////////////////////////
//
// ctor
CDebugServerManager::CDebugServerManager() :
	m_name( "RED Debug Server" ),
	m_debugMode( false ),
	m_sendStatsEnabled( false ),
	m_frameTimeLogging( false )
{
	// stats
	m_commandsTime.SetValue( 0 );
	m_sentCount.SetValue( 0 );
	m_receivedCount.SetValue( 0 );
	m_frame.SetValue( 0 );

	// states
	m_connected.SetValue( false );
	m_connectedAsResourceServer.SetValue( false );
	m_attached.SetValue( false );
	m_gameRunning.SetValue( false );
	m_initialized.SetValue( false );
	m_propsTrace.SetValue( false );
}
//////////////////////////////////////////////////////////////////////////
//
// dtor
CDebugServerManager::~CDebugServerManager()
{
}
//////////////////////////////////////////////////////////////////////////
//
// init
Bool CDebugServerManager::Init()
{
	// init
	m_gameThreadMutex.SetSpinCount( 100 );

	// register network listeners
	Red::Network::Manager::GetInstance()->RegisterListener( RED_NET_CHANNEL_DEBUG_SERVER, this );
	Red::Network::Manager::GetInstance()->RegisterListener( RED_NET_CHANNEL_RES_SERVER, this );

	// internal plugin
	DBGSRV_REG_PLUGIN( new CDebugServerInternalPlugin() );

	// init all plugins
	for ( TList<CDebugServerPlugin*>::iterator it = m_plugins.Begin(); it != m_plugins.End(); ++it )
	{
		(*it)->Init();
	}

	// initialized
	m_initialized.SetValue( true );
	return true;
}
//////////////////////////////////////////////////////////////////////////
//
// shutdown
Bool CDebugServerManager::ShutDown()
{
	RED_THREADS_MEMORY_BARRIER();
	m_initialized.SetValue( false );
	m_connected.SetValue( false );

	// unregister network listeners
	Red::Network::Manager::GetInstance()->UnregisterListener( RED_NET_CHANNEL_DEBUG_SERVER, this );
	Red::Network::Manager::GetInstance()->UnregisterListener( RED_NET_CHANNEL_RES_SERVER, this );

	// remove commands queue
	m_gameThreadMutex.Acquire();
	for ( TList<SDebugServerCommandData*>::iterator it = m_gameThreadCommands.Begin(); it != m_gameThreadCommands.End(); ++it )
	{
		delete (*it);
	}
	m_gameThreadCommands.Clear();
	m_gameThreadMutex.Release();

	// remove all plugins
	for ( TList<CDebugServerPlugin*>::iterator it = m_plugins.Begin(); it != m_plugins.End(); ++it )
	{
		(*it)->ShutDown();
		delete (*it);
	}
	m_plugins.Clear();

	// result
	return true;
}
//////////////////////////////////////////////////////////////////////////
//
// send packet
void CDebugServerManager::Send( const AnsiChar* channelName, const Red::Network::ChannelPacket& packet )
{
	Red::Network::Manager::GetInstance()->Send( channelName, packet );
}
//////////////////////////////////////////////////////////////////////////
//
// game is started
void CDebugServerManager::GameStarted()
{
	if ( !m_initialized.GetValue() )
		return;

	// set state
	RED_THREADS_MEMORY_BARRIER();
	ASSERT( !m_gameRunning.GetValue(), TXT( "DbgSrv: game already started!" ) );
	m_gameRunning.SetValue( true );

	// debug
	if ( m_debugMode )
	{
		Log( TXT( "DbgSrv" ), TXT( "GameStarted" ) );
	}

	// send to clients
	if ( m_connected.GetValue() )
	{
		SendGameStarted();

		// broadcast to all plugins
		for ( TList<CDebugServerPlugin*>::iterator it = m_plugins.Begin(); it != m_plugins.End(); ++it )
		{
			(*it)->GameStarted();
		}
	}
}
//////////////////////////////////////////////////////////////////////////
//
// game stopped
void CDebugServerManager::GameStopped()
{
	if ( !m_initialized.GetValue() )
		return;

	// set state
	RED_THREADS_MEMORY_BARRIER();
	ASSERT( m_gameRunning.GetValue(), TXT( "DbgSrv: game already stopped!" ) );
	m_gameRunning.SetValue( false );

	// debug
	if ( m_debugMode )
	{
		Log( TXT( "DbgSrv" ), TXT( "GameStopped" ) );
	}

	// send to clients
	if ( m_connected.GetValue() )
	{
		SendGameStopped();

		// broadcast to all plugins
		for ( TList<CDebugServerPlugin*>::iterator it = m_plugins.Begin(); it != m_plugins.End(); ++it )
		{
			(*it)->GameStopped();
		}
	}
}
//////////////////////////////////////////////////////////////////////////
//
// attach to world
void CDebugServerManager::AttachToWorld()
{
	// fill data
	const String& worldName = GGame->GetActiveWorld()->GetFriendlyName();
	const Uint32 worldNameHash = ::GetHash( worldName );
	LOG_ENGINE( TXT( "DBGSRV: world attached - name: [%ls] - hash: %u" ), worldName.AsChar(), worldNameHash );

	if ( !m_initialized.GetValue() )
		return;

	// now is possible to get world data
	RED_THREADS_MEMORY_BARRIER();
	ASSERT( !m_attached.GetValue(), TXT( "DbgSrv: already attached to world!" ) );
	m_attached.SetValue( true );

	// debug
	if ( m_debugMode )
	{
		Log( TXT( "DbgSrv" ), TXT( "AttachToWorld" ) );
	}

	// send to clients
	if ( m_connected.GetValue() )
	{
		SendWorldAttached();

		// broadcast to all plugins
		for ( TList<CDebugServerPlugin*>::iterator it = m_plugins.Begin(); it != m_plugins.End(); ++it )
		{
			(*it)->AttachToWorld();
		}

		// stats
		m_sentCount.Increment();
	}
}
//////////////////////////////////////////////////////////////////////////
//
// detach from world
void CDebugServerManager::DetachFromWorld()
{
	if ( !m_initialized.GetValue() )
		return;

	// from now all requests are discarded
	RED_THREADS_MEMORY_BARRIER();
	ASSERT( m_attached.GetValue(), TXT( "DbgSrv: already detached to world!" ) );
	m_attached.SetValue( false );

	// debug
	if ( m_debugMode )
	{
		Log( TXT( "DbgSrv" ), TXT( "DetachFromWorld" ) );
	}

	// send world detached
	if ( m_connected.GetValue() )
	{
		SendWorldDetached();

		// broadcast to all plugins
		for ( TList<CDebugServerPlugin*>::iterator it = m_plugins.Begin(); it != m_plugins.End(); ++it )
		{
			(*it)->DetachFromWorld();
		}

		// stats
		m_sentCount.Increment();
	}
}
//////////////////////////////////////////////////////////////////////////
//
// game thread commands
void CDebugServerManager::Tick()
{
	if ( !m_initialized.GetValue() )
		return;

	PC_SCOPE_PIX( DbgSrv_Tick );

	// init
	CTimeCounter tm;
	TList<SDebugServerCommandData*> listCopy;

	// acquire list
	{
		PC_SCOPE_PIX( DbgSrv_Tick_Acquire_List );
		if ( m_gameThreadMutex.TryAcquire() )
		{
			if ( !m_gameThreadCommands.Empty() )
			{
				// copy list of commands
				if ( m_connected.GetValue() || m_connectedAsResourceServer.GetValue() )
				{
					PC_SCOPE_PIX( DbgSrv_Tick_ListCopy );
					listCopy = m_gameThreadCommands;
				}
				else
				{
					WARN_ENGINE( TXT( "DebugServer - dropped [%i] commands caused by disconnection.." ), m_gameThreadCommands.Size() );
				}

				// clear commands
				m_gameThreadCommands.Clear();
			}

			// release list
			m_gameThreadMutex.Release();
		}
	}

	// process commands
	{
		for ( TList<SDebugServerCommandData*>::iterator it = listCopy.Begin(); it != listCopy.End(); ++it )
		{
			PC_SCOPE_PIX( DbgSrv_GameThread_Execute );

			if ( m_connected.GetValue() || m_connectedAsResourceServer.GetValue() )
			{
				// get command
				CommandHandler Command = (*it)->m_command->m_handler;

				// call if exists
				if ( Command )
				{
					const Uint32 result = Command( (*it)->m_command->m_owner, (*it)->m_data );

					// stats
					RED_THREADS_MEMORY_BARRIER();
					m_sentCount.ExchangeAdd( result );
				}
				else
				{
					// not processed
					WARN_ENGINE( TXT( "DebugServer - Cannot process game thread command.. skipping" ) );
				}
			}
			else
			{
				WARN_ENGINE( TXT( "DebugServer - dropped [%i] commands caused by disconnection.." ), listCopy.Size() );
			}

			// delete command from list
			delete (*it);
		}
	}

	// properties trace
	RED_THREADS_MEMORY_BARRIER();
	if ( m_propsTrace.GetValue() )
	{
		SendProperties();
	}

	// update of all plugins
	for ( TList<CDebugServerPlugin*>::iterator it = m_plugins.Begin(); it != m_plugins.End(); ++it )
	{
		PC_SCOPE_PIX( DbgSrv_PluginsTick );
		(*it)->Tick();
	}

	// stats
	m_commandsTime.ExchangeAdd( (Uint32)(tm.GetTimePeriodMS()) );
	m_frame.Increment();

	// send stats
	static Float DebugServerSendStatsTimer = 0.0f;
	DebugServerSendStatsTimer += GEngine->GetLastTimeDelta();
	if ( m_sendStatsEnabled && m_connected.GetValue() && m_attached.GetValue() && (DebugServerSendStatsTimer >= DebugServerSendStatsTime) )
	{
		DebugServerSendStatsTimer = 0.0f;
		SendStats();
	}

	// frame time logging
	if ( m_frameTimeLogging && m_connected.GetValue() )
	{
		SendFrameTime();
	}
}
//////////////////////////////////////////////////////////////////////////
//
// start properties tracing
void CDebugServerManager::StartPropsTrace()
{
	if ( m_propertiesMap.Size() == 0 )
	{
		ERR_ENGINE( TXT( "DebugServer - StartPropsTrace() - try to register properties to send first.." ) );
		return;
	}

	RED_THREADS_MEMORY_BARRIER();
	m_propsTrace.SetValue( true );
	SendPropertiesNames();
}
//////////////////////////////////////////////////////////////////////////
//
// finish properties tracing
void CDebugServerManager::FinishPropsTrace()
{
	RED_THREADS_MEMORY_BARRIER();
	m_propsTrace.SetValue( false );
}
//////////////////////////////////////////////////////////////////////////
//
// send traced properties
void CDebugServerManager::SendProperties()
{
	Red::Network::ChannelPacket packet( RED_NET_CHANNEL_DEBUG_SERVER );
	/*1*/packet.WriteString( "Properties" );
	
	// fill data
	THashMap<StringAnsi, StringAnsi>::iterator it = m_propertiesMap.Begin();
	while ( it != m_propertiesMap.End() )
	{
		/*2..2+props*/packet.WriteString( (*it).m_second.AsChar() );
		++it;
	}

	// send
	Send( RED_NET_CHANNEL_DEBUG_SERVER, packet );
}
//////////////////////////////////////////////////////////////////////////
//
// send traced properties names
void CDebugServerManager::SendPropertiesNames()
{
	Red::Network::ChannelPacket packet( RED_NET_CHANNEL_DEBUG_SERVER );
	/*1*/packet.WriteString( "PropertiesNames" );

	// fill data
	THashMap<StringAnsi, StringAnsi>::iterator it = m_propertiesMap.Begin();
	while ( it != m_propertiesMap.End() )
	{
		/*2..2+props*/packet.WriteString( (*it).m_first.AsChar() );
		++it;
	}

	// send
	Send( RED_NET_CHANNEL_DEBUG_SERVER, packet );
}
//////////////////////////////////////////////////////////////////////////
//
// register property to trace
Bool CDebugServerManager::RegisterNativeProperty( const StringAnsi& propName )
{
	RED_THREADS_MEMORY_BARRIER();
	if ( m_propsTrace.GetValue() )
		return false;

	// check existence
	if ( m_propertiesMap.KeyExist( propName ) )
	{
		ERR_ENGINE( TXT( "DebugServer - RegisterNativeProperty() [%s] - handler registered already!.." ), propName.AsChar() );
		return false;
	}

	// add new native property
	m_propertiesMap[ propName ] = "";
	return true;
}
//////////////////////////////////////////////////////////////////////////
//
// set property value by string directly
Bool CDebugServerManager::SetPropertyValue( const StringAnsi& propName, String value )
{
	THashMap<StringAnsi, StringAnsi>::iterator it = m_propertiesMap.Find( propName );
	if ( it == m_propertiesMap.End() )
	{
		WARN_ENGINE( TXT( "DebugServer - SetPropertyValue( %s ) - cannot find property!.." ), propName.AsChar() );
		return false;
	}

	// set native property value
	it->m_second = UNICODE_TO_ANSI( value.AsChar() );
	return true;
}
//////////////////////////////////////////////////////////////////////////
//
// log
void CDebugServerManager::Log( const String& channel, const Char* format, ... )
{
	if ( !m_initialized.GetValue() || !m_connected.GetValue() )
		return;

	PC_SCOPE_PIX( DbgSrv_Log );

	// init
	Char buffer[ 2048 ];
	va_list arglist;
	va_start( arglist, format );
	Red::System::VSNPrintF( buffer, 2048, (channel+TXT(": ")+format).AsChar(), arglist );
	va_end( arglist );

	Red::Network::ChannelPacket packet( RED_NET_CHANNEL_DEBUG_SERVER );
	/*1*/packet.WriteString( "Log" );

	// fill data
	/*2*/packet.WriteString( UNICODE_TO_ANSI( buffer ) );

	// send
	Send( RED_NET_CHANNEL_DEBUG_SERVER, packet );

	// stats
	RED_THREADS_MEMORY_BARRIER();
	m_sentCount.Increment();
}
//////////////////////////////////////////////////////////////////////////
//
// register debug plugin
Bool CDebugServerManager::RegisterPlugin( CDebugServerPlugin* plugin )
{
	if ( !plugin )
	{
		ERR_ENGINE( TXT( "DebugServer - plugin is NULL!.." ) );
		return false;
	}

	// add plugin
	if ( !m_plugins.Exist( plugin ) )
	{
		m_plugins.PushBack( plugin );

		// initialize plugin
		if ( m_initialized.GetValue() )
		{
			plugin->Init();
		}

		return true;
	}
	return false;
}
//////////////////////////////////////////////////////////////////////////
//
// register command handler
Bool CDebugServerManager::RegisterCommandHandler( CDebugServerPlugin* owner, const StringAnsi& commandName, const Bool gameThread, CommandHandler handler )
{
	// check owner
	if ( !owner )
	{
		ERR_ENGINE( TXT( "DebugServer - command [%hs:%i] - owner is NULL!.." ), commandName.AsChar(), !!gameThread );
		return false;
	}

	// check existence
	if ( m_commandMap.KeyExist( commandName ) )
	{
		ERR_ENGINE( TXT( "DebugServer - command [%hs:%i] - handler registered already!.." ), commandName.AsChar(), !!gameThread );
		return false;
	}

	// add new command
	m_commandMap[ commandName ] = SDebugServerCommand( owner, handler, gameThread );
	return true;
}
//////////////////////////////////////////////////////////////////////////
//
// on packet received
void CDebugServerManager::OnPacketReceived( const Red::System::AnsiChar* channelName, Red::Network::IncomingPacket& packet )
{
	PC_SCOPE_PIX( DbgSrv_OnPacketReceived );

	// init
	CTimeCounter tm;

	// debugger communication channels
	if ( Red::System::StringCompare( channelName, RED_NET_CHANNEL_DEBUG_SERVER ) == 0 || Red::System::StringCompare( channelName, RED_NET_CHANNEL_RES_SERVER ) == 0 )
	{
		// get command
		AnsiChar buffer[ 256 ];
		{
			PC_SCOPE_PIX( DbgSrv_OnPacketReceive_ReadCmd );
			RED_VERIFY( packet.ReadString( buffer, ARRAY_COUNT_U32( buffer ) ) );
		}

		const StringAnsi commandName( buffer );
		m_receivedCount.Increment();

		//LOG_ENGINE( TXT( ">> next instruction #%i: %s %i" ), m_receivedCount.GetValue(), command.AsChar(), m_gameThreadCommands.Size() );

		// check command
		const Bool found = m_commandMap.KeyExist( commandName );
		if ( found )
		{
			PC_SCOPE_PIX( DbgSrv_OnPacketReceive_Found );

			// init
			SDebugServerCommand& commandDef = m_commandMap[ commandName ];

			// get data
			TDynArray<String> data;
			{
				PC_SCOPE_PIX( DbgSrv_OnPacketReceive_Found_Read );
				while ( packet.ReadString( buffer, ARRAY_COUNT_U32( buffer ) ) )
				{
					if ( strlen( buffer ) )
					{
						data.PushBack( ANSI_TO_UNICODE( buffer ) );
					}
				}
			}

			// check is need to run on game thread
			if ( commandDef.m_gameThread )
			{
				PC_SCOPE_PIX( DbgSrv_OnPacketReceive_Found_PushOnGameThread );

				// store for future processing		
				m_gameThreadMutex.Acquire();
				m_gameThreadCommands.PushBack( new SDebugServerCommandData( &commandDef, data ) );
				m_gameThreadMutex.Release();
			}
			// run immediately
			else
			{
				PC_SCOPE_PIX( DbgSrv_OnPacketReceived_Found_Execute );

				// get handler
				CommandHandler Command = commandDef.m_handler;
				RED_ASSERT( Command, TXT("Empty command handler detected!!") );

				// call if exists
				const Uint32 result = Command( commandDef.m_owner, data );

				// stats
				RED_THREADS_MEMORY_BARRIER();
				m_sentCount.ExchangeAdd( result );
			}	
		}

		// unknown command
		else
		{
			WARN_ENGINE( TXT( "DebugServer - unknown command id [%s].." ), commandName.AsChar() );
		}
	}

	// stats
	m_commandsTime.ExchangeAdd( (Uint32)(tm.GetTimePeriodMS()) );
}
//////////////////////////////////////////////////////////////////////////
//
// send stats to debugger client
void CDebugServerManager::SendStats()
{
	// init
	Red::Network::ChannelPacket packet( RED_NET_CHANNEL_DEBUG_SERVER );
	/*1*/packet.WriteString( "Stats" );

	// fill data
	// internal debugger time
	/*2*/packet.WriteString( UNICODE_TO_ANSI( ToString( (Double)m_commandsTime.GetValue()/1000 ).AsChar() ) );

	// frame no
	/*3*/packet.WriteString( UNICODE_TO_ANSI( ToString( m_frame.GetValue() ).AsChar() ) );

	// memory stats
	const Int64 totalBytesAllocated = Memory::GetTotalBytesAllocated();
	/*4*/packet.WriteString( UNICODE_TO_ANSI( ToString( (Uint32)(totalBytesAllocated/1024) ).AsChar() ) );

	// last frame time
	/*5*/packet.WriteString( UNICODE_TO_ANSI( ToString( GEngine->GetLastTimeDelta() ).AsChar() ) );

	// engine time
	/*6*/packet.WriteString( UNICODE_TO_ANSI( ToString( (Double)GEngine->GetRawEngineTime() ).AsChar() ) );

	// network outgoing packet count
	const Uint32 outgoingPacketsCount = Red::Network::Manager::GetInstance()->GetOutgoingPacketsCount();
	/*7*/packet.WriteString( UNICODE_TO_ANSI( ToString( outgoingPacketsCount ).AsChar() ) );

	// gpu frame time
	extern IRender* GRender;
	if ( GRender )
		/*8.9*/packet.WriteString( UNICODE_TO_ANSI( ToString( GRender->GetLastGPUFrameDuration() ).AsChar() ) );
	else
		/*8.9*/packet.WriteString( "0.0" );

	// avg fps
	/*9.9*/packet.WriteString( UNICODE_TO_ANSI( ToString( GEngine->GetLastTickRate() ).AsChar() ) );

	// min fps
	/*10.9*/packet.WriteString( UNICODE_TO_ANSI( ToString( GEngine->GetMinTickRate() ).AsChar() ) );

	// send
	Send( RED_NET_CHANNEL_DEBUG_SERVER, packet );

	// stats
	m_sentCount.Increment();
}
//////////////////////////////////////////////////////////////////////////
//
// send frame time to debugger client
void CDebugServerManager::SendFrameTime()
{
	// init
	Red::Network::ChannelPacket packet( RED_NET_CHANNEL_DEBUG_SERVER );
	/*1*/packet.WriteString( "FrameTime" );

	// last frame time
	/*2*/packet.WriteString( UNICODE_TO_ANSI( ToString( GEngine->GetLastTimeDelta() ).AsChar() ) );

	// send
	Send( RED_NET_CHANNEL_DEBUG_SERVER, packet );

	// stats
	m_sentCount.Increment();
}
//////////////////////////////////////////////////////////////////////////
//
// send game started
void CDebugServerManager::SendGameStarted()
{
	Red::Network::ChannelPacket packet( RED_NET_CHANNEL_DEBUG_SERVER );
	/*1*/packet.WriteString( "GameStarted" );
	Send( RED_NET_CHANNEL_DEBUG_SERVER, packet );
}
//////////////////////////////////////////////////////////////////////////
//
// send game stopped
void CDebugServerManager::SendGameStopped()
{
	Red::Network::ChannelPacket packet( RED_NET_CHANNEL_DEBUG_SERVER );
	/*1*/packet.WriteString( "GameStopped" );
	Send( RED_NET_CHANNEL_DEBUG_SERVER, packet );
}
//////////////////////////////////////////////////////////////////////////
//
// send world attached
void CDebugServerManager::SendWorldAttached()
{
	Red::Network::ChannelPacket packet( RED_NET_CHANNEL_DEBUG_SERVER );
	/*1*/packet.WriteString( "WorldAttached" );
	Send( RED_NET_CHANNEL_DEBUG_SERVER, packet );
}
//////////////////////////////////////////////////////////////////////////
//
// send world detached
void CDebugServerManager::SendWorldDetached()
{
	Red::Network::ChannelPacket packet( RED_NET_CHANNEL_DEBUG_SERVER );
	/*1*/packet.WriteString( "WorldDetached" );
	Send( RED_NET_CHANNEL_DEBUG_SERVER, packet );
}

#endif


//////////////////////////////////////////////////////////////////////////
// EOF
