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

#ifndef CLOTHING_ASSET_DATA
#define CLOTHING_ASSET_DATA

#include "foundation/PxSimpleTypes.h"
#include "foundation/PxAssert.h"
#include "NxAbstractMeshDescription.h"
#include "NxApexRenderDataFormat.h"
#include "foundation/PxBounds3.h"
#ifndef __SPU__
#include "PsAllocator.h"
#include "NxRenderMeshAsset.h"
#endif

#include "PlatformMemory.h"

namespace physx
{
namespace apex
{

namespace clothing
{

// forward declarations
namespace ClothingGraphicalLodParametersNS
{
struct SkinClothMapB_Type;
struct SkinClothMapD_Type;
struct TetraLink_Type;
}

}

//typedef ClothingGraphicalLodParametersNS::SkinClothMapB_Type SkinClothMapB_TypeLocal;
//typedef ClothingGraphicalLodParametersNS::SkinClothMapC_Type SkinClothMapC_TypeLocal;
//typedef ClothingGraphicalLodParametersNS::TetraLink_Type TetraLink_TypeLocal;

namespace memory
{
//typedef ReadCache<384, 64, 16> MorphTargetCache;
//typedef ReadCache<384, 32, 16> VectorCache;
//typedef ReadCache<384, 64, 16> VectorCache2;

typedef ReadCache<256, 128, 16> MorphTargetCache;
typedef ReadCache<256, 64, 16> VectorCache;
typedef ReadCache<256, 128, 16> VectorCache2;

typedef ReadCache<128, 64, 0> IntCache;

/*typedef SpuAssociativeReadCache<256, 128, 16, 1> MorphTargetCache;
typedef SpuAssociativeReadCache<256, 64, 16, 1> VectorCache;
typedef SpuAssociativeReadCache<256, 128, 16, 1> VectorCache2;

typedef SpuAssociativeReadCache<128, 64, 0, 1> IntCache;*/



typedef DBReadWriteBuffer<3072> VectorWriteCache;
typedef ReadWriteCache<384, 64> VectorReadWriteCache;
typedef ReadWriteCache<384, 64> VectorReadWriteCache2;
//typedef ReadWriteCache<1536, 32> VectorReadWriteCache3;
//typedef ReadWriteCache<384, 64> VectorReadWriteCache3;
typedef ReadWriteCache<384, 64> VectorReadWriteCache3;
typedef ReadCache<64, 64, 0> NxVertexUVLocalCache;
typedef ReadCache<256, 128, 0> SkinClothMapCCache;
} // namespace memory


struct NxVertexUVLocal
{
	NxVertexUVLocal() {}
	NxVertexUVLocal(physx::PxF32 _u, physx::PxF32 _v)
	{
		set(_u, _v);
	}
	NxVertexUVLocal(const physx::PxF32 uv[])
	{
		set(uv);
	}

	void			set(physx::PxF32 _u, physx::PxF32 _v)
	{
		u = _u;
		v = _v;
	}

	void			set(const physx::PxF32 uv[])
	{
		u = uv[0];
		v = uv[1];
	}

	physx::PxF32&			operator [](int i)
	{
		PX_ASSERT(i >= 0 && i <= 1);
		return (&u)[i];
	}

	const physx::PxF32&	operator [](int i) const
	{
		PX_ASSERT(i >= 0 && i <= 1);
		return (&u)[i];
	}

	physx::PxF32	u, v;
};

#ifndef __SPU__
PX_COMPILE_TIME_ASSERT(sizeof(NxVertexUVLocal) == sizeof(NxVertexUV));
#endif


struct TetraEncoding_Local
{
	PxF32 sign[4];
	PxU32 lastVtxIdx;
};

#define TETRA_LUT_SIZE_LOCAL 6
static const TetraEncoding_Local tetraTableLocal[TETRA_LUT_SIZE_LOCAL] =
{
	{ {0,  0, 0, 1}, 0},
	{ {1,  0, 0, 1}, 2},
	{ {1,  0, 1, 1}, 1},

	{ { -1, -1, -1, 0}, 0},
	{ {0, -1, -1, 0}, 2 },
	{ {0, -1,  0, 0}, 1 }
};


namespace clothing
{

//This seems to be the data we are interested in in each submesh...
class ClothingAssetSubMesh
{

public:
	ClothingAssetSubMesh();

	const PxVec3* PX_RESTRICT mPositions;
	const PxVec3* PX_RESTRICT mNormals;
	const PxVec4* PX_RESTRICT mTangents;

	const PxF32* PX_RESTRICT mBoneWeights;
	const PxU16* PX_RESTRICT mBoneIndices;

	const PxU32* PX_RESTRICT mIndices;

	NxVertexUVLocal* mUvs;

	NxRenderDataFormat::Enum mPositionOutFormat;
	NxRenderDataFormat::Enum mNormalOutFormat;
	NxRenderDataFormat::Enum mTangentOutFormat;
	NxRenderDataFormat::Enum mBoneWeightOutFormat;
	NxRenderDataFormat::Enum mUvFormat;

	PxU32 mVertexCount;
	PxU32 mIndicesCount;
	PxU32 mUvCount;
	PxU32 mNumBonesPerVertex;

	PxU32 mCurrentMaxVertexSimulation;
	PxU32 mCurrentMaxVertexAdditionalSimulation;
	PxU32 mCurrentMaxIndexSimulation;
};

class ClothingMeshAssetData
{
public:

	ClothingMeshAssetData();

	const PxU32* mImmediateClothMap;
	ClothingGraphicalLodParametersNS::SkinClothMapD_Type* mSkinClothMap;
	ClothingGraphicalLodParametersNS::SkinClothMapB_Type* mSkinClothMapB;
	ClothingGraphicalLodParametersNS::TetraLink_Type* mTetraMap;

	PxU32 mImmediateClothMapCount;
	PxU32 mSkinClothMapCount;
	PxU32 mSkinClothMapBCount;
	PxU32 mTetraMapCount;

	PxU32 mSubmeshOffset;
	PxU32 mSubMeshCount;

	//Index of the physics mesh this lod relates to
	PxU32 mPhysicalMeshId;

	PxF32 mSkinClothMapThickness;
	PxF32 mSkinClothMapOffset;

	PxBounds3 mBounds;

	bool bActive;
	bool bNeedsTangents;
};


//A submesh within a ClothingPhysicalMeshData.
class ClothingPhysicalMeshSubmeshData
{
public:
	PxU32 mVertexCount;
	PxU32 mIndicesCount;
	PxU32 mMaxDistance0VerticesCount;
};

//A physical clothing mesh
class ClothingPhysicalMeshData
{
public:
	ClothingPhysicalMeshData();

	PxVec3* mVertices;
	PxVec3* mNormals;
	PxVec3* mSkinningNormals;
	PxU16* mBoneIndices;
	PxF32* mBoneWeights;
	PxU8* mOptimizationData;
	PxU32* mIndices;

	PxU32 mSkinningNormalsCount;
	PxU32 mBoneWeightsCount;
	PxU32 mOptimizationDataCount;
	PxU32 mIndicesCount;

	PxU32 mSubmeshOffset;
	PxU32 mSubmeshesCount;

	PxU32 mNumBonesPerVertex;
};

//A clothing asset contains a set of submeshes + some other data that we might be interested in...
class ClothingAssetData
{
public:
	ClothingAssetData();
	~ClothingAssetData();

	PxU8* mData;
	PxU32* mCompressedNumBonesPerVertex;
	PxU32* mCompressedTangentW;
	PxU32* mExt2IntMorphMapping;
	PxU32 mCompressedNumBonesPerVertexCount;
	PxU32 mCompressedTangentWCount;
	PxU32 mExt2IntMorphMappingCount;

	PxU32 mAssetSize;
	PxU32 mGraphicalLodsCount;
	PxU32 mPhysicalMeshesCount;
	PxU32 mPhysicalMeshOffset;


	PxU32 mBoneCount;

	PxU32 mRootBoneIndex;

	ClothingMeshAssetData* GetLod(const PxU32 lod) const
	{
		return ((ClothingMeshAssetData*)mData) + lod;
	}

	ClothingAssetSubMesh* GetSubmesh(const PxU32 lod, const PxU32 submeshIndex) const
	{
		return GetSubmesh(GetLod(lod), submeshIndex);
	}

	ClothingAssetSubMesh* GetSubmesh(const ClothingMeshAssetData* asset, const PxU32 submeshIndex) const
	{
		return ((ClothingAssetSubMesh*)(mData + asset->mSubmeshOffset)) + submeshIndex;
	}

	ClothingPhysicalMeshData* GetPhysicalMesh(const PxU32 index) const
	{
		return ((ClothingPhysicalMeshData*)(mData + mPhysicalMeshOffset)) + index;
	}

	ClothingPhysicalMeshSubmeshData* GetPhysicalSubmesh(const PxU32 physicalMesh, const PxU32 submeshIndex) const
	{
		return GetPhysicalSubmesh(GetPhysicalMesh(physicalMesh), submeshIndex);
	}

	ClothingPhysicalMeshSubmeshData* GetPhysicalSubmesh(const ClothingPhysicalMeshData* pData, const PxU32 submeshIndex) const
	{
		return ((ClothingPhysicalMeshSubmeshData*)(mData + pData->mSubmeshOffset)) + submeshIndex;
	}

	const PxU32* getCompressedNumBonesPerVertex(PxU32 graphicalLod, PxU32 submeshIndex, PxU32& mapSize);
	const PxU32* getCompressedTangentW(PxU32 graphicalLod, PxU32 submeshIndex, PxU32& mapSize);

	PxU32* getMorphMapping(PxU32 graphicalLod);

	template<bool withNormals, bool withBones, bool withMorph, bool withTangents>
	void skinToBonesInternal(NxAbstractMeshDescription& destMesh, PxU32 submeshIndex, PxU32 graphicalMeshIndex, PxU32 startVertex,
	                         PxMat44* compositeMatrices, PxVec3* morphDisplacements, memory::MorphTargetCache& cache);

	void skinToBones(NxAbstractMeshDescription& destMesh, PxU32 submeshIndex, PxU32 graphicalMeshIndex, PxU32 startVertex,
	                 PxMat44* compositeMatrices, PxVec3* morphDisplacements, memory::MorphTargetCache& cache);

	template<bool computeNormals>
	PxU32 skinClothMap(PxVec3* dstPositions, PxVec3* dstNormals, PxVec4* dstTangents, PxU32 numVertices,
	                    const NxAbstractMeshDescription& srcPM, ClothingGraphicalLodParametersNS::SkinClothMapD_Type* map,
	                    PxU32 numVerticesInMap, PxF32 offsetAlongNormal, PxF32 actorScale) const;

	void getNormalsAndVerticesForFace(PxVec3* vtx, PxVec3* nrm, PxU32 i, const NxAbstractMeshDescription& srcPM) const;

	PxU32 skinClothMapBSkinVertex(PxVec3& dstPos, PxVec3* dstNormal, PxU32 vIndex,
	                              ClothingGraphicalLodParametersNS::SkinClothMapB_Type* pTCMB, const ClothingGraphicalLodParametersNS::SkinClothMapB_Type* pTCMBEnd,
	                              const NxAbstractMeshDescription& srcPM) const;

	PxU32 skinClothMapB(PxVec3* dstPositions, PxVec3* dstNormals, PxU32 numVertices,
	                    const NxAbstractMeshDescription& srcPM, ClothingGraphicalLodParametersNS::SkinClothMapB_Type* map,
	                    PxU32 numVerticesInMap, bool computeNormals) const;

	bool skinToTetraMesh(NxAbstractMeshDescription& destMesh,
	                     const NxAbstractMeshDescription& srcPM,
	                     const ClothingMeshAssetData& graphicalLod);

	ClothingPhysicalMeshData* GetPhysicalMeshFromLod(const PxU32 graphicalLod) const
	{
		PX_ASSERT(graphicalLod < mGraphicalLodsCount);
		const PxU32 physicalMeshId = GetLod(graphicalLod)->mPhysicalMeshId;
		PX_ASSERT(physicalMeshId < mPhysicalMeshesCount);
		return GetPhysicalMesh(physicalMeshId);
	}

};

}
} // namespace apex
} // namespace physx

#endif // CLOTHING_ASSET_DATA
