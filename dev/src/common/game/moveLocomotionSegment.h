/**
 * Copyright © 2010 CDProjekt Red, Inc. All Rights Reserved.
 */
#pragma once

#include "movementObject.h"


class CMovingAgentComponent;
class IMovementTargeter;
class CMoveSteeringBehavior;

enum ELSStatus
{
	LS_InProgress,
	LS_Completed,
	LS_Failed,
};

///////////////////////////////////////////////////////////////////////////////

//! A segment defined by two points on the path between which a certain
//! movement strategy needs to be used - implementations of this interface
//! will define that very strategy.
class CMoveLSSteering;
class CMoveLSDirect;
class IMoveLocomotionSegment : public IMovementObject
{
public:
	virtual ~IMoveLocomotionSegment() {}

	// Activates the segment. Return 'true' if the segment was successfully activated,
	// or 'false' if it couldn't be activated at the moment and needs to wait
	virtual Bool Activate( CMovingAgentComponent& agent ) = 0;

	// updates the segment
	virtual ELSStatus Tick( Float timeDelta, CMovingAgentComponent& agent ) = 0;

	// Deactivates the segment
	virtual void Deactivate( CMovingAgentComponent& agent ) = 0;

	// Checks if the segment can be interrupted
	virtual Bool CanBeCanceled() const = 0;

	// Serialization support
	virtual void OnSerialize( IFile& file ) {}

	// Debug draw
	virtual void GenerateDebugFragments( CMovingAgentComponent& agent, CRenderFrame* frame ) = 0;
	virtual void GenerateDebugPage( TDynArray< String >& debugLines ) const = 0;

	virtual CMoveLSSteering *const AsCMoveLSSteering(){ return nullptr; }
	virtual CMoveLSDirect* AsCMoveLSDirect(){ return nullptr; }

	virtual void OnSteeringBehaviorChanged( CMovingAgentComponent* owner, CMoveSteeringBehavior* prevSteeringGraph, InstanceBuffer* prevRuntimeData, CMoveSteeringBehavior* newSteeringGraph, InstanceBuffer* newRuntimeData ){}
};

///////////////////////////////////////////////////////////////////////////////
