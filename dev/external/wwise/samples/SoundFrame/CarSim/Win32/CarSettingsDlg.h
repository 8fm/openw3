//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "resource.h"
#include "CarSimEditControl.h"
#include "CarParams.h"

class Car;
class CarSimEngine;

class CarSettingsDlg : public CDialog
{
public:
	CarSettingsDlg( CarSimEngine* in_pCarSimeEngine, CWnd* pParent = NULL );

	void Open();
	void Close();

	void OnSettingsFileChanged();

	void AskForSave();

	// Dialog Data
	enum { IDD = IDD_CAR_SETTINGS_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

	// Generated message map functions
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg void OnDestroy();
	afx_msg void OnBnClickedDefault();
	afx_msg void OnBnClickedSave();
	afx_msg void OnBnClickedSaveAs();
	afx_msg void OnCbnSelchangeNumOfGearCombo();
	afx_msg void OnKillFocusGearRatioEdit();
	afx_msg void OnKillFocusSettingsEdit();
	afx_msg void OnHScroll( UINT nSBCode, UINT nPos, CScrollBar* pScrollBar );

	DECLARE_MESSAGE_MAP()

private:

	void UpdateTitleBarText();

	void UpdateDisplay();

	void SetDirty( bool in_bDirty );

	CComboBox m_numOfGearCombo;

	CSliderCtrl m_gearSliderArray[CarParam::s_nMaxNumOfGear];
	CarSimNumberEdit m_gearEditArray[CarParam::s_nMaxNumOfGear];

	CarSimNumberEdit m_weightEdit;
	CarSimNumberEdit m_torqueEdit;
	CarSimNumberEdit m_dragCoefficientEdit;
	CarSimNumberEdit m_frontSurfaceEdit;
	CarSimNumberEdit m_rollingResistenceEdit;
	CarSimNumberEdit m_differentialRatioEdit;
	CarSimNumberEdit m_wheelRadiusEdit;
	CarSimNumberEdit m_minRPMEdit;
	CarSimNumberEdit m_rpmCutOffEdit;

	Car& m_rCar;

	CarSimEngine* m_pCarSimEngine;

	bool m_bDirty;
};