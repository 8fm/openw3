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

#ifndef CLOTHING_PHYSICAL_MESH_H
#define CLOTHING_PHYSICAL_MESH_H

#include "NxClothingPhysicalMesh.h"
#include "PsUserAllocated.h"
#include "ApexInterface.h"
#include "PsArray.h"
#include "NxParamArray.h"
#include "ClothingPhysicalMeshParameters.h"
#include "ModuleClothing.h"

#include "NxParameterized.h"

namespace physx
{
namespace apex
{
class ApexQuadricSimplifier;

namespace clothing
{
class ModuleClothing;


class ClothingPhysicalMesh : public NxClothingPhysicalMesh, public NxApexResource, public ApexResource, public NxParameterized::SerializationCallback
{
private:
	ClothingPhysicalMesh(ModuleClothing* module, ClothingPhysicalMeshParameters* params, NxResourceList* list);
	friend class ModuleClothing;

public:
	virtual void release();
	void destroy();

	virtual PxU32 getNumVertices() const;
	virtual PxU32 getNumIndices() const;
	virtual PxU32 getNumBonesPerVertex() const
	{
		return mParams->physicalMesh.numBonesPerVertex;
	}

	virtual void getIndices(void* indexDestination, PxU32 byteStride, PxU32 numIndices) const;

	virtual bool isTetrahedralMesh() const
	{
		return mParams->physicalMesh.isTetrahedralMesh;
	}

	virtual void simplify(PxU32 subdivisions, PxI32 maxSteps, PxF32 maxError, IProgressListener* progress);

	// user overwrites geometry
	virtual void setGeometry(bool tetraMesh, PxU32 numVertices, PxU32 vertexByteStride, const void* vertices, PxU32 numIndices, PxU32 indexByteStride, const void* indices);
	void setGeometryInternal(bool tetraMesh, PxU32 numVertices, PxU32 vertexByteStride, const void* vertices, const PxU32* masterFlags, PxU32 numIndices, PxU32 indexByteStride, const void* indices);

	// direct access to specific buffers
	virtual bool getIndices(PxU32* indices, PxU32 byteStride) const;
	virtual bool getVertices(PxVec3* vertices, PxU32 byteStride) const ;
	virtual bool getNormals(PxVec3* vertices, PxU32 byteStride) const ;
	virtual bool getBoneIndices(PxU16* boneIndices, PxU32 byteStride) const;
	virtual bool getBoneWeights(PxF32* boneWeights, PxU32 byteStride) const;
	virtual bool getConstrainCoefficients(NxClothingConstrainCoefficients* values, PxU32 byteStride) const;
	virtual void getStats(NxClothingPhysicalMeshStats& stats) const;

	// from NxParameterized::SerializationCallback
	void preSerialize(void* userData = NULL);

	void permuteBoneIndices(Array<PxI32>& old2newBoneIndices);
	void applyTransformation(const PxMat34Legacy& transformation, PxF32 scale);
	void applyPermutation(const Array<PxU32>& permutation);

	void makeCopy(ClothingPhysicalMeshParameters* params);
	void allocateNormalBuffer();
	void allocateSkinningNormalsBuffer();
	void allocateConstrainCoefficientBuffer();
	void allocateBoneIndexAndWeightBuffers();
	void freeAdditionalBuffers();
	PX_INLINE void setNumBonesPerVertex(PxU32 numBonesPerVertex)
	{
		mParams->physicalMesh.numBonesPerVertex = numBonesPerVertex;
	}

	PX_INLINE ClothingPhysicalMeshParametersNS::ConstrainCoefficient_Type* getConstrainCoefficientsBuffer()
	{
		return mParams->physicalMesh.constrainCoefficients.buf;
	}
	PX_INLINE PxU16* getBoneIndicesBuffer()
	{
		return mParams->physicalMesh.boneIndices.buf;
	}
	PX_INLINE PxF32* getBoneWeightsBuffer()
	{
		return mParams->physicalMesh.boneWeights.buf;
	}
	PX_INLINE PxVec3* getPositionBuffer()
	{
		return mParams->physicalMesh.vertices.buf;
	}
	PX_INLINE PxVec3* getNormalBuffer()
	{
		return mParams->physicalMesh.normals.buf;
	}
	PX_INLINE PxVec3* getSkinningNormalBuffer()
	{
		return mParams->physicalMesh.skinningNormals.buf;
	}
	PX_INLINE PxU32* getMasterFlagsBuffer()
	{
		return mMasterFlags.begin();
	}

	PX_INLINE PxU32* getIndicesBuffer()
	{
		return mParams->physicalMesh.indices.buf;
	}

	PX_INLINE PxF32 getMaxMaxDistance()
	{
		return mParams->physicalMesh.maximumMaxDistance;
	}
	void updateMaxMaxDistance();

	void addBoneToVertex(PxU32 vertexNumber, PxU16 boneIndex, PxF32 boneWeight);
	void sortBonesOfVertex(PxU32 vertexNumber);
	void normalizeBonesOfVertex(PxU32 vertexNumber);

	void updateSkinningNormals();
	void smoothNormals(PxU32 numIterations);

	void updateOptimizationData();

	ClothingPhysicalMeshParameters* getNxParameterized() const
	{
		return mParams;
	}

	// from ApexResource
	PxU32	getListIndex() const
	{
		return m_listIndex;
	}
	void	setListIndex(class NxResourceList& list, PxU32 index)
	{
		m_list = &list;
		m_listIndex = index;
	}

private:
	void writeBackData();
	void clearMiscBuffers();
	void computeEdgeLengths() const;

	bool removeDuplicatedTriangles(PxU32 numIndices, PxU32 indexByteStride, const void* indices);
	void computeNeighborInformation(Array<PxI32> &neighbors);
	void fixTriangleOrientations();

	ModuleClothing* mModule;

	ClothingPhysicalMeshParameters* mParams;
	bool ownsParams;

	NxParamArray<PxVec3> mVertices;
	NxParamArray<PxVec3> mNormals;
	NxParamArray<PxVec3> mSkinningNormals;
	NxParamArray<ClothingPhysicalMeshParametersNS::ConstrainCoefficient_Type> mConstrainCoefficients;
	NxParamArray<PxU16> mBoneIndices;
	NxParamArray<PxF32> mBoneWeights;
	NxParamArray<PxU32> mIndices;

	Array<PxU32> mMasterFlags;

	ApexQuadricSimplifier* mSimplifier;
	bool isDirty;
};

}
} // namespace apex
} // namespace physx

#endif // CLOTHING_PHYSICAL_MESH_H
