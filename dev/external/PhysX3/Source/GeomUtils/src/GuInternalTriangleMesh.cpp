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


#include "PsIntrinsics.h"
#include "GuInternalTriangleMesh.h"
#include "GuIceSupport.h"
#include "GuHillClimbing.h"
#include "GuConvexHull.h"
#include "PsFoundation.h"
#include "CmUtils.h"

using namespace physx;
using namespace Gu;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

InternalTriangleMesh::InternalTriangleMesh() :
	mMaterialIndices		(NULL),
	mFaceRemap				(NULL),	
	mAdjacencies			(NULL),
	mNumAdjacencies			(0),
	mConvexEdgeThreshold	(0.001f),
	mOwnsMemory				(1)
{
	mData.mNumVertices		= 0;
	mData.mNumTriangles		= 0;
	mData.mVertices			= NULL;
	mData.mTriangles		= NULL;
	mData.mExtraTrigData	= NULL;
	mData.mFlags			= 0;
	mData.mAABB.minimum		= PxVec3(0);
	mData.mAABB.maximum		= PxVec3(0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

InternalTriangleMesh::~InternalTriangleMesh()
{
	release();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// PX_SERIALIZATION
void InternalTriangleMesh::exportExtraData(PxSerializationContext& stream)
{
	mData.mOpcodeModel.exportExtraData(stream);

	//PX_DEFINE_DYNAMIC_ARRAY(InternalTriangleMesh, mData.mVertices, PxField::eVEC3, mData.mNumVertices, Ps::PxFieldFlag::eSERIALIZE),
	if(mData.mVertices)
	{
		stream.alignData(PX_SERIAL_ALIGN);
		stream.writeData(mData.mVertices, mData.mNumVertices * sizeof(PxVec3));
	}

	if(mData.mTriangles)
	{
		const PxU32 triangleSize = mData.mFlags & PxTriangleMeshFlag::eHAS_16BIT_TRIANGLE_INDICES ? sizeof(PxU16) : sizeof(PxU32);
		stream.alignData(PX_SERIAL_ALIGN);
		stream.writeData(mData.mTriangles, mData.mNumTriangles * 3 * triangleSize);
	}

	//PX_DEFINE_DYNAMIC_ARRAY(InternalTriangleMesh, mData.mExtraTrigData, PxField::eBYTE, mData.mNumTriangles, Ps::PxFieldFlag::eSERIALIZE),
	if(mData.mExtraTrigData)
	{
		// PT: it might not be needed to 16-byte align this array of PxU8....
		stream.alignData(PX_SERIAL_ALIGN);
		stream.writeData(mData.mExtraTrigData, mData.mNumTriangles * sizeof(PxU8));
	}

	//PX_DEFINE_DYNAMIC_ARRAY(InternalTriangleMesh, mMaterialIndices, PxField::eWORD, mData.mNumTriangles, Ps::PxFieldFlag::eSERIALIZE),
	if(mMaterialIndices)
	{
		stream.alignData(PX_SERIAL_ALIGN);
		stream.writeData(mMaterialIndices, mData.mNumTriangles * sizeof(PxU16));
	}

	//PX_DEFINE_DYNAMIC_ARRAY(InternalTriangleMesh, mFaceRemap, PxField::eDWORD, mData.mNumTriangles, Ps::PxFieldFlag::eSERIALIZE),
	if(mFaceRemap)
	{
		stream.alignData(PX_SERIAL_ALIGN);
		stream.writeData(mFaceRemap, mData.mNumTriangles * sizeof(PxU32));
	}

	if(mAdjacencies)
	{
		stream.alignData(PX_SERIAL_ALIGN);
		stream.writeData(mAdjacencies, mData.mNumTriangles * sizeof(PxU32) * 3);
	}
}

void InternalTriangleMesh::importExtraData(PxDeserializationContext& context)
{
	mData.mOpcodeModel.importExtraData(context);

	if(mData.mVertices)
		mData.mVertices = context.readExtraData<PxVec3, PX_SERIAL_ALIGN>(mData.mNumVertices);

	if(mData.mTriangles)
	{
		if(mData.mFlags & PxTriangleMeshFlag::eHAS_16BIT_TRIANGLE_INDICES)
			mData.mTriangles = context.readExtraData<PxU16, PX_SERIAL_ALIGN>(3*mData.mNumTriangles);
		else
			mData.mTriangles = context.readExtraData<PxU32, PX_SERIAL_ALIGN>(3*mData.mNumTriangles);
	}

	if(mData.mExtraTrigData)
		mData.mExtraTrigData = context.readExtraData<PxU8, PX_SERIAL_ALIGN>(mData.mNumTriangles);

	if(mMaterialIndices)
		mMaterialIndices = context.readExtraData<PxU16, PX_SERIAL_ALIGN>(mData.mNumTriangles);

	if(mFaceRemap)
		mFaceRemap = context.readExtraData<PxU32, PX_SERIAL_ALIGN>(mData.mNumTriangles);

	if(mAdjacencies)
		mAdjacencies = context.readExtraData<PxU32, PX_SERIAL_ALIGN>(mData.mNumTriangles * 3);

	mData.mOpcodeModel.SetMeshInterface(&mMeshInterface);
	setupMeshInterface();
}

//~PX_SERIALIZATION

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void InternalTriangleMesh::release()
{
// PX_SERIALIZATION
	if(mOwnsMemory)
//~PX_SERIALIZATION
	{
		PX_FREE_AND_RESET(mData.mExtraTrigData);

		PX_FREE_AND_RESET(mFaceRemap);
		PX_FREE_AND_RESET(mAdjacencies);
		PX_FREE_AND_RESET(mMaterialIndices);
		PX_FREE_AND_RESET(mData.mTriangles);
		PX_FREE_AND_RESET(mData.mVertices);
	}
	mData.mExtraTrigData = NULL;
	mFaceRemap = NULL;	
	mAdjacencies = NULL;
	mMaterialIndices = NULL;
	mData.mTriangles = NULL;
	mData.mVertices = NULL;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

PxVec3* InternalTriangleMesh::allocateVertices(PxU32 nbVertices)
{
	mData.mNumVertices = nbVertices;
	PX_ASSERT(!mData.mVertices);
	mData.mVertices = (PxVec3*)PX_ALLOC(mData.mNumVertices * sizeof(PxVec3), PX_DEBUG_EXP("PxVec3"));
	return mData.mVertices;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void* InternalTriangleMesh::allocateTriangles(PxU32 nbTriangles, bool force32Bit)
{
	if(mData.mNumVertices == 0)		//please specify vertex count first, so we can size the index list properly
		return NULL;

	mData.mNumTriangles = nbTriangles;
	PX_ASSERT(!mData.mTriangles);

	if(mData.mNumVertices <= 0xffff && !force32Bit)
	{
		mData.mTriangles = PX_ALLOC(mData.mNumTriangles * sizeof(PxU16) * 3, PX_DEBUG_EXP("mData.mTriangles"));
		mData.mFlags |= PxTriangleMeshFlag::eHAS_16BIT_TRIANGLE_INDICES;		
	}
	else
	{
		mData.mTriangles = PX_ALLOC(mData.mNumTriangles * sizeof(PxU32) * 3, PX_DEBUG_EXP("mData.mTriangles"));
		mData.mFlags &= ~(PxU8)PxTriangleMeshFlag::eHAS_16BIT_TRIANGLE_INDICES;		
	}

	return mData.mTriangles;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

PxU16* InternalTriangleMesh::allocateMaterials()
{
	if(!mData.mNumTriangles)	
		return NULL;
	PX_ASSERT(!mMaterialIndices);
	mMaterialIndices = PX_NEW(PxU16)[mData.mNumTriangles];
	return mMaterialIndices;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

PxU32* InternalTriangleMesh::allocateFaceRemap()
{
	if(!mData.mNumTriangles)	return NULL;
	PX_ASSERT(!mFaceRemap);
	mFaceRemap = PX_NEW(PxU32)[mData.mNumTriangles];
	return mFaceRemap;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

PxU32* InternalTriangleMesh::allocateAdjacencies()
{
	if(!mData.mNumTriangles)	return NULL;
	PX_ASSERT(!mAdjacencies);
	mAdjacencies = PX_NEW(PxU32)[mData.mNumTriangles*3];
	mData.mFlags |= PxTriangleMeshFlag::eHAS_ADJACENCY_INFO;
	mNumAdjacencies = mData.mNumTriangles*3;
	return mAdjacencies;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void InternalTriangleMesh::setupMeshInterface()
{
	mMeshInterface.SetNbVertices (getNumVertices());
	mMeshInterface.SetNbTriangles(getNumTriangles());
	mMeshInterface.SetPointers	(getTriangles(), has16BitIndices(), getVertices());
}

bool InternalTriangleMesh::loadRTree(PxInputStream& stream, const PxU32 meshVersion)
{
	//create the meshInterface:
	setupMeshInterface();

	if (meshVersion <= 9)
	{
		Ps::getFoundation().error(
			PxErrorCode::eINTERNAL_ERROR, __FILE__, __LINE__,
			"Obsolete cooked mesh found. Mesh version has been updated, please recook your meshes.");
		PX_ASSERT(0 && "Obsolete cooked mesh found. Mesh version has been updated, please recook your meshes.");
		return false;
	}

	mData.mOpcodeModel.SetMeshInterface(&mMeshInterface);
	if(!mData.mOpcodeModel.mRTree.load(stream, meshVersion))
	{
		Ps::getFoundation().error(PxErrorCode::eINTERNAL_ERROR, __FILE__, __LINE__, "RTree binary image load error.");
		return false;
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


