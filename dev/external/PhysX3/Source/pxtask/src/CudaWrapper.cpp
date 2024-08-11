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

#include "Px.h"
#include "PsFoundation.h"
#include "PxPhysics.h"

#if defined(PX_WINDOWS) && !defined(PX_WINMODERN)
#include "windows/PsWindowsInclude.h"
#include "windows/PsWindowsModuleUpdateLoader.h"
#include "PxCudaContextManager.h"
#endif // PX_WINDOWS

#ifdef PX_PHYSX_DLL_NAME_POSTFIX
#define STRINGIFY(x) #x
#define GETSTRING(x) STRINGIFY(x)
#ifdef PX_WINDOWS
#ifdef PX_X86
static const char*	gPhysXGpuLibraryName = "PhysX3Gpu" GETSTRING(PX_PHYSX_DLL_NAME_POSTFIX) "_x86.dll";
#elif defined(PX_X64)
static const char*	gPhysXGpuLibraryName = "PhysX3Gpu" GETSTRING(PX_PHYSX_DLL_NAME_POSTFIX) "_x64.dll";
#endif
#endif  // PX_WINDOWS
#undef GETSTRING
#undef STRINGIFY
#else
#ifdef PX_WINDOWS
#ifdef PX_X86
static const char*	gPhysXGpuLibraryName = "PhysX3Gpu_x86.dll";
#elif defined(PX_X64)
static const char*	gPhysXGpuLibraryName = "PhysX3Gpu_x64.dll";
#endif
#endif  // PX_WINDOWS
#endif

namespace physx
{
	class PxProfileZoneManager;
	class PxFoundation;
	class PxErrorCallback;

	class PxCudaContextManager;
	class PxCudaContextManagerDesc;

	typedef int (getOrdinal_FUNC) ( PxErrorCallback& errc );
	typedef PxCudaContextManager* (createContext_FUNC) ( PxFoundation& foundation, const PxCudaContextManagerDesc& desc, physx::PxProfileZoneManager* mgr );
	typedef void (setFoundation_FUNC) ( PxFoundation& foundation );

	static getOrdinal_FUNC*     s_getSuggestedCudaDeviceOrdinalFunc;
	static createContext_FUNC*  s_createCudaContextManagerFunc;
	static setFoundation_FUNC*	s_setFoundationFunc;

#if defined(PX_WINDOWS) && !defined(PX_WINMODERN)
	static HMODULE              s_library;

#define DEFAULT_PHYSX_GPU_GUID    "87831852-86F1-4d65-B1F9-C0B24E435096"
	void* PxLoadPhysxGPUModule(const char* appGUID)
	{	
		if( s_library == NULL )
		 s_library = GetModuleHandle(gPhysXGpuLibraryName);
		
		 if(s_library == NULL)
		 {
#if defined(PX_DEBUG) || defined(PX_CHECKED) || defined(PX_PROFILE)
			PX_UNUSED(appGUID);
			s_library = LoadLibrary(gPhysXGpuLibraryName);
#else
			shdfnd::PsModuleUpdateLoader moduleLoader(UPDATE_LOADER_DLL_NAME);
			s_library = moduleLoader.LoadModule(gPhysXGpuLibraryName, appGUID == NULL ? DEFAULT_PHYSX_GPU_GUID : appGUID );
#endif
		 }

		if( s_library )
		{
			s_createCudaContextManagerFunc = (createContext_FUNC*) GetProcAddress( s_library, "createCudaContextManagerDLL" );
			s_getSuggestedCudaDeviceOrdinalFunc = (getOrdinal_FUNC*) GetProcAddress( s_library, "getSuggestedCudaDeviceOrdinalDLL" );
			s_setFoundationFunc = (setFoundation_FUNC*) GetProcAddress( s_library, "setFoundationInstance" );
		}

		return s_library;
	}
#else
	void* PxLoadPhysxGPUModule() { return NULL; }
#endif
		
	int PxGetSuggestedCudaDeviceOrdinal(PxErrorCallback& errc)
	{
		if( !s_getSuggestedCudaDeviceOrdinalFunc )
			PxLoadPhysxGPUModule();

		if( s_getSuggestedCudaDeviceOrdinalFunc )
			return s_getSuggestedCudaDeviceOrdinalFunc(errc);
		else
			return -1;
	}

	PxCudaContextManager* PxCreateCudaContextManager(PxFoundation& foundation, const PxCudaContextManagerDesc& desc, physx::PxProfileZoneManager* mgr)
	{
		if( !s_createCudaContextManagerFunc )
		{
#if defined(PX_WINDOWS) && !defined(PX_WINMODERN)
			PxLoadPhysxGPUModule(desc.appGUID);
#else
			PxLoadPhysxGPUModule();
#endif
		}

		if (s_setFoundationFunc)
			s_setFoundationFunc( foundation );

		if( s_createCudaContextManagerFunc )
			return s_createCudaContextManagerFunc( foundation, desc, mgr );
		else
			return NULL;
	}

} // end physx namespace

