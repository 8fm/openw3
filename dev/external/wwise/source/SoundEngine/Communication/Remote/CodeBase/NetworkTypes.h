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

#pragma once

#if defined WINSOCK_VERSION

typedef sockaddr_in SocketAddrType;

#elif defined AK_USE_METRO_API

typedef Windows::Networking::HostName^ SocketAddrType;

struct SOCKET
{
	int type;
	Platform::Object^ socket;
};

#define SOCKET_ERROR -1
#define INADDR_ANY 0

// Socket creation
#define SOCK_DGRAM 0
#define SOCK_STREAM 1
#define IPPROTO_TCP 0
#define IPPROTO_UDP 0

#define SD_BOTH 0

#elif defined (__PPU__) ||  defined (AK_APPLE)  || defined(AK_ANDROID) || defined(AK_LINUX)

#if defined(AK_APPLE) || defined(AK_ANDROID) || defined(AK_LINUX)
#include <unistd.h> 
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/select.h>
#include <sys/time.h>
#include <arpa/inet.h>

typedef sockaddr_in SocketAddrType;

#define SOCKET int
#define INVALID_SOCKET 0xffffffff
#define SOCKET_ERROR -1

#define SD_BOTH SHUT_RDWR

#elif defined AK_WII

#include <revolution/os.h>
#include <revolution/soex.h>

typedef SOSockAddrIn SocketAddrType;

#define SOCKET int
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define INADDR_ANY SO_INADDR_ANY

// Socket creation
#define AF_INET SO_PF_INET
#define SOCK_DGRAM SO_SOCK_DGRAM
#define SOCK_STREAM SO_SOCK_STREAM
#define IPPROTO_TCP 0
#define IPPROTO_UDP 0

// Socket options
#define SOL_SOCKET SO_SOL_SOCKET
#define SO_REUSEADDR SO_SO_REUSEADDR

// Socket shutdown
#define SD_BOTH SO_SHUT_RDWR

#elif defined AK_WIIU

#include <cafe/net/so.h>
typedef sockaddr_in SocketAddrType;
#define SOCKET int
#define INVALID_SOCKET (int)-1

#define SD_BOTH 0

#elif defined AK_VITA

#include <net.h>

namespace AK
{
	namespace Comm
	{
		// Must be defined by the game. Called by comm code
		// to ensure the network connection is available.
		void ForceNetworkConnection();
	}
}

typedef SceNetSockaddrIn SocketAddrType;

#define SOCKET SceNetId
#define INVALID_SOCKET 0xffffffff
#define SOCKET_ERROR -1
#define INADDR_ANY SCE_NET_INADDR_ANY

// Socket creation
#define AF_INET SCE_NET_AF_INET
#define SOCK_DGRAM SCE_NET_SOCK_DGRAM
#define SOCK_STREAM SCE_NET_SOCK_STREAM
#define IPPROTO_TCP SCE_NET_IPPROTO_TCP
#define IPPROTO_UDP SCE_NET_IPPROTO_UDP

// Socket options
#define SOL_SOCKET SCE_NET_SOL_SOCKET
#define SO_REUSEADDR SCE_NET_SO_REUSEADDR

#define SD_BOTH SCE_NET_SHUT_RDWR

#elif defined AK_PS4

#include <net.h>

typedef SceNetSockaddrIn SocketAddrType;

#define SOCKET SceNetId
#define INVALID_SOCKET 0xffffffff
#define SOCKET_ERROR -1
#define INADDR_ANY SCE_NET_INADDR_ANY

// Socket creation
#define AF_INET SCE_NET_AF_INET
#define SOCK_DGRAM SCE_NET_SOCK_DGRAM
#define SOCK_STREAM SCE_NET_SOCK_STREAM
#define IPPROTO_TCP SCE_NET_IPPROTO_TCP
#define IPPROTO_UDP SCE_NET_IPPROTO_UDP

// Socket options
#define SOL_SOCKET SCE_NET_SOL_SOCKET
#define SO_REUSEADDR SCE_NET_SO_REUSEADDR

#define SD_BOTH SCE_NET_SHUT_RDWR

#elif defined AK_3DS

#include <nn/hio.h>
//#include <nn/socket.h>

struct HIOAddr
{
	AkUInt32 DeviceID;
	AkUInt16 SerialChannelID;
};

typedef HIOAddr SocketAddrType; //Would be the Serial Channel.

#define HIO_NUM_DECKS (2)

class HIOAccumulator
{
public:
	HIOAccumulator();
	virtual ~HIOAccumulator();

	bool Init( bool in_bEnableAccumulator );

	int ReadBlocking( void* in_pBuffer, int in_iReadSize, nn::hio::CTR::SerialChannel& in_rHIOSocket );
	bool IsEnabled(){ return m_bEnabled; }

private:

	struct HioDeckItem
	{
		char* pData;
		char* pReadPtr;
		int iDeckSize;
	};

	int UpdateDeck( int in_iMinimalSize, nn::hio::CTR::SerialChannel& in_rHIOSocket );

	HioDeckItem m_Deck[HIO_NUM_DECKS];
	int m_ReadIndex;
	int m_WriteIndex;
	bool m_bEnabled;
};

struct AKHioSocket
{
	nn::hio::CTR::SerialChannel hioSocket;
	void*						pMem;
	AkUInt16					chan;
	HIOAccumulator				accumulator;
	s64							tickLastPoll;
};

#define SOCKET AKHioSocket*
#define INVALID_SOCKET 0 //NULL
#define SOCKET_ERROR -1
#define INADDR_ANY 0

// Socket creation
#define SOCK_DGRAM 0
#define SOCK_STREAM 0
#define IPPROTO_TCP 0
#define IPPROTO_UDP 0

#define SD_BOTH 0

#endif
