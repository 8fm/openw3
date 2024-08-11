/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "formationMemberDataSlot.h"

#include "formationLogicSlot.h"
#include "movableRepresentationPathAgent.h"
#include "../core/mathUtils.h"

IMPLEMENT_ENGINE_CLASS( CAISlotFormationLeaderData );

///////////////////////////////////////////////////////////////////////////////
// CSlotFormationMemberData
///////////////////////////////////////////////////////////////////////////////
CSlotFormationMemberData::CSlotFormationMemberData( CActor* member )
	: Super( member )
	, m_slotIndex( CFormationSlotInstance::INVALID_INDEX )
	, m_desiredPositionDistance( 0.f )
{

}
CSlotFormationMemberData::~CSlotFormationMemberData()
{

}

CAISlotFormationLeaderData* CSlotFormationMemberData::CastLeader( CFormationLeaderData* leader )
{
	return static_cast< CAISlotFormationLeaderData* >( leader );
}


CFormationSlotInstance* CSlotFormationMemberData::GetSlot( CFormationLeaderData* leader )
{
	return CastLeader( leader )->GetSlot( m_slotIndex );
}

void CSlotFormationMemberData::SetDesiredPosition( const Vector& pos )
{
	m_memberPosition = pos;
}
Bool CSlotFormationMemberData::GetFollowPoint( CFormationLeaderData* leader, Vector3& outFollowPoint )
{
	CFormationSlotInstance* slot = GetSlot( leader );
	if ( slot )
	{
		// move straight to his desired formation position
		// TODO: handle Z and properly handle situation when this point is inaccessible (can be nicely done after navigation consistant graph)
		CActor* actor = GetOwner();
		Vector3 formationPos = leader->GetLeaderPosition();
		formationPos.AsVector2() += MathUtils::GeometryUtils::Rotate2D( slot->GetDesiredFormationPos(), DEG2RAD( leader->GetLeaderOrientation() ) );
		CMovingAgentComponent* mac = actor->GetMovingAgentComponent();
		if ( mac->GetPathAgent()->TestLocation( formationPos ) )
		{
			outFollowPoint = formationPos;
			return true;
		}
	}
	return false;
}
void CSlotFormationMemberData::FollowPoint( CFormationLeaderData* leader, Vector3& outFollowPoint )
{
	if ( !GetFollowPoint( leader, outFollowPoint ) )
	{
		outFollowPoint = leader->GetLeaderPosition();
	}
}

Bool CSlotFormationMemberData::FallingBehindTest( CFormationLeaderData* leader, Vector3& outFollowPoint )
{
	if ( m_desiredPositionDistance > m_minCachupDistance )
	{
		Vector3 followPoint;
		// find follow point
		if ( GetFollowPoint( leader, followPoint ) )
		{
			CActor* actor = GetOwner();
			// test if we have blocked line to our follow point - so we should run pathfinding
			if ( !actor->GetMovingAgentComponent()->GetPathAgent()->TestLine( followPoint ) )
			{
				outFollowPoint = followPoint;
				return false;
			}
		}
	}
	return true;
}

void CSlotFormationMemberData::Update()
{

}

void CSlotFormationMemberData::InitializeMinCatchUpDistance( Float f )
{
	m_minCachupDistance = f;
}

void CSlotFormationMemberData::PrepeareSteering( CFormationLeaderData* leader  )
{
	Super::PrepeareSteering( leader );
}
CSlotFormationMemberData* CSlotFormationMemberData::AsSlotFormationMemberData()
{
	return this;
}



///////////////////////////////////////////////////////////////////////////////
// CAISlotFormationLeaderData
///////////////////////////////////////////////////////////////////////////////
CAISlotFormationLeaderData::CAISlotFormationLeaderData()
	: m_prevSlotGeneration( 0 )
	, m_formationReorganizationDelay( 0.f )
	, m_formationBreakRatio( 0.f )
	, m_isFormationSetup( false )
{

}

Bool CAISlotFormationLeaderData::RequestSlotsCount( Uint32 count )
{
	while ( m_slots.Size() < count )
	{
		if ( !m_slotsFactory.Parse( m_slots, *this ) )
		{
			return false;
		}
	}
	return true;
}

void CAISlotFormationLeaderData::ComputeDesiredPositions()
{
	for ( auto it = m_slots.Begin(), end = m_slots.End(); it != end; ++it )
	{
		it->ComputeDesiredPos( *this );
	}
}

void CAISlotFormationLeaderData::SetupFormation()
{
	if ( !m_isFormationSetup )
	{
		m_isFormationSetup = true;

		Uint32 membersCount = m_members.Size();
		RequestSlotsCount( membersCount );
		ComputeDesiredPositions();

		Uint32 usedSlots = Min( m_slots.Size(), membersCount );
		for ( Uint32 slotIndex = 0; slotIndex < usedSlots; ++slotIndex )
		{
			auto& slot = m_slots[ slotIndex ];
			CSlotFormationMemberData* bestMember = NULL;
			Float bestMemberDistSq = FLT_MAX;

			Vector2 worldSlotPos = m_leaderPos.AsVector2();
			worldSlotPos += MathUtils::GeometryUtils::Rotate2D( slot.GetDesiredFormationPos(), DEG2RAD( m_leaderOrientation ) );

			for ( Uint32 i = 0; i < membersCount; ++i )
			{
				CSlotFormationMemberData* member = static_cast< CSlotFormationMemberData* >( &(*m_members[ i ]) );
				if ( member->GetSlotIndex() != CFormationSlotInstance::INVALID_INDEX )
				{
					continue;
				}
				CActor*	actor = member->GetOwner();
				if ( actor )
				{
					const Vector& actorWorldPos = actor->GetWorldPositionRef();

					Float distSq = (worldSlotPos - actorWorldPos).SquareMag();

					if ( distSq < bestMemberDistSq )
					{
						bestMemberDistSq = distSq;
						bestMember = member;
					}
				}
			}

			if( bestMember )
			{
				bestMember->SetDesiredPosition( bestMember->GetOwner()->GetWorldPositionRef() );
				bestMember->SetSlotIndex( slotIndex );
				slot.SetOwner( bestMember );
			}
		}
	}
}
void CAISlotFormationLeaderData::FreeFormation()
{
	if ( m_isFormationSetup )
	{
		m_isFormationSetup = false;

		auto& slots = m_slots;
		for ( Uint32 i = 0; i < slots.Size(); ++i )
		{
			slots[ i ].Free();
		}

		for ( Uint32 i = 0, membersCount = m_members.Size(); i < membersCount; ++i )
		{
			CSlotFormationMemberData* member = static_cast< CSlotFormationMemberData* >( &(*m_members[ i ]) );
			member->SetSlotIndex( CFormationSlotInstance::INVALID_INDEX );
		}

	}
}

CFormationMemberData* CAISlotFormationLeaderData::RegisterMember( CActor* actor )
{
	// reset formation order
	FreeFormation();

	return Super::RegisterMember( actor );
}
void CAISlotFormationLeaderData::UnregisterMember( CActor* actor )
{
	// reset formation order
	FreeFormation();

	Super::UnregisterMember( actor );
}
void CAISlotFormationLeaderData::DoPrecomputation( Float ownerLocalTime )
{
	// check shouldn't we reorganize
	if ( m_isFormationSetup && m_formationReorganizationDelay < ownerLocalTime )
	{
		static const Float FORMATION_BREAK_RATIO = 0.5f;
		static const Float HIGH_SLOT_DISTANCE = 2.f;
		if ( m_members.Size() > 2.f )
		{
			Float ratio = 0.f;
			Float size = Float( m_members.Size() );
			for ( auto it = m_members.Begin(), end = m_members.End(); it != end; ++it )
			{
				CSlotFormationMemberData* slotMember = static_cast< CSlotFormationMemberData* >( (&**it) );
				if ( slotMember->GetDistanceToSlot() > HIGH_SLOT_DISTANCE )
				{
					ratio += 1.f / size;
				}
			}

			m_formationBreakRatio = ratio;

			if ( ratio > FORMATION_BREAK_RATIO )
			{
				m_formationReorganizationDelay += 3.8999f;
				// reset formation order as the formation itself is highly disrupted
				FreeFormation();
			}
			else
			{
				m_formationReorganizationDelay += 1.1112f;
			}
		}
	}

	// setup formation order if not set
	SetupFormation();

	Super::DoPrecomputation( ownerLocalTime );
}
void CAISlotFormationLeaderData::Reorganize()
{
	// reset formation order
	FreeFormation();

	Super::Reorganize();
}

void CAISlotFormationLeaderData::Initialize( const IFormationLogic* logic, CActor* leader )
{
	Super::Initialize( logic, leader );

	const CSlotFormationLogic* slotLogic = static_cast< const CSlotFormationLogic* >( logic );
	IFormationPatternNode* patternDef = slotLogic->GetFormationPattern();
	m_slotsFactory.Initialize( patternDef );

	CSlotFormationMemberData* leaderData = new CSlotFormationMemberData( leader );
	m_leaderDataPtr = leaderData;
	m_leaderSlot.SetOwner( leaderData );
}

CFormationSlotInstance* CAISlotFormationLeaderData::GetSlot( Int32 referenceIndex )
{
	if ( referenceIndex < 0 )
	{
		return &m_leaderSlot;
	}
	else if ( referenceIndex >= Int32(m_slots.Size()) )
	{
		return nullptr;
	}
	return &m_slots[ referenceIndex ];
}


CAISlotFormationLeaderData*	CAISlotFormationLeaderData::AsSlotFormationLeaderData() 
{
	return this;
}