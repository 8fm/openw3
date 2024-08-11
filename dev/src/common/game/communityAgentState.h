/**
 * Copyright © 2009 CD Projekt Red. All Rights Reserved.
 */

#pragma once


enum ECommunityAgentState
{
	CAS_Unknown,
	CAS_InitAfterCreated,
    CAS_WaitingToSpawnInSpawnPoint,
	CAS_AcquireNextAP,
	CAS_Spawned,

	// Description: Agent is moving to his action point.
	// Condition:   When an action point is assigned to an agent, he will change state from ReadyToWork to this state.
	// Behavior:    Agent moves to his action point and then rotate towards it. This state will be changed to WorkInProgress after reaching destination.
	CAS_MovingToActionPoint,

	// Description: Agent is doing his work in an action point.
	// Condition:   
	// Behavior:    Agent waits for a certain amount of time simulating work (i.e. doing nothing) and after that, switches state to RedyToWork.
	CAS_WorkInProgress,

	// Description: Agent stub is spawned and ready for choosing AP, but none AP is currently available.
	// Condition:   There are two possible reasons for that: there are no free AP at this time or there is no loaded layer with APs for this agent.
	// Behavior:    SS will try to find AP again after a certain amount of time.
	CAS_NoAPFound,

	// Description: Agent is ready to work (i.e. needs action point assignment)
	// Condition:   Agent has been spawned or has completed work in his current action point
	// Behavior:    SS will try to find AP
	CAS_ReadyToWork,

	// Description: Agent is marked to despawn.
	// Condition:   
	// Behavior:    
	CAS_Despawning,

	// Description: Agent stub is waiting for removal (NPC should be despawned already and NPC will not be despawned if it exits in this state).
	// Condition:   Agent has reached his despawn position and is ready to physical despawn.
	// Behavior:	If agent is just a stub, then despawn will be immediate (removal agent stub from a stubs container).
	//				If agent is spawned, than he will be physically despawned.
	CAS_ToDespawn
};
