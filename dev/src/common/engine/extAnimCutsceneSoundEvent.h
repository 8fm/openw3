/*
 * Copyright © 2011 CD Projekt Red. All Rights Reserved.
 */

#pragma once
#include "extAnimEvent.h"

class CExtAnimCutsceneSoundEvent : public CExtAnimEvent
{
	DECLARE_RTTI_SIMPLE_CLASS( CExtAnimCutsceneSoundEvent )

public:
	CExtAnimCutsceneSoundEvent();

	CExtAnimCutsceneSoundEvent( const CName& eventName,	const CName& animationName, Float startTime, const String& trackName );
	virtual ~CExtAnimCutsceneSoundEvent();

	template < typename T >
	Bool PlaySound( T* component )
	{
		return component ? PlaySound( component, component->FindBoneByName( m_bone ) ) : PlaySound();
	}

	Bool PlaySound( CComponent* animatedComponent = NULL, Int32 bone = -1 ) const;

	RED_INLINE const StringAnsi& GetSoundEventName() const { return m_soundEventName; }

	RED_INLINE const CName& GetBone() const
	{ return m_bone; }

	RED_INLINE void SetBoneName( const CName& name )
	{ m_bone = name; }

	RED_INLINE Bool GetUseMaterialInfo() const
	{ return m_useMaterialInfo;	}

private:
	StringAnsi	m_soundEventName;
	CName		m_bone;
	Bool		m_useMaterialInfo;
};

BEGIN_CLASS_RTTI( CExtAnimCutsceneSoundEvent )
	PARENT_CLASS( CExtAnimEvent )
	PROPERTY_CUSTOM_EDIT( m_soundEventName, TXT( "Sound event name" ), TXT( "AudioEventBrowser" ) )
	PROPERTY_CUSTOM_EDIT( m_bone, TXT( "Bone on which sound is fired" ), TXT( "EntityBoneList" ) )
	PROPERTY_EDIT( m_useMaterialInfo, TXT( "Should use material information from actor" ) )
END_CLASS_RTTI()
