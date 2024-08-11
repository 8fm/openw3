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

#ifndef GROUND_EMITTER_ASSET_H
#define GROUND_EMITTER_ASSET_H

#include "NxGroundEmitterAsset.h"
#include "NxGroundEmitterActor.h"
#include "ApexInterface.h"
#include "ApexSDKHelpers.h"
#include "ApexAssetAuthoring.h"
#include "ApexAssetTracker.h"
#include "ApexString.h"
#include "NxEmitterLodParamDesc.h"
#include "NiInstancedObjectSimulation.h"
#include "NiResourceProvider.h"
#include "NxIofxAsset.h"
#include "PsShare.h"
// NxParam test
#include "NxParameterized.h"
#include "NxParamArray.h"
#include "GroundEmitterAssetParameters.h"
#include "GroundEmitterActorParameters.h"
#include "EmitterAssetPreviewParameters.h"

namespace physx
{
namespace apex
{

class NxGroundEmitterActor;
class NxGroundEmitterPreview;

namespace emitter
{

class GroundEmitterActor;
class ModuleEmitter;

///Class used to bind the emitter to a specific material
class NxMaterialFactoryMappingDesc : NxApexDesc
{
public:
	/** \brief User defined material name.
	 * The ground emitter will convert this name into an
	 * NxMaterialID at runtime by a getResource() call to the named resource provider.
	 * Note this is the physical material, not rendering material.
	 */
	const char* physicalMaterialName;

	/** \brief The name of the instanced object effects asset that will render your particles */
	const char* instancedObjectEffectsAssetName;

	/** \brief The asset name of the particle system that will simulate your particles */
	const char* instancedObjectSimulationAssetName;

	/** \brief The asset type of the particle system that will simulate your particles, aka 'NxFluidIosAsset' */
	const char* instancedObjectSimulationTypeName;

	/** \brief The weight of this factory relative to other factories on the same material */
	physx::PxF32 weight;

	/** \brief The maximum slope at which particles will be added to the surface in degrees where 0 is horizontal
	 * and 90 is vertical.
	 */
	physx::PxF32 maxSlopeAngle;

	/**
	\brief constructor sets to default.
	*/
	PX_INLINE NxMaterialFactoryMappingDesc() : NxApexDesc()
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

		if (!physicalMaterialName ||
		        !instancedObjectEffectsAssetName ||
		        !instancedObjectSimulationAssetName ||
		        !instancedObjectSimulationTypeName)
		{
			return false;
		}

		if (maxSlopeAngle > 90.0f || maxSlopeAngle < 0.0f)
		{
			return false;
		}

		return true;
	}

private:

	PX_INLINE void init()
	{
		physicalMaterialName = NULL;
		instancedObjectEffectsAssetName = NULL;
		instancedObjectSimulationAssetName = NULL;
		instancedObjectSimulationTypeName = "NxFluidIosAsset";
		weight = 1.0f;
		maxSlopeAngle = 90.0f;  // in default, spawn on even vertical faces
	}
};

///Ground Emitter actor descriptor. Used to create Ground Emitter actors
class NxGroundEmitterActorDesc : public NxApexDesc
{
public:
	physx::PxU32			raycastCollisionGroups;

	physx::PxF32			radius;						///< The ground emitter actor will create objects within a circle of size 'radius'.
	physx::PxU32			maxRaycastsPerFrame;		///< The maximum raycasts number per frame.
	physx::PxF32			raycastHeight;				///< The height from which the ground emitter will cast rays at terrain/objects opposite of the 'upDirection'.
	/**
	\brief The height above the ground to emit particles.
	 If greater than 0, the ground emitter will refresh a disc above the player's position rather than
	 refreshing a circle around the player's position.

	*/
	physx::PxF32			spawnHeight;
	NxRange<physx::PxF32> densityRange;				///< The ground emitter actor will use the maximum density in the range,
	///< but it will back off to the minimum density if the actor is LOD resource limited.
#if NX_SDK_VERSION_MAJOR == 2
	NxActor* 		attachActor;				///< The actor to whuch the emitter will be attatched
#elif NX_SDK_VERSION_MAJOR == 3
	PxActor* 		attachActor;				///< The actor to whuch the emitter will be attatched
#endif

	physx::PxVec3	attachRelativePosition;				///< The position of the emitter in the space of the actor, to which it is attatched.
	physx::PxVec3	initialPosition;					///< The position of the emitter immediately after addding to the scene.

	/**
	\brief Orientatiom of the emitter.
		 An identity matrix will result in a +Y up ground emitter, provide a
	     rotation if another orientation is desired.
	*/
	physx::PxMat34Legacy			rotation;


	/**
	\brief Pointer to callback that will be used to request materials from positions. Can be NULL; in such case a case ray will be cast
	*/
	NxMaterialLookupCallback* materialCallback;

	/**
	\brief constructor sets to default.
	*/
	PX_INLINE NxGroundEmitterActorDesc() : NxApexDesc()
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

		if (!rotation.t.isZero())
		{
			return false;
		}

		return true;
	}

private:

	PX_INLINE void init()
	{
		// authored values will be used if these defaults remain
		raycastCollisionGroups = physx::PxU32(-1);
		radius = 0.0f;
		maxRaycastsPerFrame = 0;
		raycastHeight = 0.0f;
		spawnHeight = -1.0f;
		densityRange.minimum = 0.0f;
		densityRange.maximum = 0.0f;

		attachActor = NULL;
		attachRelativePosition = physx::PxVec3(0.0f);
		initialPosition = physx::PxVec3(0.0f);
		rotation.setIdentity();

		materialCallback = NULL;
	}
};

/**
	Descriptor used to create an ApexEmitter preview.
*/
class NxGroundEmitterPreviewDesc : public NxApexDesc
{
public:
	physx::PxMat34Legacy							mPose;						// pose of the preview renderer
	physx::PxF32                          mScale;                     // scaling factor of renderable

	/**
	\brief Constructor sets to default.
	*/
	PX_INLINE NxGroundEmitterPreviewDesc() : NxApexDesc()
	{
		setToDefault();
	}

	/**
	\brief Sets parameters to default.
	*/
	PX_INLINE void setToDefault()
	{
		NxApexDesc::setToDefault();

		mPose.id();
		mScale = 1.0f;
	}

	/**
	\brief Cchecks the validity of the parameters.
	*/
	PX_INLINE bool isValid() const
	{
		return NxApexDesc::isValid();
	}
};


class GroundEmitterAsset :	public NxGroundEmitterAsset,
	public NxApexResource,
	public ApexResource
{
	friend class GroundEmitterAssetDummyAuthoring;
public:
	GroundEmitterAsset(ModuleEmitter*, NxResourceList&, const char* name);
	GroundEmitterAsset(ModuleEmitter* module,
	                   NxResourceList& list,
	                   NxParameterized::Interface* params,
	                   const char* name);
	~GroundEmitterAsset();

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
	physx::PxU32						forceLoadAssets();

	/* NxApexInterface */
	virtual void			    release();

	/* NxApexResource, ApexResource */
	physx::PxU32					    getListIndex() const
	{
		return m_listIndex;
	}
	void					    setListIndex(class NxResourceList& list, physx::PxU32 index)
	{
		m_list = &list;
		m_listIndex = index;
	}

	NxGroundEmitterActor* 		createActor(const NxGroundEmitterActorDesc&, NxApexScene&);
	void						releaseActor(NxGroundEmitterActor&);

	NxGroundEmitterPreview*	    createEmitterPreview(const NxGroundEmitterPreviewDesc& desc, NxApexAssetPreviewScene* previewScene);
	void					    releaseEmitterPreview(NxGroundEmitterPreview& preview);

	const NxRange<physx::PxF32> &		getDensityRange() const
	{
		return *((NxRange<physx::PxF32> *)(&(mParams->densityRange.min)));
	}

	const NxRange<physx::PxVec3> &     getVelocityRange() const
	{
		return *((NxRange<physx::PxVec3> *)(&(mParams->velocityRange.min.x)));
	}

	const NxRange<physx::PxF32> &     getLifetimeRange() const
	{
		return *((NxRange<physx::PxF32> *)(&(mParams->lifetimeRange.min)));
	}

	physx::PxF32                      getRadius() const
	{
		return mParams->radius;
	}
	physx::PxU32					    getMaxRaycastsPerFrame() const
	{
		return mParams->maxRaycastsPerFrame;
	}
	physx::PxF32					    getRaycastHeight() const
	{
		return mParams->raycastHeight;
	}
	physx::PxF32					    getSpawnHeight() const
	{
		return mParams->spawnHeight;
	}
	const char* 			    getRaycastCollisionGroupMaskName() const
	{
		return mParams->raycastCollisionGroupMaskName;
	}

	/* objects that assist in force loading and proper "assets own assets" behavior */
	ApexAssetTracker			mIofxAssetTracker;
	ApexAssetTracker			mIosAssetTracker;

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
	virtual NxApexActor*		createApexActor(const NxParameterized::Interface& parms, NxApexScene& apexScene);
	virtual NxApexAssetPreview* createApexAssetPreview(const NxParameterized::Interface& params, NxApexAssetPreviewScene* previewScene);

	virtual bool isValidForActorCreation(const ::NxParameterized::Interface& /*parms*/, NxApexScene& /*apexScene*/) const
	{
		return true; // TODO implement this method
	}

	virtual bool isDirty() const
	{
		return false;
	}

protected:
	/* Typical asset members */
	static const char* 		    getClassName()
	{
		return NX_GROUND_EMITTER_AUTHORING_TYPE_NAME;
	}
	static NxAuthObjTypeID	    mAssetTypeID;

	void						postDeserialize(void* userData = NULL);

	void                        destroy();
	ModuleEmitter*              mModule;
	NxResourceList              mEmitterActors;
	ApexSimpleString            mName;

	//GroundEmitterAssetParameters	mParams;
	physx::PxFileBuf&					serializeMaterialFactory(physx::PxFileBuf& stream, physx::PxU32 matFactoryIdx) const;
	physx::PxFileBuf&					deserializeMaterialFactory(physx::PxFileBuf& stream,
	        physx::PxU32 matFactoryIdx,
	        physx::PxU32 headerVersion,
	        physx::PxU32 assetVersion);

	static void					copyLodDesc(NxEmitterLodParamDesc& dst, const GroundEmitterAssetParametersNS::emitterLodParamDesc_Type& src);
	static void					copyLodDesc(GroundEmitterAssetParametersNS::emitterLodParamDesc_Type& dst, const NxEmitterLodParamDesc& src);

	NxParamArray<GroundEmitterAssetParametersNS::materialFactoryMapping_Type> *mMaterialFactoryMaps;

	void						initializeAssetNameTable();


	GroundEmitterAssetParameters*   mParams;
	GroundEmitterActorParameters*   mDefaultActorParams;
	EmitterAssetPreviewParameters*  mDefaultPreviewParams;

	physx::PxU32					mRaycastCollisionGroups; // cache lookup value
#if NX_SDK_VERSION_MAJOR == 2
	NxGroupsMask					mRaycastCollisionGroupsMask;
#elif NX_SDK_VERSION_MAJOR == 3
	physx::PxFilterData				mRaycastCollisionGroupsMask;
#endif
	bool							mShouldUseGroupsMask;

	friend class ModuleEmitter;
	friend class GroundEmitterActor;
	friend class GroundEmitterAssetPreview;
	template <class T_Module, class T_Asset, class T_AssetAuthoring> friend class ApexAuthorableObject;
};

#ifndef WITHOUT_APEX_AUTHORING
class GroundEmitterAssetAuthoring : public NxGroundEmitterAssetAuthoring, public ApexAssetAuthoring, public GroundEmitterAsset
{
protected:
	GroundEmitterAssetAuthoring(ModuleEmitter* m, NxResourceList& l) :
		GroundEmitterAsset(m, l, "GroundEmitterAuthor") {}

	GroundEmitterAssetAuthoring(ModuleEmitter* m, NxResourceList& l, const char* name) :
		GroundEmitterAsset(m, l, name) {}

	GroundEmitterAssetAuthoring(ModuleEmitter* m, NxResourceList& l, NxParameterized::Interface* params, const char* name) :
		GroundEmitterAsset(m, l, params, name) {}

	~GroundEmitterAssetAuthoring();

	NxEmitterLodParamDesc		mCurLodParamDesc;

public:
	void            release();
	const char* 			getName(void) const
	{
		return GroundEmitterAsset::getName();
	}
	const char* 	getObjTypeName() const
	{
		return GroundEmitterAsset::getClassName();
	}
	virtual bool			prepareForPlatform(physx::apex::NxPlatformTag)
	{
		APEX_INVALID_OPERATION("Not Implemented.");
		return false;
	}
	void setToolString(const char* toolName, const char* toolVersion, PxU32 toolChangelist)
	{
		ApexAssetAuthoring::setToolString(toolName, toolVersion, toolChangelist);
	}

	void			setDensityRange(const NxRange<physx::PxF32>& d)
	{
		mParams->densityRange.min = d.minimum;
		mParams->densityRange.max = d.maximum;
	}

	void            setVelocityRange(const NxRange<physx::PxVec3>& v)
	{
		mParams->velocityRange.min = v.minimum;
		mParams->velocityRange.max = v.maximum;
	}

	void            setLifetimeRange(const NxRange<physx::PxF32>& l)
	{
		mParams->lifetimeRange.min = l.minimum;
		mParams->lifetimeRange.max = l.maximum;
	}

	void            setRadius(physx::PxF32 r)
	{
		mParams->radius = r;
	}

	void            setMaxRaycastsPerFrame(physx::PxU32 m)
	{
		mParams->maxRaycastsPerFrame = m;
	}

	void            setRaycastHeight(physx::PxF32 h)
	{
		mParams->raycastHeight = h;
	}

	void            setSpawnHeight(physx::PxF32 h)
	{
		mParams->spawnHeight = h;
	}

	void            setRaycastCollisionGroupMaskName(const char* n)
	{
		NxParameterized::Handle stringHandle(*mParams);
		mParams->getParameterHandle("raycastCollisionGroupMaskName", stringHandle);
		mParams->setParamString(stringHandle, n);
	}

	void			setCurLodParamDesc(const NxEmitterLodParamDesc& d)
	{
		mCurLodParamDesc = d;
	}
	void			addMeshForGroundMaterial(const NxMaterialFactoryMappingDesc&);

	NxParameterized::Interface* getNxParameterized() const
	{
		return (NxParameterized::Interface*)getAssetNxParameterized();
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

	template <class T_Module, class T_Asset, class T_AssetAuthoring> friend class ApexAuthorableObject;
};
#endif

}
}
} // end namespace physx::apex

#endif // GROUND_EMITTER_ASSET_H
