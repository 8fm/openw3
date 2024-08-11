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

#include "GameSocket.h"

#include <AK/Tools/Common/AkPlatformFuncs.h>
#include <AK/Tools/Common/AkLock.h>

#include "Serializer.h"

class IncomingChannel
{
public:
	IncomingChannel( unsigned short in_listenPort, AkMemPoolId in_pool, const AkThreadProperties& in_threadProperties );
	virtual ~IncomingChannel();

	virtual bool Init();
	virtual void Term();
	void Reset();

	bool StartListening();
	void StopListening();

	void Process();

	bool IsReady() const;

	AkUInt16 GetPort() const;

protected:
	void Send( const AkUInt8* in_pData, int in_dataLength );

	virtual bool ProcessCommand( AkUInt8* in_pData, AkUInt32 in_uDataLength ) = 0;
	virtual void PeerConnected( const GameSocketAddr& in_rControllerAddr ) = 0;
	virtual void PeerDisconnected() = 0;
	virtual const char* GetRequestedPortName() = 0;
	virtual bool RequiresAccumulator(){ return false; }

private:
	void ReceiveCommand();

	unsigned short m_requestedPort;

	AkMemPoolId m_pool;

	GameSocket m_serverSocket;
	GameSocket m_connSocket;

	volatile bool m_bErrorProcessingConnection;

	Serializer m_serializer;
	AkUInt8* m_pRecvBuf;
	AkUInt32 m_iBufSize;

#ifdef AK_3DS // Polling for incoming data does not work in HIO
	static AK_DECLARE_THREAD_ROUTINE( ProcessConnectionThreadFunc );
	AkThread *m_pThread;
	CAkLock m_csReset;
	AkThreadProperties m_threadProperties;
#endif
};
