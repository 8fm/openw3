/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#ifdef RED_PLATFORM_WINPC

#include <TlHelp32.h>
#include "resource.h"
#include "../../common/core/gameConfiguration.h"
#include "../../common/core/clipboardBase.h"
#include "../../common/game/configParser.h"
#include "../../common/engine/baseEngine.h"
#include "../../common/engine/viewport.h"
#include "../../common/engine/renderer.h"
#include "../../common/engine/inputBufferedInputEvent.h"
#include "../../common/engine/scriptCompilationHelper.h"
#include "../../common/renderer/renderViewport.h"
#include "../../common/renderer/renderViewportWindow.h"

#include "win32splash.h"

extern ISplashScreen* GSplash;

bool InitializeGameConfiguration()
{
	if( !GameConfig::LoadConfig( TXT( "r4" ) ) )
	{
		return false;
	}
	return true;
}

void GetPlatformDefaultResolution( Int32 & width, Int32 & height )
{
	width = ::GetSystemMetrics( SM_CXSCREEN );
	height = ::GetSystemMetrics( SM_CYSCREEN );
}

struct SAssertDialogParams
{
	const Red::System::Char*	m_expression;
	const Red::System::Char*	m_file;
	Red::System::Uint32			m_line;
	const Red::System::Char*	m_details;
	const Red::System::Char*	m_message;
};

INT_PTR CALLBACK AsserDlgProcStatic( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	switch( uMsg )
	{
	case WM_INITDIALOG:
		{
			SAssertDialogParams* params = reinterpret_cast<SAssertDialogParams*>( lParam );

			HWND hExpression = GetDlgItem( hwndDlg, IDC_EXPRESSION );
			SetWindowText( hExpression, params->m_expression );

			HWND hFile = GetDlgItem( hwndDlg, IDC_FILE );
			SetWindowText( hFile, params->m_file );

			Red::System::Char lineBuf[20];
			Red::System::SNPrintF( lineBuf, ARRAY_COUNT( lineBuf ), TXT( "%d" ), params->m_line );
			HWND hLine = GetDlgItem( hwndDlg, IDC_LINE );
			SetWindowText( hLine, lineBuf );

			HWND hMessage = GetDlgItem( hwndDlg, IDC_MESSAGE );
			SetWindowText( hMessage, params->m_message );

			HWND hCallstack = GetDlgItem( hwndDlg, IDC_CALLSTACK );
			SetWindowText( hCallstack, params->m_details );

			::SetWindowPos( hwndDlg, (HWND)HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE );

			return TRUE;
		}

	case WM_COMMAND:
		{
			switch( LOWORD( wParam ) )
			{
			case IDCANCEL:
				EndDialog( hwndDlg, Red::System::Error::AA_ContinueAlways );
				return TRUE;
			case IDC_CONTINUE:
				EndDialog( hwndDlg, Red::System::Error::AA_Continue );
				return TRUE;
			case IDC_CONTINUEALW:
				EndDialog( hwndDlg, Red::System::Error::AA_ContinueAlways );
				return TRUE;
			case IDC_DEBUG:
				EndDialog( hwndDlg, Red::System::Error::AA_Break );
				return TRUE;
			case IDC_CLOSE:
				EndDialog( hwndDlg, Red::System::Error::AA_Stop );
				return TRUE;
			} 
		}
	}

	return FALSE;
}

Red::System::Error::EAssertAction AssertMessage( const Red::System::Char* cppFile, Red::System::Uint32 line, const Red::System::Char* expression, const Red::System::Char* message, const Red::System::Char* details )
{
	// Fix the line endings so they're in Window's format
	// This is needed as the dialog box won't break text onto
	// new lines if it doesn't encounter a proper windows line ending
	const size_t maxDetailsLength = 8 * 1024;
	const size_t windowsNewLineLength = Red::System::StringLength( TXT( "\r\n" ) );
	const size_t newlineLength = Red::System::StringLength( TXT( "\n" ) );
	Char detailsFixed[ maxDetailsLength ] = {0};
	Char* writeBuffer = detailsFixed;
	const Char* detailBuffer = details;
	const Char* foundNewLine = nullptr;

	while( foundNewLine = Red::System::StringSearch( detailBuffer, TXT( "\n" ) ) )
	{
		size_t strlength = foundNewLine - detailBuffer;
		if( (size_t)( writeBuffer + strlength + windowsNewLineLength ) > (size_t)( detailsFixed + maxDetailsLength ) )
			break;

		Red::System::MemoryCopy( writeBuffer, detailBuffer, sizeof( Char ) * strlength );
		writeBuffer += strlength;
		Red::System::MemoryCopy( writeBuffer, TXT( "\r\n" ), windowsNewLineLength * sizeof( Char ) );
		writeBuffer += windowsNewLineLength;
		detailBuffer = foundNewLine + newlineLength;
	}

	SAssertDialogParams params;
	params.m_expression = expression;
	params.m_file = cppFile;
	params.m_line = line;
	params.m_message = message;
	params.m_details = detailsFixed;
	
	// Make sure the mouse cursor is visible
	EMouseMode previousMouseMode = MM_Normal;
	if ( GGame != nullptr && GGame->GetViewport() != nullptr )
	{
		previousMouseMode = GGame->GetViewport()->GetMouseMode();
		GGame->GetViewport()->SetCursorVisibility( true );
		GGame->GetViewport()->SetMouseMode( MM_Normal );
#ifdef RED_ASSERTS_ENABLED
		static_cast< CRenderViewport* >( GGame->GetViewport() )->DisableActivationHandling();
#endif
	}
	else
	{
		// This can fail, but in the cases where it can fail (mismatched calls)
		// we should have GGame and GGame->GetViewport()
		while( ShowCursor( true ) < 0 );
	}

	// Enter constrained mode so that the game wont progress until the assert goes away
	Bool exitConstrainedLater = false;
	if ( ::SIsMainThread() && GEngine && GEngine->GetState() != BES_Constrained )
	{
		exitConstrainedLater = true;
		GEngine->OnEnterConstrainedRunningMode();
	}

	// Make the game window a normal one
	if ( GGame != nullptr && GGame->GetViewport() != nullptr )
	{
		HWND window = static_cast< CRenderViewport* >( GGame->GetViewport() )->GetWindow()->GetWindowHandle();
		if ( ::SIsMainThread() && GGame->GetViewport()->GetViewportWindowMode() == VWM_Fullscreen )
		{
			::SetWindowPos( window, (HWND)HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE );
		}
	}

	// Show the dialog box
	INT_PTR retVal = DialogBoxParam( GetModuleHandle( NULL ), MAKEINTRESOURCE( IDD_ASSERTDLG ), NULL, AsserDlgProcStatic, reinterpret_cast< LPARAM >( &params ) );

	// Make the game window topmost again
	if ( GGame != nullptr && GGame->GetViewport() != nullptr )
	{
		HWND window = static_cast< CRenderViewport* >( GGame->GetViewport() )->GetWindow()->GetWindowHandle();
		if ( ::SIsMainThread() && GGame->GetViewport()->GetViewportWindowMode() == VWM_Fullscreen )
		{
			::SetWindowPos( window, (HWND)HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE );
		}

		// Enable activation handling
#ifdef RED_ASSERTS_ENABLED
		static_cast< CRenderViewport* >( GGame->GetViewport() )->EnableActivationHandling();
#endif
	}

	// If we enter constrained mode above, exit it
	if ( exitConstrainedLater && GEngine )
	{
		GEngine->OnExitConstrainedRunningMode();
	}

	// Hide the cursor
	if ( GGame != nullptr && GGame->GetViewport() != nullptr )
	{
		GGame->GetViewport()->SetFocus();
		GGame->GetViewport()->SetMouseMode( previousMouseMode );
		GGame->GetViewport()->SetCursorVisibility( false );
	}
	else
	{
 		while( ShowCursor( false ) >= 0 );
	}

	return static_cast< Red::System::Error::EAssertAction >( retVal );
}

INT_PTR CALLBACK ScriptDlgProcStatic( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	switch( uMsg )
	{
	case WM_INITDIALOG:
		{
			const CScriptCompilationMessages& errorCollector = *reinterpret_cast<CScriptCompilationMessages*>( lParam );

			HWND hEdit = GetDlgItem( hwndDlg, IDC_SCRIPTERR );

			const auto& errors = errorCollector.m_errors;
			const auto& warnings = errorCollector.m_warnings;

			String lameUseStringBuffer;
			for ( const auto& err : errors )
			{
				lameUseStringBuffer += String::Printf(TXT("Error %ls(%u): %ls\r\n"), err.file.AsChar(), err.line, err.text.AsChar() );
			}

			if ( !warnings.Empty() )
			{
				lameUseStringBuffer += TXT("\r\n");
			}

			for ( const auto& war : warnings )
			{
				lameUseStringBuffer += String::Printf(TXT("Warning %ls(%u): %ls\r\n"), war.file.AsChar(), war.line, war.text.AsChar() );
			}

			SetWindowText( hEdit, lameUseStringBuffer.AsChar() );

			if ( !GIsEditor )
			{
				HWND hSkip = GetDlgItem( hwndDlg, IDSCRIPTSKIP );
				::EnableWindow( hSkip, FALSE );
				::ShowWindow( hSkip, SW_HIDE );
			}

			::SetWindowPos( hwndDlg, (HWND)HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE );

			return TRUE;
		}

	case WM_COMMAND:
		{
			switch( LOWORD( wParam ) )
			{
			case IDSCRIPTRETRY:
				EndDialog( hwndDlg, CSRV_Recompile );
				return TRUE;
			case IDSCRIPTCOPY:
				{
					HWND hEdit = GetDlgItem( hwndDlg, IDC_SCRIPTERR );
					const int len = GetWindowTextLength( hEdit );
					if ( len > 0 )
					{
						String text;
						text.Resize( len );
						GetWindowText( hEdit, text.TypedData(), len );
						GClipboard->Copy( text );
					}
				}
				return TRUE;
			case IDSCRIPTSKIP:
				EndDialog( hwndDlg, CSRV_Skip );
				return TRUE;
			case IDSCRIPTEXIT:
				EndDialog( hwndDlg, CSRV_Quit );
				return TRUE;
			} 
		}
	}

	return FALSE;
}

namespace ScriptCompilationHelpers
{
	// If this appears, it should be before we have a viewport. So simpler cursor/window management
	ECompileScriptsReturnValue LauncherScriptCompilationErrorMessage( const CScriptCompilationMessages& errorCollector )
	{
		while( ShowCursor( true ) < 0 )
			continue;

		// Show the dialog box
		INT_PTR retVal = DialogBoxParam( GetModuleHandle( NULL ), MAKEINTRESOURCE( IDD_SCRIPTERRDLG ), NULL, ScriptDlgProcStatic, reinterpret_cast< LPARAM >( &errorCollector ) );

		while( ShowCursor( false ) >= 0 )
			continue;

		return static_cast<ECompileScriptsReturnValue>( retVal );
	}

	static Win32Splash splash;
	static ISplashScreen* oldSplash;

	void ShowHideSplash( Bool show )
	{
		if ( show )
		{
			if ( GSplash != &splash )
			{
				oldSplash = GSplash;
				GSplash = &splash;
				splash.ShowSplash();
			}
		}
		else
		{
			if ( GSplash == &splash )
			{
				splash.HideSplash();
				GSplash = oldSplash;
			}
		}
	}
}

#endif
