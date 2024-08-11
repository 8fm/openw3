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


#ifndef PVD_OBJECT_MODEL_INTERNAL_TYPES_H
#define PVD_OBJECT_MODEL_INTERNAL_TYPES_H
#include "physxvisualdebuggersdk/PvdObjectModelBaseTypes.h"
#include "PsArray.h"
#include "PvdFoundation.h"
#include "PvdObjectModel.h"

namespace physx { namespace debugger {
	using namespace physx::shdfnd;
	struct PvdInternalType
	{
		enum Enum
		{
			None = 0,
#define DECLARE_INTERNAL_PVD_TYPE( type ) type,
	#include "PvdObjectModelInternalTypeDefs.h"
			Last,
#undef DECLARE_INTERNAL_PVD_TYPE
		};
	};

	PX_COMPILE_TIME_ASSERT( (PxU32)PvdInternalType::Last <= (PxU32)PvdBaseType::InternalStop );

	template<typename T>
	struct DataTypeToPvdTypeMap
	{
		bool compile_error;
	};
	template<PvdInternalType::Enum>
	struct PvdTypeToDataTypeMap
	{
		bool compile_error;
	};

	
#define DECLARE_INTERNAL_PVD_TYPE( type )																											\
	template<> struct DataTypeToPvdTypeMap<type> { enum Enum { BaseTypeEnum = PvdInternalType::type }; };											\
	template<> struct PvdTypeToDataTypeMap<PvdInternalType::type> { typedef type TDataType; };														\
	template<> struct PvdDataTypeToNamespacedNameMap<type> { NamespacedName Name; PvdDataTypeToNamespacedNameMap<type>() : Name( "physx3_debugger_internal", #type ) {} };
#include "PvdObjectModelInternalTypeDefs.h"
#undef DECLARE_INTERNAL_PVD_TYPE

	template<typename TDataType, typename TAlloc>
	DataRef<TDataType> toDataRef( const Array<TDataType, TAlloc>& data ) { return DataRef<TDataType>( data.begin(), data.end() ); }

	static inline bool safeStrEq( const DataRef<String>& lhs, const DataRef<String>& rhs )
	{
		PxU32 count = lhs.size();
		if ( count != rhs.size() )
			return false;
		for ( PxU32 idx = 0; idx < count; ++idx )
			if ( !safeStrEq( lhs[idx], rhs[idx] ) )
				return false;
		return true;
	}

	static inline char* copyStr( PxAllocatorCallback& alloc, const char* str, const char* file, int line )
	{
		str = nonNull( str );
		PxU32 len = static_cast<PxU32>( strlen( str ) );
		char* newData = (char*)alloc.allocate( len + 1, "string", file, line );
		memcpy( newData, str, len );
		newData[len] = 0;
		return newData;
	}
	
	//Used for predictable bit fields.
	template<typename TDataType
			, PxU8 TNumBits
			, PxU8 TOffset
			, typename TInputType>
	struct BitMaskSetter
	{
		//Create a mask that masks out the orginal value shift into place
		static TDataType createOffsetMask() { return createMask() << TOffset; }
		//Create a mask of TNumBits number of tis
		static TDataType createMask() { return static_cast<TDataType>((1 << TNumBits) - 1); }
		void setValue( TDataType& inCurrent, TInputType inData )
		{
			PX_ASSERT( inData < ( 1 << TNumBits ) );
			
			//Create a mask to remove the current value.
			TDataType theMask = ~(createOffsetMask());
			//Clear out current value.
			inCurrent = inCurrent & theMask;
			//Create the new value.
			TDataType theAddition = (TDataType)( inData << TOffset );
			//or it into the existing value.
			inCurrent = inCurrent | theAddition;
		}

		TInputType getValue( TDataType inCurrent )
		{
			return static_cast<TInputType>( ( inCurrent >> TOffset ) & createMask() );
		}
	};

	template<typename TObjType>
	DataRef<TObjType> getArrayData( ForwardingArray<PxU8>& dataBuffer, PvdObjectModelReader& reader, InstanceDescription instanceDesc, String propName )
	{
		PxI32 propId = reader.getMetaData().findProperty( reader.getClassOf( instanceDesc )->mClassId, propName )->mPropertyId;
		PxU32 numBytes = reader.getPropertyByteSize( instanceDesc.mInstPtr, propId );
		PxU32 numItems = reader.getNbArrayElements( instanceDesc.mInstPtr, propId );
		if ( numBytes == 0 ) return NULL;
		if ( numBytes > dataBuffer.size() ) dataBuffer.resize( numBytes );

		TObjType* dataPtr = reinterpret_cast<TObjType*>( dataBuffer.begin() );
		reader.getPropertyValue( instanceDesc, propId, dataBuffer.begin(), numBytes );
		return DataRef<TObjType>( dataPtr, numItems );
	}	
}}

#endif