/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

/// Debug flags
enum EScriptStackDebugFlags
{
	SSDF_NoAutoBreakpoint,
	SSDF_BreakOnNext,
	SSDF_BreakOnFunctionEntry,
	SSDF_BreakOnFunctionReturn,
};

#ifdef RED_NETWORK_ENABLED

// Channel Interface
#include "../redNetwork/channel.h"

#include "scriptable.h"
#include "sharedPtr.h"

class IScriptDebugVariable;

/// Script debugger, spawned when breakpoint occurs
class CScriptDebugger : public Red::Network::ChannelListener
{
protected:
	THandle< IScriptable >				m_context;		//!< Context, NULL for global function
	Bool								m_continue;		//!< Continue execution
	TDynArray< CScriptStackFrame* >		m_stack;		//!< Stack frames ( callstack )

public:
	CScriptDebugger( IScriptable* context, CScriptStackFrame& stack, Uint32 line );
	~CScriptDebugger();

	//! Process breakpoints, this is a loop
	void ProcessBreakpoint();

private:
	//! Format debugger message with callstack data
	void GetCallstack();

	//! Find local in framestack
	Red::TSharedPtr< IScriptDebugVariable > FindLocal( Uint32 stackFrameIndex, const String& path ) const;

	//! Get locals in given context
	Bool GetLocals( Uint32 stackFrame, Uint32 stamp, const String& path, Uint32 id ) const;

	Bool SetLocal( Uint32 stackFrame, const String& path, const String& value );

	//! Parse commands from debugger
	virtual void OnPacketReceived( const Red::System::AnsiChar* channelName, Red::Network::IncomingPacket& packet );
};

#endif // RED_NETWORK_ENABLED
