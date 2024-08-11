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


#ifndef PXS_ARTICULATION_H
#define PXS_ARTICULATION_H

#include "PxvArticulation.h"
#include "PxcArticulationHelper.h"
#include "PsArray.h"
#include "PxcSpatial.h"
#include "CmBitMap.h"

namespace physx
{
 
struct PxsArticulationJointCore;
class PxsDynamicsContext;
class PxsArticulationJoint;
class PxcRigidBody;

struct PxcSolverConstraintDesc;
struct PxsBodyCore;
class PxcConstraintBlockStream;
class PxsContext;

PX_ALIGN_PREFIX(64)
class PxsArticulation
{
public:
	// public interface

							PxsArticulation();
							~PxsArticulation();

	// solver methods
	PxU32					getLinkIndex(PxsArticulationLinkHandle handle)	const	{ return PxU32(handle&PXC_ARTICULATION_IDMASK); }
	PxU32					getBodyCount()									const	{ return mSolverDesc->linkCount;				}
	PxcFsData*				getFsDataPtr()									const	{ return mSolverDesc->fsData;					}
	PxU32					getTotalDataSize()								const	{ return mSolverDesc->solverDataSize;			}
	//PxU32					getTotalDataSize()								const	{ return mSolverDesc->totalDataSize;			}
	void					getSolverDesc(PxcArticulationSolverDesc& d)		const	{ d = *mSolverDesc;	}
	void					setSolverDesc(const PxcArticulationSolverDesc& d)		{ mSolverDesc = &d;	}

	const PxcArticulationSolverDesc* getSolverDescPtr()						const	{ return mSolverDesc;	}
	const PxsArticulationCore*	getCore()									const	{ return mSolverDesc->core;}	
	PxU16					getIterationCounts()							const	{ return mSolverDesc->core->solverIterationCounts; }

private:

	const PxcArticulationSolverDesc*	mSolverDesc;
	
	// debug quantities

	Cm::SpatialVector		computeMomentum(const PxcFsInertia *inertia) const;
	void					computeResiduals(const Cm::SpatialVector *, 
											 const PxcArticulationJointTransforms* jointTransforms,
											 bool dump = false) const;
	void					checkLimits() const;

} PX_ALIGN_SUFFIX(64);

// we encode articulation link handles in the lower bits of the pointer, so the
// articulation has to be aligned, which in an aligned pool means we need to size it
// appropriately

PX_COMPILE_TIME_ASSERT((sizeof(PxsArticulation)&(PXC_ARTICULATION_MAX_SIZE-1))==0);

}

#endif
