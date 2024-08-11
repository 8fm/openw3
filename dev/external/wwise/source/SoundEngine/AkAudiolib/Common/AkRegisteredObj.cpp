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
// AkRegisteredObj.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkRegisteredObj.h"
#include "AkDefault3DParams.h"
#include "AkParameterNodeBase.h"
#include "AkPBI.h"
#include "AkAudioLibIndex.h"
#include "AkRTPCMgr.h"

#if defined AK_WII_FAMILY
#include "AkWiimoteMgr.h"
#endif

CAkRegisteredObj::CAkRegisteredObj( AkGameObjectID in_GameObjID )
	: m_pListModifiedNodes( NULL )
	, m_fDryLevelValue( 1.0f )
	, m_fScalingFactor( 1.0f )
	, m_GameObjID( in_GameObjID )
	, m_refCount( 1 )
	, m_bPositionDirty( true )
	, m_bRegistered( true )
#if defined AK_WII_FAMILY
	,m_PlayCount( 0 )
#endif
{
#if defined AK_WII_FAMILY
	m_PosKeep.SetControllerActiveMask( AK_WII_MAIN_AUDIO_OUTPUT );
#endif
	for( int j = 0; j < AK_MAX_AUX_PER_OBJ; ++j )
	{
		m_EnvironmentValues[j].auxBusID = AK_INVALID_AUX_ID;
		m_EnvironmentValues[j].fControlValue = 0.0f;
	}

	memset( m_fObstructionValue, 0, sizeof( AkPackedZeroToOneValue ) * AK_NUM_LISTENERS );
	memset( m_fOcclusionValue, 0, sizeof( AkPackedZeroToOneValue ) * AK_NUM_LISTENERS );
}

CAkRegisteredObj::~CAkRegisteredObj()
{
#if defined AK_WII_FAMILY
	AKASSERT( m_PlayCount == 0 );
#endif

	FreeModifiedNodes();

	DestroyModifiedNodeList();

	m_SwitchHist.Term();

	m_arCachedEmitListPairs.Term();
}

bool CAkRegisteredObj::CreateModifiedNodeList()
{
	if( !m_pListModifiedNodes )
	{
		AkNew2( m_pListModifiedNodes, g_DefaultPoolId, AkListNode, AkListNode() );
		if( m_pListModifiedNodes )
		{
			AKRESULT eResult = m_pListModifiedNodes->Init( 0, AK_NO_MAX_LIST_SIZE );
			if( eResult != AK_Success )
			{
				DestroyModifiedNodeList();
			}
		}
	}
	return m_pListModifiedNodes != NULL;
}

void CAkRegisteredObj::DestroyModifiedNodeList()
{
	if( m_pListModifiedNodes )
	{
		m_pListModifiedNodes->Term();
		AkDelete( g_DefaultPoolId, m_pListModifiedNodes );
		m_pListModifiedNodes = NULL;
	}
}

AKRESULT CAkRegisteredObj::SetNodeAsModified(CAkParameterNodeBase* in_pNode)
{
	AKRESULT eResult = AK_Success;
	AKASSERT(in_pNode);

	WwiseObjectID wwiseId( in_pNode->ID(), in_pNode->IsBusCategory() );

	if( !CreateModifiedNodeList() )
	{
		eResult = AK_Fail;
	}
	else if( m_pListModifiedNodes->Exists( wwiseId ) )
	{
		eResult = AK_Success;// technically useless since it was already initialized to AK_Success, but I let it there to keep the code clean.
	}
	else if( m_pListModifiedNodes->AddLast( wwiseId ) == NULL )
	{
		eResult =  AK_Fail;
	}

	return eResult;

}

void CAkRegisteredObj::FreeModifiedNodes()
{
	if( m_pListModifiedNodes )
	{
		for( AkListNode::Iterator iter = m_pListModifiedNodes->Begin(); iter != m_pListModifiedNodes->End(); ++iter )
		{
			CAkParameterNodeBase* pNode = g_pIndex->GetNodePtrAndAddRef( (*iter) );
			if(pNode)
			{
				pNode->Unregister( this );
				pNode->Release();
			}
		}
	}
	g_pRTPCMgr->UnregisterGameObject( this );
}

void CAkRegisteredObj::SetPosition( 
	const AkSoundPosition* in_aPositions,
	AkUInt16 in_uNumPositions,
	AK::SoundEngine::MultiPositionType in_eMultiPositionType
	)
{
	m_PosKeep.SetPosition( in_aPositions, in_uNumPositions );
	m_PosKeep.SetMultiPositionType( in_eMultiPositionType );
	m_bPositionDirty = true;
}

void CAkRegisteredObj::SetActiveListeners( AkUInt32 in_uListenerMask )
{
	m_PosKeep.SetListenerMask( in_uListenerMask );
	m_bPositionDirty = true;
}

#if defined AK_WII
void CAkRegisteredObj::SetActiveControllers( AkUInt32 in_uActiveControllerMask )
{
	if( m_PlayCount != 0 )
	{
		AKASSERT( AK_WII_REMOTE_3 == 0x0008 );// to make sure nobody modified this define
		AkUInt32 Remotemask = AK_WII_REMOTE_0; //0x0001
		for( AkUInt32 uController = 0; uController < WPAD_MAX_CONTROLLERS; ++uController )
		{
			if( (Remotemask & in_uActiveControllerMask) && !(Remotemask & m_PosKeep.GetControllerActiveMask()) )
			{
				//Activating
				CAkWiimoteMgr::IncrementSpeakerActivityCount( uController );
			}
			else if( !(Remotemask & in_uActiveControllerMask) && (Remotemask & m_PosKeep.GetControllerActiveMask()) )
			{
				//Deactivating
				CAkWiimoteMgr::DecrementSpeakerActivityCount( uController );
			}
			Remotemask = Remotemask << 1;
		}
	}
	m_PosKeep.SetControllerActiveMask( in_uActiveControllerMask );
}
#endif

AKRESULT CAkRegisteredObj::SetGameObjectAuxSendValues( 
		AkAuxSendValue*	in_aEnvironmentValues,
		AkUInt32			in_uNumEnvValues
		)
{
	if( in_uNumEnvValues > AK_MAX_AUX_PER_OBJ )
		return AK_Fail;

	AkUInt32 uWriteIndex = 0;
	if( in_aEnvironmentValues )
	{
		for( AkUInt32 uReadIndex = 0; uReadIndex < in_uNumEnvValues; ++uReadIndex )
		{
			// Must Skip unused slots ( Determined by auxBusID == AK_INVALID_AUX_ID )
			// Skip environement that are set to 0% to avoid useless CPU.
			if( in_aEnvironmentValues[uReadIndex].auxBusID != AK_INVALID_AUX_ID && in_aEnvironmentValues[uReadIndex].fControlValue > 0.f )
			{
				m_EnvironmentValues[uWriteIndex] = in_aEnvironmentValues[uReadIndex];
				++uWriteIndex;
			}
		}
	}

	for( ; uWriteIndex < AK_MAX_AUX_PER_OBJ; ++uWriteIndex )
	{
		// Must wipe the remaining slots as they were maybe not empty at first.
		m_EnvironmentValues[uWriteIndex].auxBusID = AK_INVALID_AUX_ID;
		m_EnvironmentValues[uWriteIndex].fControlValue = 0.0f;
	}

	return AK_Success;
}

// Get number of emitter-listener pairs for this game object.
AkUInt32 CAkRegisteredObj::GetNumEmitterListenerPairs() const
{
	AkUInt32 uNumPosition = GetPosition().GetNumPosition();
	AkUInt32 uMask = GetListenerMask();
	return uNumPosition * AK::GetNumChannels( uMask );
}

// Push emitter-listener pairs computed by a PBI for this game object.
void CAkRegisteredObj::CacheEmitListenPairs( const AkVolumeDataArray & in_arEmitList )
{
	m_arCachedEmitListPairs.RemoveAll();

	m_bPositionDirty = true;

	// Cache only if registered. Unregistered objects are not notified when listeners move.
	if ( IsRegistered() )
	{
		AkUInt32 uNumItems = in_arEmitList.Length();
		if ( uNumItems > m_arCachedEmitListPairs.Reserved() )
		{
			if ( !m_arCachedEmitListPairs.GrowArray( uNumItems - m_arCachedEmitListPairs.Reserved() ) )
				return;
		}

		AkVolumeDataArray::Iterator it = in_arEmitList.Begin();
		while ( it != in_arEmitList.End() )
		{
			AkEmitterListenerPairEx * pPair = m_arCachedEmitListPairs.AddLast();
			AKASSERT( pPair );	// Was reserved.
			pPair->Copy(*it);
			++it;
		}
		// If we're here, caching was successful. Clear m_bPositionDirty.
		// However, it is more efficient not to pretend that position 
		// was cached if there are no rays.
		m_bPositionDirty = ( in_arEmitList.Length() == 0 );
	}
}

// Cache a single emitter-listener pair.
// Note: Caching will only work if there is one and only one emitter-listener pair in this game object;
// partial caching (of subset of pairs or mismatching features) is not supported.
void CAkRegisteredObj::CacheEmitListenPair( const AkEmitterListenerPairEx & in_emitListPair )
{
	if ( GetNumEmitterListenerPairs() == 1 
		&& IsRegistered() )
	{
		m_arCachedEmitListPairs.RemoveAll();
		AkEmitterListenerPairEx * pPair = m_arCachedEmitListPairs.AddLast();
		if ( !pPair )
			return;
		pPair->Copy( in_emitListPair );
		
		// Caching was successful. Clear m_bPositionDirty.
		m_bPositionDirty = false;
	}
	else
		m_bPositionDirty = true;
}

AKRESULT CAkRegisteredObj::SetObjectObstructionAndOcclusion(
		AkUInt32			in_uListener,
		AkReal32			in_fObstructionValue,
		AkReal32			in_fOcclusionValue
		)
{
	if(in_uListener >= AK_NUM_LISTENERS)
		return AK_Fail;

	m_fObstructionValue[in_uListener].Set( in_fObstructionValue );
	m_fOcclusionValue[in_uListener].Set( in_fOcclusionValue );

	return AK_Success;
}

#if defined AK_WII_FAMILY
void CAkRegisteredObj::IncrementGameObjectPlayCount()
{
	++m_PlayCount;
	AKASSERT( m_PlayCount ); // Should never be zero here.
	if( m_PlayCount == 1 )
	{
		AKASSERT( AK_WII_REMOTE_3 == 0x0008 );// to make sure nobody modified this define
		AkUInt32 Remotemask = AK_WII_REMOTE_0; //0x0001

		for( AkUInt32 uController = 0; uController < WPAD_MAX_CONTROLLERS; ++uController )
		{
			if( (Remotemask & m_PosKeep.GetControllerActiveMask()) )
			{
				//Activating
				CAkWiimoteMgr::IncrementSpeakerActivityCount( uController );
			}
			Remotemask = Remotemask << 1;
		}
	}
}

void CAkRegisteredObj::DecrementGameObjectPlayCount()
{
	--m_PlayCount;
	if( m_PlayCount == 0 )
	{
		AKASSERT( AK_WII_REMOTE_3 == 0x0008 );// to make sure nobody modified this define
		AkUInt32 Remotemask = AK_WII_REMOTE_0; //0x0001

		for( AkUInt32 uController = 0; uController < WPAD_MAX_CONTROLLERS; ++uController )
		{
			if( (Remotemask & m_PosKeep.GetControllerActiveMask()) )
			{
				//Deactivating
				CAkWiimoteMgr::DecrementSpeakerActivityCount( uController );
			}
			Remotemask = Remotemask << 1;
		}
	}
}
#endif

