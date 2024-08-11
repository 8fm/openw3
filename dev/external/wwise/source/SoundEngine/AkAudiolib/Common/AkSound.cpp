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
// AkSound.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"

#include "AkSound.h"
#include "AkAudioLibIndex.h"
#include "AkContinuationList.h"
#include "PrivateStructures.h"
#include "AkURenderer.h"
#include "AudiolibDefs.h"
#include <AK/Tools/Common/AkBankReadHelpers.h>
#include "AkCntrHistory.h"
#include "AkModifiers.h"
#include "AkFXMemAlloc.h"
#include "ActivityChunk.h"

CAkSound::CAkSound( AkUniqueID in_ulID )
:CAkSoundBase( in_ulID )
{
}

CAkSound::~CAkSound()
{
}

CAkSound* CAkSound::Create( AkUniqueID in_ulID )
{
	CAkSound* pAkSound = AkNew( g_DefaultPoolId, CAkSound( in_ulID ) );

	if( pAkSound && pAkSound->Init() != AK_Success )
	{
		pAkSound->Release();
		pAkSound = NULL;
	}
	
	return pAkSound;
}

AkNodeCategory CAkSound::NodeCategory()
{
	return AkNodeCategory_Sound;
}

AKRESULT CAkSound::PlayInternal( AkPBIParams& in_rPBIParams )
{

    return CAkURenderer::Play( this, &m_Source, in_rPBIParams );
}

AKRESULT CAkSound::ExecuteAction( ActionParams& in_rAction )
{
	AKRESULT eResult = AK_Success;

	if( in_rAction.bIsMasterCall )
	{
		//Only global pauses should Pause a state transition
		bool bPause = in_rAction.eType == ActionParamType_Pause;
		PauseTransitions( bPause );
	}
	if( IsPlaying() )
	{
		switch( in_rAction.eType )
		{
		case ActionParamType_Stop:
			eResult = CAkURenderer::Stop( this, in_rAction.pGameObj, in_rAction.transParams, in_rAction.playingID );
			break;
		case ActionParamType_Pause:
			eResult = CAkURenderer::Pause( this, in_rAction.pGameObj, in_rAction.transParams, in_rAction.playingID );
			break;
		case ActionParamType_Resume:
			eResult = CAkURenderer::Resume( this, in_rAction.pGameObj, in_rAction.transParams, in_rAction.bIsMasterResume, in_rAction.playingID );
			break;
		case ActionParamType_Break:
			PlayToEnd( in_rAction.pGameObj, in_rAction.targetNodePtr, in_rAction.playingID );
			break;
		case ActionParamType_Seek:
			SeekSound( in_rAction.pGameObj, (SeekActionParams&)in_rAction );
			break;
		}
	}
	return eResult;
}

AKRESULT CAkSound::ExecuteActionExcept( ActionParamsExcept& in_rAction )
{
	AKRESULT eResult = AK_Success;
	if( in_rAction.pGameObj == NULL )
	{
		//Only global pauses should Pause a state transition
		bool bPause = in_rAction.eType == ActionParamType_Pause;
		PauseTransitions( bPause );
	}

	if( IsPlaying() )
	{
		switch( in_rAction.eType )
		{
		case ActionParamType_Stop:
			eResult = CAkURenderer::Stop( this, in_rAction.pGameObj, in_rAction.transParams, in_rAction.playingID );
			break;
		case ActionParamType_Pause:
			eResult = CAkURenderer::Pause( this, in_rAction.pGameObj, in_rAction.transParams, in_rAction.playingID );
			break;
		case ActionParamType_Resume:
			eResult = CAkURenderer::Resume( this, in_rAction.pGameObj, in_rAction.transParams, in_rAction.bIsMasterResume, in_rAction.playingID );
			break;
		case ActionParamType_Seek:
			SeekSound( in_rAction.pGameObj, (SeekActionParamsExcept&)in_rAction );
			break;
		}
	}

	return eResult;
}

void CAkSound::SeekSound( CAkRegisteredObj * in_pGameObj, const SeekActionParams & in_rActionParams )
{
	if ( in_rActionParams.bIsSeekRelativeToDuration )
	{
		AkReal32 fSeekPercent = in_rActionParams.fSeekPercent;
		
		// Clamp to [0,1].
		if ( fSeekPercent < 0 )
			fSeekPercent = 0;
		else if ( fSeekPercent > 1 )
			fSeekPercent = 1;

		if( IsActivityChunkEnabled() )
		{
			AkActivityChunk::AkListLightCtxs& rListPBI = m_pActivityChunk->m_listPBI;
			for( AkActivityChunk::AkListLightCtxs::Iterator iter = rListPBI.Begin(); iter != rListPBI.End(); ++iter )
			{
				CAkPBI * l_pPBI = *iter;
				if( CheckObjAndPlayingID( in_pGameObj, l_pPBI->GetGameObjectPtr(), in_rActionParams.playingID, l_pPBI->GetPlayingID() ) )
				{
					l_pPBI->SeekPercent( fSeekPercent, in_rActionParams.bSnapToNearestMarker );
				}
			}
		}
	}
	else
	{
		AkTimeMs iSeekPosition = in_rActionParams.iSeekTime;
		
		// Negative seek positions are not supported on sounds: clamped to 0.
		if ( iSeekPosition < 0 )
			iSeekPosition = 0;

		if( IsActivityChunkEnabled() )
		{
			AkActivityChunk::AkListLightCtxs& rListPBI = m_pActivityChunk->m_listPBI;
			for( AkActivityChunk::AkListLightCtxs::Iterator iter = rListPBI.Begin(); iter != rListPBI.End(); ++iter )
			{
				CAkPBI * l_pPBI = *iter;
				if( CheckObjAndPlayingID( in_pGameObj, l_pPBI->GetGameObjectPtr(), in_rActionParams.playingID, l_pPBI->GetPlayingID() ) )
				{
					l_pPBI->SeekTimeAbsolute( iSeekPosition, in_rActionParams.bSnapToNearestMarker );
				}
			}
		}
	}
}

// NOTE: At this point the exception list is useless.
// REVIEW: ActionParams structures hierarchy.
void CAkSound::SeekSound( CAkRegisteredObj * in_pGameObj, const SeekActionParamsExcept & in_rActionParams )
{
	if ( in_rActionParams.bIsSeekRelativeToDuration )
	{
		AkReal32 fSeekPercent = in_rActionParams.fSeekPercent;
		
		// Clamp to [0,1].
		if ( fSeekPercent < 0 )
			fSeekPercent = 0;
		else if ( fSeekPercent > 1 )
			fSeekPercent = 1;

		if( IsActivityChunkEnabled() )
		{
			AkActivityChunk::AkListLightCtxs& rListPBI = m_pActivityChunk->m_listPBI;
			for( AkActivityChunk::AkListLightCtxs::Iterator iter = rListPBI.Begin(); iter != rListPBI.End(); ++iter )
			{
				CAkPBI * l_pPBI = *iter;
				if( ( !in_pGameObj || l_pPBI->GetGameObjectPtr() == in_pGameObj ) )
				{
					l_pPBI->SeekPercent( fSeekPercent, in_rActionParams.bSnapToNearestMarker );
				}
			}
		}
	}
	else
	{
		AkTimeMs iSeekPosition = in_rActionParams.iSeekTime;
		
		// Negative seek positions are not supported on sounds: clamped to 0.
		if ( iSeekPosition < 0 )
			iSeekPosition = 0;

		if( IsActivityChunkEnabled() )
		{
			AkActivityChunk::AkListLightCtxs& rListPBI = m_pActivityChunk->m_listPBI;
			for( AkActivityChunk::AkListLightCtxs::Iterator iter = rListPBI.Begin(); iter != rListPBI.End(); ++iter )
			{
				CAkPBI * l_pPBI = *iter;
				if( ( !in_pGameObj || l_pPBI->GetGameObjectPtr() == in_pGameObj ) )
				{
					l_pPBI->SeekTimeAbsolute( iSeekPosition, in_rActionParams.bSnapToNearestMarker );
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Name: Category
// Desc: Returns the category of the sound.
//
// Parameters: None.
//
// Return: AkObjectCategory.
//-----------------------------------------------------------------------------
AkObjectCategory CAkSound::Category()
{
	return ObjCategory_Sound;
}

//-----------------------------------------------------------------------------
// Name: SetInitialValues
// Desc: Sets the initial Bank source.
//
// Return: Ak_Success:          Initial values were set.
//         AK_InvalidParameter: Invalid parameters.
//         AK_Fail:             Failed to set the initial values.
//-----------------------------------------------------------------------------
AKRESULT CAkSound::SetInitialValues( AkUInt8* in_pData, AkUInt32 in_ulDataSize, CAkUsageSlot* /*in_pUsageSlot*/, bool in_bIsPartialLoadOnly )
{
	AKRESULT eResult = AK_Success;

	//Read ID
	// We don't care about the ID, just read/skip it
	SKIPBANKDATA(AkUInt32, in_pData, in_ulDataSize);

	//Read Source info
	if(eResult == AK_Success)
	{
		AkBankSourceData oSourceInfo;
		eResult = CAkBankMgr::LoadSource(in_pData, in_ulDataSize, oSourceInfo);
		if (eResult != AK_Success)
			return eResult;

		if (oSourceInfo.m_pParam == NULL)
		{
			//This is a file source, specified by ID.
			SetSource( oSourceInfo.m_PluginID, oSourceInfo.m_MediaInfo );
		}
		else
		{
			//This is a plugin
			SetSource( oSourceInfo.m_MediaInfo.sourceID );
		}
	}

	//ReadParameterNode
	eResult = SetNodeBaseParams(in_pData, in_ulDataSize, in_bIsPartialLoadOnly);

	if( in_bIsPartialLoadOnly )
	{
		//Partial load has been requested, probable simply replacing the actual source created by the Wwise on load bank.
		return eResult;
	}

	CHECKBANKDATASIZE( in_ulDataSize, eResult );

	return eResult;
}

AKRESULT CAkSound::PrepareData()
{
	return m_Source.PrepareData();
}

void CAkSound::UnPrepareData()
{
	m_Source.UnPrepareData();
}
