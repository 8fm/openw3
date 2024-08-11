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

#ifndef __FIELDBOUNDARY_ACTOR_H__
#define __FIELDBOUNDARY_ACTOR_H__

#include "NxApex.h"

#include "NxFieldBoundaryAsset.h"
#include "NxFieldBoundaryActor.h"
#include "FieldBoundaryAsset.h"
#include "ApexActor.h"
#include "ApexInterface.h"

#include "NiFieldBoundary.h"

class NxForceFieldShapeGroup;

namespace physx
{
namespace apex
{
namespace fieldboundary
{

class FieldBoundaryAsset;
class FieldBoundaryScene;

class FieldBoundaryActor : public NxFieldBoundaryActor, public ApexActor, public ApexActorSource, public NxApexResource, public ApexResource, public NiFieldBoundary
{
public:
	/* NxFieldBoundaryActor methods */
	FieldBoundaryActor(const NxFieldBoundaryActorDesc&, FieldBoundaryAsset&, NxResourceList&, FieldBoundaryScene&);
	~FieldBoundaryActor();
	NxFieldBoundaryAsset* 	getFieldBoundaryAsset() const;
	physx::PxMat34Legacy				getGlobalPose() const
	{
		return mPose;
	}
	void				setGlobalPose(const physx::PxMat34Legacy& pose);
	physx::PxVec3				getScale() const
	{
		return mScale;
	}
	void				setScale(const physx::PxVec3& scale);
	void                updatePoseAndBounds();  // Called by FieldBoundaryScene::fetchResults()

	/* NxApexActorSource; templates for generating NxActors and NxShapes */
	void				setActorTemplate(const NxActorDescBase*);
	void				setShapeTemplate(const NxShapeDesc*);
	void				setBodyTemplate(const NxBodyDesc*);
	bool				getActorTemplate(NxActorDescBase& dest) const;
	bool				getShapeTemplate(NxShapeDesc& dest) const;
	bool				getBodyTemplate(NxBodyDesc& dest) const;

	/* NxApexResource, ApexResource */
	void				release();
	physx::PxU32				getListIndex() const
	{
		return m_listIndex;
	}
	void				setListIndex(class NxResourceList& list, physx::PxU32 index)
	{
		m_list = &list;
		m_listIndex = index;
	}

	/* NxApexActor, ApexActor */
	void                destroy();
	NxApexAsset*		getOwner() const;
	void				setPhysXScene(NxScene*);
	NxScene*			getPhysXScene() const;
	void				getPhysicalLodRange(physx::PxF32& min, physx::PxF32& max, bool& intOnly);
	physx::PxF32		getActivePhysicalLod();
	void				forcePhysicalLod(physx::PxF32 lod);

	void* 				getShapeGroupPtr() const
	{
		return (void*)mShapeGroup;
	}
	NxApexAsset*		getNxApexAsset(void)
	{
		return (NxApexAsset*) mAsset;
	}

	/* NiFieldBoundary */
	virtual bool updateFieldBoundary(physx::Array<NiFieldShapeDesc>& shapes);


protected:
	NxApexFieldBoundaryFlag			mType;
	physx::PxMat34Legacy			mPose;
	physx::PxVec3					mScale;
	FieldBoundaryAsset*				mAsset;
	FieldBoundaryScene*				mScene;
	NxForceFieldShapeGroup*			mShapeGroup;

	bool							mShapesChanged;

	friend class FieldBoundaryScene;
};

}
}
} // end namespace physx::apex

#endif
