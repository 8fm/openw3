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

#ifndef __MODIFIER_H__
#define __MODIFIER_H__

#include "NxModifier.h"
#include "CurveImpl.h"
#include "PsUserAllocated.h"

#include "RotationModifierParams.h"
#include "SimpleScaleModifierParams.h"
#include "RandomScaleModifierParams.h"
#include "ScaleByMassModifierParams.h"
#include "ColorVsLifeModifierParams.h"
#include "ScaleVsLifeModifierParams.h"
#include "ScaleVsDensityModifierParams.h"
#include "ScaleVsCameraDistanceModifierParams.h"
#include "ColorVsDensityModifierParams.h"
#include "SubtextureVsLifeModifierParams.h"
#include "OrientAlongVelocityModifierParams.h"
#include "ScaleAlongVelocityModifierParams.h"
#include "RandomSubtextureModifierParams.h"
#include "RandomRotationModifierParams.h"
#include "ViewDirectionSortingModifierParams.h"
#include "RotationRateModifierParams.h"
#include "RotationRateVsLifeModifierParams.h"
#include "OrientScaleAlongScreenVelocityModifierParams.h"
#include "ColorVsVelocityModifierParams.h"

#include "NxParamArray.h"

#include "PsArray.h"
#include "PsMathUtils.h"

namespace physx
{
namespace apex
{

class InplaceStorage;
class InplaceHandleBase;
struct RandState;

namespace iofx
{

/**
	Directions for adding a new APEX modifier type (an official modifier, not a user modifier).

	1) In NxModifier.h, add a value to the end of ModifierTypeEnum, matching the naming convention of others.
	2) In NxModifier.h, add a new subclass of NxModifierT, specialized on your type.
		- e.g. class NxMyNewModifier : public NxModifierT<NxMyNewModifier> { };
	3) NxMyNewModifier should include pure virtual getters (by value), and setters. Do not promise to return
	   by const-reference.
    4) In Modifier.h (this file), add a subcless of your public class and ApexAllocateable
		- e.g. class MyNewModifier : public NxMyNewModifier, public physx::UserAllocated { };
	5) The Modifier.h class should provide concrete implementations of all of the functions.
		- Feel free to do the getters and setters inline in Modifier.h
	6) In Modifier.cpp, provide the implementations for the serailization functions and 'updateParticles.'
*/

// Ignore this warning temporarily. It's reset to default level at the bottom of the file.
// We'll fix this soon, just not "right this second"
#pragma warning( disable: 4100 )

class IofxModifierHelper
{
public:

	// assumes the mParams class contains a "controlPoints" member
	static void setNxCurve(const NxCurve* f,
	                       NxParameterized::Interface* mParams,
	                       NxParamDynamicArrayStruct* controlPoints)
	{
		physx::PxU32 cpSize;
		f->getControlPoints(cpSize);

		NxParameterized::Handle h(*mParams);
		mParams->getParameterHandle("controlPoints", h);
		h.resizeArray(cpSize);

		NxParamArray<ScaleVsCameraDistanceModifierParamsNS::vec2_Type>
		cpArray(mParams, "controlPoints", controlPoints);

		for (physx::PxU32 i = 0; i < cpSize; i++)
		{
			cpArray[i].x = f->getControlPoints(cpSize)[i].x;
			cpArray[i].y = f->getControlPoints(cpSize)[i].y;
		}
	}
};


class ModifierParamsMapperCPU
{
public:
	virtual void beginParams(void* params, size_t size, size_t align, physx::PxU32 randomCount) = 0;
	virtual void endParams() = 0;

	virtual void mapValue(size_t offset, physx::PxI32 value) = 0;
	void mapValue(size_t offset, physx::PxU32 value)
	{
		mapValue(offset, static_cast<physx::PxI32>(value));
	}

	virtual void mapValue(size_t offset, physx::PxF32 value) = 0;
	void mapValue(size_t offset, const physx::PxVec3& value)
	{
		mapValue(offset + offsetof(physx::PxVec3, x), value.x);
		mapValue(offset + offsetof(physx::PxVec3, y), value.y);
		mapValue(offset + offsetof(physx::PxVec3, z), value.z);
	}

	virtual void mapCurve(size_t offset, const NxCurve* curve) = 0;
};

#if defined(APEX_CUDA_SUPPORT)

class ModifierParamsMapperGPU
{
public:
	virtual InplaceStorage& getStorage() = 0;

	virtual void onParams(InplaceHandleBase handle, physx::PxU32 randomCount) = 0;
};
#endif

struct MeshInput;
struct MeshPublicState;
struct MeshPrivateState;
struct SpriteInput;
struct SpritePublicState;
struct SpritePrivateState;
struct ModifierCommonParams;

class Modifier
{
public:
	static const Modifier* castFrom(const NxModifier*);

#if defined(APEX_CUDA_SUPPORT)
	virtual void mapParamsGPU(ModifierParamsMapperGPU& mapper) const
	{
		PX_ASSERT(!"unimpl");
	}
#endif
	virtual void mapParamsCPU(ModifierParamsMapperCPU& /*mapper*/) const
	{
		PX_ASSERT(!"unimpl");
	}


	typedef void (*updateSpriteFunc)(const void* params, const SpriteInput& input, SpritePublicState& pubState, SpritePrivateState& privState, const ModifierCommonParams& common, RandState& randState);
	typedef void (*updateMeshFunc)(const void* params, const MeshInput& input, MeshPublicState& pubState, MeshPrivateState& privState, const ModifierCommonParams& common, RandState& randState);

	virtual updateSpriteFunc getUpdateSpriteFunc(ModifierStage /*stage*/) const
	{
		PX_ASSERT(!"unimpl");
		return 0;
	}
	virtual updateMeshFunc getUpdateMeshFunc(ModifierStage /*stage*/) const
	{
		PX_ASSERT(!"unimpl");
		return 0;
	}
};

// ------------------------------------------------------------------------------------------------
class RotationModifier : public NxRotationModifier, public Modifier, public physx::UserAllocated
{
public:
	RotationModifier(RotationModifierParams* params);

	virtual NxApexMeshParticleRollType::Enum getRollType() const
	{
		return mRollType;
	}
	virtual void setRollType(NxApexMeshParticleRollType::Enum rollType);
	virtual physx::PxF32 getMaxSettleRate() const
	{
		return mParams->maxSettleRatePerSec;
	}
	virtual void setMaxSettleRate(physx::PxF32 settleRate)
	{
		mParams->maxSettleRatePerSec = settleRate;
	}
	virtual physx::PxF32 getMaxRotationRate() const
	{
		return mParams->maxRotationRatePerSec;
	}
	virtual void setMaxRotationRate(physx::PxF32 rotationRate)
	{
		mParams->maxRotationRatePerSec = rotationRate;
	}

#if defined(APEX_CUDA_SUPPORT)
	virtual void mapParamsGPU(ModifierParamsMapperGPU& mapper) const;
#endif
	virtual void mapParamsCPU(ModifierParamsMapperCPU& mapper) const;

	template <class Mapper, typename Params>
	void mapParams(Mapper& mapper, Params* params) const
	{
		mapper.beginParams(params, sizeof(Params), __alignof(Params), Params::RANDOM_COUNT);

		mapper.mapValue(offsetof(Params, rollType), physx::PxU32(mRollType));
		mapper.mapValue(offsetof(Params, rollAxis), mRollAxis);
		mapper.mapValue(offsetof(Params, rollSign), mRollSign);

		mapper.mapValue(offsetof(Params, maxSettleRatePerSec), mParams->maxSettleRatePerSec);
		mapper.mapValue(offsetof(Params, maxRotationRatePerSec), mParams->maxRotationRatePerSec);

		mapper.mapValue(offsetof(Params, inAirRotationMultiplier), mParams->inAirRotationMultiplier);
		mapper.mapValue(offsetof(Params, collisionRotationMultiplier), mParams->collisionRotationMultiplier);

		mapper.mapValue(offsetof(Params, includeVerticalDirection), mParams->includeVerticalDirection);

		mapper.endParams();
	}

	virtual updateMeshFunc getUpdateMeshFunc(ModifierStage stage) const;

private:
	RotationModifierParams* mParams;
	NxApexMeshParticleRollType::Enum mRollType;
	physx::PxI32 mRollAxis;
	physx::PxF32 mRollSign;
	physx::PxF32 mLastUpdateTime;
};

// ------------------------------------------------------------------------------------------------
class SimpleScaleModifier : public NxSimpleScaleModifier, public Modifier, public physx::UserAllocated
{
public:
	SimpleScaleModifier(SimpleScaleModifierParams* params);

	virtual physx::PxU32 getModifierSpriteSemantics()
	{
		return (physx::PxU32)(1 << NxRenderSpriteSemantic::SCALE);
	}

	virtual physx::PxVec3 getScaleFactor() const
	{
		return mParams->scaleFactor;
	}
	virtual void setScaleFactor(const physx::PxVec3& s)
	{
		mParams->scaleFactor = s;
	}

#if defined(APEX_CUDA_SUPPORT)
	virtual void mapParamsGPU(ModifierParamsMapperGPU& mapper) const;
#endif
	virtual void mapParamsCPU(ModifierParamsMapperCPU& mapper) const;

	template <class Mapper, typename Params>
	void mapParams(Mapper& mapper, Params* params) const
	{
		mapper.beginParams(params, sizeof(Params), __alignof(Params), Params::RANDOM_COUNT);

		mapper.mapValue(offsetof(Params, scaleFactor), mParams->scaleFactor);

		mapper.endParams();
	}

	virtual updateSpriteFunc getUpdateSpriteFunc(ModifierStage stage) const;
	virtual updateMeshFunc getUpdateMeshFunc(ModifierStage stage) const;

private:
	SimpleScaleModifierParams* mParams;

};

// ------------------------------------------------------------------------------------------------
class ScaleByMassModifier : public NxScaleByMassModifier, public Modifier, public physx::UserAllocated
{
public:
	ScaleByMassModifier(ScaleByMassModifierParams* params);

	virtual physx::PxU32 getModifierSpriteSemantics()
	{
		return (physx::PxU32)(1 << NxRenderSpriteSemantic::SCALE);
	}

#if defined(APEX_CUDA_SUPPORT)
	virtual void mapParamsGPU(ModifierParamsMapperGPU& mapper) const;
#endif
	virtual void mapParamsCPU(ModifierParamsMapperCPU& mapper) const;

	template <class Mapper, typename Params>
	void mapParams(Mapper& mapper, Params* params) const
	{
		mapper.beginParams(params, sizeof(Params), __alignof(Params), Params::RANDOM_COUNT);

		mapper.endParams();
	}

	virtual updateSpriteFunc getUpdateSpriteFunc(ModifierStage stage) const;
	virtual updateMeshFunc getUpdateMeshFunc(ModifierStage stage) const;

private:
	ScaleByMassModifierParams* mParams;

};

// ------------------------------------------------------------------------------------------------
class RandomScaleModifier : public NxRandomScaleModifier, public Modifier, public physx::UserAllocated
{
public:
	RandomScaleModifier(RandomScaleModifierParams* params);

	virtual physx::PxU32 getModifierSpriteSemantics()
	{
		return (physx::PxU32)(1 << NxRenderSpriteSemantic::SCALE);
	}

	virtual NxRange<physx::PxF32> getScaleFactor() const
	{
		NxRange<physx::PxF32> s;
		s.minimum = mParams->minScaleFactor;
		s.maximum = mParams->maxScaleFactor;
		return s;
	}

	virtual void setScaleFactor(const NxRange<physx::PxF32>& s)
	{
		mParams->minScaleFactor = s.minimum;
		mParams->maxScaleFactor = s.maximum;
	}

#if defined(APEX_CUDA_SUPPORT)
	virtual void mapParamsGPU(ModifierParamsMapperGPU& mapper) const;
#endif
	virtual void mapParamsCPU(ModifierParamsMapperCPU& mapper) const;

	template <class Mapper, typename Params>
	void mapParams(Mapper& mapper, Params* params) const
	{
		mapper.beginParams(params, sizeof(Params), __alignof(Params), Params::RANDOM_COUNT);

		mapper.mapValue(offsetof(Params, scaleFactorMin), mParams->minScaleFactor);
		mapper.mapValue(offsetof(Params, scaleFactorMax), mParams->maxScaleFactor);

		mapper.endParams();
	}

	virtual updateSpriteFunc getUpdateSpriteFunc(ModifierStage stage) const;
	virtual updateMeshFunc getUpdateMeshFunc(ModifierStage stage) const;

private:
	RandomScaleModifierParams* mParams;
};

// ------------------------------------------------------------------------------------------------
class ColorVsLifeModifier : public NxColorVsLifeModifier, public Modifier, public physx::UserAllocated
{
public:
	ColorVsLifeModifier(ColorVsLifeModifierParams* params);

	// Methods from NxModifier
	virtual physx::PxU32 getModifierSpriteSemantics()
	{
		return (physx::PxU32)(1 << NxRenderSpriteSemantic::COLOR);
	}
	virtual physx::PxU32 getModifierMeshSemantics()
	{
		return (physx::PxU32)(1 << NxRenderInstanceSemantic::COLOR);
	}

	// Access to expected data members
	virtual ColorChannel getColorChannel() const
	{
		return mColorChannel;
	}
	virtual void setColorChannel(ColorChannel colorChannel);

	virtual const NxCurve* getFunction() const
	{
		return static_cast<const NxCurve*>(&mCurveFunction);
	}
	virtual void setFunction(const NxCurve* f)
	{
		const CurveImpl* curve = static_cast<const CurveImpl*>(f);
		mCurveFunction = *curve;

		IofxModifierHelper::setNxCurve(f,
		                               mParams,
		                               (NxParamDynamicArrayStruct*)&mParams->controlPoints);
	}

#if defined(APEX_CUDA_SUPPORT)
	virtual void mapParamsGPU(ModifierParamsMapperGPU& mapper) const;
#endif
	virtual void mapParamsCPU(ModifierParamsMapperCPU& mapper) const;

	template <class Mapper, typename Params>
	void mapParams(Mapper& mapper, Params* params) const
	{
		mapper.beginParams(params, sizeof(Params), __alignof(Params), Params::RANDOM_COUNT);

		mapper.mapValue(offsetof(Params, channel), physx::PxU32(mColorChannel));
		mapper.mapCurve(offsetof(Params, curve), static_cast<const CurveImpl*>(&mCurveFunction));

		mapper.endParams();
	}

	virtual updateSpriteFunc getUpdateSpriteFunc(ModifierStage stage) const;
	virtual updateMeshFunc getUpdateMeshFunc(ModifierStage stage) const;

private:
	ColorVsLifeModifierParams* mParams;
	ColorChannel mColorChannel;
	CurveImpl mCurveFunction;
};

// ------------------------------------------------------------------------------------------------
class ColorVsDensityModifier : public NxColorVsDensityModifier, public Modifier, public physx::UserAllocated
{
public:
	ColorVsDensityModifier(ColorVsDensityModifierParams* params);

	// Methods from NxModifier
	virtual physx::PxU32 getModifierSpriteSemantics()
	{
		return (physx::PxU32)(1 << NxRenderSpriteSemantic::COLOR);
	}
	virtual physx::PxU32 getModifierMeshSemantics()
	{
		return (physx::PxU32)(1 << NxRenderInstanceSemantic::COLOR);
	}

	// Access to expected data members
	virtual ColorChannel getColorChannel() const
	{
		return mColorChannel;
	}
	virtual void setColorChannel(ColorChannel colorChannel);

	virtual const NxCurve* getFunction() const
	{
		return static_cast<const NxCurve*>(&mCurveFunction);
	}
	virtual void setFunction(const NxCurve* f)
	{
		const CurveImpl* curve = static_cast<const CurveImpl*>(f);
		mCurveFunction = *curve;

		IofxModifierHelper::setNxCurve(f,
		                               mParams,
		                               (NxParamDynamicArrayStruct*)&mParams->controlPoints);
	}

#if defined(APEX_CUDA_SUPPORT)
	virtual void mapParamsGPU(ModifierParamsMapperGPU& mapper) const;
#endif
	virtual void mapParamsCPU(ModifierParamsMapperCPU& mapper) const;

	template <class Mapper, typename Params>
	void mapParams(Mapper& mapper, Params* params) const
	{
		mapper.beginParams(params, sizeof(Params), __alignof(Params), Params::RANDOM_COUNT);

		mapper.mapValue(offsetof(Params, channel), physx::PxU32(mColorChannel));
		mapper.mapCurve(offsetof(Params, curve), static_cast<const CurveImpl*>(&mCurveFunction));

		mapper.endParams();
	}

	virtual updateSpriteFunc getUpdateSpriteFunc(ModifierStage stage) const;
	virtual updateMeshFunc getUpdateMeshFunc(ModifierStage stage) const;

private:
	ColorVsDensityModifierParams* mParams;
	ColorChannel mColorChannel;
	CurveImpl mCurveFunction;
};

// ------------------------------------------------------------------------------------------------
class ColorVsVelocityModifier : public NxColorVsVelocityModifier, public Modifier, public physx::UserAllocated
{
public:
	ColorVsVelocityModifier(ColorVsVelocityModifierParams* params);

	// Methods from NxModifier
	virtual physx::PxU32 getModifierSpriteSemantics()
	{
		return (physx::PxU32)(1 << NxRenderSpriteSemantic::COLOR);
	}
	virtual physx::PxU32 getModifierMeshSemantics()
	{
		return (physx::PxU32)(1 << NxRenderInstanceSemantic::COLOR);
	}

	// Access to expected data members
	virtual ColorChannel getColorChannel() const
	{
		return mColorChannel;
	}
	virtual void setColorChannel(ColorChannel colorChannel);

	virtual const NxCurve* getFunction() const
	{
		return static_cast<const NxCurve*>(&mCurveFunction);
	}
	virtual void setFunction(const NxCurve* f)
	{
		const CurveImpl* curve = static_cast<const CurveImpl*>(f);
		mCurveFunction = *curve;

		IofxModifierHelper::setNxCurve(f,
		                               mParams,
		                               (NxParamDynamicArrayStruct*)&mParams->controlPoints);
	}

	virtual physx::PxF32 getVelocity0() const
	{
		return mParams->velocity0;
	}
	virtual void setVelocity0(physx::PxF32 value)
	{
		mParams->velocity0 = value;
	}

	virtual physx::PxF32 getVelocity1() const
	{
		return mParams->velocity1;
	}
	virtual void setVelocity1(physx::PxF32 value)
	{
		mParams->velocity1 = value;
	}

#if defined(APEX_CUDA_SUPPORT)
	virtual void mapParamsGPU(ModifierParamsMapperGPU& mapper) const;
#endif
	virtual void mapParamsCPU(ModifierParamsMapperCPU& mapper) const;

	template <class Mapper, typename Params>
	void mapParams(Mapper& mapper, Params* params) const
	{
		mapper.beginParams(params, sizeof(Params), __alignof(Params), Params::RANDOM_COUNT);

		mapper.mapValue(offsetof(Params, velocity0), mParams->velocity0);
		mapper.mapValue(offsetof(Params, velocity1), mParams->velocity1);
		mapper.mapValue(offsetof(Params, channel), physx::PxU32(mColorChannel));
		mapper.mapCurve(offsetof(Params, curve), static_cast<const CurveImpl*>(&mCurveFunction));

		mapper.endParams();
	}

	virtual updateSpriteFunc getUpdateSpriteFunc(ModifierStage stage) const;
	virtual updateMeshFunc getUpdateMeshFunc(ModifierStage stage) const;

private:
	ColorVsVelocityModifierParams* mParams;
	ColorChannel mColorChannel;
	CurveImpl mCurveFunction;
};

// ------------------------------------------------------------------------------------------------
class SubtextureVsLifeModifier : public NxSubtextureVsLifeModifier, public Modifier, public physx::UserAllocated
{
public:
	SubtextureVsLifeModifier(SubtextureVsLifeModifierParams* params);
	// Methods from NxModifier
	virtual physx::PxU32 getModifierSpriteSemantics()
	{
		return (physx::PxU32)(1 << NxRenderSpriteSemantic::SUBTEXTURE);
	}

	// Access to expected data members
	virtual const NxCurve* getFunction() const
	{
		return static_cast<const NxCurve*>(&mCurveFunction);
	}
	virtual void setFunction(const NxCurve* f)
	{
		const CurveImpl* curve = static_cast<const CurveImpl*>(f);
		mCurveFunction = *curve;

		IofxModifierHelper::setNxCurve(f,
		                               mParams,
		                               (NxParamDynamicArrayStruct*)&mParams->controlPoints);
	}

#if defined(APEX_CUDA_SUPPORT)
	virtual void mapParamsGPU(ModifierParamsMapperGPU& mapper) const;
#endif
	virtual void mapParamsCPU(ModifierParamsMapperCPU& mapper) const;

	template <class Mapper, typename Params>
	void mapParams(Mapper& mapper, Params* params) const
	{
		mapper.beginParams(params, sizeof(Params), __alignof(Params), Params::RANDOM_COUNT);

		mapper.mapCurve(offsetof(Params, curve), static_cast<const CurveImpl*>(&mCurveFunction));

		mapper.endParams();
	}

	virtual updateSpriteFunc getUpdateSpriteFunc(ModifierStage stage) const;

private:
	SubtextureVsLifeModifierParams* mParams;
	CurveImpl mCurveFunction;
};

// ------------------------------------------------------------------------------------------------
class OrientAlongVelocityModifier : public NxOrientAlongVelocityModifier, public Modifier, public physx::UserAllocated
{
public:
	OrientAlongVelocityModifier(OrientAlongVelocityModifierParams* params);

	// Methods from NxModifier

	// Access to expected data members
	virtual physx::PxVec3 getModelForward() const
	{
		return mParams->modelForward;
	}
	virtual void setModelForward(const physx::PxVec3& s)
	{
		mParams->modelForward = s;
	}

#if defined(APEX_CUDA_SUPPORT)
	virtual void mapParamsGPU(ModifierParamsMapperGPU& mapper) const;
#endif
	virtual void mapParamsCPU(ModifierParamsMapperCPU& mapper) const;

	template <class Mapper, typename Params>
	void mapParams(Mapper& mapper, Params* params) const
	{
		mapper.beginParams(params, sizeof(Params), __alignof(Params), Params::RANDOM_COUNT);

		mapper.mapValue(offsetof(Params, modelForward), mParams->modelForward);

		mapper.endParams();
	}

	virtual updateMeshFunc getUpdateMeshFunc(ModifierStage stage) const;

private:
	OrientAlongVelocityModifierParams* mParams;
};

// ------------------------------------------------------------------------------------------------
class ScaleAlongVelocityModifier : public NxScaleAlongVelocityModifier, public Modifier, public physx::UserAllocated
{
public:
	ScaleAlongVelocityModifier(ScaleAlongVelocityModifierParams* params);

	// Methods from NxModifier

	// Access to expected data members
	virtual physx::PxF32 getScaleFactor() const
	{
		return mParams->scaleFactor;
	}
	virtual void setScaleFactor(const physx::PxF32& s)
	{
		mParams->scaleFactor = s;
	}

#if defined(APEX_CUDA_SUPPORT)
	virtual void mapParamsGPU(ModifierParamsMapperGPU& mapper) const;
#endif
	virtual void mapParamsCPU(ModifierParamsMapperCPU& mapper) const;

	template <class Mapper, typename Params>
	void mapParams(Mapper& mapper, Params* params) const
	{
		mapper.beginParams(params, sizeof(Params), __alignof(Params), Params::RANDOM_COUNT);

		mapper.mapValue(offsetof(Params, scaleFactor), mParams->scaleFactor);

		mapper.endParams();
	}

	virtual updateMeshFunc getUpdateMeshFunc(ModifierStage stage) const;

private:
	ScaleAlongVelocityModifierParams* mParams;
};

// ------------------------------------------------------------------------------------------------
class RandomSubtextureModifier : public NxRandomSubtextureModifier, public Modifier, public physx::UserAllocated
{
public:
	RandomSubtextureModifier(RandomSubtextureModifierParams* params);

	// Methods from NxModifier
	virtual physx::PxU32 getModifierSpriteSemantics()
	{
		return (physx::PxU32)(1 << NxRenderSpriteSemantic::SUBTEXTURE);
	}

	// Access to expected data members
	virtual NxRange<physx::PxF32> getSubtextureRange() const
	{
		NxRange<physx::PxF32> s;
		s.minimum = mParams->minSubtexture;
		s.maximum = mParams->maxSubtexture;
		return s;
	}

	virtual void setSubtextureRange(const NxRange<physx::PxF32>& s)
	{
		mParams->minSubtexture = s.minimum;
		mParams->maxSubtexture = s.maximum;
	}

#if defined(APEX_CUDA_SUPPORT)
	virtual void mapParamsGPU(ModifierParamsMapperGPU& mapper) const;
#endif
	virtual void mapParamsCPU(ModifierParamsMapperCPU& mapper) const;

	template <class Mapper, typename Params>
	void mapParams(Mapper& mapper, Params* params) const
	{
		mapper.beginParams(params, sizeof(Params), __alignof(Params), Params::RANDOM_COUNT);

		mapper.mapValue(offsetof(Params, subtextureRangeMin), mParams->minSubtexture);
		mapper.mapValue(offsetof(Params, subtextureRangeMax), mParams->maxSubtexture);

		mapper.endParams();
	}

	virtual updateSpriteFunc getUpdateSpriteFunc(ModifierStage stage) const;

private:
	RandomSubtextureModifierParams* mParams;
};

// ------------------------------------------------------------------------------------------------
class RandomRotationModifier : public NxRandomRotationModifier, public Modifier, public physx::UserAllocated
{
public:
	RandomRotationModifier(RandomRotationModifierParams* params);

	// Methods from NxModifier
	virtual physx::PxU32 getModifierSpriteSemantics()
	{
		return (physx::PxU32)(1 << NxRenderSpriteSemantic::ORIENTATION);
	}

	// Access to expected data members
	virtual NxRange<physx::PxF32> getRotationRange() const
	{
		NxRange<physx::PxF32> s;
		s.minimum = mParams->minRotation;
		s.maximum = mParams->maxRotation;
		return s;
	}

	virtual void setRotationRange(const NxRange<physx::PxF32>& s)
	{
		mParams->minRotation = s.minimum;
		mParams->maxRotation = s.maximum;
	}

#if defined(APEX_CUDA_SUPPORT)
	virtual void mapParamsGPU(ModifierParamsMapperGPU& mapper) const;
#endif
	virtual void mapParamsCPU(ModifierParamsMapperCPU& mapper) const;

	template <class Mapper, typename Params>
	void mapParams(Mapper& mapper, Params* params) const
	{
		mapper.beginParams(params, sizeof(Params), __alignof(Params), Params::RANDOM_COUNT);

		mapper.mapValue(offsetof(Params, rotationRangeMin), mParams->minRotation);
		mapper.mapValue(offsetof(Params, rotationRangeMax), mParams->maxRotation);

		mapper.endParams();
	}

	virtual updateSpriteFunc getUpdateSpriteFunc(ModifierStage stage) const;

private:
	RandomRotationModifierParams* mParams;
};

// ------------------------------------------------------------------------------------------------
class ScaleVsLifeModifier : public NxScaleVsLifeModifier, public Modifier, public physx::UserAllocated
{
public:
	ScaleVsLifeModifier(ScaleVsLifeModifierParams* params);

	// Methods from NxModifier
	virtual physx::PxU32 getModifierSpriteSemantics()
	{
		return (physx::PxU32)(1 << NxRenderSpriteSemantic::SCALE);
	}

	// Access to expected data members
	virtual ScaleAxis getScaleAxis() const
	{
		return mScaleAxis;
	}
	virtual void setScaleAxis(ScaleAxis a);
	virtual const NxCurve* getFunction() const
	{
		return static_cast<const NxCurve*>(&mCurveFunction);
	}
	virtual void setFunction(const NxCurve* f)
	{
		const CurveImpl* curve = static_cast<const CurveImpl*>(f);
		mCurveFunction = *curve;

		IofxModifierHelper::setNxCurve(f,
		                               mParams,
		                               (NxParamDynamicArrayStruct*)&mParams->controlPoints);
	}

#if defined(APEX_CUDA_SUPPORT)
	virtual void mapParamsGPU(ModifierParamsMapperGPU& mapper) const;
#endif
	virtual void mapParamsCPU(ModifierParamsMapperCPU& mapper) const;

	template <class Mapper, typename Params>
	void mapParams(Mapper& mapper, Params* params) const
	{
		mapper.beginParams(params, sizeof(Params), __alignof(Params), Params::RANDOM_COUNT);

		mapper.mapValue(offsetof(Params, axis), physx::PxU32(mScaleAxis));
		mapper.mapCurve(offsetof(Params, curve), static_cast<const CurveImpl*>(&mCurveFunction));

		mapper.endParams();
	}

	virtual updateSpriteFunc getUpdateSpriteFunc(ModifierStage stage) const;
	virtual updateMeshFunc getUpdateMeshFunc(ModifierStage stage) const;

private:
	ScaleVsLifeModifierParams* mParams;
	ScaleAxis mScaleAxis;
	CurveImpl mCurveFunction;
};

// ------------------------------------------------------------------------------------------------
class ScaleVsDensityModifier : public NxScaleVsDensityModifier, public Modifier, public physx::UserAllocated
{
public:
	ScaleVsDensityModifier(ScaleVsDensityModifierParams* params);

	// Methods from NxModifier
	virtual physx::PxU32 getModifierSpriteSemantics()
	{
		return (physx::PxU32)(1 << NxRenderSpriteSemantic::SCALE);
	}

	// Access to expected data members
	virtual ScaleAxis getScaleAxis() const
	{
		return mScaleAxis;
	}
	virtual void setScaleAxis(ScaleAxis a);
	virtual const NxCurve* getFunction() const
	{
		return static_cast<const NxCurve*>(&mCurveFunction);
	}
	virtual void setFunction(const NxCurve* f)
	{
		const CurveImpl* curve = static_cast<const CurveImpl*>(f);
		mCurveFunction = *curve;

		IofxModifierHelper::setNxCurve(f,
		                               mParams,
		                               (NxParamDynamicArrayStruct*)&mParams->controlPoints);
	}

#if defined(APEX_CUDA_SUPPORT)
	virtual void mapParamsGPU(ModifierParamsMapperGPU& mapper) const;
#endif
	virtual void mapParamsCPU(ModifierParamsMapperCPU& mapper) const;

	template <class Mapper, typename Params>
	void mapParams(Mapper& mapper, Params* params) const
	{
		mapper.beginParams(params, sizeof(Params), __alignof(Params), Params::RANDOM_COUNT);

		mapper.mapValue(offsetof(Params, axis), physx::PxU32(mScaleAxis));
		mapper.mapCurve(offsetof(Params, curve), static_cast<const CurveImpl*>(&mCurveFunction));

		mapper.endParams();
	}

	virtual updateSpriteFunc getUpdateSpriteFunc(ModifierStage stage) const;
	virtual updateMeshFunc getUpdateMeshFunc(ModifierStage stage) const;

private:
	ScaleVsDensityModifierParams* mParams;
	ScaleAxis mScaleAxis;
	CurveImpl mCurveFunction;
};

// ------------------------------------------------------------------------------------------------
class ScaleVsCameraDistanceModifier : public NxScaleVsCameraDistanceModifier, public Modifier, public physx::UserAllocated
{
public:
	ScaleVsCameraDistanceModifier(ScaleVsCameraDistanceModifierParams* params);

	// Methods from NxModifier

	// Access to expected data members
	virtual ScaleAxis getScaleAxis() const
	{
		return mScaleAxis;
	}
	virtual void setScaleAxis(ScaleAxis a);
	virtual const NxCurve* getFunction() const
	{
		return static_cast<const NxCurve*>(&mCurveFunction);
	}
	virtual void setFunction(const NxCurve* f)
	{
		const CurveImpl* curve = static_cast<const CurveImpl*>(f);
		mCurveFunction = *curve;

		IofxModifierHelper::setNxCurve(f,
		                               mParams,
		                               (NxParamDynamicArrayStruct*)&mParams->controlPoints);
	}

#if defined(APEX_CUDA_SUPPORT)
	virtual void mapParamsGPU(ModifierParamsMapperGPU& mapper) const;
#endif
	virtual void mapParamsCPU(ModifierParamsMapperCPU& mapper) const;

	template <class Mapper, typename Params>
	void mapParams(Mapper& mapper, Params* params) const
	{
		mapper.beginParams(params, sizeof(Params), __alignof(Params), Params::RANDOM_COUNT);

		mapper.mapValue(offsetof(Params, axis), physx::PxU32(mScaleAxis));
		mapper.mapCurve(offsetof(Params, curve), static_cast<const CurveImpl*>(&mCurveFunction));

		mapper.endParams();
	}

	virtual updateSpriteFunc getUpdateSpriteFunc(ModifierStage stage) const;
	virtual updateMeshFunc getUpdateMeshFunc(ModifierStage stage) const;

private:
	ScaleVsCameraDistanceModifierParams* mParams;
	ScaleAxis mScaleAxis;
	CurveImpl mCurveFunction;
};

/**
	NxParameterized::Factory for modifiers. TODO: This should be a class that you instantiate, which you can then register objects with,
	and that you then pass to the Asset class for deserialization.
*/
NxModifier* CreateModifier(ModifierTypeEnum modifierType, NxParameterized::Interface* objParam, NxParameterized::Handle& h);

// ------------------------------------------------------------------------------------------------
class ViewDirectionSortingModifier : public NxViewDirectionSortingModifier, public Modifier, public physx::UserAllocated
{
public:
	ViewDirectionSortingModifier(ViewDirectionSortingModifierParams* params);

	// Methods from NxModifier
	virtual physx::PxU32 getModifierSpriteSemantics()
	{
		return (physx::PxU32)0;
	}

	// Access to expected data members

#if defined(APEX_CUDA_SUPPORT)
	virtual void mapParamsGPU(ModifierParamsMapperGPU& mapper) const;
#endif
	virtual void mapParamsCPU(ModifierParamsMapperCPU& mapper) const;

	template <class Mapper, typename Params>
	void mapParams(Mapper& mapper, Params* params) const
	{
		mapper.beginParams(params, sizeof(Params), __alignof(Params), Params::RANDOM_COUNT);

		mapper.endParams();
	}

	virtual updateSpriteFunc getUpdateSpriteFunc(ModifierStage stage) const;

private:
	ViewDirectionSortingModifierParams* mParams;
};

// ------------------------------------------------------------------------------------------------
class RotationRateModifier : public NxRotationRateModifier, public Modifier, public physx::UserAllocated
{
public:
	RotationRateModifier(RotationRateModifierParams* params);
	// Methods from NxModifier
	virtual physx::PxU32 getModifierSpriteSemantics()
	{
		return (physx::PxU32)(1 << NxRenderSpriteSemantic::ORIENTATION);
	}

	// Access to expected data members
	virtual physx::PxF32 getRotationRate() const
	{
		return mParams->rotationRate;
	}
	virtual void setRotationRate(const physx::PxF32& r)
	{
		mParams->rotationRate = r;
	}

#if defined(APEX_CUDA_SUPPORT)
	virtual void mapParamsGPU(ModifierParamsMapperGPU& mapper) const;
#endif
	virtual void mapParamsCPU(ModifierParamsMapperCPU& mapper) const;

	template <class Mapper, typename Params>
	void mapParams(Mapper& mapper, Params* params) const
	{
		mapper.beginParams(params, sizeof(Params), __alignof(Params), Params::RANDOM_COUNT);

		mapper.mapValue(offsetof(Params, rotationRate), mParams->rotationRate);

		mapper.endParams();
	}

	virtual updateSpriteFunc getUpdateSpriteFunc(ModifierStage stage) const;

private:
	RotationRateModifierParams* mParams;
};

// ------------------------------------------------------------------------------------------------
class RotationRateVsLifeModifier : public NxRotationRateVsLifeModifier, public Modifier, public physx::UserAllocated
{
public:
	RotationRateVsLifeModifier(RotationRateVsLifeModifierParams* params);
	// Methods from NxModifier
	virtual physx::PxU32 getModifierSpriteSemantics()
	{
		return (physx::PxU32)(1 << NxRenderSpriteSemantic::ORIENTATION);
	}

	// Access to expected data members
	virtual const NxCurve* getFunction() const
	{
		return static_cast<const NxCurve*>(&mCurveFunction);
	}
	virtual void setFunction(const NxCurve* f)
	{
		const CurveImpl* curve = static_cast<const CurveImpl*>(f);
		mCurveFunction = *curve;

		IofxModifierHelper::setNxCurve(f,
		                               mParams,
		                               (NxParamDynamicArrayStruct*)&mParams->controlPoints);
	}

#if defined(APEX_CUDA_SUPPORT)
	virtual void mapParamsGPU(ModifierParamsMapperGPU& mapper) const;
#endif
	virtual void mapParamsCPU(ModifierParamsMapperCPU& mapper) const;

	template <class Mapper, typename Params>
	void mapParams(Mapper& mapper, Params* params) const
	{
		mapper.beginParams(params, sizeof(Params), __alignof(Params), Params::RANDOM_COUNT);

		mapper.mapCurve(offsetof(Params, curve), static_cast<const CurveImpl*>(&mCurveFunction));

		mapper.endParams();
	}

	virtual updateSpriteFunc getUpdateSpriteFunc(ModifierStage stage) const;

private:
	RotationRateVsLifeModifierParams* mParams;
	CurveImpl mCurveFunction;
};

// ------------------------------------------------------------------------------------------------
class OrientScaleAlongScreenVelocityModifier : public NxOrientScaleAlongScreenVelocityModifier, public Modifier, public physx::UserAllocated
{
public:
	OrientScaleAlongScreenVelocityModifier(OrientScaleAlongScreenVelocityModifierParams* params);
	// Methods from NxModifier
	virtual physx::PxU32 getModifierSpriteSemantics()
	{
		return (physx::PxU32)((1 << NxRenderSpriteSemantic::ORIENTATION) | (1 << NxRenderSpriteSemantic::SCALE));
	}

	// Access to expected data members
	virtual physx::PxF32 getScalePerVelocity() const
	{
		return mParams->scalePerVelocity;
	}
	virtual void setScalePerVelocity(const physx::PxF32& s)
	{
		mParams->scalePerVelocity = s;
	}

#if defined(APEX_CUDA_SUPPORT)
	virtual void mapParamsGPU(ModifierParamsMapperGPU& mapper) const;
#endif
	virtual void mapParamsCPU(ModifierParamsMapperCPU& mapper) const;

	template <class Mapper, typename Params>
	void mapParams(Mapper& mapper, Params* params) const
	{
		mapper.beginParams(params, sizeof(Params), __alignof(Params), Params::RANDOM_COUNT);

		mapper.mapValue(offsetof(Params, scalePerVelocity), mParams->scalePerVelocity);

		mapper.endParams();
	}

	virtual updateSpriteFunc getUpdateSpriteFunc(ModifierStage stage) const;

private:
	OrientScaleAlongScreenVelocityModifierParams* mParams;
};

#pragma warning( default: 4100 )

}
}
} // namespace apex

#endif /* __MODIFIER_H__ */
