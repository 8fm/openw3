/**
* Copyright c 2012 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "storySceneEventDuration.h"

namespace DialogTimelineItems
{
	class CTimelineItemBlend;
	class CTimelineItemCurveBlend;
};

//////////////////////////////////////////////////////////////////////////
//! General purpose blend event
class CStorySceneEventBlend : public CStorySceneEventDuration
{
	DECLARE_SCENE_EVENT_CLASS( CStorySceneEventBlend, CStorySceneEventDuration )

	friend class DialogTimelineItems::CTimelineItemBlend;

protected:
	TDynArray< CGUID > m_keys;		//!< GUIDs of linked key events

public:
	CStorySceneEventBlend();
	CStorySceneEventBlend( const String& eventName, CStorySceneElement* sceneElement, const String& trackName, Float startTime, Float duration = 2.0f );

	// compiler generated cctor is ok

	Uint32 GetKeyIndex( const CGUID& key ) const { return (Uint32) m_keys.GetIndex( key ); }
	Uint32 GetNumberOfKeys() const { return m_keys.Size(); }
	CGUID GetKey( Uint32 keyIndex ) const;
	void RemoveKey( CGUID key );

	virtual void OnGuidChanged( CGUID oldGuid, CGUID newGuid ) override;
};

BEGIN_ABSTRACT_CLASS_RTTI( CStorySceneEventBlend )
	PARENT_CLASS( CStorySceneEventDuration )
	PROPERTY( m_keys )
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////
//! General purpose curve based duration blend event
class CStorySceneEventCurveBlend : public CStorySceneEventBlend
{
	DECLARE_SCENE_EVENT_CLASS( CStorySceneEventCurveBlend, CStorySceneEventBlend )

	friend class DialogTimelineItems::CTimelineItemCurveBlend;

protected:
	SMultiCurve m_curve;			//!< Curve containing transforms and custom float values

	struct CurveKeyData
	{
		static const Uint32 MAX_EXTRA_DATA = 8;

		EngineTransform m_transform;
		Float m_extraData[MAX_EXTRA_DATA];
	};

public:
	CStorySceneEventCurveBlend();
	CStorySceneEventCurveBlend( const String& eventName, CStorySceneElement* sceneElement, const String& trackName, Float startTime, Float duration = 2.0f );

	// compiler generated cctor is ok

	// Overridden from CStorySceneEvent
	virtual void OnInit( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer ) const override;
	virtual void OnDeinit( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer ) const override;

	//! Initializes the curve
	void InitCurve();

	//! Gets number of extra (float) curve data streams
	virtual Uint32 GetNumExtraCurveDataStreams() const = 0;
	//! Gets key data from the event
	virtual void GetKeyDataFromEvent( CStorySceneEvent* keyEvent, CurveKeyData& result ) const = 0;
	//! Sets key data to event
	virtual void SetKeyDataToEvent( CStorySceneEvent* keyEvent, const CurveKeyData& data ) const = 0;

	void UpdateCurveTotalTimeFromDuration( Float eventDuration ) { m_curve.SetTotalTime( eventDuration ); }
};

BEGIN_ABSTRACT_CLASS_RTTI( CStorySceneEventCurveBlend )
	PARENT_CLASS( CStorySceneEventBlend )
	PROPERTY_CUSTOM_EDIT( m_curve, TXT("Curve"), TXT("MultiCurveEditor") )
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////
//! implementation

RED_INLINE CGUID CStorySceneEventBlend::GetKey( Uint32 keyIndex ) const
{
	return m_keys[ keyIndex ];
}

RED_INLINE void CStorySceneEventBlend::RemoveKey( CGUID key )
{
	m_keys.Remove( key );
}
