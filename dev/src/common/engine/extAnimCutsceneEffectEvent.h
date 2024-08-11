/*
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "extAnimDurationEvent.h"
#include "extAnimCutsceneEvent.h"
#include "entityTemplate.h"

class CExtAnimCutsceneEffectEvent : public CExtAnimDurationEvent
{
	DECLARE_RTTI_SIMPLE_CLASS( CExtAnimCutsceneEffectEvent )

public:
	CExtAnimCutsceneEffectEvent();

	CExtAnimCutsceneEffectEvent( const CName& eventName,
		const CName& animationName, Float startTime, Float duration, const String& trackName
		);
	virtual ~CExtAnimCutsceneEffectEvent();

	RED_INLINE CEntityTemplate* LoadAndGetEntityTemplate() const
	{ return m_template.Get(); }

	RED_INLINE TSoftHandle< CEntityTemplate > GetEntityTemplateHandle() const
	{ return m_template; }

	RED_INLINE void UnloadEntityTemplate() const
	{ m_template.Release(); }

	RED_INLINE const Vector& GetSpawnPos() const
	{ return m_spawnPosMS; }

	RED_INLINE const EulerAngles&	GetSpawnRot() const
	{ return m_spawnRotMS; }

	RED_INLINE const CName& GetEffectName() const
	{ return m_effect; }

	RED_INLINE void SetSpawnPositon( const Vector& posMS )
	{ m_spawnPosMS = posMS; }

	RED_INLINE void SetSpawnRotation( const EulerAngles& rotMS )
	{ m_spawnRotMS = rotMS; }

	// This event doesn't have Process() method, because all processing
	// is done in Cutscene Instance

protected:
	CName								m_effect;
	TagList								m_tag;
	Vector								m_spawnPosMS;
	EulerAngles							m_spawnRotMS;
	TSoftHandle< CEntityTemplate >		m_template;
};

BEGIN_CLASS_RTTI( CExtAnimCutsceneEffectEvent );
	PARENT_CLASS( CExtAnimDurationEvent );
	PROPERTY_CUSTOM_EDIT( m_effect, TXT( "Effect" ), TXT( "CutsceneEffect" ) );
	PROPERTY_EDIT( m_tag, TXT( "Entity tag" ) );
	PROPERTY_EDIT( m_template, TXT( "Entity template" ) );
	PROPERTY_EDIT( m_spawnPosMS, TXT( "" ) );
	PROPERTY_EDIT( m_spawnRotMS, TXT( "" ) );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CExtAnimCutsceneSlowMoEvent	: public CExtAnimCutsceneDurationEvent
									, public ICurveDataOwner
{
	DECLARE_RTTI_SIMPLE_CLASS( CExtAnimCutsceneSlowMoEvent );

private:
	Bool			m_enabled;
	Float			m_factor;
	Bool			m_useWeightCurve;
	SCurveData		m_weightCurve;

public:
	CExtAnimCutsceneSlowMoEvent();
	CExtAnimCutsceneSlowMoEvent( const CName& eventName, const CName& animationName, Float startTime, Float duration, const String& trackName );

	virtual void StartEx( const CAnimationEventFired& info, CCutsceneInstance* cs ) const override;
	virtual void StopEx( const CAnimationEventFired& info, CCutsceneInstance* cs ) const override;
	virtual void ProcessEx( const CAnimationEventFired& info, CCutsceneInstance* cs ) const override;

public: // ICurveDataOwner
	virtual TDynArray< SCurveData* >* GetCurvesData()	{ return nullptr; }
	virtual SCurveData* GetCurveData()					{ return &m_weightCurve; }
};

BEGIN_CLASS_RTTI( CExtAnimCutsceneSlowMoEvent );
	PARENT_CLASS( CExtAnimCutsceneDurationEvent );
	PROPERTY_EDIT( m_enabled, TXT( "" ) );
	PROPERTY_EDIT( m_factor, TXT( "" ) );
	PROPERTY_EDIT( m_useWeightCurve, String::EMPTY );
	PROPERTY_CUSTOM_EDIT( m_weightCurve, TXT( "Curve range is [0,1]" ), TXT("BaseCurveDataEditor") );
END_CLASS_RTTI();


//////////////////////////////////////////////////////////////////////////

class CExtAnimCutsceneWindEvent	: public CExtAnimCutsceneDurationEvent
	, public ICurveDataOwner
{
	DECLARE_RTTI_SIMPLE_CLASS( CExtAnimCutsceneWindEvent );

private:
	Bool			m_enabled;
	Float			m_factor;
	Bool			m_useWeightCurve;
	SCurveData		m_weightCurve;

public:
	CExtAnimCutsceneWindEvent();
	CExtAnimCutsceneWindEvent( const CName& eventName, const CName& animationName, Float startTime, Float duration, const String& trackName );

	virtual void StartEx( const CAnimationEventFired& info, CCutsceneInstance* cs ) const override;
	virtual void StopEx( const CAnimationEventFired& info, CCutsceneInstance* cs ) const override;
	virtual void ProcessEx( const CAnimationEventFired& info, CCutsceneInstance* cs ) const override;

	void SetWindFactorEventOverride( Float v ) const;

public: // ICurveDataOwner
	virtual TDynArray< SCurveData* >* GetCurvesData()	{ return nullptr; }
	virtual SCurveData* GetCurveData()					{ return &m_weightCurve; }
};

BEGIN_CLASS_RTTI( CExtAnimCutsceneWindEvent );
PARENT_CLASS( CExtAnimCutsceneDurationEvent );
PROPERTY_EDIT( m_enabled, TXT( "Enable wind override" ) );
PROPERTY_EDIT( m_factor, TXT( "Wind ovverride factor, 0 - no wind" ) );
PROPERTY_EDIT( m_useWeightCurve, String::EMPTY );
PROPERTY_CUSTOM_EDIT( m_weightCurve, TXT( "Curve range is [0,1]" ), TXT("BaseCurveDataEditor") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CExtAnimCutsceneHideTerrainEvent	: public CExtAnimCutsceneDurationEvent
{
	DECLARE_RTTI_SIMPLE_CLASS( CExtAnimCutsceneHideTerrainEvent );

public:
	CExtAnimCutsceneHideTerrainEvent();
	CExtAnimCutsceneHideTerrainEvent( const CName& eventName, const CName& animationName, Float startTime, Float duration, const String& trackName );

	virtual void StartEx( const CAnimationEventFired& info, CCutsceneInstance* cs ) const override;
	virtual void StopEx( const CAnimationEventFired& info, CCutsceneInstance* cs ) const override;
	virtual void ProcessEx( const CAnimationEventFired& info, CCutsceneInstance* cs ) const override;

};

BEGIN_CLASS_RTTI( CExtAnimCutsceneHideTerrainEvent );
PARENT_CLASS( CExtAnimCutsceneDurationEvent );
END_CLASS_RTTI();