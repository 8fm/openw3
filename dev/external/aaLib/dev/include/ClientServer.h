
#pragma once

//////////////////////////////////////////////////////////////////////////

#define WIN32_MEAN_AND_LEAN
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>

#include "Packet.h"

// Minimum winsock version required
#define REQ_WINSOCK_VER		2
#define DEFAULT_PORT		47000
#define	DEFAULT_IP			"127.0.0.1"
#define DEFAULT_HOST		"localhost"

// IP number typedef for IPv4
typedef unsigned long aaIPNumber;

// include Ws2_32 library
#pragma comment (lib, "Ws2_32.lib")

//////////////////////////////////////////////////////////////////////////

class aaClient
{
	SOCKET		m_socket;
	aaInt32		m_lastError;
	WSADATA		m_wsaData;
	fd_set		m_fds;

public:
	aaClient();

	bool Initialize( const char* ip = DEFAULT_HOST, aaInt32 port = DEFAULT_PORT );
	bool Shutdown();

	// Send data with specified size (in bytes), returns true if send was successful
	// ( number of bytes sent == number of bytes requested to send ), if return value
	// is false, call GetLastError() to see what the error was
	bool Send( void* data, aaInt32 size );
	bool Read( void* data, aaInt32 size );

	aaInt32	GetLastError();

private:
	bool FillSockAddr( sockaddr_in *sockAddr, const char* serverName, aaInt32 port );
	bool aaClient::FindHostIP( const char* serverName, aaIPNumber* ip );
};

//////////////////////////////////////////////////////////////////////////

class aaServer
{
	WSADATA		m_wsaData;
	SOCKET		m_listenSocket;
	SOCKET		m_clientSocket;
	aaInt32		m_lastError;
	bool		m_asyncMode;

public:
	aaServer();

	bool Initialize( aaInt32 port = DEFAULT_PORT, bool asyncMode = false );
	bool Shutdown();

	// Send data with specified size (in bytes), returns true if send was successful
	// ( number of bytes sent == number of bytes requested to send ), if return value
	// is false, call GetLastError() to see what the error was
	bool Read( void* data, aaInt32 size );
	bool Send( void* data, aaInt32 size );

	aaInt32	GetLastError();

public: // async mode
	bool SetAsyncMode( bool flag );
	bool AsyncAccept();
	bool HasPendingData() const;

private:
	void SetServerSockAddr( sockaddr_in *pSockAddr, aaInt32 port );
};
