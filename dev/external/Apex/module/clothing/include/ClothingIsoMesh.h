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

#ifndef CLOTHING_ISO_MESH_H
#define CLOTHING_ISO_MESH_H

#include "NxClothingIsoMesh.h"
#include "PsUserAllocated.h"
#include "ApexInterface.h"

namespace physx
{
namespace apex
{
class ApexIsoMesh;
class ApexQuadricSimplifier;
class ApexMeshContractor;

namespace clothing
{
class ModuleClothing;



class ClothingIsoMesh : public NxClothingIsoMesh, public NxApexResource, public ApexResource
{
public:
	ClothingIsoMesh(ModuleClothing* module, const ApexIsoMesh& isoMesh, physx::PxU32 subdivision, NxResourceList* list);

	virtual physx::PxU32 getNumVertices() const;
	virtual physx::PxU32 getNumIndices() const;

	virtual void getVertices(void* vertexDestination, physx::PxU32 byteStride);
	virtual void getIndices(void* indexDestination, physx::PxU32 byteStride);

	virtual void release();
	void destroy();

	virtual physx::PxU32 simplify(physx::PxU32 subdivisions, physx::PxI32 maxSteps, physx::PxF32 maxError, IProgressListener* progress);
	virtual physx::PxF32 contract(physx::PxI32 steps, physx::PxF32 abortionRatio, bool expand, IProgressListener* progress);
	virtual NxClothingPhysicalMesh*	generateClothMesh(physx::PxU32 bubbleSizeToRemove, IProgressListener* progress);
	virtual NxClothingPhysicalMesh*	generateSoftbodyMesh(physx::PxU32 tetraSubdivision, IProgressListener* progress);

	// from ApexResource
	physx::PxU32					getListIndex() const
	{
		return m_listIndex;
	}
	void							setListIndex(class NxResourceList& list, physx::PxU32 index)
	{
		m_list = &list;
		m_listIndex = index;
	}

private:
	void writeBackData();

	void initContractor(IProgressListener* progress);
	void writeBackContractor(bool clearItAsWell);
	void initSimplifier(IProgressListener* progress);
	void writeBackSimplifier(bool clearItAsWell);

	ModuleClothing* mModule;

	physx::PxU32 mSubdivision;
	physx::PxBounds3 mBound;

	physx::PxU32 mNumVertices;
	physx::PxU32 mVertexCapacity;
	physx::PxVec3* mVertices;
	physx::PxU32 mNumIndices;
	physx::PxU32 mIndexCapacity;
	physx::PxU32* mIndices;

	ApexQuadricSimplifier* mSimplifier;
	ApexMeshContractor* mContractor;

	physx::PxU32 bDirty : 1;
};


}
} // namespace apex
} // namespace physx

#endif // CLOTHING_ISO_MESH_H