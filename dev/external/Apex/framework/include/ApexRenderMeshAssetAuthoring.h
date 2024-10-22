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

#ifndef APEX_RENDERMESH_ASSET_AUTHORING_H
#define APEX_RENDERMESH_ASSET_AUTHORING_H

#include "ApexRenderMeshAsset.h"
#include "ApexSharedUtils.h"
#include "NiApexRenderMeshAsset.h"
#include "NiResourceProvider.h"
#include "ApexInterface.h"
#include "ApexActor.h"
#include "ApexAssetAuthoring.h"
#include "ApexString.h"
#include "ApexVertexFormat.h"
#include "ApexSDK.h"
#include "PsShare.h"

#ifndef WITHOUT_APEX_AUTHORING

namespace physx
{
namespace apex
{

// PHTODO, put this into the authoring asset
struct VertexReductionExtraData
{
	void	set(const NxExplicitRenderTriangle& xTriangle)
	{
		smoothingMask = xTriangle.smoothingMask;
	}

	bool	canMerge(const VertexReductionExtraData& other) const
	{
		return (smoothingMask & other.smoothingMask) != 0 || smoothingMask == 0 || other.smoothingMask == 0;
	}

	physx::PxU32 smoothingMask;
};


class ApexRenderMeshAssetAuthoring : public ApexRenderMeshAsset, public ApexAssetAuthoring, public NiApexRenderMeshAssetAuthoring
{
public:
	ApexRenderMeshAssetAuthoring(NxResourceList& list, RenderMeshAssetParameters* params, const char* name);

	void						release()
	{
		NxGetApexSDK()->releaseAssetAuthoring(*this);
	}

	void						createRenderMesh(const MeshDesc& desc, bool createMappingInformation);
	physx::PxU32				createReductionMap(PxU32* map, const NxVertex* vertices, const PxU32* smoothingGroups, PxU32 vertexCount,
	        const physx::PxVec3& positionTolerance, physx::PxF32 normalTolerance, physx::PxF32 UVTolerance);

	void						deleteStaticBuffersAfterUse(bool set)
	{
		ApexRenderMeshAsset::deleteStaticBuffersAfterUse(set);
	}

	const char*					getName(void) const
	{
		return ApexRenderMeshAsset::getName();
	}
	const char* 				getObjTypeName() const
	{
		return ApexRenderMeshAsset::getClassName();
	}
	bool						prepareForPlatform(physx::apex::NxPlatformTag)
	{
		APEX_INVALID_OPERATION("Not Implemented.");
		return false;
	}
	void						setToolString(const char* toolName, const char* toolVersion, PxU32 toolChangelist)
	{
		ApexAssetAuthoring::setToolString(toolName, toolVersion, toolChangelist);
	}
	physx::PxU32				getSubmeshCount() const
	{
		return ApexRenderMeshAsset::getSubmeshCount();
	}
	physx::PxU32				getPartCount() const
	{
		return ApexRenderMeshAsset::getPartCount();
	}
	const char* 				getMaterialName(physx::PxU32 submeshIndex) const
	{
		return ApexRenderMeshAsset::getMaterialName(submeshIndex);
	}
	void						setMaterialName(physx::PxU32 submeshIndex, const char* name);
	virtual void				setWindingOrder(physx::PxU32 submeshIndex, NxRenderCullMode::Enum winding);
	virtual NxRenderCullMode::Enum	getWindingOrder(physx::PxU32 submeshIndex) const;
	const NxRenderSubmesh&		getSubmesh(physx::PxU32 submeshIndex) const
	{
		return ApexRenderMeshAsset::getSubmesh(submeshIndex);
	}
	NxRenderSubmesh&			getSubmeshWritable(physx::PxU32 submeshIndex)
	{
		return *mSubmeshes[submeshIndex];
	}
	const physx::PxBounds3&		getBounds(physx::PxU32 partIndex = 0) const
	{
		return ApexRenderMeshAsset::getBounds(partIndex);
	}
	void						getStats(NxRenderMeshAssetStats& stats) const
	{
		ApexRenderMeshAsset::getStats(stats);
	}

	// From NiApexRenderMeshAssetAuthoring
	NiApexRenderSubmesh&		getNiSubmesh(physx::PxU32 submeshIndex)
	{
		return *ApexRenderMeshAsset::mSubmeshes[submeshIndex];
	}
	void						permuteBoneIndices(const physx::Array<physx::PxI32>& old2new)
	{
		ApexRenderMeshAsset::permuteBoneIndices(old2new);
	}
	void						applyTransformation(const physx::PxMat34Legacy& transformation, physx::PxF32 scale)
	{
		ApexRenderMeshAsset::applyTransformation(transformation, scale);
	}
	void						applyScale(physx::PxF32 scale)
	{
		ApexRenderMeshAsset::applyScale(scale);
	}
	NxParameterized::Interface*	getNxParameterized() const
	{
		return mParams;
	}
	/**
	 * \brief Releases the ApexAsset but returns the NxParameterized::Interface and *ownership* to the caller.
	 */
	virtual NxParameterized::Interface* releaseAndReturnNxParameterizedInterface(void)
	{
		NxParameterized::Interface* ret = mParams;
		mParams = NULL;
		release();
		return ret;
	}

protected:
	// helper structs
	struct VertexPart
	{
		PxU32	part, vertexIndex;
		PX_INLINE bool operator()(const VertexPart& a, const VertexPart& b) const
		{
			if (a.part != b.part)
			{
				return a.part < b.part;
			}
			return a.vertexIndex < b.vertexIndex;
		}
		PX_INLINE static int cmp(const void* A, const void* B)
		{
			// Sorts by part, then vertexIndex
			const int delta = (int)((VertexPart*)A)->part - (int)((VertexPart*)B)->part;
			return delta != 0 ? delta : ((int)((VertexPart*)A)->vertexIndex - (int)((VertexPart*)B)->vertexIndex);
		}
	};

	// helper methods
	template<typename PxU>
	bool fillSubmeshMap(physx::Array<VertexPart>& submeshMap, const void* const partIndicesVoid, PxU32 numParts,
	                    const void* const vertexIndicesVoid, PxU32 numSubmeshIndices, PxU32 numSubmeshVertices);

	// protected constructors
	ApexRenderMeshAssetAuthoring(NxResourceList& list);
	virtual ~ApexRenderMeshAssetAuthoring();
};

}
} // end namespace physx::apex

#endif // WITHOUT_APEX_AUTHORING

#endif // APEX_RENDERMESH_ASSET_H
