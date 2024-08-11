/***********************************************************************
  The content of this file includes source code for the sound engine
  portion of the AUDIOKINETIC Wwise Technology and constitutes "Level
  Two Source Code" as defined in the Source Code Addendum attached
  with this file.  Any use of the Level Two Source Code shall be
  subject to the terms and conditions outlined in the Source Code
  Addendum and the End User License Agreement for Wwise(R).

  Version: v2013.2.9  Build: 4872
  Copyright (c) 2006-2014 Audiokinetic Inc.
 ***********************************************************************/


/*	
	----------------------------------------------------------------------------------------
	PC (Windows) AND Xbox360 AND PS3 implementation.
	Location: \Wwise\Communication\Remote\PC
	Header location: \Wwise\Communication\Remote\CodeBase

	-- IMPORTANT NOTE -- : Also used for the Xbox360 and PS3 implementation, even though the implementation file is located in the PC folder!!
	-----------------------------------------------------------------------------------------
*/

// Use <> here because we want the one in the project folder to be included (not the one beside this .cpp file)
#include <stdafx.h>
#include "GameSocketAddr.h"

GameSocketAddr::GameSocketAddr()
{
	m_sockAddr.sin_family = AF_INET;
	m_sockAddr.sin_port = 0;
	m_sockAddr.sin_addr.s_addr = INADDR_ANY;
}

GameSocketAddr::GameSocketAddr( const GameSocketAddr& in_rAddr )
{
	m_sockAddr = in_rAddr.m_sockAddr;
}

GameSocketAddr::GameSocketAddr( AkUInt32 in_ip, AkUInt16 in_port )
{
	m_sockAddr.sin_family = AF_INET;
	m_sockAddr.sin_port = htons( in_port );
	m_sockAddr.sin_addr.s_addr = htonl( in_ip );
}

GameSocketAddr::~GameSocketAddr()
{
}

void GameSocketAddr::SetIP( AkUInt32 in_ip )
{
	m_sockAddr.sin_addr.s_addr = htonl( in_ip );
}

AkUInt32 GameSocketAddr::GetIP() const
{
	return ntohl( m_sockAddr.sin_addr.s_addr );
}

void GameSocketAddr::SetPort( AkUInt16 in_port )
{
	m_sockAddr.sin_port = htons( in_port );
}

AkUInt16 GameSocketAddr::GetPort() const
{
	return ntohs( m_sockAddr.sin_port );
}

const SocketAddrType& GameSocketAddr::GetInternalType() const 
{
	return m_sockAddr;
}

SocketAddrType& GameSocketAddr::GetInternalType()
{
	return m_sockAddr;
}

unsigned long GameSocketAddr::ConvertIP( const char* in_pszIP )
{
	return ntohl( inet_addr( in_pszIP ) );
}
