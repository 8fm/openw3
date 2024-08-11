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



#ifndef PVD_INTERNAL_BYTE_STREAMS_H
#define PVD_INTERNAL_BYTE_STREAMS_H
#include "PvdByteStreams.h"
#include "PvdFoundation.h"

namespace physx { namespace debugger {
	struct MemPvdOutputStream : public PvdOutputStream
	{
		ForwardingMemoryBuffer mBuffer;
		MemPvdOutputStream( PxAllocatorCallback& callback, const char* memName ) : mBuffer( callback, memName ) {}
		
		virtual bool write( const PxU8* buffer, PxU32 len )
		{
			mBuffer.write( buffer, len );
			return true;
		}

		virtual bool directCopy( PvdInputStream& inStream, PxU32 len )
		{
			PxU32 offset = mBuffer.size();
			mBuffer.growBuf( len );
			PxU32 readLen = len;
			inStream.read( mBuffer.begin() + offset, readLen );
			if ( readLen != len )
				PxMemZero( mBuffer.begin() + offset, len );
			return readLen == len;
		}

		const PxU8* begin() const { return mBuffer.begin(); }
		PxU32 size() const { return mBuffer.size(); }
		void clear() { mBuffer.clear(); }
		DataRef<const PxU8> toRef() const { return DataRef<const PxU8>( mBuffer.begin(), mBuffer.end() ); }
	};

	struct MemPvdInputStream : public PvdInputStream
	{
		const PxU8* mBegin;
		const PxU8* mEnd;
		bool mGood;

		MemPvdInputStream( const MemPvdOutputStream& stream )
			: mGood( true )
		{
			mBegin = stream.mBuffer.begin();
			mEnd = stream.mBuffer.end();
		}

		MemPvdInputStream( const PxU8* beg = NULL, const PxU8* end = NULL )
		{
			mBegin = beg;
			mEnd = end;
			mGood = true;
		}

		PxU32 size() const { return mGood ? static_cast<PxU32>( mEnd - mBegin ) : 0; }
		bool isGood() const { return mGood; }

		void setup( PxU8* start, PxU8* stop )
		{
			mBegin = start;
			mEnd = stop;
		}

		void nocopyRead( PxU8*& buffer, PxU32& len )
		{
			if ( len == 0 || mGood == false ){ len = 0; buffer = NULL; return; }
			PxU32 original = len;
			len = PxMin( len, size() );
			if ( mGood && len != original ) mGood = false;
			buffer = const_cast<PxU8*>( mBegin );
			mBegin += len;
		}

		virtual bool read( PxU8* buffer, PxU32& len )
		{
			if ( len == 0 ) return true;
			PxU32 original = len;
			len = PxMin( len, size() );

			PxMemCopy( buffer, mBegin, len );
			mBegin += len;
			if ( len < original )
				PxMemZero( buffer + len, original - len );
			mGood = mGood && len == original;
			return mGood;
		}
	};
}}

#endif