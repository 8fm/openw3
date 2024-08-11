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


#include "CookingUtils.h"
#include "PxMath.h"
#include "./Ice/IceFPU.h"
#include "./Ice/IceRevisitedRadix2.h"

using namespace physx;
using namespace Gu;

ReducedVertexCloud::ReducedVertexCloud(const PxVec3* verts, PxU32 nb_verts) : mNbRVerts(0), mRVerts(NULL), mXRef(NULL)
{
	mVerts		= verts;
	mNbVerts	= nb_verts;
}

ReducedVertexCloud::~ReducedVertexCloud()
{
	Clean();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
*	Frees used ram.
*	\return		Self-reference
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ReducedVertexCloud& ReducedVertexCloud::Clean()
{
	PX_DELETE_POD(mXRef);
	PX_FREE_AND_RESET(mRVerts);
	return *this;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
*	Reduction method. Use this to create a minimal vertex cloud.
*	\param		rc		[out] result structure
*	\return		true if success
*	\warning	This is not about welding nearby vertices, here we look for real redundant ones.
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ReducedVertexCloud::Reduce(REDUCEDCLOUD* rc)
{
	//PxU32 Time = TimeGetTime();
	Clean();

	// 1) Get some ram
	mXRef = PX_NEW(PxU32)[mNbVerts];

	// 2) Initialize a three-keys radix
	//	float*	x = new float[mNbVerts];
	//	float*	y = new float[mNbVerts];
	//	float*	z = new float[mNbVerts];
	float*	f = PX_NEW_TEMP(float)[mNbVerts];

	// 3) Fill buffers
	for(PxU32 i=0;i<mNbVerts;i++)
	{
		//		x[i] = mVerts[i].x;
		//		y[i] = mVerts[i].y;
		//		z[i] = mVerts[i].z;
		f[i] = mVerts[i].x;
	}
	// 4) Perform sort
	RadixSortBuffered Radix;
	//	PxU32* Sorted = Radix.Sort(x, mNbVerts).Sort(y, mNbVerts).Sort(z, mNbVerts).GetRanks();
	//	Radix.Sort(x, mNbVerts);
	Radix.Sort((const PxU32*)f, mNbVerts, RADIX_UNSIGNED);

	for(PxU32 i=0;i<mNbVerts;i++)	f[i] = mVerts[i].y;
	Radix.Sort((const PxU32*)f, mNbVerts, RADIX_UNSIGNED);

	for(PxU32 i=0;i<mNbVerts;i++)	f[i] = mVerts[i].z;
	const PxU32* Sorted = Radix.Sort((const PxU32*)f, mNbVerts, RADIX_UNSIGNED).GetRanks();

	PX_DELETE_POD(f);

	// 5) Loop through all vertices
	// - clean vertex-list by removing redundant vertices
	// - create CrossRef list
	//	PxU32 NbVertices = 0;
	mNbRVerts = 0;
	//	PxU32 PreviousX = PX_INVALID_U32;
	//	PxU32 PreviousY = PX_INVALID_U32;
	//	PxU32 PreviousZ = PX_INVALID_U32;
	PxU32 Junk[] = {PX_INVALID_U32,PX_INVALID_U32,PX_INVALID_U32};
	const PxVec3* Previous = (const PxVec3*)Junk;
	//	Container NewVertices;
	mRVerts = (PxVec3*)PX_ALLOC(sizeof(PxVec3) * mNbVerts, PX_DEBUG_EXP("PxVec3"));
	PxU32 Nb = mNbVerts;
	while(Nb--)
		//	for(i=0;i<mNbVerts;i++)
	{
		//		PxU32 Vertex	= Sorted[i];				// Vertex number
		PxU32 Vertex	= *Sorted++;				// Vertex number
		//		PxU32 SortedX	= IR(mVerts[Vertex].x);
		//		PxU32 SortedY	= IR(mVerts[Vertex].y);
		//		PxU32 SortedZ	= IR(mVerts[Vertex].z);

		//		if(SortedX!=PreviousX || SortedY!=PreviousY || SortedZ!=PreviousZ)
		if(IR(mVerts[Vertex].x)!=IR(Previous->x) || IR(mVerts[Vertex].y)!=IR(Previous->y) || IR(mVerts[Vertex].z)!=IR(Previous->z))
		{
			//			NewVertices.Add(SortedX);
			//			NewVertices.Add(SortedY);
			//			NewVertices.Add(SortedZ);
			//			NbVertices++;
			mRVerts[mNbRVerts++] = mVerts[Vertex];
		}
		//		PreviousX = SortedX;
		//		PreviousY = SortedY;
		//		PreviousZ = SortedZ;
		//PreviousX = IR(mVerts[Vertex].x);
		//PreviousY = IR(mVerts[Vertex].y);
		//PreviousZ = IR(mVerts[Vertex].z);
		Previous = &mVerts[Vertex];

		//		mXRef[Vertex] = NbVertices-1;
		mXRef[Vertex] = mNbRVerts-1;
	}
	// Here, NbVertices==#non redundant vertices
	//	mNbRVerts = NbVertices;
	//	mRVerts = new PxVec3[mNbRVerts];
	// 6) Create real vertex-list.
	//	CopyMemory(mRVerts, NewVertices.GetEntries(), NbVertices*sizeof(PxVec3));

	// 7) Fill result structure
	if(rc)
	{
		rc->CrossRef	= mXRef;
		rc->NbRVerts	= mNbRVerts;
		rc->RVerts		= mRVerts;
	}
	/*
	Time = TimeGetTime() - Time;
	Log("Vertex reduction 1: %d => %d => %f seconds\n", mNbVerts, mNbRVerts, float(Time)/1000.0f);

	Time = TimeGetTime();
	if(Reduce2(rc))
	{
	Time = TimeGetTime() - Time;
	Log("Vertex reduction 2: %d => %d => %f seconds\n", mNbVerts, mNbRVerts, float(Time)/1000.0f);
	}
	*/
	return true;
}
