/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "speedManagmentSteeringTasks.h"

#include "../core/instanceDataLayoutCompiler.h"

#include "movementCommandBuffer.h"

IMPLEMENT_ENGINE_CLASS( IManageSpeedSteeringTask )
IMPLEMENT_ENGINE_CLASS( CMoveSTMaintainSpeed )
IMPLEMENT_ENGINE_CLASS( CMoveSTMaintainRandomSpeed )


///////////////////////////////////////////////////////////////////////////////
// IManageSpeedSteeringTask
String IManageSpeedSteeringTask::GetTaskName() const
{
	return TXT("Manage speed");
}

Bool IManageSpeedSteeringTask::OnPropertyMissing( CName propertyName, const CVariant& readValue )
{
	if ( propertyName.AsString() == TXT("importance") )
	{
		return readValue.AsType( m_speedImportance );
	}

	return TBaseClass::OnPropertyMissing( propertyName, readValue );
}

///////////////////////////////////////////////////////////////////////////////
// CMoveSTMaintainSpeed
///////////////////////////////////////////////////////////////////////////////
void CMoveSTMaintainSpeed::CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const
{
	comm.AddSpeed( m_speed, m_speedImportance );
}

String CMoveSTMaintainSpeed::GetTaskName() const
{
	return TXT( "MaintainSpeed" );
}

///////////////////////////////////////////////////////////////////////////////
// CMoveSTMaintainRandomSpeed
///////////////////////////////////////////////////////////////////////////////
void CMoveSTMaintainRandomSpeed::CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const
{
	comm.AddSpeed( data[ i_speed ], m_speedImportance );
}

void CMoveSTMaintainRandomSpeed::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	compiler << i_speed;

	TBaseClass::OnBuildDataLayout( compiler );
}
void CMoveSTMaintainRandomSpeed::OnInitData( CMovingAgentComponent& agent, InstanceBuffer& data )
{
	Float ratio = GEngine->GetRandomNumberGenerator().Get< Float >() * 2.f - 1.f;
	Bool neg = ratio < 0.f;
	ratio = ratio * ratio;
	if ( neg )
	{
		 ratio = -ratio;
	}
	ratio = (ratio + 1.f) * 0.5f;

	Float speed = m_minSpeed + ( m_maxSpeed - m_minSpeed ) * ratio;
	data[ i_speed ] = speed;
}

String CMoveSTMaintainRandomSpeed::GetTaskName() const
{
	return TXT( "MaintainRandomSpeed" );
}