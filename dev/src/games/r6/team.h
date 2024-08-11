#pragma once

#include "build.h"

#include "teamMemberComponent.h"

class CTeamSharedKnowladge;

struct SCombatLine
{
	DECLARE_RTTI_STRUCT( SCombatLine );

	SCombatLine()
		: m_position		( 0, 0, 0 )
		, m_normalDirection	( 0, 0, 0 )
		, m_lineDirection	( 0, 0, 0 )
		, m_teamCenter		( 0, 0, 0 )
		, m_enemiesCenter	( 0, 0, 0 )
		, m_isValid			( false   )
	{

	}

	Vector	m_position;
	Vector	m_normalDirection;
	Vector	m_lineDirection;
	Vector	m_teamCenter;
	Vector	m_enemiesCenter;
	Bool	m_isValid;
};

BEGIN_CLASS_RTTI( SCombatLine )
	PROPERTY_NAME( m_position		, TXT("i_position")			);	
	PROPERTY_NAME( m_normalDirection, TXT("i_normalDirection")	);
	PROPERTY_NAME( m_lineDirection	, TXT("i_lineDirection")	);
	PROPERTY_NAME( m_teamCenter		, TXT("i_teamCenter")		);
	PROPERTY_NAME( m_enemiesCenter	, TXT("i_enemiesCenter")	);	
	PROPERTY_NAME( m_isValid		, TXT("i_isValid")			);	
END_CLASS_RTTI();

struct SCombatAlley
{
	DECLARE_RTTI_STRUCT( SCombatAlley );

	SCombatLine	m_usedCombatLine;

	Vector		m_positiveEdgePosition;
	Vector		m_negativeEdgePosition;

	Float		m_positiveWidth;
	Float		m_negativeWidth;	

	Bool		m_isValid;
};

BEGIN_CLASS_RTTI( SCombatAlley )
	PROPERTY_NAME( m_usedCombatLine			, TXT("i_usedCombatLine")		);	
	PROPERTY_NAME( m_positiveEdgePosition	, TXT("i_positiveEdgePosition")	);
	PROPERTY_NAME( m_negativeEdgePosition	, TXT("i_negativeEdgePosition")	);
	PROPERTY_NAME( m_positiveWidth			, TXT("i_positiveWidth")		);
	PROPERTY_NAME( m_negativeWidth			, TXT("i_negativeWidth")		);	
	PROPERTY_NAME( m_isValid				, TXT("i_isValid")				);	
END_CLASS_RTTI();

class CTeam : public CObject
{
	DECLARE_ENGINE_CLASS( CTeam, CObject, 0 );
	
	static const Float COMBAT_ALLEY_MARGIN_PERCENTAGE;
	static const Float COMBAT_LINE_LENGHT;

private:
	TDynArray< THandle< CTeamMemberComponent > >	m_members;
	CName											m_teamName;
	Int32												m_movementTicketsLeft;
	CTeamSharedKnowladge*							m_teamSharedKnowladge;	

private:
	void CleanupNullMembers();

public:

	CTeam() : m_movementTicketsLeft( 3 ){}

	RED_INLINE void SetTeamName( CName teamName ){ m_teamName = teamName; }
	RED_INLINE const TDynArray< THandle< CTeamMemberComponent > >& GetMembers(){ return m_members; }
	RED_INLINE TDynArray< THandle< CTeamMemberComponent > >& GetMembersNoConst(){ return m_members; }

	void Initialize();

	void AddMember		( CTeamMemberComponent* teamMember );
	void RemoveMember	( CTeamMemberComponent* teamMember );

	SCombatLine CalculateCombatLine( Float linePosition, Int32 subteamFlag = 0 );
	Vector	ProjectPointOnCombatLine( const SCombatLine& combatLine, const Vector& projectedPoint, Bool onNavMesh );

	SCombatAlley CalculateCombatAlley( Float minMargin );
	SCombatAlley CalculateCombatAlleyBasedOnCombatLine( const SCombatLine& combatLine, Float minMargin );
	Vector FindNearesrPointOutsideCombatAlley( const SCombatAlley& combatAlley, const Vector& sourcePosition, Bool skipIfOutside );

	Bool AcquireMovementTicket( CTeamMemberComponent* member );
	void ReturnMovementTicket( CTeamMemberComponent* member );

	void Tick( Float timeDelta );

private:
	void funcCalculateCombatLine					( CScriptStackFrame& stack, void* result );
	void funcCalculateCombatLineByDistance			( CScriptStackFrame& stack, void* result );
	void funcCalculateCombatLineFlag				( CScriptStackFrame& stack, void* result );
	void funcCalculateCombatLineByDistanceFlag		( CScriptStackFrame& stack, void* result );
	void funcProjectPointOnCombatLine				( CScriptStackFrame& stack, void* result );
	void funcCalculateCombatAlley					( CScriptStackFrame& stack, void* result );
	void funcCalculateCombatAlleyBasedOnCombatLine	( CScriptStackFrame& stack, void* result );
	void funcFindNearestPointOutsideCombatAlley		( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CTeam );
	PARENT_CLASS( CObject );	
	PROPERTY_NAME( m_members,	TXT("i_members")	);
	PROPERTY_NAME( m_teamName,	TXT("i_teamName")	);
	PROPERTY( m_teamSharedKnowladge );
	PROPERTY_NAME( m_movementTicketsLeft, TXT("i_movementTicketsLeft") );

	NATIVE_FUNCTION( "I_CalculateCombatLine"					, funcCalculateCombatLine						);
	NATIVE_FUNCTION( "I_CalculateCombatLineByDistance"			, funcCalculateCombatLineByDistance				);
	NATIVE_FUNCTION( "I_CalculateCombatLineFlag"				, funcCalculateCombatLineFlag						);
	NATIVE_FUNCTION( "I_CalculateCombatLineByDistanceFlag"		, funcCalculateCombatLineByDistanceFlag		);

	NATIVE_FUNCTION( "I_ProjectPointOnCombatLine"				, funcProjectPointOnCombatLine					);
	NATIVE_FUNCTION( "I_CalculateCombatAlley"					, funcCalculateCombatAlley						);
	NATIVE_FUNCTION( "I_CalculateCombatAlleyBasedOnCombatLine"	, funcCalculateCombatAlleyBasedOnCombatLine		);
	NATIVE_FUNCTION( "I_FindNearestPointOutsideCombatAlley"		, funcFindNearestPointOutsideCombatAlley		);
END_CLASS_RTTI();