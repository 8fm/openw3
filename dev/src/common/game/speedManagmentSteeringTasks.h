/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "moveSteeringTask.h"

class IManageSpeedSteeringTask : public IMoveSteeringTask
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IManageSpeedSteeringTask, IMoveSteeringTask );
protected:
	Float				m_speedImportance;
public:
	IManageSpeedSteeringTask()
		: m_speedImportance( 0.5f )												{}

	String GetTaskName() const override;

	Bool OnPropertyMissing( CName propertyName, const CVariant& readValue ) override;

};
BEGIN_ABSTRACT_CLASS_RTTI( IManageSpeedSteeringTask )
	PARENT_CLASS( IMoveSteeringTask )
	PROPERTY_EDIT( m_speedImportance, TXT( "Speed importance" ) )
END_CLASS_RTTI()


///////////////////////////////////////////////////////////////////////////////
// Maintain given speed
class CMoveSTMaintainSpeed : public IManageSpeedSteeringTask
{
	DECLARE_ENGINE_CLASS( CMoveSTMaintainSpeed, IManageSpeedSteeringTask, 0 );

private:
	Float				m_speed;

public:
	CMoveSTMaintainSpeed()
		: m_speed( 1.f )														{}

	void CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const override;

	String GetTaskName() const override;

};
BEGIN_CLASS_RTTI( CMoveSTMaintainSpeed )
	PARENT_CLASS( IManageSpeedSteeringTask )
	PROPERTY_EDIT( m_speed, TXT( "Speed value" ) )
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////
// Maintain random speed
class CMoveSTMaintainRandomSpeed : public IManageSpeedSteeringTask
{
	DECLARE_ENGINE_CLASS( CMoveSTMaintainRandomSpeed, IManageSpeedSteeringTask, 0 );

private:
	Float					m_minSpeed;
	Float					m_maxSpeed;

	TInstanceVar< Float >	i_speed;

public:
	CMoveSTMaintainRandomSpeed()
		: m_minSpeed( 0.75f )
		, m_maxSpeed( 1.f )														{}

	void CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const override;

	void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler ) override;
	void OnInitData( CMovingAgentComponent& agent, InstanceBuffer& data ) override;

	String GetTaskName() const override;

};
BEGIN_CLASS_RTTI( CMoveSTMaintainRandomSpeed )
	PARENT_CLASS( IManageSpeedSteeringTask )
	PROPERTY_EDIT( m_minSpeed, TXT( "Minimal possible speed" ) )
	PROPERTY_EDIT( m_maxSpeed, TXT( "Maximum possible speed" ) )
END_CLASS_RTTI()