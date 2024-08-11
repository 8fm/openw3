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
#include "ExplosionActor.h"
#include "ExplosionAsset.h"
#include "ExplosionScene.h"
#include "NiApexSDK.h"
#include "NiApexScene.h"
#include "NxModuleExplosion.h"
#include "NxFromPx.h"

#if NX_SDK_VERSION_MAJOR == 2
#include "NxFieldBoundaryActor.h"
#include <NxForceField.h>
#include <NxForceFieldShapeGroup.h>
#include <NxForceFieldShapeGroupDesc.h>
#include <NxForceFieldLinearKernel.h>
#include <NxForceFieldLinearKernelDesc.h>
#include <NxScene.h>
#include <NxSphereForceFieldShape.h>
#include <NxSphereForceFieldShapeDesc.h>
#endif

namespace physx
{
namespace apex
{
namespace explosion
{

#if NX_SDK_VERSION_MAJOR == 2
void convertApexFieldTypeToPhysX(const NxApexForceFieldType& in, NxForceFieldType& out)
{
	switch (in)
	{
	case NX_APEX_FF_TYPE_GRAVITATIONAL:
		out = NX_FF_TYPE_GRAVITATIONAL;
		break;
	case NX_APEX_FF_TYPE_OTHER:
		out = NX_FF_TYPE_OTHER;
		break;
	case NX_APEX_FF_TYPE_NO_INTERACTION:
		out = NX_FF_TYPE_NO_INTERACTION;
		break;

	default:
		break;
	}
}
#endif

void convertApexFieldFlagToPhysX(const physx::PxU32& in, physx::PxU32& out)
{
	physx::PxU32 shiftbits = 5; //hard code
	out = in << shiftbits;
}

ExplosionActor::ExplosionActor(const NxExplosionActorDesc& desc, ExplosionAsset& asset, NxResourceList& list, ExplosionScene& scene):
	mExplosionScene(&scene),
	mDisable(false),
	mPose(desc.initialPose),
#if NX_SDK_VERSION_MAJOR == 2
	mAttachedNxActor(desc.nxActor),
	mGroup(desc.envSetting.group),
	mGroupsMask(desc.envSetting.groupsMask),
#endif
	mName(desc.actorName),
	mAsset(&asset),
#if NX_SDK_VERSION_MAJOR == 2
	mNxForceField(NULL),
	mLinearKernel(NULL),
	mOuterBoundShape(NULL),
	mInnerBoundShape(NULL),
	mDefaultIncludeSG(NULL),
	mDefaultExcludeSG(NULL),
#endif
	mDebugRenderingBuffersAllocated(false),
	mSamplePointsInBuffer(0)
{
	if (!desc.eParameter)
	{
		mParams = mAsset->mParams;
	}
	else
	{

		mParams =  desc.eParameter;
	}

	if (0 == strcmp(mParams->mode, "explosion"))
	{
		mFfieldMode = NX_APEX_FFM_EXPLOSION;
	}
	else if (0 == strcmp(mParams->mode, "implosion"))
	{
		mFfieldMode = NX_APEX_FFM_IMPLOSION;
	}
	else if (0 == strcmp(mParams->mode, "shockwave"))
	{
		mFfieldMode = NX_APEX_FFM_SHOCKWAVE;
	}

#if NX_SDK_VERSION_MAJOR == 2
	//record some basic info of NxForceField
	mScale = desc.scale * mAsset->getDefaultScale();
	convertApexFieldTypeToPhysX(desc.envSetting.fluidType, mFluidType);
	convertApexFieldTypeToPhysX(desc.envSetting.clothType, mClothType);
	convertApexFieldTypeToPhysX(desc.envSetting.softBodyType, mSoftBodyType);
	convertApexFieldTypeToPhysX(desc.envSetting.rigidBodyType, mRigidBodyType);
	convertApexFieldFlagToPhysX(desc.envSetting.flags, mFlags);

	//add field boundaries
	for (physx::PxU32 i = 0, count =  mAsset->getFieldBoundariesCount(); i < count; i++)
	{
		NxApexAsset* genericAsset = mAsset->mFieldBoundaryAssetTracker.getAssetFromName(mAsset->getFieldBoundariesName(i));
		NxFieldBoundaryAsset* fbAsset = static_cast<NxFieldBoundaryAsset*>(genericAsset);
		NxParameterized::Interface* bDesc = fbAsset->getDefaultActorDesc();
		NxApexActor* apexActor = fbAsset->createApexActor(*bDesc, *static_cast<NxApexScene*>(scene.mApexScene));
		NxFieldBoundaryActor* bound = static_cast<NxFieldBoundaryActor*>(apexActor);
		mBoundariesActors.pushBack(bound);
	}
#endif

	list.add(*this);			// Add self to asset's list of actors
	addSelfToContext(*scene.mApexScene->getApexContext());    // Add self to ApexScene
	addSelfToContext(scene);	// Add self to FieldBoundaryScene's list of actors

	initFieldSampler();
}

#if NX_SDK_VERSION_MAJOR == 2
void ExplosionActor::setPhysXScene(NxScene* scene)
#elif NX_SDK_VERSION_MAJOR == 3
void ExplosionActor::setPhysXScene(PxScene* scene)
#endif
{
	if (scene)
	{
#if NX_SDK_VERSION_MAJOR == 2
		if (mNxForceField)
		{
			releaseNxForceField();
		}

		//use linear kernel
		NxForceFieldLinearKernelDesc	lKernelDesc;
		lKernelDesc.constant = NxVec3(mParams->constantForce, 0, 0);
		if (!mParams->useUniformField)
		{
			physx::PxF32  s1, s2;
			NxMat33 m1, m2;

			s1 = mParams->nonUniformParams.fieldIntensity * mParams->nonUniformParams.distanceScale;
			m1.diagonal(NxVec3(s1, 0, 0));
			lKernelDesc.positionMultiplier = m1;
			lKernelDesc.positionTarget = NxVec3(mParams->nonUniformParams.distanceTarget, 0, 0);
			s2 = mParams->nonUniformParams.fieldIntensity * mParams->nonUniformParams.velocityScale;
			m2.diagonal(NxVec3(s2, 0, 0));
			lKernelDesc.velocityMultiplier = m2;
			lKernelDesc.velocityTarget = NxVec3(mParams->nonUniformParams.velocityTarget, 0, 0);

			lKernelDesc.falloffLinear = NxVec3(mParams->nonUniformParams.disAttenuation, 0, 0);
			lKernelDesc.falloffQuadratic = NxVec3(mParams->nonUniformParams.disAttenuation * mParams->nonUniformParams.disAttenuation, 0, 0);
			lKernelDesc.noise = NxVec3(mParams->nonUniformParams.degreeOfNoise, 0, 0);
		}

		mLinearKernel = scene->createForceFieldLinearKernel(lKernelDesc);

		//Create NxForceField in scene
		NxForceFieldDesc ffDesc;
		NxFromPxMat34(ffDesc.pose, mPose);
		ffDesc.actor = mAttachedNxActor;
		ffDesc.coordinates = NX_FFC_SPHERICAL; //always use spherical coordinate for APEX explosion feature
		ffDesc.group = mGroup;
		ffDesc.groupsMask = mGroupsMask;
		ffDesc.forceFieldVariety = 0; // set later

		ffDesc.fluidType = mFluidType;
		ffDesc.clothType = mClothType;
		ffDesc.softBodyType = mSoftBodyType;
		ffDesc.rigidBodyType = mRigidBodyType;
		ffDesc.kernel = mLinearKernel;
		mNxForceField = scene->createForceField(ffDesc);

		//always use the default NxForceFieldMaterial 0 for rb, fluid, cloth and softbody
		//don't use NxScene::createForceFieldMaterial() to create different Material ID for them,
		//because we only support a unified "scale" here
		mFfVariety = scene->createForceFieldVariety();
		mNxForceField->setForceFieldVariety(mFfVariety);
		scene->setForceFieldScale(mFfVariety, 0, mScale);

		mDefaultIncludeSG = &(mNxForceField->getIncludeShapeGroup()); //default include shape group, always exists
#endif

		physx::PxF32 nearD = -1.0f, farD  = -1.0f;
		if (mParams->useFarDistance)
		{
			farD  = mParams->farDistance;
		}
		if (mParams->useNearDistance)
		{
			nearD = mParams->nearDistance;
		}

		if (0 == strcmp(mParams->mode, "shockwave"))
		{
			if (nearD < 0)
			{
				nearD = 0;
			}
			farD = nearD + mParams->shockwaveParams.width;
		}

#if NX_SDK_VERSION_MAJOR == 2
		if (farD >= 0)
		{
			NxSphereForceFieldShapeDesc s;
			s.radius = farD;
			s.pose.id();
			s.name = "OuterBound";
			mOuterBoundShape = mDefaultIncludeSG->createShape(s);	//mOuterBoundShape is the unique include shape in mDefaultIncludeSG
		}

		if (nearD >= 0)
		{
			NxForceFieldShapeGroupDesc sgDesc;
			sgDesc.flags = NX_FFSG_EXCLUDE_GROUP;
			mDefaultExcludeSG = scene->createForceFieldShapeGroup(sgDesc);
			mNxForceField->addShapeGroup(*mDefaultExcludeSG);

			NxSphereForceFieldShapeDesc s;
			s.radius = nearD;
			NxMat34 nxPose;
			NxFromPxMat34(nxPose, mPose);
			s.pose = mAttachedNxActor ? mAttachedNxActor->getGlobalPose() * nxPose : nxPose;
			s.name = "InnerBound";
			mInnerBoundShape = mDefaultExcludeSG->createShape(s);	//mInnerBoundShape is the unique exclude shape in mDefaultExcludeSG
		}

		//add connection FieldBoundaryActors, each boundary actor is one-to-one mapping to a NxForceFieldShapeGroup
		for (physx::PxU32 i = 0; i < mBoundariesActors.size(); i++)
		{
			NxForceFieldShapeGroup* ffsg = (NxForceFieldShapeGroup*) mBoundariesActors[i]->getShapeGroupPtr();
			mNxForceField->addShapeGroup(*ffsg);
		}
#elif NX_SDK_VERSION_MAJOR == 3
		PX_UNUSED(farD);
#endif
	}
	else
	{
#if NX_SDK_VERSION_MAJOR == 2
		if (mNxForceField)
		{
			releaseNxForceField();
		}
		mAttachedNxActor = NULL;
#endif
	}
}

#if NX_SDK_VERSION_MAJOR == 2
NxScene* ExplosionActor::getPhysXScene() const
{
	if (mNxForceField)
	{
		return &mNxForceField->getScene();
	}
	else
	{
		return NULL;
	}
}
#elif NX_SDK_VERSION_MAJOR == 3
PxScene* ExplosionActor::getPhysXScene() const
{
	return NULL;
}
#endif

void ExplosionActor::getPhysicalLodRange(physx::PxF32& min, physx::PxF32& max, bool& intOnly)
{
	PX_UNUSED(min);
	PX_UNUSED(max);
	PX_UNUSED(intOnly);
	APEX_INVALID_OPERATION("not implemented");
}

physx::PxF32 ExplosionActor::getActivePhysicalLod()
{
	APEX_INVALID_OPERATION("NxExplosionActor does not support this operation");
	return -1.0f;
}


void ExplosionActor::forcePhysicalLod(physx::PxF32 lod)
{
	PX_UNUSED(lod);
	APEX_INVALID_OPERATION("not implemented");
}

/* Must be defined inside CPP file, since they require knowledge of asset class */
NxApexAsset* 		ExplosionActor::getOwner() const
{
	return (NxApexAsset*) mAsset;
}
NxExplosionAsset* 	ExplosionActor::getExplosionAsset() const
{
	return mAsset;
}

void ExplosionActor::release()
{
	if (mInRelease)
	{
		return;
	}
	destroy();
}

void ExplosionActor::destroy()
{
	ApexActor::destroy();

	setPhysXScene(NULL);

	releaseFieldSampler();

	delete this;
}

#if NX_SDK_VERSION_MAJOR == 2
void ExplosionActor::setActorTemplate(const NxActorDescBase* desc)
{
	ApexActorSource::setActorTemplate(desc);
}

void ExplosionActor::setShapeTemplate(const NxShapeDesc* desc)
{
	ApexActorSource::setShapeTemplate(desc);
}

void ExplosionActor::setBodyTemplate(const NxBodyDesc* desc)
{
	ApexActorSource::setBodyTemplate(desc);
}

bool ExplosionActor::getActorTemplate(NxActorDescBase& dest) const
{
	return ApexActorSource::getActorTemplate(dest);
}

bool ExplosionActor::getShapeTemplate(NxShapeDesc& dest) const
{
	return ApexActorSource::getShapeTemplate(dest);
}

bool ExplosionActor::getBodyTemplate(NxBodyDesc& dest) const
{
	return ApexActorSource::getBodyTemplate(dest);
}
#endif

bool ExplosionActor::enable()
{
	if (!mDisable)
	{
		return true;
	}

	mDisable = false;

#if NX_SDK_VERSION_MAJOR == 2
	mNxForceField->setRigidBodyType(mRigidBodyType);
	mNxForceField->setFluidType(mFluidType);
	mNxForceField->setClothType(mClothType);
	mNxForceField->setSoftBodyType(mSoftBodyType);
#endif

	return true;
}

bool ExplosionActor::disable()
{
	if (mDisable)
	{
		return true;
	}

#if NX_SDK_VERSION_MAJOR == 2
	if (mFfieldMode == NX_APEX_FFM_SHOCKWAVE)
	{
		physx::PxF32 r1 = mParams->nearDistance > 0 ? mParams->nearDistance : 0;
		physx::PxF32 r2 = r1 + mParams->shockwaveParams.width;
		setInnerBoundRadius(r1);
		setOuterBoundRadius(r2);
	}

	NxForceFieldType type = NX_FF_TYPE_NO_INTERACTION;
	mNxForceField->setRigidBodyType(type);
	mNxForceField->setFluidType(type);
	mNxForceField->setClothType(type);
	mNxForceField->setSoftBodyType(type);
#endif

	mDisable = true;
	mFieldSamplerChanged = true;

	return true;
}

void ExplosionActor::setPose(const physx::PxMat44& pose)
{
	mPose = pose;

#if NX_SDK_VERSION_MAJOR == 2
	NxMat34 nxPose;
	NxFromPxMat34(nxPose, mPose);

	if (mNxForceField)
	{
		mNxForceField->setPose(nxPose);
	}

	if (mInnerBoundShape)
	{
		NxMat34 spose = mAttachedNxActor ? mAttachedNxActor->getGlobalPose() * nxPose : nxPose;
		mInnerBoundShape->setPose(spose);
	}
#endif
}

void ExplosionActor::setScale(physx::PxF32 scale)
{
	mScale = scale;

#if NX_SDK_VERSION_MAJOR == 2
	if (getPhysXScene())
	{
		getPhysXScene()->setForceFieldScale(mFfVariety, 0, mScale);
	}
#endif
}

#if NX_SDK_VERSION_MAJOR == 2
void ExplosionActor::addFieldBoundary(NxFieldBoundaryActor& nxActor)
{
	PX_ASSERT(mNxForceField != NULL);

	mBoundariesActors.pushBack(&nxActor);

	NxForceFieldShapeGroup* ffsg = (NxForceFieldShapeGroup*) nxActor.getShapeGroupPtr();
	mNxForceField->addShapeGroup(*ffsg);
}

void ExplosionActor::removeFieldBoundary(NxFieldBoundaryActor& nxActor)
{
	PX_ASSERT(mNxForceField != NULL);

	mBoundariesActors.findAndReplaceWithLast(&nxActor);

	NxForceFieldShapeGroup* ffsg = (NxForceFieldShapeGroup*)nxActor.getShapeGroupPtr();

	mNxForceField->resetShapeGroupsIterator();
	physx::PxU32 nb = mNxForceField->getNbShapeGroups();
	for (physx::PxU32 i = 0; i < nb; i++)
	{
		NxForceFieldShapeGroup* group = mNxForceField->getNextShapeGroup();
		if (group == ffsg)
		{
			mNxForceField->removeShapeGroup(*group);
		}
	}
}

physx::PxF32 ExplosionActor::getOuterBoundRadius() const
{
	if (mOuterBoundShape)
	{
		return mOuterBoundShape->isSphere()->getRadius();
	}
	else
	{
		return -1.0f;
	}
}

void ExplosionActor::setOuterBoundRadius(physx::PxF32 r)
{
	if (mOuterBoundShape)
	{
		mOuterBoundShape->isSphere()->setRadius(r);
	}
	else
	{
		NxSphereForceFieldShapeDesc s;
		s.radius = r;
		s.pose.id();
		s.name = "OuterBound";
		mOuterBoundShape = mDefaultIncludeSG->createShape(s);
	}
}

physx::PxF32 ExplosionActor::	getInnerBoundRadius() const
{
	if (mInnerBoundShape)
	{
		return mInnerBoundShape->isSphere()->getRadius();
	}
	else
	{
		return -1.0f;
	}
}

void ExplosionActor::setInnerBoundRadius(physx::PxF32 r)
{
	if (mInnerBoundShape)
	{
		mInnerBoundShape->isSphere()->setRadius(r);
	}
	else
	{
		if (!mDefaultExcludeSG)
		{
			NxForceFieldShapeGroupDesc sgDesc;
			sgDesc.flags = NX_FFSG_EXCLUDE_GROUP;
			mDefaultExcludeSG = getPhysXScene()->createForceFieldShapeGroup(sgDesc);
			mNxForceField->addShapeGroup(*mDefaultExcludeSG);
		}
		NxMat34 nxPose;
		NxFromPxMat34(nxPose, mPose);

		NxSphereForceFieldShapeDesc s;
		s.radius = r;
		s.pose = mAttachedNxActor ? mAttachedNxActor->getGlobalPose() * nxPose : nxPose;
		s.name = "InnerBound";
		mInnerBoundShape = mDefaultExcludeSG->createShape(s);
	}
}
#endif

void ExplosionActor::updateExplosion(physx::PxF32 dt)
{
#if NX_SDK_VERSION_MAJOR == 2
	if (isEnable() && mInnerBoundShape && mAttachedNxActor)
	{
		NxMat34 nxPose;
		NxFromPxMat34(nxPose, mPose);
		mInnerBoundShape->isSphere()->setPose(mAttachedNxActor->getGlobalPose() * nxPose);
	}

	if (mFfieldMode == NX_APEX_FFM_SHOCKWAVE)
	{
		if (!mDisable)
		{
			physx::PxF32 r1 = getInnerBoundRadius() + mParams->shockwaveParams.travelVelocity * dt;
			physx::PxF32 r2 = getOuterBoundRadius() + mParams->shockwaveParams.travelVelocity * dt;
			if (r1 < mParams->farDistance)
			{
				if (r2 > mParams->farDistance)
				{
					r2 = mParams->farDistance;
				}

				setInnerBoundRadius(r1);
				setOuterBoundRadius(r2);
			}
			else
			{
				disable();
			}
		}
	}
	else //(mFfieldMode == NX_APEX_FFM_IMPLOSION) || (mFfieldMode == NX_APEX_FFM_EXPLOSION)
	{
		//so far no specific updates needed for implosion and explosion
	}

	if (isEnable() && mNxForceField)
	{
		// call this to update FF's bounds, fixes simulate(0) bug
		NxVec3 origin(0.0);
		mNxForceField->samplePoints(1, &origin, &origin, &origin, &origin);
	}
#elif NX_SDK_VERSION_MAJOR == 3
	PX_UNUSED(dt);
#endif
}

#if NX_SDK_VERSION_MAJOR == 2
void ExplosionActor::releaseNxForceField()
{
	if (!mNxForceField)
	{
		return;
	}

	NxScene* scene = &mNxForceField->getScene();

	//mDefaultIncludeSG (with mOuterBoundShape) is automatically released by releaseForceField()

	if (mNxForceField != NULL)
	{
		scene->releaseForceField(*mNxForceField);
		mNxForceField = NULL;
	}

	//Here only the mDefaultExcludeSG (with mInnerBoundShape) need to be released, other shapeGroup is managed by FieldBoundary actor
	if (mDefaultExcludeSG != NULL)
	{
		scene->releaseForceFieldShapeGroup(*mDefaultExcludeSG);
		mDefaultExcludeSG = NULL;
	}

	if (mLinearKernel != NULL)
	{
		scene->releaseForceFieldLinearKernel(*mLinearKernel);
		mLinearKernel = NULL;
	}

	scene->releaseForceFieldVariety(mFfVariety);

	mOuterBoundShape = NULL;
	mInnerBoundShape = NULL;
	mDefaultIncludeSG = NULL;
	mFfVariety = 0;

	ReleaseDebugRenderingBuffers();
}
#endif

// Called by ExplosionScene::fetchResults()
void ExplosionActor::updatePoseAndBounds()
{
	//no render for explosion
}

void ExplosionActor::visualizeExplosionForces(void)
{
#if NX_SDK_VERSION_MAJOR == 2 && !defined(WITHOUT_DEBUG_VISUALIZE)
	mExplosionScene->mRenderDebug->setCurrentUserPointer((void*)(NxApexActor*)this);

	physx::PxF32 innerRadius = getInnerBoundRadius();
	physx::PxF32 outerRadius = getOuterBoundRadius();
	physx::PxF32 maxRadius = physx::PxMax(innerRadius, outerRadius);
	physx::PxMat34Legacy pose = getPose();
	physx::PxVec3 loc = pose.t;
	physx::PxU32 i, j, k;
	physx::PxU32 num;
	physx::PxF32 offset;
	physx::PxF32 locX, locY, locZ;
	physx::PxVec3 pointWorldCoordinates;

	physx::PxF32 SampleSpacing = mExplosionScene->mExplosionDebugRenderParams->FORCE_FIELDS_FORCES_DEBUG_RENDERING_SPACING;

	mSamplePointsInBuffer = 0;
	if (!mDebugRenderingBuffersAllocated)
	{
		// allocate the buffers needed to sample the forces
		mSamplePointsLoc		= (physx::PxVec3*)PX_ALLOC(sizeof(physx::PxVec3) * mSamplePointsBufferSize, PX_DEBUG_EXP("ExplosionActor::samplePointsLoc"));
		mSamplePointsVel		= (physx::PxVec3*)PX_ALLOC(sizeof(physx::PxVec3) * mSamplePointsBufferSize, PX_DEBUG_EXP("ExplosionActor::samplePointsVel"));
		mSamplePointsOutForces	= (physx::PxVec3*)PX_ALLOC(sizeof(physx::PxVec3) * mSamplePointsBufferSize, PX_DEBUG_EXP("ExplosionActor::samplePointsOutForces"));
		mSamplePointsOutTorques	= (physx::PxVec3*)PX_ALLOC(sizeof(physx::PxVec3) * mSamplePointsBufferSize, PX_DEBUG_EXP("ExplosionActor::samplePointsOutTorques"));
		if ((mSamplePointsLoc == NULL) ||
		        (mSamplePointsVel == NULL) ||
		        (mSamplePointsOutForces == NULL) ||
		        (mSamplePointsOutTorques == NULL))
		{
			APEX_DEBUG_WARNING("APEX Explosion: Could not allocate buffers for debug rendering!");
			ReleaseDebugRenderingBuffers();
			return;
		}
		else
		{
			mDebugRenderingBuffersAllocated = true;
		}
	}

	num = (physx::PxU32)((maxRadius * 2.0f) / SampleSpacing);
	physx::PxF32 distanceFromCenter;

	offset = ((maxRadius * 2.0f) - ((num - 1) * SampleSpacing)) / 2.0f;

	for (i = 0; i < num; i++)
	{
		locX = -maxRadius + offset + (i * SampleSpacing);
		for (j = 0; j < num; j++)
		{
			locY = -maxRadius + offset + (j * SampleSpacing);
			for (k = 0; k < num; k++)
			{
				locZ = -maxRadius + offset + (k * SampleSpacing);
				// only process the points that are in the explosion radius
				distanceFromCenter = physx::PxSqrt(locX * locX + locY * locY + locZ * locZ);
				if (distanceFromCenter <= maxRadius)
				{
					pointWorldCoordinates = mPose.getPosition() + physx::PxVec3(locX, locY, locZ);
					addSamplePointToBuffer(pointWorldCoordinates);
				}
			}
		}
	}
	SamplePointsOutputBuffer();

	// visualize actor name
	if (mExplosionScene->mExplosionDebugRenderParams->VISUALIZE_EXPLOSION_ACTOR_NAME &&
	        mExplosionScene->mExplosionDebugRenderParams->THRESHOLD_DISTANCE_EXPLOSION_ACTOR_NAME >
	        (-mExplosionScene->mApexScene->getEyePosition(0) + this->getPose().column3.getXYZ()).magnitude())
	{
		mExplosionScene->mRenderDebug->pushRenderState();
		mExplosionScene->mRenderDebug->setCurrentColor(mExplosionScene->mRenderDebug->getDebugColor(DebugColors::Yellow));

		PxMat44 cameraFacingPose((mExplosionScene->mApexScene->getViewMatrix(0)).inverseRT());
		PxVec3 textLocation = this->getPose().column3.getXYZ() + cameraFacingPose.column1.getXYZ();
		cameraFacingPose.setPosition(textLocation);
		mExplosionScene->mRenderDebug->debugOrientedText(cameraFacingPose, " %s %s", this->getOwner()->getObjTypeName(), this->getOwner()->getName());
		mExplosionScene->mRenderDebug->popRenderState();
	}

	mExplosionScene->mRenderDebug->setCurrentUserPointer(NULL);
#endif
}

void ExplosionActor::ReleaseDebugRenderingBuffers(void)
{
	// if there are buffers allocated deallocate them
	if (mDebugRenderingBuffersAllocated)
	{
		// deallocate the buffers
		if (mSamplePointsLoc != NULL)
		{
			PX_FREE(mSamplePointsLoc);
			mSamplePointsLoc = NULL;
		}
		if (mSamplePointsVel != NULL)
		{
			PX_FREE(mSamplePointsVel);
			mSamplePointsVel = NULL;
		}
		if (mSamplePointsOutForces != NULL)
		{
			PX_FREE(mSamplePointsOutForces);
			mSamplePointsOutForces = NULL;
		}
		if (mSamplePointsOutTorques != NULL)
		{
			PX_FREE(mSamplePointsOutTorques);
			mSamplePointsOutTorques = NULL;
		}
		mDebugRenderingBuffersAllocated = false;
	}
}

void ExplosionActor::addSamplePointToBuffer(physx::PxVec3 pt)
{
	mSamplePointsLoc[mSamplePointsInBuffer] = pt;
	mSamplePointsInBuffer++;
	if (mSamplePointsInBuffer >= mSamplePointsBufferSize)
	{
		SamplePointsOutputBuffer();
	}
}

void ExplosionActor::SamplePointsOutputBuffer(void)
{
#ifndef WITHOUT_DEBUG_VISUALIZE
	physx::PxU32 i;
	physx::PxF32 forceMag;
	physx::PxU32 color;

	physx::PxF32 ForceScaling = mExplosionScene->mExplosionDebugRenderParams->FORCE_FIELDS_FORCES_DEBUG_RENDERING_SCALING;
	physx::PxF32 SampleSpacing = mExplosionScene->mExplosionDebugRenderParams->FORCE_FIELDS_FORCES_DEBUG_RENDERING_SPACING;

	if (mSamplePointsInBuffer != 0)
	{
#if NX_SDK_VERSION_MAJOR == 2
		if (mNxForceField != NULL)
		{
			mNxForceField->samplePoints(mSamplePointsInBuffer,
			                            (const NxVec3*)mSamplePointsLoc,
			                            (const NxVec3*)mSamplePointsVel,
			                            (NxVec3*)mSamplePointsOutForces,
			                            (NxVec3*)mSamplePointsOutTorques);
		}
#endif

		for (i = 0; i < mSamplePointsInBuffer; i++)
		{
			forceMag = mSamplePointsOutForces[i].magnitude();
			mSamplePointsOutForces[i].normalize();
			forceMag *= ForceScaling;
			color = mExplosionScene->mRenderDebug->getDebugColor(DebugColors::ForceArrowsNorm);
			if (forceMag > (MAX_FORCE_PERCENTAGE_OF_SPACING * SampleSpacing))
			{
				forceMag = MAX_FORCE_PERCENTAGE_OF_SPACING * SampleSpacing;
				color = mExplosionScene->mRenderDebug->getDebugColor(DebugColors::ForceArrowsHigh);
			}
			else if ((forceMag < (MIN_FORCE_PERCENTAGE_OF_SPACING * SampleSpacing))
			         && (forceMag != 0.0f))
			{
				forceMag = MIN_FORCE_PERCENTAGE_OF_SPACING * SampleSpacing;
				color = mExplosionScene->mRenderDebug->getDebugColor(DebugColors::ForceArrowsLow);;
			}
			mExplosionScene->mRenderDebug->setCurrentColor(color);
			mExplosionScene->mRenderDebug->debugRay(mSamplePointsLoc[i], mSamplePointsLoc[i] + mSamplePointsOutForces[i]);
		}
		mSamplePointsInBuffer = 0;
	}
#endif
}

}
}
} // end namespace physx::apex

#endif // NX_SDK_VERSION_NUMBER >= MIN_PHYSX_SDK_VERSION_REQUIRED

