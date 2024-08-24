//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CarSimjoystickDlg.h"

CarSimJoystickDlg::CarSimJoystickDlg( CarSimJoystickMgr* in_pJoystickMgr, CWnd* pParent )
:	CDialog(CarSimJoystickDlg::IDD, pParent)
,	m_pJoystickMgr( in_pJoystickMgr )
{
	_ASSERT( m_pJoystickMgr );
	::ZeroMemory( &m_currentAssignement, sizeof( m_currentAssignement ) );
}

void CarSimJoystickDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_JOYSTICK_COMBO, m_joystickCombo);
	DDX_Control(pDX, IDC_GEARUP_COMBO, m_gearUpCombo);
	DDX_Control(pDX, IDC_GEARDOWN_COMBO, m_gearDownCombo);
	DDX_Control(pDX, IDC_THROTTLE_COMBO, m_throttleCombo);
	DDX_Control(pDX, IDC_ENVIRONMENT_WET_COMBO, m_AuxBusCombo);
}

BEGIN_MESSAGE_MAP(CarSimJoystickDlg, CDialog)
	ON_CBN_SELCHANGE(IDC_JOYSTICK_COMBO, OnCbnSelchangeJoystickCombo)
END_MESSAGE_MAP()

BOOL CarSimJoystickDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	bool bUseJoystick = m_pJoystickMgr->GetJoystickAssignment( m_currentAssignement );

	CheckDlgButton( IDC_USE_JOYSTICK_CHECK, bUseJoystick? BST_CHECKED : BST_UNCHECKED );

	if( m_pJoystickMgr->GetAllJoystick( m_devices ) )
	{
		DeviceVector::const_iterator itEnd = m_devices.end();
		for( DeviceVector::const_iterator it = m_devices.begin(); it != itEnd; ++it )
		{
			int idx = m_joystickCombo.AddString( it->tszProductName );
			m_joystickCombo.SetItemData( idx, (DWORD_PTR)&(*it) );

			if( m_currentAssignement.m_joystickProductGuid == it->guidProduct )
				m_joystickCombo.SetCurSel( idx );
		}
	}

	OnCbnSelchangeJoystickCombo();

	return TRUE;
}
	
void CarSimJoystickDlg::OnOK()
{
	JoystickAssignment assignment = {0};

	bool bUseJoystick = IsDlgButtonChecked( IDC_USE_JOYSTICK_CHECK ) == BST_CHECKED;

	int idx = m_joystickCombo.GetCurSel();
	if( idx != CB_ERR )
	{
		const DIDEVICEINSTANCE* pJoystickInstance = (const DIDEVICEINSTANCE*)m_joystickCombo.GetItemData( idx );
		assignment.m_joystickProductGuid = pJoystickInstance->guidProduct;
	}
	
	idx = m_gearUpCombo.GetCurSel();
	if( idx != CB_ERR )
	{
		const DIDEVICEOBJECTINSTANCE* pInstance = (const DIDEVICEOBJECTINSTANCE*)m_gearUpCombo.GetItemData( idx );
		assignment.m_gearUpButtonIndex = DIDFT_GETINSTANCE( pInstance->dwType );
	}

	idx = m_gearDownCombo.GetCurSel();
	if( idx != CB_ERR )
	{
		const DIDEVICEOBJECTINSTANCE* pInstance = (const DIDEVICEOBJECTINSTANCE*)m_gearDownCombo.GetItemData( idx );
		assignment.m_gearDownButtonIndex = DIDFT_GETINSTANCE( pInstance->dwType );
	}

	idx = m_throttleCombo.GetCurSel();
	if( idx != CB_ERR )
	{
		const DIDEVICEOBJECTINSTANCE* pInstance = (const DIDEVICEOBJECTINSTANCE*)m_throttleCombo.GetItemData( idx );
		assignment.m_throttleAxisGuid = pInstance->guidType;
	}

	idx = m_AuxBusCombo.GetCurSel();
	if( idx != CB_ERR )
	{
		const DIDEVICEOBJECTINSTANCE* pInstance = (const DIDEVICEOBJECTINSTANCE*)m_AuxBusCombo.GetItemData( idx );
		assignment.m_AuxBusAxisGuid = pInstance->guidType;
	}

	m_pJoystickMgr->SetJoystickAssignement( bUseJoystick, assignment );

	CDialog::OnOK();
}

void CarSimJoystickDlg::OnCbnSelchangeJoystickCombo()
{
	m_gearUpCombo.ResetContent();
	m_gearDownCombo.ResetContent();
	m_throttleCombo.ResetContent();
	m_AuxBusCombo.ResetContent();

	int idxJoystick = m_joystickCombo.GetCurSel();
	if( idxJoystick == CB_ERR )
		return;

	const DIDEVICEINSTANCE* pJoystickInstance = (const DIDEVICEINSTANCE*)m_joystickCombo.GetItemData( idxJoystick );
	if( m_pJoystickMgr->GetAllJoystickButtons( *pJoystickInstance, m_joystickButton ) )
	{
		DeviceObjectVector::const_iterator itEnd = m_joystickButton.end();
		for( DeviceObjectVector::const_iterator it = m_joystickButton.begin(); it != itEnd; ++it )
		{
			int idx = m_gearUpCombo.AddString( it->tszName );
			m_gearUpCombo.SetItemData( idx, (DWORD_PTR)&(*it) );

			if( m_currentAssignement.m_gearUpButtonIndex == DIDFT_GETINSTANCE( it->dwType ) )
				m_gearUpCombo.SetCurSel( idx );

			idx = m_gearDownCombo.AddString( it->tszName );
			m_gearDownCombo.SetItemData( idx, (DWORD_PTR)&(*it) );

			if( m_currentAssignement.m_gearDownButtonIndex == DIDFT_GETINSTANCE( it->dwType ) )
				m_gearDownCombo.SetCurSel( idx );
		}
	}

	if( m_pJoystickMgr->GetAllJoystickAxis( *pJoystickInstance, m_joystickAxis ) )
	{
		DeviceObjectVector::const_iterator itEnd = m_joystickAxis.end();
		for( DeviceObjectVector::const_iterator it = m_joystickAxis.begin(); it != itEnd; ++it )
		{
			int idx = m_throttleCombo.AddString( it->tszName );
			m_throttleCombo.SetItemData( idx, (DWORD_PTR)&(*it) );

			if( m_currentAssignement.m_throttleAxisGuid == it->guidType )
				m_throttleCombo.SetCurSel( idx );

			idx = m_AuxBusCombo.AddString( it->tszName );
			m_AuxBusCombo.SetItemData( idx, (DWORD_PTR)&(*it) );

			if( m_currentAssignement.m_AuxBusAxisGuid == it->guidType )
				m_AuxBusCombo.SetCurSel( idx );
		}
	}
}