#pragma once

#include "actorAction.h"
#include "moveLocomotion.h"


// A curve based movement action
class ActorActionMoveOnCurveTo : public ActorAction, public CMoveLocomotion::IController
{
private:
	EActorActionResult					m_status;

public:
	ActorActionMoveOnCurveTo( CActor* actor, EActorActionType type = ActorAction_Sliding );
	~ActorActionMoveOnCurveTo();

	//! Start curve movement, no heading, duration is controlled
	Bool StartMoveOnCurveTo( const Vector& target, Float duration, Bool rightShift );

	//! Stop movement
	virtual void Stop();

	//! Update
	virtual Bool Update( Float timeDelta );

	//! Tells if a look-at can be used with the current action
	virtual Bool CanUseLookAt() const { return true; }

	//! Called when an actor is being pushed by another actor
	virtual void OnBeingPushed( const Vector& direction, Float rotation, Float speed, EPushingDirection animDirection ) {}

	// -------------------------------------------------------------------------
	// CMoveLocomotion::IController
	// -------------------------------------------------------------------------
	virtual void OnSegmentFinished( EMoveStatus status );
	virtual void OnControllerAttached();
	virtual void OnControllerDetached();

protected:
	//! Get curve movement duration
	Float GetMoveOnCurveToDuration( Float dist ) const;

	//! Reset moving agent data
	void ResetAgentMovementData();

	//! Sets the specified movement type and speed of the controlled agent
	void SetMoveSpeed( CMovingAgentComponent& mac, EMoveType moveType, Float absSpeed ) const;
};
