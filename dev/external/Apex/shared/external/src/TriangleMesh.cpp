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
#include <MeshImport.h>

#include "TriangleMesh.h"
#include "SkeletalAnim.h"
#include "PsMathUtils.h"
#include "NxFromPx.h"
#include "NxRenderMeshAsset.h"
#include "NxApexRenderDataFormat.h"
#include "PxFileBuffer.h"
#include "NxClothingIsoMesh.h"

#include "foundation/PxStrideIterator.h"

#include "PsShare.h"
#include "PsString.h"
#include "foundation/PxIntrinsics.h"
#include "PsFile.h"

#include <algorithm>
#include <vector>

#if defined(PX_PS3)
#include "RendererConfig.h"
#endif

#include <NxApexRenderDebug.h>
#include <NxResourceCallback.h>
#include <NxApexNameSpace.h>
#include <NxClothingPhysicalMesh.h>
#include <NxUserRenderer.h>
#include <NxUserRenderBoneBuffer.h>
#include <NxUserRenderBoneBufferDesc.h>
#include <NxUserRenderVertexBuffer.h>
#include <NxUserRenderVertexBufferDesc.h>
#include <NxUserRenderIndexBuffer.h>
#include <NxUserRenderIndexBufferDesc.h>
#include <NxUserRenderResourceDesc.h>
#include <NxApexRenderContext.h>

#ifdef USE_SAMPLE_RENDERER
#include <Renderer.h>
#include <RendererIndexBufferDesc.h>
#include <RendererMaterialDesc.h>
#include <RendererMaterialInstance.h>
#include <RendererMeshDesc.h>
#include <RendererMeshContext.h>
#include <RendererVertexBufferDesc.h>

#include <SampleMaterialAsset.h>
#include <SampleAssetManager.h>
#endif

const physx::PxU32 OBJ_STR_LEN = 256;

#if defined(PX_WINDOWS)
#define NOMINMAX
#include <windows.h>
#endif

#if defined(PX_APPLE)
#include <stdio.h>
#endif

mimp::MeshImport* gMeshImport = NULL; // has to be declared somewhere in the code


namespace Samples
{

//-----------------------------------------------------------------------------
struct TriangleMeshEdge
{
	void init(int v0, int v1, int edgeNr, int triNr)
	{
		if (v0 < v1)
		{
			this->v0 = v0;
			this->v1 = v1;
		}
		else
		{
			this->v0 = v1;
			this->v1 = v0;
		}
		this->edgeNr = edgeNr;
		this->triNr = triNr;
	}
	bool operator < (const TriangleMeshEdge& e) const
	{
		if (v0 < e.v0)
		{
			return true;
		}
		if (v0 > e.v0)
		{
			return false;
		}
		return v1 < e.v1;
	}
	bool operator == (const TriangleMeshEdge& e) const
	{
		if (v0 == e.v0 && v1 == e.v1)
		{
			return true;
		}
		if (v0 == e.v1 && v1 == e.v0)
		{
			return true;
		}
		return false;
	}
	int v0, v1;
	int edgeNr;
	int triNr;
};

// ------------------------------------------------------------------------------------
struct TexCoord
{
	TexCoord() {}
	TexCoord(float u, float v)
	{
		this->u = u;
		this->v = v;
	}
	void zero()
	{
		u = 0.0f;
		v = 0.0f;
	}
	TexCoord operator + (const TexCoord& tc) const
	{
		TexCoord r;
		r.u = u + tc.u;
		r.v = v + tc.v;
		return tc;
	}
	void operator += (const TexCoord& tc)
	{
		u += tc.u;
		v += tc.v;
	}
	void operator *= (float r)
	{
		u *= r;
		v *= r;
	}
	void operator /= (float r)
	{
		u /= r;
		v /= r;
	}
	float u, v;
};

// -------------------------------------------------------------------
struct SimpleVertexRef
{
	int vert, normal, texCoord;
	int indexNr;
	float handedNess;

	bool operator < (const SimpleVertexRef& r) const
	{
		if (vert < r.vert)
		{
			return true;
		}
		if (vert > r.vert)
		{
			return false;
		}
		if (normal < r.normal)
		{
			return true;
		}
		if (normal > r.normal)
		{
			return false;
		}
		if (texCoord < r.texCoord)
		{
			return true;
		}
		else if (texCoord > r.texCoord)
		{
			return false;
		}
		return handedNess > r.handedNess;
	}
	bool operator == (const SimpleVertexRef& r) const
	{
		return vert == r.vert && normal == r.normal && texCoord == r.texCoord && handedNess == r.handedNess;
	}
	void parse(char* s, int indexNr)
	{
		int nr[3] = {0, 0, 0};
		char* p = s;
		int i = 0;
		while (*p != 0 && i < 3 && sscanf(p, "%d", &nr[i]) == 1)
		{
			while (*p != '/' && *p != 0)
			{
				p++;
			}

			if (*p == 0)
			{
				break;
			}

			p++;
			i++;
			if (*p == '/')
			{
				p++;
				i++;
			}
		}
		PX_ASSERT(nr[0] > 0);
		vert = nr[0] - 1;
		texCoord = nr[1] - 1;
		normal = nr[2] - 1;
		this->indexNr = indexNr;
	}
};

// ----------------------------------------------------------------------
void TriangleSubMesh::setMaterialReference(SampleRenderer::RendererMaterial* material, SampleRenderer::RendererMaterialInstance* materialInstance)
{
	PX_UNUSED(material);
	PX_UNUSED(materialInstance);

#ifdef USE_SAMPLE_RENDERER
	if (mRendererMaterialReference != material && mRendererMaterialInstance != NULL)
	{
		delete mRendererMaterialInstance;
		mRendererMaterialInstance = NULL;
	}
	mRendererMaterialReference = material;

	if (mRendererMaterialInstance == NULL && materialInstance != NULL && mRendererMaterialReference != NULL)
	{
		mRendererMaterialInstance = new SampleRenderer::RendererMaterialInstance(*mRendererMaterialReference);
	}

	if (mRendererMaterialInstance != NULL && materialInstance != NULL)
	{
		// copy the values of all variables
		*mRendererMaterialInstance = *materialInstance;
	}
#endif // USE_SAMPLE_RENDERER
}

// ----------------------------------------------------------------------
TriangleMesh::TriangleMesh(physx::PxU32 /*moduleIdentifier*/, SampleRenderer::Renderer* renderer /*= NULL*/) :
	mDynamicVertexBuffer(NULL),
	mStaticVertexBuffer(NULL),
	mIndexBuffer(NULL),
	mBoneBuffer(NULL),
	mTextureUVOrigin(physx::NxTextureUVOrigin::ORIGIN_TOP_LEFT),
	mRenderer(renderer),
	mRendererVertexBufferDynamic(NULL),
	mRendererVertexBufferShared(NULL),
	mRendererIndexBuffer(NULL),
	mOverrideMaterial(NULL),
	mUseGpuSkinning(false)
{
	clear(NULL, NULL);
}

// ----------------------------------------------------------------------
TriangleMesh::~TriangleMesh()
{
	clear(NULL, NULL);
}


// ----------------------------------------------------------------------
void TriangleMesh::setRenderer(SampleRenderer::Renderer* renderer)
{
	PX_ASSERT(mRenderer == NULL || mRenderer == renderer);
	mRenderer = renderer;
}


// ----------------------------------------------------------------------
void TriangleMesh::clear(physx::apex::NxUserRenderResourceManager* rrm, physx::apex::NxResourceCallback* rcb)
{
	mMaxBoneIndexInternal = -1;
	mMaxBoneIndexExternal = -1;

	mName = "";
	mSkeletonFile = "";

	mBounds.setEmpty();

	oneCullModeChanged = false;
	textureUvOriginChanged = false;

	mVertices.clear();
	mNormals.clear();
	mTangents.clear();
	mBitangents.clear();
	mSkinnedVertices.clear();
	mSkinnedNormals.clear();

	mSkinningMatrices.clear();
	skinningMatricesChanged = true;
	vertexValuesChangedDynamic = false;
	vertexValuesChangedStatic = false;
	vertexCountChanged = false;
	indicesChanged = false;

	for (int i = 0; i < PC_NUM_CHANNELS; i++)
	{
		mPaintChannels[i].clear();
	}

	for (int i = 0; i < NUM_TEXCOORDS; i++)
	{
		mTexCoords[i].clear();
	}

	mIndices.clear();

	mBoneIndicesExternal.clear();
	mBoneWeights.clear();

	mNextMark = -1;
	mTriangleMarks.clear();
	mVertexMarks.clear();
	mActiveSubmeshVertices.clear();

	for (physx::PxU32 i = 0; i < mSubMeshes.size(); i++)
	{
		if (mSubMeshes[i].mRenderResource != NULL)
		{
			PX_ASSERT(rrm != NULL);
			rrm->releaseResource(*mSubMeshes[i].mRenderResource);
			mSubMeshes[i].mRenderResource = NULL;
		}

		if (mParent != NULL)
		{
			mSubMeshes[i].materialResource = NULL;
		}

		if (mSubMeshes[i].materialResource != NULL)
		{
#ifndef USE_SAMPLE_RENDERER
			PX_ASSERT(rcb != NULL);
			std::string fullMaterialName = mMaterialPrefix + mSubMeshes[i].materialName + mMaterialSuffix;
			rcb->releaseResource(APEX_MATERIALS_NAME_SPACE, fullMaterialName.c_str(), mSubMeshes[i].materialResource);
#endif
			mSubMeshes[i].materialResource = NULL;
		}

#ifdef USE_SAMPLE_RENDERER
		if (mSubMeshes[i].mRendererMeshContext != NULL)
		{
			delete mSubMeshes[i].mRendererMeshContext;
			mSubMeshes[i].mRendererMeshContext = NULL;
		}

		if (mSubMeshes[i].mRendererMaterialInstance != NULL)
		{
			delete mSubMeshes[i].mRendererMaterialInstance;
			mSubMeshes[i].mRendererMaterialInstance = NULL;
		}

		if (mSubMeshes[i].mSampleMaterial != NULL && mParent == NULL)
		{
			PX_ASSERT(rcb != NULL);
			rcb->releaseResource(APEX_MATERIALS_NAME_SPACE, NULL, mSubMeshes[i].mSampleMaterial);

			mSubMeshes[i].mRendererMaterialReference = NULL;
		}

		if (mSubMeshes[i].mRendererMesh != NULL && mParent == NULL)
		{
			mSubMeshes[i].mRendererMesh->release();
			mSubMeshes[i].mRendererMesh = NULL;
		}
#endif // USE_SAMPLE_RENDERER
	}
	mSubMeshes.clear();

	if (mDynamicVertexBuffer != NULL)
	{
		PX_ASSERT(rrm != NULL);
		rrm->releaseVertexBuffer(*mDynamicVertexBuffer);
		mDynamicVertexBuffer = NULL;
	}

	if (mStaticVertexBuffer != NULL)
	{
		PX_ASSERT(rrm != NULL);
		rrm->releaseVertexBuffer(*mStaticVertexBuffer);
		mStaticVertexBuffer = NULL;
	}

	if (mIndexBuffer != NULL)
	{
		PX_ASSERT(rrm != NULL);
		rrm->releaseIndexBuffer(*mIndexBuffer);
		mIndexBuffer = NULL;
	}

	if (mBoneBuffer != NULL)
	{
		PX_ASSERT(rrm != NULL);
		rrm->releaseBoneBuffer(*mBoneBuffer);
		mBoneBuffer = NULL;
	}


#ifdef USE_SAMPLE_RENDERER
	if (mRendererIndexBuffer != NULL)
	{
		mRendererIndexBuffer->release();
		mRendererIndexBuffer = NULL;
	}

	if (mRendererVertexBufferDynamic != NULL)
	{
		mRendererVertexBufferDynamic->release();
		mRendererVertexBufferDynamic = NULL;
	}

	if (mRendererVertexBufferShared != NULL)
	{
		mRendererVertexBufferShared->release();
		mRendererVertexBufferShared = NULL;
	}

	if (mOverrideMaterial != NULL && rcb != NULL)
	{
		rcb->releaseResource(APEX_MATERIALS_NAME_SPACE, NULL, mOverrideMaterial);
		mOverrideMaterial = NULL;
	}
#endif // USE_SAMPLE_RENDERER

	mParent = NULL;
	mUseGpuSkinning = false;
}

// ----------------------------------------------------------------------
void TriangleMesh::loadMaterials(physx::apex::NxResourceCallback* resourceCallback, physx::apex::NxUserRenderResourceManager* rrm,
                                 bool dummyMaterial, const char* materialPrefix, const char* materialSuffix, bool onlyVisibleMaterials)
{
	std::string path;
	PX_ASSERT(mParent == NULL);

	if (dummyMaterial)
	{
		static int dummyCount = 0;
		for (physx::PxU32 i = 0; i < mSubMeshes.size(); i++)
		{
			char buf[64];
			physx::string::sprintf_s(buf, 64, "DummMaterial%d", dummyCount++);
			mSubMeshes[i].materialName = buf;
		}
	}

	unsigned int maxBonesShader = 0;

	for (physx::PxU32 submeshIndex = 0; submeshIndex < mSubMeshes.size(); submeshIndex++)
	{
		TriangleSubMesh& sm = mSubMeshes[submeshIndex];

		if (sm.materialResource != NULL)
		{
			continue;
		}

		if (sm.materialName.find("invisible") != std::string::npos)
		{
			sm.show = false;
		}

		if (onlyVisibleMaterials && !sm.show)
		{
			continue;
		}

		if (sm.materialName.empty())
		{
			sm.materialName = sm.name;
		}

		std::string materialName;
		if (materialPrefix != NULL)
		{
			mMaterialPrefix = materialPrefix;
			materialName.append(materialPrefix);
		}
		materialName.append(sm.materialName);
		if (materialSuffix != NULL)
		{
			mMaterialSuffix = materialSuffix;
			materialName.append(materialSuffix);
		}

#ifdef USE_SAMPLE_RENDERER
		PX_ASSERT(sm.mSampleMaterial == NULL);
		sm.mSampleMaterial = reinterpret_cast<SampleFramework::SampleMaterialAsset*>(
		                         resourceCallback->requestResource(APEX_MATERIALS_NAME_SPACE, materialName.c_str()));
#else
		sm.materialResource = (MaterialResource*)resourceCallback->requestResource(APEX_MATERIALS_NAME_SPACE, materialName.c_str());
#endif

		if (materialName.find("__cloth") != std::string::npos)
		{
			sm.usedForCollision = false;
		}

		if (sm.materialResource != NULL || sm.mSampleMaterial != NULL)
		{
#ifdef USE_SAMPLE_RENDERER
			sm.maxBonesShader = 0;//rrm->getMaxBonesForMaterial(sm.mSampleMaterial);
#else
			sm.maxBonesShader = rrm->getMaxBonesForMaterial(sm.materialResource);
#endif
			if (maxBonesShader == 0 || sm.maxBonesShader < maxBonesShader)
			{
				maxBonesShader = sm.maxBonesShader;
			}
		}
	}

#ifdef USE_SAMPLE_RENDERER
	if (mOverrideMaterial == NULL)
	{
		mOverrideMaterial = reinterpret_cast<SampleFramework::SampleMaterialAsset*>(
		                        resourceCallback->requestResource(APEX_MATERIALS_NAME_SPACE, "materials/simple_lit_uniform_color.xml"));
	}
	PX_ASSERT(mOverrideMaterial != NULL);
#endif

	optimizeForRendering();

	const unsigned int maxBoneIndexMesh = mParent != NULL ? mParent->mMaxBoneIndexInternal : mMaxBoneIndexInternal;

	mUseGpuSkinning = maxBoneIndexMesh < maxBonesShader;
}

// ----------------------------------------------------------------------
void TriangleMesh::moveAboveGround(physx::PxF32 level)
{
	physx::PxF32 lowest = FLT_MAX;
	for (physx::PxU32 i = 0; i < mVertices.size(); i++)
	{
		lowest = physx::PxMin(lowest, mVertices[i].y);
	}

	const physx::PxF32 change = std::max(0.0f, level - lowest);
	for (physx::PxU32 i = 0; i < mVertices.size(); i++)
	{
		mVertices[i].y += change;
	}
	mBounds.minimum.y += change;
	mBounds.maximum.y += change;
}

// ----------------------------------------------------------------------
void TriangleMesh::initSingleMesh()
{
	clear(NULL, NULL);
	mSubMeshes.resize(1);
	mSubMeshes[0].init();
	mSubMeshes[0].firstIndex = 0;
}

// ----------------------------------------------------------------------
void TriangleMesh::copyFrom(const TriangleMesh& mesh)
{
	clear(NULL, NULL);

	mVertices.resize(mesh.mVertices.size());
	for (size_t i = 0; i < mesh.mVertices.size(); i++)
	{
		mVertices[i] = mesh.mVertices[i];
	}

	mNormals.resize(mesh.mNormals.size());
	for (size_t i = 0; i < mesh.mNormals.size(); i++)
	{
		mNormals[i] = mesh.mNormals[i];
	}

	mTangents.resize(mesh.mTangents.size());
	for (size_t i = 0; i < mesh.mTangents.size(); i++)
	{
		mTangents[i] = mesh.mTangents[i];
	}

	mBitangents.resize(mesh.mBitangents.size());
	for (size_t i = 0; i < mesh.mBitangents.size(); i++)
	{
		mBitangents[i] = mesh.mBitangents[i];
	}

	for (unsigned int t = 0; t < NUM_TEXCOORDS; t++)
	{
		mTexCoords[t].resize(mesh.mTexCoords[t].size());
		for (size_t i = 0; i < mesh.mTexCoords[t].size(); i++)
		{
			mTexCoords[t][i] = mesh.mTexCoords[t][i];
		}
	}

	for (unsigned int i = 0; i < PC_NUM_CHANNELS; i++)
	{
		mPaintChannels[i].resize(mesh.mPaintChannels[i].size());
		for (size_t j = 0; j < mesh.mPaintChannels[i].size(); j++)
		{
			mPaintChannels[i][j] = mesh.mPaintChannels[i][j];
		}
	}

	mSubMeshes.resize(mesh.mSubMeshes.size());
	for (size_t i = 0; i < mesh.mSubMeshes.size(); i++)
	{
		mSubMeshes[i] = mesh.mSubMeshes[i];
		mSubMeshes[i].materialResource = NULL;
		// don't copy these
		mSubMeshes[i].mRenderResource = NULL;
		mSubMeshes[i].mRendererMesh = NULL;
		mSubMeshes[i].mRendererMeshContext = NULL;
		mSubMeshes[i].mSampleMaterial = NULL;
		mSubMeshes[i].mRendererMaterialReference = NULL; 
		mSubMeshes[i].mRendererMaterialInstance = NULL;
	}

	mIndices.resize(mesh.mIndices.size());
	for (size_t i = 0; i < mesh.mIndices.size(); i++)
	{
		mIndices[i] = mesh.mIndices[i];
	}

	// skeleton binding
	mSkeletonFile = mesh.mSkeletonFile;

	mBoneIndicesExternal.resize(mesh.mBoneIndicesExternal.size());
	for (size_t i = 0; i < mesh.mBoneIndicesExternal.size(); i++)
	{
		mBoneIndicesExternal[i] = mesh.mBoneIndicesExternal[i];
	}

	mBoneWeights.resize(mesh.mBoneWeights.size());
	for (size_t i = 0; i < mesh.mBoneWeights.size(); i++)
	{
		mBoneWeights[i] = mesh.mBoneWeights[i];
	}

	mName = mesh.mName;

	mMaxBoneIndexExternal = mesh.mMaxBoneIndexExternal;
	updateBounds();
	updateBoneWeights();
}

// ----------------------------------------------------------------------
void TriangleMesh::copyFromSubMesh(const TriangleMesh& mesh, int subMeshNr, bool filterVertices)
{
	if (!filterVertices && subMeshNr < 0)
	{
		copyFrom(mesh);
		return;
	}

	clear(NULL, NULL);

	if (subMeshNr >= (int)mesh.mSubMeshes.size())
	{
		return;
	}

	int firstIndex = 0;
	int numIndices = (int)mesh.getNumIndices();
	if (subMeshNr >= 0)
	{
		const TriangleSubMesh& sm = mesh.mSubMeshes[subMeshNr];
		firstIndex = sm.firstIndex;
		numIndices = sm.numIndices;
	}

	size_t numVerts = mesh.mVertices.size();

	std::vector<int> oldToNew;
	oldToNew.resize(numVerts);
	for (size_t i = 0; i < numVerts; i++)
	{
		oldToNew[i] = -1;
	}

	mSubMeshes.resize(1);
	mSubMeshes[0].init();

	const std::vector<PaintedVertex> &physChannel = mesh.getPaintChannel(PC_LATCH_TO_NEAREST_SLAVE);
	const bool filter = filterVertices && physChannel.size() == mesh.getNumVertices();
	mIndices.clear();
	int nextIndex = 0;
	for (int i = firstIndex; i < firstIndex + numIndices; i += 3)
	{

		bool skipTri = false;
		if (filter)
		{
			for (int j = 0; j < 3; j++)
			{
				skipTri |= (physChannel[mesh.mIndices[i + j]].paintValueU32 != 0);
			}
		}
		if (skipTri)
		{
			continue;
		}

		for (int j = 0; j < 3; j++)
		{
			physx::PxU32 idx = mesh.mIndices[i + j];
			if (oldToNew[idx] >= 0)
			{
				mIndices.push_back(oldToNew[idx]);
			}
			else
			{
				mIndices.push_back(nextIndex);
				oldToNew[idx] = nextIndex;
				nextIndex++;
			}
		}
	}
	int numNewVerts = nextIndex;

	mVertices.resize(numNewVerts);
	for (size_t i = 0; i < numVerts; i++)
	{
		if (oldToNew[i] >= 0)
		{
			mVertices[oldToNew[i]] = mesh.mVertices[i];
		}
	}

	if (mesh.mNormals.size() == numVerts)
	{
		mNormals.resize(numNewVerts);
		for (size_t i = 0; i < numVerts; i++)
		{
			if (oldToNew[i] >= 0)
			{
				mNormals[oldToNew[i]] = mesh.mNormals[i];
			}
		}
	}

	if (mesh.mTangents.size() == numVerts)
	{
		mTangents.resize(numNewVerts);
		for (size_t i = 0; i < numVerts; i++)
		{
			if (oldToNew[i] >= 0)
			{
				mTangents[oldToNew[i]] = mesh.mTangents[i];
			}
		}
	}

	if (mesh.mBitangents.size() == numVerts)
	{
		mBitangents.resize(numNewVerts);
		for (size_t i = 0; i < numVerts; i++)
		{
			if (oldToNew[i] >= 0)
			{
				mBitangents[oldToNew[i]] = mesh.mBitangents[i];
			}
		}
	}

	for (unsigned int t = 0; t < NUM_TEXCOORDS; t++)
	{
		// PH: I suppose we could delete these 3 lines?
		mTexCoords[t].resize(mesh.mTexCoords[t].size());
		for (physx::PxU32 i = 0; i < mesh.mTexCoords[t].size(); i++)
		{
			mTexCoords[t][i] = mesh.mTexCoords[t][i];
		}

		if (mesh.mTexCoords[t].size() == numVerts)
		{
			mTexCoords[t].resize(numNewVerts);

			for (size_t j = 0; j < numVerts; j++)
			{
				if (oldToNew[j] >= 0)
				{
					mTexCoords[t][oldToNew[j]] = mesh.mTexCoords[t][j];
				}
			}
		}
	}

	for (int i = 0; i < PC_NUM_CHANNELS; i++)
	{
		if (mesh.mPaintChannels[i].size() == numVerts)
		{
			mPaintChannels[i].resize(numNewVerts);
			for (size_t j = 0; j < numVerts; j++)
			{
				if (oldToNew[j] >= 0)
				{
					mPaintChannels[i][oldToNew[j]] = mesh.mPaintChannels[i][j];
				}
			}
		}
	}

	// skeleton binding

	mSkeletonFile = mesh.mSkeletonFile;

	if (mesh.mBoneIndicesExternal.size() == numVerts * 4)
	{
		mBoneIndicesExternal.resize(numNewVerts * 4);
		mBoneWeights.resize(numNewVerts);

		for (size_t i = 0; i < numVerts; i++)
		{
			if (oldToNew[i] >= 0)
			{
				const int newIndex = oldToNew[i];
				for (int j = 0; j < 4; j++)
				{
					mBoneIndicesExternal[newIndex * 4 + j] = mesh.mBoneIndicesExternal[i * 4 + j];
				}
				mBoneWeights[newIndex] = mesh.mBoneWeights[i];
			}
		}
	}
	mSubMeshes[0].firstIndex = 0;
	mSubMeshes[0].numIndices = (unsigned int)mIndices.size();
	mName = mesh.mName;
	updateBounds();
	updateBoneWeights();
}

// -------------------------------------------------------------------
bool TriangleMesh::loadFromObjFile(const std::string& filename, bool useCustomChannels)
{
	initSingleMesh();
	mName = filename;

	// extract path
	size_t slashPos = filename.rfind('\\', std::string::npos);
	size_t columnPos = filename.rfind(':', std::string::npos);

	size_t pos = slashPos > columnPos ? slashPos : columnPos;
	std::string path = pos == std::string::npos ? "" : filename.substr(0, pos);

	char s[OBJ_STR_LEN], ps[OBJ_STR_LEN];
	physx::PxVec3 v;
	physx::NxVertexUV tc;
	std::vector<SimpleVertexRef> refs;
	SimpleVertexRef ref[3];

	int numIndices = 0;
	std::vector<physx::PxVec3> vertices;
	std::vector<physx::PxVec3> normals;
	std::vector<physx::NxVertexUV> texCoords;


	FILE* f = 0;

	if (physx::fopen_s(&f, filename.c_str(), "r") != 0)
	{
		return false;
	}

	std::string groupName;
	std::string useMtl;

	// first a vertex ref is generated for each v/n/t combination
	while (!feof(f))
	{
		if (fgets(s, OBJ_STR_LEN, f) == NULL)
		{
			break;
		}

		//wxMessageBox(s);
		if (strncmp(s, "usemtl", 6) == 0 || strncmp(s, "g ", 2) == 0)    // new group
		{
			if (strncmp(s, "usemtl", 6) == 0)
			{
				char sub[OBJ_STR_LEN];
				sscanf(&s[7], "%s", sub);
				useMtl = sub;
			}
			else
			{
				char sub[OBJ_STR_LEN];
				sscanf(&s[2], "%s", sub);
				groupName = sub;
			}

			size_t numSubs = mSubMeshes.size();
			if (mSubMeshes[numSubs - 1].numIndices > 0)
			{
				mSubMeshes.resize(numSubs + 1);
				mSubMeshes[numSubs].init();
				mSubMeshes[numSubs].firstIndex = (physx::PxU32)(mIndices.size());
				mSubMeshes[numSubs].materialName = useMtl;
				mSubMeshes[numSubs].originalMaterialName = useMtl;
			}
			else
			{
				mSubMeshes[numSubs - 1].materialName = useMtl;
				mSubMeshes[numSubs - 1].originalMaterialName = useMtl;
			}
			size_t subNr = mSubMeshes.size() - 1;
			mSubMeshes[subNr].name = groupName;
		}
		else if (strncmp(s, "v ", 2) == 0)  	// vertex
		{
			sscanf(s, "v %f %f %f", &v.x, &v.y, &v.z);
			vertices.push_back(v);
		}
		else if (strncmp(s, "vn ", 3) == 0)  	// normal
		{
			sscanf(s, "vn %f %f %f", &v.x, &v.y, &v.z);
			normals.push_back(v);
		}
		else if (strncmp(s, "vt ", 3) == 0)  	// texture coords
		{
			sscanf(s, "vt %f %f", &tc.u, &tc.v);
			texCoords.push_back(tc);
		}
		else if (strncmp(s, "f ", 2) == 0)  	// face, tri or quad
		{
			size_t offset = 2;
			size_t index = 0;
			while (sscanf(s + offset, "%s", ps) > 0)
			{
				offset += strlen(ps);
				while (s[offset] == ' ' || s[offset] == '\t')
				{
					offset++;
				}

				if (index >= 2)
				{
					// submit triangle
					ref[2].parse(ps, 0);
					ref[0].indexNr = numIndices++;
					refs.push_back(ref[0]);
					mIndices.push_back(0);
					ref[1].indexNr = numIndices++;
					refs.push_back(ref[1]);
					mIndices.push_back(0);
					ref[2].indexNr = numIndices++;
					refs.push_back(ref[2]);
					mIndices.push_back(0);
					mSubMeshes[mSubMeshes.size() - 1].numIndices += 3;

					ref[1] = ref[2];
					index++;
				}
				else
				{
					ref[index].parse(ps, 0);
					index++;
				}
			}
		}
	}
	fclose(f);

	// make sure that vertices with left/right handed tangent space don't get merged
	for (size_t i = 0; i < refs.size(); i += 3)
	{
		const physx::PxVec3 p0 = vertices[refs[i + 0].vert];
		const physx::PxVec3 p1 = vertices[refs[i + 1].vert];
		const physx::PxVec3 p2 = vertices[refs[i + 2].vert];

		float handedNess = 1.0f;

		if (refs[i + 0].texCoord >= 0)
		{
			const physx::PxVec3 faceNormal = (p1 - p0).cross(p2 - p0);

			const physx::NxVertexUV w0 = texCoords[refs[i + 0].texCoord];
			const physx::NxVertexUV w1 = texCoords[refs[i + 1].texCoord];
			const physx::NxVertexUV w2 = texCoords[refs[i + 2].texCoord];

			const float x1 = p1.x - p0.x;
			const float x2 = p2.x - p0.x;
			const float y1 = p1.y - p0.y;
			const float y2 = p2.y - p0.y;
			const float z1 = p1.z - p0.z;
			const float z2 = p2.z - p0.z;

			const float s1 = w1.u - w0.u;
			const float s2 = w2.u - w0.u;
			const float t1 = w1.v - w0.v;
			const float t2 = w2.v - w0.v;

			const float r = 1.0F / (s1 * t2 - s2 * t1);
			const physx::PxVec3 tangentDir((t2 * x1 - t1 * x2) * r, (t2 * y1 - t1 * y2) * r, (t2 * z1 - t1 * z2) * r);
			const physx::PxVec3 bitangentDir((s1 * x2 - s2 * x1) * r, (s1 * y2 - s2 * y1) * r, (s1 * z2 - s2 * z1) * r);

			handedNess = faceNormal.cross(tangentDir).dot(bitangentDir) > 0.0f ? 1.0f : -1.0f;
		}

		refs[i + 0].handedNess = handedNess;
		refs[i + 1].handedNess = handedNess;
		refs[i + 2].handedNess = handedNess;
	}

	// now we merge multiple v/n/t triplets
	//std::sort(refs.begin(), refs.end());

	size_t readRefs = 0;
	physx::PxVec3 defNormal(1.0f, 0.0f, 0.0f);
	bool normalsOK = true;
	int numTexCoords = (int)(texCoords.size());

	while (readRefs < refs.size())
	{
		int vertNr = (int)(mVertices.size());
		SimpleVertexRef& r = refs[readRefs];
		mVertices.push_back(vertices[r.vert]);

		if (r.normal >= 0)
		{
			mNormals.push_back(normals[r.normal]);
		}
		else
		{
			mNormals.push_back(defNormal);
			normalsOK = false;
		}

		if (r.texCoord >= 0 && r.texCoord < numTexCoords)
		{
			mTexCoords[0].push_back(texCoords[r.texCoord]);
		}
		else
		{
			mTexCoords[0].push_back(physx::NxVertexUV(0.0f, 0.0f));
		}

		mIndices[r.indexNr] = vertNr;
		readRefs++;
		/*while (readRefs < refs.size() && r == refs[readRefs])
		{
			mIndices[refs[readRefs].indexNr] = vertNr;
			readRefs++;
		}*/
	}

	complete(useCustomChannels);

	if (!normalsOK)
	{
		updateNormals(-1);
	}

	updateTangents();

	mMaterialPrefix.clear();
	mMaterialSuffix.clear();

	return true;
}

// ----------------------------------------------------------------------
bool TriangleMesh::saveToObjFile(const std::string& filename) const
{
	FILE* f = 0;
	if (physx::fopen_s(&f, filename.c_str(), "w") != 0)
	{
		return false;
	}

	fprintf(f, "# Wavefront OBJ\n");
	fprintf(f, "\n");

	fprintf(f, "\n");
	fprintf(f, "# %i vertices:\n", (int)(mVertices.size()));
	for (int i = 0; i < (int)mVertices.size(); i++)
	{
		const physx::PxVec3& v = mVertices[i];
		fprintf(f, "v %f %f %f\n", v.x, v.y, v.z);
	}

	fprintf(f, "\n");
	int numTex = (int)(mTexCoords[0].size());
	fprintf(f, "# %i texture coordinates:\n", numTex);
	for (int i = 0; i < numTex; i++)
	{
		fprintf(f, "vt %f %f\n", mTexCoords[0][i].u, mTexCoords[0][i].v);
	}

	fprintf(f, "\n");
	fprintf(f, "# %i normals:\n", (int)(mNormals.size()));
	for (int i = 0; i < (int)mNormals.size(); i++)
	{
		const physx::PxVec3& v = mNormals[i];
		fprintf(f, "vn %f %f %f\n", v.x, v.y, v.z);
	}

	for (physx::PxU32 i = 0; i < mSubMeshes.size(); i++)
	{
		const TriangleSubMesh& sm = mSubMeshes[i];
		fprintf(f, "\n");
		fprintf(f, "g %s\n", sm.name.c_str());

		for (physx::PxU32 j = sm.firstIndex; j < sm.firstIndex + sm.numIndices; j += 3)
		{
			physx::PxU32 i0 = mIndices[j];
			physx::PxU32 i1 = mIndices[j + 1];
			physx::PxU32 i2 = mIndices[j + 2];

			fprintf(f, "f %i/%i/%i %i/%i/%i %i/%i/%i\n", i0, i0, i0, i1, i1, i1, i2, i2, i2);
		}
		fprintf(f, "\n");
	}
	fclose(f);

	return true;
}

// -------------------------------------------------------------------
bool TriangleMesh::loadFromXML(const std::string& filename, bool loadCustomChannels)
{
	clear(NULL, NULL);

	FAST_XML::FastXml* fastXml = FAST_XML::createFastXml(this);
	mParserState = PS_Uninitialized;
	physx::PxFileBuffer fb(filename.c_str(), physx::PxFileBuf::OPEN_READ_ONLY);
	fastXml->processXml(fb);

	int errorLineNumber = -1;
	const char* xmlError = fastXml->getError(errorLineNumber);
	if (xmlError != NULL)
	{
#ifdef WIN32
		char temp[1024];
		sprintf_s(temp, 1024, "Xml parse error in %s on line %d:\n\n%s", filename.c_str(), errorLineNumber, xmlError);
		MessageBox(NULL, temp, "FastXML", MB_OK);
#else
		printf("Xml parse error in %s on line %d:\n\n%s", filename.c_str(), errorLineNumber, xmlError);
#endif
		return false;
	}

	if (fastXml)
	{
		fastXml->release();
	}

	// normalize bone weights
	int invalidStart = -1;
	if (mBoneWeights.size() > 0)
	{
		for (size_t i = 0; i < mVertices.size(); i++)
		{
			float sum = 0;
			for (unsigned int j = 0; j < 4; j++)
			{
				sum += mBoneWeights[i][j];
			}
			if (sum > 0)
			{
				float scale = 1.0f / sum;
				mBoneWeights[i] *= scale;
				if (invalidStart != -1)
				{
					PX_ASSERT(i > 0);
					printf("\n\nWARNING: Invalid Vertices from %d to %d\n\n\n", invalidStart, (int)(i - 1));
					invalidStart = -1;
				}
			}
			else
			{
				if (invalidStart == -1)
				{
					invalidStart = (int)i;
				}
			}
		}
	}
	if (invalidStart != -1)
	{
		printf("\n\nWARNING: Invalid Vertices from %d to %d\n\n\n", invalidStart, (int)(mVertices.size() - 1));
		invalidStart = -1;
	}

	complete(loadCustomChannels);

	//mShaderHasEnoughBones = getMaxBoneIndex() < shaderMaxBones;

	mMaterialPrefix.clear();
	mMaterialSuffix.clear();

	return true;
}

// -------------------------------------------------------------------
bool TriangleMesh::saveToXML(const std::string& filename)
{
	FILE* f = fopen(filename.c_str(), "w");
	if (!f)
	{
		return false;
	}

	fprintf(f, "<mesh>\n");

	fprintf(f, "	<sharedgeometry vertexcount=\"%i\">\n", (int)(mVertices.size()));

	fprintf(f, "		<vertexbuffer positions=\"true\" normals=\"true\">\n");
	for (int i = 0; i < (int)mVertices.size(); i++)
	{
		fprintf(f, "			<vertex>\n");
		fprintf(f, "				<position x=\"%f\" y=\"%f\" z=\"%f\" />\n", mVertices[i].x, mVertices[i].y, mVertices[i].z);
		if (i < (int)mNormals.size())
		{
			fprintf(f, "				<normal x=\"%f\" y=\"%f\" z=\"%f\" />\n", mNormals[i].x, mNormals[i].y, mNormals[i].z);
		}
		fprintf(f, "			</vertex>\n");
	}
	fprintf(f, "		</vertexbuffer>\n");

	fprintf(f, "		<vertexbuffer texture_coord_dimensions_0=\"2\">\n");
	for (physx::PxU32 i = 0; i < mTexCoords[0].size(); i++)
	{
		fprintf(f, "			<vertex>\n");
		fprintf(f, "				<texcoord u=\"%f\" v=\"%f\" />\n", mTexCoords[0][i].u, mTexCoords[0][i].v);
		fprintf(f, "			</vertex>\n");
	}
	fprintf(f, "		</vertexbuffer>\n");

	bool coeffsOK = true;
//	coeffsOK = false; // no more coeffs
	for (int i = 0; i < PC_NUM_CHANNELS; i++)
	{
		if (mPaintChannels[i].size() != mVertices.size())
		{
			coeffsOK = false;
		}
	}

	if (coeffsOK)
	{
		fprintf(f, "		<vertexbuffer physics_coeffs=\"%i\">\n", PC_NUM_CHANNELS);
		for (int i = 0; i < (int)mVertices.size(); i++)
		{
			fprintf(f, "			<vertex>\n");
			fprintf(f, "				<physics_coeffs ");
			for (int j = 0; j < PC_NUM_CHANNELS; j++)
			{
				fprintf(f, "v%i=\"%f\" ", j, mPaintChannels[j][i].paintValueF32);
			}
			fprintf(f, " />\n");
			fprintf(f, "			</vertex>\n");
		}
		fprintf(f, "		</vertexbuffer>\n");
	}

	fprintf(f, "	</sharedgeometry>\n");

	fprintf(f, "	<submeshes>\n");
	for (int i = 0; i < (int)mSubMeshes.size(); i++)
	{
		TriangleSubMesh& sm = mSubMeshes[i];
		fprintf(f, "		<submesh material=\"%s\" usesharedvertices=\"true\" use32bitindexes=\"true\" operationtype=\"triangle_list\">\n",
		        sm.materialName.c_str());
		int numTris = sm.numIndices / 3;
		fprintf(f, "			<faces count =\"%i\">\n", numTris);

		for (int j = sm.firstIndex; j < (int)sm.firstIndex + (int)sm.numIndices; j += 3)
		{
			fprintf(f, "				<face v1=\"%i\" v2=\"%i\" v3=\"%i\" />\n", mIndices[j], mIndices[j + 1], mIndices[j + 2]);
		}
		fprintf(f, "			</faces>\n");
		fprintf(f, "		</submesh>\n");
	}
	fprintf(f, "	</submeshes>\n");

	fprintf(f, "	<skeletonlink name=\"%s\" />\n", mSkeletonFile.c_str());

	if (mBoneWeights.size() > 0)
	{
		fprintf(f, "	<boneassignments>\n");
		for (unsigned int i = 0; i < (unsigned int)mBoneWeights.size(); i++)
		{
			for (unsigned int j = 0; j < 4; j++)
			{
				fprintf(f, "		<vertexboneassignment vertexindex=\"%i\" boneindex=\"%i\" weight=\"%f\" />\n",
				        i, mBoneIndicesExternal[i * 4 + j], mBoneWeights[i][j]);
			}
		}
		fprintf(f, "	</boneassignments>\n");
	}

	fprintf(f, "</mesh>\n");


	fclose(f);

	return true;
}

// ----------------------------------------------------------------------
bool TriangleMesh::loadFromParent(TriangleMesh* parent)
{
	if (parent == NULL)
	{
		return false;
	}

	mParent = parent;
	mMaxBoneIndexExternal = mParent->mMaxBoneIndexExternal;
	mMaxBoneIndexInternal = mParent->mMaxBoneIndexInternal;

	mSubMeshes.resize(mParent->mSubMeshes.size());
	for (physx::PxU32 i = 0; i < mParent->mSubMeshes.size(); i++)
	{
		mSubMeshes[i] = mParent->mSubMeshes[i];
		mSubMeshes[i].mRenderResource = NULL; // don't copy that one!!!
#ifdef USE_SAMPLE_RENDERER
		mSubMeshes[i].mRendererMeshContext = new SampleRenderer::RendererMeshContext;
		if (mSubMeshes[i].mRendererMaterialInstance != NULL)
		{
			mSubMeshes[i].mRendererMaterialInstance = new SampleRenderer::RendererMaterialInstance(*mSubMeshes[i].mRendererMaterialInstance);
		}
		mSubMeshes[i].mRendererMesh = NULL;
#endif
	}

	mMaterialPrefix.clear();
	mMaterialSuffix.clear();

	mUseGpuSkinning = parent->mUseGpuSkinning;
	mRenderer = mParent->mRenderer;

	return true;
}

// ----------------------------------------------------------------------
bool TriangleMesh::loadFromMeshImport(mimp::MeshSystemContainer* msc, bool useCustomChannels)
{
	if (msc == NULL)
	{
		return false;
	}

	mimp::MeshSystem* ms = mimp::gMeshImport->getMeshSystem(msc);

	if (ms->mMeshCount == 0)
	{
		return false;
	}

	mimp::Mesh* m = ms->mMeshes[0];
	if (m->mVertexCount == 0)
	{
		return false;
	}

	// only now that everything is OK do we delete the existing mesh
	initSingleMesh();

	// and since we generate all submeshes ourselves
	mSubMeshes.clear();

	const char* bake = strstr(ms->mAssetInfo, "applyRootTransformToMesh");
	bool bakeRootBoneTransformation = bake != NULL;
	physx::PxMat34Legacy rootBoneTransformation;
	if (ms->mSkeletonCount > 0 && bakeRootBoneTransformation)
	{
		mimp::MeshBone& b = ms->mSkeletons[0]->mBones[0];
		physx::PxQuat q;
		physx::PxQuatFromArray(q, b.mOrientation);
		rootBoneTransformation.M.fromQuat(q);
		physx::PxVec3FromArray(rootBoneTransformation.t, b.mPosition);
	}

	for (unsigned int k = 0; k < ms->mMeshCount; k++)
	{
		mimp::Mesh* m = ms->mMeshes[k];

		unsigned int base_index = (int)(mVertices.size());

		// PH: ClothingTool does not cope well with really small meshes
		if (m->mVertexCount < 3)
		{
			continue;
		}

		for (unsigned int i = 0; i < m->mVertexCount; i++)
		{
			const mimp::MeshVertex& v = m->mVertices[i];

			physx::PxVec3 p;
			physx::PxVec3FromArray(p, v.mPos);
			if (bakeRootBoneTransformation)
			{
				p = rootBoneTransformation * p;
			}

			if (m->mVertexFlags & mimp::MIVF_NORMAL)
			{
				physx::PxVec3 n;
				physx::PxVec3FromArray(n, v.mNormal);
				if (bakeRootBoneTransformation)
				{
					n = rootBoneTransformation.M * n;
				}
				mVertices.push_back(p);
				mNormals.push_back(n);
			}

			if (m->mVertexFlags & mimp::MIVF_TANGENT)
			{
				physx::PxVec3 t;
				physx::PxVec3FromArray(t, v.mTangent);
				if (bakeRootBoneTransformation)
				{
					t = rootBoneTransformation.M * t;
				}
				mTangents.push_back(t);
			}

			if (m->mVertexFlags & mimp::MIVF_BINORMAL)
			{
				physx::PxVec3 b;
				physx::PxVec3FromArray(b, v.mBiNormal);
				if (bakeRootBoneTransformation)
				{
					b = rootBoneTransformation.M * b;
				}
				mBitangents.push_back(b);
			}

			if (m->mVertexFlags & mimp::MIVF_TEXEL1)
			{
				mTexCoords[0].push_back(physx::NxVertexUV(v.mTexel1[0], v.mTexel1[1]));
			}

			if (m->mVertexFlags & mimp::MIVF_TEXEL2)
			{
				mTexCoords[1].push_back(physx::NxVertexUV(v.mTexel2[0], v.mTexel2[1]));
			}

			if (m->mVertexFlags & mimp::MIVF_TEXEL3)
			{
				mTexCoords[2].push_back(physx::NxVertexUV(v.mTexel3[0], v.mTexel3[1]));
			}

			if (m->mVertexFlags & mimp::MIVF_TEXEL4)
			{
				mTexCoords[3].push_back(physx::NxVertexUV(v.mTexel4[0], v.mTexel4[1]));
			}

			if (m->mVertexFlags & mimp::MIVF_BONE_WEIGHTING)
			{
				physx::PxVec4 boneWeight(0.0f, 0.0f, 0.0f, 0.0f);
				unsigned int boneWeightWritten = 0;
				for (unsigned int j = 0; j < 4; j++)
				{
					if (v.mWeight[j] > 0.0f)
					{
						mBoneIndicesExternal.push_back(v.mBone[j]);
						mMaxBoneIndexExternal = physx::PxMax(mMaxBoneIndexExternal, (physx::PxI32)v.mBone[j]);
						boneWeight[boneWeightWritten++] = v.mWeight[j];
					}
					else
					{
						mBoneIndicesExternal.push_back(0);
					}
				}
				PX_ASSERT(boneWeightWritten <= 4);
				mBoneWeights.push_back(boneWeight);
			}

			// Fixup
			for (unsigned int t = 0; t < NUM_TEXCOORDS; t++)
			{
				physx::NxVertexUV uv(0.0f, 0.0f);
				if (mTexCoords[t].size() > 0)
				{
					while (mTexCoords[t].size() < mVertices.size())
					{
						mTexCoords[t].push_back(uv);
					}
				}
			}
		}

		for (unsigned int i = 0; i < m->mSubMeshCount; i++)
		{
			bool showMesh = true;

			mimp::SubMesh* sm = m->mSubMeshes[i];
			if (sm->mMaterial && sm->mMaterial->mMetaData)
			{
				mimp::KeyValue kv;
				unsigned int count;
				const char** keyValues = kv.getKeyValues(sm->mMaterial->mMetaData, count);
				for (unsigned int k = 0; k < count; ++k)
				{
					const char* key = keyValues[k * 2];
					const char* kvalue = keyValues[k * 2 + 1];
					if (0 == strcmp(key, "show") && 0 == strcmp(kvalue, "invisible"))
					{
						showMesh = false;
					}
				}
			}

			TriangleSubMesh t;
			t.init();
			t.firstIndex			= (physx::PxU32)mIndices.size();
			t.numIndices			= sm->mTriCount * 3;
			t.name					= m->mName;
			t.materialName			= sm->mMaterialName;
			t.originalMaterialName	= sm->mMaterialName;
			t.show					= showMesh;

			for (unsigned int j = 0; j < t.numIndices; j++)
			{
				mIndices.push_back(sm->mIndices[j] + base_index);
			}
			mSubMeshes.push_back(t);
		}
	}

	complete(useCustomChannels);

	//mShaderHasEnoughBones = getMaxBoneIndex() < shaderMaxBones;

	mMaterialPrefix.clear();
	mMaterialSuffix.clear();

	return true;
}

// ----------------------------------------------------------------------
bool TriangleMesh::saveToMeshImport(mimp::MeshSystemContainer* msc)
{
#ifndef PX_WINDOWS
	PX_UNUSED(msc);
	return NULL;
#else
	if (msc == NULL)
	{
		return false;
	}

	mimp::MeshSystem* ms = mimp::gMeshImport->getMeshSystem(msc);

	ms->mMeshCount = 1;
	ms->mMeshes = (mimp::Mesh**)::malloc(sizeof(mimp::Mesh*) * 1);
	ms->mMeshes[0] = new mimp::Mesh;

	unsigned int vertexFlags = mimp::MIVF_POSITION;

	// fill out vertices
	{
		ms->mMeshes[0]->mVertexCount = (unsigned int)mVertices.size();
		mimp::MeshVertex* verts = ms->mMeshes[0]->mVertices = (mimp::MeshVertex*)::malloc(sizeof(mimp::MeshVertex) * mVertices.size());
		::memset(verts, 0, sizeof(mimp::MeshVertex) * mVertices.size());

		for (size_t i = 0; i < mVertices.size(); i++)
		{
			(physx::PxVec3&)verts[i].mPos = mVertices[i];
		}

		if (mNormals.size() == mVertices.size())
		{
			vertexFlags |= mimp::MIVF_NORMAL;
			for (size_t i = 0; i < mNormals.size(); i++)
			{
				(physx::PxVec3&)verts[i].mNormal = mNormals[i];
			}
		}

		if (mTangents.size() == mVertices.size())
		{
			vertexFlags |= mimp::MIVF_TANGENT;
			for (size_t i = 0; i < mTangents.size(); i++)
			{
				(physx::PxVec3&)verts[i].mTangent = mTangents[i];
			}
		}

		if (mBitangents.size() == mVertices.size())
		{
			vertexFlags |= mimp::MIVF_BINORMAL;
			for (size_t i = 0; i < mBitangents.size(); i++)
			{
				(physx::PxVec3&)verts[i].mBiNormal = mBitangents[i];
			}
		}

		if (mTexCoords[0].size() == mVertices.size())
		{
			vertexFlags |= mimp::MIVF_TEXEL1;
			for (size_t i = 0; i < mTexCoords[0].size(); i++)
			{
				(physx::NxVertexUV&)verts[i].mTexel1 = mTexCoords[0][i];
			}
		}

		if (mBoneWeights.size() == mVertices.size())
		{
			PX_ASSERT(mBoneIndicesExternal.size() == mVertices.size() * 4);
			vertexFlags |= mimp::MIVF_BONE_WEIGHTING;

			for (size_t i = 0; i < mBoneWeights.size(); i++)
			{
				for (int k = 0; k < 4; k++)
				{
					verts[i].mBone[k] = mBoneIndicesExternal[i * 4 + k];
					verts[i].mWeight[k] = mBoneWeights[i][k];
				}
			}
		}
	}

	ms->mMeshes[0]->mVertexFlags = vertexFlags;

	{
		size_t nameLen = mSubMeshes[0].name.length() + 1;
		ms->mMeshes[0]->mName = (char*)::malloc(sizeof(char) * nameLen);
		strcpy_s((char*)ms->mMeshes[0]->mName, nameLen, mSubMeshes[0].name.c_str());
	}

	ms->mMeshes[0]->mSubMeshCount = (unsigned int)mSubMeshes.size();
	ms->mMeshes[0]->mSubMeshes = (mimp::SubMesh**)::malloc(sizeof(mimp::SubMesh*) * mSubMeshes.size());

	for (size_t submeshIndex = 0; submeshIndex < mSubMeshes.size(); submeshIndex++)
	{
		ms->mMeshes[0]->mSubMeshes[submeshIndex] = new mimp::SubMesh;
		mimp::SubMesh* sm = ms->mMeshes[0]->mSubMeshes[submeshIndex];

		sm->mIndices = (unsigned int*)::malloc(sizeof(unsigned int) * mSubMeshes[submeshIndex].numIndices);

		for (size_t i = 0; i < mSubMeshes[submeshIndex].numIndices; i++)
		{
			sm->mIndices[i] = mIndices[mSubMeshes[submeshIndex].firstIndex + i];
		}
		PX_ASSERT(mSubMeshes[submeshIndex].numIndices % 3 == 0);
		sm->mTriCount = mSubMeshes[submeshIndex].numIndices / 3;

		size_t matNameLength = mSubMeshes[submeshIndex].materialName.length() + 1;
		sm->mMaterialName = (char*)::malloc(sizeof(char) * matNameLength);
		strcpy_s((char*)sm->mMaterialName, matNameLength, mSubMeshes[submeshIndex].materialName.c_str());

		sm->mVertexFlags = vertexFlags;
	}

	return true;
#endif
}

// ----------------------------------------------------------------------
void TriangleMesh::initPlane(float length, float uvDist, const char* materialName)
{
	initSingleMesh();

	mVertices.resize(9);
	mNormals.resize(9);
	mTexCoords[0].resize(9);

	for (physx::PxU32 i = 0; i < 9; i++)
	{
		mNormals[i] = physx::PxVec3(0.0f, 1.0f, 0.0f);
	}

	mVertices[0] = physx::PxVec3(-length, 0.0f, length);
	mTexCoords[0][0].set(-uvDist, uvDist);

	mVertices[1] = physx::PxVec3(0.0f, 0.0f, length);
	mTexCoords[0][1].set(0.0f, uvDist);

	mVertices[2] = physx::PxVec3(length, 0.0f, length);
	mTexCoords[0][2].set(uvDist, uvDist);

	mVertices[3] = physx::PxVec3(-length, 0.0f, 0.0f);
	mTexCoords[0][3].set(-uvDist, 0.0f);

	mVertices[4] = physx::PxVec3(0.0f, 0.0f, 0.0f);
	mTexCoords[0][4].set(0.0f, 0.0f);

	mVertices[5] = physx::PxVec3(length, 0.0f, 0.0f);
	mTexCoords[0][5].set(uvDist, 0.0f);

	mVertices[6] = physx::PxVec3(-length, 0.0f, -length);
	mTexCoords[0][6].set(-uvDist, -uvDist);

	mVertices[7] = physx::PxVec3(0.0f, 0.0f, -length);
	mTexCoords[0][7].set(0.0f, -uvDist);

	mVertices[8] = physx::PxVec3(length, 0.0f, -length);
	mTexCoords[0][8].set(uvDist, -uvDist);


	mIndices.resize(8 * 3);

	mIndices[ 0] = 0;
	mIndices[ 1] = 1;
	mIndices[ 2] = 3;
	mIndices[ 3] = 3;
	mIndices[ 4] = 1;
	mIndices[ 5] = 4;
	mIndices[ 6] = 1;
	mIndices[ 7] = 2;
	mIndices[ 8] = 4;
	mIndices[ 9] = 4;
	mIndices[10] = 2;
	mIndices[11] = 5;
	mIndices[12] = 3;
	mIndices[13] = 4;
	mIndices[14] = 6;
	mIndices[15] = 6;
	mIndices[16] = 4;
	mIndices[17] = 7;
	mIndices[18] = 4;
	mIndices[19] = 5;
	mIndices[20] = 7;
	mIndices[21] = 7;
	mIndices[22] = 5;
	mIndices[23] = 8;

	mSubMeshes[0].materialName = mSubMeshes[0].originalMaterialName = materialName;
	mSubMeshes[0].numIndices = 24;

	updateBounds();
}

// ----------------------------------------------------------------------
void TriangleMesh::initFrom(physx::apex::NxClothingIsoMesh& mesh)
{
	initSingleMesh();

	mVertices.resize(mesh.getNumVertices());
	mIndices.resize(mesh.getNumIndices());
	mesh.getVertices(&mVertices[0], 0);
	mesh.getIndices(&mIndices[0], 0);

	complete(false);
}

// ----------------------------------------------------------------------
void TriangleMesh::initFrom(physx::apex::NxClothingPhysicalMesh& mesh, bool initCustomChannels)
{
	initSingleMesh();

	mVertices.resize(mesh.getNumVertices());
	mesh.getVertices(&mVertices[0], sizeof(physx::PxVec3));

	if (mesh.isTetrahedralMesh())
	{
		// reset normals to prevent updateNormals from doing something stupid on a tet mesh!
		mNormals.clear();
		mNormals.resize(mVertices.size(), physx::PxVec3(0.0f, 0.0f, 0.0f));
		// normals are not yet generated here (only available in NxClothingAssetAuthoring)
		//mesh.getVertexValue(NxRenderVertexSemantic::NORMAL, NxRenderDataFormat::FLOAT3, &mNormals[0], sizeof(physx::PxVec3));

		/*unsigned int numIndices = mesh.getNumIndices();
		assert(numIndices % 4 == 0);
		mIndices.reserve(numIndices / 4 * 3); // same as / 4 * 12

		std::vector<unsigned int> tempIndices;
		tempIndices.resize(numIndices);
		mesh.getIndices(&tempIndices[0], 0);

		for (unsigned int i = 0; i < numIndices; i += 4)
		{
			unsigned int indices[4] =
			{
				tempIndices[i + 0],
				tempIndices[i + 1],
				tempIndices[i + 2],
				tempIndices[i + 3],
			};

			mIndices.push_back(indices[0]);
			mIndices.push_back(indices[1]);
			mIndices.push_back(indices[2]);

			mIndices.push_back(indices[0]);
			mIndices.push_back(indices[3]);
			mIndices.push_back(indices[1]);

			mIndices.push_back(indices[0]);
			mIndices.push_back(indices[2]);
			mIndices.push_back(indices[3]);

			mIndices.push_back(indices[1]);
			mIndices.push_back(indices[3]);
			mIndices.push_back(indices[2]);
		}*/
	}
	//else
	{
		mIndices.resize(mesh.getNumIndices());
		mesh.getIndices(&mIndices[0], 0);
	}

	complete(initCustomChannels);
}

// ----------------------------------------------------------------------
void TriangleMesh::initFrom(physx::apex::NxRenderMeshAssetAuthoring& mesh, bool initCustomChannels)
{
	PX_ASSERT(mesh.getPartCount() == 1);

	initSingleMesh();

	physx::PxU32 numVertices = 0;
	physx::PxU32 numIndices = 0;
	physx::PxU32 numBonesPerVertex = 0;
	for (physx::PxU32 i = 0; i < mesh.getSubmeshCount(); i++)
	{
		numVertices += mesh.getSubmesh(i).getVertexCount(0);
		numIndices += mesh.getSubmesh(i).getIndexCount(0);

		const physx::apex::NxVertexFormat& format = mesh.getSubmesh(i).getVertexBuffer().getFormat();
		const int boneIndexIndex = format.getBufferIndexFromID(format.getSemanticID(physx::apex::NxRenderVertexSemantic::BONE_INDEX));
		physx::PxU32 numBonesPerVertexSubmesh = 0;
		switch(format.getBufferFormat(boneIndexIndex))
		{
		case physx::apex::NxRenderDataFormat::USHORT1:
			numBonesPerVertexSubmesh = 1;
			break;
		case physx::apex::NxRenderDataFormat::USHORT2:
			numBonesPerVertexSubmesh = 2;
			break;
		case physx::apex::NxRenderDataFormat::USHORT3:
			numBonesPerVertexSubmesh = 3;
			break;
		case physx::apex::NxRenderDataFormat::USHORT4:
			numBonesPerVertexSubmesh = 4;
			break;
		default:
			numBonesPerVertexSubmesh = 0;
			break;
		}

		// they are either identical or one of them is 0
		PX_ASSERT(numBonesPerVertexSubmesh == numBonesPerVertex || (numBonesPerVertex * numBonesPerVertexSubmesh) == 0);
		numBonesPerVertex = numBonesPerVertexSubmesh;
	}

	mVertices.clear();
	mVertices.resize(numVertices);
	mNormals.clear();
	mNormals.resize(numVertices);
	mIndices.clear();
	mIndices.resize(numIndices);
	mSubMeshes.resize(mesh.getSubmeshCount());

	if (numBonesPerVertex > 0)
	{
		mBoneIndicesExternal.clear();
		mBoneIndicesExternal.resize(numVertices * 4, 0);
		mBoneWeights.clear();
		mBoneWeights.resize(numVertices, physx::PxVec4(0.0f));
	}
	mMaxBoneIndexExternal = -1;

	bool hasTexCoords = false;
	for (physx::PxU32 i = 0; i < mesh.getSubmeshCount(); i++)
	{
		const physx::apex::NxVertexFormat& format = mesh.getSubmesh(i).getVertexBuffer().getFormat();
		const int texCoordIndex = format.getBufferIndexFromID(format.getSemanticID(physx::apex::NxRenderVertexSemantic::TEXCOORD0));
		if (texCoordIndex != -1)
		{
			hasTexCoords = true;
		}
	}
	if (hasTexCoords)
	{
		mTexCoords[0].clear();
		mTexCoords[0].resize(numVertices);
	}

	bool hasTangents = false;
	for (physx::PxU32 i = 0; i < mesh.getSubmeshCount(); i++)
	{
		const physx::apex::NxVertexFormat& format = mesh.getSubmesh(i).getVertexBuffer().getFormat();
		const int bitangentIndex = format.getBufferIndexFromID(format.getSemanticID(physx::apex::NxRenderVertexSemantic::TANGENT));
		const int tangentIndex = format.getBufferIndexFromID(format.getSemanticID(physx::apex::NxRenderVertexSemantic::BINORMAL));
		if (tangentIndex != -1 && bitangentIndex != -1)
		{
			hasTangents = true;
		}
	}
	if (hasTangents)
	{
		mTangents.clear();
		mTangents.resize(numVertices);
		mBitangents.clear();
		mBitangents.resize(numVertices);
	}

	mBounds = physx::PxBounds3::empty();
	physx::PxU32 vertexOffset = 0;
	physx::PxU32 indexOffset = 0;
	for (physx::PxU32 submeshIndex = 0; submeshIndex < mesh.getSubmeshCount(); submeshIndex++)
	{
		const physx::NxVertexBuffer& vb = mesh.getSubmesh(submeshIndex).getVertexBuffer();
		const physx::NxVertexFormat& vf = vb.getFormat();
		int bufferIndex = 0;
		physx::NxRenderDataFormat::Enum format;

		const physx::PxU32 vertexCount = mesh.getSubmesh(submeshIndex).getVertexCount(0);

		{
			bufferIndex = vf.getBufferIndexFromID(vf.getSemanticID(physx::apex::NxRenderVertexSemantic::POSITION));
			const physx::PxVec3* positions = (const physx::PxVec3*)vb.getBufferAndFormat(format, bufferIndex);
			if (!positions || format != physx::NxRenderDataFormat::FLOAT3)
			{
				PX_ALWAYS_ASSERT();
				return;
			}

			for (physx::PxU32 i = 0; i < vertexCount; i++)
			{
				mVertices[i + vertexOffset] = positions[i];
				mBounds.include(positions[i]);
			}
		}

		{
			bufferIndex = vf.getBufferIndexFromID(vf.getSemanticID(physx::apex::NxRenderVertexSemantic::NORMAL));
			const physx::PxVec3* normals = (const physx::PxVec3*)vb.getBufferAndFormat(format, bufferIndex);
			if (!normals || format != physx::NxRenderDataFormat::FLOAT3)
			{
				PX_ALWAYS_ASSERT();
			}
			else
			{
				for (physx::PxU32 i = 0; i < vertexCount; i++)
				{
					mNormals[i + vertexOffset] = normals[i];
				}
			}
		}

		if (numBonesPerVertex > 0)
		{
			bufferIndex = vf.getBufferIndexFromID(vf.getSemanticID(physx::apex::NxRenderVertexSemantic::BONE_WEIGHT));
			const float* boneWeights = (const float*)vb.getBufferAndFormat(format, bufferIndex);
			bufferIndex = vf.getBufferIndexFromID(vf.getSemanticID(physx::apex::NxRenderVertexSemantic::BONE_INDEX));
			const short* boneIndices = (const short*)vb.getBufferAndFormat(format, bufferIndex);
			if (!boneWeights || !boneIndices)
			{
				PX_ALWAYS_ASSERT();
			}
			else
			{
				for (physx::PxU32 i = 0; i < vertexCount; i++)
				{
					for (physx::PxU32 j = 0; j < numBonesPerVertex; j++)
					{
						mBoneWeights[i + vertexOffset][j] = boneWeights[i * numBonesPerVertex + j];
						mBoneIndicesExternal[(i + vertexOffset) * 4 + j] = boneIndices[i * numBonesPerVertex + j];

						mMaxBoneIndexExternal = std::max<int>(mMaxBoneIndexExternal, boneIndices[i * numBonesPerVertex + j]);
					}
				}
			}
		}

		if (hasTexCoords)
		{
			bufferIndex = vf.getBufferIndexFromID(vf.getSemanticID(physx::apex::NxRenderVertexSemantic::TEXCOORD0));
			const physx::NxVertexUV* texCoords = (const physx::NxVertexUV*)vb.getBufferAndFormat(format, bufferIndex);
			if (texCoords == NULL || format != physx::apex::NxRenderDataFormat::FLOAT2)
			{
				PX_ALWAYS_ASSERT();
			}
			else
			{
				for (physx::PxU32 i = 0; i < vertexCount; i++)
				{
					mTexCoords[0][i + vertexOffset] = texCoords[i];
				}
			}
		}

		if (hasTangents)
		{
			bufferIndex = vf.getBufferIndexFromID(vf.getSemanticID(physx::apex::NxRenderVertexSemantic::TANGENT));
			const physx::PxVec3* tangents = (const physx::PxVec3*)vb.getBufferAndFormat(format, bufferIndex);
			if (tangents == NULL || format != physx::apex::NxRenderDataFormat::FLOAT3)
			{
				PX_ALWAYS_ASSERT();
			}
			else
			{
				for (physx::PxU32 i = 0; i < vertexCount; i++)
				{
					mTangents[i + vertexOffset] = tangents[i];
				}
			}

			bufferIndex = vf.getBufferIndexFromID(vf.getSemanticID(physx::apex::NxRenderVertexSemantic::BINORMAL));
			const physx::PxVec3* bitangents = (const physx::PxVec3*)vb.getBufferAndFormat(format, bufferIndex);
			if (bitangents == NULL || format != physx::apex::NxRenderDataFormat::FLOAT3)
			{
				PX_ALWAYS_ASSERT();
			}
			else
			{
				for (physx::PxU32 i = 0; i < vertexCount; i++)
				{
					mBitangents[i + vertexOffset] = bitangents[i];
				}
			}
		}

		bufferIndex = vf.getBufferIndexFromID(vf.getID("MAX_DISTANCE"));
		if (bufferIndex != -1)
		{
			const float* maxDistances = (const float*)vb.getBufferAndFormat(format, bufferIndex);

			const float scale = 1.0f / (mBounds.maximum - mBounds.minimum).magnitude();

			if (maxDistances != NULL && format == physx::NxRenderDataFormat::FLOAT1)
			{
				if (mPaintChannels[PC_MAX_DISTANCE].size() != mVertices.size())
				{
					mPaintChannels[PC_MAX_DISTANCE].resize(mVertices.size());
				}

				for (size_t i = 0; i < vertexCount; i++)
				{
					mPaintChannels[PC_MAX_DISTANCE][i + vertexOffset].paintValueF32 = maxDistances[i] * scale;
				}
			}
		}

		bufferIndex = vf.getBufferIndexFromID(vf.getID("USED_FOR_PHYSICS"));
		if (bufferIndex != -1)
		{
			const unsigned char* usedForPhysics = (const unsigned char*)vb.getBufferAndFormat(format, bufferIndex);

			if (usedForPhysics != NULL && format == physx::apex::NxRenderDataFormat::UBYTE1)
			{
				if (mPaintChannels[PC_LATCH_TO_NEAREST_SLAVE].size() != mVertices.size())
				{
					mPaintChannels[PC_LATCH_TO_NEAREST_SLAVE].resize(mVertices.size());
				}

				for (size_t i = 0; i < vertexCount; i++)
				{
					mPaintChannels[PC_LATCH_TO_NEAREST_SLAVE][i + vertexOffset].paintValueU32 = usedForPhysics[i] > 0 ? 0 : 0xffffffff;
				}
			}
		}

		const physx::PxU32 indexCount = mesh.getSubmesh(submeshIndex).getIndexCount(0);
		const physx::PxU32* indices = mesh.getSubmesh(submeshIndex).getIndexBuffer(0);
		for (physx::PxU32 i = 0; i < indexCount; i++)
		{
			mIndices[i + indexOffset] = indices[i] + vertexOffset;
		}

		{
			mSubMeshes[submeshIndex].init();
			char buf[64];
			physx::string::sprintf_s(buf, 64, "Submesh %d", submeshIndex);
			mSubMeshes[submeshIndex].name = buf;
			mSubMeshes[submeshIndex].materialName = mesh.getMaterialName(submeshIndex);

			mSubMeshes[submeshIndex].firstIndex = indexOffset;
			mSubMeshes[submeshIndex].numIndices = indexCount;
		}

		vertexOffset += mesh.getSubmesh(submeshIndex).getVertexCount(0);
		indexOffset += indexCount;
	}

	complete(initCustomChannels);
}

// ----------------------------------------------------------------------
void TriangleMesh::applyMorphDisplacements(const std::vector<physx::PxVec3>& displacements)
{
	if (mVertices.empty() && mParent != NULL)
	{
		if (mParent->mVertices.size() == displacements.size())
		{
			mVertices.resize(displacements.size());

			for (size_t i = 0; i < mVertices.size(); i++)
			{
				mVertices[i] = mParent->mVertices[i] + displacements[i];
			}

			mUseGpuSkinning = false;
		}
	}
}

// ----------------------------------------------------------------------
void TriangleMesh::drawPainting(PaintChannelType channelType, bool skinned, physx::apex::NxApexRenderDebug* batcher)
{
	if (batcher == NULL || channelType == PC_NUM_CHANNELS)
	{
		return;
	}

	std::vector<physx::PxVec3>& vertices = skinned ? mSkinnedVertices : (mParent == NULL ? mVertices : mParent->mVertices);
	std::vector<physx::PxU32>& indices = mParent == NULL ? mIndices : mParent->mIndices;

	physx::PxBounds3 bounds;
	getBounds(bounds);
	updateSubmeshInfo();

	for (physx::PxU32 i = 0; i < mSubMeshes.size(); i++)
	{
		if (mSubMeshes[i].selected)
		{
			const physx::PxU32 lastIndex = mSubMeshes[i].firstIndex + mSubMeshes[i].numIndices;
			for (physx::PxU32 j = mSubMeshes[i].firstIndex; j < lastIndex; j += 3)
			{
				physx::PxU32 colors[3] =
				{
					mPaintChannels[channelType][indices[j + 0]].color,
					mPaintChannels[channelType][indices[j + 1]].color,
					mPaintChannels[channelType][indices[j + 2]].color,
				};
				batcher->debugGradientTri(vertices[indices[j + 0]], vertices[indices[j + 1]], vertices[indices[j + 2]], colors[0], colors[1], colors[2]);
			}
		}
	}
}

// ----------------------------------------------------------------------
void TriangleMesh::drawVertices(PaintChannelType channelType, float maxDistanceScaling, float collisionDistanceScaling, float pointScaling,
                                float vmin, float vmax, physx::apex::NxApexRenderDebug* batcher)
{
	if (batcher == NULL || channelType == PC_NUM_CHANNELS)
	{
		return;
	}

	std::vector<physx::PxVec3>& vertices = mParent == NULL ? mVertices : mParent->mVertices;
	std::vector<physx::PxVec3>& normals = mParent == NULL ? mNormals : mParent->mNormals;

	physx::PxBounds3 bounds;
	getBounds(bounds);

	updateSubmeshInfo();

	const PaintedVertex* channel = channelType != PC_NUM_CHANNELS ? &mPaintChannels[channelType][0] : NULL;
	PX_ASSERT(channelType >= 0);
	PX_ASSERT(channelType <= PC_NUM_CHANNELS);

	const bool needsNoLines = channelType == PC_LATCH_TO_NEAREST_SLAVE || channelType == PC_LATCH_TO_NEAREST_MASTER;

	for (size_t i = 0; i < vertices.size(); i++)
	{
		if (!mActiveSubmeshVertices[i])
		{
			continue;
		}

		unsigned int color;
		color = channel != NULL ? channel[i].color : 0;

		// swap red and blue
		batcher->setCurrentColor(color);
		batcher->debugPoint(vertices[i], pointScaling);

		if (needsNoLines)
		{
			continue;
		}

		if (channel[i].paintValueF32 < vmin || channel[i].paintValueF32 > vmax)
		{
			continue;
		}

		const float channelPaintValue = channel[i].paintValueF32;
		float len = (channel != NULL ? channelPaintValue : 1.0f) * maxDistanceScaling;
		if (channelType == PC_MAX_DISTANCE && len < 0.0f)
		{
			len = 0.0f;
		}
		else if (channelType == PC_COLLISION_DISTANCE)
		{
			float scale = collisionDistanceScaling;
			if (scale == 0.0f)
			{
				scale = mPaintChannels[PC_MAX_DISTANCE][i].paintValueF32 * maxDistanceScaling;
			}
			PX_ASSERT(channelPaintValue >= vmin && channelPaintValue <= vmax);
			len = channelPaintValue * scale;

			// positive collisionDistance means moving inwards against the mesh normal
			len *= -1;
		}
		if (len != 0.0f)
		{
			physx::PxVec3 target = vertices[i] + normals[i] * len;
			batcher->debugLine(vertices[i], target);
		}
	}
}

// ----------------------------------------------------------------------
void TriangleMesh::drawVertices(size_t boneNr, float minWeight, float pointScaling, physx::apex::NxApexRenderDebug* batcher) const
{
	if ((mBoneIndicesExternal.size() != mVertices.size() * 4) || batcher == NULL)
	{
		return;
	}

	const size_t numVertices = mVertices.size();
	for (size_t i = 0; i < numVertices; i++)
	{
		bool found = false;
		float weight = 0;
		for (int j = 0; j < 4; j++)
		{
			weight = mBoneWeights[i][j];
			if (mBoneIndicesExternal[i * 4 + j] == boneNr && weight > minWeight)
			{
				found = true;
			}
		}
		if (found)
		{
			PX_ASSERT(weight >= 0.0f);
			PX_ASSERT(weight <= 1.0f);

			physx::PxU8 gray = (physx::PxU8)(255 * weight);
			batcher->setCurrentColor(gray << 16 | gray << 8 | gray);
			batcher->debugPoint(mVertices[i], pointScaling);
		}
	}
}

// ----------------------------------------------------------------------
void TriangleMesh::drawNormals(float normalScale, bool activeVerticesOnly, physx::apex::NxApexRenderDebug* batcher) const
{
	if ((normalScale <= 0.0f) || batcher == NULL)
	{
		return;
	}


	for (physx::PxU32 i = 0; i < mVertices.size(); i++)
	{
		if (activeVerticesOnly && !mActiveSubmeshVertices[i])
		{
			continue;
		}

		batcher->setCurrentColor(0x00ff0000); // red
		physx::PxVec3 otherEnd = mVertices[i];
		otherEnd += mNormals[i] * normalScale;
		batcher->debugLine(mVertices[i], otherEnd);
	}
}

// ----------------------------------------------------------------------
void TriangleMesh::drawTangents(float tangentScale, physx::apex::NxApexRenderDebug* batcher) const
{
	if (tangentScale <= 0.0f || batcher == NULL || mTangents.size() != mVertices.size() || mBitangents.size() != mVertices.size())
	{
		return;
	}

	for (physx::PxU32 i = 0; i < mVertices.size(); i++)
	{
		batcher->setCurrentColor(0x0000ff00); // green
		physx::PxVec3 otherEnd = mVertices[i] + mTangents[i] * tangentScale;
		batcher->debugLine(mVertices[i], otherEnd);

		batcher->setCurrentColor(0x000000ff); // blue
		otherEnd = mVertices[i] + mBitangents[i] * tangentScale;
		batcher->debugLine(mVertices[i], otherEnd);
	}
}

// ----------------------------------------------------------------------
void TriangleMesh::drawMaxDistancePartitions(float paintingScale, const float* partitions, size_t numPartitions, physx::apex::NxApexRenderDebug* batcher)
{
	if (batcher == NULL)
	{
		return;
	}

	hasRandomColors(numPartitions);

	batcher->pushRenderState();
	batcher->addToCurrentState(physx::DebugRenderState::SolidShaded);

	for (physx::PxU32 s = 0; s < mSubMeshes.size(); s++)
	{
		if (!mSubMeshes[s].selected)
		{
			continue;
		}

		const physx::PxU32 start = mSubMeshes[s].firstIndex;
		const physx::PxU32 end = start + mSubMeshes[s].numIndices;

		const physx::PxU32 colorDarkGray = batcher->getDebugColor(physx::DebugColors::DarkGray);
		const physx::PxU32 colorWhite = batcher->getDebugColor(physx::DebugColors::White);

		for (physx::PxU32 i = start; i < end; i += 3)
		{
			float maxMaxDistance = mPaintChannels[PC_MAX_DISTANCE][mIndices[i]].paintValueF32;
			maxMaxDistance = physx::PxMax(maxMaxDistance, mPaintChannels[PC_MAX_DISTANCE][mIndices[i + 1]].paintValueF32);
			maxMaxDistance = physx::PxMax(maxMaxDistance, mPaintChannels[PC_MAX_DISTANCE][mIndices[i + 2]].paintValueF32);
			maxMaxDistance *= paintingScale;

			physx::PxU32 color = colorDarkGray;

			if (maxMaxDistance >= 0)
			{
				color = colorWhite;
				for (physx::PxU32 j = 0; j < numPartitions; j++)
				{
					if (maxMaxDistance < partitions[j])
					{
						color = mRandomColors[j];
						break;
					}
				}
			}

			batcher->setCurrentColor(color);

			physx::PxVec3 normal = (mVertices[mIndices[i + 2]] - mVertices[mIndices[i]]).cross(mVertices[mIndices[i + 1]] - mVertices[mIndices[i]]);
			normal.normalize();
			batcher->debugTriNormals(
			    mVertices[mIndices[i + 0]], mVertices[mIndices[i + 2]], mVertices[mIndices[i + 1]],
			    normal, normal, normal);

			normal = -normal;

			batcher->debugTriNormals(
			    mVertices[mIndices[i + 0]], mVertices[mIndices[i + 1]], mVertices[mIndices[i + 2]],
			    normal, normal, normal);
		}
	}

	batcher->popRenderState();
}

// ----------------------------------------------------------------------
void TriangleMesh::drawTetrahedrons(bool wireframe, physx::PxF32 scale, physx::apex::NxApexRenderDebug* batcher)
{
	if (batcher == NULL)
	{
		return;
	}

	const physx::PxU32 numIndices = (physx::PxU32)(mIndices.size());
	if (numIndices == 0)
	{
		return;
	}

	PX_ASSERT(numIndices % 4 == 0);

	PX_ASSERT(scale < 1.0f);
	PX_ASSERT(scale >= 0.0f);

	if (scale > 1.0f)
	{
		scale = 1.0f;
	}

	const physx::PxVec3* positions = &mVertices[0];

	batcher->pushRenderState();
	batcher->setCurrentColor(batcher->getDebugColor(physx::DebugColors::DarkGreen));
	batcher->addToCurrentState(physx::DebugRenderState::SolidShaded);

	const physx::PxU32 edgeIndices[6][2] = {{0, 1}, {0, 2}, {0, 3}, {1, 2}, {1, 3}, {2, 3}};
	const physx::PxU32 sides[4][3] = {{2, 1, 0}, {0, 1, 3}, {1, 2, 3}, {2, 0, 3}};

	for (physx::PxU32 i = 0; i < numIndices; i += 4)
	{
		physx::PxVec3 vecs[4];
		physx::PxVec3 center(0.0f);
		for (physx::PxU32 j = 0; j < 4; j++)
		{
			vecs[j] = positions[mIndices[i + j]];
			center += vecs[j];
		}
		center *= 0.25f;

		for (physx::PxU32 j = 0; j < 4; j++)
		{
			vecs[j] = vecs[j] * scale + center * (1.0f - scale);
		}

		if (wireframe)
		{
			for (physx::PxU32 j = 0; j < 6; j++)
			{
				batcher->debugLine(vecs[edgeIndices[j][0]], vecs[edgeIndices[j][1]]);
			}
		}
		else
		{
			for (physx::PxU32 j = 0; j < 4; j++)
			{
				batcher->debugTri(vecs[sides[j][0]], vecs[sides[j][1]], vecs[sides[j][2]]);
			}
		}
	}

	batcher->popRenderState();
}

// ----------------------------------------------------------------------
void TriangleMesh::updateRenderResources(bool rewriteBuffers, physx::apex::NxUserRenderResourceManager& rrm, void* userRenderData)
{
	updateRenderResourcesInternal(rewriteBuffers, rrm, userRenderData, true);
}

// ----------------------------------------------------------------------
void TriangleMesh::updateRenderResourcesInternal(bool rewriteBuffers, physx::apex::NxUserRenderResourceManager& rrm, void* userRenderData, bool createResource)
{
	if (mParent != NULL && mUseGpuSkinning)
	{
		mParent->updateRenderResourcesInternal(rewriteBuffers, rrm, userRenderData, false);
	}

	const physx::PxU32 numIndices = mParent != NULL ? (physx::PxU32)(mParent->mIndices.size()) : (physx::PxU32)(mIndices.size());
	const physx::PxU32 numVertices = mParent != NULL ? (physx::PxU32)(mParent->mVertices.size()) : (physx::PxU32)(mVertices.size());

	bool resourceDirty = false;

	bool boneBufferSwitch = false;
	if (skinningMatricesChanged && mUseGpuSkinning)
	{
		if (mSkinningMatrices.empty() && mBoneBuffer != NULL)
		{
			rrm.releaseBoneBuffer(*mBoneBuffer);
			mBoneBuffer = NULL;
			boneBufferSwitch = true;
		}
		if (!mSkinningMatrices.empty() && mBoneBuffer == NULL && mUseGpuSkinning)
		{
			physx::apex::NxUserRenderBoneBufferDesc bufferDesc;
			bufferDesc.buffersRequest[physx::NxRenderBoneSemantic::POSE] = physx::NxRenderDataFormat::FLOAT3x4;
			bufferDesc.maxBones = (physx::PxU32)(mSkinningMatrices.size());
			mBoneBuffer = rrm.createBoneBuffer(bufferDesc);
			boneBufferSwitch = true;
		}

		if (!mSkinningMatrices.empty() && mBoneBuffer != NULL)
		{
			physx::apex::NxApexRenderBoneBufferData boneWriteData;
			boneWriteData.setSemanticData(physx::NxRenderBoneSemantic::POSE, &mSkinningMatrices[0], sizeof(physx::PxMat34Legacy), physx::NxRenderDataFormat::FLOAT3x4);
			mBoneBuffer->writeBuffer(boneWriteData, 0, (physx::PxU32)mSkinningMatrices.size());
		}

		skinningMatricesChanged = false;
	}

	if (numVertices == 0 || vertexCountChanged || textureUvOriginChanged)
	{
		if (mDynamicVertexBuffer != NULL)
		{
			rrm.releaseVertexBuffer(*mDynamicVertexBuffer);
			vertexValuesChangedDynamic = true;
		}
		if (mStaticVertexBuffer != NULL)
		{
			rrm.releaseVertexBuffer(*mStaticVertexBuffer);
			vertexValuesChangedStatic = true;
		}
		mDynamicVertexBuffer = mStaticVertexBuffer = NULL;
		textureUvOriginChanged = false;
		resourceDirty = true;
	}

	if (mDynamicVertexBuffer == NULL && numVertices > 0)
	{
		physx::apex::NxUserRenderVertexBufferDesc bufferDesc;
		bufferDesc.uvOrigin = mTextureUVOrigin;
		bufferDesc.hint = physx::apex::NxRenderBufferHint::DYNAMIC;

		bufferDesc.maxVerts = mParent != NULL ? (physx::PxU32)(mParent->mVertices.size()) : (physx::PxU32)(mVertices.size());

		bufferDesc.buffersRequest[physx::apex::NxRenderVertexSemantic::POSITION] = physx::apex::NxRenderDataFormat::FLOAT3;
		bufferDesc.buffersRequest[physx::apex::NxRenderVertexSemantic::NORMAL] = physx::apex::NxRenderDataFormat::FLOAT3;

		if (mParent == NULL || !mUseGpuSkinning)
		{
			mDynamicVertexBuffer = rrm.createVertexBuffer(bufferDesc);
			vertexValuesChangedDynamic = true;
		}
	}

	if (mStaticVertexBuffer == NULL && numVertices > 0)
	{
		physx::apex::NxUserRenderVertexBufferDesc bufferDesc;
		bufferDesc.uvOrigin = mTextureUVOrigin;
		bufferDesc.hint = physx::apex::NxRenderBufferHint::STATIC;

		bufferDesc.maxVerts = mParent != NULL ? (physx::PxU32)(mParent->mVertices.size()) : (physx::PxU32)(mVertices.size());

		unsigned int numSemantics = 0;

		if (mParent == NULL || !mUseGpuSkinning)
		{
			for (physx::PxU32 i = 0; i < NUM_TEXCOORDS; i++)
			{
				const physx::PxU32 numTexCoords = mParent != NULL ? (physx::PxU32)(mParent->mTexCoords[i].size()) : (physx::PxU32)mTexCoords[i].size();
				if (numTexCoords == numVertices)
				{
					bufferDesc.buffersRequest[physx::apex::NxRenderVertexSemantic::TEXCOORD0 + i] = physx::apex::NxRenderDataFormat::FLOAT2;
					numSemantics++;
				}
			}

			const physx::PxU32 numBoneIndices = mParent != NULL ? (physx::PxU32)(mParent->mBoneIndicesInternal.size()) : (physx::PxU32)(mBoneIndicesInternal.size());
			if (numBoneIndices == numVertices * 4 && mUseGpuSkinning)
			{
				PX_ASSERT(mParent == NULL);
				bufferDesc.buffersRequest[physx::apex::NxRenderVertexSemantic::BONE_INDEX] = physx::apex::NxRenderDataFormat::USHORT4;
				bufferDesc.buffersRequest[physx::apex::NxRenderVertexSemantic::BONE_WEIGHT] = physx::apex::NxRenderDataFormat::FLOAT4;
				numSemantics += 2;
			}
		}
		else
		{
			PX_ASSERT(mParent->mStaticVertexBuffer != NULL);
		}

		if ((mParent == NULL || !mUseGpuSkinning) && numSemantics > 0)
		{
			mStaticVertexBuffer = rrm.createVertexBuffer(bufferDesc);
			vertexValuesChangedStatic = true;
		}
	}


	if (mDynamicVertexBuffer && (vertexValuesChangedDynamic || rewriteBuffers))
	{
		physx::apex::NxApexRenderVertexBufferData writeData;

		const physx::PxU32 numVertices = mParent != NULL ? (physx::PxU32)(mParent->mVertices.size()) : (physx::PxU32)(mVertices.size());
		if (mSkinnedVertices.size() == numVertices && !mUseGpuSkinning)
		{
			PX_ASSERT(mSkinnedNormals.size() == mSkinnedVertices.size());
			writeData.setSemanticData(physx::apex::NxRenderVertexSemantic::POSITION, &mSkinnedVertices[0], sizeof(physx::PxVec3), physx::apex::NxRenderDataFormat::FLOAT3);
			writeData.setSemanticData(physx::apex::NxRenderVertexSemantic::NORMAL,   &mSkinnedNormals[0],  sizeof(physx::PxVec3), physx::apex::NxRenderDataFormat::FLOAT3);
		}
		else
		{
			std::vector<physx::PxVec3>& vertices = mParent != NULL ? mParent->mVertices : mVertices;
			std::vector<physx::PxVec3>& normals = mParent != NULL ? mParent->mNormals : mNormals;
			PX_ASSERT(vertices.size() == numVertices);
			PX_ASSERT(normals.size()  == numVertices);
			writeData.setSemanticData(physx::apex::NxRenderVertexSemantic::POSITION, &vertices[0], sizeof(physx::PxVec3), physx::apex::NxRenderDataFormat::FLOAT3);
			writeData.setSemanticData(physx::apex::NxRenderVertexSemantic::NORMAL,   &normals[0],  sizeof(physx::PxVec3), physx::apex::NxRenderDataFormat::FLOAT3);
		}
		mDynamicVertexBuffer->writeBuffer(writeData, 0, numVertices);
		vertexValuesChangedDynamic = false;
	}

	if (mStaticVertexBuffer != NULL && vertexValuesChangedStatic)
	{
		physx::apex::NxApexRenderVertexBufferData writeData;

		if (mParent == NULL || !mUseGpuSkinning)
		{
			physx::apex::NxRenderVertexSemantic::Enum semantics[4] =
			{
				physx::apex::NxRenderVertexSemantic::TEXCOORD0,
				physx::apex::NxRenderVertexSemantic::TEXCOORD1,
				physx::apex::NxRenderVertexSemantic::TEXCOORD2,
				physx::apex::NxRenderVertexSemantic::TEXCOORD3,
			};
			for (physx::PxU32 i = 0; i < NUM_TEXCOORDS; i++)
			{
				const physx::PxU32 numTexCoords = mParent != NULL ? (physx::PxU32)(mParent->mTexCoords[i].size()) : (physx::PxU32)(mTexCoords[i].size());
				if (numTexCoords == numVertices)
				{
					writeData.setSemanticData(semantics[i], mParent != NULL ? &mParent->mTexCoords[i][0] : &mTexCoords[i][0], sizeof(physx::apex::NxVertexUV), physx::apex::NxRenderDataFormat::FLOAT2);
				}
			}

			const physx::PxU32 numBoneIndices = mParent != NULL ? (physx::PxU32)(mParent->mBoneIndicesInternal.size()) : (physx::PxU32)(mBoneIndicesInternal.size());
			if (numBoneIndices == numVertices * 4 && mUseGpuSkinning)
			{
				PX_ASSERT(mParent == NULL);
				writeData.setSemanticData(physx::apex::NxRenderVertexSemantic::BONE_INDEX,  &mBoneIndicesInternal[0], sizeof(physx::PxU16) * 4, physx::apex::NxRenderDataFormat::USHORT4);
				writeData.setSemanticData(physx::apex::NxRenderVertexSemantic::BONE_WEIGHT, &mBoneWeights[0], sizeof(physx::PxVec4), physx::apex::NxRenderDataFormat::FLOAT4);
			}
		}
		mStaticVertexBuffer->writeBuffer(writeData, 0, numVertices);
		vertexValuesChangedStatic = false;
	}

	if (mIndexBuffer != NULL && numIndices == 0 || vertexCountChanged)
	{
		rrm.releaseIndexBuffer(*mIndexBuffer);
		mIndexBuffer = NULL;
		indicesChanged |= vertexCountChanged;
	}

	if (mIndexBuffer == NULL && (mParent == NULL || !mUseGpuSkinning) && numIndices > 0)
	{
		physx::apex::NxUserRenderIndexBufferDesc bufferDesc;
		bufferDesc.format = physx::apex::NxRenderDataFormat::UINT1;
		bufferDesc.maxIndices = numIndices;

		mIndexBuffer = rrm.createIndexBuffer(bufferDesc);
		indicesChanged = mIndexBuffer != NULL;
	}

	if (mIndexBuffer != NULL && indicesChanged)
	{
		mIndexBuffer->writeBuffer(mParent != NULL ? &mParent->mIndices[0] : &mIndices[0], sizeof(physx::PxU32), 0, mParent != NULL ? (physx::PxU32)(mParent->mIndices.size()) : (physx::PxU32)(mIndices.size()));
		indicesChanged = false;
	}

	if (boneBufferSwitch || numIndices == 0 || numVertices == 0 || vertexCountChanged || oneCullModeChanged)
	{
		for (physx::PxU32 i = 0; i < mSubMeshes.size(); i++)
		{
			if (mSubMeshes[i].mRenderResource != NULL)
			{
				rrm.releaseResource(*mSubMeshes[i].mRenderResource);
				mSubMeshes[i].mRenderResource = NULL;
			}
		}
		vertexCountChanged = false;
		oneCullModeChanged = false;
	}


	for (physx::PxU32 i = 0; i < mSubMeshes.size(); i++)
	{
		bool show = mSubMeshes[i].show;
		if (mParent != NULL)
		{
			PX_ASSERT(mParent->mSubMeshes.size() == mSubMeshes.size());
			show &= mParent->mSubMeshes[i].show;
		}

		if (mSubMeshes[i].mRenderResource != NULL && (!show || mSubMeshes[i].resourceNeedsUpdate || !createResource || resourceDirty))
		{
			rrm.releaseResource(*mSubMeshes[i].mRenderResource);
			mSubMeshes[i].mRenderResource = NULL;
			mSubMeshes[i].resourceNeedsUpdate = false;
		}

		if (mSubMeshes[i].mRenderResource == NULL && show && numIndices > 0 && numVertices > 0 && createResource)
		{
			physx::apex::NxUserRenderVertexBuffer* vertexBuffers[2] = { NULL , NULL };
			physx::PxU32 numVertexBuffers = 0;

			physx::apex::NxUserRenderResourceDesc resourceDesc;
			if (mParent != NULL && mParent->mStaticVertexBuffer != NULL)
			{
				vertexBuffers[numVertexBuffers++] = mParent->mStaticVertexBuffer;
			}
			else if (mStaticVertexBuffer)
			{
				vertexBuffers[numVertexBuffers++] = mStaticVertexBuffer;
			}

			if (mUseGpuSkinning && mParent != NULL && mParent->mDynamicVertexBuffer != NULL)
			{
				vertexBuffers[numVertexBuffers++] = mParent->mDynamicVertexBuffer;
			}
			else
			{
				PX_ASSERT(mDynamicVertexBuffer != NULL);
				vertexBuffers[numVertexBuffers++] = mDynamicVertexBuffer;
			}


			PX_ASSERT(numVertexBuffers > 0);

			resourceDesc.vertexBuffers = vertexBuffers;
			resourceDesc.numVertexBuffers = numVertexBuffers;

			resourceDesc.indexBuffer = (mParent != NULL && mParent->mIndexBuffer != NULL) ? mParent->mIndexBuffer : mIndexBuffer;
			PX_ASSERT(resourceDesc.indexBuffer != NULL);
			resourceDesc.boneBuffer = mBoneBuffer;

			resourceDesc.firstVertex = 0;
			resourceDesc.numVerts = mParent != NULL ? (physx::PxU32)(mParent->mVertices.size()) : (physx::PxU32)(mVertices.size());

			resourceDesc.firstIndex = mSubMeshes[i].firstIndex;
			resourceDesc.numIndices = mSubMeshes[i].numIndices;

			resourceDesc.firstBone = 0;
			resourceDesc.numBones = mBoneBuffer != NULL ? (physx::PxU32)(mSkinningMatrices.size()) : 0;

			resourceDesc.primitives = physx::apex::NxRenderPrimitiveType::TRIANGLES;

			resourceDesc.cullMode = mSubMeshes[i].cullMode;

			resourceDesc.material = mSubMeshes[i].materialResource;

			resourceDesc.userRenderData = userRenderData;

			mSubMeshes[i].mRenderResource = rrm.createResource(resourceDesc);
		}
	}
}

// ----------------------------------------------------------------------
void TriangleMesh::dispatchRenderResources(physx::NxUserRenderer& r, const physx::PxMat44& currentPose)
{
	for (physx::PxU32 i = 0; i < mSubMeshes.size(); i++)
	{
		if (mSubMeshes[i].mRenderResource != NULL)
		{
			physx::NxApexRenderContext context;
			context.local2world = currentPose;
			context.renderResource = mSubMeshes[i].mRenderResource;

			r.renderResource(context);
		}
	}
}

// ----------------------------------------------------------------------
void TriangleMesh::updateRenderer(bool rewriteBuffers, bool overrideMaterial, bool sharedOnly /* = false */)
{
	PX_UNUSED(rewriteBuffers);
	PX_UNUSED(overrideMaterial);
	PX_UNUSED(sharedOnly);

#ifdef USE_SAMPLE_RENDERER
	if (mParent != NULL)
	{
		if (mParent->mRenderer == NULL)
		{
			mParent->mRenderer = mRenderer;
		}

		mParent->updateRenderer(rewriteBuffers, true);
	}

	if (mRenderer == NULL)
	{
		return;
	}

	const size_t numVertices = mParent != NULL ? mParent->mVertices.size() : mVertices.size();
	const size_t numIndices  = mParent != NULL ? mParent->mIndices.size() : mIndices.size();

	if (numVertices == 0 || numIndices == 0)
	{
		return;
	}

	bool vertexBufferDynamicDirty = vertexValuesChangedDynamic || rewriteBuffers;
	vertexValuesChangedDynamic = false;
	if (mRendererVertexBufferDynamic == NULL && (mVertices.size() > 0 || mSkinnedVertices.size() > 0))
	{
		//PH: Static or dynamic, depending on whether you're in the parent or not.
		//    The child might have mVertices.size() == 0 when it can do gpu skinning, hence no buffer needed
		using SampleRenderer::RendererVertexBuffer;

		SampleRenderer::RendererVertexBufferDesc desc;
		desc.hint = mParent == NULL ? RendererVertexBuffer::HINT_STATIC : RendererVertexBuffer::HINT_DYNAMIC;
		desc.maxVertices = (physx::PxU32)numVertices;
		desc.semanticFormats[RendererVertexBuffer::SEMANTIC_POSITION]	= RendererVertexBuffer::FORMAT_FLOAT3;
		desc.semanticFormats[RendererVertexBuffer::SEMANTIC_NORMAL]		= RendererVertexBuffer::FORMAT_FLOAT3;

		PX_ASSERT(desc.isValid());

		mRendererVertexBufferDynamic = mRenderer->createVertexBuffer(desc);
		vertexBufferDynamicDirty = true;
	}

	if (mRendererVertexBufferDynamic && vertexBufferDynamicDirty)
	{
		using SampleRenderer::RendererVertexBuffer;

		physx::PxStrideIterator<physx::PxVec3> positions;
		{
			unsigned int stride = 0;
			void* data = mRendererVertexBufferDynamic->lockSemantic(RendererVertexBuffer::SEMANTIC_POSITION, stride);
			positions = PxMakeIterator((physx::PxVec3*)data, stride);
		}
		physx::PxStrideIterator<physx::PxVec3> normals;
		{
			unsigned int stride = 0;
			void* data = mRendererVertexBufferDynamic->lockSemantic(RendererVertexBuffer::SEMANTIC_NORMAL, stride);
			normals = PxMakeIterator((physx::PxVec3*)data, stride);
		}

		std::vector<physx::PxVec3>& realVertices = mParent != NULL ? mParent->mVertices : mVertices;
		std::vector<physx::PxVec3>& realNormals = mParent != NULL ? mParent->mNormals : mNormals;
		
		physx::PxVec3* inPositions = mSkinnedVertices.size() == realVertices.size() ? &mSkinnedVertices[0] : &realVertices[0];
		physx::PxVec3* inNormals = mSkinnedNormals.size() == realNormals.size() ? &mSkinnedNormals[0] : &realNormals[0];
		for (unsigned int i = 0; i < numVertices; i++)
		{
			positions[i] = inPositions[i];
			normals[i] = inNormals[i];
		}

		mRendererVertexBufferDynamic->unlockSemantic(RendererVertexBuffer::SEMANTIC_POSITION);
		mRendererVertexBufferDynamic->unlockSemantic(RendererVertexBuffer::SEMANTIC_NORMAL);
	}


	bool vertexBufferSharedDirty = rewriteBuffers || textureUvOriginChanged;
	bool hasBoneBuffer = mParent == NULL && mBoneWeights.size() == mVertices.size() && getMaxBoneIndex() < RENDERER_MAX_BONES;
	const bool hasTexCoords = mParent == NULL && mTexCoords[0].size() == mVertices.size();

	if (mRendererVertexBufferShared == NULL && mParent == NULL && (hasBoneBuffer || hasTexCoords))
	{
		// PH: Only done in the parent, hence shared
		using SampleRenderer::RendererVertexBuffer;

		SampleRenderer::RendererVertexBufferDesc desc;
		desc.hint = SampleRenderer::RendererVertexBuffer::HINT_STATIC;
		desc.maxVertices = (physx::PxU32)numVertices;
		if (hasBoneBuffer)
		{
			desc.semanticFormats[RendererVertexBuffer::SEMANTIC_BONEINDEX]	= RendererVertexBuffer::FORMAT_USHORT4;
			desc.semanticFormats[RendererVertexBuffer::SEMANTIC_BONEWEIGHT]	= RendererVertexBuffer::FORMAT_FLOAT4;
		}
		if (hasTexCoords)
		{
			desc.semanticFormats[RendererVertexBuffer::SEMANTIC_TEXCOORD0]	= RendererVertexBuffer::FORMAT_FLOAT2;
		}

		PX_ASSERT(desc.isValid());

		mRendererVertexBufferShared = mRenderer->createVertexBuffer(desc);
		vertexBufferSharedDirty = true;
	}

	if (mRendererVertexBufferShared != NULL && vertexBufferSharedDirty)
	{
		using SampleRenderer::RendererVertexBuffer;

		physx::PxStrideIterator<ushort4> boneIndices;
		if (hasBoneBuffer)
		{
			unsigned int stride = 0;
			void* data = mRendererVertexBufferShared->lockSemantic(RendererVertexBuffer::SEMANTIC_BONEINDEX, stride);
			boneIndices = physx::PxMakeIterator((ushort4*)data, stride);
		}
		physx::PxStrideIterator<physx::PxVec4> boneWeights;
		if (hasBoneBuffer)
		{
			unsigned int stride = 0;
			void* data = mRendererVertexBufferShared->lockSemantic(RendererVertexBuffer::SEMANTIC_BONEWEIGHT, stride);
			boneWeights = physx::PxMakeIterator((physx::PxVec4*)data, stride);
		}
		physx::PxStrideIterator<physx::PxVec2> texCoords;
		if (hasTexCoords)
		{
			unsigned int stride = 0;
			void* data = mRendererVertexBufferShared->lockSemantic(RendererVertexBuffer::SEMANTIC_TEXCOORD0, stride);
			texCoords = physx::PxMakeIterator((physx::PxVec2*)data, stride);
		}

		ushort4* inBoneIndices = hasBoneBuffer ? (ushort4*)&mBoneIndicesInternal[0] : NULL;
		physx::PxVec4* inBoneWeights = hasBoneBuffer ? &mBoneWeights[0] : NULL;
		physx::NxVertexUV* inTexCoords = hasTexCoords ? &mTexCoords[0][0] : NULL;
		for (unsigned int i = 0; i < numVertices; i++)
		{
			if (hasBoneBuffer)
			{
				boneIndices[i] = inBoneIndices[i];
				boneWeights[i] = inBoneWeights[i];
			}
			if (hasTexCoords)
			{
				// ugh, we switch to bottom left in SampleApexRenderer.cpp
				switch (mTextureUVOrigin)
				{
				case physx::apex::NxTextureUVOrigin::ORIGIN_TOP_LEFT:
					texCoords[i].x = inTexCoords[i].u;
					texCoords[i].y = 1.0f - inTexCoords[i].v;
					break;
				case physx::apex::NxTextureUVOrigin::ORIGIN_TOP_RIGHT:
					texCoords[i].x = 1.0f - inTexCoords[i].u;
					texCoords[i].y = 1.0f - inTexCoords[i].v;
					break;
				case physx::apex::NxTextureUVOrigin::ORIGIN_BOTTOM_LEFT:
					texCoords[i].x = inTexCoords[i].u;
					texCoords[i].y = inTexCoords[i].v;
					break;
				case physx::apex::NxTextureUVOrigin::ORIGIN_BOTTOM_RIGHT:
					texCoords[i].x = 1.0f - inTexCoords[i].u;
					texCoords[i].y = inTexCoords[i].v;
					break;
				}
			}
		}

		if (hasBoneBuffer)
		{
			mRendererVertexBufferShared->unlockSemantic(RendererVertexBuffer::SEMANTIC_BONEINDEX);
			mRendererVertexBufferShared->unlockSemantic(RendererVertexBuffer::SEMANTIC_BONEWEIGHT);
		}
		if (hasTexCoords)
		{
			mRendererVertexBufferShared->unlockSemantic(RendererVertexBuffer::SEMANTIC_TEXCOORD0);
		}
	}

	bool indexBufferDirty = rewriteBuffers;
	if (mRendererIndexBuffer == NULL && mParent == NULL)
	{
		// PH: Only done in the parent
		using SampleRenderer::RendererIndexBuffer;

		SampleRenderer::RendererIndexBufferDesc desc;
		desc.hint = RendererIndexBuffer::HINT_STATIC;
		desc.maxIndices = (physx::PxU32)numIndices;
		desc.format = RendererIndexBuffer::FORMAT_UINT32;

		PX_ASSERT(desc.isValid());

		mRendererIndexBuffer = mRenderer->createIndexBuffer(desc);
		indexBufferDirty = true;
	}

	if (mRendererIndexBuffer != NULL && indexBufferDirty && numIndices > 0)
	{
		unsigned int* indices = (unsigned int*)mRendererIndexBuffer->lock();

		unsigned int* inIndices = mParent != NULL ? &mParent->mIndices[0] : &mIndices[0];

		// PH: Ugh, I need to reverse winding to make the SampleRenderer happy (mainly because of double sided rendering in D3D)
		for (unsigned int i = 0; i < numIndices; i += 3)
		{
			indices[i + 0] = inIndices[i + 2];
			indices[i + 1] = inIndices[i + 1];
			indices[i + 2] = inIndices[i + 0];
		}

		mRendererIndexBuffer->unlock();
	}

	// only create render meshes and materials for a mesh we actually want to render
	if (!sharedOnly)
	{
//		bool hasTextures = false;
//		bool hasBones = mParent != NULL && mParent->mBoneWeights.size() == mParent->mVertices.size() && mParent->getMaxBoneIndex() < RENDERER_MAX_BONES;

		for (size_t submeshIndex = 0; submeshIndex < mSubMeshes.size(); submeshIndex++)
		{
			TriangleSubMesh& sm = mSubMeshes[submeshIndex];
			bool show = sm.show;
			if (mParent != NULL)
			{
				PX_ASSERT(mParent->mSubMeshes.size() == mSubMeshes.size());
				show &= mParent->mSubMeshes[submeshIndex].show;
			}

			if (sm.mRendererMesh == NULL && show)
			{
				using SampleRenderer::RendererMesh;
				using SampleRenderer::RendererVertexBuffer;

				RendererVertexBuffer* vertexBuffers[2];
				vertexBuffers[0] = mRendererVertexBufferDynamic;
				if (vertexBuffers[0] == NULL && mParent != NULL)
				{
					vertexBuffers[0] = mParent->mRendererVertexBufferDynamic;
				}

				PX_ASSERT(vertexBuffers[0] != NULL);

				vertexBuffers[1] = mParent != NULL ? mParent->mRendererVertexBufferShared : mRendererVertexBufferShared;

				SampleRenderer::RendererMeshDesc desc;
				desc.vertexBuffers = vertexBuffers;
				desc.numVertexBuffers = vertexBuffers[1] != NULL ? 2 : 1;
				desc.numVertices = (physx::PxU32)numVertices;

				desc.indexBuffer = mRendererIndexBuffer;
				if (desc.indexBuffer == NULL && mParent != NULL)
				{
					desc.indexBuffer = mParent->mRendererIndexBuffer;
				}
				PX_ASSERT(desc.indexBuffer != NULL);

				desc.firstIndex = sm.firstIndex;
				desc.numIndices = sm.numIndices;

				desc.primitives = RendererMesh::PRIMITIVE_TRIANGLES;

				PX_ASSERT(desc.isValid());
				sm.mRendererMesh = mRenderer->createMesh(desc);
			}
			else if (sm.mRendererMesh != NULL && !show)
			{
				sm.mRendererMesh->release();
				sm.mRendererMesh = NULL;
			}

			if (sm.mRendererMeshContext == NULL && show)
			{
				sm.mRendererMeshContext = new SampleRenderer::RendererMeshContext;
			}

			SampleFramework::SampleMaterialAsset* materialAsset = mOverrideMaterial;
			if (sm.mSampleMaterial != NULL && !overrideMaterial)
			{
				materialAsset = sm.mSampleMaterial;
			}

			PX_ASSERT(materialAsset != NULL);

			for (unsigned int i = 0; i < materialAsset->getNumVertexShaders(); i++)
			{
				if (mUseGpuSkinning == (materialAsset->getMaxBones(i) > 0))
				{
					sm.setMaterialReference(materialAsset->getMaterial(i), materialAsset->getMaterialInstance(i));
					break;
				}
			}
			PX_ASSERT(sm.mRendererMaterialReference != NULL);

			if (sm.mRendererMaterialInstance != NULL)
			{
				using SampleRenderer::RendererMaterial;

				const RendererMaterial::Variable* v = sm.mRendererMaterialInstance->findVariable("diffuseColor", RendererMaterial::VARIABLE_FLOAT4);
				if (v != NULL)
				{
					if (sm.color != 0x00808080) // check for lightgray, the default color. in this case leave the diffuseColor alone
					{
						const unsigned int color = sm.color;

						const float red   = ((color >> 16) & 0xff) / 255.f;
						const float green = ((color >>  8) & 0xff) / 255.f;
						const float blue  = ((color >>  0) & 0xff) / 255.f;

						float diffuseColor[4] = { red, green, blue, 1.0f };
						sm.mRendererMaterialInstance->writeData(*v, diffuseColor);
					}
				}
			}
		}
	}
#endif // USE_SAMPLE_RENDERER
}

// ----------------------------------------------------------------------
void TriangleMesh::queueForRendering(const physx::PxMat44& currentPose, bool wireframe)
{
	const physx::PxMat44* tmp = &currentPose;
	PX_UNUSED(tmp);

	PX_UNUSED(wireframe);

#ifdef USE_SAMPLE_RENDERER
	if (mRenderer == NULL)
	{
		return;
	}

	for (size_t submeshIndex = 0; submeshIndex < mSubMeshes.size(); submeshIndex++)
	{
		TriangleSubMesh& sm = mSubMeshes[submeshIndex];
		if (sm.mRendererMesh != NULL && sm.mRendererMeshContext != NULL && sm.mRendererMaterialReference != NULL)
		{
			using SampleRenderer::RendererMeshContext;

			RendererMeshContext::CullMode cullMode = RendererMeshContext::NONE;

			switch (sm.cullMode)
			{
			case physx::apex::NxRenderCullMode::CLOCKWISE:
				cullMode = RendererMeshContext::CLOCKWISE;
				break;
			case physx::apex::NxRenderCullMode::COUNTER_CLOCKWISE:
				cullMode = RendererMeshContext::COUNTER_CLOCKWISE;
				break;
			}

			sm.mRendererMeshContext->cullMode = cullMode;
			sm.mRendererMeshContext->fillMode = wireframe ? RendererMeshContext::LINE : RendererMeshContext::SOLID;
			sm.mRendererMeshContext->material = sm.mRendererMaterialReference;
			sm.mRendererMeshContext->materialInstance = sm.mRendererMaterialInstance;
			sm.mRendererMeshContext->mesh = sm.mRendererMesh;
			sm.mRendererMeshContext->transform = &currentPose;

			if (mUseGpuSkinning)
			{
				PX_ALWAYS_ASSERT();
			}

			mRenderer->queueMeshForRender(*sm.mRendererMeshContext);
		}
	}
#endif // USE_SAMPLE_RENDERER
}

// ----------------------------------------------------------------------
void TriangleMesh::skin(const SkeletalAnim& anim, float scale /* = 1.0f */)
{
	const size_t numVerts = mParent != NULL ? mParent->mVertices.size() : mVertices.size();
	const size_t numBoneIndices = mParent != NULL ? mParent->mBoneIndicesExternal.size() : mBoneIndicesExternal.size();

	if (numBoneIndices != numVerts * 4 || mMaxBoneIndexExternal < 0)
	{
		return;
	}

	const std::vector<physx::PxMat44>& skinningMatrices = anim.getSkinningMatrices();

	PX_ASSERT((physx::PxI32)skinningMatrices.size() > mMaxBoneIndexInternal);

	std::vector<int>& boneMappingInt2Ext = mParent != NULL ? mParent->mBoneMappingInt2Ext : mBoneMappingInt2Ext;

	PX_ASSERT(mMaxBoneIndexInternal + 1 == (int)boneMappingInt2Ext.size());
	const physx::PxU32 numSkinningMatricesNeeded = physx::PxMin<physx::PxU32>(mMaxBoneIndexInternal + 1, (physx::PxU32)skinningMatrices.size());
	if (mSkinningMatrices.size() != numSkinningMatricesNeeded)
	{
		mSkinningMatrices.resize(numSkinningMatricesNeeded);
	}

	for (physx::PxU32 i = 0; i < numSkinningMatricesNeeded; i++)
	{
		mSkinningMatrices[i] = skinningMatrices[boneMappingInt2Ext[i]] * scale;
	}

	skinningMatricesChanged = true;

	if (mUseGpuSkinning)
	{
		mSkinnedVertices.clear();
		mSkinnedNormals.clear();
		return;
	}

	vertexValuesChangedDynamic = true;

	if (mSkinnedVertices.size() != numVerts)
	{
		mSkinnedVertices.resize(numVerts);
		mSkinnedNormals.resize(numVerts);
	}

	const size_t numBones = skinningMatrices.size();
	PX_ASSERT((int)numBones > mMaxBoneIndexInternal);
	PX_UNUSED(numBones);

	const physx::PxVec3* __restrict originalVertices = mParent == NULL || (mVertices.size() == mParent->mVertices.size()) ? &mVertices[0] : &mParent->mVertices[0];
	const physx::PxVec3* __restrict originalNormals = mParent == NULL ? &mNormals[0] : &mParent->mNormals[0];
	const physx::PxU16* __restrict skinningIndices = mParent == NULL ? &mBoneIndicesInternal[0] : &mParent->mBoneIndicesInternal[0];
	const physx::PxVec4* __restrict skinningWeights = mParent == NULL ? &mBoneWeights[0] : &mParent->mBoneWeights[0];
	physx::PxVec3* __restrict destVertices = &mSkinnedVertices[0];
	physx::PxVec3* __restrict destNormals = &mSkinnedNormals[0];
	physx::PxU32* __restrict numBoneWeights = mParent == NULL ? &mNumBoneWeights[0] : &mParent->mNumBoneWeights[0];


	physx::prefetchLine(skinningWeights, 0);

	const size_t count16 = (numVerts + 15) / 16;
	for (size_t i = 0; i < count16; i++)
	{
		physx::PxU32 numBoneWeight = *numBoneWeights;

		const size_t maxVerts = (i + 1) * 16 > numVerts ? numVerts - (i * 16) : 16;

		for (size_t j = 0; j < maxVerts; j++)
		{
			physx::prefetchLine(skinningWeights + 1, 0);

			size_t twoBit = numBoneWeight & 0x3;
			numBoneWeight >>= 2;

			const physx::PxVec3 v = *originalVertices;
			const physx::PxVec3 n = *originalNormals;

			physx::PxVec3 vs(0.0f, 0.0f, 0.0f);
			physx::PxVec3 ns(0.0f, 0.0f, 0.0f);

			physx::PxVec4 skinningWeight = *skinningWeights;

			for (unsigned int k = 0; k <= twoBit; k++)
			{
				//const physx::PxU32 weightIdx = i*4 + k;
				PX_ASSERT(physx::PxIsFinite(skinningWeight[k]));
				const float weight = skinningWeight[k];

				PX_ASSERT(weight > 0.0f);
				const physx::PxU16 boneNr = skinningIndices[k];

				vs += mSkinningMatrices[boneNr] * v * weight;
				ns += mSkinningMatrices[boneNr].M * n * weight;
			}
			ns *= physx::recipSqrtFast(ns.magnitudeSquared());
			//ns.normalize();

			*destVertices = vs;
			*destNormals = ns;

			originalVertices++;
			originalNormals++;
			skinningWeights++;
			skinningIndices += 4;
			destVertices++;
			destNormals++;
		}

		numBoneWeights++;
	}
}

// ----------------------------------------------------------------------
void TriangleMesh::unskin()
{
	mSkinningMatrices.clear();
	skinningMatricesChanged = true;

	mSkinnedVertices.clear();
	mSkinnedNormals.clear();
	vertexValuesChangedDynamic = true;

	PX_ASSERT(mSkinnedVertices.empty());
}

// ----------------------------------------------------------------------
void TriangleMesh::showSubmesh(size_t index, bool on)
{
	if (index >= mSubMeshes.size())
	{
		for (size_t i = 0; i < mSubMeshes.size(); i++)
		{
			mSubMeshes[i].show = on;
		}
	}
	else
	{
		mSubMeshes[index].show = on;
	}
}

// ----------------------------------------------------------------------
void TriangleMesh::selectSubMesh(size_t index, bool selected)
{
	if (index >= mSubMeshes.size())
	{
		for (size_t i = 0; i < mSubMeshes.size(); i++)
		{
			mSubMeshes[i].selected = selected;
		}
	}
	else
	{
		mSubMeshes[index].selected = selected;
	}
	updateSubmeshInfo();
}

// ----------------------------------------------------------------------
void TriangleMesh::hideSubMesh(const char* submeshName, const char* materialName)
{
	// Selected submeshes are hidden later on
	int found = 0;
	if (materialName != NULL)
	{
		for (size_t i = 0; i < mSubMeshes.size(); i++)
		{
			if (mSubMeshes[i].name == submeshName && mSubMeshes[i].originalMaterialName == materialName)
			{
				mSubMeshes[i].selected = true;
				found++;
			}
		}
	}

	if (found == 0)
	{
		for (size_t i = 0; i < mSubMeshes.size(); i++)
		{
			if (mSubMeshes[i].name == submeshName)
			{
				mSubMeshes[i].selected = true;
				found++;
			}
		}
	}
}

// ----------------------------------------------------------------------
size_t TriangleMesh::getNumTriangles(size_t subMeshNr) const
{
	if (mParent != NULL)
	{
		return mParent->getNumTriangles(subMeshNr);
	}

	if (subMeshNr >= mSubMeshes.size())
	{
		return mIndices.size() / 3;
	}
	else
	{
		return mSubMeshes[subMeshNr].numIndices / 3;
	}
}

// ----------------------------------------------------------------------
std::vector<PaintedVertex> &TriangleMesh::getPaintChannel(PaintChannelType channelType)
{
	PX_ASSERT(0 <= channelType && channelType < PC_NUM_CHANNELS);
	return mPaintChannels[channelType];
}

// ----------------------------------------------------------------------
const std::vector<PaintedVertex> &TriangleMesh::getPaintChannel(PaintChannelType channelType) const
{
	PX_ASSERT(0 <= channelType && channelType < PC_NUM_CHANNELS);
	return mPaintChannels[channelType];
}
// ----------------------------------------------------------------------
float TriangleMesh::getMaximalMaxDistance() const
{
	if (mPaintChannels[PC_MAX_DISTANCE].size() != mVertices.size())
	{
		return 0.0f;
	}

	float maxValue = 0.0f;
	for (physx::PxU32 i = 0; i < mVertices.size(); i++)
	{
		maxValue = physx::PxMax(maxValue, mPaintChannels[PC_MAX_DISTANCE][i].paintValueF32);
	}
	return maxValue;
}

// ----------------------------------------------------------------------
int TriangleMesh::getBoneAssignments(physx::PxU32 vertNr, const physx::PxU16* &bones, const float* &weights) const
{
	if (mParent != NULL)
	{
		return mParent->getBoneAssignments(vertNr, bones, weights);
	}

	if (mBoneIndicesExternal.empty())
	{
		return 0;
	}

	if (vertNr >= mVertices.size())
	{
		return 0;
	}

	bones = &mBoneIndicesExternal[vertNr * 4];
	weights = &mBoneWeights[vertNr].x;

	for (unsigned int i = 0; i < 4; i++)
	{
		if (weights[i] == 0.0f)
		{
			return i;
		}
	}
	return 4;
}

// ----------------------------------------------------------------------
void TriangleMesh::displaceAlongNormal(float displacement)
{
	if (mNormals.size() != mVertices.size())
	{
		return;
	}
	for (int i = 0; i < (int)mVertices.size(); i++)
	{
		mVertices[i] += mNormals[i] * displacement;
	}
	updateBounds();
}

// ----------------------------------------------------------------------
bool TriangleMesh::generateTangentSpace()
{
	if (mTexCoords[0].size() == mVertices.size())
	{
		// just make the size not 0
		mTangents.push_back(physx::PxVec3(0.0f));
		updateTangents();

		return true;
	}
	return false;
}

// ----------------------------------------------------------------------
void TriangleMesh::updateBounds()
{
	mBounds.setEmpty();
	for (int i = 0; i < (int)mVertices.size(); i++)
	{
		mBounds.include(mVertices[i]);
	}
}

// ----------------------------------------------------------------------
void TriangleMesh::setSubMeshColor(size_t subMeshNr, physx::PxU32 color)
{
	if (subMeshNr >= mSubMeshes.size())
	{
		for (size_t i = 0; i < mSubMeshes.size(); i++)
		{
			setSubMeshColor(i, color);
		}
	}
	else
	{
		mSubMeshes[subMeshNr].color = color;

		if (mSubMeshes[subMeshNr].materialResource != NULL)
		{
			float red = ((color >> 16) & 0xff) / 255.f;
			float green = ((color >> 8) & 0xff) / 255.f;
			float blue = ((color >> 0) & 0xff) / 255.f;

			mSubMeshes[subMeshNr].materialResource->color = physx::PxVec3(red, green, blue);
		}
	}
}

// ----------------------------------------------------------------------
void TriangleMesh::setSubMeshMaterialName(size_t subMeshNr, const char* materialName, physx::NxResourceCallback* resourceCallback)
{
	if (subMeshNr >= mSubMeshes.size())
	{
		for (size_t i = 0; i < mSubMeshes.size(); i++)
		{
			setSubMeshMaterialName(i, materialName, resourceCallback);
		}
	}
	else
	{
		mSubMeshes[subMeshNr].materialName = materialName;
		std::string name;
		name.append(mMaterialPrefix);
		name.append(materialName);
		name.append(mMaterialSuffix);
#ifdef USE_SAMPLE_RENDERER
		if (mSubMeshes[subMeshNr].mSampleMaterial != NULL)
		{
			resourceCallback->releaseResource(APEX_MATERIALS_NAME_SPACE, NULL, mSubMeshes[subMeshNr].mSampleMaterial);
		}
		mSubMeshes[subMeshNr].mSampleMaterial = reinterpret_cast<SampleFramework::SampleMaterialAsset*>(resourceCallback->requestResource(APEX_MATERIALS_NAME_SPACE, name.c_str()));
#else
		mSubMeshes[subMeshNr].materialResource = (MaterialResource*)resourceCallback->requestResource(APEX_MATERIALS_NAME_SPACE, name.c_str());
#endif
		mSubMeshes[subMeshNr].resourceNeedsUpdate = true;
	}
}

// ----------------------------------------------------------------------
void TriangleMesh::setSubMeshUsedForCollision(size_t subMeshNr, bool enable)
{
	if (subMeshNr < mSubMeshes.size())
	{
		mSubMeshes[subMeshNr].usedForCollision = enable;
	}
	else
	{
		for (size_t i = 0; i < mSubMeshes.size(); i++)
		{
			setSubMeshUsedForCollision(i, enable);
		}
	}
}

// ----------------------------------------------------------------------
void TriangleMesh::setSubMeshHasPhysics(size_t subMeshNr, bool enable)
{
	if (subMeshNr < mSubMeshes.size())
	{
		mSubMeshes[subMeshNr].hasApexAsset = enable;
	}
	else
	{
		for (size_t i = 0; i < mSubMeshes.size(); i++)
		{
			setSubMeshHasPhysics(i, enable);
		}
	}
}

// ----------------------------------------------------------------------
void TriangleMesh::setAllColors(unsigned int color)
{
	for (size_t i = 0; i < mSubMeshes.size(); i++)
	{
		setSubMeshColor(i, color);
	}
}

// ----------------------------------------------------------------------
void TriangleMesh::subdivideSubMesh(int subMeshNr, int subdivision, bool evenOutVertexDegrees)
{
	const int newTris[8][13] =
	{
		{ 0, 1, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{ 0, 3, 2,  3, 1, 2, -1, -1, -1, -1, -1, -1, -1},
		{ 0, 1, 4,  0, 4, 2, -1, -1, -1, -1, -1, -1, -1},
		{ 0, 3, 2,  3, 1, 4,  3, 4, 2, -1, -1, -1, -1},
		{ 0, 1, 5,  1, 2, 5, -1, -1, -1, -1, -1, -1, -1},
		{ 0, 3, 5,  3, 1, 5,  1, 2, 5, -1, -1, -1, -1},
		{ 0, 1, 5,  1, 4, 5,  4, 2, 5, -1, -1, -1, -1},
		{ 0, 3, 5,  3, 1, 4,  3, 4, 5,  4, 2, 5, -1}
	};

	if (subMeshNr < 0 || subMeshNr >= (int)mSubMeshes.size())
	{
		return;
	}

	if (subdivision == 0)
	{
		return;
	}

	float maxEdgeLength = (mBounds.minimum - mBounds.maximum).magnitude() / subdivision;
	TriangleSubMesh& sm = mSubMeshes[subMeshNr];

	// move submesh to the end so it can be expanded
	std::vector<physx::PxU32> tempIndices;
	tempIndices.resize(sm.numIndices);
	for (int i = 0; i < (int)sm.numIndices; i++)
	{
		tempIndices[i] = mIndices[sm.firstIndex + i];
	}

	int numShift = (physx::PxU32)mIndices.size() - (sm.firstIndex + sm.numIndices);
	for (int i = 0; i < numShift; i++)
	{
		mIndices[sm.firstIndex + i] = mIndices[sm.firstIndex + sm.numIndices + i];
	}

	int last = (physx::PxU32)mIndices.size() - sm.numIndices;
	for (int i = 0; i < (int)sm.numIndices; i++)
	{
		mIndices[last + i] = tempIndices[i];
	}

	for (int i = 0; i < (int)mSubMeshes.size(); i++)
	{
		TriangleSubMesh& si = mSubMeshes[i];
		if (si.firstIndex > sm.firstIndex)
		{
			si.firstIndex -= sm.numIndices;
		}
	}
	sm.firstIndex = (physx::PxU32)mIndices.size() - sm.numIndices;

	// subdivide
	float h2 = maxEdgeLength * maxEdgeLength;
	mVertexFirstSplit.resize(mVertices.size());
	for (int i = 0; i < (int)mVertices.size(); i++)
	{
		mVertexFirstSplit[i] = -1;
	}
	mVertexSplits.clear();

	int numTris = sm.numIndices / 3;
	int id[6];

	vertexCountChanged |= sm.numIndices > 0;

	for (int i = 0; i < numTris; i++)
	{
		id[0] = mIndices[sm.firstIndex + 3 * i + 0];
		id[1] = mIndices[sm.firstIndex + 3 * i + 1];
		id[2] = mIndices[sm.firstIndex + 3 * i + 2];
		physx::PxVec3 p0 = mVertices[id[0]];
		physx::PxVec3 p1 = mVertices[id[1]];
		physx::PxVec3 p2 = mVertices[id[2]];

		int code = 0;
		if ((p0 - p1).magnitudeSquared() > h2)
		{
			id[3] = addSplitVert(id[0], id[1]);
			code |= 1;
		}
		if ((p1 - p2).magnitudeSquared() > h2)
		{
			id[4] = addSplitVert(id[1], id[2]);
			code |= 2;
		}
		if ((p2 - p0).magnitudeSquared() > h2)
		{
			id[5] = addSplitVert(id[2], id[0]);
			code |= 4;
		}

		const int* newId = newTris[code];

		// the first sub triangle replaces the old one
		mIndices[sm.firstIndex + 3 * i + 0] = id[*newId++];
		mIndices[sm.firstIndex + 3 * i + 1] = id[*newId++];
		mIndices[sm.firstIndex + 3 * i + 2] = id[*newId++];

		// the others are appended at the end
		while (true)
		{
			int j = *newId++;
			if (j >= 0)
			{
				mIndices.push_back(id[j]);
			}
			else
			{
				break;
			}
		}
	}
	sm.numIndices = (physx::PxU32)mIndices.size() - sm.firstIndex;

	if (evenOutVertexDegrees)
	{
		evenOutVertexDegree(subMeshNr, 3);
	}

	updateNormals(subMeshNr);
	updateTangents();
	updateBoneWeights();
	updateSubmeshInfo();
}

// ----------------------------------------------------------------------
void TriangleMesh::evenOutVertexDegree(int subMeshNr, int numIters)
{
	std::vector<TriangleMeshEdge> edges;
	TriangleMeshEdge edge;

	// compute degrees and neighbor information
	std::vector<int> vertDegs;
	vertDegs.resize(mVertices.size(), 0);

	PX_ASSERT(subMeshNr < (int)mSubMeshes.size());
	int firstIndex = 0;
	int lastIndex = (physx::PxU32)mIndices.size();
	if (subMeshNr >= 0)
	{
		firstIndex = mSubMeshes[subMeshNr].firstIndex;
		lastIndex = firstIndex + mSubMeshes[subMeshNr].numIndices;
	}
	physx::PxU32* indices = &mIndices[firstIndex];
	int numTris = (lastIndex - firstIndex) / 3;

	for (int i = 0; i < numTris; i++)
	{
		physx::PxU32 i0 = indices[3 * i];
		physx::PxU32 i1 = indices[3 * i + 1];
		physx::PxU32 i2 = indices[3 * i + 2];
		vertDegs[i0]++;
		vertDegs[i1]++;
		vertDegs[i2]++;
		edge.init(i0, i1, 0, i);
		edges.push_back(edge);
		edge.init(i1, i2, 1, i);
		edges.push_back(edge);
		edge.init(i2, i0, 2, i);
		edges.push_back(edge);
	}
	std::sort(edges.begin(), edges.end());

	std::vector<int> neighbors;
	neighbors.resize(3 * numTris, -1);

	size_t edgeNr = 0;
	size_t numEdges = edges.size();
	while (edgeNr < numEdges)
	{
		TriangleMeshEdge& e0 = edges[edgeNr];
		size_t j = edgeNr + 1;
		while (j < numEdges && edges[j] == e0)
		{
			j++;
		}
		if (j < numEdges && j == edgeNr + 2)  	// manifold edge
		{
			TriangleMeshEdge& e1 = edges[edgeNr + 1];
			neighbors[e0.triNr * 3 + e0.edgeNr] = e1.triNr;
			neighbors[e1.triNr * 3 + e1.edgeNr] = e0.triNr;
		}
		edgeNr = j;
	}

	// artifically increase the degree of border vertices
	for (int i = 0; i < numTris; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			if (neighbors[3 * i + j] < 0)
			{
				vertDegs[indices[3 * i + j]]++;
				vertDegs[indices[3 * i + (j + 1) % 3]]++;
			}
		}
	}

	// create random triangle permutation
	std::vector<int> permutation;
	permutation.resize(numTris);
	for (int i = 0; i < numTris; i++)
	{
		permutation[i] = i;
	}

	physx::PxU32 random = 0;
	for (int i = 0; i < numTris - 1; i++)
	{
		random = random * 1664525 + 1013904223;
		int j = i + random % (numTris - i);
		int p = permutation[i];
		permutation[i] = permutation[j];
		permutation[j] = p;
	}

	// flip edges
	for (int iters = 0; iters < numIters; iters++)
	{
		for (int i = 0; i < numTris; i++)
		{
			int t = permutation[i];
			for (int j = 0; j < 3; j++)
			{
				int n = neighbors[3 * t + j];
				if (n < t)
				{
					continue;
				}
				// determine the indices of the two triangles involved
				physx::PxU32& i0 = indices[3 * t + j];
				physx::PxU32& i1 = indices[3 * t + (j + 1) % 3];
				physx::PxU32& i2 = indices[3 * t + (j + 2) % 3];

				int backNr = -1;
				if (neighbors[3 * n] == t)
				{
					backNr = 0;
				}
				else if (neighbors[3 * n + 1] == t)
				{
					backNr = 1;
				}
				else if (neighbors[3 * n + 2] == t)
				{
					backNr = 2;
				}
				PX_ASSERT(backNr >= 0);
				//physx::PxU32& j0 = indices[3 * n + backNr];
				physx::PxU32& j1 = indices[3 * n + (backNr + 1) % 3];
				physx::PxU32& j2 = indices[3 * n + (backNr + 2) % 3];

				// do we want to flip?

				// geometrical tests
				physx::PxVec3& p0 = mVertices[i0];
				physx::PxVec3& p1 = mVertices[i1];
				physx::PxVec3& q0 = mVertices[i2];
				physx::PxVec3& q1 = mVertices[j2];

				// does the triangle pair form a convex shape?
				float r = (p0 - p1).magnitudeSquared();
				if (r == 0.0f)
				{
					continue;
				}

				float s = (q0 - p0).dot(p1 - p0) / r;
				if (s < 0.2f || s > 0.8f)
				{
					continue;
				}
				s = (q1 - p0).dot(p1 - p0) / r;
				if (s < 0.2f || s > 0.8f)
				{
					continue;
				}

				// the new edge shoulndnot be significantly longer than the old one
				float d0 = (mVertices[i0] - mVertices[i1]).magnitude();
				float d1 = (mVertices[i2] - mVertices[j2]).magnitude();
				if (d1 > 2.0f * d0)
				{
					continue;
				}

				// will the flip even out the degrees?
				int deg0 = vertDegs[i0];
				int deg1 = vertDegs[i1];
				int deg2 = vertDegs[i2];
				int deg3 = vertDegs[j2];

				int min = physx::PxMin(deg0, physx::PxMin(deg1, physx::PxMin(deg2, deg3)));
				int max = physx::PxMax(deg0, physx::PxMax(deg1, physx::PxMax(deg2, deg3)));
				int span = max - min;

				deg0--;
				deg1--;
				deg2++;
				deg3++;

				min = physx::PxMin(deg0, physx::PxMin(deg1, physx::PxMin(deg2, deg3)));
				max = physx::PxMax(deg0, physx::PxMax(deg1, physx::PxMax(deg2, deg3)));

				if (max - min > span)
				{
					continue;
				}


				// update degrees
				vertDegs[i0]--;
				vertDegs[i1]--;
				vertDegs[i2]++;
				vertDegs[j2]++;

				// flip
				i1 = j2;
				j1 = i2;

				// update neighbors
				int& ni0 = neighbors[3 * t + j];
				int& ni1 = neighbors[3 * t + (j + 1) % 3];

				int& nj0 = neighbors[3 * n + backNr];
				int& nj1 = neighbors[3 * n + (backNr + 1) % 3];

				ni0 = nj1;
				nj0 = ni1;
				ni1 = n;
				nj1 = t;

				// fix backwards links
				if (ni0 >= 0)
				{
					if (neighbors[3 * ni0] == n)
					{
						neighbors[3 * ni0] = t;
					}
					else if (neighbors[3 * ni0 + 1] == n)
					{
						neighbors[3 * ni0 + 1] = t;
					}
					else if (neighbors[3 * ni0 + 2] == n)
					{
						neighbors[3 * ni0 + 2] = t;
					}
					else
					{
						PX_ALWAYS_ASSERT();
					}
				}
				if (nj0 >= 0)
				{
					if (neighbors[3 * nj0] == t)
					{
						neighbors[3 * nj0] = n;
					}
					else if (neighbors[3 * nj0 + 1] == t)
					{
						neighbors[3 * nj0 + 1] = n;
					}
					else if (neighbors[3 * nj0 + 2] == t)
					{
						neighbors[3 * nj0 + 2] = n;
					}
					else
					{
						PX_ALWAYS_ASSERT();
					}
				}
			}
		}
	}
}

// ----------------------------------------------------------------------
void TriangleMesh::setCullMode(physx::NxRenderCullMode::Enum cullMode, physx::PxI32 submeshIndex)
{
	if (submeshIndex < 0)
	{
		for (physx::PxU32 i = 0; i < mSubMeshes.size(); i++)
		{
			setCullMode(cullMode, i);
		}
	}
	else if ((physx::PxU32)submeshIndex < mSubMeshes.size())
	{
		if (cullMode != mSubMeshes[submeshIndex].cullMode)
		{
			mSubMeshes[submeshIndex].cullMode = cullMode;
			oneCullModeChanged = true;
		}
	}
}

// ----------------------------------------------------------------------
void TriangleMesh::updatePaintingColors(PaintChannelType channelType, float maxDistMin, float maxDistMax, unsigned int flag, physx::apex::NxApexRenderDebug* batcher)
{
	const physx::PxU32 colorBlack = batcher != NULL ? batcher->getDebugColor(physx::DebugColors::Black) : 0xff000000;
	const physx::PxU32 colorWhite = batcher != NULL ? batcher->getDebugColor(physx::DebugColors::White) : 0xffffffff;
	const physx::PxU32 colorLightBlue = batcher != NULL ? batcher->getDebugColor(physx::DebugColors::LightBlue) : 0xff7777ff;
	const physx::PxU32 colorRed = batcher != NULL ? batcher->getDebugColor(physx::DebugColors::Red) : 0xffff0000;

	const physx::PxU32 colorDisabled = batcher != NULL ? batcher->getDebugColor(physx::DebugColors::Purple) : 0xffff00ff;
	const physx::PxU32 colorSmall = batcher != NULL ? batcher->getDebugColor(physx::DebugColors::Blue) : 0xff0000ff;;
	const physx::PxU32 colorBig = colorRed;


	if (channelType == PC_MAX_DISTANCE || channelType == PC_NUM_CHANNELS)
	{
		for (physx::PxU32 i = 0; i < mPaintChannels[PC_MAX_DISTANCE].size(); i++)
		{
			const float val = mPaintChannels[PC_MAX_DISTANCE][i].paintValueF32;
			if (val < 0.0f)
			{
				mPaintChannels[PC_MAX_DISTANCE][i].color = colorDisabled;
			}
			else if (val < maxDistMin)
			{
				mPaintChannels[PC_MAX_DISTANCE][i].color = colorSmall;
			}
			else if (val > maxDistMax)
			{
				mPaintChannels[PC_MAX_DISTANCE][i].color = colorBig;
			}
			else
			{
				const float setVal = (val - maxDistMin) / (maxDistMax - maxDistMin);
				mPaintChannels[PC_MAX_DISTANCE][i].setColor(setVal, setVal, setVal);
			}
		}
	}
	if (channelType == PC_COLLISION_DISTANCE || channelType == PC_NUM_CHANNELS)
	{
		for (physx::PxU32 i = 0; i < mPaintChannels[PC_COLLISION_DISTANCE].size(); i++)
		{
			const float v = mPaintChannels[PC_COLLISION_DISTANCE][i].paintValueF32;
			physx::PxVec3 color;
			if (physx::PxAbs(v) > 1.0f)
			{
				color = physx::PxVec3(0.0f, 0.0f, 0.0f);
			}
			else if (v == 0.0f)
			{
				color = physx::PxVec3(1.0f, 1.0f, 1.0f);
			}
			else if (v < 0.0f)
			{
				color = physx::PxVec3(1.0f, -v, 0.0f);
			}
			else // v > 0.0f
			{
				color = physx::PxVec3(0.0f, v, 1.0f);
			}
			mPaintChannels[PC_COLLISION_DISTANCE][i].setColor(color.x, color.y, color.z);
		}
	}

	if (channelType == PC_LATCH_TO_NEAREST_SLAVE || channelType == PC_NUM_CHANNELS)
	{
		for (size_t i = 0; i < mPaintChannels[PC_LATCH_TO_NEAREST_SLAVE].size(); i++)
		{
			const unsigned int slave = mPaintChannels[PC_LATCH_TO_NEAREST_SLAVE][i].paintValueU32;
			const unsigned int master = mPaintChannels[PC_LATCH_TO_NEAREST_MASTER][i].paintValueU32;

			unsigned int color;
			if (slave == 0)
			{
				if (master == 0)
				{
					color = colorRed;
				}
				else
				{
					color = colorBlack;
				}
			}
			else
			{
				if (flag != 0 && (slave & flag) == flag)
				{
					color = colorWhite;
				}
				else
				{
					color = colorLightBlue;
				}
			}

			mPaintChannels[PC_LATCH_TO_NEAREST_SLAVE][i].color = color;
		}
	}
	if (channelType == PC_LATCH_TO_NEAREST_MASTER || channelType == PC_NUM_CHANNELS)
	{
		for (size_t i = 0; i < mPaintChannels[PC_LATCH_TO_NEAREST_MASTER].size(); i++)
		{
			const unsigned int slave = mPaintChannels[PC_LATCH_TO_NEAREST_SLAVE][i].paintValueU32;
			const unsigned int master = mPaintChannels[PC_LATCH_TO_NEAREST_MASTER][i].paintValueU32;

			unsigned int color;
			if (slave != 0)
			{
				color = colorDisabled;
			}
			else
			{
				if (flag != 0 && (master & flag) == flag)
				{
					color = colorWhite;
				}
				else if (master != 0)
				{
					color = colorLightBlue;
				}
				else
				{
					color = colorRed;
				}
			}

			mPaintChannels[PC_LATCH_TO_NEAREST_MASTER][i].color = color;
		}
	}
}

// ----------------------------------------------------------------------
bool TriangleMesh::processElement(const char* elementName, int argc, const char** argv, const char* /*elementData*/, int /*lineno*/)
{
	if (strcmp(elementName, "mesh") == 0)
	{
		PX_ASSERT(mParserState == PS_Uninitialized);
		mParserState = PS_Mesh;
	}
	else if (strcmp(elementName, "sharedgeometry") == 0)
	{
		PX_ASSERT(argc == 2);
		PX_ASSERT(strcmp(argv[0], "vertexcount") == 0);
		const int vertexCount = atoi(argv[1]);
		mVertices.resize(0);
		mVertices.reserve(vertexCount);
	}
	else if (strcmp(elementName, "vertexbuffer") == 0)
	{
		for (int i = 0; i < argc; i += 2)
		{
			if (strcmp(argv[i], "normals") == 0)
			{
				mNormals.reserve(mVertices.capacity());
			}

			else if (strcmp(argv[i], "texture_coord_dimensions_0") == 0)
			{
				mTexCoords[0].reserve(mVertices.capacity());
			}
		}
	}
	else if (strcmp(elementName, "vertex") == 0)
	{
		int a = 0;
		PX_UNUSED(a);

		// do nothing?
	}
	else if (strcmp(elementName, "position") == 0)
	{
		PX_ASSERT(argc == 6);
		PX_ASSERT(strcmp(argv[0], "x") == 0);
		PX_ASSERT(strcmp(argv[2], "y") == 0);
		PX_ASSERT(strcmp(argv[4], "z") == 0);
		physx::PxVec3 pos;
		pos.x = (physx::PxF32)atof(argv[1]);
		pos.y = (physx::PxF32)atof(argv[3]);
		pos.z = (physx::PxF32)atof(argv[5]);
		mVertices.push_back(pos);
	}
	else if (strcmp(elementName, "normal") == 0)
	{
		PX_ASSERT(argc == 6);
		PX_ASSERT(strcmp(argv[0], "x") == 0);
		PX_ASSERT(strcmp(argv[2], "y") == 0);
		PX_ASSERT(strcmp(argv[4], "z") == 0);
		physx::PxVec3 normal;
		normal.x = (physx::PxF32)atof(argv[1]);
		normal.y = (physx::PxF32)atof(argv[3]);
		normal.z = (physx::PxF32)atof(argv[5]);
		mNormals.push_back(normal);
	}
	else if (strcmp(elementName, "texcoord") == 0)
	{
		PX_ASSERT(argc == 4);
		PX_ASSERT(strcmp(argv[0], "u") == 0);
		PX_ASSERT(strcmp(argv[2], "v") == 0);
		physx::NxVertexUV tc;
		tc.u = (physx::PxF32)atof(argv[1]);
		tc.v = (physx::PxF32)atof(argv[3]);
		mTexCoords[0].push_back(tc);
	}
	else if (strcmp(elementName, "colour_diffuse") == 0)
	{
		int a = 0;
		PX_UNUSED(a);
	}
	else if (strcmp(elementName, "physics_coeffs") == 0)
	{
		for (int i = 0; i < PC_NUM_CHANNELS; i++)
		{
			if (mPaintChannels[i].capacity() < mVertices.size())
			{
				mPaintChannels[i].reserve(mVertices.size());
			}

			if (i * 2 < argc)
			{
				float value = (float)atof(argv[i * 2 + 1]);
				mPaintChannels[i].push_back(PaintedVertex(value));
			}
		}
	}
	else if (strcmp(elementName, "submeshes") == 0)
	{
		PX_ASSERT(mParserState == PS_Mesh);
		mParserState = PS_Submeshes;
	}
	else if (strcmp(elementName, "submesh") == 0)
	{
		PX_ASSERT(argc >= 2);
		PX_ASSERT(strcmp(argv[0], "material") == 0);
		TriangleSubMesh sm;
		sm.init();
		sm.name = argv[1]; // ogre xml doesn't have submesh names...
		sm.materialName = argv[1];
		sm.firstIndex = (physx::PxU32)mIndices.size();
		if (sm.materialName.empty())
		{
			char buf[64];
			physx::string::sprintf_s(buf, 64, "Material_%2d", mSubMeshes.size());
			sm.materialName = buf;
		}
		sm.originalMaterialName = sm.materialName;
		mSubMeshes.push_back(sm);
	}
	else if (strcmp(elementName, "faces") == 0)
	{
		PX_ASSERT(argc == 2);
		PX_ASSERT(strcmp(argv[0], "count") == 0);
		const int faceCount = atoi(argv[1]);
		mIndices.reserve(mIndices.size() + faceCount * 3);
		PX_ASSERT(mSubMeshes.size() > 0);
		PX_ASSERT(mSubMeshes.back().numIndices == 0);
		mSubMeshes.back().numIndices = faceCount * 3;
	}
	else if (strcmp(elementName, "face") == 0)
	{
		PX_ASSERT(argc == 6);
		PX_ASSERT(strcmp(argv[0], "v1") == 0);
		PX_ASSERT(strcmp(argv[2], "v2") == 0);
		PX_ASSERT(strcmp(argv[4], "v3") == 0);
		mIndices.push_back(atoi(argv[1]));
		mIndices.push_back(atoi(argv[3]));
		mIndices.push_back(atoi(argv[5]));
	}
	else if (strcmp(elementName, "skeletonlink") == 0)
	{
		PX_ASSERT(mParserState == PS_Submeshes);
		mParserState = PS_Skeleton;
	}
	else if (strcmp(elementName, "boneassignments") == 0)
	{
		// do nothing?
		mBoneIndicesExternal.resize(mVertices.capacity() * 4, 0);
		mBoneWeights.resize(mVertices.capacity(), physx::PxVec4(0.0f));
	}
	else if (strcmp(elementName, "vertexboneassignment") == 0)
	{
		PX_ASSERT(argc == 6);
		PX_ASSERT(strcmp(argv[0], "vertexindex") == 0);
		PX_ASSERT(strcmp(argv[2], "boneindex") == 0);
		PX_ASSERT(strcmp(argv[4], "weight") == 0);

		const int vertexNr = atoi(argv[1]);
		const int boneNr = atoi(argv[3]);
		const float weight = (float)atof(argv[5]);

		mMaxBoneIndexExternal = physx::PxMax(mMaxBoneIndexExternal, boneNr);
		PX_ASSERT(vertexNr < (int)mVertices.size());
		float* weights = &mBoneWeights[vertexNr].x;
		physx::PxU16* indices = &mBoneIndicesExternal[vertexNr * 4];
		for (physx::PxU32 i = 0; i < 4; i++)
		{
			if (weights[i] == 0)
			{
				PX_ASSERT(boneNr < 0xffff);
				weights[i] = weight;
				indices[i] = (physx::PxU16)boneNr;
				break;
			}
			if (weights[i] < weight)
			{
				// move all one back
				for (physx::PxU32 j = 3; j > i; j--)
				{
					weights[j] = weights[j - 1];
					indices[j] = indices[j - 1];
				}
				weights[i] = weight;
				indices[i] = (physx::PxU16)boneNr;
				break;
			}
#if 1
			// safety
			for (physx::PxU32 j = 0; j < 4; j++)
			{
				PX_ASSERT(weights[j] >= 0.0f);
				PX_ASSERT(weights[j] <= 1.0f);
			}
			for (physx::PxU32 j = 0; j < 3; j++)
			{
				PX_ASSERT(weights[j] >= weights[j + 1]);
			}
#endif
		}
	}
	else
	{
		if (mParserState == PS_Uninitialized)
		{
			clear(NULL, NULL);
			return false;
		}
		PX_ALWAYS_ASSERT();
	}

	return true;
}



bool TriangleMesh::hasSkinningVertices()
{
	return mSkinnedVertices.size() == mVertices.size();
}



// -----------------[ private methods ]----------------------------------

// ----------------------------------------------------------------------
void TriangleMesh::updateNormals(int subMeshNr)
{
	if (mParent != NULL)
	{
		mParent->updateNormals(subMeshNr);
	}
	else
	{
		std::vector<physx::PxVec3> newNormals(mVertices.size(), physx::PxVec3(0.0f, 0.0f, 0.0f));

		const physx::PxU32 start = subMeshNr < 0 ? 0 : mSubMeshes[subMeshNr].firstIndex;
		const physx::PxU32 end = subMeshNr < 0 ? (physx::PxU32)mIndices.size() : mSubMeshes[subMeshNr].firstIndex + mSubMeshes[subMeshNr].numIndices;
		for (physx::PxU32 i = start; i < end; i += 3)
		{
			const physx::PxU32 i0 = mIndices[i + 0];
			const physx::PxU32 i1 = mIndices[i + 1];
			const physx::PxU32 i2 = mIndices[i + 2];

			physx::PxVec3 n = (mVertices[i1] - mVertices[i0]).cross(mVertices[i2] - mVertices[i0]);
			newNormals[i0] += n;
			newNormals[i1] += n;
			newNormals[i2] += n;
		}

		for (physx::PxU32 i = 0; i < newNormals.size(); i++)
		{
			if (newNormals[i].isZero())
			{
				if (i < mNormals.size())
				{
					newNormals[i] = mNormals[i];
				}
				else
				{
					newNormals[i] = physx::PxVec3(0.0f, 1.0f, 0.0f);
				}
			}
			else
			{
				newNormals[i].normalize();
			}
		}

		mNormals.resize(mVertices.size());

		for (physx::PxU32 i = 0; i < mNormals.size(); i++)
		{
			mNormals[i] = newNormals[i];
		}
	}
}

// ----------------------------------------------------------------------
void TriangleMesh::updateTangents()
{
	const int useTexCoords = 0;
	if (mParent != NULL)
	{
		mParent->updateTangents();
	}
	else if ((mTangents.empty() && mBitangents.empty()) || mTexCoords[useTexCoords].empty())
	{
		mTangents.clear();
		mBitangents.clear();
		// do nothing, no tangents!
	}
	else if (mTangents.size() != mVertices.size() || mTangents.size() != mBitangents.size() && mVertices.size() > 0 && mTexCoords[useTexCoords].size() == mVertices.size())
	{
		mTangents.clear();
		mTangents.resize(mVertices.size(), physx::PxVec3(0.0f, 0.0f, 0.0f));
		mBitangents.clear();
		mBitangents.resize(mVertices.size(), physx::PxVec3(0.0f, 0.0f, 0.0f));

		PX_ASSERT(mTangents[0].isZero());


		for (physx::PxU32 i = 0; i < mIndices.size(); i += 3)
		{
			const physx::PxVec3& p0 = mVertices[mIndices[i + 0]];
			const physx::PxVec3& p1 = mVertices[mIndices[i + 1]];
			const physx::PxVec3& p2 = mVertices[mIndices[i + 2]];

			const physx::NxVertexUV& w0 = mTexCoords[useTexCoords][mIndices[i + 0]];
			const physx::NxVertexUV& w1 = mTexCoords[useTexCoords][mIndices[i + 1]];
			const physx::NxVertexUV& w2 = mTexCoords[useTexCoords][mIndices[i + 2]];

			const float x1 = p1.x - p0.x;
			const float x2 = p2.x - p0.x;
			const float y1 = p1.y - p0.y;
			const float y2 = p2.y - p0.y;
			const float z1 = p1.z - p0.z;
			const float z2 = p2.z - p0.z;

			const float s1 = w1.u - w0.u;
			const float s2 = w2.u - w0.u;
			const float t1 = w1.v - w0.v;
			const float t2 = w2.v - w0.v;
			
			const float div = (s1 * t2 - s2 * t1);
			if (div > 0.0f)
			{
				const float r = 1.0F / div;
				physx::PxVec3 sdir((t2 * x1 - t1 * x2) * r, (t2 * y1 - t1 * y2) * r, (t2 * z1 - t1 * z2) * r);
				physx::PxVec3 tdir((s1 * x2 - s2 * x1) * r, (s1 * y2 - s2 * y1) * r, (s1 * z2 - s2 * z1) * r);

				mTangents[mIndices[i + 0]] += sdir;
				mTangents[mIndices[i + 1]] += sdir;
				mTangents[mIndices[i + 2]] += sdir;

				mBitangents[mIndices[i + 0]] += tdir;
				mBitangents[mIndices[i + 1]] += tdir;
				mBitangents[mIndices[i + 2]] += tdir;
			}
		}

		for (physx::PxU32 i = 0; i < mVertices.size(); i++)
		{
			physx::PxVec3& t = mTangents[i];
			physx::PxVec3& bt = mBitangents[i];

			// ortho-normalize tangents
			const physx::PxVec3& n = mNormals[i];
			t -= n * n.dot(t);
			t.normalize();

			const physx::PxVec3 nxt = n.cross(t);
			const physx::PxF32 sign = bt.dot(nxt) < 0.0f ? -1.0f : 1.0f;
			bt = nxt * sign;
		}
	}
}

// ----------------------------------------------------------------------
void TriangleMesh::updateBoneWeights()
{
	if (mParent != NULL)
	{
		mParent->updateBoneWeights();
	}
	else
	{
		if (mBoneWeights.size() == mVertices.size())
		{
			size_t count = (mVertices.size() + 15) / 16;
			mNumBoneWeights.resize(count, 0xffffffff);

			for (unsigned int i = 0; i < mVertices.size(); i++)
			{
				physx::PxVec4 boneWeight = mBoneWeights[i];
				int maxWeight = -1;
				for (int j = 0; j < 4; j++)
				{
					maxWeight += boneWeight[j] > 0 ? 1 : 0;
					PX_ASSERT((maxWeight == j) != (boneWeight[j] == 0.0f));
				}

				const unsigned int index = i / 16;
				const unsigned int offset = (i - (index * 16)) * 2;
				int newValue = mNumBoneWeights[index];
				newValue = (newValue & ~(0x3 << offset)) | maxWeight << offset;
				mNumBoneWeights[index] = newValue;
			}
		}
		else
		{
			PX_ASSERT(mBoneWeights.empty());
			mNumBoneWeights.clear();
		}

		optimizeForRendering();
	}
}

// ----------------------------------------------------------------------
void TriangleMesh::optimizeForRendering()
{
	PX_ASSERT(mParent == NULL);

	if (mBoneWeights.size() == mVertices.size() && mVertices.size() > 0)
	{
		PX_ASSERT(mMaxBoneIndexExternal >= 0);
		// basically compress the bone indices
		std::vector<int> ext2int(mMaxBoneIndexExternal + 1, -1);
		unsigned int maxBones = 0;

		for (size_t v = 0; v < mVertices.size(); v++)
		{
			for (unsigned int k = 0; k < 4; k++)
			{
				if (mBoneWeights[v][k] > 0.0f)
				{
					unsigned int boneIndex = mBoneIndicesExternal[v * 4 + k];

					if (ext2int[boneIndex] == -1)
					{
						ext2int[boneIndex] = maxBones++;
					}
				}
			}
		}

		mBoneIndicesInternal.resize(mBoneIndicesExternal.size(), 0);

		for (size_t v = 0; v < mVertices.size(); v++)
		{
			for (unsigned int k = 0; k < 4; k++)
			{
				unsigned int index = (unsigned int)v * 4 + k;
				if (mBoneWeights[v][k] > 0.0f)
				{
					mBoneIndicesInternal[index] = (unsigned short)ext2int[mBoneIndicesExternal[index]];
				}
				else
				{
					mBoneIndicesInternal[index] = 0;
				}
			}
		}
		mMaxBoneIndexInternal = (int)maxBones - 1;

		mBoneMappingInt2Ext.resize(maxBones);
		for (size_t i = 0; i < ext2int.size(); i++)
		{
			if (ext2int[i] >= 0)
			{
				PX_ASSERT(ext2int[i] < (int)mBoneMappingInt2Ext.size());
				mBoneMappingInt2Ext[ext2int[i]] = (int)i;
			}
		}
	}


#if 0 // PH: too advanced yet, and only a first draft

	for (size_t submeshIndex = 0; submeshIndex < mSubMeshes.size(); submeshIndex++)
	{
		TriangleSubMesh& submesh = mSubMeshes[submeshIndex];

		if (submesh.maxBonesShader > 0)
		{
			unsigned int indicesMapped = 0;
			//while (indicesMapped < submesh.numIndices)
			{
				//OptimizedRenderData renderData;

				std::vector<int> currentBoneMapping(mMaxBoneIndex + 1, -1);
				unsigned int currentBoneMappingSize = 0;

				for (unsigned int i = 0; i < submesh.numIndices; i += 3)
				{
					// collect all the bone indices
					unsigned int boneIndices[12];
					unsigned int numBoneIndices = 0;

					for (int j = 0; j < 3; j++)
					{
						const unsigned int vertexIndex = mIndices[submesh.firstIndex + i + j];
						for (int k = 0; k < 4; k++)
						{
							if (mBoneWeights[vertexIndex][k] > 0.0f)
							{
								unsigned int boneIndex = mBoneIndices[vertexIndex * 4 + k];

								bool found = false;

								for (unsigned int l = 0; l < numBoneIndices; l++)
								{
									if (boneIndices[l] == boneIndex)
									{
										found = true;
									}
								}

								if (!found)
								{
									boneIndices[numBoneIndices++] = boneIndex;
								}
							}
						}
					}

					// now let's see if they fit into the current mapping
					unsigned int notInMapping = 0;
					for (unsigned int j = 0; j < numBoneIndices; j++)
					{
						if (currentBoneMapping[boneIndices[j]] < 0)
						{
							notInMapping++;
						}
					}

					if (notInMapping == 0)
					{
						//indicesMapped += 3;
					}
					else if (notInMapping < submesh.maxBonesShader - currentBoneMappingSize)
					{
						// add them to the mapping
						for (unsigned int j = 0; j < numBoneIndices; j++)
						{
							if (currentBoneMapping[boneIndices[j]] < 0)
							{
								currentBoneMapping[boneIndices[j]] = currentBoneMappingSize++;
							}
						}

						//indicesMapped += 3;
					}
					else
					{
						// we're full I guess
						int a = 0;
						a = a;
					}
				}
			}

			int a = 0;
			a = a;
		}
	}
#endif
}

// ----------------------------------------------------------------------
void TriangleMesh::complete(bool useCustomChannels)
{
	for (physx::PxU32 i = 0; i < PC_NUM_CHANNELS; i++)
	{
		std::vector<PaintedVertex> &channel = mPaintChannels[i];
		if (useCustomChannels && (channel.size() == 0 || channel.size() != mVertices.size()))
		{
			channel.clear();
			switch (i)
			{
			case PC_MAX_DISTANCE:
				channel.resize(mVertices.size(), PaintedVertex(-0.1f));
				break;
			case PC_COLLISION_DISTANCE:
				channel.resize(mVertices.size(), PaintedVertex(-1.1f));
				break;
			case PC_LATCH_TO_NEAREST_SLAVE:
				channel.resize(mVertices.size(), PaintedVertex(0, 0.0f));
				break;
			case PC_LATCH_TO_NEAREST_MASTER:
				channel.resize(mVertices.size(), PaintedVertex(0xffffffff, 0.0f));
				break;
			default:
#if defined(PX_WINDOWS)
				DebugBreak();
#endif
				break;
			}
		}
		else if (!useCustomChannels)
		{
			channel.clear();
			channel.resize(0);
		}
	}

	if (mSubMeshes.size() < 1)
	{
		mSubMeshes.resize(1);
		mSubMeshes[0].init();
		mSubMeshes[0].firstIndex = 0;
		mSubMeshes[0].numIndices = (physx::PxU32)mIndices.size();
		mSubMeshes[0].name = "One Single Mesh";
	}
	else if (mSubMeshes.size() == 1)
	{
		mSubMeshes[0].firstIndex = 0;
		mSubMeshes[0].numIndices = (physx::PxU32)mIndices.size();
	}
	else
	{
		// Make sure all submeshes have distinct areas of the index buffers
		for (physx::PxU32 i = 0; i < mSubMeshes.size(); i++)
		{
			if (mSubMeshes[i].name.empty())
			{
				char buf[32];
				physx::string::sprintf_s(buf, 32, "Submesh %d", i);
				mSubMeshes[i].name = buf;
			}
			else
			{
				if (mSubMeshes[i].name.find("_invisible") != std::string::npos)
				{
					mSubMeshes[i].invisible = true;
				}
			}
			for (physx::PxU32 j = i + 1; j < mSubMeshes.size(); j++)
			{
				PX_ASSERT(mSubMeshes[i].firstIndex + mSubMeshes[i].numIndices <= mSubMeshes[j].firstIndex ||
				          mSubMeshes[j].firstIndex + mSubMeshes[j].numIndices <= mSubMeshes[i].firstIndex);
			}
		}
		// Merge submeshes
		std::sort(mSubMeshes.begin(), mSubMeshes.end());
		std::vector<physx::PxU32> newIndices;
		newIndices.resize(mIndices.size());
		physx::PxU32 offset = 0;
		for (physx::PxU32 i = 0; i < mSubMeshes.size(); i++)
		{
			physx::PxU32 indexStart = mSubMeshes[i].firstIndex;
			for (physx::PxU32 j = 0; j < mSubMeshes[i].numIndices; j++)
			{
				newIndices[offset + j] = mIndices[indexStart + j];
			}
			mSubMeshes[i].firstIndex = offset;
			offset += mSubMeshes[i].numIndices;
		}
		mIndices.resize(newIndices.size());
		memcpy(&mIndices[0], &newIndices[0], sizeof(physx::PxU32) * newIndices.size());

		for (size_t i = mSubMeshes.size() - 1; i > 0; i--)
		{
			if (mSubMeshes[i].name == mSubMeshes[i - 1].name && mSubMeshes[i].materialName == mSubMeshes[i - 1].materialName)
			{
				mSubMeshes[i - 1].numIndices += mSubMeshes[i].numIndices;
				mSubMeshes[i].numIndices = 0;
			}
		}
		for (size_t i = mSubMeshes.size() - 1; i > 0; i--)
		{
			if (mSubMeshes[i].numIndices == 0)
			{
				mSubMeshes.erase(mSubMeshes.begin() + i);
			}
		}
	}

	if (mNormals.size() != mVertices.size())
	{
		updateNormals(-1);
	}

	if (mTangents.size() != mVertices.size())
	{
		updateTangents();
	}

	updateBoneWeights();
	updateBounds();
}

// ----------------------------------------------------------------------
void TriangleMesh::hasRandomColors(size_t howmany)
{
	for (size_t i = mRandomColors.size(); i < howmany; i++)
	{
		const float h = physx::rand(0.0f, 359.9f);
		const float s = physx::rand(0.5f, 1.0f);
		const float v = physx::rand(0.5f, 1.0f);

		const physx::PxU32 hi = (physx::PxU32)physx::PxFloor(h / 60.0f) % 6;
		const float f = (h / 60.0f - physx::PxFloor(h / 60.0f));
		const float p = v * (1.0f - s);
		const float q = v * (1.0f - f * s);
		const float t = v * (1.0f - (1.0f - f) * s);

		float r, g, b;
		switch (hi)
		{
		case 0:
			r = v;
			g = t;
			b = p;
			break;
		case 1:
			r = q;
			g = v;
			b = p;
			break;
		case 2:
			r = p;
			g = v;
			b = t;
			break;
		case 3:
			r = p;
			g = q;
			b = v;
			break;
		case 4:
			r = t;
			g = p;
			b = v;
			break;
		case 5:
			r = v;
			g = p;
			b = q;
			break;
		default:
			r = g = b = 0.0f;
			break;
		}
		union
		{
			physx::PxU32 color;
			physx::PxU8 components[4];
		};
		color = 0;
		components[0] = (physx::PxU8)(r * 255);
		components[1] = (physx::PxU8)(g * 255);
		components[2] = (physx::PxU8)(b * 255);
		mRandomColors.push_back(color);
	}
}

// ----------------------------------------------------------------------
void TriangleMesh::updateSubmeshInfo()
{
	physx::PxU32 selectionChanged = 0;
	physx::PxU32 numNotSelected = 0;
	for (physx::PxU32 i = 0; i < mSubMeshes.size(); i++)
	{
		selectionChanged += (mSubMeshes[i].selected != mSubMeshes[i].selectionActivated) ? 1 : 0;
		numNotSelected += mSubMeshes[i].selected ? 0 : 1;
	}
	if (mActiveSubmeshVertices.size() == mVertices.size() && selectionChanged == 0)
	{
		return;
	}

	mActiveSubmeshVertices.clear();
	if (numNotSelected == 0)
	{
		mActiveSubmeshVertices.resize(mVertices.size(), true);
		for (physx::PxU32 i = 0; i < mSubMeshes.size(); i++)
		{
			mSubMeshes[i].selectionActivated = mSubMeshes[i].selected;
		}

		return;
	}

	mActiveSubmeshVertices.resize(mVertices.size(), false);
	for (physx::PxU32 i = 0; i < mSubMeshes.size(); i++)
	{
		if (mSubMeshes[i].selected)
		{
			TriangleSubMesh& sm = mSubMeshes[i];
			for (physx::PxU32 j = sm.firstIndex; j < sm.firstIndex + sm.numIndices; j++)
			{
				mActiveSubmeshVertices[mIndices[j]] = true;
			}
		}
		mSubMeshes[i].selectionActivated = mSubMeshes[i].selected;
	}
}

//------------------------------------------------------------------------------------
int TriangleMesh::addSplitVert(int vertNr0, int vertNr1)
{
	int v0, v1;
	if (vertNr0 < vertNr1)
	{
		v0 = vertNr0;
		v1 = vertNr1;
	}
	else
	{
		v0 = vertNr1;
		v1 = vertNr0;
	}

	// do we already have the split vert?
	int i = mVertexFirstSplit[v0];
	while (i >= 0)
	{
		TriangleEdgeSplit& s = mVertexSplits[i];
		if (s.adjVertNr == v1)
		{
			return s.newVertNr;
		}
		i = s.next;
	}

	TriangleEdgeSplit sNew;
	sNew.adjVertNr = v1;
	sNew.newVertNr = (physx::PxU32)mVertices.size();

	// create interpolated vertex
	physx::PxVec3 v = (mVertices[v0] + mVertices[v1]) * 0.5f;
	mVertices.push_back(v);
	if (mSkinnedVertices.size() == mVertices.size() - 1)
	{
		physx::PxVec3 sv = (mSkinnedVertices[v0] + mSkinnedVertices[v1]) * 0.5f;
		mSkinnedVertices.push_back(sv);
	}

	for (int j = 0; j < PC_NUM_CHANNELS; j++)
	{
		// ok, special case, if it's collision distance, and one of them is < -1 then the new is as well
		float v = 0.0f;
		unsigned int u = 0;
		if (j == PC_LATCH_TO_NEAREST_SLAVE || j == PC_LATCH_TO_NEAREST_MASTER)
		{
			u = mPaintChannels[j][v0].paintValueU32 | mPaintChannels[j][v1].paintValueU32;    // bitwise OR
		}
		else if (j == PC_COLLISION_DISTANCE && (mPaintChannels[PC_COLLISION_DISTANCE][v0].paintValueF32 == -1.1f || mPaintChannels[PC_COLLISION_DISTANCE][v1].paintValueF32 == -1.1f))
		{
			v = -1.1f;
		}
		else
		{
			v = (mPaintChannels[j][v0].paintValueF32 + mPaintChannels[j][v1].paintValueF32) * 0.5f;
		}
		mPaintChannels[j].push_back(PaintedVertex(u, v));
	}

	for (physx::PxU32 j = 0; j < NUM_TEXCOORDS; j++)
	{
		if (mTexCoords[j].empty())
		{
			continue;
		}

		physx::NxVertexUV newTc;
		newTc.u = (mTexCoords[j][v0].u + mTexCoords[j][v1].u) * 0.5f;
		newTc.v = (mTexCoords[j][v0].v + mTexCoords[j][v1].v) * 0.5f;
		mTexCoords[j].push_back(newTc);
	}

	int newVertNr = (int)mVertices.size() - 1;
	PX_UNUSED(newVertNr);

	// mix bone weights - kind of tricky
#if 1
	if (!mBoneIndicesExternal.empty())
	{
		physx::PxU16 newIndices[4];
		physx::PxVec4 newWeights;
		for (int j = 0; j < 4; j++)
		{
			newIndices[j] = 0;
			newWeights[j] = 0.0f;
		}

		PX_ASSERT(!mBoneWeights.empty());

		for (int k = 0; k < 2; k++)
		{
			int v = (k == 0) ? v0 : v1;
			for (int j = 0; j < 4; j++)
			{
				physx::PxU16 bi = mBoneIndicesExternal[4 * v + j];
				float bw = mBoneWeights[v][j];

				// do we have the index already? If so just average the weights
				int k = 0;
				while (k < 4 && newIndices[k] != bi)
				{
					k++;
				}
				if (k < 4)
				{
					newWeights[k] = (newWeights[k] + bw) * 0.5f;
					continue;
				}
				// else insert the pair at the right place
				k = 3;
				while (k >= 0 && newWeights[k] < bw)
				{
					if (k < 3)
					{
						newWeights[k + 1] = newWeights[k];
						newIndices[k + 1] = newIndices[k];
					}
					k--;
				}
				k++;
				if (k < 4)
				{
					newWeights[k] = bw;
					newIndices[k] = bi;
				}
			}
		}

		// copy indices to the new vertex
		float sum = 0.0f;
		for (int j = 0; j < 4; j++)
		{
			sum += newWeights[j];
		}

		if (sum > 0.0f)
		{
			sum = 1.0f / sum;
		}

		for (int j = 0; j < 4; j++)
		{
			mBoneIndicesExternal.push_back(newIndices[j]);
		}
		mBoneWeights.push_back(newWeights * sum);
	}

#else
	mFirstAssignment.resize(newVertNr + 2);
	mFirstAssignment[newVertNr] = mBoneAssignments.size();

	std::vector<BoneAssignment> assigns;
	BoneAssignment a;

	for (int i = 0; i < 2; i++)
	{
		int v = (i == 0) ? v0 : v1;
		int first = mFirstAssignment[v];
		int num = mFirstAssignment[v + 1] - first;
		for (int j = 0; j < num; j++)
		{
			a.boneNr = mBoneAssignments[first + j];
			a.weight = mBoneWeights[first + j];
			int k = 0;
			while (k < (int)assigns.size() && a.boneNr != assigns[k].boneNr)
			{
				k++;
			}
			if (k < (int)assigns.size())
			{
				assigns[k].weight = (assigns[k].weight + a.weight) * 0.5f;
			}
			else
			{
				assigns.pushBack(a);
			}
		}
	}
	// select the ones with the biggest weights
	std::sort(assigns.begin(), assigns.end());
	const int maxAssignments = 4;
	int first = assigns.size() - maxAssignments;
	if (first < 0)
	{
		first = 0;
	}
	float sum = 0.0f;
	for (int i = first; i < (int)assigns.size(); i++)
	{
		sum += assigns[i].weight;
	}
	PX_ASSERT(sum != 0.0f);
	for (int i = first; i < (int)assigns.size(); i++)
	{
		mBoneAssignments.pushBack(assigns[i].boneNr);
		mBoneWeights.pushBack(assigns[i].weight / sum);
	}
	mFirstAssignment[newVertNr + 1] = mBoneAssignments.size();
#endif

	// add split vertex info
	sNew.next = mVertexFirstSplit[v0];
	mVertexFirstSplit[v0] = (physx::PxI32)mVertexSplits.size();
	mVertexSplits.push_back(sNew);
	return sNew.newVertNr;
}

} // namespace Samples
