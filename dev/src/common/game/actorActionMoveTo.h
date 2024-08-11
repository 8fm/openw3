/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/
#pragma once

// PAKSAS TODO: dependencies inversion
#include "../game/actorAction.h"
#include "moveLocomotion.h"
#include "moveNavigationPath.h"

class CMovePIBasic;
class CMovePathIterator;
class CPathAgent;
class CMoveLSSteering;

///////////////////////////////////////////////////////////////////////////////

// An action that causes movement that involves path finding
class ActorActionMoveTo : public ActorAction, public CMoveLocomotion::IController, public IMovementTargeter
{
protected:
	CMoveLSSteering*							m_activeSegment;

	Vector										m_targetPos;				//! Current movement destination
	Float										m_toleranceRadius;			//! A radius in distance of which from the destination even a failed action is considered to be successful
	Float										m_pathfindingTolerance;
	Bool										m_isMoving;
	Bool										m_targetWasUpdated;
	Uint16										m_moveFlags;
	EngineTime									m_safetyDelay;
	EngineTime									m_pathPlotingDelay;

public:
	enum EFlags
	{
		FLAG_PRECISE_ARRIVAL					= FLAG( 0 ),
		FLAGS_DEFAULT							= 0,
	};

	ActorActionMoveTo( CActor* actor, EActorActionType type = ActorAction_Moving  );
	~ActorActionMoveTo();

	void ChangeTarget( const Vector& target, EMoveType moveType, Float absSpeed, Float radius );

	//! Start movement
	Bool StartMove( const CNode * target, Bool withHeading, EMoveType moveType, Float absSpeed, Float radius, Float tolerance = 0.f, EMoveFailureAction failureAction = MFA_REPLAN, Uint16 moveToFlags = FLAGS_DEFAULT );

	//! Start movement with end heading
	Bool StartMove( const Vector& target, Float heading, EMoveType moveType, Float absSpeed, Float radius, Float tolerance = 0.f, EMoveFailureAction failureAction = MFA_REPLAN, Uint16 moveToFlags = FLAGS_DEFAULT );

	//! Start movement away from given position
	Bool StartMoveAway( const CNode * position, Float distance, EMoveType moveType, Float absSpeed, Float radius, EMoveFailureAction failureAction = MFA_REPLAN );

	//! Start movement away from given line segment
	Bool StartMoveAwayFromLine( const Vector& positionA, const Vector& positionB, Float distance, Bool makeMinimalMovement, 
								EMoveType moveType, Float absSpeed, Float radius, EMoveFailureAction failureAction = MFA_REPLAN );

	//! Stop movement
	virtual void Stop();

	//! Is canceling of this action by other actions allowed
	virtual Bool IsCancelingAllowed() const;

	//! Update
	virtual Bool Update( Float timeDelta );

	//! Get move target
	const Vector& GetTarget() const;

	//! Tells if a look-at can be used with the current action
	virtual Bool CanUseLookAt() const { return true; }

	// -------------------------------------------------------------------------
	// CMoveLocomotion::IController implementation
	// -------------------------------------------------------------------------
	void OnSegmentFinished( EMoveStatus status ) override;
	void OnControllerAttached() override;
	void OnControllerDetached() override;

	// -------------------------------------------------------------------------
	// IMovementTargeter interface
	// -------------------------------------------------------------------------
	void UpdateChannels( const CMovingAgentComponent& agent, SMoveLocomotionGoal& goal, Float timeDelta ) override;

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

protected:
	//! Sets the specified movement type and speed of the controlled agent
	void SetMoveSpeed( CMovingAgentComponent& mac, EMoveType moveType, Float absSpeed ) const;

	//! Stop during update (finish action, etc)
	virtual void OnStopDuringMovementUpdate( EActorActionResult result );

private:
	// Retrieves a moving agent component, initializing dependent subsystems if needed
	CMovingAgentComponent* GetMAC() const;

	CPathAgent* GetPathAgent() const;

	// Cleans up after the movement is done
	void Cleanup();
};
