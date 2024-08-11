/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "profiler.h"

#include "..\..\common\core\functionFlags.h"

wxIMPLEMENT_CLASS( CSSProfiler, wxPanel );

BEGIN_EVENT_TABLE( CSSProfiler, wxPanel )
	EVT_LIST_ITEM_ACTIVATED( XRCID("FunctionList"), CSSProfiler::OnFunctionActivated )
	EVT_LIST_ITEM_SELECTED( XRCID("FunctionList"), CSSProfiler::OnFunctionSelected )
	EVT_LIST_ITEM_ACTIVATED( XRCID("CallsList"), CSSProfiler::OnCalledFunctionActivated )
	EVT_LIST_ITEM_ACTIVATED( XRCID("CalledByList"), CSSProfiler::OnCallingFunctionActivated )
	EVT_LIST_COL_CLICK( XRCID("FunctionList"), CSSProfiler::OnSortColumn )
	EVT_LIST_COL_CLICK( XRCID("CallsList"), CSSProfiler::OnSortColumn )
	EVT_LIST_COL_CLICK( XRCID("CalledByList"), CSSProfiler::OnSortColumn )
END_EVENT_TABLE()

CSSProfiler::CSSProfiler( wxWindow* parent )
	: wxPanel()
	, m_functionList( NULL )
	, m_callersList( NULL )
	, m_callsList( NULL )
{
	// Load panel
	wxXmlResource::Get()->LoadPanel( this, parent, wxT("ProfilerFrame") );

	// Get controls
	m_functionList = (wxListView*) XRCCTRL( *this, "FunctionList", wxListCtrl );
	m_callsList = (wxListView*) XRCCTRL( *this, "CallsList", wxListCtrl );
	m_callersList = (wxListView*) XRCCTRL( *this, "CalledByList", wxListCtrl );

	// Update splitters
	wxSplitterWindow* topSplitter = XRCCTRL( *this, "SplitterTop", wxSplitterWindow );
	topSplitter->SetSashPosition( -140 );
	wxSplitterWindow* bottomSplitter = XRCCTRL( *this, "SplitterBottom", wxSplitterWindow );
	bottomSplitter->SetSashPosition( -140 );

	// Update headers
	m_functionList->InsertColumn( STATS_COLUMN_NAME, wxT("Function name"), wxLIST_FORMAT_LEFT, 300 );
	m_functionList->InsertColumn( STATS_COLUMN_CLASS, wxT("Class"), wxLIST_FORMAT_LEFT, 150 );
	m_functionList->InsertColumn( STATS_COLUMN_CALLS, wxT("Calls"), wxLIST_FORMAT_LEFT, 80 );
	m_functionList->InsertColumn( STATS_COLUMN_INTICKS,	wxT("Incl ticks"), wxLIST_FORMAT_LEFT, 120 );
	m_functionList->InsertColumn( STATS_COLUMN_EXTICKS,	wxT("Excl ticks"), wxLIST_FORMAT_LEFT, 120 );
	m_functionList->InsertColumn( STATS_COLUMN_PRCIN, wxT("% Inclusive"), wxLIST_FORMAT_LEFT, 120 );
	m_functionList->InsertColumn( STATS_COLUMN_PRCEX, wxT("% Exclusive"), wxLIST_FORMAT_LEFT, 120 );
	m_functionList->InsertColumn( STATS_COLUMN_INPERCALL, wxT("In/Call"), wxLIST_FORMAT_LEFT, 80 );
	m_functionList->InsertColumn( STATS_COLUMN_EXPERCALL, wxT("Ex/Call"), wxLIST_FORMAT_LEFT, 80 );
	m_functionList->InsertColumn( STATS_COLUMN_COMMENT, wxT("Comment"), wxLIST_FORMAT_LEFT, 80 );

	// Update headers
	m_callsList->InsertColumn( STATS_COLUMN_NAME, wxT("Function name"), wxLIST_FORMAT_LEFT, 300 );
	m_callsList->InsertColumn( STATS_COLUMN_CLASS, wxT("Class"), wxLIST_FORMAT_LEFT, 150 );
	m_callsList->InsertColumn( STATS_COLUMN_CALLS, wxT("Calls"), wxLIST_FORMAT_LEFT, 80 );
	m_callsList->InsertColumn( STATS_COLUMN_INTICKS, wxT("Incl ticks"), wxLIST_FORMAT_LEFT, 120 );
	m_callsList->InsertColumn( STATS_COLUMN_EXTICKS, wxT("Excl ticks"), wxLIST_FORMAT_LEFT, 120 );
	m_callsList->InsertColumn( STATS_COLUMN_PRCIN, wxT("% Inclusive"), wxLIST_FORMAT_LEFT, 120 );
	m_callsList->InsertColumn( STATS_COLUMN_PRCEX, wxT("% Exclusive"), wxLIST_FORMAT_LEFT, 120 );
	m_callsList->InsertColumn( STATS_COLUMN_INPERCALL, wxT("In/Call"), wxLIST_FORMAT_LEFT, 80 );
	m_callsList->InsertColumn( STATS_COLUMN_EXPERCALL, wxT("Ex/Call"), wxLIST_FORMAT_LEFT, 80 );
	m_callsList->InsertColumn( STATS_COLUMN_COMMENT, wxT("Comment"), wxLIST_FORMAT_LEFT, 80 );

	// Update headers
	m_callersList->InsertColumn( STATS_COLUMN_NAME, wxT("Function name"), wxLIST_FORMAT_LEFT, 300 );
	m_callersList->InsertColumn( STATS_COLUMN_CLASS, wxT("Class"), wxLIST_FORMAT_LEFT, 150 );
	m_callersList->InsertColumn( STATS_COLUMN_CALLS, wxT("Calls"), wxLIST_FORMAT_LEFT, 80 );
	m_callersList->InsertColumn( STATS_COLUMN_INTICKS, wxT("Incl ticks"), wxLIST_FORMAT_LEFT, 120 );
	m_callersList->InsertColumn( STATS_COLUMN_EXTICKS, wxT("Excl ticks"), wxLIST_FORMAT_LEFT, 120 );
	m_callersList->InsertColumn( STATS_COLUMN_PRCIN, wxT("% Inclusive"), wxLIST_FORMAT_LEFT, 120 );
	m_callersList->InsertColumn( STATS_COLUMN_PRCEX, wxT("% Exclusive"), wxLIST_FORMAT_LEFT, 120 );
	m_callersList->InsertColumn( STATS_COLUMN_INPERCALL, wxT("In/Call"), wxLIST_FORMAT_LEFT, 80 );
	m_callersList->InsertColumn( STATS_COLUMN_EXPERCALL, wxT("Ex/Call"), wxLIST_FORMAT_LEFT, 80 );
	m_callersList->InsertColumn( STATS_COLUMN_COMMENT, wxT("Comment"), wxLIST_FORMAT_LEFT, 80 );

	// Done
	Layout();
	Show();
}

CSSProfiler::~CSSProfiler()
{
	// Delete profiling data
	ClearVector( m_functions );
}

void CSSProfiler::UpdateFunctionList()
{
	// Being update
	m_functionList->Freeze();
	m_functionList->DeleteAllItems();

	// Add functions
	for ( size_t i=0; i<m_functions.size(); i++ )
	{
		StatFunction* func = m_functions[i];
		long itemIndex = m_functionList->InsertItem( i, wxEmptyString );
		SetupFunctionLine( m_functionList, itemIndex, func );
	}
	
	// End update
	m_functionList->Thaw();
	m_functionList->Refresh();

}

void CSSProfiler::UpdateCallersList( StatFunction* func )
{
	// Callers list
	if ( func )
	{
		// Being update
		m_callersList->Freeze();
		m_callersList->DeleteAllItems();

		// Get function that called selected function
		vector< StatFunction* > callers;
		for ( size_t i=0; i<m_functions.size(); i++ )
		{
			StatFunction* testFunc = m_functions[i];

			// Check
			bool hasIt = false;
			for ( size_t j=0; j<testFunc->m_called.size(); j++ )
			{
				if ( testFunc->m_called[j] == func )
				{
					hasIt = true;
					break;
				}
			}

			// Considered function was called from this function
			if ( hasIt )
			{
				callers.push_back( testFunc );
			}
		}

		// Update list
		for ( size_t i=0; i<callers.size(); i++ )
		{
			long itemIndex = m_callersList->InsertItem( i, wxEmptyString );
			SetupFunctionLine( m_callersList, itemIndex, callers[i] );
		}

		// End update
		m_callersList->Thaw();
		m_callersList->Refresh();
	}
}

void CSSProfiler::UpdateCallsList( StatFunction* func )
{
	if( !func )
	{
		return;
	}

	// Being update
	m_callsList->Freeze();
	m_callsList->DeleteAllItems();

	// Update list
	for ( size_t i = 0; i < func->m_called.size(); ++i )
	{
		long itemIndex = m_callsList->InsertItem( i, wxEmptyString );
		SetupFunctionLine( m_callsList, itemIndex, func->m_called[i] );
	}

	// End update
	m_callsList->Thaw();
	m_callsList->Refresh();
}

void CSSProfiler::SetupFunctionLine( wxListCtrl* ctrl, long index, StatFunction* func )
{
	// Standard data
	ctrl->SetItem( index, 0, func->m_name );
	ctrl->SetItem( index, 1, func->m_class );
	ctrl->SetItem( index, 2, wxString::Format( wxT("%i"), func->m_numCalls ) );
	ctrl->SetItem( index, 3, wxString::Format( wxT("%i"), func->m_numTicksIn ) );
	ctrl->SetItem( index, 4, wxString::Format( wxT("%i"), func->m_numTicksEx ) );
	ctrl->SetItem( index, 5, wxString::Format( wxT("%1.2f%%"), func->m_prcIn * 100.0f ) );
	ctrl->SetItem( index, 6, wxString::Format( wxT("%1.2f%%"), func->m_prcEx * 100.0f ) );
	ctrl->SetItem( index, 7, wxString::Format( wxT("%i"), func->m_numTicksPerCallIn ) );
	ctrl->SetItem( index, 8, wxString::Format( wxT("%i"), func->m_numTicksPerCallEx ) );

	// String
	wxString flags = wxEmptyString;
	if ( func->m_flags & FF_NativeFunction ) 
	{
		if ( !flags.IsEmpty() ) flags += wxT(", ");
		flags += wxT("Native");
	}
	if ( func->m_flags & FF_EntryFunction )
	{
		if ( !flags.IsEmpty() ) flags += wxT(", ");
		flags += wxT("Entry");
	}
	if ( func->m_flags & FF_LatentFunction )
	{
		if ( !flags.IsEmpty() ) flags += wxT(", ");
		flags += wxT("Latent");
	}
	if ( func->m_flags & FF_EventFunction )
	{
		if ( !flags.IsEmpty() ) flags += wxT(", ");
		flags += wxT("Event");
	}
	if ( func->m_flags & FF_TimerFunction )
	{
		if ( !flags.IsEmpty() ) flags += wxT(", ");
		flags += wxT("Timer");
	}
	if ( func->m_flags & FF_CleanupFunction )
	{
		if ( !flags.IsEmpty() ) flags += wxT(", ");
		flags += wxT("Cleanup");
	}
	if ( func->m_flags & FF_SceneFunction )
	{
		if ( !flags.IsEmpty() ) flags += wxT(", ");
		flags += wxT("Scene");
	}
	if ( func->m_flags & FF_QuestFunction )
	{
		if ( !flags.IsEmpty() ) flags += wxT(", ");
		flags += wxT("Quest");
	}
	if( func->m_flags & FF_RewardFunction )
	{
		if ( !flags.IsEmpty() ) flags += wxT(", ");
		flags += wxT("Reward");
	}

	ctrl->SetItem( index, 9, flags );

	// Link with function
	ctrl->SetItemPtrData( index, (wxUIntPtr) func );
}

CSSProfiler::StatFunction* CSSProfiler::GetSelectedFunction( wxListCtrl* ctrl )
{
	// Get selected function
	long selected = m_functionList->GetFirstSelected();
	if ( selected != -1 )
	{
		return (StatFunction*)m_functionList->GetItemData( selected );
	}

	// Not found
	return NULL;
}

int CSSProfiler::FindFunctionIndex( StatFunction* func, wxListCtrl* ctrl )
{
	size_t n = ctrl->GetItemCount();
	for ( size_t i=0; i<n; i++ )
	{
		if ( func == (StatFunction*)ctrl->GetItemData( i ) )
		{
			return i;
		}
	}

	return -1;
}


void CSSProfiler::OnFunctionSelected( wxListEvent& event )
{
	StatFunction* func = GetSelectedFunction( m_functionList );
	if ( func )
	{
		// Update callers list
		UpdateCallersList( func );

		// Update called list
		UpdateCallsList( func );
	}
}

void CSSProfiler::OnFunctionActivated( wxListEvent& event )
{

}

void CSSProfiler::OnCalledFunctionActivated( wxListEvent& event )
{
	StatFunction* func = GetSelectedFunction( m_callsList );
	if ( func )
	{
		int index = FindFunctionIndex( func, m_functionList );
		if ( index != -1 )
		{
			// Select function in the main function list
			m_functionList->Select( index );
		}
	}
}

void CSSProfiler::OnCallingFunctionActivated( wxListEvent& event )
{
	StatFunction* func = GetSelectedFunction( m_callersList );
	if ( func )
	{
		int index = FindFunctionIndex( func, m_functionList );
		if ( index != -1 )
		{
			// Select function in the main function list
			m_functionList->Select( index );
		}
	}
}

enum
{
	SORT_FUNCTIONLIST,
	SORT_CALLSLIST,
	SORT_CALLERSLIST,
	SORT_INVALIDARG
};

void CSSProfiler::OnSortColumn( wxListEvent& event )
{
	int sortType = SORT_INVALIDARG;
	StatFunction* func = NULL;

	if( event.GetId() == XRCID("FunctionList") )
	{
		sortType = SORT_FUNCTIONLIST;
	}
	else
	{
		func = GetSelectedFunction( m_functionList );
		if( !func )
		{
			return;
		}

		if( event.GetId() == XRCID("CallsList") )
		{
			sortType = SORT_CALLSLIST;
		}
		else if( event.GetId() == XRCID("CalledByList") )
		{
			sortType = SORT_CALLERSLIST;
		}
	}

	if( sortType == SORT_INVALIDARG )
	{
		return;
	}

	switch( event.GetColumn() )
	{

	case STATS_COLUMN_NAME:
		if( sortType == SORT_CALLSLIST )
		{
			sort( func->m_called.begin(), func->m_called.end(), &NameComparator );
		}
		else
		{
			sort( m_functions.begin(), m_functions.end(), &NameComparator );
		}
		break;

	case STATS_COLUMN_CLASS:
		if( sortType == SORT_CALLSLIST )
		{
			sort( func->m_called.begin(), func->m_called.end(), &ClassComparator );
		}
		else
		{
			sort( m_functions.begin(), m_functions.end(), &ClassComparator );
		}
		break;

	case STATS_COLUMN_CALLS:
		if( sortType == SORT_CALLSLIST )
		{
			sort( func->m_called.begin(), func->m_called.end(), &CallsComparator );
		}
		else
		{
			sort( m_functions.begin(), m_functions.end(), &CallsComparator );
		}
		break;

	case STATS_COLUMN_INTICKS:
		if( sortType == SORT_CALLSLIST )
		{
			sort( func->m_called.begin(), func->m_called.end(), &InTicksComparator );
		}
		else
		{
			sort( m_functions.begin(), m_functions.end(), &InTicksComparator );
		}
		break;

	case STATS_COLUMN_EXTICKS:
		if( sortType == SORT_CALLSLIST )
		{
			sort( func->m_called.begin(), func->m_called.end(), &ExTicksComparator );
		}
		else
		{
			sort( m_functions.begin(), m_functions.end(), &ExTicksComparator );
		}
		break;

	case STATS_COLUMN_PRCIN:
		if( sortType == SORT_CALLSLIST )
		{
			sort( func->m_called.begin(), func->m_called.end(), &PrcInComparator );
		}
		else
		{
			sort( m_functions.begin(), m_functions.end(), &PrcInComparator );
		}
		break;

	case STATS_COLUMN_PRCEX:
		if( sortType == SORT_CALLSLIST )
		{
			sort( func->m_called.begin(), func->m_called.end(), &PrcExComparator );
		}
		else
		{
			sort( m_functions.begin(), m_functions.end(), &PrcExComparator );
		}
		break;

	case STATS_COLUMN_INPERCALL:
		if( sortType == SORT_CALLSLIST )
		{
			sort( func->m_called.begin(), func->m_called.end(), &InPerCallComparator );
		}
		else
		{
			sort( m_functions.begin(), m_functions.end(), &InPerCallComparator );
		}
		break;

	case STATS_COLUMN_EXPERCALL:
		if( sortType == SORT_CALLSLIST )
		{
			sort( func->m_called.begin(), func->m_called.end(), &ExPerCallComparator );
		}
		else
		{
			sort( m_functions.begin(), m_functions.end(), &ExPerCallComparator );
		}
		break;

	case STATS_COLUMN_COMMENT:
		if( sortType == SORT_CALLSLIST )
		{
			sort( func->m_called.begin(), func->m_called.end(), &CommentComparator );
		}
		else
		{
			sort( m_functions.begin(), m_functions.end(), &CommentComparator );
		}
		break;

	default:
		return;

	}

	switch( sortType )
	{
	case SORT_FUNCTIONLIST:
		UpdateFunctionList();
		break;

	case SORT_CALLSLIST:
		UpdateCallsList( func );
		break;

	case SORT_CALLERSLIST:
		UpdateCallersList( func );
		break;
	}
}

bool CSSProfiler::NameComparator( StatFunction* func1, StatFunction* func2 )
{
	return func1->m_name.CmpNoCase( func2->m_name ) < 0;
}

bool CSSProfiler::ClassComparator( StatFunction* func1, StatFunction* func2 )
{
	return func1->m_class.CmpNoCase( func2->m_class ) < 0;
}

bool CSSProfiler::CallsComparator( StatFunction* func1, StatFunction* func2 )
{
	return func1->m_numCalls > func2->m_numCalls;
}

bool CSSProfiler::InTicksComparator( StatFunction* func1, StatFunction* func2 )
{
	return func1->m_numTicksIn > func2->m_numTicksIn;
}

bool CSSProfiler::ExTicksComparator( StatFunction* func1, StatFunction* func2 )
{
	return func1->m_numTicksEx > func2->m_numTicksEx;
}

bool CSSProfiler::PrcInComparator( StatFunction* func1, StatFunction* func2 )
{
	return func1->m_prcIn > func2->m_prcIn;
}

bool CSSProfiler::PrcExComparator( StatFunction* func1, StatFunction* func2 )
{
	return func1->m_prcEx > func2->m_prcEx;
}

bool CSSProfiler::InPerCallComparator( StatFunction* func1, StatFunction* func2 )
{
	return func1->m_numTicksPerCallIn > func2->m_numTicksPerCallIn;
}

bool CSSProfiler::ExPerCallComparator( StatFunction* func1, StatFunction* func2 )
{
	return func1->m_numTicksPerCallEx > func2->m_numTicksPerCallEx;
}

bool CSSProfiler::CommentComparator( StatFunction* func1, StatFunction* func2 )
{
	return func1->m_flags > func2->m_flags;
}

void CSSProfiler::SetData( SProfileData* data, Red::System::Uint32 size )
{
	ClearVector( m_functions );

	m_functions.reserve( size );

	for( Red::System::Uint32 i = 0; i < size; ++i )
	{
		StatFunction* func = new StatFunction();

		m_functions.push_back( func );
	}

	for( Red::System::Uint32 i = 0; i < size; ++i )
	{
		StatFunction* func = m_functions[ i ];

		func->m_class			= data[ i ].m_class;
		func->m_name			= data[ i ].m_function;
		func->m_numCalls		= data[ i ].m_numCalls;
		func->m_flags			= data[ i ].m_flags;
		func->m_numTicksIn		= data[ i ].m_totalTimeIn;
		func->m_numTicksEx		= data[ i ].m_totalTimeEx;
		func->m_prcIn			= data[ i ].m_prcIn;
		func->m_prcEx			= data[ i ].m_prcEx;
		func->m_recursionLevel	= data[ i ].m_recursion;

		func->m_numTicksPerCallIn = func->m_numTicksIn / func->m_numCalls;
		func->m_numTicksPerCallEx = func->m_numTicksEx / func->m_numCalls;

		func->m_called.reserve( data[ i ].m_numIndicies );

		for( Red::System::Uint32 j = 0; j < data[ i ].m_numIndicies; ++j )
		{
			if( data[ i ].m_indicies[ j ] > 0 && data[ i ].m_indicies[ j ] < static_cast< Red::System::Int32 >( size ) )
			{
				func->m_called.push_back( m_functions[ data[ i ].m_indicies[ j ] ] );
			}
		}
	}

	UpdateFunctionList();
}
