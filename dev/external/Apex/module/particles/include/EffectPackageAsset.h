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

#ifndef PARTICLES_EFFECT_PACKAGE_ASSET_H
#define PARTICLES_EFFECT_PACKAGE_ASSET_H

#include "NxApex.h"
#include "PsShare.h"
#include "NxEffectPackageAsset.h"
#include "NxEffectPackageActor.h"
#include "ApexSDKHelpers.h"
#include "ApexInterface.h"
#include "ModuleParticles.h"
#include "ApexAssetAuthoring.h"
#include "ApexString.h"
#include "ApexAssetTracker.h"
#include "ApexAuthorableObject.h"
#include "EffectPackageAssetParams.h"

namespace physx
{
namespace apex
{
namespace particles
{

class EffectPackageActor;
class ModuleParticles;

class EffectPackageAsset : public NxEffectPackageAsset, public NxApexResource, public ApexResource
{
	friend class EffectPackageAssetDummyAuthoring;
public:
	EffectPackageAsset(ModuleParticles*, NxResourceList&, const char* name);
	EffectPackageAsset(ModuleParticles*, NxResourceList&, NxParameterized::Interface*, const char*);

	~EffectPackageAsset();

	/* NxApexAsset */
	const char* 					getName() const
	{
		return mName.c_str();
	}
	NxAuthObjTypeID					getObjTypeID() const
	{
		return mAssetTypeID;
	}
	const char* 					getObjTypeName() const
	{
		return getClassName();
	}

	physx::PxU32					forceLoadAssets();

	/* NxApexInterface */
	virtual void					release();

	/* NxApexResource, ApexResource */
	physx::PxU32					getListIndex() const
	{
		return m_listIndex;
	}

	void							setListIndex(class NxResourceList& list, physx::PxU32 index)
	{
		m_list = &list;
		m_listIndex = index;
	}

	/* NxEffectPackageAsset specific methods */
	void							releaseEffectPackageActor(NxEffectPackageActor&);
	const EffectPackageAssetParams&	getEffectPackageParameters() const
	{
		return *mParams;
	}
	physx::PxF32					getDefaultScale() const
	{
		return 1;
	}
	void							destroy();

	const NxParameterized::Interface* getAssetNxParameterized() const
	{
		return mParams;
	}

	virtual PxF32 getDuration() const;
	virtual bool useUniqueRenderVolume() const;

	/**
	 * \brief Releases the ApexAsset but returns the NxParameterized::Interface and *ownership* to the caller.
	 */
	virtual NxParameterized::Interface* releaseAndReturnNxParameterizedInterface()
	{
		NxParameterized::Interface* ret = mParams;
		mParams = NULL;
		release();
		return ret;
	}
	NxParameterized::Interface* getDefaultActorDesc();
	NxParameterized::Interface* getDefaultAssetPreviewDesc();
	virtual NxApexActor* createApexActor(const NxParameterized::Interface& /*parms*/, NxApexScene& /*apexScene*/);
	virtual NxApexAssetPreview* createApexAssetPreview(const ::NxParameterized::Interface& /*params*/, NxApexAssetPreviewScene* /*previewScene*/)
	{
		PX_ALWAYS_ASSERT();
		return NULL;
	}

	virtual bool isValidForActorCreation(const ::NxParameterized::Interface& /*parms*/, NxApexScene& /*apexScene*/) const
	{
		return true; // TODO implement this method
	}

	virtual bool isDirty() const
	{
		return false;
	}

	static NxAuthObjTypeID			mAssetTypeID;

protected:
	static const char* 				getClassName()
	{
		return NX_PARTICLES_EFFECT_PACKAGE_AUTHORING_TYPE_NAME;
	}

	ModuleParticles*				mModule;

	NxResourceList					mEffectPackageActors;
	ApexSimpleString				mName;
	EffectPackageAssetParams*			mParams;
	EffectPackageActorParams*			mDefaultActorParams;

	void							initializeAssetNameTable();

	friend class ModuleParticlesE;
	friend class EffectPackageActor;
	template <class T_Module, class T_Asset, class T_AssetAuthoring> friend class ApexAuthorableObject;
};

#ifndef WITHOUT_APEX_AUTHORING
class EffectPackageAssetAuthoring : public EffectPackageAsset, public ApexAssetAuthoring, public NxEffectPackageAssetAuthoring
{
public:
	/* NxEffectPackageAssetAuthoring */
	EffectPackageAssetAuthoring(ModuleParticles* m, NxResourceList& l) :
		EffectPackageAsset(m, l, "EffectPackageAssetAuthoring") {}

	EffectPackageAssetAuthoring(ModuleParticles* m, NxResourceList& l, const char* name) :
		EffectPackageAsset(m, l, name) {}

	EffectPackageAssetAuthoring(ModuleParticles* m, NxResourceList& l, NxParameterized::Interface* params, const char* name) :
		EffectPackageAsset(m, l, params, name) {}

	~EffectPackageAssetAuthoring() {}

	void							destroy()
	{
		delete this;
	}

	/* NxApexAssetAuthoring */
	const char* 					getName() const
	{
		return EffectPackageAsset::getName();
	}
	const char* 					getObjTypeName() const
	{
		return EffectPackageAsset::getClassName();
	}
	virtual bool					prepareForPlatform(physx::apex::NxPlatformTag)
	{
		APEX_INVALID_OPERATION("Not Implemented.");
		return false;
	}

	void setToolString(const char* toolName, const char* toolVersion, PxU32 toolChangelist)
	{
		ApexAssetAuthoring::setToolString(toolName, toolVersion, toolChangelist);
	}

	/* NxApexInterface */
	virtual void					release()
	{
		mModule->mSdk->releaseAssetAuthoring(*this);
	}

	NxParameterized::Interface* getNxParameterized() const
	{
		return (NxParameterized::Interface*)getAssetNxParameterized();
	}
	/**
	 * \brief Releases the ApexAsset but returns the NxParameterized::Interface and *ownership* to the caller.
	 */
	virtual NxParameterized::Interface* releaseAndReturnNxParameterizedInterface()
	{
		NxParameterized::Interface* ret = mParams;
		mParams = NULL;
		release();
		return ret;
	}
};
#endif

}
}
} // end namespace physx::apex

#endif // PARTICLES_EFFECT_PACKAGE_ASSET_H
