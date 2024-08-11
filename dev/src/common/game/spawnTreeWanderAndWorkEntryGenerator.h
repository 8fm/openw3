#pragma once

#include "encounterTypes.h"
#include "spawnTreeNode.h"
#include "spawnTreeBaseEntryGenerator.h"

struct SWorkWanderSmartAIEntryGeneratorParam
{
	DECLARE_RTTI_STRUCT( SWorkWanderSmartAIEntryGeneratorParam )
	TagList	m_apTag;
	TagList m_areaTags;
	CName	m_apAreaTag;
};
BEGIN_CLASS_RTTI( SWorkWanderSmartAIEntryGeneratorParam )
	PROPERTY_EDIT( m_apTag, TXT("") );
	PROPERTY_EDIT( m_areaTags, TXT("") );
	PROPERTY_EDIT( m_apAreaTag, TXT("") );
END_CLASS_RTTI()

struct SWanderHistoryEntryGeneratorParams
{
	DECLARE_RTTI_STRUCT( SWanderHistoryEntryGeneratorParams )
	CName m_wanderPointsGroupTag;
};
BEGIN_CLASS_RTTI( SWanderHistoryEntryGeneratorParams )
	PROPERTY_EDIT( m_wanderPointsGroupTag, TXT("") );
END_CLASS_RTTI()

struct SWanderAndWorkEntryGeneratorParams
{
	DECLARE_RTTI_STRUCT( SWanderAndWorkEntryGeneratorParams )
	SCreatureEntryEntryGeneratorNodeParam	m_creatureEntry;
	SWanderHistoryEntryGeneratorParams		m_wander;
	SWorkWanderSmartAIEntryGeneratorParam	m_work;
};

BEGIN_CLASS_RTTI( SWanderAndWorkEntryGeneratorParams )
	PROPERTY_EDIT( m_creatureEntry, TXT("") );
PROPERTY_EDIT( m_wander, TXT("") );
PROPERTY_EDIT( m_work, TXT("") );
END_CLASS_RTTI()

class CWanderAndWorkEntryGenerator : public CSpawnTreeBaseEntryGenerator
{
	DECLARE_ENGINE_CLASS( CWanderAndWorkEntryGenerator, CSpawnTreeBaseEntryGenerator, 0 );

private:
	TDynArray< SWanderAndWorkEntryGeneratorParams > m_entries;

public:
	void OnPropertyPostChange( IProperty* property ) override;
};

BEGIN_CLASS_RTTI( CWanderAndWorkEntryGenerator )
	PARENT_CLASS( CSpawnTreeBaseEntryGenerator )
	PROPERTY_EDIT( m_entries, TXT("") );
END_CLASS_RTTI()

struct SWorkSmartAIEntryGeneratorNodeParam
{
	DECLARE_RTTI_STRUCT( SWorkSmartAIEntryGeneratorNodeParam )
	TagList	m_apTag;
	TagList m_areaTags;
	CName	m_apAreaTag;
	Bool	m_keepActionPointOnceSelected;
	EMoveType m_actionPointMoveType;
};
BEGIN_CLASS_RTTI( SWorkSmartAIEntryGeneratorNodeParam )
	PROPERTY_EDIT( m_apTag, TXT("") );
PROPERTY_EDIT( m_areaTags, TXT("") );
PROPERTY_EDIT( m_apAreaTag, TXT("") );
PROPERTY_EDIT( m_keepActionPointOnceSelected, TXT("") );
PROPERTY_EDIT( m_actionPointMoveType, TXT("") );
END_CLASS_RTTI()

struct SWorkEntryGeneratorParam
{
	DECLARE_RTTI_STRUCT( SWorkEntryGeneratorParam )
	SCreatureEntryEntryGeneratorNodeParam	m_creatureEntry;
	SWorkSmartAIEntryGeneratorNodeParam		m_work;
};
BEGIN_CLASS_RTTI( SWorkEntryGeneratorParam )
	PROPERTY_EDIT( m_creatureEntry, TXT("") );
	PROPERTY_EDIT( m_work, TXT("") );
END_CLASS_RTTI()


class CWorkEntryGenerator  : public CSpawnTreeBaseEntryGenerator
{
	DECLARE_ENGINE_CLASS( CWorkEntryGenerator, CSpawnTreeBaseEntryGenerator, 0 );

private:
	TDynArray< SWorkEntryGeneratorParam > m_entries;

public:
	void OnPropertyPostChange( IProperty* property ) override;
};

BEGIN_CLASS_RTTI( CWorkEntryGenerator )
	PARENT_CLASS( CSpawnTreeBaseEntryGenerator )
	PROPERTY_EDIT( m_entries, TXT("") );
END_CLASS_RTTI()