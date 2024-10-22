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

#include "PsFoundation.h"

#if SUPPORT_UPATE_LOADER_LOGGING
#ifdef PX_X86
#define NX_USE_SDK_DLLS
#include "PhysXUpdateLoader.h"
#endif
#endif /* SUPPORT_UPATE_LOADER_LOGGING */

#include "windows/PsWindowsModuleUpdateLoader.h"

namespace physx { namespace shdfnd {

static HMODULE WINAPI PxLoadLibraryA(const char* name)
{
#ifdef PX_WINMODERN
	// convert to unicode
	const static int BUFFERSIZE = 256;
	WCHAR buffer[BUFFERSIZE];
	int succ = MultiByteToWideChar(CP_ACP, 0, name, -1, buffer, BUFFERSIZE);

	// validate
	if (succ < 0)
		succ = 0;
	if (succ < BUFFERSIZE)
		buffer[succ] = 0;
	else if (buffer[BUFFERSIZE-1])
		buffer[0] = 0;

	return (succ > 0) ? LoadPackagedLibrary(buffer, 0) : NULL;
#else
	return LoadLibraryA( name );
#endif
}

typedef HMODULE (*GetUpdatedModule_FUNC)(const char*, const char*);

#if SUPPORT_UPATE_LOADER_LOGGING
#ifdef PX_X86
typedef void (*setLogging_FUNC)(PXUL_ErrorCode, pt2LogFunc);

static void LogMessage(PXUL_ErrorCode messageType, char* message)
{
	switch(messageType)
	{
	case PXUL_ERROR_MESSAGES:
		getFoundation().error(PxErrorCode::eINTERNAL_ERROR, __FILE__, __LINE__, 
			"PhysX Update Loader Error: %s.", message);
		break;
	case PXUL_WARNING_MESSAGES:
		getFoundation().error(PX_WARN, "PhysX Update Loader Warning: %s.", message);
		break;
	case PXUL_INFO_MESSAGES:
		getFoundation().error(PX_INFO, "PhysX Update Loader Information: %s.", message);
		break;
	default:
		getFoundation().error(PxErrorCode::eINTERNAL_ERROR, __FILE__, __LINE__,
			"Unknown message type from update loader.");
		break;
	}
}
#endif
#endif /* SUPPORT_UPATE_LOADER_LOGGING */

PsModuleUpdateLoader::PsModuleUpdateLoader(const char* updateLoaderDllName)
	: mGetUpdatedModuleFunc(NULL)
{
	mUpdateLoaderDllHandle = PxLoadLibraryA(updateLoaderDllName);

	if (mUpdateLoaderDllHandle != NULL)
	{
		mGetUpdatedModuleFunc = GetProcAddress(mUpdateLoaderDllHandle, "GetUpdatedModule");

#if SUPPORT_UPATE_LOADER_LOGGING
#ifdef PX_X86
		setLogging_FUNC setLoggingFunc;
		setLoggingFunc = (setLogging_FUNC)GetProcAddress(mUpdateLoaderDllHandle, "setLoggingFunction");
		if(setLoggingFunc != NULL)		
		{
           setLoggingFunc(PXUL_ERROR_MESSAGES, LogMessage);
        }
#endif
#endif /* SUPPORT_UPATE_LOADER_LOGGING */
	}
}

PsModuleUpdateLoader::~PsModuleUpdateLoader()
{
	if (mUpdateLoaderDllHandle != NULL)
	{
		FreeLibrary(mUpdateLoaderDllHandle);
		mUpdateLoaderDllHandle = NULL;
	}
}

HMODULE PsModuleUpdateLoader::LoadModule(const char* moduleName, const char* appGUID)
{
	HMODULE result = NULL;

	if (mGetUpdatedModuleFunc != NULL)
	{
		// Try to get the module through PhysXUpdateLoader
		GetUpdatedModule_FUNC getUpdatedModuleFunc = (GetUpdatedModule_FUNC)mGetUpdatedModuleFunc;
		result = getUpdatedModuleFunc(moduleName, appGUID);
	}
	else
	{
		// If no PhysXUpdateLoader, just load the DLL directly
		result = PxLoadLibraryA(moduleName);
	}

	return result;
}

}; // end of namespace
}; // end of namespace
