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
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  


#include "NpRigidStatic.h"
#include "NpPhysics.h"
#include "ScbNpDeps.h"

using namespace physx;

NpRigidStatic::NpRigidStatic(const PxTransform& pose)
: NpRigidStaticT(PxConcreteType::eRIGID_STATIC, PxBaseFlag::eOWNS_MEMORY | PxBaseFlag::eIS_RELEASABLE)
, mRigidStatic(pose)
{
}

NpRigidStatic::~NpRigidStatic()
{
}

// PX_SERIALIZATION
void NpRigidStatic::requires(PxProcessPxBaseCallback& c)
{
	NpRigidStaticT::requires(c);	
}

NpRigidStatic* NpRigidStatic::createObject(PxU8*& address, PxDeserializationContext& context)
{
	NpRigidStatic* obj = new (address) NpRigidStatic(PxBaseFlag::eIS_RELEASABLE);
	address += sizeof(NpRigidStatic);	
	obj->importExtraData(context);
	obj->resolveReferences(context);
	return obj;
}
//~PX_SERIALIZATION

PxActor* NpGetPxActorSC(Sc::StaticCore& scStatic)
{
	char* p = reinterpret_cast<char*>(&scStatic);
	size_t scbOffset = reinterpret_cast<size_t>(&(reinterpret_cast<NpRigidStatic*>(0)->getScbRigidStaticFast()));
	return reinterpret_cast<NpRigidStatic*>(p - scbOffset - Scb::RigidStatic::getScOffset());
}

void NpRigidStatic::release()
{
	NP_WRITE_CHECK(NpActor::getOwnerScene(*this));

	NpPhysics::getInstance().notifyDeletionListenersUserRelease(this, userData);

	Scb::Scene* s = mRigidStatic.getScbSceneForAPI();

	bool noSim = mRigidStatic.getScRigidCore().getActorFlags().isSet(PxActorFlag::eDISABLE_SIMULATION);
	// important to check the non-buffered flag because it tells what the current internal state of the object is
	// (someone might switch to non-simulation and release all while the sim is running). Reading is fine even if 
	// the sim is running because actor flags are read-only internally.
	if (s && noSim)
	{
		// need to do it here because the Np-shape buffer will not be valid anymore after the release below
		// and unlike simulation objects, there is no shape buffer in the simulation controller
		getShapeManager().clearShapesOnRelease(*s, *this);
	}

	NpRigidStaticT::release();

	if (s)
	{
		s->removeRigidStatic(mRigidStatic, true, noSim);
		static_cast<NpScene*>(s->getPxScene())->removeFromRigidActorList(mIndex);
	}

	mRigidStatic.destroy();
}

void NpRigidStatic::setGlobalPose(const PxTransform& pose, bool /*wake*/)
{
	PX_CHECK_AND_RETURN(pose.isSane(), "NpRigidStatic::setGlobalPose: pose is not valid.");

	NpScene* npScene = NpActor::getAPIScene(*this);	// PT: grab this only once since it's slow
	NP_WRITE_CHECK(npScene);

	mRigidStatic.setActor2World(pose.getNormalized());
	

	if(npScene)
	{
		Ps::getFoundation().error(PxErrorCode::ePERF_WARNING, __FILE__, __LINE__, "Static actor moved. Not recommended while the actor is part of a scene.");
		mShapeManager.markAllSceneQueryForUpdate(npScene->getSceneQueryManagerFast());
		npScene->getSceneQueryManagerFast().invalidateStaticTimestamp();
	}

#if PX_SUPPORT_VISUAL_DEBUGGER
	// have to do this here since this call gets not forwarded to Scb::RigidStatic
	Scb::Scene* scbScene = NpActor::getScbFromPxActor(*this).getScbSceneForAPI();
	if(scbScene && scbScene->getSceneVisualDebugger().isConnected())
		scbScene->getSceneVisualDebugger().updatePvdProperties(&mRigidStatic);
#endif

	updateShaderComs();
}

PxTransform NpRigidStatic::getGlobalPose() const
{
	NP_READ_CHECK(NpActor::getOwnerScene(*this));
	return mRigidStatic.getActor2World();
}

PxShape* NpRigidStatic::createShape(const PxGeometry& geometry, PxMaterial*const* materials, PxU16 materialCount, PxShapeFlags shapeFlags)
{
	NP_WRITE_CHECK(NpActor::getOwnerScene(*this));

	PX_CHECK_AND_RETURN_NULL(materials, "createShape: material pointer is NULL");
	PX_CHECK_AND_RETURN_NULL(materialCount>0, "createShape: material count is zero");

	NpShape* shape = static_cast<NpShape*>(NpPhysics::getInstance().createShape(geometry, materials, materialCount, true, shapeFlags));

	if ( shape != NULL )
	{
		mShapeManager.attachShape(*shape, *this);
		GRB_EVENT(getScene(), PxSceneEvent, PxSceneEvent::PxActorCreateShape, static_cast<PxActor *>(this), static_cast<PxShape *>(shape), 1);
		shape->releaseInternal();
	}
	return shape;
}

PxU32 physx::NpRigidStaticGetShapes(Scb::RigidStatic& rigid, void* const *&shapes)
{
	NpRigidStatic* a = static_cast<NpRigidStatic*>(rigid.getScRigidCore().getPxActor());
	NpShapeManager& sm = a->getShapeManager();
	shapes = reinterpret_cast<void *const *>(sm.getShapes());
	return sm.getNbShapes();
}


void NpRigidStatic::switchToNoSim()
{
	getScbRigidStaticFast().switchToNoSim(false);
}


void NpRigidStatic::switchFromNoSim()
{
	getScbRigidStaticFast().switchFromNoSim(false);
}


#if PX_ENABLE_DEBUG_VISUALIZATION
#include "GuDebug.h"
void NpRigidStatic::visualize(Cm::RenderOutput& out, NpScene* scene)
{
	NpRigidStaticT::visualize(out, scene);

	if (getScbRigidStaticFast().getActorFlags() & PxActorFlag::eVISUALIZATION)
	{
		Scb::Scene& scbScene = scene->getScene();
		PxReal scale = scbScene.getVisualizationParameter(PxVisualizationParameter::eSCALE);

		//visualize actor frames
		PxReal actorAxes = scale * scbScene.getVisualizationParameter(PxVisualizationParameter::eACTOR_AXES);
		if (actorAxes != 0)
			out << Gu::Debug::convertToPxMat44(getGlobalPose()) << Cm::DebugBasis(PxVec3(actorAxes));
	}
}
#endif

