// This code contains NVIDIA Confidential Information and is disclosed to you
// under a form of NVIDIA software license agreement provided separately to you.
//
// Notice
// NVIDIA Corporation and its licensors retain all intellectual property and
// proprietary rights in and to this software and related documentation and
// any modifications thereto. Any use, reproduction, disclosure, or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA Corporation is strictly prohibited.
//
// ALL NVIDIA DESIGN SPECIFICATIONS, CODE ARE PROVIDED "AS IS.". NVIDIA MAKES
// NO WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ALL IMPLIED WARRANTIES OF NONINFRINGEMENT,
// MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Information and code furnished is believed to be accurate and reliable.
// However, NVIDIA Corporation assumes no responsibility for the consequences of use of such
// information or for any infringement of patents or other rights of third parties that may
// result from its use. No license is granted by implication or otherwise under any patent
// or patent rights of NVIDIA Corporation. Details are subject to change without notice.
// This code supersedes and replaces all information previously supplied.
// NVIDIA Corporation products are not authorized for use as critical
// components in life support devices or systems without express written approval of
// NVIDIA Corporation.
//
// Copyright (c) 2008-2013 NVIDIA Corporation. All rights reserved.

#include "PvdServer.h"
#include "PvdFoundation.h"
#include <Winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include "PsMutex.h"
#include "PvdNetworkStreams.h"
#include "PvdRandomAccessIOStream.h"
#include "PvdCommStreamParsing.h"
#include "PvdClient.h"
#include "PvdNetworkToByteStreams.h"
#include "PvdInternalByteStreams.h"
#include "PvdObjectModelEventStream.h"
#include "PvdCommStreamEvents.h"
#include "PvdConnection.h"

#pragma comment( lib, "Ws2_32" )

using namespace physx::shdfnd;
using namespace physx::debugger;
using namespace physx::debugger::comm;


namespace {
		// struct for naming a thread in the debugger
#pragma pack(push, 8)

	typedef struct tagTHREADNAME_INFO
	{
	   DWORD dwType;		// Must be 0x1000.
	   LPCSTR szName;		// Pointer to name (in user addr space).
	   DWORD dwThreadID;	// Thread ID (-1=caller thread).
	   DWORD dwFlags;		// Reserved for future use, must be zero.
	} THREADNAME_INFO;

#pragma pack(pop)


class SimpleThread
{
	HANDLE				thread;
	DWORD				threadId;
	const char*			mName;
public:
	SimpleThread( const char* inName ) : thread( 0 ), threadId( 0 ), mName( inName ) {}

	virtual ~SimpleThread()
	{
		waitForQuit();
	}

	void waitForQuit()
	{
		if ( thread )
			WaitForSingleObject(thread,INFINITE);
		thread = 0;
	}

	void start( PxU32 stackSize )
	{
		thread = CreateThread(NULL, stackSize, threadStart, (LPVOID) this, 0, &threadId);
		PX_ASSERT( thread );
	}

	virtual void run() = 0;

private:
	static DWORD WINAPI threadStart( void* inArg )
	{
		SimpleThread* theThread = reinterpret_cast<SimpleThread*>( inArg );
		//Set the thread name.
		if ( theThread->mName && *theThread->mName )
		{
			THREADNAME_INFO info;
			info.dwType		= 0x1000;
			info.szName		= theThread->mName;
			info.dwThreadID	= GetCurrentThreadId();
			info.dwFlags	= 0;

			// C++ Exceptions are may be disabled for this project, but SEH is not (and cannot be)
			// http://stackoverflow.com/questions/943087/what-exactly-will-happen-if-i-disable-c-exceptions-in-a-project
			__try
			{
				#define PS_MS_VC_EXCEPTION 0x406D1388
				RaiseException( PS_MS_VC_EXCEPTION, 0, sizeof(info)/sizeof(ULONG_PTR), (ULONG_PTR*)&info );
			}
			__except(EXCEPTION_EXECUTE_HANDLER)
			{
				// this runs if not attached to a debugger (thus not really naming the thread)
			}
		}
		theThread->doRun();
		return 0;
	}

	void doRun()
	{
		run();
		ExitThread(0);
	}
};


inline void safeCloseSocket( SOCKET& inSocket )
{
	if ( inSocket != INVALID_SOCKET )
		closesocket(inSocket);
	inSocket = INVALID_SOCKET;
}

inline void safeShutdownSocket( SOCKET& inSocket )
{
	if ( inSocket != INVALID_SOCKET )
	{
		//You have to shutdown the socket before closing it
		//else receivers may block for an indeterminate amount of time
		//waiting for the connection to timeout.
		shutdown( inSocket, SD_BOTH );
		safeCloseSocket( inSocket );
	}
	inSocket = INVALID_SOCKET;
}
typedef MutexT<ForwardingAllocator>				TMutexType;
typedef MutexT<ForwardingAllocator>::ScopedLock TLockType;


class SocketLayerInitializer
{
protected:
	PxAllocatorCallback&			mAllocator;
	bool							mShutdownLayer;
	PxU32							mRefCount;
	TMutexType						mMutex;
public:
	SocketLayerInitializer(PxAllocatorCallback& alloc )
		: mAllocator( alloc )
		, mRefCount( 0 )
		, mMutex( ForwardingAllocator( alloc, "SocketLayerInitializer::mMutex" ) )
	{
		WORD vreq;
		WSADATA wsaData;
		vreq = MAKEWORD(2,2);
		mShutdownLayer = (WSAStartup(vreq, &wsaData) == 0);
	}

	virtual ~SocketLayerInitializer()
	{
		if ( mShutdownLayer )
			WSACleanup();
		mShutdownLayer = false;
	}

	void addRef() { TLockType __lock( mMutex ); ++mRefCount; }
	void release()
	{
		PxU32 count = 0;
		{
			TLockType __lock( mMutex );
			if ( mRefCount ) --mRefCount;
			count = mRefCount;
		}
		if ( !count ) PVD_DELETE(mAllocator, this );
	}
};


typedef	PvdRefPtr<SocketLayerInitializer> SocketLayerInitializerPtr;


class SocketDBStreams : public PvdNetworkInStream, public PvdNetworkOutStream
{
	PxAllocatorCallback&		mAllocator;
	TMutexType					mReadMutex;
	TMutexType					mWriteMutex;
	TMutexType					mWStatisticMutex;
	TMutexType					mRStatisticMutex;
	PxU32						mRefCount;
	SOCKET						mSocket;
	PvdRandomAccessIOStream*	mFileStream;
	SocketLayerInitializerPtr	mSocketLayer;
	PxU64						mWrittenData;
	PxU64						mReadData;

public:
	SocketDBStreams( PxAllocatorCallback& alloc, SOCKET inSocket, PvdRandomAccessIOStream* inFileStream, SocketLayerInitializer* socketLayer )
		: mAllocator( alloc )
		, mReadMutex( ForwardingAllocator( alloc, "SocketDBStreams::mReadMutex" ) )
		, mWriteMutex( ForwardingAllocator( alloc, "SocketDBStreams::mWriteMutex" ) )
		, mWStatisticMutex( ForwardingAllocator( alloc, "SocketDBStreams::mWStatisticMutex" ) )
		, mRStatisticMutex( ForwardingAllocator( alloc, "SocketDBStreams::mRStatisticMutex" ) )
		, mSocket( inSocket )
		, mFileStream( inFileStream )
		, mSocketLayer( socketLayer )
		, mRefCount( 0 )
		, mWrittenData( 0 )
		, mReadData( 0 )
	{
		if ( mFileStream ) mFileStream->addRef();
	}

	virtual ~SocketDBStreams()
	{
		safeShutdownSocket( mSocket );
		if ( mFileStream ) mFileStream->release();
		mFileStream = NULL;
	}

	bool isConnected() const { return mSocket != INVALID_SOCKET; }

	void disconnect() { TLockType __lock( mWriteMutex ); safeShutdownSocket( mSocket ); }

	virtual PxU64 getWrittenDataSize()
	{
		TLockType __lock( mWStatisticMutex );
		return mWrittenData;
	}

	virtual PxU64 getLoadedDataSize()
	{
		TLockType __lock( mRStatisticMutex );
		return mReadData;
	}

	void addRef() { ++mRefCount; }
	void release()
	{
		PxU32 count = 0;
		{
			TLockType __lock( mWriteMutex );
			if ( mRefCount ) --mRefCount;
			count = mRefCount;
		}
		if ( count == 0 ) PVD_DELETE( mAllocator, this );
	}

	PvdError readBytes( PxU8* inBytes, PxU32 inLength )
	{
		if ( inLength )
		{
			int totalLen = 0;
			while( inLength )
			{
				int errorCode = 0;
				int len = 0;

				{
					TLockType __lock( mReadMutex );
					len = recv( mSocket, reinterpret_cast<char*>( inBytes ), static_cast<int>( inLength ), 0 );
					errorCode = WSAGetLastError();

				}

				if ( len <= 0 || errorCode!= 0 )
				{
					//Zero out the buffer if we fail to read the next packet.  This
					//keeps downstream users from suffering from a bad issue where
					//they get back uninitialized memory but don't realize the socket
					//was closed.  We always initialize all the memory
					memset( inBytes, 0, inLength );
					safeShutdownSocket(mSocket);
					return PvdErrorType::NetworkError;
				}

				if ( mFileStream ) mFileStream->write( inBytes, len );
				inBytes += len;
				inLength -= len;
				totalLen += len;
			}

			{
				TLockType __lock( mRStatisticMutex );
				mReadData += totalLen;
			}
		}
		return PvdErrorType::Success;
	}

	PvdError write( const PxU8* outBytes, PxU32 inLength )
	{
		if ( inLength && outBytes )
		{
			int totalLen = 0;

			while( inLength )
			{
				int len = 0;
				{
					TLockType __lock( mWriteMutex );
					len = send( mSocket, reinterpret_cast<const char*>( outBytes ), static_cast<int>( inLength ), 0 );
				}
				if ( len == SOCKET_ERROR )
				{
					safeShutdownSocket(mSocket);
					return PvdErrorType::NetworkError;
				}

				outBytes += len;
				totalLen += len;
				inLength -= len;
			}

			{
				TLockType __lock( mWStatisticMutex );
				mWrittenData += totalLen;
			}
		}
		return PvdErrorType::Success;
	}

	PvdError flush()
	{
		if ( mFileStream ) mFileStream->asyncFlush();
		return PvdErrorType::Success;
	}
};

class PvdServerImpl : public PvdServer, public SimpleThread
{
	PxAllocatorCallback&			mAllocator;
	SocketLayerInitializerPtr		mSocketLayer;
	SOCKET							mSocket;
	PxU16							mPort;
	int								mMaxConnections;
	PvdServerHandler&				mHandler;

public:
	PvdServerImpl( PxAllocatorCallback& alloc, PvdServerHandler& inHandler )
		: SimpleThread( "AsyncServer" )
		, mAllocator( alloc )
		, mSocketLayer( PVD_NEW( alloc, SocketLayerInitializer )(alloc) )
		, mSocket( INVALID_SOCKET )
		, mPort( 0 )
		, mMaxConnections( SOMAXCONN )
		, mHandler( inHandler )
	{
	}

	virtual ~PvdServerImpl()
	{
		safeCloseSocket( mSocket );
		mHandler.release();
	}

	virtual PxU16 getPort() const { return mPort; }
	//Port defaults to 5425
	virtual int setPort( PxU16 inPort )
	{
		if ( inPort != mPort )
		{
			mPort = inPort;
			safeCloseSocket( mSocket );
			waitForQuit();
			return setupAcceptThread();
		}
		return 0;
	}

	virtual int getMaxConnections() const { return mMaxConnections; }
	virtual int setMaxConnections(int inMaxConnections)
	{
		if ( mMaxConnections == inMaxConnections )
			return 0;

		mMaxConnections = inMaxConnections;
		safeCloseSocket( mSocket );
		waitForQuit();
		return setupAcceptThread();
	}

	virtual void release()
	{
		PVD_DELETE( mAllocator, this );
	}

	//virtual PxU64 getReadDataSize()
	//{}

	int setupAcceptThread()
	{
		if( mSocket != INVALID_SOCKET )
			return 0;

		mSocket = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
		if ( mSocket == INVALID_SOCKET )
		{
			PX_ASSERT( false );
			int retval = WSAGetLastError();
			return retval;
		}

		//----------------------
		// The sockaddr_in structure specifies the address family,
		// IP address, and port for the socket that is being bound.
		sockaddr_in service;
		service.sin_family = AF_INET;
		service.sin_addr.s_addr = INADDR_ANY;
		service.sin_port = htons(mPort);
		int error = bind( mSocket, (sockaddr*)&service, sizeof(service) );
		if ( error == SOCKET_ERROR )
		{
			PX_ASSERT( false );
			int retval = WSAGetLastError();
			return retval;
		}
		error = listen( mSocket, mMaxConnections );
		if ( error != 0 )
		{
			PX_ASSERT( false );
			int retval = WSAGetLastError();
			return retval;
		}
		start( 0x4000 );
		return 0;
	}

	virtual void run()
	{
		sockaddr_in remoteAddr;
		SOCKET newConnection;

		while( true )
		{
			int addrLen = sizeof( remoteAddr );
			newConnection = accept( mSocket, (sockaddr*)&remoteAddr, &addrLen );
			if ( newConnection == INVALID_SOCKET )
				return;

			sockaddr_in peerName;
			getpeername( newConnection, (sockaddr*)&peerName, &addrLen );
			char theNameBuffer[1024] = {0};
			{
				char theServBuffer[1024] = {0};
				int theResult = getnameinfo((sockaddr*) &peerName
										, sizeof (peerName)
										, theNameBuffer
										, 1024
										, theServBuffer
										, 1024
										, NI_NUMERICSERV );
				strcat_s( theNameBuffer, ":" );
				strcat_s( theNameBuffer, theServBuffer );
			}
			PvdRandomAccessIOStream* theStream = NULL;
			{
				char theFilename[1024];
				if ( mHandler.getDebugStreamFilename( theFilename, 1024, theNameBuffer ) )
				{
					//Ensure the filename is null terminated.
					theFilename[1023] = 0;
					try
					{
						ErrorCodeOption<PvdRandomAccessIOStream*> strCode = PvdRandomAccessIOStream::open( mAllocator, theFilename, true, false );
						if ( strCode.hasValue() )
							theStream = strCode.getValue();
					}
					catch( ... )
					{
					}
				}
			}

			//Sockets appear to be blocking by default.
			SocketDBStreams* theNewStream = PVD_NEW(mAllocator, SocketDBStreams)( mAllocator, newConnection, theStream, mSocketLayer );
			PxU32 theValue;
			theNewStream->readBytes( reinterpret_cast<PxU8*>( &theValue ), sizeof( PxU32 ) );
			theNewStream->addRef();
			theNewStream->addRef();
			mHandler.onNewConnection( theNameBuffer, theValue, *theNewStream, *theNewStream );
		}
	}
};

class PvdAsyncLoaderImpl : public PvdAsyncLoader, public SimpleThread
{
	PxAllocatorCallback&		mAllocator;
	PvdNetworkToInStream&		mInStream;
	PvdNetworkToOutStream		mOutStream;
	PvdClientMutator&			mMutator;
	PvdCommStreamEventReader&	mReader;
	PvdCommStreamParser&		mParser;
	PvdDisconnectionListener*	mDisconnectListener;
	mutable TMutexType			mMutex;
	char						mRemoteName[2056];
	PvdLoadState::Enum			mLoadState;
	bool						mIsLoading;
	HANDLE						mLoadStateEvent;
	MemPvdOutputStream			mEventStream;

public:
	PvdAsyncLoaderImpl( PxAllocatorCallback& cback, const char* remoteName, PxU32 firstWord, PvdNetworkToInStream& inStream, PvdNetworkOutStream& outStream, PvdRandomAccessIOStream& ioStream, PvdCommStreamEventReader& inReader )
		: SimpleThread( "PvdAsyncLoader" )
		, mAllocator( cback )
		, mInStream( inStream )
		, mOutStream( mAllocator, outStream )
		, mMutator( PvdClientMutator::create( cback, ioStream ) )
		, mReader( inReader )
		, mParser( mMutator.createParser( inReader ) )
		, mDisconnectListener( NULL )
		, mMutex( ForwardingAllocator( cback, "PvdAsyncLoaderImpl::mMutex" ) )
		, mLoadState( PvdLoadState::Loading )
		, mIsLoading( true )
		, mLoadStateEvent( CreateEvent( NULL, false, false, "" ) )
		, mEventStream( cback, "PvdAsyncLoaderImpl::mEventStream" )
	{
		mMutator.addRef();
		strcpy_s( mRemoteName, remoteName );
		start( 0x4000 );
	}

	virtual ~PvdAsyncLoaderImpl()
	{
		stopLoading();
		mReader.release();
		mParser.release();
		mMutator.release();
		mInStream.release();
		//Do *not* release out stream.  It gets released when
		//this object gets released.
		//mOutStream.release();
		CloseHandle( mLoadStateEvent );
	}

	virtual const char* getRemoteConnectionName() const { return mRemoteName; }

	virtual void setDisconnectionListener( PvdDisconnectionListener& inListener )
	{
		mDisconnectListener = &inListener;
		if ( mIsLoading == false )
			mDisconnectListener->onLoaderDisconnected( *this );
	}

	//DB access *must* go through the locks, the loader is continuously pulling from the
	//socket and converting data into the database as fast as it can.
	virtual void lock() { mMutex.lock(); }
	//Get the primary database we are writing information to.
	virtual PvdClientMutator& getClient() { return mMutator; }
	//DB access *must* go through the locks
	virtual void unlock() { mMutex.unlock(); }

	//Returns true if this object is still receiving information.
	virtual bool isLoading() const { return mIsLoading && mInStream.mStream.isConnected(); }

	//Allow us to pause the ongoing simulation by setting the loader
	//load state to paused.
	virtual PvdLoadState::Enum getLoadState() const { TLockType __lock( mMutex ); return mLoadState; }

	virtual void stopLoading()
	{
		if ( mIsLoading )
		{
			mInStream.mStream.disconnect();
			mIsLoading = false;
			mLoadState = PvdLoadState::Loading;
			SetEvent( mLoadStateEvent );
			//This makes a *huge* difference in stability.
			waitForQuit();
		}
	}

	//Allow us to pause the ongoing simulation by setting the loader
	//load state to paused.  This doesn't affect the isLoading flag.
	//Don't support this as of yet.
	virtual void setLoadState( PvdLoadState::Enum val )
	{
		TLockType __lock( mMutex );
		mLoadState = val;
		//TODO--send value up the pipe to the sdk.
		if ( mIsLoading )
		{
			PxU32 evtType = PvdSdkEventTypes::SetPauseState;
			PxU32 pauseState = val == PvdLoadState::Loading ? PvdConnectionState::RecordingOrViewing : PvdConnectionState::Paused;
			if ( mReader.isSwappingBytes() )
			{
				swapBytes( evtType );
				swapBytes( pauseState );
			}
			mOutStream << evtType;
			mOutStream << pauseState;
		}
		SetEvent( mLoadStateEvent );
	}

	bool readAndParseNextEventGroup()
	{
		bool retval;
		//You must not lock this top level read and the thread sits on the socket
		//(in an ideal world) and you would block access time to the db.
		retval = mReader.readNextEventGroup( mInStream, mEventStream );
		if ( retval )
		{
			TLockType __lock( mMutex );
			retval = mParser.parseNextEventGroup( mEventStream.toRef() );
		}
		mEventStream.clear();
		return retval;
	}

	virtual void run()
	{
		try
		{
			bool ret = true;
			while(ret && isLoading() )
			{
				mOutStream.mStream.flush();
				while( mIsLoading && mLoadState == PvdLoadState::Paused ) WaitForSingleObject( mLoadStateEvent, INFINITE );
				ret = readAndParseNextEventGroup();
			}
			//Read rest of data out of the stream.
			//Empty control statement desired.
			while( ret && readAndParseNextEventGroup() );
		}
		catch( ... )
		{
		}
		mIsLoading = false;
		{
			TLockType __lock( mMutex );
			PvdObjectModelEventStreamFactory* factory = mMutator.getEventStreamFactory();
			if ( factory != NULL )
				factory->flush();
		}
		if ( mDisconnectListener ) mDisconnectListener->onLoaderDisconnected( *this );
	}

	virtual void release()
	{
		PVD_DELETE( mAllocator, this );
	}

	virtual PxU64 getLoadedDataSize()
	{
		return mInStream.getLoadedDataSize();
	}
};

}

PvdServer& PvdServer::createServer( PxAllocatorCallback& alloc, PvdServerHandler& inHandler )
{
	return *PVD_NEW( alloc, PvdServerImpl )( alloc, inHandler );
}

PvdAsyncLoader* PvdAsyncLoader::create( PxAllocatorCallback& alloc, const char* remoteName, PxU32 firstWord, PvdNetworkInStream& inStream, PvdNetworkOutStream& outStream, PvdRandomAccessIOStream& backingStream )
{
	if ( PvdCommStreamEventReader::isPvdStream( firstWord ) == false )
	{
		PX_ASSERT( false ); return NULL;
	}
	PvdNetworkToInStream& networkToInStream( *PVD_NEW( alloc, PvdNetworkToInStream )( alloc, inStream ) );
	PvdCommStreamEventReader* reader = PvdCommStreamEventReader::create( alloc, networkToInStream, firstWord );
	if ( reader == NULL )
	{
		PX_ASSERT( false ); return NULL;
	}
	return PVD_NEW( alloc, PvdAsyncLoaderImpl )( alloc, remoteName, firstWord, networkToInStream, outStream, backingStream, *reader );
}
