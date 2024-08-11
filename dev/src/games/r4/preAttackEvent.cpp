#include "build.h"
#include "preAttackEvent.h"


IMPLEMENT_ENGINE_CLASS( CPreAttackEvent );
IMPLEMENT_ENGINE_CLASS( CPreAttackEventData );

IMPLEMENT_RTTI_BITFIELD( EPreAttackFlags );


RED_DEFINE_STATIC_NAME( OnPreAttackEvent );
RED_DEFINE_STATIC_NAME( attack_light );


///////////////////////////////////////////////////////////////////////////////////////////////////////////

CPreAttackEvent::CPreAttackEvent()
	: CExtAnimDurationEvent() 
{
	m_reportToScript = false;
	m_alwaysFiresEnd = true;
}

CPreAttackEvent::CPreAttackEvent( const CName& eventName, 
	const CName& animationName, 
	Float startTime, 
	Float duration, 
	const String& trackName )
	: CExtAnimDurationEvent( eventName, animationName, startTime, duration, trackName )
{
	m_alwaysFiresEnd = true;
	m_reportToScript = false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////

void CPreAttackEvent::Start( const CAnimationEventFired& info, CAnimatedComponent* component ) const
{
	CGameplayEntity* ent = Cast< CGameplayEntity >( component->GetEntity() );
	if ( ent != nullptr )
	{
		THandle< CGameplayEntity > hEntity( ent );
 		GCommonGame->CallEvent( CNAME( OnPreAttackEvent ), hEntity, m_eventName, AET_DurationStart, m_data, info.m_animInfo );
	}

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////

void CPreAttackEvent::Stop( const CAnimationEventFired& info, CAnimatedComponent* component ) const
{
	CGameplayEntity* ent = Cast< CGameplayEntity >( component->GetEntity() );
	if ( ent != nullptr )
	{
		THandle< CGameplayEntity > hEntity( ent );
		GCommonGame->CallEvent( CNAME( OnPreAttackEvent ), hEntity, m_eventName, AET_DurationEnd, m_data, info.m_animInfo );
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////

CPreAttackEventData::CPreAttackEventData()
	: m_flags( ETF_Damage_Hostile | ETF_Can_Parry_Attack | ETF_Damage_Neutral )
	, m_Damage_Friendly(false)
	, m_Damage_Neutral(true)
	, m_Damage_Hostile(true)
	, m_Can_Parry_Attack(true)	
	, m_attackName( CNAME(attack_light) )
	, m_weaponSlot( CNAME(r_weapon) )
	, m_rangeName( CName::NONE )
	, m_hitReactionType( 1 )
	, m_canBeDodged( true )
	, m_cameraAnimOnMissedHit( CName::NONE )
{}
