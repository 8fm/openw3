/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

// Channel Interface
#include "../redNetwork/channel.h"
#include "dynarray.h"
#include "memory.h"
#include "handleMap.h"
#include "resource.h"

#define RED_NET_CHANNEL_SCRIPT_DEBUGGER "ScriptDebugger"
#define RED_NET_CHANNEL_SCRIPT_COMPILER "ScriptCompiler"

class CScriptThread;
class IScriptLogInterface;

/// Scripting system
class CScriptingSystem : public Red::Network::ChannelListener
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Engine );
	friend class CRTTISerializer;

public:
	enum EGlobalPointers
	{
		// Enum values are important for array indexing!
		GP_GAME = 0,	//! The Game (i.e. WitcherGame)
		GP_PLAYER,		//! The Player (i.e. Witcher)
		GP_CAMERA,		//! Current camera
		GP_HUD,			//! Scaleform GUI
		GP_SOUND,		//! Sound system
		GP_DEBUG,		//! Debug attributes
		GP_TIMER,		//! Timer
		GP_INPUT,		//! Input manager
		GP_TELEMETRY,	//! Telemetry interface

		GP_SIZE			//! Always the last value!
	};

	enum EDebuggingFlag
	{
		DF_UnfilteredLocals = 0,
		DF_SortLocalsAlphaAsc,

		DF_Max
	};

	typedef TDynArray< CScriptThread*, MC_ScriptObject >		TScriptThreadArray;
	typedef TDynArray< THandle< CResource >, MC_ScriptObject >	TScriptResourceArray;

protected:
	TStaticArray< THandle< IScriptable >, GP_SIZE > m_globals;

protected:
	String							m_rootPath;				//!< Scripts root path
	CScriptNativeFunctionMap		m_nativeFunctions;		//!< Mapping of native functions
	TScriptThreadArray				m_threads;				//!< All active scripting threads
	TScriptResourceArray			m_resources;			//!< Resources loaded from script
	Uint32							m_nextThreadId;			//!< Thread ID counter
	Bool							m_isValid;				//!< Script system is valid
	Bool							m_isFinalRelease;		//!< Final release
	Uint32							m_debugFlags;
	CStandardRand					m_randomNumberGenerator;

public:
	//! Get function mapping
	RED_INLINE CScriptNativeFunctionMap& GetNativeFunctions() { return m_nativeFunctions; }

	//! Get active threads
	RED_INLINE const TScriptThreadArray& GetThreads() const { return m_threads; }

	//! Get scripts root directory
	RED_INLINE const String& GetRootPath() const { return m_rootPath; }

	//! Is the script system valid ( no compilation errors )
	RED_INLINE Bool IsValid() const { return m_isValid; }

	RED_INLINE void SetValid( Bool isValid ) { m_isValid = isValid; }

	//! Are the scripts enabled in the final release mode ?
	RED_INLINE Bool IsFinalRelease() const { return m_isFinalRelease; }

	RED_INLINE Bool IsDebugFlagSet( EDebuggingFlag flag ) const { return ( FLAG( flag ) & m_debugFlags ) != 0; }

	RED_INLINE CStandardRand& GetRandomNumberGenerator() { return m_randomNumberGenerator; }

public:
	CScriptingSystem( const Char* rootPath );

	//! Load and compile scripts
	Bool LoadScripts( IScriptLogInterface* output, const TDynArray< String >& scriptFiles, Bool fullContext = false );

	//! Disable all breakpoints
	void DisableAllBreakpoints();

	//! Call global exec function, usually used from console
	Bool CallGlobalExecFunction( const String& functionAndParams, Bool silentErrors );
	
	//! Call local object function
	Bool CallLocalFunction( IScriptable* context, const String& functionAndParams, Bool silentErrors );

public:
	//! Create script thread
	CScriptThread* CreateThread( IScriptable* context, const CFunction* entryFunction, CScriptStackFrame& callingFrame, void * returnValue = NULL );

	//! Create script thread 
	CScriptThread* CreateThreadUseGivenFrame( IScriptable* context, const CFunction* entryFunction, CScriptStackFrame& topFrame, void * returnValue = NULL );

	//! Advance active threads
	void AdvanceThreads( Float timeDelta );

public:
	//! Registers global pointer - saved as handle, object can be NULL for unregistering
	void RegisterGlobal( EGlobalPointers globalType, IScriptable *objPtr );

	//! Returns global pointer (can be NULL if pointer type hasn't been registered)
	IScriptable* GetGlobal( EGlobalPointers globalType );

public:
	//! Load script resource
	CResource* LoadScriptResource( const String& path );

public:
	void RegisterNetworkChannelListener();

	void SendOpcodes( const Char* functionName, const Char* className ) const;

private:
	//! Toggle breakpoints
	Bool ToggleBreakpoint( const ScriptBreakpoint& breakpoint, Bool state );

private:
	//! Process request from debugger
	virtual void OnPacketReceived( const Red::System::AnsiChar* channelName, Red::Network::IncomingPacket& packet ) override final;
};

#ifdef RED_LOGGING_ENABLED

// path/to/some/script.ws(14) class:function(): message
#define SCRIPT_LOG( stack, message, ... )																	\
RED_LOG																										\
(																											\
	RED_LOG_CHANNEL( Script ),																				\
	TXT( "%" ) RED_PRIWs TXT( "(%u) " ) TXT( "%" ) RED_PRIWs TXT( "::%" ) RED_PRIWs TXT( "(): " ) message,	\
	( stack ).m_function->GetCode().GetSourceFile().AsChar(),												\
	( stack ).m_line,																						\
	( stack ).m_function->GetClass() ? ( stack ).m_function->GetClass()->GetName().AsChar() : TXT(""),		\
	( stack ).m_function->GetName().AsChar(),																\
	## __VA_ARGS__																							\
)

#define SCRIPT_WARNING( stack, message, ... )																\
RED_LOG_WARNING																								\
(																											\
	RED_LOG_CHANNEL( Script ),																				\
	TXT( "%" ) RED_PRIWs TXT( "(%u) " ) TXT( "%" ) RED_PRIWs TXT( "::%" ) RED_PRIWs TXT( "(): " ) message,	\
	( stack ).m_function->GetCode().GetSourceFile().AsChar(),												\
	( stack ).m_line,																						\
	( stack ).m_function->GetClass() ? ( stack ).m_function->GetClass()->GetName().AsChar() : TXT(""),		\
	( stack ).m_function->GetName().AsChar(),																\
	## __VA_ARGS__																							\
)

#define SCRIPT_ERROR( stack, message, ... )																	\
RED_LOG_ERROR																								\
(																											\
	RED_LOG_CHANNEL( Script ),																				\
	TXT( "%" ) RED_PRIWs TXT( "(%u) " ) TXT( "%" ) RED_PRIWs TXT( "::%" ) RED_PRIWs TXT( "(): " ) message,	\
	( stack ).m_function->GetCode().GetSourceFile().AsChar(),												\
	( stack ).m_line,																						\
	( stack ).m_function->GetClass() ? ( stack ).m_function->GetClass()->GetName().AsChar() : TXT(""),		\
	( stack ).m_function->GetName().AsChar(),																\
	## __VA_ARGS__																							\
)

#define SCRIPT_LOG_CONDITION( condition, stack, message, ... )		if( condition )	SCRIPT_LOG( stack, message, ##__VA_ARGS__ )
#define SCRIPT_ERROR_CONDITION( condition, stack, message, ... )	if( condition )	SCRIPT_ERROR( stack, message, ##__VA_ARGS__ )

#define SCRIPT_WARN_ONCE( stack, txt, ... )												\
{																						\
	static TSortedSet< Uint64 > warnedAlready; 											\
	union 																				\
	{																					\
		Uint64	word; 																	\
		struct 																			\
		{ 																				\
			const void* ptr1; 															\
			const void*	ptr2; 															\
		}; 																				\
	} id; 																				\
	id.ptr1	= ( stack ).m_function; 													\
	id.ptr2 = reinterpret_cast< void* > ( ( stack ).m_line ); 							\
	if ( !warnedAlready.Exist( id.word ) ) 												\
	{																					\
		SCRIPT_WARNING( stack, txt, ## __VA_ARGS__);									\
		warnedAlready.Insert( id.word );												\
	}																					\
}

#else

#define SCRIPT_LOG( ... )
#define SCRIPT_WARNING( ... )
#define SCRIPT_ERROR( ... )
#define SCRIPT_LOG_CONDITION( ... )
#define SCRIPT_ERROR_CONDITION( ... )
#define SCRIPT_WARN_ONCE( ... )

#endif // RED_LOGGING_ENABLED

/// Scripting system singleton
extern CScriptingSystem* GScriptingSystem;
