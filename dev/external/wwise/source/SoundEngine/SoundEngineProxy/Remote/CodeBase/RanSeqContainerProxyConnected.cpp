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

#include "stdafx.h"
#ifndef AK_OPTIMIZED

#include "RanSeqContainerProxyConnected.h"
#include "AkRanSeqCntr.h"
#include "CommandData.h"
#include "CommandDataSerializer.h"

RanSeqContainerProxyConnected::RanSeqContainerProxyConnected( AkUniqueID in_id )
{
	CAkIndexable* pIndexable = AK::SoundEngine::GetIndexable( in_id, AkIdxType_AudioNode );
	if ( !pIndexable )
		pIndexable = CAkRanSeqCntr::Create( in_id );

	SetIndexable( pIndexable );
}

RanSeqContainerProxyConnected::~RanSeqContainerProxyConnected()
{
}

void RanSeqContainerProxyConnected::HandleExecute( AkUInt16 in_uMethodID, CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer )
{
	CAkRanSeqCntr * pCntr = static_cast<CAkRanSeqCntr *>( GetIndexable() );

	switch( in_uMethodID )
	{
	case IRanSeqContainerProxy::MethodMode:
		{
			RanSeqContainerProxyCommandData::Mode mode;
			if( in_rSerializer.Get( mode ) )
					pCntr->Mode( (AkContainerMode) mode.m_param1 );
			break;
		}

	case IRanSeqContainerProxy::MethodIsGlobal:
		{
			RanSeqContainerProxyCommandData::IsGlobal isGlobal;
			if( in_rSerializer.Get( isGlobal ) )
					pCntr->IsGlobal( isGlobal.m_param1 );
			break;
		}

	case IRanSeqContainerProxy::MethodSetPlaylist:
		{
			RanSeqContainerProxyCommandData::SetPlaylist setPlaylist;
			if ( in_rSerializer.Get( setPlaylist ) )
	            pCntr->SetPlaylist( setPlaylist.m_pvListBlock, setPlaylist.m_ulParamBlockSize );

			break;
		}

	case IRanSeqContainerProxy::MethodResetPlayListAtEachPlay:
		{
			RanSeqContainerProxyCommandData::ResetPlayListAtEachPlay resetPlayListAtEachPlay;
			if( in_rSerializer.Get( resetPlayListAtEachPlay ) )
					pCntr->ResetPlayListAtEachPlay( resetPlayListAtEachPlay.m_param1 );
			break;
		}

	case IRanSeqContainerProxy::MethodRestartBackward:
		{
			RanSeqContainerProxyCommandData::RestartBackward restartBackward;
			if( in_rSerializer.Get( restartBackward ) )
					pCntr->RestartBackward( restartBackward.m_param1 );
			break;
		}

	case IRanSeqContainerProxy::MethodContinuous:
		{
			RanSeqContainerProxyCommandData::Continuous continuous;
			if( in_rSerializer.Get( continuous ) )
					pCntr->Continuous( continuous.m_param1 );
			break;
		}

	case IRanSeqContainerProxy::MethodForceNextToPlay:
		{
			RanSeqContainerProxyCommandData::ForceNextToPlay forceNextToPlay;
			if( in_rSerializer.Get( forceNextToPlay ) )
			{
				CAkRegisteredObj * pGameObj = g_pRegistryMgr->GetObjAndAddref( (AkGameObjectID)forceNextToPlay.m_param2 );
				if ( pGameObj )
				{
					pCntr->ForceNextToPlay( forceNextToPlay.m_param1, pGameObj, forceNextToPlay.m_param3 );
					pGameObj->Release();
				}
			}
			break;
		}

	case IRanSeqContainerProxy::MethodNextToPlay:
		{
			RanSeqContainerProxyCommandData::NextToPlay nextToPlay;
			if( in_rSerializer.Get( nextToPlay ) )
			{
				CAkRegisteredObj * pGameObj = g_pRegistryMgr->GetObjAndAddref( (AkGameObjectID)nextToPlay.m_gameObjPtr );
				if ( pGameObj )
				{
					out_rReturnSerializer.Put( pCntr->NextToPlay( pGameObj ) );
					pGameObj->Release();
				}
			}

			break;
		}

	case IRanSeqContainerProxy::MethodRandomMode:
		{
			RanSeqContainerProxyCommandData::RandomMode randomMode;
			if( in_rSerializer.Get( randomMode ) )
					pCntr->RandomMode( (AkRandomMode) randomMode.m_param1 );
			break;
		}

	case IRanSeqContainerProxy::MethodAvoidRepeatingCount:
		{
			RanSeqContainerProxyCommandData::AvoidRepeatingCount avoidRepeatingCount;
			if( in_rSerializer.Get( avoidRepeatingCount ) )
					pCntr->AvoidRepeatingCount( avoidRepeatingCount.m_param1 );
			break;
		}

	case IRanSeqContainerProxy::MethodSetItemWeight_withID:
		{
			RanSeqContainerProxyCommandData::SetItemWeight_withID setItemWeight_withID;
			if( in_rSerializer.Get( setItemWeight_withID ) )
					pCntr->_SetItemWeight( setItemWeight_withID.m_param1, setItemWeight_withID.m_param2 );
			break;
		}

	case IRanSeqContainerProxy::MethodSetItemWeight_withPosition:
		{
			RanSeqContainerProxyCommandData::SetItemWeight_withPosition setItemWeight_withPosition;
			if( in_rSerializer.Get( setItemWeight_withPosition ) )
					pCntr->SetItemWeight( setItemWeight_withPosition.m_param1, setItemWeight_withPosition.m_param2 );
			break;
		}

	case IRanSeqContainerProxy::MethodLoop:
		{
			RanSeqContainerProxyCommandData::Loop loop;
			if( in_rSerializer.Get( loop ) )
					pCntr->Loop( loop.m_param1, loop.m_param2, loop.m_param3, loop.m_param4, loop.m_param5 );
			break;
		}

	case IRanSeqContainerProxy::MethodTransitionMode:
		{
			RanSeqContainerProxyCommandData::TransitionMode transitionMode;
			if( in_rSerializer.Get( transitionMode ) )
					pCntr->TransitionMode( (AkTransitionMode) transitionMode.m_param1 );
			break;
		}

	case IRanSeqContainerProxy::MethodTransitionTime:
		{
			RanSeqContainerProxyCommandData::TransitionTime transitionTime;
			if( in_rSerializer.Get( transitionTime ) )
					pCntr->TransitionTime( (AkReal32) transitionTime.m_param1, (AkReal32) transitionTime.m_param2, (AkReal32) transitionTime.m_param3 );
			break;
		}

	default:
		__base::HandleExecute( in_uMethodID, in_rSerializer, out_rReturnSerializer );
	}
}
#endif // #ifndef AK_OPTIMIZED
