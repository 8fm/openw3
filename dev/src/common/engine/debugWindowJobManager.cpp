/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "debugWindowJobManager.h"

#include "redGuiList.h"
#include "redGuiListItem.h"
#include "redGuiLabel.h"
#include "redGuiButton.h"
#include "redGuiCheckBox.h"
#include "redGuiGridLayout.h"
#include "redGuiManager.h"
#include "redGuiScrollPanel.h"
#include "redGuiTab.h"

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS

namespace DebugWindows
{

CDebugWindowLoadingJobs::CDebugWindowLoadingJobs()
	: CRedGuiWindow( 100, 100, 600, 400 )
{
	SetCaption( TXT("Loading Jobs") );

	RedGui::CRedGuiGridLayout* menuRow1 = new RedGui::CRedGuiGridLayout( 0, 0, 100, 26 );
	menuRow1->SetDock( RedGui::DOCK_Top );
	menuRow1->SetMargin( Box2( 5, 5, 5, 5 ) );
	menuRow1->SetDimensions( 5, 1 );
	AddChild( menuRow1 );

	RedGui::CRedGuiButton* resetStats = new RedGui::CRedGuiButton( 0, 0, 40, 20 );
	resetStats->SetText( TXT("Reset") );
	resetStats->SetMargin( Box2( 15, 5, 10, 0 ) );
	resetStats->EventButtonClicked.Bind( this, &CDebugWindowLoadingJobs::NotifyOnResetStats );
	menuRow1->AddChild( resetStats );

	RedGui::CRedGuiTab* tabs = new RedGui::CRedGuiTab( 0, 0, 100, 100 );
	tabs->SetDock( RedGui::DOCK_Fill );
	tabs->SetMargin( Box2( 5, 5, 5, 5 ) );
	AddChild( tabs );

	// tab0
	{
		const Uint32 tabIndex = tabs->AddTab( TXT("Categories") );
		RedGui::CRedGuiScrollPanel* classStatsTab = tabs->GetTabAt( tabIndex );

		m_jobCategoryList = new RedGui::CRedGuiList( 0, 0, 100, 100 );
		m_jobCategoryList->SetDock( RedGui::DOCK_Fill );
		m_jobCategoryList->SetMargin( Box2( 5, 5, 5, 5 ) );
		m_jobCategoryList->AppendColumn( TXT("Job category"), 250 );
		m_jobCategoryList->AppendColumn( TXT("Queued"), 55, RedGui::SA_Integer );
		m_jobCategoryList->AppendColumn( TXT("Finished"), 55, RedGui::SA_Integer );
		m_jobCategoryList->AppendColumn( TXT("Failed"), 55, RedGui::SA_Integer );
		//m_jobCategoryList->AppendColumn( TXT("Size [MB]"), 60, RedGui::SA_Real );
		m_jobCategoryList->AppendColumn( TXT("Time [ms]"), 60, RedGui::SA_Real );
		m_jobCategoryList->SetSorting( true );
		classStatsTab->AddChild( m_jobCategoryList );
	}

	tabs->SetActiveTab( 0 );
}

CDebugWindowLoadingJobs::~CDebugWindowLoadingJobs()
{
}

void CDebugWindowLoadingJobs::OnWindowOpened( CRedGuiControl* control )
{
	GRedGui::GetInstance().EventTick.Bind( this, &CDebugWindowLoadingJobs::NotifyOnTick );

	RefreshCategoryList();

	SJobManager::GetInstance().AttachMonitor( this );
}

void CDebugWindowLoadingJobs::OnWindowClosed( CRedGuiControl* control )
{
	GRedGui::GetInstance().EventTick.Unbind( this, &CDebugWindowLoadingJobs::NotifyOnTick );

	SJobManager::GetInstance().AttachMonitor( nullptr );
}

void CDebugWindowLoadingJobs::NotifyOnTick( RedGui::CRedGuiEventPackage& eventPackage, Float deltaTime )
{
	RefreshCategoryList();
}

void CDebugWindowLoadingJobs::NotifyOnResetStats( RedGui::CRedGuiEventPackage& eventPackage )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_accessMutex );

	for ( auto it = m_categories.Begin(); it != m_categories.End(); ++it )
	{
		(*it).m_second->Reset();
	}

	RefreshCategoryList();
}

CDebugWindowLoadingJobs::JobCategory* CDebugWindowLoadingJobs::MapCategory( const Char* name )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_accessMutex );

	JobCategory* ret = nullptr;
	m_categories.Find( (void*)name, ret );

	if ( !ret )
	{
		ret = new CDebugWindowLoadingJobs::JobCategory( name );
		m_categories.Insert( (void*)name, ret );
	}

	return ret;
}

CDebugWindowLoadingJobs::JobInfo* CDebugWindowLoadingJobs::MapJob( const ILoadJob* job )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_accessMutex );

	JobInfo* ret = nullptr;
	m_jobs.Find( (void*)job, ret );

	if ( !ret )
	{
		JobCategory* category = MapCategory( job->GetDebugName() );

		ret = new JobInfo( job, category );
		m_jobs.Insert( (void*) job, ret );
	}

	return ret;
}

void CDebugWindowLoadingJobs::OnJobIssued( class ILoadJob* job )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_accessMutex );

	JobInfo* ret = MapJob( job );
	if ( ret )
	{
		ret->m_wasQueued = true;
		ret->m_category->m_queuedCount += 1;
	}
}

void CDebugWindowLoadingJobs::OnJobStarted( class ILoadJob* job )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_accessMutex );

	JobInfo* ret = MapJob( job );
	if ( ret )
	{
		if ( ret->m_wasQueued )
			ret->m_category->m_queuedCount -= 1;

		ret->m_startTime.SetNow();
		ret->m_state = JOB_Processing;
	}
}

void CDebugWindowLoadingJobs::OnJobFinished( class ILoadJob* job, const EJobResult result )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_accessMutex );

	JobInfo* ret = MapJob( job );
	if ( ret )
	{
		ret->m_finishTime.SetNow();

		const Double jobTime = (ret->m_finishTime - ret->m_startTime);
		ret->m_category->m_time += jobTime;
		//ret->m_category->m_size += job->GetDebugDataSize();

		if ( result == JR_Failed )
		{
			ret->m_state = JOB_Failed;
			ret->m_category->m_failedCount += 1;
		}
		else
		{
			ret->m_state = JOB_Finished;
			ret->m_category->m_finishedCount += 1;
		}
	}
}

void CDebugWindowLoadingJobs::OnJobCanceled( class ILoadJob* job )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_accessMutex );

	JobInfo* ret = MapJob( job );
	if ( ret )
	{
		ret->m_startTime.SetNow();
		ret->m_state = JOB_Canceled;
	}
}

void CDebugWindowLoadingJobs::JobCategory::UpdateCaption()
{
	m_item->SetText( String::Printf( TXT("%d"), m_queuedCount ), 1 );
	m_item->SetText( String::Printf( TXT("%d"), m_finishedCount ), 2 );
	m_item->SetText( String::Printf( TXT("%d"), m_failedCount ), 3 );
	//m_item->SetText( String::Printf( TXT("%1.2f"), m_size / (1024.0f*1024.0f) ), 4 );
	m_item->SetText( String::Printf( TXT("%1.2f"), m_time * 1000.0 ), 5 );
}

void CDebugWindowLoadingJobs::RefreshCategoryList()
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_accessMutex );

	// create missing items, update existing ones
	for ( auto it = m_categories.Begin(); it != m_categories.End(); ++it )
	{
		JobCategory* info = (*it).m_second;

		if ( !info->m_item )
		{
			info->m_item = new RedGui::CRedGuiListItem( info->m_name );
			m_jobCategoryList->AddItem( info->m_item );
		}

		info->UpdateCaption();
	}
}

} // DebugWindows

#endif
#endif