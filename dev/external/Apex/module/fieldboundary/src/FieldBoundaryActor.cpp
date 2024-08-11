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

#include "NxApexDefs.h"
#include "MinPhysxSdkVersion.h"
#if NX_SDK_VERSION_NUMBER >= MIN_PHYSX_SDK_VERSION_REQUIRED && NX_SDK_VERSION_MAJOR == 2

#include "NxApex.h"

#include "PxMemoryBuffer.h"
#include "FieldBoundaryActor.h"
#include "FieldBoundaryAsset.h"
#include "FieldBoundaryScene.h"
#include "NiApexSDK.h"
#include "NiApexScene.h"

class NxForceField;
#include <NxForceFieldShape.h>
#include <NxForceFieldShapeDesc.h>

#include <NxBoxForceFieldShape.h>
#include <NxBoxForceFieldShapeDesc.h>
#include <NxCapsuleForceFieldShape.h>
#include <NxCapsuleForceFieldShapeDesc.h>
#include <NxConvexForceFieldShapeDesc.h>
#include <NxCooking.h>
#include <NxConvexMeshDesc.h>
#include <NxForceFieldShapeGroup.h>
#include <NxForceFieldShapeGroupDesc.h>
#include <NxScene.h>
#include <NxSphereForceFieldShape.h>
#include <NxSphereForceFieldShapeDesc.h>
#include "NxFromPx.h"
#include <NiFieldSamplerManager.h>

#include "ApexResourceHelper.h"

namespace physx
{
namespace apex
{
namespace fieldboundary
{

static NxConvexMesh* createConvexMesh(const ConvexMeshDesc& meshData, physx::PxVec3 scale, physx::PxBounds3 bounds)
{
	Array<PxVec3> scaledPoints;
	scaledPoints.resize(meshData.numPoints);
	for (physx::PxU32 i = 0; i < scaledPoints.size(); ++i)
	{
		scaledPoints[i] = meshData.points[i].multiply(scale);
	}

	NxConvexMeshDesc meshDesc;
	meshDesc.flags = meshData.flags; //meshDesc.flags = NX_CF_COMPUTE_CONVEX | NX_CF_INFLATE_CONVEX | NX_CF_USE_UNCOMPRESSED_NORMALS;
	meshDesc.numVertices = meshData.numPoints;
	meshDesc.pointStrideBytes = meshData.pointStrideBytes;
	meshDesc.points = scaledPoints.begin();

	NxConvexMesh* convexMesh = NULL;
	PxMemoryBuffer stream;
	stream.setEndianMode(physx::PxFileBuf::ENDIAN_NONE);
	NxFromPxStream nxs(stream);
	if (!NxGetApexSDK()->getCookingInterface()->NxCookConvexMesh(meshDesc, nxs))
	{
		scaledPoints.resize(8);
		for (physx::PxU32 i = 0; i < 8; ++i)
		{
			scaledPoints[i] = physx::PxVec3((i & 1) ? bounds.maximum.x : bounds.minimum.x, (i & 2) ? bounds.maximum.y : bounds.minimum.y, (i & 4) ? bounds.maximum.z : bounds.minimum.z).multiply(scale);
		}
		meshDesc.points = scaledPoints.begin();
		meshDesc.numVertices = 8;
		PxMemoryBuffer stream2(stream.getWriteBuffer(), stream.getWriteBufferSize());
		stream2.setEndianMode(physx::PxFileBuf::ENDIAN_NONE);
		NxFromPxStream nxs2(stream2);
		if (!NxGetApexSDK()->getCookingInterface()->NxCookConvexMesh(meshDesc, nxs2))
		{
			return NULL; // something seriously wrong
		}
		convexMesh = NxGetApexSDK()->getPhysXSDK()->createConvexMesh(nxs2);
	}
	else
	{
		convexMesh = NxGetApexSDK()->getPhysXSDK()->createConvexMesh(nxs);
	}

	return convexMesh;
}

FieldBoundaryActor::FieldBoundaryActor(const NxFieldBoundaryActorDesc& desc, FieldBoundaryAsset& asset, NxResourceList& list, FieldBoundaryScene& scene) :
	mType(NX_APEX_FFB_INCLUDE),
	mPose(desc.initialPose),
	mAsset(&asset),
	mScene(&scene),
	mShapeGroup(NULL),
	mShapesChanged(true)
{
	mScale = desc.scale.multiply(mAsset->getDefaultScale());
	mType = mAsset->mFlag;

	list.add(*this);			// Add self to asset's list of actors
	addSelfToContext(*scene.mApexScene->getApexContext());    // Add self to ApexScene
	addSelfToContext(scene);	// Add self to FieldBoundaryScene's list of actors

	NiFieldSamplerManager* fieldSamplerManager = mScene->getNiFieldSamplerManager();
	if (fieldSamplerManager != 0)
	{
		NiFieldBoundaryDesc fieldBoundaryDesc;
		fieldBoundaryDesc.boundaryFilterData = ApexResourceHelper::resolveCollisionGroup64(!desc.boundaryGroupMaskName.empty() ? desc.boundaryGroupMaskName.c_str() : mAsset->mParams->boundaryGroupMaskName);

		fieldSamplerManager->registerFieldBoundary(DYNAMIC_CAST(NiFieldBoundary*)(this), fieldBoundaryDesc);
	}
}


FieldBoundaryActor::~FieldBoundaryActor()
{
}

void FieldBoundaryActor::release()
{
	if (mInRelease)
	{
		return;
	}
	destroy();
}

void FieldBoundaryActor::destroy()
{
	ApexActor::destroy();

	setPhysXScene(NULL);

	NiFieldSamplerManager* fieldSamplerManager = mScene->getNiFieldSamplerManager();
	if (fieldSamplerManager != 0)
	{
		fieldSamplerManager->unregisterFieldBoundary(DYNAMIC_CAST(NiFieldBoundary*)(this));
	}

	delete this;
}

void FieldBoundaryActor::setPhysXScene(NxScene* scene)
{
	if (scene)
	{
		if (mShapeGroup)
		{
			NxScene& nxscene = mShapeGroup->getScene();
			nxscene.releaseForceFieldShapeGroup(*mShapeGroup);
			//mAsset->mModule->mSdk->releaseObjectDesc( mShapeGroup);
			mShapeGroup = NULL;
		}

		//create NxForceFieldShapeGroup in scene
		NxForceFieldShapeGroupDesc sgDesc;
		if (mAsset->mFlag)
		{
			sgDesc.flags = NX_FFSG_EXCLUDE_GROUP;
		}
		mShapeGroup = scene->createForceFieldShapeGroup(sgDesc);

		physx::PxU32 convexIndex = 0;
		NxForceFieldShape* fs = NULL;
		for (physx::PxU32 shapeIndex = 0; shapeIndex < mAsset->mShapes.size(); shapeIndex++)
		{
			FieldShapeDesc& fsDesc = mAsset->mShapes[shapeIndex];
			if (fsDesc.type == NX_APEX_SHAPE_SPHERE)
			{
				NxSphereForceFieldShapeDesc s;
				s.radius = fsDesc.radius * mScale.x;
				NxFromPxMat34(s.pose, mPose * fsDesc.pose);
				fs = mShapeGroup->createShape(s);
			}
			else if (fsDesc.type == NX_APEX_SHAPE_BOX)
			{
				NxBoxForceFieldShapeDesc b;
				physx::PxVec3 scale = fsDesc.dimensions.multiply(mScale);
				b.dimensions = NXFROMPXVEC3(scale);
				NxFromPxMat34(b.pose, mPose * fsDesc.pose);
				fs = mShapeGroup->createShape(b);
			}
			else if (fsDesc.type == NX_APEX_SHAPE_CAPSULE)
			{
				NxCapsuleForceFieldShapeDesc c;
				c.radius = fsDesc.radius * mScale.x;
				c.height = fsDesc.height * mScale.y;
				NxFromPxMat34(c.pose, mPose * fsDesc.pose);
				fs = mShapeGroup->createShape(c);
			}
			else if (fsDesc.type == NX_APEX_SHAPE_CONVEX)
			{
				PX_ASSERT(convexIndex < mAsset->mConvex.size());

				NxConvexForceFieldShapeDesc x;
				physx::PxMat34Legacy pose = mPose * fsDesc.pose;

				physx::PxBounds3 bounds;
				PxBounds3boundsOfOBB(bounds, pose.M, pose.t, fsDesc.dimensions);
				x.meshData = createConvexMesh(mAsset->mConvex[convexIndex], mScale, bounds);
				NxFromPxMat34(x.pose, pose);

				fs = mShapeGroup->createShape(x);

				convexIndex ++;
			}
			(void)fs;
		}

		if (mShapeGroup)
		{
			//mAsset->mModule->mSdk->createObjectDesc( this, mShapeGroup );
		}
	}
	else
	{
		if (mShapeGroup)
		{
			NxScene& nxscene = mShapeGroup->getScene();
			nxscene.releaseForceFieldShapeGroup(*mShapeGroup);
			//mAsset->mModule->mSdk->releaseObjectDesc( mShapeGroup);

			mShapeGroup = NULL;
		}
	}
}

NxScene* FieldBoundaryActor::getPhysXScene() const
{
	if (mShapeGroup)
	{
		return &mShapeGroup->getScene();
	}
	else
	{
		return NULL;
	}
}

void FieldBoundaryActor::getPhysicalLodRange(physx::PxF32& min, physx::PxF32& max, bool& intOnly)
{
	PX_UNUSED(min);
	PX_UNUSED(max);
	PX_UNUSED(intOnly);
	APEX_INVALID_OPERATION("not implemented");
}

physx::PxF32 FieldBoundaryActor::getActivePhysicalLod()
{
	APEX_INVALID_OPERATION("NxFieldBoundaryActor does not support this operation");
	return -1.0f;
}

void FieldBoundaryActor::forcePhysicalLod(physx::PxF32 lod)
{
	PX_UNUSED(lod);
	APEX_INVALID_OPERATION("not implemented");
}

/* Must be defined inside CPP file, since they require knowledge of asset class */
NxApexAsset* 			FieldBoundaryActor::getOwner() const
{
	return (NxApexAsset*) mAsset;
}
NxFieldBoundaryAsset* 	FieldBoundaryActor::getFieldBoundaryAsset() const
{
	return mAsset;
}

void FieldBoundaryActor::setActorTemplate(const NxActorDescBase* desc)
{
	ApexActorSource::setActorTemplate(desc);
}

void FieldBoundaryActor::setShapeTemplate(const NxShapeDesc* desc)
{
	ApexActorSource::setShapeTemplate(desc);
}
void FieldBoundaryActor::setBodyTemplate(const NxBodyDesc* desc)
{
	ApexActorSource::setBodyTemplate(desc);
}
bool FieldBoundaryActor::getActorTemplate(NxActorDescBase& dest) const
{
	return ApexActorSource::getActorTemplate(dest);
}
bool FieldBoundaryActor::getShapeTemplate(NxShapeDesc& dest) const
{
	return ApexActorSource::getShapeTemplate(dest);
}
bool FieldBoundaryActor::getBodyTemplate(NxBodyDesc& dest) const
{
	return ApexActorSource::getBodyTemplate(dest);
}

void FieldBoundaryActor::setGlobalPose(const physx::PxMat34Legacy& pose)
{
	mPose = pose;
	NxMat34 nxPose;
	NxFromPxMat34(nxPose, pose);
	physx::PxU32 nbShapes = mShapeGroup->getNbShapes();
	mShapeGroup->resetShapesIterator();
	for (physx::PxU32 i = 0; i < nbShapes; i++)
	{
		NxForceFieldShape* fs = mShapeGroup->getNextShape();
		fs->setPose(nxPose);
	}

	mShapesChanged = true;
}

void FieldBoundaryActor::setScale(const physx::PxVec3& scale)
{
	physx::PxU32 nbShapes = mShapeGroup->getNbShapes();
	//PX_ASSERT(nbShapes == mAsset->mShapes.size());
	mShapeGroup->resetShapesIterator();

	physx::PxU32 convexIndex = 0;
	for (physx::PxU32 shapeIndex = 0; shapeIndex < nbShapes; shapeIndex++)
	{
		NxForceFieldShape* fs = mShapeGroup->getNextShape();
		FieldShapeDesc& fsDesc = mAsset->mShapes[shapeIndex];

		switch (fs->getType())
		{
		case NX_SHAPE_SPHERE:
		{
			physx::PxF32 radius = fs->isSphere()->getRadius();
			fs->isSphere()->setRadius(scale.x * radius / mScale.x);
		}
		break;
		case NX_SHAPE_BOX:
		{
			NxVec3 dimensions = fs->isBox()->getDimensions();
			NxVec3 compscale;
			compscale.setx(scale.x / mScale.x);
			compscale.sety(scale.y / mScale.y);
			compscale.setz(scale.z / mScale.z);
			dimensions.arrayMultiply(dimensions, compscale);
			fs->isBox()->setDimensions(dimensions);
		}
		break;
		case NX_SHAPE_CAPSULE:
		{
			NxVec3 compscale;
			compscale.setx(scale.x / mScale.x);
			compscale.sety(scale.y / mScale.y);
			physx::PxF32 radius = fs->isCapsule()->getRadius() * compscale.x;
			physx::PxF32 height = fs->isCapsule()->getHeight() * compscale.y;
			fs->isCapsule()->setRadius(radius);
			fs->isCapsule()->setHeight(height);
		}
		break;
		case NX_SHAPE_CONVEX:
		{
			PX_ASSERT(convexIndex < mAsset->mConvex.size());
			NxForceFieldShape* toDelete = fs;
			NxForceFieldShape* toCreate = NULL;

			NxConvexForceFieldShapeDesc x;
			physx::PxMat34Legacy pose = mPose * fsDesc.pose;

			physx::PxBounds3 bounds;
			PxBounds3boundsOfOBB(bounds, pose.M, pose.t, fsDesc.dimensions);
			x.meshData = createConvexMesh(mAsset->mConvex[convexIndex], scale, bounds);
			NxFromPxMat34(x.pose, pose);

			toCreate = mShapeGroup->createShape(x);	//toCreate is added to the back of mShapeGroup
			mShapeGroup->releaseShape(*toDelete);   //will call replaceWithLast(), toCreate will just replace toDetele entry in mShapeGroup
			(void)toCreate;

			convexIndex ++;
		}
		break;
		default:
			break;
		}
	}

	mScale = scale;

	mShapesChanged = true;
}

// Called by FieldBoundaryScene::fetchResults() with render data lock acquired
void FieldBoundaryActor::updatePoseAndBounds()
{
	//no render for field boundary
}

bool FieldBoundaryActor::updateFieldBoundary(physx::Array<NiFieldShapeDesc>& shapes)
{
	if (!mShapesChanged)
	{
		return false;
	}

	if (shapes.empty())
	{
		physx::PxU32 outShapeCount = 0;
		//count valid shapes
		physx::PxU32 shapeCount = mAsset->mShapes.size();
		for (physx::PxU32 shapeIndex = 0; shapeIndex < shapeCount; shapeIndex++)
		{
			const FieldShapeDesc& fsDesc = mAsset->mShapes[shapeIndex];
			if (fsDesc.type < NX_APEX_SHAPE_CONVEX)
			{
				++outShapeCount;
			}
		}
		shapes.resize(outShapeCount);
	}

	physx::PxU32 outShapeIndex = 0;
	physx::PxU32 shapeCount = mAsset->mShapes.size();
	for (physx::PxU32 shapeIndex = 0; shapeIndex < shapeCount; shapeIndex++)
	{
		const FieldShapeDesc& fsDesc = mAsset->mShapes[shapeIndex];
		if (fsDesc.type < NX_APEX_SHAPE_CONVEX)
		{
			NiFieldShapeDesc& desc = shapes[outShapeIndex++];

			physx::PxMat34Legacy pose = mPose * fsDesc.pose;
			pose.getInverse(desc.worldToShape);

			desc.weight = 1;

			switch (fsDesc.type)
			{
			case NX_APEX_SHAPE_SPHERE:
			{
				desc.type = NiFieldShapeType::SPHERE;
				desc.dimensions.x = fsDesc.radius * mScale.x;
				desc.dimensions.y = 0;
				desc.dimensions.z = 0;
			}
			break;
			case NX_APEX_SHAPE_BOX:
			{
				desc.type = NiFieldShapeType::BOX;
				desc.dimensions = fsDesc.dimensions.multiply(mScale);
			}
			break;
			case NX_APEX_SHAPE_CAPSULE:
			{
				desc.type = NiFieldShapeType::CAPSULE;
				desc.dimensions.x = fsDesc.radius * mScale.x;
				desc.dimensions.y = fsDesc.height * mScale.y;
				desc.dimensions.z = 0;
			}
			break;
			default:
				PX_ALWAYS_ASSERT();
				break;
			}
		}
	}

	mShapesChanged = false;
	return true;
}


}
}
} // end namespace physx::apex

#endif // NX_SDK_VERSION_NUMBER >= MIN_PHYSX_SDK_VERSION_REQUIRED
