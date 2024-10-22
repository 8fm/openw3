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


#ifndef PX_PHYSICS_ARTICULATION_SIM
#define PX_PHYSICS_ARTICULATION_SIM


#include "PsUserAllocated.h"
#include "CmPhysXCommon.h"
#include "PxvArticulation.h"
#include "ScArticulationCore.h" 

namespace physx
{

class PxsArticulation;
class PxsTransformCache;

namespace Cm
{
	class SpatialVector;
}

namespace Sc
{

	class BodySim;
	class ArticulationJointSim;
	class ArticulationCore;
	class Scene;

	class ArticulationSim : public Ps::UserAllocated 
	{
	public:
											ArticulationSim(ArticulationCore& core, 
												Scene& scene,
												BodyCore& root);

											~ArticulationSim();

		PX_INLINE	PxsArticulation*		getLowLevelArticulation() const { return mLLArticulation; }
		PX_INLINE	ArticulationCore&		getCore() const { return mCore; }

								void		addBody(BodySim& body, 
													BodySim* parent, 
													ArticulationJointSim* joint);
								void		removeBody(BodySim &sim);
								
								PxsArticulationLinkHandle	getLinkHandle(BodySim& body) const;
								
								void		checkResize() const;						// resize LL memory if necessary
								
								void		debugCheckWakeCounterOfLinks(PxReal wakeCounter) const;
								void		debugCheckSleepStateOfLinks(bool isSleeping) const;

								bool		isSleeping() const;
								void		internalWakeUp(PxReal wakeCounter);	// called when sim sets sleep timer
								void		sleepCheck(PxReal dt);
								void		updateCachedTransforms(PxsTransformCache& cache);


	// temporary, to make sure link handles are set in island detection. This should be done on insertion,
	// but when links are inserted into the scene they don't know about the articulation so for the
	// moment we fix it up at sim start

								void		fixupHandles();

								void		setActive(const bool b);
								void		updateForces(PxReal dt, PxReal oneOverDt, bool updateGravity, const PxVec3& gravity, bool hasStaticTouch, bool simUsesAdaptiveForce);

	// drive cache implementation
	//
						ArticulationDriveCache*		
											createDriveCache(PxReal compliance,
															 PxU32 driveIterations) const;

						void				updateDriveCache(ArticulationDriveCache& cache,
															 PxReal compliance,
															 PxU32 driveIterations) const;

						void				releaseDriveCache(ArticulationDriveCache& cache) const;
						
						void				applyImpulse(BodyCore& link,
														 const ArticulationDriveCache& driveCache,
														 const PxVec3& force,
														 const PxVec3& torque);

						void				computeImpulseResponse(BodyCore& link,
																   PxVec3& linearResponse,
																   PxVec3& angularResponse,
																   const ArticulationDriveCache& driveCache,
																   const PxVec3& force,
																   const PxVec3& torque) const;


	private:
					ArticulationSim&		operator=(const ArticulationSim&);
								PxU32		findBodyIndex(BodySim &body) const;
								void		setIslandHandle(BodySim &body,
															PxU32 index);


					PxsArticulation*					mLLArticulation;
					Scene&								mScene;
					ArticulationCore&					mCore;
					Ps::Array<PxsArticulationLink>		mLinks;
					Ps::Array<BodySim*>					mBodies;
					Ps::Array<ArticulationJointSim*>	mJoints;
					

					// DS: looks slightly fishy, but reallocating/relocating the LL memory for the articulation
					// really is supposed to leave it behaviorally equivalent, and so we can reasonably make
					// all this stuff mutable and checkResize const

					mutable PxcArticulationSolverDesc	mSolverData;

					// persistent state of the articulation for warm-starting joint load computation
					mutable Ps::Array<Ps::aos::Mat33V>	mInternalLoads;
					mutable Ps::Array<Ps::aos::Mat33V>	mExternalLoads;

					// persistent data used during the solve that can be released at frame end

					mutable Ps::Array<PxTransform>		mPose;
					mutable Ps::Array<PxcSIMDSpatial>	mMotionVelocity;		// saved here in solver
					mutable Ps::Array<char>				mFsDataBytes;			// drive cache creation (which is const) can force a resize
					mutable Ps::Array<char>				mScratchMemory;			// drive cache creation (which is const) can force a resize

					mutable bool						mUpdateSolverData;
	};

} // namespace Sc

}

#endif
