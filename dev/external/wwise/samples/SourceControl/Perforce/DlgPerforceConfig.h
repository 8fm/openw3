////////////////////////////////////////////////////////////////////////
// DlgPerforceConfig.h
//
// Header file for the CDlgPerforceConfig dialog, used to let the user
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

// CDlgPerforceConfig dialog
class CDlgPerforceConfig
	: public ISourceControlDialogBase
{
public:
	// Constructor
	CDlgPerforceConfig( ISourceControlUtilities* in_pUtilities );

	// Destructor
	virtual ~CDlgPerforceConfig();

	// ISourceControlDialogBase
	virtual HINSTANCE GetResourceHandle() const;
	virtual void GetDialog( UINT & out_uiDialogID ) const;
	virtual bool HasHelp() const;
	virtual bool Help( HWND in_hWnd ) const;
	virtual bool WindowProc( HWND in_hWnd, UINT in_message, WPARAM in_wParam, LPARAM in_lParam, LRESULT & out_lResult );

private:

	// Overrides
	void OnInitDialog( HWND in_hWnd );

	bool OnBnClickedOk( HWND in_hWnd );
	bool OnBnClickedDiffBrowse( HWND in_hWnd );

	// Data
	ISourceControlUtilities* m_pUtilities;
};
