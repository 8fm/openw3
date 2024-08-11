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
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  

#include "PxErrors.h"
#include "PxErrorCallback.h"
#include "PsErrorHandler.h"
#include "PsBitUtils.h"
#include "PsFoundation.h"
#include "PsMutex.h"

using namespace physx;
using namespace physx::shdfnd;

PX_COMPILE_TIME_ASSERT(ErrorHandler::MAX_LISTENER_COUNT <= 31);

	namespace {
		PX_INLINE Foundation::Mutex& getMutex() { return getFoundation().getErrorMutex(); }
	}

ErrorHandler::ErrorHandler()
: mListenerCount(0)
, mCallbackBitmap(0)
{
	for( PxU32 i = 0; i < MAX_LISTENER_COUNT; i++ )
	{
		mErrorCallbacks[i] = NULL;
	}
}

ErrorHandler::~ErrorHandler()
{
	Foundation::Mutex::ScopedLock lock(getMutex());

	for( PxI32 i = 0; i < MAX_LISTENER_COUNT; i++ )
	{
		mErrorCallbacks[i] = NULL;
	}

	mListenerCount = 0;
}


void ErrorHandler::reportError(PxErrorCode::Enum code, const char* message, const char* file, int line)
{
	Foundation::Mutex::ScopedLock lock(getMutex());

	for( PxI32 i = 0; i < MAX_LISTENER_COUNT; i++ )
	{
		if( mErrorCallbacks[i] != NULL )
			mErrorCallbacks[i]->reportError( code, message, file, line );
	}
}


PxI32 ErrorHandler::registerErrorCallback( PxErrorCallback& callback )
{
	Foundation::Mutex::ScopedLock lock(getMutex());

	//Only support at most 31 listener
	PxU32 reserveBitmap = (~mCallbackBitmap) & 0x7FFFFFFF;

	PX_ASSERT(reserveBitmap != 0);

	//Use bitmap to find the empty listener 
	PxU32 callbackIdx = lowestSetBit(reserveBitmap);

	if( callbackIdx < MAX_LISTENER_COUNT )
	{
		mErrorCallbacks[callbackIdx] = &callback;
		mCallbackBitmap |= 1<<callbackIdx;
		mListenerCount++;
		return callbackIdx;
	}

	return -1;
}


void ErrorHandler::unRegisterErrorCallback( PxErrorCallback& callback )
{
	for( PxI32 i = 0; i < MAX_LISTENER_COUNT; i++ )
	{
		if( mErrorCallbacks[i] == &callback )
			unRegisterErrorCallback( i );
	}
}

void ErrorHandler::unRegisterErrorCallback( PxI32 index )
{
	if( mErrorCallbacks[index] != NULL )
	{
		Foundation::Mutex::ScopedLock lock(getMutex());

		mErrorCallbacks[index] = NULL;
		mCallbackBitmap &= ~(1<<index);

		mListenerCount--;
	}
}