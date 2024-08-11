/**
* Copyright c 2012 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "storySceneEventDuration.h"

RED_DECLARE_NAME( MIMIC_FILTER_ENABLE_VARIABLE );
RED_DECLARE_NAME( MIMIC_FILTER_INDEX_VARIABLE );
RED_DECLARE_NAME( MIMIC_FILTER_BLEND_VARIABLE );
RED_DECLARE_NAME( MIMIC_FILTER_WEIGHT_VARIABLE );

class CStorySceneEventMimicsFilter	: public CStorySceneEventDuration
									, public ICurveDataOwner
{
	DECLARE_SCENE_EVENT_CLASS( CStorySceneEventMimicsFilter, CStorySceneEventDuration );

protected:
	CName			m_actor;
	CName			m_filterName;
	Float			m_weight;

	Bool			m_useWeightCurve;
	SCurveData		m_weightCurve;

public:
	CStorySceneEventMimicsFilter();
	CStorySceneEventMimicsFilter( const String& eventName, CStorySceneElement* sceneElement, Float startTime, const CName& actor, const String& trackName );

	// compiler generated cctor is ok

	virtual CStorySceneEventMimicsFilter* Clone() const override;

public:
	virtual void OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;
	virtual void OnProcess( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;
	virtual void OnEnd( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;

public: // ICurveDataOwner
	virtual TDynArray< SCurveData* >* GetCurvesData() { return nullptr; }
	virtual SCurveData* GetCurveData() { return &m_weightCurve; }

	virtual CName GetSubject() const override { return m_actor; }

#ifndef NO_EDITOR
public:
	Bool UseWeightCurve() const { return m_useWeightCurve; }
	const SCurveData* GetWeightCurve() const { return &m_weightCurve; }
#endif

private:
	Int32 GetMimicFilterIndex( const CActor* actor ) const;

	void Progress( CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, Float weight ) const;
};

BEGIN_CLASS_RTTI( CStorySceneEventMimicsFilter )
	PARENT_CLASS( CStorySceneEventDuration )
	PROPERTY_CUSTOM_EDIT( m_actor, TXT( "Actor" ), TXT( "DialogVoiceTag" ) )
	PROPERTY_CUSTOM_EDIT( m_filterName, TXT( "Mimic filter" ), TXT( "SceneMimicFilterSelection" ) );
	PROPERTY_EDIT( m_weight, TXT( "Filter weight factor" ) );
	PROPERTY_EDIT( m_useWeightCurve, TXT( "" ) );
	PROPERTY_CUSTOM_EDIT( m_weightCurve, String::EMPTY, TXT("BaseCurveDataEditor") );
END_CLASS_RTTI()
