////////////////////////////////////////////////////////////////////////
// DlgOnCreateModify.cpp
//
// Implementation file for the CDlgOnCreateModify dialog, used to let the user
// choose a new name for a file.
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
///////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include ".\DlgOnCreateModify.h"
#include "resource.h"
#include <AK/Wwise/SourceControl/ISourceControl.h>

#include <windowsx.h>

// Constructor
CDlgOnCreateModify::CDlgOnCreateModify( bool in_bAddFiles )
	: m_bNeedToAddFiles( in_bAddFiles )
	, m_uiResult( 0 )
{
}

// Destructor
CDlgOnCreateModify::~CDlgOnCreateModify()
{
}

void CDlgOnCreateModify::OnInitDialog( HWND in_hWnd )
{
	Button_SetCheck( ::GetDlgItem( in_hWnd, IDC_CHECK_ADD ), m_bNeedToAddFiles? TRUE : FALSE );
}

bool CDlgOnCreateModify::OnBnClickedYes( HWND in_hWnd )
{
	m_bNeedToAddFiles = Button_GetCheck( ::GetDlgItem( in_hWnd, IDC_CHECK_ADD ) ) == TRUE;

	return false;
}

bool CDlgOnCreateModify::NeedToAddFiles() const
{
	return m_bNeedToAddFiles;
}

INT_PTR CDlgOnCreateModify::GetResult() const
{
	return m_uiResult;
}

// ISourceControlDialogBase
HINSTANCE CDlgOnCreateModify::GetResourceHandle() const
{    
	AFX_MANAGE_STATE( ::AfxGetStaticModuleState() );
	return ::AfxGetResourceHandle();
}

void CDlgOnCreateModify::GetDialog( UINT & out_uiDialogID ) const
{
	out_uiDialogID = IDD_ONCREATEMODIFY;
}

bool CDlgOnCreateModify::HasHelp() const
{
	return false;
}

bool CDlgOnCreateModify::Help( HWND in_hWnd ) const
{
	return false;
}

bool CDlgOnCreateModify::WindowProc( HWND in_hWnd, UINT in_message, WPARAM in_wParam, LPARAM in_lParam, LRESULT & out_lResult )
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
			{
				bool bPostClose = false;

				switch ( LOWORD( in_wParam ) )
				{
					case ID_YES:
						bReturn = OnBnClickedYes( in_hWnd );
						bPostClose = true;
						break;

					case ID_NO:
					case ID_CANCEL:
						bPostClose = true;
						break;
				}

				if ( bPostClose )
				{
					m_uiResult = LOWORD( in_wParam );
					PostMessage( in_hWnd, WM_CLOSE, 0, 0 );
				}
				break;
			}
        }
    }

    return bReturn;
}