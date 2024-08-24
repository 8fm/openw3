////////////////////////////////////////////////////////////////////////
// DlgRename.h
//
// Header file for the CDlgRename dialog, used to let the user
// choose a new name for a file.
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
///////////////////////////////////////////////////////////////////////

#pragma once

#include <AK/Wwise/SourceControl/ISourceControlDialogBase.h>
#include <AK/Wwise/SourceControl/ISourceControlUtilities.h>

using namespace AK;
using namespace Wwise;

// CDlgRename dialog
class CDlgRename
	: public ISourceControlDialogBase
{
public:
	// Constructor
	CDlgRename( const CString& in_csFilename, ISourceControlUtilities* in_pUtilities );

	// Destructor
	virtual ~CDlgRename();

	// ISourceControlDialogBase
	virtual HINSTANCE GetResourceHandle() const;
	virtual void GetDialog( UINT & out_uiDialogID ) const;
	virtual bool HasHelp() const;
	virtual bool Help( HWND in_hWnd ) const;
	virtual bool WindowProc( HWND in_hWnd, UINT in_message, WPARAM in_wParam, LPARAM in_lParam, LRESULT & out_lResult );

	CString GetNewFilename() const;   
private:

	// Overrides
	virtual void OnInitDialog( HWND in_hWnd );

	// Message handlers
	bool OnBnClickedOk( HWND in_hWnd );
	bool OnEnChangeFileName( HWND in_hWnd );

	// Data
	CString m_csExtension;
	CString m_csName;
	CString m_csFolder;
	ISourceControlUtilities* m_pUtilities;
};
