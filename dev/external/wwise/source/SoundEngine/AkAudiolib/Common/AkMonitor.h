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

//////////////////////////////////////////////////////////////////////
//
// AkMonitor.h
//
// alessard
//
//////////////////////////////////////////////////////////////////////

#ifndef _AKMONITOR_H_
#define _AKMONITOR_H_

#include "IALMonitor.h"
#include <AK/Tools/Common/AkLock.h>

#include "AkMonitorData.h"

#include "AkChunkRing.h"
#include "AkKeyList.h"
#include "AkList2.h"
#include <AK/Tools/Common/AkPlatformFuncs.h>
#include "AkIDStringMap.h"
#include "AkStateMgr.h"

using namespace AKPLATFORM;

static inline AkWwiseGameObjectID GameObjectToWwiseGameObject( AkGameObjectID in_GameObject )
{
	// IMPORTANT: Because AK_INVALID_GAME_OBJECT is (-1), we need to propagate the sign bit 
	// when AkGameObjectID is smaller than AkWwiseGameObjectID.
	if ( in_GameObject==AK_INVALID_GAME_OBJECT && sizeof(AkGameObjectID)!=sizeof(AkWwiseGameObjectID) )
		return WWISE_INVALID_GAME_OBJECT;
	return (AkWwiseGameObjectID)in_GameObject;
}

class CAkUsageSlot;
class AkMediaEntry;

class AkMonitor 
	: public AK::IALMonitor
{
public:
    virtual ~AkMonitor();

	//Singleton
	static AkMonitor* Instance();
	static AkMonitor* Get() { return m_pInstance; }
	static void Destroy();

	// IALMonitor members
    virtual void Register( AK::IALMonitorSink* in_pMonitorSink, AkMonitorData::MaskType in_whatToMonitor );
    virtual void Unregister( AK::IALMonitorSink* in_pMonitorSink );
	virtual void SetMeterWatches( AkMonitorData::MeterWatch* in_pWatches, AkUInt32 in_uiWatchCount );
	virtual void SetWatches( AkMonitorData::Watch* in_pWatches, AkUInt32 in_uiWatchCount );
	virtual void SetGameSyncWatches( AkUniqueID* in_pWatches, AkUInt32 in_uiWatchCount );

	static void RecapRegisteredObjects();
	static void RecapSwitches();
	static void RecapMemoryPools();
    static void RecapDevices();
    static void RecapStreamRecords();

	static void RecapDataSoundBank();
	static void RecapMedia();
	static void RecapEvents();
	static void RecapGameSync();

	static void RecapSinkTypes();

	// AkMonitor members
	AKRESULT StartMonitoring();
	void StopMonitoring();

	static bool IsMonitoring() { return !Get()->m_sink2Filter.IsEmpty(); }
	static AkMonitorData::MaskType GetNotifFilter() { return Get()->m_uiNotifFilter; } // what is being monitored
	static bool GetAndClearMeterWatchesDirty() { bool bDirty = Get()->m_bMeterWatchesDirty; Get()->m_bMeterWatchesDirty = false; return bDirty; }

	static void PostWatchedGameObjPositions();
	static void PostWatchesRTPCValues();

	static AkTimeMs GetThreadTime(){return m_ThreadTime;}
	static void SetThreadTime(AkTimeMs in_ThreadTime){m_ThreadTime = in_ThreadTime;}

	static void Monitor_PostCode( AK::Monitor::ErrorCode in_eErrorCode, AK::Monitor::ErrorLevel in_eErrorLevel, AkPlayingID in_playingID = AK_INVALID_PLAYING_ID, AkGameObjectID in_gameObjID = AK_INVALID_GAME_OBJECT, AkUniqueID in_soundID = AK_INVALID_UNIQUE_ID, bool in_bIsBus = false );
	static void Monitor_PostCodeWithParam( AK::Monitor::ErrorCode in_eErrorCode, AK::Monitor::ErrorLevel in_eErrorLevel, AkUInt32 in_param1, AkPlayingID in_playingID = AK_INVALID_PLAYING_ID, AkGameObjectID in_gameObjID = AK_INVALID_GAME_OBJECT, AkUniqueID in_soundID = AK_INVALID_UNIQUE_ID, bool in_bIsBus = false );
#ifdef AK_SUPPORT_WCHAR	
	static void Monitor_PostString( const wchar_t* in_pszError, AK::Monitor::ErrorLevel in_eErrorLevel );
#endif //AK_SUPPORT_WCHAR	
	static void Monitor_PostString( const char* in_pszError, AK::Monitor::ErrorLevel in_eErrorLevel );
	static void Monitor_ObjectNotif( AkPlayingID in_PlayingID, AkGameObjectID in_GameObject, const AkCustomParamType & in_CustomParam, AkMonitorData::NotificationReason in_eNotifReason, AkCntrHistArray in_cntrHistArray, AkUniqueID in_targetObjectID, bool in_bTargetIsBus, AkTimeMs in_timeValue, AkUniqueID in_playlistItemID = AK_INVALID_UNIQUE_ID );
	static void Monitor_MarkersNotif( AkPlayingID in_PlayingID, AkGameObjectID in_GameObject, const AkCustomParamType & in_CustomParam, AkMonitorData::NotificationReason in_eNotifReason, AkCntrHistArray in_cntrHistArray, const char* in_strLabel );
	static void Monitor_BankNotif( AkUniqueID in_BankID, AkUniqueID in_LanguageID, AkMonitorData::NotificationReason in_eNotifReason, AkUInt32 in_uPrepareFlags );
	static void Monitor_PrepareNotif( AkMonitorData::NotificationReason in_eNotifReason, AkUniqueID in_GameSyncorEventID, AkUInt32 in_groupID, AkGroupType in_GroupType, AkUInt32 in_NumEvents );
	static void Monitor_StateChanged( AkStateGroupID in_StateGroup, AkStateID in_PreviousState, AkStateID in_NewState );
	static void Monitor_SwitchChanged( AkSwitchGroupID in_SwitchGroup, AkSwitchStateID in_Switch, AkGameObjectID in_GameObject );
	static void Monitor_ObjectRegistration( bool in_isRegistration, AkGameObjectID in_GameObject, void * in_pMonitorData, bool in_bRecap = false );
	static void Monitor_FreeString( void * in_pMonitorData );
	static void Monitor_ParamChanged( AkMonitorData::NotificationReason in_eNotifReason, AkUniqueID in_ElementID, bool in_bIsBusElement, AkGameObjectID in_GameObject );
	static void Monitor_EventTriggered( AkPlayingID in_PlayingID, AkUniqueID in_EventID, AkGameObjectID in_GameObject, const AkCustomParamType & in_CustomParam);
	static void Monitor_ActionDelayed( AkPlayingID in_PlayingID, AkUniqueID in_ActionID, AkGameObjectID in_GameObject, AkTimeMs in_DelayTime, const AkCustomParamType & in_CustomParam );
	static void Monitor_ActionTriggered( AkPlayingID in_PlayingID, AkUniqueID in_ActionID, AkGameObjectID in_GameObject, const AkCustomParamType & in_CustomParam );
	static void Monitor_BusNotification( AkUniqueID in_BusID, AkMonitorData::BusNotification in_NotifReason, AkUInt32 in_bitsFXBypass, AkUInt32 in_bitsMask );
	static void Monitor_PathEvent( AkPlayingID _playingI_D, AkUniqueID _who_, AkMonitorData::AkPathEvent _event_, AkUInt32 _index_);
	static void Monitor_MusicTransNotif( AkPlayingID in_PlayingID, AkGameObjectID in_GameObject, AkMonitorData::NotificationReason in_eNotifReason, AkUInt32 in_transRuleIndex, AkUniqueID in_switchCntrID, AkUniqueID in_nodeSrcID, AkUniqueID in_nodeDestID, AkUniqueID in_segmentSrcID, AkUniqueID in_segmentDestID, AkUniqueID in_cueSrc, AkUniqueID in_cueDest,	AkTimeMs in_time );
	static void Monitor_TimeStamp();
	
#ifdef AK_SUPPORT_WCHAR
	static void Monitor_errorMsg2( const wchar_t* in_psz1, const wchar_t* in_psz2 );
#endif //AK_SUPPORT_WCHAR	
	static void Monitor_errorMsg2( const char* in_psz1, const char* in_psz2 );

	static void Monitor_LoadedBank( CAkUsageSlot* in_pUsageSlot, bool in_bIsDestroyed );
	static void Monitor_MediaPrepared( AkMediaEntry& in_rMediaEntry );
	static void Monitor_EventPrepared( AkUniqueID in_EventID, AkUInt32 in_RefCount );
	static void Monitor_GameSync(  AkUniqueID in_GroupID, AkUniqueID in_GameSyncID, bool in_bIsEnabled, AkGroupType in_GroupType  );

#ifdef AK_SUPPORT_WCHAR
	static void Monitor_SetPoolName( AkMemPoolId in_PoolId, wchar_t * in_tcsPoolName );
#endif //AK_SUPPORT_WCHAR	
	static void Monitor_SetPoolName( AkMemPoolId in_PoolId, char * in_tcsPoolName );

	static void Monitor_SetParamNotif_Float( AkMonitorData::NotificationReason in_eNotifReason, AkUniqueID in_ElementID, bool in_bIsBusElement, AkGameObjectID in_GameObject, AkReal32 in_TargetValue, AkValueMeaning in_ValueMeaning, AkTimeMs in_TransitionTime );
	static void Monitor_Dialogue( AkMonitorData::MonitorDataType in_type, AkUniqueID in_idDialogueEvent, AkUniqueID in_idObject, AkUInt32 in_cPath, AkArgumentValueID * in_pPath, AkPlayingID in_idSequence, AkUInt16 in_uRandomChoice, AkUInt16 in_uTotalProbability, AkUInt8 in_uWeightedDecisionType, AkUInt32 in_uWeightedPossibleCount, AkUInt32 in_uWeightedTotalCount );
	static void Monitor_PluginSendData( void * in_pData, AkUInt32 in_uDataSize, AkUniqueID in_audioNodeID, AkPluginID in_pluginID, AkUInt32 in_uFXIndex );	

	static void Monitor_ExternalSourceData(AkPlayingID in_idPlay, AkGameObjectID in_idGameObj, AkUniqueID in_idSrc, const AkOSChar* in_pszFile);

	static void * Monitor_AllocateGameObjNameString( AkGameObjectID in_GameObject, const char* in_GameObjString );

	static AkMemPoolId Monitor_GetPoolId() { return Get()->m_MonitorPoolId; };

	static inline void SetLocalOutput( AkUInt32 in_uErrorLevel, AK::Monitor::LocalOutputFunc in_pMonitorFunc ) { m_uLocalOutputErrorLevel = in_uErrorLevel; m_funcLocalOutput = in_pMonitorFunc; }

	static AkUInt8 GetMeterWatchDataTypes( AkUniqueID in_busID );

	void Recap( AkMonitorData::MaskType newMonitorTypes );

	AKRESULT StartProfilerCapture( const AkOSChar* in_szFilename );
	AKRESULT StopProfilerCapture();

protected:
	//Singleton
	AkMonitor();

	// Helpers.
	static void Monitor_SendErrorData1( AK::Monitor::ErrorCode in_eErrorCode, AK::Monitor::ErrorLevel in_eErrorLevel, AkUInt32 in_param1, AkPlayingID in_playingID, AkGameObjectID in_gameObjID, AkUniqueID in_soundID, bool in_bIsBus );
	static void Monitor_PostToLocalOutput( AK::Monitor::ErrorCode in_eErrorCode, AK::Monitor::ErrorLevel in_eErrorLevel, const AkOSChar * in_pszError, AkPlayingID in_playingID, AkGameObjectID in_gameObjID );

private:

	AK_DEFINE_ARRAY_POOL( MonitorPoolDefault, AkMonitor::m_MonitorPoolId )

	AkEvent m_hMonitorEvent;
	AkEvent m_hMonitorDoneEvent;
	bool	m_bStopThread;

	friend struct AkMonitorDataCreator;
	friend struct AkProfileDataCreator;

	bool DispatchNotification();

    static void StartStreamProfiler( );
    static void StopStreamProfiler( );

	static void AddWatchesByGameObjectName( const char* in_pszGameObjectName );
	static void AddWatchForGameObject( AkGameObjectID in_GameObject, const char* in_pszGameObjectName );
	static void RemoveWatchForGameObject( AkGameObjectID in_GameObject );

	static void RecapGroupHelper(CAkStateMgr::PreparationGroups& in_Groups, AkGroupType in_Type );

	static AkTimeMs m_ThreadTime;

    static AK_DECLARE_THREAD_ROUTINE( MonitorThreadFunc );

	static AkMonitor* m_pInstance; // Pointer on the unique Monitor instance

	static AkIDStringHash m_idxGameObjectString;

	static AkThread		m_hThread;

	inline void SignalNotifyEvent()
	{
		AkSignalEvent( m_hMonitorEvent );
	};

    typedef CAkKeyList<AK::IALMonitorSink*, AkMonitorData::MaskType, AkFixedSize, MonitorPoolDefault> MonitorSink2Filter;
	MonitorSink2Filter m_sink2Filter;

	CAkLock m_registrationLock;
	
	AkChunkRing m_ringItems;

	AkMonitorData::MaskType m_uiNotifFilter; // Global filter 

	bool m_bMeterWatchesDirty;

	class AkLocalProfilerCaptureSink * m_pLocalProfilerCaptureSink;


	typedef AkHashList<AkGameObjectID, AkUInt32, AK_SMALL_HASH_SIZE> AkMapGameObjectWatch;
	static AkMapGameObjectWatch m_mapGameObjectWatch;
	static bool m_arrayListenerWatch[AK_NUM_LISTENERS];

	typedef AkArray<AkMonitorData::Watch,const AkMonitorData::Watch&,MonitorPoolDefault,1> WatchesArray;
	static WatchesArray m_watches;

	typedef AkArray<AkUniqueID,AkUniqueID,MonitorPoolDefault,1> GameSyncWatchesArray;
	static GameSyncWatchesArray m_gameSyncWatches;

	typedef AkHashList<AkUniqueID/*bus id*/, AkUInt8 /*bitmask of BusMeterDataType*/, AK_SMALL_HASH_SIZE> AkMeterWatchMap;
	static AkMeterWatchMap m_meterWatchMap;

	static AkMemPoolId m_MonitorQueuePoolId;
	static AkMemPoolId m_MonitorPoolId;
	static AkUInt32 m_uLocalOutputErrorLevel; // Bitfield of AK::Monitor::ErrorLevel
	static AK::Monitor::LocalOutputFunc m_funcLocalOutput;
};

// Use this to send a monitor data item: space is initialized in constructor, 
// item is 'queued' in destructor.
struct AkMonitorDataCreator
{
	AkMonitorDataCreator( AkMonitorData::MonitorDataType in_MonitorDataType, AkInt32 in_lSize );
	~AkMonitorDataCreator();

	AkMonitorData::MonitorDataItem * m_pData;

private:
	AkInt32 m_lSize;
};

// Use this to send a profiling data item: space is initialized in constructor, 
// item is 'queued' in destructor. The difference between this and AkMonitorDataCreator
// is that this one doesn't block on a full queue, it just skips the item (as profiling info
// is not 'critical')
struct AkProfileDataCreator
{
	AkProfileDataCreator( AkInt32 in_lSize );
	~AkProfileDataCreator();

	AkMonitorData::MonitorDataItem * m_pData;

private:
	AkInt32 m_lSize;
};

//Please use macros

//The monitor is started and Stopped no matter the build type

#ifndef AK_OPTIMIZED

#define MONITOR_BANKNOTIF( in_BankID, in_LanguageID, in_eNotifReason )\
		AkMonitor::Monitor_BankNotif( in_BankID, in_LanguageID, in_eNotifReason, 0 )

#define MONITOR_PREPAREEVENTNOTIF( in_eNotifReason, in_EventID )\
		AkMonitor::Monitor_PrepareNotif( in_eNotifReason, in_EventID, 0, AkGroupType_Switch, 0 )

#define MONITOR_PREPARENOTIFREQUESTED( in_eNotifReason, in_NumNotifs )\
		AkMonitor::Monitor_PrepareNotif( in_eNotifReason, 0, 0, AkGroupType_Switch, in_NumNotifs )

#define MONITOR_PREPAREGAMESYNCNOTIF( in_eNotifReason, in_GameSyncID, in_GroupID, in_GroupType )\
		AkMonitor::Monitor_PrepareNotif( in_eNotifReason, in_GameSyncID, in_GroupID, in_GroupType, 0 )

#define MONITOR_PREPAREBANKREQUESTED( in_eNotifReason, in_BankID, in_uPrepareFlags )\
		AkMonitor::Monitor_BankNotif( in_BankID, AK_INVALID_UNIQUE_ID, in_eNotifReason, in_uPrepareFlags )

#define MONITOR_STATECHANGED(in_StateGroup, in_PreviousState, in_NewState)\
		AkMonitor::Monitor_StateChanged(in_StateGroup, in_PreviousState, in_NewState)

#define MONITOR_SWITCHCHANGED( in_SwitchGroup, in_Switch, in_GameObject )\
		AkMonitor::Monitor_SwitchChanged( in_SwitchGroup, in_Switch, in_GameObject )

#define MONITOR_OBJREGISTRATION( in_isRegistration, in_GameObject, in_GameObjString )\
		AkMonitor::Monitor_ObjectRegistration( in_isRegistration, in_GameObject, in_GameObjString )

#define MONITOR_FREESTRING( in_GameObjString )\
		AkMonitor::Monitor_FreeString( in_GameObjString )

#define MONITOR_OBJECTNOTIF( in_PlayingID, in_GameObject, in_CustomParam, in_eNotifReason, in_cntrHistArray, in_targetObjectID, in_bTargetIsBus, in_timeValue )\
		AkMonitor::Monitor_ObjectNotif( in_PlayingID, in_GameObject, in_CustomParam, in_eNotifReason, in_cntrHistArray, in_targetObjectID, in_bTargetIsBus, in_timeValue )

#define MONITOR_MUSICOBJECTNOTIF( in_PlayingID, in_GameObject, in_CustomParam, in_eNotifReason, in_targetObjectID, in_playlistItemID )\
		AkMonitor::Monitor_ObjectNotif( in_PlayingID, in_GameObject, in_CustomParam, in_eNotifReason, CAkCntrHist(), in_targetObjectID, false, 0, in_playlistItemID );

#define MONITOR_EVENTENDREACHEDNOTIF( in_PlayingID, in_GameObject, in_CustomParam, in_EventID_ )\
		AkMonitor::Monitor_ObjectNotif( in_PlayingID, in_GameObject, in_CustomParam, AkMonitorData::NotificationReason_EventEndReached, CAkCntrHist(), false, 0, 0 );

#define MONITOR_EVENTMARKERNOTIF( in_PlayingID, in_GameObject, in_CustomParam, in_EventID_, in_strLabel )\
		AkMonitor::Monitor_MarkersNotif( in_PlayingID, in_GameObject, in_CustomParam, AkMonitorData::NotificationReason_EventMarker, CAkCntrHist(), in_strLabel );

#define MONITOR_PARAMCHANGED( in_eNotifReason, in_ElementID, in_bIsBusElement, in_GameObject )\
		AkMonitor::Monitor_ParamChanged( in_eNotifReason, in_ElementID, in_bIsBusElement, in_GameObject )

#define MONITOR_SETPARAMNOTIF_FLOAT( in_eNotifReason, in_ElementID, in_bIsBusElement, in_GameObject, in_TargetValue, in_ValueMeaning, in_TransitionTime ) \
		AkMonitor::Monitor_SetParamNotif_Float( in_eNotifReason, in_ElementID, in_bIsBusElement, in_GameObject, in_TargetValue, in_ValueMeaning, in_TransitionTime )

#define MONITOR_ERROR( in_ErrorCode )\
		AkMonitor::Monitor_PostCode( in_ErrorCode, AK::Monitor::ErrorLevel_Error )

#define MONITOR_ERRORMESSAGE( in_ErrorCode )\
		AkMonitor::Monitor_PostCode( in_ErrorCode, AK::Monitor::ErrorLevel_Message )

#define MONITOR_MSGEX( in_ErrorCode, in_PlayingID, in_GameObject, in_WwiseObjectID, in_WwiseObjectIsBus )\
		AkMonitor::Monitor_PostCode( in_ErrorCode, AK::Monitor::ErrorLevel_Message, in_PlayingID, in_GameObject, in_WwiseObjectID )

#define MONITOR_ERROREX( in_ErrorCode, in_PlayingID, in_GameObject, in_WwiseObjectID, in_WwiseObjectIsBus )\
		AkMonitor::Monitor_PostCode( in_ErrorCode, AK::Monitor::ErrorLevel_Error, in_PlayingID, in_GameObject, in_WwiseObjectID )

#define MONITOR_ERROR_PARAM( in_ErrorCode, in_param1, in_PlayingID, in_GameObject, in_WwiseObjectID, in_WwiseObjectIsBus )\
		AkMonitor::Monitor_PostCodeWithParam( in_ErrorCode, AK::Monitor::ErrorLevel_Error, in_param1, in_PlayingID, in_GameObject, in_WwiseObjectID, in_WwiseObjectIsBus )

#define MONITOR_SOURCE_ERROR( in_ErrorCode, in_pContext )\
		{	\
			AkSrcTypeInfo * _pSrcType = in_pContext->GetSrcTypeInfo();	\
			AkMonitor::Monitor_PostCodeWithParam( in_ErrorCode, AK::Monitor::ErrorLevel_Error, _pSrcType->mediaInfo.sourceID, in_pContext->GetPlayingID(), in_pContext->GetGameObjectPtr()->ID(), in_pContext->GetSoundID(), false );	\
		}

#define MONITOR_PLUGIN_ERROR( in_ErrorCode, in_pContext, in_pluginID )\
			AkMonitor::Monitor_PostCodeWithParam( in_ErrorCode, AK::Monitor::ErrorLevel_Error, in_pluginID, in_pContext->GetPlayingID(), in_pContext->GetGameObjectPtr()->ID(), in_pContext->GetSoundID(), false );	\
		
#define MONITOR_EVENTTRIGGERED( in_PlayingID, in_EventID, in_GameObject, in_CustomParam )\
		AkMonitor::Monitor_EventTriggered( in_PlayingID, in_EventID, in_GameObject, in_CustomParam )

#define MONITOR_ACTIONDELAYED( in_PlayingID, in_ActionID, in_GameObject, in_DelayTime, in_CustomParam )	\
		AkMonitor::Monitor_ActionDelayed( in_PlayingID, in_ActionID, in_GameObject, in_DelayTime, in_CustomParam )

#define MONITOR_ACTIONTRIGGERED( in_PlayingID, in_ActionID, in_GameObject, in_CustomParam )	\
		AkMonitor::Monitor_ActionTriggered( in_PlayingID, in_ActionID, in_GameObject, in_CustomParam )

#define MONITOR_BUSNOTIFICATION( in_BusID, in_NotifReason, in_bitsFXBypass, in_bitsMask )\
		AkMonitor::Monitor_BusNotification( in_BusID, in_NotifReason, in_bitsFXBypass, in_bitsMask )	

#define MONITOR_PATH_EVENT( _playingID_, _who_, _event_, _index_ )\
		AkMonitor::Monitor_PathEvent( _playingID_, _who_, _event_, _index_ )

#define MONITOR_ERRORMSG2( _MSG1_, _MSG2_ )\
		AkMonitor::Monitor_errorMsg2( _MSG1_, _MSG2_ )

#define MONITOR_ERRORMSG( _MSG1_ )\
		AkMonitor::Monitor_PostString( _MSG1_, AK::Monitor::ErrorLevel_Error )

#define MONITOR_MSG( _MSG_ )\
		AkMonitor::Monitor_PostString( _MSG_, AK::Monitor::ErrorLevel_Message );

#define MONITOR_LOADEDBANK( _BankSlot_, _IsDestroyed_ )\
		AkMonitor::Monitor_LoadedBank( _BankSlot_, _IsDestroyed_ )

#define MONITOR_MEDIAPREPARED( _MediaItem_ )\
		AkMonitor::Monitor_MediaPrepared( _MediaItem_ )

#define MONITOR_EVENTPREPARED( _EventID_, _RefCount_ )\
		AkMonitor::Monitor_EventPrepared( _EventID_, _RefCount_ )

#define MONITOR_GAMESYNC( _GroupID_, _SyncID_, _IsEnabled_, _SyncType_ )\
		AkMonitor::Monitor_GameSync( _GroupID_, _SyncID_, _IsEnabled_, _SyncType_ )

#define MONITOR_MUSICTRANS( in_PlayingID, in_GameObject, in_eNotifReason, in_transRuleIndex, in_switchCntrID, in_nodeSrcID, in_nodeDestID, in_segmentSrcID, in_segmentDestID, in_cueSrc, in_cueDest, in_time )\
		AkMonitor::Monitor_MusicTransNotif( in_PlayingID, in_GameObject, in_eNotifReason, in_transRuleIndex, in_switchCntrID, in_nodeSrcID, in_nodeDestID, in_segmentSrcID, in_segmentDestID, in_cueSrc, in_cueDest, in_time )

#define MONITOR_PLUGINSENDDATA( in_pData, in_uDataSize, in_audioNodeID, in_pluginID, in_uFXIndex )\
		AkMonitor::Monitor_PluginSendData( in_pData, in_uDataSize, in_audioNodeID, in_pluginID, in_uFXIndex );

#define MONITOR_RESOLVEDIALOGUE( in_idDialogueEvent, in_idResolved, in_cPath, in_pPath, in_idSequence, in_uRandomChoice, in_uTotalProbability, in_uWeightedDecisionType, in_uWeightedPossibleCount, in_uWeightedTotalCount )\
	AkMonitor::Monitor_Dialogue( AkMonitorData::MonitorDataResolveDialogue, in_idDialogueEvent, in_idResolved, in_cPath, in_pPath, in_idSequence, in_uRandomChoice, in_uTotalProbability, in_uWeightedDecisionType, in_uWeightedPossibleCount, in_uWeightedTotalCount )

#define MONITOR_TEMPLATESOURCE( _in_idPlay, _in_idGameObj, _in_idSrc, _in_szFile ) \
	AkMonitor::Monitor_ExternalSourceData( _in_idPlay, _in_idGameObj, _in_idSrc, _in_szFile );

#else

#define MONITOR_BANKNOTIF( in_BankID, in_LanguageID, in_eNotifReason )

#define MONITOR_PREPAREEVENTNOTIF( in_eNotifReason, in_EventID )

#define MONITOR_PREPARENOTIFREQUESTED( in_eNotifReason, in_NumNotifs )

#define MONITOR_PREPAREGAMESYNCNOTIF( in_eNotifReason, in_GameSyncID, in_GroupID, in_GroupType )

#define MONITOR_PREPAREBANKREQUESTED( in_eNotifReason, in_BankID, in_uPrepareFlags )

#define MONITOR_STATECHANGED(in_StateGroup, in_PreviousState, in_NewState)

#define MONITOR_SWITCHCHANGED( in_SwitchGroup, in_Switch, in_GameObject )

#define MONITOR_OBJREGISTRATION( in_isRegistration, in_GameObject, in_GameObjString )

#define MONITOR_FREESTRING( in_GameObjString )

#define MONITOR_OBJECTNOTIF( in_PlayingID, in_GameObject, in_CustomParam, in_eNotifReason, in_cntrHistArray, in_targetObjectID, in_bTargetIsBus, in_timeValue )

#define MONITOR_MUSICOBJECTNOTIF( in_PlayingID, in_GameObject, in_CustomParam, in_eNotifReason, in_targetObjectID, in_playlistItemID )

#define MONITOR_EVENTENDREACHEDNOTIF( in_PlayingID, in_GameObject, in_CustomParam, in_EventID_  )

#define MONITOR_EVENTMARKERNOTIF( in_PlayingID, in_GameObject, in_CustomParam, in_EventID_, in_strLabel  )

#define MONITOR_PARAMCHANGED( in_eNotifReason, in_ElementID, in_bIsBusElement, in_GameObject )

#define MONITOR_SETPARAMNOTIF_FLOAT( in_eNotifReason, in_ElementID, in_bIsBusElement, in_GameObject, in_TargetValue, in_ValueMeaning, in_TransitionTime ) 

#define MONITOR_ERROR( in_ErrorCode )

#define MONITOR_ERRORMESSAGE( in_ErrorCode )

#define MONITOR_MSGEX( in_ErrorCode, in_PlayingID, in_GameObject, in_WwiseObjectID, in_WwiseObjectIsBus )

#define MONITOR_ERROREX( in_ErrorCode, in_PlayingID, in_GameObject, in_WwiseObjectID, in_WwiseObjectIsBus )

#define MONITOR_ERROR_PARAM( in_ErrorCode, in_param1, in_PlayingID, in_GameObject, in_WwiseObjectID, in_WwiseObjectIsBus )

#define MONITOR_SOURCE_ERROR( in_ErrorCode, in_pContext )

#define MONITOR_PLUGIN_ERROR( in_ErrorCode, in_pContext, in_pluginID )

#define MONITOR_EVENTTRIGGERED( in_PlayingID, in_EventID, in_GameObject, in_CustomParam )

#define MONITOR_ACTIONDELAYED( in_PlayingID, in_ActionID, in_GameObject, in_DelayTime, in_CustomParam )

#define MONITOR_ACTIONTRIGGERED( in_PlayingID, in_ActionID, in_GameObject, in_CustomParam )

#define MONITOR_BUSNOTIFICATION( in_BusID, in_NotifReason, in_bitsFXBypass, in_bitsMask )

#define MONITOR_PATH_EVENT( _playingID_, _who_, _event_, _index_ )

#define MONITOR_ERRORMSG2( _MSG1_, _MSG2_ )

#define MONITOR_ERRORMSG( _MSG1_ )

#define MONITOR_MSG( _MSG_ )

#define MONITOR_LOADEDBANK( _BankSlot_, _IsDestroyed_ )

#define MONITOR_MEDIAPREPARED( _MediaItem_ )

#define MONITOR_EVENTPREPARED( _EventID_, _RefCount_ )

#define MONITOR_GAMESYNC( _GroupID_, _SyncID_, _IsEnabled_, _SyncType_ )

#define MONITOR_RESOLVEDIALOGUE( in_idDialogueEvent, in_idResolved, in_cPath, in_pPath, in_idSequence, in_uRandomChoice, in_uTotalProbability, in_uWeightedDecisionType, in_uWeightedPossibleCount, in_uWeightedTotalCount )

#define MONITOR_MUSICTRANS( in_PlayingID, in_GameObject, in_eNotifReason, in_transRuleIndex, in_switchCntrID, in_nodeSrcID, in_nodeDestID, in_segmentSrcID, in_segmentDestID, in_cueSrc, in_cueDest, in_time )

#define MONITOR_PLUGINSENDDATA( in_pData, in_uDataSize, in_audioNodeID, in_pluginID, in_uFXIndex )

#define MONITOR_TEMPLATESOURCE( _in_idPlay, _in_idGameObj, _in_idSrc, _in_szFile )

#endif

#endif	// _AKMONITOR_H_
