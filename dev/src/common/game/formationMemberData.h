/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../core/refCountPointer.h"

class CAISlotFormationLeaderData;
class CSlotFormationMemberData;
class CSteeringFormationMemberData;


class CFormationMemberData
{
protected:
	THandle< CActor >				m_owner;
	Int32							m_refCnt;

	// cached member positioning data
	// NOTICE: they might not represent current member position, but reather his desired and accessible possition he is heading towards now
	Vector							m_memberPosition;											// Member desired position
public:
	CFormationMemberData( CActor* member )
		: m_owner( member )
		, m_refCnt( 0 )
		, m_memberPosition( member->GetWorldPositionRef() )										{}
	virtual ~CFormationMemberData()																{}

	virtual void					FollowPoint( CFormationLeaderData* leader, Vector3& outFollowPoint );
	virtual Bool					FallingBehindTest( CFormationLeaderData* leader, Vector3& outFollowPoint );
	virtual Float					ComputePriorityForSlot( CFormationLeaderData* leader, const Vector& slotPosition );
	virtual void					PrepeareSteering( CFormationLeaderData* leader  );
	virtual void					Update();
	virtual void					UpdateAsLeader();

	CActor*							GetOwner() const											{ return m_owner.Get(); }
	const Vector&					GetDesiredPosition() const									{ return m_memberPosition; }

	virtual void					InitializeMinCatchUpDistance( Float f );

	RED_INLINE void				AddRef()													{ ++m_refCnt; }
	RED_INLINE void				Release()													{ if ( --m_refCnt <= 0 ) { delete this; } }

	virtual CSlotFormationMemberData*		AsSlotFormationMemberData();
	virtual CSteeringFormationMemberData*	AsSteeringFormationMemberData();
};

typedef TRefCountPointer< CFormationMemberData > CFormationMemberDataPtr;


class CFormationLeaderData
{
	DECLARE_RTTI_SIMPLE_CLASS( CFormationLeaderData );

public:
	struct MemberOrder
	{
		// NOTICE: compare functions are not so cheap as they looks like, because they are doing THandle::Get's with each comparison
		static RED_INLINE Bool Less( const CFormationMemberData* m1, const CFormationMemberData* m2 )		{ return m1->GetOwner() < m2->GetOwner(); }
		static RED_INLINE Bool Less( const CFormationMemberDataPtr& m1, const CFormationMemberDataPtr& m2 )	{ return m1->GetOwner() < m2->GetOwner(); }
		static RED_INLINE Bool Less( const CActor* a, const CFormationMemberData* m )						{ return a < m->GetOwner(); }
		static RED_INLINE Bool Less( const CActor* a, const CFormationMemberDataPtr& m )					{ return a < m->GetOwner(); }
		static RED_INLINE Bool Less( const CFormationMemberData* m, const CActor* a )						{ return m->GetOwner() < a; }
		static RED_INLINE Bool Less( const CFormationMemberDataPtr& m, const CActor* a )					{ return m->GetOwner() < a; }
	};
	typedef TSortedArray< CFormationMemberDataPtr, MemberOrder > MemberList;

protected:
	const IFormationLogic*			m_definition;
	MemberList						m_members;
	CActor*							m_leader;
	Vector							m_leaderPos;
	Float							m_leaderOrientation;
	Float							m_leaderSpeed;
	EMoveType						m_leaderMoveType;
	Float							m_expirationTime;
	Float							m_memberDataLastUpdate;
	Uint32							m_flags;
	CFormationMemberDataPtr			m_leaderDataPtr;

	virtual void					DoPrecomputation( Float ownerLocalTime );
	void							Precompute( Float ownerLocalTime );
public:
	enum EStateFlags
	{
		FLAG_IS_MOVING				= FLAG( 0 ),
		FLAG_IS_PAUSED				= FLAG( 1 ),
	};

	CFormationLeaderData();
	virtual ~CFormationLeaderData();

	virtual void					Initialize( const IFormationLogic* logic, CActor* leader );
	void							Deinitialize()											{ m_leader = NULL; }

	virtual CFormationMemberData*	RegisterMember( CActor* actor );
	virtual void					UnregisterMember( CActor* actor );

	void							Precompute( CBehTreeInstance* owner );
	
	virtual void					Reorganize();

	void							PauseFormation()										{ m_flags |= FLAG_IS_PAUSED; m_flags &= ~FLAG_IS_MOVING; }
	void							ResumeFormation()										{ m_flags &= ~FLAG_IS_PAUSED; }

	Bool							IsFormationMoving() const								{ return (m_flags & FLAG_IS_MOVING) != 0; }
	Bool							IsFormationPaused() const								{ return (m_flags & FLAG_IS_PAUSED) != 0; }
	CActor*							GetLeader() const										{ return m_leader; }
	const Vector&					GetLeaderPosition() const								{ return m_leaderPos; }
	Float							GetLeaderOrientation() const							{ return m_leaderOrientation; }
	Float							GetLeaderSpeed() const									{ return m_leaderSpeed; }
	EMoveType						GetLeaderMoveType() const								{ return m_leaderMoveType; }
	const MemberList&				GetMemberList() const									{ return m_members; }
	virtual void					GetCenterOfMass( Vector& outCenterOfMass ) const;

	virtual CAISlotFormationLeaderData*		AsSlotFormationLeaderData();
};

BEGIN_NODEFAULT_CLASS_RTTI( CFormationLeaderData )
END_CLASS_RTTI()

