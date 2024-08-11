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



#ifndef PVD_MEM_RANDOM_ACCESS_IO_STREAM_H
#define PVD_MEM_RANDOM_ACCESS_IO_STREAM_H
#include "physxvisualdebuggersdk/PvdRandomAccessIOStream.h"
#include "physxvisualdebuggersdk/PvdFoundation.h"

namespace physx { namespace debugger {

	struct PvdMemRandomAccessIOStream : public PvdRandomAccessIOStream
	{
		ForwardingMemoryBuffer mBuffer;
		PxU32 mRefCount;

	public:
		PvdMemRandomAccessIOStream( PxAllocatorCallback& callback, const char* memName )
			: mBuffer( callback, memName )
			, mRefCount( 0 )
		{
		}

		const PxU8* begin() const { return mBuffer.begin(); }
		const PxU8* end() const { return mBuffer.end(); }
		PxU32		size() const { return mBuffer.size(); }
		


		//Refcounted object.
		virtual void addRef() { ++mRefCount; }
		virtual void release() { if ( mRefCount ) --mRefCount; if (!mRefCount ){ PVD_DELETE (mBuffer.getAllocator(), this ); } }

		virtual Option<ErrorCode> read( PxU8* buffer, PxU32 inLen, PxU64 inOffset ) 
		{
			PxU32 offset = (PxU32)( PxMin( inOffset, (PxU64)size() ) );
			PxU32 available = PxMin( size() - offset, inLen );
			if ( available < inLen )
				PX_ASSERT( false );
			PxMemCopy( buffer, begin() + offset, available );
			if ( available < inLen )
				PxMemZero( buffer + available, inLen - available );
			return None();
		}

		//Pulled in from output stream
		virtual bool write( const PxU8* buffer, PxU32 inLen )
		{
			mBuffer.write( buffer, inLen );
			return true;
		}
		
		virtual bool directCopy( PvdInputStream& stream, PxU32 inLen )
		{
			PxU32 offset = mBuffer.size();
			PxU32 maxAvailable = (PxU32)(PxMin( (PxU32)PX_MAX_U32 - mBuffer.size(), inLen ));
			mBuffer.growBuf( maxAvailable );
			stream.read( mBuffer.begin() + offset, maxAvailable );
			return true;
		}

		//Direct copy another stream.
		virtual bool directCopy( PvdInputStream& stream, PxU64 inLen )
		{
			PxU32 offset = mBuffer.size();
			PxU32 maxAvailable = (PxU32)(PxMin( (PxU64)PX_MAX_U32 - mBuffer.size(), inLen ));
			mBuffer.growBuf( maxAvailable );
			stream.read( mBuffer.begin() + offset, maxAvailable );
			return true;
		}

		//Zero pad the file length till we end on a page boundary.
		virtual void zeroPadToPageSize( PxU32 inPageSize = 0x1000 )
		{
			PxU32 current = size();
			PxU32 desired = (PxU32)toPaddedSize( current, inPageSize );
			if ( desired > current )
				mBuffer.writeZeros( desired - current );
		}
		virtual PxU64 getLength() const { return size(); }
		virtual Option<ErrorCode> flush() { return None(); }
		virtual Option<ErrorCode> asyncFlush() { return None(); }
		double getTimeCost() { return 0.0; }
	};
}}

#endif