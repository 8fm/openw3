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
#ifndef PVD_COMM_STREAM_PARSING_H
#define PVD_COMM_STREAM_PARSING_H
#include "PvdObjectModelInternalTypes.h"

namespace physx { namespace debugger {
	class PvdObjectModelMutator;
	class PvdObjectModelMetaData;
	class PvdInputStream;
	class PvdOutputStream;
}}

namespace physx { namespace debugger { namespace comm {
	class PvdCommStreamMetaDataMutator;

	//The parser does interact with the database, so it really cannot be on
	//another thread or its interaction needs to be protected.
	class PvdCommStreamParser
	{
	protected:
		virtual ~PvdCommStreamParser(){}
	public:
		virtual bool parseNextEventGroup( DataRef<const PxU8> eventData ) = 0;
		virtual void release() = 0;
	};

	//The reader can sit and just read data over and over again.
	//It doesn't interact with the database, so we can put it on another thread
	//if desired.
	class PvdCommStreamEventReader
	{
	protected:
		virtual ~PvdCommStreamEventReader(){}
	public:
		static bool isPvdStream( PxU32 firstWord );
		//Read the next event and place the data, unchanged or endian converted, into result.
		virtual bool readNextEventGroup( PvdInputStream& inStream, PvdOutputStream& result ) = 0;
		virtual bool isSwappingBytes() = 0;
		virtual void release() = 0;
		virtual PvdCommStreamParser& createParser( PvdObjectModelMetaData& metaData
													, PvdObjectModelMutator& model
													, PvdCommStreamMetaDataMutator&	commStreamMeta ) = 0;

		static PvdCommStreamEventReader* create( PxAllocatorCallback& alloc, PvdInputStream& inStream, PxU32 inFirstWord );
	};
}}}

#endif
