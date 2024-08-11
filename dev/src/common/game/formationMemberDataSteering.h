/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "formationMemberData.h"



class CSteeringFormationMemberData : public CFormationMemberData
{
	typedef CFormationMemberData Super;
public:
	struct SComrade
	{
		CActor*							m_actor;
		Float							m_actorSqDistance;
		union 
		{
			CSteeringFormationMemberData*	m_memberData;

		};

		Bool operator<( const SComrade& c ) const
		{
			return m_actorSqDistance < c.m_actorSqDistance;	// NOTICE: weak order
		}
	};
	typedef TSortedArray< SComrade > Comrades;				// NOTICE: because of weak order you can't do binary 'Find' on comrades list

protected:
	Comrades												m_cachedMembers;
	Float													m_cachedLeaderDistanceSq;
	Float													m_minCatchUpDistanceSq;
	//Vector2													m_lastSteeringOutput;
	//Float													m_lastSteeringSpeed;

public:
	CSteeringFormationMemberData( CActor* member )
		: CFormationMemberData( member )														{}
	~CSteeringFormationMemberData();

	void							CacheClosebyMembers( CFormationLeaderData* leader, Float cacheDistanceSq );
	void							ClearCache();
	const Comrades&					GetCachedMembers() const									{ return m_cachedMembers; }
	Float							GetCachedLeaderDistanceSq() const							{ return m_cachedLeaderDistanceSq; }

	void							FollowPoint( CFormationLeaderData* leader, Vector3& outFollowPoint ) override;
	Bool							FallingBehindTest( CFormationLeaderData* leader, Vector3& outFollowPoint ) override;
	void							PrepeareSteering( CFormationLeaderData* leader  ) override;
	CSteeringFormationMemberData*	AsSteeringFormationMemberData() override;
	void							InitializeMinCatchUpDistance( Float f ) override;
};



class CAISteeringFormationLeaderData : public CFormationLeaderData
{
	typedef CFormationLeaderData Super;
	DECLARE_RTTI_SIMPLE_CLASS( CAISteeringFormationLeaderData );
protected:
	Vector											m_centerOfMass;

	void							DoPrecomputation( Float ownerLocalTime  ) override;

public:
	CAISteeringFormationLeaderData()															{}

	void							GetCenterOfMass( Vector& outCenterOfMass ) const override;

};

BEGIN_NODEFAULT_CLASS_RTTI( CAISteeringFormationLeaderData )
END_CLASS_RTTI()