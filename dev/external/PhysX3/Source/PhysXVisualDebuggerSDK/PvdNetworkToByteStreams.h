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
#ifndef PVD_NETWORK_TO_BYTE_STREAMS_H
#define PVD_NETWORK_TO_BYTE_STREAMS_H
#include "physxvisualdebuggersdk/PvdByteStreams.h"
#include "physxvisualdebuggersdk/PvdErrorCodes.h"
#include "physxvisualdebuggersdk/PvdNetworkStreams.h"

namespace physx { namespace debugger {

struct PvdNetworkToInStream : public PvdInputStream
{
	PxAllocatorCallback& mAlloc;
	PvdNetworkInStream& mStream;
	PvdNetworkToInStream( PxAllocatorCallback& alloc, PvdNetworkInStream& str ) : mAlloc( alloc ), mStream( str ) {}
	virtual ~PvdNetworkToInStream()
	{
		mStream.release();
	}
	virtual bool read( PxU8* buffer, PxU32& len ) 
	{
		PvdError error = mStream.readBytes( buffer, len );
		if ( error != PvdErrorType::Success )
			len = 0;
		return error == PvdErrorType::Success;
	}
	virtual void release()
	{
		PVD_DELETE( mAlloc, this );
	}

	virtual PxU64 getLoadedDataSize()
	{
		return mStream.getLoadedDataSize();
	}
};


struct PvdNetworkToOutStream : public PvdOutputStream
{
	PxAllocatorCallback& mAlloc;
	PvdNetworkOutStream& mStream;
	PvdNetworkToOutStream( PxAllocatorCallback& alloc, PvdNetworkOutStream& str )  : mAlloc( alloc ), mStream( str ) {}
	virtual ~PvdNetworkToOutStream()
	{
		mStream.release();
	}
	virtual bool write( const PxU8* buffer, PxU32 len )
	{
		PvdError error = mStream.write( buffer, len );
		return error == PvdErrorType::Success;
	}
	virtual bool directCopy( PvdInputStream& inStream, PxU32 len ) 
	{
		PxU8 buf[2048];
		while( len )
		{
			PxU32 request = 2048;
			inStream.read( buf, request );
			bool success = write( buf, request );
			len -= request;
			if ( success == false || 
				(len > 0 && request != 2048 ) )
				return false;
		}
		return true;
	}
	virtual void release()
	{
		PVD_DELETE( mAlloc, this );
	}
};
}}

#endif