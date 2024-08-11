/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#include "build.h"

#include "scriptCompilationHelper.h"

#include "../redSystem/utilityTimers.h"
#include "../redNetwork/manager.h"

#include "../core/rttiSerializer.h"
#include "../core/version.h"
#include "../core/scriptingSystem.h"
#include "../core/feedback.h"
#include "../core/contentManager.h"

#include "baseEngine.h"

#define RED_NET_CHANNEL_SCRIPT "scripts"
#define RED_NET_CHANNEL_SCRIPT_PROFILER "ScriptProfiler"

#ifdef RED_NETWORK_ENABLED

CScriptNetworkHandler::CScriptNetworkHandler()
:	m_overrideScriptPath( false )
,	m_reloadRequested( false )
,	m_profileFrameCount( 0 )
,	m_continuousProfiling( false )
,	m_profileReportCount( 0 )
,	m_sendingProfilingPackets( false )
{

}

CScriptNetworkHandler::~CScriptNetworkHandler()
{

}

void CScriptNetworkHandler::Initialize()
{
	RED_ASSERT( Red::Network::Manager::GetInstance() );

	Red::Network::Manager::GetInstance()->RegisterListener( RED_NET_CHANNEL_SCRIPT, this );
	Red::Network::Manager::GetInstance()->RegisterListener( RED_NET_CHANNEL_SCRIPT_PROFILER, this );
}

void CScriptNetworkHandler::OnPacketReceived( const AnsiChar* channelName, Red::Network::IncomingPacket& packet )
{
	AnsiChar instruction[ 32 ];
	RED_VERIFY( packet.ReadString( instruction, ARRAY_COUNT_U32( instruction ) ) );

	if( Red::System::StringCompare( channelName, RED_NET_CHANNEL_SCRIPT ) == 0 )
	{
		if( Red::System::StringCompare( instruction, "reload" ) == 0 )
		{
			m_reloadRequested = true;

			if( packet.Read( m_overrideScriptPath ) )
			{
				if( m_overrideScriptPath )
				{
					Char pathOverride[ 1024 ];
					RED_VERIFY( packet.ReadString( pathOverride, ARRAY_COUNT_U32( pathOverride ) ) );

					m_overriddenScriptPath = pathOverride;
				}
			}
		}
		else if( Red::System::StringCompare( instruction, "pkgSync" ) == 0 )
		{
			THashMap< String, String > dirs;
			GContentManager->EnumScriptDirectories( dirs );

			Red::Network::ChannelPacket pkgSyncPkt( RED_NET_CHANNEL_SCRIPT );

			RED_VERIFY( pkgSyncPkt.WriteString( "pkgSyncListing" ) );
			RED_VERIFY( pkgSyncPkt.Write( dirs.Size() ) );

			for( auto it : dirs )
			{
				RED_VERIFY( pkgSyncPkt.WriteString( it.m_first.AsChar() ) );
				RED_VERIFY( pkgSyncPkt.WriteString( it.m_second.AsChar() ) );
			}

			Red::Network::Manager::GetInstance()->Send( RED_NET_CHANNEL_SCRIPT, pkgSyncPkt );
		}
	}
	else if( Red::System::StringCompare( channelName, RED_NET_CHANNEL_SCRIPT_PROFILER ) == 0 )
	{
		if( Red::System::StringCompare( instruction, "profileDiscrete" ) == 0 )
		{
			if( !m_continuousProfiling.GetValue() )
			{
				Uint32 framesToProfile = 0;

				RED_VERIFY( packet.Read( framesToProfile ) );

				StartProfiling( framesToProfile );
			}
		}
		else if( Red::System::StringCompare( instruction, "profileContinuousStart" ) == 0 )
		{
			if( !m_continuousProfiling.GetValue() && m_profileFrameCount.GetValue() == 0 )
			{
				m_profilingRequested.SetValue( true );
				m_continuousProfiling.SetValue( true );
			}
		}
		else if( Red::System::StringCompare( instruction, "profileContinuousStop" ) == 0 )
		{
			if( m_continuousProfiling.GetValue() )
			{
				m_continuousProfiling.SetValue( false );
			}
		}
	}
}

void CScriptNetworkHandler::StartProfiling( Uint32 numberOfFramesToProfile )
{
	if( numberOfFramesToProfile > 0 )
	{
		if( m_profileFrameCount.GetValue() == 0 )
		{
			m_profilingRequested.SetValue( true );
		}

		m_profileFrameCount.ExchangeAdd( numberOfFramesToProfile );
	}
}

void CScriptNetworkHandler::EndProfiling()
{
	// Grab function report
	TDynArray< FuncPerfData* > perfData;
	SRTTI::GetInstance().EndFunctionProfiling( perfData );

	// Analyze report
	WriteProfilingReport( perfData );

	// Done
	perfData.ClearPtr();
}

void CScriptNetworkHandler::WriteProfilingReport( TDynArray< FuncPerfData* > perfData )
{
	Char logBuffer[ Red::System::Log::MAX_LINE_LENGTH ];
	Uint32 bufferUsed = 0;

	for ( Uint32 i = 0; i < perfData.Size(); ++i )
	{
		FuncPerfData* data = perfData[ i ];
	
		// Stats
		bufferUsed += Red::System::SNPrintF
		(
			&logBuffer[ bufferUsed ],
			Red::System::Log::MAX_LINE_LENGTH - bufferUsed,
			TXT( "%" ) RED_PRIWs TXT( "%45" ) RED_PRIWs TXT( "| %6i | %4.2Lf\n" ),
			( data->m_function->GetClass() )? ( data->m_function->GetClass()->GetName().AsString() + TXT( ":" ) ).AsChar() : TXT( "" ),
			data->m_function->GetName().AsString().AsChar(),
			data->m_numCalls,
			( data->m_numTicks / Red::System::Clock::GetInstance().GetTimer().GetFrequency() ) * 1000.0
		);
	}

	RED_LOG( ScriptProfiler, logBuffer );

	// Index functions
	THashMap< CFunction*, Int32 > functionIndices;
	functionIndices.Reserve( perfData.Size() );
	for ( Uint32 i = 0; i < perfData.Size(); ++i )
	{
		CFunction* func = perfData[ i ]->m_function;
		functionIndices.Insert( func, functionIndices.Size() );
	}

    // Calculate total ticks (sum of top level function ticks)
	TDynArray<Bool> calledFunctionIndices;
	calledFunctionIndices.Grow( perfData.Size() );
	for ( Uint32 i = 0; i < perfData.Size(); ++i )
	{
		FuncPerfData* data = perfData[ i ];
		for ( Uint32 j = 0; j < data->m_called.Size(); ++j )
		{
			// Set flag for all called functions
			Int32 index = -1;

			if ( functionIndices.Find( data->m_called[ j ], index ) )
			{
				calledFunctionIndices[ index ] = true;
			}
		}
	}

	Uint64 sumTicks = 0;
	for ( Uint32 i = 0; i < perfData.Size(); ++i )
	{
		// Count ticks from all functions with not set flag
		if (calledFunctionIndices[i] == false)
		{
       		FuncPerfData* data = perfData[i];
			sumTicks += data->m_numTicks;
		}
	}

	Int32 totalTicks = (Int32)(sumTicks / Red::System::Clock::GetInstance().GetTimer().GetFrequency() * 1000000.0);

	// "Header" packet
	Red::Network::ChannelPacket* header = new Red::Network::ChannelPacket( RED_NET_CHANNEL_SCRIPT_PROFILER );
	
	RED_VERIFY( header->WriteString( "profileReportHeader" ) );
	
	// Write an ID to identify packets associated with this report
	RED_VERIFY( header->Write( m_profileReportCount ) );

	// Tell the system how many functions are stored in this packet 
	RED_VERIFY( header->Write( perfData.Size() ) );

	m_profilingPackets.PushBack( header );

	// Send Packet
	Red::Network::ChannelPacket* packet = nullptr;
	Uint16 numFunctionsInPacket = 0;
	Uint16 functionCountPacketPosition = 0;

	for ( Uint32 i = 0; i < perfData.Size(); ++i )
	{
		if( !packet )
		{
			packet = new Red::Network::ChannelPacket( RED_NET_CHANNEL_SCRIPT_PROFILER );
			numFunctionsInPacket = 0;

			// This packet contains profiling data
			RED_VERIFY( packet->WriteString( "profileReportBody" ) );

			// Write an ID to identify packets associated with this report
			RED_VERIFY( packet->Write( m_profileReportCount ) );

			// Write an ID to identify packets associated with this report
			functionCountPacketPosition = packet->GetSize();
			RED_VERIFY( packet->Write( numFunctionsInPacket ) );
		}

		if( !AddProfileDataToPacket( packet, perfData[ i ], totalTicks, functionIndices ) )
		{
			// We'll need to put this function into the next packet instead
			--i;

			//Update the function count value in the packet
			RED_VERIFY( packet->Write( numFunctionsInPacket, functionCountPacketPosition ) );

			// Queue packet for sending
			m_profilingPackets.PushBack( packet );

			packet = nullptr;
		}
		else
		{
			++numFunctionsInPacket;
		}
	}

	if( packet )
	{
		//Update the function count value in the packet
		RED_VERIFY( packet->Write( numFunctionsInPacket, functionCountPacketPosition ) );

		// Queue packet for sending
		m_profilingPackets.PushBack( packet );

		packet = nullptr;
	}

	m_profilingPacketSent.SetValue( 0 );
	m_sendingProfilingPackets.SetValue( true );
}

#define RETURN_IF_FALSE( a ) if( !a ) { return false; }

Bool CScriptNetworkHandler::AddProfileDataToPacket( Red::Network::ChannelPacket* packet, FuncPerfData* data, Uint32 totalTicks, THashMap< CFunction*, Int32 >& functionIndices )
{
	CFunction* func = data->m_function;

	// Write class name
	CName className = ( func->GetClass() )? func->GetClass()->GetName() : CName::NONE;
	RETURN_IF_FALSE( packet->WriteString( className.AsChar() ) );

	// Write function name
	RETURN_IF_FALSE( packet->WriteString( func->GetName().AsChar() ) );

	// Write number of times this function was called
	RETURN_IF_FALSE( packet->Write( data->m_numCalls ) );

	// Write flags
	RETURN_IF_FALSE( packet->Write( func->GetFlags() ) );

	// Write total time ( inclusive ) spent in this function
	const Int32 totalTimeIn = ( Int32 )( data->m_numTicks / Red::System::Clock::GetInstance().GetTimer().GetFrequency() * 1000000.0 );
	const Int32 totalTimeEx = totalTimeIn - ( Int32 )( data->m_numExTicks / Red::System::Clock::GetInstance().GetTimer().GetFrequency() * 1000000.0 );
	RETURN_IF_FALSE( packet->Write( totalTimeIn ) );
	RETURN_IF_FALSE( packet->Write( totalTimeEx ) );

	// Write percentage of time spent in this function
	const Float prcIn = totalTimeIn / (Float)totalTicks;
	const Float prcEx = totalTimeEx / (Float)totalTicks;
	RETURN_IF_FALSE( packet->Write( prcIn ) );
	RETURN_IF_FALSE( packet->Write( prcEx ) );

	// Write recursion level
	RETURN_IF_FALSE( packet->Write( data->m_recusion ) );

	// Write called functions
	RETURN_IF_FALSE( packet->Write( data->m_called.Size() ) );
	for ( Uint32 j = 0; j < data->m_called.Size(); ++j )
	{
		// Convert function to indices
		Int32 index = -1;
		RETURN_IF_FALSE( functionIndices.Find( data->m_called[ j ], index ) );
		RETURN_IF_FALSE( packet->Write( index ) );
	}

	return true;
}

void CScriptNetworkHandler::Update()
{
	extern Bool GProfileFunctionCalls;

	if( m_reloadRequested )
	{
		TDynArray<String> scriptFiles;

		if( m_overrideScriptPath )
		{
			GFileManager->FindFiles( m_overriddenScriptPath, TXT( "*.ws" ), scriptFiles, true );
			for ( String& filePath : scriptFiles )
			{
				CUpperToLower conv( filePath.TypedData(), filePath.Size() );
			}
		}
		else
		{
			GContentManager->EnumScriptFiles( scriptFiles );
		}

		GScriptingSystem->LoadScripts( NULL, scriptFiles );

		m_reloadRequested = false;
	}

	if( m_sendingProfilingPackets.GetValue() )
	{
		if( Red::Network::Manager::GetInstance()->Send( RED_NET_CHANNEL_SCRIPT_PROFILER, *m_profilingPackets[ m_profilingPacketSent.GetValue() ] ) )
		{
			m_profilingPacketSent.Increment();

			if( m_profilingPacketSent.GetValue() == m_profilingPackets.Size() )
			{
				m_profilingPackets.ClearPtr();
				m_sendingProfilingPackets.SetValue( false );
			}
		}
		else
		{
			RED_LOG_WARNING( ScriptProfiler, TXT( "Unable to send profiling packet, waiting another frame" ) );
		}
	}
	else
	{
		if( m_profilingRequested.GetValue() )
		{
			SRTTI::GetInstance().BeginFunctionProfiling();

			m_profilingRequested.SetValue( false );

			Red::Network::ChannelPacket packet( RED_NET_CHANNEL_SCRIPT_PROFILER );

			packet.WriteString( "profilingStarted" );
			packet.Write( m_continuousProfiling.GetValue() );

			Red::Network::Manager::GetInstance()->Send( RED_NET_CHANNEL_SCRIPT_PROFILER, packet );
		}
		else if( GProfileFunctionCalls )
		{
			if( ( m_profileFrameCount.GetValue() > 0 ) && ( m_profileFrameCount.Decrement() == 0 ) )
			{
				EndProfiling();
			}
			else if( m_profileFrameCount.GetValue() == 0 && !m_continuousProfiling.GetValue() )
			{
				EndProfiling();
			}
		}
	}

}

#endif // RED_NETWORK_ENABLED
