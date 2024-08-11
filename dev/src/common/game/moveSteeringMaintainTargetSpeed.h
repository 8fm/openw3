#pragma once

#include "../engine/springDampers.h"

#include "../core/instanceVar.h"
#include "../core/instanceDataLayoutCompiler.h"

#include "speedManagmentSteeringTasks.h"
#include "movementCommandBuffer.h"

// Fuck. Previous version of MaintainTargetSpeed was completely destroyed. I'm bringing it back at different name
class CMoveSTSaneMaintainTargetSpeed : public IManageSpeedSteeringTask
{
	DECLARE_ENGINE_CLASS( CMoveSTSaneMaintainTargetSpeed, IManageSpeedSteeringTask, 0 )

public:
	CMoveSTSaneMaintainTargetSpeed()										{}

	void				CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const override;

	String				GetTaskName() const override;
};

BEGIN_CLASS_RTTI( CMoveSTSaneMaintainTargetSpeed );
	PARENT_CLASS( IManageSpeedSteeringTask );
END_CLASS_RTTI();

// This class is applicable only for predefined paths following.
class CMoveSTMaintainTargetSpeed : public IManageSpeedSteeringTask
{
	DECLARE_ENGINE_CLASS( CMoveSTMaintainTargetSpeed, IManageSpeedSteeringTask, 0 )

protected:
	Float									m_allowedDiffPerSecond;
	Float									m_stopSpeedThreshold;
	Float									m_distanceCoefficient;

	TInstanceVar< Float >					i_previousVelocity;
public:
	CMoveSTMaintainTargetSpeed()
		: m_allowedDiffPerSecond( 10.0f )
		, m_stopSpeedThreshold( 0.001f )
		, m_distanceCoefficient( 0.2f )	{ m_speedImportance = 1.f; }

	void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler ) override;
	void OnInitData( CMovingAgentComponent& agent, InstanceBuffer& data ) override;

	void				CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const override;

	String				GetTaskName() const override;
protected:
	virtual Bool ShouldMaintainTargetSpeed( IMovementCommandBuffer& comm )const;
	//Bool CalcMatchTargetSpeed( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta, Float &newSpeed, Float &newImportance )const;
};

BEGIN_CLASS_RTTI( CMoveSTMaintainTargetSpeed );
	PARENT_CLASS( IManageSpeedSteeringTask );
	PROPERTY_EDIT( m_allowedDiffPerSecond, TXT("100 immediate response, 0.1f slight delay before matching speed") )
	PROPERTY_EDIT( m_stopSpeedThreshold, TXT("Below that threshold and if the target speed is decreasing the actor speed will be set to 0") )
	PROPERTY_EDIT( m_distanceCoefficient, TXT("How much distance affects acceleration and deacceleration") );
END_CLASS_RTTI();
