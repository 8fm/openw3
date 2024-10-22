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


#include "ScScene.h"
#include "ScConstraintProjectionManager.h"
#include "ScBodySim.h"
#include "ScStaticSim.h"
#include "PxsContext.h"
#include "ScConstraintCore.h"
#include "ScConstraintSim.h"
#include "ScConstraintInteraction.h"
#include "ScRbElementInteraction.h"
#include "CmVisualization.h"

using namespace physx;

Sc::ConstraintSim::ConstraintSim(ConstraintCore& core, 
							 RigidCore* r0,
							 RigidCore* r1,
							 Scene& scene) :
	mScene				(scene),
	mCore				(core),
	mFlags				(0)
{
	mSolverOutput.linearImpulse = PxVec3(0);
	mSolverOutput.angularImpulse = PxVec3(0);
	mSolverOutput.broken = false;

	mBodies[0] = (r0 && (r0->getActorCoreType() != PxActorType::eRIGID_STATIC)) ? static_cast<BodySim*>(r0->getSim()) : 0;
	mBodies[1] = (r1 && (r1->getActorCoreType() != PxActorType::eRIGID_STATIC)) ? static_cast<BodySim*>(r1->getSim()) : 0;
	
	if (!createLLConstraint())
		return;

	PxReal linBreakForce, angBreakForce;
	core.getBreakForce(linBreakForce, angBreakForce);
	if ((linBreakForce < PX_MAX_F32) || (angBreakForce < PX_MAX_F32))
		setFlag(eBREAKABLE);

	core.setSim(this);

	if (needsProjection())
		scene.getProjectionManager().addToPendingGroupUpdates(*this);

	ConstraintSim* cs = this;  // to make the Wii U compiler happy
	mInteraction = mScene.getConstraintInteractionPool()->construct(cs, 
																	r0 ? *r0->getSim() : scene.getStaticAnchor(), 
																	r1 ? *r1->getSim() : scene.getStaticAnchor());

	mInteraction->initialize();			// TODO: what's this?
}


Sc::ConstraintSim::~ConstraintSim()
{
	if (readFlag(ConstraintSim::ePENDING_GROUP_UPDATE))
		mScene.getProjectionManager().removeFromPendingGroupUpdates(*this);

	PX_ASSERT(mInteraction);  // This is fine now, a body which gets removed from the scene removes all constraints automatically
	if (!isBroken())
		mInteraction->destroy();
	else
	{
		// broken constraints should have been destroyed already
		PX_ASSERT(!mInteraction->isRegistered());
	}
	mScene.getConstraintInteractionPool()->destroy(mInteraction);

	destroyLLConstraint();

	mCore.setSim(NULL);
}


bool Sc::ConstraintSim::createLLConstraint()
{
	PxsConstraint& llc = mLowLevelConstraint;
	ConstraintCore& core = getCore();
	PxU32 constantBlockSize = core.getConstantBlockSize();

	void* constantBlock = mScene.allocateConstraintBlock(constantBlockSize);
	if(!constantBlock)
	{
		Ps::getFoundation().error(PxErrorCode::eINTERNAL_ERROR, __FILE__, __LINE__, "Constraint: could not allocate low-level resources.");
		return false;
	}

	//Ensure the constant block isn't just random data because some functions may attempt to use it before it is
	//setup.  Specifically pvd visualization of joints
	//-CN

	PxMemZero( constantBlock, constantBlockSize);

	core.getBreakForce(llc.linBreakForce, llc.angBreakForce);
	llc.flags					= PxU32(core.getFlags());
	llc.solverPrepSpuByteSize	= core.getSolverPrepSpuByteSize();
	llc.constantBlockSize		= constantBlockSize;

	llc.solverPrep				= core.getSolverPrep();
	llc.project					= core.getProject();
	llc.solverPrepSpu			= core.getSolverPrepSpu();
	llc.constantBlock			= constantBlock;

	llc.writeback				= &mSolverOutput;
	llc.body0					= mBodies[0] ? &mBodies[0]->getLowLevelBody() : 0;
	llc.body1					= mBodies[1] ? &mBodies[1]->getLowLevelBody() : 0;
	llc.bodyCore0				= mBodies[0] ? &llc.body0->getCore() : NULL;
	llc.bodyCore1				= mBodies[1] ? &llc.body1->getCore() : NULL;

	return true;
}


void Sc::ConstraintSim::destroyLLConstraint()
{
	if(mLowLevelConstraint.constantBlock)
	{
		mScene.deallocateConstraintBlock(mLowLevelConstraint.constantBlock,
										 mLowLevelConstraint.constantBlockSize);
	}
}


void Sc::ConstraintSim::postBodiesChange(RigidCore* r0, RigidCore* r1)
{
	BodySim* b = getConstraintGroupBody();
	if (b)
	{
		mScene.getProjectionManager().invalidateGroup(*b->getConstraintGroup(), this);
		if (needsProjection() && (!readFlag(ConstraintSim::ePENDING_GROUP_UPDATE)))
			mScene.getProjectionManager().addToPendingGroupUpdates(*this);
	}

	BodySim* b0 = (r0 && (r0->getActorCoreType() != PxActorType::eRIGID_STATIC)) ? static_cast<BodySim*>(r0->getSim()) : 0;
	BodySim* b1 = (r1 && (r1->getActorCoreType() != PxActorType::eRIGID_STATIC)) ? static_cast<BodySim*>(r1->getSim()) : 0;

	PxsConstraint& c = mLowLevelConstraint;

	c.body0 = b0 ? &b0->getLowLevelBody() : NULL;
	c.body1 = b1 ? &b1->getLowLevelBody() : NULL;

	c.bodyCore0 = c.body0 ? &c.body0->getCore() : NULL;
	c.bodyCore1 = c.body1 ? &c.body1->getCore() : NULL;

	PX_ASSERT(mInteraction);
	if (!isBroken())
		mInteraction->destroy();
	else
	{
		// broken constraints should have been destroyed already
		PX_ASSERT(!mInteraction->isRegistered());
	}

	mBodies[0] = b0;
	mBodies[1] = b1;

	mScene.getConstraintInteractionPool()->destroy(mInteraction);

	ConstraintSim* cs = this;  // to make the Wii U compiler happy
	mInteraction = mScene.getConstraintInteractionPool()->construct(cs, 
																	r0 ? *r0->getSim() : mScene.getStaticAnchor(), 
																	r1 ? *r1->getSim() : mScene.getStaticAnchor());

	mInteraction->initialize();
}


void Sc::ConstraintSim::checkMaxForceExceeded()
{
	PX_ASSERT(mInteraction->isRegistered());
	PX_ASSERT(readFlag(eCHECK_MAX_FORCE_EXCEEDED));

	if (mSolverOutput.broken)
	{
		mScene.addBrokenConstraint(&mCore);
		mCore.breakApart();
		PX_ASSERT(mInteraction->isRegistered());
		mInteraction->destroy();

		updateRelatedSIPs();

		PX_ASSERT(!readFlag(eCHECK_MAX_FORCE_EXCEEDED));
	}
}

void Sc::ConstraintSim::getForce(PxVec3& lin, PxVec3& ang)
{
	const PxReal recipDt = mScene.getOneOverDt();
	lin = mSolverOutput.linearImpulse * recipDt;
	ang = mSolverOutput.angularImpulse * recipDt;
}

void Sc::ConstraintSim::updateRelatedSIPs()
{
	Actor& a0 = mInteraction->getActor0();
	Actor& a1 = mInteraction->getActor1();
	Actor& actor = (a0.getActorInteractionCount()<  a1.getActorInteractionCount()) ? a0 : a1;

	Cm::Range<Interaction*const> interactions = actor.getActorInteractions();
	for(; !interactions.empty(); interactions.popFront())
	{
		Interaction *const interaction = interactions.front();
		if(interaction->getInteractionFlags() & PX_INTERACTION_FLAG_RB_ELEMENT)
			((RbElementInteraction*)interaction)->setDirty(CoreInteraction::CIF_DIRTY_FILTER_STATE);
	}
}


void Sc::ConstraintSim::setBreakForceLL(PxReal linear, PxReal angular)
{
	PxU8 wasBreakable = readFlag(eBREAKABLE);
	PxU8 isBreakable;
	if ((linear < PX_MAX_F32) || (angular < PX_MAX_F32))
		isBreakable = eBREAKABLE;
	else
		isBreakable = 0;

	if (isBreakable != wasBreakable)
	{
		if (isBreakable)
		{
			PX_ASSERT(!readFlag(eCHECK_MAX_FORCE_EXCEEDED));
			setFlag(eBREAKABLE);
			if (mScene.getInteractionScene().isActiveInteraction(mInteraction))
				mScene.addActiveBreakableConstraint(this);
		}
		else
		{
			if (readFlag(eCHECK_MAX_FORCE_EXCEEDED))
				mScene.removeActiveBreakableConstraint(this);
			clearFlag(eBREAKABLE);
		}
	}

	mLowLevelConstraint.linBreakForce = linear;
	mLowLevelConstraint.angBreakForce = angular;
}


void Sc::ConstraintSim::postFlagChange(PxConstraintFlags oldFlags, PxConstraintFlags newFlags)
{
	mLowLevelConstraint.flags = newFlags;

	// PT: don't convert to bool if not needed
	const PxU32 hadProjection   = (oldFlags & PxConstraintFlag::ePROJECTION);
	const PxU32 needsProjection = (newFlags & PxConstraintFlag::ePROJECTION);

	if(needsProjection && !hadProjection)
	{
		PX_ASSERT(!readFlag(ConstraintSim::ePENDING_GROUP_UPDATE)); // Non-projecting constrainst should not be part of the update list

		Sc::BodySim* b0 = getBody(0);
		Sc::BodySim* b1 = getBody(1);
		if ((!b0 || b0->getConstraintGroup()) && (!b1 || b1->getConstraintGroup()))
		{
			// Already part of a constraint group but not as a projection constraint -> re-generate projection tree
			PX_ASSERT(b0 != NULL || b1 != NULL);
			if (b0)
				b0->getConstraintGroup()->rebuildProjectionTrees();
			else
				b1->getConstraintGroup()->rebuildProjectionTrees();
		}
		else
		{
			// Not part of a constraint group yet
			mScene.getProjectionManager().addToPendingGroupUpdates(*this);
		}
	}
	else if(!needsProjection && hadProjection)
	{
		if (!readFlag(ConstraintSim::ePENDING_GROUP_UPDATE))
		{
			Sc::BodySim* b = getConstraintGroupBody();
			if (b)
			{
				PX_ASSERT(b->getConstraintGroup());
				mScene.getProjectionManager().invalidateGroup(*b->getConstraintGroup(), NULL);
			}
			// This is conservative but it could be the case that this constraint with projection was the only
			// one in the group and thus the whole group must be killed. If we had a counter for the number of
			// projecting constraints per group, we could just update the projection tree if the counter was
			// larger than 1. But switching the projection flag does not seem likely anyway.
		}
		else
			mScene.getProjectionManager().removeFromPendingGroupUpdates(*this);  // Was part of a group which got invalidated
	}
}


Sc::RigidSim& Sc::ConstraintSim::getRigid(PxU32 i)
{
	PX_ASSERT(mInteraction);

	if (i == 0)
		return static_cast<RigidSim&>(mInteraction->getActor0());
	else
		return static_cast<RigidSim&>(mInteraction->getActor1());
}


bool Sc::ConstraintSim::hasDynamicBody()
{
	return (mBodies[0] && (!mBodies[0]->isKinematic())) || (mBodies[1] && (!mBodies[1]->isKinematic()));
}

bool Sc::ConstraintSim::isBroken() const
{
	return mSolverOutput.broken != 0;
}

void Sc::ConstraintSim::projectPose(BodySim* childBody)
{
#ifdef PX_DEBUG
	// We expect bodies in low level constraints to have same order as high level counterpart
	PxsRigidBody* b0 = mLowLevelConstraint.body0;
	PxsRigidBody* b1 = mLowLevelConstraint.body1;
	PX_ASSERT(	(childBody == getBody(0) && &childBody->getLowLevelBody() == b0) ||
				(childBody == getBody(1) && &childBody->getLowLevelBody() == b1) );
#endif

	PxsConstraintProject(getLowLevelConstraint(), childBody == getBody(1), mScene.getDt());
}


bool Sc::ConstraintSim::needsProjection()
{
	return (getCore().getFlags() & PxConstraintFlag::ePROJECTION ) && !mSolverOutput.broken;
}


PX_INLINE Sc::BodySim* Sc::ConstraintSim::getConstraintGroupBody()
{
	BodySim* b = NULL;
	if (mBodies[0] && mBodies[0]->getConstraintGroup())
		b = mBodies[0];
	else if (mBodies[1] && mBodies[1]->getConstraintGroup())
		b = mBodies[1];

	return b;
}


void Sc::ConstraintSim::visualize(PxRenderBuffer& output)
{
	if (!(getCore().getFlags() & PxConstraintFlag::eVISUALIZATION))
		return;

	PxsRigidBody* b0 = mLowLevelConstraint.body0;
	PxsRigidBody* b1 = mLowLevelConstraint.body1;
	
	const PxTransform& t0 = b0 ? b0->getPose() : PxTransform(PxIdentity);
	const PxTransform& t1 = b1 ? b1->getPose() : PxTransform(PxIdentity);

	PxReal frameScale = mScene.getVisualizationScale() * mScene.getVisualizationParameter(PxVisualizationParameter::eJOINT_LOCAL_FRAMES);
	PxReal limitScale = mScene.getVisualizationScale() * mScene.getVisualizationParameter(PxVisualizationParameter::eJOINT_LIMITS);

	Cm::RenderOutput renderOut( static_cast<Cm::RenderBuffer &>( output ) );
	Cm::ConstraintImmediateVisualizer viz( frameScale, limitScale, renderOut );

	mCore.getVisualize()(viz, mLowLevelConstraint.constantBlock, t0, t1,
		PxConstraintVisualizationFlag::eLOCAL_FRAMES | PxConstraintVisualizationFlag::eLIMITS);
}
