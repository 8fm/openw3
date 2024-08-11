/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "resource.h"

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
	String detailsStr = details;
	detailsStr.ReplaceAll( TXT( "\n" ), TXT( "\r\n" ) );

	SAssertDialogParams params;
	params.m_expression = expression;
	params.m_file = cppFile;
	params.m_line = line;
	params.m_message = message;
	params.m_details = detailsStr.TypedData();

	INT_PTR retVal = DialogBoxParam( GetModuleHandle( NULL ), MAKEINTRESOURCE( IDD_ASSERTDLG ), NULL, AsserDlgProcStatic, reinterpret_cast< LPARAM >( &params ) );

	return static_cast< Red::System::Error::EAssertAction >( retVal );
}
