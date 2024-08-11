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

#ifndef __ATTRACTOR_FS_ASSET_H__
#define __ATTRACTOR_FS_ASSET_H__

#include "BasicFSAsset.h"
#include "AttractorFSAssetPreview.h"
#include "AttractorFSAssetParams.h"
#include "AttractorFSActorParams.h"


namespace physx
{
namespace apex
{

class NxRenderMeshAsset;

namespace basicfs
{

class AttractorFSActor;

class AttractorFSAsset : public BasicFSAsset
{
	friend class BasicFSAssetDummyAuthoring;
public:
	AttractorFSAsset(ModuleBasicFS*, NxResourceList&, const char*);
	AttractorFSAsset(ModuleBasicFS*, NxResourceList&, NxParameterized::Interface*, const char*);
	~AttractorFSAsset();

	/* NxApexAsset */
	NxAuthObjTypeID			getObjTypeID() const
	{
		return mAssetTypeID;
	}
	const char* 			getObjTypeName() const
	{
		return getClassName();
	}

	/* NxApexInterface */
	virtual void			release()
	{
		mModule->mSdk->releaseAsset(*this);
	}

	// TODO: implement forceLoadAssets
	PxU32					forceLoadAssets()
	{
		return 0;
	}

	NxParameterized::Interface* getAssetNxParameterized() const
	{
		return mParams;
	}

	NxParameterized::Interface* releaseAndReturnNxParameterizedInterface(void)
	{
		NxParameterized::Interface* ret = mParams;
		mParams = NULL;
		release();
		return ret;
	}

	/* NxBasicFSAsset specific methods */
	void                    destroy();

	/**
	* \brief Apply any changes that may been made to the NxParameterized::Interface on this asset.
	*/
	virtual void applyEditingChanges(void)
	{
		APEX_INVALID_OPERATION("Not yet implemented!");
	}

	NxParameterized::Interface* getDefaultActorDesc();
	virtual NxApexActor* createApexActor(const NxParameterized::Interface& /*parms*/, NxApexScene& /*apexScene*/);

	NxAttractorFSPreview*	createAttractorFSPreview(const NxAttractorFSPreviewDesc& desc, NxApexAssetPreviewScene* previewScene);
	NxAttractorFSPreview*	createAttractorFSPreviewImpl(const NxAttractorFSPreviewDesc& desc, AttractorFSAsset* TurboAsset, NxApexAssetPreviewScene* previewScene);
	void					releaseAttractorFSPreview(NxAttractorFSPreview& preview);

	NxParameterized::Interface* getDefaultAssetPreviewDesc();

	virtual NxApexAssetPreview* createApexAssetPreview(const NxParameterized::Interface& /*params*/, NxApexAssetPreviewScene* previewScene);

	virtual bool isValidForActorCreation(const ::NxParameterized::Interface& /*parms*/, NxApexScene& /*apexScene*/) const
	{
		return true; // todo implement this method
	}

	/* Typical asset members */
	static const char* 			getClassName() // return to protected
	{
		return NX_ATTRACTOR_FS_AUTHORING_TYPE_NAME;
	}

protected:

	static NxAuthObjTypeID		mAssetTypeID;

	AttractorFSAssetParams*		mParams;
	AttractorFSActorParams*		mDefaultActorParams;
	AttractorFSPreviewParams*	mDefaultPreviewParams;

	friend class ModuleBasicFS;
	friend class AttractorFSActor;
	template <class T_Module, class T_Asset, class T_AssetAuthoring> friend class ApexAuthorableObject;

};

class AttractorFSAssetAuthoring : public AttractorFSAsset, public ApexAssetAuthoring, public NxBasicFSAssetAuthoring
{
public:
	/* NxBasicFSAssetAuthoring */
	AttractorFSAssetAuthoring(ModuleBasicFS* m, NxResourceList& l) :
		AttractorFSAsset(m, l, "AttractorFSAssetAuthoring") {}

	AttractorFSAssetAuthoring(ModuleBasicFS* m, NxResourceList& l, const char* name) :
		AttractorFSAsset(m, l, name) {}

	AttractorFSAssetAuthoring(ModuleBasicFS* m, NxResourceList& l, NxParameterized::Interface* params, const char* name) :
		AttractorFSAsset(m, l, params, name) {}

	~AttractorFSAssetAuthoring() {}
	void                    destroy()
	{
		delete this;
	}

	/* NxApexAssetAuthoring */
	const char* 			getName(void) const
	{
		return AttractorFSAsset::getName();
	}
	const char* 			getObjTypeName() const
	{
		return AttractorFSAsset::getClassName();
	}
	bool					prepareForPlatform(physx::apex::NxPlatformTag)
	{
		APEX_INVALID_OPERATION("Not Implemented.");
		return false;
	}

	void					setToolString(const char* toolName, const char* toolVersion, PxU32 toolChangelist)
	{
		ApexAssetAuthoring::setToolString(toolName, toolVersion, toolChangelist);
	}

	/* NxApexInterface */
	virtual void			release()
	{
		mModule->mSdk->releaseAssetAuthoring(*this);
	}

	NxParameterized::Interface* getNxParameterized() const
	{
		return AttractorFSAsset::getAssetNxParameterized();
	}

	NxParameterized::Interface* releaseAndReturnNxParameterizedInterface(void)
	{
		return AttractorFSAsset::releaseAndReturnNxParameterizedInterface();
	}

};

}
}
} // end namespace physx::apex

#endif