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
// AkVPLSrcCbxNode.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"

#include "AkVPLSrcCbxNode.h"

#include "AkLEngine_SoftwarePipeline.h"

#include "AkEffectsMgr.h"
#include "AkFxBase.h"
#include "AkFXMemAlloc.h"
#include "AkSoundBase.h"
#include "AkVPLSrcNode.h"
#include "AkVPLSrcCbxNode.h"
#include "AkVPLFilterNode.h"
#include "AkVPLFilterNodeOutOfPlace.h"
#include "AkVPLPitchNode.h"
#include "AkVPLLPFNode.h"
#include "AudiolibDefs.h"
#include "AkMonitor.h"
#include "AkMath.h"
#include "Ak3DListener.h"
#include "AkEnvironmentsMgr.h"
#include "AkPositionRepository.h"
#include "AkSpeakerPan.h"
#include "AkURenderer.h"
#include "AkBus.h"
#include "AkOutputMgr.h"

#define MAX_NODES		(3+AK_NUM_EFFECTS_PER_OBJ)	// Max nodes in the cbx node list.

extern AkInitSettings g_settings;

AkReal32 g_fVolumeThreshold = AK_OUTPUT_THRESHOLD;
AkReal32 g_fVolumeThresholdDB = AK_MINIMUM_VOLUME_DBFS;

AkVPLSrcCbxRec::AkVPLSrcCbxRec( CAkVPLSrcCbxNode * in_pCbxNode )
	: m_Pitch( in_pCbxNode )
{
	for ( AkUInt32 uFXIndex = 0; uFXIndex < AK_NUM_EFFECTS_PER_OBJ; ++uFXIndex )
		m_pFilter[ uFXIndex ] = NULL;
}

void AkVPLSrcCbxRec::ClearVPL()
{
	for ( AkUInt32 uFXIndex = 0; uFXIndex < AK_NUM_EFFECTS_PER_OBJ; ++uFXIndex )
	{
		if( m_pFilter[ uFXIndex ] != NULL )
		{
			m_pFilter[ uFXIndex ]->Term();
			AkDelete( g_LEngineDefaultPoolId, m_pFilter[ uFXIndex ] );
			m_pFilter[ uFXIndex ] = NULL;
		}
	}

	m_Pitch.Term();
	m_LPF.Term();
}

CAkVPLSrcCbxNodeBase::CAkVPLSrcCbxNodeBase()
	: m_pHdrBus( NULL )
	, m_fBehavioralVolume( 0 )
	, m_fMaxVolumeDB( AK_SAFE_MINIMUM_VOLUME_LEVEL )
#ifndef AK_OPTIMIZED
	, m_fWindowTop( 0 )
	, m_fLastEnvelope( 0 )
	, m_fNormalizationGain( 1.f )
#endif
	, m_eState( NodeStateInit )
	, m_bAudible( true )
	, m_bPreviousSilent( true )
	, m_bFirstBufferProcessed( false )
	, m_bFirstSendsCalculated( false )
	, m_bIsAuxRoutable( false )
	, m_bHasStarved( false )
	, m_uNumSends( 0 )
#ifndef AK_OPTIMIZED
	, m_iWasStarvationSignaled( 0 )
#endif
{
	for ( AkUInt32 i = 0; i < MAX_NUM_SOURCES; ++i )
		m_pSources[i] = NULL;
}

CAkVPLSrcCbxNodeBase::~CAkVPLSrcCbxNodeBase()
{
	m_arVolumeData.Term();
}

//-----------------------------------------------------------------------------
// Name: Start
// Desc: Indication that processing will start.
//
// Parameters:
//	None.
//-----------------------------------------------------------------------------

void CAkVPLSrcCbxNodeBase::Start()
{
	// Do not start node if not in init state.
	if( m_eState != NodeStateInit )
	{
		// Only report error if sound was not stopped... otherwise a false error.
		if( m_eState != NodeStateStop )
		{
			Stop();
			MONITOR_SOURCE_ERROR( AK::Monitor::ErrorCode_CannotPlaySource_InconsistentState, GetContext() );
		}
		return;
	}

	// Start the current node
	m_pSources[ 0 ]->Start();

	m_eState = NodeStatePlay;
} // Start

//-----------------------------------------------------------------------------
// Name: FetchStreamedData
// Desc: Fetches streaming data of source record 0. 
//		 If the source is ready and the context is ready to be connected, 
//		 returns success.
//
// Return:
//	Ak_Success: Ready to create pipeline.
//	Ak_FormatNotReady: Not ready to create pipeline.
//-----------------------------------------------------------------------------
AKRESULT CAkVPLSrcCbxNodeBase::FetchStreamedData(
	CAkPBI * in_pCtx 
	)
{
	AKASSERT( m_pSources[ 0 ] );

	AKRESULT l_eResult = m_pSources[ 0 ]->FetchStreamedData();

	if ( l_eResult == AK_FormatNotReady )
	{
		// Not ready. Bail out. 
		// Notify source starvation if applicable.
		if ( in_pCtx->GetFrameOffset() < 0 )
		{
			// We're late, and the source is not prebuffering.
			HandleSourceStarvation();
		}
		return AK_FormatNotReady;
	}
	else if ( l_eResult == AK_Success )
	{
		// Optimization: keep sources disconnected until they are needed, that is, when their frame offset 
		// is smaller than one sink frame (will run within this frame) + the amount of look-ahead quanta.
#ifdef AK_MOTION
		AkInt32 iMaxOffset = (1 + g_settings.uContinuousPlaybackLookAhead) * ( (!in_pCtx->IsForFeedbackPipeline()) ? AK_NUM_VOICE_REFILL_FRAMES : AK_FEEDBACK_MAX_FRAMES_PER_BUFFER );
#else
		AkInt32 iMaxOffset = (1 + g_settings.uContinuousPlaybackLookAhead) * AK_NUM_VOICE_REFILL_FRAMES;
#endif

		if ( in_pCtx->GetFrameOffset() < iMaxOffset )
		{
			// We're ready, and frame offset is smaller than an audio frame. Connect now 
			// to start pulling data in this audio frame.
			// Source is ready: Add pipeline.
			AKASSERT( m_pSources[ 0 ]->IsIOReady() );

			// Post Source Starvation notification if context requires sample accuracy.
			if ( in_pCtx->GetFrameOffset() < 0 )
			{
				// We're late, and the source is not prebuffering.
				HandleSourceStarvation();
			}
			return AK_Success;			
		}
		else
		{
			// Frame offset greater than an audio frame: keep it in list of sources not connected.
			return AK_FormatNotReady;
		}
	}
	return AK_Fail;
}

//-----------------------------------------------------------------------------
// Name: Pause
// Desc: Pause the source.
//
// Parameters:
//	None.
//-----------------------------------------------------------------------------
void  CAkVPLSrcCbxNodeBase::Pause()
{
	// Do not pause node if not in play state.
	if( m_eState != NodeStatePlay )
	{
		Stop();
		MONITOR_SOURCE_ERROR( AK::Monitor::ErrorCode_CannotPlaySource_InconsistentState, GetContext() );
		return;
	}

	// Pause the current node

	AKASSERT( m_pSources[ 0 ] );
	m_pSources[ 0 ]->Pause();

	m_eState = NodeStatePause;
} // Pause

void CAkVPLSrcCbxNodeBase::SetAudible( CAkPBI * in_pCtx, bool in_bAudible )
{
	AKASSERT( in_pCtx );

	if( m_bAudible )
	{
		if( !in_bAudible )
		{
			in_pCtx->Virtualize();
		}
	}
	else if( in_bAudible )
	{
		in_pCtx->Devirtualize();
	}
	
	m_bAudible = in_bAudible;
}

CAkVPLSrcCbxNode::CAkVPLSrcCbxNode()
	: m_uChannelMask( AK_SPEAKER_FRONT_CENTER )
	, m_cbxRec( this )
{
}

void CAkVPLSrcCbxNode::Init( AkUInt32 in_uSampleRate )
{
	m_uSampleRate = in_uSampleRate;
}

void CAkVPLSrcCbxNode::Term()
{
	//Also disconnect from Auxiliary busses
	for(AkUInt32 i = 0; i < m_uNumSends; i++)
		 m_arSendValues[i].PerDeviceAuxBusses.Term();

	RemovePipeline( CtxDestroyReasonFinished );

	// Remove next sources.
	for( AkUInt32 i = 1; i < MAX_NUM_SOURCES; i++ )
	{
		if ( m_pSources[ i ] )
		{
			m_pSources[i]->Term( CtxDestroyReasonFinished );
			AkDelete( g_LEngineDefaultPoolId, m_pSources[i] );
			m_pSources[i] = NULL;
		}
	}
} // Term

//-----------------------------------------------------------------------------
// Name: Stop
// Desc: Indication that processing will stop.
//
// Parameters:
//	None.
//-----------------------------------------------------------------------------
void CAkVPLSrcCbxNode::Stop()
{
	// Stop the current node

	if ( m_pSources[ 0 ] )
		m_pSources[ 0 ]->Stop();

	m_eState = NodeStateStop;
} // Stop



//-----------------------------------------------------------------------------
// Name: Resume
// Desc: Resume the source.
//
// Parameters:
//	None.
//-----------------------------------------------------------------------------
void CAkVPLSrcCbxNode::Resume()
{
	switch( m_eState )
	{
	case NodeStatePause:
		AKASSERT( m_pSources[ 0 ] );
		m_pSources[ 0 ]->Resume( m_cbxRec.m_Pitch.GetLastRate() );
		m_eState = NodeStatePlay;
		break;

	case NodeStatePlay:
		// We were not paused, nothing to resume.
		// Do Nothing
		// This can happen and is not an error, see : WG-16984
		break;

	default:
		// Do not resume node if not in pause state
		Stop();
		MONITOR_SOURCE_ERROR( AK::Monitor::ErrorCode_CannotPlaySource_InconsistentState, GetContext() );
		break;
	}
} // Resume

bool CAkVPLSrcCbxNode::StartRun( AkVPLState & io_state )
{
	// Do this as soon as possible to avoid a LHS on some platforms.
	const AkChannelMask uChannelMask = m_uChannelMask;

	CAkPBI * pSrcContext = m_pSources[ 0 ]->GetContext(); // remember the FIRST context to play in this frame
	AKASSERT( pSrcContext != NULL );

	AkUInt32 ulStopOffset = pSrcContext->GetStopOffset();
	if ( AK_EXPECT_FALSE( ulStopOffset != AK_NO_IN_BUFFER_STOP_REQUESTED ) )
	{
		io_state.bStop = true;
		if ( AK_EXPECT_FALSE( ulStopOffset == 0 ) )
		{
			Stop();
			return false; // stop immediately: no need to run at all.
		}
	}

	AkReal32 fLPF, fObsLPF;
	bool bNextSilent, bAudible;
	io_state.SetChannelMask( uChannelMask );
	io_state.bIsAuxRoutable = m_bIsAuxRoutable;	// Cached in ComputeVolumeRays().
	GetVolumes( io_state.bIsAuxRoutable, pSrcContext, uChannelMask, bNextSilent, bAudible, fLPF, fObsLPF );

	//In the feedback pipeline, we don't need to wait for the voice to be silent to stop.  The fade out is handled by the device.
	if ( bNextSilent || pSrcContext->IsForFeedbackPipeline() )
	{
		//If the next buffer is silent, the LPF isn't computed.  Just keep the same values for the last buffer.
		fLPF = m_cbxRec.m_LPF.GetLPF();
		fObsLPF = m_ObstructionLPF.GetLPF();

		if ( pSrcContext->WasStopped() )
			io_state.bStop = true;
		else if ( pSrcContext->WasPaused() )
			io_state.bPause = true;
	}

	io_state.bAudible = bAudible;

	bool bNeedToRun = true;

	if ( bAudible ) 
	{
		// Set LPF.
		m_cbxRec.m_LPF.SetLPF( fLPF );
		m_ObstructionLPF.SetLPF( AkMath::Max( fObsLPF, pSrcContext->GetOutputBusOutputLPF() ) );

		// Switching from non-audible to audible
		CAkVPLSrcNode * l_pSrcNode = m_pSources[ 0 ];
		if ( !m_bAudible || !l_pSrcNode->IsIOReady() )
		{
			if ( m_eBelowThresholdBehavior == AkBelowThresholdBehavior_SetAsVirtualVoice ) 
			{
				if( !l_pSrcNode->IsIOReady() )
				{
					// Note: The pipeline is connected but streaming is not ready. This is the consequence 
					// of the "voice initially virtual faking scheme" (see AddSrc). Let's repair it now.

					// IMPORTANT: Behavioral handling of virtual voices should be done here as well. 
					// However this path is only used with FromBeginning voices Initially Under Threshold,
					// and currently no high-level engine handles this case differently.
					/// AkBehavioralVirtualHandlingResult eVirtualHandlingResult = pSrcContext->NotifyVirtualOff( m_eVirtualBehavior );
					AKASSERT( pSrcContext->NotifyVirtualOff( m_eVirtualBehavior ) == VirtualHandling_NotHandled );

					AKRESULT l_StreamResult = l_pSrcNode->FetchStreamedData();
					if( l_StreamResult == AK_FormatNotReady )
					{
					// not ready, let gain some time by telling we are not audible.
						bAudible = false;
						bNeedToRun = false;
					}
					else if( l_StreamResult != AK_Success )
					{
						Stop();
						// not ready, let's gain some time by telling we are not audible.
						bAudible = false;
						bNeedToRun = false;
					}
				}
				else // yes, must check it again, may have been set to false in previous call.
				{
					// Notify the upper engine now before processing VirtualOff on the pipeline.
					bool bVirtualOffUseSourceOffset = false;
					AkCtxVirtualHandlingResult eVirtualHandlingResult = pSrcContext->NotifyVirtualOff( m_eVirtualBehavior );
					switch ( eVirtualHandlingResult )
					{
					case VirtualHandling_ShouldStop:
						// Stop and don't monitor: the behavioral engine thinks that there is no point in restarting this voice.
						Stop();
						return false;
					case VirtualHandling_RequiresSeeking:
						bVirtualOffUseSourceOffset = true;
						break;
					default:
						break;
					}

					if ( m_cbxRec.Head()->VirtualOff( m_eVirtualBehavior, bVirtualOffUseSourceOffset ) != AK_Success )
					{
						Stop();
						MONITOR_SOURCE_ERROR( AK::Monitor::ErrorCode_CannotPlaySource_VirtualOff, GetContext() );
						return false;
					}
				}

			}
		}
	}
	else // !audible
	{
		if (!m_bFirstBufferProcessed && !HasOutputDevice())
		{
			//No listener to hear this sound.
			pSrcContext->Monitor(AkMonitorData::NotificationReason_VirtualNoListener);
		}

		if ( m_eBelowThresholdBehavior == AkBelowThresholdBehavior_SetAsVirtualVoice ) 
		{
			bNeedToRun = false;

			// Switching from audible to non-audible
			if ( m_bAudible ) 
			{
				m_cbxRec.Head()->VirtualOn( m_eVirtualBehavior );

				if ( m_eVirtualBehavior != AkVirtualQueueBehavior_Resume ) 
				{
					AKASSERT( m_eVirtualBehavior == AkVirtualQueueBehavior_FromBeginning 
						|| m_eVirtualBehavior == AkVirtualQueueBehavior_FromElapsedTime );

					// Release any incomplete output buffer.
					// Note. We could keep incomplete output buffers and consume them
					// before time skipping, but the pitch node does not support it.
					// Release (we dropped a few milliseconds, and perhaps a few markers).
					m_cbxRec.Head()->ReleaseBuffer();
				}
			}

			if ( m_eVirtualBehavior == AkVirtualQueueBehavior_FromElapsedTime )
			{
				AkInt32 iFrameOffset = pSrcContext->GetFrameOffset();
				if( iFrameOffset < io_state.MaxFrames() )
				{
					io_state.result = SourceTimeSkip( io_state.MaxFrames() );
				}

				pSrcContext->ConsumeFrameOffset( io_state.MaxFrames() );
			}
		}
		else if ( m_eBelowThresholdBehavior == AkBelowThresholdBehavior_KillVoice )
		{
			//Setting it to no more data will directly term it.
			Stop();
			bNeedToRun = false;

			pSrcContext->Monitor(AkMonitorData::NotificationReason_KilledVolumeUnderThreshold);
		}
		else // else we are not audible but we must consume a buffer.
		{
			// We are in Continue To Play Mode, We must be audible from here.
			// Take note that higher in this function io_state.bAudible = bAudible; was set.
			// It is expected, it will prevent some of the processing alter on in the pipeline.
			bAudible = true;
		}
	}

	// Handle source starvation.
	if ( AK_EXPECT_FALSE( m_bHasStarved ) )
	{
		// Seek source to correct position if pSrcContext supports sample-accurate VirtualOff through seeking.
		AkCtxVirtualHandlingResult eVirtualHandlingResult = pSrcContext->NotifyVirtualOff( m_eVirtualBehavior );
		if ( VirtualHandling_RequiresSeeking == eVirtualHandlingResult )
		{
			SeekSource();
			m_bPreviousSilent = true;	// To force a fade-in upon return.
		}
		else if ( VirtualHandling_ShouldStop == eVirtualHandlingResult )
		{
			Stop();
			bNeedToRun = false;
		}
			
		m_bHasStarved = false;
	}

	AkInt32 iFrameOffset = pSrcContext->GetFrameOffset();
	// upstream nodes are not yet required to output data if frame offset greater than one frame.
	bNeedToRun = bNeedToRun && ( iFrameOffset < io_state.MaxFrames() );
	pSrcContext->ConsumeFrameOffset( io_state.MaxFrames() );

	// Update state.
	SetAudible( pSrcContext, bAudible );

	if ( bNeedToRun )
	{
		// Important: update m_bPreviousSilent only if bNeedToRun.
		m_bPreviousSilent = bNextSilent;
	
		if ( AK_EXPECT_FALSE( !PipelineAdded() ) )
		{
			AKRESULT eAddResult = AddPipeline();
			if ( eAddResult == AK_Success )
			{
				pSrcContext->CalcEffectiveParams(); // Motion parameters are invalidated by StartStream()
				// Now recalc volume using proper channel mask, but keep the original virtualness decision
				pSrcContext->GetPrevPosParams().Invalidate();
				io_state.SetChannelMask( m_uChannelMask );
				AkReal32 fDummy;
				GetVolumes( io_state.bIsAuxRoutable, pSrcContext, m_uChannelMask, bNextSilent, bAudible, fDummy, fDummy );
			}
			else
			{
				Stop();
				bNeedToRun = false;
			}
		}
	}
	m_bFirstBufferProcessed = true;

	return bNeedToRun;
}

void CAkVPLSrcCbxNode::ConsumeBuffer( AkVPLState & io_state )
{
	CAkPBI * pCtx = m_pSources[ 0 ]->GetContext();
	if ( pCtx->GetRegisteredNotif() & AK_EnableGetSourcePlayPosition )
	{
		// The cookie is the source.
		g_pPositionRepository->UpdatePositionInfo( pCtx->GetPlayingID(), &io_state.posInfo, m_pSources[ 0 ] );
	}

	// Handle stop offsets.
	// IMPORTANT: Currently stop offsets and sample accurate containers using stitch buffers are 2 paths that are
	// mutually exclusive. If this changes, changing the number of valid frames on the io_state should be done 
	// differently with m_bufferMix.
	AkUInt32 uStopOffset = pCtx->GetAndClearStopOffset();
	if ( AK_EXPECT_FALSE( uStopOffset != AK_NO_IN_BUFFER_STOP_REQUESTED ) )
	{
		if ( uStopOffset < io_state.uValidFrames )
			io_state.uValidFrames = (AkUInt16)uStopOffset;
		io_state.bStop = true;
	}

	/////////////////////////////////////////////////////////////////////
	// If a sample accurate source is pending, we must keep on fetching 
	// it since all PCMex files will require multiple fetch..
	// WG-14332
	// The problem could also occur if the file header is too big to make it possible to have the sound sample accurate.

	if ( m_pSources[ 1 ] )
	{
		AKRESULT eResult = m_pSources[ 1 ]->FetchStreamedData();
		if ( eResult == AK_Fail )
		{
			io_state.result = AK_Fail;
			return;
		}
	}
}

//-----------------------------------------------------------------------------
// Name: ReleaseBuffer
// Desc: Releases a processed buffer.
//-----------------------------------------------------------------------------
void CAkVPLSrcCbxNode::ReleaseBuffer()
{
	// Otherwise we need to release an upstream node's buffer.

	m_cbxRec.Head()->ReleaseBuffer();
} // ReleaseBuffer

void CAkVPLSrcCbxNode::SwitchToNextSrc()
{
	m_pSources[ 0 ]->Term( CtxDestroyReasonFinished );
	AkDelete( g_LEngineDefaultPoolId, m_pSources[ 0 ] );

	m_pSources[ 0 ] = m_pSources[ 1 ];
	m_pSources[ 1 ] = NULL;

	m_pSources[ 0 ]->Start();

	for ( AkUInt32 uFXIndex = 0; uFXIndex < AK_NUM_EFFECTS_PER_OBJ; ++uFXIndex )
	{
		CAkVPLFilterNodeBase * pFilter = m_cbxRec.m_pFilter[ uFXIndex ];
		if ( pFilter )
			pFilter->SetPBI( m_pSources[ 0 ]->GetContext() );
	}

	// No need to take care of pitch node since the change originates from it.
}

void CAkVPLSrcCbxNode::UpdateFx( AkUInt32 in_uFXIndex )
{
	if ( in_uFXIndex == (AkUInt32) -1 ) 
	{
		AKASSERT( false && "UpdateFx not supported for source plugin!" );
		return;
	}
	// Find neighbors of the node (for re-connection)

	CAkVPLNode * pNext = NULL;
	CAkVPLNode * pPrev = NULL;

	bool bPrevIsFilter = false;
	bool bNextIsFilter = false;

	{	
		for ( int iNextFX = in_uFXIndex + 1; iNextFX < AK_NUM_EFFECTS_PER_OBJ; ++iNextFX )
		{
			if ( m_cbxRec.m_pFilter[ iNextFX ] )
			{
				pNext = m_cbxRec.m_pFilter[ iNextFX ];
				bNextIsFilter = true;
				break;
			}
		}

		if ( pNext == NULL )
			pNext = &m_cbxRec.m_LPF;

		for ( int iPrevFX = in_uFXIndex - 1; iPrevFX >= 0; --iPrevFX )
		{
			if ( m_cbxRec.m_pFilter[ iPrevFX ] )
			{
				pPrev = m_cbxRec.m_pFilter[ iPrevFX ];
				bPrevIsFilter = true;
				break;
			}
		}
		if ( pPrev == NULL )
			pPrev = &m_cbxRec.m_Pitch;
	}

	// Determine input format

	CAkPBI * pCtx = m_pSources[ 0 ]->GetContext();
	AkAudioFormat informat = pCtx->GetMediaFormat();
	if ( bPrevIsFilter )
		informat.uChannelMask = static_cast<CAkVPLFilterNodeBase *>( pPrev )->GetOutputChannelMask();
	informat.uBitsPerSample = AK_LE_NATIVE_SAMPLETYPE == AK_FLOAT ? 32 : 16;
	informat.uBlockAlign = informat.GetNumChannels() * informat.uBitsPerSample / 8;
	informat.uInterleaveID = AK_LE_NATIVE_INTERLEAVE;
	informat.uSampleRate = m_uSampleRate;
	informat.uTypeID = AK_LE_NATIVE_SAMPLETYPE;

	AkChannelMask uOutChannelMask = informat.uChannelMask;

	// Remove existing FX 

	CAkVPLFilterNodeBase * pFilter = m_cbxRec.m_pFilter[ in_uFXIndex ];
	if ( pFilter )
	{
		uOutChannelMask = pFilter->GetOutputChannelMask();

		// Out of place effect possibly consumed part of the buffer so release it, in place effect don<t need to
		// Need to release references to input buffer all the way to the next out of place effect
		for ( int iNextFX = in_uFXIndex; iNextFX < AK_NUM_EFFECTS_PER_OBJ; ++iNextFX )
		{
			if ( m_cbxRec.m_pFilter[ iNextFX ] )
			{	
				bool bBufferReleased = m_cbxRec.m_pFilter[ iNextFX ]->ReleaseInputBuffer();
				if ( bBufferReleased && iNextFX != in_uFXIndex )
					break;
			}
		}

		pNext->Disconnect( );

		pFilter->Term();
		AkDelete( g_LEngineDefaultPoolId, pFilter );
		pFilter = NULL;

		m_cbxRec.m_pFilter[ in_uFXIndex ] = NULL;
	}
	
	// Insert new FX

	CAkSoundBase * pSound = pCtx->GetSound();		

	AkFXDesc fxDesc;
	pSound->GetFX( in_uFXIndex, fxDesc, pCtx->GetGameObjectPtr() );

	AkChannelMask uNewOutChannelMask;

	if( fxDesc.pFx )
	{
		IAkPlugin * pPlugin = NULL;
		AKRESULT l_eResult = CAkEffectsMgr::Alloc( AkFXMemAlloc::GetLower(), fxDesc.pFx->GetFXID(), pPlugin );
		if ( l_eResult != AK_Success )
		{
			MONITOR_PLUGIN_ERROR( AK::Monitor::ErrorCode_PluginAllocationFailed, pCtx, fxDesc.pFx->GetFXID() );	
			pNext->Connect( pPrev );
			return; // Silently fail, add pipeline will continue with the pipeline creation without the effect
		}
			
		AkPluginInfo pluginInfo;
		pPlugin->GetPluginInfo( pluginInfo );

		if ( pluginInfo.bIsAsynchronous != 
#ifdef AK_PS3
			true
#else
			false
#endif
			)
		{
			MONITOR_ERROR( AK::Monitor::ErrorCode_PluginExecutionInvalid ); 	
			pPlugin->Term( AkFXMemAlloc::GetLower() );
			pNext->Connect( pPrev );
			return; // Silently fail, add pipeline will continue with the pipeline creation without the effect
		}
			
		if ( pluginInfo.bIsInPlace )
			pFilter = AkNew( g_LEngineDefaultPoolId, CAkVPLFilterNode() );
		else
			pFilter = AkNew( g_LEngineDefaultPoolId, CAkVPLFilterNodeOutOfPlace() );

		if( pFilter == NULL )
		{
			pNext->Connect( pPrev );
			return;
		}

		l_eResult = pFilter->Init( pPlugin, fxDesc, in_uFXIndex, pCtx, informat );

		// Note: Effect can't be played for some reason but don't kill the sound itself yet...
		if ( l_eResult != AK_Success )
		{
			AKASSERT( pFilter );
			pFilter->Term();
			AkDelete( g_LEngineDefaultPoolId, pFilter );
			pNext->Connect( pPrev );
			return;
		}

		m_cbxRec.m_pFilter[ in_uFXIndex ] = pFilter;

		pFilter->SetBypassed( fxDesc.bIsBypassed );
		pFilter->Connect( pPrev );

		uNewOutChannelMask = pFilter->GetOutputChannelMask();
	}
	else
	{
		uNewOutChannelMask = informat.uChannelMask;
	}

	if( uNewOutChannelMask == uOutChannelMask )
	{
		pNext->Connect( pFilter ? pFilter : pPrev );
	}
	else if( bNextIsFilter )
	{
		// Need to reinitialize next effect if channel mask has changed
		UpdateFx( in_uFXIndex + 1 ); 
	}
	else
	{
		m_cbxRec.m_LPF.Term();
		m_cbxRec.m_LPF.Init( uNewOutChannelMask );
		m_ObstructionLPF.Term();
		m_ObstructionLPF.Init( uNewOutChannelMask );

		m_uChannelMask = uNewOutChannelMask;
#ifdef AK_MOTION
		// Motion Feedback parameters have allocations that are channel dependent. Changing the config also requires changing the feedback.
		pCtx->InvalidateFeedbackParameters();
#endif

		pNext->Connect( pFilter ? pFilter : pPrev );
	}
}

void CAkVPLSrcCbxNode::SetFxBypass(
		AkUInt32 in_bitsFXBypass,
		AkUInt32 in_uTargetMask /* = 0xFFFFFFFF */ )
{
	for ( AkUInt32 uFXIndex = 0; uFXIndex < AK_NUM_EFFECTS_PER_OBJ; ++uFXIndex )
	{
		CAkVPLFilterNodeBase * pFilter = m_cbxRec.m_pFilter[ uFXIndex ];
		if ( pFilter && ( in_uTargetMask & ( 1 << uFXIndex ) ) )
			pFilter->SetBypassed( ( in_bitsFXBypass & ( 1 << uFXIndex ) ) != 0 );
	}
}

void CAkVPLSrcCbxNode::RefreshBypassFx()
{	
	for ( AkUInt32 uFXIndex = 0; uFXIndex < AK_NUM_EFFECTS_PER_OBJ; ++uFXIndex )
	{
		RefreshBypassFx( uFXIndex );
	}
}

void CAkVPLSrcCbxNode::RefreshBypassFx( AkUInt32 in_uFXIndex )
{
	if ( m_pSources[ 0 ] )
	{
		CAkPBI * pCtx = m_pSources[ 0 ]->GetContext();
		CAkSoundBase * pSound = pCtx->GetSound();		
		
		CAkVPLFilterNodeBase * pFilter = m_cbxRec.m_pFilter[ in_uFXIndex ];
		if ( pFilter )
		{
			AkFXDesc l_TempFxDesc;
			pSound->GetFX( in_uFXIndex, l_TempFxDesc, pCtx->GetGameObjectPtr() );
			pFilter->SetBypassed( l_TempFxDesc.bIsBypassed );
		}
	}
}


//-----------------------------------------------------------------------------
// Name: AddPipeline
// Desc: Create the pipeline for the source.
//-----------------------------------------------------------------------------
AKRESULT CAkVPLSrcCbxNode::AddPipeline()
{
	AKASSERT( m_pSources[ 0 ] != NULL );
	/*AKASSERT( in_pSrcRec->m_pSrc->IsIOReady() ); */ // could be "not IO ready" due to handling of voices initially virtual

	CAkVPLNode *	  l_pNode[MAX_NODES];
	AkUInt8			  l_cCnt	= 0;
	AKRESULT		  l_eResult = AK_Success;

	CAkPBI * l_pCtx = m_pSources[ 0 ]->GetContext();
	AKASSERT( l_pCtx != NULL );

	//---------------------------------------------------------------------
	// Create the source node.
	//---------------------------------------------------------------------	 
	l_pNode[l_cCnt++] = m_pSources[ 0 ];
	AkAudioFormat l_Format = l_pCtx->GetMediaFormat();
	CAkSoundBase * pSound = l_pCtx->GetSound();

	//---------------------------------------------------------------------
	// Create the resampler/pitch node.
	//---------------------------------------------------------------------

	m_cbxRec.m_Pitch.Init( &l_Format, l_pCtx, m_uSampleRate );

	l_pNode[l_cCnt++] = &m_cbxRec.m_Pitch;

	// now that the sample conversion stage is passed, the sample rate must be the native sample rate for others components.
	l_Format.uSampleRate = m_uSampleRate;

	//---------------------------------------------------------------------
	// Create the insert effect.
	//---------------------------------------------------------------------

	for ( AkUInt32 uFXIndex = 0; uFXIndex < AK_NUM_EFFECTS_PER_OBJ; ++uFXIndex )
	{
		AkFXDesc fxDesc;
		pSound->GetFX( uFXIndex, fxDesc, l_pCtx->GetGameObjectPtr() );

		if( fxDesc.pFx )
		{
			IAkPlugin * pPlugin = NULL;
			l_eResult = CAkEffectsMgr::Alloc( AkFXMemAlloc::GetLower(), fxDesc.pFx->GetFXID(), pPlugin );
			if ( l_eResult != AK_Success ) // Note: Effect can't be created for some reason but don't kill the sound itself yet...
			{
				MONITOR_PLUGIN_ERROR( AK::Monitor::ErrorCode_PluginAllocationFailed, l_pCtx, fxDesc.pFx->GetFXID() );				
				continue;
			}

			AkPluginInfo pluginInfo;
			pPlugin->GetPluginInfo( pluginInfo );

			if ( pluginInfo.bIsAsynchronous != 
		#ifdef AK_PS3
				true
		#else
				false
		#endif
				)
			{
				MONITOR_ERROR( AK::Monitor::ErrorCode_PluginExecutionInvalid ); 	
				pPlugin->Term( AkFXMemAlloc::GetLower() );
				continue;
			}
			
			CAkVPLFilterNodeBase * pFilter;
			if ( pluginInfo.bIsInPlace )
				pFilter = AkNew( g_LEngineDefaultPoolId, CAkVPLFilterNode() );
			else
				pFilter = AkNew( g_LEngineDefaultPoolId, CAkVPLFilterNodeOutOfPlace() );

			if( pFilter == NULL )
			{ 
				MONITOR_ERROR( AK::Monitor::ErrorCode_PluginAllocationFailed );	
				l_eResult = AK_Fail; 
				pPlugin->Term( AkFXMemAlloc::GetLower() );
				goto AddPipelineError; 
			}

#ifdef _DEBUG
			AkChannelMask eOrigMask = l_Format.GetChannelMask();
#endif

			l_eResult = pFilter->Init( pPlugin, fxDesc, uFXIndex, l_pCtx, l_Format );
			AKASSERT( !pluginInfo.bIsInPlace || eOrigMask == l_Format.GetChannelMask() ); // Only out-of-place plug-ins can change channel mask.
			AKASSERT( AK::GetNumChannels( l_Format.GetChannelMask() ) <= AK_VOICE_MAX_NUM_CHANNELS );

			// Note: Effect can't be played for some reason but don't kill the sound itself yet...
			if ( l_eResult != AK_Success )
			{
				AKASSERT( pFilter );
				pFilter->Term();
				AkDelete( g_LEngineDefaultPoolId, pFilter );
				continue;
			}

			m_cbxRec.m_pFilter[ uFXIndex ] = pFilter;
			l_pNode[l_cCnt++] = pFilter;
		}
	}

	m_uChannelMask = l_Format.GetChannelMask(); // remember this -- effects can change the channel mask.
#ifdef AK_MOTION
	// Motion Feedback parameters have allocations that are channel dependent. Changing the config also requires changing the feedback.
	l_pCtx->InvalidateFeedbackParameters();
#endif

	//---------------------------------------------------------------------
	// Create the lpf node.
	//---------------------------------------------------------------------
	l_eResult = m_cbxRec.m_LPF.Init( m_uChannelMask );
	if( l_eResult != AK_Success ) 
		goto AddPipelineError;

	l_pNode[l_cCnt++] = &m_cbxRec.m_LPF;

	//---------------------------------------------------------------------
	// Create another lpf node for obstruction
	// [not connected]
	//---------------------------------------------------------------------

	l_eResult = m_ObstructionLPF.Init( m_uChannelMask );
	if( l_eResult != AK_Success ) 
		goto AddPipelineError;

	//---------------------------------------------------------------------
	// Connect the nodes.
	//---------------------------------------------------------------------
	while( --l_cCnt )
	{
		l_pNode[l_cCnt]->Connect( l_pNode[l_cCnt-1]) ;
	}

	// Make sure it is updated at least once before starting the pipeline.(WG-20417)
	RefreshBypassFx();

AddPipelineError:

	return l_eResult;
} // AddPipeline

// Compute speaker distribution with 2D positioning -> Device volume.
void CAkVPLSrcCbxNodeBase::ComputeSpeakerMatrix2D(
	bool				in_bIsAuxRoutable, 
	CAkPBI*	AK_RESTRICT	in_pContext,
	const AkVolumeDataArray & in_arVolumeData,
	AkChannelMask		in_uInputConfig,
	AkReal32 			in_fBehavioralVolume	// Collapsed volume of actor and voice bus hierarchies, and fades.
	)
{
	// In 2D positioning, there is always only one ray, with potentially multiple listeners.
	AKASSERT( in_arVolumeData.Length() >= 1 );
	const AkRayVolumeData & rVolumeData = in_arVolumeData[0];

	const AkSoundParams & params = in_pContext->GetEffectiveParams();
	//Update the LPF param now.
	for (AkDeviceInfoList::Iterator it = m_OutputDevices.Begin(); it != m_OutputDevices.End(); ++it)
		(*it)->fLPF = params.LPF;
	
	const BaseGenParams& basePosParams = in_pContext->GetBasePosParams();
	Prev2DParams& l_prev2DParams = in_pContext->GetPrevPosParams();

#ifdef AK_MOTION
	AkFeedbackParams *pFeedbackParam = in_pContext->GetFeedbackParameters();
#endif // AK_MOTION

	if( l_prev2DParams.prev2DParams	!= basePosParams
		|| in_pContext->HasPositioningTypeChanged()
		|| l_prev2DParams.prevVolume	!= in_fBehavioralVolume
		|| l_prev2DParams.prevDryLevel	!= rVolumeData.fDryMixGain
#ifdef AK_MOTION
		|| (pFeedbackParam && (l_prev2DParams.prevMotionVol != pFeedbackParam->m_NewVolume))
#endif // AK_MOTION
		|| m_bPreviousSilent	// Need to recompute volumes if was silent in previous frame.
		|| (in_pContext->GetRegisteredNotif() & AK_SpeakerVolumeMatrix) 
		|| m_bDeviceChange)
	{
		l_prev2DParams.prev2DParams		= basePosParams;
		l_prev2DParams.prevVolume		= in_fBehavioralVolume;
		l_prev2DParams.prevDryLevel		= rVolumeData.fDryMixGain;

		// Discard LFE, consider only fullband channels.
		AkUInt32 uNumChannels = AK::GetNumChannels( in_uInputConfig );
		AKASSERT( uNumChannels );
#ifdef AK_LFECENTER
		bool bSourceHasLFE = AK::HasLFE( in_uInputConfig );
		AkUInt32 uChanLFE = uNumChannels - 1;	// Only relevant if source has LFE.
#else
		bool bSourceHasLFE = false;
#endif
		AkUInt32 uNumFullBandChannels = ( bSourceHasLFE ) ? uNumChannels - 1 : uNumChannels;

		for (AkDeviceInfoList::Iterator it = m_OutputDevices.Begin(); it != m_OutputDevices.End(); ++it)
		{
			// Get volume matrix for device uDeviceID.
			AkDeviceInfo * AK_RESTRICT pVolumeMx = *it;
			pVolumeMx->fLPF = params.LPF;

			AkPanningConversion Pan( basePosParams );

			AkSIMDSpeakerVolumes volumes[AK_VOICE_MAX_NUM_CHANNELS];
			CAkSpeakerPan::GetSpeakerVolumes2DPan( Pan.fX, Pan.fY, Pan.fCenter, basePosParams.bIsPannerEnabled, in_uInputConfig, pVolumeMx->GetOutputConfig(), volumes );

			for (AkUInt32 uChan=0; uChan<uNumFullBandChannels; uChan++ )
			{
				pVolumeMx->mxDirect[uChan].Next = volumes[uChan];
				pVolumeMx->mxDirect[uChan].Next.Mul( in_fBehavioralVolume );
			}

	#ifdef AK_LFECENTER
			// Treat LFE separately.
			if ( bSourceHasLFE )
			{
				pVolumeMx->mxDirect[uChanLFE].Next.Zero();
				pVolumeMx->mxDirect[uChanLFE].Next.volumes.fLfe = in_fBehavioralVolume;
			}
	#endif // AK_LFECENTER

			pVolumeMx->mxAttenuations.dry.fNext = rVolumeData.fDryMixGain;
			if ( in_bIsAuxRoutable )
			{
				pVolumeMx->mxAttenuations.gameDef.fNext = 1.0f;
				pVolumeMx->mxAttenuations.userDef.fNext = 1.0f;
			}
		}
		/// else Volumes did not change: Nothing to do. Next volumes are already equal to previous.
	}
}

// Compute volumes of all emitter-listener pairs for this sound. 
// Returns true if routed to aux.
bool CAkVPLSrcCbxNodeBase::ComputeVolumeRays()
{
	CAkPBI * AK_RESTRICT pContext = m_pSources[ 0 ]->GetContext(); // remember the FIRST context to play in this frame
	AKASSERT( pContext != NULL );

	pContext->CalcEffectiveParams();

	bool bIsAuxRoutable = pContext->IsAuxRoutable();
	m_bIsAuxRoutable = bIsAuxRoutable;	// cache value on Cbx
	if( !pContext->IsForcedVirtualized() )
	{
		// Get volumes coming from actor-mixer structure and bus "voice volume". Apply fading.
		m_fBehavioralVolume = pContext->ComputeCollapsedVoiceVolume();

		if( pContext->GetPannerType() == Ak2D )
		{
			// In the case of 2D positioning, we just want a volume data item with zeroed angles and distance 
			// (could be useful in the future), and a correctly set listener mask for device routing. 
			// Since the distance/angles don't change, we allocate / add it once to the array.
			if ( AK_EXPECT_FALSE( m_arVolumeData.IsEmpty() ) )
			{
				if ( !m_arVolumeData.AddLast() )
					return false;
			}
			m_arVolumeData[0].SetListenerMask( (AkUInt8)pContext->GetGameObjectPtr()->GetListenerMask() );

			AkReal32 fDryLevel = bIsAuxRoutable ? pContext->GetDryLevelValue() : 1.0f;
			fDryLevel *= pContext->GetOutputBusVolumeValue();

			m_arVolumeData[0].fDryMixGain = fDryLevel;
		}
		else
		{
			// 3D: Compute 3D emitter-listener pairs and get volumes for each of them
			AkPositionSourceType ePosType = pContext->GetPositionSourceType();
			AkUInt32 uNumRays = pContext->ComputeVolumeData3D( ePosType, m_arVolumeData );
			if ( uNumRays )
			{
				CAkListener::Get3DVolumes(
					ePosType, 
					bIsAuxRoutable,
					pContext,
					m_arVolumeData
					);
			}
		}
	}
	else
	{
		m_arVolumeData.RemoveAll();
		pContext->VirtualPositionUpdate();
	}

	// Compute send values for this frame.  Compute sends as long as the voice is audible.  
	// At this point m_bAudible tells the state of the voice in the LAST frame.  
	// If it was audible, it will need to fade out for a frame before going virtual, even when forced.
	if ( bIsAuxRoutable && m_bAudible )
	{
		AkAuxSendValueEx environmentValues[ AK_MAX_AUX_SUPPORTED ];
		pContext->GetAuxSendsValues( environmentValues );
		MergeLastAndCurrentValues( environmentValues, m_arSendValues, m_bFirstSendsCalculated, m_uNumSends, this );
		m_bFirstSendsCalculated = true;
		return true;
	}
	else
		m_uNumSends = 0;

	return false;
}

// Compute max volume of all paths (auxiliary and dry), taking pre-computed
// bus gains into account.
void CAkVPLSrcCbxNodeBase::ComputeMaxVolume()
{
	AkReal32 fBehavioralVolume = m_fBehavioralVolume;
	if ( !m_bIsAuxRoutable )
	{
		for(AkDeviceInfoList::Iterator itConnection = m_OutputDevices.Begin(); itConnection != m_OutputDevices.End(); ++itConnection)
		{
			AkReal32 fMaxVolume = 0;
			AkDevice * pDevice = CAkOutputMgr::GetDevice( (*itConnection)->GetDeviceID() );
			if ( !(*itConnection)->bCrossDeviceSend && pDevice != NULL)
			{				
				AkUInt32 uDeviceListeners = pDevice->uListeners;
#ifdef AK_VITA_HW
				AkReal32 fVolumeDirect = fBehavioralVolume;
#else
				AkReal32 fVolumeDirect = fBehavioralVolume * (*itConnection)->pMixBus->m_fDownstreamGain;
#endif				

				// Check all rays that apply to this device.
				AkVolumeDataArray::Iterator it = m_arVolumeData.Begin();
				while ( it != m_arVolumeData.End() )
				{
					if ( (*it).ListenerMask() & uDeviceListeners )
					{
						// Check dry path.
						AkReal32 fDryVolume = fVolumeDirect * (*it).fDryMixGain;
						if ( fMaxVolume < fDryVolume )
							fMaxVolume = fDryVolume;
					}
					++it;
				}
			}
			(*itConnection)->fMaxVolume = fMaxVolume;
		}
	}
	else
	{
		for(AkDeviceInfoList::Iterator itConnection = m_OutputDevices.Begin(); itConnection != m_OutputDevices.End(); ++itConnection)
		{
			AkReal32 fMaxVolume = 0;
			AkReal32 fMaxAuxGameDefMix = 0;
			AkReal32 fMaxAuxUserDefMix = 0;
		
			AkDevice * pDevice = CAkOutputMgr::GetDevice( (*itConnection)->GetDeviceID() );
			if (pDevice == NULL)
			{
				(*itConnection)->fMaxVolume = 0.f;
				continue;	//Device disconnected. 
			}

			AkUInt32 uDeviceListeners = pDevice->uListeners;

#ifdef AK_VITA_HW
			AkReal32 fVolumeDirect = fBehavioralVolume;
#else
			AkReal32 fVolumeDirect = fBehavioralVolume * (*itConnection)->pMixBus->m_fDownstreamGain;
#endif

			// Check all rays that apply to this device.
			AkVolumeDataArray::Iterator it = m_arVolumeData.Begin();
			while ( it != m_arVolumeData.End() )
			{
				if ( (*it).ListenerMask() & uDeviceListeners )
				{
					// Get max dry path in fMaxVolume, and find and max auxiliary gains.
					AkReal32 fDryVolume = fVolumeDirect * (*it).fDryMixGain;
					if ( fMaxVolume < fDryVolume )
						fMaxVolume = fDryVolume;
					fMaxAuxGameDefMix = AkMax( fMaxAuxGameDefMix, (*it).fGameDefAuxMixGain );
					fMaxAuxUserDefMix = AkMax( fMaxAuxUserDefMix, (*it).fUserDefAuxMixGain );
				}
				++it;
			}

			// Clear max volume value obtained from direct path if the connection is of type "cross-device send".
			if ( AK_EXPECT_FALSE( (*itConnection)->bCrossDeviceSend ) )
				fMaxVolume = 0;
			
			// Check for auxiliary send values.
			fMaxAuxGameDefMix *= fBehavioralVolume;
			fMaxAuxUserDefMix *= fBehavioralVolume;
			AkUInt8 uNumSends;
			const AkMergedEnvironmentValue * pSends = GetSendValues( uNumSends );
			for( int i = 0; i < m_uNumSends; i++ )
			{
				for(AkDeviceVPLArray::Iterator it = pSends[i].PerDeviceAuxBusses.Begin(); it != pSends[i].PerDeviceAuxBusses.End(); ++it)			
				{
					// Check if this aux bus applies to connection being inspected.
					if ((*itConnection)->uDeviceID == (*it).key)
					{
						// It does. Collapse all gains for this send.
						AkAuxType eType = pSends[i].eAuxType;
						AkReal32 fSendVolume = ( eType == AkAuxType_GameDef ) ? fMaxAuxGameDefMix : fMaxAuxUserDefMix;
						AkReal32 fTreeGain = pSends[i].fControlValue * (*it).item->m_fDownstreamGain;
						fSendVolume *= fTreeGain;

						// Compare with max volume.
						if ( fMaxVolume < fSendVolume )
							fMaxVolume = fSendVolume;
						break;
					}
				}
			}
			(*itConnection)->fMaxVolume = fMaxVolume;
		}
	}

	// HDR.
	if ( GetHdrBus() )
	{
		// Got an HDR bus. Perform HDR processing if we are connected to the main device.
		AkDeviceInfo *pMainDeviceVol = m_OutputDevices.GetVolumesByID(AK_MAIN_OUTPUT_DEVICE);
		if ( pMainDeviceVol )
		{
			// Compute volume as seen by HDR bus. It depends on the voice dynamic range and the estimated envelope.
			/// In dBs. 
			CAkPBI * AK_RESTRICT pContext = m_pSources[ 0 ]->GetContext();
			AKASSERT( pContext != NULL );
			const AkSoundParams & params = pContext->GetEffectiveParams();
			
			// If relative loudness is below a certain threshold, do not consider this sound when computing the bus's HDR window.
			// Note: relative loudness is always negative, so we negate it before checking it against the voice dynamic range.
			AkReal32 fMaxVolumeDB = AkMath::FastLinTodB( pMainDeviceVol->fMaxVolume );
			// Store for later use.
			m_fMaxVolumeDB = fMaxVolumeDB;
			
			// Push volume to HDR bus only if within the region of interest.
			if ( params.hdr.bEnableEnvelope )
			{
				AkReal32 fActiveRange = params.hdr.fActiveRange;
				AkReal32 fRelativeLoudness = GetAnalyzedEnvelope();
				if ( -fRelativeLoudness < fActiveRange )
					GetHdrBus()->PushEffectiveVoiceVolume( fMaxVolumeDB + fRelativeLoudness );
#ifndef AK_OPTIMIZED
				m_fLastEnvelope = fRelativeLoudness;
#endif
			}
			else
			{
				// No envelope.
				GetHdrBus()->PushEffectiveVoiceVolume( fMaxVolumeDB );
#ifndef AK_OPTIMIZED
				m_fLastEnvelope = 0;
#endif
			}
		}
	}
}

void CAkVPLSrcCbxNodeBase::GetVolumes( 
	bool in_bIsAuxRoutable, 
	CAkPBI* AK_RESTRICT in_pContext,
	AkChannelMask in_uInputConfig,
	bool & out_bNextSilent,		// True if sound is under threshold at the end of the frame
	bool & out_bAudible,		// False if sound is under threshold for whole frame: then it may be stopped or processsed as virtual.
	AkReal32 &out_fLPF,
	AkReal32 &out_fObsLPF
	)
{

	//Init LPF with valid values.
	out_fLPF = AK_MIN_LOPASS_VALUE;
	out_fObsLPF = AK_MIN_LOPASS_VALUE;

	// m_fBehavioralVolume on stack: loudness normalization needs to remain separate for monitoring.
	AkReal32 fBehavioralVolume = m_fBehavioralVolume;
	AkReal32 fMaxVolume = 0.f;
	AkDeviceInfo *pMainDeviceVol = m_OutputDevices.GetVolumesByID(AK_MAIN_OUTPUT_DEVICE);
	if ( GetHdrBus() && pMainDeviceVol ) //HDR is only on the main hierarchy.
	{
		fMaxVolume = pMainDeviceVol->fMaxVolume;

		// Take the max between the bus window top and this voice's peak value.
		// Note: the voice peak has to be transformed through the HDR gain computer for
		// a proper comparison. 
		AkReal32 fVoiceHdrPeak = GetHdrBus()->GetMaxVoiceWindowTop( m_fMaxVolumeDB );
		AkReal32 fWindowTop = GetHdrBus()->GetHdrWindowTop();
		if ( fVoiceHdrPeak > fWindowTop )
		{
			// When this voice's peak is larger than fGlobalWindowTop, this voice has 
			// "detached" from the HDR bus attenuation. Compute its own independent window top.
			fWindowTop = fVoiceHdrPeak;
		}

		// Compute HDR attenuation:
		// HDR gain (in dB) = Voice peak - Window top - (Max volume - HDR bus downstream gain).
		// The part in parentheses represents the difference between the actual voice volume and the volume that 
		// is seen by the HDR bus (in other words, all submixing stages between this voice and the HDR bus).
		// Since Voice Peak = Max Volume, we can reduce the formula to
		// HDR gain = Hdr downstream gain - Window top + Behavioral volume.
		AkReal32 fHdrGain = AkMath::dBToLin( -fWindowTop ) * GetHdrBus()->m_fDownstreamGain;

		// Update volumes.
		fBehavioralVolume *= fHdrGain;
		fMaxVolume *= fHdrGain;
		m_fBehavioralVolume = fBehavioralVolume;
		pMainDeviceVol->fMaxVolume = fMaxVolume;
#ifndef AK_OPTIMIZED
		m_fWindowTop = fWindowTop;
#endif
	}
	
	//To test for audibility use the max of all devices!
	for(AkDeviceInfoList::Iterator itDeviceVol = m_OutputDevices.Begin(); itDeviceVol != m_OutputDevices.End(); ++itDeviceVol)
		fMaxVolume = AkMax(fMaxVolume, (*itDeviceVol)->fMaxVolume);

	// Next silent. 
	bool bNextSilent = ( fMaxVolume <= g_fVolumeThreshold ) || ( m_arVolumeData.Length() == 0 );

	// Add makeup gain and loudness normalization, after evaluating against threshold.
	// fBehavioralVolume is updated after having been stored in m_fBehavioralVolume in order to avoid
	// reporting normalization gain in voice volume.
	AkReal32 fMakeUpGain = m_pSources[ 0 ]->GetMakeupGain();
	fBehavioralVolume *= fMakeUpGain;
#ifndef AK_OPTIMIZED
	m_fNormalizationGain = fMakeUpGain;	// Store for monitoring.
#endif

	// Test for audibility. 
	if ( bNextSilent && ( m_bPreviousSilent || !m_bFirstBufferProcessed ) )
	{
		// Not audible. Bail out now.
		m_bPreviousSilent = true;	// force to true if first run.
		out_bAudible = false;
		out_bNextSilent = bNextSilent;

		// Ensure motion is silent.
#ifdef AK_MOTION
		AkFeedbackParams *pFeedbackParam = in_pContext->GetFeedbackParameters();
		if (AK_EXPECT_FALSE( pFeedbackParam != NULL ))
			pFeedbackParam->ZeroNewVolumes();
#endif
		
		return;
	}

	out_bAudible = true;

	// Prepare speaker matrix for this frame. Move new "Next" to "Previous".
	AkUInt32 uNumChannels = AK::GetNumChannels( in_uInputConfig );
	m_OutputDevices.Reset( uNumChannels );

	if ( !bNextSilent )
	{
		// without 3D
		if( in_pContext->GetPannerType() == Ak2D )
		{
			ComputeSpeakerMatrix2D(
				in_bIsAuxRoutable, 
				in_pContext,
				m_arVolumeData,
				in_uInputConfig,
				fBehavioralVolume	// Collapsed volumes of actor and voice bus hierarchies, fades, and HDR attenuation
				);
		}
		// with 3D
		else
		{
			CAkListener::ComputeSpeakerMatrix(
				in_bIsAuxRoutable, 
				in_pContext,
				m_arVolumeData,
				in_uInputConfig,
				fBehavioralVolume,	// Collapsed volumes of actor and voice bus hierarchies, fades, and HDR attenuation
				m_OutputDevices
				);
		}

		in_pContext->ResetPositioningTypeChanged();

		if (HasOutputDevice())
		{
			out_fLPF = AK_MAX_LOPASS_VALUE;
			out_fObsLPF = AK_MAX_LOPASS_VALUE;
			for (AkDeviceInfoList::Iterator it = m_OutputDevices.Begin(); it != m_OutputDevices.End(); ++it)
			{				
				AkDeviceInfo & volumesDevice = **it;

				out_fLPF = AkMath::Min(out_fLPF, volumesDevice.fLPF);
				out_fObsLPF = AkMath::Min(out_fObsLPF, volumesDevice.fObsLPF);

	#ifdef AK_MOTION
				//Compute the motion volumes for motion-only sources.
				AkFeedbackParams *pFeedbackParam = in_pContext->GetFeedbackParameters();
				if (AK_EXPECT_FALSE( pFeedbackParam != NULL ))
				{
					// Motion require that speaker volumes are completely computed in order to determine if it 
					// is silent. Push volumes to motion, and check if it decided that it was silent.
					/// TEMP Using one and only device.
					pFeedbackParam->ComputePlayerVolumes(in_pContext);
					bNextSilent &= pFeedbackParam->m_bSilent;
					if ( bNextSilent )
					{
						// Clear next volume values if we are going to be "silent" (under threshold). This forces 
						// a nice fade out when the threshold is above -96dB.
						m_OutputDevices.ClearNext( uNumChannels );
						if ( m_bPreviousSilent || !m_bFirstBufferProcessed )
						{
							// Not audible after all.
							m_bPreviousSilent = true;	// force to true if first run.
							out_bAudible = false;
						}					
					}
				}
	#endif // AK_MOTION

				// Send callback notification to allow user modifications to volume
				if ( in_pContext->GetRegisteredNotif() & AK_SpeakerVolumeMatrix )
					CAkListener::DoSpeakerVolumeMatrixCallback( in_pContext->GetPlayingID(), uNumChannels, in_uInputConfig, volumesDevice.GetOutputConfig(), volumesDevice );
				
				if ( m_bPreviousSilent )
				{
					// Not audible during last frame: Clear previous volume values to avoid any glitch
					// (usually happens with streamed sounds with frame offset larger than audio frame).
					volumesDevice.ClearPrev( uNumChannels );
				}
				else if ( !m_bFirstBufferProcessed )
				{
					// First run: set the previous volume values equal to the next values.
					volumesDevice.Reset( uNumChannels );
				}
				
#ifndef AK_OPTIMIZED
				if ( AK_EXPECT_FALSE( in_pContext->IsMonitoringMute() ) )
				{					
					volumesDevice.ZeroAll();

					// Ensure volumes are recomputed in 2D mode.
					in_pContext->GetPrevPosParams().Invalidate();
				}
#endif
			}
		}
		else
		{
			// Failed allocating volumes for any device. Pretend we're silent.
			out_bAudible = false;
			bNextSilent = true;
		}
	}
	else // next silent
	{
		// Clear next volume values if we are going to be "silent" (under threshold). This forces 
		// a nice fade out when the threshold is above -96dB.
		AKASSERT( !m_bPreviousSilent );	// would have bailed out already
		m_OutputDevices.ClearNext( uNumChannels );
	}

	m_bDeviceChange = false;

	out_bNextSilent = bNextSilent;

} //GetVolumes

//-----------------------------------------------------------------------------
// Name: AddSrc
// Desc: Add a source.
//
// Parameters:
//	CAkPBI * in_pCtx    : Context to add.
//	bool			  in_bActive : true=source should be played. 
//
// Return:
//	Ak_Success :		    Source was added and pipeline created.
//  AK_InsufficientMemory : Error memory not available.
//  AK_NoDataReady :        IO not completed.
//-----------------------------------------------------------------------------
AKRESULT CAkVPLSrcCbxNodeBase::AddSrc( CAkPBI * in_pCtx, bool in_bActive )
{
	CAkVPLSrcNode * pSrc = CAkVPLSrcNode::Create( in_pCtx );
	if ( pSrc )
		return AddSrc( pSrc, in_bActive, true );

	return AK_Fail;
}

AKRESULT CAkVPLSrcCbxNodeBase::AddSrc( CAkVPLSrcNode * in_pSrc, bool in_bActive, bool in_bFirstTime )
{
	AKRESULT l_eResult = AK_Success;

	CAkPBI * pCtx = in_pSrc->GetContext();
	pCtx->SetCbx( this );

	// Dont get this info if we are the sequel of a sample accurate container, they must all share the behavior of the first sound.
	if( in_bActive && in_bFirstTime )
	{
		m_eBelowThresholdBehavior = pCtx->GetVirtualBehavior( m_eVirtualBehavior );

		// Allocate a slot for one emitter-listener pair volume (common case). 
		// Multi-listener / position will require more.
		if ( m_arVolumeData.Reserved() == 0 )
		{
			if ( AK_EXPECT_FALSE( m_arVolumeData.Reserve( 1 ) != AK_Success ) )
				l_eResult = AK_Fail;
		}
	}

	if ( l_eResult == AK_Success )
	{
		//Do check here to start stream only if required
		bool bIsUnderThreshold = false;
		if( m_eBelowThresholdBehavior != AkBelowThresholdBehavior_ContinueToPlay )
		{
			bIsUnderThreshold = pCtx->IsInitiallyUnderThreshold( m_arVolumeData );
		}

		if( bIsUnderThreshold && m_eBelowThresholdBehavior == AkBelowThresholdBehavior_KillVoice )
		{
			pCtx->Monitor(AkMonitorData::NotificationReason_KilledVolumeUnderThreshold);
			l_eResult = AK_PartialSuccess;
		}
		// Must check in_bActive here, if not active, it is because we are in sample accurate mode, and in this mode, we have no choice but to keep streaming
		else if( bIsUnderThreshold && m_eVirtualBehavior == AkVirtualQueueBehavior_FromBeginning && in_bActive && in_bFirstTime )
		{
			l_eResult = AK_Success;//We fake success so the pipeline gets connected.

			// IMPORTANT. m_bAudible is used to know when we need to become virtual (call VirtualOn()). 
			// In this case - PlayFromBeginning mode, starting virtual - we skip the virtual handling mechanism:
			// do not start stream. 
			SetAudible( pCtx, false );
		}
		else
		{
			l_eResult = in_pSrc->FetchStreamedData();
		}
	}

	// The source was created/initialized successfully, but I/O could still be pending.
	if( l_eResult == AK_Success || l_eResult == AK_FormatNotReady )
	{
		if ( in_bActive )
		{
			m_bPreviousSilent = pCtx->NeedsFadeIn();
			AKASSERT( !m_pSources[ 0 ] );
			m_pSources[ 0 ] = in_pSrc;
		}
		else
		{
			AKASSERT( !m_pSources[ 1 ] );
			m_pSources[ 1 ] = in_pSrc;
		}

		return l_eResult;
	}

	// Failure case: clear source.

	in_pSrc->Term( CtxDestroyReasonPlayFailed );
	AkDelete( g_LEngineDefaultPoolId, in_pSrc );

	return l_eResult;
} // AddSrc

void CAkVPLSrcCbxNodeBase::AddOutputBus( AkVPL *in_pVPL, AkOutputDeviceID in_uDeviceID, bool bCrossDeviceSend )
{		
	m_bDeviceChange = true;
	AkDeviceInfo * pDeviceOutput = m_OutputDevices.CreateDevice( in_pVPL, in_uDeviceID, bCrossDeviceSend );
	if ( pDeviceOutput )
	{
		AkDevice * pDevice = CAkOutputMgr::GetDevice( in_uDeviceID );
		AKASSERT( pDevice );	// Must exist otherwise m_OutputDevices.CreateDevice() would have failed.
		if ( pDevice->EnsurePanCacheExists( pDeviceOutput->GetOutputConfig() ) != AK_Success )
		{
			// Failure: remove device, because pipeline processing takes for granted that pan caches exist.
			for (AkDeviceInfoList::IteratorEx it = BeginBusEx(); it != EndBus();++it)
			{
				if ((*it)->pMixBus = in_pVPL)
				{
					RemoveOutputBus( it );
					break;
				}
			}
		}
	}
}

void CAkVPLSrcCbxNode::RestorePreviousVolumes( AkPipelineBuffer* AK_RESTRICT io_pBuffer )
{
	GetContext()->GetPrevPosParams().Invalidate();
}

void CAkVPLSrcCbxNode::StopLooping( CAkPBI * in_pCtx )
{
	if ( m_pSources[ 0 ] && in_pCtx == m_pSources[ 0 ]->GetContext() )
	{
		if ( !m_pSources[ 0 ]->IsIOReady() ||
			m_pSources[ 0 ]->StopLooping() != AK_Success )
		{
			// When StopLooping return an error, it means that the source must be stopped.
			// ( occurs with audio input plug-in only actually )
			Stop();
		}
	}
	else if ( m_pSources[ 1 ] && in_pCtx == m_pSources[ 1 ]->GetContext() ) 
	{
		m_pSources[ 1 ]->Term( CtxDestroyReasonFinished );
		AkDelete( g_LEngineDefaultPoolId, m_pSources[ 1 ] );
		m_pSources[ 1 ] = NULL;
	}
}

void CAkVPLSrcCbxNode::SeekSource()
{
	// Bail out if FromBeginning: the behavior is undefined. Some sources seek or prepare their stream
	// when becoming virtual, others when becoming physical.
	// The source offset on the PBI needs to be cleared.
	if ( AkVirtualQueueBehavior_FromBeginning == m_eVirtualBehavior
		&& AkBelowThresholdBehavior_SetAsVirtualVoice == m_eBelowThresholdBehavior )
	{
		CAkPBI * pCtx = GetContext();
		if ( pCtx )
			pCtx->SetSourceOffsetRemainder( 0 );
		return;
	}

	// If the source is not ready, seeking will be handled in StartStream().
	if ( m_pSources[ 0 ]
		&& m_pSources[ 0 ]->IsIOReady() )
	{
		ReleaseBuffer();

		if( m_cbxRec.Head()->Seek() != AK_Success )
		{
			// There was an error. Stop.
			Stop();
			return;
		}
	}
}

AKRESULT CAkVPLSrcCbxNode::SourceTimeSkip( AkUInt32 in_uMaxFrames )
{
	AKRESULT eReturn = m_cbxRec.Head()->TimeSkip( in_uMaxFrames ); // TimeLapse modified param according to the number of frames actually elapsed.

	if( eReturn == AK_Fail )
	{		
		if ( m_pSources[ 0 ] )
		{
			MONITOR_SOURCE_ERROR( AK::Monitor::ErrorCode_CannotPlaySource_TimeSkip, m_pSources[ 0 ]->GetContext() );
		}
	}

	return eReturn;
}

bool CAkVPLSrcCbxNode::IsUsingThisSlot( const CAkUsageSlot* in_pUsageSlot )
{
	for ( AkUInt32 uFXIndex = 0; uFXIndex < AK_NUM_EFFECTS_PER_OBJ; ++uFXIndex )
	{
		if( m_cbxRec.m_pFilter[ uFXIndex ] && m_cbxRec.m_pFilter[ uFXIndex ]->IsUsingThisSlot( in_pUsageSlot ) )
			return true;
	}

	return false;
}

bool CAkVPLSrcCbxNode::IsUsingThisSlot( const AkUInt8* in_pData )
{
	for ( AkUInt32 uFXIndex = 0; uFXIndex < AK_NUM_EFFECTS_PER_OBJ; ++uFXIndex )
	{
		if( m_cbxRec.m_pFilter[ uFXIndex ] && m_cbxRec.m_pFilter[ uFXIndex ]->IsUsingThisSlot( in_pData ) )
			return true;
	}

	return false;
}

void CAkVPLSrcCbxNode::RemovePipeline( AkCtxDestroyReason in_eReason )
{
	if ( m_pSources[ 0 ] )
	{
		m_pSources[ 0 ]->Term( in_eReason );
		AkDelete( g_LEngineDefaultPoolId, m_pSources[ 0 ] );
		m_pSources[ 0 ] = NULL;
	}

	m_cbxRec.ClearVPL();
	m_ObstructionLPF.Term();
	m_bAudible = true;
}

void CAkVPLSrcCbxNode::RelocateMedia( AkUInt8* in_pNewMedia,  AkUInt8* in_pOldMedia )
{
	m_cbxRec.m_Pitch.RelocateMedia( in_pNewMedia, in_pOldMedia );
}
