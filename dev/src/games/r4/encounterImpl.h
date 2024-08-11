/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "../../common/game/encounter.h"
#include "../../common/game/spawnCondition.h"

//////////////////////////////////////////////////////////////////////////
//////////////// CONDITIONS
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////

class CPlayerLevelCondition : public ISpawnCondition
{
	DECLARE_RTTI_SIMPLE_CLASS( CPlayerLevelCondition );
protected:
	Int32 m_minLevel;
	Int32 m_maxLevel;
public:
	CPlayerLevelCondition()
		: m_minLevel( 0 )
		, m_maxLevel( 0 ) {}
	Bool Test( CSpawnTreeInstance& instance ) override;
};
BEGIN_CLASS_RTTI( CPlayerLevelCondition );
	PARENT_CLASS( ISpawnCondition )
	PROPERTY_EDIT( m_minLevel,			TXT("Player's minimum level to fulfill this condition") );
	PROPERTY_EDIT( m_maxLevel,			TXT("Player's maximum level to fulfill this condition") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CDaytimeCondition : public ISpawnCondition
{
	DECLARE_RTTI_SIMPLE_CLASS( CDaytimeCondition );
protected:
	GameTime					m_begin;
	GameTime					m_end;
public:
	CDaytimeCondition()
		: ISpawnCondition()
		, m_begin( 0 )
		, m_end( GameTime( 0, 12, 0, 0 ) )									{}

	Bool Test( CSpawnTreeInstance& instance ) override;

	Bool OnPropertyMissing( CName propertyName, const CVariant& readValue ) override;
};
BEGIN_CLASS_RTTI( CDaytimeCondition );
	PARENT_CLASS( ISpawnCondition )
	PROPERTY_CUSTOM_EDIT( m_begin, TXT( "Starting daytime" ), TXT( "DayTimeEditor" ) );
	PROPERTY_CUSTOM_EDIT( m_end, TXT( "Ending daytime" ), TXT( "DayTimeEditor" ) );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CSpawnConditionFact : public ISpawnCondition
{
	DECLARE_RTTI_SIMPLE_CLASS( CSpawnConditionFact );
protected:
	String			m_fact;
	Int32			m_value;
	ECompareFunc	m_compare;
public:
	Bool Test( CSpawnTreeInstance& instance ) override;
};
BEGIN_CLASS_RTTI( CSpawnConditionFact );
	PARENT_CLASS( ISpawnCondition );
	PROPERTY_EDIT( m_fact, TXT("Fact name") );
	PROPERTY_EDIT( m_value, TXT("Fact value") );
	PROPERTY_EDIT( m_compare, TXT("Comparison function") );
END_CLASS_RTTI();