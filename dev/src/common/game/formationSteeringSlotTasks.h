/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "formationSteeringTasks.h"


///////////////////////////////////////////////////////////////////////////////
// Steering towards member "slot"
class CFormationSteerToSlotSteeringTask : public IFormationFragmentarySteeringTask
{
	DECLARE_ENGINE_CLASS( CFormationSteerToSlotSteeringTask, IFormationFragmentarySteeringTask, 0 );
protected:
	Float							m_speedImportance;

	void							CalculateSteeringInput( IMovementCommandBuffer& comm, InstanceBuffer& data, SFormationSteeringInput& formationData, Vector2& outHeading, Float& outHeadingRatio, Float& outOverrideSpeed, Float& outOverrideSpeedImportance ) const override;
public:
	CFormationSteerToSlotSteeringTask()
		: IFormationFragmentarySteeringTask( 0.5f )
		, m_speedImportance( 1.f )												{}

	String							GetTaskName() const override;
};

BEGIN_CLASS_RTTI( CFormationSteerToSlotSteeringTask )
	PARENT_CLASS( IFormationFragmentarySteeringTask )
	PROPERTY_EDIT( m_speedImportance, TXT("Speed input importance") )
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////
// Catchup slot
class CFormationCatchupSlotSteeringTask : public IFormationSteeringTask
{
	DECLARE_ENGINE_CLASS( CFormationCatchupSlotSteeringTask, IFormationSteeringTask, 0 );
protected:
	Float							m_speedImportance;

	Float							m_toleranceDistance;
	Float							m_maxDistance;

	Float							m_cachupSpeed;

public:
	CFormationCatchupSlotSteeringTask()
		: m_speedImportance( 1.f )
		, m_toleranceDistance( 0.2f )
		, m_maxDistance( 2.f )
		, m_cachupSpeed( 1.f )												{}

	String							GetTaskName() const override;

	void							CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const override;
};

BEGIN_CLASS_RTTI( CFormationCatchupSlotSteeringTask )
	PARENT_CLASS( IFormationSteeringTask )
	PROPERTY_EDIT( m_speedImportance, TXT("Speed input importance") )
	PROPERTY_EDIT( m_toleranceDistance, TXT("Distance below which algorithm has no importance") )
	PROPERTY_EDIT( m_maxDistance, TXT("Distance at which algorithm has max importance") )
	PROPERTY_EDIT( m_cachupSpeed, TXT("Speed input") )
END_CLASS_RTTI()