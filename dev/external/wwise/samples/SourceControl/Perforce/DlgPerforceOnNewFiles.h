////////////////////////////////////////////////////////////////////////
// DlgPerforceOnNewFiles.h
//
// Header file for the CDlgPerforceOnNewFiles dialog, used to let the user
// choose if he want to check out files that are not checked out before
// saving.
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
///////////////////////////////////////////////////////////////////////

#pragma once

#include <AK/Wwise/SourceControl/ISourceControlDialogBase.h>
#include <AK/Wwise/SourceControl/ISourceControlUtilities.h>
#include <AK/Wwise/SourceControl/ISourceControl.h>

using namespace AK;
using namespace Wwise;

// CDlgPerforceOnNewFiles dialog
class CDlgPerforceOnNewFiles
	: public ISourceControlDialogBase
{
public:
	// Constructor
	CDlgPerforceOnNewFiles( 
		bool in_bEnableAddFiles, 
		bool in_bEnableCheckOutFiles, 
		bool in_bAddFiles, 
		bool in_bCheckOutFiles,
		AK::Wwise::ISourceControlUtilities* in_pUtilities,
		const WCHAR** in_ppFilenameList,
		unsigned int in_uiFilenameListCount );

	// Destructor
	virtual ~CDlgPerforceOnNewFiles();

	// ISourceControlDialogBase
	virtual HINSTANCE GetResourceHandle() const;
	virtual void GetDialog( UINT & out_uiDialogID ) const;
	virtual bool HasHelp() const;
	virtual bool Help( HWND in_hWnd ) const;
	virtual bool WindowProc( HWND in_hWnd, UINT in_message, WPARAM in_wParam, LPARAM in_lParam, LRESULT & out_lResult );

	bool NeedToCheckOutFiles() const;
	bool NeedToAddFiles() const;
	INT_PTR GetResult() const;
private:

	// Overrides
	virtual void OnInitDialog( HWND in_hWnd );
	bool OnBnClickedOK( HWND in_hWnd );

	// Data
	bool m_bEnableAddFiles; 
	bool m_bEnableCheckOutFiles;
	bool m_bNeedToCheckOutFiles;
	bool m_bNeedToAddFiles;
	INT_PTR m_uiResult;

	AK::Wwise::ISourceControlUtilities* m_pUtilities;
	const WCHAR** m_ppFilenameList;
	unsigned int m_uiFilenameListCount;
};
