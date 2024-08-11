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
// AkRegistryMgr.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkRegistryMgr.h"
#include "AkRegisteredObj.h"
#include "Ak3DListener.h"
#include "AkParameterNodeBase.h"
#include "AkMonitor.h"
#include "AkRTPCMgr.h"
#include "AkEnvironmentsMgr.h"
#include "AkLEngine.h"

#ifdef AK_MOTION
	#include "AkFeedbackMgr.h"
#endif // AK_MOTION

CAkRegistryMgr::CAkRegistryMgr()
{
}

CAkRegistryMgr::~CAkRegistryMgr()
{
}

AKRESULT CAkRegistryMgr::Init()
{
	m_mapRegisteredObj.Init( g_DefaultPoolId );
	AKRESULT eResult = m_listModifiedNodes.Init( MIN_SIZE_REG_ENTRIES, MAX_SIZE_REG_ENTRIES );

	if ( eResult == AK_Success )
	{
		// Register 'omni' object at the default listener's position.
		CAkRegisteredObj * pObj = RegisterObject( 0 ); // Register 'omni' game object
		if ( pObj )
		{
			AkSoundPosition pos = { 0 };
			pObj->SetPosition( 
				&pos, 
				1, 
				AK::SoundEngine::MultiPositionType_SingleSource
				);
		}
	}

#ifndef AK_OPTIMIZED
	m_relativePositionTransport = g_DefaultSoundPosition;
#endif

	return eResult;
}

void CAkRegistryMgr::Term()
{
	if( m_mapRegisteredObj.IsInitialized() )
		UnregisterObject( 0 ); // unregister 'omni' game object

	UnregisterAll();
	m_mapRegisteredObj.Term();
	m_listModifiedNodes.Term();
}

CAkRegisteredObj * CAkRegistryMgr::GetObjAndAddref( AkGameObjectID in_GameObjectID )
{
	CAkRegisteredObj** l_ppRegObj = m_mapRegisteredObj.Exists( in_GameObjectID );
	if(!l_ppRegObj)
		return NULL;

	CAkRegisteredObj* l_pRegObj = *l_ppRegObj;

	l_pRegObj->AddRef();

	return l_pRegObj;
}

CAkRegisteredObj* CAkRegistryMgr::RegisterObject( AkGameObjectID in_GameObjectID, AkUInt32 uListenerMask, void * in_pMonitorData )
{
	CAkRegisteredObj** l_ppRegObj = m_mapRegisteredObj.Exists( in_GameObjectID );
	if(l_ppRegObj)
	{
		MONITOR_FREESTRING( in_pMonitorData );
		(*l_ppRegObj)->SetActiveListeners(uListenerMask);
		return *l_ppRegObj;
	}

	CAkRegisteredObj* l_pRegObj = AkNew( g_DefaultPoolId, CAkRegisteredObj( in_GameObjectID ) );
	if( l_pRegObj )
	{
		l_ppRegObj = m_mapRegisteredObj.Set( in_GameObjectID );
		if ( l_ppRegObj )
		{
			MONITOR_OBJREGISTRATION( true, in_GameObjectID, in_pMonitorData );
			l_pRegObj->SetActiveListeners(uListenerMask);
			*l_ppRegObj = l_pRegObj;
		}
		else
		{
			MONITOR_FREESTRING( in_pMonitorData );
			AkDelete( g_DefaultPoolId, l_pRegObj );
			l_pRegObj = NULL;
		}
	}
	else
	{
		MONITOR_FREESTRING( in_pMonitorData );
	}

	return l_pRegObj;
}

void CAkRegistryMgr::UnregisterObject(AkGameObjectID in_GameObjectID)
{
	AkMapRegisteredObj::IteratorEx it = m_mapRegisteredObj.FindEx( in_GameObjectID );
	if( it != m_mapRegisteredObj.End() )
	{
		CAkRegisteredObj * pGameObj = (*it).item;
		m_mapRegisteredObj.Erase( it );
		pGameObj->Unregister();
		pGameObj->Release();
		
		// Unregistered
		MONITOR_OBJREGISTRATION( false, in_GameObjectID, NULL );
	}
}

#ifndef AK_OPTIMIZED
void CAkRegistryMgr::SetRelativePosition( 
	const AkSoundPosition & in_rPosition
	)
{
	m_relativePositionTransport = in_rPosition;
	NotifyListenerPosChanged( 0 );
}
#endif

AKRESULT CAkRegistryMgr::SetPosition( 
	AkGameObjectID in_GameObjectID, 
	const AkSoundPosition* in_aPositions,
	AkUInt16 in_uNumPositions,
	AK::SoundEngine::MultiPositionType in_eMultiPositionType
	)
{
	CAkRegisteredObj** ppRegObj = m_mapRegisteredObj.Exists(in_GameObjectID);
	if ( !ppRegObj )
		return AK_Fail;

	(*ppRegObj)->SetPosition( in_aPositions, in_uNumPositions, in_eMultiPositionType );

	return AK_Success;
}

AKRESULT CAkRegistryMgr::SetActiveListeners(
	AkGameObjectID in_GameObjectID,	///< Game object.
	AkUInt32 in_uListenerMask		///< Bitmask representing active listeners. LSB = Listener 0, set to 1 means active.
	)
{
	CAkRegisteredObj** ppRegObj = m_mapRegisteredObj.Exists(in_GameObjectID);
	if ( !ppRegObj )
		return AK_Fail;
	
	CAkLEngine::ReevaluateBussesForGameObject((*ppRegObj), (*ppRegObj)->GetListenerMask(), in_uListenerMask);

	(*ppRegObj)->SetActiveListeners( in_uListenerMask );

	return AK_Success;
}

#if defined AK_WII
AKRESULT CAkRegistryMgr::SetActiveControllers(
	AkGameObjectID in_GameObjectID,
	AkUInt32 in_uActiveControllerMask
	)
{
	if ( in_GameObjectID == 0 ) // omni
		return AK_Fail;

	CAkRegisteredObj** ppRegObj = m_mapRegisteredObj.Exists(in_GameObjectID);
	if ( !ppRegObj )
		return AK_Fail;	

	(*ppRegObj)->SetActiveControllers( in_uActiveControllerMask );

	return AK_Success;
}
#endif

AKRESULT CAkRegistryMgr::GetPosition( 
	AkGameObjectID in_GameObjectID, 
	AkSoundPositionRef & out_Position)
{
	CAkRegisteredObj** ppRegObj = m_mapRegisteredObj.Exists(in_GameObjectID);
	if ( !ppRegObj )
		return AK_Fail;

	out_Position = (*ppRegObj)->GetPosition();

	return AK_Success;
}

AKRESULT CAkRegistryMgr::SetGameObjectAuxSendValues( 
		AkGameObjectID		in_GameObjectID,
		AkAuxSendValue*	in_aEnvironmentValues,
		AkUInt32			in_uNumEnvValues
		)
{
	CAkRegisteredObj** ppRegObj = m_mapRegisteredObj.Exists( in_GameObjectID );
	if ( !ppRegObj )
		return AK_Fail;

	return (*ppRegObj)->SetGameObjectAuxSendValues( in_aEnvironmentValues, in_uNumEnvValues );
}

AKRESULT CAkRegistryMgr::SetGameObjectOutputBusVolume( 
		AkGameObjectID		in_GameObjectID,	///< the unique object ID
		AkReal32			in_fControlValue	///< control value for dry level
		)
{
	CAkRegisteredObj** ppRegObj = m_mapRegisteredObj.Exists( in_GameObjectID );
	if ( !ppRegObj )
		return AK_Fail;

	(*ppRegObj)->SetDryLevelValue( in_fControlValue );
	return AK_Success;
}

AKRESULT CAkRegistryMgr::SetGameObjectScalingFactor(
		AkGameObjectID		in_GameObjectID,
		AkReal32			in_fControlValue
		)
{
	CAkRegisteredObj** ppRegObj = m_mapRegisteredObj.Exists( in_GameObjectID );
	if ( !ppRegObj )
		return AK_Fail;

	(*ppRegObj)->SetScalingFactor( in_fControlValue );
	return AK_Success;
}

AKRESULT CAkRegistryMgr::SetObjectObstructionAndOcclusion(  
	AkGameObjectID in_GameObjectID,		///< Game object ID.
	AkUInt32 in_uListener,				///< Listener ID.
	AkReal32 in_fObstructionLevel,		///< ObstructionLevel : [0.0f..1.0f]
	AkReal32 in_fOcclusionLevel			///< OcclusionLevel   : [0.0f..1.0f]
	)
{
	CAkRegisteredObj** ppRegObj = m_mapRegisteredObj.Exists( in_GameObjectID );
	if ( !ppRegObj )
		return AK_Fail;

	return (*ppRegObj)->SetObjectObstructionAndOcclusion( in_uListener, in_fObstructionLevel, in_fOcclusionLevel );
}

AkSwitchHistItem CAkRegistryMgr::GetSwitchHistItem( 
	CAkRegisteredObj *	in_pGameObj,
	AkUniqueID          in_SwitchContID
	)
{
	AKASSERT( in_pGameObj );
	if ( in_pGameObj != NULL )
	{
		AkSwitchHistItem * pSwitchHistItem = in_pGameObj->GetSwitchHist().Exists( in_SwitchContID );
		if ( pSwitchHistItem )
		{
			return *pSwitchHistItem;
		}
	}

	AkSwitchHistItem item;
	item.LastSwitch = AK_INVALID_UNIQUE_ID;
	item.NumPlayBack = 0;
	return item;
}

AKRESULT CAkRegistryMgr::SetSwitchHistItem( 
	CAkRegisteredObj *	in_pGameObj,
	AkUniqueID          in_SwitchContID,
	const AkSwitchHistItem & in_SwitchHistItem
	)
{
	AKASSERT( in_pGameObj );
	if ( in_pGameObj != NULL )
	{
		return in_pGameObj->GetSwitchHist().Set( in_SwitchContID, in_SwitchHistItem ) ? AK_Success : AK_Fail;
	}
	return AK_Fail;
}

AKRESULT CAkRegistryMgr::ClearSwitchHist(
	AkUniqueID          in_SwitchContID,
	CAkRegisteredObj *	in_pGameObj
	)
{
	if( in_pGameObj != NULL )
	{
		in_pGameObj->GetSwitchHist().Unset( in_SwitchContID );
	}
	else
	{
		for ( AkMapRegisteredObj::Iterator iter = m_mapRegisteredObj.Begin(); iter != m_mapRegisteredObj.End(); ++iter )
			(*iter).item->GetSwitchHist().Unset( in_SwitchContID );
	}

	return AK_Success;
}

void CAkRegistryMgr::UnregisterAll()
{
	if ( m_mapRegisteredObj.IsInitialized() )
	{
		for ( AkMapRegisteredObj::IteratorEx iter = m_mapRegisteredObj.BeginEx(); iter != m_mapRegisteredObj.End(); )
		{
			if( (*iter).key != 0 ) //skip game object 0
			{
				(*iter).item->Release();

				// Unregistered
				MONITOR_OBJREGISTRATION( false, (*iter).key, NULL );

				iter = m_mapRegisteredObj.Erase( iter );
			}
			else
				++iter;
		}
	}
}

void CAkRegistryMgr::SetNodeIDAsModified(CAkParameterNodeBase* in_pNode)
{
	AKASSERT(in_pNode);
	WwiseObjectID wwiseId( in_pNode->ID(), in_pNode->IsBusCategory() );

	if( !m_listModifiedNodes.Exists( wwiseId ) )
	{
		// What if this AddLast fails... Should handle it correctly, allowing memory to be released when unregistering a game object.
		m_listModifiedNodes.AddLast( wwiseId );
	}
}

void CAkRegistryMgr::NotifyListenerPosChanged(
	AkUInt32 in_uListenerMask	// Bitmask of listeners whose position changed.
	)
{
	AkMapRegisteredObj::Iterator it = m_mapRegisteredObj.Begin();
	while ( it != m_mapRegisteredObj.End() )
	{
		(*it).item->NotifyListenerPosDirty( in_uListenerMask );
		++it;
	}

	// Update Wwise Transport game object position.
#ifndef AK_OPTIMIZED
	CAkRegisteredObj** ppRegObj = m_mapRegisteredObj.Exists( 0 );
	if ( ppRegObj )
	{
		// m_relativePositionTransport vectors assume an unrotated listener. 
		// Construct real position for GO_Transport.
		const AkListenerData & listenerData = CAkListener::GetListenerData( 0 );
		const AkReal32 * pRotationMatrix = &listenerData.Matrix[0][0];
		AkSoundPosition pos;

		// Position: rotate relative position with listener's orientation, then translate to its origin.
		AkVector rotatedPos;
		AkMath::RotateVector( m_relativePositionTransport.Position, pRotationMatrix, rotatedPos );
		const AkVector & listenerPos = listenerData.position.Position;
		pos.Position.X = listenerPos.X + rotatedPos.X;
		pos.Position.Y = listenerPos.Y + rotatedPos.Y;
		pos.Position.Z = listenerPos.Z + rotatedPos.Z;

		// Orientation: rotate relative orientation with listener's orientation.
		AkMath::RotateVector( m_relativePositionTransport.Orientation, pRotationMatrix, pos.Orientation );

		(*ppRegObj)->SetPosition( &pos, 1, AK::SoundEngine::MultiPositionType_SingleSource );
	}
#endif
}

AKRESULT CAkRegistryMgr::GetActiveGameObjects( 
		AK::SoundEngine::Query::AkGameObjectsList& io_GameObjectList	///< returned list of active game objects.
		)
{
	for ( AkMapRegisteredObj::IteratorEx iter = m_mapRegisteredObj.BeginEx(); iter != m_mapRegisteredObj.End(); ++iter )
	{
		if( (*iter).item->IsActive() )
		{
			if( !io_GameObjectList.AddLast( (*iter).key ) )
				return AK_InsufficientMemory;
		}
	}

	return AK_Success;
}

bool CAkRegistryMgr::IsGameObjectActive( 
		AkGameObjectID in_GameObjectId
		)
{
	AkMapRegisteredObj::Iterator iter = m_mapRegisteredObj.FindEx( in_GameObjectId ) ;
	if( iter != m_mapRegisteredObj.End() )
		return (*iter).item->IsActive();

	return false;
}

#ifndef AK_OPTIMIZED
void CAkRegistryMgr::PostEnvironmentStats()
{
	//Count Num game objects that have an environment
	AkUInt32 uNumGameObj = 0;

	for ( AkMapRegisteredObj::Iterator iter = m_mapRegisteredObj.Begin(); iter != m_mapRegisteredObj.End(); ++iter )
	{
		const AkAuxSendValue* pEvn = (*iter).item->GetGameDefinedAuxSends();
		if( pEvn->auxBusID != AK_INVALID_AUX_ID )
		{
			++uNumGameObj;
		}
	}

    AkInt32 sizeofItem = SIZEOF_MONITORDATA_TO( environmentData.envPacket )
						+ uNumGameObj * sizeof( AkMonitorData::EnvPacket );

    AkProfileDataCreator creator( sizeofItem );
	if ( !creator.m_pData )
		return;

    creator.m_pData->eDataType = AkMonitorData::MonitorDataEnvironment;

	creator.m_pData->environmentData.ulNumEnvPacket = uNumGameObj;

	int i = 0;
	for ( AkMapRegisteredObj::Iterator iter = m_mapRegisteredObj.Begin(); iter != m_mapRegisteredObj.End(); ++iter )
	{
		const AkAuxSendValue* pEvn = (*iter).item->GetGameDefinedAuxSends();
		if( pEvn->auxBusID != AK_INVALID_AUX_ID )
		{
			creator.m_pData->environmentData.envPacket[i].gameObjID = GameObjectToWwiseGameObject( (*iter).key );
			creator.m_pData->environmentData.envPacket[i].fDryValue = (*iter).item->GetDryLevelValue();
			int j = 0;
			for(; j < AK_MAX_AUX_PER_OBJ; ++j )
			{
				creator.m_pData->environmentData.envPacket[i].environments[j] = pEvn[j];
			}
			for(; j < AK_MAX_GAME_DEFINED_AUX_PER_OBJ_PROFILER; ++j )
			{
				creator.m_pData->environmentData.envPacket[i].environments[j].auxBusID = AK_INVALID_AUX_ID;
				creator.m_pData->environmentData.envPacket[i].environments[j].fControlValue = 0.f;
			}
			++i;
		}
	}
}

void CAkRegistryMgr::PostObsOccStats()
{
	//Count num game objects that have an obs or occ and will require a packet
	AkUInt32 uNumPackets = 0;

	//Keep track of which registered objects contain relevant obs/occ info
	AkUInt32 uNumRegObj = m_mapRegisteredObj.Length();
	bool* abObjContainsObsOcc = (bool*)AkAlloc( AkMonitor::Monitor_GetPoolId(), sizeof( bool ) * uNumRegObj );
	if( !abObjContainsObsOcc )
		return; //memory error

	memset( abObjContainsObsOcc, 0, sizeof( bool ) * uNumRegObj );

	int iCurrObj = 0;
	for ( AkMapRegisteredObj::Iterator iter = m_mapRegisteredObj.Begin(); iter != m_mapRegisteredObj.End(); ++iter )
	{
		for ( int iListener = 0; iListener < AK_NUM_LISTENERS; ++iListener )
		{
			AkReal32 fObsValue = (*iter).item->GetObjectObstructionValue( iListener );
			AkReal32 fOccValue = (*iter).item->GetObjectOcclusionValue( iListener );
			if( ( fObsValue != 0.0f ) || ( fOccValue != 0.0f ) )
			{
				++uNumPackets;
				abObjContainsObsOcc[iCurrObj] = true;
				break; //go to next object
			}
		}
		iCurrObj++;
	}

    AkInt32 sizeofItem = SIZEOF_MONITORDATA_TO( obsOccData.obsOccPacket )
						+ uNumPackets * sizeof( AkMonitorData::ObsOccPacket );

    AkProfileDataCreator creator( sizeofItem );
	if ( !creator.m_pData )
		return;

    creator.m_pData->eDataType = AkMonitorData::MonitorDataObsOcc;

	creator.m_pData->obsOccData.ulNumPacket = uNumPackets;

	int iCurrPacket = 0;
	iCurrObj = 0; //reset our counter
	for ( AkMapRegisteredObj::Iterator iter = m_mapRegisteredObj.Begin(); iter != m_mapRegisteredObj.End(); ++iter )
	{
		if( abObjContainsObsOcc[iCurrObj++] )
		{
			creator.m_pData->obsOccData.obsOccPacket[iCurrPacket].gameObjID = GameObjectToWwiseGameObject( (*iter).key );
			for ( int iListener = 0; iListener < AK_NUM_LISTENERS; ++iListener )
			{
				AkReal32 fObsValue = (*iter).item->GetObjectObstructionValue( iListener );
				AkReal32 fOccValue = (*iter).item->GetObjectOcclusionValue( iListener );
				creator.m_pData->obsOccData.obsOccPacket[iCurrPacket].fObsValue[iListener] = fObsValue;
				creator.m_pData->obsOccData.obsOccPacket[iCurrPacket].fOccValue[iListener] = fOccValue;
			}
			++iCurrPacket;
		}
	}

	if( abObjContainsObsOcc )
		AkFree( AkMonitor::Monitor_GetPoolId(), abObjContainsObsOcc );
}

void CAkRegistryMgr::PostListenerStats()
{
	AkUInt32 uNumGameObj = m_mapRegisteredObj.Length();

    AkInt32 sizeofItem = SIZEOF_MONITORDATA_TO( listenerData.gameObjMask )
						+ uNumGameObj * sizeof( AkMonitorData::GameObjectListenerMaskPacket );

    AkProfileDataCreator creator( sizeofItem );
	if ( !creator.m_pData )
		return;

	creator.m_pData->eDataType = AkMonitorData::MonitorDataListeners;

#ifdef AK_MOTION
	CAkFeedbackDeviceMgr *pFeedbackMgr = CAkFeedbackDeviceMgr::Get();
#endif // AK_MOTION

	for( int i = 0; i < AK_NUM_LISTENERS; ++i )
	{
		const AkListenerData & rListener = CAkListener::GetListenerData( i );
		AkMonitorData::ListenerPacket &rPacket = creator.m_pData->listenerData.listeners[i];
		rPacket.bSpatialized = rListener.bSpatialized;

		AkSIMDSpeakerVolumes customSpeakerGain;
		customSpeakerGain = rListener.customSpeakerGain;
		customSpeakerGain.FastLinTodB();
	
		rPacket.VolumeOffset.fFrontLeft = customSpeakerGain.volumes.fFrontLeft;
		rPacket.VolumeOffset.fFrontRight = customSpeakerGain.volumes.fFrontRight;

#ifdef AK_LFECENTER
		rPacket.VolumeOffset.fCenter = customSpeakerGain.volumes.fCenter;
		rPacket.VolumeOffset.fLfe = customSpeakerGain.volumes.fLfe;
#else
		rPacket.VolumeOffset.fCenter = 0;
		rPacket.VolumeOffset.fLfe = 0;
#endif // AK_LFECENTER

#ifdef AK_REARCHANNELS
		rPacket.VolumeOffset.fRearLeft = customSpeakerGain.volumes.fRearLeft;
		rPacket.VolumeOffset.fRearRight = customSpeakerGain.volumes.fRearRight;
#else
		rPacket.VolumeOffset.fRearLeft = 0;
		rPacket.VolumeOffset.fRearRight = 0;
#endif // AK_REARCHANNELS

#ifdef AK_71AUDIO
		rPacket.VolumeOffset.fSideLeft = customSpeakerGain.volumes.fSideLeft;
		rPacket.VolumeOffset.fSideRight = customSpeakerGain.volumes.fSideRight;
#else
		rPacket.VolumeOffset.fSideLeft = 0;
		rPacket.VolumeOffset.fSideRight = 0;
#endif

		rPacket.bMotion = (CAkListener::GetFeedbackMask() & (1 << i)) != 0;
				
#ifdef AK_MOTION
		if (pFeedbackMgr != NULL)
			rPacket.iMotionPlayer = pFeedbackMgr->ListenerToPlayer(i);
		else
#endif // AK_MOTION
			rPacket.iMotionPlayer = 0;
	}

	creator.m_pData->listenerData.ulNumGameObjMask = uNumGameObj;

	int j = 0;
	for ( AkMapRegisteredObj::Iterator iter = m_mapRegisteredObj.Begin(); iter != m_mapRegisteredObj.End(); ++iter )
	{
		creator.m_pData->listenerData.gameObjMask[j].gameObject = GameObjectToWwiseGameObject( (*iter).key );
		creator.m_pData->listenerData.gameObjMask[j].uListenerMask = (*iter).item->GetListenerMask();
		++j;
	}
}

#if defined AK_WII_FAMILY

#include "AkWiimoteMgr.h"

void CAkRegistryMgr::PostControllerStats()
{
	AkUInt32 uNumGameObj = m_mapRegisteredObj.Length();

    AkInt32 sizeofItem = offsetof( AkMonitorData::MonitorDataItem, controllerData.gameObjMask )
						+ uNumGameObj * sizeof( AkMonitorData::GameObjectControllerMaskPacket );

    AkProfileDataCreator creator( sizeofItem );
	if ( !creator.m_pData )
		return;

	creator.m_pData->eDataType = AkMonitorData::MonitorDataControllers;

	for( AkUInt32 i = 0; i < WPAD_MAX_CONTROLLERS; ++i )
	{
		creator.m_pData->controllerData.controllers[i].bIsActive = CAkWiimoteMgr::AreSpeakersActivated();
		creator.m_pData->controllerData.controllers[i].Volume = CAkWiimoteMgr::GetSpeakerVolume(i);
	}

	creator.m_pData->controllerData.ulNumGameObjMask = uNumGameObj;

	int j = 0;
	for ( AkMapRegisteredObj::Iterator iter = m_mapRegisteredObj.Begin(); iter != m_mapRegisteredObj.End(); ++iter )
	{
		creator.m_pData->controllerData.gameObjMask[j].gameObject = GameObjectToWwiseGameObject( (*iter).key );
		creator.m_pData->controllerData.gameObjMask[j].uControllerMask = (*iter).item->GetPosition().GetControllerActiveMask();
		++j;
	}
}
#endif // AK_WII

#ifdef AK_MOTION
void CAkRegistryMgr::PostfeedbackGameObjStats()
{
	CAkFeedbackDeviceMgr * pMgr = CAkFeedbackDeviceMgr::Get();
	if (pMgr == NULL)
		return;
		
	//Go through the game objects
	AkUInt32 uSize = NumRegisteredObject();

	AkUInt32 sizeofItem = offsetof( AkMonitorData::MonitorDataItem, feedbackGameObjData ) + sizeof(AkMonitorData::FeedbackGameObjMonitorData) 
		+ (uSize - 1) * sizeof(AkMonitorData::GameObjectPlayerMaskPacket);	//Minus 1 because there is already one in the struct.

	//Fill the allocated struct
	AkProfileDataCreator creator( sizeofItem );
	if ( !creator.m_pData )
		return;

	creator.m_pData->eDataType = AkMonitorData::MonitorDataFeedbackGameObjs;
	creator.m_pData->feedbackGameObjData.ulNumGameObjMask = uSize;

	AkUInt32 i = 0;
	for ( AkMapRegisteredObj::Iterator iter = m_mapRegisteredObj.Begin(); iter != m_mapRegisteredObj.End(); ++iter , ++i)
	{
		AkMonitorData::GameObjectPlayerMaskPacket &rGameObjInfo = creator.m_pData->feedbackGameObjData.gameObjInfo[i];
		rGameObjInfo.uPlayerMask = 0;
		rGameObjInfo.gameObject = GameObjectToWwiseGameObject( (*iter).key );

		AkUInt32 uListenersMask = (*iter).item->GetListenerMask() & CAkListener::GetFeedbackMask();
		for(AkUInt8 iListener = 0; iListener < AK_NUM_LISTENERS; ++iListener)
		{
			if (uListenersMask & (1 << iListener))
				rGameObjInfo.uPlayerMask |= pMgr->ListenerToPlayer(iListener);
		}
	}
}
#endif
#endif
