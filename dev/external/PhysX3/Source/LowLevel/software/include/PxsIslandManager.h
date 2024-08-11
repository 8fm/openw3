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


#ifndef PXS_ISLAND_MANAGER_H
#define PXS_ISLAND_MANAGER_H

#include "PxPreprocessor.h"

#include "PxsIslandManagerAux.h"
#include "CmPhysXCommon.h"
#include "PxTask.h"

namespace physx
{

class PxsRigidBody;
class PxsArticulation;
class PxsContactManager;
struct PxsConstraint;

namespace Cm
{
	class EventProfiler;
}

//----------------------------------------------------------------------------//

#define DEFAULT_NUM_NODES 256
#define DEFAULT_NUM_EDGES 256
#define DEFAULT_NUM_ARTICS 32

#ifdef PX_PS3
class IslandGenSpuTask;
#endif

class PxsIslandManager
{
public:

#ifdef PX_PS3
	friend class IslandGenSpuTask;
#endif

	PxsIslandManager(const PxU32 rigidBodyOffset, Cm::EventProfiler* eventProfiler=NULL);
	~PxsIslandManager();

	void preAllocate(PxU32 nbBodies);

	//Add/modify bodies - inlined to accelerate addActors
	PX_FORCE_INLINE void addBody(void* owner, PxsIslandManagerNodeHook& hook, bool kinematic)
	{
		const NodeType freeNode=mNodeManager.getAvailableElem();
		Node& node=mNodeManager.get(freeNode);
		node.init();
		node.setRigidBodyOwner(owner);
		node.setKinematic(kinematic);
		node.setIsNew();
		mNodeChangeManager.addCreatedNode(freeNode);
		hook.index=freeNode;

		mNumAddedRBodies += !kinematic ? 1 : 0;
		mNumAddedKinematics += !kinematic ? 0 : 1;

		if(kinematic)
			mNodeManager.setKinematicNode(freeNode);

		mHasAnythingChanged = true;
	}

	//Call this is the kinematic state changes.
	void setKinematic(const PxsIslandManagerNodeHook&, bool kinematic);

	//If the body has wakeCounter == 0 and would go to sleep if it wasn't touching anything awake then call notifyReadyForSleeping.
	PX_FORCE_INLINE	void notifyReadyForSleeping(const PxsIslandManagerNodeHook& hook)		
	{	
		mNodeManager.get(hook.index).setReadyForSleeping(); 
		mHasAnythingChanged = true;
	}
	//If the body has wakeCounter > 0 and shouldn't be in the sleeping state then call notifyNotReadyForSleeping.
	PX_FORCE_INLINE	void notifyNotReadyForSleeping(const PxsIslandManagerNodeHook& hook)	
	{	
		mNodeManager.get(hook.index).setNotReadyForSleeping(); 
		mHasAnythingChanged = true;
	}

	// for externally triggered state changes only
	//A pre-condition of sleeping is that a body is ready for sleeping.  
	//Both sleeping and ready-for-sleeping flags are set here.
	PX_FORCE_INLINE	void setAsleep(const PxsIslandManagerNodeHook& hook)					
	{	
		mNodeManager.get(hook.index).setAsleepAndReadyForSleeping();	
		mHasAnythingChanged = true;
	}
	// for externally triggered state changes only
	//A pre-condition of sleeping is that a body is ready for sleeping.  
	//Both sleeping and ready-for-sleeping flags are cleared here.
	PX_FORCE_INLINE	void setAwake(const PxsIslandManagerNodeHook& hook)						
	{	
		mNodeManager.get(hook.index).setAwakeAndNotReadyForSleeping(); 
		mHasAnythingChanged = true;
	}

	// Get sleep state of body
	PX_FORCE_INLINE	bool getIsReadyForSleeping(const PxsIslandManagerNodeHook& hook) const	
	{ 	
		return mNodeManager.get(hook.index).getIsReadyForSleeping(); 
	}
	PX_FORCE_INLINE	bool getIsInSleepingIsland(const PxsIslandManagerNodeHook& hook) const	
	{	
		return mNodeManager.get(hook.index).getIsInSleepingIsland(); 
	}

	//Add articulations.
	void addArticulationLink(PxsIslandManagerNodeHook& hook);
	void setArticulationLinkHandle(PxsArticulationLinkHandle link, void* owner, const PxsIslandManagerNodeHook& hook);
	void setArticulationRootLinkHandle(PxsArticulationLinkHandle link, void* owner, const PxsIslandManagerNodeHook& hook);

	//Remove a node (body or articulation)
	void removeNode(PxsIslandManagerNodeHook& hook);

	//Add/remove/modify edges.
	enum eEdgeType
	{
		EDGE_TYPE_CONTACT_MANAGER=0,
		EDGE_TYPE_CONSTRAINT,
		EDGE_TYPE_ARTIC,
		MAX_NUM_EDGE_TYPES
	};
	void addEdge(const eEdgeType edgeType, const PxsIslandManagerNodeHook body0hook, const PxsIslandManagerNodeHook body1hook, PxsIslandManagerEdgeHook& edgeHook);
	void setEdgeRigidCM(const PxsIslandManagerEdgeHook& edgeHook, PxsContactManager* cm);
	void clearEdgeRigidCM(const PxsIslandManagerEdgeHook& edgeHook);
	void setEdgeConstraint(const PxsIslandManagerEdgeHook& hook, PxsConstraint* constraint);
	void setEdgeArticulationJoint(const PxsIslandManagerEdgeHook& hook);
	void setEdgeConnected(const PxsIslandManagerEdgeHook& edgeHook);
	void setEdgeUnconnected(const PxsIslandManagerEdgeHook& edgeHook);
	void removeEdge(const eEdgeType edgeType, PxsIslandManagerEdgeHook& hook);

	//Update all the islands.
	void updateIslands(PxBaseTask* continuation, const PxU32 numSpus);

	//Data that is available after updateIslands.
	//Get the bodies that have been woken up.
	PX_FORCE_INLINE const PxU8*const* getBodiesToWake() const {return mProcessSleepingIslandsComputeData.mBodiesToWakeOrSleep;}
	PX_FORCE_INLINE PxU32 getNumBodiesToWake() const {return mProcessSleepingIslandsComputeData.mBodiesToWakeSize;}
	//Get the bodies that have been put to sleep.
	PX_FORCE_INLINE const PxU8*const* getBodiesToSleep() const {return &mProcessSleepingIslandsComputeData.mBodiesToWakeOrSleep[mProcessSleepingIslandsComputeData.mBodiesToWakeOrSleepCapacity-mProcessSleepingIslandsComputeData.mBodiesToSleepSize];}
	PX_FORCE_INLINE PxU32 getNumBodiesToSleep() const {return mProcessSleepingIslandsComputeData.mBodiesToSleepSize;}
	//Some pointer swizzling reveals a rigid body or an articulation
	static PX_FORCE_INLINE void getBodyToWakeOrSleep(const PxU8* bodyPtr, void*& rBodyOwner, void*& articOwner)
	{
		PX_ASSERT((0==((size_t)bodyPtr & 0x0f)) || (0==(((size_t)bodyPtr - 1) & 0x0f)));

		rBodyOwner=NULL;
		articOwner=NULL;
		if(0==((size_t)bodyPtr & 0x0f))
		{
			rBodyOwner=(void*)bodyPtr;
		}
		else
		{
			articOwner = (void*)((size_t)bodyPtr & ~1);
		}
	}

	//After waking bodies the contact managers that are created and given to edges needs to be passed
	//to the structures that will be eventually passed to the solver.
	void setWokenPairContactManagers();

	//After waking bodies the freshly made contact managers need a pass through narrowphase.
	PX_FORCE_INLINE NarrowPhaseContactManager* getNarrowPhaseContactManagers() {return mProcessSleepingIslandsComputeData.mNarrowPhaseContactManagers;}
	PX_FORCE_INLINE PxU32 getNumNarrowPhaseContactManagers() {return mProcessSleepingIslandsComputeData.mNarrowPhaseContactManagersSize;}

	//After all narrowphase and contact modification the empty contact managers need to be removed
	//from the solver lists.
	void finishSolverIslands();

	//Used by solver after calling finishSolverIslands
	PX_FORCE_INLINE PxU32 getActiveKinematicCount()	const {return mProcessSleepingIslandsComputeData.mSolverKinematicsSize;}
	PX_FORCE_INLINE PxsRigidBody*const* getActiveKinematics() const {return mProcessSleepingIslandsComputeData.mSolverKinematics;}
	PX_FORCE_INLINE PxU32 getIslandCount() const {return mProcessSleepingIslandsComputeData.mIslandIndicesSize;}
	PX_FORCE_INLINE const PxsIslandIndices* getIslandIndices() const  {return mProcessSleepingIslandsComputeData.mIslandIndices;}
	PX_FORCE_INLINE const PxsIslandObjects& getIslandObjects() const {return mIslandObjects;}

	//When the island manager results are no longer required it is legal to delete temporary buffers
	//that stored the island results and other temporary memory that was used to help compute the island results.
	void freeBuffers();

	//Test if ppu fallback occurred on ps3.
#ifdef PX_PS3
	bool getPPUFallback() const {return mPPUFallback;}
#endif

private:

	PxU32 mRigidBodyOffset;
	Cm::EventProfiler* mEventProfiler;

	//All nodes
	NodeManager mNodeManager;

	//All edges
	EdgeManager mEdgeManager;

	//Created/deleted nodes recorded for each update
	NodeChangeManager mNodeChangeManager;

	//Created/deleted/joined/broken edges recorded for each update
	EdgeChangeManager mEdgeChangeManager;

	//All islands and a bitmap of active islands.
	IslandManager mIslands;

	//All root articulations
	ArticulationRootManager mRootArticulationManager;

	//Track number of bodies/artics/kinematics/edges that have been added and removed.
	PxU32 mNumAddedRBodies;
	PxU32 mNumAddedArtics;
	PxU32 mNumAddedKinematics;
	PxU32 mNumAddedEdges[MAX_NUM_EDGE_TYPES];

	//Track number of edges with a kinematic node.
	PxU32 mNumEdgesWithKinematicNodes;

	//Track if everything is asleep.
	//If there are no islands for the solver then there is no solver work to do and we can safely say that everything is asleep.
	//If everything is asleep and nothing changes before the next update then we can do nothing in the next update.
	//This is set at the end of freeBuffers to true or false depending on the number of solver islands.
	bool mEverythingAsleep;

	//Track if anything has changed since the last update completed that requires a fresh island update in the next update.
	//This is set true if any api call (addNode/removeEdge/setKinematic etc) is called.
	//This is set false at the end of freeBuffers.
	bool mHasAnythingChanged;

	//Track if work is done in the current frame.
	//This is set in updateIslands depending on mEverythingAsleep and mHasAnythingChanged.  It is cleared in freeBuffers.
	bool mPerformIslandUpdate;

	//Data processed in processSleepingIslands.
	ProcessSleepingIslandsComputeData mProcessSleepingIslandsComputeData;

	//
	PxsIslandObjects mIslandObjects;

	//Buffer to hold temporary memory.
	PxU32 mBufferSize;
	PxU8* mBuffer;

	//Buffers used in update.
	IslandManagerUpdateWorkBuffers mIslandManagerUpdateWorkBuffers;

	PxU32 resizeForKinematics();
	void resizeArrays();
	void updateIslands();
#ifdef PX_PS3
	bool mPPUFallback;
	void updateIslandsSPU(PxBaseTask* continuation, const PxU32 numSpus);
#endif
#ifdef PX_DEBUG
	bool isValid();
#endif

	void removeEmptySolverContactManagers();

#ifdef PX_PS3
	IslandGenSpuTask* mIslandGenSpuTask;
#endif
};

} //namespace physx

#endif //PXS_ISLAND_MANAGER_H
