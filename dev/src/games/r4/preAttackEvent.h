#pragma once
#include "..\..\common\engine\extanimdurationevent.h"

enum EPreAttackFlags
{
	ETF_Damage_Friendly		= FLAG( 0 ),
	ETF_Damage_Neutral		= FLAG( 1 ),
	ETF_Damage_Hostile		= FLAG( 2 ),
	ETF_Can_Parry_Attack	= FLAG( 3 ),
};


BEGIN_BITFIELD_RTTI( EPreAttackFlags, 1 );
	BITFIELD_OPTION( ETF_Damage_Friendly );
	BITFIELD_OPTION( ETF_Damage_Neutral );
	BITFIELD_OPTION( ETF_Damage_Hostile );
	BITFIELD_OPTION( ETF_Can_Parry_Attack );
END_BITFIELD_RTTI();

class CPreAttackEventData
{
	DECLARE_RTTI_SIMPLE_CLASS( CPreAttackEventData )

	CPreAttackEventData();

public:		
	CName					m_attackName;
	CName					m_weaponSlot;
	CName					m_rangeName;
	Int32						m_hitReactionType;
	Int32						m_flags;
	Int32						m_swingDir;	
	Int32						m_swingType;
	
	CName					m_hitFX;
	CName					m_hitBackFX;
	CName					m_hitParriedFX;
	CName					m_hitBackParriedFX;

	//temp instead of flags while flags doesnt work in script 
	Bool					m_Damage_Friendly	;	
	Bool					m_Damage_Neutral	;	
	Bool					m_Damage_Hostile	;
	Bool					m_Can_Parry_Attack	;
	CName					m_soundAttackType;

	Bool					m_canBeDodged;
	CName					m_cameraAnimOnMissedHit;
};

BEGIN_CLASS_RTTI( CPreAttackEventData );

	PROPERTY_EDIT( m_attackName, TXT("name of the attack event") );
	PROPERTY_EDIT( m_weaponSlot, TXT("which weapon slot to use during the attack to deal damage and buffs") );
	PROPERTY_CUSTOM_EDIT( m_hitReactionType, TXT("standard hit reaction for this attack"), TXT("ScriptedEnum_EHitReactionType") );
	PROPERTY_EDIT( m_rangeName, TXT( "Attack range name" ) );
	
	//TEMP bools instead of flags
	//PROPERTY_BITFIELD_EDIT( m_flags, EPreAttackFlags, TXT("Various options"))
	PROPERTY_EDIT( m_Damage_Friendly,	TXT("") );
	PROPERTY_EDIT( m_Damage_Neutral,	TXT("") );
	PROPERTY_EDIT( m_Damage_Hostile,	TXT("") );
	PROPERTY_EDIT( m_Can_Parry_Attack,	TXT("") );

	PROPERTY_EDIT( m_hitFX,	TXT("") );
	PROPERTY_EDIT( m_hitBackFX,	TXT("") );
	PROPERTY_EDIT( m_hitParriedFX,	TXT("") );
	PROPERTY_EDIT( m_hitBackParriedFX,	TXT("") );

	PROPERTY_CUSTOM_EDIT( m_swingType , TXT("param for parry"), TXT("ScriptedEnum_EAttackSwingType") );
	PROPERTY_CUSTOM_EDIT( m_swingDir  , TXT("param for parry"), TXT("ScriptedEnum_EAttackSwingDirection") );

	PROPERTY_EDIT( m_soundAttackType,	TXT("") );

	PROPERTY_EDIT( m_canBeDodged, String::EMPTY );
	PROPERTY_EDIT( m_cameraAnimOnMissedHit, String::EMPTY );

END_CLASS_RTTI();


class CPreAttackEvent : public CExtAnimDurationEvent
{
	DECLARE_RTTI_SIMPLE_CLASS( CPreAttackEvent )
public:

	CPreAttackEvent();
	CPreAttackEvent( const CName& eventName, const CName& animationName, Float startTime, Float duration, const String& trackName );

	void Start( const CAnimationEventFired& info, CAnimatedComponent* component ) const;
	void Stop( const CAnimationEventFired& info, CAnimatedComponent* component ) const;

	CPreAttackEventData m_data;
};

BEGIN_CLASS_RTTI( CPreAttackEvent );
	PARENT_CLASS( CExtAnimDurationEvent );
	PROPERTY_INLINED( m_data, TXT("event data") );
END_CLASS_RTTI();

