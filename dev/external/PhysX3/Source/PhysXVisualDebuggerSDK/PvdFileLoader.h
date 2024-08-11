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
#ifndef PVD_FILE_LOADER_H
#define PVD_FILE_LOADER_H
#include "physxvisualdebuggersdk/PvdObjectModelBaseTypes.h"

namespace physx { namespace debugger { 
	class PvdClientMutator;
	class PvdRandomAccessIOStream;

	class PvdFileStreamLoader
	{
	protected:
		virtual ~PvdFileStreamLoader(){}
	public:
		//load near to the next chunk size.  If function returns zero,
		//then no more data is possible to be read.  Most likely we will
		//go over the requested amount by a small amount.  Returns the exact
		//amount of bytes processed this request.
		virtual PxU32 loadNextChunk(PxU32 inRequestedAmountInBytes) = 0;
		virtual PvdClientMutator& getClient() = 0;
		virtual void release() = 0;
	};

	struct PvdFileTypes
	{
		enum Enum
		{
			Unknown,
			Stream,
			Database,
		};
	};

	class PvdFileLoaderFactory
	{
	protected:
		virtual ~PvdFileLoaderFactory(){}

	public:
		virtual PvdFileStreamLoader* createFileStreamLoader( PvdRandomAccessIOStream& stream, PvdRandomAccessIOStream& backingStream ) = 0;
		virtual PvdFileTypes::Enum getFileType( PxU32 inFirstFileWord ) = 0;
		virtual void release() = 0;

		static PvdFileLoaderFactory& createFileLoaderFactory( PxAllocatorCallback& alloc );
	};

}}

#endif