/***********************************************************************
  The content of this file includes source code for the sound engine
  portion of the AUDIOKINETIC Wwise Technology and constitutes "Level
  Two Source Code" as defined in the Source Code Addendum attached
  with this file.  Any use of the Level Two Source Code shall be
  subject to the terms and conditions outlined in the Source Code
  Addendum and the End User License Agreement for Wwise(R).

  Version: v2013.2.9  Build: 4872
  Copyright (c) 2006-2014 Audiokinetic Inc.
 ***********************************************************************/

#pragma once

#include <crtdbg.h>

#include <AK/IBytes.h>
#include <AK/SoundFrame/SF.h>
#include <AK/SoundEngine/Common/AkSoundEngine.h>

#include "SFPrivate.h"
#include "SFLibObjects.h"
#include "SFBytesMem.h"
#include "WObjectType.h"

using namespace AK;
using namespace SoundFrame;

//-------------------------------------------------------------------------------------------------
//  
class CSoundFrame
	: public CRefCountBase<ISoundFrame>
{
public:
	CSoundFrame( IClient * );
	~CSoundFrame();

	// ISoundFrame
	virtual bool Connect( DWORD in_dwProcessID );
	virtual bool IsConnected() const;
	virtual bool PlayEvents( const AkUniqueID * in_pEvents, long in_cEvents, AkGameObjectID in_gameObjectID = IGameObject::s_WwiseGameObject );
	virtual bool PlayEvents( LPCWSTR * in_pszEvents, long in_cEvents, AkGameObjectID in_gameObjectID = IGameObject::s_WwiseGameObject );
	virtual bool ExecuteActionOnEvent( AkUniqueID in_eventID, AK::SoundEngine::AkActionOnEventType in_ActionType, AkGameObjectID in_gameObjectID = AK_INVALID_GAME_OBJECT, AkTimeMs in_uTransitionDuration = 0, AkCurveInterpolation in_eFadeCurve = AkCurveInterpolation_Linear );

/** NOT IMPLEMENTED
	virtual bool ResolveDialogueEvent( AkUniqueID in_dialogueEventID, const AkUniqueID * in_aArgumentValues, long in_cNumArguments, ISoundObject ** out_ppSoundObject );
	virtual bool ResolveDialogueEvent( LPCWSTR in_pszDialogueEvent, LPCWSTR * in_aArgumentValues, long in_cNumArguments, ISoundObject ** out_ppSoundObject );
**/

	virtual bool SetPlayBackMode( bool in_bPlayback ) const;

	virtual bool GetCurrentState( AkStateGroupID in_stateGroupID, IState** out_ppCurrentState ) const;
	virtual bool GetCurrentState( LPCWSTR in_szStateGroupName, IState** out_ppCurrentState ) const;

	virtual bool SetCurrentState( AkStateGroupID in_stateGroupID, AkStateID in_currentStateID );
	virtual bool SetCurrentState( LPCWSTR in_szStateGroupName, LPCWSTR in_szCurrentStateName );

	virtual bool GetCurrentSwitch( AkSwitchGroupID in_switchGroupID, ISwitch** out_ppCurrentSwitch, AkGameObjectID in_gameObjectID = IGameObject::s_WwiseGameObject ) const;
	virtual bool GetCurrentSwitch( LPCWSTR in_szSwitchGroupName, ISwitch** out_ppCurrentSwitch, AkGameObjectID in_gameObjectID = IGameObject::s_WwiseGameObject ) const;

	virtual bool SetCurrentSwitch( AkSwitchGroupID in_switchGroupID, AkSwitchStateID in_currentSwitchID, AkGameObjectID in_gameObjectID = IGameObject::s_WwiseGameObject );
	virtual bool SetCurrentSwitch( LPCWSTR in_szSwitchGroupName, LPCWSTR in_szCurrentSwitchName, AkGameObjectID in_gameObjectID = IGameObject::s_WwiseGameObject );

	virtual bool RegisterGameObject( AkGameObjectID in_gameObjectID, LPCWSTR in_szGameObjectName = L"" );
	virtual bool UnregisterGameObject( AkGameObjectID in_gameObjectID );

	virtual bool SetRTPCValue( AkRtpcID in_gameParameterID, AkRtpcValue in_value, AkGameObjectID in_gameObjectID = IGameObject::s_WwiseGameObject );
	virtual bool SetRTPCValue( LPCWSTR in_szGameParameterName, AkRtpcValue in_value, AkGameObjectID in_gameObjectID = IGameObject::s_WwiseGameObject );

	virtual bool ResetRTPCValue( AkRtpcID in_gameParameterID, AkGameObjectID in_gameObjectID = IGameObject::s_WwiseGameObject );
	virtual bool ResetRTPCValue( LPCWSTR in_szGameParameterName, AkGameObjectID in_gameObjectID = IGameObject::s_WwiseGameObject );

	virtual bool PostTrigger( AkTriggerID in_triggerID, AkGameObjectID in_gameObjectID = IGameObject::s_WwiseGameObject );
	virtual bool PostTrigger( LPCWSTR in_szTriggerName,	AkGameObjectID in_gameObjectID = IGameObject::s_WwiseGameObject );

	virtual bool SetActiveListeners( AkGameObjectID in_gameObjectID, AkUInt32 in_uiListenerMask );
	virtual bool SetAttenuationScalingFactor( AkGameObjectID in_GameObjectID, AkReal32 in_fAttenuationScalingFactor );
	virtual bool SetMultiplePositions(AkGameObjectID in_GameObjectID, const AkSoundPosition * in_pPositions, AkUInt16 in_NumPositions, AK::SoundEngine::MultiPositionType in_eMultiPositionType = AK::SoundEngine::MultiPositionType_MultiDirections );
	virtual bool SetPosition( AkGameObjectID in_gameObjectID, const AkSoundPosition& in_rPosition );
	virtual bool SetListenerPosition( const AkListenerPosition& in_rPosition, AkUInt32 in_uiIndex = 0 );
	virtual bool SetListenerScalingFactor( AkUInt32 in_uiIndex, AkReal32 in_fAttenuationScalingFactor );
	virtual bool SetListenerSpatialization( AkUInt32 in_uiIndex, bool in_bSpatialized, AkSpeakerVolumes * in_pVolumeOffsets = NULL );

	virtual bool SetGameObjectAuxSendValues( AkGameObjectID in_gameObjectID, AkAuxSendValue* in_aEnvironmentValues, AkUInt32 in_uNumEnvValues );
	virtual bool SetGameObjectOutputBusVolume( AkGameObjectID in_gameObjectID, AkReal32 in_fControlValue );
	virtual bool SetObjectObstructionAndOcclusion( AkGameObjectID in_ObjectID, AkUInt32 in_uListener, AkReal32 in_fObstructionLevel, AkReal32 in_fOcclusionLevel );

	virtual bool PostMsgMonitor( LPCWSTR in_pszMessage );
	virtual bool StopAll( AkGameObjectID in_GameObjID );
	virtual bool StopPlayingID( AkPlayingID in_playingID, AkTimeMs in_uTransitionDuration = 0, AkCurveInterpolation in_eFadeCurve = AkCurveInterpolation_Linear );

	virtual bool GetEventList( IEventList ** out_ppEventList ) const;
	virtual bool GetEvent( AkUniqueID in_eventID, IEvent ** out_ppEvent ) const;
	virtual bool GetEvent( LPCWSTR in_pszEvent, IEvent ** out_ppEvent ) const;
	virtual bool GetEvents( const AkUniqueID * in_pEvents, long in_cEvents, IEventList ** out_ppEventList ) const;
	virtual bool GetEvents( LPCWSTR * in_pszEvents, long in_cEvents, IEventList ** out_ppEventList ) const;

	virtual bool GetDialogueEventList( IDialogueEventList ** out_ppDialogueEventList ) const;
	virtual bool GetDialogueEvent( AkUniqueID in_dialogueEventID, IDialogueEvent ** out_ppDialogueEvent ) const;
	virtual bool GetDialogueEvent( LPCWSTR in_pszDialogueEvent, IDialogueEvent ** out_ppDialogueEvent ) const;
	virtual bool GetDialogueEvents( const AkUniqueID * in_pDialogueEvents, long in_cDialogueEvents, IDialogueEventList ** out_ppDialogueEventList ) const;
	virtual bool GetDialogueEvents( LPCWSTR * in_pszDialogueEvents, long in_cDialogueEvents, IDialogueEventList ** out_ppDialogueEventList ) const;

	virtual bool GetSoundObject( AkUniqueID in_soundObjectID, ISoundObject ** out_ppSoundObject ) const;
	virtual bool GetSoundObjects( const AkUniqueID * in_pSoundObjects, long in_nSoundObjects, ISoundObjectList ** out_ppSoundObjectList ) const;

	virtual bool GetStateGroupList( IStateGroupList ** out_ppStateGroupList	) const;
	virtual bool GetStateGroup( AkUniqueID in_stateGroupID,	IStateGroup ** out_ppStateGroup	) const;
	virtual bool GetStateGroup( LPCWSTR in_pszStateGroup, IStateGroup ** out_ppStateGroup ) const;
	virtual bool GetStateGroups( const AkUniqueID * in_pStateGroups, long in_cStateGroups, IStateGroupList ** out_ppStateGroupList ) const;
	virtual bool GetStateGroups( LPCWSTR * in_pszStateGroups, long in_cStateGroups, IStateGroupList ** out_ppStateGroupList ) const;

	virtual bool GetSwitchGroupList( ISwitchGroupList ** out_ppSwitchGroupList	) const;
	virtual bool GetSwitchGroup( AkUniqueID in_switchGroupID,	ISwitchGroup ** out_ppSwitchGroup	) const;
	virtual bool GetSwitchGroup( LPCWSTR in_pszSwitchGroup, ISwitchGroup ** out_ppSwitchGroup ) const;
	virtual bool GetSwitchGroups( const AkUniqueID * in_pSwitchGroups, long in_cSwitchGroups, ISwitchGroupList ** out_ppSwitchGroupList ) const;
	virtual bool GetSwitchGroups( LPCWSTR * in_pszSwitchGroups, long in_cSwitchGroups, ISwitchGroupList ** out_ppSwitchGroupList ) const;

	virtual bool GetGameObjectList( IGameObjectList ** out_ppGameObjectList	) const;

	virtual bool GetEventOriginalFileList( LPCWSTR in_pszEvent, IOriginalFileList ** out_ppOriginalFileList	) const;
	virtual bool GetDialogueEventOriginalFileList( LPCWSTR in_pszDialogueEvent, IOriginalFileList ** out_ppOriginalFileList	) const;

	virtual bool EventHasVoiceContent( LPCWSTR  in_pszEvent ) const;
	virtual bool DialogueEventHasVoiceContent( LPCWSTR  in_pszDialogueEvent	) const;

	virtual bool GetGameParameterList( IGameParameterList ** out_ppGameParameterList ) const;
	virtual bool GetGameParameter( AkUniqueID in_gameParameterID, IGameParameter ** out_ppGameParameter	) const;
	virtual bool GetGameParameter( LPCWSTR in_pszGameParameter, IGameParameter ** out_ppGameParameter ) const;
	virtual bool GetGameParameters( const AkUniqueID * in_pGameParameters, long in_cGameParameters, IGameParameterList ** out_ppGameParameterList ) const;
	virtual bool GetGameParameters( LPCWSTR * in_pszGameParameters, long in_cGameParameters, IGameParameterList ** out_ppGameParameterList ) const;
	virtual bool GetConversionSettingsList( IConversionSettingsList ** out_ppConversionSettingsList ) const;

	virtual bool GetTriggerList( ITriggerList ** out_ppTriggerList ) const;
	virtual bool GetTrigger( AkUniqueID in_triggerID, ITrigger ** out_ppTrigger	) const;
	virtual bool GetTrigger( LPCWSTR in_pszTrigger, ITrigger ** out_ppTrigger ) const;
	virtual bool GetTriggers( const AkUniqueID * in_pTriggers, long in_cTriggers, ITriggerList ** out_ppTriggerList ) const;
	virtual bool GetTriggers( LPCWSTR * in_pszTriggers, long in_cTriggers, ITriggerList ** out_ppTriggerList ) const;

	virtual bool GetArgument( AkUniqueID in_ArgumentID, IArgument ** out_ppArgument ) const;
	virtual bool GetArguments( const AkUniqueID * in_pArguments, long in_cArguments, IArgumentList ** out_ppArgumentList ) const;
	
	virtual bool GetAuxBusList( IAuxBusList ** out_ppAuxBusList ) const;
	virtual bool GetAuxBus( AkUniqueID in_environmentID, IAuxBus ** out_ppAuxBus ) const;
	virtual bool GetAuxBus( LPCWSTR in_pszAuxBus, IAuxBus ** out_ppAuxBus ) const;
	virtual bool GetAuxBus( const AkUniqueID * in_pAuxBus, long in_cAuxBus, IAuxBusList ** out_ppAuxBusList ) const;
	virtual bool GetAuxBus( LPCWSTR * in_pszAuxBus, long in_cAuxBus, IAuxBusList ** out_ppAuxBusList ) const;

	virtual DnDType GetDnDType(	IDataObject * in_pDataObject );
	virtual bool ProcessEventDnD( IDataObject * in_pDataObject, IEventList ** out_ppEventList );
	virtual bool ProcessStateGroupDnD( IDataObject * in_pDataObject, IStateGroupList ** out_ppStateGroupList );
	virtual bool ProcessSwitchGroupDnD( IDataObject * in_pDataObject, ISwitchGroupList ** out_ppSwitchGroupList );
	virtual bool ProcessGameParameterDnD( IDataObject * in_pDataObject,	IGameParameterList ** out_ppGameParameterList );
	virtual bool ProcessTriggerDnD( IDataObject * in_pDataObject,	ITriggerList ** out_ppTriggerList );
	virtual bool ProcessAuxBusDnD( IDataObject * in_pDataObject,	IAuxBusList ** out_ppAuxBusList );
	virtual bool ProcessDialogueEventDnD( IDataObject * in_pDataObject, IDialogueEventList ** out_ppDialogueEventList );

	virtual const WCHAR * GetCurrentProjectName() const;
	virtual const WCHAR * GetCurrentProjectOriginalRoot() const;
	virtual GUID GetCurrentProjectID() const;

	virtual bool ProcessDefinitionFiles( LPCWSTR * in_pszPaths, long in_nFiles );
	virtual bool GenerateSoundBanks( LPCWSTR * in_pszBanks, long in_nBanks, 
				                     LPCWSTR * in_pszPlatforms, long in_nPlatforms,
									 LPCWSTR * in_pszLanguages, long in_nLanguages );

	virtual bool ConvertExternalSources( LPCWSTR * in_pszPlatforms, long in_cPlatforms,
											LPCWSTR * in_pszFileSourcesInput, long in_cFileSourceInput,
											LPCWSTR * in_pszFileSourcesOutput, long in_cFileSourceOutput );

	virtual bool ListenAttenuation( const AkUniqueID * in_pSoundObjects, long in_nSoundObjects );

	virtual bool GetAkUniqueID( const GUID& in_guid, AkUniqueID& out_uniqueID ) const;

	virtual bool GetWwiseObjectPath( const AK::SoundFrame::ISFObject* in_pObject, const AK::SoundFrame::ISFObject* in_pParent, LPWSTR out_szBuffer, long in_lLength ) const;

private:
	DnDType ConvertType( long in_lType, long in_lCustomParam );
	bool	GetDnDObjectList( IDataObject * in_pDataObject, AkUniqueID ** out_pIDArray, long& out_lCount );
	void	OnServerStartup( HWND in_hwndServer, ULONG in_ulServerVersion );
	void	OnServerShutdown();

	void	SendCopyData( DWORD in_dwMessageID, SFWriteBytesMem& in_rWriteByte ) const;
	BOOL    ReceiveCopyData( HWND hwndSender, COPYDATASTRUCT * pCopyData );

	void	WriteGameObjectID(SFWriteBytesMem &in_rWriter, AkGameObjectID in_id) const;

	template< class TObject, class TList >
	TList* GetCopyDataObjectList( SFReadBytesMem& bytes ) const;

	template<class TList>
	bool ProcessObjectDnD( IDataObject * in_pDataObject, TList ** out_ppObjectList, SFObjectType in_eType );

	template<class TList>
	bool GetObjectList( TList ** out_ppObjectList, UINT in_msg ) const;

	template<class TObject, class TList>
	bool GetObject( AkUniqueID in_id, TObject ** out_pObject, SFObjectType in_eType ) const;

	template<class TObject, class TList>
	bool GetObjectByString( LPCWSTR in_szObject, TObject ** out_pObject, SFObjectType in_eType ) const;

	template<class TList>
	bool GetObjects( const AkUniqueID * in_pObjects, long in_cObjects, TList ** out_ppObjectList, SFObjectType in_eType ) const;

	template<class TList>
	bool GetObjectsByString( LPCWSTR * in_pszObjects, long in_cObjects, TList ** out_ppObjectList, SFObjectType in_eType ) const;

	bool SendGetGUIDRequest( SFWriteBytesMem& bytes, GUID& out_GUID ) const;
	bool GetGUID_Type( WObjectType in_eType, AkUniqueID in_id, GUID& out_GUID ) const;
	bool GetGUID_Sibling( const GUID& in_guidParent, AkUniqueID in_id, GUID& out_GUID ) const;

	static LRESULT CALLBACK MsgWndProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );

	HWND m_hwndMessage; // message window for communication.
	HWND m_hwndServer;
	DWORD m_dwProcessID; // optional process id of server.

	IClient * m_pClient;

	mutable void * m_pCopyData; // Holds our data while we go up the call stack when receiving a WM_COPYDATA.

	CLIPFORMAT m_cfWObjectID; // Clipboard format for WObject IDs

	bool  m_bProjectInfoValid;
	WCHAR m_wszProjectName[ kStrSize ];
	WCHAR m_wszOriginalPath[ MAX_PATH ];
	GUID  m_guidProject;
};
