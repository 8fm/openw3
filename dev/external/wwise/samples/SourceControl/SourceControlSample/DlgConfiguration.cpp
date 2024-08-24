//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "DlgConfiguration.h"
#include "Resource.h"

// To use Edit and ComboBox macros
#include <windowsx.h>

DlgConfiguration::DlgConfiguration( ISourceControlUtilities* in_pUtilities )
	: m_pSourceControlUtilities( in_pUtilities )
{
}

// ISourceControlDialogBase
HINSTANCE DlgConfiguration::GetResourceHandle() const
{
	AFX_MANAGE_STATE( ::AfxGetStaticModuleState() );
	return ::AfxGetResourceHandle();
}

void DlgConfiguration::GetDialog( UINT & out_uiDialogID ) const
{
	out_uiDialogID = IDD_CONFIGURATION;
}

bool DlgConfiguration::Help( HWND in_hWnd ) const
{
	return false;
}

bool DlgConfiguration::HasHelp() const
{
	return false;
}

bool DlgConfiguration::WindowProc( HWND in_hWnd, UINT in_message, WPARAM in_wParam, LPARAM in_lParam, LRESULT & out_lResult )
{
	bool bStopMsg = false;

	if ( in_message == WM_INITDIALOG )
	{
		bStopMsg = OnInitDialog( in_hWnd );
	}

	// E.g. Catch window command actions (only for the main dialog) to enable/disable controls
	else if ( in_message == WM_COMMAND )
	{
		// Notification code
		switch ( HIWORD( in_wParam ) )
		{
		case BN_CLICKED:
			// Check which button was clicked
			switch ( LOWORD( in_wParam ) )
			{
			case IDOK:
				// The user pressed the 'Ok' button
				bStopMsg = OnOK( in_hWnd );
				break;
			}           
		} // End switch hi word (notification code)

	} // End command window event

	// Return false to let the parent window deal with the message. Return true
	// for message you don't want the parent window to handle.

	return bStopMsg;
}

// Local functions
bool DlgConfiguration::OnInitDialog( HWND in_hWnd )
{
	AFX_MANAGE_STATE( ::AfxGetStaticModuleState() );

	CString csText;

	// Fill the combo box
	HWND hCombo = ::GetDlgItem( in_hWnd, IDC_COMBO );

	csText.LoadString( IDS_CONFIGURATION_COMBO_TEXT1 );
	ComboBox_InsertString( hCombo, 0, csText );
	
	csText.LoadString( IDS_CONFIGURATION_COMBO_TEXT2 );
	ComboBox_InsertString( hCombo, 1, csText );

	ComboBox_SetCurSel( hCombo, 1 );

	// Set the Caption text
	HWND hCaption = ::GetDlgItem( in_hWnd, IDC_CAPTION );

	csText.LoadString( IDS_CONFIGURATION_CAPTION_TEXT );
	Edit_SetText( hCaption, csText ); 

	return false;
}

bool DlgConfiguration::OnOK( HWND in_hWnd )
{
	AFX_MANAGE_STATE( ::AfxGetStaticModuleState() );

	// Show a simple message box
	CString csMessage;
	CString csCaption;
	csMessage.LoadString( IDS_CONFIGURATION_MESSAGEBOX_MESSAGE );
	csCaption.LoadString( IDS_CONFIGURATION_MESSAGEBOX_CAPTION );

	m_pSourceControlUtilities->MessageBox( in_hWnd, csMessage, csCaption, MB_OK );

	return false;
}