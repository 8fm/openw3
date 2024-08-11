/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "animationReporterTodoList.h"
#include "animationReporter.h"

EdAnimationReporterTodoList::EdAnimationReporterTodoList()
{

}

EdAnimationReporterTodoList::~EdAnimationReporterTodoList()
{
	Clear();
}

void EdAnimationReporterTodoList::Sort( ETODOViewSort sortType )
{
	switch ( sortType )
	{
	case TDVS_Prio:
		qsort( m_tasks.TypedData(), m_tasks.Size(), sizeof( EdAnimationReporterTodoTask* ), &EdAnimationReporterTodoTask::CmpFuncByPrio );
		break;
	case TDVS_Type:
		qsort( m_tasks.TypedData(), m_tasks.Size(), sizeof( EdAnimationReporterTodoTask* ), &EdAnimationReporterTodoTask::CmpFuncByType );
		break;
	case TDVS_Category:
		qsort( m_tasks.TypedData(), m_tasks.Size(), sizeof( EdAnimationReporterTodoTask* ), &EdAnimationReporterTodoTask::CmpFuncByCat );
		break;
	}
}

void EdAnimationReporterTodoList::Clear()
{
	m_tasks.ClearPtr();
}

void EdAnimationReporterTodoList::AddTask( EdAnimationReporterTodoTask* task )
{
	for ( Uint32 i=0; i<m_tasks.Size(); ++i )
	{
		EdAnimationReporterTodoTask* t = m_tasks[ i ];
		if ( task->IsEqual( t ) )
		{
			t->AddCount();
			delete task;
			return;
		}
	}
	m_tasks.PushBack( task );
}

Uint32 EdAnimationReporterTodoList::GetTaskNum() const
{
	return m_tasks.Size();
}

const EdAnimationReporterTodoTask* EdAnimationReporterTodoList::GetTask( Uint32 i ) const
{
	return m_tasks[ i ];
}

EdAnimationReporterTodoTask* EdAnimationReporterTodoList::CreateTask( Uint32 type ) const
{
	switch ( type )
	{
	case ART_MissingResource:
		return new EdAnimationReporterMissingResource();
	case ART_EmptySlot:
		return new EdAnimationReporterEmptySlot();
	case ART_DuplicatedAnimation:
		return new EdAnimationReporterDuplicatedAnimation();
	case ART_DuplicatedBehaviorSlots:
		return new EdAnimationReporterDuplicatedBehaviorSlots();
	case ART_AnimNotCompressed:
		return new EdAnimationReporterAnimNotCompressed();
	case ART_ScriptPlaySlotAnim:
		return new EdAnimationReporterScriptPlaySlotAnim();
	default:
		ASSERT( 0 );
		return NULL;
	}
}

void EdAnimationReporterTodoList::Serialize( IFile& file )
{
	if ( file.IsReader() )
	{
		Uint32 size = 0;
		file << size;

		m_tasks.Resize( size );

		for ( Uint32 i=0; i<size; ++i )
		{
			Uint32 type = 0;
			file << type;

			EdAnimationReporterTodoTask* task = CreateTask( type );
			task->Serialize( file );

			m_tasks[ i ] = task;
		}
	}
	else
	{
		Uint32 size = m_tasks.Size();
		file << size;

		for ( Uint32 i=0; i<size; ++i )
		{
			EdAnimationReporterTodoTask* task = m_tasks[ i ];

			Uint32 type = (Uint32)task->GetType();
			file << type;

			task->Serialize( file );
		}
	}
}

Uint32 EdAnimationReporterTodoList::GetErrorsNum() const
{
	return GetNumByType( ARTT_Error );
}

Uint32 EdAnimationReporterTodoList::GetWarnsNum() const
{
	return GetNumByType( ARTT_Warn );
}

Uint32 EdAnimationReporterTodoList::GetChecksNum() const
{
	return GetNumByType( ARTT_Check );
}

Uint32 EdAnimationReporterTodoList::GetInfosNum() const
{
	return GetNumByType( ARTT_Info );
}

Uint32  EdAnimationReporterTodoList::GetNumByType( EAnimationReporterTaskType type ) const
{
	Uint32 sum = 0;

	for ( Uint32 i=0; i<m_tasks.Size(); ++i )
	{
		if ( m_tasks[ i ]->GetTaskType() == type )
		{
			sum++;
		}
	}

	return sum;
}

//////////////////////////////////////////////////////////////////////////

void CEdAnimationReporterWindow::RefreshTodoList()
{
	wxString html;

	html += wxT("<table border=1>");

	wxString s1 = m_todoSort == TDVS_Prio ?		wxT("Sort") : wxT("<a href=\"SortPrio\">Sort</a>");
	wxString s2 = m_todoSort == TDVS_Type ?		wxT("Sort") : wxT("<a href=\"SortType\">Sort</a>");
	wxString s3 = m_todoSort == TDVS_Category ?	wxT("Sort") : wxT("<a href=\"SortCat\">Sort</a>");

	html += wxT("<tr>");
	html += wxString::Format( wxT("<th align=center><i>Priority </i>%s</th>"), s1.wc_str() );
	html += wxString::Format( wxT("<th width=100 align=center><i>Type </i>%s</th>"), s2.wc_str() );
	html += wxString::Format( wxT("<th width=100 align=center><i>Category </i>%s</th>"), s3.wc_str() );
	html += wxString::Format( wxT("<th width=100 align=center><i>Count</i></th>") );
	html += wxString::Format( wxT("<th align=left><i>Desc</i></th>") );
	html += wxString::Format( wxT("<th align=left><i>Link</i></th>") );
	html += wxT("</tr>");

	m_todoList.Sort( m_todoSort );

	for ( Uint32 i=0; i<m_todoList.GetTaskNum(); ++i )
	{
		const EdAnimationReporterTodoTask* task = m_todoList.GetTask( i );

		html += wxT("<tr>");
		html += wxString::Format( wxT("<th align=center>%s</th>"), task->GetPrioDesc().wc_str() );
		html += wxString::Format( wxT("<th width=100 align=center>%s</th>"), task->GetTypeDesc().wc_str() );
		html += wxString::Format( wxT("<th width=100 align=center>%s</th>"), task->GetCategoryDesc().wc_str() );
		html += wxString::Format( wxT("<th width=100 align=center>%d</th>"), task->GetCount() );
		html += wxString::Format( wxT("<th align=left>%s</th>"), task->GetDesc().wc_str() );
		html += wxString::Format( wxT("<th align=left>%s</th>"), task->GetLink().wc_str() );
		html += wxT("</tr>");
	}

	html += wxT("</table>");

	wxHtmlWindow* htmlWindow = XRCCTRL( *this, "todoHtml", wxHtmlWindow );
	htmlWindow->SetPage( html );
}

void CEdAnimationReporterWindow::OnTodoLinkClicked( wxHtmlLinkEvent& event )
{
	const wxHtmlLinkInfo& linkInfo = event.GetLinkInfo();
	wxString href = linkInfo.GetHref();

	if ( href == wxT("SortPrio") )
	{
		m_todoSort = TDVS_Prio;
		RefreshTodoList();
	}
	else if ( href == wxT("SortType") )
	{
		m_todoSort = TDVS_Type;
		RefreshTodoList();
	}
	else if ( href == wxT("SortCat") )
	{
		m_todoSort = TDVS_Category;
		RefreshTodoList();
	}
}
