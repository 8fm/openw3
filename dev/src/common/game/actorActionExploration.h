/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "actorAction.h"
#include "moveLocomotion.h"
#include "expIntarface.h"

class CScriptedExplorationTraverser;
class CActionAreaComponent;

// A simple sliding movement action + movement on path towards exploration (if needed)
class ActorActionExploration : public ActorActionMoveTo
{
private:
	EActorActionResult				m_status;
	Bool							m_markerOldExp;

	THandle< CScriptedExplorationTraverser >	m_traverser;

	SExplorationQueryToken			m_token;
	Bool							m_isApproachingExploration;
	THandle< IScriptable >			m_explorationListener;

	// Optionnal target fed to the steering graph
	// Will be excluded of obstacle avoidance 
	THandle<CNode>							m_steeringGraphTargetNode;

public:
	ActorActionExploration( CActor* actor, EActorActionType type = ActorAction_Exploration );
	~ActorActionExploration();

	RED_INLINE CScriptedExplorationTraverser* GetTraverser(){ return m_traverser.Get(); }
	RED_INLINE EActorActionResult				GetStatus(){ return m_status; }

	//! Move using the specified action area
	Bool StartExploring( CActionAreaComponent* actionArea );
	Bool StartExploring( const SExplorationQueryToken & token, const THandle< IScriptable >& listener );
	Bool MoveAndStartExploring( const SExplorationQueryToken & token, const THandle< IScriptable >& listener, CNode *const steeringGraphTargetNode );

	//! Stop movement
	virtual void Stop();

	//! Update
	virtual Bool Update( Float timeDelta );

	//! Debug draw
	virtual void GenerateDebugFragments( CRenderFrame* frame );

	//! Tells if a look-at can be used with the current action
	virtual Bool CanUseLookAt() const { return true; }
	
	//! GC serialization
	virtual void OnGCSerialize( IFile& file );

	//! Called when an actor is being pushed by another actor
	virtual void OnBeingPushed( const Vector& direction, Float rotation, Float speed, EPushingDirection animDirection ) {}

	// -------------------------------------------------------------------------
	// CMoveLocomotion::IController
	// -------------------------------------------------------------------------
	virtual void OnSegmentFinished( EMoveStatus status );
	virtual void OnControllerAttached();
	virtual void OnControllerDetached();

	// -------------------------------------------------------------------------
	// ActorActionMoveTo
	// -------------------------------------------------------------------------
	virtual void OnStopDuringMovementUpdate( EActorActionResult result );
	void UpdateChannels( const CMovingAgentComponent& agent, SMoveLocomotionGoal& goal, Float timeDelta ) override;

	

private:
	Bool MoveIfRequired();
	Bool CheckIfRequiresToMove(	Vector * moveToLoc = nullptr, Float * minDist = nullptr );

	void DeleteTraverser();
};
