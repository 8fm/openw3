/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "actorAction.h"

///////////////////////////////////////////////////////////////////////////////

// An action that causes movement that involves path finding
class ActorActionMoveOutsideNavdata : public ActorAction, public CMoveLocomotion::IController, public IMovementTargeter
{
public:
	ActorActionMoveOutsideNavdata( CActor* actor, EActorActionType type = ActorAction_MovingOutNavdata );
	~ActorActionMoveOutsideNavdata();

	//! Start movement
	Bool StartMove( const CNode* target, EMoveType moveType, Float absSpeed, Float radius );

	//! Stop movement
	virtual void Stop();

	//! Is canceling of this action by other actions allowed
	virtual Bool IsCancelingAllowed() const;

	//! Update
	virtual Bool Update( Float timeDelta );

	//! Tells if a look-at can be used with the current action
	virtual Bool CanUseLookAt() const { return true; }

	//! Debug description
	String GetDescription() const;

	// -------------------------------------------------------------------------
	// IMoveDFSteeringFactory implementation
	// -------------------------------------------------------------------------
	virtual void OnSegmentFinished( EMoveStatus status ) override;
	virtual void OnControllerAttached() override;
	virtual void OnControllerDetached() override;

	// -------------------------------------------------------------------------
	// IMovementTargeter interface
	// -------------------------------------------------------------------------
	virtual void UpdateChannels( const CMovingAgentComponent& agent, SMoveLocomotionGoal& goal, Float timeDelta ) override;

private:
	//! Sets the specified movement type and speed of the controlled agent
	void SetMoveSpeed( CMovingAgentComponent& mac, EMoveType moveType, Float absSpeed ) const;
	
	//! Cleans up after the movement is done
	void Cleanup();

private:
	CMoveLSSteering*								m_activeSegment;
	CMovingAgentComponent*							m_targetMAC;

	THandle< CNode >								m_target;
	Float											m_desiredDistance;
	Bool											m_isMoving;
};