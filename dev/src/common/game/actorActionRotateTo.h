/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "moveLocomotion.h"


class ActorActionRotateTo : public ActorAction, public CMoveLocomotion::IController, public IMovementTargeter
{
public:
	enum EResult
	{
		R_Started,
		R_NoRotationNeeded,
		R_Failed,
	};

private:
	EActorActionResult			m_status;
	Float						m_angleTolerance;

	Float						m_targetYaw;
public:
	ActorActionRotateTo( CActor* actor );
	~ActorActionRotateTo();

	//! Start rotating to face target
	EResult StartRotateTo( const Vector& target, Float angleTolerance );

	//! Update rotating ( in case it is moving )
	Bool UpdateRotateTo( const Vector& target );

	//! Start rotating to match the orientation
	EResult StartOrienting( Float orientation, Float angleTolerance );

	//! Update orientation ( in case it is changing )
	Bool UpdateOrienting( Float orientation );

	

	//! Stop movement
	virtual void Stop();

	//! Update
	virtual Bool Update( Float timeDelta );

	//! Tells if a look-at can be used with the current action
	virtual Bool CanUseLookAt() const { return true; }

	// -------------------------------------------------------------------------
	// CMoveLocomotion::IController
	// -------------------------------------------------------------------------
	void OnSegmentFinished( EMoveStatus status ) override;
	void OnControllerAttached() override;
	void OnControllerDetached() override;

	// -------------------------------------------------------------------------
	// IMovementTargeter interface
	// -------------------------------------------------------------------------
	void UpdateChannels( const CMovingAgentComponent& agent, SMoveLocomotionGoal& goal, Float timeDelta ) override;
};
