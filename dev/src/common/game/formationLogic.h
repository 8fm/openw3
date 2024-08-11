/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

class CFormationLeaderData;
class CFormationMemberData;
class CAIFormationDataPtr;

class IFormationLogic : public CObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IFormationLogic, CObject )
protected:
	Float							m_recomputionDelay;
	Float							m_minCatchupDistance;

public:
	// this whole interface is accessible on object initialization
	virtual CClass*					GetFormationLeaderType() const;
	virtual void					SetupLeaderData( CFormationLeaderData* leaderData, CActor* leader ) const;
	virtual CFormationMemberData*	SpawnMemberData( CActor* actor ) const;
	virtual void					SetupMemberData( CFormationMemberData* memberData ) const;
public:
	IFormationLogic()
		: m_recomputionDelay( 0.1f )
		, m_minCatchupDistance( 2.f )															{}

	CFormationMemberData*			CreateMemberData( CActor* actor ) const;

	Float							GetRecomputationDelay() const								{ return m_recomputionDelay; }

	CFormation*						GetFormation() const;
};

BEGIN_ABSTRACT_CLASS_RTTI( IFormationLogic )
	PARENT_CLASS( CObject )
	PROPERTY_EDIT( m_recomputionDelay, TXT("Formation update delays") )
	PROPERTY_EDIT( m_minCatchupDistance, TXT("Min distance on which member can run 'catchup' behavior that involves pathfinding") )
END_CLASS_RTTI()

