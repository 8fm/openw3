/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "aiAction.h"


class CAIActionRotateMACToNode
	: public CAIAction
	, public CMoveLocomotion::IController
	, public IMovementTargeter
{
	DECLARE_AI_ACTION_CLASS( CAIActionRotateMACToNode, CAIAction )

protected:
	// const data
	Float								m_angularTolerance;
	Bool								m_takeToleranceFromInteraction;

	// runtime data
	THandle< CMovingAgentComponent >	m_component;
	THandle< CNode >					m_targetNode;
	Float								m_targetYaw;

	// runtime flags
	Bool								m_isRotating	: 1;
	

public:
	CAIActionRotateMACToNode();	
	
protected:
	// CAIAction interface
	virtual Bool CanBeStartedOn( CComponent* component ) const	override;
	virtual EAIActionStatus StartOn( CComponent* component )	override;
	virtual EAIActionStatus Tick( Float timeDelta )				override;
	virtual EAIActionStatus Stop( EAIActionStatus newStatus )	override;
	virtual EAIActionStatus Reset()								override;
	virtual Bool ShouldBeTicked() const							override { return true; }

public:
	// CMoveLocomotion::IController implementation
	virtual void OnSegmentFinished( EMoveStatus status )		override;
	virtual void OnControllerAttached()							override;
	virtual void OnControllerDetached()							override;

public:
	// IMovementTargeter interface
	virtual void UpdateChannels( const CMovingAgentComponent& agent, SMoveLocomotionGoal& goal, Float timeDelta ) override;

protected: 
	// to override in derived classes
	virtual Float RecomputeTargetYaw() const;
	Float GetCurrentAngularTolerance() const;
};

BEGIN_CLASS_RTTI( CAIActionRotateMACToNode )
	PARENT_CLASS( CAIAction )
	PROPERTY_EDIT_RANGE( m_angularTolerance, TXT("Tolerance value in degrees."), FLT_EPSILON, 180.f );
	PROPERTY_EDIT( m_takeToleranceFromInteraction, TXT( "If this is true, angular tolerance will be overriden by target interaction tolerance when possible." ) );
END_CLASS_RTTI()


class CAIActionRotateMACToInteraction
	: public CAIActionRotateMACToNode
{
	DECLARE_AI_ACTION_CLASS( CAIActionRotateMACToInteraction, CAIActionRotateMACToNode )
	
protected:
	// CAIAction interface
	virtual Bool CanBeStartedOn( CComponent* component ) const	override;
	virtual EAIActionStatus StartOn( CComponent* component )	override;

protected:
	// CAIActionRotateMACToNode interface
	virtual Float RecomputeTargetYaw() const					override;

private:
	RED_INLINE Bool CheckInteractRotation( Float rot1, Float rot2, Float tol ) const; 
};

BEGIN_CLASS_RTTI( CAIActionRotateMACToInteraction )
	PARENT_CLASS( CAIActionRotateMACToNode )
END_CLASS_RTTI()