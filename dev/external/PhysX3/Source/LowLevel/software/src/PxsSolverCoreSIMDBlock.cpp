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


#include "PxPreprocessor.h"
#include "PsVecMath.h"

#ifdef PX_SUPPORT_SIMD

#include "CmPhysXCommon.h"
#include "PxcSolverBody.h"
#include "PxcSolverContact.h"
#include "PxcSolverConstraint1D.h"
#include "PxcSolverConstraintDesc.h"
#include "PxcThresholdStreamElement.h"
#include "PxsSolverContext.h"
#include "PsUtilities.h"
#include "PxvShader.h"
#include "PsAtomic.h"
#include "PxcThresholdStreamElement.h"
#include "PxsSolverCoreGeneral.h"
#include "PxcSolverContact4.h"
#include "PxcSolverConstraint1D4.h"

namespace physx
{

static void solveContact4_Block(const PxcSolverConstraintDesc* PX_RESTRICT desc, PxcSolverContext& cache)
{
	PxcSolverBody& b00 = *desc[0].bodyA;
	PxcSolverBody& b01 = *desc[0].bodyB;
	PxcSolverBody& b10 = *desc[1].bodyA;
	PxcSolverBody& b11 = *desc[1].bodyB;
	PxcSolverBody& b20 = *desc[2].bodyA;
	PxcSolverBody& b21 = *desc[2].bodyB;
	PxcSolverBody& b30 = *desc[3].bodyA;
	PxcSolverBody& b31 = *desc[3].bodyB;

	//We'll need this.
	const Vec4V vZero	= V4Zero();
	const Vec4V vOne	= V4One();
	
	
	Vec4V linVel00 = V4LoadA(&b00.linearVelocity.x);
	Vec4V linVel01 = V4LoadA(&b01.linearVelocity.x);
	Vec4V angVel00 = V4LoadA(&b00.angularVelocity.x);
	Vec4V angVel01 = V4LoadA(&b01.angularVelocity.x);

	Vec4V linVel10 = V4LoadA(&b10.linearVelocity.x);
	Vec4V linVel11 = V4LoadA(&b11.linearVelocity.x);
	Vec4V angVel10 = V4LoadA(&b10.angularVelocity.x);
	Vec4V angVel11 = V4LoadA(&b11.angularVelocity.x);

	Vec4V linVel20 = V4LoadA(&b20.linearVelocity.x);
	Vec4V linVel21 = V4LoadA(&b21.linearVelocity.x);
	Vec4V angVel20 = V4LoadA(&b20.angularVelocity.x);
	Vec4V angVel21 = V4LoadA(&b21.angularVelocity.x);

	Vec4V linVel30 = V4LoadA(&b30.linearVelocity.x);
	Vec4V linVel31 = V4LoadA(&b31.linearVelocity.x);
	Vec4V angVel30 = V4LoadA(&b30.angularVelocity.x);
	Vec4V angVel31 = V4LoadA(&b31.angularVelocity.x);


	Vec4V linVel0T0, linVel0T1, linVel0T2;
	Vec4V linVel1T0, linVel1T1, linVel1T2;
	Vec4V angVel0T0, angVel0T1, angVel0T2;
	Vec4V angVel1T0, angVel1T1, angVel1T2;


	PX_TRANSPOSE_44_34(linVel00, linVel10, linVel20, linVel30, linVel0T0, linVel0T1, linVel0T2);
	PX_TRANSPOSE_44_34(linVel01, linVel11, linVel21, linVel31, linVel1T0, linVel1T1, linVel1T2);
	PX_TRANSPOSE_44_34(angVel00, angVel10, angVel20, angVel30, angVel0T0, angVel0T1, angVel0T2);
	PX_TRANSPOSE_44_34(angVel01, angVel11, angVel21, angVel31, angVel1T0, angVel1T1, angVel1T2);


	const PxU8* PX_RESTRICT last = desc[0].constraint + getConstraintLength(desc[0]);

	//hopefully pointer aliasing doesn't bite.
	const PxU8* PX_RESTRICT currPtr = desc[0].constraint;


	//TODO - can I avoid this many tests???

	const PxU8* PX_RESTRICT prefetchAddress = currPtr + sizeof(PxcSolverContactHeader4) + sizeof(PxcSolverContactBatchPointDynamic4);

	while(currPtr < last)
	{

		PxcSolverContactHeader4* PX_RESTRICT hdr = (PxcSolverContactHeader4*)currPtr;

		currPtr = (PxU8* PX_RESTRICT)(hdr + 1);

		const PxU32 numNormalConstr = hdr->numNormalConstr;
		const PxU32	numFrictionConstr = hdr->numFrictionConstr;

		PxcSolverContactBatchPointDynamic4* PX_RESTRICT contacts = (PxcSolverContactBatchPointDynamic4*)currPtr;
		//const Vec4V dominance1 = V4Neg(__dominance1);

		currPtr = (PxU8*)(contacts + numNormalConstr);
		
		PxcSolverContactFrictionDynamic4* PX_RESTRICT frictions = (PxcSolverContactFrictionDynamic4*)currPtr;
		currPtr += numFrictionConstr * sizeof(PxcSolverContactFrictionDynamic4);
		
		Vec4V accumulatedNormalImpulse = vZero;

		const Vec4V invMass0D0 = hdr->invMassADom0;
		const Vec4V invMass1D1 = hdr->invMassBDom1;

		const Vec4V normalT0 = hdr->normalX;
		const Vec4V normalT1 = hdr->normalY;
		const Vec4V normalT2 = hdr->normalZ;

		const Vec4V __normalVel1 = V4Mul(linVel0T0, normalT0);
		const Vec4V __normalVel3 = V4Mul(linVel1T0, normalT0);
		const Vec4V _normalVel1 = V4MulAdd(linVel0T1, normalT1, __normalVel1);
		const Vec4V _normalVel3 = V4MulAdd(linVel1T1, normalT1, __normalVel3);

		Vec4V normalVel1 = V4MulAdd(linVel0T2, normalT2, _normalVel1);
		Vec4V normalVel3 = V4MulAdd(linVel1T2, normalT2, _normalVel3);

		Vec4V accumDeltaF = vZero;

		for(PxU32 i=0;i<numNormalConstr;i++)
		{
			PxcSolverContactBatchPointDynamic4& c = contacts[i];
			Ps::prefetchLine(prefetchAddress);
			Ps::prefetchLine(prefetchAddress, 128);
			Ps::prefetchLine(prefetchAddress, 256);
			prefetchAddress += 384;
			//Ps::prefetchLine((&contacts[i+2]), 384);

			const Vec4V delAngVel0T0 = c.delAngVel0X;
			const Vec4V delAngVel0T1 = c.delAngVel0Y;
			const Vec4V delAngVel0T2 = c.delAngVel0Z;
			const Vec4V delAngVel1T0 = c.delAngVel1X;
			const Vec4V delAngVel1T1 = c.delAngVel1Y;
			const Vec4V delAngVel1T2 = c.delAngVel1Z;

			const Vec4V appliedForce = c.appliedForce;
			const Vec4V velMultiplier = c.velMultiplier;
			const Vec4V maxImpulse = c.maxImpulse;
			
			//const Vec4V targetVel = c.targetVelocity;
			//const Vec4V scaledBias = c.scaledBias;

			const Vec4V raXnT0 = c.raXnX;
			const Vec4V raXnT1 = c.raXnY;
			const Vec4V raXnT2 = c.raXnZ;
			const Vec4V rbXnT0 = c.rbXnX;
			const Vec4V rbXnT1 = c.rbXnY;
			const Vec4V rbXnT2 = c.rbXnZ;

			
			const Vec4V __normalVel2 = V4Mul(raXnT0, angVel0T0);
			const Vec4V __normalVel4 = V4Mul(rbXnT0, angVel1T0);

			
			const Vec4V _normalVel2 = V4MulAdd(raXnT1, angVel0T1, __normalVel2);
			const Vec4V _normalVel4 = V4MulAdd(rbXnT1, angVel1T1, __normalVel4);

			
			const Vec4V normalVel2 = V4MulAdd(raXnT2, angVel0T2, _normalVel2);
			const Vec4V normalVel4 = V4MulAdd(rbXnT2, angVel1T2, _normalVel4);

			const Vec4V biasedErr = c.biasedErr;

			//Linear component - normal * invMass_dom

			const Vec4V _normalVel(V4Add(normalVel1, normalVel2));
			const Vec4V __normalVel(V4Add(normalVel3, normalVel4));
		
			const Vec4V normalVel = V4Sub(_normalVel, __normalVel );

			const Vec4V _deltaF = V4NegMulSub(normalVel, velMultiplier, biasedErr);
			const Vec4V nAppliedForce = V4Neg(appliedForce);

			const Vec4V _deltaF2 = V4Max(_deltaF, nAppliedForce);
			const Vec4V _newAppliedForce(V4Add(appliedForce, _deltaF2));
			const Vec4V newAppliedForce = V4Min(_newAppliedForce, maxImpulse);
			const Vec4V deltaF = V4Sub(newAppliedForce, appliedForce);

			accumDeltaF = V4Add(accumDeltaF, deltaF);

			normalVel1 = V4MulAdd(invMass0D0, deltaF, normalVel1);
			normalVel3 = V4MulAdd(invMass1D1, deltaF, normalVel3);
			
			angVel0T0 = V4MulAdd(delAngVel0T0, deltaF, angVel0T0);
			angVel1T0 = V4MulAdd(delAngVel1T0, deltaF, angVel1T0);
			
			angVel0T1 = V4MulAdd(delAngVel0T1, deltaF, angVel0T1);
			angVel1T1 = V4MulAdd(delAngVel1T1, deltaF, angVel1T1);

			angVel0T2 = V4MulAdd(delAngVel0T2, deltaF, angVel0T2);
			angVel1T2 = V4MulAdd(delAngVel1T2, deltaF, angVel1T2);

			c.appliedForce = newAppliedForce;

			accumulatedNormalImpulse = V4Add(accumulatedNormalImpulse, newAppliedForce);
		}

		const Vec4V accumDeltaF_IM0 = V4Mul(accumDeltaF, invMass0D0);
		const Vec4V accumDeltaF_IM1 = V4Mul(accumDeltaF, invMass1D1);

		linVel0T0 = V4MulAdd(normalT0, accumDeltaF_IM0, linVel0T0);
		linVel1T0 = V4MulAdd(normalT0, accumDeltaF_IM1, linVel1T0);
		linVel0T1 = V4MulAdd(normalT1, accumDeltaF_IM0, linVel0T1);
		linVel1T1 = V4MulAdd(normalT1, accumDeltaF_IM1, linVel1T1);
		linVel0T2 = V4MulAdd(normalT2, accumDeltaF_IM0, linVel0T2);
		linVel1T2 = V4MulAdd(normalT2, accumDeltaF_IM1, linVel1T2);


		if(cache.doFriction && numFrictionConstr)
		{
			const Vec4V staticFric = hdr->staticFriction;
			const Vec4V dynamicFric = hdr->dynamicFriction;

			const Vec4V maxFrictionImpulse = V4Mul(staticFric, accumulatedNormalImpulse);
			const Vec4V maxDynFrictionImpulse = V4Mul(dynamicFric, accumulatedNormalImpulse);
			const Vec4V negMaxDynFrictionImpulse = V4Neg(maxDynFrictionImpulse);
			const Vec4V negMaxFrictionImpulse = V4Neg(maxFrictionImpulse);

			for(PxU32 i=0;i<numFrictionConstr;i++)
			{
				PxcSolverContactFrictionDynamic4& f = frictions[i];
				Ps::prefetchLine(prefetchAddress);
				Ps::prefetchLine(prefetchAddress);
				Ps::prefetchLine(prefetchAddress);
				prefetchAddress += 384;

				if(cache.writeBackIteration)
				{
					Ps::prefetchLine(f.frictionBrokenWritebackByte[0]);
					Ps::prefetchLine(f.frictionBrokenWritebackByte[1]);
					Ps::prefetchLine(f.frictionBrokenWritebackByte[2]);
					Ps::prefetchLine(f.frictionBrokenWritebackByte[3]);
				}

				const Vec4V delAngVel0T0 = f.delAngVel0X;
				const Vec4V delAngVel0T1 = f.delAngVel0Y;
				const Vec4V delAngVel0T2 = f.delAngVel0Z;

				const Vec4V delAngVel1T0 = f.delAngVel1X;
				const Vec4V delAngVel1T1 = f.delAngVel1Y;
				const Vec4V delAngVel1T2 = f.delAngVel1Z;


				const Vec4V appliedForce = f.appliedForce;
				//const Vec4V bias = f.bias;
				const Vec4V velMultiplier = f.velMultiplier;
				//const Vec4V targetVel = f.targetVelocity;
				
				const Vec4V normalT0 = f.normalX;
				const Vec4V normalT1 = f.normalY;
				const Vec4V normalT2 = f.normalZ;
				const Vec4V raXnT0 = f.raXnX;
				const Vec4V raXnT1 = f.raXnY;
				const Vec4V raXnT2 = f.raXnZ;
				const Vec4V rbXnT0 = f.rbXnX;
				const Vec4V rbXnT1 = f.rbXnY;
				const Vec4V rbXnT2 = f.rbXnZ;

				const Vec4V oldBroken = f.broken;

				const Vec4V __normalVel1 = V4Mul(linVel0T0, normalT0);
				const Vec4V __normalVel2 = V4Mul(raXnT0, angVel0T0);
				const Vec4V __normalVel3 = V4Mul(linVel1T0, normalT0);
				const Vec4V __normalVel4 = V4Mul(rbXnT0, angVel1T0);

				const Vec4V _normalVel1 = V4MulAdd(linVel0T1, normalT1, __normalVel1);
				const Vec4V _normalVel2 = V4MulAdd(raXnT1, angVel0T1, __normalVel2);
				const Vec4V _normalVel3 = V4MulAdd(linVel1T1, normalT1, __normalVel3);
				const Vec4V _normalVel4 = V4MulAdd(rbXnT1, angVel1T1, __normalVel4);

				const Vec4V normalVel1 = V4MulAdd(linVel0T2, normalT2, _normalVel1);
				const Vec4V normalVel2 = V4MulAdd(raXnT2, angVel0T2, _normalVel2);
				const Vec4V normalVel3 = V4MulAdd(linVel1T2, normalT2, _normalVel3);
				const Vec4V normalVel4 = V4MulAdd(rbXnT2, angVel1T2, _normalVel4);

				const Vec4V _normalVel = V4Add(normalVel1, normalVel2);
				const Vec4V __normalVel = V4Add(normalVel3, normalVel4);

				/*const Vec4V delLinVel20 = V4Mul(normalT2, invMass0D0);
				const Vec4V delLinVel21 = V4Mul(normalT2, invMass1D1);*/

				//relative normal velocity for all 4 constraints		

				//const Vec4V _tmp1 = V4Sub(bias, targetVel);

				// appliedForce -bias * velMultiplier - a hoisted part of the total impulse computation
			

				const Vec4V normalVel = V4Sub(_normalVel, __normalVel );

				const Vec4V tmp1 = V4Sub(appliedForce, f.scaledBias); 

				const Vec4V totalImpulse = V4NegMulSub(normalVel, velMultiplier, tmp1);

				const BoolV clampLow = V4IsGrtr(negMaxFrictionImpulse, totalImpulse);
				const BoolV clampHigh = V4IsGrtr(totalImpulse, maxFrictionImpulse);

				const Vec4V totalClampedLow = V4Max(negMaxDynFrictionImpulse, totalImpulse);
				const Vec4V totalClampedHigh = V4Min(maxDynFrictionImpulse, totalImpulse);

				const Vec4V newAppliedForce1 = V4Sel(clampHigh, totalClampedHigh, totalImpulse);
				const Vec4V broken1 = V4Sel(clampHigh, vOne, oldBroken);

				const Vec4V newAppliedForce = V4Sel(clampLow, totalClampedLow,
															  newAppliedForce1);

				const Vec4V broken = V4Sel(clampLow, vOne, broken1 );
				

				const Vec4V deltaF =V4Sub(newAppliedForce, appliedForce);

				f.appliedForce = newAppliedForce;
				f.broken = broken;

				const Vec4V deltaFIM0 = V4Mul(deltaF, invMass0D0);
				const Vec4V deltaFIM1 = V4Mul(deltaF, invMass1D1);

				linVel0T0 = V4MulAdd(normalT0, deltaFIM0, linVel0T0);
				linVel1T0 = V4MulAdd(normalT0, deltaFIM1, linVel1T0);
				angVel0T0 = V4MulAdd(delAngVel0T0, deltaF, angVel0T0);
				angVel1T0 = V4MulAdd(delAngVel1T0, deltaF, angVel1T0);

				linVel0T1 = V4MulAdd(normalT1, deltaFIM0, linVel0T1);
				linVel1T1 = V4MulAdd(normalT1, deltaFIM1, linVel1T1);
				angVel0T1 = V4MulAdd(delAngVel0T1, deltaF, angVel0T1);
				angVel1T1 = V4MulAdd(delAngVel1T1, deltaF, angVel1T1);

				linVel0T2 = V4MulAdd(normalT2, deltaFIM0, linVel0T2);
				linVel1T2 = V4MulAdd(normalT2, deltaFIM1, linVel1T2);
				angVel0T2 = V4MulAdd(delAngVel0T2, deltaF, angVel0T2);
				angVel1T2 = V4MulAdd(delAngVel1T2, deltaF, angVel1T2);
			}
		}
	}

	PX_TRANSPOSE_34_44(linVel0T0, linVel0T1, linVel0T2, linVel00, linVel10, linVel20, linVel30);
	PX_TRANSPOSE_34_44(linVel1T0, linVel1T1, linVel1T2, linVel01, linVel11, linVel21, linVel31);
	PX_TRANSPOSE_34_44(angVel0T0, angVel0T1, angVel0T2, angVel00, angVel10, angVel20, angVel30);
	PX_TRANSPOSE_34_44(angVel1T0, angVel1T1, angVel1T2, angVel01, angVel11, angVel21, angVel31);

	// Write back
	V3StoreU(Vec3V_From_Vec4V_WUndefined(linVel00), b00.linearVelocity);
	V3StoreU(Vec3V_From_Vec4V_WUndefined(linVel10), b10.linearVelocity);
	V3StoreU(Vec3V_From_Vec4V_WUndefined(linVel20), b20.linearVelocity);
	V3StoreU(Vec3V_From_Vec4V_WUndefined(linVel30), b30.linearVelocity);

	V3StoreU(Vec3V_From_Vec4V_WUndefined(linVel01), b01.linearVelocity);
	V3StoreU(Vec3V_From_Vec4V_WUndefined(linVel11), b11.linearVelocity);
	V3StoreU(Vec3V_From_Vec4V_WUndefined(linVel21), b21.linearVelocity);
	V3StoreU(Vec3V_From_Vec4V_WUndefined(linVel31), b31.linearVelocity);

	V3StoreU(Vec3V_From_Vec4V_WUndefined(angVel00), b00.angularVelocity);
	V3StoreU(Vec3V_From_Vec4V_WUndefined(angVel10), b10.angularVelocity);
	V3StoreU(Vec3V_From_Vec4V_WUndefined(angVel20), b20.angularVelocity);
	V3StoreU(Vec3V_From_Vec4V_WUndefined(angVel30), b30.angularVelocity);

	V3StoreU(Vec3V_From_Vec4V_WUndefined(angVel01), b01.angularVelocity);
	V3StoreU(Vec3V_From_Vec4V_WUndefined(angVel11), b11.angularVelocity);
	V3StoreU(Vec3V_From_Vec4V_WUndefined(angVel21), b21.angularVelocity);
	V3StoreU(Vec3V_From_Vec4V_WUndefined(angVel31), b31.angularVelocity);
}

static void solveContact4_StaticBlock(const PxcSolverConstraintDesc* PX_RESTRICT desc, PxcSolverContext& cache)
{
	PxcSolverBody& b00 = *desc[0].bodyA;
	PxcSolverBody& b10 = *desc[1].bodyA;
	PxcSolverBody& b20 = *desc[2].bodyA;
	PxcSolverBody& b30 = *desc[3].bodyA;

	const PxU8* PX_RESTRICT last = desc[0].constraint + getConstraintLength(desc[0]);

	//hopefully pointer aliasing doesn't bite.
	const PxU8* PX_RESTRICT currPtr = desc[0].constraint;


	//We'll need this.
	const Vec4V vZero	= V4Zero();
	const Vec4V vOne	= V4One();
	
	Vec4V linVel00 = V4LoadA(&b00.linearVelocity.x);
	Vec4V angVel00 = V4LoadA(&b00.angularVelocity.x);

	Vec4V linVel10 = V4LoadA(&b10.linearVelocity.x);
	Vec4V angVel10 = V4LoadA(&b10.angularVelocity.x);

	Vec4V linVel20 = V4LoadA(&b20.linearVelocity.x);
	Vec4V angVel20 = V4LoadA(&b20.angularVelocity.x);

	Vec4V linVel30 = V4LoadA(&b30.linearVelocity.x);
	Vec4V angVel30 = V4LoadA(&b30.angularVelocity.x);

	Vec4V linVel0T0, linVel0T1, linVel0T2;
	Vec4V angVel0T0, angVel0T1, angVel0T2;


	PX_TRANSPOSE_44_34(linVel00, linVel10, linVel20, linVel30, linVel0T0, linVel0T1, linVel0T2);
	PX_TRANSPOSE_44_34(angVel00, angVel10, angVel20, angVel30, angVel0T0, angVel0T1, angVel0T2);

	const PxU8* PX_RESTRICT prefetchAddress = currPtr + sizeof(PxcSolverContactHeader4) + sizeof(PxcSolverContactBatchPointBase4);
	while((currPtr < last))
	{
		PxcSolverContactHeader4* PX_RESTRICT hdr = (PxcSolverContactHeader4*)currPtr;
		
		currPtr = (PxU8* PX_RESTRICT)(hdr + 1);		

		const PxU32 numNormalConstr = hdr->numNormalConstr;
		const PxU32	numFrictionConstr = hdr->numFrictionConstr;

		PxcSolverContactBatchPointBase4* PX_RESTRICT contacts = (PxcSolverContactBatchPointBase4*)currPtr;

		currPtr = (PxU8*)(contacts + numNormalConstr);

		PxcSolverContactFrictionBase4* PX_RESTRICT frictions = (PxcSolverContactFrictionBase4*)currPtr;
		currPtr += numFrictionConstr * sizeof(PxcSolverContactFrictionBase4);
		
		Vec4V accumulatedNormalImpulse = vZero;

		const Vec4V invMass0 = hdr->invMassADom0;

		const Vec4V normalT0 = hdr->normalX;
		const Vec4V normalT1 = hdr->normalY;
		const Vec4V normalT2 = hdr->normalZ;

		const Vec4V __normalVel1 = V4Mul(linVel0T0, normalT0);
		const Vec4V _normalVel1 = V4MulAdd(linVel0T1, normalT1, __normalVel1);

		/*const Vec4V delLinVel00 = V4Mul(normalT0, invMass0);

		const Vec4V delLinVel10 = V4Mul(normalT1, invMass0);

		const Vec4V delLinVel20 = V4Mul(normalT2, invMass0);*/

		Vec4V normalVel1 = V4MulAdd(linVel0T2, normalT2, _normalVel1);

		Vec4V accumDeltaF = vZero;


		for(PxU32 i=0;i<numNormalConstr;i++)
		{
			PxcSolverContactBatchPointBase4& c = contacts[i];
			Ps::prefetchLine(prefetchAddress);
			Ps::prefetchLine(prefetchAddress, 128);
			prefetchAddress += 256;
			//Ps::prefetchLine((&contacts[i+1]), 256);
			//Static contact batch point is 176 bytes. 2 cache line fetches should get it all but we fetch 3 in case of alignment spills
			
			//Ps::prefetchLine((&contacts[i+1]), 384);
			
			const Vec4V delAngVel0T0 = c.delAngVel0X;
			const Vec4V delAngVel0T1 = c.delAngVel0Y;
			const Vec4V delAngVel0T2 = c.delAngVel0Z;

			const Vec4V appliedForce = c.appliedForce;
			const Vec4V velMultiplier = c.velMultiplier;
			const Vec4V maxImpulse = c.maxImpulse;

			//const Vec4V targetVel = c.targetVelocity;
			//const Vec4V scaledBias = c.scaledBias;
			
			const Vec4V raXnT0 = c.raXnX;
			const Vec4V raXnT1 = c.raXnY;
			const Vec4V raXnT2 = c.raXnZ;

			const Vec4V __normalVel2 = V4Mul(raXnT0, angVel0T0);

			const Vec4V _normalVel2 = V4MulAdd(raXnT1, angVel0T1, __normalVel2);

			const Vec4V normalVel2 = V4MulAdd(raXnT2, angVel0T2, _normalVel2);


			//NormalVel stores relative normal velocity for all 4 constraints

			const Vec4V biasedErr = c.biasedErr;

			//Linear component - normal * invMass_dom

			const Vec4V normalVel = V4Add(normalVel1, normalVel2);

			const Vec4V _deltaF = V4Max(V4NegMulSub(normalVel, velMultiplier, biasedErr), V4Neg(appliedForce));

			const Vec4V _newAppliedForce(V4Add(appliedForce, _deltaF));
			const Vec4V newAppliedForce = V4Min(_newAppliedForce, maxImpulse);
			const Vec4V deltaF = V4Sub(newAppliedForce, appliedForce);

			accumDeltaF = V4Add(accumDeltaF, deltaF);

			normalVel1 = V4MulAdd(invMass0, deltaF, normalVel1);
			
			angVel0T0 = V4MulAdd(delAngVel0T0, deltaF, angVel0T0);

			angVel0T1 = V4MulAdd(delAngVel0T1, deltaF, angVel0T1);

			angVel0T2 = V4MulAdd(delAngVel0T2, deltaF, angVel0T2);
			
			c.appliedForce = newAppliedForce;
			
			accumulatedNormalImpulse = V4Add(accumulatedNormalImpulse, newAppliedForce);
		}	

		const Vec4V deltaFInvMass = V4Mul(accumDeltaF, invMass0);

		linVel0T0 = V4MulAdd(normalT0, deltaFInvMass, linVel0T0);
		linVel0T1 = V4MulAdd(normalT1, deltaFInvMass, linVel0T1);
		linVel0T2 = V4MulAdd(normalT2, deltaFInvMass, linVel0T2);

		if(cache.doFriction && numFrictionConstr)
		{
			const Vec4V staticFric = hdr->staticFriction;

			const Vec4V dynamicFric = hdr->dynamicFriction;

			const Vec4V maxFrictionImpulse = V4Mul(staticFric, accumulatedNormalImpulse);
			const Vec4V maxDynFrictionImpulse = V4Mul(dynamicFric, accumulatedNormalImpulse);
			const Vec4V negMaxDynFrictionImpulse = V4Neg(maxDynFrictionImpulse);
			const Vec4V negMaxFrictionImpulse = V4Neg(maxFrictionImpulse);

			for(PxU32 i=0;i<numFrictionConstr;i++)
			{
				PxcSolverContactFrictionBase4& f = frictions[i];
				Ps::prefetchLine(prefetchAddress);
				Ps::prefetchLine(prefetchAddress, 128);
				Ps::prefetchLine(prefetchAddress, 256);
				prefetchAddress += 384;
				

				if(cache.writeBackIteration)
				{
					Ps::prefetchLine(f.frictionBrokenWritebackByte[0]);
					Ps::prefetchLine(f.frictionBrokenWritebackByte[1]);
					Ps::prefetchLine(f.frictionBrokenWritebackByte[2]);
					Ps::prefetchLine(f.frictionBrokenWritebackByte[3]);
				}

				const Vec4V delAngVel0T0 = f.delAngVel0X;
				const Vec4V delAngVel0T1 = f.delAngVel0Y;
				const Vec4V delAngVel0T2 = f.delAngVel0Z;

				const Vec4V appliedForce = f.appliedForce;
				//const Vec4V bias = f.bias;
				const Vec4V velMultiplier = f.velMultiplier;

				//const Vec4V targetVel = f.targetVelocity;

				const Vec4V normalT0 = f.normalX;
				const Vec4V normalT1 = f.normalY;
				const Vec4V normalT2 = f.normalZ;
				const Vec4V raXnT0 = f.raXnX;
				const Vec4V raXnT1 = f.raXnY;
				const Vec4V raXnT2 = f.raXnZ;
				const Vec4V oldBroken = f.broken;

				const Vec4V __normalVel1 = V4Mul(linVel0T0, normalT0);
				const Vec4V __normalVel2 = V4Mul(raXnT0, angVel0T0);

				const Vec4V _normalVel1 = V4MulAdd(linVel0T1, normalT1, __normalVel1);
				const Vec4V _normalVel2 = V4MulAdd(raXnT1, angVel0T1, __normalVel2);

				const Vec4V normalVel1 = V4MulAdd(linVel0T2, normalT2, _normalVel1);
				const Vec4V normalVel2 = V4MulAdd(raXnT2, angVel0T2, _normalVel2);

				//relative normal velocity for all 4 constraints
				const Vec4V normalVel = V4Add(normalVel1, normalVel2);

				// appliedForce -bias * velMultiplier - a hoisted part of the total impulse computation
				const Vec4V tmp1 = V4Sub(appliedForce, f.scaledBias); 
				/*const Vec4V delLinVel00 = V4Mul(normalT0, invMass0);
				const Vec4V delLinVel10 = V4Mul(normalT1, invMass0);
				const Vec4V delLinVel20 = V4Mul(normalT2, invMass0);	*/

				const Vec4V totalImpulse = V4NegMulSub(normalVel, velMultiplier, tmp1);

				const BoolV clampLow = V4IsGrtr(negMaxFrictionImpulse, totalImpulse);
				const BoolV clampHigh = V4IsGrtr(totalImpulse, maxFrictionImpulse);

				const Vec4V totalClampedLow = V4Max(negMaxDynFrictionImpulse, totalImpulse);
				const Vec4V totalClampedHigh = V4Min(maxDynFrictionImpulse, totalImpulse);

				const Vec4V newAppliedForce = V4Sel(clampLow, totalClampedLow,
															  V4Sel(clampHigh, totalClampedHigh, totalImpulse));
				
				const Vec4V broken = V4Sel(clampLow, vOne, V4Sel(clampHigh, vOne, oldBroken));

				const Vec4V deltaF = V4Sub(newAppliedForce, appliedForce);

				const Vec4V deltaFInvMass = V4Mul(invMass0, deltaF);


				linVel0T0 = V4MulAdd(normalT0, deltaFInvMass, linVel0T0);
				angVel0T0 = V4MulAdd(delAngVel0T0, deltaF, angVel0T0);

				linVel0T1 = V4MulAdd(normalT1, deltaFInvMass, linVel0T1);
				angVel0T1 = V4MulAdd(delAngVel0T1, deltaF, angVel0T1);

				linVel0T2 = V4MulAdd(normalT2, deltaFInvMass, linVel0T2);
				angVel0T2 = V4MulAdd(delAngVel0T2, deltaF, angVel0T2);

				f.appliedForce = newAppliedForce;

				f.broken = broken;
			}
		}
	}

	PX_TRANSPOSE_34_44(linVel0T0, linVel0T1, linVel0T2, linVel00, linVel10, linVel20, linVel30);
	PX_TRANSPOSE_34_44(angVel0T0, angVel0T1, angVel0T2, angVel00, angVel10, angVel20, angVel30);

	// Write back
	V3StoreU(Vec3V_From_Vec4V_WUndefined(linVel00), b00.linearVelocity);
	V3StoreU(Vec3V_From_Vec4V_WUndefined(linVel10), b10.linearVelocity);
	V3StoreU(Vec3V_From_Vec4V_WUndefined(linVel20), b20.linearVelocity);
	V3StoreU(Vec3V_From_Vec4V_WUndefined(linVel30), b30.linearVelocity);

	V3StoreU(Vec3V_From_Vec4V_WUndefined(angVel00), b00.angularVelocity);
	V3StoreU(Vec3V_From_Vec4V_WUndefined(angVel10), b10.angularVelocity);
	V3StoreU(Vec3V_From_Vec4V_WUndefined(angVel20), b20.angularVelocity);
	V3StoreU(Vec3V_From_Vec4V_WUndefined(angVel30), b30.angularVelocity);
}

static void concludeContact4_Block(const PxcSolverConstraintDesc* PX_RESTRICT desc, PxcSolverContext& cache, PxU32 contactSize, PxU32 frictionSize)
{
	const PxU8* PX_RESTRICT last = desc[0].constraint + getConstraintLength(desc[0]);

	//hopefully pointer aliasing doesn't bite.
	const PxU8* PX_RESTRICT currPtr = desc[0].constraint;

	while((currPtr < last))
	{
		PxcSolverContactHeader4* PX_RESTRICT hdr = (PxcSolverContactHeader4*)currPtr;
		
		currPtr = (PxU8* PX_RESTRICT)(hdr + 1);		

		const PxU32 numNormalConstr = hdr->numNormalConstr;
		const PxU32	numFrictionConstr = hdr->numFrictionConstr;

		PxcSolverContactBatchPointBase4* PX_RESTRICT contacts = (PxcSolverContactBatchPointBase4*)currPtr;
		currPtr += (numNormalConstr * contactSize);

		PxcSolverContactFrictionBase4* PX_RESTRICT frictions = (PxcSolverContactFrictionBase4*)currPtr;
		currPtr += (numFrictionConstr * frictionSize);
		

		for(PxU32 i=0;i<numNormalConstr;i++)
		{
			PxcSolverContactBatchPointBase4& c = *contacts;
			contacts = (PxcSolverContactBatchPointBase4*)(((PxU8*)contacts) + contactSize);
			//c.biasedErr = V4Min(c.biasedErr, vZero);
			c.biasedErr = V4Sub(c.biasedErr, c.scaledBias);
		}	

		if(cache.doFriction && numFrictionConstr)
		{
			for(PxU32 i=0;i<numFrictionConstr;i++)
			{
				PxcSolverContactFrictionBase4& f = *frictions;
				frictions = (PxcSolverContactFrictionBase4*)(((PxU8*)frictions) + frictionSize);
				f.scaledBias = V4Sub(f.scaledBias, f.bias);
				//f.bias = vZero;
			}
		}
	}
}

void writeBackContact4_Block(const PxcSolverConstraintDesc* PX_RESTRICT desc, PxcSolverContext& cache,
							 const PxcSolverBodyData** PX_RESTRICT bd0, const PxcSolverBodyData** PX_RESTRICT bd1)
{
	const PxU8* PX_RESTRICT last = desc[0].constraint + getConstraintLength(desc[0]);

	//hopefully pointer aliasing doesn't bite.
	const PxU8* PX_RESTRICT currPtr = desc[0].constraint;
	PxReal* PX_RESTRICT vForceWriteback0 = reinterpret_cast<PxReal* PX_RESTRICT>(desc[0].writeBack);
	PxReal* PX_RESTRICT vForceWriteback1 = reinterpret_cast<PxReal* PX_RESTRICT>(desc[1].writeBack);
	PxReal* PX_RESTRICT vForceWriteback2 = reinterpret_cast<PxReal* PX_RESTRICT>(desc[2].writeBack);
	PxReal* PX_RESTRICT vForceWriteback3 = reinterpret_cast<PxReal* PX_RESTRICT>(desc[3].writeBack);

	const PxU8 type = *desc[0].constraint;
	const PxU32 contactSize = type == PXS_SC_TYPE_BLOCK_RB_CONTACT ? sizeof(PxcSolverContactBatchPointDynamic4) : sizeof(PxcSolverContactBatchPointBase4);
	const PxU32 frictionSize = type == PXS_SC_TYPE_BLOCK_RB_CONTACT ? sizeof(PxcSolverContactFrictionDynamic4) : sizeof(PxcSolverContactFrictionBase4);


	Vec4V normalForce = V4Zero();


	//We'll need this.
	//const Vec4V vZero	= V4Zero();

	bool writeBackThresholds[4] = {false, false, false, false};

	while((currPtr < last))
	{
		PxcSolverContactHeader4* PX_RESTRICT hdr = (PxcSolverContactHeader4*)currPtr;
		
		currPtr = (PxU8* PX_RESTRICT)(hdr + 1);		

		const PxU32 numNormalConstr = hdr->numNormalConstr;
		const PxU32	numFrictionConstr = hdr->numFrictionConstr;

		PxcSolverContactBatchPointBase4* PX_RESTRICT contacts = (PxcSolverContactBatchPointBase4*)currPtr;
		currPtr += (numNormalConstr * contactSize);

		PxcSolverContactFrictionBase4* PX_RESTRICT frictions = (PxcSolverContactFrictionBase4*)currPtr;
		currPtr += (numFrictionConstr * frictionSize);

		writeBackThresholds[0] = hdr->flags[0] & PxcSolverContactHeader::eHAS_FORCE_THRESHOLDS;
		writeBackThresholds[1] = hdr->flags[1] & PxcSolverContactHeader::eHAS_FORCE_THRESHOLDS;
		writeBackThresholds[2] = hdr->flags[2] & PxcSolverContactHeader::eHAS_FORCE_THRESHOLDS;
		writeBackThresholds[3] = hdr->flags[3] & PxcSolverContactHeader::eHAS_FORCE_THRESHOLDS;
		


		for(PxU32 i=0;i<numNormalConstr;i++)
		{
			PxcSolverContactBatchPointBase4& c = *contacts;
			contacts = (PxcSolverContactBatchPointBase4*)(((PxU8*)contacts) + contactSize);
			const FloatV appliedForce0 = V4GetX(c.appliedForce);
			const FloatV appliedForce1 = V4GetX(c.appliedForce);
			const FloatV appliedForce2 = V4GetX(c.appliedForce);
			const FloatV appliedForce3 = V4GetX(c.appliedForce);

			normalForce = V4Add(normalForce, c.appliedForce);

			if(vForceWriteback0 && i < hdr->numNormalConstr0)
				FStore(appliedForce0, vForceWriteback0++);
			if(vForceWriteback1 && i < hdr->numNormalConstr1)
				FStore(appliedForce1, vForceWriteback1++);
			if(vForceWriteback2 && i < hdr->numNormalConstr2)
				FStore(appliedForce2, vForceWriteback2++);
			if(vForceWriteback3 && i < hdr->numNormalConstr3)
				FStore(appliedForce3, vForceWriteback3++);
		}	

		if(numFrictionConstr)
		{
			const Vec4V zero = V4Zero();
			for(PxU32 i=0;i<numFrictionConstr;i++)
			{
				PxcSolverContactFrictionBase4& f = *frictions;
				frictions = (PxcSolverContactFrictionBase4*)(((PxU8*)frictions) + frictionSize);
				PX_ALIGN(16, PxU32 unbroken[4]);
				BStoreA(V4IsEq(f.broken, zero), unbroken);

				for(PxU32 a = 0; a < 4; ++a)
				{
					if(f.frictionBrokenWritebackByte[a] != NULL && !unbroken[a])
						*f.frictionBrokenWritebackByte[a] = 1;	// PT: bad L2 miss here
				}
			}
		}
	}

	PX_ALIGN(16, PxReal nf[4]);
	V4StoreA(normalForce, nf);

	for(PxU32 a = 0; a < 4; ++a)
	{
		if(writeBackThresholds[a] && desc[a].linkIndexA == PxcSolverConstraintDesc::NO_LINK && desc[a].linkIndexB == PxcSolverConstraintDesc::NO_LINK &&
			PX_IR(nf[a]) !=0 && (bd0[a]->reportThreshold < PX_MAX_REAL  || bd1[a]->reportThreshold < PX_MAX_REAL))
		{
			PxcThresholdStreamElement elt;
			elt.normalForce = nf[a];
			elt.threshold = PxMin<float>(bd0[a]->reportThreshold, bd1[a]->reportThreshold);
			elt.body0 = bd0[a]->originalBody;
			elt.body1 = bd1[a]->originalBody;
			Ps::order(elt.body0,elt.body1);
			PX_ASSERT(elt.body0 < elt.body1);
			PX_ASSERT(cache.mThresholdStreamIndex<cache.mThresholdStreamLength);
			cache.mThresholdStream[cache.mThresholdStreamIndex++] = elt;
		}
	}
}


static void solve1D4_Block(const PxcSolverConstraintDesc* PX_RESTRICT desc, PxcSolverContext& /*cache*/)
{

	PxcSolverBody& b00 = *desc[0].bodyA;
	PxcSolverBody& b01 = *desc[0].bodyB;

	PxcSolverBody& b10 = *desc[1].bodyA;
	PxcSolverBody& b11 = *desc[1].bodyB;

	PxcSolverBody& b20 = *desc[2].bodyA;
	PxcSolverBody& b21 = *desc[2].bodyB;

	PxcSolverBody& b30 = *desc[3].bodyA;
	PxcSolverBody& b31 = *desc[3].bodyB;

	PxU8* PX_RESTRICT bPtr = desc[0].constraint;
	//PxU32 length = desc.constraintLength;

	const PxcSolverConstraint1DHeader4* PX_RESTRICT  header = (const PxcSolverConstraint1DHeader4* PX_RESTRICT)(bPtr);
	PxcSolverConstraint1DDynamic4* PX_RESTRICT base = (PxcSolverConstraint1DDynamic4* PX_RESTRICT)(header+1);

	//const FloatV fZero = FZero();
	Vec4V linVel00 = V4LoadA(&b00.linearVelocity.x);
	Vec4V linVel01 = V4LoadA(&b01.linearVelocity.x);
	Vec4V angVel00 = V4LoadA(&b00.angularVelocity.x);
	Vec4V angVel01 = V4LoadA(&b01.angularVelocity.x);

	Vec4V linVel10 = V4LoadA(&b10.linearVelocity.x);
	Vec4V linVel11 = V4LoadA(&b11.linearVelocity.x);
	Vec4V angVel10 = V4LoadA(&b10.angularVelocity.x);
	Vec4V angVel11 = V4LoadA(&b11.angularVelocity.x);

	Vec4V linVel20 = V4LoadA(&b20.linearVelocity.x);
	Vec4V linVel21 = V4LoadA(&b21.linearVelocity.x);
	Vec4V angVel20 = V4LoadA(&b20.angularVelocity.x);
	Vec4V angVel21 = V4LoadA(&b21.angularVelocity.x);

	Vec4V linVel30 = V4LoadA(&b30.linearVelocity.x);
	Vec4V linVel31 = V4LoadA(&b31.linearVelocity.x);
	Vec4V angVel30 = V4LoadA(&b30.angularVelocity.x);
	Vec4V angVel31 = V4LoadA(&b31.angularVelocity.x);


	Vec4V linVel0T0, linVel0T1, linVel0T2;
	Vec4V linVel1T0, linVel1T1, linVel1T2;
	Vec4V angVel0T0, angVel0T1, angVel0T2;
	Vec4V angVel1T0, angVel1T1, angVel1T2;


	PX_TRANSPOSE_44_34(linVel00, linVel10, linVel20, linVel30, linVel0T0, linVel0T1, linVel0T2);
	PX_TRANSPOSE_44_34(linVel01, linVel11, linVel21, linVel31, linVel1T0, linVel1T1, linVel1T2);
	PX_TRANSPOSE_44_34(angVel00, angVel10, angVel20, angVel30, angVel0T0, angVel0T1, angVel0T2);
	PX_TRANSPOSE_44_34(angVel01, angVel11, angVel21, angVel31, angVel1T0, angVel1T1, angVel1T2);

	const Vec4V	invMass0D0 = header->invMass0Dom0;
	const Vec4V	invMass1D1 = header->invMass1Dom1;

	PxU32 maxConstraints = header->count;

	for(PxU32 a = 0; a < maxConstraints; ++a)
	{
		PxcSolverConstraint1DDynamic4& c = *base;
		base++;
		Ps::prefetchLine(base);
		Ps::prefetchLine(base,128);
		Ps::prefetchLine(base,256);
		Ps::prefetchLine(base,384);


		const Vec4V velMultiplier = c.velMultiplier;
		const Vec4V impulseMultiplier = c.impulseMultiplier;
		const Vec4V constant = c.constant;		
		const Vec4V minImpulse = c.minImpulse;
		const Vec4V maxImpulse = c.maxImpulse;
		
		const Vec4V appliedForce = c.appliedForce;

		//Everything above this point is just standard SOA loading code!

		//transpose clinVel/cangVel to enable us to use Multiply-adds

		const Vec4V clinVel0T0 = c.lin0X;
		const Vec4V clinVel0T1 = c.lin0Y;
		const Vec4V clinVel0T2 = c.lin0Z;

		const Vec4V clinVel1T0 = c.lin1X;
		const Vec4V clinVel1T1 = c.lin1Y;
		const Vec4V clinVel1T2 = c.lin1Z;

		const Vec4V cangVel0T0 = c.ang0X;
		const Vec4V cangVel0T1 = c.ang0Y;
		const Vec4V cangVel0T2 = c.ang0Z;

		const Vec4V cangVel1T0 = c.ang1X;
		const Vec4V cangVel1T1 = c.ang1Y;
		const Vec4V cangVel1T2 = c.ang1Z;

		const Vec4V cangVelInertia0T0 = c.ang0InvInertiaX;
		const Vec4V cangVelInertia0T1 = c.ang0InvInertiaY;
		const Vec4V cangVelInertia0T2 = c.ang0InvInertiaZ;

		const Vec4V cangVelInertia1T0 = c.ang1InvInertia1X;
		const Vec4V cangVelInertia1T1 = c.ang1InvInertia1Y;
		const Vec4V cangVelInertia1T2 = c.ang1InvInertia1Z;

		const Vec4V linProj00(V4Mul(clinVel0T0, linVel0T0));
		const Vec4V linProj10(V4Mul(clinVel1T0, linVel1T0));
		const Vec4V angProj00(V4Mul(cangVel0T0, angVel0T0));
		const Vec4V angProj10(V4Mul(cangVel1T0, angVel1T0));

		const Vec4V linProj01(V4MulAdd(clinVel0T1, linVel0T1, linProj00));
		const Vec4V linProj11(V4MulAdd(clinVel1T1, linVel1T1, linProj10));
		const Vec4V angProj01(V4MulAdd(cangVel0T1, angVel0T1, angProj00));
		const Vec4V angProj11(V4MulAdd(cangVel1T1, angVel1T1, angProj10));
		
		const Vec4V linProj0(V4MulAdd(clinVel0T2, linVel0T2, linProj01));
		const Vec4V linProj1(V4MulAdd(clinVel1T2, linVel1T2, linProj11));
		const Vec4V angProj0(V4MulAdd(cangVel0T2, angVel0T2, angProj01));
		const Vec4V angProj1(V4MulAdd(cangVel1T2, angVel1T2, angProj11));

		const Vec4V projectVel0(V4Add(linProj0, angProj0));
		const Vec4V projectVel1(V4Add(linProj1, angProj1));
		
		const Vec4V normalVel(V4Sub(projectVel0, projectVel1));

		const Vec4V unclampedForce = V4MulAdd(appliedForce, impulseMultiplier, V4MulAdd(normalVel, velMultiplier, constant));
		const Vec4V clampedForce = V4Max(minImpulse, V4Min(maxImpulse, unclampedForce));
		const Vec4V deltaF = V4Sub(clampedForce, appliedForce);
		c.appliedForce = clampedForce;

		const Vec4V deltaFInvMass0 = V4Mul(deltaF, invMass0D0);
		const Vec4V deltaFInvMass1 = V4Mul(deltaF, invMass1D1);

		linVel0T0 = V4MulAdd(clinVel0T0, deltaFInvMass0, linVel0T0);
		linVel1T0 = V4MulAdd(clinVel1T0, deltaFInvMass1, linVel1T0);
		angVel0T0 = V4MulAdd(cangVelInertia0T0, deltaF, angVel0T0);
		angVel1T0 = V4MulAdd(cangVelInertia1T0, deltaF, angVel1T0);

		linVel0T1 = V4MulAdd(clinVel0T1, deltaFInvMass0, linVel0T1);
		linVel1T1 = V4MulAdd(clinVel1T1, deltaFInvMass1, linVel1T1);
		angVel0T1 = V4MulAdd(cangVelInertia0T1, deltaF, angVel0T1);
		angVel1T1 = V4MulAdd(cangVelInertia1T1, deltaF, angVel1T1);

		linVel0T2 = V4MulAdd(clinVel0T2, deltaFInvMass0, linVel0T2);
		linVel1T2 = V4MulAdd(clinVel1T2, deltaFInvMass1, linVel1T2);
		angVel0T2 = V4MulAdd(cangVelInertia0T2, deltaF, angVel0T2);
		angVel1T2 = V4MulAdd(cangVelInertia1T2, deltaF, angVel1T2);
	}

	PX_TRANSPOSE_34_44(linVel0T0, linVel0T1, linVel0T2, linVel00, linVel10, linVel20, linVel30);
	PX_TRANSPOSE_34_44(linVel1T0, linVel1T1, linVel1T2, linVel01, linVel11, linVel21, linVel31);
	PX_TRANSPOSE_34_44(angVel0T0, angVel0T1, angVel0T2, angVel00, angVel10, angVel20, angVel30);
	PX_TRANSPOSE_34_44(angVel1T0, angVel1T1, angVel1T2, angVel01, angVel11, angVel21, angVel31);


	// Write back
	V3StoreU(Vec3V_From_Vec4V_WUndefined(linVel00), b00.linearVelocity);
	V3StoreU(Vec3V_From_Vec4V_WUndefined(linVel10), b10.linearVelocity);
	V3StoreU(Vec3V_From_Vec4V_WUndefined(linVel20), b20.linearVelocity);
	V3StoreU(Vec3V_From_Vec4V_WUndefined(linVel30), b30.linearVelocity);

	V3StoreU(Vec3V_From_Vec4V_WUndefined(linVel01), b01.linearVelocity);
	V3StoreU(Vec3V_From_Vec4V_WUndefined(linVel11), b11.linearVelocity);
	V3StoreU(Vec3V_From_Vec4V_WUndefined(linVel21), b21.linearVelocity);
	V3StoreU(Vec3V_From_Vec4V_WUndefined(linVel31), b31.linearVelocity);

	V3StoreU(Vec3V_From_Vec4V_WUndefined(angVel00), b00.angularVelocity);
	V3StoreU(Vec3V_From_Vec4V_WUndefined(angVel10), b10.angularVelocity);
	V3StoreU(Vec3V_From_Vec4V_WUndefined(angVel20), b20.angularVelocity);
	V3StoreU(Vec3V_From_Vec4V_WUndefined(angVel30), b30.angularVelocity);

	V3StoreU(Vec3V_From_Vec4V_WUndefined(angVel01), b01.angularVelocity);
	V3StoreU(Vec3V_From_Vec4V_WUndefined(angVel11), b11.angularVelocity);
	V3StoreU(Vec3V_From_Vec4V_WUndefined(angVel21), b21.angularVelocity);
	V3StoreU(Vec3V_From_Vec4V_WUndefined(angVel31), b31.angularVelocity);
	
}

static void conclude1D4_Block(const PxcSolverConstraintDesc* PX_RESTRICT desc, PxcSolverContext& /*cache*/)
{
	PxcSolverConstraint1DHeader4* header = reinterpret_cast<PxcSolverConstraint1DHeader4*>(desc[0].constraint);
	PxU8* base = desc[0].constraint + sizeof(PxcSolverConstraint1DHeader4);
	PxU32 stride = header->type == PXS_SC_TYPE_BLOCK_1D ? sizeof(PxcSolverConstraint1DDynamic4) : sizeof(PxcSolverConstraint1DBase4);

	for(PxU32 i=0; i<header->count; i++)
	{
		PxcSolverConstraint1DBase4& c = *reinterpret_cast<PxcSolverConstraint1DBase4*>(base);
		c.constant = c.unbiasedConstant;
		base += stride;
	}
	PX_ASSERT(desc[0].constraint + getConstraintLength(desc[0]) == base);
}

void writeBack1D4(const PxcSolverConstraintDesc* PX_RESTRICT desc, PxcSolverContext& /*cache*/,
							 const PxcSolverBodyData** PX_RESTRICT /*bd0*/, const PxcSolverBodyData** PX_RESTRICT /*bd1*/)
{
	PxsConstraintWriteback* writeback0 = reinterpret_cast<PxsConstraintWriteback*>(desc[0].writeBack);
	PxsConstraintWriteback* writeback1 = reinterpret_cast<PxsConstraintWriteback*>(desc[1].writeBack);
	PxsConstraintWriteback* writeback2 = reinterpret_cast<PxsConstraintWriteback*>(desc[2].writeBack);
	PxsConstraintWriteback* writeback3 = reinterpret_cast<PxsConstraintWriteback*>(desc[3].writeBack);

	if(writeback0 || writeback1 || writeback2 || writeback3)
	{
		PxcSolverConstraint1DHeader4* header = reinterpret_cast<PxcSolverConstraint1DHeader4*>(desc[0].constraint);
		PxU8* base = desc[0].constraint + sizeof(PxcSolverConstraint1DHeader4);
		PxU32 stride = header->type == PXS_SC_TYPE_BLOCK_1D ? sizeof(PxcSolverConstraint1DDynamic4) : sizeof(PxcSolverConstraint1DBase4);

		const Vec4V zero = V4Zero();
		Vec4V linX(zero), linY(zero), linZ(zero); 

		for(PxU32 i=0; i<header->count; i++)
		{
			const PxcSolverConstraint1DBase4* c = reinterpret_cast<PxcSolverConstraint1DBase4*>(base);

			//Load in flags
			const VecI32V flags = I4LoadU((PxI32*)&c->flags[0]);
			//Work out masks
			const VecI32V mask = I4Load(PXS_SC_FLAG_OUTPUT_FORCE);

			const VecI32V masked = VecI32V_And(flags, mask);
			const BoolV isEq = VecI32V_IsEq(masked, mask);

			const Vec4V appliedForce = V4Sel(isEq, c->appliedForce, zero);

			linX = V4MulAdd(c->lin0X, appliedForce, linX);
			linY = V4MulAdd(c->lin0Y, appliedForce, linY);
			linZ = V4MulAdd(c->lin0Z, appliedForce, linZ);

			base += stride;
		}

		//We need to do the cross product now

		Vec4V angX = V4NegMulSub(header->body0WorkOffsetZ, linY, V4Mul(header->body0WorkOffsetY, linZ));
		Vec4V angY = V4NegMulSub(header->body0WorkOffsetX, linZ, V4Mul(header->body0WorkOffsetZ, linX));
		Vec4V angZ = V4NegMulSub(header->body0WorkOffsetY, linX, V4Mul(header->body0WorkOffsetX, linY));

		const Vec4V linLenSq = V4MulAdd(linZ, linZ, V4MulAdd(linY, linY, V4Mul(linX, linX)));
		const Vec4V angLenSq = V4MulAdd(angZ, angZ, V4MulAdd(angY, angY, V4Mul(angX, angX)));

		const Vec4V linLen = V4Sqrt(linLenSq);
		const Vec4V angLen = V4Sqrt(angLenSq);

		const BoolV broken = BOr(V4IsGrtr(linLen, header->linBreakImpulse), V4IsGrtr(angLen, header->angBreakImpulse));

		PX_ALIGN(16, PxU32 iBroken[4]);
		BStoreA(broken, iBroken);


		Vec4V lin0, lin1, lin2, lin3;
		Vec4V ang0, ang1, ang2, ang3;

		PX_TRANSPOSE_34_44(linX, linY, linZ, lin0, lin1, lin2, lin3);
		PX_TRANSPOSE_34_44(angX, angY, angZ, ang0, ang1, ang2, ang3);

		if(writeback0)
		{
			V3StoreU(Vec3V_From_Vec4V_WUndefined(lin0), writeback0->linearImpulse);
			V3StoreU(Vec3V_From_Vec4V_WUndefined(ang0), writeback0->angularImpulse);
			writeback0->broken = iBroken[0] != 0;
		}
		if(writeback1)
		{
			V3StoreU(Vec3V_From_Vec4V_WUndefined(lin1), writeback1->linearImpulse);
			V3StoreU(Vec3V_From_Vec4V_WUndefined(ang1), writeback1->angularImpulse);
			writeback1->broken = iBroken[1] != 0;
		}
		if(writeback2)
		{
			V3StoreU(Vec3V_From_Vec4V_WUndefined(lin2), writeback2->linearImpulse);
			V3StoreU(Vec3V_From_Vec4V_WUndefined(ang2), writeback2->angularImpulse);
			writeback2->broken = iBroken[2] != 0;
		}
		if(writeback3)
		{
			V3StoreU(Vec3V_From_Vec4V_WUndefined(lin3), writeback3->linearImpulse);
			V3StoreU(Vec3V_From_Vec4V_WUndefined(ang3), writeback3->angularImpulse);
			writeback3->broken = iBroken[3] != 0;
		}

		PX_ASSERT(desc[0].constraint + getConstraintLength(desc[0]) == base);
	}
}


void solveContactPreBlock(const PxcSolverConstraintDesc* PX_RESTRICT desc, const PxU32 /*constraintCount*/, PxcSolverContext& cache)
{
	solveContact4_Block(desc, cache);
}

void solveContactPreBlock_Static(const PxcSolverConstraintDesc* PX_RESTRICT desc, const PxU32  /*constraintCount*/, PxcSolverContext& cache)
{
	solveContact4_StaticBlock(desc, cache);
}

void solveContactPreBlock_Conclude(const PxcSolverConstraintDesc* PX_RESTRICT desc, const PxU32  /*constraintCount*/, PxcSolverContext& cache)
{
	solveContact4_Block(desc, cache);
	concludeContact4_Block(desc, cache, sizeof(PxcSolverContactBatchPointDynamic4), sizeof(PxcSolverContactFrictionDynamic4));
}

void solveContactPreBlock_ConcludeStatic(const PxcSolverConstraintDesc* PX_RESTRICT desc, const PxU32  /*constraintCount*/, PxcSolverContext& cache)
{
	solveContact4_StaticBlock(desc, cache);
	concludeContact4_Block(desc, cache, sizeof(PxcSolverContactBatchPointBase4), sizeof(PxcSolverContactFrictionBase4));
}

void solveContactPreBlock_WriteBack(const PxcSolverConstraintDesc* PX_RESTRICT desc, const PxU32  /*constraintCount*/, PxcSolverContext& cache,
										PxcThresholdStreamElement* PX_RESTRICT thresholdStream, const PxU32 /*thresholdStreamLength*/, PxI32* outThresholdPairs)
{
	solveContact4_Block(desc, cache);

	const PxcSolverBodyData* bd0[4] = {	&cache.solverBodyArray[desc[0].bodyADataIndex], 
										&cache.solverBodyArray[desc[1].bodyADataIndex],
										&cache.solverBodyArray[desc[2].bodyADataIndex],
										&cache.solverBodyArray[desc[3].bodyADataIndex]};

	const PxcSolverBodyData* bd1[4] = {	&cache.solverBodyArray[desc[0].bodyBDataIndex], 
										&cache.solverBodyArray[desc[1].bodyBDataIndex],
										&cache.solverBodyArray[desc[2].bodyBDataIndex],
										&cache.solverBodyArray[desc[3].bodyBDataIndex]};

	writeBackContact4_Block(desc, cache, bd0, bd1);

	if(cache.mThresholdStreamIndex > (cache.mThresholdStreamLength - 4))
	{
		//Write back to global buffer
		PxI32 threshIndex = physx::shdfnd::atomicAdd(outThresholdPairs, cache.mThresholdStreamIndex) - cache.mThresholdStreamIndex;
		for(PxU32 a = 0; a < cache.mThresholdStreamIndex; ++a)
		{
			thresholdStream[a + threshIndex] = cache.mThresholdStream[a];
		}
		cache.mThresholdStreamIndex = 0;
	}
}

void contactPreBlock_WriteBack(const PxcSolverConstraintDesc* PX_RESTRICT desc, const PxU32  /*constraintCount*/, PxcSolverContext& cache,
										PxcThresholdStreamElement* PX_RESTRICT thresholdStream, const PxU32 /*thresholdStreamLength*/, PxI32* outThresholdPairs)
{
	const PxcSolverBodyData* bd0[4] = {	&cache.solverBodyArray[desc[0].bodyADataIndex], 
										&cache.solverBodyArray[desc[1].bodyADataIndex],
										&cache.solverBodyArray[desc[2].bodyADataIndex],
										&cache.solverBodyArray[desc[3].bodyADataIndex]};

	const PxcSolverBodyData* bd1[4] = {	&cache.solverBodyArray[desc[0].bodyBDataIndex], 
										&cache.solverBodyArray[desc[1].bodyBDataIndex],
										&cache.solverBodyArray[desc[2].bodyBDataIndex],
										&cache.solverBodyArray[desc[3].bodyBDataIndex]};

	writeBackContact4_Block(desc, cache, bd0, bd1);

	if(cache.mThresholdStreamIndex > (cache.mThresholdStreamLength - 4))
	{
		//Write back to global buffer
		PxI32 threshIndex = physx::shdfnd::atomicAdd(outThresholdPairs, cache.mThresholdStreamIndex) - cache.mThresholdStreamIndex;
		for(PxU32 a = 0; a < cache.mThresholdStreamIndex; ++a)
		{
			thresholdStream[a + threshIndex] = cache.mThresholdStream[a];
		}
		cache.mThresholdStreamIndex = 0;
	}
}

void solveContactPreBlock_WriteBackStatic(const PxcSolverConstraintDesc* PX_RESTRICT desc, const PxU32  /*constraintCount*/, PxcSolverContext& cache,
										PxcThresholdStreamElement* PX_RESTRICT thresholdStream, const PxU32 /*thresholdStreamLength*/, PxI32* outThresholdPairs)
{
	solveContact4_StaticBlock(desc, cache);
	const PxcSolverBodyData* bd0[4] = {	&cache.solverBodyArray[desc[0].bodyADataIndex], 
										&cache.solverBodyArray[desc[1].bodyADataIndex],
										&cache.solverBodyArray[desc[2].bodyADataIndex],
										&cache.solverBodyArray[desc[3].bodyADataIndex]};

	const PxcSolverBodyData* bd1[4] = {	&cache.solverBodyArray[desc[0].bodyBDataIndex], 
										&cache.solverBodyArray[desc[1].bodyBDataIndex],
										&cache.solverBodyArray[desc[2].bodyBDataIndex],
										&cache.solverBodyArray[desc[3].bodyBDataIndex]};

	writeBackContact4_Block(desc, cache, bd0, bd1);

	if(cache.mThresholdStreamIndex > (cache.mThresholdStreamLength - 4))
	{
		//Write back to global buffer
		PxI32 threshIndex = physx::shdfnd::atomicAdd(outThresholdPairs, cache.mThresholdStreamIndex) - cache.mThresholdStreamIndex;
		for(PxU32 a = 0; a < cache.mThresholdStreamIndex; ++a)
		{
			thresholdStream[a + threshIndex] = cache.mThresholdStream[a];
		}
		cache.mThresholdStreamIndex = 0;
	}
}

void solve1D4_Block(const PxcSolverConstraintDesc* PX_RESTRICT desc, const PxU32  /*constraintCount*/, PxcSolverContext& cache)
{
	solve1D4_Block(desc, cache);
}


void solve1D4Block_Conclude(const PxcSolverConstraintDesc* PX_RESTRICT desc, const PxU32  /*constraintCount*/, PxcSolverContext& cache)
{
	solve1D4_Block(desc, cache);
	conclude1D4_Block(desc, cache);
}


void solve1D4Block_WriteBack(const PxcSolverConstraintDesc* PX_RESTRICT desc, const PxU32  /*constraintCount*/, PxcSolverContext& cache,
										PxcThresholdStreamElement* PX_RESTRICT /*thresholdStream*/, const PxU32 /*thresholdStreamLength*/, PxI32* /*outThresholdPairs*/)
{
	solve1D4_Block(desc, cache);

	const PxcSolverBodyData* bd0[4] = {	&cache.solverBodyArray[desc[0].bodyADataIndex], 
										&cache.solverBodyArray[desc[1].bodyADataIndex],
										&cache.solverBodyArray[desc[2].bodyADataIndex],
										&cache.solverBodyArray[desc[3].bodyADataIndex]};

	const PxcSolverBodyData* bd1[4] = {	&cache.solverBodyArray[desc[0].bodyBDataIndex], 
										&cache.solverBodyArray[desc[1].bodyBDataIndex],
										&cache.solverBodyArray[desc[2].bodyBDataIndex],
										&cache.solverBodyArray[desc[3].bodyBDataIndex]};

	writeBack1D4(desc, cache, bd0, bd1);
}

void writeBack1D4Block(const PxcSolverConstraintDesc* PX_RESTRICT desc, const PxU32  /*constraintCount*/, PxcSolverContext& cache,
										PxcThresholdStreamElement* PX_RESTRICT /*thresholdStream*/, const PxU32 /*thresholdStreamLength*/, PxI32* /*outThresholdPairs*/)
{
	const PxcSolverBodyData* bd0[4] = {	&cache.solverBodyArray[desc[0].bodyADataIndex], 
										&cache.solverBodyArray[desc[1].bodyADataIndex],
										&cache.solverBodyArray[desc[2].bodyADataIndex],
										&cache.solverBodyArray[desc[3].bodyADataIndex]};

	const PxcSolverBodyData* bd1[4] = {	&cache.solverBodyArray[desc[0].bodyBDataIndex], 
										&cache.solverBodyArray[desc[1].bodyBDataIndex],
										&cache.solverBodyArray[desc[2].bodyBDataIndex],
										&cache.solverBodyArray[desc[3].bodyBDataIndex]};

	writeBack1D4(desc, cache, bd0, bd1);
}

}

#endif
