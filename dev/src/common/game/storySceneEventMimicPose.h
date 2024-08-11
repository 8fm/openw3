/**
* Copyright c 2012 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "storySceneEventDuration.h"

RED_DECLARE_NAME( MIMIC_POSE_ENABLE_VARIABLE );
RED_DECLARE_NAME( MIMIC_POSE_INDEX_VARIABLE );
RED_DECLARE_NAME( MIMIC_POSE_BLEND_VARIABLE );
RED_DECLARE_NAME( MIMIC_POSE_WEIGHT_VARIABLE );

class CStorySceneEventMimicsPose	: public CStorySceneEventDuration
									, public ICurveDataOwner
{
	DECLARE_SCENE_EVENT_CLASS( CStorySceneEventMimicsPose, CStorySceneEventDuration );

protected:
	CName			m_actor;
	CName			m_poseName;
	Float			m_weight;

	Bool			m_useWeightCurve;
	SCurveData		m_weightCurve;

public:
	CStorySceneEventMimicsPose();
	CStorySceneEventMimicsPose( const String& eventName, CStorySceneElement* sceneElement, Float startTime, const CName& actor, const String& trackName );

	// compiler generated cctor is ok

	virtual CStorySceneEventMimicsPose* Clone() const override;

public:
	virtual void OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;
	virtual void OnProcess( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;
	virtual void OnEnd( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;

	virtual CName GetSubject() const override { return m_actor; }

public: // ICurveDataOwner
	virtual TDynArray< SCurveData* >* GetCurvesData() { return nullptr; }
	virtual SCurveData* GetCurveData() { return &m_weightCurve; }

#ifndef NO_EDITOR
public:
	Bool UseWeightCurve() const { return m_useWeightCurve; }
	const SCurveData* GetWeightCurve() const { return &m_weightCurve; }
#endif

private:
	Int32 GetMimicPoseIndex( const CActor* actor ) const;

	void Progress( CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, Float weight ) const;
};

BEGIN_CLASS_RTTI( CStorySceneEventMimicsPose )
	PARENT_CLASS( CStorySceneEventDuration )
	PROPERTY_CUSTOM_EDIT( m_actor, TXT( "Actor" ), TXT( "DialogVoiceTag" ) )
	PROPERTY_CUSTOM_EDIT( m_poseName, TXT( "Mimic pose" ), TXT( "SceneMimicPoseSelection" ) );
	PROPERTY_EDIT_RANGE( m_weight, TXT( "Pose wight factor" ), 0.f, 1.f );
	PROPERTY_EDIT( m_useWeightCurve, TXT( "" ) );
	PROPERTY_CUSTOM_EDIT( m_weightCurve, String::EMPTY, TXT("BaseCurveDataEditor") );
END_CLASS_RTTI()
