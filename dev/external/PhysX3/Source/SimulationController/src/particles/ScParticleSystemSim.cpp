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


#include "ScParticleSystemSim.h"
#if PX_USE_PARTICLE_SYSTEM_API

#include "ScParticlePacketShape.h"
#include "ScParticleBodyInteraction.h"

#include "ScParticleSystemCore.h"

#include "ScPhysics.h"

#include "ScNPhaseCore.h"
#include "ScBodySim.h"
#include "ScStaticSim.h"
#include "PxParticleReadData.h"
#include "PxsContext.h"
#include "PxvParticleSystemSim.h"
#include "PxsParticleContactManagerStream.h"
#include "CmEventProfiler.h"

using namespace physx;

//----------------------------------------------------------------------------//

Sc::ParticleSystemSim::ParticleSystemSim(Scene& scene, ParticleSystemCore& core)
: ActorSim(scene, core, IslandNodeInfo::eNON_PARTICIPANT)
, mParticlePacketShapePool(PX_DEBUG_EXP("ParticlePacketShapePool"))
, mParticlePacketShapes(PX_DEBUG_EXP("ParticleSysPacketShapes"))
, mInteractionCount(0)
, mCollisionInputPrepTask(this, "Sc::ParticleSystemSim::prepareCollisionInput")
{
	// Set size of interaction list
	Actor::setInteractionCountHint(32);
	
	PxsContext* llContext = getScene().getInteractionScene().getLowLevelContext();
	PxsParticleData* particleData = core.obtainStandaloneData();
	PX_ASSERT(particleData);
	
	bool useGpu = getCore().getFlags() & PxParticleBaseFlag::eGPU;
	mLLSim = llContext->addParticleSystem(particleData, core.getLowLevelParameter(), useGpu);
	
	//CPU fall back
	if (!mLLSim && useGpu)
	{
		getFoundation().error(PxErrorCode::eDEBUG_WARNING, __FILE__, __LINE__, "GPU particle system creation failed. Falling back to CPU implementation.");
		mLLSim = llContext->addParticleSystem(particleData, core.getLowLevelParameter(), false);
		getCore().notifyCpuFallback();
	}

	if (mLLSim)
	{
		if (getCore().getFlags() & PxParticleBaseFlag::eENABLED)
		{
			mLLSim->setSimulatedV(true);
		}
	}
	else
	{
		core.setSim(NULL);
		core.returnStandaloneData(particleData);
	}
}

//----------------------------------------------------------------------------//

void Sc::ParticleSystemSim::release(bool releaseParticleData)
{
	releaseParticlePacketShapes();

	if (mLLSim) //might be not the case if low level sim creation failed.
	{
		PxsContext* llContext = getScene().getInteractionScene().getLowLevelContext();

		PxsParticleData* particleData = llContext->removeParticleSystem(mLLSim, !releaseParticleData);
		if (!releaseParticleData)
		{
			PX_ASSERT(particleData);
			getCore().returnStandaloneData(particleData);
		}
		getCore().setSim(NULL);
	}
	delete this;
}

//----------------------------------------------------------------------------//

Sc::ParticleSystemCore&	Sc::ParticleSystemSim::getCore() const
{
	return static_cast<ParticleSystemCore&>(Sc::ActorSim::getActorCore());
}

//----------------------------------------------------------------------------//

#if PX_SUPPORT_GPU_PHYSX
void Sc::ParticleSystemSim::getReadWriteCudaBuffersGpu(struct PxCudaReadWriteParticleBuffers& buffers)
{
	PX_ASSERT(mLLSim);
	mLLSim->getReadWriteCudaBuffersGpuV(buffers);
}

//----------------------------------------------------------------------------//

void Sc::ParticleSystemSim::setValidParticleRangeGpu(PxU32 validParticleRange)
{
	PX_ASSERT(mLLSim);
	mLLSim->setValidParticleRangeGpuV(validParticleRange);
}

//----------------------------------------------------------------------------//

void Sc::ParticleSystemSim::setDeviceExclusiveModeFlagsGpu(PxU32 flags)
{
	PX_ASSERT(mLLSim);
	mLLSim->setDeviceExclusiveModeFlagsGpuV(flags);
}

//----------------------------------------------------------------------------//

bool Sc::ParticleSystemSim::isInDeviceExclusiveModeGpu() const
{
	PX_ASSERT(mLLSim);
	return mLLSim->isInDeviceExclusiveModeGpuV() != 0;
}

#endif

//----------------------------------------------------------------------------//

void Sc::ParticleSystemSim::unlinkParticleShape(ParticlePacketShape* particleShape)
{
	// Remove the specified particle shape from the shape list
	PX_ASSERT(particleShape);
	PX_ASSERT(mParticlePacketShapes.size() > 0);
	PxU32 index = particleShape->getIndex();
	PX_ASSERT(mParticlePacketShapes[index] == particleShape);
	mParticlePacketShapes.back()->setIndex(index);
	mParticlePacketShapes.replaceWithLast(index);
}

//----------------------------------------------------------------------------//

void Sc::ParticleSystemSim::releaseParticlePacketShapes()
{
	PxU32 shapesCount = mParticlePacketShapes.size();
	for(PxU32 i=0; i < shapesCount; i++)
	{
		ParticlePacketShape* fs = mParticlePacketShapes.back();
		mParticlePacketShapePool.destroy(fs); // The shape will remove itself from the particle shape list
	}
}

//----------------------------------------------------------------------------//

PxU32 Sc::ParticleSystemSim::getInternalFlags() const
{
	return getCore().getInternalFlags();
}

//----------------------------------------------------------------------------//

PxFilterData Sc::ParticleSystemSim::getSimulationFilterData() const
{
	return getCore().getSimulationFilterData();
}

//----------------------------------------------------------------------------//

void Sc::ParticleSystemSim::scheduleRefiltering()
{
	// Schedule a refiltering
	for (PxU32 i=0; i < mParticlePacketShapes.size(); i++)
	{
		mParticlePacketShapes[i]->setInteractionsDirty(CoreInteraction::CIF_DIRTY_FILTER_STATE);
	}
}

//----------------------------------------------------------------------------//

void Sc::ParticleSystemSim::resetFiltering()
{
	// Remove all shapes from broadphase and add again

	Sc::Scene& scene = getScene();
	const PxU32 size = mParticlePacketShapes.size();
	for (PxU32 i=0; i<size; i++)
	{
		scene.removeBroadPhaseVolume(*mParticlePacketShapes[i]);
		scene.addBroadPhaseVolume(*mParticlePacketShapes[i]);
	}
}

//----------------------------------------------------------------------------//

void Sc::ParticleSystemSim::setFlags(PxU32 flags)
{
	if ((getCore().getFlags() & PxParticleBaseFlag::eENABLED) && !(flags & PxParticleBaseFlag::eENABLED))
	{
		mLLSim->setSimulatedV(true);
	}
	else if (!(getCore().getFlags() & PxParticleBaseFlag::eENABLED) && (flags & PxParticleBaseFlag::eENABLED))
	{
		mLLSim->setSimulatedV(false);
	}
}

//----------------------------------------------------------------------------//

void Sc::ParticleSystemSim::getSimParticleData(PxvParticleSystemSimDataDesc& simParticleData, bool devicePtr) const
{
	PX_ASSERT(mLLSim);
	mLLSim->getSimParticleDataV(simParticleData, devicePtr);
}

//----------------------------------------------------------------------------//

PxvParticleSystemState&	Sc::ParticleSystemSim::getParticleState()
{
	PX_ASSERT(mLLSim);
	return mLLSim->getParticleStateV();
}

//----------------------------------------------------------------------------//

void Sc::ParticleSystemSim::addInteraction(const ParticlePacketShape& particleShape, const ShapeSim& shape, bool secondaryBroadphase)
{
	PX_ASSERT(mLLSim);
	const PxsShapeCore& shapeCore = shape.getCore().getCore();
	bool isDynamic = shape.actorIsDynamic();
	PxsRigidCore& rigidCore = shape.getPxsRigidCore();
	if(isDynamic)
		getInteractionScene().getLowLevelContext()->getBodyTransformVault().addBody(static_cast<PxsBodyCore&>(rigidCore));
		
	mLLSim->addInteractionV(*particleShape.getLowLevelParticleShape(), (PxvShapeHandle)&shapeCore, (PxvBodyHandle)&rigidCore, isDynamic, secondaryBroadphase);
	mInteractionCount++; 
}

//----------------------------------------------------------------------------//

void Sc::ParticleSystemSim::removeInteraction(const ParticlePacketShape& particleShape, const ShapeSim& shape, bool isDyingRb, bool secondaryBroadphase) 
{
	PX_ASSERT(mLLSim);
	const PxsShapeCore& shapeCore = shape.getCore().getCore();
	bool isDynamic = shape.actorIsDynamic();
	PxsRigidCore& rigidCore = shape.getPxsRigidCore();
	if(isDynamic)
		getInteractionScene().getLowLevelContext()->getBodyTransformVault().removeBody(static_cast<PxsBodyCore&>(rigidCore));
	
	mLLSim->removeInteractionV(*particleShape.getLowLevelParticleShape(), (PxvShapeHandle)&shapeCore, (PxvBodyHandle)&rigidCore, isDynamic, isDyingRb, secondaryBroadphase);
	mInteractionCount--; 
}

//----------------------------------------------------------------------------//

void Sc::ParticleSystemSim::onRbShapeChange(const ParticlePacketShape& particleShape, const ShapeSim& shape)
{
	PX_ASSERT(mLLSim);
//	const PxsShapeCore& shapeCore = shape.getCore().getCore();
	const PxsBodyCore* bodyCore = shape.getBodySim() ? &shape.getBodySim()->getBodyCore().getCore() : NULL;
	
	mLLSim->onRbShapeChangeV(*particleShape.getLowLevelParticleShape(), (PxvShapeHandle)&shape.getCore().getCore(), (PxvBodyHandle)bodyCore);
}

//----------------------------------------------------------------------------//

PX_FORCE_INLINE void addImpulseAtPos(PxsBodyCore& bodyCore, const PxVec3& impulse, const PxVec3& worldPos)
{
	const PxTransform& pose = bodyCore.body2World;
	PxVec3 torque = (worldPos - pose.p).cross(impulse);

	bodyCore.linearVelocity +=  impulse * bodyCore.inverseMass;
	bodyCore.angularVelocity += pose.q.rotate(bodyCore.inverseInertia.multiply(pose.q.rotateInv(torque)));
}

//----------------------------------------------------------------------------//

/**
currently, it may be that shape id's from removed shapes are reported. 
Try to catch these and ignore them in order to avoid artefacts. 
*/
void Sc::ParticleSystemSim::updateRigidBodies()
{
	if (!(getCore().getFlags() & PxParticleBaseFlag::eCOLLISION_TWOWAY)
		|| !(getCore().getFlags() & PxParticleBaseFlag::eCOLLISION_WITH_DYNAMIC_ACTORS)) 
		return;

	PxReal particleMass = getCore().getParticleMass();

	//Particle data might not even be available if count is zero.
	if (getParticleState().getParticleCountV() == 0)
		return;

	PxvParticleSystemStateDataDesc particlesCore;
	getParticleState().getParticlesV(particlesCore, false, false);

	if (particlesCore.validParticleRange == 0)
		return;
	
	PX_ASSERT(particlesCore.bitMap);
	PX_ASSERT(particlesCore.flags.ptr());
	PX_ASSERT(particlesCore.positions.ptr());

	PxvParticleSystemSimDataDesc particlesSim;
	getSimParticleData(particlesSim, false);
	PX_ASSERT(particlesSim.twoWayImpluses.ptr());
	PX_ASSERT(particlesSim.twoWayBodies.ptr());

	Cm::BitMap::Iterator it(*particlesCore.bitMap);
	for (PxU32 p = it.getNext(); p != Cm::BitMap::Iterator::DONE; p = it.getNext()) 
	{
		if (!particlesSim.twoWayBodies[p])
			continue;

		PxvParticleFlags flags = particlesCore.flags[p];
		PX_ASSERT(flags.api & PxParticleFlag::eCOLLISION_WITH_DYNAMIC);		

		PxsBodyCore& body = *reinterpret_cast<PxsBodyCore*>(particlesSim.twoWayBodies[p]);
		BodyCore& scBody = BodyCore::getCore(body);

		if (scBody.getCore().inverseMass == 0.0f) // kinematic
			continue;

		PxDominanceGroupPair cdom = getScene().getDominanceGroupPair(getCore().getDominanceGroup(), scBody.getDominanceGroup());

		if (cdom.dominance0 == 0.0f)
			continue;

		if (cdom.dominance1 == 0.0f)
			PX_WARN_ONCE(true, "Dominance: unsupport particle dominance is 1.0 and rigid body dominance is 0.0");
			
		if (flags.api & PxParticleFlag::eCOLLISION_WITH_DRAIN)
			continue;

		const PxVec3& position = particlesCore.positions[p];
		const PxVec3& impulse = particlesSim.twoWayImpluses[p];
		PX_ASSERT(impulse.isFinite());

		if (!impulse.isZero())
		{
			PX_ASSERT(scBody.getSim());
			scBody.getSim()->internalWakeUp();
			addImpulseAtPos(scBody.getCore(), impulse*particleMass, position);
		}
	}
}

//----------------------------------------------------------------------------//

/* particle update has the following structure:
 *
 *	The update process for the particle array in low level is to process the active 
 *	particle buffer by removing the deleted particles and moving particles from 
 *	the top of the array to fill the spaces, then to append the new particles to 
 *	the array. Using this knowledge we synchronize the IndexMap on the host. 
 *
 *	TODO: there's an obvious optimization here: replacing deleted particles in place with 
 *	new particles, and then just processing remaining deletes and adds.
 */
void Sc::ParticleSystemSim::startStep()
{
	CM_PROFILE_ZONE_WITH_SUBSYSTEM(getScene(),Sim,ParticleSystemSim_startStep);

	PxVec3 externalAcceleration = getCore().getExternalAcceleration();
	if ((getCore().getActorFlags() & PxActorFlag::eDISABLE_GRAVITY) == false)
		externalAcceleration += getScene().getGravity();

	mLLSim->setExternalAccelerationV(externalAcceleration);

	// Set simulation time step
	mLLSim->setSimulationTimeStepV(static_cast<PxReal>(getScene().getDt()));
}

//----------------------------------------------------------------------------//

void Sc::ParticleSystemSim::addParticlePacket(PxvParticleShape* llParticleShape)
{
	PX_ASSERT(llParticleShape);

	PxU32 index = mParticlePacketShapes.size();
	Sc::ParticlePacketShape* particleShape = mParticlePacketShapePool.construct(*this, index, llParticleShape);

	if (particleShape)
	{
		mParticlePacketShapes.pushBack(particleShape);	// Add shape to list.
	}
	else
	{
		llParticleShape->destroyV();
	}
}

//----------------------------------------------------------------------------//

void Sc::ParticleSystemSim::removeParticlePacket(const PxvParticleShape * llParticleShape)
{
	// Find corresponding particle packet shape.
	void* data = llParticleShape->getUserDataV();
	PX_ASSERT(data);
	ParticlePacketShape* particleShape = reinterpret_cast<ParticlePacketShape*>(data);
	// Cleanup shape. The shape will remove itself from particle shape list.
	mParticlePacketShapePool.destroy(particleShape);
}

//----------------------------------------------------------------------------//

/**
- Create / delete / update particle shapes.

Only call this function when the packet/shapes update task for the particle system has successful finished
and after the whole particle pipeline has finished.
*/
void Sc::ParticleSystemSim::processShapesUpdate()
{
	CM_PROFILE_ZONE(getScene().getEventProfiler(),Cm::ProfileEventId::Sim::GetParticleSystemSim_shapesUpdateProcessing() );

	PxvParticleShapeUpdateResults updateResults;
	mLLSim->getShapesUpdateV(updateResults);

	//
	// Process particle shape update results
	//

	for(PxU32 i=0; i < updateResults.destroyedShapeCount; i++)
	{
		// Delete particle packet
		PX_ASSERT(updateResults.destroyedShapes[i]);
		removeParticlePacket(updateResults.destroyedShapes[i]);
	}

	for(PxU32 i=0; i < updateResults.createdShapeCount; i++)
	{
		// Create new particle packet
		PX_ASSERT(updateResults.createdShapes[i]);
		addParticlePacket(updateResults.createdShapes[i]);
	}
}

//----------------------------------------------------------------------------//

void Sc::ParticleSystemSim::endStep()
{
	CM_PROFILE_ZONE_WITH_SUBSYSTEM(getScene(),Sim,ParticleSystemSim_endStep );
	
	// applies int buffered interaction updates that where produced asynchronously to gpu execution.
	// this has to go before processShapesUpdate, since that will update interactions as well
	mLLSim->flushBufferedInteractionUpdatesV();

	// some lowlevel implementations (gpu) got an update at the end of the step.
	processShapesUpdate();

	// 2-way interaction
	updateRigidBodies();
}

//----------------------------------------------------------------------------//

#if PX_ENABLE_DEBUG_VISUALIZATION

/**
Render particle system state before simulation starts. CollisionNormals are a product of of simulate and not available here.
*/
void Sc::ParticleSystemSim::visualizeStartStep(Cm::RenderOutput& out)
{
	if (!(getCore().getActorFlags() & PxActorFlag::eVISUALIZATION)) 
		return;

	//all particle visualization in world space!
	out << PxTransform(PxIdentity);

	if (getScene().getVisualizationParameter(PxVisualizationParameter::ePARTICLE_SYSTEM_BOUNDS) > 0.0f)
		visualizeParticlesBounds(out);

	visualizeParticles(out);

	if (getScene().getVisualizationParameter(PxVisualizationParameter::ePARTICLE_SYSTEM_GRID) > 0.0f)
		visualizeSpatialGrid(out);

	if (getScene().getVisualizationParameter(PxVisualizationParameter::ePARTICLE_SYSTEM_BROADPHASE_BOUNDS) > 0.0f)
		visualizeBroadPhaseBounds(out);

	if(0) // MS: Might be helpful for debugging
		visualizeInteractions(out);	
}

/**
Render particle system data that is available after simulation and not up to date after the application made updates (CollisionNormals). 
*/
void Sc::ParticleSystemSim::visualizeEndStep(Cm::RenderOutput& out)
{
	if (!(getCore().getActorFlags() & PxActorFlag::eVISUALIZATION)) 
		return;

	//all particle visualization in world space!
	out << PxTransform(PxIdentity);

	visualizeCollisionNormals(out);
}

//----------------------------------------------------------------------------//

void Sc::ParticleSystemSim::visualizeParticlesBounds(Cm::RenderOutput& out)
{
	PxBounds3 bounds = getParticleState().getWorldBoundsV();
	out << PxU32(PxDebugColor::eARGB_RED) << Cm::DebugBox(bounds);
}

//----------------------------------------------------------------------------//

void Sc::ParticleSystemSim::visualizeSpatialGrid(Cm::RenderOutput& out)
{
	PxReal packetSize = getCore().getGridSize();

	for (PxU32 i = 0; i < mParticlePacketShapes.size(); i++)
	{
		ParticlePacketShape* particleShape = mParticlePacketShapes[i];

		PxBounds3 bounds = particleShape->getBounds();
		PxVec3 centerGridSpace = bounds.getCenter() / packetSize;		
		
		for (PxU32 d = 0; d < 3; d++)
			bounds.minimum[d] = Ps::floor(centerGridSpace[d]) * packetSize;
		for (PxU32 d = 0; d < 3; d++)
			bounds.maximum[d] = Ps::ceil(centerGridSpace[d]) * packetSize;

		out << PxU32(PxDebugColor::eARGB_BLUE) << Cm::DebugBox(bounds);
	}
}

//----------------------------------------------------------------------------//

void Sc::ParticleSystemSim::visualizeBroadPhaseBounds(Cm::RenderOutput& out)
{
	for (PxU32 i = 0; i < mParticlePacketShapes.size(); i++)
	{
		ParticlePacketShape* particleShape = mParticlePacketShapes[i];
		PxBounds3 bounds = particleShape->getBounds();
		out << PxU32(PxDebugColor::eARGB_BLUE) << Cm::DebugBox(bounds);
	}
}

//----------------------------------------------------------------------------//

void Sc::ParticleSystemSim::visualizeParticles(Cm::RenderOutput& out)
{
	PxReal timestep = getScene().getDt();

	PxvParticleSystemStateDataDesc particlesCore;
	getParticleState().getParticlesV(particlesCore, false, false);
	
	if (particlesCore.numParticles == 0)
		return;
		
	PX_ASSERT(particlesCore.bitMap);

	bool arePositionsReadeable = getCore().getParticleReadDataFlags() & PxParticleReadDataFlag::ePOSITION_BUFFER;
	bool areVelocitiesReadeable = getCore().getParticleReadDataFlags() & PxParticleReadDataFlag::eVELOCITY_BUFFER;

	if (getScene().getVisualizationParameter(PxVisualizationParameter::ePARTICLE_SYSTEM_MAX_MOTION_DISTANCE) > 0.0f)
	{
		if (arePositionsReadeable)
		{
			PxReal radius = getCore().getMaxMotionDistance();

			PxQuat q0(0.0f,	0.0f,			0.0f,	1.0f			);
			PxQuat q1(0.0f,	PxSqrt(0.5f),	0.0f,	PxSqrt(0.5f)	);
			PxQuat q2(0.5f,	0.5f,			0.5f,	0.5f			);

			Cm::BitMap::Iterator it(*particlesCore.bitMap);
			for (PxU32 p = it.getNext(); p != Cm::BitMap::Iterator::DONE; p = it.getNext())
			{
				const PxVec3& position = particlesCore.positions[p];

				if (areVelocitiesReadeable && particlesCore.velocities[p].magnitude()*timestep >= radius*0.99f)
					out << PxU32(PxDebugColor::eARGB_RED);
				else
					out << PxU32(PxDebugColor::eARGB_GREEN);

				out << PxTransform(position, q0) << Cm::DebugCircle(12, radius);
				out << PxTransform(position, q1) << Cm::DebugCircle(12, radius);
				out << PxTransform(position, q2) << Cm::DebugCircle(12, radius);
			}
		}
		else
		{
			PX_WARN_ONCE(true,
				"PxVisualizationParameter::ePARTICLE_SYSTEM_MAX_MOTION_DISTANCE cannot be rendered without specifying PxParticleReadDataFlag::ePOSITION_BUFFER");
		}
	}

	if (getScene().getVisualizationParameter(PxVisualizationParameter::ePARTICLE_SYSTEM_POSITION) > 0.0f)
	{
		if (arePositionsReadeable)
		{
			PxReal rad = getScene().getVisualizationParameter(PxVisualizationParameter::ePARTICLE_SYSTEM_POSITION) * getScene().getVisualizationScale()*0.5f;
			PxVec3 a(0,0,rad);
			PxVec3 b(0,rad,0);
			PxVec3 c(rad,0,0);

			out << PxU32(PxDebugColor::eARGB_BLUE) << Cm::RenderOutput::LINES << PxMat44(PxIdentity);

			Cm::BitMap::Iterator it(*particlesCore.bitMap);
			for (PxU32 p = it.getNext(); p != Cm::BitMap::Iterator::DONE; p = it.getNext())
			{
				const PxVec3& position = particlesCore.positions[p];
				out << position + a << position - a;
				out << position + b << position - b;
				out << position + c << position - c;
			}
		}
		else
		{
			PX_WARN_ONCE(true,
				"PxVisualizationParameter::ePARTICLE_SYSTEM_POSITION cannot be rendered without specifying \
				PxParticleReadDataFlag::ePOSITION_BUFFER");
		}
	}

	if (getScene().getVisualizationParameter(PxVisualizationParameter::ePARTICLE_SYSTEM_VELOCITY) > 0.0f)
	{
		if (arePositionsReadeable && areVelocitiesReadeable)
		{
			PxReal scale = getScene().getVisualizationParameter(PxVisualizationParameter::ePARTICLE_SYSTEM_VELOCITY) * getScene().getVisualizationScale();

			out << PxU32(PxDebugColor::eARGB_RED) << PxMat44(PxIdentity);

			Cm::BitMap::Iterator it(*particlesCore.bitMap);
			for (PxU32 p = it.getNext(); p != Cm::BitMap::Iterator::DONE; p = it.getNext())
			{
				out << Cm::DebugArrow(particlesCore.positions[p], particlesCore.velocities[p] * timestep, scale);
			}
		}
		else
		{
			PX_WARN_ONCE(true,
				"PxVisualizationParameter::ePARTICLE_SYSTEM_VELOCITY cannot be rendered without specifying \
				PxParticleReadDataFlag::ePOSITION_BUFFER and PxParticleReadDataFlag::eVELOCITY_BUFFER");
		}
	}
}

//----------------------------------------------------------------------------//

void Sc::ParticleSystemSim::visualizeCollisionNormals(Cm::RenderOutput& out)
{
	PxvParticleSystemStateDataDesc particlesCore;
	getParticleState().getParticlesV(particlesCore, false, false);

	if (particlesCore.numParticles == 0)
		return;

	PX_ASSERT(particlesCore.bitMap);

	bool arePositionsReadeable = getCore().getParticleReadDataFlags() & PxParticleReadDataFlag::ePOSITION_BUFFER;
	bool areCollisionNormalsReadeable = getCore().getParticleReadDataFlags() & PxParticleReadDataFlag::eCOLLISION_NORMAL_BUFFER;

	if (getScene().getVisualizationParameter(PxVisualizationParameter::ePARTICLE_SYSTEM_COLLISION_NORMAL) > 0.0f)
	{
		if (arePositionsReadeable && areCollisionNormalsReadeable)
		{
			PxvParticleSystemSimDataDesc particlesSim;
			getSimParticleData(particlesSim, false);

			PxReal scale = getScene().getVisualizationParameter(PxVisualizationParameter::ePARTICLE_SYSTEM_COLLISION_NORMAL) * getScene().getVisualizationScale();

			out << PxU32(PxDebugColor::eARGB_GREEN) << PxMat44(PxIdentity);

			if(particlesSim.collisionNormals.ptr())
			{			
				Cm::BitMap::Iterator it(*particlesCore.bitMap);
				for (PxU32 p = it.getNext(); p != Cm::BitMap::Iterator::DONE; p = it.getNext())
				{
					if (!particlesSim.collisionNormals[p].isZero())	
					{
						const PxVec3& position = particlesCore.positions[p];
						const PxVec3& normal = particlesSim.collisionNormals[p];
						out << Cm::DebugArrow(position, normal*scale, 0.1f*scale);
					}
				}
			}
		}
		else
		{
			PX_WARN_ONCE(true,
				"PxVisualizationParameter::ePARTICLE_SYSTEM_COLLISION_NORMAL cannot be rendered without specifying \
				PxParticleReadDataFlag::ePOSITION_BUFFER and PxParticleReadDataFlag::eCOLLISION_NORMAL_BUFFER");
		}
	}
}

//----------------------------------------------------------------------------//

void Sc::ParticleSystemSim::visualizeInteractions(Cm::RenderOutput& out)
{
	out << PxU32(PxDebugColor::eARGB_GREEN) << Cm::RenderOutput::LINES;
	for(PxU32 i=0; i < mParticlePacketShapes.size(); i++)
	{
		ParticlePacketShape* particleShape = mParticlePacketShapes[i];

		Cm::Range<ParticleElementRbElementInteraction*const> interactions = particleShape->getPacketShapeInteractions();
		for (; !interactions.empty(); interactions.popFront())
		{
			ParticleElementRbElementInteraction*const interaction = interactions.front();

			PX_ASSERT(interaction->getType() == PX_INTERACTION_TYPE_PARTICLE_BODY);

			PxBounds3 bounds = particleShape->getBounds();
			out << bounds.getCenter() << interaction->getRbShape().getAbsPose().p;
		}
	}
}

//----------------------------------------------------------------------------//

PxBaseTask& Sc::ParticleSystemSim::scheduleShapeGeneration(InteractionScene& scene, const Ps::Array<ParticleSystemSim*>& particleSystems, PxBaseTask& continuation)
{
	Ps::Array<PxvParticleSystemSim*, Ps::TempAllocator> llParticleSystems(particleSystems.size());
	Ps::Array<PxvParticleShapesUpdateInput, Ps::TempAllocator> llInputs(particleSystems.size());
	 
	for (PxU32 i = 0; i < particleSystems.size(); ++i)
	{
		PX_ASSERT(&particleSystems[i]->getInteractionScene() == &scene);
		particleSystems[i]->createShapeUpdateInput(llInputs[i]);
		llParticleSystems[i] = particleSystems[i]->mLLSim;
	}
	
	return scene.getLowLevelContext()->getParticleSystemBatcher().scheduleShapeGeneration(llParticleSystems.begin(), llInputs.begin(), particleSystems.size(), continuation);
}

//----------------------------------------------------------------------------//

PxBaseTask& Sc::ParticleSystemSim::scheduleDynamicsCpu(InteractionScene& scene, const Ps::Array<ParticleSystemSim*>& particleSystems, PxBaseTask& continuation)
{
	Ps::Array<PxvParticleSystemSim*, Ps::TempAllocator> llParticleSystems(particleSystems.size());

	for (PxU32 i = 0; i < particleSystems.size(); ++i)
	{
		PX_ASSERT(&particleSystems[i]->getInteractionScene() == &scene);
		llParticleSystems[i] = particleSystems[i]->mLLSim;
	}

	return scene.getLowLevelContext()->getParticleSystemBatcher().scheduleDynamicsCpu(llParticleSystems.begin(), particleSystems.size(), continuation);
}

//----------------------------------------------------------------------------//

PxBaseTask& Sc::ParticleSystemSim::scheduleCollisionPrep(InteractionScene& scene, const Ps::Array<ParticleSystemSim*>& particleSystems, PxBaseTask& continuation)
{
	Ps::Array<PxvParticleSystemSim*, Ps::TempAllocator> llParticleSystems(particleSystems.size());
	Ps::Array<PxLightCpuTask*, Ps::TempAllocator> inputPrepTasks(particleSystems.size());
	
	for (PxU32 i = 0; i < particleSystems.size(); ++i)
	{
		PX_ASSERT(&particleSystems[i]->getInteractionScene() == &scene);
		inputPrepTasks[i] = &particleSystems[i]->mCollisionInputPrepTask;
		llParticleSystems[i] = particleSystems[i]->mLLSim;
	}

	return scene.getLowLevelContext()->getParticleSystemBatcher().scheduleCollisionPrep(llParticleSystems.begin(), inputPrepTasks.begin(), inputPrepTasks.size(), continuation);
}

//----------------------------------------------------------------------------//

PxBaseTask& Sc::ParticleSystemSim::scheduleCollisionCpu(InteractionScene& scene, const Ps::Array<ParticleSystemSim*>& particleSystems, PxBaseTask& continuation)
{
	Ps::Array<PxvParticleSystemSim*, Ps::TempAllocator> llParticleSystems(particleSystems.size());

	for (PxU32 i = 0; i < particleSystems.size(); ++i)
	{
		PX_ASSERT(&particleSystems[i]->getInteractionScene() == &scene);
		llParticleSystems[i] = particleSystems[i]->mLLSim;
	}

	return scene.getLowLevelContext()->getParticleSystemBatcher().scheduleCollisionCpu(llParticleSystems.begin(), particleSystems.size(), continuation);
}

//----------------------------------------------------------------------------//

PxBaseTask& Sc::ParticleSystemSim::schedulePipelineGpu(InteractionScene& scene, const Ps::Array<ParticleSystemSim*>& particleSystems, PxBaseTask& continuation)
{
	Ps::Array<PxvParticleSystemSim*, Ps::TempAllocator> llParticleSystems(particleSystems.size());
	for (PxU32 i = 0; i < particleSystems.size(); ++i)
	{
		PX_ASSERT(&particleSystems[i]->getInteractionScene() == &scene);
		llParticleSystems[i] = particleSystems[i]->mLLSim;
	}

	return scene.getLowLevelContext()->getParticleSystemBatcher().schedulePipelineGpu(llParticleSystems.begin(), particleSystems.size(), continuation);
}

//----------------------------------------------------------------------------//

void Sc::ParticleSystemSim::createShapeUpdateInput(PxvParticleShapesUpdateInput& input)
{
	PxvParticleShape** llShapes = NULL;
	if (mParticlePacketShapes.size() > 0)
	{
		//note: this buffer needs to be deallocated in LL 
		llShapes = (PxvParticleShape**)PX_ALLOC_TEMP(mParticlePacketShapes.size()*sizeof(PxvParticleShape*), PX_DEBUG_EXP("PxvParticleShape*"));
		for (PxU32 i = 0; i < mParticlePacketShapes.size(); ++i)
			llShapes[i] = mParticlePacketShapes[i]->getLowLevelParticleShape();
	}

	input.shapes = llShapes;
	input.shapeCount = mParticlePacketShapes.size();
}

//----------------------------------------------------------------------------//

void Sc::ParticleSystemSim::prepareCollisionInput(PxBaseTask* /*continuation*/)
{
	PxU32 numParticleShapes = mParticlePacketShapes.size();
	PxU32 numInteractionsTest = 0;

	//note: this buffer needs to be deallocated in LL 
	PxU32 cmStreamSize = PxsParticleContactManagerStreamWriter::getStreamSize(numParticleShapes, mInteractionCount);
	PxU8* cmStream = (PxU8*)(PX_ALLOC_TEMP(cmStreamSize, PX_DEBUG_EXP("ParticleContactManagerStream")));

	PxsParticleContactManagerStreamWriter swriter(cmStream, numParticleShapes, mInteractionCount);

	for (PxU32 s = 0; s < mParticlePacketShapes.size(); ++s)
	{
		const Sc::ParticlePacketShape& particleShape = *mParticlePacketShapes[s];
		swriter.addParticleShape(particleShape.getLowLevelParticleShape());

		//count number of interactions... could be cached
		for(Cm::Range<ParticleElementRbElementInteraction*const> interactions = particleShape.getPacketShapeInteractions(); !interactions.empty(); interactions.popFront())
		{
			PX_ASSERT(interactions.front()->getType()==PX_INTERACTION_TYPE_PARTICLE_BODY);
			const Sc::ParticleElementRbElementInteraction& cm = *interactions.front();
			
			if (!cm.isDisabled())
			{
				PX_ASSERT(cm.getElementSim1().getElementType() == PX_ELEMENT_TYPE_SHAPE);
				const Sc::ShapeSim& shapeSim = cm.getRbShape();
				bool isDynamic = shapeSim.actorIsDynamic();
				const RigidSim& rigidSim = shapeSim.getRbSim();				
				PxsRigidCore& rigidCore =  static_cast<BodyCore&>(rigidSim.getActorCore()).getCore();	
				const PxTransform* w2sOld = isDynamic ? getInteractionScene().getLowLevelContext()->getBodyTransformVault().getTransform(static_cast<const PxsBodyCore&>(rigidCore)) : NULL;
				swriter.addContactManager(&rigidCore, &shapeSim.getCore().getCore(), w2sOld, (shapeSim.getFlags() & PxShapeFlag::ePARTICLE_DRAIN) != 0, isDynamic);
				numInteractionsTest++;
			}
		}
	}
	PX_ASSERT(numInteractionsTest == mInteractionCount);

	//passes ownership to low level object
	PxvParticleCollisionUpdateInput input;
	input.contactManagerStream = cmStream;
	mLLSim->passCollisionInputV(input);
}

//----------------------------------------------------------------------------//

#endif  // PX_ENABLE_DEBUG_VISUALIZATION


#endif	// PX_USE_PARTICLE_SYSTEM_API
