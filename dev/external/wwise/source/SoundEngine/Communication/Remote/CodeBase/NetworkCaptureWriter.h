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

#ifndef AK_OPTIMIZED

#include <AK/SoundEngine/Common/AkTypes.h>
#include "GameSocket.h"
#include <AK/Tools/Common/AkLock.h>
#include <AK/Tools/Common/AkPlatformFuncs.h>
#include "AkChunkRing.h"

#define NETWORK_CAPTURE_MAX_STRING_LENGTH 256
#define IPLENGTH 24

//This class is the Write factory and the manager of the connection.
class NetworkCaptureMgr
{
public:
	NetworkCaptureMgr();
	static AK_FUNC( NetworkCaptureMgr*, Instance )();
	void SetIP(AkUInt32 in_uIPAddr, AkUInt16 in_uPort);
	bool IsInitialized() {return m_uIPAddr != 0;}
	bool IsConnected() { return m_PoolID != AK_INVALID_POOL_ID; }

	static AK_FUNC( AkUInt32, GenerateFileID )() {return ++s_FileIDGenerator;}

	AKRESULT Connect();
	void	 Term();

	AKRESULT BeginTransaction( AkUInt32 in_UID, const AkOSChar* in_CaptureFileName, bool in_bOverwrite );
	AKRESULT EndTransaction( AkUInt32 in_UID );
	AKRESULT SendData(AkUInt32 in_UID, void* in_pData, AkUInt32 in_size);
	AKRESULT Seek(AkUInt32 in_UID, AkUInt32 in_pos);
	AKRESULT SendKeepAlive();
	AKRESULT SendStdOut(const char* in_szMsg);
	AKRESULT SendFinished();
	AKRESULT SendBankOp();

	AKRESULT WaitForMemory();

private:

	enum FileCommand {CmdWrite = 0, CmdOpenWrite = 1, CmdOpenOverwrite = 2, CmdClose = 3, CmdSeek = 4, CmdStdOut = 5, CmdKeepAlive = 6, CmdFinished = 7, CmdBankOp = 8};
	struct Packet
	{
		AkUInt32 cmdAndID;
		AkUInt32 size;
		void* pData;
		bool bAllocated;
	};

	AKRESULT Disconnect();
	NetworkCaptureMgr::Packet* CreatePacketHeader(FileCommand in_cmd, AkUInt32 in_UID, AkUInt32 in_size, bool in_bAlloc = false);
	void SignalWrite( Packet * in_pPacket, bool in_bIncludeDataSize = false);

	static AK_DECLARE_THREAD_ROUTINE(TransferThreadFunc);
	void TransferThread();

	GameSocket m_socket;
	AkUInt32 m_uIPAddr;
	AkUInt16 m_uPort;
	CAkLock m_Lock;
	AkChunkRing m_Buffer;
	AkMemPoolId m_PoolID;
	AkEvent m_DrainEvent;
	AkEvent m_WriteEvent;
	AkEvent m_IOEvent;
	static AkThread m_Thread;
	bool m_bFinished;
	
	static AkUInt32 s_FileIDGenerator;
};

#endif
