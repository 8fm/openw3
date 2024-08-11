/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS

#include "redGuiWindow.h"
#include "../core/loadingJobManager.h"
#include "../core/loadingJobMonitor.h"
#include "../core/loadingJob.h"

namespace DebugWindows
{
	class CDebugWindowLoadingJobs : public RedGui::CRedGuiWindow, public ILoadingJobMonitor
	{
	public:
		CDebugWindowLoadingJobs();
		~CDebugWindowLoadingJobs();

	private:
		void OnWindowOpened( CRedGuiControl* control );
		void OnWindowClosed( CRedGuiControl* control );

		void NotifyOnTick( RedGui::CRedGuiEventPackage& eventPackage, Float deltaTime );
		void NotifyOnResetStats( RedGui::CRedGuiEventPackage& eventPackage );

		// ILoadingJobMonitor interface
		virtual void OnJobIssued( class ILoadJob* job );
		virtual void OnJobStarted( class ILoadJob* job );
		virtual void OnJobFinished( class ILoadJob* job, const EJobResult result );
		virtual void OnJobCanceled( class ILoadJob* job );

	private:
		struct JobCategory
		{
			const Char*					m_name;				//!< Type of job
			Uint32						m_finishedCount;	//!< Number of jobs executed so far
			Uint32						m_failedCount;		//!< Number of jobs failed so far
			Uint32						m_queuedCount;		//!< Number of jobs queued
			Uint64						m_size;				//!< Total data size
			Double						m_time;				//!< Total time taken
			RedGui::CRedGuiListItem*	m_item;				//!< List item

			DECLARE_STRUCT_MEMORY_ALLOCATOR( MC_Debug );

			RED_INLINE JobCategory( const Char* name )
				: m_name( name )
				, m_finishedCount( 0 )
				, m_failedCount( 0 )
				, m_queuedCount( 0 )
				, m_size( 0 )
				, m_time( 0 )
				, m_item( nullptr )
			{}

			void Reset()
			{
				m_failedCount = 0;
				m_finishedCount = 0;
				m_time = 0;
				m_size = 0;
			}

			void UpdateCaption();
		};

		struct JobInfo
		{
			DECLARE_STRUCT_MEMORY_ALLOCATOR( MC_Debug );

			void*					m_key;				//!< Hash key (job reference)
			EJobPriority			m_priority;			//!< Assigned job priority
			EngineTime				m_issueTime;		//!< Time this job was issued
			EngineTime				m_startTime;		//!< Time this job was started
			EngineTime				m_finishTime;		//!< Time this job was finished
			EJobState				m_state;			//!< Job state
			JobCategory*			m_category;			//!< Job category
			bool					m_wasQueued;		//!< Did we count the queueing of this job ?

			RED_INLINE JobInfo( const ILoadJob* issuedJob, JobCategory* category )
				: m_key( (void*)issuedJob )
				, m_priority( issuedJob->GetPriority() )
				, m_issueTime( EngineTime::GetNow() )
				, m_state( JOB_Pending )
				, m_category( category )
				, m_wasQueued( false )
			{}
		};

		// gui
		RedGui::CRedGuiList*					m_jobCategoryList;

		// logic
		THashMap< void*, JobInfo* >				m_jobs;
		THashMap< void*, JobCategory* >			m_categories;	
		Red::Threads::CMutex					m_accessMutex;

		void RefreshCategoryList();

		JobCategory* MapCategory( const Char* name );
		JobInfo* MapJob( const ILoadJob* job );
	};

}	// namespace DebugWindows

#endif	// NO_DEBUG_WINDOWS
#endif	// NO_RED_GUI
