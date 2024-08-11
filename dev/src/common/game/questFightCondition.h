#pragma once

#include "questCondition.h"
#include "../../common/engine/globalEventsManager.h"

///////////////////////////////////////////////////////////////////////////////

enum EQueryFightMode
{
	QFM_Killed,
	QFM_Stunned,
	QFM_Hit_By_Aard,
	QFM_Hit_By_Igni,
	QFM_Hit_By_Yrden,
	QFM_Hit,
	QFM_Hit_By_Bomb,
	QFM_Hit_By_Bolt,
	QFM_Hit_By_Axii,
	QFM_KnockedUnconscious,
};

BEGIN_ENUM_RTTI( EQueryFightMode );
	ENUM_OPTION( QFM_Killed );
	ENUM_OPTION( QFM_Stunned );
	ENUM_OPTION( QFM_Hit_By_Aard );
	ENUM_OPTION( QFM_Hit_By_Igni );
	ENUM_OPTION( QFM_Hit_By_Yrden );
	ENUM_OPTION( QFM_Hit );
	ENUM_OPTION( QFM_Hit_By_Bomb );
	ENUM_OPTION( QFM_Hit_By_Bolt );
	ENUM_OPTION( QFM_Hit_By_Axii );
	ENUM_OPTION( QFM_KnockedUnconscious );
END_ENUM_RTTI();

///////////////////////////////////////////////////////////////////////////////

class CQuestFightCondition : public IQuestCondition, public IGlobalEventsListener
{
	DECLARE_ENGINE_CLASS( CQuestFightCondition, IQuestCondition, 0 )

private:
	CName				m_tag;
	CName				m_attackerTag;
	Int32				m_referenceValue;
	EQueryFightMode		m_damageMode;

	String				m_factId;
	Bool				m_isFulfilled;
	Bool				m_wasRegistered;

protected:
	CQuestFightCondition();
	virtual ~CQuestFightCondition();

	//! IQuestCondition implementation
	virtual void OnActivate() override;
	virtual void OnDeactivate() override;
	virtual Bool OnIsFulfilled() override;

	Bool RegisterCallback( Bool reg );
	void CreateHitFact( const Char* hitType, String& hitFact);
	Bool CreateFactId();
	void QueryFact();
	void HandleFactIdCreationFailure() const;

	//! IGlobalEventsListener
	virtual void OnGlobalEvent( EGlobalEventCategory eventCategory, EGlobalEventType eventType, const SGlobalEventParam& param ) override;

public:
#ifndef NO_EDITOR_PROPERTY_SUPPORT
	virtual String	GetDescription() const 
	{ 
		return String::Printf( TXT( "Victim: %s, Attacker: %s, Ref Value: %d, Fight Mode: %s" ),
			m_tag.AsChar(), m_attackerTag.AsChar(), m_referenceValue, CEnum::ToString( m_damageMode ).AsChar() ); 
	}
#endif
};
BEGIN_CLASS_RTTI( CQuestFightCondition )
	PARENT_CLASS( IQuestCondition )
	PROPERTY_EDIT( m_tag, TXT("Victim tag.") )
	PROPERTY_EDIT( m_attackerTag, TXT("Attacker tag (Valid only for 'hit' and 'killed' checks).") )
	PROPERTY_EDIT( m_referenceValue, TXT("Value we want to compare to.") )
	PROPERTY_EDIT( m_damageMode, TXT("Type of damage for which we want to check the statistics.") )
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////

class CQuestInCombatCondition : public IQuestCondition
{
	DECLARE_ENGINE_CLASS( CQuestInCombatCondition, IQuestCondition, 0 )

private:
	Bool	m_isInCombat;

public:
	CQuestInCombatCondition();

protected:
	//! IQuestCondition implementation
	virtual Bool OnIsFulfilled();

public:
#ifndef NO_EDITOR_PROPERTY_SUPPORT
	virtual String	GetDescription() const { return TXT( "InCombat" ); }
#endif
};

BEGIN_CLASS_RTTI( CQuestInCombatCondition )
	PARENT_CLASS( IQuestCondition )
	PROPERTY_EDIT( m_isInCombat, TXT("Flag whether player is in combat") )
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////
