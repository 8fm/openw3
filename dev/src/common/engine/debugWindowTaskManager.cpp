/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS

#include "debugWindowTaskManager.h"
#include "redGuiList.h"
#include "redGuiLabel.h"
#include "redGuiCheckBox.h"
#include "redGuiManager.h"
#include "redGuiTimelineChart.h"
#include "redGuiGridLayout.h"
#include "redGuiAdvancedSlider.h"
#include "redGuiTimeline.h"
#include "../core/taskThread.h"
#include "../core/taskScheduler.h"
#include "../core/loadingJobManager.h"
#include "../core/taskManager.h"

extern void (* volatile onTaskStartedProcessing)( const CTask* );
extern void (* volatile onTaskFinishedProcessing)( const CTask* );

namespace DebugWindows
{
	Red::Threads::CMutex CDebugWindowTaskManager::m_accessMutex;
	TDynArray< CDebugWindowTaskManager::STaskThreadInfo* > CDebugWindowTaskManager::m_threads;
	TDynArray< CDebugWindowTaskManager::STaskStats > CDebugWindowTaskManager::m_taskTypes;
	RedGui::CRedGuiCheckBox* CDebugWindowTaskManager::m_pauseTimeline;
	RedGui::CRedGuiTimelineChart* CDebugWindowTaskManager::m_timelineChart = 0;

	CDebugWindowTaskManager::STaskInfo::STaskInfo( const CTask* sourceJob ) 
		: m_id( ( Uint64 ) sourceJob )
		, m_type( sourceJob->GetDebugName() )
		, m_color( sourceJob->GetDebugColor() )
		, m_state( JS_Pending )
		, m_thread( nullptr )
		, m_isIOTask( false )
	{
	}

	CDebugWindowTaskManager::STaskThreadInfo::STaskThreadInfo( const CTaskThread* sourceThread, RedGui::CRedGuiTimeline* timeline ) 
		: m_thread( sourceThread )
		, m_name( ANSI_TO_UNICODE( sourceThread->GetThreadName() ) )
		, m_timeline( timeline )
	{
	}

	CDebugWindowTaskManager::STaskStats::STaskStats( const Char* type /*= nullptr */ ) 
		: m_type( type )
		, m_count( 0 )
		, m_totalTime( 0.0f )
		, m_maxTime( 0.0f )
		, m_lastTime( 0.0f )
	{
	}

	void CDebugWindowTaskManager::STaskStats::Update( Float taskTime )
	{
		m_totalTime += taskTime;
		m_maxTime = Max< Float >( m_maxTime, taskTime );
		m_lastTime = taskTime;
		m_count += 1;
	}

	CDebugWindowTaskManager::CDebugWindowTaskManager()
		: RedGui::CRedGuiWindow( 200, 200, 650, 600 )
	{
		GRedGui::GetInstance().EventTick.Bind( this, &CDebugWindowTaskManager::NotifyOnTick );

		SetCaption( TXT("Task manager") );
		CreateControls();
	}

	CDebugWindowTaskManager::~CDebugWindowTaskManager()
	{
		GRedGui::GetInstance().EventTick.Unbind( this, &CDebugWindowTaskManager::NotifyOnTick );
	}

	void CDebugWindowTaskManager::OnWindowOpened( CRedGuiControl* control )
	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_accessMutex );

		// Reset time base
		m_timeBase = (Int32) (Float) EngineTime::GetNow();

		// reset timelines
		m_timelineChart->ResetTimelines();

		m_timelineChart->AddTimeline( TXT( "Main") );

		const TDynArray< CTaskThread* >& taskThread = GTaskManager->GetTaskScheduler().GetThreadList();
		for( auto i = taskThread.Begin(); i != taskThread.End(); ++i )
		{
			Uint64 test = ( Uint64 ) &( *i )->GetTaskDispatcher();
			// Create thread description
			// add new timeline to chart
			m_timelineChart->AddTimeline( ANSI_TO_UNICODE( ( *i )->GetThreadName() ) );
			RedGui::CRedGuiTimeline* timeline = m_timelineChart->FindTimeline( ANSI_TO_UNICODE( ( *i )->GetThreadName() ) );
			m_threads.PushBack( new STaskThreadInfo( *i, timeline ) );
		}

		Vector size = m_timelineChart->GetSize();
		size.Y = 10.0f * taskThread.Size();
		m_timelineChart->SetSize( size );

		onTaskStartedProcessing = &OnTaskStartedProcessing;
		onTaskFinishedProcessing = &OnTaskFinishedProcessing;
	}

	void CDebugWindowTaskManager::OnWindowClosed( CRedGuiControl* control )
	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_accessMutex );

		// Uninstall job manager listener
		//SJobManager::GetInstance().SetListener( nullptr );

		// Cleanup data
		m_taskTypes.Clear();
		m_threads.ClearPtr();

		// clear data in chart
		m_timelineChart->ClearData();

		onTaskStartedProcessing = 0;
		onTaskFinishedProcessing = 0;
	}

	void CDebugWindowTaskManager::NotifyOnTick( RedGui::CRedGuiEventPackage& eventPackage, Float deltaTime )
	{
		RED_UNUSED( eventPackage );

		if( GetVisible() == false )
		{
			return;
		}

		// update job types list
		const Uint32 jobTypeCount = m_taskTypes.Size();
		for ( Uint32 i=0; i<jobTypeCount; ++i )
		{
			const STaskStats& stat = m_taskTypes[i];

			Int32 itemIndex = -1;
			const Uint32 itemCount = m_taskList->GetItemCount();
			for( Uint32 j=0; j<itemCount; ++j )
			{
				if( m_taskList->GetItemText( j, 0 ) == stat.m_type.AsChar() )
				{
					itemIndex = j;
				}
			}

			if( itemIndex == -1 )
			{
				itemIndex = m_taskList->AddItem( stat.m_type.AsChar(), stat.m_color );
			}

			m_taskList->SetItemText( String::Printf( TXT("%i"), stat.m_count ), itemIndex, 1 );
			m_taskList->SetItemText( String::Printf( TXT("%1.3f"), stat.m_totalTime ), itemIndex, 2 );
			m_taskList->SetItemText( String::Printf( TXT("%1.3f"), stat.m_maxTime * 1000.0f ), itemIndex, 3 );
			m_taskList->SetItemText( String::Printf( TXT("%1.3f"), stat.m_lastTime * 1000.0f ), itemIndex, 4 );
		}

		SetCaption( String::Printf( TXT("Task manager %i"), m_timelineChart->GetTaskCachedCount() ) );

		if( m_pauseTimeline->GetChecked() ) return;

		RedGui::CRedGuiTimeline* timeline = m_timelineChart->FindTimeline( TXT( "Main" ) );
		if( !timeline ) return;
		Float current = EngineTime::GetNow();
		timeline->AddEvent( TXT("Engine Tick"), current - m_timelineChart->GetStartTimeSeconds(), current - m_timelineChart->GetStartTimeSeconds(), Color::WHITE );
	}

	void CDebugWindowTaskManager::OnTaskStartedProcessing( const CTask* task )
	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_accessMutex );

		STaskInfo* taskInfo = new STaskInfo( task );

		// Find thread
		STaskThreadInfo* taskThreadInfo = nullptr;
		for ( Uint32 j=0; j<m_threads.Size(); ++j )
		{
			if ( ( Uint64 ) &m_threads[j]->m_thread->GetTaskDispatcher() == ( Uint64 ) task->GetDispatcher() )
			{
				taskThreadInfo = m_threads[j];
				break;
			}
		}

		// Assign
		if ( taskThreadInfo )
		{
			// Assign job to thread
			taskInfo->m_thread = taskThreadInfo;
			taskThreadInfo->m_tasks.PushBack( taskInfo );
		}

		// Change state
		taskInfo->m_state = JS_Processing;
		taskInfo->m_startTime = EngineTime::GetNow();
	}

	void CDebugWindowTaskManager::OnTaskFinishedProcessing( const CTask* task )
	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_accessMutex );

		if( m_pauseTimeline->GetChecked() ) return;

		STaskThreadInfo* taskThreadInfo = nullptr;
		for ( Uint32 j=0; j<m_threads.Size(); ++j )
		{
			taskThreadInfo = m_threads[j];

			if ( ( Uint64 ) &taskThreadInfo->m_thread->GetTaskDispatcher() == ( Uint64 ) task->GetDispatcher() )
			{
				for ( Int32 i=taskThreadInfo->m_tasks.Size(); i>0; ++i )
				{
					if ( taskThreadInfo->m_tasks[ i - 1 ]->m_id == ( Uint64 ) task )
					{
						STaskInfo* taskInfo = taskThreadInfo->m_tasks[ i - 1 ];
						// Remember result
						taskInfo->m_finishTime = EngineTime::GetNow();


						STaskStats* taskStat = nullptr;
						String taskType = task->GetDebugName();
						for ( Uint32 k=0; k<m_taskTypes.Size(); ++k )
						{
							STaskStats& stats = m_taskTypes[k];
							size_t index = 0;
							if ( stats.m_type.FindSubstring( taskType, index ) )
							{
								taskStat = &stats;
								break;
							}	
						}

						if ( !taskStat )
						{
							taskStat = new ( m_taskTypes ) STaskStats( taskType.AsChar() );
							taskStat->m_color = ( Color ) task->GetDebugColor();
						}
						EngineTime dif = taskInfo->m_finishTime - taskInfo->m_startTime;
						const Float taskTime = dif;
						taskStat->Update( taskTime );

						Color color = taskInfo->m_color;
						if( taskStat->m_count & 1 )
						{
							color.R /= 2;
							color.G /= 2;
							color.B /= 2;
						}

						taskThreadInfo->m_timeline->AddEvent( taskThreadInfo->m_name, taskInfo->m_startTime - m_timelineChart->GetStartTimeSeconds(), taskInfo->m_finishTime - m_timelineChart->GetStartTimeSeconds(), color );

						delete taskInfo;
						taskThreadInfo->m_tasks.RemoveAtFast( i - 1 );
						// Done
						return;
					}
				}
			}
		}

	}

	void CDebugWindowTaskManager::CreateControls()
	{
		RedGui::CRedGuiGridLayout* layout = new RedGui::CRedGuiGridLayout( 0, 0, 100, 50 );
		layout->SetDock( RedGui::DOCK_Top );
		layout->SetDimensions( 3, 1 );
		AddChild( layout );
		{
			m_pauseTimeline = new RedGui::CRedGuiCheckBox( 0, 0, 100, 23 );
			m_pauseTimeline->SetMargin( Box2( 5, 5, 5, 5 ) );
			m_pauseTimeline->SetChecked( false );
			m_pauseTimeline->SetText( TXT("Pause") );
			m_pauseTimeline->EventCheckedChanged.Bind( this, &CDebugWindowTaskManager::OnTimelinesPause );
			layout->AddChild( m_pauseTimeline );

			m_zoomSlider = new RedGui::CRedGuiAdvancedSlider( 0, 0, 300, 20 );
			m_zoomSlider->SetMargin( Box2( 5, 5, 5, 5 ) );
			m_zoomSlider->SetMinValue( 1 );
			m_zoomSlider->SetMaxValue( 100 );
			m_zoomSlider->SetValue( 1 );
			m_zoomSlider->SetStepValue( 1 );
			m_zoomSlider->EventScroll.Bind( this, &CDebugWindowTaskManager::OnTimelinesZoom );
			layout->AddChild( m_zoomSlider );

			m_timelineLegend = new RedGui::CRedGuiCheckBox( 0, 0, 100, 23 );
			m_timelineLegend->SetMargin( Box2( 5, 5, 5, 5 ) );
			m_timelineLegend->SetChecked( true );
			m_timelineLegend->SetText( TXT("Timelines legend") );
			m_timelineLegend->EventCheckedChanged.Bind( this, &CDebugWindowTaskManager::OnTimelinesLegend );
			layout->AddChild( m_timelineLegend );
		}

		m_timelineChart = new RedGui::CRedGuiTimelineChart( 0, 0, 100, 300 );
		m_timelineChart->SetMargin( Box2( 5, 5, 5, 5 ) );
		m_timelineChart->SetDock( RedGui::DOCK_Top );
		AddChild( m_timelineChart );

		m_pendingTasksLabel = new RedGui::CRedGuiLabel( 0, 0, 100, 20 );
		m_pendingTasksLabel->SetMargin( Box2( 5, 5, 5, 5 ) );
		m_pendingTasksLabel->SetDock( RedGui::DOCK_Top );
		m_pendingTasksLabel->SetText( TXT("Pending task count: ") );
		AddChild( m_pendingTasksLabel );

		m_taskList = new RedGui::CRedGuiList( 0, 0, 100, 200 );
		m_taskList->SetMargin( Box2( 5, 5, 5 ,5 ) );
		m_taskList->SetDock( RedGui::DOCK_Fill );
		m_taskList->SetSorting( true );
		m_taskList->AppendColumn( TXT("JOB Type"), 200 );
		m_taskList->AppendColumn( TXT("Count"), 100, RedGui::SA_Integer );
		m_taskList->AppendColumn( TXT("Total [s]"), 100, RedGui::SA_Real );
		m_taskList->AppendColumn( TXT("Max [ms]"), 100, RedGui::SA_Real );
		m_taskList->AppendColumn( TXT("Last [ms]"), 100, RedGui::SA_Real );
		AddChild( m_taskList );
	}

	void CDebugWindowTaskManager::OnTimelinesLegend( RedGui::CRedGuiEventPackage& eventPackage, Bool value )
	{
		RED_UNUSED( eventPackage );

		m_timelineChart->SetShowTimelineLabel( value );
	}

	void CDebugWindowTaskManager::OnTimelinesZoom( RedGui::CRedGuiEventPackage& eventPackage, Float value )
	{
		RED_UNUSED( eventPackage );

		m_timelineChart->SetFrameZoom( ( Uint32 ) value );
	}

	void CDebugWindowTaskManager::OnTimelinesPause( RedGui::CRedGuiEventPackage& eventPackage, Bool value )
	{
		m_timelineChart->SetPause( value );
	}

}	// namespace DebugWindows

#endif	//NO_DEBUG_WINDOWS
#endif	//NO_RED_GUI
