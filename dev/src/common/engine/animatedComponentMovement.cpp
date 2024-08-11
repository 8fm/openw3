
/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "animatedComponent.h"
#include "skeleton.h"
#include "entity.h"
#include "speedConfig.h"

#ifdef DEBUG_AC
#pragma optimize("",off)
#endif

void CAnimatedComponent::SetMoveDirection( Float moveDirection )
{
	m_moveDirection = moveDirection;
}
void CAnimatedComponent::SetMoveTarget( const Vector& target )
{
	m_moveTarget = target;
}

void CAnimatedComponent::SetMoveHeading( Float heading )
{
	ASSERT( !Red::Math::NumericalUtils::IsNan( heading ) );

	m_moveHeading = heading;
}

void CAnimatedComponent::SetMoveRotation( Float currentRotation, Float rawDesiredRotation )
{
	ASSERT( !Red::Math::NumericalUtils::IsNan( currentRotation ) );
	m_moveRotation = currentRotation;
	m_moveRawDesiredRotation = rawDesiredRotation;
}

void CAnimatedComponent::SetMoveRotationSpeed( Float speed )
{
	m_moveRotationSpeed = speed;
}

Float CAnimatedComponent::ConvertSpeedAbsToRel( Float speedAbs )const 
{
	if ( m_speedConfig )
	{
		return m_speedConfig->ConvertSpeedAbsToRel( speedAbs );
	}
	else if ( m_skeleton )
	{
		return m_skeleton->deprec_ConvertSpeedAbsToRel( speedAbs );
	}
	return 0.0f;
}

Float CAnimatedComponent::ConvertSpeedRelToAbs( Float speedRel )const 
{
	if ( m_speedConfig )
	{
		return m_speedConfig->ConvertSpeedRelToAbs( speedRel );
	}
	else if ( m_skeleton )
	{
		return m_skeleton->deprec_ConvertSpeedRelToAbs( speedRel );
	}
	return 0.0f;
}

Float CAnimatedComponent::GetAbsoluteMoveSpeed() const
{
	if ( m_skeleton )
	{
		return ConvertSpeedRelToAbs( m_relativeMoveSpeed );
	}

	return 0.f;
}

Float CAnimatedComponent::GetSpeedRelSpan( ) const
{
	if ( m_speedConfig )
	{
		return m_speedConfig->GetSpeedRelSpan();
	}
	else if ( m_skeleton )
	{
		return m_skeleton->deprec_GetSpeedRelSpan();
	}

	return 0.f;
}

Float CAnimatedComponent::GetMoveRotationWorldSpace() const
{
	return m_moveRotation;
}

Float CAnimatedComponent::GetMoveRotationModelSpace() const
{
	return DistanceBetweenAngles( GetEntity()->GetRotation().Yaw, m_moveRotation );
}

Float CAnimatedComponent::GetMoveRawDesiredRotationWorldSpace() const
{
	// TODO: WTF? GOTTA TALK WITH TOMSIN/JAREK WHEN THEY ARE AVAILABLE WHATS UP WITH LEFT/RIGHT POSITIVE/NEGATIVE ROTATIONS
	return -m_moveRawDesiredRotation;
}

Float CAnimatedComponent::GetRotationSpeed() const
{
	return m_moveRotationSpeed;
}

const Vector& CAnimatedComponent::GetMoveTargetWorldSpace() const
{
	return m_moveTarget;
}

Vector CAnimatedComponent::GetMoveTargetModelSpace() const
{
	return m_moveTarget - GetWorldPositionRef();
}

Float CAnimatedComponent::GetMoveHeadingWorldSpace() const
{
	return m_moveHeading;
}

Float CAnimatedComponent::GetMoveHeadingModelSpace() const
{
	return m_moveHeading != CEntity::HEADING_ANY ? DistanceBetweenAngles( GetEntity()->GetWorldYaw(), m_moveHeading ) : CEntity::HEADING_ANY;
}

Float CAnimatedComponent::GetMoveDirectionWorldSpace() const 
{
	return m_moveDirection;
}

Float CAnimatedComponent::GetDesiredMoveDirectionWorldSpace() const
{
	return m_moveDirection;
}

Float CAnimatedComponent::GetMoveDirectionModelSpace() const 
{
	const Float dist = DistanceBetweenAngles( GetEntity()->GetWorldYaw(), m_moveDirection );
	return dist;
}




#ifdef DEBUG_AC
#pragma optimize("",on)
#endif
