#include "build.h"
#include "movementGoal.h"


IMPLEMENT_ENGINE_CLASS( SMoveLocomotionGoal );


SMoveLocomotionGoal::SMoveLocomotionGoal() 
	: m_isFulfilled( true )
	, m_hasHeading( false )
	, m_hasOrientation( false )
	, m_orientationClamping( true )
	, m_hasSpeed( false )
	, m_hasGoalPosition( false )
	, m_hasGoalTolerance( false )
	, m_hasDistanceToGoal( true )
	, m_hasDestinationPosition( false )
	, m_hasDistanceToDestination( false )
	, m_matchMoveDirectionWithOrientation( false )
	, m_matchOrientationWithHeading( false )
	, m_snapToMinimalSpeed( false )
	, m_headingToGoal( 0.0f, 0.0f )
	, m_orientation( 0.0f )
	, m_desiredSpeed( 1.0f )
	, m_maxWaitTime( MAX_MOVEMENT_WAIT_TIME )
	, m_headingImportanceMultiplier( 1.f )
{}

Bool SMoveLocomotionGoal::IsSet() const
{
	// even though heading is a part of the goal specification, it's not required
	// by the goal to be fulfilled
	return !m_isFulfilled || !m_expectedBehNotification.Empty();
}

Bool SMoveLocomotionGoal::CanBeProcessed() const
{
	// even though heading is a part of the goal specification, it's not required
	// by the goal to be fulfilled
	return IsSet()|| m_orientationGoalAlwaysSet;
}

void SMoveLocomotionGoal::SetFulfilled( Bool val )
{
	m_isFulfilled &= val;
}

void SMoveLocomotionGoal::SetHeadingGoal( const CMovingAgentComponent& agent, const Vector2& heading, Bool normalize /* = true */)
{
	m_hasHeading = true;
	m_headingToGoal = normalize ? heading.AsVector2().Normalized() : heading;

	Float agentOrientation			= agent.GetWorldYaw();
	m_rotationToGoal				= ::EulerAngles::AngleDistance( agentOrientation, ::EulerAngles::YawFromXY( m_headingToGoal.X, m_headingToGoal.Y ) );
}

void SMoveLocomotionGoal::SetGoalPosition( const Vector& position )
{
	m_hasGoalPosition = true;
	m_goalPosition = position;
}

void SMoveLocomotionGoal::SetGoalTolerance( Float toleranceRadius )
{
	m_hasGoalTolerance = true;
	m_goalTolerance = toleranceRadius;
}

void SMoveLocomotionGoal::SetDistanceToGoal( Float distance )
{
	m_hasDistanceToGoal = true;
	m_distanceToGoal = distance;
}

void SMoveLocomotionGoal::SetDestinationPosition( const Vector& position )
{
	m_hasDestinationPosition = true;
	m_destinationPosition = position;
}

void SMoveLocomotionGoal::SetDistanceToDestination( Float distance )
{
	m_hasDistanceToDestination = true;
	m_distanceToDestination = distance;
}

void SMoveLocomotionGoal::SetOrientationGoal( const CMovingAgentComponent& agent, Float orientation, Bool alwaysSet, Bool clamped )
{
	m_orientationGoalAlwaysSet = alwaysSet;
	m_orientation = orientation;
	m_orientationClamping = clamped;

	Float agentOrientation			= agent.GetWorldYaw();
	m_orientationDiff				= ::EulerAngles::AngleDistance( agentOrientation, m_orientation );
	m_hasOrientation				= m_orientationGoalAlwaysSet ? true : ( fabs( m_orientationDiff ) > 1e-6f ); // MS: this is some very tiny angle (around 0.000000017 radians)
}

void SMoveLocomotionGoal::SetSpeedGoal( Float speed )
{
	m_hasSpeed = true;
	m_desiredSpeed					= ::Min( speed, m_desiredSpeed );
}

void SMoveLocomotionGoal::SetMaxWaitTime( Float time )
{
	m_maxWaitTime = ::Clamp( time, 0.0f, MAX_MOVEMENT_WAIT_TIME );
}

void SMoveLocomotionGoal::Clear()
{
	m_isFulfilled = true;
	m_hasHeading = false;
	m_hasOrientation = false;
	m_orientationClamping = true;
	m_hasSpeed = false;
	m_hasGoalPosition = false;
	m_hasGoalTolerance = false;
	m_hasDistanceToGoal = false;
	m_orientationGoalAlwaysSet = false;
	m_snapToMinimalSpeed = false;
	m_desiredSpeed = FLT_MAX;
	m_maxWaitTime = MAX_MOVEMENT_WAIT_TIME;
	m_matchMoveDirectionWithOrientation = false;
	m_matchOrientationWithHeading = false;
	m_headingImportanceMultiplier = 1.f;
	m_goalTarget = NULL;
	m_flags.ClearFast();
}

void SMoveLocomotionGoal::SetFlag( CName flag )
{
	SCustomFlagValue flagVal;
	flagVal.m_type = SCustomFlagValue::ECFT_None;
	m_flags.Insert( flag, flagVal );
}
void SMoveLocomotionGoal::SetFlag( CName flag, Float val )
{
	SCustomFlagValue flagVal;
	flagVal.m_type = SCustomFlagValue::ECFT_Float;
	flagVal.m_float = val;
	m_flags.Insert( flag, flagVal );
}
void SMoveLocomotionGoal::SetFlag( CName flag, Int32 val )
{
	SCustomFlagValue flagVal;
	flagVal.m_type	= SCustomFlagValue::ECFT_Int;
	flagVal.m_int	= val;
	m_flags.Insert( flag, flagVal );
}
void SMoveLocomotionGoal::SetFlag( CName flag, Bool val )
{
	SCustomFlagValue flagVal;
	flagVal.m_type	= SCustomFlagValue::ECFT_Bool;
	flagVal.m_bool	= val;
	m_flags.Insert( flag, flagVal );
}
void SMoveLocomotionGoal::SetFlag( CName flag, const Vector& val )
{
	SCustomFlagValue flagVal;
	flagVal.m_type = SCustomFlagValue::ECFT_Vector;
	for ( Uint32 i = 0; i < 4; ++i )
	{
		flagVal.m_vec.m_v[ i ] = val.A[ i ];
	}
	
	m_flags.Insert( flag, flagVal );
}
void SMoveLocomotionGoal::SetFlag( CName flag, void* obj, CClass* ptrClass )
{
	SCustomFlagValue flagVal;
	flagVal.m_type = SCustomFlagValue::ECFT_Pointer;
	flagVal.m_obj.m_ptr = obj;
	flagVal.m_obj.m_class = ptrClass;
	m_flags.Insert( flag, flagVal );
}

Bool SMoveLocomotionGoal::HasFlag( CName flag ) const
{
	return m_flags.Find( flag ) != m_flags.End();
}

Bool SMoveLocomotionGoal::GetFlag( CName flag, Float& outVal ) const
{
	auto itFind = m_flags.Find( flag );
	if ( itFind != m_flags.End() )
	{
		const SCustomFlagValue& val = itFind->m_second;
		if ( val.m_type == SCustomFlagValue::ECFT_Float )
		{
			outVal = val.m_float;

			return true;
		}
	}
	return false;
}

Bool SMoveLocomotionGoal::GetFlag( CName flag, Int32& outVal ) const
{
	auto itFind = m_flags.Find( flag );
	if ( itFind != m_flags.End() )
	{
		const SCustomFlagValue& val = itFind->m_second;
		if ( val.m_type == SCustomFlagValue::ECFT_Int )
		{
			outVal = val.m_int;

			return true;
		}
	}
	return false;
}

Bool SMoveLocomotionGoal::GetFlag( CName flag, Bool& outVal ) const
{
	auto itFind = m_flags.Find( flag );
	if ( itFind != m_flags.End() )
	{
		const SCustomFlagValue& val = itFind->m_second;
		if ( val.m_type == SCustomFlagValue::ECFT_Bool )
		{
			outVal = val.m_bool;

			return true;
		}
	}
	return false;
}

Bool SMoveLocomotionGoal::GetFlag( CName flag, Vector& outVal ) const
{
	auto itFind = m_flags.Find( flag );
	if ( itFind != m_flags.End() )
	{
		const SCustomFlagValue& val = itFind->m_second;
		if ( val.m_type == SCustomFlagValue::ECFT_Vector )
		{
			outVal = val.m_vec.m_v;

			return true;
		}
	}
	return false;
}

Bool SMoveLocomotionGoal::GetFlag( CName flag, void*& outObj, CClass* ptrClass ) const
{
	auto itFind = m_flags.Find( flag );
	if ( itFind != m_flags.End() )
	{
		const SCustomFlagValue& val = itFind->m_second;
		if ( val.m_type == SCustomFlagValue::ECFT_Pointer && ::SRTTI::GetInstance().CanCast( val.m_obj.m_class, ptrClass ) )
		{
			outObj = val.m_obj.m_ptr;

			return true;
		}
	}
	return false;
}

void SMoveLocomotionGoal::AddNotification( const CName& notificationName, Float timeout )
{
	if ( notificationName.Empty() )
	{
		return;
	}
	Uint32 count = m_expectedBehNotification.Size();
	Uint32 i = 0;
	for ( i = 0; i < count; ++i )
	{
		if ( m_expectedBehNotification[i].m_name == notificationName )
		{
			m_expectedBehNotification[i].m_timeout = timeout;
			break;
		}
	}
	if ( i >= count )
	{
		m_expectedBehNotification.PushBack( SMoveLocomotionGoal::BehNotification( notificationName, timeout ) );
	}
}
