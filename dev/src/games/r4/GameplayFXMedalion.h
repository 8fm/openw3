/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma once

struct SGameplayFXMedalionEntityHighlightInfo
{
	THandle<CEntity>		m_entity;
	Float					m_timeSinceStart;
	Bool					m_effectStopped;
};


class CGameplayFXMedalion : public CEntity
{
	DECLARE_ENGINE_CLASS( CGameplayFXMedalion, CEntity, 0 );

private:
	Float												m_beginRadius;
	Float												m_endRadius;
	Float												m_ringRadiusTolerance;
	Bool												m_debugLoop;
	Float												m_distPerSec;
	Float												m_sustainTime;
	CName												m_highlightTag;
	Bool												m_isStarted;

	Float												m_currentRadius;
	Float												m_endTime;

	TDynArray<SGameplayFXMedalionEntityHighlightInfo>	m_highlightList;
	TDynArray<SGameplayFXMedalionEntityHighlightInfo>	m_toRemoveList;

	//
	TDynArray< CEntity* >								m_entitiesCollector;			//<! Const size array for collecting tagged entities

public:

	CGameplayFXMedalion();
	virtual void OnAttached( CWorld* world );
	virtual void OnDetached( CWorld* world );
	virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag );
	virtual void OnTick( Float timeDelta );

};

BEGIN_CLASS_RTTI( CGameplayFXMedalion );
PARENT_CLASS( CEntity );
	PROPERTY_EDIT( m_beginRadius, TXT("Effect start radius") );
	PROPERTY_EDIT( m_endRadius, TXT("Effect end radius") );
	PROPERTY_EDIT( m_ringRadiusTolerance, TXT("Radius tolerance to highligh item") );
	PROPERTY_EDIT( m_debugLoop, TXT("Is effect looped? (debug only!)") );
	PROPERTY_EDIT( m_distPerSec, TXT("Growth speed (units per sec)") );
	PROPERTY_EDIT( m_sustainTime, TXT("Time to sustain effect") );
	PROPERTY_EDIT( m_highlightTag, TXT("Tag of objects to highlight") );
END_CLASS_RTTI();

