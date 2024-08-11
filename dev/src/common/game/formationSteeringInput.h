/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

class CFormationMemberData;
class CFormationLeaderData;


struct SFormationSteeringInput
{
	DECLARE_RTTI_STRUCT( SFormationSteeringInput );

	CFormationMemberData*				m_memberData;
	CFormationLeaderData*				m_leaderData;

	// this interface is more conveniant we will switch to it gradually
	CFormationMemberData*				MemberData() const															{ return m_memberData; }
	CFormationLeaderData*				LeaderData() const															{ return m_leaderData; }

	static void							SetGeneralFormationData( SMoveLocomotionGoal& goal, SFormationSteeringInput* formationData );
	static SFormationSteeringInput*		GetGeneralFormationData( const SMoveLocomotionGoal& goal );
	static CName						GetGoalCustomParameterName()												{ return CNAME( Formation ); }
};

BEGIN_NODEFAULT_CLASS_RTTI( SFormationSteeringInput )
END_CLASS_RTTI()



