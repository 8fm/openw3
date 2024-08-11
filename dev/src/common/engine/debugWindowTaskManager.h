/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS

#include "redGuiWindow.h"
#include "../core/engineTime.h"

class CTaskThread;
class CTask;

namespace DebugWindows
{
	class CDebugWindowTaskManager : public RedGui::CRedGuiWindow
	{
		enum EJobState
		{
			JS_Pending,
			JS_Processing,
			JS_Finished,
			JS_Failed,
			JS_Canceled,
		};

	public: /* because of TDynArray::ClearPtr template */
		struct STaskThreadInfo;

		struct STaskInfo
		{
			DECLARE_STRUCT_MEMORY_ALLOCATOR( MC_Debug );

			Uint64					m_id;				//!< ID of job
			const Char*				m_type;				//!< Type of job
			String					m_shortInfo;		//!< Short job info
			STaskThreadInfo*		m_thread;			//!< Thread that is processing this job
			EngineTime				m_startTime;		//!< Time this job was started
			EngineTime				m_finishTime;		//!< Time this job was finished
			EJobState				m_state;			//!< Job state
			Bool					m_isIOTask;			//!< Is this an IO task
			Color					m_color;			//!< Job color

			RED_INLINE STaskInfo( const CTask* sourceJob );
		};

		struct STaskThreadInfo
		{
			DECLARE_STRUCT_MEMORY_ALLOCATOR( MC_Debug );

			const CTaskThread*			m_thread;			//!< Thread data
			TDynArray< STaskInfo* >		m_tasks;			//!< Task processed on this thread
			String						m_name;
			RedGui::CRedGuiTimeline*	m_timeline;
			RED_INLINE STaskThreadInfo( const CTaskThread* sourceThread, RedGui::CRedGuiTimeline* timeline );
		};

		struct STaskStats
		{
			String			m_type;
			Uint64			m_count;
			Float			m_totalTime;
			Float			m_maxTime;
			Float			m_lastTime;
			Color			m_color;

			STaskStats( const Char* type = nullptr );
			void Update( Float taskTime );
		};

	public:
		CDebugWindowTaskManager();
		~CDebugWindowTaskManager();

		static void OnTaskStartedProcessing( const CTask* sourceJob );
		static void OnTaskFinishedProcessing( const CTask* sourceJob );

	private:
		void OnWindowOpened( CRedGuiControl* control );
		void OnWindowClosed( CRedGuiControl* control );

		void NotifyOnTick( RedGui::CRedGuiEventPackage& eventPackage, Float deltaTime );
		void OnTimelinesLegend( RedGui::CRedGuiEventPackage& eventPackage, Bool value );
		void OnTimelinesPause( RedGui::CRedGuiEventPackage& eventPackage, Bool value );
		void OnTimelinesZoom( RedGui::CRedGuiEventPackage& eventPackage, Float value );

		void CreateControls();

	private:
		// gui
		RedGui::CRedGuiCheckBox*				m_timelineLegend;
		static RedGui::CRedGuiCheckBox*			m_pauseTimeline;
		static RedGui::CRedGuiTimelineChart*	m_timelineChart;
		RedGui::CRedGuiLabel*					m_pendingTasksLabel;
		RedGui::CRedGuiList*					m_taskList;
		RedGui::CRedGuiAdvancedSlider*			m_zoomSlider;

		// logic
		static TDynArray< STaskThreadInfo* >	m_threads;
		static TDynArray< STaskStats >			m_taskTypes;
		static Red::Threads::CMutex				m_accessMutex;
		Bool									m_showIO;
		Int32									m_timeBase;
		EngineTime								m_trackTime;
		Bool									m_snapTime;
		Float									m_timeScale;
		Uint32									m_pendingTaskCount;
	};

}	// namespace DebugWindows

#endif	// NO_DEBUG_WINDOWS
#endif	// NO_RED_GUI
