/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "formationPattern.h"
#include "formationMemberData.h"

class CSlotFormationMemberData;
class CFormationSlotInstance;

class SFormationSlotConstraint
{
	friend struct SFormationConstraintDefinition;
protected:
	Int32								m_referenceSlot;
	EFormationConstraintType			m_type;
	Vector2								m_value;
	Float								m_strength;
	Float								m_tolerance;

public:
	SFormationSlotConstraint()													{}

	// preprocessing time
	Bool								ComputeDesiredPos( CAISlotFormationLeaderData& formation, Vector2& outPos ) const;

	// per-frame steering computations
	CFormationSlotInstance*				GetReferencedSlot( CAISlotFormationLeaderData& formation ) const;
	void								ApplyToSteering( CMovingAgentComponent& mac, CSlotFormationMemberData& member, CAISlotFormationLeaderData& formation, CFormationSlotInstance* referenceSlot, Vector2& headingOutput, Float& speedOutput, Float& headingWeight, Float& speedWeight ) const;
};

class CFormationSlotInstance
{
	friend class CFormationSlotDefinition;
protected:
	TDynArray< SFormationSlotConstraint >		m_constraints;
	Vector2										m_desiredFormationPos;
	Uint32										m_slotGeneration;
	CFormationMemberDataPtr						m_memberPtr;
public:
	static const Uint32 INVALID_INDEX = 0x7fff;

	Bool								ComputeDesiredPos( CAISlotFormationLeaderData& formation );
	const Vector2&						GetDesiredFormationPos() const			{ return m_desiredFormationPos; }
	Bool								IsOccupied() const						{ return m_memberPtr; }
	void								Free()									{ m_memberPtr.Clear(); }
	CSlotFormationMemberData*			GetOwner() const;
	void								SetOwner( CSlotFormationMemberData* p );

	const TDynArray< SFormationSlotConstraint >& GetConstraints() const			{ return m_constraints; }
};
