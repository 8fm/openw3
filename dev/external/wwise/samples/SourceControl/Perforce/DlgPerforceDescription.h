////////////////////////////////////////////////////////////////////////
// DlgPerforceDescription.h
//
// Header file for the CDlgPerforceDescription dialog, used to let the user
// enter a submit descripton.
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
///////////////////////////////////////////////////////////////////////

#pragma once

#include <AK/Wwise/SourceControl/ISourceControlDialogBase.h>
#include <AK/Wwise/SourceControl/ISourceControlUtilities.h>

using namespace AK;
using namespace Wwise;

// CDlgPerforceDescription dialog
class CDlgPerforceDescription
	: public ISourceControlDialogBase
{
public:
	// Constructor
	CDlgPerforceDescription( ISourceControlUtilities* in_pUtilities );

	// Destructor
	virtual ~CDlgPerforceDescription();

	// ISourceControlDialogBase
	virtual HINSTANCE GetResourceHandle() const;
	virtual void GetDialog( UINT & out_uiDialogID ) const;
	virtual bool HasHelp() const;
	virtual bool Help( HWND in_hWnd ) const;;
	virtual bool WindowProc( HWND in_hWnd, UINT in_message, WPARAM in_wParam, LPARAM in_lParam, LRESULT & out_lResult );

	const CString& GetDescription() const;
	bool CheckOutAfterSubmit() const;
private:

	// Overrides
	void OnInitDialog( HWND in_hWnd );
	bool OnBnClickedOk( HWND in_hWnd );

	// Data
	CString m_csDescription;
	bool m_bCheckOutAfterSubmit;
	ISourceControlUtilities* m_pUtilities;
};
