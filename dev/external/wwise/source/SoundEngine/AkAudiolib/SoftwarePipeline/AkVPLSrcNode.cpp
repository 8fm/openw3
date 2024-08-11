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

/////////////////////////////////////////////////////////////////////
//
// AkVPLSrcNode.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkVPLNode.h"
#include "AkVPLSrcNode.h"
#ifdef AK_VITA_HW
#include "AkSrcBankVAG.h"
#include "AkSrcFileVAG.h"
#include "AkSrcBankAt9.h"
#include "AkSrcFileAt9.h"
#include "AkSrcHwBankPCM.h"
#include "AkSrcHwFilePCM.h"
#else
#include "AkSrcBankPCM.h"
#include "AkSrcFilePCM.h"
#endif
#include "AkSrcPhysModel.h"
#include "AudiolibDefs.h"
#include "math.h"
#include "AkPositionRepository.h"
#include "AkPlayingMgr.h"
#include "AkMonitor.h"

#ifdef __PPU__
#include "AkSrcFileADPCMPS3.h"
#include "AkSrcBankADPCMPS3.h"
#else
#include "AkSrcFileADPCM.h"
#include "AkSrcBankADPCM.h"
#endif

#include "AkEffectsMgr.h"

#if ( defined AK_XBOXONE && !defined AK_XBOXONE_ADK ) || defined AK_XBOX360
#include "AkSrcBankXMA.h"
#include "AkSrcFileXMA.h"
#endif

CAkVPLSrcNode::CAkVPLSrcNode( CAkPBI * in_pCtx )
	: m_pAnalysisData( NULL )
	, m_pCtx( in_pCtx )
	, m_bIOReady( false )
	, m_bWaitForBuffering( false )
{
	AKASSERT( m_pCtx != NULL );	 
}


// Create a VPLSrcNode of the appropriate type
CAkVPLSrcNode * CAkVPLSrcNode::Create( CAkPBI * in_pCtx )
{
	CAkVPLSrcNode * pSrc = NULL;
	AkSrcTypeInfo * pSrcType = in_pCtx->GetSrcTypeInfo();

	{
		// Create the source.

		AkSrcType eType = (AkSrcType) pSrcType->mediaInfo.Type;
		AkUInt32 uiID = pSrcType->dwID;

		if ( eType == SrcTypeModelled )
		{
			pSrc = AkNew( g_LEngineDefaultPoolId, CAkSrcPhysModel( in_pCtx ) );
		}
		else if( eType == SrcTypeNone )
		{
			// Occurs when the source was not set, most likely the source was pushed by Wwise but the real source was not loaded from a bank.
			MONITOR_SOURCE_ERROR( AK::Monitor::ErrorCode_SelectedMediaNotAvailable, in_pCtx );
		}
		else
		{
			switch ( CODECID_FROM_PLUGINID( uiID ) )
			{
			case AKCODECID_ADPCM:
				if ( eType == SrcTypeFile )
					pSrc = AkNew( g_LEngineDefaultPoolId, CAkSrcFileADPCM( in_pCtx ) );
				else if ( eType == SrcTypeMemory )
					pSrc = AkNew( g_LEngineDefaultPoolId, CAkSrcBankADPCM( in_pCtx ) );
				break;

#if ( defined AK_XBOXONE && !defined AK_XBOXONE_ADK ) || defined AK_XBOX360
			case AKCODECID_XMA:
				if ( eType == SrcTypeFile )
					pSrc = AkNew( g_LEngineDefaultPoolId, CAkSrcFileXMA( in_pCtx ) );
				else if( eType == SrcTypeMemory )
					pSrc = AkNew( g_LEngineDefaultPoolId, CAkSrcBankXMA( in_pCtx ) );
				break;
#endif
#ifdef AK_VITA_HW
			case AKCODECID_VAG:
				if ( eType == SrcTypeFile )
				{
					pSrc = AkNew( g_LEngineDefaultPoolId, CAkSrcFileVAG( in_pCtx ) );
				}
				else if( eType == SrcTypeMemory )
				{
					pSrc = AkNew( g_LEngineDefaultPoolId, CAkSrcBankVAG( in_pCtx ) );
				}
				break;

			case AKCODECID_ATRAC9:
				if ( eType == SrcTypeFile )
				{
					pSrc = AkNew( g_LEngineDefaultPoolId, CAkSrcFileAt9( in_pCtx ) );
				}
				else if( eType == SrcTypeMemory )
				{
					pSrc = AkNew( g_LEngineDefaultPoolId, CAkSrcBankAt9( in_pCtx ) );
				}
				break;

			case AKCODECID_PCM:
				if ( eType == SrcTypeFile )
					pSrc = AkNew( g_LEngineDefaultPoolId, CAkSrcHwFilePCM( in_pCtx ) );
				else if( eType == SrcTypeMemory )
					pSrc = AkNew( g_LEngineDefaultPoolId, CAkSrcHwBankPCM( in_pCtx ) );
				break;
#else
			case AKCODECID_PCM:
				if ( eType == SrcTypeFile )
					pSrc = AkNew( g_LEngineDefaultPoolId, CAkSrcFilePCM( in_pCtx ) );
				else if( eType == SrcTypeMemory )
					pSrc = AkNew( g_LEngineDefaultPoolId, CAkSrcBankPCM( in_pCtx ) );
				break;
#endif

			case AK_INVALID_UNIQUE_ID:
				// WG-14239
				// If we end up here, it is probably because the source is not existing.
				// It may occur if a CAkSource was loaded, the media was excluded from 
				// this bank and the media is supposed to be from memory, and the media was 
				// not in any bank at all. Since this sound was nowhere, there was nowhere to point at.
				MONITOR_SOURCE_ERROR( AK::Monitor::ErrorCode_SelectedMediaNotAvailable, in_pCtx );
				break;

			default:
				pSrc = (CAkVPLSrcNode*)CAkEffectsMgr::AllocCodec( in_pCtx, eType, uiID ); // This will allocate bank or file source as necessary
				break;
			}
		}
	}

	if ( pSrc == NULL )
		in_pCtx->Destroy( CtxDestroyReasonPlayFailed );

	return pSrc;
} // Create

//-----------------------------------------------------------------------------
// Name: Term
// Desc: Terminate.
//-----------------------------------------------------------------------------
void CAkVPLSrcNode::Term( AkCtxDestroyReason in_eReason )
{
	// Unregister from the playing manager for source play position notification
	if ( m_pCtx->GetRegisteredNotif() & AK_EnableGetSourcePlayPosition )
	{
		g_pPositionRepository->RemoveSource( m_pCtx->GetPlayingID(), this );
	}

	m_pCtx->Destroy( in_eReason );

	StopStream();
} // Term

//-----------------------------------------------------------------------------
// Name: Start
// Desc: Indication that processing will start.
//-----------------------------------------------------------------------------
void CAkVPLSrcNode::Start()
{
	// Set the context's duration for crossfades.
	AkReal32 fOriginalDuration = 0.f;
	if ( IsIOReady() )
		fOriginalDuration = GetDuration();
	m_pCtx->CalcEffectiveParams();
	AkReal32 fEstimatedDuration = ( fOriginalDuration / powf( 2.0f, ( m_pCtx->GetEffectiveParams().Pitch/1200.0f ) ) );
	m_pCtx->Play( fEstimatedDuration );

	// Notify the playing manager for sound duration callback
	g_pPlayingMgr->NotifyDuration(
		m_pCtx->GetPlayingID(), 
		fOriginalDuration,
		fEstimatedDuration,
		m_pCtx->GetSoundID());

	// Register to the playing manager for source play position notification
	if ( m_pCtx->GetRegisteredNotif() & AK_EnableGetSourcePlayPosition )
	{
		g_pPositionRepository->AddSource( m_pCtx->GetPlayingID(), this );
	}
} // Start

//-----------------------------------------------------------------------------
// Name: Stop
// Desc: Indication that processing will stop.
//-----------------------------------------------------------------------------
void CAkVPLSrcNode::Stop()
{
	m_pCtx->Stop();
} // Stop

//-----------------------------------------------------------------------------
// Name: Pause
// Desc: Indication that processing will pause.
//-----------------------------------------------------------------------------
void CAkVPLSrcNode::Pause()
{
	m_pCtx->Pause();
	g_pPositionRepository->SetRate( m_pCtx->GetPlayingID(), 0 );
} // Pause

//-----------------------------------------------------------------------------
// Name: Resume
// Desc: Indication that processing will resume.
//-----------------------------------------------------------------------------
void CAkVPLSrcNode::Resume( AkReal32 in_fOutputRate )
{
	m_pCtx->Resume();
	g_pPositionRepository->SetRate( m_pCtx->GetPlayingID(), in_fOutputRate );
} // Resume

//-----------------------------------------------------------------------------
// Name: Seek
// Desc: Called after a seek notification. Changes the source position if the 
//		 PBI still indicates that it should be changed.
//-----------------------------------------------------------------------------
AKRESULT CAkVPLSrcNode::Seek()
{
	if ( m_pCtx->RequiresSourceSeek() )
		return ChangeSourcePosition();
	return AK_Success;
}

//-----------------------------------------------------------------------------
// Name: Connect
// Desc: Connects the specified effect as input.
//
// Parameters:
//	CAkVPLNode * in_pInput : Pointer to the effect to connect.
//-----------------------------------------------------------------------------
void CAkVPLSrcNode::Connect( CAkVPLNode * in_pInput )
{
	AKASSERT( !"Not implemented" );
} // Connect

//-----------------------------------------------------------------------------
// Name: Disconnect
// Desc: Disconnects the specified effect as input.
//-----------------------------------------------------------------------------
void CAkVPLSrcNode::Disconnect( )
{
	AKASSERT( !"Not implemented" );
} // Disconnect


AKRESULT CAkVPLSrcNode::TimeSkip( AkUInt32 & io_uFrames )
{
	AkVPLState state;
	state.SetRequestSize( (AkUInt16) io_uFrames );
    GetBuffer( state );
	if ( state.HasData() )
	{
		io_uFrames = state.uValidFrames;
		ReleaseBuffer();
	}
	else
	{
		io_uFrames = 0;
	}

	return state.result;
}

//-----------------------------------------------------------------------------
// Name: FetchStreamedData
// Desc: Performs I/O on streamed source in order to get audio format and 
//		 first data buffer.
// Note: Applies on source. Sets its IOReady flag.
//
// Return:
//	Ak_Success: Source is ready to be connected to a pipeline.
//	Ak_FormatNotReady: Source is not ready.
//  AK_Fail:    I/O error.
//-----------------------------------------------------------------------------

AKRESULT CAkVPLSrcNode::FetchStreamedData()
{
	if ( IsIOReady() )
		return AK_Success;

	AKRESULT eResult = StartStream();
	if ( eResult == AK_Success )
		SetIOReady();
	return eResult;
}

void CAkVPLSrcNode::NotifySourceStarvation()
{
	AKASSERT( !m_bWaitForBuffering );
	MONITOR_SOURCE_ERROR( AK::Monitor::ErrorCode_StreamingSourceStarving, m_pCtx );
}

