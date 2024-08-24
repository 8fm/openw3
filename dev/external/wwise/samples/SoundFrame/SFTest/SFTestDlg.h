//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

// SFTestDlg.h : header file
//

#pragma once

#include <AK/SoundFrame/SF.h>

#include "SFTest.h"
#include "SFTestDropTarget.h"
#include "SFTestListBox.h"
#include "SFTestComboBox.h"
#include "SFGameObjectListCtrl.h"
#include "SFTestPositioningDlg.h"
#include "SFTestEnvironmentDlg.h"

#include <map>

using namespace AK;
using namespace SoundFrame;

// CSFTestDlg dialog
// This is the Main dialog of the application
// It implements IClient interface and is the owner of the Sound Frame object.
class CSFTestDlg 
	: public CDialog
	, public IClient
{
public:
	CSFTestDlg(CWnd* pParent = NULL);	// standard constructor
	~CSFTestDlg();

	enum { IDD = IDD_SFTEST_DIALOG };

	// IClient
	virtual void OnConnect( bool bConnect );
	virtual void OnEventNotif( Notif in_eNotif, AkUniqueID in_eventID );
	virtual void OnDialogueEventNotif( Notif in_eNotif, AkUniqueID in_dialogueEventID );
	virtual void OnSoundObjectNotif( Notif in_eNotif, AkUniqueID in_soundObjectID );
	virtual void OnStatesNotif( Notif in_eNotif, AkUniqueID in_stateGroupID	);
	virtual void OnSwitchesNotif( Notif in_eNotif, AkUniqueID in_switchGroupID );
	virtual void OnGameObjectsNotif( Notif in_eNotif, AkGameObjectID in_gameObjectID );
	virtual void OnGameParametersNotif( Notif in_eNotif, AkUniqueID in_gameParameterID );
	virtual void OnTriggersNotif( Notif in_eNotif, AkUniqueID in_triggerID );
	virtual void OnArgumentsNotif( Notif in_eNotif, AkUniqueID in_argumentID );
	virtual void OnAuxBusNotif( Notif in_eNotif, AkUniqueID in_AuxBusID );

	// CSFTestDlg
	ISoundFrame * GetSoundFrame();

	void AddEventsToList( IEventList * in_pEventList );
	void AddStateGroupToList( IStateGroupList * in_pStateGroupList );
	void AddSwitchGroupToList( ISwitchGroupList * in_pSwitchGroupList );
	void AddGameParameterToList( IGameParameterList * in_pGameParameterList );
	void AddConversionSettingsToList( IConversionSettingsList * in_pConversionSettingsList );
	void AddTriggerToList( ITriggerList * in_pTriggerList );
	void AddAuxBusToList( IAuxBusList * in_pAuxBusList );
	void AddDialogueEventToList( IDialogueEventList * in_pDialogueEventList );

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnLbnSelchangeEventlist();
	afx_msg void OnLbnSelchangeActionlist();
	afx_msg void OnLbnSelchangeStateGrouplist();
	afx_msg void OnCbnSelchangeCurrentStateCombo();
	afx_msg void OnLbnSelchangeSwitchGrouplist();
	afx_msg void OnCbnSelchangeCurrentSwitchCombo();
	afx_msg void OnLbnSelchangeGameParameterlist();
	afx_msg void OnLbnSelchangeDialogueEventlist();
	afx_msg void OnLbnSelchangeArgumentlist();
	afx_msg void OnLbnSelchangeTriggerlist();
	afx_msg void OnLvnItemChangedGameObjectList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnBnClickedPlay();
	afx_msg void OnBnClickedStopAll();
	afx_msg void OnBnClickedGetAllEvents();
	afx_msg void OnBnClickedClearEvents();
	afx_msg void OnBnClickedGetAllStateGroups();
	afx_msg void OnBnClickedClearStateGroups();
	afx_msg void OnBnClickedGetAllSwitchGroups();
	afx_msg void OnBnClickedClearSwitchGroups();
	afx_msg void OnBnClickedResetGameParameter();
	afx_msg void OnBnClickedGetAllGameParameters();
	afx_msg void OnBnClickedClearGameParameters();
	afx_msg void OnBnClickedGetAllTriggers();
	afx_msg void OnBnClickedClearTriggers();
	afx_msg void OnBnClickedPostTrigger();
	afx_msg void OnBnClickedGetAllDialogueEvents();
	afx_msg void OnBnClickedClearDialogueEvents();
	afx_msg void OnBnClickedAddGameObject();
	afx_msg void OnBnClickedRemoveGameObject();
	afx_msg void OnBnClickedSBDefBrowse();
	afx_msg void OnBnClickedSBDefImport();
	afx_msg void OnBnClickedGenerateSoundBank();
	afx_msg void OnBnClickedConvertExternalsourcesButton();
	afx_msg void OnBnClickedEsInDefBrowseButton();
	afx_msg void OnBnClickedOutInDefBrowseButton();
	afx_msg void OnBnClickedPlayBackMode();
	afx_msg void OnBnClickedEditPosition();
	afx_msg void OnBnClickedEditAuxBus();
	afx_msg void OnBnClickedPostMonitoringMsg();
	afx_msg void OnHScroll( UINT nSBCode, UINT nPos, CScrollBar* pScrollBar );

	DECLARE_MESSAGE_MAP()

private:
	void ResetGameParameter();

	void GetEventList();
	void GetDialogueEventList();
	void GetStateGroupList();
	void GetSwitchGroupList();
	void GetGameObjectList();
	void GetGameParameterList();
	void GetConversionSettingsList();
	void GetTriggerList();

	void ClearEventList();
	void ClearDialogueEventList();
	void ClearActionList();
	void ClearStateGroupList();
	void ClearSwitchGroupList();
	void ClearGameObjectList();
	void ClearGameParameterList();
	void ClearConversionSettingsList();
	void ClearTriggerList();
	void ClearArgumentList();
	void ClearArgumentValueList();
	void ClearEventSourcesValueList();
	void ClearDialogueEventSourcesValueList();

	void UpdateExplorerPath(ISFObject *in_pObject, ISFObject* in_pParent);

	void SetProjectTitle( const WCHAR * in_szNewProject );
	void LoadProject();
	void SaveProject();

	bool UseString();

	AkGameObjectID GetCurrentGameObject( AkGameObjectID in_defaultGameObject = IGameObject::s_WwiseGameObject );
	IDialogueEvent * GetSelectedDialogueEvent();

	// The only Sound Frame pointer that should be released
	// other dialogs will receive the pointer but they will not addref it
	ISoundFrame * m_pSoundFrame;

	SFTestListBox<IEvent, IEventList> m_eventList;
	SFTestListBox<IAction, IActionList> m_actionList;
	SFTestListBox<IStateGroup, IStateGroupList> m_stateGroupList;
	SFTestListBox<ISwitchGroup, ISwitchGroupList> m_switchGroupList;
	SFTestListBox<IGameParameter, IGameParameterList> m_gameParameterList;
	SFTestListBox<IConversionSettings, IConversionSettingsList> m_conversionSettingsList;
	SFTestListBox<ITrigger, ITriggerList> m_triggerList;
	SFTestListBox<IDialogueEvent, IDialogueEventList> m_dialogueEventList;
	SFTestListBox<IArgument, IArgumentList> m_argumentList;
	SFTestListBox<IArgumentValue, IArgumentValueList> m_argumentValueList;
	SFTestListBox<IOriginalFile, IOriginalFileList> m_eventSourcesValueList;
	SFTestListBox<IOriginalFile, IOriginalFileList> m_dialogueEventSourcesValueList;
	
	SFTestComboBox<IState, IStateList, AkUniqueID> m_currentStateCombo;
	SFTestComboBox<ISwitch, ISwitchList, AkUniqueID> m_currentSwitchCombo;

	SFGameObjectListCtrl m_gameObjectListCtrl;

	CSliderCtrl m_gameParameterSlider;

	CSFEventDropTarget m_eventTarget;
	CSFStateGroupDropTarget m_statesTarget;
	CSFSwitchGroupDropTarget m_switchesTarget;
	CSFGameParameterDropTarget m_gameParametersTarget;
	CSFTriggerDropTarget m_triggersTarget;
	CSFAuxBusDropTarget m_AuxBusTarget;
	CSFDialogueEventDropTarget m_dialogueEventTarget;

	CSFTestPositioningDlg m_positioningDialog;
	CSFTestAuxBusDlg m_AuxBusDialog;

	AkUniqueID m_soundObjectDisplayed;

	typedef std::map<AkGameObjectID, double> GameObjectToRTPCValueMap;
	typedef std::map<AkUniqueID, GameObjectToRTPCValueMap> GameParametersValuesMap;

	// Game parameters values cache
	GameParametersValuesMap m_gameParametersValuesMap;
public:
	afx_msg void OnBnClickedGetallconversionsettings();
	afx_msg void OnBnClickedClearconversionsettings();
};
