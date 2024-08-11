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



#ifndef PVD_FILE_STREAMS_H
#define PVD_FILE_STREAMS_H
#include "PvdByteStreams.h"
#include <stdio.h>

namespace physx { namespace debugger {
class PvdFileOutputStream : public PvdOutputStream
{
public:
	PvdFileOutputStream(const char* filename)
	{
		mFile = NULL;
		fopen_s(&mFile, filename, "wb");
	}

	virtual	~PvdFileOutputStream()
	{
		if(mFile)
			fclose(mFile);
	}

	virtual bool write( const char* buffer, PxU32 len )
	{
		if(mFile)
		{
			if(len == fwrite(buffer, 1, len, mFile))
				return true;
		}
		return false;
	}

	virtual bool write( const PxU8* buffer, PxU32 len )
	{
		if(mFile)
		{
			if(len == fwrite(buffer, 1, len, mFile))
				return true;
		}
		return false;
	}

	virtual bool directCopy( PvdInputStream& inStream, PxU32 len )
	{
		PX_ASSERT(0);
		return false;
	}

private:
	FILE* mFile;

};

}}


#endif