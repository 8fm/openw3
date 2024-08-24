////////////////////////////////////////////////////////////////////////
// DlgRename.cpp
//
// Implementation file for the CDlgRename dialog, used to let the user
// choose a new name for a file.
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
///////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include ".\DlgRename.h"
#include "resource.h"
#include <AK/Wwise/SourceControl/ISourceControl.h>

#include <windowsx.h>

namespace
{
	const CString k_csInvalidFileNameChar = L":<>*?\"\\/|";
};

// Constructor
CDlgRename::CDlgRename( const CString& in_csFilename, ISourceControlUtilities* in_pUtilities )
	: m_pUtilities( in_pUtilities )
{
	char acDrive[_MAX_DRIVE];
	char acDir[_MAX_DIR];
	char acFilename[_MAX_FNAME];
	char acExt[_MAX_EXT];
	CW2A strFilename( in_csFilename );

	::_splitpath( strFilename, acDrive, acDir, acFilename, acExt );

	m_csFolder = CString( acDrive ) + CString( acDir );
	m_csName = CString( acFilename );
	m_csExtension = CString( acExt );
}

// Destructor
CDlgRename::~CDlgRename()
{
}

void CDlgRename::OnInitDialog( HWND in_hWnd )
{
	// Setup controls
	CEdit* pControlName = (CEdit*)CWnd::FromHandle( ::GetDlgItem( in_hWnd, IDC_NEW_FILE_NAME ) );
	pControlName->SetLimitText( AK::Wwise::SourceControlConstant::s_uiMaxWorkUnitName );
	pControlName->SetWindowText( m_csName );

	// Select the name and give it the focus so the user can
	// start typing right-away
	pControlName->SetFocus();
	pControlName->SetSel( 0, -1, true );
}

bool CDlgRename::OnBnClickedOk( HWND in_hWnd )
{
	bool bReturn = false;

	// Get the final name
	CEdit* pControlName = (CEdit*)CWnd::FromHandle( ::GetDlgItem( in_hWnd, IDC_NEW_FILE_NAME ) );

	pControlName->GetWindowText( m_csName );

	if ( m_csName.IsEmpty() || ( m_csName.FindOneOf( k_csInvalidFileNameChar ) != -1 ) )
	{
		CString csMessage;
		CString csCaption;

		csMessage.LoadString( IDS_ERROR_RENAME_MESSAGE );
		csCaption.LoadString( IDS_MESSAGEBOX_CAPTION );

		m_pUtilities->MessageBox( in_hWnd, csMessage, csCaption, MB_OK );

		bReturn = true;
	}

	return bReturn;
}

bool CDlgRename::OnEnChangeFileName( HWND in_hWnd )
{
	CEdit* pControlName = (CEdit*)CWnd::FromHandle( ::GetDlgItem( in_hWnd, IDC_NEW_FILE_NAME ) );
	CEdit* pControlPath = (CEdit*)CWnd::FromHandle( ::GetDlgItem( in_hWnd, IDC_NEW_FILE_PATH ) );

	pControlName->GetWindowText( m_csName );

	CString csFullPath = GetNewFilename();

	pControlPath->SetWindowText( csFullPath );

	return false;
}

CString CDlgRename::GetNewFilename() const
{
	return m_csFolder + m_csName + m_csExtension;
}

// ISourceControlDialogBase
HINSTANCE CDlgRename::GetResourceHandle() const
{    
	AFX_MANAGE_STATE( ::AfxGetStaticModuleState() );
	return ::AfxGetResourceHandle();
}

void CDlgRename::GetDialog( UINT & out_uiDialogID ) const
{
	out_uiDialogID = IDD_RENAME;
}

bool CDlgRename::HasHelp() const
{
	return false;
}

bool CDlgRename::Help( HWND in_hWnd ) const
{
	return false;
}

bool CDlgRename::WindowProc( HWND in_hWnd, UINT in_message, WPARAM in_wParam, LPARAM in_lParam, LRESULT & out_lResult )
{
	bool bReturn = false;

    if ( in_message == WM_INITDIALOG )
    {
		OnInitDialog( in_hWnd );
    }
    else if ( in_message == WM_COMMAND )
    {
        // Notification code
        switch ( HIWORD( in_wParam ) )
        {
			case BN_CLICKED:
				switch ( LOWORD( in_wParam ) )
				{
					case IDOK:
						bReturn = OnBnClickedOk( in_hWnd );
						break;
				}
				break;

			case EN_CHANGE:
				switch ( LOWORD( in_wParam ) )
				{
					case IDC_NEW_FILE_NAME:
						bReturn = OnEnChangeFileName( in_hWnd );
						break;
				}
				break;
        }
    }

    return bReturn;
}