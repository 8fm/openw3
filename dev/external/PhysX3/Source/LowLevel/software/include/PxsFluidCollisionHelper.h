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


#ifndef PXS_FLUID_COLLISION_HELPER_H
#define PXS_FLUID_COLLISION_HELPER_H

#include "PxvDynamics.h"
#include "PxsFluidSpatialHash.h"

namespace physx
{

#define RANDOMIZED_COLLISION_PROTOTYPE 0
#define PXS_SURFACE_NORMAL_UNIT_TOLERANCE 5e-2f

struct PxsShapeCore;

struct PxsW2STransformTemp
{
	PxTransform			w2sOld;
	PxTransform			w2sNew;
};

PX_FORCE_INLINE PxF32 invertDcNum(PxF32 dcNum)
{
	PX_ASSERT(dcNum > 0.0f);
	if (dcNum < 3.0f)
	{
		PX_ASSERT(dcNum  == 1.0f || dcNum == 2.0f);
		return physx::intrinsics::fsel(dcNum - 1.5f, 0.5f, 1.0f);
	}
	else
	{
		return 1.0f / dcNum;
	}
}


#if RANDOMIZED_COLLISION_PROTOTYPE
static bool rrEnabled = false;
static PxF32 rrVelocityThresholdSqr = 10.0f;
static PxF32 rrRestitutionMin = 0.3f;
static PxF32 rrRestitutionMax = 0.5f;
static PxF32 rrDynamicFrictionMin = 0.03f;
static PxF32 rrDynamicFrictionMax = 0.05f;
static PxF32 rrStaticFrictionSqrMin = 1.0f;
static PxF32 rrStaticFrictionSqrMax = 2.0f;
static PxF32 rrAngleRadMax = physx::intrinsics::sin(PxPi/16);

static void selectRandomParameters(PxVec3& outSurfaceNormal, PxF32& outRestitution, PxF32& outDynamicFriction, PxF32& outStaticFrictionSqr, 
								   const PxU32 particleIndex, const PxVec3& surfaceNormal, const PxVec3& velocity, const PxsFluidCollisionParameters& params)
{
	static PxF32 noiseScale = (1.0f / 65536.0f);
	PxU32 noiseFactorP = particleIndex*particleIndex; //taking the square of the particleIndex yields better results.
	PxU32 noiseFactorTP = params.temporalNoise*noiseFactorP;

	PxF32 noise0 = ((noiseFactorTP*1735339) & 0xffff) * noiseScale;
	PxF32 noise1 = ((noiseFactorTP*1335379) & 0xffff) * noiseScale;
	PxF32 noise2 = ((noiseFactorP*1235303) & 0xffff) * noiseScale;

	outRestitution			= (1.0f - noise0)*rrRestitutionMin			+ noise0*rrRestitutionMax;
	outDynamicFriction		= (1.0f - noise1)*rrDynamicFrictionMin		+ noise1*rrDynamicFrictionMax;
	outStaticFrictionSqr    = (1.0f - noise2)*rrStaticFrictionSqrMin	+ noise2*rrStaticFrictionSqrMax; 

	if (velocity.magnitudeSquared() > rrVelocityThresholdSqr)
	{
		PxF32 noise3 = ((noiseFactorTP*14699023) & 0xffff) * noiseScale;
		PxF32 noise4 = ((noiseFactorTP*16699087) & 0xffff) * noiseScale;
		PxF32 noise5 = ((noiseFactorTP*11999027) & 0xffff) * noiseScale;

		PxVec3 tangent0, tangent1;
		normalToTangents(surfaceNormal, tangent0, tangent1);

		PxF32 angleNoise = noise3 * PxTwoPi;
		PxF32 angleCosNoise = physx::intrinsics::cos(angleNoise);
		PxF32 angleSinNoise = physx::intrinsics::sin(angleNoise);

		//skew towards mean
		PxF32 radiusNoise = noise4*noise5;
		PxVec3 tangent = tangent0*angleCosNoise + tangent1*angleSinNoise;

		outSurfaceNormal = surfaceNormal + tangent * radiusNoise * rrAngleRadMax;
		outSurfaceNormal.normalize();
	}
	else
	{
		outSurfaceNormal = surfaceNormal;
	}
}
#endif

//-------------------------------------------------------------------------------------------------------------------//

PX_FORCE_INLINE void clampVelocity(PxVec3& velocity, PxReal maxMotion, PxReal timeStep)
{
	PxReal velocityMagnitude = velocity.magnitude();
	if (velocityMagnitude * timeStep > maxMotion)
	{
		PxReal scaleFactor = maxMotion / (velocityMagnitude * timeStep);
		velocity *= scaleFactor;
	}
}

//-------------------------------------------------------------------------------------------------------------------//

PX_FORCE_INLINE void integrateParticleVelocity(PxsFluidParticle& particle, const PxF32 maxMotionDistance, const PxVec3& acceleration,
											   const PxF32 dampingDtComp, const PxF32 timeStep)
{	
	// Integrate
	particle.velocity += acceleration * timeStep;

	// Damp
	particle.velocity *= dampingDtComp;

	// Clamp velocity such that particle stays within maximum motion distance
	clampVelocity(particle.velocity, maxMotionDistance, timeStep);

	PX_ASSERT((particle.velocity * timeStep).magnitude() <= maxMotionDistance + 1e-5f);
}

//-----------------------------------------------------------------------------------------------------------------------//

PX_FORCE_INLINE void addDiscreteCollision(PxsParticleCollData& collData,
												 const PxVec3& newSurfaceNormal,
												 const PxVec3& newSurfacePos,
												 const PxVec3& newSurfaceVel,
												 const PxF32& dcNum)
{
	collData.flags |= PXS_FLUID_COLL_FLAG_DC;

	if (collData.flags & PXS_FLUID_COLL_FLAG_RESET_SNORMAL)
	{
		collData.surfaceNormal = newSurfaceNormal;
		collData.flags &= ~PXS_FLUID_COLL_FLAG_RESET_SNORMAL;
	}else	
		collData.surfaceNormal += newSurfaceNormal;

	// Discrete collisions will be averaged	
	collData.surfacePos += newSurfacePos;
	collData.surfaceVel += newSurfaceVel;
	collData.dcNum += dcNum;	// The passed surface normal/position/velocity can itself consist of
	// summed up normals/positions/velocities (for meshes for instance).
}

PX_FORCE_INLINE void addContinuousCollision(PxsParticleCollData& collData,												   
												   const PxVec3& newSurfaceNormal,
												   const PxVec3& newSurfacePos,
												   const PxVec3& newSurfaceVel)		
{
	collData.flags &= ~PXS_FLUID_COLL_FLAG_DC;	// Continuous collisions take precedence over discrete collisions
	collData.flags |= PXS_FLUID_COLL_FLAG_CC;

	collData.surfaceNormal = newSurfaceNormal;
	collData.surfacePos = newSurfacePos;
	collData.surfaceVel = newSurfaceVel;
}

PX_FORCE_INLINE void addConstraint(PxsParticleCollData& collData,
										  const PxVec3& newSurfaceNormal,
										  const PxVec3& newSurfacePos)
{
	//sschirm: Turns out that there are cases where a perfectly  normalized normal (-1,0,0) which is rotated by a 
	//quat with PxQuat::isSane(), has !PxVec3::isNormalized(). Therefore we intruduce a less conservative assert here.
	PX_ASSERT(PxAbs(newSurfaceNormal.magnitude()-1)<PXS_SURFACE_NORMAL_UNIT_TOLERANCE);	
	PxsFluidConstraint cN(newSurfaceNormal, newSurfacePos);
	if (!(collData.particleFlags.low & PxvInternalParticleFlag::eCONSTRAINT_0_VALID))
	{
		*collData.c0 = cN;		
		collData.particleFlags.low |= PxvInternalParticleFlag::eCONSTRAINT_0_VALID;
	}
	else if (!(collData.particleFlags.low & PxvInternalParticleFlag::eCONSTRAINT_1_VALID))
	{
		*collData.c1 = cN;		
		collData.particleFlags.low |= PxvInternalParticleFlag::eCONSTRAINT_1_VALID;
	}
	else
	{
		// Important: If the criterion to select the overwrite constraint changes, the fluid vs. static
		//            mesh code needs to be adjusted accordingly.

		// Overwrite constraint with the largest distance {old position} <--> {shape surface}.
		// The old position must be used since the new position is corrected after each collision occurrence.
		PxReal dist0 = collData.c0->normal.dot(collData.oldPos) - collData.c0->d;
		PxReal dist1 = collData.c1->normal.dot(collData.oldPos) - collData.c1->d;
		PxReal distN = cN.normal.dot(collData.oldPos) - cN.d;

		if (dist0 < dist1)
		{
			if (distN < dist1)
			{
				*collData.c1 = cN;			
				collData.particleFlags.low |= PxvInternalParticleFlag::eCONSTRAINT_1_VALID;
				collData.particleFlags.low &= PxU16(~PxvInternalParticleFlag::eCONSTRAINT_1_DYNAMIC);
			}			
		}
		else if (distN < dist0)
		{
			*collData.c0 = cN;		
			collData.particleFlags.low |= PxvInternalParticleFlag::eCONSTRAINT_0_VALID;
			collData.particleFlags.low &= PxU16(~PxvInternalParticleFlag::eCONSTRAINT_0_DYNAMIC);
		}		
	}
}

PX_FORCE_INLINE void addConstraintDynamic(PxsParticleCollData& collData,
										  const PxVec3& newSurfaceNormal,
										  const PxVec3& newSurfacePos,
										  const PxVec3& newSurfaceVel,
										  const PxsBodyCore* body,									
										  PxsFluidConstraintDynamic& c0Dynamic,
										  PxsFluidConstraintDynamic& c1Dynamic)
{
	//sschirm: Turns out that there are cases where a perfectly  normalized normal (-1,0,0) which is rotated by a 
	//quat with PxQuat::isSane(), has !PxVec3::isNormalized(). Therefore we intruduce a less conservative assert here.
	PX_ASSERT(PxAbs(newSurfaceNormal.magnitude()-1)<PXS_SURFACE_NORMAL_UNIT_TOLERANCE);	
	PxsFluidConstraint cN(newSurfaceNormal, newSurfacePos);
	if (!(collData.particleFlags.low & PxvInternalParticleFlag::eCONSTRAINT_0_VALID))
	{
		*collData.c0 = cN;
		c0Dynamic.velocity = newSurfaceVel;
		c0Dynamic.twoWayBody = body;
		collData.particleFlags.low |= (PxvInternalParticleFlag::eCONSTRAINT_0_VALID | PxvInternalParticleFlag::eCONSTRAINT_0_DYNAMIC);
	}
	else if (!(collData.particleFlags.low  & PxvInternalParticleFlag::eCONSTRAINT_1_VALID))
	{
		*collData.c1 = cN;
		c1Dynamic.velocity = newSurfaceVel;
		c1Dynamic.twoWayBody = body;
		collData.particleFlags.low |= (PxvInternalParticleFlag::eCONSTRAINT_1_VALID | PxvInternalParticleFlag::eCONSTRAINT_1_DYNAMIC);
	}
	else
	{
		// Important: If the criterion to select the overwrite constraint changes, the fluid vs. static
		//            mesh code needs to be adjusted accordingly.

		// Overwrite constraint with the largest distance {old position} <--> {shape surface}.
		// The old position must be used since the new position is corrected after each collision occurrence.
		PxReal dist0 = collData.c0->normal.dot(collData.oldPos) - collData.c0->d;
		PxReal dist1 = collData.c1->normal.dot(collData.oldPos) - collData.c1->d;
		PxReal distN = cN.normal.dot(collData.oldPos) - cN.d;

		if (dist0 < dist1)
		{
			if (distN < dist1)
			{
				*collData.c1 = cN;
				c1Dynamic.velocity = newSurfaceVel;
				c1Dynamic.twoWayBody = body;
				collData.particleFlags.low |= PxvInternalParticleFlag::eCONSTRAINT_1_DYNAMIC;
			}			
		}
		else if (distN < dist0)
		{
			*collData.c0 = cN;
			c0Dynamic.velocity = newSurfaceVel;
			c0Dynamic.twoWayBody = body;
			collData.particleFlags.low |= PxvInternalParticleFlag::eCONSTRAINT_0_DYNAMIC;
		}		
	}
}

/*!
Reflect velocity on shape surface.
- To apply friction, the current velocity is used
- For restitution a different velocity can be used
(This can help to avoid jittering effects. After the fluid particle dynamics update, forces are applied
to integrate the new velocities. If particle collision constraints work on these new velocities,
jittering can result. Using the old velocities (before the forces were applied) to compute the
normal impulse can solve this problem)
*/
PX_FORCE_INLINE void reflectVelocity(PxVec3& reflectedVel, const PxVec3& inVel, const PxVec3& oldVel, 
											const PxVec3& surfaceNormal, const PxVec3& surfaceVel, PxU32 particleIndex, 
											const PxsFluidCollisionParameters& params)
{ 
	PX_UNUSED(particleIndex);

	PxVec3 relativeVel = inVel - surfaceVel;
	PxReal projectedRelativeVel = surfaceNormal.dot(relativeVel);

	if(projectedRelativeVel < 0.0f) // Particle is moving closer to surface (else the collision will be resolved)
	{
		PxF32 rDynamicFriction;
		PxF32 rStaticFrictionSqr;
		PxF32 rRestitution;
		PxVec3 rSurfaceNormal;

#if RANDOMIZED_COLLISION_PROTOTYPE
		if (rrEnabled)
		{
			selectRandomParameters(rSurfaceNormal, rRestitution, rDynamicFriction, rStaticFrictionSqr, 
				particleIndex, surfaceNormal, relativeVel, params);
		}
		else
#endif
		{
			rDynamicFriction = params.dynamicFriction;
			rStaticFrictionSqr = params.staticFrictionSqr;
			rRestitution = params.restitution;
			rSurfaceNormal = surfaceNormal;
		}

		PxVec3 newNormalComponent = rSurfaceNormal * projectedRelativeVel;
		PxVec3 newTangentialComponent = relativeVel - newNormalComponent;

		PxVec3 oldRelativeVel = oldVel - surfaceVel;
		PxReal oldProjectedRelativeVel = rSurfaceNormal.dot(oldRelativeVel);
		PxVec3 oldNormalComponent = rSurfaceNormal * oldProjectedRelativeVel;

		// static friction (this works based on the quotient between tangential and normal velocity magnitude).
		PxVec3 diffNormalComponent = newNormalComponent - oldNormalComponent;

		PxReal stictionSqr = rStaticFrictionSqr*diffNormalComponent.magnitudeSquared();

		//if (newTangentialComponent.magnitudeSquared() < stictionSqr)
		//	newTangentialComponent = PxVec3(0);
		PxF32 diff = newTangentialComponent.magnitudeSquared() - stictionSqr;
		newTangentialComponent.x = physx::intrinsics::fsel(diff, newTangentialComponent.x, 0.0f);
		newTangentialComponent.y = physx::intrinsics::fsel(diff, newTangentialComponent.y, 0.0f);
		newTangentialComponent.z = physx::intrinsics::fsel(diff, newTangentialComponent.z, 0.0f);

		// pseudo dynamic friction (not dependent on normal component!)
		reflectedVel = newTangentialComponent * (1.0f - rDynamicFriction);

		// restitution is computed using the old velocity
		//if (oldProjectedRelativeVel < 0.0f)
		//	reflectedVel -= oldNormalComponent * mParams.restitution;
		PxVec3 reflectedVelTmp = reflectedVel - oldNormalComponent * rRestitution;
		reflectedVel.x = physx::intrinsics::fsel(oldProjectedRelativeVel, reflectedVel.x, reflectedVelTmp.x);
		reflectedVel.y = physx::intrinsics::fsel(oldProjectedRelativeVel, reflectedVel.y, reflectedVelTmp.y);
		reflectedVel.z = physx::intrinsics::fsel(oldProjectedRelativeVel, reflectedVel.z, reflectedVelTmp.z);

		reflectedVel += surfaceVel;
	}
	else
		reflectedVel = inVel;
}

PX_FORCE_INLINE void updateParticle(PxsFluidParticle& particle, const PxsParticleCollData& collData, bool projection, const PxPlane& projectionPlane, PxBounds3& worldBounds)
{
	//move worldBounds update here to avoid LHS
	if (!projection)
	{
		particle.velocity = collData.velocity; 
		particle.position = collData.newPos;
		PX_ASSERT(particle.position.isFinite());
		worldBounds.include(collData.newPos);	
	}
	else
	{
		const PxReal dist = projectionPlane.n.dot(collData.velocity);
		particle.velocity = collData.velocity - (projectionPlane.n * dist);		
		const PxVec3 pos = projectionPlane.project(collData.newPos);
		PX_ASSERT(pos.isFinite());
		particle.position = pos;
		worldBounds.include(pos);
	}
	particle.flags = collData.particleFlags;	
}

PX_FORCE_INLINE void clampToMaxMotion(PxsParticleCollData& collData, PxF32 maxMotionDistance, PxF32 maxMotionDistanceSqr)
{
	PxVec3 motionVec = collData.newPos - collData.oldPos;
	PxReal motionDistanceSqr = motionVec.magnitudeSquared();
	if (motionDistanceSqr > maxMotionDistanceSqr)
	{
		collData.newPos = collData.oldPos + (motionVec * maxMotionDistance * physx::intrinsics::recipSqrt(motionDistanceSqr));
	}
}

PX_FORCE_INLINE void updateCollDataDynamic(PxsParticleCollData& collData,										   
										   const PxTransform& bodyToWorld,
										   const PxVec3& linearVel,
										   const PxVec3& angularVel,										 											   
										   const PxsBodyCore* twoWayBody,
										   const PxTransform& shapeToWorld,
										   const PxReal timeStep,										
										   PxsFluidConstraintDynamic& c0Dynamic,
										   PxsFluidConstraintDynamic& c1Dynamic)
{	
	if (collData.localFlags & PXS_FLUID_COLL_FLAG_L_ANY)
	{
		PxVec3 newSurfaceNormal = shapeToWorld.rotate(collData.localSurfaceNormal);
		PxVec3 newSurfacePos = shapeToWorld.transform(collData.localSurfacePos);

		PxVec3 rotatedSurfacePosBody = newSurfacePos - bodyToWorld.p;

		PxVec3 angularSurfaceVel = angularVel.cross(rotatedSurfacePosBody);
		PxVec3 newSurfaceVel = angularSurfaceVel + linearVel;

		if (collData.localFlags & PXS_FLUID_COLL_FLAG_L_CC)
		{
			addContinuousCollision(collData, newSurfaceNormal, newSurfacePos, newSurfaceVel);
			//old body gets overwritten if a new one appears
			collData.twoWayBody = twoWayBody;
			collData.particleFlags.api |= PxParticleFlag::eCOLLISION_WITH_DYNAMIC;
		}
		if (collData.localFlags & PXS_FLUID_COLL_FLAG_L_DC)
		{
			addDiscreteCollision(collData, newSurfaceNormal, newSurfacePos, newSurfaceVel, 1);
			//old body gets overwritten if a new one appears
			collData.twoWayBody = twoWayBody;
			collData.particleFlags.api |= PxParticleFlag::eCOLLISION_WITH_DYNAMIC;
		}
		if (collData.localFlags & PXS_FLUID_COLL_FLAG_L_CC_PROX)
		{
			// Try to the predict the constraint for the next pose of the shape

			//sschirm: this code tries to call inv sqrt as much as possible it seems!
			// Predict surface position (for the rotation part an approximation is used)
			PxReal surfacePosDist = rotatedSurfacePosBody.magnitude();
			newSurfacePos = rotatedSurfacePosBody + angularSurfaceVel * timeStep;
			newSurfacePos = newSurfacePos.getNormalized();
			newSurfacePos *= surfacePosDist;

			newSurfacePos += (bodyToWorld.p + (linearVel * timeStep));

			// Predict surface normal (for the rotation an approximation is used)
			newSurfaceNormal += (angularVel.cross(newSurfaceNormal)) * timeStep;
			newSurfaceNormal = newSurfaceNormal.getNormalized();

			addConstraintDynamic(collData, newSurfaceNormal, newSurfacePos, newSurfaceVel, twoWayBody, c0Dynamic, c1Dynamic);
		}
	}
}

PX_FORCE_INLINE void updateCollDataStatic(PxsParticleCollData& collData, 
										  const PxTransform& shapeToWorld,
										  const PxReal /*timeStep*/)
{
	if (collData.localFlags & PXS_FLUID_COLL_FLAG_L_ANY)
	{
		PxVec3 newSurfaceNormal = shapeToWorld.rotate(collData.localSurfaceNormal);
		PxVec3 newSurfacePos = shapeToWorld.transform(collData.localSurfacePos);

		if (collData.localFlags & PXS_FLUID_COLL_FLAG_L_CC)
		{
			addContinuousCollision(collData, newSurfaceNormal, newSurfacePos, PxVec3(0));
			collData.particleFlags.api |= PxParticleFlag::eCOLLISION_WITH_STATIC;
		}
		if (collData.localFlags & PXS_FLUID_COLL_FLAG_L_DC)
		{
			addDiscreteCollision(collData, newSurfaceNormal, newSurfacePos, PxVec3(0), 1);
			collData.particleFlags.api |= PxParticleFlag::eCOLLISION_WITH_STATIC;
		}
		if (collData.localFlags & PXS_FLUID_COLL_FLAG_L_CC_PROX)
		{			
			addConstraint(collData, newSurfaceNormal, newSurfacePos);
		}
	}
}

PX_FORCE_INLINE void updateCollDataStaticMesh(PxsParticleCollData& collData,	  
											  const PxTransform& shapeToWorld,
											  const PxReal /*timeStep*/)
{
	if (collData.localFlags & PXS_FLUID_COLL_FLAG_L_ANY)
	{
		if (collData.localFlags & PXS_FLUID_COLL_FLAG_L_CC)
		{
			PxVec3 newSurfaceNormal(shapeToWorld.rotate(collData.localSurfaceNormal));

			// For static meshes, the old particle position is passed
			addContinuousCollision(collData, newSurfaceNormal, collData.oldPos, PxVec3(0));
			collData.particleFlags.api |= PxParticleFlag::eCOLLISION_WITH_STATIC;
		}
		if (collData.localFlags & PXS_FLUID_COLL_FLAG_L_DC)
		{
			// Average discrete collision data, transform to world space, multiply result to maintain the
			// weight of the data
			PX_ASSERT(collData.localDcNum > 0.0f);
			PxReal invDcNum = invertDcNum(collData.localDcNum);
			PxVec3 newSurfaceNormal(collData.localSurfaceNormal * invDcNum);
			PxVec3 newSurfacePos(collData.localSurfacePos * invDcNum);

			newSurfaceNormal = shapeToWorld.rotate(newSurfaceNormal) * collData.localDcNum;
			newSurfacePos = shapeToWorld.transform(newSurfacePos) * collData.localDcNum;

			addDiscreteCollision(collData, newSurfaceNormal, newSurfacePos, PxVec3(0), collData.localDcNum);
			collData.particleFlags.api |= PxParticleFlag::eCOLLISION_WITH_STATIC;
		}
		//if (collData.localFlags & PXS_FLUID_COLL_FLAG_L_CC_PROX)  mesh constraints already writed in collision function		
	}
}

PX_FORCE_INLINE bool applyConstraints(const PxVec3& rayOrig, PxVec3& rayDir, const PxVec3& oldVelocity, const PxsBodyCore*& twoWayBody,
									  PxVec3& shapeNormal, const PxsFluidConstraint* constr0, const PxsFluidConstraint* constr1,
									  const PxsBodyCore* constr0TwoWayBody, const PxsBodyCore* constr1TwoWayBody,
									  const PxVec3& constr0Velocity, const PxVec3& constr1Velocity,
									  const PxU32 particleIndex, const PxsFluidCollisionParameters& params, const PxvParticleFlags& particleFlags)
{
	PX_ASSERT(particleFlags.low & PxvInternalParticleFlag::eCONSTRAINT_0_VALID);	// There must be one constraint to get here
	bool needsRescaling = false;
	PxVec3 rayDirTmp = rayDir;   //avoid LHS
	PxVec3 newPos = rayOrig + rayDirTmp;	

	if (!(particleFlags.low & PxvInternalParticleFlag::eCONSTRAINT_1_VALID))
	{
		PxReal projectedNewPosC0 = constr0->normal.dot(newPos);

		if (projectedNewPosC0 < constr0->d)
		{
			twoWayBody = constr0TwoWayBody;
			shapeNormal = constr0->normal;
		}
		else
			return false;

		PxVec3 velocity = rayDirTmp * params.invTimeStep;
		reflectVelocity(rayDirTmp, velocity, oldVelocity, constr0->normal, constr0Velocity, particleIndex, params);

		// Compute motion direction of reflected particle and integrate position
		rayDirTmp *= params.timeStep;
		newPos = rayOrig + rayDirTmp;

		//
		// Constraint has been applied. Do second pass using the modified particle velocity and position
		//
		// - Check if modified particle is closer to the surface than in the last simulation step.
		//   If this is the case then move the particle such that the distance is at least as large as in the
		//   last step.
		//
		projectedNewPosC0 = constr0->normal.dot(newPos);
		if (constr0->d > projectedNewPosC0)
		{
			newPos = newPos + (constr0->normal * ((constr0->d - projectedNewPosC0) * 1.01f));	// Move particle in direction of surface normal
			rayDirTmp = newPos - rayOrig;
			needsRescaling = true;
		}
	}
	else
	{
		PxReal projectedNewPosC0 = constr0->normal.dot(newPos);
		PxReal projectedNewPosC1 = constr1->normal.dot(newPos);

		PxVec3 constraintVelocity;
		bool violateC0 = projectedNewPosC0 < constr0->d;
		bool violateC1 = projectedNewPosC1 < constr1->d;

		if (violateC0)
		{
			twoWayBody = constr0TwoWayBody;
			shapeNormal = constr0->normal;
			constraintVelocity = constr0Velocity;
		}
		else if (violateC1)
		{
			twoWayBody = constr1TwoWayBody;
			shapeNormal = constr1->normal;
			constraintVelocity = constr1Velocity;
		}
		else
			return false;

		if (!(violateC0 && violateC1))
		{
			PxVec3 velocity = rayDirTmp * params.invTimeStep;
			reflectVelocity(rayDirTmp, velocity, oldVelocity, shapeNormal, constraintVelocity, particleIndex, params);

			// Compute motion direction of reflected particle and integrate position
			rayDirTmp *= params.timeStep;
		}
		else
		{
			// removed reflection code for this case (leads to jittering on edges)
			// missing restitution/static friction term might cause other artifacts though
			rayDirTmp *= (1.0f - params.dynamicFriction);
		}
		newPos = rayOrig + rayDirTmp;

		//
		// Constraint has been applied. Do second pass using the modified particle velocity and position
		//
		// - Check if modified particle is closer to the surface than in the last simulation step.
		//   If this is the case then move the particle such that the distance is at least as large as in the
		//   last step.
		//

		projectedNewPosC0 = constr0->normal.dot(newPos);

		PxReal n0dotn1 = constr0->normal.dot(constr1->normal);

		if (PxAbs(n0dotn1) > (1.0f - PXS_FLUID_COLL_VEL_PROJECTION_CROSS_EPSILON))
		{
			// angle between collision surfaces above a certain threshold
			if (projectedNewPosC0 < constr0->d)
			{
				newPos = newPos + (constr0->normal * ((constr0->d - projectedNewPosC0) * 1.01f));	// Move particle in direction of surface normal
				rayDirTmp = newPos - rayOrig;
				needsRescaling = true;
			}
		}
		else
		{
			PxReal projectedNewPosC1 = constr1->normal.dot(newPos);

			PxReal distChange0 = constr0->d - projectedNewPosC0;
			PxVec3 newPos0 = newPos + constr0->normal * distChange0;	// Push particle in direction of surface normal

			PxReal distChange1 = constr1->d - projectedNewPosC1;
			PxVec3 newPos1 = newPos + constr1->normal * distChange1;	// Push particle in direction of surface normal

			if (projectedNewPosC0 < constr0->d || projectedNewPosC1 < constr1->d)
			{
				PxReal projectedNewPosC1nC0 = constr0->normal.dot(newPos1);
				PxReal projectedNewPosC0nC1 = constr1->normal.dot(newPos0);

				if (projectedNewPosC1nC0 < constr0->d && projectedNewPosC0nC1 < constr1->d)
				{
					PxReal factor = 1.0f / (1.0f - (n0dotn1 * n0dotn1));
					PxReal a0 = (distChange0 - (n0dotn1 * distChange1)) * factor;
					PxReal a1 = (distChange1 - (n0dotn1 * distChange0)) * factor;
					newPos += (constr0->normal*a0) + (constr1->normal*a1);

					rayDirTmp = newPos - rayOrig;

					PxVec3 epsVec = (constr0->normal+constr1->normal) * 0.5f * PXS_FLUID_COLL_VEL_PROJECTION_PROJ;
					rayDirTmp += epsVec * (rayDirTmp.dot(rayDirTmp));
				}
				else if (projectedNewPosC1nC0 < constr0->d)
				{
					newPos = newPos + (constr0->normal * ((1.0f + PXS_FLUID_COLL_VEL_PROJECTION_PROJ) * distChange0));
					rayDirTmp = newPos - rayOrig;
				}
				else
				{
					newPos = newPos + (constr1->normal * ((1.0f + PXS_FLUID_COLL_VEL_PROJECTION_PROJ) * distChange1));
					rayDirTmp = newPos - rayOrig;
				}
				needsRescaling = true;
			}
		}
	}

	// Clamp velocity to magnitude of original velocity
	if (needsRescaling)
	{
		PxF32 originalLengthSqr = rayDir.magnitudeSquared();
		PxF32 lengthSqr = rayDirTmp.magnitudeSquared();
		if (lengthSqr > originalLengthSqr)
		{
			rayDirTmp *= physx::intrinsics::sqrt(originalLengthSqr) * physx::intrinsics::recipSqrt(lengthSqr);
		}
	}
	rayDir = rayDirTmp;
	return true;
}

PX_FORCE_INLINE void initCollDataAndApplyConstraints(PxsParticleCollData& collData, const PxsFluidParticle& particle, const PxVec3& oldVelocity,
				const PxF32 restOffset, const PxVec3& constr0Velocity, const PxVec3& constr1Velocity, const PxsBodyCore* constr0TwoWayBody,
				const PxsBodyCore* constr1TwoWayBody, PxU32 particleIndex, const PxsFluidCollisionParameters& params)
{
	PX_ASSERT(particle.flags.api & PxParticleFlag::eVALID);

	collData.init(particle.position, restOffset, particleIndex, particle.flags);
	PxVec3 motionDir = particle.velocity * params.timeStep;

	//
	// Apply constraints from last simulation step
	//
	if (particle.flags.low & PxvInternalParticleFlag::eANY_CONSTRAINT_VALID)
	{
		PxVec3 motionDirOld = motionDir;	
	
		bool isColliding = applyConstraints(collData.oldPos, motionDir, oldVelocity, collData.twoWayBody, collData.surfaceNormal, collData.c0 , collData.c1,
			constr0TwoWayBody, constr1TwoWayBody, constr0Velocity, constr1Velocity, particleIndex, params, particle.flags);

		//can't have no collision but a twoWayShape
		PX_ASSERT(isColliding || !collData.twoWayBody);

		if (isColliding)
		{
			if (collData.twoWayBody)
			{
				//params.flags & PxParticleBaseFlag::eCOLLISION_TWOWAY doesn't really matter to compute this if two way is off
				collData.twoWayImpulse = (motionDirOld - motionDir) * params.invTimeStep;
				collData.particleFlags.api |= PxParticleFlag::eCOLLISION_WITH_DYNAMIC;
			}
			else
			{
				collData.particleFlags.api |= PxParticleFlag::eCOLLISION_WITH_STATIC;
			}
			collData.flags |= PXS_FLUID_COLL_FLAG_RESET_SNORMAL;
		}
	}

	collData.newPos = collData.oldPos + motionDir;
	collData.velocity = motionDir * params.invTimeStep;
}

void collisionResponse(PxsParticleCollData& collData, bool twoWay, PxsFluidCollisionParameters& params)
{
	bool continuousCollision = (collData.flags & PXS_FLUID_COLL_FLAG_CC) > 0;
	bool discreteCollision = (collData.flags & PXS_FLUID_COLL_FLAG_DC) > 0;

	// update of newPos
	PxVec3 surfaceNormal = collData.surfaceNormal;    //avoid LHS
	PxVec3 surfaceVel = collData.surfaceVel;          //avoid LHS
	if (continuousCollision)
	{
		// Particle has penetrated shape surface -> Push particle back to surface
		PX_ASSERT(!(collData.flags & PXS_FLUID_COLL_FLAG_DC));
		PX_ASSERT(collData.ccTime < 1.0f);
		PX_ASSERT(PxAbs(collData.surfaceNormal.magnitude()-1)<PXS_SURFACE_NORMAL_UNIT_TOLERANCE);	

		collData.newPos = collData.surfacePos;
	}
	else if (discreteCollision)
	{
		PxReal invDCNum = invertDcNum(collData.dcNum);
		collData.newPos = collData.surfacePos * invDCNum;
		surfaceVel = collData.surfaceVel * invDCNum;
		collData.surfaceVel = surfaceVel;

		// Since normals have unit length, we do not need to average, it is enough to
		// normalize the summed up contact normals.
		if (invDCNum == 1.0f); else
		{
			surfaceNormal = collData.surfaceNormal * physx::intrinsics::recipSqrt(collData.surfaceNormal.magnitudeSquared());
			collData.surfaceNormal = surfaceNormal;
		}

		collData.dcNum = 0.0f;
	}
	else
	{
		// Note: Proximity collisions have no immediate effect on the particle position,
		//       they are only used to build constraints.

		PX_ASSERT(!(collData.flags & (PXS_FLUID_COLL_FLAG_DC | PXS_FLUID_COLL_FLAG_CC)));
		return;		// It is important to return here if there is no collision
	}

	PX_ASSERT(continuousCollision || discreteCollision);

	PxVec3 newVel;
	reflectVelocity(newVel, collData.velocity, collData.velocity, surfaceNormal, surfaceVel, collData.debugParticleIndex, params);

	// if the collData.twoWayShape is set, we have a collision with a dynamic rb.
	if (twoWay && collData.twoWayBody) 
	{
		collData.twoWayImpulse = collData.velocity - newVel;
	}

	collData.velocity = newVel;
}

PX_FORCE_INLINE void computeLocalCellHash(PxsFluidLocalCellHash& localCellHash, PxU16* hashKeyArray,
			  const PxsFluidParticle* particles, PxU32 numParticles, const PxVec3& packetCorner, const PxReal cellSizeInv)
{
	PX_ASSERT(numParticles <= PXS_FLUID_SUBPACKET_PARTICLE_LIMIT_COLLISION);
	
	PxU32 numHashEntries = Ps::nextPowerOfTwo(numParticles + 1);
	numHashEntries = PxMin((PxU32)PXS_FLUID_LOCAL_HASH_SIZE_MESH_COLLISION, numHashEntries);

	// Make sure the number of hash entries is a power of 2 (requirement for the used hash function)
	PX_ASSERT((((numHashEntries-1)^numHashEntries)+1) == (2*numHashEntries));
	PX_ASSERT(numHashEntries > numParticles);

	// Get local cell hash for the current subpacket
	PxsFluidSpatialHash::buildLocalHash(particles, numParticles, localCellHash.hashEntries,
		localCellHash.particleIndices, hashKeyArray, numHashEntries, cellSizeInv, packetCorner);

	localCellHash.numHashEntries = numHashEntries;
	localCellHash.numParticles = numParticles;
	localCellHash.isHashValid = true;
}

}

#endif // PXS_FLUID_COLLISION_HELPER_H
