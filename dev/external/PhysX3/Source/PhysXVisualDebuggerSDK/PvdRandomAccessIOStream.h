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



#ifndef PVD_RANDOM_ACCESS_IO_STREAM_H
#define PVD_RANDOM_ACCESS_IO_STREAM_H
#include "physxvisualdebuggersdk/PvdObjectModelBaseTypes.h"
#include "PvdByteStreams.h"
#include "foundation/PxMath.h"
#include "PxMemory.h"

namespace physx { namespace debugger {
	struct ErrorCode
	{
		PxU32 mErrorCode;
		ErrorCode( PxU32 code = 0 ) : mErrorCode( code ) {}
		operator PxU32 () const { return mErrorCode; }
	};
	//Wraps a possibly OS-specific error code.
	template<typename TDatatype>
	class ErrorCodeOption : public Option<TDatatype>
	{
		ErrorCode	mErrorCode;
	public:
		ErrorCodeOption( ErrorCode code ) : mErrorCode( code ) {}
		ErrorCodeOption( const TDatatype& inType ) : Option<TDatatype>( inType ) {}
		ErrorCodeOption( const ErrorCodeOption<TDatatype>& inOther )
			: Option<TDatatype>( inOther )
			, mErrorCode( inOther.mErrorCode )
		{
		}
		ErrorCodeOption& operator=( const ErrorCodeOption<TDatatype>& inOther )
		{
			Option<TDatatype>::operator=( inOther );
			mErrorCode = inOther.mErrorCode;
			return *this;
		}
	};
	//Database stream can read from anywhere but writes only to the
	//end of the stream.
	//File supports multiple readers without serializing access but writers
	//are serialized.  Writes always write to the end of the file.
	class PvdRandomAccessIOStream : public PvdOutputStream
	{
	protected:
		virtual ~PvdRandomAccessIOStream(){}

	public:
		//Refcounted object.
		virtual void addRef() = 0;
		virtual void release() = 0;

		virtual Option<ErrorCode> read( PxU8* buffer, PxU32 inLen, PxU64 inOffset ) = 0;

		//Pulled in from output stream
		//virtual bool write( const PxU8* buffer, PxU32 inLen ) = 0;

		//Direct copy another stream.
		virtual bool directCopy( PvdInputStream& inStream, PxU64 inLen ) = 0;
		//Zero pad the file length till we end on a page boundary.
		virtual void zeroPadToPageSize( PxU32 inPageSize = 0x1000 ) = 0;
		virtual PxU64 getLength() const = 0;
		virtual Option<ErrorCode> flush() = 0;
		virtual Option<ErrorCode> asyncFlush() = 0;
		virtual double getTimeCost() = 0;
		
#ifdef PX_WINDOWS
		static ErrorCodeOption<PvdRandomAccessIOStream*> open( PxAllocatorCallback& allocator, const char* inFilename, bool inCanWrite, bool inIsTemporary = false ); //throws runtime error on failure.
#endif
	};

	struct IOStreamToInputStream : public PvdInputStream
	{
		PxU64						mOffset;
		PxU64						mLen;
		PvdRandomAccessIOStream&	mStream;

		IOStreamToInputStream( PxU64 offset, PvdRandomAccessIOStream& stream )
			: mOffset( offset ), mLen( stream.getLength() ), mStream( stream )
		{
		}

		virtual bool read( PxU8* buffer, PxU32& len ) 
		{
			PxU32 available = (PxU32)PxMin( (PxU64)len, mLen - mOffset );
			Option<ErrorCode> error = mStream.read( buffer, available, mOffset );
			PX_ASSERT( error.hasValue() == false );
			if ( len > available ) physx::PxMemZero( buffer + available, len - available );
			PX_ASSERT( len == available );
			len = available;
			mOffset += available;
			return !error.hasValue();
		}
	};

}}

#endif
