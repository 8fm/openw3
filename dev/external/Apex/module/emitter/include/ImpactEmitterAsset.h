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

#ifndef IMPACT_EMITTER_ASSET_H
#define IMPACT_EMITTER_ASSET_H

#include "NxImpactEmitterAsset.h"
#include "ApexInterface.h"
#include "ApexSDKHelpers.h"
#include "ApexAssetAuthoring.h"
#include "ApexAssetTracker.h"
#include "ApexString.h"
#include "NiResourceProvider.h"
#include "NxEmitterLodParamDesc.h"
#include "ApexAuthorableObject.h"
#include "ImpactEmitterAssetParameters.h"
#include "ImpactEmitterActorParameters.h"
#include "EmitterAssetPreviewParameters.h"
#include "ImpactObjectEvent.h"

#if NX_SDK_VERSION_MAJOR == 2
#include "ImpactExplosionEvent.h"
#include "ExplosionAsset.h"
#endif

#include "PsShare.h"

namespace physx
{
namespace apex
{
namespace emitter
{

class ImpactEmitterActor;
class ModuleEmitter;

///Impact emitter actor descriptor. Used to create Impact emitter actors.
class NxImpactEmitterActorDesc : public NxApexDesc
{
public:
#if NX_SDK_VERSION_MAJOR == 2
	///Impact force field settings
	explosion::NxExplosionEnvSettings explosionEnv;
#endif

	/**
	\brief constructor sets to default.
	*/
	PX_INLINE NxImpactEmitterActorDesc() : NxApexDesc()
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
#if NX_SDK_VERSION_MAJOR == 2
		// authored values will be used if these defaults remain
		// TODO: Initialize Member Variables
		explosionEnv.setToDefault();
#endif
	}
};


/**
	Descriptor used to create a Destructible preview.
*/
class NxImpactEmitterPreviewDesc : public NxApexDesc
{
public:

	/**
	\brief Constructor sets to default.
	*/
	PX_INLINE		NxImpactEmitterPreviewDesc()
	{
		setToDefault();
	}


	/**
	\brief Resets descriptor to default settings.
	*/
	PX_INLINE void	setToDefault()
	{
		NxApexDesc::setToDefault();

		mPose.id();
		mScale = 1.0f;
	}

	/**
		Returns true iff an object can be created using this descriptor.
	*/
	PX_INLINE bool	isValid() const
	{
		return NxApexDesc::isValid();
	}

	/**
		Initial global pose of preview mesh
	*/
	physx::PxMat34Legacy	mPose;
	physx::PxReal			mScale;
};


class ImpactEmitterAsset :	public NxImpactEmitterAsset,
	public NxApexResource,
	public ApexResource,
	public NxParameterized::SerializationCallback
{
	friend class ImpactEmitterAssetDummyAuthoring;
public:

	ImpactEmitterAsset(ModuleEmitter*, NxResourceList&, const char* name);
	ImpactEmitterAsset(ModuleEmitter* module,
	                   NxResourceList& list,
	                   NxParameterized::Interface* params,
	                   const char* name);

	~ImpactEmitterAsset();

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
	physx::PxU32					    getListIndex() const
	{
		return m_listIndex;
	}
	void					    setListIndex(class NxResourceList& list, physx::PxU32 index)
	{
		m_list = &list;
		m_listIndex = index;
	}

	physx::PxU32                querySetID(const char* setName);
	void						getSetNames(const char** outSetNames, physx::PxU32& inOutNameCount);

	const NxParameterized::Interface* getAssetNxParameterized() const
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
	NxParameterized::Interface* getDefaultActorDesc();
	NxParameterized::Interface* getDefaultAssetPreviewDesc();
	virtual NxApexActor* createApexActor(const NxParameterized::Interface& parms, NxApexScene& apexScene);
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
		return NX_IMPACT_EMITTER_AUTHORING_TYPE_NAME;
	}
	static NxAuthObjTypeID	    mAssetTypeID;
	void						initializeAssetNameTable();
	void                        destroy();

	/* NxParameterized Serialization callbacks */
	void						preSerialize(void* userData = NULL);
	void						postDeserialize(void* userData = NULL);
	void						buildEventNameIndexMap();

	ModuleEmitter*              mModule;
	NxResourceList              mEmitterActors;
	ApexSimpleString            mName;

	/* objects that assist in force loading and proper "assets own assets" behavior */
	ApexAssetTracker			mIofxAssetTracker;
	ApexAssetTracker			mIosAssetTracker;
#if NX_SDK_VERSION_MAJOR == 2
	ApexAssetTracker			mExplosionAssetTracker;
#endif

	/* asset values */
	class EventNameIndexMap : public physx::UserAllocated
	{
	public:
		ApexSimpleString		eventSetName;
		physx::Array<physx::PxU16>	eventIndices;
	};

	ImpactEmitterAssetParameters*   	mParams;
	ImpactEmitterActorParameters*   	mDefaultActorParams;
	EmitterAssetPreviewParameters*      mDefaultPreviewParams;
	physx::Array<EventNameIndexMap*> mEventNameIndexMaps;

	friend class ModuleEmitter;
	friend class ImpactEmitterActor;
	friend class ImpactEmitterParticleEvent;
#if NX_SDK_VERSION_MAJOR == 2
	friend class ImpactEmitterExplosionEvent;
#endif
	template <class T_Module, class T_Asset, class T_AssetAuthoring> friend class ApexAuthorableObject;
};

#ifndef WITHOUT_APEX_AUTHORING
class ImpactEmitterAssetAuthoring : public NxImpactEmitterAssetAuthoring, public ApexAssetAuthoring, public ImpactEmitterAsset
{
protected:
	ImpactEmitterAssetAuthoring(ModuleEmitter* m, NxResourceList& l) :
		ImpactEmitterAsset(m, l, "ImpactEmitterAuthor") {}

	ImpactEmitterAssetAuthoring(ModuleEmitter* m, NxResourceList& l, const char* name) :
		ImpactEmitterAsset(m, l, name) {}

	ImpactEmitterAssetAuthoring(ModuleEmitter* m, NxResourceList& l, NxParameterized::Interface* params, const char* name) :
		ImpactEmitterAsset(m, l, params, name) {}

public:
	void			release();
	const char* 			getName(void) const
	{
		return ImpactEmitterAsset::getName();
	}
	const char* 	getObjTypeName() const
	{
		return ImpactEmitterAsset::getClassName();
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

#endif // IMPACT_EMITTER_ASSET_H
