/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#ifdef RED_NETWORK_ENABLED

#include "configVar.h"
#include "configVarSystem.h"
#include "configVarRegistry.h"
#include "remoteConfig.h"

#include "../redNetwork/manager.h"

namespace Config
{
	const AnsiChar* CRemoteConfig::CHANNEL_NAME = "Config";
	const Uint32 CRemoteConfig::CHANNEL_MAGIC = 0xCC00CC00; // keep distinct
	const Uint32 CRemoteConfig::MAX_PACKET_SIZE = 60000;

	CRemoteConfig*	CRemoteConfig::st_instance = nullptr;

	CRemoteConfig::CRemoteConfig( )
		: m_manager( &SConfig::GetInstance() )
	{
		Red::Network::Manager::GetInstance()->RegisterListener( CHANNEL_NAME, this );
	}

	CRemoteConfig::~CRemoteConfig()
	{
		Red::Network::Manager::GetInstance()->UnregisterListener( CHANNEL_NAME, this );
	}

	void CRemoteConfig::Initialize()
	{
		if (!st_instance)
		{
			st_instance = new CRemoteConfig();
		}
	}

	void CRemoteConfig::Shutdown()
	{
		if (st_instance)
		{
			delete st_instance;
			st_instance = nullptr;
		}
	}
	
	void CRemoteConfig::OnPacketReceived( const Red::System::AnsiChar* channelName, Red::Network::IncomingPacket& packet )
	{
		// make sure it's a config packet
		Uint32 magic = 0;
		if ( !packet.Read<Uint32>( magic ) || (magic != CHANNEL_MAGIC) )
			return;

		// read the command
		AnsiChar cmd[16];
		if ( !packet.ReadString( cmd, ARRAY_COUNT_U32(cmd) ) )
			return;

		// process command
		ProcessPacket( cmd, packet );
	}

	bool CRemoteConfig::ProcessPacket( const AnsiChar* cmd, Red::Network::IncomingPacket& packet )
	{
		if ( 0 == Red::StringCompare( cmd, "list" ) )
		{
			// get group prefix
			AnsiChar groupPrefix[32];
			if ( !packet.ReadString( groupPrefix, ARRAY_COUNT_U32(groupPrefix) ) )
				return false;

			// get name prefix
			AnsiChar namePrefix[32];
			if ( !packet.ReadString( namePrefix, ARRAY_COUNT_U32(namePrefix) ) )
				return false;

			// list all config variables matching the selection
			TDynArray< IConfigVar* > allVars;
			m_manager->GetRegistry().EnumVars( allVars, groupPrefix, namePrefix );

			// send the list (in parts if needed)
			Uint32 varIndex = 0;
			Uint32 numPackets = 0;
			Uint32 numDataSent = 0;
			while ( varIndex < allVars.Size() )
			{
				Red::Network::OutgoingPacket sendPacket;
				sendPacket.Write<Uint32>(CHANNEL_MAGIC);
				sendPacket.WriteString<AnsiChar>("vars");

				while ( varIndex < allVars.Size() )
				{
					const IConfigVar* var = allVars[ varIndex ];

					// get value as string
					String currentValue;
					var->GetText( currentValue ); // we don't care if we fail here

					// compute the size needed to store the result
					Uint32 sizeNeeded = currentValue.GetLength();
					sizeNeeded += (Uint32)Red::StringLength(var->GetName());
					sizeNeeded += (Uint32)Red::StringLength(var->GetGroup());

					// to big for current packet, split it
					if ( sendPacket.GetSize() + sizeNeeded >= MAX_PACKET_SIZE )
						break;

					// compose runtime flags
					Uint8 flags = 0;
					if ( var->HasFlag(eConsoleVarFlag_ReadOnly ) ) flags |= 1;
					if ( var->HasFlag(eConsoleVarFlag_Developer ) ) flags |= 2;
					
					// add variable data to the packet
					sendPacket.Write<Uint8>(1 + (Uint8) var->GetType());
					sendPacket.Write<Uint8>(flags);
					sendPacket.WriteString<AnsiChar>(var->GetName());
					sendPacket.WriteString<AnsiChar>(var->GetGroup());
					sendPacket.WriteString<AnsiChar>(UNICODE_TO_ANSI(currentValue.AsChar()));

					// mark variable as sent
					varIndex += 1;
				}

				// end of packet marker
				sendPacket.Write<Uint8>(0);
				numPackets += 1;
				numDataSent += sendPacket.GetSize();

				// send it
				Red::Network::Manager::GetInstance()->Send( CHANNEL_NAME, sendPacket );
			}

			// stats
			LOG_CORE( TXT("ConfigRemote: sent %d vars in %d packets (%d bytes)"), varIndex, numPackets, numDataSent );

			// serviced
			return true;
		}
		else if ( 0 == Red::StringCompare( cmd, "get" ) )
		{
			// get group name
			AnsiChar groupName[128];
			if ( !packet.ReadString( groupName, ARRAY_COUNT_U32(groupName) ) )
				return false;

			// get variable name
			AnsiChar varName[128];
			if ( !packet.ReadString( varName, ARRAY_COUNT_U32(varName) ) )
				return false;

			// find variable
			IConfigVar* var = m_manager->GetRegistry().Find( groupName, varName );
			if ( !var )
				return false;

			// read current value
			String currentValue;
			if ( !var->GetText( currentValue ) )
				return false;

			// compose runtime flags
			Uint8 flags = 0;
			if ( var->HasFlag(eConsoleVarFlag_ReadOnly ) ) flags |= 1;
			if ( var->HasFlag(eConsoleVarFlag_Developer ) ) flags |= 2;

			// send the result - a single "list" packet will do
			Red::Network::OutgoingPacket sendPacket;
			sendPacket.Write<Uint32>(CHANNEL_MAGIC);
			sendPacket.WriteString<AnsiChar>("vars");
			sendPacket.Write<Uint8>(1 + (Uint8) var->GetType());
			sendPacket.Write<Uint8>(flags);
			sendPacket.WriteString<AnsiChar>(var->GetName());
			sendPacket.WriteString<AnsiChar>(var->GetGroup());
			sendPacket.WriteString<AnsiChar>(UNICODE_TO_ANSI(currentValue.AsChar()));
			sendPacket.Write<Uint8>(0); // EOP - do not remove

			// send it
			Red::Network::Manager::GetInstance()->Send( CHANNEL_NAME, sendPacket );

			// serviced
			return true;
		}
		else if ( 0 == Red::StringCompare( cmd, "set" ) )
		{
			// get group name
			AnsiChar groupName[128];
			if ( !packet.ReadString( groupName, ARRAY_COUNT_U32(groupName) ) )
				return false;

			// get variable name
			AnsiChar varName[128];
			if ( !packet.ReadString( varName, ARRAY_COUNT_U32(varName) ) )
				return false;

			// get value to set
			AnsiChar value[2048];
			if ( !packet.ReadString( value, ARRAY_COUNT_U32(value) ) )
				return false;

			// find variable
			IConfigVar* var = m_manager->GetRegistry().Find( groupName, varName );
			if ( !var )
				return false;

			// set the value
			const String newValue( ANSI_TO_UNICODE( value ) );
			if ( !var->SetText( newValue, eConfigVarSetMode_Remote ) )
				return false;

			// serviced
			return true;
		}
		else if ( 0 == Red::StringCompare( cmd, "save" ) )
		{
			m_manager->Save();
			return true;
		}

		// not serviced
		return false;
	}

}

#endif // RED_NETWORK_ENABLED
