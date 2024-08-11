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
#include "PvdRandomAccessIOStream.h"
#include "PvdFoundation.h"
#include "PsMutex.h"
#include "PsTime.h"
#include "PvdObjectModelInternalTypes.h"
#include <windows.h>
#include <limits>
#include <stdio.h>

using namespace physx::shdfnd;
using namespace physx::debugger;

namespace {

struct FileBuffer : public OVERLAPPED
{
	ForwardingMemoryBuffer	mBuffer;
	PxU32					mIOSetIndex;
	FileBuffer(PxAllocatorCallback& callback)
		: mBuffer( callback, "FileBuffer::mBuffer" )
		, mIOSetIndex( PX_MAX_U32 )
	{
		OVERLAPPED* base = static_cast< OVERLAPPED* >( this );
		memset( base, 0, sizeof( OVERLAPPED ) );
	}
	~FileBuffer()
	{
	}

	bool isInFlight() { return mIOSetIndex != PX_MAX_U32; }

	DWORD clear()
	{
		mBuffer.clear();
		return ERROR_SUCCESS;
	}
};

struct FileBufferIOSetSetter
{
	void operator()( FileBuffer& buffer, PxU32 idx ) { buffer.mIOSetIndex = idx; }
};

struct FileBufferIOSetGetter
{
	PxU32 operator()( FileBuffer& buffer ) { return buffer.mIOSetIndex; }
};

typedef InvasiveSet<FileBuffer, FileBufferIOSetGetter, FileBufferIOSetSetter> TFileBufferIOSet;

#define TNumBuffers 16

class RandomAccessIOStreamImpl : public PvdRandomAccessIOStream
{
typedef MutexT<ForwardingAllocator> TMutexType;
typedef TMutexType::ScopedLock TScopedLockType;

	PxAllocatorCallback&			mAllocator;
	HANDLE							mFile;
	HANDLE							mIOCompletionPort;

	mutable TMutexType				mMutex;
	ForwardingArray<FileBuffer*>	mBuffers;
	ForwardingArray<FileBuffer*>	mAvailableBuffers;
	PxU64							mFileLen;
	FileBuffer*						mWriteBuffer;
	const PxU32						mMaxBufferLen;
	PxU32							mRefCount;
	bool							mIsTemporaryFile;
	const char*						mFilename;
	TFileBufferIOSet				mOutstandingIOOps;
	Time::Second					mTimeCost;
	Time							mProfileTimer;

public:
	RandomAccessIOStreamImpl( PxAllocatorCallback& inCallback, PxU32 inMaxBufferLen, const char* inFilename, bool openForWrite, bool inIsTemporary, HANDLE inFile, HANDLE inIOCompletionPort )
		: mAllocator( inCallback )
		, mFileLen( 0 )
		, mWriteBuffer( 0 )
		, mMutex( ForwardingAllocator( inCallback, "RandomAccessIOStreamImpl::mMutex" ) )
		, mBuffers( inCallback, "RandomAccessIOStreamImpl::mBuffers" )
		, mAvailableBuffers( inCallback, "RandomAccessIOStreamImpl::mAvailableBuffers" )
		, mMaxBufferLen( inMaxBufferLen )
		, mRefCount( 0 )
		, mIsTemporaryFile( inIsTemporary )
		, mOutstandingIOOps( inCallback, "RandomAccessIOStreamImpl::mOutstandingIOOps" )
		, mFile( inFile )
		, mIOCompletionPort( inIOCompletionPort )
		, mTimeCost( 0.0 )
	{
		mFilename = copyStr( inCallback, inFilename, __FILE__, __LINE__ );
		if ( openForWrite == false )
		{
			//set the write offset based on the final file position.
			DWORD sizeHigh = 0;
			DWORD sizeLow = GetFileSize( mFile, &sizeHigh );
			mFileLen = sizeLow;
			mFileLen += ((PxU64)sizeHigh) << 32;
		}
		
		for ( PxU32 idx = 0; idx < TNumBuffers; ++idx )
		{
			mBuffers.pushBack( PVD_NEW( mAllocator, FileBuffer )( inCallback ) );
			mAvailableBuffers.pushBack( mBuffers[idx] );
		}
	}

	virtual ~RandomAccessIOStreamImpl()
	{
		if ( mFile != INVALID_HANDLE_VALUE )
		{
			//Wait for any pending io.
			flush();
			CloseHandle( mFile );
			if ( mIsTemporaryFile )
				remove( mFilename );
		}
		mFile = INVALID_HANDLE_VALUE;
		if( mIOCompletionPort != NULL )
		{
			CloseHandle( mIOCompletionPort );
			mIOCompletionPort = NULL;
		}
		mOutstandingIOOps.clear();
		for ( PxU32 idx = 0; idx < TNumBuffers; ++idx )
			PVD_DELETE( mAllocator, mBuffers[idx] );
		mAllocator.deallocate( (void*)mFilename );
	}

	virtual double getTimeCost()
	{
		TScopedLockType theLock( mMutex );
		return (double)mTimeCost;
	}
	
	virtual void addRef()
	{
		TScopedLockType theLock( mMutex );
		++mRefCount;
	}

	virtual void release()
	{
		PxU32 theRefCount = 0;
		{
			TScopedLockType theLock( mMutex );
			if ( mRefCount ) --mRefCount;
			theRefCount = mRefCount;
		}
		if ( !theRefCount ) PVD_DELETE(mAllocator, this );	
	}

	FileBuffer* getNextQueuedBuffer()
	{
		LPOVERLAPPED theOverlapped = NULL;
		DWORD transferBytes = 0;
		ULONG_PTR transferKey = NULL;
		do
		{
			GetQueuedCompletionStatus( mIOCompletionPort, &transferBytes, &transferKey,  &theOverlapped, INFINITE );
			PX_ASSERT( transferKey == reinterpret_cast<ULONG_PTR>( this ) );
		}
		while ( theOverlapped == NULL );
		return static_cast<FileBuffer*>( theOverlapped );
	}

	//Buffer is in mAvailableBuffers when found.
	FileBuffer* waitForNextCompletedIOOp(PxU32& outOpLen) 
	{
		outOpLen = 0;
		PxU32 theCount = 0;
		{
			TScopedLockType theLock( mMutex );
			theCount = mOutstandingIOOps.size();
		}
		if ( theCount != 0 ) 
		{
			FileBuffer* theBuffer = getNextQueuedBuffer();
			{
				TScopedLockType theLock( mMutex );
				PX_ASSERT( theBuffer->isInFlight() );
				mOutstandingIOOps.remove( *theBuffer );
				outOpLen = static_cast<PxU32>( theBuffer->InternalHigh );
				theBuffer->clear();
				mAvailableBuffers.pushBack( theBuffer );
				return theBuffer;
			}
		}
		return NULL;
	}

	FileBuffer* nextAvailableBuffer()
	{
		FileBuffer* retval = NULL;
		do
		{
			{
				//atomically grab the next buffer and add it to the outstanding
				//io operation record.
				TScopedLockType theLock( mMutex );
				if ( mAvailableBuffers.size() )
				{
					retval = mAvailableBuffers.back();
					PX_ASSERT( retval->isInFlight() == false );
					mAvailableBuffers.popBack();
				}
			}
			if ( retval == NULL )
			{
				PxU32 ignored;
				waitForNextCompletedIOOp(ignored);
			}
		}while( retval == NULL );

		return retval;
	}

	void addOutstandingIOOperation(FileBuffer* inBuffer )
	{
		TScopedLockType theLock( mMutex );
		mOutstandingIOOps.insert( *inBuffer );
	}

	virtual Option<ErrorCode> flush()
	{
		//keep anyone else from messing with the file while we do this.
		TScopedLockType theLock( mMutex );
		sendOutstandingWriteBuffer();
		PxU32 ignored;

		for( FileBuffer* theBuffer = waitForNextCompletedIOOp(ignored);
			theBuffer;
			theBuffer = waitForNextCompletedIOOp(ignored) )
		{
			//empty for loop on purpose.
		}
		return None();
	}

	
	virtual Option<ErrorCode> read( PxU8* inBytes, PxU32 inLength, PxU64 inOffset )
	{	
		if ( inLength == 0 )
			return None();
		if ( inBytes == NULL )
			return ErrorCode( PX_MAX_U32 );

		PxU64 desiredEnd = inLength + inOffset;
		if ( desiredEnd > getLength() )
			return ErrorCode( PX_MAX_U32 - 1 );
		
		//We have to flush before a read else we get hammered because at this point I am too
		//lazy to write the proper logic to mess with the write buffer along with the outstanding
		//buffers that haven't been cleared yet.
		flush();
		//Get the next available buffer.
		FileBuffer* nextBuffer = nextAvailableBuffer();
		//confirmed means you can actually read from the file itself.
		//unconfirmed means you have to read from the overlapped structure in flight.
		while( inLength )
		{
			nextBuffer->clear();
			nextBuffer->Offset = static_cast<PxU32>( inOffset );
			nextBuffer->OffsetHigh = static_cast<PxU32>( inOffset >> 32 );
			
			DWORD readLen = 0;
			BOOL success = ReadFile( mFile, inBytes, static_cast<DWORD>( inLength ), &readLen, nextBuffer );
			DWORD error = GetLastError();
			PxU32 readResultLen = 0;
			if ( success || error == ERROR_IO_PENDING )
			{
				addOutstandingIOOperation(nextBuffer);
				bool containsIOOp = false;
				do
				{
					PxU32 lastOpLen = 0;
					{
						TScopedLockType theLock( mMutex );
						containsIOOp = nextBuffer->isInFlight();
					}
					if ( containsIOOp ) 
						waitForNextCompletedIOOp(lastOpLen);
					else
						readResultLen = lastOpLen;
				}while( containsIOOp );
			}
			else
				return ErrorCode( error );
			if ( readResultLen == 0 )
			{
				if ( success )
					readResultLen = readLen;
				else
					readResultLen = inLength; //assume we read everything.  Pending ops usually do.
			}
			inBytes += readResultLen;
			inLength -= readResultLen;
			inOffset += readResultLen;
		}
		return None();
	}

	Option<ErrorCode> sendOutstandingWriteBuffer()
	{
		TScopedLockType theLock( mMutex );
		if ( mWriteBuffer != NULL &&
			mWriteBuffer->mBuffer.size() )
		{
			BOOL success = WriteFile( mFile, mWriteBuffer->mBuffer.begin(), mWriteBuffer->mBuffer.size(), NULL, mWriteBuffer );
			DWORD error = GetLastError();
			if ( success || error == ERROR_IO_PENDING )
			{
				addOutstandingIOOperation( mWriteBuffer );
				mWriteBuffer = NULL;
			}
			else 
				return ErrorCode( error );
		}
		return None();
	}

	template<typename TWriteOperator>
	void doWrite( TWriteOperator inOperator )
	{
		if ( inOperator.getLength() )
		{
			TScopedLockType theLock( mMutex );

			mProfileTimer.getElapsedSeconds();

			while( inOperator.getLength() )
			{
				if ( mWriteBuffer == NULL )
				{
					mWriteBuffer = nextAvailableBuffer();
					mWriteBuffer->Offset = static_cast<PxU32>( mFileLen );
					mWriteBuffer->OffsetHigh = static_cast<PxU32>( mFileLen >> 32 );
					mWriteBuffer->mBuffer.clear();
				}

				PxU32 maxWritePossible = static_cast<PxU32>( mMaxBufferLen - static_cast<PxU32>( mWriteBuffer->mBuffer.size() ) );

				PxU32 amountToWrite = static_cast<PxU32>( PxMin( (PxU64)inOperator.getLength(), (PxU64)maxWritePossible ) );

				inOperator.writeData( mWriteBuffer->mBuffer, amountToWrite );

				if ( static_cast<PxU32>( mWriteBuffer->mBuffer.size() ) == mMaxBufferLen )
					sendOutstandingWriteBuffer();

				mFileLen += amountToWrite;
			}

			mTimeCost += mProfileTimer.getElapsedSeconds();
		}
	}

	struct BytesWriteOperator
	{
		const PxU8* mBytes;
		PxU32 mLength;
		BytesWriteOperator( const PxU8* inBytes, PxU32 inLen )
			: mBytes( inBytes )
			, mLength( inLen )
		{
		}

		PxU32 getLength() const { return mLength; }

		void writeData( ForwardingMemoryBuffer& ioList, PxU32 inLen )
		{
			ioList.write( mBytes, inLen );
			mLength -= inLen;
			mBytes += inLen;
		}
	};

	virtual bool write( const PxU8* inBytes, PxU32 inLength )
	{	
		doWrite( BytesWriteOperator( inBytes, inLength ) );		
		return true;
	}

	struct ZeroPadWriteOperator
	{
		PxU32 mLength;
		ZeroPadWriteOperator( PxU32 inLen ) : mLength( inLen ) {}
		PxU32 getLength() const { return mLength; }
		void writeData( ForwardingMemoryBuffer& ioList, PxU32 inLen )
		{
			ioList.writeZeros( inLen );
			mLength -= inLen;
		}
	};

	virtual void zeroPadToPageSize( PxU32 inPageSize )
	{
		TScopedLockType theLock( mMutex );
		PxU64 paddedLen = toPaddedSize( mFileLen, inPageSize );
		PxU32 amountToWrite = static_cast<PxU32>( paddedLen - mFileLen );
		doWrite( ZeroPadWriteOperator( amountToWrite ) );
	}

	struct SerializerInStreamWriter
	{
		PxU64						mLength;
		PvdInputStream&				mStream;
		SerializerInStreamWriter(PvdInputStream& s, PxU64 l )
			: mLength( l )
			, mStream( s )
		{
		}

		PxU64 getLength() const { return mLength; }

		void writeData( ForwardingMemoryBuffer& ioList, PxU32 inLen )
		{
			PxU32 listSize = ioList.size();
			ioList.growBuf( inLen );
			mStream.read( reinterpret_cast<char*>( ioList.begin() + listSize ), inLen );
			mLength -= inLen;
		}
	};
	
	virtual bool directCopy( PvdInputStream& inStream, PxU32 inLen )
	{
		doWrite( SerializerInStreamWriter( inStream, inLen ) );
		return true;
	}
	
	virtual bool directCopy( PvdInputStream& inStream, PxU64 inLen )
	{
		doWrite( SerializerInStreamWriter( inStream, inLen ) );
		return true;
	}

	virtual PxU64 getLength() const { TScopedLockType theLock( mMutex ); return mFileLen; }

	virtual Option<ErrorCode> asyncFlush() { return sendOutstandingWriteBuffer(); }
};

}

#define TBufSize 0x40000


ErrorCodeOption<PvdRandomAccessIOStream*> PvdRandomAccessIOStream::open( PxAllocatorCallback& alloc, const char* inFilename, bool openForWrite, bool inIsTemporary )
{	
	DWORD mask = GENERIC_READ;
	if ( openForWrite )
		mask |= GENERIC_WRITE;
	HANDLE file = INVALID_HANDLE_VALUE;
	if ( openForWrite )
		file = CreateFile( inFilename, mask, FILE_SHARE_READ, NULL, TRUNCATE_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED | FILE_FLAG_SEQUENTIAL_SCAN, 0 );
	if ( file == INVALID_HANDLE_VALUE )
		file = CreateFile( inFilename, mask, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED | FILE_FLAG_SEQUENTIAL_SCAN, 0 );

	if ( file == INVALID_HANDLE_VALUE )
		return ErrorCodeOption<PvdRandomAccessIOStream*>( ErrorCode( GetLastError() ) );

	RandomAccessIOStreamImpl* newStream = (RandomAccessIOStreamImpl*)alloc.allocate( sizeof( RandomAccessIOStreamImpl ), "RandomAccessIOStreamImpl", __FILE__, __LINE__ );
	//We use buffers for everything, attempt to get to zero-copy writes and reads.
	HANDLE port = CreateIoCompletionPort( file, NULL, reinterpret_cast<ULONG_PTR>( newStream ), 10 );
	if ( port == NULL )
	{
		alloc.deallocate( newStream );
		return ErrorCodeOption<PvdRandomAccessIOStream*>( ErrorCode( GetLastError() ) );
	}
	new (newStream) RandomAccessIOStreamImpl( alloc, TBufSize, inFilename, openForWrite, inIsTemporary, file, port );
	return newStream;
}
