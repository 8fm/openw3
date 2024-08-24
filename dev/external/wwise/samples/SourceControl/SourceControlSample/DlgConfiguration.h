//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#ifndef _CONFIGURATION_DIALOG_H
#define _CONFIGURATION_DIALOG_H

#include "AK/Wwise/SourceControl/ISourceControl.h"

using namespace AK;
using namespace Wwise;

class DlgConfiguration
	: public ISourceControlDialogBase
{
public:
	DlgConfiguration( ISourceControlUtilities* in_pUtilities );

	// ISourceControlDialogBase implementation
	virtual HINSTANCE GetResourceHandle() const;
	virtual void GetDialog( UINT & out_uiDialogID ) const;
	virtual bool Help( HWND in_hWnd	) const;
	virtual bool HasHelp() const;
	virtual bool WindowProc( HWND in_hWnd, UINT in_message,	WPARAM in_wParam, LPARAM in_lParam,	LRESULT & out_lResult );

private:
	bool OnInitDialog( HWND in_hWnd );
	bool OnOK( HWND in_hWnd );
	
	ISourceControlUtilities* m_pSourceControlUtilities;
};

#endif // _CONFIGURATION_DIALOG_H