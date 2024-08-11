/*
 * Copyright © 2012 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "behTreeNodeCustomSteering.h"

class CBehTreeNodeKeepDistanceInstance;

class CBehTreeNodeKeepDistanceDefinition : public CBehTreeNodeCustomSteeringDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeKeepDistanceDefinition, CBehTreeNodeCustomSteeringDefinition, CBehTreeNodeKeepDistanceInstance, KeepDistance );
protected:
	CBehTreeValFloat		m_distance;
	CBehTreeValBool			m_notFacingTarget;
public:
	CBehTreeNodeKeepDistanceDefinition()
		: m_distance( 10.f ), m_notFacingTarget( false )	{};

	virtual IBehTreeNodeInstance* SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL ) const;
};

BEGIN_CLASS_RTTI( CBehTreeNodeKeepDistanceDefinition );
	PARENT_CLASS( CBehTreeNodeCustomSteeringDefinition );
	PROPERTY_EDIT( m_distance, TXT("Distance to maintain"));
	PROPERTY_EDIT( m_notFacingTarget,  TXT("If npc should not face target "));
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehTreeNodeKeepDistanceInstance : public CBehTreeNodeCustomSteeringInstance
{
	typedef CBehTreeNodeCustomSteeringInstance Super;
protected:
	Float					m_distance;
	Bool					m_notFacingTarget;
	Bool					m_exit;
public:

	typedef CBehTreeNodeKeepDistanceDefinition Definition;

	CBehTreeNodeKeepDistanceInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );

	////////////////////////////////////////////////////////////////////
	//! Behavior execution cycle
	Bool IsAvailable();
	virtual void Update();
	virtual Bool Activate();
	virtual void Deactivate();
	virtual void UpdateChannels( const CMovingAgentComponent& agent, SMoveLocomotionGoal& goal, Float timeDelta );
	virtual Bool IsFinished() const;

	Bool IsInRange() const;
};