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

#if defined(__CUDACC__) && defined(__CUDA_ARCH__) && __CUDA_ARCH__ < 200
#define _CUDA_OPT_LOC_MEM_ 1
#else
#define _CUDA_OPT_LOC_MEM_ 0
#endif

#define PI 3.141592653589793f

//--- Helpers

PX_CUDA_CALLABLE PX_INLINE unsigned int binSearch(float val, unsigned int count, const float* data)
{
	unsigned int beg = 0;
	unsigned int end = count;

	while (beg < end)
	{
		unsigned int mid = beg + ((end - beg) >> 1);
		if (val < data[mid])
		{
			end = mid;
		}
		else
		{
			beg = mid + 1;
		}
	}
	return beg;
}

PX_CUDA_CALLABLE PX_INLINE void approxAxisAngleToMat33(const physx::PxVec3& axisAngle, physx::PxMat33Legacy& rot)
{
	const physx::PxF32 x = 0.5f * axisAngle.x;
	const physx::PxF32 y = 0.5f * axisAngle.y;
	const physx::PxF32 z = 0.5f * axisAngle.z;
	const physx::PxF32 xx = x * x;
	const physx::PxF32 yy = y * y;
	const physx::PxF32 zz = z * z;
	const physx::PxF32 xy = x * y;
	const physx::PxF32 yz = y * z;
	const physx::PxF32 zx = z * x;
	const physx::PxF32 twoRecipNorm2 = 2.0f / (1.0f + xx + yy + zz);	// w = 1
	rot(0, 0) = 1.0f - twoRecipNorm2 * (yy + zz);
	rot(0, 1) = twoRecipNorm2 * (xy - z);
	rot(0, 2) = twoRecipNorm2 * (zx + y);
	rot(1, 0) = twoRecipNorm2 * (xy + z);
	rot(1, 1) = 1.0f - twoRecipNorm2 * (zz + xx);
	rot(1, 2) = twoRecipNorm2 * (yz - x);
	rot(2, 0) = twoRecipNorm2 * (zx - y);
	rot(2, 1) = twoRecipNorm2 * (yz + x);
	rot(2, 2) = 1.0f - twoRecipNorm2 * (xx + yy);
}


PX_CUDA_CALLABLE PX_INLINE bool approxEquals(physx::PxF32 a, physx::PxF32 b, physx::PxF32 eps)
{
	const physx::PxF32 diff = physx::PxAbs(a - b);
	return (diff < eps);
}

PX_CUDA_CALLABLE PX_INLINE bool approxEquals(const physx::PxVec3& a, const physx::PxVec3& b, physx::PxF32 eps)
{
	return	approxEquals(a.x, b.x, eps) &&
	        approxEquals(a.y, b.y, eps) &&
	        approxEquals(a.z, b.z, eps);
}

PX_CUDA_CALLABLE PX_INLINE int maxAbsElementIndex(const physx::PxVec3& v)
{
	const physx::PxVec3 a(physx::PxAbs(v.x), physx::PxAbs(v.y), physx::PxAbs(v.z));
	const int m01 = (a.y > a.x);
	const int m2 = (a.z > a[m01]);
	return (m2 << 1) | (m01 >> m2);
}


PX_CUDA_CALLABLE PX_INLINE void generateRotationMatrix(const physx::PxVec3& srcVec, const physx::PxVec3& dstVec, physx::PxMat33Legacy& outRotMat)
{
	if (approxEquals(srcVec, dstVec, 0.0001f))
	{
		outRotMat.setIdentity();
		return;
	}

	physx::PxVec3 crossResult = srcVec.cross(dstVec);
	crossResult *= physx::PxAcos(srcVec.dot(dstVec));

	approxAxisAngleToMat33(crossResult, outRotMat);
}

PX_CUDA_CALLABLE PX_INLINE void generateRandomRotation(const physx::PxVec3& srcVec, physx::PxMat33Legacy& outRotMat, physx::RandState& randState)
{
	physx::PxVec3 tmpRotVec(srcVec);
	tmpRotVec *= randState.nextFloat(0.0f, 2 * PI);
	approxAxisAngleToMat33(tmpRotVec, outRotMat);
}

// ------------------------------------------------------------------------------------------------


//--- Rotation modifier ---

struct PARAMS_NAME(Rotation)
{
	static const physx::PxU32 RANDOM_COUNT = 3;

	physx::PxU32	rollType;
	physx::PxI32	rollAxis;
	physx::PxF32	rollSign;

	physx::PxF32	maxSettleRatePerSec;
	physx::PxF32	maxRotationRatePerSec;

	physx::PxF32	inAirRotationMultiplier;
	physx::PxF32	collisionRotationMultiplier;

	bool			includeVerticalDirection;

#ifndef __CUDACC__
	template <typename R>
	void reflect(R& r)
	{
		r.reflect(rollType);
		r.reflect(rollAxis);
		r.reflect(rollSign);
		r.reflect(maxSettleRatePerSec);
		r.reflect(maxRotationRatePerSec);
		r.reflect(inAirRotationMultiplier);
		r.reflect(collisionRotationMultiplier);
		r.reflect(includeVerticalDirection);
	}
#endif
};

PX_CUDA_CALLABLE PX_INLINE void chooseUp(physx::PxVec3& outUp, unsigned int rollType, physx::RandState& randState)
{
	const float angle = randState.nextFloat(0.0f, 2 * PI);
	const float up = angle < PI ? -1.0f : 1.0f;
	const float cosAng = physx::PxCos(angle);
	const float sinAng = physx::PxSin(angle);

	switch (rollType)
	{
	default:
		outUp = physx::PxVec3(0.0f, 0.0f, 1.0f);
		break;

	case physx::apex::NxApexMeshParticleRollType::FLAT_X:
		outUp = physx::PxVec3(up, 0.0f, 0.0f);
		break;
	case physx::apex::NxApexMeshParticleRollType::FLAT_Y:
		outUp = physx::PxVec3(0.0f, up, 0.0f);
		break;
	case physx::apex::NxApexMeshParticleRollType::FLAT_Z:
		outUp = physx::PxVec3(0.0f, 0.0f, up);
		break;

	case physx::apex::NxApexMeshParticleRollType::LONG_X:
		outUp = physx::PxVec3(0.0f, cosAng, sinAng);
		break;
	case physx::apex::NxApexMeshParticleRollType::LONG_Y:
		outUp = physx::PxVec3(cosAng, 0.0f, sinAng);
		break;
	case physx::apex::NxApexMeshParticleRollType::LONG_Z:
		outUp = physx::PxVec3(cosAng, sinAng, 0.0f);
		break;
	}
}

PX_CUDA_CALLABLE PX_INLINE void updateParticleRollBoxFromCollision(const PARAMS_NAME(Rotation)& params, physx::PxVec3& particleAngularDelta, const physx::PxMat33Legacy& rot, const physx::PxVec3& collisionNormal, float timeSlice)
{
	const float maxSettle = params.maxSettleRatePerSec * timeSlice;
	const float maxSettle2 = maxSettle * maxSettle; // where to compute this?

	if (params.rollType != physx::apex::NxApexMeshParticleRollType::SPHERICAL)
	{
		// Settling
		physx::PxVec3 a;
		if (params.rollAxis < 0)
		{
			// Cubic rolling, must choose most normal-pointing mRollAxis
			physx::PxVec3 overlap;
			rot.multiplyByTranspose(collisionNormal, overlap);
			const physx::PxU32 bestAxis = maxAbsElementIndex(overlap);
			rot.getColumn(bestAxis, a);
		}
		else
		{
			// Flat or long box, mRollAxis is chosen
			rot.getColumn(params.rollAxis, a);
		}

		physx::PxVec3 settle = (params.rollSign * a.dot(collisionNormal)) * a.cross(collisionNormal);
		if (maxSettle != 0.0f)
		{
			const float settle2 = settle.magnitudeSquared();
			// Cap how much settling is allowed per simulation tick
			if (settle2 > maxSettle2)
			{
				settle *= maxSettle * physx::PxRecipSqrt(settle2);
			}
		}
		particleAngularDelta += settle;
	}
}

PX_CUDA_CALLABLE PX_INLINE void updateParticleRollBoxNoCollision(const PARAMS_NAME(Rotation)& params, physx::PxVec3& particleAngularDelta, float timeSlice)
{
	const float maxRotation = params.maxRotationRatePerSec * timeSlice;
	const float maxRotation2 = maxRotation * maxRotation;

	if (maxRotation != 0.0f)
	{
		// Cap the total amount of roll.
		const float roll2 = particleAngularDelta.magnitudeSquared();
		if (roll2 > maxRotation2)
		{
			particleAngularDelta *= maxRotation * physx::PxRecipSqrt(roll2);
		}
	}
}


template <bool spawn, int usage, typename Input, typename PubState, typename PrivState>
MODIFIER_DECL void modifierRotation(const PARAMS_NAME(Rotation)& params, const Input& input, PubState& pubState, PrivState& privState, const physx::apex::iofx::ModifierCommonParams& common, physx::RandState& randState)
{
	if (usage == physx::apex::ModifierUsage_Mesh)
	{
		// TODO: This should really go into a 'settle' modifier, that says that objects should try to settle with one side up
		// or another, but for now it's here.
		if (spawn)
		{
			// Need to come up with a default pose.
			physx::PxMat33Legacy rotationOnSpawn;
			if (params.rollType == physx::apex::NxApexMeshParticleRollType::SPHERICAL)
			{
				physx::PxVec3 randomRotation;
				randomRotation.x = randState.nextFloat(-PI, PI);
				randomRotation.y = randState.nextFloat(-PI, PI);
				randomRotation.z = randState.nextFloat(-PI, PI);
				approxAxisAngleToMat33(randomRotation, rotationOnSpawn);
			}
			else
			{
				physx::PxVec3 upVector;
				chooseUp(upVector, params.rollType, randState);

				physx::PxVec3 unitSystemUp = common.upVector;
				unitSystemUp.normalize();

				physx::PxMat33Legacy rotateToUp, randomRotation;
				generateRotationMatrix(upVector, unitSystemUp, rotateToUp);
				generateRandomRotation(unitSystemUp, randomRotation, randState);

				rotationOnSpawn = randomRotation * rotateToUp;
			}
			privState.rotation = rotationOnSpawn;
			pubState.rotation = rotationOnSpawn;
		}
		else
		{
			physx::PxVec3 rollDelta;
			physx::PxVec3 delta = input.velocity * common.deltaTime;

			if (common.inputHasCollision)
			{
				if (input.collisionFlags != 0)
				{
					rollDelta = params.collisionRotationMultiplier * input.collisionNormal.cross(delta);
				}
				else
				{
					if (params.includeVerticalDirection)
					{
						// I'd like to use the absolute value of the "up" speed... just check it in first.
						rollDelta = params.inAirRotationMultiplier * delta;
					}
					else
					{
						rollDelta = params.inAirRotationMultiplier * common.upVector.cross( delta );
					}
				}

				updateParticleRollBoxFromCollision(params, rollDelta, privState.rotation, input.collisionNormal, common.deltaTime);
				updateParticleRollBoxNoCollision(params, rollDelta, common.deltaTime);
			}
			else
			{
				if (params.includeVerticalDirection)
				{
					// I'd like to use the absolute value of the "up" speed... just check it in first.
					rollDelta = params.inAirRotationMultiplier * delta;
				}
				else
				{
					rollDelta = params.inAirRotationMultiplier * common.upVector.cross( delta );
				}
				updateParticleRollBoxNoCollision(params, rollDelta, common.deltaTime);
			}

			if (rollDelta.magnitudeSquared() > 0.0f)
			{
				// update particle transform
				// A) This is because maxAngle isn't really working properly
				// B) This should be a configurable parameter of the system
				physx::PxMat33Legacy rot;
				approxAxisAngleToMat33(rollDelta, rot);
				privState.rotation = rot * privState.rotation;
			}
			pubState.rotation = privState.rotation;
		}
	}
}

//--- SimpleScale modifier ---

struct PARAMS_NAME(SimpleScale)
{
	static const physx::PxU32 RANDOM_COUNT = 0;

	physx::PxVec3 scaleFactor;

#ifndef __CUDACC__
	template <typename R>
	void reflect(R& r)
	{
		r.reflect(scaleFactor);
	}
#endif
};

template <bool spawn, int usage, typename Input, typename PubState, typename PrivState>
MODIFIER_DECL void modifierSimpleScale(const PARAMS_NAME(SimpleScale)& params, const Input& /*input*/, PubState& pubState, PrivState& /*privState*/, const physx::apex::iofx::ModifierCommonParams& /*common*/, physx::RandState& /*physx::RandState*/)
{
	pubState.scale.x *= params.scaleFactor.x;
	pubState.scale.y *= params.scaleFactor.y;
	pubState.scale.z *= params.scaleFactor.z;
}

//--- RandomScale modifier ---

struct PARAMS_NAME(RandomScale)
{
	static const physx::PxU32 RANDOM_COUNT = 1;

	physx::PxF32	scaleFactorMin;
	physx::PxF32	scaleFactorMax;

#ifndef __CUDACC__
	template <typename R>
	void reflect(R& r)
	{
		r.reflect(scaleFactorMin);
		r.reflect(scaleFactorMax);
	}
#endif
};

template <bool spawn, int usage, typename Input, typename PubState, typename PrivState>
MODIFIER_DECL void modifierRandomScale(const PARAMS_NAME(RandomScale)& params, const Input& /*input*/, PubState& pubState, PrivState& /*privState*/, const physx::apex::iofx::ModifierCommonParams& /*common*/, physx::RandState& randState)
{
	const float scaleFactor = randState.nextFloat(params.scaleFactorMin, params.scaleFactorMax);

	pubState.scale *= scaleFactor;
}

//--- ScaleByMass modifier ---

struct PARAMS_NAME(ScaleByMass)
{
	static const physx::PxU32 RANDOM_COUNT = 0;

	physx::PxF32 scaleFactor;

#ifndef __CUDACC__
	template <typename R>
	void reflect(R& r)
	{
		r.reflect(scaleFactor);
	}
#endif
};

template <bool spawn, int usage, typename Input, typename PubState, typename PrivState>
MODIFIER_DECL void modifierScaleByMass(const PARAMS_NAME(ScaleByMass)& /*params*/, const Input& input, PubState& pubState, PrivState& /*privState*/, const physx::apex::iofx::ModifierCommonParams& /*common*/, physx::RandState& /*physx::RandState*/)
{
	pubState.scale *= input.mass;
}

//--- ColorVsLife modifier ---

struct PARAMS_NAME(ColorVsLife)
{
	static const physx::PxU32 RANDOM_COUNT = 0;

	physx::PxU32	channel;
	CURVE_TYPE		curve;

#ifndef __CUDACC__
	template <typename R>
	void reflect(R& r)
	{
		r.reflect(channel);
		r.reflect(curve);
	}
#endif
};

template <bool spawn, int usage, typename Input, typename PubState, typename PrivState>
MODIFIER_DECL void modifierColorVsLife(const PARAMS_NAME(ColorVsLife)& params, const Input& input, PubState& pubState, PrivState& /*privState*/, const physx::apex::iofx::ModifierCommonParams& /*common*/, physx::RandState& /*physx::RandState*/)
{
#define _MODIFIER_CODE_(channel) \
	{ \
		if (pubState.color[channel] < 0.0f) \
			pubState.color[channel] = EVAL_CURVE(params.curve, input.liferemain); \
		else \
			pubState.color[channel] *= EVAL_CURVE(params.curve, input.liferemain); \
	}
#if _CUDA_OPT_LOC_MEM_
	if (params.channel == 0) _MODIFIER_CODE_(0)
		else if (params.channel == 1) _MODIFIER_CODE_(1)
			else if (params.channel == 2) _MODIFIER_CODE_(2)
				else if (params.channel == 3) _MODIFIER_CODE_(3)
#else
	_MODIFIER_CODE_(params.channel)
#endif
#undef _MODIFIER_CODE_

				}

//--- ColorVsDensity modifier ---

struct PARAMS_NAME(ColorVsDensity)
{
	static const physx::PxU32 RANDOM_COUNT = 0;

	physx::PxU32	channel;
	CURVE_TYPE		curve;

#ifndef __CUDACC__
	template <typename R>
	void reflect(R& r)
	{
		r.reflect(channel);
		r.reflect(curve);
	}
#endif
};

template <bool spawn, int usage, typename Input, typename PubState, typename PrivState>
MODIFIER_DECL void modifierColorVsDensity(const PARAMS_NAME(ColorVsDensity)& params, const Input& input, PubState& pubState, PrivState& /*privState*/, const physx::apex::iofx::ModifierCommonParams& /*common*/, physx::RandState& /*physx::RandState*/)
{
#define _MODIFIER_CODE_(channel) \
	{ \
		if (pubState.color[channel] < 0.0f) \
			pubState.color[channel] = EVAL_CURVE(params.curve, input.density); \
		else \
			pubState.color[channel] *= EVAL_CURVE(params.curve, input.density); \
	}
#if _CUDA_OPT_LOC_MEM_
	if (params.channel == 0) _MODIFIER_CODE_(0)
		else if (params.channel == 1) _MODIFIER_CODE_(1)
			else if (params.channel == 2) _MODIFIER_CODE_(2)
				else if (params.channel == 3) _MODIFIER_CODE_(3)
#else
	_MODIFIER_CODE_(params.channel)
#endif
#undef _MODIFIER_CODE_
				}

//--- ColorVsVelocity modifier ---

struct PARAMS_NAME(ColorVsVelocity)
{
	static const physx::PxU32 RANDOM_COUNT = 0;

	physx::PxF32	velocity0;
	physx::PxF32	velocity1;
	physx::PxU32	channel;
	CURVE_TYPE		curve;

#ifndef __CUDACC__
	template <typename R>
	void reflect(R& r)
	{
		r.reflect(velocity0);
		r.reflect(velocity1);
		r.reflect(channel);
		r.reflect(curve);
	}
#endif
};

template <bool spawn, int usage, typename Input, typename PubState, typename PrivState>
MODIFIER_DECL void modifierColorVsVelocity(const PARAMS_NAME(ColorVsVelocity)& params, const Input& input, PubState& pubState, PrivState& /*privState*/, const physx::apex::iofx::ModifierCommonParams& /*common*/, physx::RandState& /*physx::RandState*/)
{
	physx::PxF32 velocity = (input.velocity.magnitude() - params.velocity0) / (params.velocity1 - params.velocity0);
	velocity = physx::PxClamp(velocity, 0.0f, 1.0f);

#define _MODIFIER_CODE_(channel) \
	{ \
		if (pubState.color[channel] < 0.0f) \
			pubState.color[channel] = EVAL_CURVE(params.curve, velocity); \
		else \
			pubState.color[channel] *= EVAL_CURVE(params.curve, velocity); \
	}
#if _CUDA_OPT_LOC_MEM_
	if (params.channel == 0) _MODIFIER_CODE_(0)
		else if (params.channel == 1) _MODIFIER_CODE_(1)
			else if (params.channel == 2) _MODIFIER_CODE_(2)
				else if (params.channel == 3) _MODIFIER_CODE_(3)
#else
	_MODIFIER_CODE_(params.channel)
#endif
#undef _MODIFIER_CODE_
}

//--- SubtextureVsLife modifier ---

struct PARAMS_NAME(SubtextureVsLife)
{
	static const physx::PxU32 RANDOM_COUNT = 0;

	CURVE_TYPE		curve;

#ifndef __CUDACC__
	template <typename R>
	void reflect(R& r)
	{
		r.reflect(curve);
	}
#endif
};

template <bool spawn, int usage, typename Input, typename PubState, typename PrivState>
MODIFIER_DECL void modifierSubtextureVsLife(const PARAMS_NAME(SubtextureVsLife)& params, const Input& input, PubState& pubState, PrivState& /*privState*/, const physx::apex::iofx::ModifierCommonParams& /*common*/, physx::RandState& /*physx::RandState*/)
{
	if (usage == physx::apex::ModifierUsage_Sprite)
	{
		pubState.subTextureId += EVAL_CURVE(params.curve, input.liferemain);
	}
}

//--- OrientAlongVelocity modifier ---

PX_CUDA_CALLABLE PX_INLINE bool buildRotationMatrix(const physx::PxVec3& srcVec, const physx::PxVec3& dstVec, physx::PxMat33Legacy& outRotMat)
{
	physx::PxVec3 axis = srcVec.cross(dstVec);

	physx::PxF32 cosAngle = srcVec.dot(dstVec);
	physx::PxF32 angle = physx::PxAcos(cosAngle);

	physx::PxF32 axisLen = axis.normalize();
	if (axisLen < 0.0001f)
	{
		return false;
	}

	physx::PxQuat quat(angle, axis);
	outRotMat.fromQuat(quat);
	return true;
}

struct PARAMS_NAME(OrientAlongVelocity)
{
	static const physx::PxU32 RANDOM_COUNT = 0;

	physx::PxVec3	modelForward;

#ifndef __CUDACC__
	template <typename R>
	void reflect(R& r)
	{
		r.reflect(modelForward);
	}
#endif
};

template <bool spawn, int usage, typename Input, typename PubState, typename PrivState>
MODIFIER_DECL void modifierOrientAlongVelocity(const PARAMS_NAME(OrientAlongVelocity)& params, const Input& input, PubState& pubState, PrivState& privState, const physx::apex::iofx::ModifierCommonParams& /*common*/, physx::RandState& /*physx::RandState*/)
{
	if (usage == physx::apex::ModifierUsage_Mesh)
	{
		physx::PxVec3 vel = input.velocity;
		float velMagnitude = vel.normalize(); // normalize it
		// If travelling too slowly, this will be unstable, so don't do anything.
		if (physx::PxAbs(velMagnitude) >= 0.0001f)
		{
			physx::PxMat33Legacy rotMat;
			if (buildRotationMatrix(params.modelForward, vel, rotMat))
			{
				privState.rotation = rotMat;
			}
		}
		pubState.rotation = privState.rotation;
	}
}

//--- ScaleAlongVelocity modifier ---

///p,q -> p^q = n (n - must be normalized!)
PX_CUDA_CALLABLE PX_INLINE void buildPlaneBasis(const physx::PxVec3& n, physx::PxVec3& p, physx::PxVec3& q)
{
	const physx::PxF32 SQRT1_2 = physx::PxSqrt(physx::PxF32(0.5));
	if (physx::PxAbs(n.z) > SQRT1_2)
	{
		// choose p in y-z plane
		float k = sqrtf(n.y * n.y + n.z * n.z);
		p.x = 0;
		p.y = -n.z / k;
		p.z = n.y / k;
		// set q = n x p
		q.x = k;
		q.y = -n.x * p.z;
		q.z = n.x * p.y;
	}
	else
	{
		// choose p in x-y plane
		float k = physx::PxSqrt(n.x * n.x + n.y * n.y);
		p.x = -n.y / k;
		p.y = n.x / k;
		p.z = 0;
		// set q = n x p
		q.x = -n.z * p.y;
		q.y = n.z * p.x;
		q.z = k;
	}
}

PX_CUDA_CALLABLE PX_INLINE void buildScaleAlongAxis(const physx::PxVec3& scaleAxis, physx::PxF32 scale, physx::PxMat33Legacy& scaleMat)
{
	physx::PxVec3 axis0, axis1;
	buildPlaneBasis(scaleAxis, axis0, axis1);

	physx::PxMat33Legacy rotToAxisMat;
	rotToAxisMat.setColumn(0, axis0);
	rotToAxisMat.setColumn(1, axis1);
	rotToAxisMat.setColumn(2, scaleAxis);

	scaleMat = rotToAxisMat;
	scaleMat.setColumn(2, scaleAxis * scale);

	scaleMat.setMultiplyTransposeRight(scaleMat, rotToAxisMat);
}

struct PARAMS_NAME(ScaleAlongVelocity)
{
	static const physx::PxU32 RANDOM_COUNT = 0;

	physx::PxF32	scaleFactor;

#ifndef __CUDACC__
	template <typename R>
	void reflect(R& r)
	{
		r.reflect(scaleFactor);
	}
#endif
};

template <bool spawn, int usage, typename Input, typename PubState, typename PrivState>
MODIFIER_DECL void modifierScaleAlongVelocity(const PARAMS_NAME(ScaleAlongVelocity)& params, const Input& input, PubState& pubState, PrivState& /*privState*/, const physx::apex::iofx::ModifierCommonParams& /*common*/, physx::RandState& /*physx::RandState*/)
{
	if (usage == physx::apex::ModifierUsage_Mesh)
	{
		physx::PxVec3 vel = input.velocity;
		physx::PxF32 velMagnitude = vel.normalize(); // normalize it

		physx::PxMat33Legacy scaleMat;
		scaleMat.setIdentity();
		if (velMagnitude >= 0.0001f)
		{
			physx::PxF32 scale = 1.0f + velMagnitude * params.scaleFactor;

			buildScaleAlongAxis(vel, scale, scaleMat);
		}
		pubState.rotation = scaleMat * pubState.rotation;
	}
}

//--- RandomSubtexture modifier ---

struct PARAMS_NAME(RandomSubtexture)
{
	static const physx::PxU32 RANDOM_COUNT = 1;

	physx::PxF32	subtextureRangeMin;
	physx::PxF32	subtextureRangeMax;

#ifndef __CUDACC__
	template <typename R>
	void reflect(R& r)
	{
		r.reflect(subtextureRangeMin);
		r.reflect(subtextureRangeMax);
	}
#endif
};

template <bool spawn, int usage, typename Input, typename PubState, typename PrivState>
MODIFIER_DECL void modifierRandomSubtexture(const PARAMS_NAME(RandomSubtexture)& params, const Input& /*input*/, PubState& pubState, PrivState& /*privState*/, const physx::apex::iofx::ModifierCommonParams& /*common*/, physx::RandState& randState)
{
	if (usage == physx::apex::ModifierUsage_Sprite)
	{
		pubState.subTextureId += randState.nextFloat(params.subtextureRangeMin, params.subtextureRangeMax);
	}
}

//--- RandomRotation modifier ---

struct PARAMS_NAME(RandomRotation)
{
	static const physx::PxU32 RANDOM_COUNT = 1;

	physx::PxF32	rotationRangeMin;
	physx::PxF32	rotationRangeMax;

#ifndef __CUDACC__
	template <typename R>
	void reflect(R& r)
	{
		r.reflect(rotationRangeMin);
		r.reflect(rotationRangeMax);
	}
#endif
};

template <bool spawn, int usage, typename Input, typename PubState, typename PrivState>
MODIFIER_DECL void modifierRandomRotation(const PARAMS_NAME(RandomRotation)& params, const Input& /*input*/, PubState& pubState, PrivState& /*privState*/, const physx::apex::iofx::ModifierCommonParams& /*common*/, physx::RandState& randState)
{
	if (usage == physx::apex::ModifierUsage_Sprite)
	{
		const physx::PxF32 DegToRad = PI / 180;
		pubState.rotation += DegToRad * randState.nextFloat(params.rotationRangeMin, params.rotationRangeMax);
	}
}

//--- ScaleVsLife modifier ---

struct PARAMS_NAME(ScaleVsLife)
{
	static const physx::PxU32 RANDOM_COUNT = 0;

	physx::PxU32	axis;
	CURVE_TYPE		curve;

#ifndef __CUDACC__
	template <typename R>
	void reflect(R& r)
	{
		r.reflect(axis);
		r.reflect(curve);
	}
#endif
};

template <bool spawn, int usage, typename Input, typename PubState, typename PrivState>
MODIFIER_DECL void modifierScaleVsLife(const PARAMS_NAME(ScaleVsLife)& params, const Input& input, PubState& pubState, PrivState& /*privState*/, const physx::apex::iofx::ModifierCommonParams& /*common*/, physx::RandState& /*physx::RandState*/)
{
#define _MODIFIER_CODE_(axis) \
	{ \
		pubState.scale[axis] *= EVAL_CURVE(params.curve, input.liferemain); \
	}
#if _CUDA_OPT_LOC_MEM_
	if (params.axis == 0) _MODIFIER_CODE_(0)
		else if (params.axis == 1) _MODIFIER_CODE_(1)
			else if (params.axis == 2) _MODIFIER_CODE_(2)
#else
	_MODIFIER_CODE_(params.axis)
#endif
#undef _MODIFIER_CODE_
			}

//--- ScaleVsDensity modifier ---

struct PARAMS_NAME(ScaleVsDensity)
{
	static const physx::PxU32 RANDOM_COUNT = 0;

	physx::PxU32	axis;
	CURVE_TYPE		curve;

#ifndef __CUDACC__
	template <typename R>
	void reflect(R& r)
	{
		r.reflect(axis);
		r.reflect(curve);
	}
#endif
};

template <bool spawn, int usage, typename Input, typename PubState, typename PrivState>
MODIFIER_DECL void modifierScaleVsDensity(const PARAMS_NAME(ScaleVsDensity)& params, const Input& input, PubState& pubState, PrivState& /*privState*/, const physx::apex::iofx::ModifierCommonParams& /*common*/, physx::RandState& /*physx::RandState*/)
{
#define _MODIFIER_CODE_(axis) \
	{ \
		pubState.scale[axis] *= EVAL_CURVE(params.curve, input.density); \
	}
#if _CUDA_OPT_LOC_MEM_
	if (params.axis == 0) _MODIFIER_CODE_(0)
		else if (params.axis == 1) _MODIFIER_CODE_(1)
			else if (params.axis == 2) _MODIFIER_CODE_(2)
#else
	_MODIFIER_CODE_(params.axis)
#endif
#undef _MODIFIER_CODE_
			}

//--- ScaleVsCameraDistance modifier ---

struct PARAMS_NAME(ScaleVsCameraDistance)
{
	static const physx::PxU32 RANDOM_COUNT = 0;

	physx::PxU32	axis;
	CURVE_TYPE		curve;

#ifndef __CUDACC__
	template <typename R>
	void reflect(R& r)
	{
		r.reflect(axis);
		r.reflect(curve);
	}
#endif
};

template <bool spawn, int usage, typename Input, typename PubState, typename PrivState>
MODIFIER_DECL void modifierScaleVsCameraDistance(const PARAMS_NAME(ScaleVsCameraDistance)& params, const Input& input, PubState& pubState, PrivState& /*privState*/, const physx::apex::iofx::ModifierCommonParams& common, physx::RandState& /*physx::RandState*/)
{
	float cameraDistance = (input.position - common.eyePosition).magnitude();

#define _MODIFIER_CODE_(axis) \
	{ \
		pubState.scale[axis] *= EVAL_CURVE(params.curve, cameraDistance); \
	}
#if _CUDA_OPT_LOC_MEM_
	if (params.axis == 0) _MODIFIER_CODE_(0)
		else if (params.axis == 1) _MODIFIER_CODE_(1)
			else if (params.axis == 2) _MODIFIER_CODE_(2)
#else
	_MODIFIER_CODE_(params.axis)
#endif
#undef _MODIFIER_CODE_
			}

//--- ViewDirectionSorting modifier ---

struct PARAMS_NAME(ViewDirectionSorting)
{
	static const physx::PxU32 RANDOM_COUNT = 0;

#ifndef __CUDACC__
	template <typename R>
	void reflect(R&)
	{
	}
#endif
};

template <bool spawn, int usage, typename Input, typename PubState, typename PrivState>
MODIFIER_DECL void modifierViewDirectionSorting(const PARAMS_NAME(ViewDirectionSorting)& /*params*/, const Input& /*input*/, PubState& /*pubState*/, PrivState& /*privState*/, const physx::apex::iofx::ModifierCommonParams& /*common*/, physx::RandState& /*physx::RandState*/)
{
}

//--- RotationRate modifier ---

struct PARAMS_NAME(RotationRate)
{
	static const physx::PxU32 RANDOM_COUNT = 0;

	physx::PxF32	rotationRate;

#ifndef __CUDACC__
	template <typename R>
	void reflect(R& r)
	{
		r.reflect(rotationRate);
	}
#endif
};

template <bool spawn, int usage, typename Input, typename PubState, typename PrivState>
MODIFIER_DECL void modifierRotationRate(const PARAMS_NAME(RotationRate)& params, const Input& /*input*/, PubState& pubState, PrivState& privState, const physx::apex::iofx::ModifierCommonParams& common, physx::RandState& /*physx::RandState*/)
{
	if (usage == physx::apex::ModifierUsage_Sprite)
	{
		const physx::PxF32 TwoPi = 2 * PI;
		const physx::PxF32 rotationDelta = params.rotationRate * TwoPi * common.deltaTime;
		privState.rotation += rotationDelta;
		pubState.rotation += privState.rotation;
	}
}

//--- RotationRateVsLife modifier ---

struct PARAMS_NAME(RotationRateVsLife)
{
	static const physx::PxU32 RANDOM_COUNT = 0;

	CURVE_TYPE		curve;

#ifndef __CUDACC__
	template <typename R>
	void reflect(R& r)
	{
		r.reflect(curve);
	}
#endif
};

template <bool spawn, int usage, typename Input, typename PubState, typename PrivState>
MODIFIER_DECL void modifierRotationRateVsLife(const PARAMS_NAME(RotationRateVsLife)& params, const Input& input, PubState& pubState, PrivState& privState, const physx::apex::iofx::ModifierCommonParams& common, physx::RandState& /*physx::RandState*/)
{
	if (usage == physx::apex::ModifierUsage_Sprite)
	{
		const physx::PxF32 TwoPi = 2 * PI;
		const physx::PxF32 rotationDelta = EVAL_CURVE(params.curve, input.liferemain) * TwoPi * common.deltaTime;
		privState.rotation += rotationDelta;
		pubState.rotation += privState.rotation;
	}
}

//--- OrientScaleAlongScreenVelocity modifier ---

struct PARAMS_NAME(OrientScaleAlongScreenVelocity)
{
	static const physx::PxU32 RANDOM_COUNT = 0;

	physx::PxF32	scalePerVelocity;

#ifndef __CUDACC__
	template <typename R>
	void reflect(R& r)
	{
		r.reflect(scalePerVelocity);
	}
#endif
};

template <bool spawn, int usage, typename Input, typename PubState, typename PrivState>
MODIFIER_DECL void modifierOrientScaleAlongScreenVelocity(const PARAMS_NAME(OrientScaleAlongScreenVelocity)& params, const Input& input, PubState& pubState, PrivState& privState, const physx::apex::iofx::ModifierCommonParams& common, physx::RandState& /*physx::RandState*/)
{
	if (usage == physx::apex::ModifierUsage_Sprite)
	{
		physx::PxVec3 viewPos, viewVel; // position & velocity in view space

		viewPos.x = (input.position - common.eyePosition).dot(common.eyeAxisX);
		viewPos.y = (input.position - common.eyePosition).dot(common.eyeAxisY);
		viewPos.z = (input.position - common.eyePosition).dot(common.eyeDirection);   // eyeDir = eyeAxisZ

		// 4 is simply a hack that looks decent in UE3 ATM, we should use the FOV to determine when to
		// cull particles, otherwise you end up with particles around the zNear plane that have huge
		// 'd' values, resulting in bad scaling
		if (viewPos.z < common.zNear * 4)
		{
			pubState.rotation = privState.rotation;
			pubState.scale.x = 0.0f;
			pubState.scale.y = 0.0f;
			return;
		}

		viewVel.x = (input.velocity).dot(common.eyeAxisX);
		viewVel.y = (input.velocity).dot(common.eyeAxisY);
		viewVel.z = (input.velocity).dot(common.eyeDirection);   // eyeDir = eyeAxisZ

		// tan(angle) = (Vy*Pz - Py*Vz) / (Vx*Pz - Px*Vz)
		const physx::PxF32 velX = viewVel.x * viewPos.z - viewPos.x * viewVel.z;
		const physx::PxF32 velY = viewVel.y * viewPos.z - viewPos.y * viewVel.z;

		const physx::PxF32 velLengthMultiplier = 1.0f / viewPos.z;

		const physx::PxF32 dx = velX * velLengthMultiplier;
		const physx::PxF32 dy = velY * velLengthMultiplier;

		const physx::PxF32 d = physx::PxSqrt(dx * dx + dy * dy);

		if (d >= 1e-5f)
		{
			// "Note that the order of arguments is reversed; the function atan2(y,x) computes
			//  the angle corresponding to the point (x,y)."
			// see http://en.wikipedia.org/wiki/Atan2
			privState.rotation = physx::PxAtan2(velY, velX);

			const physx::PxF32 scale = 1.0f + d * params.scalePerVelocity;
			pubState.scale.x *= scale;
		}
		pubState.rotation = privState.rotation;


	}
}

#undef _CUDA_OPT_LOC_MEM_
