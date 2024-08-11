/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "aiAction.h"


class CAIActionMove abstract
	: public CAIAction
	, public CMoveLocomotion::IController
	, public IMovementTargeter
{
	DECLARE_AI_ACTION_CLASS( CAIActionMove, CAIAction )

protected:
	// const data
	EMoveType							m_moveType;
	Float								m_absSpeed;
	Float								m_toleranceRadius;
	Bool								m_takeToleranceFromInteraction;

	// runtime data
	THandle< CMovingAgentComponent >	m_component;
	EngineTime							m_safetyDelay;
	EngineTime							m_pathPlotingDelay;
	Vector								m_targetPos;
	
	// runtime flags
	Bool								m_isMoving		: 1;
	Bool								m_targetUpdate	: 1;

public:
	CAIActionMove();	
	
protected:
	// CAIAction interface
	virtual Bool CanBeStartedOn( CComponent* component ) const		override;
	virtual EAIActionStatus StartOn( CComponent* component )		override;
	virtual EAIActionStatus Tick( Float timeDelta )					override;
	virtual EAIActionStatus Stop( EAIActionStatus newStatus )		override;
	virtual EAIActionStatus Reset()									override;
	virtual Bool ShouldBeTicked() const								override { return true; }
	virtual void OnGenerateDebugFragments( CRenderFrame* frame )	override;

public:
	// CMoveLocomotion::IController implementation
	virtual void OnSegmentFinished( EMoveStatus status )			override;
	virtual void OnControllerAttached()								override;
	virtual void OnControllerDetached()								override;

public:
	// IMovementTargeter interface
	virtual void UpdateChannels( const CMovingAgentComponent& agent, SMoveLocomotionGoal& goal, Float timeDelta ) override;

protected:
	// methods to override in derived classes
	virtual Float GetRealToleranceRadius() const;
	Float GetCurrentToleranceRadius() const;
};

BEGIN_ABSTRACT_CLASS_RTTI( CAIActionMove )
	PARENT_CLASS( CAIAction )
	PROPERTY_EDIT( m_moveType , TXT("Walking, running or absolute speed.") )
	PROPERTY_EDIT_RANGE( m_absSpeed , TXT("Speed valid only if moveType == MT_AbsSpeed."), 0.f, FLT_MAX )
	PROPERTY_EDIT_RANGE( m_toleranceRadius , TXT("How close to target we need to end up to be satisfied. Enter some non-zero positive value here."), FLT_EPSILON, FLT_MAX )
	PROPERTY_EDIT( m_takeToleranceFromInteraction, TXT( "If this is true, tolerance radius will be overriden by target interaction tolerance." ) )
END_CLASS_RTTI()

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
class CAIActionMoveN abstract : public CAIActionMove
{
	DECLARE_AI_ACTION_CLASS( CAIActionMoveN, CAIActionMove )

protected:
	// runtime data
	THandle< CNode >		m_targetNode;

protected:
	// CAIAction interface
	virtual Bool CanBeStartedOn( CComponent* component ) const	override;
	virtual EAIActionStatus StartOn( CComponent* component )	override;
	virtual EAIActionStatus Tick( Float timeDelta )				override;
	virtual EAIActionStatus Reset()								override;

protected:
	virtual Vector RecomputeTargetPos() const					= 0;
};

BEGIN_ABSTRACT_CLASS_RTTI( CAIActionMoveN )
	PARENT_CLASS( CAIActionMove )
END_CLASS_RTTI()

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
class CAIActionMoveToNode : public CAIActionMoveN
{
	DECLARE_AI_ACTION_CLASS( CAIActionMoveToNode, CAIActionMoveN )

protected:
	virtual Vector RecomputeTargetPos()	const					override;
};

BEGIN_CLASS_RTTI( CAIActionMoveToNode )
	PARENT_CLASS( CAIActionMoveN )
END_CLASS_RTTI()

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
class CAIActionMoveAwayFromNode : public CAIActionMoveN
{
	DECLARE_AI_ACTION_CLASS( CAIActionMoveAwayFromNode, CAIActionMoveN )

protected:
	// const properties
	Float					m_alertDistance;
	Float					m_awayDistance;
	Float					m_safeZoneRadius;

public:
	CAIActionMoveAwayFromNode();

#ifndef NO_EDITOR
	virtual void OnPropertyPostChange( IProperty* prop ) override;
#endif

protected:
	virtual Bool CanBeStartedOn( CComponent* component ) const	override;
	virtual Vector RecomputeTargetPos() const					override;
	virtual Float GetRealToleranceRadius() const				override;
};

BEGIN_CLASS_RTTI( CAIActionMoveAwayFromNode )
	PARENT_CLASS( CAIActionMoveN )
	PROPERTY_EDIT_RANGE( m_alertDistance, TXT("How close we need to be to start moving away?"), FLT_EPSILON, FLT_MAX )
	PROPERTY_EDIT_RANGE( m_awayDistance, TXT("How far should we move away?"), FLT_EPSILON, FLT_MAX )
	PROPERTY_EDIT_RANGE( m_safeZoneRadius, TXT("Safe zone radius"), FLT_EPSILON, FLT_MAX )
END_CLASS_RTTI()

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
class CAIActionMoveToInteraction : public CAIActionMove
{
	DECLARE_AI_ACTION_CLASS( CAIActionMoveToInteraction, CAIActionMove )

protected:
	// runtime data
	THandle< CR6InteractionComponent >	m_interaction;
	Vector								m_interactionPos;

public:
	virtual Bool CanBeStartedOn( CComponent* component ) const	override;
	virtual EAIActionStatus StartOn( CComponent* component )	override;
	virtual EAIActionStatus Tick( Float timeDelta )				override;
	virtual EAIActionStatus Reset()								override;

protected:
	Vector RecomputeTargetPos( CR6InteractionComponent* interaction ) const;
};

BEGIN_CLASS_RTTI( CAIActionMoveToInteraction )
	PARENT_CLASS( CAIActionMove )
END_CLASS_RTTI()

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------