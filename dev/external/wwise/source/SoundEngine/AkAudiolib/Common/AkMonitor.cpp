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
// AkMonitor.cpp
//
// alessard
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"

#ifndef AK_OPTIMIZED

#define AK_MONITOR_IMPLEMENT_ERRORCODES // so that AkMonitorError.h defines the table

#include "AkLEngine.h"
#include "AkMonitor.h"
#include "IALMonitorSink.h"
#include <AK/Tools/Common/AkAutoLock.h>
#include "AkAudioLibTimer.h"
#include "AkAudioLib.h"
#include "AkEndianByteSwap.h"
#include "AkRegisteredObj.h"
#include "AkRTPCMgr.h"
#include "AkRegistryMgr.h"
#include "Ak3DListener.h"
#include "AkBankMgr.h"
#include "AkEvent.h"
#include "ICommunicationCentral.h"
#include "AkAudioMgr.h"
#include "CommandDataSerializer.h"
#include "AkBus.h"

#include "../../Communication/Common/CommunicationDefines.h"

extern AK::Comm::ICommunicationCentral * g_pCommCentral;

#include <ctype.h>

extern AkPlatformInitSettings g_PDSettings;

#define AK_MAX_MONITORING_SINKS				4
#define MAX_NOTIFICATIONS_PER_CALL			256

static int CompareNoCase( const char* in_s1, const char* in_s2, int in_n )
{
	int c( 0 );
	while( c < in_n )
	{
		if( tolower( in_s1[c] ) != tolower( in_s2[c] ) )
			return !0;

		++c;
	}
	
	return 0;
}

static bool DoesGameObjectNameMatch( const char* in_pszGameObjectWatchName, const char* in_pszGameObjectName )
{
	// If the name is empty, we don't consider it equal
	if( in_pszGameObjectWatchName == NULL || in_pszGameObjectWatchName[0] == 0 )
		return false;

	if( in_pszGameObjectWatchName[0] == '*' )
		return true;

	if( in_pszGameObjectName == NULL || in_pszGameObjectName[0] == 0 )
		return false;

	int iWatchNameLen = (int)strlen( in_pszGameObjectWatchName );
	if( in_pszGameObjectWatchName[iWatchNameLen-1] == '*' )
		return CompareNoCase( in_pszGameObjectWatchName, in_pszGameObjectName, iWatchNameLen-1 ) == 0;
	
	return CompareNoCase( in_pszGameObjectWatchName, in_pszGameObjectName, iWatchNameLen ) == 0;
}

extern AkInitSettings g_settings;

AkMonitor*	AkMonitor::m_pInstance 	= NULL;
AkIDStringHash AkMonitor::m_idxGameObjectString;

bool AkMonitor::m_arrayListenerWatch[AK_NUM_LISTENERS] = {false};
AkMonitor::AkMapGameObjectWatch AkMonitor::m_mapGameObjectWatch;
AkMonitor::WatchesArray AkMonitor::m_watches;
AkMonitor::GameSyncWatchesArray AkMonitor::m_gameSyncWatches;
AkMonitor::AkMeterWatchMap AkMonitor::m_meterWatchMap;

AkMemPoolId	AkMonitor::m_MonitorQueuePoolId = AK_INVALID_POOL_ID;
AkMemPoolId	AkMonitor::m_MonitorPoolId      = AK_INVALID_POOL_ID;
AkUInt32 AkMonitor::m_uLocalOutputErrorLevel = AK::Monitor::ErrorLevel_Error; 
AK::Monitor::LocalOutputFunc AkMonitor::m_funcLocalOutput = NULL;

AkThread AkMonitor::m_hThread;

AkMonitorDataCreator::AkMonitorDataCreator(	AkMonitorData::MonitorDataType in_MonitorDataType, AkInt32 in_lSize )
	: m_pData( NULL )
	, m_lSize( in_lSize )
{
	AkMonitor * pMonitor = AkMonitor::Get();

	// Note: Insufficient memory errors can land here after the monitor manager was term()ed.
	// Filter out the data types that are not requested.  It is useless to take memory in the queue for something that won't be sent.
	if ( !pMonitor || pMonitor->m_sink2Filter.IsEmpty() || (pMonitor->m_uiNotifFilter & AKMONITORDATATYPE_TOMASK( in_MonitorDataType )) == 0)
		return;

	// Retry queuing data until there is enough space. Another thread is emptying the queue
	// and signals MonitorDone when going back to sleep.
	while ( !( m_pData = (AkMonitorData::MonitorDataItem *) pMonitor->m_ringItems.BeginWrite( m_lSize ) ) )
		AkWaitForEvent( pMonitor->m_hMonitorDoneEvent );

	m_pData->eDataType = in_MonitorDataType;
}

AkMonitorDataCreator::~AkMonitorDataCreator()
{
	if ( !m_pData )
		return;

	AKASSERT( m_pData->eDataType < AkMonitorData::MonitorDataEndOfItems );
	AKASSERT( m_lSize == AkMonitorData::RealSizeof( *m_pData ) );

	AkMonitor * pMonitor = AkMonitor::Get();

	pMonitor->m_ringItems.EndWrite( m_pData, m_lSize );
	pMonitor->SignalNotifyEvent();
}

AkProfileDataCreator::AkProfileDataCreator( AkInt32 in_lSize )
	: m_pData( NULL )
	, m_lSize( in_lSize )
{
	AkMonitor * pMonitor = AkMonitor::Get();

	if ( pMonitor->m_sink2Filter.IsEmpty() )
		return;

	// Try once to write in the queue.
	m_pData = (AkMonitorData::MonitorDataItem *) pMonitor->m_ringItems.BeginWrite( m_lSize );
}

AkProfileDataCreator::~AkProfileDataCreator()
{
	if ( !m_pData )
		return;

	AKASSERT( m_pData->eDataType < AkMonitorData::MonitorDataEndOfItems );
	AKASSERT( m_lSize == AkMonitorData::RealSizeof( *m_pData ) );

	AkMonitor * pMonitor = AkMonitor::Get();

	pMonitor->m_ringItems.EndWrite( m_pData, m_lSize );
	pMonitor->SignalNotifyEvent();
}

class AkLocalProfilerCaptureSink
	: public AK::IALMonitorSink
{
public:
	AkLocalProfilerCaptureSink( AK::IAkStdStream * in_pStream )
		: m_pStream( in_pStream )
	{
		CommunicationDefines::ProfilingSessionHeader hdr;

		if ( CommunicationDefines::NeedEndianSwap( CommunicationDefines::g_eConsoleType ) )
		{
			hdr.uVersion = AK::EndianByteSwap::DWordSwap( CommunicationDefines::kProfilingSessionVersion );
			hdr.uProtocol = AK::EndianByteSwap::DWordSwap( AK_COMM_PROTOCOL_VERSION );
			hdr.uConsoleType = AK::EndianByteSwap::DWordSwap( CommunicationDefines::g_eConsoleType );
		}
		else
		{
			hdr.uVersion = CommunicationDefines::kProfilingSessionVersion;
			hdr.uProtocol = AK_COMM_PROTOCOL_VERSION;
			hdr.uConsoleType = CommunicationDefines::g_eConsoleType;
		}

		AkInt32 cWritten = 0;
		m_serializerOuter.GetWriter()->WriteBytes( (void *) &hdr, (AkUInt32) sizeof( hdr ), cWritten ); // bypass serializer to avoid size chunk being placed

		AkMonitor::Get()->Register( this, AkMonitorData::AllMonitorData );
	}

	~AkLocalProfilerCaptureSink()
	{
		AkMonitor::Get()->Unregister( this );

		if ( m_serializerOuter.GetWrittenSize() )
		{
			AkUInt32 uWritten = 0;
			m_pStream->Write( m_serializerOuter.GetWrittenBytes(), m_serializerOuter.GetWrittenSize(), true, AK_DEFAULT_PRIORITY, 0.0f, uWritten );
		}

		m_pStream->Destroy();
	}

    virtual void MonitorNotification( const AkMonitorData::MonitorDataItem& in_rMonitorItem, bool )
	{
		m_serializerInner.Put( in_rMonitorItem );
		m_serializerOuter.Put( (void *) m_serializerInner.GetWrittenBytes(), (AkUInt32) m_serializerInner.GetWrittenSize() );
	    m_serializerInner.GetWriter()->Clear();
		if ( m_serializerOuter.GetWrittenSize() >= 16384 )
		{
			AkUInt32 uWritten = 0;
			m_pStream->Write( m_serializerOuter.GetWrittenBytes(), (AkUInt32) m_serializerOuter.GetWrittenSize(), true, AK_DEFAULT_PRIORITY, 0.0f, uWritten );
		    m_serializerOuter.GetWriter()->Clear();
		}
	}

	virtual void FlushAccumulated()
	{
	}

private:
	AK::IAkStdStream * m_pStream;				// Profiling session written to this stream
    CommandDataSerializer m_serializerInner;
    CommandDataSerializer m_serializerOuter;
};

AkMonitor::AkMonitor()
	: m_uiNotifFilter( 0 )
	, m_pLocalProfilerCaptureSink( NULL )
{
	AKASSERT( AkMonitorData::MonitorDataEndOfItems <= 64 ); // If this asserts, MaskType needs to be bigger than 64 bits.
	AkClearThread(&m_hThread);
	AkClearEvent( m_hMonitorEvent );
	AkClearEvent( m_hMonitorDoneEvent );
	m_bStopThread = false;	
}

AkMonitor::~AkMonitor()
{
	StopMonitoring();
}

AkMonitor* AkMonitor::Instance()
{
    if (!m_pInstance)
    {
        m_pInstance = AkNew(g_DefaultPoolId, AkMonitor());
    }
    return m_pInstance;
}

void AkMonitor::Destroy()
{
	if (m_pInstance)
	{
		AkDelete(g_DefaultPoolId,m_pInstance);
		m_pInstance = NULL;
	}
}

void AkMonitor::Register( AK::IALMonitorSink* in_pMonitorSink, AkMonitorData::MaskType in_whatToMonitor )
{
	CAkFunctionCritical SpaceSetAsCritical;

	// Halt all other writers to the monitoring ring buffer, then flush it.
	// This is necessary so that the registering client does not receive notifications
	// prior to their respective recaps.

	m_ringItems.LockWrite();
	while ( !m_ringItems.IsEmpty() )
		AkWaitForEvent( m_hMonitorDoneEvent );

	// At this point, the monitoring thread is starved for notifications and goes back to sleep.

	m_registrationLock.Lock(); // Do all sink2filter operations inside the registration lock.
	
	// Preserve existing sinks, update global notify mask
	AkMonitorData::MaskType eTypesCurrent = 0;
	AkMonitorData::MaskType eTypesGlobal = in_whatToMonitor;
	int cSinks = m_sink2Filter.Length();
	MapStruct<AK::IALMonitorSink*, AkMonitorData::MaskType> aSinksCopy[ AK_MAX_MONITORING_SINKS ];
	{
		int i = 0;
		for( MonitorSink2Filter::Iterator it = m_sink2Filter.Begin(); it != m_sink2Filter.End(); ++it )
		{
			aSinksCopy[ i++ ]= (*it);

			if ( (*it).key == in_pMonitorSink )
			{
				eTypesCurrent = (*it).item;
				(*it).item = in_whatToMonitor;
			}
			else
				eTypesGlobal |= (*it).item;
		}
	}

	AkMonitorData::MaskType eTypesRecap = in_whatToMonitor & (~eTypesCurrent);
	if ( eTypesRecap )
	{
		// Register only recapping sink (other sinks must NOT receive recap)
		m_sink2Filter.RemoveAll();
		AkMonitorData::MaskType * pType = m_sink2Filter.Set( in_pMonitorSink );
		*pType = eTypesRecap;
		m_uiNotifFilter = eTypesRecap;

		m_registrationLock.Unlock();

		Recap( eTypesRecap );

		// Make sure recap is done before proceeding.
		while ( !m_ringItems.IsEmpty() )
			AkWaitForEvent( m_hMonitorDoneEvent );

		m_registrationLock.Lock();

		// Reinstate all other sinks
		for ( int i = 0; i < cSinks; ++i )
			m_sink2Filter.Set( aSinksCopy[ i ].key, aSinksCopy[ i ].item );
		*pType = in_whatToMonitor;
	}

	m_uiNotifFilter = eTypesGlobal;

	m_registrationLock.Unlock();

	m_ringItems.UnlockWrite();
}

void AkMonitor::Recap( AkMonitorData::MaskType newMonitorTypes )
{
	if( newMonitorTypes & AKMONITORDATATYPE_TOMASK( AkMonitorData::MonitorDataTimeStamp ) )
	{
		Monitor_TimeStamp();
	}

	if( newMonitorTypes & AKMONITORDATATYPE_TOMASK( AkMonitorData::MonitorDataObjRegistration ) )
	{
		RecapRegisteredObjects();
	}

	if( newMonitorTypes & AKMONITORDATATYPE_TOMASK( AkMonitorData::MonitorDataSwitch ) )
	{
		RecapSwitches();
	}

	if ( newMonitorTypes & AKMONITORDATATYPE_TOMASK( AkMonitorData::MonitorDataMemoryPoolName ) )
	{
        RecapMemoryPools();
	}

	if ( newMonitorTypes & AKMONITORDATATYPE_TOMASK( AkMonitorData::MonitorDataSoundBank ) )
	{
        RecapDataSoundBank();
	}

	if ( newMonitorTypes & AKMONITORDATATYPE_TOMASK( AkMonitorData::MonitorDataMedia ) )
	{
        RecapMedia();
	}

	if ( newMonitorTypes & AKMONITORDATATYPE_TOMASK( AkMonitorData::MonitorDataEvent ) )
	{
        RecapEvents();
	}

	if ( newMonitorTypes & AKMONITORDATATYPE_TOMASK( AkMonitorData::MonitorDataGameSync ) )
	{
        RecapGameSync();
	}

    if ( newMonitorTypes & AKMONITORDATATYPE_TOMASK( AkMonitorData::MonitorDataDevicesRecord ) )
	{
        // Note: On the stream mgr API, StartMonitoring() means "start listening",
        // which is true only when someone listens. 
        StartStreamProfiler( );

        RecapDevices();
	}

	if ( newMonitorTypes & AKMONITORDATATYPE_TOMASK( AkMonitorData::MonitorDataPipeline ) )
	{
		RecapSinkTypes();
	}

    if ( newMonitorTypes & AKMONITORDATATYPE_TOMASK( AkMonitorData::MonitorDataStreamsRecord ) )
	{
        // Note: On the stream mgr API, StartMonitoring() means "start listening",
        // which is true only when someone listens. 
        StartStreamProfiler( );
        RecapStreamRecords();
	}
}

void AkMonitor::Unregister( AK::IALMonitorSink* in_pMonitorSink )
{
	CAkFunctionCritical SpaceSetAsCritical;

	AkAutoLock<CAkLock> gate( m_registrationLock );

	m_sink2Filter.Unset( in_pMonitorSink );

	m_uiNotifFilter = 0;
	for( MonitorSink2Filter::Iterator it = m_sink2Filter.Begin(); it != m_sink2Filter.End(); ++it )
		m_uiNotifFilter |= (*it).item;

	if ( !( m_uiNotifFilter & ( AKMONITORDATATYPE_TOMASK(AkMonitorData::MonitorDataDevicesRecord) | AKMONITORDATATYPE_TOMASK(AkMonitorData::MonitorDataStreamsRecord) ) ) )
	{
		// Stop stream profiling if no one listens.
		// (See note near StopStreamProfiler( ) implementation).
		StopStreamProfiler();
	}
}

AkUInt8 AkMonitor::GetMeterWatchDataTypes( AkUniqueID in_busID )
{
	AkUInt8* pMask = m_meterWatchMap.Exists( in_busID );

	if( pMask )
		return *pMask;

	return 0;
}

void AkMonitor::SetMeterWatches( AkMonitorData::MeterWatch* in_pWatches, AkUInt32 in_uiWatchCount )
{
	m_meterWatchMap.RemoveAll();

	for( AkUInt32 i = 0; i < in_uiWatchCount; ++i )
	{
		AkUniqueID busID = in_pWatches[i].busID;
		
		AkUInt8 * pWatch = m_meterWatchMap.Set(busID);
		if ( pWatch ) // properly handle out-of-memory
			*pWatch = in_pWatches[i].uBusMeterDataTypes;
	}

	m_bMeterWatchesDirty = true;
}

void AkMonitor::SetWatches( AkMonitorData::Watch* in_pWatches, AkUInt32 in_uiWatchCount )
{
	// Clear the current watch
	for( int i = 0; i < AK_NUM_LISTENERS; ++i )
		m_arrayListenerWatch[i] = false;

	m_mapGameObjectWatch.RemoveAll();
	m_watches.RemoveAll();

	// Populate the watch list
	for( AkUInt32 i = 0; i < in_uiWatchCount; ++i )
	{
		switch( in_pWatches[i].eType )
		{
		case AkMonitorData::WatchType_GameObjectID:
		{
			CAkRegisteredObj* pObj = g_pRegistryMgr->GetObjAndAddref( (AkGameObjectID)in_pWatches[i].ID.gameObjectID );
			if( pObj )
			{
				pObj->Release();
				m_mapGameObjectWatch.Set( (AkGameObjectID)in_pWatches[i].ID.gameObjectID );
			}
			break;
		}
		case AkMonitorData::WatchType_GameObjectName:
			AddWatchesByGameObjectName( in_pWatches[i].szName );
			break;
		case AkMonitorData::WatchType_ListenerID:
			m_arrayListenerWatch[in_pWatches[i].ID.uiListenerID] = true;
			break;
		}

		m_watches.AddLast( in_pWatches[i] );
	}
}

void AkMonitor::SetGameSyncWatches( AkUniqueID* in_pWatches, AkUInt32 in_uiWatchCount )
{
	m_gameSyncWatches.RemoveAll();

	// Populate the watch list
	for( AkUInt32 i = 0; i < in_uiWatchCount; ++i )
	{
		m_gameSyncWatches.AddLast( in_pWatches[i] );
	}
}

void AkMonitor::AddWatchesByGameObjectName( const char* in_pszGameObjectWatchName )
{
	// For all existing game objects, check if they match a specific name
	for( AkIDStringHash::AkStringHash::Iterator iter = m_idxGameObjectString.m_list.Begin(); iter != m_idxGameObjectString.m_list.End(); ++iter )
	{
		if( DoesGameObjectNameMatch( in_pszGameObjectWatchName, &((*iter).item) ) )
		{
			m_mapGameObjectWatch.Set( (*iter).key );
		}
	}
}

void AkMonitor::AddWatchForGameObject( AkGameObjectID in_GameObject, const char* in_pszGameObjectName )
{
	// Check in the watch list
	for( WatchesArray::Iterator iter = m_watches.Begin(); iter != m_watches.End(); ++iter )
	{
		if( ((*iter).eType == AkMonitorData::WatchType_GameObjectID && 
			 (*iter).ID.gameObjectID == GameObjectToWwiseGameObject( in_GameObject ) )
			||
			((*iter).eType == AkMonitorData::WatchType_GameObjectName &&
			 DoesGameObjectNameMatch( (*iter).szName, in_pszGameObjectName ) ) )
		{
			m_mapGameObjectWatch.Set( in_GameObject );
		}
	}
}

void AkMonitor::RemoveWatchForGameObject( AkGameObjectID in_GameObject )
{
	m_mapGameObjectWatch.Unset( in_GameObject );
}

void AkMonitor::RecapRegisteredObjects()
{
	CAkRegistryMgr::AkMapRegisteredObj& regObjects = g_pRegistryMgr->GetRegisteredObjectList();
	for ( CAkRegistryMgr::AkMapRegisteredObj::Iterator iterObj = regObjects.Begin(); iterObj != regObjects.End(); ++iterObj )
	{
		AkGameObjectID gameObjID = (*iterObj).key;
		AkIDStringHash::AkStringHash::IteratorEx Iterator = m_idxGameObjectString.m_list.FindEx( gameObjID );
		Monitor_ObjectRegistration(true, gameObjID, Iterator.pItem, true);
	}
}

void AkMonitor::RecapSwitches()
{
	AKASSERT( g_pRTPCMgr );
	for( CAkRTPCMgr::AkMapSwitchEntries::Iterator iter = g_pRTPCMgr->m_SwitchEntries.Begin(); iter != g_pRTPCMgr->m_SwitchEntries.End(); ++iter )
	{
		const MapStruct<AkSwitchKey, AkSwitchStateID> & assoc = *iter;
		Monitor_SwitchChanged(assoc.key.m_SwitchGroup, assoc.item, assoc.key.m_pGameObj?assoc.key.m_pGameObj->ID():AK_INVALID_GAME_OBJECT);
	}
}

void AkMonitor::RecapMemoryPools()
{
	AkUInt32 ulNumPools = AK::MemoryMgr::GetMaxPools();
	for ( AkUInt32 ulPool = 0; ulPool < ulNumPools; ++ulPool )
	{
		AkOSChar * pszName = AK::MemoryMgr::GetPoolName(ulPool);
		if ( !pszName )
			continue;

		Monitor_SetPoolName(ulPool, pszName);
	}
}

void AkMonitor::RecapDataSoundBank()
{
	CAkBankList& rMainBankList = g_pBankManager->GetBankListRef();
	AkAutoLock<CAkBankList> BankListGate( rMainBankList );
	CAkBankList::AkListLoadedBanks& rBankList = rMainBankList.GetUNSAFEBankListRef();
	for( CAkBankList::AkListLoadedBanks::Iterator iter = rBankList.Begin(); iter != rBankList.End(); ++iter )
		Monitor_LoadedBank(iter.pItem, false);
}

void AkMonitor::RecapMedia()
{
	AkAutoLock<CAkLock> gate( g_pBankManager->m_MediaLock );

	CAkBankMgr::AkMediaHashTable::Iterator iter = g_pBankManager->m_MediaHashTable.Begin();
	while( iter != g_pBankManager->m_MediaHashTable.End() )
	{
		Monitor_MediaPrepared((*iter).item);
		++iter;
	}
}

void AkMonitor::RecapEvents()
{
	AKASSERT( g_pIndex );
	CAkIndexItem<CAkEvent*>& l_rIdx = g_pIndex->m_idxEvents;
	AkAutoLock<CAkLock> IndexLock( g_pIndex->m_idxEvents.GetLock() );

	CAkIndexItem<CAkEvent*>::AkMapIDToPtr::Iterator iter = l_rIdx.m_mapIDToPtr.Begin();
	while( iter != l_rIdx.m_mapIDToPtr.End() )
	{
		Monitor_EventPrepared((*iter)->ID(), static_cast<CAkEvent*>(*iter)->GetPreparationCount());
		++iter;
	}
}

void AkMonitor::RecapGroupHelper(CAkStateMgr::PreparationGroups& in_Groups, AkGroupType in_Type )
{
	CAkStateMgr::PreparationGroups::Iterator iter = in_Groups.Begin();
	while( iter != in_Groups.End() )
	{
		CAkStateMgr::PreparationStateItem* pItem = iter.pItem;

		// We have a group, we will then have to iterate trough each game sync.
		CAkPreparedContent::ContentList& rContentList = pItem->GetPreparedcontent()->m_PreparableContentList;
		CAkPreparedContent::ContentList::Iterator subIter = rContentList.Begin();
		while( subIter != rContentList.End() )
		{
			Monitor_GameSync(pItem->GroupID(), *(subIter.pItem), true, in_Type);
			++subIter;
		}
		++iter;
	}
}

void AkMonitor::RecapGameSync()
{
	if( g_pStateMgr )
	{
		AkAutoLock<CAkLock> gate( g_pStateMgr->m_PrepareGameSyncLock );// to pad monitoring recaps.
		RecapGroupHelper( g_pStateMgr->m_PreparationGroupsStates, AkGroupType_State );
		RecapGroupHelper( g_pStateMgr->m_PreparationGroupsSwitches, AkGroupType_Switch );
	}
}

void AkMonitor::RecapDevices()
{
    AKASSERT( AK::IAkStreamMgr::Get( ) );
    AK::IAkStreamMgrProfile * pStmMgrProfile = AK::IAkStreamMgr::Get( )->GetStreamMgrProfile( );
    if ( !pStmMgrProfile )
        return; // Profiling interface not implemented in that stream manager.

    // Get all devices.
    AkUInt32 ulNumDevices = pStmMgrProfile->GetNumDevices( );
	for ( AkUInt32 ulDevice = 0; ulDevice < ulNumDevices; ++ulDevice )
	{
        AK::IAkDeviceProfile * pDevice = pStmMgrProfile->GetDeviceProfile( ulDevice );
        AKASSERT( pDevice );

		AkMonitorDataCreator creator( AkMonitorData::MonitorDataDevicesRecord, SIZEOF_MONITORDATA( deviceRecordData ) );
		if ( !creator.m_pData )
			return;

		pDevice->GetDesc( creator.m_pData->deviceRecordData );
        pDevice->ClearNew( );   // Whether it was new or not, it is not anymore.
	}
}

void AkMonitor::RecapStreamRecords()
{
    AKASSERT( AK::IAkStreamMgr::Get( ) );
    AK::IAkStreamMgrProfile * pStmMgrProfile = AK::IAkStreamMgr::Get( )->GetStreamMgrProfile( );
    if ( !pStmMgrProfile )
        return; // Profiling interface not implemented in that stream manager.

    // Get all stream records, for all devices.
    AkUInt32 ulNumDevices = pStmMgrProfile->GetNumDevices( );
    IAkDeviceProfile * pDevice;
    for ( AkUInt32 ulDevice=0; ulDevice<ulNumDevices; ulDevice++ )
    {
        pDevice = pStmMgrProfile->GetDeviceProfile( ulDevice );
        AKASSERT( pDevice != NULL );

		pDevice->OnProfileStart();

        AkUInt32 ulNumStreams = pDevice->GetNumStreams( );
        AK::IAkStreamProfile * pStream;
	    for ( AkUInt32 ulStream = 0; ulStream < ulNumStreams; ++ulStream )
	    {
			AkMonitorDataCreator creator( AkMonitorData::MonitorDataStreamsRecord, SIZEOF_MONITORDATA( streamRecordData ) );
			if ( !creator.m_pData )
			{
				pDevice->OnProfileEnd();
				return;
			}

			creator.m_pData->streamRecordData.ulNumNewRecords = 1;
            pStream = pDevice->GetStreamProfile( ulStream );
            AKASSERT( pStream );

            pStream->GetStreamRecord( *creator.m_pData->streamRecordData.streamRecords );
            pStream->ClearNew( );   // Whether it was new or not, it is not anymore.		
	    }

		pDevice->OnProfileEnd();
    }
}

void AkMonitor::RecapSinkTypes()
{
	AkUInt32 uMaxNameLen = 32;
	for ( AkUInt32 i = 0; i < AkSink_NumSinkTypes; i++ )
	{
		AkMonitorDataCreator creator( AkMonitorData::MonitorDataPlatformSinkType, SIZEOF_MONITORDATA( platformSinkTypeData ) + uMaxNameLen );
		if ( creator.m_pData )
		{
			creator.m_pData->platformSinkTypeData.uSinkType = (AkUInt16)i;
			creator.m_pData->platformSinkTypeData.uBufSize = (AkUInt16)uMaxNameLen;
			CAkLEngine::GetSinkTypeText( (AkSinkType)i, uMaxNameLen + 1, creator.m_pData->platformSinkTypeData.szNameBuf );
		}
	}
}

// Note: On the stream mgr API, StartMonitoring() means "start listening",
// which is false unless someone listens. It could be NOT the case even if 
// AkMonitor::StartMonitoring() had been called (see AkMonitor::IsMonitoring()
// implementation).
void AkMonitor::StartStreamProfiler( )
{
    AKASSERT( AK::IAkStreamMgr::Get( ) );
    if ( AK::IAkStreamMgr::Get( )->GetStreamMgrProfile( ) )
        AK::IAkStreamMgr::Get( )->GetStreamMgrProfile( )->StartMonitoring( );
}

void AkMonitor::StopStreamProfiler( )
{
    AKASSERT( AK::IAkStreamMgr::Get( ) );
    if ( AK::IAkStreamMgr::Get( )->GetStreamMgrProfile( ) )
    	AK::IAkStreamMgr::Get( )->GetStreamMgrProfile( )->StopMonitoring( );
}

AKRESULT AkMonitor::StartProfilerCapture( const AkOSChar* in_szFilename )
{
	if ( m_pLocalProfilerCaptureSink )
		return AK_AlreadyConnected;

#ifdef PROXYCENTRAL_CONNECTED
	// If CommunicationCentral is not initialized, there is no comm pool; use the monitor pool 
	// for profiler capture serializers in that case.
	if ( AK::ALWriteBytesMem::GetMemPool() == AK_INVALID_POOL_ID )
		AK::ALWriteBytesMem::SetMemPool( m_MonitorPoolId );
#endif

	AK::IAkStdStream * pStream = NULL;
	
	AkFileSystemFlags fsFlags;
	fsFlags.uCompanyID = AKCOMPANYID_AUDIOKINETIC;
	fsFlags.uCodecID = AKCODECID_PROFILERCAPTURE;
	fsFlags.bIsLanguageSpecific = false;
	fsFlags.pCustomParam = NULL;
	fsFlags.uCustomParamSize = 0;
	fsFlags.bIsFromRSX = false;

	AKRESULT eResult = AK::IAkStreamMgr::Get()->CreateStd( in_szFilename, &fsFlags, AK_OpenModeWriteOvrwr, pStream, true );
	if( eResult != AK_Success )
		return eResult;

	pStream->SetStreamName( in_szFilename );

	m_pLocalProfilerCaptureSink = AkNew( m_MonitorPoolId, AkLocalProfilerCaptureSink( pStream ) );
	if ( !m_pLocalProfilerCaptureSink )
	{
		pStream->Destroy();
		return AK_InsufficientMemory;
	}

	return AK_Success;
}

AKRESULT AkMonitor::StopProfilerCapture()
{
	if ( !m_pLocalProfilerCaptureSink )
		return AK_Fail;

	AkDelete( m_MonitorPoolId, m_pLocalProfilerCaptureSink );
	m_pLocalProfilerCaptureSink = NULL;

	return AK_Success;
}

AKRESULT AkMonitor::StartMonitoring()
{
	if( AkIsValidThread(&m_hThread) )
		return AK_Success;

	AkUInt32 uQueuePoolSize = g_settings.uMonitorQueuePoolSize ? g_settings.uMonitorQueuePoolSize : MONITOR_QUEUE_POOL_SIZE;
	m_MonitorQueuePoolId = AK::MemoryMgr::CreatePool(NULL, uQueuePoolSize, uQueuePoolSize, AkMalloc | AkFixedSizeBlocksMode );
    if ( m_MonitorQueuePoolId == AK_INVALID_POOL_ID )
		return AK_Fail;

	m_ringItems.Init( m_MonitorQueuePoolId, uQueuePoolSize );

	m_MonitorPoolId = AK::MemoryMgr::CreatePool(NULL, g_settings.uMonitorPoolSize ? g_settings.uMonitorPoolSize : MONITOR_POOL_SIZE, MONITOR_POOL_BLOCK_SIZE, AkMalloc );
	if( m_MonitorPoolId == AK_INVALID_POOL_ID )
		return AK_Fail;

	if ( m_sink2Filter.Init( AK_MAX_MONITORING_SINKS, AK_MAX_MONITORING_SINKS ) != AK_Success )
		return AK_Fail;

	m_mapGameObjectWatch.Init( m_MonitorPoolId );
	m_idxGameObjectString.Init( m_MonitorPoolId );
	m_meterWatchMap.Init( m_MonitorPoolId );

	AK_SETPOOLNAME(m_MonitorQueuePoolId,AKTEXT("Monitor Queue"));
	AK_SETPOOLNAME(m_MonitorPoolId,AKTEXT("Monitor"));

	m_bStopThread = false;
	if ( AkCreateEvent( m_hMonitorEvent ) != AK_Success
		|| AkCreateEvent( m_hMonitorDoneEvent ) != AK_Success )
		return AK_Fail;

	AkCreateThread(	MonitorThreadFunc,    // Start address
					this,                 // Parameter
					g_PDSettings.threadMonitor,
					&m_hThread,
					"AK::Monitor" );	  // Handle
	if ( !AkIsValidThread(&m_hThread) )
	{
		AKASSERT( !"Could not create monitor thread" );
		return AK_Fail;
	}

	return AK_Success;
}

void AkMonitor::StopMonitoring()
{
	if(AkIsValidThread(&m_hThread))
	{
		m_bStopThread = true;
		AkSignalEvent( m_hMonitorEvent );
		AkWaitForSingleThread( &m_hThread );

		AkCloseThread( &m_hThread );
	}

	AkDestroyEvent( m_hMonitorEvent );
	AkDestroyEvent( m_hMonitorDoneEvent );

	StopProfilerCapture();
    StopStreamProfiler( );

	if ( m_MonitorPoolId != AK_INVALID_POOL_ID )
	{
		m_sink2Filter.Term();
		m_idxGameObjectString.Term();
		m_mapGameObjectWatch.Term();
		m_meterWatchMap.Term();
		m_watches.Term();
		m_gameSyncWatches.Term();
		AK::MemoryMgr::DestroyPool( m_MonitorPoolId );
		m_MonitorPoolId = AK_INVALID_POOL_ID;
	}

	if ( m_MonitorQueuePoolId != AK_INVALID_POOL_ID )
	{
		m_ringItems.Term( m_MonitorQueuePoolId );
		AK::MemoryMgr::DestroyPool( m_MonitorQueuePoolId );
		m_MonitorQueuePoolId = AK_INVALID_POOL_ID;
	}
}

void AkMonitor::PostWatchedGameObjPositions()
{
	int i = 0;

	// Use the watched game objects only
	AkUInt32 uNumGameObjs = m_mapGameObjectWatch.Length();
	AkUInt32 uNumListeners = 0;
	for( i = 0 ; i < AK_NUM_LISTENERS; ++i )
		uNumListeners += m_arrayListenerWatch[i] ? 1 : 0;
	
    AkInt32 sizeofItem = SIZEOF_MONITORDATA_TO( gameObjPositionData.positions )
						+ (uNumGameObjs + uNumListeners) * sizeof( AkMonitorData::GameObjPositionMonitorData::Position );

    AkProfileDataCreator creator( sizeofItem );
	if ( !creator.m_pData )
		return;

	creator.m_pData->eDataType = AkMonitorData::MonitorDataGameObjPosition;

	creator.m_pData->gameObjPositionData.ulNumGameObjPositions = uNumGameObjs;
	creator.m_pData->gameObjPositionData.ulNumListenerPositions = uNumListeners;

	int iGameObj = 0;
	for ( AkMapGameObjectWatch::Iterator iter = m_mapGameObjectWatch.Begin(); iter != m_mapGameObjectWatch.End(); ++iter )
	{
		AkMonitorData::GameObjPosition & gameObjPos = creator.m_pData->gameObjPositionData.positions[iGameObj].gameObjPosition;

		gameObjPos.gameObjID = GameObjectToWwiseGameObject( (*iter).key );

		AkSoundPositionRef position;
		if( g_pRegistryMgr->GetPosition( (*iter).key, position ) == AK_Success )
		{
			gameObjPos.position = position.GetFirstPositionFixme();
		}
		else
		{
			AKASSERT( !"Why can't we obtain a position for this object?" );
		}
		++iGameObj;
	}

	int iListener = 0;
	for( i = 0; i < AK_NUM_LISTENERS; ++i )
	{
		if( m_arrayListenerWatch[i] )
		{
			creator.m_pData->gameObjPositionData.positions[iGameObj+iListener].listenerPosition.uIndex = i;

			AkListenerPosition& position = creator.m_pData->gameObjPositionData.positions[iGameObj+iListener].listenerPosition.position;
			CAkListener::GetPosition( i, position );

			++iListener;
		}
	}
}

void AkMonitor::PostWatchesRTPCValues()
{
	// Use the watched game objects only
	AkUInt32 uNumGameObjs = m_mapGameObjectWatch.Length();
	AkUInt32 uNumGameSyncs = m_gameSyncWatches.Length();

    AkInt32 sizeofItem = SIZEOF_MONITORDATA_TO( rtpcValuesData.rtpcValues )
						+ (uNumGameObjs*uNumGameSyncs) * sizeof( AkMonitorData::RTPCValuesPacket );

    AkProfileDataCreator creator( sizeofItem );
	if ( !creator.m_pData )
		return;

	creator.m_pData->eDataType = AkMonitorData::MonitorDataRTPCValues;

	creator.m_pData->rtpcValuesData.ulNumRTPCValues = uNumGameObjs*uNumGameSyncs;

	int index = 0;
	for ( AkMapGameObjectWatch::Iterator iter = m_mapGameObjectWatch.Begin(); iter != m_mapGameObjectWatch.End(); ++iter )
	{
		for ( unsigned int j = 0; j < m_gameSyncWatches.Length(); ++j )
		{
			creator.m_pData->rtpcValuesData.rtpcValues[index].gameObjectID = GameObjectToWwiseGameObject( (*iter).key );
			creator.m_pData->rtpcValuesData.rtpcValues[index].rtpcID = m_gameSyncWatches[j];

			CAkRegisteredObj* pGameObj = g_pRegistryMgr->GetObjAndAddref( (*iter).key );
			
			creator.m_pData->rtpcValuesData.rtpcValues[index].value = 0;

			if( pGameObj )
			{
				bool bGameObjSpecificVal = false;
				AkReal32 fValue = 0.0f;
				bool bHasValue = g_pRTPCMgr->GetRTPCValue( m_gameSyncWatches[j], pGameObj, fValue, bGameObjSpecificVal );
				if(! bHasValue )
				{
					fValue = g_pRTPCMgr->GetDefaultValue( m_gameSyncWatches[j], &bHasValue );
				}

				creator.m_pData->rtpcValuesData.rtpcValues[index].bHasValue = bHasValue;
				creator.m_pData->rtpcValuesData.rtpcValues[index].value = fValue;

				pGameObj->Release();
			}
			else
			{
				creator.m_pData->rtpcValuesData.rtpcValues[index].bHasValue = false;
				AKASSERT( !"Why can't we obtain a game object pointer for this game object?" );
			}
			++index;
		}
	}
}

bool AkMonitor::DispatchNotification()
{
	//In this thread, we don't want to notify about memory problems in the communication pool.  
	//It would cause another allocation which would also fail.  We'd wait indefinitely.
	if (g_pCommCentral)
		AK::MemoryMgr::SetMonitoring(g_pCommCentral->GetPool(), false);

	bool bReturnVal = false;

	// First keep a local copy of the sinks -- this is to prevent problems with sinks registering from other threads
	// while we notify.

	m_registrationLock.Lock();

	// Allocate copy on the stack ( CAkKeyList copy was identified as a bottleneck here )
	int cSinks = m_sink2Filter.Length();
	MapStruct<AK::IALMonitorSink*, AkMonitorData::MaskType> aSinksCopy[ AK_MAX_MONITORING_SINKS ];
	{
		int i = 0;
		for( MonitorSink2Filter::Iterator it = m_sink2Filter.Begin(); it != m_sink2Filter.End(); ++it )
			aSinksCopy[ i++ ] = (*it);
	}

	// Next, send a maximum of MAX_NOTIFICATIONS_PER_CALL notifications in the queue
	int iNotifCount = MAX_NOTIFICATIONS_PER_CALL;
	while ( !m_ringItems.IsEmpty() )
	{
		if(--iNotifCount == 0)
		{
			bReturnVal = true; //remaining items to notify
			break; //exit while loop
		}

		AkMonitorData::MonitorDataItem * pItem = (AkMonitorData::MonitorDataItem * ) m_ringItems.BeginRead();
		AKASSERT( pItem->eDataType < AkMonitorData::MonitorDataEndOfItems );

		AkMonitorData::MaskType uMask = AKMONITORDATATYPE_TOMASK( pItem->eDataType ) ;
		for ( int i = 0; i < cSinks; ++i )
		{
			if ( aSinksCopy[ i ].item & uMask ) //Apply filter for this sink
				aSinksCopy[ i ].key->MonitorNotification( *pItem, true/*Accumulate mode ON*/ );
		}

		m_ringItems.EndRead( pItem, AkMonitorData::RealSizeof( *pItem ) );
	}

	AkSignalEvent( m_hMonitorDoneEvent );

	for ( int i = 0; i < cSinks; ++i )
	{
		aSinksCopy[ i ].key->FlushAccumulated();
	}

	m_registrationLock.Unlock();

	if (g_pCommCentral)
		AK::MemoryMgr::SetMonitoring(g_pCommCentral->GetPool(), true);

	return bReturnVal;
}

AK_DECLARE_THREAD_ROUTINE( AkMonitor::MonitorThreadFunc )
{
	AkMonitor& rThis = *AK_GET_THREAD_ROUTINE_PARAMETER_PTR(AkMonitor);

	while( true )
	{
		AkWaitForEvent( rThis.m_hMonitorEvent );

		if ( rThis.m_bStopThread )
		{
			// Stop event.  Bail out.
			break;
		}

		// Something in the queue!
		if ( !rThis.m_ringItems.IsEmpty() )
			while( rThis.DispatchNotification() ) {} //loop until there are no more notifications
	}

	AkExitThread(AK_RETURN_THREAD_OK);
}

void AkMonitor::Monitor_PostCode( 
	AK::Monitor::ErrorCode in_eErrorCode, 
	AK::Monitor::ErrorLevel in_eErrorLevel, 
	AkPlayingID in_playingID,
	AkGameObjectID in_gameObjID,
	AkUniqueID in_soundID,
	bool in_bIsBus)
{
	if ( in_eErrorCode >= 0 && in_eErrorCode < AK::Monitor::Num_ErrorCodes && ( m_uLocalOutputErrorLevel & in_eErrorLevel ) )
	{
		Monitor_PostToLocalOutput( in_eErrorCode, in_eErrorLevel, AK::Monitor::s_aszErrorCodes[ in_eErrorCode ], in_playingID, in_gameObjID );
	}

	Monitor_SendErrorData1( in_eErrorCode, in_eErrorLevel, 0, in_playingID, in_gameObjID, in_soundID, in_bIsBus );
}

void AkMonitor::Monitor_PostCodeWithParam( 
	AK::Monitor::ErrorCode in_eErrorCode, 
	AK::Monitor::ErrorLevel in_eErrorLevel, 
	AkUInt32 in_param1,
	AkPlayingID in_playingID,
	AkGameObjectID in_gameObjID,
	AkUniqueID in_soundID,
	bool in_bIsBus)
{
	if ( in_eErrorCode >= 0 && in_eErrorCode < AK::Monitor::Num_ErrorCodes && ( m_uLocalOutputErrorLevel & in_eErrorLevel ) )
	{
		const AkOSChar* pszError = AK::Monitor::s_aszErrorCodes[ in_eErrorCode ];

		const size_t k_uMaxStrLen = 128;
		AkOSChar szMsg[ k_uMaxStrLen ];
		AKPLATFORM::SafeStrCpy( szMsg, pszError, k_uMaxStrLen );
		if ( in_param1 != AK_INVALID_UNIQUE_ID )
		{
			const size_t k_uMaxIDStrLen = 16;
			AkOSChar szID[ k_uMaxIDStrLen ];
			AK_OSPRINTF( szID, k_uMaxIDStrLen, AKTEXT(": %u"), in_param1 );
			AKPLATFORM::SafeStrCat( szMsg, szID, k_uMaxStrLen );
		}

		Monitor_PostToLocalOutput( in_eErrorCode, in_eErrorLevel, szMsg, in_playingID, in_gameObjID );
	}

	Monitor_SendErrorData1( in_eErrorCode, in_eErrorLevel, in_param1, in_playingID, in_gameObjID, in_soundID, in_bIsBus );
}

void AkMonitor::Monitor_SendErrorData1( 
	AK::Monitor::ErrorCode in_eErrorCode, 
	AK::Monitor::ErrorLevel in_eErrorLevel, 
	AkUInt32 in_param1,
	AkPlayingID in_playingID,
	AkGameObjectID in_gameObjID,
	AkUniqueID in_soundID,
	bool in_bIsBus )
{
	AkMonitorDataCreator creator( 
		in_eErrorLevel == AK::Monitor::ErrorLevel_Message ? AkMonitorData::MonitorDataMessageCode : AkMonitorData::MonitorDataErrorCode, 
		SIZEOF_MONITORDATA( errorData1 ) );
	if ( !creator.m_pData )
		return;

	creator.m_pData->errorData1.playingID = in_playingID;
	creator.m_pData->errorData1.gameObjID = GameObjectToWwiseGameObject( in_gameObjID );
	creator.m_pData->errorData1.eErrorCode = in_eErrorCode;
	creator.m_pData->errorData1.uParam1 = in_param1;
	creator.m_pData->errorData1.soundID.id = in_soundID;
	creator.m_pData->errorData1.soundID.bIsBus = in_bIsBus;
}

void AkMonitor::Monitor_PostToLocalOutput( 
	AK::Monitor::ErrorCode in_eErrorCode, 
	AK::Monitor::ErrorLevel in_eErrorLevel, 
	const AkOSChar * in_pszError,
	AkPlayingID in_playingID,
	AkGameObjectID in_gameObjID )
{
	if ( m_funcLocalOutput )
	{
		m_funcLocalOutput( in_eErrorCode, in_pszError, in_eErrorLevel, in_playingID, in_gameObjID );
	}
	else
	{
		AKPLATFORM::OutputDebugMsg( in_eErrorLevel == AK::Monitor::ErrorLevel_Message ? AKTEXT("AK Message: ") : AKTEXT("AK Error: ") );
		AKPLATFORM::OutputDebugMsg( in_pszError );
		AKPLATFORM::OutputDebugMsg( AKTEXT("\n") );
	}
}

#ifdef AK_SUPPORT_WCHAR
void AkMonitor::Monitor_PostString( const wchar_t* in_pszError, AK::Monitor::ErrorLevel in_eErrorLevel )
{
	if ( in_pszError )
	{
		if ( m_uLocalOutputErrorLevel & in_eErrorLevel )
		{
			if ( m_funcLocalOutput )
			{
#ifdef AK_OS_WCHAR
				m_funcLocalOutput( Monitor::ErrorCode_NoError, in_pszError, in_eErrorLevel, AK_INVALID_PLAYING_ID, AK_INVALID_GAME_OBJECT );
#else
				char szError[ AK_MAX_STRING_SIZE ]; 
				AkWideCharToChar( in_pszError, AK_MAX_STRING_SIZE, szError );
				m_funcLocalOutput( Monitor::ErrorCode_NoError, szError, in_eErrorLevel, AK_INVALID_PLAYING_ID, AK_INVALID_GAME_OBJECT );
#endif
			}
			else
			{
				AKPLATFORM::OutputDebugMsg( in_eErrorLevel == AK::Monitor::ErrorLevel_Message ? L"AK Message: " : L"AK Error: " );
				AKPLATFORM::OutputDebugMsg( in_pszError );
				AKPLATFORM::OutputDebugMsg( L"\n" );
			}
		}

		AkUInt16 wStringSize = (AkUInt16) wcslen(in_pszError) + 1;
		AkMonitorDataCreator creator( 
			in_eErrorLevel == AK::Monitor::ErrorLevel_Message ? AkMonitorData::MonitorDataMessageString : AkMonitorData::MonitorDataErrorString, 
			offsetof( AkMonitorData::MonitorDataItem, debugData.szMessage ) + wStringSize * sizeof( AkUtf16 ) );
		if ( !creator.m_pData )
			return;

		creator.m_pData->debugData.wStringSize = wStringSize;

		AK_WCHAR_TO_UTF16( creator.m_pData->debugData.szMessage, in_pszError, wStringSize);
	}
}
#endif //AK_SUPPORT_WCHAR


#ifdef AK_SUPPORT_WCHAR
void AkMonitor::Monitor_PostString( const char* in_pszError, AK::Monitor::ErrorLevel in_eErrorLevel )
{
	if ( in_pszError )
	{
		wchar_t wszError[ AK_MAX_STRING_SIZE ]; 
		AkUtf8ToWideChar( in_pszError, AK_MAX_STRING_SIZE, wszError );
		wszError[ AK_MAX_STRING_SIZE - 1 ] = 0;
		Monitor_PostString(wszError, in_eErrorLevel);
	}
}
#else
void AkMonitor::Monitor_PostString( const char* in_pszError, AK::Monitor::ErrorLevel in_eErrorLevel )
{
	if ( in_pszError )
	{
		if ( m_uLocalOutputErrorLevel & in_eErrorLevel )
		{
			if ( m_funcLocalOutput )
			{
				m_funcLocalOutput( Monitor::ErrorCode_NoError, in_pszError, in_eErrorLevel, AK_INVALID_PLAYING_ID, AK_INVALID_GAME_OBJECT );
			}
			else
			{
				AKPLATFORM::OutputDebugMsg( in_eErrorLevel == AK::Monitor::ErrorLevel_Message ? AKTEXT("AK Message: ") : AKTEXT("AK Error: ") );
				AKPLATFORM::OutputDebugMsg( in_pszError );
				AKPLATFORM::OutputDebugMsg( AKTEXT("\n") );
			}
		}

		AkUInt16 wStringSize = (AkUInt16) strlen(in_pszError) + 1;
		AkMonitorDataCreator creator( 
			in_eErrorLevel == AK::Monitor::ErrorLevel_Message ? AkMonitorData::MonitorDataMessageString : AkMonitorData::MonitorDataErrorString, 
			SIZEOF_MONITORDATA_TO( debugData.szMessage ) + wStringSize * sizeof( AkUtf16 ) );
		if ( !creator.m_pData )
			return;

		creator.m_pData->debugData.wStringSize = wStringSize;

		AK_CHAR_TO_UTF16( creator.m_pData->debugData.szMessage, in_pszError, wStringSize);
	}
}
#endif //AK_SUPPORT_WCHAR

void AkMonitor::Monitor_ObjectNotif( 
									AkPlayingID in_PlayingID, 
									AkGameObjectID in_GameObject, 
									const AkCustomParamType & in_CustomParam, 
									AkMonitorData::NotificationReason in_eNotifReason, 
									AkCntrHistArray in_cntrHistArray, 
									AkUniqueID in_targetObjectID,
									bool in_bTargetIsBus,
									AkTimeMs in_timeValue, 
									AkUniqueID in_playlistItemID )
{
	AkMonitorDataCreator creator( AkMonitorData::MonitorDataObject, SIZEOF_MONITORDATA( objectData ) );
	if ( !creator.m_pData )
		return;

	creator.m_pData->objectData.eNotificationReason = in_eNotifReason ;
	creator.m_pData->objectData.gameObjPtr = GameObjectToWwiseGameObject( in_GameObject );
	creator.m_pData->objectData.customParam = in_CustomParam;
	creator.m_pData->objectData.playingID = in_PlayingID;
	creator.m_pData->objectData.cntrHistArray = in_cntrHistArray;
	if( creator.m_pData->objectData.cntrHistArray.uiArraySize > AK_CONT_HISTORY_SIZE )
		creator.m_pData->objectData.cntrHistArray.uiArraySize = AK_CONT_HISTORY_SIZE;
	creator.m_pData->objectData.targetObjectID.id = in_targetObjectID;
	creator.m_pData->objectData.targetObjectID.bIsBus = in_bTargetIsBus;
	creator.m_pData->objectData.timeValue = in_timeValue;
	creator.m_pData->objectData.playlistItemID = in_playlistItemID;
}

void AkMonitor::Monitor_BankNotif( AkUniqueID in_BankID, AkUniqueID in_LanguageID, AkMonitorData::NotificationReason in_eNotifReason, AkUInt32 in_uPrepareFlags )
{
	const char* pBankName = g_pBankManager->GetBankFileName( in_BankID );
	AkUInt16 wStringSize = 0;
	if ( pBankName ) 
		wStringSize = (AkUInt16) ( strlen( pBankName ) + 1 );

	AkMonitorDataCreator creator( AkMonitorData::MonitorDataBank, SIZEOF_MONITORDATA_TO( bankData.szBankName ) + wStringSize );
	if ( !creator.m_pData )
		return;

	creator.m_pData->bankData.bankID = in_BankID;
	creator.m_pData->bankData.languageID = in_LanguageID;
	creator.m_pData->bankData.uFlags = in_uPrepareFlags;
	creator.m_pData->bankData.eNotificationReason = in_eNotifReason;
	creator.m_pData->bankData.wStringSize = wStringSize;

	if( wStringSize )
	{
		strcpy( creator.m_pData->bankData.szBankName, pBankName );
	}
}

void AkMonitor::Monitor_PrepareNotif( AkMonitorData::NotificationReason in_eNotifReason, AkUniqueID in_GameSyncorEventID, AkUInt32 in_groupID, AkGroupType in_GroupType, AkUInt32 in_NumEvents )
{
	AkMonitorDataCreator creator( AkMonitorData::MonitorDataPrepare, SIZEOF_MONITORDATA( prepareData ) );
	if ( !creator.m_pData )
		return;

	creator.m_pData->prepareData.eNotificationReason = in_eNotifReason;
	creator.m_pData->prepareData.gamesyncIDorEventID = in_GameSyncorEventID;
	creator.m_pData->prepareData.groupID			 = in_groupID;
	creator.m_pData->prepareData.groupType			 = in_GroupType;
	creator.m_pData->prepareData.uNumEvents			 = in_NumEvents;
}

void AkMonitor::Monitor_StateChanged( AkStateGroupID in_StateGroup, AkStateID in_PreviousState, AkStateID in_NewState )
{
	AkMonitorDataCreator creator( AkMonitorData::MonitorDataState, SIZEOF_MONITORDATA( stateData ) );
	if ( !creator.m_pData )
		return;

	creator.m_pData->stateData.stateGroupID = in_StateGroup;
	creator.m_pData->stateData.stateFrom = in_PreviousState;
	creator.m_pData->stateData.stateTo = in_NewState;
}

void AkMonitor::Monitor_SwitchChanged( AkSwitchGroupID in_SwitchGroup, AkSwitchStateID in_Switch, AkGameObjectID in_GameObject )
{
	AkMonitorDataCreator creator( AkMonitorData::MonitorDataSwitch, SIZEOF_MONITORDATA( switchData ) );
	if ( !creator.m_pData )
		return;

	creator.m_pData->switchData.switchGroupID = in_SwitchGroup;
	creator.m_pData->switchData.switchState = in_Switch;
	creator.m_pData->switchData.gameObj = GameObjectToWwiseGameObject( in_GameObject );
}

void AkMonitor::Monitor_ObjectRegistration( bool in_isRegistration, AkGameObjectID in_GameObject, void * in_pMonitorData, bool in_bRecap )
{
	AkIDStringHash::AkStringHash::Item * pItem = (AkIDStringHash::AkStringHash::Item *) in_pMonitorData;

	// Send notification
	{
		AkUInt16 wStringSize = 0;
		if ( pItem )
			wStringSize = (AkUInt16) strlen( &( pItem->Assoc.item ) ) + 1;

		AkMonitorDataCreator creator( AkMonitorData::MonitorDataObjRegistration, SIZEOF_MONITORDATA_TO( objRegistrationData.szName ) + wStringSize );
		if ( creator.m_pData )
		{
			creator.m_pData->objRegistrationData.isRegistered = in_isRegistration;
			creator.m_pData->objRegistrationData.gameObjPtr = GameObjectToWwiseGameObject( in_GameObject );
			creator.m_pData->objRegistrationData.wStringSize = wStringSize;

			if( pItem )
				AkMemCpy( creator.m_pData->objRegistrationData.szName, (void *) &( pItem->Assoc.item ), wStringSize );
		}
	}
	
	if(in_bRecap)
		return;

	// Remember name in our own map
	if( in_isRegistration )
	{
		if( pItem )
		{
			if( m_idxGameObjectString.Set( pItem ) != AK_Success )
			{
				MONITOR_FREESTRING( pItem );
				pItem = NULL;
				// Game object with no name, maybe watch it if we have a "*" watch
				AddWatchForGameObject( in_GameObject, "" );
			}
			else
			{
				AddWatchForGameObject( in_GameObject, &( pItem->Assoc.item ) );
			}
			// m_idxGameObjectString. Set fails, there is nothing to do, this string will stay unknown.
		}
		else
		{
			// Game object with no name, maybe watch it if we have a "*" watch
			AddWatchForGameObject( in_GameObject, "" );
		}
	}
	else
	{
		m_idxGameObjectString.Unset( in_GameObject );
		RemoveWatchForGameObject( in_GameObject );
	}
}

void AkMonitor::Monitor_FreeString( void * in_pMonitorData )
{
	if( in_pMonitorData )
	{
		AkIDStringHash::AkStringHash::Item * pItem = (AkIDStringHash::AkStringHash::Item *) in_pMonitorData;
		m_idxGameObjectString.FreePreallocatedString( pItem );
	}
}

void AkMonitor::Monitor_ParamChanged( AkMonitorData::NotificationReason in_eNotifReason, AkUniqueID in_ElementID, bool in_bIsBusElement, AkGameObjectID in_GameObject )
{
	AkMonitorDataCreator creator( AkMonitorData::MonitorDataParamChanged, SIZEOF_MONITORDATA( paramChangedData ) );
	if ( !creator.m_pData )
		return;

	creator.m_pData->paramChangedData.eNotificationReason = in_eNotifReason;
	creator.m_pData->paramChangedData.elementID.id = in_ElementID;
	creator.m_pData->paramChangedData.elementID.bIsBus = in_bIsBusElement;
	creator.m_pData->paramChangedData.gameObjPtr = GameObjectToWwiseGameObject( in_GameObject );
}

void AkMonitor::Monitor_EventTriggered( AkPlayingID in_PlayingID, AkUniqueID in_EventID, AkGameObjectID in_GameObject, const AkCustomParamType & in_CustomParam)
{
	AkMonitorDataCreator creator( AkMonitorData::MonitorDataEventTriggered, SIZEOF_MONITORDATA( eventTriggeredData ) );
	if ( !creator.m_pData )
		return;

	creator.m_pData->eventTriggeredData.playingID = in_PlayingID;
	creator.m_pData->eventTriggeredData.eventID = in_EventID;
	creator.m_pData->eventTriggeredData.gameObjPtr = GameObjectToWwiseGameObject( in_GameObject );
	creator.m_pData->eventTriggeredData.customParam = in_CustomParam;
}

void AkMonitor::Monitor_ActionDelayed( AkPlayingID in_PlayingID, AkUniqueID in_ActionID, AkGameObjectID in_GameObject, AkTimeMs in_DelayTime, const AkCustomParamType & in_CustomParam )
{
	if( in_ActionID != AK_INVALID_UNIQUE_ID )
	{
		AkMonitorDataCreator creator( AkMonitorData::MonitorDataActionDelayed, SIZEOF_MONITORDATA( actionDelayedData ) );
		if ( !creator.m_pData )
			return;

		creator.m_pData->actionDelayedData.playingID = in_PlayingID;
		creator.m_pData->actionDelayedData.actionID = in_ActionID;
		creator.m_pData->actionDelayedData.gameObjPtr = GameObjectToWwiseGameObject( in_GameObject );
		creator.m_pData->actionDelayedData.delayTime = in_DelayTime;
		creator.m_pData->actionDelayedData.customParam = in_CustomParam;
	}
}

void AkMonitor::Monitor_ActionTriggered( AkPlayingID in_PlayingID, AkUniqueID in_ActionID, AkGameObjectID in_GameObject, const AkCustomParamType & in_CustomParam )
{
	if( in_ActionID != AK_INVALID_UNIQUE_ID )
	{
		AkMonitorDataCreator creator( AkMonitorData::MonitorDataActionTriggered, SIZEOF_MONITORDATA( actionTriggeredData ) );
		if ( !creator.m_pData )
			return;

		creator.m_pData->actionTriggeredData.playingID = in_PlayingID;
		creator.m_pData->actionTriggeredData.actionID = in_ActionID;
		creator.m_pData->actionTriggeredData.gameObjPtr = GameObjectToWwiseGameObject( in_GameObject );
		creator.m_pData->actionTriggeredData.customParam = in_CustomParam;
	}
}

void AkMonitor::Monitor_BusNotification( AkUniqueID in_BusID, AkMonitorData::BusNotification in_NotifReason, AkUInt32 in_bitsFXBypass, AkUInt32 in_bitsMask )
{
	AkMonitorDataCreator creator( AkMonitorData::MonitorDataBusNotif, SIZEOF_MONITORDATA( busNotifData ) );
	if ( !creator.m_pData )
		return;

	creator.m_pData->busNotifData.busID = in_BusID;
	creator.m_pData->busNotifData.notifReason = in_NotifReason;
	creator.m_pData->busNotifData.bitsFXBypass = (AkUInt8) in_bitsFXBypass;
	creator.m_pData->busNotifData.bitsMask = (AkUInt8) in_bitsMask;
}

void AkMonitor::Monitor_PathEvent( AkPlayingID in_playingID, AkUniqueID in_who, AkMonitorData::AkPathEvent in_eEvent, AkUInt32 in_index)
{
	AkMonitorDataCreator creator( AkMonitorData::MonitorDataPath, SIZEOF_MONITORDATA( pathData ) );
	if ( !creator.m_pData )
		return;

	creator.m_pData->pathData.playingID = (in_playingID);
	creator.m_pData->pathData.ulUniqueID = (in_who);
	creator.m_pData->pathData.eEvent =(in_eEvent);
	creator.m_pData->pathData.ulIndex = (in_index);
}

#ifdef AK_SUPPORT_WCHAR
void AkMonitor::Monitor_errorMsg2( const wchar_t* in_psz1, const wchar_t* in_psz2 )
{
	if( in_psz1 && in_psz2 )
	{
		wchar_t wszBuffer[ AK_MAX_STRING_SIZE ];
		AKPLATFORM::SafeStrCpy( wszBuffer, in_psz1, AK_MAX_STRING_SIZE );
		AKPLATFORM::SafeStrCat( wszBuffer, in_psz2, AK_MAX_STRING_SIZE );

		Monitor_PostString( wszBuffer, AK::Monitor::ErrorLevel_Error );
	}
}
#endif //AK_SUPPORT_WCHAR

void AkMonitor::Monitor_errorMsg2( const char* in_psz1, const char* in_psz2 )
{
	if( in_psz1 && in_psz2 )
	{
		char szBuffer[ AK_MAX_STRING_SIZE ];
		AKPLATFORM::SafeStrCpy( szBuffer, in_psz1, AK_MAX_STRING_SIZE );
		AKPLATFORM::SafeStrCat( szBuffer, in_psz2, AK_MAX_STRING_SIZE );

		Monitor_PostString( szBuffer, AK::Monitor::ErrorLevel_Error );
	}
}

void AkMonitor::Monitor_LoadedBank( CAkUsageSlot* in_pUsageSlot, bool in_bIsDestroyed )
{
	if( in_pUsageSlot )// In some situations, NULL will be received, and it means that nothing must be notified.( keeping the if inside the MACRO )
	{
		AkMonitorDataCreator creator( AkMonitorData::MonitorDataSoundBank, SIZEOF_MONITORDATA( loadedSoundBankData ) );
		if ( !creator.m_pData )
			return;

		creator.m_pData->loadedSoundBankData.bankID = in_pUsageSlot->m_BankID;
		creator.m_pData->loadedSoundBankData.memPoolID = in_pUsageSlot->m_memPoolId;
		creator.m_pData->loadedSoundBankData.uBankSize = in_pUsageSlot->m_uLoadedDataSize;
		creator.m_pData->loadedSoundBankData.uNumIndexableItems = in_pUsageSlot->m_listLoadedItem.Length();
		creator.m_pData->loadedSoundBankData.uNumMediaItems = in_pUsageSlot->m_uNumLoadedItems;
		creator.m_pData->loadedSoundBankData.bIsExplicitelyLoaded = in_pUsageSlot->WasLoadedAsABank();

		creator.m_pData->loadedSoundBankData.bIsDestroyed = in_bIsDestroyed;
	}
}

void AkMonitor::Monitor_MediaPrepared( AkMediaEntry& in_rMediaEntry )
{
	// One entry per bank + one if the media was explicitely prepared too.
	AkUInt32 numBankOptions = in_rMediaEntry.GetNumBankOptions();
	AkUInt32 arraysize = numBankOptions;

	if( in_rMediaEntry.IsDataPrepared() )
		++arraysize;

	AkMonitorDataCreator creator( AkMonitorData::MonitorDataMedia, SIZEOF_MONITORDATA_TO( mediaPreparedData.bankMedia ) + ( arraysize * sizeof( AkMonitorData::MediaPreparedMonitorData::BankMedia ) ) );
	if ( !creator.m_pData )
		return;

	creator.m_pData->mediaPreparedData.uMediaID = in_rMediaEntry.GetSourceID();
	creator.m_pData->mediaPreparedData.uArraySize = arraysize;

	AkUInt32 i = 0;
	for( ; i < numBankOptions; ++i )
	{
		creator.m_pData->mediaPreparedData.bankMedia[i].bankID = in_rMediaEntry.m_BankSlots[i].key->m_BankID;
		creator.m_pData->mediaPreparedData.bankMedia[i].uMediaSize = in_rMediaEntry.m_BankSlots[i].item.uInMemoryDataSize;
	}
	if( in_rMediaEntry.IsDataPrepared() )
	{
		creator.m_pData->mediaPreparedData.bankMedia[i].bankID = AK_INVALID_UNIQUE_ID;
		creator.m_pData->mediaPreparedData.bankMedia[i].uMediaSize = in_rMediaEntry.m_preparedMediaInfo.uInMemoryDataSize;
	}
}

void AkMonitor::Monitor_EventPrepared( AkUniqueID in_EventID, AkUInt32 in_RefCount )
{
	AkMonitorDataCreator creator( AkMonitorData::MonitorDataEvent, SIZEOF_MONITORDATA( eventPreparedData ) );
	if ( !creator.m_pData )
		return;

	creator.m_pData->eventPreparedData.eventID = in_EventID;
	creator.m_pData->eventPreparedData.uRefCount = in_RefCount;
}

void AkMonitor::Monitor_GameSync( AkUniqueID in_GroupID, AkUniqueID in_GameSyncID, bool in_bIsEnabled, AkGroupType in_GroupType )
{
	AkMonitorDataCreator creator( AkMonitorData::MonitorDataGameSync, SIZEOF_MONITORDATA( gameSyncData ) );
	if ( !creator.m_pData )
		return;

	creator.m_pData->gameSyncData.groupID = in_GroupID;
	creator.m_pData->gameSyncData.syncID = in_GameSyncID;
	creator.m_pData->gameSyncData.bIsEnabled = in_bIsEnabled;
	creator.m_pData->gameSyncData.eSyncType = in_GroupType;
}

void AkMonitor::Monitor_SetParamNotif_Float( AkMonitorData::NotificationReason in_eNotifReason, AkUniqueID in_ElementID, bool in_bIsBusElement, AkGameObjectID in_GameObject, AkReal32 in_TargetValue, AkValueMeaning in_ValueMeaning, AkTimeMs in_TransitionTime )
{
	AkMonitorDataCreator creator( AkMonitorData::MonitorDataSetParam, SIZEOF_MONITORDATA( setParamData ) );
	if ( !creator.m_pData )
		return;

	creator.m_pData->setParamData.eNotificationReason = in_eNotifReason;
	creator.m_pData->setParamData.elementID.id = in_ElementID;
	creator.m_pData->setParamData.elementID.bIsBus = in_bIsBusElement;
	creator.m_pData->setParamData.gameObjPtr = GameObjectToWwiseGameObject( in_GameObject );
	creator.m_pData->setParamData.valueMeaning = in_ValueMeaning;
	creator.m_pData->setParamData.fTarget = in_TargetValue;
	creator.m_pData->setParamData.transitionTime = in_TransitionTime;
}

void AkMonitor::Monitor_Dialogue( 
	AkMonitorData::MonitorDataType in_type, 
	AkUniqueID in_idDialogueEvent, 
	AkUniqueID in_idObject, 
	AkUInt32 in_cPath, 
	AkArgumentValueID * in_pPath, 
	AkPlayingID in_idSequence, 
	AkUInt16 in_uRandomChoice, 
	AkUInt16 in_uTotalProbability,
	AkUInt8 in_uWeightedDecisionType, 
	AkUInt32 in_uWeightedPossibleCount, 
	AkUInt32 in_uWeightedTotalCount )
{
	if ( in_idDialogueEvent == 0 )
		return;

	AkMonitorDataCreator creator( in_type, SIZEOF_MONITORDATA_TO( commonDialogueData.aPath ) + ( in_cPath * sizeof( AkArgumentValueID ) ) );
	if ( !creator.m_pData )
		return;

	creator.m_pData->commonDialogueData.idDialogueEvent = in_idDialogueEvent;
	creator.m_pData->commonDialogueData.idObject = in_idObject;
	creator.m_pData->commonDialogueData.uPathSize = in_cPath;
	creator.m_pData->commonDialogueData.idSequence = in_idSequence;
	creator.m_pData->commonDialogueData.uRandomChoice = in_uRandomChoice;
	creator.m_pData->commonDialogueData.uTotalProbability = in_uTotalProbability;
	creator.m_pData->commonDialogueData.uWeightedDecisionType = in_uWeightedDecisionType;
	creator.m_pData->commonDialogueData.uWeightedPossibleCount = in_uWeightedPossibleCount;
	creator.m_pData->commonDialogueData.uWeightedTotalCount = in_uWeightedTotalCount;

	for ( AkUInt32 i = 0; i < in_cPath; ++i )
		creator.m_pData->commonDialogueData.aPath[ i ] = in_pPath[ i ];
}

//***********************************

void * AkMonitor::Monitor_AllocateGameObjNameString( AkGameObjectID in_GameObject, const char* in_GameObjString )
{
	return m_idxGameObjectString.Preallocate( in_GameObject, in_GameObjString );
}

#ifdef AK_SUPPORT_WCHAR
void AkMonitor::Monitor_SetPoolName( AkMemPoolId in_PoolId, wchar_t * in_tcsPoolName )
{
	// Monitor is not yet instantiated when some basic pools get created -- just skip the
	// notification, no one is possibly connected anyway.
	if ( !AkMonitor::Get() || !in_tcsPoolName )
		return;

	AkUInt16 wStringSize = (AkUInt16) wcslen( in_tcsPoolName ) + 1;

	AkMonitorDataCreator creator( AkMonitorData::MonitorDataMemoryPoolName, offsetof(AkMonitorData::MonitorDataItem, memoryPoolNameData.szName) + wStringSize * sizeof( AkUtf16 ) );
	if ( !creator.m_pData )
		return;

	creator.m_pData->memoryPoolNameData.ulPoolId = in_PoolId;
	creator.m_pData->memoryPoolNameData.wStringSize = wStringSize;
	
	AK_WCHAR_TO_UTF16( creator.m_pData->memoryPoolNameData.szName, in_tcsPoolName, wStringSize);
}

void AkMonitor::Monitor_SetPoolName( AkMemPoolId in_PoolId, char * in_tcsPoolName )
{
	wchar_t wideString[ AK_MAX_PATH ];	
	if(AkCharToWideChar( in_tcsPoolName, AK_MAX_PATH, wideString ) > 0)
		Monitor_SetPoolName( in_PoolId, wideString );
}

#else
void AkMonitor::Monitor_SetPoolName( AkMemPoolId in_PoolId, char * in_tcsPoolName )
{
	// Monitor is not yet instantiated when some basic pools get created -- just skip the
	// notification, no one is possibly connected anyway.
	if ( !AkMonitor::Get() || !in_tcsPoolName )
		return;

	AkUInt16 wStringSize = (AkUInt16) OsStrLen( in_tcsPoolName ) + 1;

	AkMonitorDataCreator creator( AkMonitorData::MonitorDataMemoryPoolName, SIZEOF_MONITORDATA_TO( memoryPoolNameData.szName ) + wStringSize * sizeof( AkUtf16 ) );
	if ( !creator.m_pData )
		return;

	creator.m_pData->memoryPoolNameData.ulPoolId = in_PoolId;
	creator.m_pData->memoryPoolNameData.wStringSize = wStringSize;
	
	AK_CHAR_TO_UTF16( creator.m_pData->memoryPoolNameData.szName, in_tcsPoolName, wStringSize);
}

#endif //AK_SUPPORT_WCHAR

void AkMonitor::Monitor_MarkersNotif( AkPlayingID in_PlayingID, AkGameObjectID in_GameObject, const AkCustomParamType & in_CustomParam, AkMonitorData::NotificationReason in_eNotifReason, AkCntrHistArray in_cntrHistArray, const char* in_strLabel )
{
	AkUInt16 wStringSize = 0;
	if( in_strLabel )
		wStringSize = (AkUInt16)strlen( in_strLabel )+1;

	AkUInt32 sizeofData = SIZEOF_MONITORDATA_TO( markersData.szLabel ) + wStringSize;
	AkMonitorDataCreator creator( AkMonitorData::MonitorDataMarkers, sizeofData );
	if ( !creator.m_pData )
		return;

	creator.m_pData->markersData.eNotificationReason = in_eNotifReason ;
	creator.m_pData->markersData.gameObjPtr = GameObjectToWwiseGameObject( in_GameObject );
	creator.m_pData->markersData.customParam = in_CustomParam;
	creator.m_pData->markersData.playingID = in_PlayingID;
	creator.m_pData->markersData.cntrHistArray = in_cntrHistArray;
	creator.m_pData->markersData.targetObjectID = 0;
	creator.m_pData->markersData.wStringSize = wStringSize;
	if( wStringSize )
		strcpy( creator.m_pData->markersData.szLabel, in_strLabel );
}

void AkMonitor::Monitor_MusicTransNotif( 
	AkPlayingID in_PlayingID, 
	AkGameObjectID in_GameObject, 
	AkMonitorData::NotificationReason in_eNotifReason, 
	AkUInt32 in_transRuleIndex, 
	AkUniqueID in_switchCntrID, 
	AkUniqueID in_nodeSrcID,
	AkUniqueID in_nodeDestID, 
	AkUniqueID in_segmentSrcID, 
	AkUniqueID in_segmentDestID, 
	AkUniqueID in_cueSrc,	// Pass cue name hash if transition was scheduled on cue, AK_INVALID_UNIQUE_ID otherwise
	AkUniqueID in_cueDest,	// Pass cue name hash if transition was scheduled on cue, AK_INVALID_UNIQUE_ID otherwise
	AkTimeMs in_time )
{
	AkMonitorDataCreator creator( AkMonitorData::MonitorDataMusicTransition, SIZEOF_MONITORDATA( musicTransData ) );
	if ( !creator.m_pData )
		return;

	creator.m_pData->musicTransData.playingID				= in_PlayingID;
	creator.m_pData->musicTransData.gameObj					= GameObjectToWwiseGameObject( in_GameObject );
	creator.m_pData->musicTransData.eNotificationReason		= in_eNotifReason;
	creator.m_pData->musicTransData.uTransitionRuleIndex	= in_transRuleIndex;
	creator.m_pData->musicTransData.musicSwitchContainer	= in_switchCntrID;
	creator.m_pData->musicTransData.nodeSrcID				= in_nodeSrcID;
	creator.m_pData->musicTransData.nodeDestID				= in_nodeDestID;
	creator.m_pData->musicTransData.segmentSrc				= in_segmentSrcID;
	creator.m_pData->musicTransData.segmentDest				= in_segmentDestID;
	creator.m_pData->musicTransData.time					= in_time;
	creator.m_pData->musicTransData.cueSrc					= in_cueSrc;
	creator.m_pData->musicTransData.cueDest					= in_cueDest;
}

void AkMonitor::Monitor_TimeStamp()
{
	AkMonitorDataCreator creator( AkMonitorData::MonitorDataTimeStamp, SIZEOF_MONITORDATA( timeStampData ) );
	if ( !creator.m_pData )
		return;

	creator.m_pData->timeStampData.timeStamp = GetThreadTime();
}

void AkMonitor::Monitor_PluginSendData( void * in_pData, AkUInt32 in_uDataSize, AkUniqueID in_audioNodeID, AkPluginID in_pluginID, AkUInt32 in_uFXIndex )
{
	if ( in_uDataSize > 0 )
	{
		AkProfileDataCreator creator( SIZEOF_MONITORDATA_TO( pluginMonitorData.arBytes ) + in_uDataSize );
		if ( !creator.m_pData )
			return;

		creator.m_pData->eDataType = AkMonitorData::MonitorDataPlugin;
		creator.m_pData->pluginMonitorData.audioNodeID	= in_audioNodeID;
		creator.m_pData->pluginMonitorData.pluginID		= in_pluginID;
		creator.m_pData->pluginMonitorData.uFXIndex		= in_uFXIndex;
		creator.m_pData->pluginMonitorData.uDataSize		= in_uDataSize;
		memcpy( creator.m_pData->pluginMonitorData.arBytes, in_pData, in_uDataSize );
	}
}

void AkMonitor::Monitor_ExternalSourceData( AkPlayingID in_idPlay, AkGameObjectID in_idGameObj, AkUniqueID in_idSrc, const AkOSChar* in_pszFile )
{
	AkUInt16 wStringSize = (AkUInt16) AKPLATFORM::OsStrLen( in_pszFile ) + 1;

	AkMonitorDataCreator creator( AkMonitorData::MonitorDataExternalSource, SIZEOF_MONITORDATA_TO( templateSrcData.szUtf16File ) + wStringSize * sizeof( AkUtf16 ) );
	if ( !creator.m_pData )
		return;

	creator.m_pData->templateSrcData.idGameObj = in_idGameObj;
	creator.m_pData->templateSrcData.idPlay = in_idPlay;
	creator.m_pData->templateSrcData.idSource = in_idSrc;
	creator.m_pData->templateSrcData.wStringSize = wStringSize;

	AK_OSCHAR_TO_UTF16( creator.m_pData->templateSrcData.szUtf16File, in_pszFile, wStringSize);
}

// This function was previously inlined, but the 360 compiler had problems inlining it.
AkUInt32 AkMonitorData::RealSizeof( const MonitorDataItem & in_rItem )
{	
	switch ( in_rItem.eDataType )
	{
	case MonitorDataTimeStamp:
		return SIZEOF_MONITORDATA( timeStampData );

	case MonitorDataPluginTimer:
		return SIZEOF_MONITORDATA_TO( pluginTimerData.pluginData )
			+ in_rItem.pluginTimerData.ulNumTimers * sizeof( PluginTimerData );

	case MonitorDataMemoryPool:
		return SIZEOF_MONITORDATA_TO( memoryData.poolData )
			+ in_rItem.memoryData.ulNumPools * sizeof( MemoryPoolData );

	case MonitorDataEnvironment:
		return SIZEOF_MONITORDATA_TO( environmentData.envPacket )
			+ in_rItem.environmentData.ulNumEnvPacket * sizeof( EnvPacket );

	case MonitorDataObsOcc:
		return SIZEOF_MONITORDATA_TO( obsOccData.obsOccPacket )
			+ in_rItem.obsOccData.ulNumPacket * sizeof( ObsOccPacket );

	case MonitorDataListeners:
		return SIZEOF_MONITORDATA_TO( listenerData.gameObjMask )
			+ in_rItem.listenerData.ulNumGameObjMask * sizeof( GameObjectListenerMaskPacket );

    case MonitorDataStreaming:
		return SIZEOF_MONITORDATA_TO( streamingData.streamData )
			+ in_rItem.streamingData.ulNumStreams * sizeof( StreamData );

    case MonitorDataStreamingDevice:
		return SIZEOF_MONITORDATA_TO( streamingDeviceData.deviceData )
			+ in_rItem.streamingDeviceData.ulNumDevices * sizeof( DeviceData );

    case MonitorDataStreamsRecord:
		return SIZEOF_MONITORDATA_TO( streamRecordData.streamRecords )
			+ in_rItem.streamRecordData.ulNumNewRecords * sizeof( StreamRecord );

    case MonitorDataDevicesRecord:
        return SIZEOF_MONITORDATA( deviceRecordData );

	case MonitorDataObject:
		return SIZEOF_MONITORDATA( objectData );

	case MonitorDataAudioPerf:
		return SIZEOF_MONITORDATA( audioPerfData );

	case MonitorDataGameObjPosition:
		return SIZEOF_MONITORDATA_TO( gameObjPositionData.positions )
			+ (in_rItem.gameObjPositionData.ulNumGameObjPositions + 
			   in_rItem.gameObjPositionData.ulNumListenerPositions) 
			   * sizeof( GameObjPositionMonitorData::Position );

	case MonitorDataBank:
		return SIZEOF_MONITORDATA_TO( bankData.szBankName )
			+ in_rItem.bankData.wStringSize;

	case MonitorDataPrepare:
		return SIZEOF_MONITORDATA( prepareData );

	case MonitorDataState:
		return SIZEOF_MONITORDATA( stateData );

	case MonitorDataSwitch:
		return SIZEOF_MONITORDATA( switchData );

	case MonitorDataParamChanged:
		return SIZEOF_MONITORDATA( paramChangedData );

	case MonitorDataEventTriggered:
		return SIZEOF_MONITORDATA( eventTriggeredData );

	case MonitorDataActionDelayed:
		return SIZEOF_MONITORDATA( actionDelayedData );

	case MonitorDataActionTriggered:
		return SIZEOF_MONITORDATA( actionTriggeredData );

	case MonitorDataBusNotif:
		return SIZEOF_MONITORDATA( busNotifData );

	case MonitorDataPath:
		return SIZEOF_MONITORDATA( pathData );

	case MonitorDataSoundBank:
		return SIZEOF_MONITORDATA( loadedSoundBankData );

	case MonitorDataEvent:
		return SIZEOF_MONITORDATA( eventPreparedData );

	case MonitorDataGameSync:
		return SIZEOF_MONITORDATA( gameSyncData );

	case MonitorDataSetParam:
		return SIZEOF_MONITORDATA( setParamData );

	case MonitorDataObjRegistration:
		return SIZEOF_MONITORDATA_TO( objRegistrationData.szName )
			+ in_rItem.objRegistrationData.wStringSize;

	case MonitorDataErrorCode:
	case MonitorDataMessageCode:
		return SIZEOF_MONITORDATA( errorData1 );

	case MonitorDataErrorString:
	case MonitorDataMessageString:
		return SIZEOF_MONITORDATA_TO( debugData.szMessage )
			+ in_rItem.debugData.wStringSize * sizeof( AkUtf16 );

	case MonitorDataMemoryPoolName:
		return SIZEOF_MONITORDATA_TO( memoryPoolNameData.szName )
			+ in_rItem.memoryPoolNameData.wStringSize * sizeof( AkUtf16 );

	case MonitorDataPipeline:
		return SIZEOF_MONITORDATA_TO( pipelineData.placeholder )
			+ in_rItem.pipelineData.numPipelineData * sizeof( PipelineData )
			+ in_rItem.pipelineData.numPipelineDevMap * sizeof( PipelineDevMap );

	case MonitorDataMarkers:
		return SIZEOF_MONITORDATA_TO( markersData.szLabel )
			+ in_rItem.markersData.wStringSize;

	case MonitorDataOutput:
		return SIZEOF_MONITORDATA( outputData );

	case MonitorDataSegmentPosition:
		return SIZEOF_MONITORDATA_TO( segmentPositionData.positions )
			+ in_rItem.segmentPositionData.numPositions * sizeof( SegmentPositionData );

	case MonitorDataControllers:
		return SIZEOF_MONITORDATA_TO( controllerData.gameObjMask )
			+ in_rItem.controllerData.ulNumGameObjMask * sizeof( GameObjectControllerMaskPacket );
			
	case MonitorDataRTPCValues:
		return SIZEOF_MONITORDATA_TO( rtpcValuesData.rtpcValues )
			+ in_rItem.rtpcValuesData.ulNumRTPCValues * sizeof( RTPCValuesPacket );

	case MonitorDataMedia:
		return SIZEOF_MONITORDATA_TO( mediaPreparedData.bankMedia )
			+ in_rItem.mediaPreparedData.uArraySize * sizeof( AkMonitorData::MediaPreparedMonitorData::BankMedia );

	case MonitorDataFeedback:
		return SIZEOF_MONITORDATA( feedbackData );

	case MonitorDataFeedbackDevices:
		return SIZEOF_MONITORDATA( feedbackDevicesData )
			+ (in_rItem.feedbackDevicesData.usDeviceCount - 1) * sizeof(AkMonitorData::FeedbackDeviceIDMonitorData);

	case MonitorDataFeedbackGameObjs:
		return SIZEOF_MONITORDATA( feedbackGameObjData )
			+ (in_rItem.feedbackGameObjData.ulNumGameObjMask - 1) * sizeof(AkMonitorData::GameObjectPlayerMaskPacket);
	
	case MonitorDataResolveDialogue:	
		return SIZEOF_MONITORDATA_TO( commonDialogueData.aPath )
			+ in_rItem.commonDialogueData.uPathSize * sizeof( AkArgumentValueID );

	case MonitorDataMusicTransition:
		return SIZEOF_MONITORDATA( musicTransData );

	case MonitorDataPlugin:
		return SIZEOF_MONITORDATA_TO( pluginMonitorData.arBytes )
			+ in_rItem.pluginMonitorData.uDataSize;

	case MonitorDataExternalSource:
		return SIZEOF_MONITORDATA_TO( templateSrcData.szUtf16File )
			+ in_rItem.templateSrcData.wStringSize * sizeof( AkUtf16 );

	case MonitorDataMeter:
		return SIZEOF_MONITORDATA_TO( meterData.busMeters )
			+ (in_rItem.meterData.uNumBusses)* sizeof( BusMeterData );

	case MonitorDataSends:
		return SIZEOF_MONITORDATA( sendsData )
			+ (in_rItem.sendsData.ulNumSends - 1) * sizeof(AkMonitorData::SendsData);

	case MonitorDataPlatformSinkType:
		return SIZEOF_MONITORDATA( platformSinkTypeData )
			+ (in_rItem.platformSinkTypeData.uBufSize) * sizeof(char);

	default:
		AKASSERT( false && "Need accurate size for MonitorDataItem member" );
		return sizeof( MonitorDataItem );
	}
}
#endif
