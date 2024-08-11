/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI 

#include "redGuiControl.h"
#include "../core/engineTime.h"

namespace RedGui
{
	class CRedGuiTimelineChart : public CRedGuiControl
	{
		DECLARE_CLASS_MEMORY_POOL( MemoryPool_RedGui, MC_RedGuiControls );
	public:
		CRedGuiTimelineChart( Uint32 left, Uint32 top, Uint32 width, Uint32 height );
		virtual ~CRedGuiTimelineChart();

		virtual void Draw();

		void SetShowTimelineLabel( Bool value );
		Bool GetShowTimelineLabel() const;

		void SetAutoScroll( Bool value );
		Bool GetAutoScroll() const;
		void SetFrameZoom( Uint32 zoom );
		void SetPause( Bool pause );

		void AddTimeline( const String& name );
		void RemoveTimeline( const String& name );
		void RemoveAllTimelines();
		Int32 FindTimelineIndex( const String& name );
		CRedGuiTimeline* FindTimeline( const String& name );
		Uint32 GetTimelineCount() const;

		// manage events
		void StartEvent( const String& eventName, const String& timelineName );		// start event with current engine time
		void StartEvent( const String& eventName, const String& timelineName, Float startTime );
		void StopEvent( const String& eventName, const String& timelineName );		// stop event with current engine time
		void StopEvent( const String& eventName, const String& timelineName, Float stopTime );
		void AddEvent( const String& eventName, const String& timelineName, Float startTime, Float stopTime, Color color = Color::LIGHT_BLUE );

		void ClearData();
		void ResetTimelines();

		Float GetStartTimeSeconds() const;
		Uint32 GetTaskCachedCount();

	private:
		void NotifyOnTick( RedGui::CRedGuiEventPackage& eventPackage, Float deltaTime );
		void NotifyScrollChangePosition( RedGui::CRedGuiEventPackage& eventPackage, Uint32 value );
		void NotifyMouseWheel( RedGui::CRedGuiEventPackage& eventPackage, Int32 delta );
		void NotifyClientSizeChanged( RedGui::CRedGuiEventPackage& eventPackage, const Vector2& oldSize, const Vector2& newSize );

		void CreateControls();

		void DrawSingleTimeline( CRedGuiTimeline* timeline, const Vector2& position, const Vector2& size );
		void DrawTimelines();
		void DrawTimelinesLabels();
		
		void UpdateScrollSize();
		void UpdateScrollPosition();
		void UpdateView();

		void ScrollToRight();

		Bool OnInternalInputEvent( enum ERedGuiInputEvent event, const Vector2& data );

	private:
		Bool				m_showTimelineLabel;
		Bool				m_autoScroll;

		Float				m_deltaTimeSeconds;
		Float				m_currentTimeSeconds;
		Float				m_startTimeSeconds;
		EngineTime			m_currentTimeTicks;
		EngineTime			m_startTimeTicks;

		CRedGuiPanel*		m_labelsPanel;
		CRedGuiPanel*		m_croppClient;
		CRedGuiScrollBar*	m_horizontalBar;
		CRedGuiScrollBar*	m_verticalBar;

		Int32				m_firstVisibleItem;
		Int32				m_lastVisibleItem;
		Uint32				m_horizontalRange;
		Uint32				m_verticalRange;
		Uint32				m_maxItemWidth;
		Uint32				m_maxVerticalItemCount;
		Vector2				m_firstItemPosition;

		Uint32				m_frameZoom;

		Bool				m_pause;

		Float				m_nowTime;
		Float				m_fromTime;

		TimelineCollection	m_timelines;
	};

}	// namespace RedGui

#endif	// NO_RED_GUI
