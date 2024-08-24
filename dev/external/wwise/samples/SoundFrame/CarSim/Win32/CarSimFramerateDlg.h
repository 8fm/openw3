//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "resource.h"

class CarSimFramerateDlg : public CDialog
{
public:
	CarSimFramerateDlg( CWnd* pParent = NULL );

	void SetFramerate( const UINT in_uiFramerate );
	UINT GetFramerate() const;

	// Dialog Data
	enum { IDD = IDD_FRAMERATE_DIALOG };

protected:

	virtual BOOL OnInitDialog();
	virtual void OnOK();

	DECLARE_MESSAGE_MAP()

private:

	UINT m_uiFramerate;
};