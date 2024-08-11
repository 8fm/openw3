#pragma once
#include "../../common/engine/entitytemplateparams.h"

class CMonsterParam : public CGameplayEntityParam
{
	DECLARE_ENGINE_CLASS( CMonsterParam, CGameplayEntityParam, 0 );

protected:
    Int32   m_monsterCategory;
    CName   m_soundMonsterName;
    Bool    m_isTeleporting;
    Bool    m_canBeTargeted;
	Bool	m_canBeHitByFists;
	Bool	m_canBeStrafed;

public:
    CMonsterParam() : m_monsterCategory(0), m_isTeleporting(false), m_canBeTargeted(true), m_canBeStrafed(true)
    {}

    Int32 GetMonsterCategory() const  { return m_monsterCategory;  }
    CName GetSoundMonsterName() const { return m_soundMonsterName; }
    Bool IsMonsterTeleporting() const { return m_isTeleporting; }
    Bool CanMonsterBeTargeted() const { return m_canBeTargeted; }
    Bool CanMonsterBeHitByFists() const { return m_canBeHitByFists; }
};

BEGIN_CLASS_RTTI( CMonsterParam );
	PARENT_CLASS( CGameplayEntityParam );
    PROPERTY_EDIT( m_isTeleporting, TXT( "Is monster teleporting" ) );
    PROPERTY_EDIT( m_canBeTargeted, TXT( "Can monster be targeted" ) );
    PROPERTY_EDIT( m_canBeHitByFists, TXT( "Can monster be hit by fists" ) );
	PROPERTY_EDIT( m_canBeStrafed, TXT("Can monster be strafed") )
	PROPERTY_CUSTOM_EDIT( m_monsterCategory, TXT( "Monster category" ), TXT("ScriptedEnum_EMonsterCategory") );
    PROPERTY_EDIT_NAME( m_soundMonsterName, TXT( "soundMonsterName" ), TXT( "Sound monster name" ) );
END_CLASS_RTTI();