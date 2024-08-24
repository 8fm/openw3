////////////////////////////////////////////////////////////////////////
// DlgLockMessage.h
//
// Header file for the CDlgLockMessage dialog, used to let the user
// enter a submit descripton.
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
// Author: gpaille
//
///////////////////////////////////////////////////////////////////////

#pragma once

#include <AK/Wwise/SourceControl/ISourceControlDialogBase.h>
#include <AK/Wwise/SourceControl/ISourceControlUtilities.h>

using namespace AK;
using namespace Wwise;

// CDlgLockMessage dialog
class CDlgLockMessage
	: public ISourceControlDialogBase
{
public:
	// Constructor
	CDlgLockMessage( ISourceControlUtilities* in_pUtilities );

	// Destructor
	virtual ~CDlgLockMessage();

	// ISourceControlDialogBase
	virtual HINSTANCE GetResourceHandle() const;
	virtual void GetDialog( UINT & out_uiDialogID ) const;
	virtual bool HasHelp() const;
	virtual bool Help( HWND in_hWnd ) const;
	virtual bool WindowProc( HWND in_hWnd, UINT in_message, WPARAM in_wParam, LPARAM in_lParam, LRESULT & out_lResult );

	const CString& GetMessage() const;
	const bool StealLocks() const;
private:

	// Overrides
	void OnInitDialog( HWND in_hWnd );
	bool OnBnClickedOk( HWND in_hWnd );

	// Data
	CString m_csMessage;
	bool m_bStealLocks;
	ISourceControlUtilities* m_pUtilities;
};
