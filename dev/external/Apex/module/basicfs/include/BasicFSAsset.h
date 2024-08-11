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

#ifndef BASIC_FS_ASSET_H
#define BASIC_FS_ASSET_H

#include "NxApex.h"

#include "NxBasicFSAsset.h"
#include "ApexSDKHelpers.h"
#include "ApexInterface.h"
#include "ModuleBasicFS.h"
#include "ApexAssetAuthoring.h"
#include "ApexString.h"
#include "ApexAssetTracker.h"
#include "ApexAuthorableObject.h"

#include "NiFieldBoundary.h"

namespace physx
{
namespace apex
{

class NxRenderMeshAsset;

namespace basicfs
{

class BasicFSActor;

///p,q -> p cross q = n (n - must be normalized!)
PX_INLINE void BuildPlaneBasis(const PxVec3& n, PxVec3& p, PxVec3& q)
{
	PxReal anz = PxAbs(n.z);
	if (anz * anz > 0.5)
	{
		// choose p in y-z plane
		float k = sqrtf(n.y * n.y + n.z * n.z);
		p.x = 0;
		p.y = -n.z / k;
		p.z = n.y / k;
		// set q = n cross p
		q.x = k;
		q.y = -n.x * p.z;
		q.z = n.x * p.y;
	}
	else
	{
		// choose p in x-y plane
		float k = PxSqrt(n.x * n.x + n.y * n.y);
		p.x = -n.y / k;
		p.y = n.x / k;
		p.z = 0;
		// set q = n cross p
		q.x = -n.z * p.y;
		q.y = n.z * p.x;
		q.z = k;
	}
}

class BasicFSAsset : public NxBasicFSAsset, public NxApexResource, public ApexResource
{
	friend class BasicFSAssetDummyAuthoring;
public:
	BasicFSAsset(ModuleBasicFS*, const char*);
	virtual ~BasicFSAsset();

	/* NxApexAsset */
	const char* 			getName() const
	{
		return mName.c_str();
	}

	// TODO: implement forceLoadAssets
	PxU32					forceLoadAssets()
	{
		return 0;
	}

	/* NxApexResource, ApexResource */
	PxU32					getListIndex() const
	{
		return m_listIndex;
	}
	void					setListIndex(class NxResourceList& list, PxU32 index)
	{
		m_list = &list;
		m_listIndex = index;
	}

	/**
	* \brief Apply any changes that may been made to the NxParameterized::Interface on this asset.
	*/
	virtual void applyEditingChanges(void)
	{
		APEX_INVALID_OPERATION("Not yet implemented!");
	}

	NxParameterized::Interface* getDefaultActorDesc() = 0;
	virtual NxApexActor* createApexActor(const NxParameterized::Interface& /*parms*/, NxApexScene& /*apexScene*/) = 0;

	NxParameterized::Interface* getDefaultAssetPreviewDesc()
	{
		APEX_INVALID_OPERATION("Not yet implemented!");
		return NULL;
	}

	virtual NxApexAssetPreview* createApexAssetPreview(const NxParameterized::Interface& /*params*/, NxApexAssetPreviewScene* /*previewScene*/)
	{
		APEX_INVALID_OPERATION("Not yet implemented!");
		return NULL;
	}

	virtual bool isValidForActorCreation(const ::NxParameterized::Interface& /*parms*/, NxApexScene& /*apexScene*/) const
	{
		return true; // todo implement this method
	}

	virtual bool isDirty() const
	{
		return false;
	}


protected:

	ModuleBasicFS* 				mModule;
	NxResourceList				mFSActors;
	ApexSimpleString			mName;

	friend class ModuleBasicFS;
	friend class BasicFSActor;
};

}
}
} // end namespace physx::apex

#endif // BASIC_FS_ASSET_H
