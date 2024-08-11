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

#ifndef APEX_EMITTER_ASSET_H
#define APEX_EMITTER_ASSET_H

#include "NxApex.h"
#include "NxApexEmitterAsset.h"
#include "EmitterGeom.h"
#include "ApexInterface.h"
#include "ApexSDKHelpers.h"
#include "ApexAssetAuthoring.h"
#include "ApexAssetTracker.h"
#include "ApexString.h"
#include "NiResourceProvider.h"
#include "NiInstancedObjectSimulation.h"
#include "NxEmitterLodParamDesc.h"
#include "ApexAuthorableObject.h"
#include "ApexEmitterAssetParameters.h"
#include "ApexEmitterActorParameters.h"
#include "EmitterAssetPreviewParameters.h"
#include "PsShare.h"

namespace physx
{
namespace apex
{

namespace iofx
{
class NxIofxAsset;
}

namespace emitter
{

class ApexEmitterActor;
class ModuleEmitter;

/**
	Descriptor used to create an ApexEmitter preview.
*/
class NxApexEmitterPreviewDesc : public NxApexDesc
{
public:
	physx::PxMat34Legacy		mPose;						///< pose of the preview renderer
	physx::PxF32                mScale;                     ///< scaling factor of renderable

	/**
	\brief Constructor sets to default.
	*/
	PX_INLINE NxApexEmitterPreviewDesc() : NxApexDesc()
	{
		setToDefault();
	}

	/**
	\brief Sets to default values.
	*/
	PX_INLINE void setToDefault()
	{
		NxApexDesc::setToDefault();

		mPose.id();
		mScale = 1.0f;
	}

	/**
	\brief checks the valididty of parameters.
	*/
	PX_INLINE bool isValid() const
	{
		return NxApexDesc::isValid();
	}
};


class NxApexEmitterActorDesc : public NxApexDesc
{
public:
	physx::PxMat34Legacy	initialPose; ///< the pose of the emitter immediately after addding to the scene.

#if NX_SDK_VERSION_MAJOR == 2
	NxActor* 				attachActor; ///< the actor to which the emitter will be attatched. May be NULL to keep the emitter unattatched.
#elif NX_SDK_VERSION_MAJOR == 3
	PxActor* 				attachActor; ///< the actor to which the emitter will be attatched. May be NULL to keep the emitter unattatched.
#endif

	physx::PxMat34Legacy	attachRelativePose; ///< the pose of the emitter in the space of the actor, to which it is attatched. Overrides the initial pose.

	physx::PxU32			overlapTestCollisionGroups; ///< collision groups used to reject particles overlapping with the geometry

	bool			emitAssetParticles; ///< indicates whether authored asset particle list will be emitted, defaults to true

	physx::PxF32	emitterDuration;
	/**
	\brief constructor sets to default.
	*/
	PX_INLINE NxApexEmitterActorDesc() : NxApexDesc()
	{
		init();
	}

	/**
	\brief sets members to default values.
	*/
	PX_INLINE void setToDefault()
	{
		NxApexDesc::setToDefault();
		init();
	}

	/**
	\brief checks if this is a valid descriptor.
	*/
	PX_INLINE bool isValid() const
	{
		if (!NxApexDesc::isValid())
		{
			return false;
		}

		return true;
	}

private:

	PX_INLINE void init()
	{
		initialPose.id();
		attachActor = NULL;
		attachRelativePose.id();
		overlapTestCollisionGroups = 0;
		emitAssetParticles = true;
		emitterDuration = PX_MAX_F32;
	}
};

class ApexEmitterAsset : public NxApexEmitterAsset,
	public NxApexResource,
	public ApexResource,
	public NxParameterized::SerializationCallback
{
	friend class ApexEmitterAssetDummyAuthoring;
public:
	ApexEmitterAsset(ModuleEmitter*, NxResourceList&, const char* name);
	ApexEmitterAsset(ModuleEmitter* module,
	                 NxResourceList& list,
	                 NxParameterized::Interface* params,
	                 const char* name);

	~ApexEmitterAsset();

	/* NxApexAsset */
	const char* 			    getName() const
	{
		return mName.c_str();
	}
	NxAuthObjTypeID			    getObjTypeID() const
	{
		return mAssetTypeID;
	}
	const char* 			    getObjTypeName() const
	{
		return getClassName();
	}
	physx::PxU32				forceLoadAssets();

	/* NxApexInterface */
	virtual void			    release();

	/* NxApexResource, ApexResource */
	physx::PxU32			    getListIndex() const
	{
		return m_listIndex;
	}
	void					    setListIndex(class NxResourceList& list, physx::PxU32 index)
	{
		m_list = &list;
		m_listIndex = index;
	}

	/* NxApexEmitterAsset specific methods */

	NxEmitterExplicitGeom* 		isExplicitGeom();
	const NxEmitterExplicitGeom* 	isExplicitGeom() const;

	const NxEmitterGeom*		getGeom() const
	{
		return mGeom->getNxEmitterGeom();
	}

	const char* 				getInstancedObjectEffectsAssetName(void) const
	{
		return mParams->iofxAssetName->name();
	}
	const char* 				getInstancedObjectSimulatorAssetName(void) const
	{
		return mParams->iosAssetName->name();
	}
	const char* 				getInstancedObjectSimulatorTypeName(void) const
	{
		return mParams->iosAssetName->className();
	}

	const NxRange<physx::PxF32> &		getDensityRange() const
	{
		return *((NxRange<physx::PxF32> *)(&(mParams->densityRange.min)));
	}

	const NxRange<physx::PxF32> &		getRateRange() const
	{
		return *((NxRange<physx::PxF32> *)(&(mParams->rateRange.min)));
	}

	const NxRange<physx::PxVec3> &     getVelocityRange() const
	{
		return *((NxRange<physx::PxVec3> *)(&(mParams->velocityRange.min.x)));
	}

	const NxRange<physx::PxF32> &     getLifetimeRange() const
	{
		return *((NxRange<physx::PxF32> *)(&(mParams->lifetimeRange.min)));
	}

	physx::PxU32						getMaxSamples() const
	{
		return mParams->maxSamples;
	}

	physx::PxF32						getEmitDuration() const
	{
		return mParams->emitterDuration;
	}

	const NxEmitterLodParamDesc& getLodParamDesc() const
	{
		return mLodDesc;
	}

	NxApexEmitterActor* 	    createEmitterActor(const NxApexEmitterActorDesc&, const NxApexScene&);
	void					    releaseEmitterActor(NxApexEmitterActor&);
	void                        destroy();

	NxApexEmitterPreview*	    createEmitterPreview(const NxApexEmitterPreviewDesc& desc, NxApexAssetPreviewScene* previewScene);
	void					    releaseEmitterPreview(NxApexEmitterPreview& preview);

	const NxParameterized::Interface*			getAssetNxParameterized() const
	{
		return mParams;
	}
	/**
	 * \brief Releases the ApexAsset but returns the NxParameterized::Interface and *ownership* to the caller.
	 */
	virtual NxParameterized::Interface* releaseAndReturnNxParameterizedInterface(void)
	{
		NxParameterized::Interface* ret = mParams;
		mParams = NULL;
		release();
		return ret;
	}

	NxParameterized::Interface*			getDefaultActorDesc();
	NxParameterized::Interface*			getDefaultAssetPreviewDesc();
	virtual NxApexActor*		createApexActor(const NxParameterized::Interface& params, NxApexScene& apexScene);
	virtual NxApexAssetPreview* createApexAssetPreview(const NxParameterized::Interface& params, NxApexAssetPreviewScene* previewScene);
	virtual bool isValidForActorCreation(const ::NxParameterized::Interface& actorParams, NxApexScene& /*apexScene*/) const;

	virtual bool isDirty() const
	{
		return false;
	}

protected:
	/* Typical asset members */
	static const char* 		    getClassName()
	{
		return NX_APEX_EMITTER_AUTHORING_TYPE_NAME;
	}
	static NxAuthObjTypeID	    mAssetTypeID;

	NxIofxAsset* 			getIofxAsset();                 // callable by actors
	NiIosAsset* 				getIosAsset();                  // callable by actors

	/* NxParameterized Serialization callbacks */
	void						preSerialize(void* userData = NULL)
	{
		PX_UNUSED(userData);
	}
	void						postDeserialize(void* userData = NULL);

	void copyLodDesc2(NxEmitterLodParamDesc& dst,
	                  const ApexEmitterAssetParametersNS::emitterLodParamDesc_Type& src);
	void copyLodDesc2(ApexEmitterAssetParametersNS::emitterLodParamDesc_Type& dst,
	                  const NxEmitterLodParamDesc& src);


	ApexEmitterAssetParameters*	mParams;
	ApexEmitterActorParameters*  mDefaultActorParams;
	EmitterAssetPreviewParameters* mDefaultPreviewParams;

	ModuleEmitter*              mModule;
	NxResourceList              mEmitterActors;
	ApexSimpleString            mName;

	/* NxApexEmitterAsset specific asset members */
	EmitterGeom* 				mGeom;

	/* this lod is only for the getter and setter, it should mirror the parameterized lod desc */
	NxEmitterLodParamDesc		mLodDesc;

	/* objects that assist in force loading and proper "assets own assets" behavior */
	ApexAssetTracker			mIofxAssetTracker;
	ApexAssetTracker			mIosAssetTracker;
	void						initializeAssetNameTable();

	friend class ModuleEmitter;
	friend class ApexEmitterActor;
	friend class ApexEmitterAssetPreview;
	template <class T_Module, class T_Asset, class T_AssetAuthoring> friend class ApexAuthorableObject;
};

#ifndef WITHOUT_APEX_AUTHORING
class ApexEmitterAssetAuthoring : public ApexEmitterAsset, public ApexAssetAuthoring, public NxApexEmitterAssetAuthoring
{
public:
	/* NxApexEmitterAssetAuthoring */
	ApexEmitterAssetAuthoring(ModuleEmitter* m, NxResourceList& l) :
		ApexEmitterAsset(m, l, "ApexEmitterAuthor") {}

	ApexEmitterAssetAuthoring(ModuleEmitter* m, NxResourceList& l, const char* name) :
		ApexEmitterAsset(m, l, name) {}

	ApexEmitterAssetAuthoring(ModuleEmitter* m, NxResourceList& l, NxParameterized::Interface* params, const char* name) :
		ApexEmitterAsset(m, l, params, name) {}

	~ApexEmitterAssetAuthoring() {}

	NxEmitterBoxGeom* 			setBoxGeom();
	NxEmitterSphereGeom* 		setSphereGeom();
	NxEmitterSphereShellGeom* 	setSphereShellGeom();
	NxEmitterExplicitGeom* 		setExplicitGeom();

	void					setInstancedObjectEffectsAssetName(const char*);
	void					setInstancedObjectSimulatorAssetName(const char*);
	void					setInstancedObjectSimulatorTypeName(const char*);

	// Scalable parameters
	/* Set density to 1.0,1.0 if using explicit geometry with no density limits */
	void					setDensityRange(const NxRange<physx::PxF32>& r)
	{
		mParams->densityRange.min = r.minimum;
		mParams->densityRange.max = r.maximum;
	}

	void					setRateRange(const NxRange<physx::PxF32>& r)
	{
		mParams->rateRange.min = r.minimum;
		mParams->rateRange.max = r.maximum;
	}


	// Noise parameters
	void					setVelocityRange(const NxRange<physx::PxVec3>& v)
	{
		mParams->velocityRange.min.x = v.minimum.x;
		mParams->velocityRange.min.y = v.minimum.y;
		mParams->velocityRange.min.z = v.minimum.z;
		mParams->velocityRange.max.x = v.maximum.x;
		mParams->velocityRange.max.y = v.maximum.y;
		mParams->velocityRange.max.z = v.maximum.z;
	}

	void					setLifetimeRange(const NxRange<physx::PxF32>& l)
	{
		mParams->lifetimeRange.min = l.minimum;
		mParams->lifetimeRange.max = l.maximum;
	}


	/* Max samples is ignored if using explicit geometry */
	void					setMaxSamples(physx::PxU32 max)
	{
		mParams->maxSamples = max;
	}

	void					setLodParamDesc(const NxEmitterLodParamDesc& d)
	{
		mLodDesc = d;
		copyLodDesc2(mParams->lodParamDesc, d);
	}

	const NxEmitterGeom*		getGeom() const
	{
		return ApexEmitterAsset::getGeom();
	}
	const char* 				getInstancedObjectEffectsAssetName(void) const
	{
		return ApexEmitterAsset::getInstancedObjectEffectsAssetName();
	}
	const char* 				getInstancedObjectSimulatorAssetName(void) const
	{
		return ApexEmitterAsset::getInstancedObjectSimulatorAssetName();
	}
	const char* 				getInstancedObjectSimulatorTypeName(void) const
	{
		return ApexEmitterAsset::getInstancedObjectSimulatorTypeName();
	}
	const NxRange<physx::PxF32>& getDensityRange() const
	{
		return ApexEmitterAsset::getDensityRange();
	}
	const NxRange<physx::PxF32>& getRateRange() const
	{
		return ApexEmitterAsset::getRateRange();
	}
	const NxRange<physx::PxVec3>& getVelocityRange() const
	{
		return ApexEmitterAsset::getVelocityRange();
	}
	const NxRange<physx::PxF32>& getLifetimeRange() const
	{
		return ApexEmitterAsset::getLifetimeRange();
	}
	PxU32						 getMaxSamples() const
	{
		return ApexEmitterAsset::getMaxSamples();
	}
	const NxEmitterLodParamDesc& getLodParamDesc() const
	{
		return ApexEmitterAsset::getLodParamDesc();
	}

	/* NxApexAssetAuthoring */
	const char* 			getName(void) const
	{
		return ApexEmitterAsset::getName();
	}
	const char* 			getObjTypeName() const
	{
		return ApexEmitterAsset::getClassName();
	}
	virtual bool			prepareForPlatform(physx::apex::NxPlatformTag)
	{
		APEX_INVALID_OPERATION("Not Implemented.");
		return false;
	}
	void					setToolString(const char* toolName, const char* toolVersion, PxU32 toolChangelist)
	{
		ApexAssetAuthoring::setToolString(toolName, toolVersion, toolChangelist);
	}
	NxParameterized::Interface* getNxParameterized() const
	{
		return (NxParameterized::Interface*)getAssetNxParameterized();
	}

	/**
	* \brief Releases the ApexAsset but returns the NxParameterized::Interface and *ownership* to the caller.
	*/
	virtual NxParameterized::Interface* releaseAndReturnNxParameterizedInterface(void)
	{
		NxParameterized::Interface* ret = getNxParameterized();
		mParams = NULL;
		release();
		return ret;
	}


	/* NxApexInterface */
	virtual void			release();
};
#endif

}
}
} // end namespace physx::apex

#endif // APEX_EMITTER_ASSET_H
