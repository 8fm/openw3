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

#include "ApexRenderMeshAsset.h"
#include "ApexRenderMeshActor.h"
#include "ApexSharedUtils.h"

#include "NiApexSDK.h"
#include "NiResourceProvider.h"

namespace physx
{
namespace apex
{


// ApexRenderMeshAsset functions

ApexRenderMeshAsset::ApexRenderMeshAsset(NxResourceList& list, const char* name, NxAuthObjTypeID ownerModuleID) :
	mOwnerModuleID(ownerModuleID),
	mParams(NULL),
	mOpaqueMesh(NULL),
	mName(name)
{
	list.add(*this);
}



ApexRenderMeshAsset::~ApexRenderMeshAsset()
{
	// this should have been cleared in releaseActor()
	PX_ASSERT(mRuntimeSubmeshData.empty());

	// Release named resources
	NiResourceProvider* resourceProvider = NiGetApexSDK()->getInternalResourceProvider();
	for (physx::PxU32 i = 0 ; i < mMaterialIDs.size() ; i++)
	{
		resourceProvider->releaseResource(mMaterialIDs[i]);
	}

	setSubmeshCount(0);
}



void ApexRenderMeshAsset::destroy()
{
	for (PxU32 i = 0; i < mSubmeshes.size(); i++)
	{
		mSubmeshes[i]->setParams(NULL, NULL);
	}

	if (mParams != NULL)
	{
		if (!mParams->isReferenced)
		{
			mParams->destroy();
		}
		mParams = NULL;
	}

	// this is necessary so that all the actors will be destroyed before the destructor runs
	{
		ScopedWriteLock lock( mActorListLock );
		mActorList.clear();
	}

	delete this;
}



bool ApexRenderMeshAsset::createFromParameters(RenderMeshAssetParameters* params)
{
	mParams = params;

	NxParameterized::Handle handle(*mParams);
	int size;

	// submeshes
	mParams->getParameterHandle("submeshes", handle);
	mParams->getArraySize(handle, size);
	setSubmeshCount(size);
	for (int i = 0; i < size; ++i)
	{
		NxParameterized::Handle elementHandle(*mParams);
		handle.getChildHandle(i, elementHandle);
		NxParameterized::Interface* submeshParams = NULL;
		mParams->getParamRef(elementHandle, submeshParams);

		mSubmeshes[i]->setParams(static_cast<SubmeshParameters*>(submeshParams), NULL);
	}

	createLocalData();

	return true;
}

// Load all of our named resources (that consists of materials) if they are
// not registered in the NRP
physx::PxU32 ApexRenderMeshAsset::forceLoadAssets()
{
	physx::PxU32 assetLoadedCount = 0;
	NiResourceProvider* nrp = NiGetApexSDK()->getInternalResourceProvider();
	NxResID materialNS = NiGetApexSDK()->getMaterialNameSpace();

	for (physx::PxU32 i = 0; i < mMaterialIDs.size(); i++)
	{

		if (!nrp->checkResource(materialNS, mParams->materialNames.buf[i]))
		{
			/* we know for SURE that createResource() has already been called, so just getResource() */
			nrp->getResource(mMaterialIDs[i]);
			assetLoadedCount++;
		}
	}

	return assetLoadedCount;
}


NxRenderMeshActor* ApexRenderMeshAsset::createActor(const NxRenderMeshActorDesc& desc)
{
	ScopedWriteLock lock( mActorListLock );
	return PX_NEW(ApexRenderMeshActor)(desc, *this, mActorList);
}



void ApexRenderMeshAsset::releaseActor(NxRenderMeshActor& renderMeshActor)
{
	{
		ApexRenderMeshActor* actor = DYNAMIC_CAST(ApexRenderMeshActor*)(&renderMeshActor);
		ScopedWriteLock lock( mActorListLock );		// actor->destroy may end up touching the actor list
		actor->destroy();
	}

	// Last one out turns out the lights
	physx::PxU32 actorListSize;
	{
		ScopedReadLock lock( mActorListLock );
		actorListSize = mActorList.getSize();
	}
	if ( 0 == actorListSize )
	{
		NxUserRenderResourceManager* rrm = NiGetApexSDK()->getUserRenderResourceManager();
		for (physx::PxU32 i = 0 ; i < mRuntimeSubmeshData.size() ; i++)
		{
			if (mRuntimeSubmeshData[i].staticVertexBuffer != NULL)
			{
				rrm->releaseVertexBuffer(*mRuntimeSubmeshData[i].staticVertexBuffer);
				mRuntimeSubmeshData[i].staticVertexBuffer = NULL;
			}
			if (mRuntimeSubmeshData[i].skinningVertexBuffer != NULL)
			{
				rrm->releaseVertexBuffer(*mRuntimeSubmeshData[i].skinningVertexBuffer);
				mRuntimeSubmeshData[i].skinningVertexBuffer = NULL;
			}
			if (mRuntimeSubmeshData[i].dynamicVertexBuffer != NULL)
			{
				rrm->releaseVertexBuffer(*mRuntimeSubmeshData[i].dynamicVertexBuffer);
				mRuntimeSubmeshData[i].dynamicVertexBuffer = NULL;
			}
		}
		mRuntimeSubmeshData.clear();
	}
}



void ApexRenderMeshAsset::permuteBoneIndices(const physx::Array<physx::PxI32>& old2new)
{
	physx::PxI32 maxBoneIndex = -1;
	for (physx::PxU32 i = 0; i < mSubmeshes.size(); i++)
	{
		NxRenderDataFormat::Enum format;
		const NxVertexBuffer& vb = mSubmeshes[i]->getVertexBuffer();
		const NxVertexFormat& vf = vb.getFormat();
		physx::PxU32 bufferIndex = vf.getBufferIndexFromID(vf.getSemanticID(physx::NxRenderVertexSemantic::BONE_INDEX));
		physx::PxU16* boneIndices = (physx::PxU16*)vb.getBufferAndFormat(format, bufferIndex);
		if (boneIndices == NULL)
		{
			continue;
		}

		physx::PxU32 numBonesPerVertex = 0;
		switch (format)
		{
		case NxRenderDataFormat::USHORT1:
			numBonesPerVertex = 1;
			break;
		case NxRenderDataFormat::USHORT2:
			numBonesPerVertex = 2;
			break;
		case NxRenderDataFormat::USHORT3:
			numBonesPerVertex = 3;
			break;
		case NxRenderDataFormat::USHORT4:
			numBonesPerVertex = 4;
			break;
		default:
			continue;
		}

		const PxU32 numVertices = vb.getVertexCount();
		for (PxU32 j = 0; j < numVertices; j++)
		{
			for (PxU32 k = 0; k < numBonesPerVertex; k++)
			{
				physx::PxU16& index = boneIndices[j * numBonesPerVertex + k];
				PX_ASSERT(old2new[index] >= 0);
				PX_ASSERT(old2new[index] <= 0xffff);
				index = (physx::PxU16)old2new[index];
				maxBoneIndex = physx::PxMax(maxBoneIndex, (physx::PxI32)index);
			}
		}
	}
	mParams->boneCount = maxBoneIndex + 1;
}



void ApexRenderMeshAsset::applyTransformation(const physx::PxMat34Legacy& transformation, physx::PxF32 scale)
{
	for (physx::PxU32 submeshId = 0; submeshId < mSubmeshes.size(); submeshId++)
	{
		NiApexVertexBuffer& vb = mSubmeshes[submeshId]->getVertexBufferWritable();
		vb.applyTransformation(transformation);
		vb.applyScale(scale);
	}

	// if the transform will mirror the mesh, change the triangle winding in the ib
	if (transformation.M.determinant() * scale < 0.0f)
	{
		for (physx::PxU32 submeshId = 0; submeshId < mSubmeshes.size(); submeshId++)
		{
			physx::PxU32 numIndices = mSubmeshes[submeshId]->getTotalIndexCount();
			// assume that all of the parts are contiguous
			physx::PxU32* indices = mSubmeshes[submeshId]->getIndexBufferWritable(0);
			for (physx::PxU32 i = 0; i < numIndices; i += 3)
			{
				physx::swap<physx::PxU32>(indices[i + 1], indices[i + 2]);
			}
		}
	}

	updatePartBounds();
}



void ApexRenderMeshAsset::applyScale(physx::PxF32 scale)
{
	for (physx::PxU32 submeshId = 0; submeshId < mSubmeshes.size(); submeshId++)
	{
		NiApexVertexBuffer& vb = mSubmeshes[submeshId]->getVertexBufferWritable();
		vb.applyScale(scale);
	}

	for (int partId = 0; partId < mParams->partBounds.arraySizes[0]; partId++)
	{
		PX_ASSERT(!mParams->partBounds.buf[partId].isEmpty());
		mParams->partBounds.buf[partId].scaleFast(scale);
	}
}



bool ApexRenderMeshAsset::mergeBinormalsIntoTangents()
{
	bool changed = false;
	for (physx::PxU32 submeshId = 0; submeshId < mSubmeshes.size(); submeshId++)
	{
		NiApexVertexBuffer& vb = mSubmeshes[submeshId]->getVertexBufferWritable();
		changed |= vb.mergeBinormalsIntoTangents();
	}
	return changed;
}



NxTextureUVOrigin::Enum ApexRenderMeshAsset::getTextureUVOrigin() const
{
	PX_ASSERT(mParams->textureUVOrigin < 4);
	return static_cast<NxTextureUVOrigin::Enum>(mParams->textureUVOrigin);
}



void ApexRenderMeshAsset::createLocalData()
{
	mMaterialIDs.resize(mParams->materialNames.arraySizes[0]);
	NiResourceProvider* resourceProvider = NiGetApexSDK()->getInternalResourceProvider();
	NxResID materialNS = NiGetApexSDK()->getMaterialNameSpace();
	NxResID customVBNS = NiGetApexSDK()->getCustomVBNameSpace();


	// Resolve material names using the NRP...
	for (int i = 0; i < mParams->materialNames.arraySizes[0]; ++i)
	{
		if (resourceProvider)
		{
			PX_ASSERT(mParams->materialNames.buf[i] && mParams->materialNames.buf[i][0] != '\0');
			mMaterialIDs[i] = resourceProvider->createResource(materialNS, mParams->materialNames.buf[i]);
		}
		else
		{
			mMaterialIDs[i] = INVALID_RESOURCE_ID;
		}
	}

	// Resolve custom vertex buffer semantics using the NRP...
	mRuntimeCustomSubmeshData.resize(getSubmeshCount());
	//JPB memset(mRuntimeCustomSubmeshData.begin(), 0, sizeof(CustomSubmeshData) * mRuntimeCustomSubmeshData.size());

	for (physx::PxU32 i = 0; i < getSubmeshCount(); ++i)
	{
		const NxVertexFormat& fmt = getSubmesh(i).getVertexBuffer().getFormat();

		mRuntimeCustomSubmeshData[i].customBufferFormats.resize(fmt.getCustomBufferCount());
		mRuntimeCustomSubmeshData[i].customBufferResIDs.resize(fmt.getCustomBufferCount());
		mRuntimeCustomSubmeshData[i].customBufferVoidPtrs.resize(fmt.getCustomBufferCount());

		physx::PxU32 customBufferIndex = 0;
		for (physx::PxU32 j = 0; j < fmt.getBufferCount(); ++j)
		{
			if (fmt.getBufferSemantic(j) != NxRenderVertexSemantic::CUSTOM)
			{
				continue;
			}
			NxRenderDataFormat::Enum f = fmt.getBufferFormat(j);
			const char* name = fmt.getBufferName(j);
			if (resourceProvider)
				mRuntimeCustomSubmeshData[i].customBufferResIDs[customBufferIndex] =
				    resourceProvider->createResource(customVBNS, name, true);
			else
			{
				mRuntimeCustomSubmeshData[i].customBufferResIDs[customBufferIndex] = INVALID_RESOURCE_ID;
			}

			mRuntimeCustomSubmeshData[i].customBufferFormats[customBufferIndex] = f;
			mRuntimeCustomSubmeshData[i].customBufferVoidPtrs[customBufferIndex] = 0;

			++customBufferIndex;
		}
	}

	// find the bone count
	// LRR - required for new deserialize path
	// PH - mBoneCount is now serialized
	if (mParams->boneCount == 0)
	{
		for (physx::PxU32 i = 0; i < getSubmeshCount(); i++)
		{

			NxRenderDataFormat::Enum format;
			const NxVertexBuffer& vb = mSubmeshes[i]->getVertexBuffer();
			const NxVertexFormat& vf = vb.getFormat();
			physx::PxU32 bufferIndex = vf.getBufferIndexFromID(vf.getSemanticID(physx::NxRenderVertexSemantic::BONE_INDEX));
			physx::PxU16* boneIndices = (physx::PxU16*)vb.getBufferAndFormat(format, bufferIndex);

			if (boneIndices == NULL)
			{
				continue;
			}

			if (!vertexSemanticFormatValid(NxRenderVertexSemantic::BONE_INDEX, format))
			{
				continue;
			}

			const PxU32 bonesPerVert = vertexSemanticFormatElementCount(NxRenderVertexSemantic::BONE_INDEX, format);

			PX_ASSERT(format == NxRenderDataFormat::USHORT1 || format == NxRenderDataFormat::USHORT2 || format == NxRenderDataFormat::USHORT3 || format == NxRenderDataFormat::USHORT4);

			const PxU32 numVertices = vb.getVertexCount();
			for (PxU32 v = 0; v < numVertices; v++)
			{
				for (PxU32 b = 0; b < bonesPerVert; b++)
				{
					mParams->boneCount = physx::PxMax(mParams->boneCount, (physx::PxU32)(boneIndices[v * bonesPerVert + b] + 1));
				}
			}
		}
	}

	// PH - have one bone at all times, if it's just one, it is used as current pose (see ApexRenderMeshActor::dispatchRenderResources)
	if (mParams->boneCount == 0)
	{
		mParams->boneCount = 1;
	}
}

void ApexRenderMeshAsset::getStats(NxRenderMeshAssetStats& stats) const
{
	stats.totalBytes = sizeof(ApexRenderMeshAsset);

	for (int i = 0; i < mParams->materialNames.arraySizes[0]; ++i)
	{
		stats.totalBytes += (physx::PxU32) strlen(mParams->materialNames.buf[i]) + 1;
	}

	stats.totalBytes += mParams->partBounds.arraySizes[0] * sizeof(physx::PxBounds3);
	stats.totalBytes += mName.len() + 1;

	stats.submeshCount = mSubmeshes.size();
	stats.partCount = mParams->partBounds.arraySizes[0];
	stats.vertexCount = 0;
	stats.indexCount = 0;
	stats.vertexBufferBytes = 0;
	stats.indexBufferBytes = 0;

	for (physx::PxU32 i = 0; i < mSubmeshes.size(); ++i)
	{
		const ApexRenderSubmesh& submesh =	*mSubmeshes[i];

		submesh.addStats(stats);
	}
}


void ApexRenderMeshAsset::updatePartBounds()
{
	for (int i = 0; i < mParams->partBounds.arraySizes[0]; i++)
	{
		mParams->partBounds.buf[i].setEmpty();
	}

	for (physx::PxU32 i = 0; i < mSubmeshes.size(); i++)
	{
		const physx::PxU32* part = mSubmeshes[i]->mParams->vertexPartition.buf;

		NxRenderDataFormat::Enum format;
		const NxVertexBuffer& vb = mSubmeshes[i]->getVertexBuffer();
		const NxVertexFormat& vf = vb.getFormat();
		physx::PxU32 bufferIndex = vf.getBufferIndexFromID(vf.getSemanticID(physx::NxRenderVertexSemantic::POSITION));
		physx::PxVec3* positions = (physx::PxVec3*)vb.getBufferAndFormat(format, bufferIndex);
		if (positions == NULL)
		{
			continue;
		}
		if (format != NxRenderDataFormat::FLOAT3)
		{
			continue;
		}

		for (int p = 0; p < mParams->partBounds.arraySizes[0]; p++)
		{
			const physx::PxU32 start = part[p];
			const physx::PxU32 end = part[p + 1];
			for (physx::PxU32 v = start; v < end; v++)
			{
				mParams->partBounds.buf[p].include(positions[v]);
			}
		}
	}
}

void ApexRenderMeshAsset::setSubmeshCount(physx::PxU32 submeshCount)
{
	const physx::PxU32 oldSize = mSubmeshes.size();

	for (physx::PxU32 i = oldSize; i-- > submeshCount;)
	{
		PX_DELETE(mSubmeshes[i]);
	}

	mSubmeshes.resize(submeshCount);

	for (physx::PxU32 i = oldSize; i < submeshCount; ++i)
	{
		mSubmeshes[i] = PX_NEW(ApexRenderSubmesh);
	}
}


}
} // end namespace physx::apex
