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
#ifndef PVD_MEM_NETWORK_STREAMS_H
#define PVD_MEM_NETWORK_STREAMS_H
#include "physxvisualdebuggersdk/PvdNetworkStreams.h"
#include "physxvisualdebuggersdk/PvdFoundation.h"

namespace physx { namespace debugger {


	struct MemPvdNetworkOutStream : public PvdNetworkOutStream
	{
		PxAllocatorCallback&	mAllocator;
		ForwardingMemoryBuffer	mBuffer;
		bool					mConnected;
		PxU32					mRefCount;
		PxU32					mDataWritten;

		typedef MutexT<ForwardingAllocator> TMutexType;
		typedef TMutexType::ScopedLock		TScopedLockType;
		mutable TMutexType					mMutex;

		MemPvdNetworkOutStream( PxAllocatorCallback& alloc, String streamName )
			: mAllocator(alloc)
			, mMutex( ForwardingAllocator( alloc, "MemPvdNetworkOutStream::mMutex" ) )
			, mBuffer( alloc, streamName )
			, mConnected( true )
			, mRefCount( 0 )
			, mDataWritten( 0 )
		{
		}
		
		virtual PvdError write( const PxU8* inBytes, PxU32 inLength )
		{
			if ( !mConnected ) { PX_ASSERT( false ); return PvdErrorType::NetworkError; }
			mBuffer.write( inBytes, inLength );
			{
				TScopedLockType theLocker(mMutex);
				mDataWritten += inLength;
			}
			return PvdErrorType::Success;
		}

		virtual bool isConnected() const { return mConnected; }
		virtual void disconnect() { mConnected = false; }
		virtual PvdError flush() { return PvdErrorType::Success; }
		virtual void destroy() { release(); }
		virtual void addRef() { ++mRefCount; }
		virtual void release()
		{
			if ( mRefCount ) --mRefCount;
			if ( !mRefCount ) PVD_DELETE( mBuffer.getAllocator(), this );
		}
		virtual PxU64 getWrittenDataSize()
		{
			TScopedLockType theLocker(mMutex);
			return mDataWritten;
		}
	};
}}

#endif