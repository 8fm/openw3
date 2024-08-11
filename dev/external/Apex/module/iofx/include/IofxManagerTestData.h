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

#ifndef IOFX_MANAGER_TEST_DATA_H
#define IOFX_MANAGER_TEST_DATA_H

#include "NxApex.h"
#include "PsArray.h"

namespace physx
{
namespace apex
{
namespace iofx
{

#ifdef APEX_TEST

struct IofxManagerTestData
{
	bool mIsCPUTest;
	bool mIsGPUTest;

	//Common data
	physx::shdfnd::Array<physx::PxU32>				mInStateToInput;
	physx::shdfnd::Array<physx::PxU32>				mOutStateToInput;
	physx::shdfnd::Array<physx::PxU32>				mCountPerActor;
	physx::shdfnd::Array<physx::PxU32>				mStartPerActor;
	physx::shdfnd::Array<physx::PxVec4>				mPositionMass;

	physx::PxU32									mNumParticles;
	physx::PxU32									mMaxInputID;
	physx::PxU32									mMaxStateID;
	physx::PxU32									mCountActorIDs;

	unsigned int									mNOT_A_PARTICLE;
	unsigned int									mNEW_PARTICLE_FLAG;
	unsigned int									mACTOR_ID_START_BIT;
	unsigned int									mSTATE_ID_MASK;

	//GPU specific data
	physx::shdfnd::Array<physx::PxU32>				mSortedActorIDs;
	physx::shdfnd::Array<physx::PxU32>				mSortedStateIDs;
	physx::shdfnd::Array<physx::PxU32>				mActorStart;
	physx::shdfnd::Array<physx::PxU32>				mActorEnd;
	physx::shdfnd::Array<physx::PxU32>				mActorVisibleEnd;
	physx::shdfnd::Array<physx::PxVec4>				mMinBounds;
	physx::shdfnd::Array<physx::PxVec4>				mMaxBounds;

	IofxManagerTestData() : mIsCPUTest(false), mIsGPUTest(false) {}
};

#endif

}
}
} // namespace physx::apex

#endif // IOFX_MANAGER_TEST_DATA_H
