/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "formationMemberDataSteering.h"

#include "movableRepresentationPathAgent.h"
#include "movingAgentComponent.h"

IMPLEMENT_ENGINE_CLASS( CAISteeringFormationLeaderData );

///////////////////////////////////////////////////////////////////////////////
// CSteeringFormationMemberData
///////////////////////////////////////////////////////////////////////////////
CSteeringFormationMemberData::~CSteeringFormationMemberData()
{}

void CSteeringFormationMemberData::FollowPoint( CFormationLeaderData* leader, Vector3& outFollowPoint )
{
	CActor* me = m_owner.Get();
	Vector3 position = me->GetWorldPositionRef().AsVector3();

	Vector3 followPoint = leader->GetLeaderPosition();
	Float closestMemberSq = (followPoint - position).SquareMag();

	const auto& members = leader->GetMemberList();
	for ( auto it = members.Begin(), end = members.End(); it != end; ++it )
	{
		CFormationMemberData* member = (*it).Get();
		CSteeringFormationMemberData* steeringMember = static_cast< CSteeringFormationMemberData* > ( member );
		// if given member is further from the leader than us - bail out
		if ( steeringMember->m_cachedLeaderDistanceSq >= m_cachedLeaderDistanceSq )
		{
			continue;
		}
		const Vector3& memberPos = steeringMember->GetOwner()->GetWorldPositionRef().AsVector3();
		Float distSq = (memberPos - position).SquareMag();
		if ( distSq >= closestMemberSq )
		{
			continue;
		}
		closestMemberSq = distSq;
		followPoint = memberPos;
	}

	outFollowPoint = followPoint;
}

Bool CSteeringFormationMemberData::FallingBehindTest( CFormationLeaderData* leader, Vector3& outFollowPoint )
{
	CActor* actor = m_owner.Get();
	CPathAgent* pathAgent = actor->GetMovingAgentComponent()->GetPathAgent();

	Vector3 followPoint;
	FollowPoint( leader, followPoint );
	if ( ( followPoint.AsVector2() - pathAgent->GetPosition().AsVector2() ).SquareMag() < m_minCatchUpDistanceSq )
	{
		return true;
	}

	if ( !pathAgent->TestLine( followPoint ) )
	{
		outFollowPoint = followPoint;

		return false;
	}

	return true;
}

void CSteeringFormationMemberData::PrepeareSteering( CFormationLeaderData* leader )
{
	// NOTICE: For now we will use hardcoded max distance for formation algorithms.
	// To switch to user-defined distance we would do special steering node that does
	// precaching. Altrought it would be cool, it will be another constrain for
	// steering graph creators - so try to automize it for now.
	const Float CACHE_MAX_DIST = 10.f;
	CacheClosebyMembers( leader, CACHE_MAX_DIST*CACHE_MAX_DIST );
}

void CSteeringFormationMemberData::CacheClosebyMembers( CFormationLeaderData* leader, Float cacheDistanceSq )
{
	struct Local : public Red::System::NonCopyable
	{
		Local( CSteeringFormationMemberData* me, CActor* myOwner, Float cacheDistanceSq )
			: m_pos( myOwner->GetWorldPositionRef().AsVector2() )
			, m_cacheDistanceSq( cacheDistanceSq )
			, m_list( me->m_cachedMembers ) {}
		Float Process( CActor* actor )
		{
			const Vector& actorPos = actor->GetWorldPositionRef();
			Float distSq = (m_pos - actorPos.AsVector2()).SquareMag();
			if ( distSq < m_cacheDistanceSq )
			{
				SComrade c;
				c.m_actor = actor;
				c.m_actorSqDistance = distSq;

				m_list.PushBack( c );
			}
			return distSq;
		}

		Vector2				m_pos;
		Float				m_cacheDistanceSq;
		Comrades&			m_list;
	};

	ClearCache();

	CActor* myOwner = m_owner.Get();
	Local local( this, myOwner, cacheDistanceSq );

	const auto& members = leader->GetMemberList();

	for ( auto it = members.Begin(), end = members.End(); it != end; ++it )
	{
		CFormationMemberData* member = (*it).Get();
		CActor* actor = member->GetOwner();
		if ( actor == myOwner )
		{
			continue;
		}
		local.Process( actor );

	}
	m_cachedLeaderDistanceSq = local.Process( leader->GetLeader() );

	m_cachedMembers.Sort();
}
void CSteeringFormationMemberData::ClearCache()
{
	m_cachedMembers.ClearFast();
}

CSteeringFormationMemberData* CSteeringFormationMemberData::AsSteeringFormationMemberData()
{
	return this;
}
void CSteeringFormationMemberData::InitializeMinCatchUpDistance( Float f )
{
	m_minCatchUpDistanceSq = f * f;
}

///////////////////////////////////////////////////////////////////////////////
// CAISteeringFormationLeaderData
///////////////////////////////////////////////////////////////////////////////
void CAISteeringFormationLeaderData::DoPrecomputation( Float ownerLocalTime )
{
	Super::DoPrecomputation( ownerLocalTime );

	m_centerOfMass = m_leaderPos;												// as fallback leaders pos is center of mass
	Float overalWeight = 1.f;

	for ( auto it = m_members.Begin(), end = m_members.End(); it != end; ++it )
	{
		CFormationMemberData* member = (*it).Get();
		const Vector& pos = member->GetOwner()->GetWorldPositionRef();
		Float newWeight = overalWeight + 1.f;
		m_centerOfMass =														// move center of mass
			pos * (1.f / newWeight) +											// member impact
			m_centerOfMass * (overalWeight / newWeight);						// current center of mass impact
	}
}
void CAISteeringFormationLeaderData::GetCenterOfMass( Vector& outCenterOfMass ) const
{
	outCenterOfMass = m_centerOfMass;
}

