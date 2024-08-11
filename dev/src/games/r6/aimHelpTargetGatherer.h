#pragma once

#include "../../common/game/itemEntity.h"
#include "../../common/game/binaryStorage.h"

class CAimTarget;

enum EAimHelpCheckTargets
{
	AHCT_Actors	,
	AHCT_Objects,
	AHCT_All
};

struct SAimHelpParams
{
	DECLARE_RTTI_STRUCT( SAimHelpParams );

	SAimHelpParams( Float nearDistance, Float nearAngle, Float farDistance, Float farAngle)
	{
		m_nearDistance	= nearDistance;
		m_nearAngle		= nearAngle;
		m_farDistance	= farDistance;
		m_farAngle		= farAngle;
	}

	SAimHelpParams()	:
		m_nearDistance(.0f)	,
		m_nearAngle(.0f)	,
		m_farDistance(.0f)	,		
		m_farAngle(.0f)		
	{
	}

	Float	m_nearDistance;
	Float	m_nearAngle;
	Float	m_farDistance;
	Float	m_farAngle;
};
BEGIN_CLASS_RTTI( SAimHelpParams )
	PROPERTY_EDIT_NAME( m_nearDistance	, TXT("i_NearDistanceF"), TXT("") );
	PROPERTY_EDIT_NAME( m_nearAngle		, TXT("i_NearAngleF")	, TXT("") );
	PROPERTY_EDIT_NAME( m_farDistance	, TXT("i_FarDistanceF")	, TXT("")  );
	PROPERTY_EDIT_NAME( m_farAngle		, TXT("i_FarAngleF")	, TXT("")  );
END_CLASS_RTTI();

enum EAimHelpLineOfSight
{
	AHLOS_NotSet	,
	AHLOS_No		,
	AHLOS_Yes
};

struct SPotentialAimHelpTarget
{
	Vector				m_direction;
	Float				m_distance;
	Float				m_angle;
	CGameplayEntity*	m_entity;
	EAimHelpLineOfSight	m_lineOfSight;

	SPotentialAimHelpTarget()
		: m_lineOfSight	( AHLOS_NotSet )
	{
	}

	SPotentialAimHelpTarget( CGameplayEntity* entity, Vector& direction, Float distance, Float angle)
	{
		m_entity		= entity;
		m_direction		= direction;
		m_distance		= distance;
		m_angle			= angle;
		m_lineOfSight	= AHLOS_NotSet;
	}

	RED_INLINE Bool operator<( const SPotentialAimHelpTarget& other) const
	{
		return m_angle < other.m_angle;
	}
};

// TODO: Not optimized at all, needs rethinking
class CAimHelpTargetsGatherer : public CComponent
{
	DECLARE_ENGINE_CLASS( CAimHelpTargetsGatherer, CComponent, 0 );

private:
	static const Uint32	AIM_MAX_ENTITIES;
	static const Uint32	AIM_MAX_LINES_OF_SIGHT;
	static const Float	AIM_AABB_EXTRA_MARGIN;

	SAimHelpParams	m_aimParams;
	Float			m_backDistanceForAngle;

	TDynArray< TPointerWrapper< CAimTarget > >			m_gameplayEntitiesInRange;
	TDynArray< SPotentialAimHelpTarget >				m_targetsInRange;
	Bool												m_areEntitiesSorted;


	void GatherValidEntitiesInBox( const Vector& origin, const Vector& intendedDirection );
	void SortGatheredEntities( const Vector& origin, const Vector& intendedDirection );

public:
	CAimHelpTargetsGatherer();

	void PreUpdate();

	void AddAimingHelpParams( SAimHelpParams& aimParams );

	CGameplayEntity* GetClosestValidEntity( const Vector& origin, const Vector& intendedDirection );
	CGameplayEntity* GetClosestValidEntityInParams( const Vector& origin, const Vector& intendedDirection, SAimHelpParams& aimParams );
	CGameplayEntity* GetClosestValidEntityInLineOfSight( const Vector& origin, const Vector& intendedDirection );


private:
	void funcInitialize( CScriptStackFrame& stack, void* result );
	void funcPreUpdate( CScriptStackFrame& stack, void* result );
	void funcAddAimingHelpParams( CScriptStackFrame& stack, void* result );	
	void funcGetClosestValidEntity( CScriptStackFrame& stack, void* result );	
	void funcGetClosestValidEntityInParams( CScriptStackFrame& stack, void* result );	
	void funcGetClosestValidEntityInSight( CScriptStackFrame& stack, void* result );	
};
BEGIN_CLASS_RTTI( CAimHelpTargetsGatherer );
	PARENT_CLASS( CComponent );
	NATIVE_FUNCTION( "PreUpdate", funcPreUpdate );
	NATIVE_FUNCTION( "AddAimingHelpParams", funcAddAimingHelpParams );
	NATIVE_FUNCTION( "GetClosestValidEntity", funcGetClosestValidEntity );
	NATIVE_FUNCTION( "GetClosestValidEntityInParams", funcGetClosestValidEntityInParams );
	NATIVE_FUNCTION( "GetClosestValidEntityInSight", funcGetClosestValidEntityInSight );
	PROPERTY_INLINED(m_aimParams, TXT( "Angles and distances" ));
	PROPERTY_EDIT(m_backDistanceForAngle, TXT( "back distance to start calculating the angle"));
END_CLASS_RTTI();