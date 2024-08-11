/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#ifndef NO_DEBUG_SERVER

#include "factsDebuggerPlugin.h"
#include "factsDebuggerCommands.h"
#include "../engine/debugServerManager.h"

const Uint32 LinesPerMessage = 150;
const Double FactsSendingInterval = 3.0;
const Uint32 FactsLimitPerSendOperation = 2000;

Bool CFactsDebuggerPlugin::Init() 
{
#ifndef RED_PROFILE_BUILD
	m_lastSendTime = m_factsSendingTimer.GetSeconds();
	m_factsDB = GCommonGame->GetSystem< CFactsDB >();
	GGame->GetGlobalEventsManager()->AddListener( GEC_Fact, this );
	DBGSRV_REG_COMMAND( this, "getFactIDs", CDebugServerCommandGetFactIDs );
	DBGSRV_REG_COMMAND( this, "getFactData", CDebugServerCommandGetFactData );	
#endif
	return true;
}

Bool CFactsDebuggerPlugin::ShutDown()
{
	#ifndef RED_PROFILE_BUILD
	GGame->GetGlobalEventsManager()->RemoveListener( GEC_Fact, this );
	m_pendingFacts.Clear();
	#endif
	return true;
}

void CFactsDebuggerPlugin::GameStarted()
{

}

void CFactsDebuggerPlugin::GameStopped()
{
	m_pendingFacts.Clear();
}

void CFactsDebuggerPlugin::AttachToWorld()
{

}

void CFactsDebuggerPlugin::DetachFromWorld()
{

}

void CFactsDebuggerPlugin::Tick()
{
	#ifndef RED_PROFILE_BUILD
	if ( !DBGSRV_CALL( IsConnected() ) )
		return;

	const Double now = m_factsSendingTimer.GetSeconds();
	if ( !ShouldSendFacts( now ) )
		return;

	SendPendingFacts();
	m_lastSendTime = now;
	#endif
}

Uint32 CFactsDebuggerPlugin::SendFactIDs() const
{
	#ifndef RED_PROFILE_BUILD
	if ( m_factsDB == nullptr )
	{
		return 0;
	}

	TDynArray< String > idsList;
	m_factsDB->GetIDs( idsList );

	Uint32 linesInPacket = 0;
	Uint32 packetsCount = 0;

	Red::Network::ChannelPacket packet( RED_NET_CHANNEL_DEBUG_SERVER );
	packet.WriteString( "factIDs" ); linesInPacket++;

	const Uint32 count = idsList.Size();
	for ( Uint32 i = 0; i < count; ++i )
	{
		const String& factID = idsList[i];
		packet.WriteString( UNICODE_TO_ANSI( idsList[i].AsChar() ) );
		linesInPacket++;
		if ( linesInPacket % LinesPerMessage == 0 )
		{
			Red::Network::Manager::GetInstance()->Send( RED_NET_CHANNEL_DEBUG_SERVER, packet );
			++packetsCount;
			packet.Clear( RED_NET_CHANNEL_DEBUG_SERVER );
			packet.WriteString( "factIDs" ); linesInPacket++;
		}	
	}

	if ( linesInPacket != 0 )
	{
		Red::Network::Manager::GetInstance()->Send( RED_NET_CHANNEL_DEBUG_SERVER, packet );
		++packetsCount;
		packet.Clear( RED_NET_CHANNEL_DEBUG_SERVER );
	}

	packet.WriteString( "factIDs" );
	Red::Network::Manager::GetInstance()->Send( RED_NET_CHANNEL_DEBUG_SERVER, packet );

	return ++packetsCount;
	#else
	return 0;
	#endif
}

Uint32 CFactsDebuggerPlugin::SendFactData( const String& factId ) const
{
	#ifndef RED_PROFILE_BUILD
	if ( m_factsDB == nullptr )
	{
		return 0;
	}

	TDynArray< const CFactsDB::Fact* > factsList;	
	m_factsDB->GetFacts( factId, factsList );

	Uint32 linesInPacket = 0;
	Uint32 packetsCount = 0;

	Red::Network::ChannelPacket packet( RED_NET_CHANNEL_DEBUG_SERVER );
	packet.WriteString( "factData" ); linesInPacket++;

	const Uint32 count = factsList.Size();
	for ( Uint32 i = 0; i < count; ++i )
	{
		packet.WriteString( UNICODE_TO_ANSI( FactAsString( factsList[i] ).AsChar() ) );
		linesInPacket++;
		if ( linesInPacket % LinesPerMessage == 0 )
		{
			Red::Network::Manager::GetInstance()->Send( RED_NET_CHANNEL_DEBUG_SERVER, packet );
			++packetsCount;
			packet.Clear( RED_NET_CHANNEL_DEBUG_SERVER );
			packet.WriteString( "factData" ); linesInPacket++;
		}	
	}

	if ( linesInPacket != 0 )
	{
		Red::Network::Manager::GetInstance()->Send( RED_NET_CHANNEL_DEBUG_SERVER, packet );
		++packetsCount;
		packet.Clear( RED_NET_CHANNEL_DEBUG_SERVER );
	}

	packet.WriteString( "factData" );
	Red::Network::Manager::GetInstance()->Send( RED_NET_CHANNEL_DEBUG_SERVER, packet );

	return ++packetsCount;
	#else
	return 0;
	#endif
}

String CFactsDebuggerPlugin::FactAsString( const CFactsDB::Fact* fact ) const
{
	#ifndef RED_PROFILE_BUILD
	EngineTime time( fact->m_time );

	String timeStr = String::Printf( TXT( "%f" ), (float) time );
	String tmpStr = String::Printf( TXT( "Time: %s, Val: %d, Exp: %s" ), 
		timeStr.AsChar(), 
		fact->m_value, 
		( ( fact->m_expiryTime == CFactsDB::EXP_NEVER ) ? TXT( "no" ) : TXT( "yes" ) ) );

	return tmpStr;
	#else
	return String::EMPTY;
	#endif
}

void CFactsDebuggerPlugin::OnGlobalEvent( EGlobalEventCategory eventCategory, EGlobalEventType eventType, const SGlobalEventParam& param )
{
	#ifndef RED_PROFILE_BUILD
	if ( !DBGSRV_CALL( IsConnected() ) )
		return;

	if ( eventCategory != GEC_Fact )
		return;

	SFactInfo factInfo;
	factInfo.eventType = eventType;
	factInfo.factId = param.Get< String >();
	m_pendingFacts.Push( factInfo );
	#endif
}

void CFactsDebuggerPlugin::SendFactAdded( const String& factId ) const
{
	#ifndef RED_PROFILE_BUILD
	Red::Network::ChannelPacket packet( RED_NET_CHANNEL_DEBUG_SERVER );	
	packet.WriteString( "factAdded" );
	packet.WriteString( UNICODE_TO_ANSI( factId.AsChar() ) );
	Red::Network::Manager::GetInstance()->Send( RED_NET_CHANNEL_DEBUG_SERVER, packet );
	#endif
}

void CFactsDebuggerPlugin::SendFactRemoved( const String& factId ) const
{
	#ifndef RED_PROFILE_BUILD
	Red::Network::ChannelPacket packet( RED_NET_CHANNEL_DEBUG_SERVER );	
	packet.WriteString( "factRemoved" );
	packet.WriteString( UNICODE_TO_ANSI( factId.AsChar() ) );
	Red::Network::Manager::GetInstance()->Send( RED_NET_CHANNEL_DEBUG_SERVER, packet );
	#endif
}

Bool CFactsDebuggerPlugin::ShouldSendFacts( const Double currentTime ) const
{
	return currentTime - m_lastSendTime > FactsSendingInterval;
}

void CFactsDebuggerPlugin::SendPendingFacts()
{
	#ifndef RED_PROFILE_BUILD
	Uint32 factsSent = 0;

	while( !m_pendingFacts.Empty() )
	{
		const SFactInfo& factInfo = m_pendingFacts.Front();
		m_pendingFacts.Pop();

		if ( factInfo.eventType == GET_FactAdded )
		{
			SendFactAdded( factInfo.factId );
		}
		else if ( factInfo.eventType == GET_FactRemoved )
		{
			SendFactRemoved( factInfo.factId );
		}

		if ( ++factsSent >= FactsLimitPerSendOperation )
			break;
	}
	#endif
}

#endif
