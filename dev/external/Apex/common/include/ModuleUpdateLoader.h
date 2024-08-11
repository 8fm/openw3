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

#ifndef MODULEUPDATELOADER_H
#define MODULEUPDATELOADER_H

#ifdef WIN32

#ifdef PX_X64
#define UPDATE_LOADER_DLL_NAME "PhysXUpdateLoader64.dll"
#else
#define UPDATE_LOADER_DLL_NAME "PhysXUpdateLoader.dll"
#endif

// This GUID should change any time we release APEX with a public interface change.
#if NX_SDK_VERSION_MAJOR == 2
#define DEFAULT_APP_GUID "DF5B1644-4FBF-4462-9481-E4DEFA8DF926"
#else
#define DEFAULT_APP_GUID "37AFBC25-6FAF-4436-86A3-B6C71655565C"
#endif

#include "PsWindowsInclude.h"

class ModuleUpdateLoader
{
public:
	ModuleUpdateLoader(const char* updateLoaderDllName);
	~ModuleUpdateLoader();

	// Loads the given module through the update loader. Loads it from the path if 
	// the update loader doesn't find the requested module. Returns NULL if no
	// module found.
	HMODULE loadModule(const char* moduleName, const char* appGuid);

protected:
	HMODULE mUpdateLoaderDllHandle;
	FARPROC mGetUpdatedModuleFunc;

	// unit test fixture
	friend class ModuleUpdateLoaderTest;
};

#endif	// WIN32
#endif	// MODULEUPDATELOADER_H
