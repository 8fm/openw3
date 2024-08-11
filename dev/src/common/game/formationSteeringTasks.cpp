/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "formationSteeringTasks.h"

#include "formationMemberDataSteering.h"
#include "movementCommandBuffer.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "../core/mathUtils.h"

IMPLEMENT_ENGINE_CLASS( IFormationSteeringTask )
IMPLEMENT_ENGINE_CLASS( IFormationFragmentarySteeringTask )
IMPLEMENT_ENGINE_CLASS( CFormationSteerToLeaderSteeringTask )
IMPLEMENT_ENGINE_CLASS( CFormationSteerToCenterOfMassSteeringTask )
IMPLEMENT_ENGINE_CLASS( CFormationDontBackDownSteeringTask )
IMPLEMENT_ENGINE_CLASS( CFormationFaceSteeringTask )
IMPLEMENT_ENGINE_CLASS( CFormationSteerToPathSteeringTask )
IMPLEMENT_ENGINE_CLASS( CFormationKeepDistanceToMembersSteeringTask )
IMPLEMENT_ENGINE_CLASS( CFormationKeepSpeedSteeringTask )
IMPLEMENT_ENGINE_CLASS( CFormationKeepAwaylLeaderSteeringTask )
IMPLEMENT_ENGINE_CLASS( CFormationKeepComradesSpeedSteeringTask )
IMPLEMENT_ENGINE_CLASS( CFormationDontFallBehindSteeringTask )

///////////////////////////////////////////////////////////////////////////////
// IFormationSteeringTask
///////////////////////////////////////////////////////////////////////////////
String IFormationSteeringTask::GetTaskName() const
{
	return TXT("Formation");
}

///////////////////////////////////////////////////////////////////////////////
// IFormationFragmentarySteeringTask
///////////////////////////////////////////////////////////////////////////////

void IFormationFragmentarySteeringTask::CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const
{
	SFormationSteeringInput* input = SFormationSteeringInput::GetGeneralFormationData( comm.GetGoal() );
	if ( !input )
	{
		return;
	}

	Vector2 heading( 0.f,0.f );
	Float headingRatio = 1.f;
	Float overrideSpeed = 1.f;
	Float overrideSpeedImportance = 0.f;

	CalculateSteeringInput( comm, data, *input, heading, headingRatio, overrideSpeed, overrideSpeedImportance );

	if ( headingRatio > 0.f )
	{
		comm.AddHeading( heading, m_importance * headingRatio );
	}
	if ( overrideSpeedImportance > 0.f )
	{
		comm.AddSpeed( overrideSpeed, overrideSpeedImportance );
	}
}

String IFormationFragmentarySteeringTask::GetTaskName() const
{
	return TXT("Formation fragmentary task");
}

///////////////////////////////////////////////////////////////////////////////
// CFormationSteerToLeaderSteeringTask
///////////////////////////////////////////////////////////////////////////////
void CFormationSteerToLeaderSteeringTask::CalculateSteeringInput( IMovementCommandBuffer& comm, InstanceBuffer& data, SFormationSteeringInput& formationData, Vector2& outHeading, Float& outHeadingRatio, Float& outOverrideSpeed, Float& outOverrideSpeedImportance ) const
{
	CMovingAgentComponent& mac = comm.GetAgent();
	const Vector& leaderPos = formationData.m_leaderData->GetLeaderPosition();

	outHeading = ( leaderPos.AsVector2() - mac.GetWorldPositionRef().AsVector2() ).Normalized();
}

String CFormationSteerToLeaderSteeringTask::GetTaskName() const
{
	return TXT("Steer to formation leader");
}


///////////////////////////////////////////////////////////////////////////////
// CFormationSteerToCenterOfMassSteeringTask
///////////////////////////////////////////////////////////////////////////////
void CFormationSteerToCenterOfMassSteeringTask::CalculateSteeringInput( IMovementCommandBuffer& comm, InstanceBuffer& data, SFormationSteeringInput& formationData, Vector2& outHeading, Float& outHeadingRatio, Float& outOverrideSpeed, Float& outOverrideSpeedImportance ) const
{
	CMovingAgentComponent& mac = comm.GetAgent();
	Vector centerOfMass;
	formationData.m_leaderData->GetCenterOfMass( centerOfMass );

	outHeading = ( centerOfMass.AsVector2() - mac.GetWorldPositionRef().AsVector2() ).Normalized();
}

String CFormationSteerToCenterOfMassSteeringTask::GetTaskName() const
{
	return TXT("Steer to formation center of mass");
}

///////////////////////////////////////////////////////////////////////////////
// CFormationDontBackDownSteeringTask
///////////////////////////////////////////////////////////////////////////////
void CFormationDontBackDownSteeringTask::CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const
{
	SFormationSteeringInput* input = SFormationSteeringInput::GetGeneralFormationData( comm.GetGoal() );
	if ( !input )
	{
		return;
	}

	Vector2 currHeading = comm.GetHeading();
	if ( currHeading.IsAlmostZero() )
	{
		return;
	}

	Vector2 toTarget = EulerAngles::YawToVector2( input->m_leaderData->GetLeaderOrientation() );
	Float headingLen = currHeading.Mag();
	Vector2 normalizedHeadig = currHeading * (1.f / headingLen);
	Float cosAngle = data[ i_cosAngle ];

	Float cosGamma = toTarget.Dot( normalizedHeadig );
	if ( cosGamma < cosAngle )
	{
		Float sinAngle = data[ i_sinAngle ];

		Float sinGamma = sqrt( 1.f - cosGamma );
		Float r = (headingLen * sinGamma / data[ i_sinAngle ]);

		if ( toTarget.CrossZ( normalizedHeadig ) < 0.f )
		{
			sinAngle = -sinAngle;
		}

		Vector2 modifedHeading = MathUtils::GeometryUtils::Rotate2D( toTarget, sinAngle, cosAngle );
		modifedHeading *= r;

		comm.ModifyHeading( modifedHeading );
	}
}
String CFormationDontBackDownSteeringTask::GetTaskName() const
{
	return TXT("Formation: Move only forward");
}

void CFormationDontBackDownSteeringTask::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_cosAngle;
	compiler << i_sinAngle;
}
void CFormationDontBackDownSteeringTask::OnInitData( CMovingAgentComponent& agent, InstanceBuffer& data )
{
	TBaseClass::OnInitData( agent, data );

	data[ i_cosAngle ] = cosf( DEG2RAD( m_maxAngleDifference ) );
	data[ i_sinAngle ] = sinf( DEG2RAD( m_maxAngleDifference ) );
}

///////////////////////////////////////////////////////////////////////////////
// CFormationFaceSteeringTask
///////////////////////////////////////////////////////////////////////////////
void CFormationFaceSteeringTask::CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const
{
	SFormationSteeringInput* input = SFormationSteeringInput::GetGeneralFormationData( comm.GetGoal() );
	if ( !input )
	{
		return;
	}
	CMovingAgentComponent& moveAgent	= comm.GetAgent();
	Float desiredYaw					= input->m_leaderData->GetLeaderOrientation();
	Float currYaw						= moveAgent.GetWorldYaw();
	Float yawDiff						= EulerAngles::AngleDistance( currYaw, desiredYaw );
	comm.LockRotation( yawDiff );
}
String CFormationFaceSteeringTask::GetTaskName() const
{
	return TXT("Face formation orientation");
}

///////////////////////////////////////////////////////////////////////////////
// CFormationSteerToPathSteeringTask
///////////////////////////////////////////////////////////////////////////////
void CFormationSteerToPathSteeringTask::CalculateSteeringInput( IMovementCommandBuffer& comm, InstanceBuffer& data, SFormationSteeringInput& formationData, Vector2& outHeading, Float& outHeadingRatio, Float& outOverrideSpeed, Float& outOverrideSpeedImportance ) const
{
	outHeadingRatio = 0.f;
}
String CFormationSteerToPathSteeringTask::GetTaskName() const
{
	return TXT("Steer to formation path");
}

///////////////////////////////////////////////////////////////////////////////
// CFormationKeepDistanceToMembersSteeringTask
///////////////////////////////////////////////////////////////////////////////
void CFormationKeepDistanceToMembersSteeringTask::CalculateSteeringInput( IMovementCommandBuffer& comm, InstanceBuffer& data, SFormationSteeringInput& formationData, Vector2& outHeading, Float& outHeadingRatio, Float& outOverrideSpeed, Float& outOverrideSpeedImportance ) const
{
	CSteeringFormationMemberData* steeringMember = formationData.m_memberData->AsSteeringFormationMemberData();
	if ( !steeringMember )
	{
		outHeadingRatio = 0.f;
		return;
	}
	Float desiredDistanceSq =  m_desiredDistance * m_desiredDistance;
	Vector2 ownerPos = comm.GetAgent().GetWorldPositionRef().AsVector2();
	Vector2 accumulator( 0.f, 0.f );
	const auto& cachedMembers = steeringMember->GetCachedMembers();
	for( auto it = cachedMembers.Begin(), end = cachedMembers.End(); it != end; ++it )
	{
		const CSteeringFormationMemberData::SComrade& cachedData = *it;
		if ( cachedData.m_actorSqDistance >= desiredDistanceSq )
		{
			// other cached actors will be too far
			break;
		}

		Vector2 diff = ownerPos - cachedData.m_actor->GetWorldPositionRef();
		Float dist = sqrt( cachedData.m_actorSqDistance );
		Float ratio = 1.f;
		if ( dist > m_minDistance )
		{
			ratio = 1.f - (dist - m_minDistance) / (m_desiredDistance - m_minDistance);
		}
		accumulator += diff * (ratio / dist);	
	}

	Float outputLen = accumulator.Mag();
	if ( outputLen < NumericLimits< Float >::Epsilon() )
	{
		outHeadingRatio = 0.f;
		return;
	}
	outHeadingRatio = outputLen >= 1.f ? 1.f : outputLen;
	outHeading = accumulator * (1.f / outputLen);
}
String CFormationKeepDistanceToMembersSteeringTask::GetTaskName() const
{
	return TXT("Keep distance to formation members");
}

///////////////////////////////////////////////////////////////////////////////
// CFormationKeepSpeedSteeringTask
///////////////////////////////////////////////////////////////////////////////
void CFormationKeepSpeedSteeringTask::CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const
{
	SFormationSteeringInput* input = SFormationSteeringInput::GetGeneralFormationData( comm.GetGoal() );
	if ( !input )
	{
		return;
	}
	const CMovingAgentComponent& mac	= comm.GetAgent();
	const auto* leader					= input->m_leaderData;
	Float leaderAbsSpeed				= leader->GetLeaderSpeed();
	Float relSpeed						= mac.ConvertSpeedAbsToRel( leaderAbsSpeed ) / mac.ConvertSpeedAbsToRel( mac.GetMaxSpeed() );

	comm.AddSpeed( relSpeed, m_speedImportance );
}
String CFormationKeepSpeedSteeringTask::GetTaskName() const
{
	return TXT("Keep formation speed");
}

///////////////////////////////////////////////////////////////////////////////
// CFormationKeepAwaylLeaderSteeringTask
///////////////////////////////////////////////////////////////////////////////
void CFormationKeepAwaylLeaderSteeringTask::CalculateSteeringInput( IMovementCommandBuffer& comm, InstanceBuffer& data, SFormationSteeringInput& formationData, Vector2& outHeading, Float& outHeadingRatio, Float& outOverrideSpeed, Float& outOverrideSpeedImportance ) const
{
	const auto* leaderData = formationData.m_leaderData;
	const Vector& m_ownerPos = comm.GetAgent().GetWorldPositionRef();
	const Vector& leaderPos = leaderData->GetLeader()->GetWorldPositionRef();
	Vector2 diff = m_ownerPos.AsVector2() - leaderPos.AsVector2();
	Float distSq = diff.SquareMag();
	if ( distSq >= m_noticeLeaderDistance*m_noticeLeaderDistance || distSq < NumericLimits< Float >::Epsilon() )
	{
		outHeadingRatio = 0.f;
		return;
	}

	Float dist = sqrt( distSq );

	Float ratio = 1.f;
	if ( dist > m_minLeaderDistance )
	{
		ratio = 1.f - (dist - m_minLeaderDistance) / (m_noticeLeaderDistance - m_minLeaderDistance);
	}
	outHeadingRatio = ratio;
	outHeading = diff * (1.f / dist);			// normalize
}
String CFormationKeepAwaylLeaderSteeringTask::GetTaskName() const
{
	return TXT("Keep away leader");
}
///////////////////////////////////////////////////////////////////////////////
// CFormationKeepComradesSpeedSteeringTask
///////////////////////////////////////////////////////////////////////////////
void CFormationKeepComradesSpeedSteeringTask::CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const
{
	SFormationSteeringInput* formationData = SFormationSteeringInput::GetGeneralFormationData( comm.GetGoal() );
	if ( !formationData )
	{
		return;
	}
	CSteeringFormationMemberData* steeringMember = formationData->m_memberData->AsSteeringFormationMemberData();
	if ( !steeringMember )
	{
		return;
	}
	Float maxDistanceSq =  m_distanceToComrades * m_distanceToComrades;
	Float accumulator = 0.f;
	Float weight = 0.f;
	CMovingAgentComponent& myMac = comm.GetAgent();

	const auto& cachedMembers = steeringMember->GetCachedMembers();
	for( auto it = cachedMembers.Begin(), end = cachedMembers.End(); it != end; ++it )
	{
		const CSteeringFormationMemberData::SComrade& cachedData = *it;
		if ( cachedData.m_actorSqDistance >= maxDistanceSq )
		{
			// other cached actors will be too far
			break;
		}
		Float guyWeight = 1.f - cachedData.m_actorSqDistance / maxDistanceSq;				// NOTICE: square root outcome interpolation
		CMovingAgentComponent* guyMac = cachedData.m_actor->GetMovingAgentComponent();
		if ( guyMac )
		{
			Float desiredSpeed = myMac.ConvertSpeedAbsToRel( guyMac->GetAbsoluteMoveSpeed() ) / myMac.GetMaxSpeed();

			Float newWeight = weight + guyWeight;

			accumulator =
				desiredSpeed * (guyWeight / newWeight) +
				accumulator * (weight / newWeight);

			weight = newWeight;
		}
	}

	if ( weight > 0.f )
	{
		Float importance = weight >= 1.f ? m_speedImportance : weight * m_speedImportance;
		comm.AddSpeed( accumulator, importance );
	}
	
}
String CFormationKeepComradesSpeedSteeringTask::GetTaskName() const
{
	return TXT("Formation: keep surrounding members speed");
}
///////////////////////////////////////////////////////////////////////////////
// CFormationDontFallBehindSteeringTask
///////////////////////////////////////////////////////////////////////////////
void CFormationDontFallBehindSteeringTask::CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const
{
	SFormationSteeringInput* formationData = SFormationSteeringInput::GetGeneralFormationData( comm.GetGoal() );
	if ( !formationData )
	{
		return;
	}
	CSteeringFormationMemberData* steeringMember = formationData->m_memberData->AsSteeringFormationMemberData();
	if ( steeringMember )
	{
		const auto& cachedMembers = steeringMember->GetCachedMembers();
		if ( cachedMembers.Empty() )
		{
			return;
		}
		Float distSq = cachedMembers[ 0 ].m_actorSqDistance;
		if ( distSq >= m_maxFallBehindDistance*m_maxFallBehindDistance )
		{
			return;
		}

		CMovingAgentComponent& mac = comm.GetAgent();
		const Vector2& leaderPos = formationData->LeaderData()->GetLeader()->GetWorldPositionRef().AsVector2();
		const Vector2& myPos = mac.GetWorldPositionRef().AsVector2();
		Vector2 myForward = mac.GetWorldForward().AsVector2();
		if ( (leaderPos - myPos).Dot( myForward ) < 0.f )
		{
			return;
		}

		Float importance = m_speedImportance;
		if ( distSq > m_minFallBehindDistance*m_minFallBehindDistance )
		{
			Float dist = sqrt( distSq );
			importance *= 1.f - (dist - m_minFallBehindDistance) / (m_maxFallBehindDistance - m_minFallBehindDistance);
		}

		comm.AddSpeed( 1.f, importance );
	}
	
}
String CFormationDontFallBehindSteeringTask::GetTaskName() const
{
	return TXT("Formation: dont fall behind");
}
