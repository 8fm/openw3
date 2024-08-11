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

#include "stdafx.h"
#include "GameSocket.h"
#include "GameSocketAddr.h"
#include <stdio.h>
#include <time.h>

#include <libnetctl.h>

GameSocket::GameSocket()
	: m_socket( INVALID_SOCKET )
{
}

GameSocket::~GameSocket()
{
}

bool GameSocket::Create( AkInt32 in_type, AkInt32 in_protocol, bool /*in_bRequiresAccumulator*/  )
{
	m_socket = sceNetSocket( "AK::Comm", SCE_NET_AF_INET, in_type, in_protocol );

	if( m_socket < 0 )
		m_socket = INVALID_SOCKET;

	return m_socket != INVALID_SOCKET;
}

void GameSocket::ReuseAddress()
{
	int bReuseAddr = 1;
	sceNetSetsockopt( m_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&bReuseAddr, sizeof bReuseAddr );
}

void GameSocket::NoDelay()
{
	int flag = 1;
	sceNetSetsockopt( m_socket, IPPROTO_TCP, SCE_NET_TCP_NODELAY, (char *) &flag, sizeof(int) );
}

AkInt32 GameSocket::Connect( const GameSocketAddr& in_rAddr )
{
	int res = sceNetConnect( m_socket, (SceNetSockaddr*)&in_rAddr.GetInternalType(), sizeof( in_rAddr.GetInternalType() ) );

	if ( res < 0 )
		return SOCKET_ERROR;

	return res;
}

AkInt32 GameSocket::Bind( const GameSocketAddr& in_rAddr )
{
	int result = sceNetBind( m_socket, (SceNetSockaddr*)&in_rAddr.GetInternalType(), sizeof( in_rAddr.GetInternalType() ) );

	if( result < 0 )
		result = SOCKET_ERROR;

	return result;
}

AkInt32 GameSocket::Listen( AkInt32 in_backlog ) const
{
	int res = sceNetListen( m_socket, in_backlog );

	if ( res < 0 )
		return SOCKET_ERROR;

	return res;
}

void GameSocket::Accept( GameSocketAddr& out_rAddr, GameSocket & out_targetSocket )
{
	AKASSERT( out_targetSocket.m_socket == INVALID_SOCKET );

	int addrSize = sizeof( out_rAddr.GetInternalType() );
	SOCKET res = sceNetAccept( m_socket, (SceNetSockaddr*)&out_rAddr.GetInternalType(), (SceNetSocklen_t*)&addrSize );
	if ( res >= 0 )
		out_targetSocket.m_socket = res;
}

AkInt32 GameSocket::Send( const void* in_pBuf, AkInt32 in_length, AkInt32 in_flags ) const
{
	// Ensure that the whole buffer is sent.
	const char* pBuf = (const char*)in_pBuf;

	AkInt32 toSend = in_length;

	while( toSend > 0 )
	{
		AkInt32 sendVal = sceNetSend( m_socket, pBuf, toSend, in_flags );

		// If there's an error or the socket has been closed by peer.
		if ( sendVal < 0 )
			return SOCKET_ERROR;

		if ( sendVal == 0 )
			return 0;

		pBuf += sendVal;
		toSend -= sendVal;
	}

	return in_length - toSend;
}

AkInt32 GameSocket::Recv( void* in_pBuf, AkInt32 in_length, AkInt32 in_flags ) const
{
	int res = sceNetRecv( m_socket, in_pBuf, in_length, SCE_NET_MSG_WAITALL );

	if ( res < 0 )
		return SOCKET_ERROR;

	return res;
}

AkInt32 GameSocket::SendTo( const void* in_pBuf, AkInt32 in_length, AkInt32 in_flags, const GameSocketAddr& in_rAddr ) const
{
	int res = sceNetSendto( m_socket, (const char*)in_pBuf, in_length, in_flags, (SceNetSockaddr*)&in_rAddr.GetInternalType(), sizeof( in_rAddr.GetInternalType() ) );

	if ( res < 0 )
		return SOCKET_ERROR;

	return res;
}

AkInt32 GameSocket::RecvFrom( void* in_pBuf, AkInt32 in_length, AkInt32 in_flags, GameSocketAddr& out_rAddr ) const
{
	int addrSize = sizeof( out_rAddr.GetInternalType() );	
	int res = sceNetRecvfrom( m_socket, (char*)in_pBuf, in_length, in_flags, (SceNetSockaddr*)&out_rAddr.GetInternalType(), (SceNetSocklen_t*)&addrSize );

	if ( res < 0 )
		return SOCKET_ERROR;

	return res;
}



AkInt32 GameSocket::Poll( PollType in_ePollType, AkUInt32 in_timeout ) const
{
	SceNetId epoll = sceNetEpollCreate( "akepoll", 0 );
	if ( epoll < 0 )
	{
		return SOCKET_ERROR;
	}

	SceNetEpollEvent pollEvent = { 0 };
	pollEvent.events = ( in_ePollType == PollRead ) ? SCE_NET_EPOLLIN : SCE_NET_EPOLLOUT;
	// balary TODO pollEvent.data.ext.id = m_socket;

	int res = sceNetEpollControl( epoll, SCE_NET_EPOLL_CTL_ADD, m_socket, &pollEvent );
	if ( res < 0 )
	{
		sceNetEpollDestroy( epoll );
		return SOCKET_ERROR;
	}

	if ( in_timeout != -1 )
		in_timeout *= 1000; // We need microseconds
	SceNetEpollEvent events[1];
	res = sceNetEpollWait( epoll, events, 1, in_timeout );

	sceNetEpollDestroy( epoll );

	if ( res > 0 )
	{
		// balary todo AKASSERT( events->data.ext.id == m_socket );
		AKASSERT( 0 != ( events->events & ( (in_ePollType == PollRead) ? SCE_NET_EPOLLIN : SCE_NET_EPOLLOUT ) ) );

		return 1;
	}
	else if ( res == 0 )
	{
		return 0;
	}

	return SOCKET_ERROR;
}

AkInt32 GameSocket::Shutdown( AkInt32 in_how ) const
{
	int res = sceNetShutdown( m_socket, in_how );

	if ( res < 0 )
		return SOCKET_ERROR;

	return res;
}

AkInt32 GameSocket::Close()
{
	AkInt32 result = sceNetSocketClose( m_socket );
	m_socket = INVALID_SOCKET;

	return result;
}

bool GameSocket::IsValid() const
{
	return m_socket != INVALID_SOCKET;
}

AkUInt16 GameSocket::GetPort() const
{
	SceNetSockaddrIn localAddr = { 0 };
	localAddr.sin_len = sizeof( localAddr );
	SceNetSocklen_t addr_size = sizeof(localAddr);
	int ret = sceNetGetsockname( m_socket, (SceNetSockaddr*)&localAddr, &addr_size );
	if ( ret == 0 )
	{
		return sceNetNtohs( localAddr.sin_port );
	}

	return 0;
}

/*
AkInt32 GameSocket::Select( fd_set* in_readfds, fd_set* in_writefds, fd_set* exceptfds, const timeval* in_timeout )
{
	return ::socketselect( FD_SETSIZE, in_readfds, in_writefds, exceptfds, (timeval*)in_timeout );
}
*/
