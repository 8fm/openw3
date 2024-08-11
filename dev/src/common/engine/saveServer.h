/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#pragma once
#ifndef __SAVE_SERVER_H__
#define __SAVE_SERVER_H__

#include "../redNetwork/network.h"
#include "../redNetwork/address.h"

#ifdef SAVE_SERVER_ENABLED
#define SS_NET_CALL( x ) if ( false == Red::Network::Base::x ) \
{ \
	Int32 er = Red::Network::Base::GetLastError(); \
	if ( er != RED_NET_ERR_ACCEPT_OK ) \
	{  \
		RED_LOG( SaveServer, TXT("%ls failed, error code: %ld"), TXT(#x), er );	\
	} \
	return; \
}

class CSaveServer
{
	Red::Network::SocketId m_serverSocketId;
	Red::Network::SocketId m_clientSocketId;
	Bool m_updating;
	Bool m_running : 1;
	Bool m_connected : 1;
	

	static const Uint32 CMD_BUF_SIZE = 12;

public:
	CSaveServer()
		: m_running( false )
		, m_connected( false )
	{
	}

	void Open();
	void Update();
	void Close();

private:
	Uint32 RecvAll( Uint8* buffer, Uint32 size, Bool& noDataYet );
	Uint32 SendAll( Uint8* buffer, Uint32 size );
	void ZeroResponse();
	void CloseClient();

	void OnLoadCommand();
	void OnGrabCommand();
	void OnListCommand();
};
#endif // SAVE_SERVER_ENABLED

#endif // __USER_PROFILE_MANAGER_H__