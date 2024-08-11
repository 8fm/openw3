/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#include "build.h"
#include "debuggerLocalsHelper.h"
#include "debuggerHelper.h"
#include "packetUtils.h"

#include "../../common/redNetwork/manager.h"
#include "app.h"

wxDEFINE_EVENT( ssEVT_LOCALS_EVENT, CLocalsEvent );
wxIMPLEMENT_DYNAMIC_CLASS( CLocalsEvent, wxEvent );

CLocalsEvent::CLocalsEvent( wxEventType commandType, int winid )
:	wxEvent( commandType, winid )
{

}

CLocalsEvent::CLocalsEvent( SDebugValue* property, SDebugValueChild* children, Red::System::Uint32 numChildren, Red::System::Uint32 stackFrame, Red::System::Uint32 stamp, const wxString& path, Red::System::Uint32 itemId )
:	wxEvent( wxID_ANY, ssEVT_LOCALS_EVENT )
,	m_property( property )
,	m_children( children )
,	m_numChildren( numChildren )
,	m_stackFrame( stackFrame )
,	m_stamp( stamp )
,	m_path( path )
,	m_itemId( itemId )
{
	m_referenceCount = new Red::Threads::CAtomic< Red::System::Uint32 >();

	m_referenceCount->SetValue( 1 );
}

CLocalsEvent::CLocalsEvent( const CLocalsEvent& other )
:	wxEvent( wxID_ANY, ssEVT_LOCALS_EVENT )
,	m_property( other.m_property )
,	m_children( other.m_children )
,	m_numChildren( other.m_numChildren )
,	m_referenceCount( other.m_referenceCount )
,	m_stackFrame( other.m_stackFrame )
,	m_path( other.m_path )
,	m_itemId( other.m_itemId )
{
	m_referenceCount->Increment();
}

CLocalsEvent::~CLocalsEvent()
{
	if( m_referenceCount->Decrement() == 0 )
	{
		delete m_property;
		delete [] m_children;
		delete m_referenceCount;

		m_property = nullptr;
		m_children = nullptr;
		m_referenceCount = nullptr;

		m_numChildren = 0;
	}
}


//////////////////////////////////////////////////////////////////////////

wxDEFINE_EVENT( ssEVT_LOCALS_MODIFY_ACK_EVENT, CLocalsModificationAckEvent );
wxIMPLEMENT_DYNAMIC_CLASS( CLocalsModificationAckEvent, wxEvent );

CLocalsModificationAckEvent::CLocalsModificationAckEvent( wxEventType commandType, int winid )
:	wxEvent( commandType, winid )
{

}

CLocalsModificationAckEvent::CLocalsModificationAckEvent( Red::System::Bool success )
:	wxEvent( wxID_ANY, ssEVT_LOCALS_MODIFY_ACK_EVENT )
,	m_success( success )
{

}

CLocalsModificationAckEvent::~CLocalsModificationAckEvent()
{

}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_CLASS( CDebuggerLocalsHelper, wxEvtHandler );

//////////////////////////////////////////////////////////////////////////

CDebuggerLocalsHelper::CDebuggerLocalsHelper()
:	m_property( nullptr )
,	m_children( nullptr )
,	m_hash( 0 )
,	m_stamp( 0 )
,	m_stackFrame( 0 )
,	m_numChildren( 0 )
,	m_numChildrenReceived( 0 )
{
}

CDebuggerLocalsHelper::~CDebuggerLocalsHelper()
{
	if( Red::Network::Manager::GetInstance() )
	{
		Red::Network::Manager::GetInstance()->UnregisterListener( RED_NET_CHANNEL_SCRIPT_DEBUGGER, this );
	}
}

Red::System::Bool CDebuggerLocalsHelper::Initialize( const wxChar* ip )
{
	if( Red::Network::Manager::GetInstance() )
	{
		Red::Network::Address address( ip,  wxTheSSApp->GetNetworkPort() );
		Red::Network::Manager::GetInstance()->ConnectTo( address, RED_NET_CHANNEL_SCRIPT_DEBUGGER );
		Red::Network::Manager::GetInstance()->RegisterListener( RED_NET_CHANNEL_SCRIPT_DEBUGGER, this );

		return true;
	}

	return false;
}

void CDebuggerLocalsHelper::OnPacketReceived( const Red::System::AnsiChar* channelName, Red::Network::IncomingPacket& packet )
{
	if( Red::System::StringCompare( channelName, RED_NET_CHANNEL_SCRIPT_DEBUGGER ) == 0 )
	{
		Red::System::AnsiChar messageType[ 32 ];
		RED_VERIFY( packet.ReadString( messageType, ARRAY_COUNT_U32( messageType ) ) );

		if( Red::System::StringCompare( messageType, "LocalsHeader" ) == 0 )
		{
			ProcessLocalsHeader( packet );
		}
		else if( Red::System::StringCompare( messageType, "LocalsBody" ) == 0 )
		{
			ProcessLocalsBody( packet );
		}
		else if( Red::System::StringCompare( messageType, "LocalsModificationAck" ) == 0 )
		{
			Red::System::Bool success = false;
			RED_VERIFY( packet.Read( success ), TXT( "Couldn't read locals header property icon" ) );

			CLocalsModificationAckEvent* event = new CLocalsModificationAckEvent( success );
			QueueEvent( event );
		}
	}
}

void CDebuggerLocalsHelper::ProcessLocalsHeader( Red::Network::IncomingPacket& packet )
{
	RED_ASSERT( m_property == nullptr, TXT( "We haven't finished processing the previous property: %" ) RED_PRIWs, m_property->name.wx_str() );
	RED_ASSERT( m_children == nullptr, TXT( "We haven't finished processing the previous property" ) );

	RED_VERIFY( packet.Read( m_stamp ), TXT( "Couldn't read locals header packet stamp value" ) );
	RED_VERIFY( packet.Read( m_hash ), TXT( "Couldn't read locals header packet hash value" ) );
	RED_VERIFY( packet.Read( m_stackFrame ), TXT( "Couldn't read locals header stack frame index" ) );
	PacketUtilities::ReadString( packet, m_path );
	RED_VERIFY( packet.Read( m_id ), TXT( "Couldn't read locals header item id" ) );

	m_property = new SDebugValue();
	PacketUtilities::ReadString( packet, m_property->name );
	PacketUtilities::ReadString( packet, m_property->value );
	RED_VERIFY( packet.Read( m_property->icon ), TXT( "Couldn't read locals header property icon" ) );
	RED_VERIFY( packet.Read( m_property->isModifiable ), TXT( "Couldn't read locals header isModifiable" ) );

	RED_VERIFY( packet.Read( m_numChildren ), TXT( "Couldn't read number of children from locals header packet for property: %" ) RED_PRIWs, m_property->name.wx_str() );
	
	m_children = new SDebugValueChild[ m_numChildren ];
	m_numChildrenReceived = 0;

	ProcessLocalsEvent();
}

void CDebuggerLocalsHelper::ProcessLocalsBody( Red::Network::IncomingPacket& packet )
{
	Red::System::THash32 hash;
	RED_VERIFY( packet.Read( hash ), TXT( "Couldn't read locals header packet hash value" ) );

	if( hash == m_hash )
	{
		Red::System::Uint32 childrenInPacket = 0;
		RED_VERIFY( packet.Read( childrenInPacket ), TXT( "Couldn't read locals body child count: %" ) RED_PRIWs, m_property->name.wx_str() );

		for( Red::System::Uint32 i = 0; i < childrenInPacket; ++i )
		{
			SDebugValueChild& child = m_children[ m_numChildrenReceived + i ];

			RED_VERIFY( packet.Read( child.isExpandable ), TXT( "Couldn't read locals body isExpandable (Child %u): %" ) RED_PRIWs, m_numChildrenReceived + i, m_property->name.wx_str() );
			RED_VERIFY( packet.Read( child.icon ), TXT( "Couldn't read locals body icon (Child %u): %" ) RED_PRIWs, m_numChildrenReceived + i, m_property->name.wx_str() );
			PacketUtilities::ReadString( packet, child.name );
			PacketUtilities::ReadString( packet, child.value );
			RED_VERIFY( packet.Read( child.isModifiable ), TXT( "Couldn't read locals body isModifiable (Child %u): %" ) RED_PRIWs, m_numChildrenReceived + i, m_property->name.wx_str() );
		}

		m_numChildrenReceived += childrenInPacket;

		ProcessLocalsEvent();
	}
	else
	{
		RED_HALT( "Hash values don't match for LocalsBody packet! %u != %u", hash, m_hash );
	}
}

void CDebuggerLocalsHelper::ProcessLocalsEvent()
{
	if( m_numChildrenReceived == m_numChildren )
	{
		CLocalsEvent* event = new CLocalsEvent( m_property, m_children, m_numChildren, m_stackFrame, m_stamp, m_path, m_id );
		QueueEvent( event );

		// These pointers are free'd by the event once it's destroyed
		m_property		= nullptr;
		m_children		= nullptr;
		m_numChildren	= 0;
		m_stackFrame	= 0;
		m_path.Clear();
	}
}

void CDebuggerLocalsHelper::RequestLocals( Red::System::Uint32 stackFrameIndex, Red::System::Uint32 stamp, const Red::System::Char* path, Red::System::Uint32 id )
{
	Red::Network::Manager* network = Red::Network::Manager::GetInstance();
	if( network && network->IsInitialized() )
	{
		Red::Network::ChannelPacket packet( RED_NET_CHANNEL_SCRIPT_DEBUGGER );

		RED_VERIFY( packet.WriteString( "LocalsRequest" ) );
		RED_VERIFY( packet.Write( stamp ) );
		RED_VERIFY( packet.Write( stackFrameIndex ) );
		RED_VERIFY( packet.WriteString( path ) );
		RED_VERIFY( packet.Write( id ) );

		network->Send( RED_NET_CHANNEL_SCRIPT_DEBUGGER, packet );
	}
}

void CDebuggerLocalsHelper::SetLocalsValue( Red::System::Uint32 stackFrameIndex, const wxString& path, const wxString& value )
{
	Red::Network::Manager* network = Red::Network::Manager::GetInstance();
	if( network && network->IsInitialized() )
	{
		Red::Network::ChannelPacket packet( RED_NET_CHANNEL_SCRIPT_DEBUGGER );

		RED_VERIFY( packet.WriteString( "LocalsModification" ) );
		RED_VERIFY( packet.Write( stackFrameIndex ) );
		RED_VERIFY( packet.WriteString( path.wx_str() ) );
		RED_VERIFY( packet.WriteString( value.wx_str() ) );

		network->Send( RED_NET_CHANNEL_SCRIPT_DEBUGGER, packet );
	}
}
