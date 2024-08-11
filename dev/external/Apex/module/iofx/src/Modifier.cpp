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


#include "Modifier.h"

#include "ApexSDKHelpers.h"
#include "ApexSharedUtils.h"
//#include "ApexSharedSerialization.h"
#include "NiInstancedObjectSimulation.h"
#include "NxIofxActor.h"
#include "NxParamArray.h"
#include "IofxAsset.h"
#include "NxApexUtils.h"


namespace physx
{
namespace apex
{
namespace iofx
{

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
RotationModifier::RotationModifier(RotationModifierParams* params) :
	mParams(params),
	mRollType(NxApexMeshParticleRollType::SPHERICAL),
	mRollAxis(0),
	mRollSign(0),
	mLastUpdateTime(0.0f)
{
	NxParameterized::Handle h(*mParams);
	mParams->getParameterHandle("rollType", h);
	setRollType((NxApexMeshParticleRollType::Enum) h.parameterDefinition()->enumValIndex(mParams->rollType));
}

// ------------------------------------------------------------------------------------------------
void RotationModifier::setRollType(NxApexMeshParticleRollType::Enum rollType)
{
	PX_ASSERT(rollType < NxApexMeshParticleRollType::COUNT);
	mRollType = rollType;

	NxParameterized::Handle h(*mParams);
	mParams->getParameterHandle("rollType", h);
	mParams->setParamEnum(h, h.parameterDefinition()->enumVal((int)rollType));

	switch (mRollType)
	{
	default:
		mRollSign = 1.0f;
		mRollAxis = -1;
		break;
	case NxApexMeshParticleRollType::FLAT_X:
		mRollSign = 1.0f;
		mRollAxis = 0;
		break;
	case NxApexMeshParticleRollType::FLAT_Y:
		mRollSign = 1.0f;
		mRollAxis = 1;
		break;
	case NxApexMeshParticleRollType::FLAT_Z:
		mRollSign = 1.0f;
		mRollAxis = 2;
		break;
	case NxApexMeshParticleRollType::LONG_X:
		mRollSign = -1.0f;
		mRollAxis = 0;
		break;
	case NxApexMeshParticleRollType::LONG_Y:
		mRollSign = -1.0f;
		mRollAxis = 1;
		break;
	case NxApexMeshParticleRollType::LONG_Z:
		mRollSign = -1.0f;
		mRollAxis = 2;
		break;
	}
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
SimpleScaleModifier::SimpleScaleModifier(SimpleScaleModifierParams* params) :
	mParams(params)
{ }


// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
RandomScaleModifier::RandomScaleModifier(RandomScaleModifierParams* param) :
	mParams(param)
{ }


// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
ScaleByMassModifier::ScaleByMassModifier(ScaleByMassModifierParams* params) :
	mParams(params)
{ }


// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
ColorVsLifeModifier::ColorVsLifeModifier(ColorVsLifeModifierParams* params) :
	mParams(params)
{
	NxParameterized::Handle h(*mParams);
	mParams->getParameterHandle("colorChannel", h);

	mColorChannel = (ColorChannel)(h.parameterDefinition()->enumValIndex(mParams->colorChannel));

	//do this in the NxCurve constructor... (for inplace stuff)
	NxParamArray<NxVec2R> cp(mParams, "controlPoints", (NxParamDynamicArrayStruct*)&mParams->controlPoints);
	for (physx::PxU32 i = 0; i < cp.size(); i++)
	{
		mCurveFunction.addControlPoint(cp[i]);
	}
}

// ------------------------------------------------------------------------------------------------

void ColorVsLifeModifier::setColorChannel(ColorChannel colorChannel)
{
	mColorChannel = colorChannel;

	NxParameterized::Handle h(*mParams);
	mParams->getParameterHandle("colorChannel", h);
	mParams->setParamEnum(h, h.parameterDefinition()->enumVal((int)colorChannel));
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
ColorVsDensityModifier::ColorVsDensityModifier(ColorVsDensityModifierParams* params) :
	mParams(params)
{
	NxParameterized::Handle h(*mParams);
	mParams->getParameterHandle("colorChannel", h);

	mColorChannel = (ColorChannel)(h.parameterDefinition()->enumValIndex(mParams->colorChannel));

	//do this in the NxCurve constructor... (for inplace stuff)
	NxParamArray<NxVec2R> cp(mParams, "controlPoints", (NxParamDynamicArrayStruct*)&mParams->controlPoints);
	for (physx::PxU32 i = 0; i < cp.size(); i++)
	{
		mCurveFunction.addControlPoint(cp[i]);
	}
}

// ------------------------------------------------------------------------------------------------

void ColorVsDensityModifier::setColorChannel(ColorChannel colorChannel)
{
	mColorChannel = colorChannel;

	NxParameterized::Handle h(*mParams);
	mParams->getParameterHandle("colorChannel", h);
	mParams->setParamEnum(h, h.parameterDefinition()->enumVal((int)colorChannel));
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
ColorVsVelocityModifier::ColorVsVelocityModifier(ColorVsVelocityModifierParams* params) :
	mParams(params)
{
	NxParameterized::Handle h(*mParams);
	mParams->getParameterHandle("colorChannel", h);

	mColorChannel = (ColorChannel)(h.parameterDefinition()->enumValIndex(mParams->colorChannel));

	//do this in the NxCurve constructor... (for inplace stuff)
	NxParamArray<NxVec2R> cp(mParams, "controlPoints", (NxParamDynamicArrayStruct*)&mParams->controlPoints);
	for (physx::PxU32 i = 0; i < cp.size(); i++)
	{
		mCurveFunction.addControlPoint(cp[i]);
	}
}

// ------------------------------------------------------------------------------------------------

void ColorVsVelocityModifier::setColorChannel(ColorChannel colorChannel)
{
	mColorChannel = colorChannel;

	NxParameterized::Handle h(*mParams);
	mParams->getParameterHandle("colorChannel", h);
	mParams->setParamEnum(h, h.parameterDefinition()->enumVal((int)colorChannel));
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
SubtextureVsLifeModifier::SubtextureVsLifeModifier(SubtextureVsLifeModifierParams* params) :
	mParams(params)
{
	//do this in the NxCurve constructor... (for inplace stuff)
	NxParamArray<NxVec2R> cp(mParams, "controlPoints", (NxParamDynamicArrayStruct*)&mParams->controlPoints);
	for (physx::PxU32 i = 0; i < cp.size(); i++)
	{
		mCurveFunction.addControlPoint(cp[i]);
	}
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
OrientAlongVelocityModifier::OrientAlongVelocityModifier(OrientAlongVelocityModifierParams* params) :
	mParams(params)
{ }

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
ScaleAlongVelocityModifier::ScaleAlongVelocityModifier(ScaleAlongVelocityModifierParams* params) :
	mParams(params)
{ }


// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
RandomSubtextureModifier::RandomSubtextureModifier(RandomSubtextureModifierParams* params) :
	mParams(params)
{}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
RandomRotationModifier::RandomRotationModifier(RandomRotationModifierParams* params) :
	mParams(params)
{}


// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
ScaleVsLifeModifier::ScaleVsLifeModifier(ScaleVsLifeModifierParams* params) :
	mParams(params)
{
	NxParameterized::Handle h(*mParams);
	mParams->getParameterHandle("scaleAxis", h);

	mScaleAxis = (ScaleAxis)(h.parameterDefinition()->enumValIndex(mParams->scaleAxis));

	//do this in the NxCurve constructor... (for inplace stuff)
	NxParamArray<NxVec2R> cp(mParams, "controlPoints", (NxParamDynamicArrayStruct*)&mParams->controlPoints);
	for (physx::PxU32 i = 0; i < cp.size(); i++)
	{
		mCurveFunction.addControlPoint(cp[i]);
	}
}

void ScaleVsLifeModifier::setScaleAxis(ScaleAxis a)
{
	mScaleAxis = a;

	NxParameterized::Handle h(*mParams);
	mParams->getParameterHandle("scaleAxis", h);
	mParams->setParamEnum(h, h.parameterDefinition()->enumVal((int)a));
}


// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
ScaleVsDensityModifier::ScaleVsDensityModifier(ScaleVsDensityModifierParams* params) :
	mParams(params)
{
	NxParameterized::Handle h(*mParams);
	mParams->getParameterHandle("scaleAxis", h);

	mScaleAxis = (ScaleAxis)(h.parameterDefinition()->enumValIndex(mParams->scaleAxis));

	//do this in the NxCurve constructor... (for inplace stuff)
	NxParamArray<NxVec2R> cp(mParams, "controlPoints", (NxParamDynamicArrayStruct*)&mParams->controlPoints);
	for (physx::PxU32 i = 0; i < cp.size(); i++)
	{
		mCurveFunction.addControlPoint(cp[i]);
	}
}

void ScaleVsDensityModifier::setScaleAxis(ScaleAxis a)
{
	mScaleAxis = a;

	NxParameterized::Handle h(*mParams);
	mParams->getParameterHandle("scaleAxis", h);
	mParams->setParamEnum(h, h.parameterDefinition()->enumVal((int)a));
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
ScaleVsCameraDistanceModifier::ScaleVsCameraDistanceModifier(ScaleVsCameraDistanceModifierParams* params) :
	mParams(params)
{
	NxParameterized::Handle h(*mParams);
	mParams->getParameterHandle("scaleAxis", h);

	mScaleAxis = (ScaleAxis)(h.parameterDefinition()->enumValIndex(mParams->scaleAxis));

	//do this in the NxCurve constructor... (for inplace stuff)
	NxParamArray<NxVec2R> cp(mParams, "controlPoints", (NxParamDynamicArrayStruct*)&mParams->controlPoints);
	for (physx::PxU32 i = 0; i < cp.size(); i++)
	{
		mCurveFunction.addControlPoint(cp[i]);
	}
}

void ScaleVsCameraDistanceModifier::setScaleAxis(ScaleAxis a)
{
	mScaleAxis = a;

	NxParameterized::Handle h(*mParams);
	mParams->getParameterHandle("scaleAxis", h);
	mParams->setParamEnum(h, h.parameterDefinition()->enumVal((int)a));
}



ViewDirectionSortingModifier::ViewDirectionSortingModifier(ViewDirectionSortingModifierParams* params)
	: mParams(params)
{
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
RotationRateModifier::RotationRateModifier(RotationRateModifierParams* params) :
	mParams(params)
{
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
RotationRateVsLifeModifier::RotationRateVsLifeModifier(RotationRateVsLifeModifierParams* params) :
	mParams(params)
{
	//do this in the NxCurve constructor... (for inplace stuff)
	NxParamArray<NxVec2R> cp(mParams, "controlPoints", (NxParamDynamicArrayStruct*)&mParams->controlPoints);
	for (physx::PxU32 i = 0; i < cp.size(); i++)
	{
		mCurveFunction.addControlPoint(cp[i]);
	}
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
OrientScaleAlongScreenVelocityModifier::OrientScaleAlongScreenVelocityModifier(OrientScaleAlongScreenVelocityModifierParams* params) :
	mParams(params)
{
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
NxModifier* CreateModifier(ModifierTypeEnum modifierType, NxParameterized::Interface* objParam, NxParameterized::Handle& h)
{
	PX_UNUSED(objParam);
	PX_ASSERT(objParam == h.getConstInterface());

#define _MODIFIER(Type) \
case ModifierType_##Type: \
	h.initParamRef(#Type "ModifierParams", true); \
	h.getParamRef(refParam); \
	return PX_NEW(Type##Modifier)((Type##ModifierParams*)refParam); \
	 
	NxParameterized::Interface* refParam = 0;

	// TODO: This should go to an actual factory which can be used to extend modifiers.
	switch (modifierType)
	{
#include "ModifierList.h"

	default:
		PX_ALWAYS_ASSERT();
	}
	return 0;
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------

const Modifier* Modifier::castFrom(const NxModifier* modifier)
{
#define _MODIFIER(Type) \
case ModifierType_##Type: \
	return static_cast<const Modifier*>( static_cast<const Type ## Modifier *>(modifier) ); \
	 
	ModifierTypeEnum modifierType = modifier->getModifierType();
	switch (modifierType)
	{
#include "ModifierList.h"

	default:
		PX_ALWAYS_ASSERT();
		return 0;
	}
}

}
}
} // namespace physx::apex
