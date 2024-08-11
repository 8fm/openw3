/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once


#ifndef NO_DEBUG_SERVER

//////////////////////////////////////////////////////////////////////////
// headers
#include "../redNetwork/manager.h"


//////////////////////////////////////////////////////////////////////////
// defines
#define RED_NET_CHANNEL_DEBUG_SERVER "DebugServer"
#define RED_NET_CHANNEL_RES_SERVER "ResourceServer"


//////////////////////////////////////////////////////////////////////////
// forwards
class CDebugServerPlugin;
struct SDebugServerCommand;


//////////////////////////////////////////////////////////////////////////
// consts
const Uint32 DebugServerAPI = 19;


//////////////////////////////////////////////////////////////////////////
// typedefs
typedef Uint32 (*CommandHandler)( CDebugServerPlugin* owner, const TDynArray<String>& data );


//////////////////////////////////////////////////////////////////////////
// structs
struct SDebugServerCommandData
{
	DECLARE_STRUCT_MEMORY_ALLOCATOR( MC_Debug );
	SDebugServerCommandData( SDebugServerCommand* command, const TDynArray<String>& data )
	{
		m_command = command;
		m_data = data;
	}

	SDebugServerCommand*	m_command;
	TDynArray<String>		m_data;
};

struct SDebugServerCommand
{
	DECLARE_STRUCT_MEMORY_ALLOCATOR( MC_Debug );
	SDebugServerCommand()
	{
		m_gameThread = false;
		m_owner = nullptr;
		m_handler = nullptr;
	}

	SDebugServerCommand( CDebugServerPlugin* owner, CommandHandler handler, Bool gameThread )
	{
		m_gameThread = gameThread;
		m_owner = owner;
		m_handler = handler;
	}

	Bool				m_gameThread;
	CDebugServerPlugin* m_owner;
	CommandHandler		m_handler;
};


//////////////////////////////////////////////////////////////////////////
// declarations
class CDebugServerManager : public Red::Network::ChannelListener
{
DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Debug );

protected:
	// stats
	Red::Threads::CAtomic<Uint32>			m_commandsTime;
	Red::Threads::CAtomic<Uint32>			m_sentCount;
	Red::Threads::CAtomic<Uint32>			m_receivedCount;
	Red::Threads::CAtomic<Uint32>			m_frame;

	// game thread queue
	Red::Threads::CMutex					m_gameThreadMutex;
	TList<SDebugServerCommandData*>			m_gameThreadCommands;

	// commands map
	THashMap<StringAnsi, SDebugServerCommand>	m_commandMap;

	// plugins
	TList<CDebugServerPlugin*>				m_plugins;

	// properties
	THashMap<StringAnsi, StringAnsi>		m_propertiesMap;

	// states
	Red::Threads::CAtomic<Bool>				m_initialized;
	Red::Threads::CAtomic<Bool>				m_connected;
	Red::Threads::CAtomic<Bool>				m_connectedAsResourceServer;
	Red::Threads::CAtomic<Bool>				m_attached;
	Red::Threads::CAtomic<Bool>				m_gameRunning;
	Red::Threads::CAtomic<Bool>				m_propsTrace;
	Bool									m_debugMode;
	Bool									m_sendStatsEnabled;
	Bool									m_frameTimeLogging;
	
	// common
	const AnsiChar*							m_name;

public:
	CDebugServerManager();
	~CDebugServerManager();

	// common
	Bool Init();
	Bool ShutDown();
	void Send( const AnsiChar* channelName, const Red::Network::ChannelPacket& packet );

	// states
	const Bool IsInitialized() { return m_initialized.GetValue(); }
	const Bool IsConnected() { return m_connected.GetValue(); }
	const Bool IsConnectedAsResourceServer() { return m_connectedAsResourceServer.GetValue(); }
	const Bool IsAttached() { return m_attached.GetValue(); }
	const Bool IsGameRunning() { return m_gameRunning.GetValue(); }
	const Bool IsPropertiesTracing() { return m_propsTrace.GetValue(); }
	const Bool IsDebugMode() { return m_debugMode; }
	void SetConnected( Bool res = false ) { if ( res ) m_connectedAsResourceServer.SetValue( true ); else m_connected.SetValue( true ); }
	void SetDisconnected( Bool res = false ) { if ( res ) m_connectedAsResourceServer.SetValue( false ); else m_connected.SetValue( false ); }
	void EnableFrameTimeLogging( Bool enable ) { m_frameTimeLogging = enable; }

	// accessors
	const Uint32 GetAPIVersion() { return DebugServerAPI; }
	void SetName( const AnsiChar* name ) { m_name = name; }
	const AnsiChar* GetName() { return m_name; }
	const Uint32 GetReceivedCount() { return m_receivedCount.GetValue(); }
	const Uint32 GetSentCount() { return m_sentCount.GetValue(); }
	const Uint32 GetGameThreadCommandsCount() { return m_gameThreadCommands.Size(); }

	// life-time
	void GameStarted();
	void GameStopped();
	void AttachToWorld();
	void DetachFromWorld();
	void Tick();

	// property tracking
	void StartPropsTrace();
	void FinishPropsTrace();
	Bool RegisterNativeProperty( const StringAnsi& propName );
	Bool SetPropertyValue( const StringAnsi& propName, String value );

	// log
	void Log( const String& channel, const Char* format, ... );

	// plugins
	Bool RegisterPlugin( CDebugServerPlugin* plugin );

	// commands
	Bool RegisterCommandHandler( CDebugServerPlugin* owner, const StringAnsi& commandName, const Bool gameThread, CommandHandler handler );

private:
	// listener to remote clients
	virtual void OnPacketReceived( const Red::System::AnsiChar* channelName, Red::Network::IncomingPacket& packet ) override final;

	// helpers
	void SendStats();
	void SendGameStarted();
	void SendGameStopped();
	void SendWorldAttached();
	void SendWorldDetached();
	void SendProperties();
	void SendPropertiesNames();
	void SendFrameTime();

	// friendships
	friend class CDebugServerCommandAreWeConnected;
};


//////////////////////////////////////////////////////////////////////////
// Singleton
typedef TSingleton<CDebugServerManager> SDebugServerMgr;


//////////////////////////////////////////////////////////////////////////
// macros
#define DBGSRV()  SDebugServerMgr::GetInstance()
#define DBGSRV_CALL( func )  SDebugServerMgr::GetInstance().func
#define DBGSRV_REG_PLUGIN( plugin )  SDebugServerMgr::GetInstance().RegisterPlugin( plugin )
#define DBGSRV_REG_COMMAND( ownerPlugin, commandName, handlerClass )  SDebugServerMgr::GetInstance().RegisterCommandHandler( ownerPlugin, ( commandName ), (handlerClass::gameThread), (handlerClass::ProcessCommand) )
#define DBGSRV_REG_NATIVE_PROP( propName )  SDebugServerMgr::GetInstance().RegisterNativeProperty( propName )
#define DBGSRV_SET_NATIVE_PROP_VALUE( propName, value )  SDebugServerMgr::GetInstance().SetPropertyValue( propName, ToString( value ) )
#define DBGSRV_SET_STRING_PROP_VALUE( propName, value )  SDebugServerMgr::GetInstance().SetPropertyValue( propName, value )
#define DBGSRV_LOG( channel, format, ... )  SDebugServerMgr::GetInstance().Log( channel, format, __VA_ARGS__ )

#else

#define DBGSRV() (0(void))
#define DBGSRV_CALL( func ) false
#define DBGSRV_REG_PLUGIN( plugin ) false
#define DBGSRV_REG_COMMAND( ownerPlugin, commandName, handlerClass ) false
#define DBGSRV_REG_NATIVE_PROP( propName ) false
#define DBGSRV_SET_NATIVE_PROP_VALUE( propName, value ) false
#define DBGSRV_SET_STRING_PROP_VALUE( propName, value ) false
#define DBGSRV_LOG( channel, format, ... ) (0(void))

#endif


//////////////////////////////////////////////////////////////////////////
// EOF
