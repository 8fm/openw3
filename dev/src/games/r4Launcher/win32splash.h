/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "resource.h"
#include "../../common/core/feedback.h"
#include "../../common/core/fileSys.h"

// Win32 splash screen
class Win32Splash : public ISplashScreen
{
protected:
	static const DWORD WM_USER_SHOWWINDOW = WM_USER + 1;
	static const DWORD WM_USER_SET_TEXT = WM_USER + 2;

protected:
	HWND					m_wndDialog;			//!< Handle to dialog box
	HWND					m_wndProgressText;		//!< Handle to progress text
	Char					m_lastText[ 512 ];		//!< Last text to display
	HANDLE					m_threadHandle;			//!< Handle to internal UI thread
	Red::Threads::CMutex	m_dataMutex;			//!< Access to data lock

public:
	Win32Splash()
		: m_wndDialog( NULL )
		, m_wndProgressText( NULL )
		, m_threadHandle( NULL )
	{
	}

	~Win32Splash()
	{
	}

	void ShowSplash()
	{	
		// Create thread and window on first use
		if ( !m_threadHandle )
		{
			// Create thread
			m_threadHandle = CreateThread( NULL, 0, &Win32Splash::ThreadFunction, this, 0, NULL );

			// Wait until window is initialized
			while ( !m_wndDialog )
			{
				Sleep( 100 );
			}
		}

		// Show window
		PostMessage( m_wndDialog, WM_USER_SHOWWINDOW, 1, 0 );
	}

	void HideSplash()
	{
		PostMessage( m_wndDialog, WM_USER_SHOWWINDOW, 0, 0 );
		//WaitForSingleObject(m_threadHandle, INFINITE );
		m_threadHandle = nullptr;
	}

	virtual void UpdateProgress( const Char* info, ... )
	{
		if ( m_wndProgressText )
		{
			Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_dataMutex );

			// Hide the splash
			if ( 0 == Red::System::StringCompareNoCase( info, TXT("HideSplash") ) )
			{
				m_wndProgressText = NULL;
				HideSplash();
				return;
			}

			// Print the text
			va_list arglist;
			va_start(arglist, info);
			Red::System::VSNPrintF( m_lastText, ARRAY_COUNT(m_lastText), info, arglist ); 

			// Update text
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
			LONG_PTR feedback = lParam;
			SetWindowLongPtr( hwndDlg, GWLP_USERDATA, feedback );
			return TRUE;
		}

		// Color of text
		if ( uMsg == WM_CTLCOLORSTATIC )
		{
			HDC hDC = (HDC) wParam;
			SetBkMode( hDC, TRANSPARENT );
			SetTextColor( hDC, RGB(255,255,255) );
			if ( (HWND)lParam == GetDlgItem( hwndDlg, IDC_PROGRESS_TEXT ) )
			{
				return (INT_PTR)GetStockObject( BLACK_BRUSH );
			}
			else
			{
				return (INT_PTR)GetStockObject( NULL_BRUSH );
			}
		}

		// Do not process
		return FALSE;
	}

	//! Thread function
	static DWORD WINAPI ThreadFunction( LPVOID lpParam )
	{
		// Get the feedback system instance
		Win32Splash* splash = ( Win32Splash* ) lpParam;

		// Change priority to allow easy ui update
		SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL );

		// Create window
		splash->ConditionalInitDialog();

		// Process messages
		MSG msg;
		while ( GetMessage( &msg, splash->m_wndDialog, 0, 0 ) )
		{
			// Show/Hide window
			if ( msg.message == WM_USER_SHOWWINDOW )
			{
				if ( msg.wParam != 0 )
				{
					// Show the window
					ShowWindow( splash->m_wndDialog, SW_SHOWNA );
					SetForegroundWindow( splash->m_wndDialog );
				}
				else
				{
					// Hide the progress window
					DestroyWindow( splash->m_wndDialog );
					splash->m_wndDialog = nullptr;
					splash->m_wndProgressText = nullptr;

					// Quit the app
					PostQuitMessage(0);
				}

				// Process another message
				continue;
			}

			// Update text
			if ( msg.message == WM_USER_SET_TEXT )
			{
				Red::Threads::CScopedLock< Red::Threads::CMutex > lock( splash->m_dataMutex );
				SendMessage( splash->m_wndProgressText, WM_SETTEXT, 0, ( LPARAM ) splash->m_lastText );
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
			m_wndDialog = CreateDialogParam( GetModuleHandle(NULL), MAKEINTRESOURCE( IDD_SPLASH ), NULL, &Win32Splash::DialogProc, (LPARAM)this );
			ASSERT( m_wndDialog );

			// Get the text box
			m_wndProgressText = GetDlgItem( m_wndDialog, IDC_PROGRESS_TEXT );
			ASSERT( m_wndProgressText );

			// Get the background static control
			HWND wndStatic = GetDlgItem( m_wndDialog, IDC_STATICBG );
			ASSERT( wndStatic );
			
			//HBITMAP splashImage = (HBITMAP)::LoadImage( GetModuleHandle(NULL), MAKEINTRESOURCE( IDB_SPLASH ), IMAGE_BITMAP, 0, 0, 0 );
			const String splashPath = GFileManager->GetBaseDirectory() + TXT("splashscreen.bmp");
			HBITMAP splashImage = static_cast<HBITMAP>( LoadImage( NULL, splashPath.AsChar(), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE ) );
			SendMessage( wndStatic, STM_SETIMAGE, IMAGE_BITMAP, reinterpret_cast<LPARAM>( splashImage ) );
		}
	}
};
