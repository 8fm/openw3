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

#ifndef PVDBINDING_ERROR_STREAM_H
#define PVDBINDING_ERROR_STREAM_H

#include "foundation/PxErrorCallback.h"
#include "physxprofilesdk/PxProfileBase.h"

#include <stdio.h>

namespace physx { namespace profile {

inline void printInfo(const char* format, ...)
{
	PX_UNUSED(format); 
#if PRINT_TEST_INFO
	va_list va;
	va_start(va, format);
	vprintf(format, va);
	va_end(va); 
#endif
}

class PVDBindingErrorStream : public PxErrorCallback
{
public:

	PVDBindingErrorStream() {}
	void reportError(PxErrorCode::Enum e, const char* message, const char* file, int line)
	{
		PX_UNUSED(line); 
		PX_UNUSED(file); 
		switch (e)
		{
		case PxErrorCode::eINVALID_PARAMETER:
			printf( "on invalid parameter: %s", message );
			break;
		case PxErrorCode::eINVALID_OPERATION:
			printf( "on invalid operation: %s", message );
			break;
		case PxErrorCode::eOUT_OF_MEMORY:
			printf( "on out of memory: %s", message );
			break;
		case PxErrorCode::eDEBUG_INFO:
			printf( "on debug info: %s", message );
			break;
		case PxErrorCode::eDEBUG_WARNING:
			printf( "on debug warning: %s", message );
			break;
		default:
			printf( "on unknown error: %s", message );
			break;
		}
	}
};

}}

#endif // PVDBINDING_ERROR_STREAM_H
