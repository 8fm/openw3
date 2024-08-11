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


#ifndef PX_PHYSICS_NP_PHYSICS_GPU
#define PX_PHYSICS_NP_PHYSICS_GPU

#include "PxPhysXCommonConfig.h"

#if PX_SUPPORT_GPU_PHYSX

#include "CmPhysXCommon.h"
#include "PsArray.h"

namespace physx
{
	class PxCudaContextManager;

	class NpPhysicsGpu
	{
	public:

		bool createTriangleMeshMirror(const class PxTriangleMesh& triangleMesh, PxCudaContextManager& contextManager);
		void releaseTriangleMeshMirror(const class PxTriangleMesh& triangleMesh, PxCudaContextManager* contextManager = NULL);
		
		bool createHeightFieldMirror(const class PxHeightField& heightField, PxCudaContextManager& contextManager);
		void releaseHeightFieldMirror(const class PxHeightField& heightField, PxCudaContextManager* contextManager = NULL);
		
		bool createConvexMeshMirror(const class PxConvexMesh& convexMesh, PxCudaContextManager& contextManager);
		void releaseConvexMeshMirror(const class PxConvexMesh& convexMesh, PxCudaContextManager* contextManager = NULL);

		void setExplicitCudaFlushCountHint(const class PxScene& scene, PxU32 cudaFlushCount);
		bool setTriangleMeshCacheSize(const class PxScene& scene, PxU32 size);
	
		NpPhysicsGpu(class NpPhysics& npPhysics);
		virtual ~NpPhysicsGpu();
	
	private:

		NpPhysicsGpu& operator=(const NpPhysicsGpu&);

		struct MeshMirror 
		{
			PxU32 mirrorHandle;
			PxCudaContextManager* ctx;
			const void* meshAddress;
		};
	
		Ps::Array<MeshMirror>::Iterator findMeshMirror(const void* meshAddress);
		Ps::Array<MeshMirror>::Iterator findMeshMirror(const void* meshAddress, PxCudaContextManager& ctx);
		class PxPhysXGpu* getPhysXGpu(bool createIfNeeded, const char* functionName, bool doWarn = true);
		bool checkNewMirror(const void* meshPtr, PxCudaContextManager& ctx, const char* functionName);
		bool checkMirrorExists(Ps::Array<MeshMirror>::Iterator it, const char* functionName);
		bool checkMirrorHandle(PxU32 mirrorHandle, const char* functionName);
		void addMeshMirror(const void* meshPtr, PxU32 mirrorHandle, PxCudaContextManager& ctx);
		void removeMeshMirror(const void* meshPtr, PxU32 mirrorHandle);
		void releaseMeshMirror(const void* meshPtr, const char* functionName, PxCudaContextManager* ctx);
	
		Ps::Array<MeshMirror> mMeshMirrors;
		class NpPhysics& mNpPhysics;
	};

}

#endif
#endif

