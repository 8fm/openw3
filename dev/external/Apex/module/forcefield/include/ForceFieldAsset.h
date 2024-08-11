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

#ifndef FORCEFIELD_ASSET_H
#define FORCEFIELD_ASSET_H

#include "NxApex.h"
#include "PsShare.h"
#include "NxForceFieldAsset.h"
#include "NxForceFieldActor.h"
#include "NxForceFieldPreview.h"
#include "ApexSDKHelpers.h"
#include "ApexInterface.h"
#include "ModuleForceField.h"
#include "ApexAssetAuthoring.h"
#include "ApexString.h"
#include "ApexAssetTracker.h"
#include "ApexAuthorableObject.h"
#include "ForceFieldAssetParams.h"

namespace physx
{
namespace apex
{
namespace forcefield
{

class NxForceFieldActorDesc : public NxApexDesc
{
public:
	physx::NxGroupsMask64 samplerFilterData;
	physx::NxGroupsMask64 boundaryFilterData;
	physx::PxMat44 initialPose;
	physx::PxF32	 scale;
	PxActor* nxActor;
	const char* actorName;

	/**
	\brief constructor sets to default.
	*/
	PX_INLINE NxForceFieldActorDesc() : NxApexDesc()
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
		initialPose = PxMat44::createIdentity();
		scale = 1.0f;
		nxActor = NULL;
		actorName = NULL;
	}
};

/**
\brief Descriptor for a ForceField Asset
*/
class NxForceFieldPreviewDesc
{
public:
	NxForceFieldPreviewDesc() :
		mPose(physx::PxMat44()),
		mIconScale(1.0f),
		mPreviewDetail(APEX_FORCEFIELD::FORCEFIELD_DRAW_ICON)
	{
		mPose = PxMat44::createIdentity();
	};

	/**
	\brief The pose that translates from explosion preview coordinates to world coordinates.
	*/
	physx::PxMat44							mPose;
	/**
	\brief The scale of the icon.
	*/
	physx::PxF32							mIconScale;
	/**
	\brief The detail options of the preview drawing
	*/
	physx::PxU32							mPreviewDetail;
};


class ForceFieldActor;

class ForceFieldAsset : public NxForceFieldAsset, public NxApexResource, public ApexResource
{
	friend class ForceFieldAssetDummyAuthoring;
public:
	ForceFieldAsset(ModuleForceField*, NxResourceList&, const char* name);
	ForceFieldAsset(ModuleForceField*, NxResourceList&, NxParameterized::Interface*, const char*);

	~ForceFieldAsset();

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
	virtual void					release()
	{
		mModule->mSdk->releaseAsset(*this);
	}

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

	/* NxForceFieldAsset specific methods */
	void							releaseForceFieldActor(NxForceFieldActor&);
	const ForceFieldAssetParams&	getForceFieldParameters() const
	{
		return *mParams;
	}
	physx::PxF32					getDefaultScale() const
	{
		return mParams->defScale;
	}
	void							destroy();
	NxForceFieldPreview*			createForceFieldPreview(const NxForceFieldPreviewDesc& desc, NxApexAssetPreviewScene* previewScene);
	NxForceFieldPreview*			createForceFieldPreviewImpl(const NxForceFieldPreviewDesc& desc, ForceFieldAsset* ForceFieldAsset, NxApexAssetPreviewScene* previewScene);
	void							releaseForceFieldPreview(NxForceFieldPreview& preview);

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
	virtual NxApexActor* createApexActor(const NxParameterized::Interface& /*parms*/, NxApexScene& /*apexScene*/);
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
	static const char* 				getClassName()
	{
		return NX_FORCEFIELD_AUTHORING_TYPE_NAME;
	}
	static NxAuthObjTypeID			mAssetTypeID;

	ModuleForceField*				mModule;

	NxResourceList					mForceFieldActors;
	ApexSimpleString				mName;
	ForceFieldAssetParams*			mParams;
	ForceFieldActorParams*			mDefaultActorParams;
	ForceFieldAssetPreviewParams*	mDefaultPreviewParams;

	void							initializeAssetNameTable();

	friend class ModuleForceField;
	friend class ForceFieldActor;
	template <class T_Module, class T_Asset, class T_AssetAuthoring> friend class ApexAuthorableObject;
};

#ifndef WITHOUT_APEX_AUTHORING
class ForceFieldAssetAuthoring : public ForceFieldAsset, public ApexAssetAuthoring, public NxForceFieldAssetAuthoring
{
public:
	/* NxForceFieldAssetAuthoring */
	ForceFieldAssetAuthoring(ModuleForceField* m, NxResourceList& l) :
		ForceFieldAsset(m, l, "ForceFieldAssetAuthoring") {}

	ForceFieldAssetAuthoring(ModuleForceField* m, NxResourceList& l, const char* name) :
		ForceFieldAsset(m, l, name) {}

	ForceFieldAssetAuthoring(ModuleForceField* m, NxResourceList& l, NxParameterized::Interface* params, const char* name) :
		ForceFieldAsset(m, l, params, name) {}

	~ForceFieldAssetAuthoring() {}

	void							destroy()
	{
		delete this;
	}

	/* NxApexAssetAuthoring */
	const char* 					getName(void) const
	{
		return ForceFieldAsset::getName();
	}
	const char* 					getObjTypeName() const
	{
		return ForceFieldAsset::getClassName();
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
	virtual NxParameterized::Interface* releaseAndReturnNxParameterizedInterface(void)
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

#endif // FORCEFIELD_ASSET_H
