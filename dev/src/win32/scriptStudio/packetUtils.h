/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#pragma once
#ifndef __PACKET_UTILITIES_H__
#define __PACKET_UTILITIES_H__

#include "../../common/redNetwork/packet.h"

namespace PacketUtilities
{
	void ReadString( Red::Network::IncomingPacket& packet, wxString& str );
}

#endif //__PACKET_UTILITIES_H__
