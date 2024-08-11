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

#ifndef OPC_MODELDATA_H
#define OPC_MODELDATA_H

#include "./Ice/IceFPU.h"
#include "OPC_Common.h"
#include "GuRTree.h"

namespace physx
{
namespace Gu
{
	class MeshInterface;

	// bit 1 is always expected to be set to differentiate between leaf and non-leaf node
	PX_FORCE_INLINE PxU32 LeafGetNbTriangles(PxU32 Data) { return ((Data>>1) & 15)+1; }
	PX_FORCE_INLINE PxU32 LeafGetTriangleIndex(PxU32 Data) { return Data>>5; }
	PX_FORCE_INLINE PxU32 LeafSetData(PxU32 nb, PxU32 index)
	{
		PX_ASSERT(nb>0 && nb<=16); PX_ASSERT(index < (1<<27));
		return (index<<5)|(((nb-1)&15)<<1) | 1;
	}

	struct LeafTriangles
	{
		PxU32			Data;

		// Gets number of triangles in the leaf, returns the number of triangles N, with 0 < N <= 16
		PX_FORCE_INLINE	PxU32	GetNbTriangles()				const	{ return LeafGetNbTriangles(Data); }

		// Gets triangle index for this leaf. Indexed model's array of indices retrieved with RTreeMidphase::GetIndices()
		PX_FORCE_INLINE	PxU32	GetTriangleIndex()				const	{ return LeafGetTriangleIndex(Data); }
		PX_FORCE_INLINE	void	SetData(PxU32 nb, PxU32 index)			{ Data = LeafSetData(nb, index); }
	};

	struct RTreeMidphaseData 
	{
		const MeshInterface*	mIMesh;
		const Gu::RTree*		mRTree;
	};

	PX_COMPILE_TIME_ASSERT(sizeof(LeafTriangles)==4); // RTree has space for 4 bytes
} // namespace Gu

}

#endif // OPC_MODELDATA_H
