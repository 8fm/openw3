/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "formationMemberData.h"

#include "behTreeInstance.h"
#include "formationLogic.h"
#include "movableRepresentationPathAgent.h"

IMPLEMENT_ENGINE_CLASS( CFormationLeaderData );

///////////////////////////////////////////////////////////////////////////////
// CFormationMemberData
///////////////////////////////////////////////////////////////////////////////

Float CFormationMemberData::ComputePriorityForSlot( CFormationLeaderData* leader, const Vector& slotPosition )
{
	return 0.f;
}
void CFormationMemberData::PrepeareSteering( CFormationLeaderData* leader )
{

}
void CFormationMemberData::Update()
{
	m_memberPosition = m_owner.Get()->GetWorldPositionRef();
}
void CFormationMemberData::UpdateAsLeader()
{
	CActor* actor = m_owner.Get();
	if ( actor )
	{
		m_memberPosition = actor->GetWorldPositionRef();
	}
}
CSlotFormationMemberData* CFormationMemberData::AsSlotFormationMemberData()
{
	return nullptr;
}
CSteeringFormationMemberData* CFormationMemberData::AsSteeringFormationMemberData()
{
	return nullptr;
}
void CFormationMemberData::FollowPoint( CFormationLeaderData* leader, Vector3& outFollowPoint )
{
	outFollowPoint = leader->GetLeaderPosition();
}

Bool CFormationMemberData::FallingBehindTest( CFormationLeaderData* leader, Vector3& outFollowPoint )
{
	return true;
}
void CFormationMemberData::InitializeMinCatchUpDistance( Float f )
{

}

///////////////////////////////////////////////////////////////////////////////
// CFormationLeaderData
///////////////////////////////////////////////////////////////////////////////
CFormationLeaderData::CFormationLeaderData()
	: m_definition( NULL )
	, m_leader( NULL )
	, m_leaderSpeed( 0.f )
	, m_expirationTime( 0.f )
	, m_memberDataLastUpdate( 0.f )
	, m_flags( 0 )
{

}
CFormationLeaderData::~CFormationLeaderData()
{

}

void CFormationLeaderData::Initialize( const IFormationLogic* logic, CActor* leader )
{
	m_definition = logic;
	m_leader = leader;
	m_leaderPos = leader->GetWorldPositionRef();
	m_leaderOrientation = leader->GetWorldYaw();
}

void CFormationLeaderData::DoPrecomputation( Float ownerLocalTime )
{
	m_flags = 0;
	if ( !IsFormationPaused() )
	{
		m_leaderPos = m_leader->GetWorldPositionRef();
		m_leaderOrientation = m_leader->GetWorldYaw();
		m_leaderSpeed = m_leader->GetMovingAgentComponent()->GetAbsoluteMoveSpeed();
		if ( m_leader->IsMoving() || m_leaderSpeed > NumericLimits< Float >::Epsilon() )
		{
			m_flags |= FLAG_IS_MOVING;
		}
		else
		{
			m_flags &= ~FLAG_IS_MOVING;
		}
	}
	
}

CFormationMemberData* CFormationLeaderData::RegisterMember( CActor* actor )
{
	CFormationMemberData* memberData = m_definition->CreateMemberData( actor );
	m_members.Insert( memberData );
	return memberData;
}
void CFormationLeaderData::UnregisterMember( CActor* actor )
{
	auto itFind = m_members.Find( actor );
	if ( itFind != m_members.End() )
	{
		m_members.Erase( itFind );
	}
}

void CFormationLeaderData::Precompute( Float aiTime )
{
	if ( m_memberDataLastUpdate != aiTime )
	{
		m_memberDataLastUpdate = aiTime;
		if ( !IsFormationPaused() )
		{
			CFormationMemberData* leaderMember = m_leaderDataPtr.Get();
			if ( leaderMember )
			{
				leaderMember->UpdateAsLeader();
			}
		}
	}
	if ( m_expirationTime < aiTime )
	{
		m_expirationTime = aiTime + m_definition->GetRecomputationDelay();

		DoPrecomputation( aiTime );
	}
}
void CFormationLeaderData::Precompute( CBehTreeInstance* owner )
{
	Precompute( owner->GetLocalTime() );
}
void CFormationLeaderData::Reorganize()
{
	m_expirationTime = 0.f;
}

void CFormationLeaderData::GetCenterOfMass( Vector& outCenterOfMass ) const
{
	outCenterOfMass = m_leaderPos;
}
CAISlotFormationLeaderData* CFormationLeaderData::AsSlotFormationLeaderData()
{
	return nullptr;
}
