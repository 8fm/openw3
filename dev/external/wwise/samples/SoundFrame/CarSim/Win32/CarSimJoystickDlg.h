//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "resource.h"
#include "CarSimjoystickMgr.h"

class CarSimJoystickDlg : public CDialog
{
public:
	CarSimJoystickDlg( CarSimJoystickMgr* in_pJoystickMgr, CWnd* pParent = NULL);

	// Dialog Data
	enum { IDD = IDD_JOYSTICK_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

	// Generated message map functions
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnCbnSelchangeJoystickCombo();

	DECLARE_MESSAGE_MAP()

private:

	CComboBox m_joystickCombo;
	CComboBox m_gearUpCombo;
	CComboBox m_gearDownCombo;
	CComboBox m_throttleCombo;
	CComboBox m_AuxBusCombo;

	CarSimJoystickMgr* m_pJoystickMgr;

	JoystickAssignment m_currentAssignement;

	// The combo box keep pointer to the element of thoses vectors
	// they should not be changed without changing the Combo box accordingly.
	DeviceVector m_devices;
	DeviceObjectVector m_joystickButton;
	DeviceObjectVector m_joystickAxis;
};