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

// AkQueryParameters.cpp : Get some parameters values from the sound engine
//
#include "stdafx.h"

#include "Ak3DListener.h"
#include "AkAudioLib.h"
#include <AK/Tools/Common/AkAutoLock.h>
#include "AkAudioMgr.h"
#include "AkCritical.h"
#include "AkEnvironmentsMgr.h"
#include "AkEvent.h"
#include "AkStateMgr.h"
#include "AkRegistryMgr.h"
#include "AkRegisteredObj.h"
#include "AkParameterNode.h"
#include <AK/SoundEngine/Common/AkQueryParameters.h>
#include "AkURenderer.h"
#include "AkPlayingMgr.h"

namespace AK
{
	namespace SoundEngine
	{
		namespace Query
		{
			AKRESULT GetPosition( 
				AkGameObjectID in_GameObj, 
				AkSoundPosition& out_rPosition
				)
			{
				CAkFunctionCritical SetSpaceAsCritical; //use global lock
				CAkRegisteredObj * pObj = g_pRegistryMgr->GetObjAndAddref( in_GameObj );
				if( !pObj )
					return AK_IDNotFound;
				
				out_rPosition = pObj->GetPosition().GetFirstPositionFixme();
				pObj->Release();
				return AK_Success;
			}

			AKRESULT GetActiveListeners(
				AkGameObjectID in_GameObj,			///< Game object.
				AkUInt32& out_ruListenerMask		///< Bitmask representing active listeners. LSB = Listener 0, set to 1 means active.
				)
			{
				CAkFunctionCritical SetSpaceAsCritical; //use global lock
				CAkRegisteredObj * pObj = g_pRegistryMgr->GetObjAndAddref( in_GameObj );
				if( !pObj )
					return AK_IDNotFound;

				out_ruListenerMask = pObj->GetListenerMask();
				pObj->Release();

				return AK_Success;
			}

			AKRESULT GetListenerPosition( 
				AkUInt32 in_ulIndex,
				AkListenerPosition& out_rPosition
				)
			{
				CAkFunctionCritical SetSpaceAsCritical; //use global lock
				if( in_ulIndex >= AK_NUM_LISTENERS )
					return AK_InvalidParameter;
				CAkListener::GetPosition( in_ulIndex, out_rPosition );
				return AK_Success;
			}

			AKRESULT GetListenerSpatialization(
				AkUInt32 in_ulIndex,						///< Listener index. 
				bool& out_rbSpatialized,
				AkSpeakerVolumes& out_rVolumeOffsets
				)
			{
				CAkFunctionCritical SetSpaceAsCritical; //use global lock
				return CAkListener::GetListenerSpatialization( in_ulIndex, out_rbSpatialized, out_rVolumeOffsets );
			}

			AKRESULT GetRTPCValue( 
				AkRtpcID in_RTPCid, 
				AkGameObjectID in_GameObj,
				AkReal32& out_rfValue, 
				RTPCValue_type&	io_rValueType
				)
			{
				CAkFunctionCritical SetSpaceAsCritical; //use global lock


				switch( io_rValueType )
				{
				case RTPCValue_Global:
					// Make sure we don't force GameObject specific, the user asked for global.
					in_GameObj = AK_INVALID_GAME_OBJECT;

					// No Break here.

				case RTPCValue_GameObject:
					{
						CAkRegisteredObj * pObj = g_pRegistryMgr->GetObjAndAddref( in_GameObj );
						bool bObjectSpecific;
						bool bResult = g_pRTPCMgr->GetRTPCValue( in_RTPCid, pObj, out_rfValue, bObjectSpecific );
						io_rValueType = bObjectSpecific ? RTPCValue_GameObject : RTPCValue_Global;
						if( pObj )
							pObj->Release();

						if( bResult )
							return AK_Success;
					}
					// Failed to get GameObject/Global value, fallback on Default.

					// No Break here.

				case RTPCValue_Default:
				default:
					bool bRTPCFound;
					out_rfValue = g_pRTPCMgr->GetDefaultValue( in_RTPCid, &bRTPCFound );
					io_rValueType = bRTPCFound ? RTPCValue_Default : RTPCValue_Unavailable;
					break;
				}
				
				return AK_Success;
			}

#ifdef AK_SUPPORT_WCHAR
			AKRESULT GetRTPCValue( 
				const wchar_t* in_pszRTPCName, 
				AkGameObjectID in_GameObj,
				AkReal32& out_rfValue, 
				RTPCValue_type&	io_rValueType
				)
			{
				AkRtpcID id = AK::SoundEngine::GetIDFromString( in_pszRTPCName );
				if ( id == AK_INVALID_RTPC_ID )
					return AK_IDNotFound;

				return GetRTPCValue( id, in_GameObj, out_rfValue, io_rValueType );
			}
#endif //AK_SUPPORT_WCHAR

			AKRESULT GetRTPCValue( 
				const char* in_pszRTPCName, 
				AkGameObjectID in_GameObj,
				AkReal32& out_rfValue, 
				RTPCValue_type&	io_rValueType
				)
			{
				AkRtpcID id = AK::SoundEngine::GetIDFromString( in_pszRTPCName );
				if ( id == AK_INVALID_RTPC_ID )
					return AK_IDNotFound;

				return GetRTPCValue( id, in_GameObj, out_rfValue, io_rValueType );
			}

			AKRESULT GetSwitch( 
				AkSwitchGroupID in_SwitchGroup, 
				AkGameObjectID in_GameObj,
				AkSwitchStateID& out_rSwitchState
				)
			{
				CAkFunctionCritical SetSpaceAsCritical; //use global lock
				CAkRegisteredObj * pObj = g_pRegistryMgr->GetObjAndAddref( in_GameObj );
				if( !pObj )
					return AK_IDNotFound;

				out_rSwitchState = g_pRTPCMgr->GetSwitch( in_SwitchGroup, pObj );
				pObj->Release();
				return AK_Success;
			}

#ifdef AK_SUPPORT_WCHAR
			AKRESULT GetSwitch( 
				const wchar_t* in_pstrSwitchGroupName, 
				AkGameObjectID in_GameObj,
				AkSwitchStateID& out_rSwitchState
				)
			{
				AkSwitchGroupID switchGroup = AK::SoundEngine::GetIDFromString( in_pstrSwitchGroupName );
				if ( switchGroup == AK_INVALID_RTPC_ID )
					return AK_IDNotFound;

				return GetSwitch( switchGroup, in_GameObj, out_rSwitchState );
			}
#endif //AK_SUPPORT_WCHAR

			AKRESULT GetSwitch( 
				const char* in_pstrSwitchGroupName, 
				AkGameObjectID in_GameObj,
				AkSwitchStateID& out_rSwitchState
				)
			{
				AkSwitchGroupID switchGroup = AK::SoundEngine::GetIDFromString( in_pstrSwitchGroupName );
				if ( switchGroup == AK_INVALID_RTPC_ID )
					return AK_IDNotFound;

				return GetSwitch( switchGroup, in_GameObj, out_rSwitchState );
			}

			AKRESULT GetState( 
				AkStateGroupID in_StateGroup, 
				AkStateID& out_rState )
			{
				CAkFunctionCritical SetSpaceAsCritical; //use global lock
				out_rState = g_pStateMgr->GetState( in_StateGroup );
				return AK_Success;
			}

#ifdef AK_SUPPORT_WCHAR
			AKRESULT GetState( 
				const wchar_t* in_pstrStateGroupName, 
				AkStateID& out_rState )
			{
				AkStateGroupID stateGroup = AK::SoundEngine::GetIDFromString( in_pstrStateGroupName );
				return GetState( stateGroup, out_rState );
			}
#endif //AK_SUPPORT_WCHAR

			AKRESULT GetState( 
				const char* in_pstrStateGroupName, 
				AkStateID& out_rState )
			{
				AkStateGroupID stateGroup = AK::SoundEngine::GetIDFromString( in_pstrStateGroupName );
				return GetState( stateGroup, out_rState );
			}

			AKRESULT GetGameObjectAuxSendValues( 
				AkGameObjectID		in_GameObj,					///< the unique object ID
				AkAuxSendValue*	out_paEnvironmentValues,	///< variable-size array of AkAuxSendValue(s)
				AkUInt32&			io_ruNumEnvValues			///< number of elements in struct
				)
			{
				AKRESULT eReturn = AK_Success;

				if( io_ruNumEnvValues == 0 || !out_paEnvironmentValues )
					return AK_InvalidParameter;

				CAkFunctionCritical SetSpaceAsCritical; //use global lock
				CAkRegisteredObj * pObj = g_pRegistryMgr->GetObjAndAddref( in_GameObj );
				if( !pObj )
					return AK_IDNotFound;

				const AkAuxSendValue* pEnvValues = pObj->GetGameDefinedAuxSends();
				pObj->Release();

				AkUInt32 uNumEnvRet = 0;
				for(int iAuxIdx=0; iAuxIdx < AK_MAX_AUX_PER_OBJ; ++iAuxIdx)
				{
					if( pEnvValues[iAuxIdx].auxBusID == AK_INVALID_AUX_ID )
						break;

					uNumEnvRet++;
				}

				if( io_ruNumEnvValues < uNumEnvRet )
					eReturn = AK_PartialSuccess;

				io_ruNumEnvValues = AkMin( io_ruNumEnvValues, uNumEnvRet );
				AkMemCpy( out_paEnvironmentValues, (void*)pEnvValues, io_ruNumEnvValues * sizeof(AkAuxSendValue) );
				return eReturn;
			}

			AKRESULT GetGameObjectDryLevelValue( 
				AkGameObjectID		in_GameObj,			///< the unique object ID
				AkReal32&			out_rfControlValue	///< control value for dry level
				)
			{
				CAkFunctionCritical SetSpaceAsCritical; //use global lock
				CAkRegisteredObj * pObj = g_pRegistryMgr->GetObjAndAddref( in_GameObj );
				if( !pObj )
					return AK_IDNotFound;

				out_rfControlValue = pObj->GetDryLevelValue();
				pObj->Release();

				return AK_Success;
			}

			AKRESULT GetObjectObstructionAndOcclusion(  
				AkGameObjectID in_ObjectID,				///< Game object ID.
				AkUInt32 in_uListener,					///< Listener ID.
				AkReal32& out_rfObstructionLevel,		///< ObstructionLevel : [0.0f..1.0f]
				AkReal32& out_rfOcclusionLevel			///< OcclusionLevel   : [0.0f..1.0f]
				)
			{
				CAkFunctionCritical SetSpaceAsCritical; //use global lock
				CAkRegisteredObj * pObj = g_pRegistryMgr->GetObjAndAddref( in_ObjectID );
				if( !pObj )
					return AK_IDNotFound;

				out_rfObstructionLevel = pObj->GetObjectObstructionValue( in_uListener );
				out_rfOcclusionLevel = pObj->GetObjectOcclusionValue( in_uListener );
				pObj->Release();

				return AK_Success;
			}

			// Advanced functions

			AKRESULT QueryAudioObjectIDs(
				AkUniqueID in_eventID,
				AkUInt32& io_ruNumItems,
				AkObjectInfo* out_aObjectInfos 
				)
			{
				if( io_ruNumItems && !out_aObjectInfos )
					return AK_InvalidParameter;

				CAkFunctionCritical SetSpaceAsCritical; //use global lock
				CAkEvent* pEvent = g_pIndex->m_idxEvents.GetPtrAndAddRef( in_eventID );
				if( !pEvent )
					return AK_IDNotFound;

				AKRESULT eResult = pEvent->QueryAudioObjectIDs( io_ruNumItems, out_aObjectInfos );
				pEvent->Release();
				return eResult;
			}

#ifdef AK_SUPPORT_WCHAR
			AKRESULT QueryAudioObjectIDs(
				const wchar_t* in_pszEventName,
				AkUInt32& io_ruNumItems,
				AkObjectInfo* out_aObjectInfos 
				)
			{
				AkUniqueID eventID = AK::SoundEngine::GetIDFromString( in_pszEventName );
				if ( eventID == AK_INVALID_UNIQUE_ID )
					return AK_IDNotFound;

				return QueryAudioObjectIDs( eventID, io_ruNumItems, out_aObjectInfos );
			}
#endif //AK_SUPPORT_WCHAR

			AKRESULT QueryAudioObjectIDs(
				const char* in_pszEventName,
				AkUInt32& io_ruNumItems,
				AkObjectInfo* out_aObjectInfos 
				)
			{
				AkUniqueID eventID = AK::SoundEngine::GetIDFromString( in_pszEventName );
				if ( eventID == AK_INVALID_UNIQUE_ID )
					return AK_IDNotFound;

				return QueryAudioObjectIDs( eventID, io_ruNumItems, out_aObjectInfos );
			}

			AKRESULT GetPositioningInfo( 
				AkUniqueID in_ObjectID,
				AkPositioningInfo& out_rPositioningInfo 
				)
			{
				CAkFunctionCritical SetSpaceAsCritical; //use global lock
				CAkParameterNodeBase * pObj = g_pIndex->GetNodePtrAndAddRef( in_ObjectID, AkNodeType_Default );
				if( !pObj )
					return AK_IDNotFound;

				CAkParameterNode* pParam = (CAkParameterNode *)( pObj );
				AKRESULT eResult = pParam->GetStatic3DParams( out_rPositioningInfo );
				pObj->Release();

				return eResult;
			}

			AKRESULT GetActiveGameObjects( 
				AkGameObjectsList& io_GameObjectList
				)
			{
				CAkFunctionCritical SetSpaceAsCritical; //use global lock
				return g_pRegistryMgr->GetActiveGameObjects( io_GameObjectList );
			}

			bool GetIsGameObjectActive(
				AkGameObjectID in_GameObjId
				)
			{
				CAkFunctionCritical SetSpaceAsCritical; //use global lock
				return g_pRegistryMgr->IsGameObjectActive( in_GameObjId );
			}

			AKRESULT GetMaxRadius(
				AkRadiusList & io_RadiusList
				)
			{
				CAkFunctionCritical SetSpaceAsCritical; //use global lock
				return CAkURenderer::GetMaxRadius( io_RadiusList );
			}

			AkReal32 GetMaxRadius(
				AkGameObjectID in_GameObjId
				)
			{
				CAkFunctionCritical SetSpaceAsCritical; //use global lock
				return CAkURenderer::GetMaxRadius( in_GameObjId );
			}

			AkUniqueID GetEventIDFromPlayingID( AkPlayingID in_playingID )
			{
				// Global Lock not required here, only acceding Playing manager which has its own lock.
				//CAkFunctionCritical SetSpaceAsCritical; //use global lock

				/// \return AK_INVALID_UNIQUE_ID on failure.
				if( g_pPlayingMgr )
					return g_pPlayingMgr->GetEventIDFromPlayingID( in_playingID );
				return AK_INVALID_UNIQUE_ID;
			}

			AkGameObjectID GetGameObjectFromPlayingID( AkPlayingID in_playingID )
			{
				// Global Lock not required here, only acceding Playing manager which has its own lock.
				//CAkFunctionCritical SetSpaceAsCritical; //use global lock

				/// \return AK_INVALID_GAME_OBJECT on failure.
				if( g_pPlayingMgr )
					return g_pPlayingMgr->GetGameObjectFromPlayingID( in_playingID );
				return AK_INVALID_GAME_OBJECT;
			}

			AKRESULT GetPlayingIDsFromGameObject(
				AkGameObjectID in_GameObjId,	
				AkUInt32& io_ruNumIDs,
				AkPlayingID* out_aPlayingIDs
				)
			{
				// Global Lock not required here, only acceding Playing manager which has its own lock.
				//CAkFunctionCritical SetSpaceAsCritical; //use global lock

				/// \aknote It is possible to call GetPlayingIDsFromGameObject with io_ruNumItems = 0 to get the total size of the
				/// structure that should be allocated for out_aPlayingIDs. \endaknote
				/// \return AK_Success if succeeded, AK_InvalidParameter if out_aPlayingIDs is NULL while io_ruNumItems > 0
				if( g_pPlayingMgr )
					return g_pPlayingMgr->GetPlayingIDsFromGameObject( in_GameObjId, io_ruNumIDs, out_aPlayingIDs );
				return AK_Fail;
			}
			
			AKRESULT GetCustomPropertyValue(
				AkUniqueID in_ObjectID,		///< Object ID
				AkUInt32 in_uPropID,			///< Property ID
				AkInt32& out_iValue				///< Property Value
				)
			{
				CAkFunctionCritical SetSpaceAsCritical; //use global lock

				CAkParameterNodeBase * pObj = g_pIndex->GetNodePtrAndAddRef( in_ObjectID, AkNodeType_Default );
				if( !pObj )
					return AK_IDNotFound;

				AkPropValue * pValue = pObj->FindCustomProp( in_uPropID );
				if ( pValue )
				{
					out_iValue = pValue->iValue;
					pObj->Release();
					return AK_Success;
				}
				else
				{
					pObj->Release();
					return AK_PartialSuccess;
				}
			}

			AKRESULT GetCustomPropertyValue(
				AkUniqueID in_ObjectID,		///< Object ID
				AkUInt32 in_uPropID,			///< Property ID
				AkReal32& out_fValue			///< Property Value
				)
			{
				CAkFunctionCritical SetSpaceAsCritical; //use global lock

				CAkParameterNodeBase * pObj = g_pIndex->GetNodePtrAndAddRef( in_ObjectID, AkNodeType_Default );
				if( !pObj )
					return AK_IDNotFound;

				AkPropValue * pValue = pObj->FindCustomProp( in_uPropID );
				if ( pValue )
				{
					out_fValue = pValue->fValue;
					pObj->Release();
					return AK_Success;
				}
				else
				{
					pObj->Release();
					return AK_PartialSuccess;
				}
			}

		} // namespace Query
	} // namespace SoundEngine
} // namespace AK
