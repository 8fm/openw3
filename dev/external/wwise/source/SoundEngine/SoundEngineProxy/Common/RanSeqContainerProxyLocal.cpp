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
#ifndef PROXYCENTRAL_CONNECTED

#include "RanSeqContainerProxyLocal.h"

#include "AkRanSeqCntr.h"
#include "AkAudioLib.h"
#include "AkRegistryMgr.h"
#include "AkCritical.h"


RanSeqContainerProxyLocal::RanSeqContainerProxyLocal( AkUniqueID in_id )
{
	CAkIndexable* pIndexable = AK::SoundEngine::GetIndexable( in_id, AkIdxType_AudioNode );

	SetIndexable( pIndexable != NULL ? pIndexable : CAkRanSeqCntr::Create( in_id ) );
}

RanSeqContainerProxyLocal::~RanSeqContainerProxyLocal()
{
}

void RanSeqContainerProxyLocal::Mode( AkContainerMode in_eMode	)
{
	CAkRanSeqCntr* pIndexable = static_cast<CAkRanSeqCntr*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		pIndexable->Mode( in_eMode );
	}
}

void RanSeqContainerProxyLocal::IsGlobal( bool in_bIsGlobal )
{
	CAkRanSeqCntr* pIndexable = static_cast<CAkRanSeqCntr*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		pIndexable->IsGlobal( in_bIsGlobal );
	}
}

void RanSeqContainerProxyLocal::SetPlaylist( void* in_pvListBlock, AkUInt32 in_ulParamBlockSize )
{
	CAkRanSeqCntr* pIndexable = static_cast<CAkRanSeqCntr*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		pIndexable->SetPlaylist( in_pvListBlock, in_ulParamBlockSize );
	}
}

void RanSeqContainerProxyLocal::ResetPlayListAtEachPlay( bool in_bResetPlayListAtEachPlay )
{
	CAkRanSeqCntr* pIndexable = static_cast<CAkRanSeqCntr*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		pIndexable->ResetPlayListAtEachPlay( in_bResetPlayListAtEachPlay );
	}
}

void RanSeqContainerProxyLocal::RestartBackward( bool in_bRestartBackward )
{
	CAkRanSeqCntr* pIndexable = static_cast<CAkRanSeqCntr*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		pIndexable->RestartBackward( in_bRestartBackward );
	}
}

void RanSeqContainerProxyLocal::Continuous( bool in_bIsContinuous )
{
	CAkRanSeqCntr* pIndexable = static_cast<CAkRanSeqCntr*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		pIndexable->Continuous( in_bIsContinuous );
	}
}

void RanSeqContainerProxyLocal::ForceNextToPlay( AkInt16 in_position, AkWwiseGameObjectID in_gameObjPtr, AkPlayingID in_playingID )
{
	CAkRanSeqCntr* pIndexable = static_cast<CAkRanSeqCntr*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical; // Need to protect object registry

		CAkRegisteredObj * pGameObj = g_pRegistryMgr->GetObjAndAddref( (AkGameObjectID)in_gameObjPtr );
		if ( pGameObj )
		{
			pIndexable->ForceNextToPlay( in_position, pGameObj, in_playingID );
			pGameObj->Release();
		}
	}
}

AkInt16 RanSeqContainerProxyLocal::NextToPlay( AkWwiseGameObjectID in_gameObjPtr )
{
	CAkRanSeqCntr* pIndexable = static_cast<CAkRanSeqCntr*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical; // Need to protect object registry

		CAkRegisteredObj * pGameObj = g_pRegistryMgr->GetObjAndAddref( (AkGameObjectID)in_gameObjPtr );
		if ( pGameObj )
		{
			AkInt16 iNext = pIndexable->NextToPlay( pGameObj );
			pGameObj->Release();
			return iNext;
		}
	}
	return 0;
}

void RanSeqContainerProxyLocal::RandomMode( AkRandomMode in_eRandomMode )
{
	CAkRanSeqCntr* pIndexable = static_cast<CAkRanSeqCntr*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		pIndexable->RandomMode( in_eRandomMode );
	}
}

void RanSeqContainerProxyLocal::AvoidRepeatingCount( AkUInt16 in_count )
{
	CAkRanSeqCntr* pIndexable = static_cast<CAkRanSeqCntr*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		pIndexable->AvoidRepeatingCount( in_count );
	}
}

void RanSeqContainerProxyLocal::SetItemWeight( AkUniqueID in_itemID, AkUInt32 in_weight )
{
	CAkRanSeqCntr* pIndexable = static_cast<CAkRanSeqCntr*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		pIndexable->_SetItemWeight( in_itemID, in_weight );
	}
}

void RanSeqContainerProxyLocal::SetItemWeight( AkUInt16 in_position, AkUInt32 in_weight )
{
	CAkRanSeqCntr* pIndexable = static_cast<CAkRanSeqCntr*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		pIndexable->SetItemWeight( in_position, in_weight );
	}
}

void RanSeqContainerProxyLocal::Loop( bool in_bIsLoopEnabled, bool in_bIsLoopInfinite, AkInt16 in_loopCount, AkInt16 in_loopModMin, AkInt16 in_loopModMax )
{
	CAkRanSeqCntr* pIndexable = static_cast<CAkRanSeqCntr*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		pIndexable->Loop( in_bIsLoopEnabled, in_bIsLoopInfinite, in_loopCount, in_loopModMin, in_loopModMax );
	}
}

void RanSeqContainerProxyLocal::TransitionMode( AkTransitionMode in_eTransitionMode )
{
	CAkRanSeqCntr* pIndexable = static_cast<CAkRanSeqCntr*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		pIndexable->TransitionMode( in_eTransitionMode );
	}
}

void RanSeqContainerProxyLocal::TransitionTime( AkTimeMs in_transitionTime, AkTimeMs in_rangeMin, AkTimeMs in_rangeMax )
{
	CAkRanSeqCntr* pIndexable = static_cast<CAkRanSeqCntr*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		pIndexable->TransitionTime( (AkReal32) in_transitionTime, (AkReal32) in_rangeMin, (AkReal32)  in_rangeMax );
	}
}
#endif
#endif // #ifndef AK_OPTIMIZED
