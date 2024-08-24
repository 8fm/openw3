////////////////////////////////////////////////////////////////////////
// DlgLockMessage.cpp
//
// Implementation file for the CDlgLockMessage dialog, used to let the user
// enter a Check In description
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
// Author: gpaille
//
///////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include ".\DlgLockMessage.h"
#include "resource.h"

#include <windowsx.h>

// Constructor
CDlgLockMessage::CDlgLockMessage( ISourceControlUtilities* in_pUtilities )
	: m_pUtilities( in_pUtilities )
	, m_bStealLocks( false )
{
}

// Destructor
CDlgLockMessage::~CDlgLockMessage()
{
}

void CDlgLockMessage::OnInitDialog( HWND in_hWnd )
{
	::SetFocus( ::GetDlgItem( in_hWnd, IDC_MESSAGE ) );
}

bool CDlgLockMessage::OnBnClickedOk( HWND in_hWnd )
{
	bool bReturn = false;

	// Get the description
	CEdit* pControlMessage = (CEdit*)CWnd::FromHandle( ::GetDlgItem( in_hWnd, IDC_MESSAGE ) );

	pControlMessage->GetWindowText( m_csMessage );

	HWND hStealLocks = ::GetDlgItem( in_hWnd, IDC_CHECK_STEAL_LOCKS );
	m_bStealLocks = Button_GetCheck( hStealLocks ) ? true : false;

	return bReturn;
}

const CString& CDlgLockMessage::GetMessage() const
{
	return m_csMessage;
}

const bool CDlgLockMessage::StealLocks() const
{
	return m_bStealLocks;
}

// ISourceControlDialogBase
HINSTANCE CDlgLockMessage::GetResourceHandle() const
{    
	AFX_MANAGE_STATE( ::AfxGetStaticModuleState() );
	return ::AfxGetResourceHandle();
}

void CDlgLockMessage::GetDialog( UINT & out_uiDialogID ) const
{
	out_uiDialogID = IDD_LOCK_MESSAGE;
}

bool CDlgLockMessage::HasHelp() const
{
	return false;
}

bool CDlgLockMessage::Help( HWND in_hWnd ) const
{
	return false;
}

bool CDlgLockMessage::WindowProc( HWND in_hWnd, UINT in_message, WPARAM in_wParam, LPARAM in_lParam, LRESULT & out_lResult )
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
        }
    }

    return bReturn;
}