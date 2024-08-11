/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#include "build.h"
#include "profilerHelper.h"
#include "packetUtils.h"
#include "profiler.h"
#include "app.h"
#include "../../common/redNetwork/manager.h"

wxDEFINE_EVENT( ssEVT_PROFILER_STARTED_EVENT, CProfilerStartedEvent );
wxDEFINE_EVENT( ssEVT_PROFILER_REPORT_EVENT, CProfilerReportEvent );

wxIMPLEMENT_DYNAMIC_CLASS( CProfilerStartedEvent, wxEvent );
wxIMPLEMENT_DYNAMIC_CLASS( CProfilerReportEvent, wxEvent );

//////////////////////////////////////////////////////////////////////////

CProfilerStartedEvent::CProfilerStartedEvent( wxEventType commandType, int winid )
{

}

CProfilerStartedEvent::CProfilerStartedEvent( Red::System::Bool continuous )
:	wxEvent( wxID_ANY, ssEVT_PROFILER_STARTED_EVENT )
,	m_continuous( continuous )
{

}

CProfilerStartedEvent::~CProfilerStartedEvent()
{

}

//////////////////////////////////////////////////////////////////////////

CProfilerReportEvent::CProfilerReportEvent( wxEventType commandType, int winid )
{

}

CProfilerReportEvent::CProfilerReportEvent( SProfileData* data, Red::System::Uint32 size )
:	wxEvent( wxID_ANY, ssEVT_PROFILER_REPORT_EVENT )
,	m_data( data )
,	m_size( size )
{
	m_referenceCount = new Red::Threads::CAtomic< Red::System::Uint32 >( 1 );
}

CProfilerReportEvent::CProfilerReportEvent( const CProfilerReportEvent& other )
:	wxEvent( wxID_ANY, ssEVT_PROFILER_REPORT_EVENT )
,	m_data( other.m_data )
,	m_size( other.m_size )
,	m_referenceCount( other.m_referenceCount )
{
	m_referenceCount->Increment();
}

CProfilerReportEvent::~CProfilerReportEvent()
{
	if( m_referenceCount->Decrement() == 0 )
	{
		delete [] m_data;
		delete m_referenceCount;

		m_data = nullptr;
		m_referenceCount = nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_CLASS( CProfilerHelper, wxEvtHandler );

//////////////////////////////////////////////////////////////////////////

CProfilerHelper::CProfilerHelper()
:	m_profileReport( nullptr )
,	m_profileReportSize( 0 )
,	m_profileReportPosition( 0 )
,	m_profileReportId( 0 )
{
}

CProfilerHelper::~CProfilerHelper()
{
	if( Red::Network::Manager::GetInstance() )
	{
		Red::Network::Manager::GetInstance()->UnregisterListener( RED_NET_CHANNEL_SCRIPT_PROFILER, this );
	}
}

Red::System::Bool CProfilerHelper::Initialize( const wxChar* ip )
{
	if( Red::Network::Manager::GetInstance() )
	{
		Red::Network::Address address( ip,  wxTheSSApp->GetNetworkPort() );
		Red::Network::Manager::GetInstance()->ConnectTo( address, RED_NET_CHANNEL_SCRIPT_PROFILER );
		Red::Network::Manager::GetInstance()->RegisterListener( RED_NET_CHANNEL_SCRIPT_PROFILER, this );

		return true;
	}

	return false;
}

void CProfilerHelper::OnPacketReceived( const Red::System::AnsiChar* channelName, Red::Network::IncomingPacket& packet )
{
	if( Red::System::StringCompare( channelName, RED_NET_CHANNEL_SCRIPT_PROFILER ) == 0 )
	{
		Red::System::AnsiChar messageType[ 32 ];
		RED_VERIFY( packet.ReadString( messageType, ARRAY_COUNT_U32( messageType ) ) );

		if( Red::System::StringCompare( messageType, "profileReportHeader" ) == 0 )
		{
			ProfileReportHeaderReceived( packet );
		}
		else if( Red::System::StringCompare( messageType, "profileReportBody" ) == 0 )
		{
			ProfileReportBodyReceived( packet );
		}
		else if( Red::System::StringCompare( messageType, "profilingStarted" ) == 0 )
		{
			Red::System::Bool isContinuous = false;

			RED_VERIFY( packet.Read( isContinuous ) );

			CProfilerStartedEvent* event = new CProfilerStartedEvent( isContinuous );
			QueueEvent( event );
		}
	}
}

void CProfilerHelper::StartProfiling( Red::System::Uint32 framesToProfile )
{
	Red::Network::Manager* network = Red::Network::Manager::GetInstance();
	if( network && network->IsInitialized() )
	{
		Red::Network::ChannelPacket packet( RED_NET_CHANNEL_SCRIPT_PROFILER );

		RED_VERIFY( packet.WriteString( "profileDiscrete" ) );
		RED_VERIFY( packet.Write( framesToProfile ) );

		network->Send( RED_NET_CHANNEL_SCRIPT_PROFILER, packet );
	}
}

void CProfilerHelper::StartContinuousProfiling()
{
	Red::Network::Manager* network = Red::Network::Manager::GetInstance();
	if( network && network->IsInitialized() )
	{
		Red::Network::ChannelPacket packet( RED_NET_CHANNEL_SCRIPT_PROFILER );

		RED_VERIFY( packet.WriteString( "profileContinuousStart" ) );

		network->Send( RED_NET_CHANNEL_SCRIPT_PROFILER, packet );
	}
}

void CProfilerHelper::StopContinuousProfiling()
{
	Red::Network::Manager* network = Red::Network::Manager::GetInstance();
	if( network && network->IsInitialized() )
	{
		Red::Network::ChannelPacket packet( RED_NET_CHANNEL_SCRIPT_PROFILER );

		RED_VERIFY( packet.WriteString( "profileContinuousStop" ) );

		network->Send( RED_NET_CHANNEL_SCRIPT_PROFILER, packet );
	}
}

void CProfilerHelper::ProfileReportHeaderReceived( Red::Network::IncomingPacket& packet )
{
	RED_ASSERT( m_profileReport == nullptr, TXT( "Profile report already in progress" ) );
	
	RED_VERIFY( packet.Read( m_profileReportId ) );
	RED_VERIFY( packet.Read( m_profileReportSize ) );

	m_profileReport = new SProfileData[ m_profileReportSize ];

	m_profileReportPosition = 0;
}

void CProfilerHelper::ProfileReportBodyReceived( Red::Network::IncomingPacket& packet )
{
	Red::System::Uint32 id;
	RED_VERIFY( packet.Read( id ) );
	RED_ASSERT( id == m_profileReportId );

	Red::System::Uint16 size;
	RED_VERIFY( packet.Read( size ) );

	for ( Red::System::Uint32 i = 0; i < size; ++i )
	{
		RED_ASSERT( m_profileReportPosition < m_profileReportSize, TXT( "Incorrect Profile Report size" ) );

		SProfileData& data = m_profileReport[ m_profileReportPosition ];
		++m_profileReportPosition;

		PacketUtilities::ReadString( packet, data.m_class );
		PacketUtilities::ReadString( packet, data.m_function );

		RED_VERIFY( packet.Read( data.m_numCalls ) );
		RED_VERIFY( packet.Read( data.m_flags ) );

		RED_VERIFY( packet.Read( data.m_totalTimeIn ) );
		RED_VERIFY( packet.Read( data.m_totalTimeEx ) );

		RED_VERIFY( packet.Read( data.m_prcIn ) );
		RED_VERIFY( packet.Read( data.m_prcEx ) );

		RED_VERIFY( packet.Read( data.m_recursion ) );

		RED_VERIFY( packet.Read( data.m_numIndicies ) );

		data.m_indicies = new Red::System::Int32[ data.m_numIndicies ];

		for ( Red::System::Uint32 j = 0; j < data.m_numIndicies; ++j )
		{
			RED_VERIFY( packet.Read( data.m_indicies[ j ] ) );
		}
	}

	if( m_profileReportPosition == m_profileReportSize )
	{
		CProfilerReportEvent* event = new CProfilerReportEvent( m_profileReport, m_profileReportSize );
		QueueEvent( event );

		m_profileReport = nullptr;
	}
}
