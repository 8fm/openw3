/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#include "build.h"
#include "packetUtils.h"

namespace PacketUtilities {

void ReadString( Red::Network::IncomingPacket& packet, wxString& str )
{
	Red::System::Uint16 length = 0;
	Red::System::Uint16 position = packet.GetPosition();

	if( packet.GetPacket().HasDebugPadding() )
	{
		// 2 headers, One for the string (UniChar), one for the length (Uint16)
		position += sizeof( Red::Network::Packet::Padding ) * 2;
	}

	RED_VERIFY( packet.Peek( length, position ) );

	// Include space for the null terminator
	Red::System::Uint16 bufferSize = length + 1;

	wxStringBuffer buffer( str, bufferSize );

	Red::System::Char* strBuffer = static_cast< Red::System::Char* >( &(*buffer) );

	RED_VERIFY( packet.ReadString( strBuffer, bufferSize ) );
}

}