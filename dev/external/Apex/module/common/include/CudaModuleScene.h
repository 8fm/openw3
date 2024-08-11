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

#ifndef __CUDA_MODULE_SCENE__
#define __CUDA_MODULE_SCENE__

// Make this header is safe for inclusion in headers that are shared with device code.
#if !defined(__CUDACC__)

#include "NxApexDefs.h"
#if defined(APEX_CUDA_SUPPORT)
#include "NiApexScene.h"
#include "PsShare.h"
#include "PsArray.h"
#include <cuda.h>
#include "ApexCudaWrapper.h"

namespace physx
{
class PhysXGpuIndicator;
class PxGpuDispatcher;

namespace apex
{
class NxApexCudaTestManager;

/* Every CUDA capable NiModuleScene will derive this class.  It
 * provides the access methods to your CUDA kernels that were compiled
 * into object files by nvcc.
 */
class CudaModuleScene
{
public:
	CudaModuleScene(NiApexScene& scene, NxModule& module);
	virtual ~CudaModuleScene() {}

	void destroy(NiApexScene& scene);

	void*	getHeadCudaObj()
	{
		return cudaObjList.head();
	}

	physx::PxGpuDispatcher*	mGpuDispatcher;

	physx::PhysXGpuIndicator*		mPhysXGpuIndicator;

protected:
	CUmodule getCudaModule(int modIndex);

	void initCudaObj(ApexCudaTexRef& obj);
	void initCudaObj(ApexCudaVar& obj);
	void initCudaObj(ApexCudaFunc& obj);
	void initCudaObj(ApexCudaSurfRef& obj);

	physx::Array<CUmodule>			mCuModules;

	ApexCudaObjList cudaObjList;

	ApexCudaTestManager& mCudaTestManager;

	NxModule& mNxModule;

private:
	CudaModuleScene& operator=(const CudaModuleScene&);
};

}
} // namespace physx::apex

#endif
#endif

#endif // __CUDA_MODULE_SCENE__
