////////////////////////////////////////////////////////////////////////
// DlgPerforceDescription.cpp
//
// Implementation file for the CDlgPerforceDescription dialog, used to let the user
// enter a Check In description
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
///////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include ".\DlgPerforceDescription.h"
#include "resource.h"

#include <windowsx.h>

// Constructor
CDlgPerforceDescription::CDlgPerforceDescription( ISourceControlUtilities* in_pUtilities )
	: m_pUtilities( in_pUtilities )
	, m_bCheckOutAfterSubmit( false )
{
}

// Destructor
CDlgPerforceDescription::~CDlgPerforceDescription()
{
}

void CDlgPerforceDescription::OnInitDialog( HWND in_hWnd )
{
	::SetFocus( ::GetDlgItem( in_hWnd, IDC_DESCRIPTION ) );
}

bool CDlgPerforceDescription::OnBnClickedOk( HWND in_hWnd )
{
	bool bReturn = false;

	// Get the description
	CEdit* pControlDescription = (CEdit*)CWnd::FromHandle( ::GetDlgItem( in_hWnd, IDC_DESCRIPTION ) );

	pControlDescription->GetWindowText( m_csDescription );

	if ( m_csDescription.IsEmpty() )
	{
		AFX_MANAGE_STATE( AfxGetStaticModuleState() );

		CString csMessage;
		CString csCaption;

		csMessage.LoadString( IDS_PERFORCE_DESCRIPTION_MESSAGE );
		csCaption.LoadString( IDS_PERFORCE_MESSAGEBOX_CAPTION );

		m_pUtilities->MessageBox( in_hWnd, csMessage, csCaption, MB_OK );

		bReturn = true;
	}

	HWND hCheckOut = ::GetDlgItem( in_hWnd, IDC_CHECK_OUT_AFTER_SUBMIT );
	m_bCheckOutAfterSubmit = Button_GetCheck( hCheckOut ) ? true : false;

	return bReturn;
}

const CString& CDlgPerforceDescription::GetDescription() const
{
	return m_csDescription;
}

bool CDlgPerforceDescription::CheckOutAfterSubmit() const
{
	return m_bCheckOutAfterSubmit;
}

// ISourceControlDialogBase
HINSTANCE CDlgPerforceDescription::GetResourceHandle() const
{    
	AFX_MANAGE_STATE( ::AfxGetStaticModuleState() );
	return ::AfxGetResourceHandle();
}

void CDlgPerforceDescription::GetDialog( UINT & out_uiDialogID ) const
{
	out_uiDialogID = IDD_PERFORCE_DESCRIPTION;
}

bool CDlgPerforceDescription::HasHelp() const
{
	return false;
}

bool CDlgPerforceDescription::Help( HWND in_hWnd ) const
{
	return false;
}

bool CDlgPerforceDescription::WindowProc( HWND in_hWnd, UINT in_message, WPARAM in_wParam, LPARAM in_lParam, LRESULT & out_lResult )
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