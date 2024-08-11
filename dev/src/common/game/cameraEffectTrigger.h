/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

enum EEffectEntityPosition
{
	EEP_Camera,
	EEP_Player,
	EEP_XYCameraZPlayer,
	EEP_XYPlayerZCamera,
	EEP_XYCameraZTerrain,
	EEP_XYPlayerZTerrain
};

BEGIN_ENUM_RTTI( EEffectEntityPosition );
	ENUM_OPTION( EEP_Camera );
	ENUM_OPTION( EEP_Player );
	ENUM_OPTION( EEP_XYCameraZPlayer );
	ENUM_OPTION( EEP_XYPlayerZCamera );
	ENUM_OPTION( EEP_XYCameraZTerrain );
	ENUM_OPTION( EEP_XYPlayerZTerrain );
END_ENUM_RTTI();

enum EEffectEntityRotation
{
	EER_None,
	EER_Spawn,
	EER_Continuous
};

BEGIN_ENUM_RTTI( EEffectEntityRotation );
	ENUM_OPTION( EER_None );
	ENUM_OPTION( EER_Spawn );
	ENUM_OPTION( EER_Continuous );
END_ENUM_RTTI();

////////////////////////////////////////////////////////////////////////////////////////////////////

class CCameraEffectTrigger : public CGameplayEntity
{
	DECLARE_ENGINE_CLASS( CCameraEffectTrigger, CGameplayEntity, 0 );

protected:
	CName					m_effectName;
	Bool					m_isPlayingEffect;
	Bool					m_useSharedEffects;
	Bool					m_isTriggerActivated;
	Bool					m_isTicked;
	EEffectEntityPosition	m_effectEntityPosition;
	EEffectEntityRotation	m_effectEntityRotation;
	Vector3					m_effectEntityOffset;
	THandle< CEntity >		m_effectsEntity;
	GameTime				m_playFrom;
	GameTime				m_playTo;

public:
	CCameraEffectTrigger();

	// Entity was attached from world
	virtual void OnAttached( CWorld* world );

	// Entity was detached from world
	virtual void OnDetached( CWorld* world );

	// Something has entered trigger area owned by this entity
	virtual void OnAreaEnter( CTriggerAreaComponent* area, CComponent* activator );

	// Something has exited trigger area owned by this entity
	virtual void OnAreaExit( CTriggerAreaComponent* area, CComponent* activator );

	// Tick notification
	virtual void OnTick( Float timeDelta );

protected:
	Vector CalcEntityPosition() const;
	void RotateEntityForward();
	Bool CanPlayEffect() const;
	Bool IsAffectedByDayTime() const;
	Bool CheckDayTime() const;
	Bool ShouldBeTicked() const;
	void RegisterForTick( Bool reg );

	void StartEffect();
	void StopEffect();
};

////////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_CLASS_RTTI( CCameraEffectTrigger );
	PARENT_CLASS( CGameplayEntity );
	PROPERTY_SAVED( m_isPlayingEffect );
	PROPERTY_EDIT( m_effectName,			TXT( "Name of the camera effect to play" ) );
	PROPERTY_EDIT( m_useSharedEffects,		TXT( "Use separate shared effects entity that follows the camera" ) );
	PROPERTY_EDIT( m_effectEntityPosition,	TXT( "Shared effects entity position" ) );
	PROPERTY_EDIT( m_effectEntityRotation,	TXT( "Shared effects entity rotation type" ) );
	PROPERTY_EDIT( m_effectEntityOffset,	TXT( "Shared effects entity position offset" ) );
	PROPERTY_CUSTOM_EDIT( m_playFrom,		TXT( "Play effect from selected day time (if both playFrom and playTo are set to 0:00:00 effect will be played all day long" ), TXT( "DayTimeEditor" ) );
	PROPERTY_CUSTOM_EDIT( m_playTo,			TXT( "Play effect to selected day time (if both playFrom and playTo are set to 0:00:00 effect will be played all day long" ), TXT( "DayTimeEditor" ) );
END_CLASS_RTTI();

////////////////////////////////////////////////////////////////////////////////////////////////////
