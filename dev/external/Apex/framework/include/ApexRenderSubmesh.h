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

#ifndef APEX_RENDER_SUBMESH_H
#define APEX_RENDER_SUBMESH_H

#include "NiApexRenderMeshAsset.h"
#include "ApexVertexBuffer.h"
#include "SubmeshParameters.h"

namespace physx
{
namespace apex
{

class ApexRenderSubmesh : public NiApexRenderSubmesh, public UserAllocated
{
public:
	ApexRenderSubmesh() : mParams(NULL) {}
	~ApexRenderSubmesh() {}

	// from NxRenderSubmesh
	virtual physx::PxU32				getVertexCount(physx::PxU32 partIndex) const
	{
		return mParams->vertexPartition.buf[partIndex + 1] - mParams->vertexPartition.buf[partIndex];
	}

	virtual const NiApexVertexBuffer&	getVertexBuffer() const
	{
		return mVertexBuffer;
	}

	virtual physx::PxU32				getFirstVertexIndex(physx::PxU32 partIndex) const
	{
		return mParams->vertexPartition.buf[partIndex];
	}

	virtual physx::PxU32				getIndexCount(physx::PxU32 partIndex) const
	{
		return mParams->indexPartition.buf[partIndex + 1] - mParams->indexPartition.buf[partIndex];
	}

	virtual const physx::PxU32*			getIndexBuffer(physx::PxU32 partIndex) const
	{
		return mParams->indexBuffer.buf + mParams->indexPartition.buf[partIndex];
	}

	virtual const physx::PxU32*			getSmoothingGroups(physx::PxU32 partIndex) const
	{
		return mParams->smoothingGroups.buf != NULL ? (mParams->smoothingGroups.buf + mParams->indexPartition.buf[partIndex]/3) : NULL;
	}


	// from NiApexRenderSubmesh
	virtual NiApexVertexBuffer&			getVertexBufferWritable()
	{
		return mVertexBuffer;
	}

	virtual PxU32*						getIndexBufferWritable(PxU32 partIndex)
	{
		return mParams->indexBuffer.buf + mParams->indexPartition.buf[partIndex];
	}

	virtual void						applyPermutation(const Array<PxU32>& old2new, const Array<PxU32>& new2old);

	// own methods

	physx::PxU32						getTotalIndexCount() const
	{
		return mParams->indexBuffer.arraySizes[0];
	}

	physx::PxU32*						getIndexBufferWritable(physx::PxU32 partIndex) const
	{
		return mParams->indexBuffer.buf + mParams->indexPartition.buf[partIndex];
	}

	bool								createFromParameters(SubmeshParameters* params);

	void								setParams(SubmeshParameters* submeshParams, VertexBufferParameters* vertexBufferParams);

	void								addStats(NxRenderMeshAssetStats& stats) const;

	void								buildVertexBuffer(const NxVertexFormat& format, PxU32 vertexCount);

	SubmeshParameters*  mParams;

private:
	ApexVertexBuffer    mVertexBuffer;

	// No assignment
	ApexRenderSubmesh&					operator = (const ApexRenderSubmesh&);
};

} // namespace apex
} // namespace physx

#endif // APEX_RENDER_SUBMESH_H
