
#pragma once

#include "storySceneEvent.h"

class CStorySceneEventHitSound : public CStorySceneEvent
{
	DECLARE_SCENE_EVENT_CLASS( CStorySceneEventHitSound, CStorySceneEvent )

protected:
	CName		m_soundAttackType;
	CName		m_actor;
	CName		m_actorAttacker;
	CName		m_actorAttackerWeaponSlot;
	CName		m_actorAttackerWeaponName;

public:
	CStorySceneEventHitSound();
	CStorySceneEventHitSound( const String& eventName, CStorySceneElement* sceneElement, Float startTime, const CName& actor, const String& trackName );

	// compiler generated cctor is ok

	virtual CStorySceneEventHitSound* Clone() const override;

public:
	virtual void OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;
};

BEGIN_CLASS_RTTI( CStorySceneEventHitSound )
	PARENT_CLASS( CStorySceneEvent )
	PROPERTY_EDIT( m_actor, TXT( "Actor victim" ) )
	PROPERTY_EDIT( m_actorAttacker, TXT( "Actor opponent" ) ) // This isn't used anymore - leaving it in, just in case the data is needed in the future.
	PROPERTY_EDIT( m_soundAttackType, TXT( "Wwise switch opponent_attack_type" ) )
	PROPERTY_EDIT( m_actorAttackerWeaponSlot, TXT( "Wwise switch opponent_weapon_type" ) )
	PROPERTY_EDIT( m_actorAttackerWeaponName, TXT( "Wwise switch opponent_weapon_size" ) )
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////

