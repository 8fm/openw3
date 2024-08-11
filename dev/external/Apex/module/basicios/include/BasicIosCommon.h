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

#ifndef __BASIC_IOS_COMMON_H__
#define __BASIC_IOS_COMMON_H__

#include "PxMat34Legacy.h"
#include "foundation/PxBounds3.h"
#include "foundation/PxVec3.h"
#include "InplaceTypes.h"

namespace physx
{
	namespace apex
	{
		class ApexCpuInplaceStorage;

		namespace basicios
		{

			struct InjectorParams
			{
				physx::PxF32 mLODMaxDistance;
				physx::PxF32 mLODDistanceWeight;
				physx::PxF32 mLODSpeedWeight;
				physx::PxF32 mLODLifeWeight;
				physx::PxF32 mLODBias;
				physx::PxU32 mLocalIndex;

#ifndef __CUDACC__
				template <typename R>
				void reflect(R& r)
				{
					r.reflect(mLODMaxDistance);
					r.reflect(mLODDistanceWeight);
					r.reflect(mLODSpeedWeight);
					r.reflect(mLODLifeWeight);
					r.reflect(mLODBias);
					r.reflect(mLocalIndex);
				}
#endif
			};

			typedef InplaceArray<InjectorParams> InjectorParamsArray;

			struct CollisionData
			{
				physx::PxVec3	bodyCMassPosition;
				physx::PxVec3	bodyLinearVelocity;
				physx::PxVec3	bodyAngluarVelocity;
				physx::PxF32	materialRestitution;

#ifndef __CUDACC__
				template <typename R>
				void reflect(R&)
				{
				}
#endif
			};

			struct CollisionSphereData : CollisionData
			{
				physx::PxBounds3 aabb;
				physx::PxMat34Legacy pose, inversePose;

				physx::PxF32 radius;
			};

			struct CollisionCapsuleData : CollisionData
			{
				physx::PxBounds3 aabb;
				physx::PxMat34Legacy pose, inversePose;

				physx::PxF32 halfHeight;
				physx::PxF32 radius;
			};

			struct CollisionBoxData : CollisionData
			{
				physx::PxBounds3 aabb;
				physx::PxMat34Legacy pose, inversePose;

				physx::PxVec3 halfSize;
			};

			struct CollisionHalfSpaceData : CollisionData
			{
				physx::PxVec3 normal;
				physx::PxVec3 origin;
			};

			struct CollisionConvexMeshData : CollisionData
			{
				physx::PxBounds3 aabb;
				physx::PxMat34Legacy pose, inversePose;

				PxU32 numPolygons;
				PxU32 firstPlane;
				PxU32 firstVertex;
				PxU32 polygonsDataOffset;
			};

			struct CollisionTriMeshData: CollisionData
			{
				physx::PxBounds3 aabb;
				physx::PxMat34Legacy pose, inversePose;

				PxU32 numTriangles;
				PxU32 firstIndex;
				PxU32 firstVertex;
			};

			struct SimulationParams
			{
				PxF32 collisionThreshold;
				PxF32 collisionDistance; 

				InplaceArray<CollisionBoxData> boxes;
				InplaceArray<CollisionSphereData> spheres;
				InplaceArray<CollisionCapsuleData> capsules;
				InplaceArray<CollisionHalfSpaceData> halfSpaces;
				InplaceArray<CollisionConvexMeshData> convexMeshes;
				InplaceArray<CollisionTriMeshData> trimeshes;

				PxPlane* convexPlanes;
				PxVec4*  convexVerts;
				PxU32*   convexPolygonsData;

				PxVec4*  trimeshVerts;
				PxU32*   trimeshIndices;

#ifndef __CUDACC__
				template<typename R> void reflect(R& r) 			
				{
					r.reflect(boxes);
					r.reflect(spheres);
					r.reflect(capsules);
					r.reflect(halfSpaces);
					r.reflect(convexMeshes);
					r.reflect(trimeshes);
				}
#endif
				SimulationParams(): convexPlanes(NULL), convexVerts(NULL), convexPolygonsData(NULL) {}
			};

			struct GridDensityParams
			{
				bool Enabled;
				physx::PxF32 GridSize;
				physx::PxU32 GridMaxCellCount;
				PxU32 GridResolution;
				physx::PxVec3 DensityOrigin;
				GridDensityParams(): Enabled(false) {}
			};
		
			struct GridDensityFrustumParams
			{
				PxReal nearDimX;
				PxReal farDimX;
				PxReal nearDimY;
				PxReal farDimY;
				PxReal dimZ; 
			};

#ifdef __CUDACC__

#define SIM_FETCH_PLANE(plane, name, idx) { float4 f4 = tex1Dfetch(KERNEL_TEX_REF(name), idx); plane = physx::PxPlane(f4.x, f4.y, f4.z, f4.w); }
#define SIM_FETCH(name, idx) tex1Dfetch(KERNEL_TEX_REF(name), idx)
#define SIM_FLOAT4 float4
#define SIM_INT_AS_FLOAT(x) __int_as_float(x)
#define SIM_MEM_TYPE const physx::PxU8*

			__device__ PX_INLINE physx::PxReal splitFloat4(physx::PxVec3& v3, const SIM_FLOAT4& f4)
			{
				v3.x = f4.x;
				v3.y = f4.y;
				v3.z = f4.z;
				return f4.w;
			}
			__device__ PX_INLINE SIM_FLOAT4 combineFloat4(const physx::PxVec3& v3, physx::PxReal w)
			{
				return make_float4(v3.x, v3.y, v3.z, w);
			}
#else

#define SIM_FETCH_PLANE(plane, name, idx) plane = mem##name[idx];
#define SIM_FETCH(name, idx) mem##name[idx]
#define SIM_FLOAT4 physx::PxVec4
#define SIM_INT_AS_FLOAT(x) *(const PxF32*)(&x)
#define SIM_MEM_TYPE ApexCpuInplaceStorage& 

			PX_INLINE physx::PxReal splitFloat4(physx::PxVec3& v3, const SIM_FLOAT4& f4)
			{
				v3 = f4.getXYZ();
				return f4.w;
			}
			PX_INLINE SIM_FLOAT4 combineFloat4(const physx::PxVec3& v3, physx::PxReal w)
			{
				return physx::PxVec4(v3.x, v3.y, v3.z, w);
			}

#endif
		}
	}
} // namespace physx::apex

#endif
