//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include <AK/SoundFrame/SF.h>

#include "SFTest.h"
#include "SFTestListBox.h"
#include "SFTestComboBox.h"
#include "SFTestDropTarget.h"

#include <map>

using namespace AK;
using namespace SoundFrame;

// Dialog used to test the AuxBus function in the Sound frame
class CSFTestAuxBusDlg 
	: public CDialog
{
public:
	CSFTestAuxBusDlg(CWnd* pParent = NULL);	// standard constructor
	~CSFTestAuxBusDlg();

	enum { IDD = IDD_EDIT_ENVIRONMENT_DIALOG };

	void Init( ISoundFrame * in_pSoundFrame, CSFAuxBusDropTarget* in_pDropTarget );
	void AddAuxBusToList( IAuxBusList * in_pAuxBusList );

	void OnAuxBusNotif( IClient::Notif in_eNotif, AkUniqueID in_AuxBusID );

	void UpdateGameObjectList();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnLbnSelchangeActiveGameObjectListenerCombo();
	afx_msg void OnLbnSelchangeAuxBusGameObjectCombo();
	afx_msg void OnBnClickedGetAllAuxBus();
	afx_msg void OnBnClickedClearAuxBus();
	afx_msg void OnGameObjectAuxBusChange();
	afx_msg void OnHScroll( UINT nSBCode, UINT nPos, CScrollBar* pScrollBar );

	DECLARE_MESSAGE_MAP()

private:

	static const int s_iNumOfAuxBus = 4;
	static const int s_iNumOfSpeakers = 6;

	void OnObstructionOcclusionChanged();
	void OnDryLevelChanged();

	void FillAuxBusCombo();

	ISoundFrame * m_pSoundFrame;

	SFTestListBox<IAuxBus, IAuxBusList> m_AuxBusList;

	SFTestComboBox<IGameObject, IGameObjectList, AkGameObjectID> m_activeGameObjectCombo;
	CComboBox m_activeListenerCombo;

	SFTestComboBox<IGameObject, IGameObjectList, AkGameObjectID> m_AuxBusGameObjectCombo;
	SFTestComboBox<IAuxBus, IAuxBusList, AkUniqueID> m_AuxBusCombo[s_iNumOfAuxBus];

	CSFAuxBusDropTarget* m_pDropTarget;

	CStatic m_AuxBusVolumeStatic[s_iNumOfSpeakers];
	CSliderCtrl m_obstructionLevelSlider;
	CSliderCtrl m_occlusionLevelSlider;

	CSliderCtrl m_AuxBusSlider[s_iNumOfAuxBus];
	CSliderCtrl m_dryLevelSlider;

	CStatic m_AuxBusStatic[s_iNumOfAuxBus];

	struct AuxBusSettings
	{
		// Default constructor
		AuxBusSettings();

		AkSpeakerVolumes m_sVolumes;
	};

	typedef std::pair<AkGameObjectID, UINT32> GameObjectListenerPair;

	struct GameObjectListenerPairLess
	{
		bool operator()( const GameObjectListenerPair& in_rLeft, const GameObjectListenerPair& in_rRight ) const
		{
			return (in_rLeft.first == in_rRight.first)? in_rLeft.second < in_rRight.second : in_rLeft.first < in_rRight.first;
		}
	};

	struct ObstructionOcclusionLevel
	{
		// Default constructor
		ObstructionOcclusionLevel();

		AkReal32 m_fObstruction;
		AkReal32 m_fOcclusion;
	};

	struct GameObjectAuxBus
	{
		// Default constructor
		GameObjectAuxBus();

		AkAuxSendValue m_sAuxBusValue[4];
		AkReal32 m_fDryLevel;
	};

	typedef std::map<AkUniqueID, AuxBusSettings> AuxBusSettingsMap;
	typedef std::map<GameObjectListenerPair, ObstructionOcclusionLevel, GameObjectListenerPairLess> ObstructionOcclusionMap;
	typedef std::map<AkGameObjectID, GameObjectAuxBus> GameObjectAuxBusMap;

	// Maps to remember the value we set since they can't be gotten from the Sound Frame
	AuxBusSettingsMap m_AuxBusSettingsMap;
	ObstructionOcclusionMap m_obstructionOcclusionMap;
	GameObjectAuxBusMap m_gameObjectAuxBusMap;
};