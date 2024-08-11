/**
* Copyright © 2010 CDProjekt Red, Inc. All Rights Reserved.
*/
#pragma once

#include "moveLocomotionSegment.h"
#include "movementGoal.h"
#include "movementCommandBuffer.h"

class CSteeringPipeline;
struct SMoveSteeringOutput;

///////////////////////////////////////////////////////////////////////////////
// CMoveLSSteering
class CMoveLSSteering : public IMoveLocomotionSegment, public IMovementCommandBuffer
{
private:

private:
	TDynArray< IMovementTargeter* >			m_targeters;

	Float									m_stopTimer;
	Float									m_timeDelta;

	Bool									m_switchToEntityRepresentation;
	Bool									m_includeStaticTargeters;
	Bool									m_stopTimerOn;
	Bool									m_forceNoTimeout;

public:
	CMoveLSSteering( Bool switchToEntityRepresentation = false );
	~CMoveLSSteering();

	// Adds a new movement targeter
	CMoveLSSteering& AddTargeter( IMovementTargeter* targeter );

	// Removes a new movement targeter
	CMoveLSSteering& RemoveTargeter( IMovementTargeter* targeter );

	// Makes the segment treat static targeters as a vital part of the goal
	RED_INLINE void IncludeStaticTargeters( Bool enable )						{ m_includeStaticTargeters = enable; }

	// -------------------------------------------------------------------------
	// IMoveLocomotionSegment implementation
	// -------------------------------------------------------------------------
	virtual Bool Activate( CMovingAgentComponent& agent );
	virtual ELSStatus Tick( Float timeDelta, CMovingAgentComponent& agent );
	virtual void Deactivate( CMovingAgentComponent& agent );
	virtual Bool CanBeCanceled() const;
	virtual void OnSerialize( IFile& file );
	virtual void GenerateDebugFragments( CMovingAgentComponent& agent, CRenderFrame* frame );
	virtual void GenerateDebugPage( TDynArray< String >& debugLines ) const;
	virtual CMoveLSSteering *const AsCMoveLSSteering()override;
	virtual void OnSteeringBehaviorChanged( CMovingAgentComponent* owner, CMoveSteeringBehavior* prevSteeringGraph, InstanceBuffer* prevRuntimeData, CMoveSteeringBehavior* newSteeringGraph, InstanceBuffer* newRuntimeData ) override;
	// -------------------------------------------------------------------------

	void SetForceNoTimeout()													{ m_forceNoTimeout = true; }
	void SetUseFootstepHandler( Bool use )										{}

private:
	ELSStatus CalcGoal( Float timeDelta );
	void EnableStopTimer( Bool enable );
};

///////////////////////////////////////////////////////////////////////////////
