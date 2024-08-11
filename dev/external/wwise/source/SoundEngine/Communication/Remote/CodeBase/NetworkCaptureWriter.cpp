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
#include "NetworkCaptureWriter.h"
#include "GameSocketAddr.h"
#include <AK/Tools/Common/AkPlatformFuncs.h>
#include <AK/Tools/Common/AkObject.h>
#include "AkEndianByteSwap.h"
#include "Network.h"
#include <AK/Tools/Common/AkAutoLock.h>

#ifdef AK_PS4
#include "gnm.h"
#endif

#ifndef AK_OPTIMIZED

using namespace AKPLATFORM;

AkUInt32 NetworkCaptureMgr::s_FileIDGenerator =1  ;

AkThread NetworkCaptureMgr::m_Thread;

#define CMD_BITS 24
/*********************************************************
Network protocol:
UID (4 bytes) : A unique ID for the file so both sides know which file is being transferred
Size(4 bytes) : The amount of data being sent for this file.  This is always byte swapped for Windows.

When beginning a new File:
Header (8 bytes)
Name (unlimited, null terminated, ANSI char)

Sending file data:
Header (8 bytes)
Data (unlimited, unformatted)

When closing a file:
Header (8 bytes) with the size of the packet being 0.

Since we want to overwrite the header, simply re-open the file with the new header to overwrite it.

**********************************************************/

NetworkCaptureMgr::NetworkCaptureMgr()
{
	m_uIPAddr = 0;
	m_uPort = 0;
	m_bFinished = false;
	m_PoolID = AK_INVALID_POOL_ID;
}

NetworkCaptureMgr* NetworkCaptureMgr::Instance()
{
	static NetworkCaptureMgr g_Factory;
	return &g_Factory;
}

void NetworkCaptureMgr::SetIP(AkUInt32 in_uIPAddr, AkUInt16 in_uPort)
{
	m_uIPAddr = in_uIPAddr;
	m_uPort = in_uPort;
	OutputDebugMsg("\n");
	OutputDebugMsg("-------------------------------------------------\n");
	OutputDebugMsg("!!! Warning NetworkCaptureWriter is Activated !!!\n");
	OutputDebugMsg("!!!     Connection might block execution      !!!\n");
	OutputDebugMsg("-------------------------------------------------\n");
	OutputDebugMsg("\n");
}

AKRESULT NetworkCaptureMgr::Connect()
{
	GameSocketAddr remoteAddr( m_uIPAddr, m_uPort );

	if( !m_socket.Create( SOCK_STREAM, IPPROTO_TCP ) )
		return AK_Fail;

	if( m_socket.Connect( remoteAddr ) < 0 )
	{
		m_socket.Close();
		return AK_Fail;
	}

	m_bFinished = false;

	//Allocate memory for background transfer. 
	const AkUInt32 uPoolSize = 32*1024;
	m_PoolID = AK::MemoryMgr::CreatePool(NULL, uPoolSize, uPoolSize, AkMalloc|AkFixedSizeBlocksMode);
	AK::MemoryMgr::SetPoolName(m_PoolID, AKTEXT("GameSim Comm"));

	AK::MemoryMgr::PoolStats stats;
	AK::MemoryMgr::GetPoolStats(m_PoolID, stats);
	m_Buffer.Init(m_PoolID, stats.uReserved);

	AkThreadProperties threadProps;
	AkGetDefaultThreadProperties(threadProps);

	AkCreateEvent(m_DrainEvent);
	AkCreateEvent(m_WriteEvent);
	AkCreateEvent(m_IOEvent);

	AkCreateThread(TransferThreadFunc, NULL, threadProps, &m_Thread, "GameSim Comm");

	return AK_Success;
}

void NetworkCaptureMgr::Term()
{
	if (m_PoolID == AK_INVALID_POOL_ID)
		return;	//Not initialized at all.

	SendFinished();

	if( AkIsValidThread( &m_Thread ) )
	{
		AkWaitForSingleThread( &m_Thread );
		AkCloseThread( &m_Thread );
		AkClearThread( &m_Thread );
	}

	Disconnect();

	m_Buffer.Term(m_PoolID);
	AK::MemoryMgr::DestroyPool(m_PoolID);
	m_PoolID = AK_INVALID_POOL_ID;

	AkDestroyEvent(m_DrainEvent);
	AkDestroyEvent(m_WriteEvent);
	AkDestroyEvent(m_IOEvent);
}

AKRESULT NetworkCaptureMgr::Disconnect()
{
	m_Lock.Lock();

	m_bFinished = true;
	//Linger for a bit to let the socket finish the transfers.  There is a linger option on sockets, but not all platform have it.
	AKPLATFORM::AkSleep(100);	
	m_socket.Shutdown( SD_BOTH );
	m_socket.Close();

	m_Lock.Unlock();

	return AK_Success;
}

AKRESULT NetworkCaptureMgr::WaitForMemory()
{
	if (m_Buffer.IsEmpty())
	{
		return AK_Fail;
	}
#ifdef AK_PS4
	// On PS4 this needs to be called regularly.
	AK_RELEASE_GPU_OFFLINE_FRAME
#endif
	AkWaitForEvent(m_DrainEvent);
	return AK_Success;
}

NetworkCaptureMgr::Packet* NetworkCaptureMgr::CreatePacketHeader(FileCommand in_cmd, AkUInt32 in_UID, AkUInt32 in_size, bool in_bAlloc)
{
	Packet* pPacket = NULL;	
	while(pPacket == NULL && !m_bFinished)
	{
		if (in_bAlloc)
			pPacket = (Packet*)m_Buffer.BeginWrite(sizeof(Packet) + in_size);
		else
			pPacket = (Packet*)m_Buffer.BeginWrite(sizeof(Packet));

		if (pPacket == NULL && !m_bFinished)
		{
			//Not enough space.  Wait for other transfers to finish
			//Can't use m_DrainEvent because multiple threads could wait on it.  This is not supported on the Wii.
			AKPLATFORM::AkSleep(100);
		}
	}

	if (m_bFinished)
	{
		if (pPacket != NULL)
			m_Buffer.EndWrite(pPacket, sizeof(Packet));	//Unlock the buffer.  This packet won't be sent.
		return NULL;
	}

	if (in_bAlloc)
		pPacket->pData = (char*)pPacket + sizeof(Packet);
	else
		pPacket->pData = NULL;

	pPacket->cmdAndID = in_cmd << CMD_BITS | in_UID;
	pPacket->size = in_size;
	pPacket->bAllocated = in_bAlloc;

	return pPacket;
}

AKRESULT NetworkCaptureMgr::BeginTransaction( AkUInt32 in_UID, const AkOSChar* in_CaptureFileName, bool in_bOverwrite )
{	
	if (!IsConnected())
		return AK_Fail;

	//Transmit the file name as ANSI string.
	char* pszFile;
	CONVERT_OSCHAR_TO_CHAR(in_CaptureFileName, pszFile);	

	size_t len = strlen(pszFile)+1;

	Packet *pPacket = CreatePacketHeader(in_bOverwrite ? CmdOpenOverwrite : CmdOpenWrite, in_UID, (AkUInt32)(len), true);
	if (!pPacket)
		return AK_Fail;

	strcpy((char*)pPacket->pData, pszFile);

	SignalWrite(pPacket, true);	

	return m_bFinished ? AK_Fail : AK_Success;
}


AKRESULT NetworkCaptureMgr::SendData(AkUInt32 in_UID, void* in_pData, AkUInt32 in_size)
{
	if (!IsConnected())
		return AK_Fail;

	Packet *pPacket = CreatePacketHeader(CmdWrite, in_UID, in_size);
	if (!pPacket)
		return AK_Fail;

	pPacket->pData = in_pData;
	SignalWrite(pPacket);

	//Block until the packet is written.  This is only called in the IO thread.
	AkWaitForEvent(m_IOEvent);

	return m_bFinished ? AK_Fail : AK_Success;
}

AKRESULT NetworkCaptureMgr::EndTransaction( AkUInt32 in_UID )
{
	if (!IsConnected())
		return AK_Fail;

	Packet *pPacket = CreatePacketHeader(CmdClose, in_UID, 0);
	if (!pPacket)
		return AK_Fail;

	SignalWrite(pPacket);	

	return m_bFinished ? AK_Fail : AK_Success;
}

AKRESULT NetworkCaptureMgr::Seek( AkUInt32 in_UID, AkUInt32 in_pos )
{
	if (!IsConnected())
		return AK_Fail;

	Packet *pPacket = CreatePacketHeader(CmdSeek, in_UID, 4, true);
	if (!pPacket)
		return AK_Fail;

	if (Network::SameEndianAsNetwork())
		*(AkUInt32*)pPacket->pData = AK::EndianByteSwap::DWordSwap(in_pos);

	SignalWrite(pPacket);

	//Block until the packet is written.  This is only called in the IO thread.
	AkWaitForEvent(m_IOEvent);
	return m_bFinished ? AK_Fail : AK_Success;
}

AKRESULT NetworkCaptureMgr::SendKeepAlive()
{
	if (!IsConnected())
		return AK_Fail;

	Packet *pPacket = CreatePacketHeader(CmdKeepAlive, 0, 0);
	if (!pPacket)
		return AK_Fail;

	SignalWrite(pPacket);

	return AK_Success;
}

AKRESULT NetworkCaptureMgr::SendStdOut(const char* in_szMsg)
{
	if (!IsConnected())
		return AK_Fail;

	size_t len = strlen(in_szMsg);
	Packet *pPacket = CreatePacketHeader(CmdStdOut, 0, (AkUInt32)len, true);
	if (!pPacket)
		return AK_Fail;

	strcpy((char*)pPacket->pData, in_szMsg);
	SignalWrite(pPacket);
	
	return AK_Success;
}

AKRESULT NetworkCaptureMgr::SendFinished()
{
	if (!IsConnected())
		return AK_Fail;

	Packet *pPacket = CreatePacketHeader(CmdFinished, 0, 0);

	//Make sure the thread will wake up to exit cleanly.
	m_bFinished = true;
	if (pPacket)
		SignalWrite(pPacket);
	else
		AkSignalEvent(m_WriteEvent);
	
	return AK_Success;
}

AKRESULT NetworkCaptureMgr::SendBankOp()
{
	if (!IsConnected())
		return AK_Fail;

	Packet *pPacket = CreatePacketHeader(CmdBankOp, 0, 0);
	if (!pPacket)
		return AK_Fail;

	SignalWrite(pPacket);

	return AK_Success;
}

AK_DECLARE_THREAD_ROUTINE( NetworkCaptureMgr::TransferThreadFunc )
{
	NetworkCaptureMgr::Instance()->TransferThread();
	AkExitThread(0);
}

void NetworkCaptureMgr::TransferThread()
{
	while ((!m_bFinished || !m_Buffer.IsEmpty()) && IsConnected())
	{
		if (m_Buffer.IsEmpty())
		{
			AkSignalEvent(m_DrainEvent);
			AkWaitForEvent(m_WriteEvent);
			continue;	//Re-validate the end conditions.
		}

		Packet * pItem = (Packet*) m_Buffer.BeginRead();
		
		AkUInt32 size = pItem->size;
		AkUInt32 cmdID = pItem->cmdAndID;
		if (Network::SameEndianAsNetwork())
		{
			size = AK::EndianByteSwap::DWordSwap(size);
			cmdID = AK::EndianByteSwap::DWordSwap(cmdID);
		}

		if (m_socket.Send(&cmdID, 4, 0) != 4 ||
			m_socket.Send(&size, 4, 0) != 4)
			break;	//Lost connection;

		//Send the data itself
		AkUInt32 remaining = pItem->size;
		char *pData = (char*)pItem->pData;		
		while(remaining > 0)
		{
			AkInt32 sent = m_socket.Send(pData, remaining, 0);
			if (sent <= 0)
				break;

			remaining -= sent;
			pData += sent;
		}
		
		if (remaining > 0)
			break;	//Comm error, disconnected.

		m_Buffer.EndRead( pItem, sizeof(Packet) + (pItem->bAllocated ? pItem->size : 0) );
		AkSignalEvent(m_DrainEvent);

		AkUInt32 cmd = pItem->cmdAndID >> CMD_BITS;
		if (cmd == CmdWrite ||			
			cmd == CmdSeek)
		{
			AkSignalEvent(m_IOEvent);
		}
	}
	
	Disconnect();

	//Wakeup the sleeping threads, if any.  All IO operations are terminated.
	AkSignalEvent(m_IOEvent);
	AkSignalEvent(m_DrainEvent);
}

void NetworkCaptureMgr::SignalWrite( Packet * in_pPacket, bool )
{
	m_Buffer.EndWrite(in_pPacket, sizeof(Packet) + (in_pPacket->bAllocated ? in_pPacket->size : 0));
	AkSignalEvent(m_WriteEvent);
}
#endif
