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


#ifndef PX_PHYSX_GPU_H
#define PX_PHYSX_GPU_H

#include "Pxg.h"
#include "Ps.h"
#include "PxSceneGpu.h"

namespace physx
{

class PxFoundation;
class PxCudaContextManager;

namespace Cm
{
	class EventProfiler;
	class RenderBuffer;
}

/**
\brief Interface to create and run CUDA enabled PhysX features.

The methods of this interface are expected not to be called concurrently. 
Also they are expected to not be called concurrently with any methods of PxSceneGpu and any tasks spawned before the end pipeline ... TODO make clear.
*/
class PxPhysXGpu
{
public:
	/**
	\brief Closes this instance of the interface.
	*/
	virtual		void					release() = 0;

	/**
	\brief Create a gpu scene instance.
	
	\param contextManager The PxCudaContextManager the scene is supposed to use.
	\param rigidBodyAccess The PxRigidBodyAccessGpu implementation the scene is supposed to use.
	\return pointer to gpu scene class (might be NULL on failure).
	*/
	virtual		class PxSceneGpu*		createScene(physx::PxCudaContextManager& contextManager, 
													class PxRigidBodyAccessGpu& rigidBodyAccess, 
													Cm::EventProfiler& eventProfiler,
													Cm::RenderBuffer& debugRenderBuffer) = 0;
	
	/**
	Mirror a triangle mesh onto the gpu memory corresponding to contextManager. Returns a handle for the mirrored mesh, PX_INVALID_U32 if failed.
	*/
	virtual		physx::PxU32			createTriangleMeshMirror(const class PxTriangleMesh& triangleMesh, physx::PxCudaContextManager& contextManager) = 0;
	
	/**
	Mirror a height field mesh onto the gpu memory corresponding to contextManager. Returns a handle for the mirrored mesh, PX_INVALID_U32 if failed.
	*/
	virtual		physx::PxU32			createHeightFieldMirror(const class PxHeightField& heightField, physx::PxCudaContextManager& contextManager) = 0;

	/**
	Mirror a convex mesh onto the gpu memory corresponding to contextManager. Returns a handle for the mirrored mesh, PX_INVALID_U32 if failed.
	*/
	virtual		physx::PxU32			createConvexMeshMirror(const class PxConvexMesh& convexMesh, physx::PxCudaContextManager& contextManager) = 0;

	/**
	Release a mesh mirror, providing the mirror handle. The mesh might still be mirrored implicitly if its in contact with particle systems.
	*/
	virtual		void					releaseMirror(physx::PxU32 mirrorHandle) = 0;

	/**
	Set the explicit count down counter to explicitly flush the cuda push buffer.
	*/
	virtual		void					setExplicitCudaFlushCountHint(const class PxgSceneGpu& scene, physx::PxU32 cudaFlushCount) = 0;

	/**
	Set the amount of memory for triangle mesh cache. Returns true if cache memory is sucessfully allocated, false otherwise.
	*/
	virtual		bool					setTriangleMeshCacheSize(const class PxgSceneGpu& scene, physx::PxU32 size) = 0;
};

}

/**
Create PxPhysXGpu interface class.
*/
PX_C_EXPORT PX_PHYSX_GPU_API physx::PxPhysXGpu* PX_CALL_CONV PxCreatePhysXGpu();

#endif // PX_PHYSX_GPU_H
