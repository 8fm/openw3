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

#include "EffectPackageActor.h"
#include "FloatMath.h"
#include "ParticlesScene.h"
#include "NiApexScene.h"
#include "NxApexEmitterActor.h"
#include "NxApexEmitterAsset.h"
#include "NxTurbulenceFSActor.h"
#include "NxAttractorFSActor.h"
#include "NxJetFSActor.h"
#include "NxNoiseFSActor.h"
#include "NxVortexFSActor.h"
#include "NxParamUtils.h"
#include "NxImpactEmitterActor.h"
#include "NxGroundEmitterActor.h"
#include "NxTurbulenceFSAsset.h"
#include "NxBasicIosAsset.h"

#include "NxBasicFSAsset.h"
#include "PxRenderDebug.h"
#include "NiApexScene.h"
#include "NxForceFieldAsset.h"
#include "NxForceFieldActor.h"

#include "NxHeatSourceAsset.h"
#include "NxHeatSourceActor.h"
#include "HeatSourceAssetParams.h"

#include "ApexEmitterActorParameters.h"
#include "ApexEmitterAssetParameters.h"
#include "NxSubstanceSourceAsset.h"

#include "NxSubstanceSourceActor.h"
#include "NxSubstanceSourceAsset.h"
#include "SubstanceSourceAssetParams.h"
#include "TurbulenceFSAssetParams.h"

#pragma warning(disable:4100)

namespace physx
{
namespace apex
{
namespace particles
{

static PxTransform _getPose(physx::PxF32 x, physx::PxF32 y, physx::PxF32 z, physx::PxF32 rotX, physx::PxF32 rotY, physx::PxF32 rotZ)
{
	PxTransform ret;
	ret.p = PxVec3(x, y, z);
	fm_eulerToQuat(rotX * FM_DEG_TO_RAD, rotY * FM_DEG_TO_RAD, rotZ * FM_DEG_TO_RAD, &ret.q.x);
	return ret;
}

void _getRot(const PxQuat& q, PxF32& rotX, PxF32& rotY, PxF32& rotZ)
{
	fm_quatToEuler((PxF32*)&q, rotX, rotY, rotZ);
	rotX *= FM_RAD_TO_DEG;
	rotY *= FM_RAD_TO_DEG;
	rotZ *= FM_RAD_TO_DEG;
}

static PxF32 ranf(void)
{
	PxU32 r = (PxU32)::rand();
	r &= 0x7FFF;
	return (PxF32)r * (1.0f / 32768.0f);
}

static PxF32 ranf(PxF32 min, PxF32 max)
{
	return ranf() * (max - min) + min;
}

EffectPackageActor::EffectPackageActor(NxEffectPackageAsset* apexAsset,
                                       const EffectPackageAssetParams* assetParams,
                                       const EffectPackageActorParams* actorParams,
                                       physx::apex::NxApexSDK& sdk,
                                       physx::apex::NxApexScene& scene,
                                       ParticlesScene& dynamicSystemScene,
									   NxModuleTurbulenceFS* moduleTurbulenceFS)
{
	mEmitterValidateCallback = NULL;
	mAlive = true;
	mSimTime = 0;
	mCurrentLifeTime = 0;
	mFadeIn = false;
	mFadeInTime = 0;
	mFadeInDuration = 0;
	mFadeOut = false;
	mFadeOutTime = 0;
	mFadeOutDuration = 0;
	mFirstFrame = true;
	mData = assetParams;
	mAsset = apexAsset;
	mScene = &scene;
	mModuleTurbulenceFS = moduleTurbulenceFS;
	mEnabled = actorParams->Enabled;
	mVisible = false;
	mEverVisible = false;
	mOffScreenTime = 0;
	mVisState = VS_ON_SCREEN;
	mFadeTime = 0;
	mNotVisibleTime = 0;
	mPose = actorParams->InitialPose;

	for (physx::PxI32 i = 0; i < mData->Effects.arraySizes[0]; i++)
	{
		NxParameterized::Interface* iface = mData->Effects.buf[i];
		PX_ASSERT(iface);
		if (iface)
		{
			EffectType type = getEffectType(iface);
			switch (type)
			{
			case ET_EMITTER:
			{
				EmitterEffect* ed = static_cast< EmitterEffect*>(iface);
				if (ed->Emitter)
				{
					EffectEmitter* ee = PX_NEW(EffectEmitter)(ed->Emitter->name(), ed, sdk, scene, dynamicSystemScene, mPose, mEnabled);
					mEffects.pushBack(static_cast< EffectData*>(ee));
				}
			}
			break;
			case ET_HEAT_SOURCE:
			{
				HeatSourceEffect* ed = static_cast< HeatSourceEffect*>(iface);
				EffectHeatSource* ee = PX_NEW(EffectHeatSource)(ed->HeatSource->name(), ed, sdk, scene, dynamicSystemScene, moduleTurbulenceFS, mPose, mEnabled);
				mEffects.pushBack(static_cast< EffectData*>(ee));
			}
			break;
			case ET_SUBSTANCE_SOURCE:
				{
					SubstanceSourceEffect* ed = static_cast< SubstanceSourceEffect*>(iface);
					EffectSubstanceSource* ee = PX_NEW(EffectSubstanceSource)(ed->SubstanceSource->name(), ed, sdk, scene, dynamicSystemScene, moduleTurbulenceFS, mPose, mEnabled);
					mEffects.pushBack(static_cast< EffectData*>(ee));
				}
				break;
			case ET_TURBULENCE_FS:
			{
				TurbulenceFieldSamplerEffect* ed = static_cast< TurbulenceFieldSamplerEffect*>(iface);
				EffectTurbulenceFS* ee = PX_NEW(EffectTurbulenceFS)(ed->TurbulenceFieldSampler->name(), ed, sdk, scene, dynamicSystemScene, moduleTurbulenceFS, mPose, mEnabled);
				mEffects.pushBack(static_cast< EffectData*>(ee));
			}
			break;
			case ET_JET_FS:
			{
				JetFieldSamplerEffect* ed = static_cast< JetFieldSamplerEffect*>(iface);
				EffectJetFS* ee = PX_NEW(EffectJetFS)(ed->JetFieldSampler->name(), ed, sdk, scene, dynamicSystemScene, mPose, mEnabled);
				mEffects.pushBack(static_cast< EffectData*>(ee));
			}
			break;
			case ET_NOISE_FS:
			{
				NoiseFieldSamplerEffect* ed = static_cast< NoiseFieldSamplerEffect*>(iface);
				EffectNoiseFS* ee = PX_NEW(EffectNoiseFS)(ed->NoiseFieldSampler->name(), ed, sdk, scene, dynamicSystemScene, mPose, mEnabled);
				mEffects.pushBack(static_cast< EffectData*>(ee));
			}
			break;
			case ET_VORTEX_FS:
			{
				VortexFieldSamplerEffect* ed = static_cast< VortexFieldSamplerEffect*>(iface);
				EffectVortexFS* ee = PX_NEW(EffectVortexFS)(ed->VortexFieldSampler->name(), ed, sdk, scene, dynamicSystemScene, mPose, mEnabled);
				mEffects.pushBack(static_cast< EffectData*>(ee));
			}
			break;


			case ET_ATTRACTOR_FS:
			{
				AttractorFieldSamplerEffect* ed = static_cast< AttractorFieldSamplerEffect*>(iface);
				EffectAttractorFS* ee = PX_NEW(EffectAttractorFS)(ed->AttractorFieldSampler->name(), ed, sdk, scene, dynamicSystemScene, mPose, mEnabled);
				mEffects.pushBack(static_cast< EffectData*>(ee));
			}
			break;
			case ET_FORCE_FIELD:
			{
				ForceFieldEffect* ed = static_cast< ForceFieldEffect*>(iface);
				EffectForceField* ee = PX_NEW(EffectForceField)(ed->ForceField->name(), ed, sdk, scene, dynamicSystemScene, mPose, mEnabled);
				mEffects.pushBack(static_cast< EffectData*>(ee));
			}
			break;
			default:
				PX_ALWAYS_ASSERT();
				break;
			}

		}
	}
	addSelfToContext(*dynamicSystemScene.mApexScene->getApexContext());    // Add self to ApexScene
	addSelfToContext(dynamicSystemScene);	// Add self to ParticlesScene's list of actors
}

EffectPackageActor::~EffectPackageActor(void)
{
	for (PxU32 i = 0; i < mEffects.size(); i++)
	{
		EffectData* ed = mEffects[i];
		delete ed;
	}
}

EffectType EffectPackageActor::getEffectType(const NxParameterized::Interface* iface)
{
	EffectType ret = ET_LAST;

	if (strcmp(iface->className(), EmitterEffect::staticClassName()) == 0)
	{
		ret = ET_EMITTER;
	}
	else if (strcmp(iface->className(), HeatSourceEffect::staticClassName()) == 0)
	{
		ret = ET_HEAT_SOURCE;
	}
	else if (strcmp(iface->className(), SubstanceSourceEffect::staticClassName()) == 0)
	{
		ret = ET_SUBSTANCE_SOURCE;
	}
	else if (strcmp(iface->className(), ForceFieldEffect::staticClassName()) == 0)
	{
		ret = ET_FORCE_FIELD;
	}
	else if (strcmp(iface->className(), JetFieldSamplerEffect::staticClassName()) == 0)
	{
		ret = ET_JET_FS;
	}
	else if (strcmp(iface->className(), NoiseFieldSamplerEffect::staticClassName()) == 0)
	{
		ret = ET_NOISE_FS;
	}
	else if (strcmp(iface->className(), VortexFieldSamplerEffect::staticClassName()) == 0)
	{
		ret = ET_VORTEX_FS;
	}
	else if (strcmp(iface->className(), AttractorFieldSamplerEffect::staticClassName()) == 0)
	{
		ret = ET_ATTRACTOR_FS;
	}
	else if (strcmp(iface->className(), TurbulenceFieldSamplerEffect::staticClassName()) == 0)
	{
		ret = ET_TURBULENCE_FS;
	}
	else
	{
		PX_ALWAYS_ASSERT();
	}

	return ret;
}

const PxTransform& EffectPackageActor::getPose(void) const
{
	return mPose;
}

void EffectPackageActor::visualize(physx::general_renderdebug4::RenderDebug* callback, bool solid) const
{
	callback->pushRenderState();

	callback->addToCurrentState(DebugRenderState::CameraFacing);
	callback->addToCurrentState(DebugRenderState::CenterText);

	callback->debugText(mPose.p - PxVec3(0, 0.35f, 0), mAsset->getName());

	callback->debugAxes(PxMat44(mPose));

	for (physx::PxU32 i = 0; i < mEffects.size(); i++)
	{
		mEffects[i]->visualize(callback, solid);
	}

	callback->popRenderState();
}

void EffectPackageActor::setPose(const PxTransform& pose)
{
	mPose = pose;
	for (physx::PxU32 i = 0; i < mEffects.size(); i++)
	{
		mEffects[i]->refresh(mPose, mEnabled, true, mRenderVolume,mEmitterValidateCallback);
	}
}

void EffectPackageActor::refresh(void)
{
	for (physx::PxU32 i = 0; i < mEffects.size(); i++)
	{
		EffectData* ed = mEffects[i];
		if (ed->getEffectActor())
		{
			ed->refresh(mPose, mEnabled, true, mRenderVolume,mEmitterValidateCallback);
		}
	}
}

void EffectPackageActor::release(void)
{
	delete this;
}

const char* EffectPackageActor::getName(void) const
{
	return mData->name();
}


PxU32 EffectPackageActor::getEffectCount(void) const // returns the number of effects in the effect package
{
	return mEffects.size();
}

EffectType EffectPackageActor::getEffectType(PxU32 effectIndex) const // return the type of effect.
{
	EffectType ret = ET_LAST;

	if (effectIndex < mEffects.size())
	{
		EffectData* ed = mEffects[effectIndex];
		ret = ed->getType();
	}

	return ret;
}

NxApexActor* EffectPackageActor::getEffectActor(PxU32 effectIndex) const // return the base NxApexActor pointer
{
	NxApexActor* ret = NULL;

	if (effectIndex < mEffects.size())
	{
		EffectData* ed = mEffects[effectIndex];
		ret = ed->getEffectActor();
	}


	return ret;
}
void EffectPackageActor::setEmitterState(bool state) // set the state for all emitters in this effect package.
{
	for (PxU32 i = 0; i < mEffects.size(); i++)
	{
		if (mEffects[i]->getType() == ET_EMITTER)
		{
			NxApexActor* a = mEffects[i]->getEffectActor();
			NxApexEmitterActor* ae = static_cast< NxApexEmitterActor*>(a);
			if (state)
			{
				ae->startEmit(false);
			}
			else
			{
				ae->stopEmit();
			}
		}
	}
}

PxU32 EffectPackageActor::getActiveParticleCount(void) const // return the total number of particles still active in this effect package.
{
	PxU32 ret = 0;

	for (PxU32 i = 0; i < mEffects.size(); i++)
	{
		if (mEffects[i]->getType() == ET_EMITTER)
		{
			NxApexActor* a = mEffects[i]->getEffectActor();
			if (a)
			{
				NxApexEmitterActor* ae = static_cast< NxApexEmitterActor*>(a);
				ret += ae->getActiveParticleCount();
			}
		}
	}
	return ret;
}

bool EffectPackageActor::isStillEmitting(void) const // return true if any emitters are still actively emitting particles.
{
	bool ret = false;

	for (PxU32 i = 0; i < mEffects.size(); i++)
	{
		if (mEffects[i]->getType() == ET_EMITTER)
		{
			NxApexActor* a = mEffects[i]->getEffectActor();
			NxApexEmitterActor* ae = static_cast< NxApexEmitterActor*>(a);
			if (ae->isEmitting())
			{
				ret = true;
				break;
			}
		}
	}
	return ret;
}

// Effect class implementations

EffectEmitter::EffectEmitter(const char* parentName,
                             const EmitterEffect* data,
                             NxApexSDK& sdk,
                             NxApexScene& scene,
                             ParticlesScene& dscene,
                             const PxTransform& rootPose,
                             bool parentEnabled) : mData(data), EffectData(ET_EMITTER, &sdk,
	                                     &scene,
	                                     &dscene,
	                                     parentName,
	                                     NX_APEX_EMITTER_AUTHORING_TYPE_NAME,
	                                     data->EffectProperties.InitialDelayTime,
	                                     data->EffectProperties.Duration,
	                                     data->EffectProperties.RepeatCount,
	                                     data->EffectProperties.RepeatDelay,
	                                     data->EffectProperties.RandomizeRepeatTime,
	                                     data->EffectProperties.Enable,
	                                     _getPose(data->EffectProperties.Position.TranslateX,
	                                             data->EffectProperties.Position.TranslateY,
	                                             data->EffectProperties.Position.TranslateZ,
	                                             data->EffectProperties.Orientation.RotateX,
	                                             data->EffectProperties.Orientation.RotateY,
	                                             data->EffectProperties.Orientation.RotateZ))
{
	mEmitterVelocity = physx::PxVec3(0, 0, 0);
	mFirstVelocityFrame = true;
	mHaveSetPosition = false;
	mVelocityTime = 0;
	mLastEmitterPosition = physx::PxVec3(0, 0, 0);
}

EffectEmitter::~EffectEmitter(void)
{
}

EffectTurbulenceFS::EffectTurbulenceFS(const char* parentName,
                                       TurbulenceFieldSamplerEffect* data,
                                       NxApexSDK& sdk,
                                       NxApexScene& scene,
                                       ParticlesScene& dscene,
									   NxModuleTurbulenceFS* moduleTurbulenceFS,
                                       const PxTransform& rootPose,
                                       bool parentEnabled) :
	mData(data),
	mModuleTurbulenceFS(moduleTurbulenceFS),
	EffectData(ET_TURBULENCE_FS, &sdk, &scene, &dscene, parentName, NX_TURBULENCE_FS_AUTHORING_TYPE_NAME, data->EffectProperties.InitialDelayTime, data->EffectProperties.Duration, data->EffectProperties.RepeatCount, data->EffectProperties.RepeatDelay, data->EffectProperties.RandomizeRepeatTime, data->EffectProperties.Enable,
	           _getPose(data->EffectProperties.Position.TranslateX,
	                    data->EffectProperties.Position.TranslateY,
	                    data->EffectProperties.Position.TranslateZ,
	                    data->EffectProperties.Orientation.RotateX,
	                    data->EffectProperties.Orientation.RotateY,
	                    data->EffectProperties.Orientation.RotateZ))
{
}

EffectTurbulenceFS::~EffectTurbulenceFS(void)
{
}

EffectJetFS::EffectJetFS(const char* parentName,
                         JetFieldSamplerEffect* data,
                         NxApexSDK& sdk,
                         NxApexScene& scene,
                         ParticlesScene& dscene,
                         const PxTransform& rootPose,
                         bool parentEnabled) : mData(data), EffectData(ET_JET_FS, &sdk, &scene, &dscene, parentName, NX_JET_FS_AUTHORING_TYPE_NAME, data->EffectProperties.InitialDelayTime, data->EffectProperties.Duration, data->EffectProperties.RepeatCount, data->EffectProperties.RepeatDelay, data->EffectProperties.RandomizeRepeatTime, data->EffectProperties.Enable,
	                                 _getPose(data->EffectProperties.Position.TranslateX,
	                                         data->EffectProperties.Position.TranslateY,
	                                         data->EffectProperties.Position.TranslateZ,
	                                         data->EffectProperties.Orientation.RotateX,
	                                         data->EffectProperties.Orientation.RotateY,
	                                         data->EffectProperties.Orientation.RotateZ))
{
}

EffectJetFS::~EffectJetFS(void)
{
}

EffectAttractorFS::EffectAttractorFS(const char* parentName,
                                     AttractorFieldSamplerEffect* data,
                                     NxApexSDK& sdk,
                                     NxApexScene& scene,
                                     ParticlesScene& dscene,
                                     const PxTransform& rootPose,
                                     bool parentEnabled) : mData(data), EffectData(ET_ATTRACTOR_FS, &sdk, &scene, &dscene, parentName, NX_ATTRACTOR_FS_AUTHORING_TYPE_NAME, data->EffectProperties.InitialDelayTime, data->EffectProperties.Duration, data->EffectProperties.RepeatCount, data->EffectProperties.RepeatDelay, data->EffectProperties.RandomizeRepeatTime, data->EffectProperties.Enable,
	                                             _getPose(data->EffectProperties.Position.TranslateX,
	                                                     data->EffectProperties.Position.TranslateY,
	                                                     data->EffectProperties.Position.TranslateZ,
	                                                     data->EffectProperties.Orientation.RotateX,
	                                                     data->EffectProperties.Orientation.RotateY,
	                                                     data->EffectProperties.Orientation.RotateZ))
{
}

EffectAttractorFS::~EffectAttractorFS(void)
{
}

void EffectEmitter::visualize(physx::general_renderdebug4::RenderDebug* callback, bool solid) const
{
	callback->debugText(mPose.p, mData->Emitter->name());
	callback->debugAxes(PxMat44(mPose));

}



EffectHeatSource::EffectHeatSource(const char* parentName,
                                   HeatSourceEffect* data,
                                   NxApexSDK& sdk,
                                   NxApexScene& scene,
                                   ParticlesScene& dscene,
                                   NxModuleTurbulenceFS* moduleTurbulenceFS,
                                   const PxTransform& rootPose,
                                   bool parentEnabled) : mData(data),
	mModuleTurbulenceFS(moduleTurbulenceFS),
	EffectData(ET_HEAT_SOURCE, &sdk, &scene, &dscene, parentName, NX_HEAT_SOURCE_AUTHORING_TYPE_NAME, data->EffectProperties.InitialDelayTime, data->EffectProperties.Duration, data->EffectProperties.RepeatCount, data->EffectProperties.RepeatDelay, data->EffectProperties.RandomizeRepeatTime, data->EffectProperties.Enable,
	           _getPose(data->EffectProperties.Position.TranslateX,
	                    data->EffectProperties.Position.TranslateY,
	                    data->EffectProperties.Position.TranslateZ,
	                    data->EffectProperties.Orientation.RotateX,
	                    data->EffectProperties.Orientation.RotateY,
	                    data->EffectProperties.Orientation.RotateZ))
{
}

EffectHeatSource::~EffectHeatSource(void)
{
}

void EffectHeatSource::visualize(physx::general_renderdebug4::RenderDebug* callback, bool solid) const
{
}

void EffectHeatSource::refresh(const PxTransform& parent, bool parentEnabled, bool fromSetPose, NxApexRenderVolume* renderVolume,NxApexEmitterActor::NxApexEmitterValidateCallback *callback)
{
	if (parentEnabled && mEnabled && mAsset)
	{

		physx::PxMat44 myPose(parent * mLocalPose);

		if (mActor == NULL && mAsset && !fromSetPose)
		{
			NxParameterized::Interface* descParams = mAsset->getDefaultActorDesc();
			if (descParams)
			{
				PxTransform pose(myPose);
				bool ok = NxParameterized::setParamVec3(*descParams, "position", pose.p);
				PX_ASSERT(ok);
				PX_UNUSED(ok);

				mActor = mAsset->createApexActor(*descParams, *mApexScene);
			}
			const NxParameterized::Interface* iface = mAsset->getAssetNxParameterized();
			const turbulencefs::HeatSourceAssetParams* hap = static_cast< const turbulencefs::HeatSourceAssetParams*>(iface);
			mAverageTemperature = hap->averageTemperature;
			mStandardDeviationTemperature = hap->stdTemperature;
		}
		else if (mActor)
		{
			NxHeatSourceActor* a = static_cast< NxHeatSourceActor*>(mActor);
			physx::apex::NxApexShape *s = a->getShape();
			s->setPose(myPose);
		}
	}
	else
	{
		releaseActor();
	}
}

EffectSubstanceSource::EffectSubstanceSource(const char* parentName,
								   SubstanceSourceEffect* data,
								   NxApexSDK& sdk,
								   NxApexScene& scene,
								   ParticlesScene& dscene,
								   NxModuleTurbulenceFS* moduleTurbulenceFS,
								   const PxTransform& rootPose,
								   bool parentEnabled) : mData(data),
								   mModuleTurbulenceFS(moduleTurbulenceFS),
								   EffectData(ET_SUBSTANCE_SOURCE, &sdk, &scene, &dscene, parentName, NX_SUBSTANCE_SOURCE_AUTHORING_TYPE_NAME, data->EffectProperties.InitialDelayTime, data->EffectProperties.Duration, data->EffectProperties.RepeatCount, data->EffectProperties.RepeatDelay, data->EffectProperties.RandomizeRepeatTime, data->EffectProperties.Enable,
								   _getPose(data->EffectProperties.Position.TranslateX,
								   data->EffectProperties.Position.TranslateY,
								   data->EffectProperties.Position.TranslateZ,
								   data->EffectProperties.Orientation.RotateX,
								   data->EffectProperties.Orientation.RotateY,
								   data->EffectProperties.Orientation.RotateZ))
{
}

EffectSubstanceSource::~EffectSubstanceSource(void)
{
}

void EffectSubstanceSource::visualize(physx::general_renderdebug4::RenderDebug* callback, bool solid) const
{
}

void EffectSubstanceSource::refresh(const PxTransform& parent, bool parentEnabled, bool fromSetPose, NxApexRenderVolume* renderVolume,NxApexEmitterActor::NxApexEmitterValidateCallback *callback)
{
	if (parentEnabled && mEnabled && mAsset)
	{

		physx::PxMat44 myPose(parent * mLocalPose);

		if (mActor == NULL && mAsset && !fromSetPose)
		{
			NxParameterized::Interface* descParams = mAsset->getDefaultActorDesc();
			if (descParams)
			{
				PxTransform pose(myPose);
				bool ok = NxParameterized::setParamVec3(*descParams, "position", pose.p);
				PX_ASSERT(ok);
				PX_UNUSED(ok);

				mActor = mAsset->createApexActor(*descParams, *mApexScene);
			}
			const NxParameterized::Interface* iface = mAsset->getAssetNxParameterized();
			const turbulencefs::SubstanceSourceAssetParams* hap = static_cast< const turbulencefs::SubstanceSourceAssetParams*>(iface);
			mAverageDensity = hap->averageDensity;
			mStandardDeviationDensity = hap->stdDensity;
		}
		else if (mActor)
		{
			NxSubstanceSourceActor* a = static_cast< NxSubstanceSourceActor*>(mActor);
			physx::apex::NxApexShape *s = a->getShape();
			s->setPose(myPose);
			PX_UNUSED(s);
		}
	}
	else
	{
		releaseActor();
	}
}


void EffectTurbulenceFS::visualize(physx::general_renderdebug4::RenderDebug* callback, bool solid) const
{

}

void EffectJetFS::visualize(physx::general_renderdebug4::RenderDebug* callback, bool solid) const
{

}

void EffectAttractorFS::visualize(physx::general_renderdebug4::RenderDebug* callback, bool solid) const
{

}

void EffectEmitter::computeVelocity(physx::PxF32 dtime)
{
	mVelocityTime += dtime;
	if (mFirstVelocityFrame)
	{
		if (mActor)
		{
			mFirstVelocityFrame = false;
			mVelocityTime = 0;
			NxApexEmitterActor* ea = static_cast< NxApexEmitterActor*>(mActor);
			mLastEmitterPosition = ea->getGlobalPose().getPosition();
		}
	}
	else if (mHaveSetPosition && mActor)
	{
		mHaveSetPosition = false;
		NxApexEmitterActor* ea = static_cast< NxApexEmitterActor*>(mActor);
		physx::PxVec3 newPos = ea->getGlobalPose().getPosition();
		mEmitterVelocity = (newPos - mLastEmitterPosition) * (1.0f / mVelocityTime);
		mLastEmitterPosition = newPos;
		mVelocityTime = 0;
	}
}

void EffectEmitter::refresh(const PxTransform& parent,
							bool parentEnabled,
							bool fromSetPose,
							NxApexRenderVolume* renderVolume,
							NxApexEmitterActor::NxApexEmitterValidateCallback *callback)
{
	if (parentEnabled && mEnabled && mAsset)
	{
		mPose = parent * mLocalPose;

		physx::PxMat44 myPose(mPose);

		if (mActor == NULL && mAsset && !fromSetPose)
		{
			mFirstVelocityFrame = true;
			mHaveSetPosition = false;
			NxParameterized::Interface* descParams = mAsset->getDefaultActorDesc();
			if (descParams)
			{
				bool ok = NxParameterized::setParamMat44(*descParams, "initialPose", myPose);
				PX_UNUSED(ok);
				PX_ASSERT(ok);
				ok = NxParameterized::setParamBool(*descParams, "emitAssetParticles", true);
				PX_ASSERT(ok);
				NxApexEmitterAsset* easset = static_cast< NxApexEmitterAsset*>(mAsset);
				NxApexEmitterActor* ea = mParticlesScene->getEmitterFromPool(easset);
				if (ea)
				{
					ea->setCurrentPose(myPose);
					mActor = static_cast< NxApexActor*>(ea);
				}
				else
				{
					mActor = mAsset->createApexActor(*descParams, *mApexScene);
				}
				if (mActor)
				{
					const NxParameterized::Interface* iface = mAsset->getAssetNxParameterized();
					const char* className = iface->className();
					if (strcmp(className, "ApexEmitterAssetParameters") == 0)
					{						
						const emitter::ApexEmitterAssetParameters* ap = static_cast<const  emitter::ApexEmitterAssetParameters*>(iface);
						mRateRange.minimum = ap->rateRange.min;
						mRateRange.maximum = ap->rateRange.max;
						mLifetimeRange.minimum = ap->lifetimeRange.min;
						mLifetimeRange.maximum = ap->lifetimeRange.max;
						NxApexEmitterActor* ea = static_cast< NxApexEmitterActor*>(mActor);
						ea->setRateRange(mRateRange);
						ea->setLifetimeRange(mLifetimeRange);
						ea->startEmit(false);
						ea->setPreferredRenderVolume(renderVolume);
						ea->setApexEmitterValidateCallback(callback);
					}
				}
			}
		}
		else if (mActor)
		{
			NxApexEmitterActor* ea = static_cast< NxApexEmitterActor*>(mActor);
			mHaveSetPosition = true; // set semaphore for computing the velocity.
			ea->setCurrentPose(myPose);
			ea->setPreferredRenderVolume(renderVolume);
		}
	}
	else
	{
		releaseActor();
	}
}


void EffectTurbulenceFS::refresh(const PxTransform& parent, bool parentEnabled, bool fromSetPose, NxApexRenderVolume* renderVolume,NxApexEmitterActor::NxApexEmitterValidateCallback *callback)
{
	if (parentEnabled && mEnabled && mAsset)
	{
		physx::PxMat44 myPose(parent * mLocalPose);

		if (mActor == NULL && mAsset && !fromSetPose)
		{
			NxParameterized::Interface* descParams = mAsset->getDefaultActorDesc();
			if (descParams)
			{
				bool ok = NxParameterized::setParamMat44(*descParams, "initialPose", myPose);
				PX_UNUSED(ok);
				PX_ASSERT(ok);
				mActor = mAsset->createApexActor(*descParams, *mApexScene);
			}
		}
		else if (mActor)
		{
			NxTurbulenceFSActor* a = static_cast< NxTurbulenceFSActor*>(mActor);
			a->setPose(myPose);
		}
	}
	else
	{
		releaseActor();
	}
}

void EffectJetFS::refresh(const PxTransform& parent, bool parentEnabled, bool fromSetPose, NxApexRenderVolume* renderVolume,NxApexEmitterActor::NxApexEmitterValidateCallback *callback)
{
	if (parentEnabled && mEnabled && mAsset)
	{
		physx::PxMat44 myPose(parent * mLocalPose);

		if (mActor == NULL && mAsset && !fromSetPose)
		{
			NxParameterized::Interface* descParams = mAsset->getDefaultActorDesc();
			if (descParams)
			{
				mActor = mAsset->createApexActor(*descParams, *mApexScene);
				if (mActor)
				{
					NxJetFSActor* fs = static_cast< NxJetFSActor*>(mActor);
					if (fs)
					{
						fs->setCurrentPose(myPose);
					}
				}
			}
		}
		else if (mActor)
		{
			NxJetFSActor* a = static_cast< NxJetFSActor*>(mActor);
			a->setCurrentPose(myPose);
		}
	}
	else
	{
		releaseActor();
	}
}
void EffectAttractorFS::refresh(const PxTransform& parent, bool parentEnabled, bool fromSetPose, NxApexRenderVolume* renderVolume,NxApexEmitterActor::NxApexEmitterValidateCallback *callback)
{
	if (parentEnabled && mEnabled && mAsset)
	{
		PxTransform initialPose = _getPose(mData->EffectProperties.Position.TranslateX,
		                                   mData->EffectProperties.Position.TranslateY,
		                                   mData->EffectProperties.Position.TranslateZ,
		                                   mData->EffectProperties.Orientation.RotateX,
		                                   mData->EffectProperties.Orientation.RotateY,
		                                   mData->EffectProperties.Orientation.RotateZ);
		physx::PxMat44 myPose(parent * initialPose);
		if (mActor == NULL && mAsset && !fromSetPose)
		{
			NxParameterized::Interface* descParams = mAsset->getDefaultActorDesc();
			if (descParams)
			{
				bool ok = NxParameterized::setParamVec3(*descParams, "position", myPose.getPosition());
				PX_UNUSED(ok);
				PX_ASSERT(ok);
				mActor = mAsset->createApexActor(*descParams, *mApexScene);
			}
		}
		else if (mActor)
		{
			NxAttractorFSActor* a = static_cast< NxAttractorFSActor*>(mActor);
			a->setCurrentPosition(myPose.getPosition());
		}
	}
	else
	{
		releaseActor();
	}
}


EffectForceField::EffectForceField(const char* parentName, ForceFieldEffect* data, NxApexSDK& sdk, NxApexScene& scene, ParticlesScene& dscene, const PxTransform& rootPose, bool parentEnabled) : mData(data), EffectData(ET_FORCE_FIELD, &sdk, &scene, &dscene, parentName, NX_FORCEFIELD_AUTHORING_TYPE_NAME, data->EffectProperties.InitialDelayTime, data->EffectProperties.Duration, data->EffectProperties.RepeatCount, data->EffectProperties.RepeatDelay, data->EffectProperties.RandomizeRepeatTime, data->EffectProperties.Enable,
	        _getPose(data->EffectProperties.Position.TranslateX,
	                 data->EffectProperties.Position.TranslateY,
	                 data->EffectProperties.Position.TranslateZ,
	                 data->EffectProperties.Orientation.RotateX,
	                 data->EffectProperties.Orientation.RotateY,
	                 data->EffectProperties.Orientation.RotateZ))
{
}

EffectForceField::~EffectForceField(void)
{
}

void EffectForceField::visualize(physx::general_renderdebug4::RenderDebug* callback, bool solid) const
{

}

void EffectForceField::refresh(const PxTransform& parent, bool parentEnabled, bool fromSetPose, NxApexRenderVolume* renderVolume,NxApexEmitterActor::NxApexEmitterValidateCallback *callback)
{
	if (parentEnabled && mEnabled && mAsset)
	{
		PxTransform initialPose = _getPose(mData->EffectProperties.Position.TranslateX,
		                                   mData->EffectProperties.Position.TranslateY,
		                                   mData->EffectProperties.Position.TranslateZ,
		                                   mData->EffectProperties.Orientation.RotateX,
		                                   mData->EffectProperties.Orientation.RotateY,
		                                   mData->EffectProperties.Orientation.RotateZ);
		physx::PxMat44 myPose(parent * initialPose);
		if (mActor == NULL && mAsset && !fromSetPose)
		{
			NxParameterized::Interface* descParams = mAsset->getDefaultActorDesc();
			if (descParams)
			{
				mActor = mAsset->createApexActor(*descParams, *mApexScene);
				if (mActor)
				{
					NxForceFieldActor* fs = static_cast< NxForceFieldActor*>(mActor);
					if (fs)
					{
						fs->setPose(myPose);
					}
				}
			}
		}
		else if (mActor)
		{
			NxForceFieldActor* a = static_cast< NxForceFieldActor*>(mActor);
			a->setPose(myPose);
		}
	}
	else
	{
		releaseActor();
	}
}


EffectData::EffectData(EffectType type,
                       NxApexSDK* sdk,
                       NxApexScene* scene,
                       ParticlesScene* dscene,
                       const char* assetName,
                       const char* nameSpace,
                       PxF32 initialDelayTime,
                       PxF32 duration,
                       PxU32 repeatCount,
                       PxF32 repeatDelay,
                       PxF32 randomDeviation,
                       bool enabled,
                       const PxTransform& localPose)
{
	mLocalPose = localPose;
	mEnabled = enabled;
	mRandomDeviation = randomDeviation;
	mUseEmitterPool = dscene->getModuleParticles()->getUseEmitterPool();
	mFirstRate = true;
	mType = type;
	mState = ES_INITIAL_DELAY;
	mStateTime = getRandomTime(initialDelayTime);
	mStateCount = 0;
	mParticlesScene = dscene;
	mInitialDelayTime = initialDelayTime;
	mDuration = duration;
	mRepeatCount = repeatCount;
	mRepeatDelay = repeatDelay;
	mApexSDK = sdk;
	mApexScene = scene;
	mActor = NULL;
	mNameSpace = nameSpace;
	mAsset = (physx::apex::NxApexAsset*)mApexSDK->getNamedResourceProvider()->getResource(mNameSpace, assetName);
	if (mAsset)
	{
		if (mType == ET_EMITTER)
		{
			if (!mUseEmitterPool)
			{
				mApexSDK->getNamedResourceProvider()->setResource(mNameSpace, assetName, mAsset, true);
			}
		}
		else
		{
			mApexSDK->getNamedResourceProvider()->setResource(mNameSpace, assetName, mAsset, true);
		}
	}
}

EffectData::~EffectData(void)
{
	releaseActor();
	if (mAsset)
	{
		if (mType == ET_EMITTER)
		{
			if (!mUseEmitterPool)
			{
				mApexSDK->getNamedResourceProvider()->releaseResource(mNameSpace, mAsset->getName());
			}
		}
		else
		{
			mApexSDK->getNamedResourceProvider()->releaseResource(mNameSpace, mAsset->getName());
		}
	}
}

void EffectData::releaseActor(void)
{
	if (mActor)
	{
		if (mType == ET_EMITTER && mUseEmitterPool)
		{
			NxApexEmitterActor* ae = static_cast< NxApexEmitterActor*>(mActor);
			mParticlesScene->addToEmitterPool(ae);
		}
		else
		{
			mActor->release();
		}
		mActor = NULL;
	}
}

PxF32 EffectData::getRandomTime(PxF32 baseTime)
{
	PxF32 deviation = baseTime * mRandomDeviation;
	return ranf(baseTime - deviation, baseTime + deviation);
}

bool EffectData::simulate(PxF32 dtime, bool& reset)
{
	bool ret = false;

	if (!mEnabled)
	{
		return false;
	}

	switch (mState)
	{
	case ES_INITIAL_DELAY:
		mStateTime -= dtime;
		if (mStateTime <= 0)
		{
			mState = ES_ACTIVE;		// once past the initial delay, it is now active
			mStateTime = getRandomTime(mDuration);	// Time to the next state change...
			ret = true;	// set ret to true because the effect is alive
		}
		break;
	case ES_ACTIVE:
		ret = true;		// it's still active..
		mStateTime -= dtime;	// decrement delta time to next state change.
		if (mStateTime <= 0)   // if it's time for a state change.
		{
			if (mDuration == 0)   // if the
			{
				if (mRepeatDelay > 0)   // if there is a delay until the time we repeate
				{
					mStateTime = getRandomTime(mRepeatDelay);	// set time until repeat delay
					mState = ES_REPEAT_DELAY; // change state to repeat delay
				}
				else
				{
					mStateTime = getRandomTime(mDuration); // if there is no repeat delay; just continue
					if (mRepeatCount > 1)
					{
						reset = true; // looped..
					}
				}
			}
			else
			{
				mStateCount++;	// increment the state change counter.
				if (mStateCount >= mRepeatCount && mRepeatCount != 9999)   // have we hit the total number repeat counts.
				{
					mState = ES_DONE; // then we are completely done; the actor is no longer alive
					ret = false;
				}
				else
				{
					if (mRepeatDelay > 0)   // is there a repeat delay?
					{
						mStateTime = getRandomTime(mRepeatDelay);
						mState = ES_REPEAT_DELAY;
					}
					else
					{
						mStateTime = getRandomTime(mDuration);
						reset = true;
					}
				}
			}
		}
		break;
	case ES_REPEAT_DELAY:
		mStateTime -= dtime;
		if (mStateTime < 0)
		{
			mState = ES_ACTIVE;
			mStateTime = getRandomTime(mDuration);
			reset = true;
			ret = true;
		}
		break;
	}
	return ret;
}

void EffectPackageActor::updateParticles(PxF32 dtime)
{
	mSimTime = dtime;
	mCurrentLifeTime += mSimTime;
}

PxF32 EffectPackageActor::internalGetDuration(void)
{
	PxF32 duration = 1000;
	for (physx::PxU32 i = 0; i < mEffects.size(); i++)
	{
		EffectData* ed = mEffects[i];
		if (ed->getDuration() < duration)
		{
			duration = ed->getDuration();
		}
	}
	return duration;
}

// applies velocity adjustment to this range
static void processVelocityAdjust(const particles::EmitterEffectNS::EmitterVelocityAdjust_Type& vprops,
                                  const physx::PxVec3& velocity,
                                  NxRange<physx::PxF32> &range)
{
	
	physx::PxF32 r = 1;
	physx::PxF32 v = velocity.magnitude();	// compute the absolute magnitude of the current emitter velocity
	if (v <= vprops.VelocityLow)   // if the velocity is less than the minimum velocity adjustment range
	{
		r = vprops.LowValue;	// Use the 'low-value' ratio adjustment
	}
	else if (v >= vprops.VelocityHigh)   // If the velocity is greater than the high range
	{
		r = vprops.HighValue; // then clamp tot he high value adjustment
	}
	else
	{
		PxF32 ratio = 1;
		PxF32 diff = vprops.VelocityHigh - vprops.VelocityLow; // Compute the velocity differntial
		if (diff > 0)
		{
			ratio = 1.0f / diff;	// compute the inverse velocity differential
		}
		physx::PxF32 l = (v - vprops.VelocityLow) * ratio;	// find out the velocity lerp rate
		r = (vprops.HighValue - vprops.LowValue) * l + vprops.LowValue;
	}
	range.minimum *= r;
	range.maximum *= r;
}


void EffectPackageActor::updatePoseAndBounds(bool screenCulling, bool znegative)
{
	if (!mEnabled)
	{
		for (physx::PxU32 i = 0; i < mEffects.size(); i++)
		{
			EffectData* ed = mEffects[i];
			if (ed->getEffectActor())
			{
				ed->releaseActor();
			}
		}
		mAlive = false;
		return;
	}

	PxF32 ZCOMPARE = -1;
	if (znegative)
	{
		ZCOMPARE *= -1;
	}

	//
	bool prevVisible = mVisible;
	mVisible = true;  // default visibile state is based on whether or not this effect is enabled.
	physx::PxF32 emitterRate = 1;
	mVisState = VS_ON_SCREEN; // default value
	if (mVisible)   // if it's considered visible/enabled then let's do the LOD cacluation
	{
		const physx::PxMat44& viewMatrix = mScene->getViewMatrix();
		physx::PxVec3 pos = viewMatrix.transform(mPose.p);
		float magnitudeSquared = pos.magnitudeSquared();
		if (mData->LODSettings.CullByDistance)   // if distance culling is enabled
		{

			// If the effect is past the maximum distance then mark it is no longer visible.
			if (magnitudeSquared > mData->LODSettings.FadeDistanceEnd * mData->LODSettings.FadeDistanceEnd)
			{
				mVisible = false;
				mVisState = VS_OFF_SCREEN;
			} // if the effect is within the fade range; then compute the lerp value for it along that range as 'emitterRate'
			else if (magnitudeSquared > mData->LODSettings.FadeDistanceEnd * mData->LODSettings.FadeDistanceBegin)
			{
				physx::PxF32 distance = physx::PxSqrt(magnitudeSquared);
				physx::PxF32 delta = mData->LODSettings.FadeDistanceEnd - mData->LODSettings.FadeDistanceBegin;
				if (delta > 0)
				{
					emitterRate = 1.0f - ((distance - mData->LODSettings.FadeDistanceBegin) / delta);
				}
			}
		}
		// If it's still considered visible (i.e. in range) and off screen culling is enabled; let's test it's status on/off screen
		if (mVisible && mData->LODSettings.CullOffScreen && screenCulling)
		{
			if (magnitudeSquared < (mData->LODSettings.ScreenCullDistance * mData->LODSettings.ScreenCullDistance))
			{
				mVisState = VS_TOO_CLOSE;
			}
			else if (pos.z * ZCOMPARE > 0)
			{
				mVisState = VS_BEHIND_SCREEN;
			}
			else
			{
				const physx::PxMat44& projMatrix = mScene->getProjMatrix();
				PxVec4 p(pos.x, pos.y, pos.z, 1);
				p = projMatrix.transform(p);
				PxF32 recipW = 1.0f / p.w;

				p.x = p.x * recipW;
				p.y = p.y * recipW;
				p.z = p.z * recipW;

				PxF32 smin = -1 - mData->LODSettings.ScreenCullSize;
				PxF32 smax = 1 + mData->LODSettings.ScreenCullSize;

				if (p.x >= smin && p.x <= smax && p.y >= smin && p.y <= smax)
				{
					mVisState = VS_ON_SCREEN;
				}
				else
				{
					mVisState = VS_OFF_SCREEN;
				}
			}
		}
	}
	if (mVisState == VS_ON_SCREEN || mVisState == VS_TOO_CLOSE)
	{
		mOffScreenTime = 0;
	}
	else
	{
		mOffScreenTime += mSimTime;
		if (mOffScreenTime > mData->LODSettings.OffScreenCullTime)
		{
			mVisible = false; // mark it as non-visible due to it being off sceen too long.
			mAlive = false;
		}
		else
		{
			mVisible = mEverVisible; // was it ever visible?
		}
	}

	if (mFirstFrame && !mVisible && screenCulling)
	{
		if (getDuration() != 0)
		{
			mEnabled = false;
			return;
		}
	}


	if (mVisible)
	{
		mEverVisible = true;
	}


	bool aliveState = mVisible;

	// do the fade in/fade out over time logic...
	if (mData->LODSettings.FadeOutRate > 0)   // If there is a fade in/out time.
	{
		if (aliveState)    // if the effect is considered alive/visible then attenuate the emitterRate based on that fade in time value
		{
			mFadeTime += mSimTime;
			if (mFadeTime < mData->LODSettings.FadeOutRate)
			{
				emitterRate = emitterRate * mFadeTime / mData->LODSettings.FadeOutRate;
			}
			else
			{
				mFadeTime = mData->LODSettings.FadeOutRate;
			}
		}
		else // if the effect is not visible then attenuate it based on the fade out time
		{
			mFadeTime -= mSimTime;
			if (mFadeTime > 0)
			{
				emitterRate = emitterRate * mFadeTime / mData->LODSettings.FadeOutRate;
				aliveState = true; // still alive because it hasn't finsihed fading out...
			}
			else
			{
				mFadeTime = 0;
			}
		}
	}

	if (mFadeIn)
	{
		mFadeInDuration += mSimTime;
		if (mFadeInDuration > mFadeInTime)
		{
			mFadeIn = false;
		}
		else
		{
			PxF32 fadeScale = (mFadeInDuration / mFadeInTime);
			emitterRate *= fadeScale;
		}
	}

	if (mFadeOut)
	{
		mFadeOutDuration += mSimTime;
		if (mFadeOutDuration > mFadeOutTime)
		{
			aliveState = mVisible = false;
		}
		else
		{
			PxF32 fadeScale = 1.0f - (mFadeOutDuration / mFadeOutTime);
			emitterRate *= fadeScale;
		}
	}

	if (mVisible)
	{
		mNotVisibleTime = 0;
	}
	else
	{
		mNotVisibleTime += mSimTime;
	}

	bool anyAlive = false;

	for (physx::PxU32 i = 0; i < mEffects.size(); i++)
	{
		bool alive = aliveState;
		EffectData* ed = mEffects[i];
		// only emitters can handle a 'repeat' count.
		// others have an initial delay
		bool reset = false;
		if (ed->getDuration() == 0)
		{
			reset = !prevVisible; // if it was not previously visible then force a reset to bring it back to life.
		}
		if (alive)
		{
			alive = ed->simulate(mSimTime, reset);
			anyAlive = true;
		}
		if (ed->isDead())   // if it's lifetime has completely expired kill it!
		{
			if (ed->getEffectActor())
			{
				ed->releaseActor();
			}
		}
		else
		{
			switch (ed->getType())
			{
			case ET_EMITTER:
			{
				EffectEmitter* ee = static_cast< EffectEmitter*>(ed);
				ee->computeVelocity(mSimTime);
				NxApexEmitterActor* ea = static_cast< NxApexEmitterActor*>(ed->getEffectActor());
				// if there is already an emitter actor...
				if (ea)
				{
					if (alive)   // is it alive?
					{
						if (reset)   // is it time to reset it's condition?
						{
							ea->startEmit(false);
						}
						if (mData->LODSettings.FadeEmitterRate || mData->LODSettings.RandomizeEmitterRate || mFadeOut || mFadeIn)
						{
							// attenuate the emitter rate range based on the previously computed LOD lerp value
							NxRange<physx::PxF32> rateRange;
							if (mData->LODSettings.RandomizeEmitterRate && ee->mFirstRate)
							{
								physx::PxF32 rate = ranf(ee->mRateRange.minimum, ee->mRateRange.maximum);
								ee->mRateRange.minimum = rate;
								ee->mRateRange.maximum = rate;
								if (!mData->LODSettings.FadeEmitterRate)
								{
									ea->setRateRange(rateRange);
								}
								ee->mFirstRate = false;
							}
							if (mData->LODSettings.FadeEmitterRate ||
							        mFadeOut ||
							        mFadeIn ||
							        ee->mData->EmitterVelocityChanges.AdjustEmitterRate.AdjustEnabled ||
							        ee->mData->EmitterVelocityChanges.AdjustLifetime.AdjustEnabled)
							{
								rateRange.minimum = ee->mRateRange.minimum * emitterRate;
								rateRange.maximum = ee->mRateRange.maximum * emitterRate;
								if (ee->mData->EmitterVelocityChanges.AdjustEmitterRate.AdjustEnabled)
								{
									processVelocityAdjust(ee->mData->EmitterVelocityChanges.AdjustEmitterRate, ee->mEmitterVelocity, rateRange);
								}
								ea->setRateRange(rateRange);
								if (ee->mData->EmitterVelocityChanges.AdjustLifetime.AdjustEnabled)
								{
									NxRange<physx::PxF32> rateRange = ee->mLifetimeRange;
									processVelocityAdjust(ee->mData->EmitterVelocityChanges.AdjustLifetime, ee->mEmitterVelocity, rateRange);
									ea->setLifetimeRange(rateRange);
								}
							}
						}
					}
					else
					{
						if (mNotVisibleTime > mData->LODSettings.NonVisibleDeleteTime)   // if it's been non-visible for a long time; delete it, don't just disable it!
						{
							if (ed->getEffectActor())
							{
								ed->releaseActor();
							}
						}
						else
						{
							if (ea->isEmitting())
							{
								ea->stopEmit(); // just stop emitting but don't destroy the actor.
							}
						}
					}
				}
				else
				{
					if (alive)   // if it is now alive but was not previously; start the initial instance.
					{
						ed->refresh(mPose, true, false, mRenderVolume,mEmitterValidateCallback);
					}
				}
			}
			break;
			case ET_ATTRACTOR_FS:
			{
				//EffectAttractorFS *ee = static_cast< EffectAttractorFS *>(ed);
				NxAttractorFSActor* ea = static_cast< NxAttractorFSActor*>(ed->getEffectActor());
				// if there is already an emitter actor...
				if (ea)
				{
					if (alive)   // is it alive?
					{
						if (reset)   // is it time to reset it's condition?
						{
							ea->setEnabled(true);
						}
						if (mData->LODSettings.FadeAttractorFieldStrength)
						{
							// TODO
						}
					}
					else
					{
						if (mNotVisibleTime > mData->LODSettings.NonVisibleDeleteTime)   // if it's been non-visible for a long time; delete it, don't just disable it!
						{

							if (ed->getEffectActor())
							{
								ed->releaseActor();
							}
						}
						else
						{
							ea->setEnabled(false);
						}
					}
				}
				else
				{
					if (alive)   // if it is now alive but was not previously; start the initial instance.
					{
						ed->refresh(mPose, true, false, mRenderVolume,mEmitterValidateCallback);
					}
				}
			}
			break;
			case ET_JET_FS:
			{
				//EffectJetFS *ee = static_cast< EffectJetFS *>(ed);
				NxJetFSActor* ea = static_cast< NxJetFSActor*>(ed->getEffectActor());
				// if there is already an emitter actor...
				if (ea)
				{
					if (alive)   // is it alive?
					{
						if (reset)   // is it time to reset it's condition?
						{
							ea->setEnabled(true);
						}
						if (mData->LODSettings.FadeJetFieldStrength)
						{
							// TODO
						}
					}
					else
					{
						if (mNotVisibleTime > mData->LODSettings.NonVisibleDeleteTime)   // if it's been non-visible for a long time; delete it, don't just disable it!
						{
							if (ed->getEffectActor())
							{
								ed->releaseActor();
							}
						}
						else
						{
							ea->setEnabled(false);
						}
					}
				}
				else
				{
					if (alive)   // if it is now alive but was not previously; start the initial instance.
					{
						ed->refresh(mPose, true, false, mRenderVolume,mEmitterValidateCallback);
					}
				}
			}
			break;
			case ET_NOISE_FS:
			{
				NxNoiseFSActor* ea = static_cast< NxNoiseFSActor*>(ed->getEffectActor());
				// if there is already an emitter actor...
				if (ea)
				{
					if (alive)   // is it alive?
					{
						if (reset)   // is it time to reset it's condition?
						{
							ea->setEnabled(true);
						}
					}
					else
					{
						if (mNotVisibleTime > mData->LODSettings.NonVisibleDeleteTime)   // if it's been non-visible for a long time; delete it, don't just disable it!
						{
							if (ed->getEffectActor())
							{
								ed->releaseActor();
							}
						}
						else
						{
							ea->setEnabled(false);
						}
					}
				}
				else
				{
					if (alive)   // if it is now alive but was not previously; start the initial instance.
					{
						ed->refresh(mPose, true, false, mRenderVolume,mEmitterValidateCallback);
					}
				}
			}
			break;
			case ET_VORTEX_FS:
			{
				NxVortexFSActor* ea = static_cast< NxVortexFSActor*>(ed->getEffectActor());
				// if there is already an emitter actor...
				if (ea)
				{
					if (alive)   // is it alive?
					{
						if (reset)   // is it time to reset it's condition?
						{
							ea->setEnabled(true);
						}
					}
					else
					{
						if (mNotVisibleTime > mData->LODSettings.NonVisibleDeleteTime)   // if it's been non-visible for a long time; delete it, don't just disable it!
						{
							if (ed->getEffectActor())
							{
								ed->releaseActor();
							}
						}
						else
						{
							ea->setEnabled(false);
						}
					}
				}
				else
				{
					if (alive)   // if it is now alive but was not previously; start the initial instance.
					{
						ed->refresh(mPose, true, false, mRenderVolume,mEmitterValidateCallback);
					}
				}
			}
			break;
			case ET_TURBULENCE_FS:
			{
				//EffectTurbulenceFS *ee = static_cast< EffectTurbulenceFS *>(ed);
				NxTurbulenceFSActor* ea = static_cast< NxTurbulenceFSActor*>(ed->getEffectActor());
				// if there is already an emitter actor...
				if (ea)
				{
					if (alive)   // is it alive?
					{
						if (reset)   // is it time to reset it's condition?
						{
							ea->setEnabled(true);
						}
						if (mData->LODSettings.FadeTurbulenceNoise)
						{
							// TODO
						}
					}
					else
					{
						if (mNotVisibleTime > mData->LODSettings.NonVisibleDeleteTime)   // if it's been non-visible for a long time; delete it, don't just disable it!
						{
							if (ed->getEffectActor())
							{
								ed->releaseActor();
							}
						}
						else
						{
							ea->setEnabled(false);
						}
					}
				}
				else
				{
					if (alive)   // if it is now alive but was not previously; start the initial instance.
					{
						ed->refresh(mPose, true, false, mRenderVolume,mEmitterValidateCallback);
					}
				}
			}
			break;
			case ET_FORCE_FIELD:
			{
				//EffectForceField *ee = static_cast< EffectForceField *>(ed);
				NxForceFieldActor* ea = static_cast< NxForceFieldActor*>(ed->getEffectActor());
				// if there is already an emitter actor...
				if (ea)
				{
					if (alive)   // is it alive?
					{
						if (reset)   // is it time to reset it's condition?
						{
							ea->enable();
						}
						if (mData->LODSettings.FadeForceFieldStrength)
						{
							// TODO
						}
					}
					else
					{
						if (mNotVisibleTime > mData->LODSettings.NonVisibleDeleteTime)   // if it's been non-visible for a long time; delete it, don't just disable it!
						{
							if (ed->getEffectActor())
							{
								ed->releaseActor();
							}
						}
						else
						{
							if (ea->isEnable())
							{
								ea->disable();
							}
						}
					}
				}
				else
				{
					if (alive)   // if it is now alive but was not previously; start the initial instance.
					{
						ed->refresh(mPose, true, false, mRenderVolume,mEmitterValidateCallback);
					}
				}
			}
			break;
			case ET_HEAT_SOURCE:
			{
				EffectHeatSource* ee = static_cast< EffectHeatSource*>(ed);
				NxHeatSourceActor* ea = static_cast< NxHeatSourceActor*>(ed->getEffectActor());
				// if there is already an emitter actor...
				if (ea)
				{
					if (alive)   // is it alive?
					{
						if (reset)   // is it time to reset it's condition?
						{
							ea->setTemperature(ee->mAverageTemperature, ee->mStandardDeviationTemperature);
							//ea->enable();
							//TODO!
						}
						if (mData->LODSettings.FadeHeatSourceTemperature)
						{
							// TODO
						}
					}
					else
					{
						if (mNotVisibleTime > mData->LODSettings.NonVisibleDeleteTime)   // if it's been non-visible for a long time; delete it, don't just disable it!
						{
							if (ed->getEffectActor())
							{
								ed->releaseActor();
							}
						}
						else
						{
							ea->setTemperature(0, 0);
						}
					}
				}
				else
				{
					if (alive)   // if it is now alive but was not previously; start the initial instance.
					{
						ed->refresh(mPose, true, false, mRenderVolume,mEmitterValidateCallback);
					}
				}
			}
			break;
			case ET_SUBSTANCE_SOURCE:
				{
					EffectSubstanceSource* ee = static_cast< EffectSubstanceSource*>(ed);
					NxSubstanceSourceActor* ea = static_cast< NxSubstanceSourceActor*>(ed->getEffectActor());
					// if there is already an emitter actor...
					if (ea)
					{
						if (alive)   // is it alive?
						{
							if (reset)   // is it time to reset it's condition?
							{
								ea->setDensity(ee->mAverageDensity, ee->mStandardDeviationDensity);
								//ea->enable();
								//TODO!
							}
							if (mData->LODSettings.FadeHeatSourceTemperature)
							{
								// TODO
							}
						}
						else
						{
							if (mNotVisibleTime > mData->LODSettings.NonVisibleDeleteTime)   // if it's been non-visible for a long time; delete it, don't just disable it!
							{
								if (ed->getEffectActor())
								{
									ed->releaseActor();
								}
							}
							else
							{
								ea->setDensity(0, 0);
							}
						}
					}
					else
					{
						if (alive)   // if it is now alive but was not previously; start the initial instance.
						{
							ed->refresh(mPose, true, false, mRenderVolume,mEmitterValidateCallback);
						}
					}
				}
				break;
			default:
				PX_ALWAYS_ASSERT(); // effect type not handled!
				break;
			}
		}
	}
	mAlive = anyAlive;
	mFirstFrame = false;
}


/**
\brief Returns the name of the effect at this index.

\param [in] effectIndex : The effect number to refer to; must be less than the result of getEffectCount
*/
const char* EffectPackageActor::getEffectName(PxU32 effectIndex) const
{
	const char* ret = NULL;
	if (effectIndex < mEffects.size())
	{
		EffectData* ed = mEffects[effectIndex];
		ret = ed->getEffectAsset()->getName();
	}
	return ret;
}

/**
\brief Returns true if this sub-effect is currently enabled.

\param [in] effectIndex : The effect number to refer to; must be less than the result of getEffectCount
*/
bool EffectPackageActor::isEffectEnabled(PxU32 effectIndex) const
{
	bool ret = false;
	if (effectIndex < mEffects.size())
	{
		EffectData* ed = mEffects[effectIndex];
		ret = ed->isEnabled();
	}
	return ret;
}

/**
\brief Set's the enabled state of this sub-effect

\param [in] effectIndex : The effect number to refer to; must be less than the result of getEffectCount
\param [in] state : Whether the effect should be enabled or not.
*/
bool EffectPackageActor::setEffectEnabled(PxU32 effectIndex, bool state)
{
	bool ret = false;

	if (effectIndex < mEffects.size())
	{
		EffectData* ed = mEffects[effectIndex];
		ed->setEnabled(state);
		ret = true;
	}

	return ret;
}

/**
\brief Returns the pose of this sub-effect; returns as a a bool the active state of this effect.

\param [in] effectIndex : The effect number to refer to; must be less than the result of getEffectCount
\param [pose] : Contains the pose requested
\param [worldSpace] : Whether to return the pose in world-space or in parent-relative space.
*/
bool EffectPackageActor::getEffectPose(PxU32 effectIndex, PxTransform& pose, bool worldSpace)
{
	bool ret = false;

	if (effectIndex < mEffects.size())
	{
		EffectData* ed = mEffects[effectIndex];

		if (worldSpace)
		{
			pose = ed->getLocalPose();
		}
		else
		{
			pose = ed->getWorldPose();
		}
		ret = true;
	}

	return ret;
}

/**
\brief Sets the pose of this sub-effect; returns as a a bool the active state of this effect.

\param [in] effectIndex : The effect number to refer to; must be less than the result of getEffectCount
\param [pose] : Contains the pose to be set
\param [worldSpace] : Whether to return the pose in world-space or in parent-relative space.
*/
bool EffectPackageActor::setEffectPose(PxU32 effectIndex, const PxTransform& pose, bool worldSpace)
{
	bool ret = false;

	if (effectIndex < mEffects.size())
	{
		EffectData* ed = mEffects[effectIndex];
		if (worldSpace)
		{
			PxTransform p = getPose(); // get root pose
			PxTransform i = p.getInverse();
			PxTransform l = i * pose;
			ed->setLocalPose(pose);		// change the local pose
			setPose(p);
		}
		else
		{
			ed->setLocalPose(pose);		// change the local pose
			PxTransform p = getPose();
			setPose(p);
		}
		ret = true;
	}
	return ret;
}

/**
\brief Returns the current lifetime of the particle.
*/
PxF32 EffectPackageActor::getCurrentLife(void) const
{
	return mCurrentLifeTime;
}

PxF32 EffectData::getRealDuration(void) const
{
	PxF32 ret = 0;

	if (mDuration != 0 && mRepeatCount != 9999)   // if it's not an infinite lifespan...
	{
		ret = mInitialDelayTime + mRepeatCount * mDuration + mRepeatCount * mRepeatDelay;
	}

	return ret;
}

PxF32 EffectPackageActor::getDuration(void) const
{
	PxF32 ret = 0;

	for (physx::PxU32 i = 0; i < mEffects.size(); i++)
	{
		EffectData* ed = mEffects[i];
		if (ed->getType() == ET_EMITTER)   // if it's an emitter
		{
			PxF32 v = ed->getRealDuration();
			if (v == 0)	// any infinite lifespan sub-effect means the entire effect package has an infinite life
			{
				ret = 0;
				break;
			}
			else if (v > ret)
			{
				ret = v;
			}
		}
	}
	return ret;
}

void	EffectPackageActor::setPreferredRenderVolume(NxApexRenderVolume* volume)
{
	mRenderVolume = volume;
	for (physx::PxU32 i = 0; i < mEffects.size(); i++)
	{
		EffectData* ed = mEffects[i];
		if (ed->getType() == ET_EMITTER)   // if it's an emitter
		{
			EffectEmitter* ee = static_cast< EffectEmitter*>(ed);
			if (ee->getEffectActor())
			{
				NxApexEmitterActor* ea = static_cast< NxApexEmitterActor*>(ee->getEffectActor());
				ea->setPreferredRenderVolume(volume);
			}
		}
	}

}


EffectNoiseFS::EffectNoiseFS(const char* parentName,
                             NoiseFieldSamplerEffect* data,
                             NxApexSDK& sdk,
                             NxApexScene& scene,
                             ParticlesScene& dscene,
                             const PxTransform& rootPose,
                             bool parentEnabled) : mData(data), EffectData(ET_NOISE_FS, &sdk, &scene, &dscene, parentName, NX_NOISE_FS_AUTHORING_TYPE_NAME, data->EffectProperties.InitialDelayTime, data->EffectProperties.Duration, data->EffectProperties.RepeatCount, data->EffectProperties.RepeatDelay, data->EffectProperties.RandomizeRepeatTime, data->EffectProperties.Enable,
	                                     _getPose(data->EffectProperties.Position.TranslateX,
	                                             data->EffectProperties.Position.TranslateY,
	                                             data->EffectProperties.Position.TranslateZ,
	                                             data->EffectProperties.Orientation.RotateX,
	                                             data->EffectProperties.Orientation.RotateY,
	                                             data->EffectProperties.Orientation.RotateZ))
{
}

EffectNoiseFS::~EffectNoiseFS(void)
{
}

void EffectNoiseFS::visualize(physx::general_renderdebug4::RenderDebug* callback, bool solid) const
{

}

void EffectNoiseFS::refresh(const PxTransform& parent, bool parentEnabled, bool fromSetPose, NxApexRenderVolume* renderVolume,NxApexEmitterActor::NxApexEmitterValidateCallback *callback)
{
	if (parentEnabled && mEnabled && mAsset)
	{
		physx::PxMat44 myPose(parent * mLocalPose);

		if (mActor == NULL && mAsset && !fromSetPose)
		{
			NxParameterized::Interface* descParams = mAsset->getDefaultActorDesc();
			if (descParams)
			{
				mActor = mAsset->createApexActor(*descParams, *mApexScene);
				if (mActor)
				{
					NxNoiseFSActor* fs = static_cast< NxNoiseFSActor*>(mActor);
					if (fs)
					{
						fs->setCurrentPose(myPose);
					}
				}
			}
		}
		else if (mActor)
		{
			NxNoiseFSActor* a = static_cast< NxNoiseFSActor*>(mActor);
			a->setCurrentPose(myPose);
		}
	}
	else
	{
		releaseActor();
	}
}

EffectVortexFS::EffectVortexFS(const char* parentName,
                               VortexFieldSamplerEffect* data,
                               NxApexSDK& sdk,
                               NxApexScene& scene,
                               ParticlesScene& dscene,
                               const PxTransform& rootPose,
                               bool parentEnabled) : mData(data), EffectData(ET_VORTEX_FS, &sdk, &scene, &dscene, parentName, NX_VORTEX_FS_AUTHORING_TYPE_NAME, data->EffectProperties.InitialDelayTime, data->EffectProperties.Duration, data->EffectProperties.RepeatCount, data->EffectProperties.RepeatDelay, data->EffectProperties.RandomizeRepeatTime, data->EffectProperties.Enable,
	                                       _getPose(data->EffectProperties.Position.TranslateX,
	                                               data->EffectProperties.Position.TranslateY,
	                                               data->EffectProperties.Position.TranslateZ,
	                                               data->EffectProperties.Orientation.RotateX,
	                                               data->EffectProperties.Orientation.RotateY,
	                                               data->EffectProperties.Orientation.RotateZ))
{
}

EffectVortexFS::~EffectVortexFS(void)
{
}

void EffectVortexFS::visualize(physx::general_renderdebug4::RenderDebug* callback, bool solid) const
{

}

void EffectVortexFS::refresh(const PxTransform& parent, bool parentEnabled, bool fromSetPose, NxApexRenderVolume* renderVolume,NxApexEmitterActor::NxApexEmitterValidateCallback *callback)
{
	if (parentEnabled && mEnabled && mAsset)
	{
		physx::PxMat44 myPose(parent * mLocalPose);

		if (mActor == NULL && mAsset && !fromSetPose)
		{
			NxParameterized::Interface* descParams = mAsset->getDefaultActorDesc();
			if (descParams)
			{
				mActor = mAsset->createApexActor(*descParams, *mApexScene);
				if (mActor)
				{
					NxVortexFSActor* fs = static_cast< NxVortexFSActor*>(mActor);
					if (fs)
					{
						fs->setCurrentPose(myPose);
					}
				}
			}
		}
		else if (mActor)
		{
			NxVortexFSActor* a = static_cast< NxVortexFSActor*>(mActor);
			a->setCurrentPose(myPose);
		}
	}
	else
	{
		releaseActor();
	}
}

const char * EffectPackageActor::hasTurbulenceVolumeRenderMaterial(physx::PxU32 &index) const
{
	const char *ret = NULL;

	for (physx::PxU32 i=0; i<mEffects.size(); i++)
	{
		EffectData *d = mEffects[i];
		if ( d->getType() == ET_TURBULENCE_FS )
		{
			const NxParameterized::Interface *iface = d->getAsset()->getAssetNxParameterized();
			const turbulencefs::TurbulenceFSAssetParams *ap = static_cast< const turbulencefs::TurbulenceFSAssetParams *>(iface);
			if ( ap->volumeRenderMaterialName )
			{
				if ( strlen(ap->volumeRenderMaterialName->name()) > 0 )
				{
					index = i;
					ret = ap->volumeRenderMaterialName->name();
					break;
				}
			}
		}
	}

	return ret;
}

} // end of particles namespace
} // end of apex namespace
} // end of physx namespace
