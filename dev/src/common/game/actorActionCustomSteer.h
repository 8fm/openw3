/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "actorAction.h"


class ActorActionCustomSteer : public ActorAction, public CMoveLocomotion::IController
{
private:
	Bool											m_isMoving;
	IMovementTargeter*								m_targeter;
	EMoveType										m_moveType;
	Float											m_absSpeed;

public:
	ActorActionCustomSteer( CActor* actor, EActorActionType type = ActorAction_CustomSteer );
	~ActorActionCustomSteer();

	//! Start movement
	Bool StartMove( IMovementTargeter* targeter, EMoveType moveType, Float absSpeed, EMoveFailureAction failureAction = MFA_REPLAN );

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

	void SetMoveType( EMoveType val ) { m_moveType = val; }

protected:
	//! Sets the specified movement type and speed of the controlled agent
	void SetMoveSpeed( CMovingAgentComponent& mac, EMoveType moveType, Float absSpeed ) const;

private:
	// Cleans up after the movement is done
	void Cleanup();
};

///////////////////////////////////////////////////////////////////////////////
