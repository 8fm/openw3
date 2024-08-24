//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CarSimFramerateDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

namespace
{
	static const UINT s_uiMinFramerate = 1;
	static const UINT s_uiMaxFramerate = 200;
	static const UINT s_uiDefaultFramerate = 60;
}

CarSimFramerateDlg::CarSimFramerateDlg( CWnd* pParent )
	: CDialog( CarSimFramerateDlg::IDD, pParent )
	, m_uiFramerate( s_uiDefaultFramerate )
{}

void CarSimFramerateDlg::SetFramerate( const UINT in_uiFramerate )
{
	_ASSERT( in_uiFramerate >= s_uiMinFramerate && in_uiFramerate <= s_uiMaxFramerate );
	m_uiFramerate = in_uiFramerate;
}

UINT CarSimFramerateDlg::GetFramerate() const
{
	return m_uiFramerate;
}

BEGIN_MESSAGE_MAP(CarSimFramerateDlg, CDialog)
END_MESSAGE_MAP()

BOOL CarSimFramerateDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	CString csFramerate;
	csFramerate.Format( L"%d", m_uiFramerate );
	SetDlgItemText( IDC_FRAMERATE_EDIT, csFramerate );

	return TRUE;
}

void CarSimFramerateDlg::OnOK()
{
	CString csFramerate;
	GetDlgItemText( IDC_FRAMERATE_EDIT, csFramerate );

	UINT uiFramerate = _ttoi( csFramerate );
	if( uiFramerate >= s_uiMinFramerate && uiFramerate <= s_uiMaxFramerate )
	{
		m_uiFramerate = uiFramerate;

		CDialog::OnOK();
	}
	else
	{
		CString csMessage;
		csMessage.Format( IDS_INVALID_FRAMERATE_MESSAGE, s_uiMinFramerate, s_uiMaxFramerate );
		MessageBox( csMessage );
	}
}
