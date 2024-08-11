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

#ifndef BASIC_IOS_ACTOR_TEST_DATA_H
#define BASIC_IOS_ACTOR_TEST_DATA_H

#include "NxApex.h"
#include "PsArray.h"

namespace physx
{
namespace apex
{
namespace basicios
{

#ifdef APEX_TEST

struct BasicIosActorTestData
{
	bool mIsCPUTest;
	bool mIsGPUTest;

	//Common data
	//NxArray<physx::PxU32>				mOutStateToInput;
	physx::shdfnd::Array<physx::PxU32>	mInStateToInput;
	physx::shdfnd::Array<physx::PxF32>	mBenefit;
	physx::shdfnd::Array<physx::PxF32>	mBenefitOld;
	physx::shdfnd::Array<physx::PxU32>	mHoleScanSum;
	//NxArray<physx::PxU32>				mHistogram;

	physx::PxU32						mParticleCountOld;
	physx::PxU32						mParticleBudget;
	physx::PxU32						mInjectedCount;
	physx::PxU32						mActiveParticleCount;
	physx::PxF32						mBenefitMin;
	physx::PxF32						mBenefitMax;
	physx::PxU32						mBoundHistorgram;
	physx::PxU32						mHistogramBeg;
	physx::PxU32						mHistogramBack;

	unsigned int						mHOLE_SCAN_FLAG;
	unsigned int						mHOLE_SCAN_MASK;
	unsigned int						mNOT_A_PARTICLE;
	unsigned int						mNEW_PARTICLE_FLAG;

	//GPU Data
	physx::shdfnd::Array<physx::PxU32>	mMoveIndices;
	physx::PxU32						mTmpScan;


	BasicIosActorTestData() : mIsCPUTest(false), mIsGPUTest(false) {}
};

#endif

}
}
} // namespace physx::apex

#endif // BASIC_IOS_ACTOR_TEST_DATA_H
