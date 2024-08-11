/*
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma once


class ISpellCastPosition;

enum EProjectileCastPosition
{
	PCP_Bone,
	PCP_EntityRoot
};

BEGIN_ENUM_RTTI( EProjectileCastPosition );
	ENUM_OPTION( PCP_Bone );
	ENUM_OPTION( PCP_EntityRoot );
END_ENUM_RTTI();

// An animation event that can spawn an entity template
class CExtAnimProjectileEvent : public CExtAnimEvent
{
	DECLARE_RTTI_SIMPLE_CLASS( CExtAnimProjectileEvent )

private:
	THandle< CEntityTemplate >		m_spell;
	EProjectileCastPosition			m_castPosition;
	CName							m_boneName;

public:
	// Constructor required by the serialization mechanism
	CExtAnimProjectileEvent();

	CExtAnimProjectileEvent( const CName& eventName,
		const CName& animationName, 
		Float startTime, 
		const String& trackName );

	virtual void Process( const CAnimationEventFired& info, CAnimatedComponent* component ) const;

private:
	Bool GetSituationFromBone( const CEntity& caster, Vector& outPosition, EulerAngles& outRotation ) const;
	Bool GetSituationFromEntity( const CEntity& caster, Vector& outPosition, EulerAngles& outRotation ) const;
};

BEGIN_CLASS_RTTI( CExtAnimProjectileEvent );
	PARENT_CLASS( CExtAnimEvent );
	PROPERTY_EDIT( m_spell, TXT( "A spell to cast" ) );
	PROPERTY_EDIT( m_castPosition, TXT( "How the spell should be cast" ) );
	PROPERTY_EDIT( m_boneName, TXT( "Name of the bone from which the spell will be cast" ) );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////
