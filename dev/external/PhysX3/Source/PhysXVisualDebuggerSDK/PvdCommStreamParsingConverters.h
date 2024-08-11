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


#ifndef PVD_COMM_STREAM_PARSING_CONVERTERS_H
#define PVD_COMM_STREAM_PARSING_CONVERTERS_H
#include "PvdObjectModelBaseTypes.h"

namespace physx { namespace debugger {
	class StringTable;
}}

namespace physx { namespace debugger { namespace comm {
	

	struct ParserConverterData
	{
		PxU32 incomingOffset;
		PxU32 outgoingOffset;
		PxU32 numItems;
		ParserConverterData() : incomingOffset( 0 ), outgoingOffset( 0 ), numItems( 0 ) {}
	};

	struct ConverterSrcData
	{
		PxU8* src; //Ptr to incoming data
		PxU8* srcStringStart; //Ptr to next string data, if any
		PxU8* srcEnd; //Ptr to end of the src data.
		ConverterSrcData( PxU8* _src, PxU8* _srcEnd, PxU32 dataSectionSize )
			: src( _src ), srcStringStart( _src + dataSectionSize ), srcEnd( _srcEnd )
		{
			if ( src == srcEnd ) srcStringStart = srcEnd;
			verify();
		}
		ConverterSrcData( const ConverterSrcData& other )
			: src( other.src )
			, srcStringStart( other.srcStringStart )
			, srcEnd( other.srcEnd )
		{
		}
		inline void setStringStart( PxU8* newStart )
		{
			srcStringStart = newStart;
			verify();
		}
		inline void verify()
		{
			PX_ASSERT( src <= srcEnd );
			PX_ASSERT( srcStringStart <= srcEnd );
			PX_ASSERT( src <= srcStringStart );
		}

		inline const char* nextStr()
		{
			const char* retval = "";
			PX_ASSERT( srcStringStart < srcEnd );
			if ( srcStringStart < srcEnd )
			{
				retval = reinterpret_cast<const char*>( srcStringStart );
				srcStringStart += safeStrLen( retval ) + 1;
			}
			PX_ASSERT( srcStringStart <= srcEnd );
			return retval;
		}
	};

	//Converters need to do 3 things
	//1. Endian convert
	//2. Stream convert (void* to 32 bit db id)
	//3. String convert (const char* to 32 bit StringHandle)
	//4. Marshal between one type of data item and another (u8 -> u32, or u8 -> float)
	//Furthermore, there are 3 conditions this happens under.
	//1. Convert this data, store result back in the src location.
	//2. Convert this data, store result into dest location.
	//2. Convert a large block of this data, as quickly as you can.
	//The first three conditions are handled by the underlying datatype of the converter.
	//the last three conditions need to be handled by the converters themselves.
	//Strings are stored, null terminated, *after* the initial block of data.  Thus
	//anything sending string data will send it *after* the main message.
	//For arrays, this means after each entry in the array any strings contains in the entry
	//are embedded in the data stream.  For property messages, this means the string is sent
	//after the property message.
	//
	//srcEnd marks the absolute end of the message, string offset
	//is always <= srcEnd - src;
	//
	//Finally the converter returns the location of the next string in the dataset.
	//which must lie directly after the last string that was read.
	class ParserStreamDataConverter
	{
	protected:
		virtual ~ParserStreamDataConverter(){}
	public:
		virtual PxU8* Convert( ParserConverterData data, ConverterSrcData srcData ) = 0;
		//Convert, write results to dest
		virtual PxU8* Convert( ParserConverterData data, ConverterSrcData srcData, PxU8* dest ) = 0;
		//Convert, write results to src
		virtual void ConvertBlock( ParserConverterData data, ConverterSrcData srcData ) = 0;
		//Convert, write results to dest
		virtual void ConvertBlock( ParserConverterData data, ConverterSrcData srcData, PxU8* dest ) = 0;
	};

	class ParserIdToDBHandle
	{
	protected:
		virtual ~ParserIdToDBHandle(){}
	public:
		virtual PxI32 getDBHandle( PxU32 streamId ) = 0;
		virtual PxI32 getDBHandle( PxU64 streamId ) = 0;
	};

	class ParserStreamDataConverterFactory
	{
	protected:
		virtual ~ParserStreamDataConverterFactory(){}
	public:
		virtual ParserStreamDataConverter* getConverter( PxI32 incomingDatatype, PxI32 outgoingDatatype, PxU32 incomingDatatypePackedWidth ) = 0;
		virtual void release() = 0;

		static ParserStreamDataConverterFactory& createConverterFactory( PxAllocatorCallback& alloc, bool swapBytes, bool PtrsAre64Bit
																		, ParserIdToDBHandle& handleConversion, StringTable& stringTable );
	};
	
	struct ConverterAndData
	{
		ParserConverterData			mData;
		ParserStreamDataConverter*	mConverter;
		ConverterAndData( ParserStreamDataConverter* converter )
			: mData( ParserConverterData() ), mConverter( converter ) {}

		void Convert( ConverterSrcData& srcData )
		{
			PX_ASSERT( mData.incomingOffset >= mData.outgoingOffset);
			srcData.setStringStart( mConverter->Convert( mData, srcData ) );
		}
		void Convert( ConverterSrcData& srcData, PxU8* dest )
		{
			PX_ASSERT( mData.incomingOffset >= mData.outgoingOffset);
			srcData.setStringStart( mConverter->Convert( mData, srcData, dest ) );
		}
		void ConvertBlock( ConverterSrcData& srcData )
		{
			PX_ASSERT( mData.incomingOffset == 0 );
			PX_ASSERT( mData.outgoingOffset == 0 );
			mConverter->ConvertBlock( mData, srcData );
		}
		void ConvertBlock( ConverterSrcData& srcData, PxU8* dest )
		{
			PX_ASSERT( mData.incomingOffset == 0 );
			PX_ASSERT( mData.outgoingOffset == 0 );
			mConverter->ConvertBlock( mData, srcData, dest );
		}
	};
}}}



#endif