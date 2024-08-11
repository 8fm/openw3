/**
* Copyright © 2011 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "actorAction.h"


///////////////////////////////////////////////////////////////////////////////

class IMoveDynamicFollowerController;
class CPathAgent;

///////////////////////////////////////////////////////////////////////////////

// An action that causes movement that involves path finding
class ActorActionMoveToDynamicNode : public ActorAction, public CMoveLocomotion::IController, public IMovementTargeter
{
private:
	CMoveLSSteering*								m_activeSegment;
	CMovingAgentComponent*							m_targetMAC;

	THandle< CNode >								m_target;
	Float											m_desiredDistance;
	Float											m_pathfindingTolerance;
	Bool											m_keepDistance;
	Bool											m_isMoving;
	EngineTime										m_lastTargetUpdate;
public:
	ActorActionMoveToDynamicNode( CActor* actor, EActorActionType type = ActorAction_DynamicMoving  );
	~ActorActionMoveToDynamicNode();

	//! Start movement
	Bool StartMove( const CNode* target, EMoveType moveType, Float absSpeed, Float radius, Float tolerance, Bool keepDistance = false, EMoveFailureAction failureAction = MFA_REPLAN );

	//! Stop movement
	virtual void Stop();

	//! Is canceling of this action by other actions allowed
	virtual Bool IsCancelingAllowed() const;

	//! Update
	virtual Bool Update( Float timeDelta );

	//! Tells if a look-at can be used with the current action
	virtual Bool CanUseLookAt() const { return true; }

	// -------------------------------------------------------------------------
	// Debugging
	// -------------------------------------------------------------------------
	// Debug draw
	void GenerateDebugFragments( CRenderFrame* frame );
	using IMovementTargeter::GenerateDebugFragments;

	// Additional debug
	void DebugDraw( IDebugFrame& debugFrame ) const;

	// Debug description
	String GetDescription() const;

	// -------------------------------------------------------------------------
	// IMoveDFSteeringFactory implementation
	// -------------------------------------------------------------------------
	void OnSegmentFinished( EMoveStatus status ) override;
	void OnControllerAttached() override;
	void OnControllerDetached() override;

	// -------------------------------------------------------------------------
	// IMovementTargeter interface
	// -------------------------------------------------------------------------
	void UpdateChannels( const CMovingAgentComponent& agent, SMoveLocomotionGoal& goal, Float timeDelta ) override;
	// -------------------------------------------------------------------------

protected:
	//! Sets the specified movement type and speed of the controlled agent
	void SetMoveSpeed( CMovingAgentComponent& mac, EMoveType moveType, Float absSpeed ) const;

private:
	CPathAgent* GetPathAgent() const;

	// Cleans up after the movement is done
	void Cleanup();
};

///////////////////////////////////////////////////////////////////////////////
