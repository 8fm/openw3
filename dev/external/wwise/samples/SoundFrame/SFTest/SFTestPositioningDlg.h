//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////


#pragma once

#include <AK/SoundFrame/SF.h>

#include "SFTest.h"
#include "SFTestComboBox.h"
#include "SFTestEditControl.h"

#include <bitset>
#include <map>
#include <vector>

using namespace AK;
using namespace SoundFrame;

// Dialog used to test the positioning function in the Sound frame
class CSFTestPositioningDlg 
	: public CDialog
{
public:
	CSFTestPositioningDlg(CWnd* pParent = NULL);	// standard constructor
	~CSFTestPositioningDlg();

	enum { IDD = IDD_EDIT_POSITION_DIALOG };

	void Init( ISoundFrame * in_pSoundFrame );

	void GetGameObjectPosition( AkGameObjectID in_gameObjectID, AkSoundPosition& out_rPosition );

	void UpdateGameObjectList();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnCbnSelchangeGameObjectCombo();
	afx_msg void OnCbnSelchangeListenerCombo();
	afx_msg void OnBnClickedListenerCheck();
	afx_msg void OnBnClickedEnableSpatialization();
	afx_msg void OnHScroll( UINT nSBCode, UINT nPos, CScrollBar* pScrollBar );
	afx_msg void OnKillFocusVolumeEdit();

	DECLARE_MESSAGE_MAP()

private:

	void OnGameObjectPosOriChanged();
	void OnListenerPosOriChanged();

	// Return the angle specified from the vector in degree (range from [0 to 360[)
	int CalcAngle( const double& in_dblX, const double& in_dblZ );

	typedef std::bitset<8> ActiveListenerBitset;

	struct GameObjectInfo
	{
		// Default constructor
		GameObjectInfo();
		
		AkSoundPosition m_position;
		ActiveListenerBitset m_activeListener;
	};

	struct ListenerInfo
	{
		// Default constructor
		ListenerInfo();

		AkListenerPosition m_position;
		bool m_bSpatialized;
		AkSpeakerVolumes m_volumeOffsets;
	};

	typedef std::map<AkGameObjectID, GameObjectInfo> GameObjectInfoMap;
	typedef std::vector<ListenerInfo> ListenerInfoVector;

	SFTestComboBox<IGameObject, IGameObjectList, AkGameObjectID> m_gameObjectCombo;
	CComboBox m_listenerCombo;

	CSliderCtrl m_gameObjectPosX;
	CSliderCtrl m_gameObjectPosZ;
	CSliderCtrl m_gameObjectOriAngle;

	CSliderCtrl m_listenerPosX;
	CSliderCtrl m_listenerPosZ;
	CSliderCtrl m_listenerOriAngle;

	CSFTestVolumeEdit m_volumeLeftEdit;
	CSFTestVolumeEdit m_volumeRightEdit;
	CSFTestVolumeEdit m_volumeCenterEdit;
	CSFTestVolumeEdit m_volumeLFEEdit;
	CSFTestVolumeEdit m_volumeRearLeftEdit;
	CSFTestVolumeEdit m_volumeRearRightEdit;

	ISoundFrame * m_pSoundFrame;

	// Maps to remember the value we set since they can't be gotten from the Sound Frame
	GameObjectInfoMap m_objectInfoMap;
	ListenerInfoVector m_listenerInfoVector;
};