/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "storySceneEventDuration.h"

//////////////////////////////////////////////////////////////////////////

struct SStorySceneEventGroupEntry
{
	DECLARE_RTTI_STRUCT( SStorySceneEventGroupEntry );

	Float						m_time;
	CStorySceneEvent*			m_event;

	SStorySceneEventGroupEntry() {}

	SStorySceneEventGroupEntry( Float time, CStorySceneEvent* event )
		: m_time( time )
		, m_event( event )
	{
	}
};

BEGIN_CLASS_RTTI( SStorySceneEventGroupEntry )
	PROPERTY( m_time );
	PROPERTY( m_event );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////
// DIALOG_TOMSIN_TODO - to do wywalenia i przerobienia na parentowanie eventow miedzy soba

class CStorySceneEventGroup : public CStorySceneEventDuration
{
	DECLARE_SCENE_EVENT_CLASS( CStorySceneEventGroup, CStorySceneEventDuration )

protected:
	TDynArray< SStorySceneEventGroupEntry >	m_events;

public:
	CStorySceneEventGroup();
	CStorySceneEventGroup( const String& eventName, CStorySceneElement* sceneElement, const String& trackName, Float startTime, Float duration = 2.0f );
	CStorySceneEventGroup( const CStorySceneEventGroup& );

	virtual CStorySceneEventGroup* Clone() const override;

	virtual void Serialize(  IFile& file  ) override;

public:
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler ) override;
	virtual void OnInitInstance( InstanceBuffer& instance ) const override;
	virtual void OnReleaseInstance( InstanceBuffer& instance ) const override;

protected:
	virtual void OnInit( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer ) const override;
	virtual void OnDeinit( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer ) const override;

public:
	virtual void OnProcess( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;

public:
	void AddEvent( CStorySceneEvent* event, Float time );

	RED_INLINE const TDynArray< SStorySceneEventGroupEntry >& GetEvents() const { return m_events; }
};

BEGIN_CLASS_RTTI( CStorySceneEventGroup )
	PARENT_CLASS( CStorySceneEventDuration )
END_CLASS_RTTI()
