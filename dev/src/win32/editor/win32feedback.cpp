/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "res/resource.h"
#include "selectFromListDialog.h"
#include "../../common/core/feedback.h"
#include "utils.h"

Bool GSilentFeedback = false;

#define LAST_TEXT_MAX_LENGTH 512
#define LAST_TITLE_MAX_LENGTH 512

class wxFrame* InspectObject( ISerializable* object, const String& tag );

// Win32 feedback system
class Win32Feedback : public IFeedbackSystem
{
protected:
	static const DWORD WM_USER_SHOWWINDOW = WM_USER + 1;
	static const DWORD WM_USER_SET_TEXT = WM_USER + 2;
	static const DWORD WM_USER_SET_PROGRESS = WM_USER + 3;
	static const DWORD WM_USER_SET_RANGE = WM_USER + 4;
	static const DWORD WM_USER_SET_NAME = WM_USER + 5;

	static const DWORD ID_ETA_TIMER = 12345;

protected:
	HWND					m_wndDialog;		//!< Handle to dialog box
	HWND					m_wndText;			//!< Handle to progress text
	HWND					m_wndButton;		//!< Handle to cancel button
	HWND					m_wndBar;			//!< Handle to progress bar
	HWND					m_wndTime;			//!< Handle to cancel button
	Uint32					m_lastRange;		//!< Last progress bar range
	Uint32					m_lastValue;		//!< Last progress bar value
	Char					m_lastText[ LAST_TEXT_MAX_LENGTH ];	//!< Last text to display
	Char					m_lastTitle[ LAST_TITLE_MAX_LENGTH ];	//!< Last title to display
	Int32					m_taskCount;		//!< Number of active slow tasks
	HANDLE					m_threadHandle;		//!< Handle to internal UI thread
	Bool					m_isTaskCanceled;	//!< Is the current task canceled
	Bool					m_canCancel;		//!< Can we cancel the task
	Red::Threads::CMutex	m_dataMutex;		//!< Access to data lock
	Uint32					m_countedSeconds;	//!< Seconds counted since start of the task
	volatile Bool			m_isVisible;		//!< Are we visible

public:
	Win32Feedback()
		: m_wndDialog( NULL )
		, m_wndText( NULL )
		, m_wndBar( NULL )
		, m_wndButton( NULL )
		, m_wndTime( NULL )
		, m_lastRange( 0 )
		, m_lastValue( 0 )
		, m_taskCount( 0 )
		, m_threadHandle( NULL )
		, m_isTaskCanceled( false )
		, m_canCancel( false )
		, m_countedSeconds( 0 )
		, m_isVisible( false )
	{
	}

	~Win32Feedback()
	{

	}

	virtual void BeginTask( const Char* name, Bool canCancel )
	{
		// Push the task
		ASSERT( m_taskCount >= 0 );
		m_taskCount++;

		// Show window
		if ( m_taskCount == 1 )
		{
			// Create thread and window on first use
			if ( !m_threadHandle )
			{
				// Create thread
				m_threadHandle = CreateThread( NULL, 0, &Win32Feedback::ThreadFunction, this, 0, NULL );

				// Wait until window is initialized
				while ( !m_wndDialog )
				{
					Sleep( 100 );
				}
			}

			// Update the cancel flags
			m_canCancel = canCancel;
			m_isTaskCanceled = false;

			// Start timer
			m_countedSeconds = 0;

			// Set defaults
			Red::System::StringCopy( m_lastTitle, name, LAST_TITLE_MAX_LENGTH );

			// Show window
			PostMessage( m_wndDialog, WM_USER_SHOWWINDOW, 1, 0 );
		}
	}

	virtual void EndTask()
	{
		// Pop the task
		ASSERT( m_taskCount > 0 );
		--m_taskCount;
		
		// Hide dialog
		if ( m_taskCount == 0 )
		{
			PostMessage( m_wndDialog, WM_USER_SHOWWINDOW, 0, 0 );
			while ( m_isVisible )
			{
				Sleep( 100 );
			}
		}
	}

	virtual void UpdateTaskProgress( Uint32 current, Uint32 total )
	{
		if ( m_taskCount && m_wndBar )
		{
			// Make sure the values are small to fit in Win32's 16bit restriction (fix this properly later)
			current = total > 0 ? 10000*current / total : 0;
			total = 10000;

			// Update range
			if ( m_lastRange != total )
			{
				Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_dataMutex );

				m_lastRange = total;
				PostMessage( m_wndDialog, WM_USER_SET_RANGE, 0, 0 ) ;
			}

			// Update position
			if ( m_lastValue != current )
			{
				Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_dataMutex );

				m_lastValue = current;
				PostMessage( m_wndDialog, WM_USER_SET_PROGRESS, 0, 0 ) ;
			}
		}
	}

	virtual void UpdateTaskInfo( const Char* info, ... )
	{
		if ( m_taskCount && m_wndText )
		{
			Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_dataMutex );

			// Print the text
			va_list arglist;
			va_start(arglist, info);
			Red::System::VSNPrintF( m_lastText, ARRAY_COUNT(m_lastText), info, arglist ); 

			// Update text
			PostMessage( m_wndDialog, WM_USER_SET_TEXT, 0, 0 );
		}
	}

	virtual Bool IsTaskCanceled()
	{
		return m_isTaskCanceled;
	}

	//! Ask user a yes/no question
	virtual Bool AskYesNo( const Char* info, ... )
	{
		Char buffer[ 8192 ];

		// Print the text
		va_list arglist;
		va_start(arglist, info);
		Red::System::VSNPrintF( buffer, ARRAY_COUNT(buffer), info, arglist ); 

		// Show message box
		if ( GSilentFeedback )
		{
			LOG_EDITOR( TXT("<<<< UserQuestion: %s"), buffer );
			return false;
		}
		else
		{
			return IDYES == ::MessageBox( ::GetForegroundWindow(), buffer, TXT("Question"), MB_TASKMODAL | MB_TOPMOST | MB_ICONQUESTION | MB_YESNO );
		}
	}

	//! Ask user a yes/no/cancel question. 
	virtual EFeedbackYesNoCancelResult AskYesNoCancel( const Char* info, ... )
	{
		Char buffer[ 8192 ];

		// Print the text
		va_list arglist;
		va_start(arglist, info);
		Red::System::VSNPrintF( buffer, ARRAY_COUNT(buffer), info, arglist ); 

		// Show message box
		if ( GSilentFeedback )
		{
			LOG_EDITOR( TXT("<<<< UserQuestion: %s"), buffer );
			return FeedbackCancel;
		}
		else
		{
			int res = ::MessageBox( ::GetForegroundWindow(), buffer, TXT("Question"), MB_TASKMODAL | MB_TOPMOST | MB_ICONQUESTION | MB_YESNOCANCEL );
			if( res == IDYES )
			{
				return FeedbackYes;
			}
			else if( res == IDNO )
			{
				return FeedbackNo;
			}
			else // cancel
			{
				return FeedbackCancel;
			}
		}
	}

	//! Show message to user
	virtual void ShowMsg( const Char* title, const Char* msg, ... )
	{
		Char buffer[ 8192 ];

		// Print the text
		va_list arglist;
		va_start(arglist, msg);
		Red::System::VSNPrintF( buffer, ARRAY_COUNT(buffer), msg, arglist ); 

		// Show message box
		if ( GSilentFeedback )
		{
			LOG_EDITOR( TXT("<<<< UserMsg: %s"), buffer );
		}
		else
		{
			::MessageBox( ::GetForegroundWindow(), buffer, title, MB_TASKMODAL | MB_TOPMOST | MB_ICONINFORMATION );
		}
	}

	//! Show warning to user
	virtual void ShowWarn( const Char* msg, ... )
	{
		Char buffer[ 8192 ];

		// Print the text
		va_list arglist;
		va_start(arglist, msg);
		Red::System::VSNPrintF( buffer, ARRAY_COUNT(buffer), msg, arglist ); 

		// Show message box
		if ( GSilentFeedback )
		{
			LOG_EDITOR( TXT("<<<< UserWarn: %s"), buffer );
		}
		else
		{
			::MessageBox( ::GetForegroundWindow(), buffer, TXT("Warning"), MB_TASKMODAL | MB_TOPMOST | MB_ICONWARNING );
		}
	}

	//! Show error message to user
	virtual void ShowError( const Char* msg, ... )
	{
		Char buffer[ 8192 ];

		// Print the text
		va_list arglist;
		va_start(arglist, msg);
		Red::System::VSNPrintF( buffer, ARRAY_COUNT(buffer), msg, arglist ); 

		// Show message box
		if ( GSilentFeedback )
		{
			LOG_EDITOR( TXT("<<<< UserError: %s"), buffer );
		}
		else
		{
			::MessageBox( ::GetForegroundWindow(), buffer, TXT("Error"), MB_TASKMODAL | MB_TOPMOST | MB_ICONERROR );
		}
	}

	//! Ask the user to confirm a bunch of questions
	virtual Bool ShowMultiBoolDialog( const String& title, const TDynArray<String>& questions, TDynArray<Bool>& answers )
	{
		return MultiBoolDialog( wxTheApp->GetTopWindow(), title, questions, answers );
	}

	virtual void UpdateTaskName( const Char* name )
	{
		// Set name
		Red::System::StringCopy( m_lastTitle, name, LAST_TITLE_MAX_LENGTH );

		// Update window title
		PostMessage( m_wndDialog, WM_USER_SET_NAME, 0, 0 );
	}

protected:
	//! Dialog message processing function
	static INT_PTR CALLBACK DialogProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
	{
		// Dialog init
		if ( uMsg == WM_INITDIALOG )
		{
			LONG_PTR feedback = lParam;
			SetWindowLongPtr( hwndDlg, GWLP_USERDATA, feedback );
			return TRUE;
		}

		// Timer
		if ( uMsg == WM_TIMER && wParam == ID_ETA_TIMER )
		{
			Win32Feedback* feedback = ( Win32Feedback* ) GetWindowLongPtr( hwndDlg, GWLP_USERDATA );
			if ( feedback )
			{
				// Count time
				feedback->m_countedSeconds++;

				// Format string
				const Int32 min = feedback->m_countedSeconds / 60;
				const Int32 sec = feedback->m_countedSeconds % 60;
				String text = String::Printf( TXT("Time: %02i:%02i"), min, sec );

				// Update string
				SendMessage( feedback->m_wndTime, WM_SETTEXT, 0, ( LPARAM ) text.AsChar() );
				return TRUE;
			}			
		}

		// Cancel button pressed
		if ( uMsg == WM_COMMAND && LOWORD(wParam) == IDCANCEL )
		{
			Win32Feedback* feedback = ( Win32Feedback* ) GetWindowLongPtr( hwndDlg, GWLP_USERDATA );
			if ( feedback && feedback->m_canCancel )
			{
				// Cancel
				feedback->m_isTaskCanceled = true;

				// Update display
				SendMessage( feedback->m_wndText, WM_SETTEXT, 0, ( LPARAM ) TXT("Canceling...") );
			}
			return TRUE;
		}

		// Do not process
		return FALSE;
	}

	//! Thread function
	static DWORD WINAPI ThreadFunction( LPVOID lpParam )
	{
		// Get the feedback system instance
		Win32Feedback* feedback = ( Win32Feedback* ) lpParam;

		// Change priority to allow easy ui update
		SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL );

		// Create window
		feedback->ConditionalInitDialog();

		// Process messages
		MSG msg;
		while ( GetMessage( &msg, feedback->m_wndDialog, 0, 0 ) )
		{
			// Show/Hide window
			if ( msg.message == WM_USER_SHOWWINDOW )
			{
				if ( msg.wParam != 0 )
				{
					// Set initial data
					SendMessage( feedback->m_wndDialog, WM_SETTEXT, 0, (LPARAM)feedback->m_lastTitle );
					SendMessage( feedback->m_wndText, WM_SETTEXT, 0, (LPARAM)TXT("Please wait...") );
					SendMessage( feedback->m_wndTime, WM_SETTEXT, 0, (LPARAM)TXT("Time: 00:00") );

					// Enable/Disable cancel button
					EnableWindow( feedback->m_wndButton, feedback->m_canCancel );

					// Show the window
					ShowWindow( feedback->m_wndDialog, SW_SHOWNA );
					SetForegroundWindow( feedback->m_wndDialog );

					// Start time
					SetTimer( feedback->m_wndDialog, ID_ETA_TIMER, 1000, NULL );

					// Visible
					feedback->m_isVisible = true;
				}
				else
				{
					// Hide the progress window
					ShowWindow( feedback->m_wndDialog, SW_HIDE );

					// Kill the timer
					KillTimer( feedback->m_wndDialog, ID_ETA_TIMER );

					// Hidden
					feedback->m_isVisible = false;
				}
				
				// Process another message
				continue;
			}

			// Update text
			if ( msg.message == WM_USER_SET_TEXT )
			{
				Red::Threads::CScopedLock< Red::Threads::CMutex > lock( feedback->m_dataMutex );
				SendMessage( feedback->m_wndText, WM_SETTEXT, 0, ( LPARAM ) feedback->m_lastText );
			}
			
			// Update range
			if ( msg.message == WM_USER_SET_RANGE )
			{
				Red::Threads::CScopedLock< Red::Threads::CMutex > lock( feedback->m_dataMutex );
				SendMessage( feedback->m_wndBar, PBM_SETRANGE, 0, MAKELPARAM( 0, feedback->m_lastRange ) );
			}

			// Update position
			if ( msg.message == WM_USER_SET_PROGRESS )
			{
				Red::Threads::CScopedLock< Red::Threads::CMutex > lock( feedback->m_dataMutex );
				SendMessage( feedback->m_wndBar, PBM_SETPOS, feedback->m_lastValue, 0 );
			}

			// Update task name
			if ( msg.message == WM_USER_SET_NAME )
			{
				Red::Threads::CScopedLock< Red::Threads::CMutex > lock( feedback->m_dataMutex );
				SendMessage( feedback->m_wndDialog, WM_SETTEXT, 0, (LPARAM)feedback->m_lastTitle );
			}

			// Process message
			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}

		// Done
		return 0;
	}

	//! Show object inspector
	virtual void InspectObject( class ISerializable* object, const String& tag )
	{
		::InspectObject( object, tag );
	}

protected:
	//! Initialize dialog, only once
	void ConditionalInitDialog()
	{
		if ( !m_wndDialog )
		{
			// Load the dialog
			m_wndDialog = CreateDialogParam( GetModuleHandle(NULL), MAKEINTRESOURCE( IDD_PROGRESSDLG ), NULL, &Win32Feedback::DialogProc, (LPARAM)this );
			ASSERT( m_wndDialog );

			// Get the text box
			m_wndText = GetDlgItem( m_wndDialog, IDC_PROGRESS_TEXT );
			ASSERT( m_wndText );

			// Get the progress bar
			m_wndBar = GetDlgItem( m_wndDialog, IDC_PROGRESS_BAR );
			ASSERT( m_wndBar );

			// Get the cancel button
			m_wndButton = GetDlgItem( m_wndDialog, IDCANCEL );
			ASSERT( m_wndButton );

			// Get the time label
			m_wndTime = GetDlgItem( m_wndDialog, IDC_PROGRESS_TIME );
			ASSERT( m_wndTime );

			// Update range
			m_lastRange = 100;
			SendMessage( m_wndBar, PBM_SETRANGE, 0, MAKELPARAM(0, m_lastRange) );

			// Update value
			m_lastValue = 0;
			SendMessage( m_wndBar, PBM_SETPOS, 0, m_lastValue );
		}
	}

	//! Show List of items for user to choose from
	virtual void ShowList( const Char* caption, const TDynArray< String >& itemList, TDynArray< Uint32 >& selectedIndexes )
	{
		CEdSelectFromListDialog* dialog = new CEdSelectFromListDialog( wxTheFrame, itemList, selectedIndexes );
		dialog->SetTitle( caption );

		dialog->ShowModal();
	}

	// Show the formatted dialogbox, check editor's utils.cpp for details
	virtual Int32 FormattedDialogBox( const String& caption, String code, ... ) override
	{
		va_list va;
		va_start( va, code );
		Int32 r = CEdFormattedDialog::ShowFormattedDialogVA( nullptr, caption.AsChar(), code.AsChar(), va );
		va_end( va );
		return r;
	}
};

void InitWin32FeedbackSystem()
{
	static Win32Feedback feedbackSystem;
	GFeedback = &feedbackSystem;
}
