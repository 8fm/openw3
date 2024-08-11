/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "formationSlot.h"

#include "formationMemberDataSlot.h"
#include "../core/mathUtils.h"


///////////////////////////////////////////////////////////////////////////////
// SFormationSlotConstraint
///////////////////////////////////////////////////////////////////////////////
Bool SFormationSlotConstraint::ComputeDesiredPos( CAISlotFormationLeaderData& formation, Vector2& outPos ) const
{
	if ( m_type == EFormationConstraint_POSITION )
	{
		// TODO: referencing higher slot indexes
		CFormationSlotInstance* slot = formation.GetSlot( m_referenceSlot );
		if ( slot )
		{
			outPos = slot->GetDesiredFormationPos() + m_value;
			return true;
		}
	}
	return false;
}

CFormationSlotInstance* SFormationSlotConstraint::GetReferencedSlot( CAISlotFormationLeaderData& formation ) const
{
	return formation.GetSlot( m_referenceSlot );
}
void SFormationSlotConstraint::ApplyToSteering( CMovingAgentComponent& mac, CSlotFormationMemberData& member, CAISlotFormationLeaderData& formation, CFormationSlotInstance* referenceSlot, Vector2& headingOutput, Float& speedOutput, Float& headingWeight, Float& speedWeight ) const
{
	CSlotFormationMemberData* referencedMember = referenceSlot->GetOwner();
	if ( !referencedMember )
	{
		return;
	}
	switch ( m_type )
	{
	case EFormationConstraint_POSITION:
		{
			const Vector& currentPos = mac.GetWorldPositionRef();
			const Vector& targetPos = referencedMember->GetDesiredPosition();
			Vector2 steerTarget = targetPos + MathUtils::GeometryUtils::Rotate2D( m_value, DEG2RAD( formation.GetLeaderOrientation() ) );
			
			Vector2 diffPos = steerTarget - currentPos.AsVector2();
			Float dist = diffPos.Mag();

			if ( dist > 1.f )
			{
				diffPos *= 1.f / dist;
			}

			// store desired position in member
			member.SetDistanceToSlot( dist );
			member.SetDesiredPosition( currentPos + diffPos );

			Float weight = m_strength;
			Float newHeadingWeight = headingWeight + weight;

			Float baseHeadingRatio = headingWeight / newHeadingWeight;
			Float applyHeadingRatio = weight / newHeadingWeight;

			Float desiredSpeed = 0.5f;// cachupspeed;

			Vector2 headingInput = diffPos;

			// compute ratio
			if ( dist < m_tolerance )
			{
				CActor* targetActor = referencedMember->GetOwner();
				CMovingAgentComponent* targetMac = targetActor ? targetActor->GetMovingAgentComponent() : NULL;
				if ( targetMac )
				{
					Vector2 targetHeading = targetMac->GetLocalToWorld().GetAxisY().AsVector2().Normalized();
					
					//Float targetSpeed = mac.ConvertSpeedAbsToRel( targetMac->GetAbsoluteMoveSpeed() );

					//targetHeading *= targetSpeed;

					Float goToSlotRatio = dist / m_tolerance;

					headingInput = targetHeading + (headingInput - targetHeading) * goToSlotRatio;
				}
			}

			// modify heading output
			headingOutput = headingOutput * baseHeadingRatio + headingInput * applyHeadingRatio;
			headingWeight = newHeadingWeight;

			//Float prevSpeedWeight = speedWeight;
			//speedWeight += m_strength;

			//speedOutput =
			//	speedOutput * (prevSpeedWeight/ speedWeight) +
			//	desiredSpeed * (m_strength / speedWeight);
		}
		break;
	case EFormationConstraint_SPEED:
		{
			CActor* actor = referencedMember->GetOwner();
			if ( !actor )
			{
				break;
			}
			Float prevSpeedWeight = speedWeight;
			speedWeight += m_strength;

			CMovingAgentComponent* guyMac = actor->GetMovingAgentComponent();
			if ( !guyMac )
			{
				break;
			}

			Float desiredSpeed = mac.ConvertSpeedAbsToRel( guyMac->GetAbsoluteMoveSpeed() ) / mac.GetMaxSpeed();

			speedOutput =
				speedOutput * (prevSpeedWeight/ speedWeight) +
				desiredSpeed * (m_strength / speedWeight);
		}
		break;
	}
}

///////////////////////////////////////////////////////////////////////////////
// CFormationSlotInstance
///////////////////////////////////////////////////////////////////////////////
Bool CFormationSlotInstance::ComputeDesiredPos( CAISlotFormationLeaderData& formation )
{
	m_desiredFormationPos.Set( 0.f, 0.f );
	for ( auto it = m_constraints.Begin(), end = m_constraints.End(); it != end; ++it )
	{
		if ( it->ComputeDesiredPos( formation, m_desiredFormationPos ) )
		{
			return true;
		}
	}
	return false;
}
CSlotFormationMemberData* CFormationSlotInstance::GetOwner() const
{
	return static_cast< CSlotFormationMemberData* >( &*m_memberPtr );
}

void CFormationSlotInstance::SetOwner( CSlotFormationMemberData* p )
{
	m_memberPtr = p;
}