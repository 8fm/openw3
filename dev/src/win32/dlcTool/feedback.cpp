#include "build.h"
#include "resource.h"

#pragma comment ( lib, "comctl32.lib" )

namespace DLCTool 
{
	HWND GMainWindow;
}

// Win32 feedback system
class Win32Feedback
{
protected:
	static const DWORD WM_USER_SHOWWINDOW = WM_USER + 1;
	static const DWORD WM_USER_SET_TEXT = WM_USER + 2;
	static const DWORD WM_USER_SET_PROGRESS = WM_USER + 3;
	static const DWORD WM_USER_SET_RANGE = WM_USER + 4;

	static const DWORD ID_ETA_TIMER = 12345;

protected:
	HWND			m_wndDialog;		//!< Handle to dialog box
	HWND			m_wndText;			//!< Handle to progress text
	HWND			m_wndBar;			//!< Handle to progress bar
	INT				m_lastRange;		//!< Last progress bar range
	INT				m_lastValue;		//!< Last progress bar value
	WCHAR			m_lastText[ 512 ];	//!< Last text to display
	WCHAR			m_lastTitle[ 512 ];	//!< Last title to display
	HANDLE			m_threadHandle;		//!< Handle to internal UI thread
	bool			m_isVisible;		//!< Is the window visible

public:
	Win32Feedback()
		: m_wndDialog( NULL )
		, m_wndText( NULL )
		, m_wndBar( NULL )
		, m_lastRange( 0 )
		, m_lastValue( 0 )
		, m_threadHandle( NULL )
		, m_isVisible( false )
	{
	}

	~Win32Feedback()
	{
		EndTask();
	}

	void BeginTask( const WCHAR* name )
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

		// Set defaults
		wcscpy_s( m_lastTitle, ARRAYSIZE(m_lastTitle), name );

		// Show window
		PostMessage( m_wndDialog, WM_USER_SHOWWINDOW, 1, 0 );
	}

	void EndTask()
	{
		if ( m_isVisible )
		{
			PostMessage( m_wndDialog, WM_USER_SHOWWINDOW, 0, 0 );
			while ( m_isVisible )
			{
				Sleep( 100 );
			}
		}
	}

	void UpdateTaskProgress( INT current, INT total )
	{
		if ( m_isVisible )
		{
			if ( m_wndBar )
			{
				// Update range
				if ( m_lastRange != total )
				{
					m_lastRange = total;
					PostMessage( m_wndDialog, WM_USER_SET_RANGE, 0, 0 ) ;
				}

				// Update position
				if ( m_lastValue != current )
				{
					m_lastValue = current;
					PostMessage( m_wndDialog, WM_USER_SET_PROGRESS, 0, 0 ) ;
				}
			}
		}
	}

	void UpdateTaskInfo( const WCHAR* info )
	{
		if ( m_isVisible )
		{
			// Update text
			wcscpy_s( m_lastText, ARRAYSIZE(m_lastText), info );
			PostMessage( m_wndDialog, WM_USER_SET_TEXT, 0, 0 );
		}
	}

protected:
	//! Dialog message processing function
	static INT_PTR CALLBACK DialogProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
	{
		// Dialog init
		if ( uMsg == WM_INITDIALOG )
		{
			void* feedback = ( void* ) lParam;
			SetWindowLongPtr( hwndDlg, GWLP_USERDATA, (LONG)feedback );
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
					SendMessage( feedback->m_wndText, WM_SETTEXT, 0, (LPARAM)TEXT("Please wait...") );

					// Set icon
					HICON hIcon = LoadIcon( GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_WITCHER) );
					SendMessage( feedback->m_wndDialog, WM_SETICON, ICON_BIG, (LPARAM)hIcon );
					SendMessage( feedback->m_wndDialog, WM_SETICON, ICON_SMALL, (LPARAM)hIcon );

					// Show the window
					ShowWindow( feedback->m_wndDialog, SW_SHOWNA );
					SetForegroundWindow( feedback->m_wndDialog );

					// Visible
					feedback->m_isVisible = true;
				}
				else
				{
					// Hide the progress window
					ShowWindow( feedback->m_wndDialog, SW_HIDE );

					// Hidden
					feedback->m_isVisible = false;
				}

				// Process another message
				continue;
			}

			// Update text
			if ( msg.message == WM_USER_SET_TEXT )
			{
				SendMessage( feedback->m_wndText, WM_SETTEXT, 0, ( LPARAM ) feedback->m_lastText );
			}

			// Update range
			if ( msg.message == WM_USER_SET_RANGE )
			{
				SendMessage( feedback->m_wndBar, PBM_SETRANGE, 0, MAKELPARAM( 0, feedback->m_lastRange ) );
			}

			// Update position
			if ( msg.message == WM_USER_SET_PROGRESS )
			{
				SendMessage( feedback->m_wndBar, PBM_SETPOS, feedback->m_lastValue, 0 );
			}

			// Process message
			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}

		// Done
		return 0;
	}

protected:
	//! Initialize dialog, only once
	void ConditionalInitDialog()
	{
		if ( !m_wndDialog )
		{
			// Load the dialog
			m_wndDialog = CreateDialogParam( GetModuleHandle(NULL), MAKEINTRESOURCE( IDD_PROGRESS ), NULL, &Win32Feedback::DialogProc, (LPARAM)this );
			ASSERT( m_wndDialog );

			// Get the text box
			m_wndText = GetDlgItem( m_wndDialog, IDC_PROGRESS_TEXT );
			ASSERT( m_wndText );

			// Get the progress bar
			m_wndBar = GetDlgItem( m_wndDialog, IDC_PROGRESS_BAR );
			ASSERT( m_wndBar );

			// Update range
			m_lastRange = 100;
			SendMessage( m_wndBar, PBM_SETRANGE, 0, MAKELPARAM(0, m_lastRange) );

			// Update value
			m_lastValue = 0;
			SendMessage( m_wndBar, PBM_SETPOS, 0, m_lastValue );

			DLCTool::GMainWindow = m_wndDialog;
		}
	}
};

namespace DLCTool
{
	// Get feedback interface
	Win32Feedback& GetFeedback()
	{
		static Win32Feedback theFeedback;
		return theFeedback;
	}

	// Show progress window
	void ShowProgressWindow()
	{
		const WCHAR* string = GetStringById( String_ToolTitle );		
		GetFeedback().BeginTask( string );
	}

	// Hide progress window
	void HideProgressWindow()
	{
		GetFeedback().EndTask();
	}

	// Update task title
	void UpdateTaskInfo( StringID id )
	{
		const WCHAR* string = GetStringById( id );
		GetFeedback().UpdateTaskInfo( string );
	}

	// Update task progress
	void UpdateTaskProgress( INT cur, INT max )
	{
		GetFeedback().UpdateTaskProgress( cur, max );
	}
}