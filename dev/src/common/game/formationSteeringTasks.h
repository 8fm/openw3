/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "moveSteeringTask.h"
#include "formationSteeringInput.h"

class CFormationMemberData;
class CFormationLeaderData;



///////////////////////////////////////////////////////////////////////////////
// Base abstract class
class IFormationSteeringTask : public IMoveSteeringTask
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IFormationSteeringTask, IMoveSteeringTask );

public:
	String							GetTaskName() const;
};

BEGIN_ABSTRACT_CLASS_RTTI( IFormationSteeringTask )
	PARENT_CLASS( IMoveSteeringTask )
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////
class IFormationFragmentarySteeringTask : public IFormationSteeringTask
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IFormationFragmentarySteeringTask, IFormationSteeringTask );
protected:
	Float							m_importance;

	virtual void					CalculateSteeringInput( IMovementCommandBuffer& comm, InstanceBuffer& data, SFormationSteeringInput& formationData, Vector2& outHeading, Float& outHeadingRatio, Float& outOverrideSpeed, Float& outOverrideSpeedImportance ) const = 0;
public:
	IFormationFragmentarySteeringTask( Float defaultImportance = 0.25f )
		: m_importance( defaultImportance )										{}

	String							GetTaskName() const override;
	void							CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const override;
};

BEGIN_ABSTRACT_CLASS_RTTI( IFormationFragmentarySteeringTask )
	PARENT_CLASS( IFormationSteeringTask )
	PROPERTY_EDIT( m_importance, TXT("Heading importance") )
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////
// Steering towards "leader"
class CFormationSteerToLeaderSteeringTask : public IFormationFragmentarySteeringTask
{
	DECLARE_ENGINE_CLASS( CFormationSteerToLeaderSteeringTask, IFormationFragmentarySteeringTask, 0 );
protected:
	void							CalculateSteeringInput( IMovementCommandBuffer& comm, InstanceBuffer& data, SFormationSteeringInput& formationData, Vector2& outHeading, Float& outHeadingRatio, Float& outOverrideSpeed, Float& outOverrideSpeedImportance ) const override;
public:
	CFormationSteerToLeaderSteeringTask()
		: IFormationFragmentarySteeringTask( 0.25f )							{}

	String							GetTaskName() const override;
};

BEGIN_CLASS_RTTI( CFormationSteerToLeaderSteeringTask )
	PARENT_CLASS( IFormationFragmentarySteeringTask )
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////
// Steering towards "center of mass"
class CFormationSteerToCenterOfMassSteeringTask : public IFormationFragmentarySteeringTask
{
	DECLARE_ENGINE_CLASS( CFormationSteerToCenterOfMassSteeringTask, IFormationFragmentarySteeringTask, 0 );
protected:
	void							CalculateSteeringInput( IMovementCommandBuffer& comm, InstanceBuffer& data, SFormationSteeringInput& formationData, Vector2& outHeading, Float& outHeadingRatio, Float& outOverrideSpeed, Float& outOverrideSpeedImportance ) const override;
public:
	CFormationSteerToCenterOfMassSteeringTask()
		: IFormationFragmentarySteeringTask( 0.3f )													{}

	String							GetTaskName() const override;
};

BEGIN_CLASS_RTTI( CFormationSteerToCenterOfMassSteeringTask )
	PARENT_CLASS( IFormationFragmentarySteeringTask )
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////
// Don't go back - limit movement only forward
class CFormationDontBackDownSteeringTask : public IFormationSteeringTask
{
	DECLARE_ENGINE_CLASS( CFormationDontBackDownSteeringTask, IFormationSteeringTask, 0 );
protected:
	Float						m_maxAngleDifference;
	TInstanceVar< Float >		i_cosAngle;
	TInstanceVar< Float >		i_sinAngle;
public:
	CFormationDontBackDownSteeringTask()
		: m_maxAngleDifference( 90.f )											{}
	void						CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const override;
	String						GetTaskName() const override;

	void						OnBuildDataLayout( InstanceDataLayoutCompiler& compiler ) override;
	void						OnInitData( CMovingAgentComponent& agent, InstanceBuffer& data ) override;
};

BEGIN_CLASS_RTTI( CFormationDontBackDownSteeringTask )
	PARENT_CLASS( IFormationSteeringTask )
	PROPERTY_EDIT( m_maxAngleDifference, TXT("Max angle difference") );
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////
// Look in direction formation is facing
class CFormationFaceSteeringTask : public IFormationSteeringTask
{
	DECLARE_ENGINE_CLASS( CFormationFaceSteeringTask, IFormationSteeringTask, 0 );
public:
	void						CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const override;
	String						GetTaskName() const override;
};

BEGIN_CLASS_RTTI( CFormationFaceSteeringTask )
	PARENT_CLASS( IFormationSteeringTask )
END_CLASS_RTTI()


///////////////////////////////////////////////////////////////////////////////
// Steer to leaders path
class CFormationSteerToPathSteeringTask : public IFormationFragmentarySteeringTask
{
	DECLARE_ENGINE_CLASS( CFormationSteerToPathSteeringTask, IFormationFragmentarySteeringTask, 0 );
protected:
	void							CalculateSteeringInput( IMovementCommandBuffer& comm, InstanceBuffer& data, SFormationSteeringInput& formationData, Vector2& outHeading, Float& outHeadingRatio, Float& outOverrideSpeed, Float& outOverrideSpeedImportance ) const override;
public:
	CFormationSteerToPathSteeringTask()
		: IFormationFragmentarySteeringTask( 0.25f )							{}

	String							GetTaskName() const override;
};

BEGIN_CLASS_RTTI( CFormationSteerToPathSteeringTask )
	PARENT_CLASS( IFormationFragmentarySteeringTask )
END_CLASS_RTTI()


///////////////////////////////////////////////////////////////////////////////
// Keep distance to other comrades
class CFormationKeepDistanceToMembersSteeringTask : public IFormationFragmentarySteeringTask
{
	DECLARE_ENGINE_CLASS( CFormationKeepDistanceToMembersSteeringTask, IFormationFragmentarySteeringTask, 0 );
protected:
	Float							m_minDistance;
	Float							m_desiredDistance;

	void							CalculateSteeringInput( IMovementCommandBuffer& comm, InstanceBuffer& data, SFormationSteeringInput& formationData, Vector2& outHeading, Float& outHeadingRatio, Float& outOverrideSpeed, Float& outOverrideSpeedImportance ) const override;
public:
	CFormationKeepDistanceToMembersSteeringTask()
		: IFormationFragmentarySteeringTask( 0.25f )
		, m_minDistance( 0.5f )
		, m_desiredDistance( 1.25f )											{}

	String							GetTaskName() const override;
};

BEGIN_CLASS_RTTI( CFormationKeepDistanceToMembersSteeringTask )
	PARENT_CLASS( IFormationFragmentarySteeringTask )
	PROPERTY_EDIT( m_minDistance, TXT("Distance under with repultion has maximal importance") )
	PROPERTY_EDIT( m_desiredDistance, TXT("Distance at with there is no repultion/gravity") )
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////
// Keep formation speed
class CFormationKeepSpeedSteeringTask : public IFormationSteeringTask
{
	DECLARE_ENGINE_CLASS( CFormationKeepSpeedSteeringTask, IFormationSteeringTask, 0 );
protected:
	Float							m_speedImportance;
public:
	CFormationKeepSpeedSteeringTask()
		: m_speedImportance( 0.5f )												{}

	void							CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const override;
	String							GetTaskName() const override;
};

BEGIN_CLASS_RTTI( CFormationKeepSpeedSteeringTask )
	PARENT_CLASS( IFormationSteeringTask )
	PROPERTY_EDIT( m_speedImportance, TXT("Speed input importance") )
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////
// Keep away leader
class CFormationKeepAwaylLeaderSteeringTask : public IFormationFragmentarySteeringTask
{
	DECLARE_ENGINE_CLASS( CFormationKeepAwaylLeaderSteeringTask, IFormationFragmentarySteeringTask, 0 );
protected:
	Float							m_minLeaderDistance;
	Float							m_noticeLeaderDistance;

	void							CalculateSteeringInput( IMovementCommandBuffer& comm, InstanceBuffer& data, SFormationSteeringInput& formationData, Vector2& outHeading, Float& outHeadingRatio, Float& outOverrideSpeed, Float& outOverrideSpeedImportance ) const override;
public:
	CFormationKeepAwaylLeaderSteeringTask()
		: IFormationFragmentarySteeringTask( 0.75f )
		, m_minLeaderDistance( 1.0f )
		, m_noticeLeaderDistance( 3.f )									{}

	String							GetTaskName() const override;
};

BEGIN_CLASS_RTTI( CFormationKeepAwaylLeaderSteeringTask )
	PARENT_CLASS( IFormationFragmentarySteeringTask )
	PROPERTY_EDIT( m_minLeaderDistance, TXT("Distance under with repultion has maximal importance") )
	PROPERTY_EDIT( m_noticeLeaderDistance, TXT("Distance at with there is no repultion/gravity") )
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////
// Keep comrades speed
class CFormationKeepComradesSpeedSteeringTask : public IFormationSteeringTask
{
	DECLARE_ENGINE_CLASS( CFormationKeepComradesSpeedSteeringTask, IFormationSteeringTask, 0 );
protected:
	Float							m_speedImportance;
	Float							m_distanceToComrades;

public:
	CFormationKeepComradesSpeedSteeringTask()
		: m_speedImportance( 0.5f )
		, m_distanceToComrades( 2.f )											{}

	void							CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const override;
	String							GetTaskName() const override;
};

BEGIN_CLASS_RTTI( CFormationKeepComradesSpeedSteeringTask )
	PARENT_CLASS( IFormationSteeringTask )
	PROPERTY_EDIT( m_speedImportance, TXT("Speed input importance") )
	PROPERTY_EDIT( m_distanceToComrades, TXT("Distance in which task collect comrades") )
END_CLASS_RTTI()


///////////////////////////////////////////////////////////////////////////////
// Dont fall behind
class CFormationDontFallBehindSteeringTask : public IFormationSteeringTask
{
	DECLARE_ENGINE_CLASS( CFormationDontFallBehindSteeringTask, IFormationSteeringTask, 0 );
protected:
	Float							m_speedImportance;
	Float							m_minFallBehindDistance;
	Float							m_maxFallBehindDistance;

public:
	CFormationDontFallBehindSteeringTask()
		: m_speedImportance( 1.f )
		, m_minFallBehindDistance( 2.0f )
		, m_maxFallBehindDistance( 5.f )									{}

	void							CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const override;
	String							GetTaskName() const override;
};

BEGIN_CLASS_RTTI( CFormationDontFallBehindSteeringTask )
	PARENT_CLASS( IFormationSteeringTask )
	PROPERTY_EDIT( m_speedImportance, TXT("Speed input importance") )
	PROPERTY_EDIT( m_minFallBehindDistance, TXT("Distance from which node starts to impact speed") )
	PROPERTY_EDIT( m_maxFallBehindDistance, TXT("Distance at which node impacts movement with full importance") )
END_CLASS_RTTI()

