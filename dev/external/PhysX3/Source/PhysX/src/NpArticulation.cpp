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


#include "NpArticulation.h"
#include "NpArticulationLink.h"
#include "NpWriteCheck.h"
#include "NpReadCheck.h"
#include "NpFactory.h"
#include "ScbArticulation.h"
#include "PxTransform.h"
#include "NpAggregate.h"

namespace physx
{

// PX_SERIALIZATION
void NpArticulation::requires(PxProcessPxBaseCallback& c)
{
	// Collect articulation links
	const PxU32 nbLinks = mArticulationLinks.size();
	for(PxU32 i=0; i<nbLinks; i++)
		c.process(*mArticulationLinks[i]);
}

void NpArticulation::exportExtraData(PxSerializationContext& stream)
{
	Cm::exportInlineArray(mArticulationLinks, stream);
	stream.writeName(mName);
}

void NpArticulation::importExtraData(PxDeserializationContext& context)
{	
	Cm::importInlineArray(mArticulationLinks, context);
	context.readName(mName);
}

void NpArticulation::resolveReferences(PxDeserializationContext& context)
{
	const PxU32 nbLinks = mArticulationLinks.size();
	for(PxU32 i=0;i<nbLinks;i++)
	{
		context.translatePxBase(mArticulationLinks[i]);
	}
	
	mAggregate = NULL;	
}

NpArticulation* NpArticulation::createObject(PxU8*& address, PxDeserializationContext& context)
{
	NpArticulation* obj = new (address) NpArticulation(PxBaseFlag::eIS_RELEASABLE);
	address += sizeof(NpArticulation);	
	obj->importExtraData(context);
	obj->resolveReferences(context);
	return obj;
}
//~PX_SERIALIZATION

NpArticulation::NpArticulation() 
:	PxArticulation(PxConcreteType::eARTICULATION, PxBaseFlag::eOWNS_MEMORY | PxBaseFlag::eIS_RELEASABLE)
,	mAggregate(NULL)
,	mName(NULL)
{
	PxArticulation::userData = NULL;
}


NpArticulation::~NpArticulation()
{
	NpFactory::getInstance().onArticulationRelease(this);
}


void NpArticulation::release()
{
	NP_WRITE_CHECK(getNpScene());

	NpPhysics::getInstance().notifyDeletionListenersUserRelease(this, userData);

	//!!!AL TODO: Order should not matter in this case. Optimize by having a path which does not restrict release to leaf links or
	//      by using a more advanced data structure
	PxU32 idx = 0;
	while(mArticulationLinks.size())
	{
		idx = idx % mArticulationLinks.size();

		if (mArticulationLinks[idx]->getNbChildren() == 0)
		{
			mArticulationLinks[idx]->releaseInternal();  // deletes joint, link and removes link from list
		}
		else
		{
			idx++;
		}
	}

	NpScene* npScene = getNpScene();
	if(npScene)
	{
		npScene->getScene().removeArticulation(getArticulation());

		npScene->removeFromArticulationList(*this);
	}

	mArticulationLinks.clear();

	mArticulation.destroy();
}


PxScene* NpArticulation::getScene() const
{
	return getNpScene();
}


PxU32 NpArticulation::getInternalDriveIterations() const
{
	NP_READ_CHECK(getNpScene());
	return getArticulation().getInternalDriveIterations();
}

void NpArticulation::setInternalDriveIterations(PxU32 iterations)
{
	NP_WRITE_CHECK(getNpScene());
	getArticulation().setInternalDriveIterations(iterations);
}

PxU32 NpArticulation::getExternalDriveIterations() const
{
	NP_READ_CHECK(getNpScene());
	return getArticulation().getExternalDriveIterations();
}

void NpArticulation::setExternalDriveIterations(PxU32 iterations)
{
	NP_WRITE_CHECK(getNpScene());
	getArticulation().setExternalDriveIterations(iterations);
}

PxU32 NpArticulation::getMaxProjectionIterations() const
{
	NP_READ_CHECK(getNpScene());
	return getArticulation().getMaxProjectionIterations();
}

void NpArticulation::setMaxProjectionIterations(PxU32 iterations)
{
	NP_WRITE_CHECK(getNpScene());
	getArticulation().setMaxProjectionIterations(iterations);
}

PxReal NpArticulation::getSeparationTolerance() const
{
	NP_READ_CHECK(getNpScene());
	return getArticulation().getSeparationTolerance();
}

void NpArticulation::setSeparationTolerance(PxReal tolerance)
{
	NP_WRITE_CHECK(getNpScene());
	getArticulation().setSeparationTolerance(tolerance);
}

void NpArticulation::setSolverIterationCounts(PxU32 positionIters, PxU32 velocityIters)
{
	NP_WRITE_CHECK(getNpScene());
	PX_CHECK_AND_RETURN(positionIters > 0, "Articulation::setSolverIterationCount: positionIters must be more than zero!");
	PX_CHECK_AND_RETURN(positionIters <= 255, "Articulation::setSolverIterationCount: positionIters must be no greater than 255!");
	PX_CHECK_AND_RETURN(velocityIters > 0, "Articulation::setSolverIterationCount: velocityIters must be more than zero!");
	PX_CHECK_AND_RETURN(velocityIters <= 255, "Articulation::setSolverIterationCount: velocityIters must be no greater than 255!");

	getArticulation().setSolverIterationCounts((velocityIters & 0xff) << 8 | (positionIters & 0xff));
}


void NpArticulation::getSolverIterationCounts(PxU32 & positionIters, PxU32 & velocityIters) const
{
	NP_READ_CHECK(getNpScene());
	PxU16 x = getArticulation().getSolverIterationCounts();
	velocityIters = x >> 8;
	positionIters = x & 0xff;
}


bool NpArticulation::isSleeping() const
{
	NpScene* scene = getNpScene();
	PX_UNUSED(scene);

	NP_READ_CHECK(scene);
	PX_CHECK_AND_RETURN_VAL(scene, "Articulation::isSleeping: articulation must be in a scene.", true);

	return getArticulation().isSleeping();
}


void NpArticulation::setSleepThreshold(PxReal threshold)
{
	NP_WRITE_CHECK(getNpScene());
	getArticulation().setSleepThreshold(threshold);
}


PxReal NpArticulation::getSleepThreshold() const
{
	NP_READ_CHECK(getNpScene());
	return getArticulation().getSleepThreshold();
}


void NpArticulation::setWakeCounter(PxReal wakeCounterValue)
{
	NP_WRITE_CHECK(getNpScene());

	for(PxU32 i=0; i < mArticulationLinks.size(); i++)
	{
		mArticulationLinks[i]->getScbBodyFast().setWakeCounter(wakeCounterValue);
	}

	getArticulation().setWakeCounter(wakeCounterValue);
}


PxReal NpArticulation::getWakeCounter() const
{
	NP_READ_CHECK(getNpScene());
	return getArticulation().getWakeCounter();
}


void NpArticulation::wakeUpInternal(bool forceWakeUp, bool autowake)
{
	NpScene* scene = getNpScene();
	PX_ASSERT(scene);
	PxReal wakeCounterResetValue = scene->getWakeCounterResetValueInteral();

	Scb::Articulation& a = getArticulation();
	PxReal wakeCounter = a.getWakeCounter();

	bool needsWakingUp = isSleeping() && (autowake || forceWakeUp);
	if (autowake && (wakeCounter < wakeCounterResetValue))
	{
		wakeCounter = wakeCounterResetValue;
		needsWakingUp = true;
	}

	if (needsWakingUp)
	{
		for(PxU32 i=0; i < mArticulationLinks.size(); i++)
		{
			mArticulationLinks[i]->getScbBodyFast().wakeUpInternal(wakeCounter);
		}

		a.wakeUpInternal(wakeCounter);
	}
}


void NpArticulation::wakeUp()
{
	NpScene* scene = getNpScene();
	PX_UNUSED(scene);

	NP_WRITE_CHECK(scene);
	PX_CHECK_AND_RETURN(scene, "Articulation::wakeUp: articulation must be in a scene.");

	for(PxU32 i=0; i < mArticulationLinks.size(); i++)
	{
		mArticulationLinks[i]->getScbBodyFast().wakeUpInternal(scene->getWakeCounterResetValueInteral());
	}

	getArticulation().wakeUp();
}


void NpArticulation::putToSleep()
{
	NpScene* scene = getNpScene();
	PX_UNUSED(scene);

	NP_WRITE_CHECK(scene);
	PX_CHECK_AND_RETURN(scene, "Articulation::putToSleep: articulation must be in a scene.");

	for(PxU32 i=0; i < mArticulationLinks.size(); i++)
	{
		mArticulationLinks[i]->getScbBodyFast().putToSleepInternal();
	}

	getArticulation().putToSleep();
}


PxArticulationLink*	NpArticulation::createLink(PxArticulationLink* parent, const PxTransform& pose)
{
	PX_CHECK_AND_RETURN_NULL(pose.isSane(), "NpArticulation::createLink pose is not valid.");
	
	NP_WRITE_CHECK(getNpScene());
	
	if(parent && mArticulationLinks.empty())
	{
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Root articulation link must have NULL parent pointer!");
		return NULL;
	}

	if(!parent && !mArticulationLinks.empty())
	{
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Non-root articulation link must have valid parent pointer!");
		return NULL;
	}

	NpArticulationLink* parentLink = static_cast<NpArticulationLink*>(parent);

	NpArticulationLink* link = static_cast<NpArticulationLink*>(NpFactory::getInstance().createArticulationLink(*this, parentLink, pose.getNormalized()));

	if(link)
	{
		if(getNpScene())
			getNpScene()->addArticulationLink(*link);
	}
	return link;
}


PxU32 NpArticulation::getNbLinks() const
{
	NP_READ_CHECK(getNpScene());
	return mArticulationLinks.size();
}


PxU32 NpArticulation::getLinks(PxArticulationLink** buffer, PxU32 bufferSize) const
{
	NP_READ_CHECK(getNpScene());
	const PxU32 size = mArticulationLinks.size();

	const PxU32 writeCount = PxMin(size, bufferSize);
	for(PxU32 i=0; i<writeCount; i++)
		buffer[i] = mArticulationLinks[i];

	return writeCount;
//	return Ps::dumpPointerArray((const void**)mArticulationLinks.begin(), mArticulationLinks.size(), (void**)buffer, bufferSize);
}


PxBounds3 NpArticulation::getWorldBounds(float inflation) const
{
	NP_READ_CHECK(getNpScene());
	PxBounds3 bounds = PxBounds3::empty();

	for(PxU32 i=0; i < mArticulationLinks.size(); i++)
	{
		bounds.include(mArticulationLinks[i]->getWorldBounds());
	}
	PX_ASSERT(bounds.isValid());

	// PT: unfortunately we can't just scale the min/max vectors, we need to go through center/extents.
	const PxVec3 center = bounds.getCenter();
	const PxVec3 inflatedExtents = bounds.getExtents() * inflation;
	return PxBounds3::centerExtents(center, inflatedExtents);
}


PxAggregate* NpArticulation::getAggregate() const
{
	NP_READ_CHECK(getNpScene());
	return mAggregate;
}


void NpArticulation::setName(const char* debugName)
{
	NP_WRITE_CHECK(getNpScene());
	mName = debugName;
}


const char* NpArticulation::getName() const
{
	NP_READ_CHECK(getNpScene());
	return mName;
}


NpScene* NpArticulation::getNpScene() const
{
	return static_cast<NpScene*>(mArticulation.getScbSceneForAPI() ? mArticulation.getScbSceneForAPI()->getPxScene() : NULL);
}


#if PX_ENABLE_DEBUG_VISUALIZATION
void NpArticulation::visualize(Cm::RenderOutput& out, NpScene* scene)
{
	for(PxU32 i=0;i<mArticulationLinks.size();i++)
		mArticulationLinks[i]->visualize(out, scene);
}
#endif  // PX_ENABLE_DEBUG_VISUALIZATION

PxArticulation* NpGetPxArticulation(Sc::ArticulationCore& constraint)
{
	char* p = reinterpret_cast<char*>(&constraint);
	size_t scbOffset = reinterpret_cast<size_t>(&(reinterpret_cast<NpArticulation*>(0)->getScbArticulation()));
	return reinterpret_cast<NpArticulation*>(p - scbOffset - Scb::Articulation::getScOffset());
}

const PxArticulation* NpGetPxArticulation(const Sc::ArticulationCore& constraint)
{
	const char* p = reinterpret_cast<const char*>(&constraint);
	size_t scbOffset = reinterpret_cast<size_t>(&(reinterpret_cast<const NpArticulation*>(0)->getScbArticulation()));
	return reinterpret_cast<const NpArticulation*>(p - scbOffset - Scb::Articulation::getScOffset());
}


PxArticulationDriveCache* NpArticulation::createDriveCache(PxReal compliance, PxU32 driveIterations) const
{
	PX_CHECK_AND_RETURN_NULL(mArticulation.getScbScene()!=NULL, "PxArticulation::createDriveCache: object must be in a scene");
	NP_READ_CHECK(getNpScene());	// doesn't modify the scene, only reads

	return reinterpret_cast<PxArticulationDriveCache*>(mArticulation.getScArticulation().createDriveCache(compliance, driveIterations));
}


void NpArticulation::updateDriveCache(PxArticulationDriveCache& cache, PxReal compliance, PxU32 driveIterations) const
{
	PX_CHECK_AND_RETURN(mArticulation.getScbScene()!=NULL, "PxArticulation::updateDriveCache: object must be in a scene");

	Sc::ArticulationDriveCache& c = reinterpret_cast<Sc::ArticulationDriveCache&>(cache);
	PX_CHECK_AND_RETURN(mArticulation.getScArticulation().getCacheLinkCount(c) == mArticulationLinks.size(), "PxArticulation::updateDriveCache: Articulation size has changed; drive cache is invalid");

	NP_READ_CHECK(getNpScene());	// doesn't modify the scene, only reads
	mArticulation.getScArticulation().updateDriveCache(c, compliance, driveIterations);
}

void NpArticulation::releaseDriveCache(PxArticulationDriveCache&cache ) const
{
	PX_CHECK_AND_RETURN(mArticulation.getScbScene()!=NULL, "PxArticulation::releaseDriveCache: object must be in a scene");
	NP_READ_CHECK(getNpScene());	// doesn't modify the scene, only reads

	mArticulation.getScArticulation().releaseDriveCache(reinterpret_cast<Sc::ArticulationDriveCache&>(cache));
}

void NpArticulation::applyImpulse(PxArticulationLink* link,
								  const PxArticulationDriveCache& driveCache,
								  const PxVec3& force,
								  const PxVec3& torque)
{
	PX_CHECK_AND_RETURN(mArticulation.getScbScene()!=NULL, "PxArticulation::applyImpulse: object must be in a scene");
	PX_CHECK_AND_RETURN(force.isFinite() && torque.isFinite(), "PxArticulation::applyImpulse: invalid force/torque");
	const Sc::ArticulationDriveCache& c = reinterpret_cast<const Sc::ArticulationDriveCache&>(driveCache);
	PX_CHECK_AND_RETURN(mArticulation.getScArticulation().getCacheLinkCount(c) == mArticulationLinks.size(), "PxArticulation::applyImpulse: Articulation size has changed; drive cache is invalid");

	NP_WRITE_CHECK(getNpScene());

	if(isSleeping())
		wakeUp();

	mArticulation.getScArticulation().applyImpulse(static_cast<NpArticulationLink*>(link)->getScbBodyFast().getScBody(), c,force, torque);
	for(PxU32 i=0;i<mArticulationLinks.size();i++)
	{
		PxVec3 lv = mArticulationLinks[i]->getScbBodyFast().getScBody().getLinearVelocity(),
			   av = mArticulationLinks[i]->getScbBodyFast().getScBody().getAngularVelocity();
		mArticulationLinks[i]->setLinearVelocity(lv);
		mArticulationLinks[i]->setAngularVelocity(av);
	}
}

void NpArticulation::computeImpulseResponse(PxArticulationLink* link,
											PxVec3& linearResponse, 
											PxVec3& angularResponse,
											const PxArticulationDriveCache& driveCache,
											const PxVec3& force,
											const PxVec3& torque) const
{

	PX_CHECK_AND_RETURN(mArticulation.getScbScene()!=NULL, "PxArticulation::computeImpulseResponse: object must be in a scene");
	PX_CHECK_AND_RETURN(force.isFinite() && torque.isFinite(), "PxArticulation::computeImpulseResponse: invalid force/torque");
	NP_READ_CHECK(getNpScene());

	const Sc::ArticulationDriveCache& c = reinterpret_cast<const  Sc::ArticulationDriveCache&>(driveCache);
	PX_CHECK_AND_RETURN(mArticulation.getScArticulation().getCacheLinkCount(c) == mArticulationLinks.size(), "PxArticulation::computeImpulseResponse: Articulation size has changed; drive cache is invalid");
	PX_UNUSED(&c);

	mArticulation.getScArticulation().computeImpulseResponse(static_cast<NpArticulationLink*>(link)->getScbBodyFast().getScBody(),
															 linearResponse, angularResponse,
															 reinterpret_cast<const Sc::ArticulationDriveCache&>(driveCache),
															 force, torque);
}

NpArticulationLink* NpArticulation::getRoot()
{
	if(!mArticulationLinks.size())
		return NULL;

	PX_ASSERT(mArticulationLinks[0]->getInboundJoint()==NULL);
	return mArticulationLinks[0];
}


Scb::Body* NpArticulationGetRootFromScb(Scb::Articulation&c)
{
	const size_t offset = size_t(&(reinterpret_cast<NpArticulation*>(0)->getScbArticulation()));
	NpArticulation* np = reinterpret_cast<NpArticulation*>(reinterpret_cast<char*>(&c)-offset);

	NpArticulationLink* a = np->getRoot();
	return a ? &a->getScbBodyFast() : NULL;
}

}
