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
// AkSoundBase.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkMonitor.h"
#include "AkSoundBase.h"
#include "AkModifiers.h"
#include "AkURenderer.h"
#include "AkAudioLibIndex.h"
#include "ActivityChunk.h"

CAkSoundBase::CAkSoundBase( 
    AkUniqueID in_ulID
    )
:CAkParameterNode(in_ulID)
{
}

CAkSoundBase::~CAkSoundBase()
{
}

AKRESULT CAkSoundBase::PlayToEnd( CAkRegisteredObj * in_pGameObj, CAkParameterNodeBase* in_pNodePtr, AkPlayingID in_PlayingID )
{	
	if( IsActivityChunkEnabled() )
	{
		AkActivityChunk::AkListLightCtxs& rListPBI = m_pActivityChunk->m_listPBI;
		for( AkActivityChunk::AkListLightCtxs::Iterator iter = rListPBI.Begin(); iter != rListPBI.End(); ++iter )
		{
			CAkPBI * l_pPBI = *iter;
			if( ( !in_pGameObj || l_pPBI->GetGameObjectPtr() == in_pGameObj ) &&
				( in_PlayingID == AK_INVALID_PLAYING_ID || in_PlayingID == l_pPBI->GetPlayingID() ) )
			{
				l_pPBI->PlayToEnd( in_pNodePtr );
			}
		}
	}
	return AK_Success;
}

void CAkSoundBase::ParamNotification( NotifParams& in_rParams )
{
	CAkPBI* l_pPBI    = NULL;

	if( IsActivityChunkEnabled() )
	{
		AkActivityChunk::AkListLightCtxs& rListPBI = m_pActivityChunk->m_listPBI;
		for( AkActivityChunk::AkListLightCtxs::Iterator iter = rListPBI.Begin(); iter != rListPBI.End(); ++iter )
		{
			l_pPBI = *iter;
			AKASSERT( l_pPBI != NULL );

			if( in_rParams.pExceptObjects == NULL || in_rParams.pGameObj != NULL )
			{
				if(	(in_rParams.pGameObj == NULL) 
					|| l_pPBI->GetGameObjectPtr() == in_rParams.pGameObj )
				{
					// Update Behavioural side.
					l_pPBI->ParamNotification( in_rParams );
				}
			}
			else
			{
				GameObjExceptArray* l_pExceptArray = static_cast<GameObjExceptArray*>( in_rParams.pExceptObjects );
				bool l_bIsException = false;
				for( GameObjExceptArray::Iterator itGameObj = l_pExceptArray->Begin(); itGameObj != l_pExceptArray->End(); ++itGameObj )
				{
					if( *(itGameObj.pItem) == l_pPBI->GetGameObjectPtr() )
					{
						l_bIsException = true;
						break;
					}
				}
				if( !l_bIsException )
				{
					l_pPBI->ParamNotification( in_rParams );
				}
			}
		}
	}
}

void CAkSoundBase::MuteNotification(AkReal32 in_fMuteRatio, AkMutedMapItem& in_rMutedItem, bool /*in_bIsFromBus = false*/ )
{
	if( IsActivityChunkEnabled() )
	{
		AkActivityChunk::AkListLightCtxs& rListPBI = m_pActivityChunk->m_listPBI;
		for( AkActivityChunk::AkListLightCtxs::Iterator iter = rListPBI.Begin(); iter != rListPBI.End(); ++iter )
		{
			(*iter)->MuteNotification( in_fMuteRatio, in_rMutedItem, false );
		}
	}
}

void CAkSoundBase::MuteNotification( AkReal32 in_fMuteRatio, CAkRegisteredObj * in_pGameObj, AkMutedMapItem& in_rMutedItem, bool in_bPrioritizeGameObjectSpecificItems /* = false */ )
{
	const bool bForceNotify = !in_pGameObj || ( in_bPrioritizeGameObjectSpecificItems && ! in_pGameObj );

	if( IsActivityChunkEnabled() )
	{
		AkActivityChunk::AkListLightCtxs& rListPBI = m_pActivityChunk->m_listPBI;
		for( AkActivityChunk::AkListLightCtxs::Iterator iter = rListPBI.Begin(); iter != rListPBI.End(); ++iter )
		{
			CAkPBI * l_pPBI = *iter;

			if( bForceNotify || l_pPBI->GetGameObjectPtr() == in_pGameObj )
			{
				// Update Behavioural side.
				l_pPBI->MuteNotification( in_fMuteRatio, in_rMutedItem, in_bPrioritizeGameObjectSpecificItems );
			}
		}
	}
}

void CAkSoundBase::ForAllPBI( 
	AkForAllPBIFunc in_funcForAll,
	CAkRegisteredObj * in_pGameObj,
	void * in_pCookie )
{
	if( IsActivityChunkEnabled() )
	{
		AkActivityChunk::AkListLightCtxs& rListPBI = m_pActivityChunk->m_listPBI;
		for( AkActivityChunk::AkListLightCtxs::Iterator iter = rListPBI.Begin(); iter != rListPBI.End(); ++iter )
		{
			if ( !in_pGameObj || ( in_pGameObj == (*iter)->GetGameObjectPtr() ) )
				in_funcForAll( *iter, in_pGameObj, in_pCookie );
		}
	}
}

// Notify the children PBI that a change int the Positioning parameters occured from RTPC
void CAkSoundBase::PropagatePositioningNotification( AkReal32 in_RTPCValue, AkRTPC_ParameterID in_ParameterID, CAkRegisteredObj * in_pGameObj, void*	in_pExceptArray )
{
	if( IsActivityChunkEnabled() )
	{
		AkActivityChunk::AkListLightCtxs& rListPBI = m_pActivityChunk->m_listPBI;
		for( AkActivityChunk::AkListLightCtxs::Iterator iter = rListPBI.Begin(); iter != rListPBI.End(); ++iter )
		{
			CAkPBI * l_pPBI = *iter;

			if( in_pExceptArray == NULL || in_pGameObj != NULL )
			{
				if(	( in_pGameObj == NULL ) 
					|| l_pPBI->GetGameObjectPtr() == in_pGameObj )
				{
					// Update Behavioural side.
					l_pPBI->PositioningChangeNotification( in_RTPCValue, in_ParameterID );
				}
			}
			else
			{
				GameObjExceptArray* l_pExceptArray = static_cast<GameObjExceptArray*>( in_pExceptArray );
				bool l_bIsException = false;
				for( GameObjExceptArray::Iterator itGameObj = l_pExceptArray->Begin(); itGameObj != l_pExceptArray->End(); ++itGameObj )
				{
					if( *(itGameObj.pItem) == l_pPBI->GetGameObjectPtr() )
					{
						l_bIsException = true;
						break;
					}
				}
				if( !l_bIsException )
				{
					l_pPBI->PositioningChangeNotification( in_RTPCValue, in_ParameterID );
				}
			}
		}
	}
}

void CAkSoundBase::RecalcNotification()
{
	if( IsActivityChunkEnabled() )
	{
		AkActivityChunk::AkListLightCtxs& rListPBI = m_pActivityChunk->m_listPBI;
		for( AkActivityChunk::AkListLightCtxs::Iterator iter = rListPBI.Begin(); iter != rListPBI.End(); ++iter )
		{
			(*iter)->RecalcNotification();
		}
	}
}

void CAkSoundBase::NotifyBypass(
	AkUInt32 in_bitsFXBypass,
	AkUInt32 in_uTargetMask,
	CAkRegisteredObj * in_pGameObj,
	void* in_pExceptArray /* = NULL */ )
{
	if( IsActivityChunkEnabled() )
	{
		AkActivityChunk::AkListLightCtxs& rListPBI = m_pActivityChunk->m_listPBI;
		for( AkActivityChunk::AkListLightCtxs::Iterator iter = rListPBI.Begin(); iter != rListPBI.End(); ++iter )
		{
			CAkPBI* l_pPBI = *iter;

			if( in_pExceptArray == NULL || in_pGameObj != NULL )
			{
				if(	( in_pGameObj == NULL ) 
					|| l_pPBI->GetGameObjectPtr() == in_pGameObj )
				{
					// Update Behavioural side.
					l_pPBI->NotifyBypass( in_bitsFXBypass, in_uTargetMask );
				}
			}
			else
			{
				GameObjExceptArray* l_pExceptArray = static_cast<GameObjExceptArray*>( in_pExceptArray );
				bool l_bIsException = false;
				for( GameObjExceptArray::Iterator itGameObj = l_pExceptArray->Begin(); itGameObj != l_pExceptArray->End(); ++itGameObj )
				{
					if( *(itGameObj.pItem) == l_pPBI->GetGameObjectPtr() )
					{
						l_bIsException = true;
						break;
					}
				}
				if( !l_bIsException )
				{
					l_pPBI->NotifyBypass( in_bitsFXBypass, in_uTargetMask );
				}
			}
		}
	}
}

void CAkSoundBase::UpdateFx(
		AkUInt32	   	in_uFXIndex
		)
{
	if( IsActivityChunkEnabled() )
	{
		AkActivityChunk::AkListLightCtxs& rListPBI = m_pActivityChunk->m_listPBI;
		for( AkActivityChunk::AkListLightCtxs::Iterator iter = rListPBI.Begin(); iter != rListPBI.End(); ++iter )
		{
			(*iter)->UpdateFx( in_uFXIndex );
		}
	}
}

#ifndef AK_OPTIMIZED

void CAkSoundBase::InvalidatePaths()
{
	if( IsActivityChunkEnabled() )
	{
		AkActivityChunk::AkListLightCtxs& rListPBI = m_pActivityChunk->m_listPBI;
		for( AkActivityChunk::AkListLightCtxs::Iterator iter = rListPBI.Begin(); iter != rListPBI.End(); ++iter )
		{
			(*iter)->InvalidatePaths();
		}
	}
}

#endif

AkInt16 CAkSoundBase::Loop()
{
	AkInt32 iLoop = m_props.GetAkProp( AkPropID_Loop, (AkInt32) AkLoopVal_NotLooping ).iValue;
	ApplyRange<AkInt32>( AkPropID_Loop, iLoop );
	return (AkInt16) iLoop;
}

AkReal32 CAkSoundBase::LoopStartOffset() const
{
	AkReal32 fLoopStart = m_props.GetAkProp( AkPropID_LoopStart, 0.0f ).fValue;
	return fLoopStart;
}

AkReal32 CAkSoundBase::LoopEndOffset() const
{
	AkReal32 fLoopEnd = m_props.GetAkProp( AkPropID_LoopEnd, 0.0f ).fValue;
	return fLoopEnd;
}

AkReal32 CAkSoundBase::LoopCrossfadeDuration() const
{
	AkReal32 fLoopCrossfadeDuration = m_props.GetAkProp( AkPropID_LoopCrossfadeDuration, 0.0f ).fValue;
	return fLoopCrossfadeDuration;
}

void CAkSoundBase::LoopCrossfadeCurveShape( AkCurveInterpolation& out_eCrossfadeUpType,  AkCurveInterpolation& out_eCrossfadeDownType) const
{
	out_eCrossfadeUpType = (AkCurveInterpolation)m_props.GetAkProp( AkPropID_CrossfadeUpCurve, (AkInt32)AkCurveInterpolation_Sine ).iValue;
	out_eCrossfadeDownType = (AkCurveInterpolation)m_props.GetAkProp( AkPropID_CrossfadeDownCurve, (AkInt32)AkCurveInterpolation_SineRecip ).iValue;
}

void CAkSoundBase::GetTrim( AkReal32& out_fBeginTrimOffsetSec, AkReal32& out_fEndTrimOffsetSec ) const
{
	out_fBeginTrimOffsetSec = m_props.GetAkProp( AkPropID_TrimInTime, 0.0f ).fValue;
	out_fEndTrimOffsetSec = m_props.GetAkProp( AkPropID_TrimOutTime, 0.0f ).fValue;
}

void CAkSoundBase::GetFade(	AkReal32& out_fBeginFadeOffsetSec, AkCurveInterpolation& out_eBeginFadeCurveType, 
				AkReal32& out_fEndFadeOffsetSec, AkCurveInterpolation& out_eEndFadeCurveType  ) const
{
	out_fBeginFadeOffsetSec = m_props.GetAkProp( AkPropID_FadeInTime, 0.0f ).fValue;
	out_fEndFadeOffsetSec = m_props.GetAkProp( AkPropID_FadeOutTime, 0.0f ).fValue;

	out_eBeginFadeCurveType = (AkCurveInterpolation)m_props.GetAkProp( AkPropID_FadeInCurve, (AkInt32)AkCurveInterpolation_Sine ).iValue;
	out_eEndFadeCurveType = (AkCurveInterpolation)m_props.GetAkProp( AkPropID_FadeOutCurve, (AkInt32)AkCurveInterpolation_Sine ).iValue;
}

void CAkSoundBase::MonitorNotif(AkMonitorData::NotificationReason in_eNotifReason, AkGameObjectID in_GameObjID, UserParams& in_rUserParams, PlayHistory& in_rPlayHistory)
{
	MONITOR_OBJECTNOTIF( in_rUserParams.PlayingID(), in_GameObjID, in_rUserParams.CustomParam(), in_eNotifReason, in_rPlayHistory.HistArray, ID(), false, 0 );
}
