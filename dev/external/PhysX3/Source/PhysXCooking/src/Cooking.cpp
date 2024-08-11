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


#include "PsFoundation.h"
#include "PsUtilities.h"
#include "PsFastMemory.h"
#include "CmPhysXCommon.h"
#include "PxPhysXConfig.h"
#include "PxSimpleTriangleMesh.h"
#include "PxTriangleMeshDesc.h"
#include "PxConvexMeshDesc.h"
#include "PxCooking.h"
#include "PxErrorCallback.h"
#include "Cooking.h"
#include "GuTriangleMesh.h"
#include "TriangleMeshBuilder.h"
#include "GuConvexMesh.h"
#include "ConvexMeshBuilder.h"
#include "CmIO.h"
#include "PxHeightFieldDesc.h"
#include "GuHeightField.h"
#include "GuHeightFieldCooking.h"

PX_INLINE char	littleEndian()	{ int i = 1; return *((char*)&i);	}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "hulllib.h"

using namespace physx;
using namespace Gu;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Cooking::setParams(const PxCookingParams& params)
{
	mParams = params;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

const PxCookingParams& Cooking::getParams()
{
	return mParams;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool Cooking::platformMismatch()
{
	// Get current endianness (the one for the platform where cooking is performed)
	PxI8 currentEndian = littleEndian();

	bool mismatch;
	switch(mParams.targetPlatform)
	{
	case PxPlatform::ePC:
		mismatch = currentEndian!=1;	// The PC files must be little endian
		break;
	case PxPlatform::eXENON:
		mismatch = currentEndian!=0;	// The Xenon files must be little endian
		break;
	case PxPlatform::ePLAYSTATION3:
		mismatch = currentEndian!=0;
		break;
	case PxPlatform::eARM:
		mismatch = currentEndian!=1;
		break;
	case PxPlatform::eWIIU:
		mismatch = currentEndian!=0;	// Both are supported but big endian is default
		break;
	default:
		PX_ASSERT(0);
		mismatch = false;
		break;
	}
	return mismatch;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Cooking::release()
{
	delete this;

	Ps::Foundation::decRefCount();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool Cooking::cookTriangleMesh(const PxTriangleMeshDesc& desc, PxOutputStream& stream)
{
	// cooking code does lots of float bitwise 
	// reinterpretation that generates exceptions
	PX_FPU_GUARD;

	if(desc.convexEdgeThreshold != 0.001f)
	{
		Ps::getFoundation().error(PxErrorCode::eDEBUG_WARNING, __FILE__, __LINE__, "PxTriangleMeshDesc::convexEdgeThreshold is deprecated therefore convexEdgeThreshold will be ignored.");
	}

	if(!desc.isValid())	return false;

	TriangleMeshBuilder* mesh = PX_NEW(TriangleMeshBuilder);
	if(!mesh)
		return false;

	//	int t0 = timeGetTime();
	if(!mesh->loadFromDesc(desc, mParams))
	{
		PX_DELETE_AND_RESET(mesh);
		return false;
	}
	//	t0 = timeGetTime() - t0;

	mesh->save(stream, platformMismatch());

	PX_DELETE_AND_RESET(mesh);

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool Cooking::cookConvexMesh(const PxConvexMeshDesc& desc_, PxOutputStream& stream)
{
	PX_FPU_GUARD;

	if(!desc_.isValid())
		return false;

	PxConvexMeshDesc desc = desc_;

	HullDesc HD;
	HullResult HR;
	HullLibrary HL;
	if(desc.flags & PxConvexFlag::eCOMPUTE_CONVEX)
	{
		const bool inflateHull = (desc.flags & PxConvexFlag::eCOMPUTE_CONVEX) && (desc.flags & PxConvexFlag::eINFLATE_CONVEX);
		HD.mFlags			= QF_TRIANGLES|QF_SKIN_WIDTH;
		HD.mVcount			= desc.points.count;
		HD.mVertices		= (const float*)desc.points.data;
		HD.mVertexStride	= desc.points.stride;
		HD.mSkinWidth		= inflateHull ? mParams.skinWidth : 0.0f;
		if(desc.vertexLimit < 256 && desc.vertexLimit > 3)
			HD.mMaxVertices		= desc.vertexLimit;

		HullError he = HL.CreateConvexHull(HD, HR);
		if(he==QE_OK)
		{
			desc.points.count		= HR.mNumOutputVertices;
			desc.points.stride		= sizeof(PxVec3);
			desc.points.data		= HR.mOutputVertices;
			desc.triangles.count	= HR.mNumFaces;
			desc.triangles.data		= HR.mIndices;
			desc.triangles.stride	= sizeof(int)*3;
			desc.flags				&= ~PxConvexFlag::eCOMPUTE_CONVEX;
		}
		else return false;
	}

	if(desc.points.count>=256)
	{
		Ps::getFoundation().error(PxErrorCode::eINTERNAL_ERROR, __FILE__, __LINE__, "Cooking::cookConvexMesh: user-provided hull must have less than 256 vertices!");
		HL.ReleaseResult(HR);
		return false;
	}

	////////////

	ConvexMeshBuilder mesh;
	if(!mesh.loadFromDesc(desc, mParams.targetPlatform))
	{
		HL.ReleaseResult(HR);
		return false;
	}

	mesh.save(stream, platformMismatch());
	HL.ReleaseResult(HR);
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool  Cooking::computeHullPolygons(const PxSimpleTriangleMesh& mesh, PxAllocatorCallback& inCallback, PxU32& nbIndices, PxU32*& indices, PxU32& nbPolygons, PxHullPolygon*& hullPolygons)
{
	PxVec3* geometry = (PxVec3*)PxAlloca(sizeof(PxVec3)*mesh.points.count);
	Ps::gatherStrided(mesh.points.data, geometry, mesh.points.count, sizeof(PxVec3), mesh.points.stride);

	PxU32* topology = (PxU32*)PxAlloca(sizeof(PxU32)*3*mesh.triangles.count);
	if (mesh.flags & PxMeshFlag::e16_BIT_INDICES)
	{
		// conversion; 16 bit index -> 32 bit index & stride
		PxU32* dest = topology;
		const PxU32* pastLastDest = topology + 3*mesh.triangles.count;
		const PxU8* source = (const PxU8 *)mesh.triangles.data;
		while (dest < pastLastDest)
		{
			const PxU16 * trig16 = (const PxU16 *)source;
			*dest++ = trig16[0];
			*dest++ = trig16[1];
			*dest++ = trig16[2];
			source += mesh.triangles.stride;
		}
	}
	else
	{
		Ps::gatherStrided(mesh.triangles.data, topology, mesh.triangles.count, sizeof(PxU32) * 3, mesh.triangles.stride);
	}

	ConvexMeshBuilder meshBuilder;
	if(!meshBuilder.computeHullPolygons(mesh.points.count,geometry,mesh.triangles.count,topology,inCallback,nbIndices,indices,nbPolygons,hullPolygons))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool Cooking::cookHeightField(const PxHeightFieldDesc& desc, PxOutputStream& stream)
{
	PX_FPU_GUARD;

	if(!desc.isValid())
		return false;

	Gu::HeightField* hf = PX_NEW(Gu::HeightField)(NULL);
	if(!hf)
		return false;

	if(!hf->loadFromDesc(desc, false))
	{
		hf->releaseMemory(false); // see the comment for the next releaseMemory(false) call
		PX_DELETE_AND_RESET(hf);
		return false;
	}

	if (!saveHeightField(*hf, stream, platformMismatch()))
		return false;

	// AP: this is not ideal but we use aligned free for tiled and regular for non-tiled
	// and by the time we get to destructor we forget the context. TODO: use aligned always?
	hf->releaseMemory(false);
	PX_DELETE_AND_RESET(hf);

	return true;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

PxCooking* PxCreateCooking(PxU32 /*version*/, PxFoundation& foundation, const PxCookingParams& params)
{
	PX_ASSERT(static_cast<Ps::Foundation*>(&foundation) == &Ps::Foundation::getInstance());
	PX_UNUSED(foundation);

	Ps::Foundation::incRefCount();

	return PX_NEW(Cooking)(params);
}

