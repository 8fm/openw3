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


#ifndef PXC_NPTHREADCONTEXT_H
#define PXC_NPTHREADCONTEXT_H

#include "PxvConfig.h"

#include "CmScaling.h"

#include "Opcode.h"
#include "CmRenderOutput.h"
#include "PxcCorrelationBuffer.h"
#include "PxcFrictionPatchStreamPair.h"
#include "PxcCCDStateStreamPair.h"
#include "PxcNpCacheStreamPair.h"
#include "PxcConstraintBlockStream.h"
#include "PxcNpCCDState.h"
#include "OPC_LSSCollider.h"
#include "OPC_AABBCollider.h"
#include "OPC_OBBCollider.h"
#include "OPC_SphereCollider.h"
#include "GuGeometryUnion.h"
#include "GuContactBuffer.h"
#include "PxvContext.h"

namespace physx
{

class PxsAABBManager;
class PxsTransformCache;
   
/*!
Per-thread context used by contact generation routines.
*/

class PxcNpThreadContext
{
	PX_NOCOPY(PxcNpThreadContext)
public:
	PxcNpThreadContext(PxReal meshContactMargin, PxReal correlationDist, PxReal toleranceLength,
					   const Cm::RenderOutput& renderOutput,
					   PxcNpMemBlockPool& memBlockPool, bool createContacts);
	~PxcNpThreadContext();

#if PX_ENABLE_SIM_STATS
	void clearStats();
#endif

	// debugging
	Cm::RenderOutput 				mRenderOutput;

	// dsequeira: Need to think about this block pool allocation a bit more. Ideally we'd be 
	// taking blocks from a single pool, except that we want to be able to selectively reclaim
	// blocks if the user needs to defragment, depending on which artifacts they're willing
	// to tolerate, such that the blocks we don't reclaim are contiguous.
#if PX_ENABLE_SIM_STATS
	PxU32 discreteContactPairs	[PxGeometryType::eGEOMETRY_COUNT][PxGeometryType::eGEOMETRY_COUNT];
#endif

	PxsConstraintBlockManager		mConstraintBlockManager;	// fo when this thread context is "lead" on an island
	PxcConstraintBlockStream 		mConstraintBlockStream;		// constraint block pool
	PxcContactBlockStream 			mContactBlockStream;		// constraint block pool
	PxcFrictionPatchStreamPair		mFrictionPatchStreamPair;	// patch streams
	PxcNpCacheStreamPair			mNpCacheStreamPair;			// narrow phase pairwise data cache

	// Everything below here is scratch state. Most of it can even overlap.

	// temporary contact buffer
	Gu::ContactBuffer				mContactBuffer;

	// temporary buffer for correlation
	PxcCorrelationBuffer			mCorrelationBuffer;   

	//temporary buffer for storing CCD impacts,  since writing to bodies directly would be unthreadsafe without locking.

	Ps::Array<PxcCCDImpact>
									mSweepImpacts;      

	// DS: this stuff got moved here from the PxcNpPairContext. As Pierre says:
	////////// PT: those members shouldn't be there in the end, it's not necessary

	PxReal							mDt; // AP: still needed for ccd
	//PxsAABBManager*					mAABBMgr;
	PxU32							mCCDPass;
	PxU32							mCCDFaceIndex;
	PxsFrictionModel				mFrictionType;
	PxsTransformCache*				mTransformCache;
	bool							mPCM;
	bool							mContactCache;
	bool							mCreateContactStream;	// flag to enforce that contacts are stored persistently per workunit. Used for PVD.
	PxU32							mCompressedCacheSize;
	PxU32							mConstraintSize;
	float							mToleranceLength;
	float							mCorrelationDistance;
};   

}

#endif
