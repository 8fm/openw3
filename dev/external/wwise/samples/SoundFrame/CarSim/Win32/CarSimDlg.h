//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

// CarSimDlg.h : header file
//

#pragma once

#include "ICarSimUI.h"
#include "CarSimAssignmentDlg.h"
#include "CarSettingsDlg.h"

class CarSimEngine;

// CCarSimDlg dialog
class CCarSimDlg : public CDialog,
					public ICarSimUI
{
// Construction
public:
	CCarSimDlg(CarSimEngine* in_pCarSimEngine, CWnd* pParent = NULL);	// standard constructor

	// ICarSimUI
	virtual void OnSimulationSettingsChanged();
	virtual void OnNotif( ObjectType in_eObjectType, IClient::Notif in_eNotif, AkUniqueID in_objectID );
	virtual void UpdateFrame();

// Dialog Data
	enum { IDD = IDD_CAR_SIM_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg void OnDestroy();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnCommandFileNew();
	afx_msg void OnCommandFileOpen();
	afx_msg void OnCommandFileSave();
	afx_msg void OnCommandFileSaveAs();
	afx_msg void OnCommandAssignments();
	afx_msg void OnCommandJoystick();
	afx_msg void OnCommandFramerate();
	afx_msg void OnCommandAbout();
	afx_msg void OnCommandHelp();
	afx_msg void OnKillFocusGameObjectName();
	afx_msg void OnBnClickedPlay();
	afx_msg void OnBnClickedStop();
	afx_msg void OnBnClickedGearUp();
	afx_msg void OnBnClickedGearDown();
	afx_msg void OnBnClickedBrowseCarSettings();
	afx_msg void OnBnClickedEditCarSettings();
	afx_msg void OnBnClickedAuxBusCheck();
	afx_msg void OnHScroll( UINT nSBCode, UINT nPos, CScrollBar* pScrollBar );
	afx_msg void OnActivate( UINT nState, CWnd* pWndOther, BOOL bMinimized );

	DECLARE_MESSAGE_MAP()

private:

	void OnAuxBusChange();
	void OnThrottleChange();

	void UpdateTitleBarText();

	void SetDirty( bool in_bDirty );

	bool AskForSave();

	CarSimAssignmentDlg m_assignmentDialog;
	CarSettingsDlg m_settingsDialog;

	CSliderCtrl m_throttleSlider;

	CSliderCtrl m_dryVolumeSlider;
	CSliderCtrl m_wetVolumeSlider;

	CarSimEngine* m_pCarSimEngine;

	bool m_bDirty;
};
