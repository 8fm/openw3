/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "res/resource.h"
#include "../../common/redSystem/crt.h"
#include "../../common/redSystem/error.h"

#include "../../common/engine/viewport.h"
#include "../../common/engine/inputDeviceManager.h"
#include "../../common/engine/inputEditorInterface.h"

struct SAssertDialogParams
{
	const Char*	m_expression;
	const Char* m_file;
	Uint32		m_line;
	const Char* m_details;
	const Char* m_message;
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

			Char lineBuf[20];
			Red::System::SNPrintF( lineBuf, ARRAY_COUNT( lineBuf ), TXT( "%d" ), params->m_line );
			HWND hLine = GetDlgItem( hwndDlg, IDC_LINE );
			SetWindowText( hLine, lineBuf );

			HWND hMessage = GetDlgItem( hwndDlg, IDC_MESSAGE );
			SetWindowText( hMessage, params->m_message );

			HWND hCallstack = GetDlgItem( hwndDlg, IDC_CALLSTACK );
			SetWindowText( hCallstack, params->m_details );

			// MattH: Disabled copy to clipboard as allocating memory after an assert fired is incredibly dangerous

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

Red::System::Error::EAssertAction AssertMessageImp( const Char* cppFile, Uint32 line, const Char* expression, const Char* message, const Char* details )
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

	IInputDeviceManager* inputDeviceManager = GEngine ? GEngine->GetInputDeviceManager() : nullptr;

	if ( inputDeviceManager && inputDeviceManager->GetEditorInterface() )
	{
		// TBD: Still doesn't help with the clipcursorrect, but whatever
		inputDeviceManager->GetEditorInterface()->SetAssertHookInputCaptureOverride( true );
	}

	EMouseMode previousMouseMode = MM_Normal;
	Bool refullscreenize = false;
	if ( GGame != nullptr && GGame->GetViewport() != nullptr )
	{
		refullscreenize = GGame->GetViewport()->GetViewportWindowMode() == VWM_Fullscreen;
		if ( refullscreenize )
		{
			GGame->GetViewport()->RequestWindowMode( VWM_Windowed );
		}
		previousMouseMode = GGame->GetViewport()->GetMouseMode();
		GGame->GetViewport()->SetMouseMode( MM_Normal );
	}
	else
	{
		// This can fail, but in the cases where it can fail (mismatched calls)
		// we should have GGame and GGame->GetViewport()
		while( ShowCursor( true ) < 0 );
	}

	INT_PTR retVal = DialogBoxParam( GetModuleHandle( NULL ), MAKEINTRESOURCE( IDD_ASSERTDLG ), NULL, AsserDlgProcStatic, reinterpret_cast< LPARAM >( &params ) );

	if ( inputDeviceManager && inputDeviceManager->GetEditorInterface() )
	{
		inputDeviceManager->GetEditorInterface()->SetAssertHookInputCaptureOverride( false );
	}

	if ( GGame != nullptr && GGame->GetViewport() != nullptr )
	{
		Bool gameActive = GGame->IsActive() && !GGame->IsPaused();
		if ( refullscreenize )
		{
			GGame->GetViewport()->RequestWindowMode( VWM_Fullscreen );
		}
		if ( gameActive )
		{
			GGame->GetViewport()->SetCursorVisibility( false );
		}
		GGame->GetViewport()->SetMouseMode( previousMouseMode );

		if ( gameActive )
		{
			GGame->GetViewport()->SetFocus();
			GGame->GetViewport()->Activate();
		}
	}
	else
	{
 		while( ShowCursor( false ) >= 0 );
	}

	return static_cast< Red::System::Error::EAssertAction >( retVal );
}
