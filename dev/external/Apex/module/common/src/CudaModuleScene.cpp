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

#include "NxApexDefs.h"
#if defined(APEX_CUDA_SUPPORT)

#include "NxApex.h"
#include "NiApexSDK.h"
#include "NiApexScene.h"
#include "ApexCutil.h"
#include "CudaModuleScene.h"
#include <cuda.h>
#include <texture_types.h>

#include "PxTaskManager.h"
#include "PxGpuDispatcher.h"
#include "PxCudaContextManager.h"

namespace physx
{
class PhysXGpuIndicator;

namespace apex
{

/**
 * Workaround hacks for using nvcc --compiler output object files
 * without linking with CUDART.  We must implement our own versions
 * of these functions that the object files are hard-coded to call into.
 */

#define MAX_MODULES					64
static void* moduleTable[ MAX_MODULES ];
static int numRegisteredModules = 0;

#define MAX_FUNCTIONS				256
typedef struct
{
	int         modIndex;
	const char* funcName;
} cuFuncDesc;
static cuFuncDesc functionTable[ MAX_FUNCTIONS ];
static int numRegisteredFunctions = 0;

const char* funcNameTable[ MAX_FUNCTIONS ];


#define MAX_TEXTURES				256
typedef struct
{
	int                             modIndex;
	const char*                     texRefName;
	const struct textureReference*  texRefData;
	int                             dim;
	int                             read_normalized_float;
} cuTexRefDesc;
static cuTexRefDesc textureTable[ MAX_TEXTURES ];
static int numRegisteredTextures = 0;


#define MAX_SURFACES				256
typedef struct
{
	int                             modIndex;
	const char*                     surfRefName;
	const struct surfaceReference*  surfRefData;
	int                             dim;
} cuSurfRefDesc;
static cuSurfRefDesc surfaceTable[ MAX_SURFACES ];
static int numRegisteredSurfaces = 0;


#define MAX_VARIABLES				256
typedef struct
{
	int			modIndex;
	const char* varName;
	int			size;
} cuVarDesc;
static cuVarDesc variableTable[ MAX_VARIABLES ];
static int numRegisteredVariables = 0;

CudaModuleScene::CudaModuleScene(NiApexScene& scene, NxModule& module
#if OLD_CUDA_FUNC_REGISTRATION
                                 , const char** funcNames, physx::PxU32 numFuncs
#endif
                                )
	: mCudaTestManager(scene.getApexCudaTestManager())
	, mNxModule(module)
{
	physx::PxTaskManager* tm = scene.getTaskManager();
	physx::PxGpuDispatcher* gd = tm->getGpuDispatcher();
	PX_ASSERT(gd != NULL);
	mGpuDispatcher = gd;

	physx::PxScopedCudaLock _lock_(*mGpuDispatcher->getCudaContextManager());

	/* Formally load the CUDA modules, get CUmodule handles */
	mCuModules.resize(numRegisteredModules);

	NiApexSDK* apexSdk = NiGetApexSDK();
	mPhysXGpuIndicator = apexSdk->registerPhysXIndicatorGpuClient();
}

void CudaModuleScene::destroy(NiApexScene&)
{
	if (mCuModules.empty() == false)
	{
		physx::PxScopedCudaLock _lock_(*mGpuDispatcher->getCudaContextManager());

		cudaObjList.releaseAll();

		for (physx::PxU32 i = 0 ; i < mCuModules.size() ; i++)
		{
			if (mCuModules[i] != (CUmodule)0)
			{
				CUT_SAFE_CALL(cuModuleUnload(mCuModules[i]));
			}
		}
	}

	NiApexSDK* apexSdk = NiGetApexSDK();
	apexSdk->unregisterPhysXIndicatorGpuClient(mPhysXGpuIndicator);
	mPhysXGpuIndicator = NULL;
}

CUmodule CudaModuleScene::getCudaModule(int modIndex)
{
	if (mCuModules[modIndex] == (CUmodule)0)
	{
#if APEX_EXPERIMENT_WITH_PTX
		CUjit_option jitOption					= CU_JIT_FALLBACK_STRATEGY;
		CUjit_fallback_enum fallbackStrategy[1]	= {CU_PREFER_PTX};

		CUT_SAFE_CALL(cuModuleLoadDataEx(&mCuModules[modIndex], moduleTable[modIndex], 1, &jitOption, (void**)&fallbackStrategy));
#else
		CUT_SAFE_CALL(cuModuleLoadDataEx(&mCuModules[modIndex], moduleTable[modIndex], 0, NULL, NULL));
#endif
	}
	return mCuModules[modIndex];
}

void CudaModuleScene::initCudaObj(ApexCudaTexRef& texRef)
{
	const char* texRefName = texRef.getName();

	for (int j = 0 ; j < numRegisteredTextures ; j++)
	{
		if (strcmp(textureTable[j].texRefName, texRefName) == 0)
		{
			CUmodule cuModule = getCudaModule(textureTable[j].modIndex);
			PX_ASSERT(cuModule != (CUmodule)0);

			CUtexref cuTexRef;
			CUT_SAFE_CALL(cuModuleGetTexRef(&cuTexRef, cuModule, texRefName));

			const struct textureReference* texRefData = textureTable[j].texRefData;

			PX_ASSERT(texRefData->channelDesc.x > 0);
			int numChannels = 1;
			if (texRefData->channelDesc.y > 0)
			{
				PX_ASSERT(texRefData->channelDesc.y == texRefData->channelDesc.x);
				++numChannels;
			}
			if (texRefData->channelDesc.z > 0)
			{
				PX_ASSERT(texRefData->channelDesc.z == texRefData->channelDesc.x);
				++numChannels;
			}
			if (texRefData->channelDesc.w > 0)
			{
				PX_ASSERT(texRefData->channelDesc.w == texRefData->channelDesc.x);
				++numChannels;
			}

			CUarray_format cuFormat = CUarray_format(0);
			switch (texRefData->channelDesc.f)
			{
			case cudaChannelFormatKindSigned:
				switch (texRefData->channelDesc.x)
				{
				case  8:
					cuFormat = CU_AD_FORMAT_SIGNED_INT8;
					break;
				case 16:
					cuFormat = CU_AD_FORMAT_SIGNED_INT16;
					break;
				case 32:
					cuFormat = CU_AD_FORMAT_SIGNED_INT32;
					break;
				}
				break;
			case cudaChannelFormatKindUnsigned:
				switch (texRefData->channelDesc.x)
				{
				case  8:
					cuFormat = CU_AD_FORMAT_UNSIGNED_INT8;
					break;
				case 16:
					cuFormat = CU_AD_FORMAT_UNSIGNED_INT16;
					break;
				case 32:
					cuFormat = CU_AD_FORMAT_UNSIGNED_INT32;
					break;
				}
				break;
			case cudaChannelFormatKindFloat:
				cuFormat = CU_AD_FORMAT_FLOAT;
				break;
			};
			PX_ASSERT(cuFormat != 0);

			int cuFlags = 0;
			if (textureTable[j].read_normalized_float == 0)
			{
				cuFlags |= CU_TRSF_READ_AS_INTEGER;
			}
			if (textureTable[j].texRefData->normalized != 0)
			{
				cuFlags |= CU_TRSF_NORMALIZED_COORDINATES;
			}

			texRef.init(cudaObjList, cuTexRef, cuModule, cuFormat, numChannels, textureTable[j].dim, cuFlags);
			break;
		}
	}
}

void CudaModuleScene::initCudaObj(ApexCudaVar& var)
{
	const char* varName = var.getName();

	for (int j = 0 ; j < numRegisteredVariables ; j++)
	{
		if (strcmp(variableTable[j].varName, varName) == 0)
		{
			CUmodule cuModule = getCudaModule(variableTable[j].modIndex);
			PX_ASSERT(cuModule != (CUmodule)0);

			CUdeviceptr cuDevPtr;
			size_t size;
			cuModuleGetGlobal(&cuDevPtr, &size, cuModule, varName);

			var.init(cudaObjList, mGpuDispatcher->getCudaContextManager(), cuDevPtr, cuModule, size);
			break;
		}
	}
}

void CudaModuleScene::initCudaObj(ApexCudaFunc& func)
{
	const char* funcName = func.getName();

	for (int j = 0 ; j < numRegisteredFunctions ; j++)
	{
		if (strcmp(functionTable[j].funcName, funcName) == 0)
		{
			CUmodule cuModule = getCudaModule(functionTable[j].modIndex);
			PX_ASSERT(cuModule != (CUmodule)0);

			CUfunction cuFunc = 0;
			CUT_SAFE_CALL(cuModuleGetFunction(&cuFunc, cuModule, funcName));

			PxU16 id = mGpuDispatcher->registerKernelNames( &funcName, 1 );
			func.init(cudaObjList, mGpuDispatcher, cuFunc, cuModule, id, &mNxModule, &mCudaTestManager);
			break;
		}
	}
}

void CudaModuleScene::initCudaObj(ApexCudaSurfRef& surfRef)
{
	if (mGpuDispatcher->getCudaContextManager()->supportsArchSM20() == false)
	{
		return;
	}

	const char* surfRefName = surfRef.getName();

	for (int j = 0 ; j < numRegisteredSurfaces ; j++)
	{
		if (strcmp(surfaceTable[j].surfRefName, surfRefName) == 0)
		{
			CUmodule cuModule = getCudaModule(surfaceTable[j].modIndex);
			PX_ASSERT(cuModule != (CUmodule)0);

			CUsurfref cuSurfRef;
			CUT_SAFE_CALL(cuModuleGetSurfRef(&cuSurfRef, cuModule, surfRefName));

			surfRef.init(cudaObjList, cuSurfRef, cuModule);
			break;
		}
	}

}

/*
 * These calls are all made _before_ main() during static initialization
 * of your APEX module.  So calling into APEX Framework or other
 * external code modules is out of the question.
 */

#include "driver_types.h"

#define CUDARTAPI __stdcall

typedef struct uint3_t
{
	unsigned int x, y, z;
} uint3;

typedef struct dim3_t
{
	unsigned int x, y, z;
} dim3;

extern "C"
void** CUDARTAPI __cudaRegisterFatBinary(void* fatBin)
{
	//HACK to get real fatbin in CUDA 4.0
	struct CUIfatbinStruct
	{
		int magic;
		int version;
		void* fatbinArray;
		char* fatbinFile;
	};
	const CUIfatbinStruct* fatbinStruct = (const CUIfatbinStruct*)fatBin;
	if (fatbinStruct->magic == 0x466243B1)
	{
		fatBin = fatbinStruct->fatbinArray;
	}

	if (numRegisteredModules < MAX_MODULES)
	{
		moduleTable[ numRegisteredModules ] = fatBin;
		return (void**)(size_t) numRegisteredModules++;
	}
	return NULL;
}

extern "C"
void CUDARTAPI __cudaUnregisterFatBinary(void** fatCubinHandle)
{
	moduleTable[(int)(size_t) fatCubinHandle ] = 0;
}

extern "C"
void CUDARTAPI __cudaRegisterTexture(
    void**                    fatCubinHandle,
    const struct textureReference*  hostvar,
    const void**                    deviceAddress,
    const char*                     deviceName,
    int                       dim,
    int                       read_normalized_float,
    int                       ext)
{
	PX_UNUSED(fatCubinHandle);
	PX_UNUSED(hostvar);
	PX_UNUSED(deviceAddress);
	PX_UNUSED(deviceName);
	PX_UNUSED(dim);
	PX_UNUSED(read_normalized_float);
	PX_UNUSED(ext);

	if (numRegisteredTextures < MAX_TEXTURES)
	{
		//Fix for CUDA 5.5 - remove leading "::"
		while (*deviceName == ':')
		{
			++deviceName;
		}

		// We need this association of function to module in order to find textures and globals
		textureTable[ numRegisteredTextures ].modIndex = (int)(size_t) fatCubinHandle;
		textureTable[ numRegisteredTextures ].texRefName = deviceName;
		textureTable[ numRegisteredTextures ].texRefData = hostvar;
		textureTable[ numRegisteredTextures ].dim = dim;
		textureTable[ numRegisteredTextures ].read_normalized_float = read_normalized_float;
		numRegisteredTextures++;
	}
}

extern "C"
void CUDARTAPI __cudaRegisterSurface(
    void**                          fatCubinHandle,
    const struct surfaceReference*        hostvar,
    const void**                          deviceAddress,
    const char*                           deviceName,
    int                             dim,
    int                             ext)
{
	PX_UNUSED(fatCubinHandle);
	PX_UNUSED(hostvar);
	PX_UNUSED(deviceAddress);
	PX_UNUSED(deviceName);
	PX_UNUSED(dim);
	PX_UNUSED(ext);

	if (numRegisteredSurfaces < MAX_SURFACES)
	{
		surfaceTable[ numRegisteredSurfaces ].modIndex = (int)(size_t) fatCubinHandle;
		surfaceTable[ numRegisteredSurfaces ].surfRefName = deviceName;
		surfaceTable[ numRegisteredSurfaces ].surfRefData = hostvar;
		surfaceTable[ numRegisteredSurfaces ].dim = dim;
		numRegisteredSurfaces++;
	}
}

extern "C" void CUDARTAPI __cudaRegisterVar(
    void**                    fatCubinHandle,
    char*                    hostVar,
    char*                    deviceAddress,
    const char*                    deviceName,
    int                      ext,
    int                      size,
    int                      constant,
    int                      global)
{
	PX_UNUSED(fatCubinHandle);
	PX_UNUSED(hostVar);
	PX_UNUSED(deviceAddress);
	PX_UNUSED(deviceName);
	PX_UNUSED(ext);
	PX_UNUSED(size);
	PX_UNUSED(constant);
	PX_UNUSED(global);

	if (constant != 0 && numRegisteredVariables < MAX_VARIABLES)
	{
		variableTable[ numRegisteredVariables ].modIndex = (int)(size_t) fatCubinHandle;
		variableTable[ numRegisteredVariables ].varName = deviceName;
		variableTable[ numRegisteredVariables ].size = size;
		numRegisteredVariables++;
	}
}


extern "C" void CUDARTAPI __cudaRegisterShared(
    void**                    fatCubinHandle,
    void**                    devicePtr
)
{
	PX_UNUSED(fatCubinHandle);
	PX_UNUSED(devicePtr);
}



extern "C"
void CUDARTAPI __cudaRegisterFunction(
    void**  fatCubinHandle,
    const char*   hostFun,
    char*   deviceFun,
    const char*   deviceName,
    int     thread_limit,
    uint3*  tid,
    uint3*  bid,
    dim3*   bDim,
    dim3*   gDim,
    int*    wSize)
{
	PX_UNUSED(hostFun);
	PX_UNUSED(deviceFun);
	PX_UNUSED(thread_limit);
	PX_UNUSED(tid);
	PX_UNUSED(bid);
	PX_UNUSED(bDim);
	PX_UNUSED(gDim);
	PX_UNUSED(wSize);

	if (numRegisteredFunctions < MAX_FUNCTIONS)
	{
		// We need this association of function to module in order to find textures and globals
		functionTable[ numRegisteredFunctions ].modIndex = (int)(size_t) fatCubinHandle;
		functionTable[ numRegisteredFunctions ].funcName = deviceName;
		funcNameTable[ numRegisteredFunctions ] = deviceName;
		numRegisteredFunctions++;
	}
}

/* These functions are implemented just to resolve link dependencies */

extern "C"
cudaError_t CUDARTAPI cudaLaunch(const char* entry)
{
	PX_UNUSED(entry);
	return cudaSuccess;
}

extern "C"
cudaError_t CUDARTAPI cudaSetupArgument(
    const void*   arg,
    size_t  size,
    size_t  offset)
{
	PX_UNUSED(arg);
	PX_UNUSED(size);
	PX_UNUSED(offset);
	return cudaSuccess;
}

extern "C"
struct cudaChannelFormatDesc CUDARTAPI cudaCreateChannelDesc(
    int x, int y, int z, int w, enum cudaChannelFormatKind f)
{
	struct cudaChannelFormatDesc desc;
	desc.x = x;
	desc.y = y;
	desc.z = z;
	desc.w = w;
	desc.f = f;
	return desc;
}

}
} // namespace physx::apex

#endif
