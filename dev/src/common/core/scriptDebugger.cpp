/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifdef RED_NETWORK_ENABLED

#include "scriptingSystem.h"
#include "scriptDebugger.h"
#include "scriptStackFrame.h"

#include "../redNetwork/manager.h"

CScriptDebugger::CScriptDebugger( IScriptable* context, CScriptStackFrame& stack, Uint32 line )
	: m_context( context )
	, m_continue( false )
{
	Red::Network::Manager::GetInstance()->RegisterListener( RED_NET_CHANNEL_SCRIPT_DEBUGGER, this );

	// Grab callstack, start with breakpoint place
	stack.m_line = line;

	// Go to parent stack frames building the callstack
	CScriptStackFrame* frame = &stack;
	while ( frame )
	{
		// Add to stack list
		frame->m_debugFlags = 0;
		m_stack.PushBack( frame );

		// Go to base frame
		frame = frame->m_parent;
	}

	// Send the debug breakpoint event
	{
		// Inform the debugger about the breakpoint
		Red::Network::ChannelPacket packet( RED_NET_CHANNEL_SCRIPT_DEBUGGER );
		packet.WriteString( "BreakpointHit" );
		packet.WriteString( stack.m_function->GetCode().GetSourceFile().AsChar() );
		packet.Write( stack.m_line );
		Red::Network::Manager::GetInstance()->Send( RED_NET_CHANNEL_SCRIPT_DEBUGGER, packet );
	}
}

CScriptDebugger::~CScriptDebugger()
{
	Red::Network::Manager::GetInstance()->UnregisterListener( RED_NET_CHANNEL_SCRIPT_DEBUGGER, this );
}

void CScriptDebugger::ProcessBreakpoint()
{
#ifdef RED_PLATFORM_WINPC
	::ReleaseCapture();
	::ClipCursor( nullptr );
	while ( ::ShowCursor( true ) < 0 );
#endif

	// While not requested to continue
	while ( !m_continue )
	{
		// Do not eat all the CPU
		Red::Threads::SleepOnCurrentThread( 30 );

		if( Red::Network::Manager::GetInstance()->GetNumberOfConnectionsToChannel( RED_NET_CHANNEL_SCRIPT_DEBUGGER ) == 0 )
		{
			m_continue = true;
			GScriptingSystem->DisableAllBreakpoints();
		}
	}
}

void CScriptDebugger::OnPacketReceived( const Red::System::AnsiChar*, Red::Network::IncomingPacket& packet )
{
	AnsiChar requestType[ 32 ];
	packet.ReadString( requestType, ARRAY_COUNT_U32( requestType ) );

	if( Red::System::StringCompare( requestType, "DebugContinue" ) == 0 )
	{
		RED_LOG_SPAM( RED_LOG_CHANNEL( ScriptDebugger ), TXT( "Debugger: continue" ) );

		m_continue = true;
	}
	else if( Red::System::StringCompare( requestType, "DebugStepOver" ) == 0 )
	{
		// Break on next element in this frame
		m_stack[ 0 ]->m_debugFlags = SSDF_BreakOnNext;

		RED_LOG_SPAM( RED_LOG_CHANNEL( ScriptDebugger ), TXT( "Debugger: step over" ) );

		m_continue = true;
	}
	else if( Red::System::StringCompare( requestType, "DebugStepInto" ) == 0 )
	{
		// Break on any function call
		m_stack[ 0 ]->m_debugFlags = SSDF_BreakOnFunctionEntry;

		m_continue = true;
	}
	else if( Red::System::StringCompare( requestType, "DebugStepOut" ) == 0 )
	{
		if ( m_stack.Size() > 1 )
		{
			// Break on the return from this function
			m_stack[ 1 ]->m_debugFlags = SSDF_BreakOnFunctionReturn;
		
			// Continue
			RED_LOG_SPAM( RED_LOG_CHANNEL( ScriptDebugger ), TXT( "Debugger: step out" ) );

			m_continue = true;
		}
		else
		{
			// Continue
			RED_LOG_WARNING( RED_LOG_CHANNEL( ScriptDebugger ), TXT( "Debugger: unable to step out" ) );
		}
	}
	else if( Red::System::StringCompare( requestType, "CallstackRequest" ) == 0 )
	{
		GetCallstack();
	}
	else if( Red::System::StringCompare( requestType, "LocalsRequest" ) == 0 )
	{
		Uint32 frameIndex = 0;
		Char path[ 256 ];
		Uint32 stamp = 0;
		Uint32 id = 0;

		RED_VERIFY( packet.Read( stamp ), TXT( "Could not read stamp values for LocalsRequest packet" ) );
		RED_VERIFY( packet.Read( frameIndex ) );
		RED_VERIFY( packet.ReadString( path, ARRAY_COUNT_U32( path ) ) );
		RED_VERIFY( packet.Read( id ) );

		GetLocals( frameIndex, stamp, path, id );
	}
	else if( Red::System::StringCompare( requestType, "LocalsModification" ) == 0 )
	{
		Uint32 frameIndex = 0;
		Char path[ 256 ];
		Char value[ 256 ];

		RED_VERIFY( packet.Read( frameIndex ) );
		RED_VERIFY( packet.ReadString( path, ARRAY_COUNT_U32( path ) ) );
		RED_VERIFY( packet.ReadString( value, ARRAY_COUNT_U32( value ) ) );

		RED_LOG_SPAM( RED_LOG_CHANNEL( ScriptDebugger ), TXT( "Locals modification request: Change '%ls' to '%ls'" ), path, value );

		Bool success = SetLocal( frameIndex, path, value );

		// Send ack
		Red::Network::ChannelPacket ackPacket( RED_NET_CHANNEL_SCRIPT_DEBUGGER );
		ackPacket.WriteString( "LocalsModificationAck" );
		ackPacket.Write( success );
		Red::Network::Manager::GetInstance()->Send( RED_NET_CHANNEL_SCRIPT_DEBUGGER, ackPacket );
	}
}

void CScriptDebugger::GetCallstack()
{
	// Inform the debugger about the breakpoint
	Red::Network::ChannelPacket packet( RED_NET_CHANNEL_SCRIPT_DEBUGGER );

	packet.WriteString( "Callstack" );

	// Write number of frames
	packet.Write( m_stack.Size() );

	for ( Uint32 i = 0; i < m_stack.Size(); ++i )
	{
		CScriptStackFrame* frame = m_stack[ i ];
		ASSERT( frame->m_function );

		// Location
		packet.WriteString( frame->m_function->GetCode().GetSourceFile().AsChar() );
		packet.Write( frame->m_line );

		// Function name
		String functionName = frame->m_function->GetName().AsString();
		if ( frame->m_function->GetClass() )
		{
			String className = frame->m_function->GetClass()->GetName().AsString();
			functionName = className + TXT( "::" ) + functionName;
		}
		packet.WriteString( functionName.AsChar() );

		// Parameters
		const Uint32 numParams = static_cast< Uint32 >( frame->m_function->GetNumParameters() );
		packet.Write( numParams );
		for ( Uint32 j = 0; j < numParams; ++j )
		{
			CProperty* prop = frame->m_function->GetParameter( j );

			// Write property name
			packet.WriteString( prop->GetName().AsString().AsChar() );

			// Export parameter value
			String value = TXT( "Unknown" );
			const void* data = prop->GetOffsetPtr( frame->m_locals );
			prop->GetType()->ToString( data, value );
			packet.WriteString( value.AsChar() );
		}
	}

	// Get the C++ callstack
	Red::System::Error::Callstack nativeCallstack;

	Red::System::Error::Handler::GetInstance()->GetCallstack( nativeCallstack, SGetMainThreadId() );

	Char buffer[ 512 ];

	packet.Write( nativeCallstack.numFrames );
	for( Uint32 i = 0; i < nativeCallstack.numFrames; ++i )
	{
		Red::System::SNPrintF
		(
			buffer,
			ARRAY_COUNT( buffer ),
			TXT( "% " ) RED_PRIWs TXT( " (%u)" ),
			nativeCallstack.frame[ i ].file,
			nativeCallstack.frame[ i ].line
		);

		packet.WriteString( buffer );

		packet.WriteString( nativeCallstack.frame[ i ].symbol );
	}

	Red::Network::Manager::GetInstance()->Send( RED_NET_CHANNEL_SCRIPT_DEBUGGER, packet );
}

#endif // RED_NETWORK_ENABLED
