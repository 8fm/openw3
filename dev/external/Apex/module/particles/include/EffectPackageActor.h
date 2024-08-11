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

#ifndef PARTICLES_EFFECT_PACKAGE_ACTOR_H

#define PARTICLES_EFFECT_PACKAGE_ACTOR_H

#include "ApexActor.h"
#include "NxEffectPackageActor.h"
#include "EffectPackageAsset.h"
#include "ParticlesBase.h"
#include "foundation/PxTransform.h"

namespace physx
{
namespace apex
{

class NxApexScene;

namespace turbulencefs
{
class NxModuleTurbulenceFS;
}

namespace particles
{

enum VisState
{
	VS_TOO_CLOSE,
	VS_ON_SCREEN,
	VS_BEHIND_SCREEN,
	VS_OFF_SCREEN,
};

class EffectData : public physx::shdfnd::UserAllocated
{
public:
	enum EffectState
	{
		ES_INITIAL_DELAY,
		ES_ACTIVE,
		ES_REPEAT_DELAY,
		ES_DONE
	};

	EffectData(EffectType type,
	           NxApexSDK* sdk,
	           NxApexScene* scene,
	           ParticlesScene* dscene,
	           const char* assetName,
	           const char* nameSpace,
	           PxF32 initialDelayTime,
	           PxF32 duration,
	           PxU32 repeatCount,
	           PxF32 repeateDelay,
	           PxF32 randomDeviation,
	           bool enabled,
	           const PxTransform& localPose);

	virtual ~EffectData(void);
	void releaseActor(void);

	PxF32 getRandomTime(PxF32 baseTime);

	EffectType getType(void) const
	{
		return mType;
	}
	virtual void release(void) = 0;
	virtual void visualize(physx::general_renderdebug4::RenderDebug* callback, bool solid) const = 0;
	virtual void refresh(const PxTransform& parent,
						bool parentEnabled,
						bool fromSetPose,
						NxApexRenderVolume* renderVolume,
						NxApexEmitterActor::NxApexEmitterValidateCallback *callback) = 0;

	bool isDead(void) const
	{
		return mState == ES_DONE;
	}

	NxApexActor* getEffectActor(void) const
	{
		return mActor;
	}
	NxApexAsset* getEffectAsset(void) const
	{
		return mAsset;
	}

	bool isEnabled(void) const
	{
		return mEnabled;
	}

	void setEnabled(bool state)
	{
		mEnabled = state;
	}

	bool simulate(PxF32 dtime, bool& reset);

	PxU32 getRepeatCount(void) const
	{
		return mRepeatCount;
	};
	PxF32 getDuration(void) const
	{
		return mDuration;
	};

	PxF32 getRealDuration(void) const;

	void setLocalPose(const PxTransform& p)
	{
		mLocalPose = p;
	}
	const PxTransform& getWorldPose(void) const
	{
		return mPose;
	};
	const PxTransform& getLocalPose(void) const
	{
		return mLocalPose;
	};

	NxApexAsset * getAsset(void) const { return mAsset; };

	bool					mFirstRate: 1;
protected:
	bool					mUseEmitterPool: 1;
	bool					mEnabled: 1;
	EffectState				mState;
	PxF32					mRandomDeviation;
	PxF32					mStateTime;
	PxU32					mStateCount;
	const char*				mNameSpace;
	ParticlesScene*			mParticlesScene;
	NxApexScene*				mApexScene;
	NxApexSDK*				mApexSDK;
	NxApexAsset*				mAsset;
	NxApexActor*				mActor;
	PxF32					mInitialDelayTime;
	PxF32					mDuration;
	PxU32					mRepeatCount;
	PxF32					mRepeatDelay;
	EffectType				mType;
	PxTransform				mPose;				// world space pose
	PxTransform				mLocalPose;			// local space pose
};

class EffectForceField : public EffectData
{
public:
	EffectForceField(const char* parentName,
	                 ForceFieldEffect* data,
	                 NxApexSDK& sdk,
	                 NxApexScene& scene,
	                 ParticlesScene& dscene,
	                 const PxTransform& rootPose,
	                 bool parentEnabled);

	virtual ~EffectForceField(void);

	virtual void release(void)
	{
		delete this;
	}

	virtual void visualize(physx::general_renderdebug4::RenderDebug* callback, bool solid) const;
	virtual void refresh(const PxTransform& parent, bool parentEnabled, bool fromSetPos, NxApexRenderVolume* renderVolume,NxApexEmitterActor::NxApexEmitterValidateCallback *callback);

	ForceFieldEffect*		mData;
};



class EffectEmitter : public EffectData
{
public:
	EffectEmitter(const char* parentName,
	              const EmitterEffect* data,
	              NxApexSDK& sdk,
	              NxApexScene& scene,
	              ParticlesScene& dscene,
	              const PxTransform& rootPose,
	              bool parentEnabled);

	virtual ~EffectEmitter(void);

	virtual void release(void)
	{
		delete this;
	}
	void computeVelocity(physx::PxF32 dtime);
	virtual void visualize(physx::general_renderdebug4::RenderDebug* callback, bool solid) const;

	virtual void refresh(const PxTransform& parent, bool parentEnabled, bool fromSetPose, NxApexRenderVolume* renderVolume,NxApexEmitterActor::NxApexEmitterValidateCallback *callback);

	NxRange<physx::PxF32> mRateRange;
	NxRange<physx::PxF32> mLifetimeRange;
	const EmitterEffect*		mData;
	bool					mFirstVelocityFrame: 1;
	bool					mHaveSetPosition;
	physx::PxVec3			mLastEmitterPosition;
	physx::PxF32			mVelocityTime;
	physx::PxVec3			mEmitterVelocity;
};

class EffectHeatSource : public EffectData
{
public:
	EffectHeatSource(const char* parentName,
	                 HeatSourceEffect* data,
	                 NxApexSDK& sdk,
	                 NxApexScene& scene,
	                 ParticlesScene& dscene,
					 NxModuleTurbulenceFS* moduleTurbulenceFS,
	                 const PxTransform& rootPose,
	                 bool parentEnabled);

	virtual ~EffectHeatSource(void);

	virtual void release(void)
	{
		delete this;
	}

	virtual void visualize(physx::general_renderdebug4::RenderDebug* callback, bool solid) const;
	virtual void refresh(const PxTransform& parent, bool parentEnabled, bool fromSetPose, NxApexRenderVolume* renderVolume,NxApexEmitterActor::NxApexEmitterValidateCallback *callback);


	physx::PxF32			mAverageTemperature;
	physx::PxF32			mStandardDeviationTemperature;

	NxModuleTurbulenceFS*	mModuleTurbulenceFS;
	HeatSourceEffect*		mData;
};

class EffectSubstanceSource : public EffectData
{
public:
	EffectSubstanceSource(const char* parentName,
		SubstanceSourceEffect* data,
		NxApexSDK& sdk,
		NxApexScene& scene,
		ParticlesScene& dscene,
		NxModuleTurbulenceFS* moduleTurbulenceFS,
		const PxTransform& rootPose,
		bool parentEnabled);

	virtual ~EffectSubstanceSource(void);

	virtual void release(void)
	{
		delete this;
	}

	virtual void visualize(physx::general_renderdebug4::RenderDebug* callback, bool solid) const;
	virtual void refresh(const PxTransform& parent, bool parentEnabled, bool fromSetPose, NxApexRenderVolume* renderVolume,NxApexEmitterActor::NxApexEmitterValidateCallback *callback);


	physx::PxF32			mAverageDensity;
	physx::PxF32			mStandardDeviationDensity;

	NxModuleTurbulenceFS*	mModuleTurbulenceFS;
	SubstanceSourceEffect*		mData;
};



class EffectTurbulenceFS : public EffectData
{
public:
	EffectTurbulenceFS(const char* parentName,
	                   TurbulenceFieldSamplerEffect* data,
	                   NxApexSDK& sdk,
	                   NxApexScene& scene,
	                   ParticlesScene& dscene,
					   NxModuleTurbulenceFS* moduleTurbulenceFS,
	                   const PxTransform& rootPose,
	                   bool parentEnabled);

	virtual ~EffectTurbulenceFS(void);

	virtual void release(void)
	{
		delete this;
	}

	virtual void visualize(physx::general_renderdebug4::RenderDebug* callback, bool solid) const;
	virtual void refresh(const PxTransform& parent, bool parentEnabled, bool fromSetPose, NxApexRenderVolume* renderVolume,NxApexEmitterActor::NxApexEmitterValidateCallback *callback);

	NxModuleTurbulenceFS*	mModuleTurbulenceFS;
	TurbulenceFieldSamplerEffect*	mData;
};

class EffectJetFS : public EffectData
{
public:
	EffectJetFS(const char* parentName,
	            JetFieldSamplerEffect* data,
	            NxApexSDK& sdk,
	            NxApexScene& scene,
	            ParticlesScene& dscene,
	            const PxTransform& rootPose,
	            bool parentEnabled);
	virtual ~EffectJetFS(void);

	virtual void release(void)
	{
		delete this;
	}

	virtual void visualize(physx::general_renderdebug4::RenderDebug* callback, bool solid) const;
	virtual void refresh(const PxTransform& parent, bool parentEnabled, bool fromSetPose, NxApexRenderVolume* renderVolume,NxApexEmitterActor::NxApexEmitterValidateCallback *callback);

	JetFieldSamplerEffect*	mData;
};

class EffectNoiseFS : public EffectData
{
public:
	EffectNoiseFS(const char* parentName,
	              NoiseFieldSamplerEffect* data,
	              NxApexSDK& sdk,
	              NxApexScene& scene,
	              ParticlesScene& dscene,
	              const PxTransform& rootPose,
	              bool parentEnabled);
	virtual ~EffectNoiseFS(void);

	virtual void release(void)
	{
		delete this;
	}

	virtual void visualize(physx::general_renderdebug4::RenderDebug* callback, bool solid) const;
	virtual void refresh(const PxTransform& parent, bool parentEnabled, bool fromSetPose, NxApexRenderVolume* renderVolume,NxApexEmitterActor::NxApexEmitterValidateCallback *callback);

	NoiseFieldSamplerEffect*	mData;
};


class EffectVortexFS : public EffectData
{
public:
	EffectVortexFS(const char* parentName,
	               VortexFieldSamplerEffect* data,
	               NxApexSDK& sdk,
	               NxApexScene& scene,
	               ParticlesScene& dscene,
	               const PxTransform& rootPose,
	               bool parentEnabled);
	virtual ~EffectVortexFS(void);

	virtual void release(void)
	{
		delete this;
	}

	virtual void visualize(physx::general_renderdebug4::RenderDebug* callback, bool solid) const;
	virtual void refresh(const PxTransform& parent, bool parentEnabled, bool fromSetPose, NxApexRenderVolume* renderVolume,NxApexEmitterActor::NxApexEmitterValidateCallback *callback);

	VortexFieldSamplerEffect*	mData;
};



class EffectAttractorFS : public EffectData
{
public:
	EffectAttractorFS(const char* parentName,
	                  AttractorFieldSamplerEffect* data,
	                  NxApexSDK& sdk,
	                  NxApexScene& scene,
	                  ParticlesScene& dscene,
	                  const PxTransform& rootPose,
	                  bool parentEnabled);

	virtual ~EffectAttractorFS(void);

	virtual void release(void)
	{
		delete this;
	}

	virtual void visualize(physx::general_renderdebug4::RenderDebug* callback, bool solid) const;
	virtual void refresh(const PxTransform& parent, bool parentEnabled, bool fromSetPose, NxApexRenderVolume* renderVolume,NxApexEmitterActor::NxApexEmitterValidateCallback *callback);

	AttractorFieldSamplerEffect*		mData;
};


class EffectPackageActor : public NxEffectPackageActor, public physx::shdfnd::UserAllocated, public ParticlesBase
{
public:
	EffectPackageActor(NxEffectPackageAsset* asset,
	                   const EffectPackageAssetParams* assetParams,
	                   const EffectPackageActorParams* actorParams,
	                   physx::apex::NxApexSDK& sdk,
	                   physx::apex::NxApexScene& scene,
	                   ParticlesScene& dynamicSystemScene,
					   NxModuleTurbulenceFS* moduleTurbulenceFS);

	virtual ~EffectPackageActor(void);

	virtual ParticlesType getParticlesType(void) const
	{
		return ParticlesBase::DST_EFFECT_PACKAGE_ACTOR;
	}

	void updateParticles(PxF32 dtime);
	void updatePoseAndBounds(bool screenCulling, bool znegative);

	virtual void setPose(const physx::PxTransform& pose);
	virtual const PxTransform& getPose(void) const;
	virtual void visualize(physx::general_renderdebug4::RenderDebug* callback, bool solid) const;

	virtual void refresh(void);
	virtual void release(void);

	virtual const char* getName(void) const;

	virtual PxU32 getEffectCount(void) const; // returns the number of effects in the effect package
	virtual EffectType getEffectType(PxU32 effectIndex) const; // return the type of effect.
	virtual NxApexActor* getEffectActor(PxU32 effectIndex) const; // return the base NxApexActor pointer
	virtual void setEmitterState(bool state); // set the state for all emitters in this effect package.
	virtual PxU32 getActiveParticleCount(void) const; // return the total number of particles still active in this effect package.
	virtual bool isStillEmitting(void) const; // return true if any emitters are still actively emitting particles.


	/**
	\brief Returns the name of the effect at this index.

	\param [in] effectIndex : The effect number to refer to; must be less than the result of getEffectCount
	*/
	virtual const char* getEffectName(PxU32 effectIndex) const;

	/**
	\brief Returns true if this sub-effect is currently enabled.

	\param [in] effectIndex : The effect number to refer to; must be less than the result of getEffectCount
	*/
	virtual bool isEffectEnabled(PxU32 effectIndex) const;

	/**
	\brief Set's the enabled state of this sub-effect

	\param [in] effectIndex : The effect number to refer to; must be less than the result of getEffectCount
	\param [in] state : Whether the effect should be enabled or not.
	*/
	virtual bool setEffectEnabled(PxU32 effectIndex, bool state);

	/**
	\brief Returns the pose of this sub-effect; returns as a a bool the active state of this effect.

	\param [in] effectIndex : The effect number to refer to; must be less than the result of getEffectCount
	\param [pose] : Contains the pose requested
	\param [worldSpace] : Whether to return the pose in world-space or in parent-relative space.
	*/
	virtual bool getEffectPose(PxU32 effectIndex, PxTransform& pose, bool worldSpace);

	/**
	\brief Sets the pose of this sub-effect; returns as a a bool the active state of this effect.

	\param [in] effectIndex : The effect number to refer to; must be less than the result of getEffectCount
	\param [pose] : Contains the pose to be set
	\param [worldSpace] : Whether to return the pose in world-space or in parent-relative space.
	*/
	virtual bool setEffectPose(PxU32 effectIndex, const PxTransform& pose, bool worldSpace);

	/**
	\brief Returns the current lifetime of the particle.
	*/
	virtual PxF32 getCurrentLife(void) const;


	virtual PxF32 getDuration(void) const;

	/**
	\brief Returns the owning asset
	*/
	virtual NxApexAsset* getOwner() const
	{
		return mAsset;
	}

	/**
	\brief Returns the range of possible values for physical Lod overwrite

	\param [out] min		The minimum lod value
	\param [out] max		The maximum lod value
	\param [out] intOnly	Only integers are allowed if this is true, gets rounded to nearest

	\note The max value can change with different graphical Lods
	\see NxApexActor::forcePhysicalLod()
	*/
	virtual void getPhysicalLodRange(physx::PxF32& min, physx::PxF32& max, bool& intOnly)
	{
		min = 0;
		max = 100000;
		intOnly = false;
	}

	/**
	\brief Get current physical lod.
	*/
	virtual physx::PxF32 getActivePhysicalLod()
	{
		return 0;
	}

	/**
	\brief Force an APEX Actor to use a certian physical Lod

	\param [in] lod	Overwrite the Lod system to use this Lod.

	\note Setting the lod value to a negative number will turn off the overwrite and proceed with regular Lod computations
	\see NxApexActor::getPhysicalLodRange()
	*/
	virtual void forcePhysicalLod(physx::PxF32 lod)
	{
		PX_UNUSED(lod);
	}

	/**
	\brief Ensure that all module-cached data is cached.
	*/
	virtual void cacheModuleData() const
	{

	}

	virtual void setEnabled(bool state)
	{
		mEnabled = state;
		refresh();
	}

	bool getEnabled(void) const
	{
		return mEnabled;
	}

#if NX_SDK_VERSION_MAJOR == 2
	// NxScene pointer may be NULL
	virtual void		setPhysXScene(NxScene* s)
	{
		mPhysXScene = s;
	}

	virtual NxScene*	getPhysXScene() const
	{
		return mPhysXScene;
	}
#elif NX_SDK_VERSION_MAJOR == 3
	virtual void		setPhysXScene(PxScene* s)
	{
		mPhysXScene = s;
	}
	virtual PxScene*	getPhysXScene() const
	{
		return mPhysXScene;
	}
#endif

	physx::PxF32 internalGetDuration(void);

	virtual bool isAlive(void) const
	{
		return mAlive;
	}

	virtual void fadeOut(physx::PxF32 fadeTime)
	{
		if (!mFadeOut)
		{
			mFadeOutTime = fadeTime;
			mFadeOutDuration = 0;

			if (mFadeIn)
			{
				PxF32 fadeLerp = mFadeInDuration / mFadeInTime;
				if (fadeLerp > 1)
				{
					fadeLerp = 1;
				}
				mFadeOutDuration = 1 - (fadeLerp * mFadeOutTime);
				mFadeIn = false;
			}

			mFadeOut = true;
		}
	}

	virtual void fadeIn(physx::PxF32 fadeTime)
	{
		if (!mFadeIn)
		{
			mFadeInTime = fadeTime;
			mFadeInDuration = 0;
			mFadeIn = true;
			if (mFadeOut)
			{
				PxF32 fadeLerp = mFadeOutDuration / mFadeOutTime;
				if (fadeLerp > 1)
				{
					fadeLerp = 1;
				}
				mFadeInDuration = 1 - (fadeLerp * mFadeInTime);
				mFadeOut = false;
			}
		}
	}

	virtual void                 setPreferredRenderVolume(NxApexRenderVolume* volume);

	virtual const char * hasTurbulenceVolumeRenderMaterial(physx::PxU32 &index) const;

	virtual void setApexEmitterValidateCallback(NxApexEmitterActor::NxApexEmitterValidateCallback *callback)
	{
		mEmitterValidateCallback = callback;
	}

private:

#if NX_SDK_VERSION_MAJOR == 2
	NxScene*					mPhysXScene;
#elif NX_SDK_VERSION_MAJOR == 3
	physx::PxScene*			mPhysXScene;
#endif

	EffectType getEffectType(const NxParameterized::Interface* iface);

	bool						mAlive:1;
	bool						mEnabled: 1;
	bool						mVisible: 1;
	bool						mEverVisible: 1;
	bool						mFirstFrame: 1;
	bool						mFadeOut: 1;
	PxF32						mFadeOutTime;
	PxF32						mFadeOutDuration;

	bool						mFadeIn: 1;
	PxF32						mFadeInTime;
	PxF32						mFadeInDuration;

	NxApexEmitterActor::NxApexEmitterValidateCallback *mEmitterValidateCallback;

	PxF32						mFadeTime;
	PxF32                       mNotVisibleTime;
	VisState					mVisState;
	PxF32						mOffScreenTime;

	PxTransform					mPose;
	const EffectPackageAssetParams*	mData;
	Array< EffectData* >		mEffects;
	PxF32                       mSimTime;
	PxF32						mCurrentLifeTime;

	physx::apex::NxApexScene*	mScene;
	NxModuleTurbulenceFS*		mModuleTurbulenceFS;
	NxEffectPackageAsset*	mAsset;
	NxApexRenderVolume*	mRenderVolume;
};

} // end of particles namespace
} // end of apex namespace
} // end of physx namespace

#endif
