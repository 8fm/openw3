/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "moveSteeringCondition.h"

class IFormationSteeringCondition : public IMoveSteeringCondition
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IFormationSteeringCondition, IMoveSteeringCondition )
public:
	String GetConditionName() const override;
};

BEGIN_ABSTRACT_CLASS_RTTI( IFormationSteeringCondition )
	PARENT_CLASS( IMoveSteeringCondition )
END_CLASS_RTTI()

class CFormationIsMovingSteeringCondition : public IFormationSteeringCondition
{
	DECLARE_ENGINE_CLASS( CFormationIsMovingSteeringCondition, IFormationSteeringCondition, 0 )
public:
	String GetConditionName() const override;

	Bool Evaluate( const IMovementCommandBuffer& comm, InstanceBuffer& data ) const;
};


BEGIN_CLASS_RTTI( CFormationIsMovingSteeringCondition )
	PARENT_CLASS( IFormationSteeringCondition )
END_CLASS_RTTI()

class CFormationIsBrokenSteeringCondition : public IFormationSteeringCondition
{
	DECLARE_ENGINE_CLASS( CFormationIsBrokenSteeringCondition, IFormationSteeringCondition, 0 )
protected:
	Float						m_howMuchBroken;
public:
	CFormationIsBrokenSteeringCondition()
		: m_howMuchBroken( 0.5f )																			{}
	String GetConditionName() const override;

	Bool Evaluate( const IMovementCommandBuffer& comm, InstanceBuffer& data ) const;
};


BEGIN_CLASS_RTTI( CFormationIsBrokenSteeringCondition )
	PARENT_CLASS( IFormationSteeringCondition )
	PROPERTY_EDIT( m_howMuchBroken, TXT( "Ratio [0..1] of how much broken formation have to be." ) )
END_CLASS_RTTI()
