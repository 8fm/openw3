/**
* Copyright c 2012 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "storySceneEventCameraBlend.h"

namespace DialogTimelineItems
{
	class CTimelineItemEnhancedCameraBlend;
};

class CStorySceneEventEnhancedCameraBlend : public CStorySceneEventCurveBlend
{
	DECLARE_SCENE_EVENT_CLASS( CStorySceneEventEnhancedCameraBlend, CStorySceneEventCurveBlend )

protected:
	// Extra camera animated data (apart from camera transform)
	enum EExtraCurveData
	{
		EExtraCurveData_FOV = 0,
		EExtraCurveData_Zoom,
		EExtraCurveData_DOFBlurDistFar,
		EExtraCurveData_DOFBlurDistNear,
		EExtraCurveData_DOFFocusDistFar,
		EExtraCurveData_DOFFocusDistNear,
		EExtraCurveData_DOFIntensity,

		EExtraCurveData_COUNT
	};

	StorySceneCameraDefinition m_baseCameraDefinition;

public:
	CStorySceneEventEnhancedCameraBlend();
	CStorySceneEventEnhancedCameraBlend( const String& eventName, CStorySceneElement* sceneElement, const String& trackName );

	// compiler generated cctor is ok

	virtual CStorySceneEventEnhancedCameraBlend* Clone() const override;

	// Overridden from CStorySceneEventDuration
	virtual void OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;
	virtual void OnEnd( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;
	virtual void OnProcess( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;

	// Overridden from CStorySceneEventBlend
	virtual Uint32 GetNumExtraCurveDataStreams() const override;
	virtual void GetKeyDataFromEvent( CStorySceneEvent* keyEvent, CurveKeyData& result ) const override;
	virtual void SetKeyDataToEvent( CStorySceneEvent* keyEvent, const CurveKeyData& data ) const override;

	Bool GetCameraStateAt( Float localTime, StorySceneCameraDefinition* result ) const;
	Bool GetAbsoluteCameraStateAt( Float localTime, StorySceneCameraDefinition* result ) const;

protected:
	static void CopyKeyDataToCameraDefinition( const CurveKeyData& src, StorySceneCameraDefinition* dst );
	static void CopyCameraDefinitionToKeyData( CurveKeyData& dst, const StorySceneCameraDefinition* src );

	friend class DialogTimelineItems::CTimelineItemEnhancedCameraBlend;
};

BEGIN_CLASS_RTTI( CStorySceneEventEnhancedCameraBlend )
	PARENT_CLASS( CStorySceneEventCurveBlend )
	PROPERTY( m_baseCameraDefinition )
END_CLASS_RTTI()