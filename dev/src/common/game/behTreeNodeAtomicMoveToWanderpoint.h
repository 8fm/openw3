/*
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "behTreeNodeAtomicMove.h"

class CBehTreeNodeAtomicMoveToWanderpointInstance;


//////////////////////////////////////////////////////////////////////////
// Action target move to
class CBehTreeNodeAtomicMoveToWanderpointDefinition : public CBehTreeNodeAtomicMoveToActionTargetDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeAtomicMoveToWanderpointDefinition, CBehTreeNodeAtomicMoveToActionTargetDefinition, CBehTreeNodeAtomicMoveToWanderpointInstance, AtomicMoveToWanderpoint );
protected:
	CBehTreeValBool			m_rightSideMovement;
public:
	CBehTreeNodeAtomicMoveToWanderpointDefinition()
		: m_rightSideMovement( true )									{}

	IBehTreeNodeInstance*	SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeNodeAtomicMoveToWanderpointDefinition )
	PARENT_CLASS( CBehTreeNodeAtomicMoveToActionTargetDefinition)
	PROPERTY_EDIT( m_rightSideMovement, TXT("Turn on right side movement for this actor") )
END_CLASS_RTTI()

class CBehTreeNodeAtomicMoveToWanderpointInstance : public CBehTreeNodeAtomicMoveToActionTargetInstance
{
private:
	typedef CBehTreeNodeAtomicMoveToActionTargetInstance Super;
protected:
	Bool					m_rightSideMovement;
	Bool					m_isMovingToProxyPoint;
	Bool					m_forcedPathfinding;
	Vector3					m_proxyPoint;
	Vector3					m_desiredSpot;
	THandle< CWanderPointComponent > m_lastWanderpoint;

	Bool					StartActorMoveTo()override;
	Bool					ComputeTargetAndHeading() override;
	Bool					OnDestinationReached() override;
public:
	typedef CBehTreeNodeAtomicMoveToWanderpointDefinition Definition;

	CBehTreeNodeAtomicMoveToWanderpointInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )
		, m_rightSideMovement( def.m_rightSideMovement.GetVal( context ) )									{}

	void				Deactivate() override;

	void				Update() override;
};
