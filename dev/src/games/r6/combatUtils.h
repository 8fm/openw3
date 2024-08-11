#pragma once

#include "build.h"

class CTeamManager;

struct SRestrictionArea
{
	DECLARE_RTTI_STRUCT( SRestrictionArea );


	Vector	m_center;
	Vector	m_forwardDirection;
	Float	m_forwardLength;
	Float	m_sideLength;

	Bool IsInside( const Vector& testedPosition );
};

BEGIN_CLASS_RTTI( SRestrictionArea )
	PROPERTY_NAME( m_center				, TXT("i_center")				);	
	PROPERTY_NAME( m_forwardDirection	, TXT("i_forwardDirection")		);	
	PROPERTY_NAME( m_forwardLength		, TXT("i_forwardLength")		);
	PROPERTY_NAME( m_sideLength			, TXT("i_sideLength")			);		
END_CLASS_RTTI();

class CCombatUtils : public CObject
{
	static const Float	FIND_FLANKING_ANGLE_STEP;

	DECLARE_ENGINE_CLASS( CCombatUtils, CObject, 0 );

private:
	CPhysicsEngine::CollisionMask	m_LOScollisionMask;
	CPhysicsWorld*						m_physicsWorld;	
	CTeamManager*						m_teamManager;

public:
	RED_INLINE CTeamManager * GetTeamManager()	{ return m_teamManager; }

	void Initialize();	

private:	
	void RotateVector( Vector& toRotate, Float angle );
	Bool TestLOS( const Vector& startPosition, const Vector& endPosition ); //returns true if test pass - no obstacles
	Vector ClampToArea( const SRestrictionArea& restrictionArea, const Vector& sourcePosition );

private:
	void funcFindFlankingPosition	( CScriptStackFrame& stack, void* result );
	void funcGetTeamManager			( CScriptStackFrame& stack, void* result );
	void funcGetNearestOnNavMesh	( CScriptStackFrame& stack, void* result );
	void funcTestLOS				( CScriptStackFrame& stack, void* result );
	void funcClampToArea			( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CCombatUtils )
	PARENT_CLASS( CObject );
	PROPERTY( m_teamManager );
	NATIVE_FUNCTION( "I_FindFlankingPosition"	, funcFindFlankingPosition		);
	NATIVE_FUNCTION( "I_GetTeamManager"			, funcGetTeamManager			);
	NATIVE_FUNCTION( "I_GetNearestOnNavMesh"	, funcGetNearestOnNavMesh		);
	NATIVE_FUNCTION( "I_TestLOS"				, funcTestLOS					);
	NATIVE_FUNCTION( "I_ClampToArea"			, funcClampToArea				);
END_CLASS_RTTI();