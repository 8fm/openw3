/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#include "build.h"
#include "debuggerHelper.h"
#include "packetUtils.h"
#include "events/eventOpcodeListing.h"
#include "../../common/redNetwork/manager.h"
#include "app.h"

wxDEFINE_EVENT( ssEVT_BREAKPOINT_TOGGLE_CONFIRMATION_EVENT, CBreakpointToggleConfirmationEvent );
wxDEFINE_EVENT( ssEVT_BREAKPOINT_HIT_EVENT, CBreakpointHitEvent );
wxDEFINE_EVENT( ssEVT_CALLSTACK_EVENT, CCallstackUpdateEvent );

wxIMPLEMENT_DYNAMIC_CLASS( CBreakpointToggleConfirmationEvent, wxEvent );
wxIMPLEMENT_DYNAMIC_CLASS( CBreakpointHitEvent, wxEvent );
wxIMPLEMENT_DYNAMIC_CLASS( CCallstackUpdateEvent, wxEvent );

CBreakpointToggleConfirmationEvent::CBreakpointToggleConfirmationEvent( wxEventType commandType, int winid )
:	wxEvent( commandType, winid )
{

}

CBreakpointToggleConfirmationEvent::CBreakpointToggleConfirmationEvent( const wxString& file, Red::System::Int32 line, Red::System::Bool success )
:	wxEvent( wxID_ANY, ssEVT_BREAKPOINT_TOGGLE_CONFIRMATION_EVENT )
,	m_file( file )
,	m_line( line )
,	m_success( success )
{

}

//////////////////////////////////////////////////////////////////////////

CBreakpointHitEvent::CBreakpointHitEvent( wxEventType commandType, int winid )
:	wxEvent( commandType, winid )
{

}

CBreakpointHitEvent::CBreakpointHitEvent( const wxString& file, Red::System::Int32 line )
:	wxEvent( wxID_ANY, ssEVT_BREAKPOINT_HIT_EVENT )
,	m_file( file )
,	m_line( line )
{

}

CBreakpointHitEvent::~CBreakpointHitEvent()
{

}

//////////////////////////////////////////////////////////////////////////

CCallstackUpdateEvent::CCallstackUpdateEvent( wxEventType commandType, int winid )
:	wxEvent( commandType, winid )
{

}

CCallstackUpdateEvent::CCallstackUpdateEvent( Red::System::Uint32 numberOfFrames, SFrame* frames, Red::System::Uint32 numberOfNativeFrames, SNativeFrame* nativeFrames )
:	wxEvent( wxID_ANY, ssEVT_CALLSTACK_EVENT )
,	m_frames( frames )
,	m_nativeFrames( nativeFrames )
,	m_numberOfFrames( numberOfFrames )
,	m_numNativeFrames( numberOfNativeFrames )
{
	m_referenceCount = new Red::Threads::CAtomic< Red::System::Uint32 >( 1 );
}

CCallstackUpdateEvent::CCallstackUpdateEvent( const CCallstackUpdateEvent& other )
:	wxEvent( wxID_ANY, ssEVT_CALLSTACK_EVENT )
,	m_frames( other.m_frames )
,	m_nativeFrames( other.m_nativeFrames )
,	m_numberOfFrames( other.m_numberOfFrames )
,	m_numNativeFrames( other.m_numNativeFrames )
,	m_referenceCount( other.m_referenceCount )
{
	m_referenceCount->Increment();
}

CCallstackUpdateEvent::~CCallstackUpdateEvent()
{
	if( m_referenceCount->Decrement() == 0 )
	{
		delete [] m_frames;
		delete [] m_nativeFrames;
		delete m_referenceCount;

		m_frames = nullptr;
		m_nativeFrames = nullptr;

		m_numberOfFrames = 0;
		m_numNativeFrames = 0;

		m_referenceCount = nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////

wxIMPLEMENT_CLASS( CDebuggerHelper, wxEvtHandler );

//////////////////////////////////////////////////////////////////////////

CDebuggerHelper::CDebuggerHelper()
{
}

CDebuggerHelper::~CDebuggerHelper()
{
	if( Red::Network::Manager::GetInstance() )
	{
		Red::Network::Manager::GetInstance()->UnregisterListener( RED_NET_CHANNEL_SCRIPT_DEBUGGER, this );
	}
}

Red::System::Bool CDebuggerHelper::Initialize( const wxChar* ip )
{
	if( Red::Network::Manager::GetInstance() )
	{
		Red::Network::Address address( ip, wxTheSSApp->GetNetworkPort() );
		Red::Network::Manager::GetInstance()->ConnectTo( address, RED_NET_CHANNEL_SCRIPT_DEBUGGER );
		Red::Network::Manager::GetInstance()->RegisterListener( RED_NET_CHANNEL_SCRIPT_DEBUGGER, this );

		return true;
	}

	return false;
}

void CDebuggerHelper::OnPacketReceived( const Red::System::AnsiChar* channelName, Red::Network::IncomingPacket& packet )
{
	if( Red::System::StringCompare( channelName, RED_NET_CHANNEL_SCRIPT_DEBUGGER ) == 0 )
	{
		Red::System::AnsiChar messageType[ 32 ];
		RED_VERIFY( packet.ReadString( messageType, ARRAY_COUNT_U32( messageType ) ) );

		if( Red::System::StringCompare( messageType, "BreakpointConfirm" ) == 0 )
		{
			wxString file;
			Red::System::Int32 line;
			Red::System::Bool success;

			PacketUtilities::ReadString( packet, file );
			RED_VERIFY( packet.Read( line ) );
			RED_VERIFY( packet.Read( success ) );

			CBreakpointToggleConfirmationEvent* event = new CBreakpointToggleConfirmationEvent( file, line, success );
			QueueEvent( event );
		}
		else if( Red::System::StringCompare( messageType, "BreakpointHit" ) == 0 )
		{
			wxString file;
			Red::System::Uint32 line;

			PacketUtilities::ReadString( packet, file );
			RED_VERIFY( packet.Read( line ) );

			CBreakpointHitEvent* event = new CBreakpointHitEvent( file, static_cast< Red::System::Int32 >( line ) );
			QueueEvent( event );
		}
		else if( Red::System::StringCompare( messageType, "Callstack" ) == 0 )
		{
			OnCallstackReceived( packet );
		}
		else if( Red::System::StringCompare( messageType, "OpcodeBreakdownResponse" ) == 0 )
		{
			OnOpcodesReceived( packet );
		}
	}
}

Red::System::Bool CDebuggerHelper::DebugContinue()
{
	Red::Network::Manager* network = Red::Network::Manager::GetInstance();
	if( network && network->IsInitialized() )
	{
		Red::Network::ChannelPacket packet( RED_NET_CHANNEL_SCRIPT_DEBUGGER );

		RED_VERIFY( packet.WriteString( "DebugContinue" ) );

		return network->Send( RED_NET_CHANNEL_SCRIPT_DEBUGGER, packet );
	}

	return false;
}

Red::System::Bool CDebuggerHelper::DebugStepOver()
{
	Red::Network::Manager* network = Red::Network::Manager::GetInstance();
	if( network && network->IsInitialized() )
	{
		Red::Network::ChannelPacket packet( RED_NET_CHANNEL_SCRIPT_DEBUGGER );

		RED_VERIFY( packet.WriteString( "DebugStepOver" ) );

		return network->Send( RED_NET_CHANNEL_SCRIPT_DEBUGGER, packet );
	}

	return false;
}

Red::System::Bool CDebuggerHelper::DebugStepOut()
{
	Red::Network::Manager* network = Red::Network::Manager::GetInstance();
	if( network && network->IsInitialized() )
	{
		Red::Network::ChannelPacket packet( RED_NET_CHANNEL_SCRIPT_DEBUGGER );

		RED_VERIFY( packet.WriteString( "DebugStepOut" ) );

		return network->Send( RED_NET_CHANNEL_SCRIPT_DEBUGGER, packet );
	}

	return false;
}

Red::System::Bool CDebuggerHelper::DebugStepInto()
{
	Red::Network::Manager* network = Red::Network::Manager::GetInstance();
	if( network && network->IsInitialized() )
	{
		Red::Network::ChannelPacket packet( RED_NET_CHANNEL_SCRIPT_DEBUGGER );

		RED_VERIFY( packet.WriteString( "DebugStepInto" ) );

		return network->Send( RED_NET_CHANNEL_SCRIPT_DEBUGGER, packet );
	}

	return false;
}

Red::System::Bool CDebuggerHelper::RequestCallstack()
{
	Red::Network::Manager* network = Red::Network::Manager::GetInstance();
	if( network && network->IsInitialized() )
	{
		Red::Network::ChannelPacket packet( RED_NET_CHANNEL_SCRIPT_DEBUGGER );

		RED_VERIFY( packet.WriteString( "CallstackRequest" ) );

		return network->Send( RED_NET_CHANNEL_SCRIPT_DEBUGGER, packet );
	}

	return false;
}

Red::System::Bool CDebuggerHelper::SetLocalsSorted( Red::System::Bool enabled )
{
	Red::Network::Manager* network = Red::Network::Manager::GetInstance();
	if( network && network->IsInitialized() )
	{
		Red::Network::ChannelPacket packet( RED_NET_CHANNEL_SCRIPT_DEBUGGER );

		RED_VERIFY( packet.WriteString( "SortLocals" ) );
		RED_VERIFY( packet.Write( enabled ) );

		return network->Send( RED_NET_CHANNEL_SCRIPT_DEBUGGER, packet );
	}

	return false;
}

Red::System::Bool CDebuggerHelper::SetLocalsUnfiltered( Red::System::Bool enabled )
{
	Red::Network::Manager* network = Red::Network::Manager::GetInstance();
	if( network && network->IsInitialized() )
	{
		Red::Network::ChannelPacket packet( RED_NET_CHANNEL_SCRIPT_DEBUGGER );

		RED_VERIFY( packet.WriteString( "UnfilteredLocals" ) );
		RED_VERIFY( packet.Write( enabled ) );

		return network->Send( RED_NET_CHANNEL_SCRIPT_DEBUGGER, packet );
	}

	return false;
}

Red::System::Bool CDebuggerHelper::ToggleBreakpoint( const wxString& filepath, Red::System::Int32 line, Red::System::Bool enabled )
{
	Red::Network::Manager* network = Red::Network::Manager::GetInstance();
	if( network && network->IsInitialized() )
	{
		Red::Network::ChannelPacket packet( RED_NET_CHANNEL_SCRIPT_DEBUGGER );

		RED_VERIFY( packet.WriteString( "BreakpointToggle" ) );
		RED_VERIFY( packet.WriteString( filepath.wx_str() ) );
		RED_VERIFY( packet.Write( line ) );
		RED_VERIFY( packet.Write( enabled ) );

		return network->Send( RED_NET_CHANNEL_SCRIPT_DEBUGGER, packet );
	}

	return false;
}

Red::System::Bool CDebuggerHelper::DisableAllBreakpoints()
{
	Red::Network::Manager* network = Red::Network::Manager::GetInstance();
	if( network && network->IsInitialized() )
	{
		Red::Network::ChannelPacket packet( RED_NET_CHANNEL_SCRIPT_DEBUGGER );

		RED_VERIFY( packet.WriteString( "ClearAllBreakpoints" ) );

		return Red::Network::Manager::GetInstance()->Send( RED_NET_CHANNEL_SCRIPT_DEBUGGER, packet );
	}

	return false;
}

void CDebuggerHelper::OnCallstackReceived( Red::Network::IncomingPacket& packet )
{
	// Script Frames
	Red::System::Uint32 numFrames;

	RED_VERIFY( packet.Read( numFrames ) );
	SFrame* frames = new SFrame[ numFrames ];

	for( Red::System::Uint32 iFrame = 0; iFrame < numFrames; ++iFrame )
	{
		// Filename
		PacketUtilities::ReadString( packet, frames[ iFrame ].file );
		frames[ iFrame ].file.Replace( wxT( "/" ), wxT( "\\" ) );

		// Line Number
		RED_VERIFY( packet.Read( frames[ iFrame ].line ) );

		// Function name
		PacketUtilities::ReadString( packet, frames[ iFrame ].function );

		// Parameters
		RED_VERIFY( packet.Read( frames[ iFrame ].numParameters ) );

		frames[ iFrame ].parameters = new SProperty[ frames[ iFrame ].numParameters ];
		
		for( Red::System::Uint32 iParam = 0; iParam < frames[ iFrame ].numParameters; ++iParam )
		{
			// Property name
			PacketUtilities::ReadString( packet, frames[ iFrame ].parameters[ iParam ].name );

			// Property Value
			PacketUtilities::ReadString( packet, frames[ iFrame ].parameters[ iParam ].value );
		}
	}

	// Native Frames
	Red::System::Uint32 numNativeFrames;
	RED_VERIFY( packet.Read( numNativeFrames ) );

	SNativeFrame* nativeFrames = new SNativeFrame[ numNativeFrames ];

	for( Red::System::Uint32 iFrame = 0; iFrame < numNativeFrames; ++iFrame )
	{
		// Location
		PacketUtilities::ReadString( packet, nativeFrames[ iFrame ].location );

		// Symbol
		PacketUtilities::ReadString( packet, nativeFrames[ iFrame ].symbol );
	}

	CCallstackUpdateEvent* event = new CCallstackUpdateEvent( numFrames, frames, numNativeFrames, nativeFrames );
	QueueEvent( event );
}

void CDebuggerHelper::OnOpcodesReceived( Red::Network::IncomingPacket& packet )
{
	using Red::System::Uint32;

	vector< SOpcodeFunction > functions;

	Uint32 numFunctions = 0;
	RED_VERIFY( packet.Read( numFunctions ) );

	functions.resize( numFunctions );

	for( Uint32 i = 0; i < numFunctions; ++i )
	{
		SOpcodeFunction& function = functions[ i ];

		PacketUtilities::ReadString( packet, function.m_file );

		Uint32 numLines = 0;
		RED_VERIFY( packet.Read( numLines ) );

		function.m_lines.resize( numLines );

		for( Uint32 j = 0; j < numLines; ++j )
		{
			SOpcodeLine& line = function.m_lines[ j ];

			RED_VERIFY( packet.Read( line.m_line ) );
			PacketUtilities::ReadString( packet, line.m_details );
		}
	}

	CSSOpcodeListingEvent* event = new CSSOpcodeListingEvent( functions );
	QueueEvent( event );
}
