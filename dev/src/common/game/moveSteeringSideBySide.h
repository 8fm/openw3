/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "moveSteeringCondition.h"
#include "targetSteeringTask.h"


class CMoveSTWalkSideBySide : public IMoveSteeringTask
{
	DECLARE_ENGINE_CLASS( CMoveSTWalkSideBySide, IMoveSteeringTask, 0 )

protected:
	Float				m_sideBySideDistance;
	Float				m_minApproachDistance;
	Float				m_maxApproachDistance;

	Float				m_catchupSpeedMultiplier;
	Float				m_slowDownSpeedMultiplier;

	Float				m_headingImportance;
	Float				m_speedImportance;

public:
	CMoveSTWalkSideBySide()
		: m_sideBySideDistance( 1.5f )
		, m_minApproachDistance( 0.2f )
		, m_maxApproachDistance( 1.f )
		, m_catchupSpeedMultiplier( 1.5f )
		, m_slowDownSpeedMultiplier( 0.75f )
		, m_headingImportance( 1.f )
		, m_speedImportance( 1.f )												{}


	void				CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const override;

	String				GetTaskName() const override;

	void				GenerateDebugFragments( const CMovingAgentComponent& agent, CRenderFrame* frame ) const override;

	static void			ComputeSideBySidePosition( const Vector& myPos, const Vector& targetPos, const Vector2& targetHeading, Float sideBySideDistance, Vector& outSideBySidePos );
};

BEGIN_CLASS_RTTI( CMoveSTWalkSideBySide )
	PARENT_CLASS( IMoveSteeringTask )
	PROPERTY_EDIT( m_sideBySideDistance, TXT("Distance at which we follow our partner side-by-side") )
	PROPERTY_EDIT( m_minApproachDistance, TXT("Distance at which we fully mimic our partner movement") )
	PROPERTY_EDIT( m_maxApproachDistance, TXT("Distance at which we start to mimic our partner movement") )

	PROPERTY_EDIT( m_catchupSpeedMultiplier, TXT("[1,2] If actor is maxApproachDistance away from target then catchupSpeedMultiplier is applied on target speed") )
	PROPERTY_EDIT( m_slowDownSpeedMultiplier, TXT("]0,2] If actor is maxApproachDistance in front of the target then slowDownSpeedMultiplier is applied on target speed") )

	PROPERTY_EDIT( m_headingImportance, TXT("Heading output importance") )
	PROPERTY_EDIT( m_speedImportance, TXT("Speed output importance") )
END_CLASS_RTTI();


class CMoveSCWalkSideBySide : public IMoveSteeringCondition
{
	DECLARE_ENGINE_CLASS( CMoveSCWalkSideBySide, IMoveSteeringCondition, 0 )

protected:
	Float				m_sideBySideDistance;
	Float				m_distanceLimit;
public:
	CMoveSCWalkSideBySide()
		: m_sideBySideDistance( 1.5f )
		, m_distanceLimit( 8.f )													{}

	Bool				Evaluate( const IMovementCommandBuffer& comm, InstanceBuffer& data ) const override;
	String				GetConditionName() const override;
};

BEGIN_CLASS_RTTI( CMoveSCWalkSideBySide );
	PARENT_CLASS( IMoveSteeringCondition );
	PROPERTY_EDIT( m_sideBySideDistance, TXT("Distance at which we follow our partner side-by-side") )
	PROPERTY_EDIT( m_distanceLimit, TXT("Distance limit at which we consider side-by-side following") )
END_CLASS_RTTI();

class CMoveSTFaceTargetFacing : public IMoveTargetSteeringTask
{
	DECLARE_ENGINE_CLASS( CMoveSTFaceTargetFacing, IMoveTargetSteeringTask, 0 )

public:
	void				CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const override;

	String				GetTaskName() const override;
};

BEGIN_CLASS_RTTI( CMoveSTFaceTargetFacing );
	PARENT_CLASS( IMoveTargetSteeringTask );
END_CLASS_RTTI();
