/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "formationMemberData.h"
#include "formationSlot.h"



class CSlotFormationMemberData : public CFormationMemberData
{
	typedef CFormationMemberData Super;
protected:
	Uint32							m_slotIndex;
	Float							m_desiredPositionDistance;
	Float							m_minCachupDistance;

	Bool							GetFollowPoint( CFormationLeaderData* leader, Vector3& outFollowPoint );
public:
	CSlotFormationMemberData( CActor* member );
	~CSlotFormationMemberData();

	void							FollowPoint( CFormationLeaderData* leader, Vector3& outFollowPoint ) override;
	Bool							FallingBehindTest( CFormationLeaderData* leader, Vector3& outFollowPoint ) override;
	void							PrepeareSteering( CFormationLeaderData* leader  ) override;
	void							Update() override;
	void							InitializeMinCatchUpDistance( Float f ) override;
	CSlotFormationMemberData*		AsSlotFormationMemberData() override;

	CAISlotFormationLeaderData*		CastLeader( CFormationLeaderData* leader );				// If we know that our member are of slot class, than we are sure our leader is also.
	CFormationSlotInstance*			GetSlot( CFormationLeaderData* leader );

	void							SetDesiredPosition( const Vector& pos );
	void							SetDistanceToSlot( Float f )							{ m_desiredPositionDistance = f; }
	Float							GetDistanceToSlot() const								{ return m_desiredPositionDistance; }

	Uint32							GetSlotIndex() const									{ return m_slotIndex; }
	void							SetSlotIndex( Uint32 slot )								{ m_slotIndex = slot; }

	
};


typedef TRefCountPointer< CSlotFormationMemberData > CSlotFormationMemberDataPtr;


class CAISlotFormationLeaderData : public CFormationLeaderData
{
	typedef CFormationLeaderData Super;
	DECLARE_RTTI_SIMPLE_CLASS( CAISlotFormationLeaderData );
protected:
	TDynArray< CFormationSlotInstance >		m_slots;
	IFormationPatternNode::CParser			m_slotsFactory;
	CFormationSlotInstance					m_leaderSlot;
	Uint32									m_prevSlotGeneration;

	Float									m_formationReorganizationDelay;
	Float									m_formationBreakRatio;

	Bool									m_isFormationSetup;


	Bool							RequestSlotsCount( Uint32 count );
	void							ComputeDesiredPositions();

	void							SetupFormation();
	void							FreeFormation();

	void							DoPrecomputation( Float ownerLocalTime ) override;
public:
	CAISlotFormationLeaderData();

	CFormationMemberData*			RegisterMember( CActor* actor ) override;
	void							UnregisterMember( CActor* actor ) override;

	void							Reorganize() override;

	void							Initialize( const IFormationLogic* logic, CActor* leader ) override;

	CFormationSlotInstance*			GetSlot( Int32 referenceIndex );
	Uint32							GetNextGeneration()										{ return ++m_prevSlotGeneration; }
	Float							GetCurrentBreakRatio() const							{ return m_formationBreakRatio; }

	CAISlotFormationLeaderData*		AsSlotFormationLeaderData() override;
};

BEGIN_NODEFAULT_CLASS_RTTI( CAISlotFormationLeaderData )
END_CLASS_RTTI()