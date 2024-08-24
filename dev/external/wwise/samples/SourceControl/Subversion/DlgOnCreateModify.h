////////////////////////////////////////////////////////////////////////
// DlgOnCreateModify.h
//
// Header file for the CDlgOnCreateModify dialog, used to let the user
// choose if he want to add new files to source control
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
///////////////////////////////////////////////////////////////////////

#pragma once

#include <AK/Wwise/SourceControl/ISourceControlDialogBase.h>
#include <AK/Wwise/SourceControl/ISourceControlUtilities.h>

using namespace AK;
using namespace Wwise;

// CDlgOnCreateModify dialog
class CDlgOnCreateModify
	: public ISourceControlDialogBase
{
public:
	// Constructor
	CDlgOnCreateModify( bool in_bAddFiles );

	// Destructor
	virtual ~CDlgOnCreateModify();

	// ISourceControlDialogBase
	virtual HINSTANCE GetResourceHandle() const;
	virtual void GetDialog( UINT & out_uiDialogID ) const;
	virtual bool HasHelp() const;
	virtual bool Help( HWND in_hWnd ) const;
	virtual bool WindowProc( HWND in_hWnd, UINT in_message, WPARAM in_wParam, LPARAM in_lParam, LRESULT & out_lResult );

	bool NeedToAddFiles() const;
	INT_PTR GetResult() const;
private:

	// Overrides
	virtual void OnInitDialog( HWND in_hWnd );
	bool OnBnClickedYes( HWND in_hWnd );

	// Data
	bool m_bNeedToAddFiles;
	INT_PTR m_uiResult;
};
