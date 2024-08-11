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
// AkVPLFinalMixNode.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkVPLFinalMixNode.h"
#include "AudiolibDefs.h"
#include "AkLEngine.h"
#include "AkBus.h"
#include "AkMeterTools.h"

// Effect
#include "AkEffectsMgr.h"
#include "AkEnvironmentsMgr.h"
#include "AkFXMemAlloc.h"
#include "AkMonitor.h"
#include "AkAudioLibTimer.h"
#include "AkMath.h"
#include "AkOutputMgr.h"

#ifdef AK_71FROM51MIXER
#include "AkProfile.h"
#endif

//-----------------------------------------------------------------------------
// Name: Init
// Desc: Initialize the source.
//-----------------------------------------------------------------------------
AKRESULT CAkVPLFinalMixNode::Init( AkUInt32 in_uChannelMask )
{
	InitPan( NULL, in_uChannelMask, in_uChannelMask );
	m_Mixer.Init( LE_MAX_FRAMES_PER_BUFFER );

#ifdef AK_VITA_HW
	m_bUpdateVolumeOnNextFrame = true;
#endif

	for ( AkUInt32 uFXIndex = 0; uFXIndex < AK_NUM_EFFECTS_PER_OBJ; ++uFXIndex )
	{
		FX & fx = m_aFX[ uFXIndex ];
		fx.id = AK_INVALID_PLUGINID;
		fx.pParam = NULL;
		fx.pEffect = NULL;
		fx.pBusFXContext = NULL;
		fx.bBypass = 0;
		fx.bLastBypass = 0;
	}
	m_bBypassAllFX = 0;
	m_bLastBypassAllFX = 0;

	ResetVolumes();
	m_eState = NodeStateStop;
	m_BufferOut.Clear();
	m_BufferOut.eState = AK_NoMoreData;

	m_ulBufferOutSize = LE_MAX_FRAMES_PER_BUFFER * GetNumChannels( in_uChannelMask ) * sizeof(AkReal32);
	void * pData = AkMalign( g_LEngineDefaultPoolId, m_ulBufferOutSize, AK_BUFFER_ALIGNMENT );
	if ( !pData )
		return AK_InsufficientMemory;
	
	AkZeroMemAligned( pData, m_ulBufferOutSize );
	m_BufferOut.AttachContiguousDeinterleavedData( 
		pData,						// Buffer.
		LE_MAX_FRAMES_PER_BUFFER,	// Buffer size (in sample frames).
		0,							// Valid frames.
		in_uChannelMask );			// Chan config.
	
#ifndef AK_OPTIMIZED
	m_uMixingVoiceCount = 0;
#endif

#if defined (AK_PS3 )
	m_pLastItemMix = NULL;
#endif

#ifdef AK_VITA_HW
	m_bInitFx = true;
#endif
	return AK_Success;
} // Init

//-----------------------------------------------------------------------------
// Name: Term
// Desc: Terminate.
//
// Parameters:
//
// Return:
//	AKRESULT
//		AK_Success : terminated successfully.
//		AK_Fail    : failed.
//-----------------------------------------------------------------------------
AKRESULT CAkVPLFinalMixNode::Term()
{
	if( m_BufferOut.HasData() )
	{
		AkFalign( g_LEngineDefaultPoolId, m_BufferOut.GetContiguousDeinterleavedData() );
		m_BufferOut.ClearData();
	}

	DropFx();

	return AK_Success;
} // Term

//-----------------------------------------------------------------------------
// Name: ReleaseBuffer
// Desc: Releases a processed buffer.
//
// Parameters:
//	AkAudioBuffer* io_pBuffer : Pointer to the buffer object to release.
//
// Return:
//	Ak_Success: Buffer was relesed.
//  AK_Fail:    Failed to release the buffer.
//-----------------------------------------------------------------------------
AKRESULT CAkVPLFinalMixNode::ReleaseBuffer()
{
	// Assume output buffer was entirely consumed by client. Do not actually release the memory here.
	// Clear markers for next iteration
	m_BufferOut.uValidFrames = 0;

	if ( m_BufferOut.eState == AK_NoMoreData )
	{
		m_eState = NodeStateStop;
	}
	else
	{
		m_eState = NodeStatePlay;
	}
		
	// Reset state in case AddAndMixBuffer does not get called again
	m_BufferOut.eState = AK_NoMoreData;

	// Also clean the data for next iteration.
	if ( m_BufferOut.HasData() )
		AkZeroMemAligned(m_BufferOut.GetContiguousDeinterleavedData(), m_ulBufferOutSize);
	// no need to clear m_MasterOut buffer since the data will be overwritten on final mix

	return AK_Success;
} // ReleaseBuffer

//-----------------------------------------------------------------------------
// Name: Connect
// Desc: Connects the specified effect as input.
//
// Parameters:
//	CAkVPLNode * in_pInput : Pointer to the effect to connect.
//-----------------------------------------------------------------------------
void CAkVPLFinalMixNode::Connect( CAkVPLMixBusNode * in_pInput )
{
	if ( m_eState != NodeStatePlay && CAkOutputMgr::GetDevice(AK_MAIN_OUTPUT_DEVICE)->pFinalMix == this)
	{
		SetNextVolume( g_MasterBusCtx.GetVolume( BusVolumeType_ToNextBusWithEffect ) );
		TagPreviousVolumes();
	}
} // Connect

#ifdef AK_VITA_HW
void CAkVPLFinalMixNode::ConnectMasterBusFx()
{
	if (m_bInitFx)
	{
		SetAllInsertFx();
		m_bInitFx = false;
	}
}
#endif

//-----------------------------------------------------------------------------
// Name: ConsumeBuffer
// Desc: Mix input buffer with current output buffer
//
// Parameters:
//	AkAudioBuffer* io_pBuffer : Pointer to a buffer object.
//
// Return:
//	Ak_Success: Buffer was returned.
//  AK_Fail:    Failed to return a buffer.
//-----------------------------------------------------------------------------
AKRESULT CAkVPLFinalMixNode::ConsumeBuffer( 
			AkAudioBufferBus* 		io_rpBuffer,
			bool					in_bPan,
			AkAudioMix				in_PanMix[]
			)
{
	AKRESULT	l_eResult = AK_Success;
	// The tail of the mix bus from which this is coming from is not considered here on PS3
#ifndef AK_PS3
	if( io_rpBuffer->uValidFrames > 0 )
#endif	
	{
#ifndef AK_OPTIMIZED
		IncrementMixingVoiceCount();
#endif
		// Set plugin audio buffer eState before execution (it will get updated by the effect chain)
		m_BufferOut.eState = AK_DataReady;
	
		// Master bus is revived. Initialize the effect on it if necessary to consider parameter changes.
		if ( m_eState == NodeStateStop )
		{
			// This can fail, but if it does we still want to mix the buffer without the effect
			// It also cleans up for itself and the node will know no effect need to be inserted
			SetAllInsertFx();	
		}
		m_eState = NodeStatePlay;
#ifdef AK_PS3
		if ( m_pLastItemMix == NULL )
		{
			// First mixer to be added to final mix: push job (m_Mixer.FinalExecuteSPU() blocks on job chain).
			m_Mixer.ExecuteSPU(io_rpBuffer, m_BufferOut, in_PanMix);
		}
		else
		{
			m_pLastItemMix->pNextItemMix = io_rpBuffer;
			m_pLastItemMix->pNextItemVolumes = in_PanMix;
			m_pLastItemMix->pNextVolumeAttenuation = NULL;
		}

		m_pLastItemMix = io_rpBuffer;
		io_rpBuffer->pNextItemMix = NULL;
		io_rpBuffer->pNextItemVolumes = NULL;
		io_rpBuffer->pNextVolumeAttenuation = NULL;
		
#else
		// apply the volumes and final mix with previous buffer (if any)
		m_Mixer.Mix( io_rpBuffer, &m_BufferOut, in_bPan, in_PanMix );
#endif
	}
	return l_eResult;
}

//-----------------------------------------------------------------------------
// Name: GetResultingBuffer
// Desc: Get the resulting mixed buffer.
//-----------------------------------------------------------------------------
void CAkVPLFinalMixNode::GetResultingBuffer( AkPipelineBufferBase& io_rBuffer
#ifdef AK_PS4
	, bool in_bForce71
#endif
	)
{ 
	// Execute the master bus insert effect.
	if( m_eState == NodeStatePlay )
	{
		for ( AkUInt32 uFXIndex = 0; uFXIndex < AK_NUM_EFFECTS_PER_OBJ; ++uFXIndex )
		{
			FX & fx = m_aFX[ uFXIndex ];
			if ( fx.pEffect != NULL )
			{
				if ( ( fx.bBypass | m_bBypassAllFX ) == 0 )
				{
					// Ensure SIMD can be used without additional considerations
					AKASSERT( m_BufferOut.MaxFrames() % 4 == 0 );

				#ifdef AK_PS3
					AK::MultiCoreServices::DspProcess* pDspProcess = NULL;
					fx.pEffect->Execute( &m_BufferOut, pDspProcess );
					if ( pDspProcess )
					{
						CAkLEngine::QueueDspProcess(pDspProcess); // needs to be declared sync so that it happens in sequence after mixing of busses into final
						CAkLEngine::QueueSync();
					}	
				#else
					AK_START_PLUGIN_TIMER( fx.id );
					fx.pEffect->Execute( &m_BufferOut );
					AK_STOP_PLUGIN_TIMER( fx.id );
					AKASSERT( m_BufferOut.uValidFrames <= m_BufferOut.MaxFrames() );	// Produce <= than requested
				#endif	

					AKSIMD_ASSERTFLUSHZEROMODE;
				}
				else
				{
					if ( ( fx.bLastBypass | m_bLastBypassAllFX ) == 0 )
					{
						fx.pEffect->Reset( );
					}
				}
				fx.bLastBypass = fx.bBypass;
			}
		}

		m_bLastBypassAllFX = m_bBypassAllFX;

		// Declare output only when playing (so that we can revert to PassSilence mode)
		io_rBuffer.uValidFrames = m_BufferOut.MaxFrames();
	}

	m_BufferOut.m_fNextVolume = GetNextVolume();
	m_BufferOut.m_fPreviousVolume = GetPreviousVolume();

#ifdef AK_PS3
		// this will queue the volume job
#ifndef AK_OPTIMIZED
	m_BufferOut.pMeterCtx = m_pMeterCtx;
#endif
		m_Mixer.FinalInterleaveExecuteSPU( &m_BufferOut, &io_rBuffer );
#elif defined(AK_APPLE)	
		m_Mixer.ProcessVolume( &m_BufferOut, &io_rBuffer );
#elif defined (AK_WIIU)
		//The WiiU takes de-interleaved data.
	io_rBuffer.AttachContiguousDeinterleavedData(m_BufferOut.GetContiguousDeinterleavedData(), io_rBuffer.MaxFrames(), io_rBuffer.uValidFrames, io_rBuffer.GetChannelMask());
#else

	// IMPORTANT: Only platforms whose sink has a separate ring buffer (hence requiring a copy of the buffer passed 
	// to PassData()) can avoid the final mix. Otherwise, this mixing step is required in order to clear the sink's buffer
	// when the engine is silent.
#ifdef AK_ANDROID
	if (io_rBuffer.uValidFrames > 0)
#endif
	{
		// apply the volumes and final mix (overwriting destination buffer)
		switch( m_BufferOut.GetChannelMask() )
		{
		case AK_SPEAKER_SETUP_STEREO :
#if defined( AK_71FROMSTEREOMIXER )
			// On some platforms, 5.1 needs to be interleaved into 7.1 channels
			if (!in_bForce71 || AK_PERF_OFFLINE_RENDERING)
			{
				m_Mixer.MixFinalStereo( &m_BufferOut, &io_rBuffer );
			}
			else
			{
				m_Mixer.MixFinal71FromStereo( &m_BufferOut, &io_rBuffer );
			}
#else
			m_Mixer.MixFinalStereo( &m_BufferOut, &io_rBuffer );
#endif
			break;

#if defined(AK_REARCHANNELS) && defined(AK_LFECENTER)
		case AK_SPEAKER_SETUP_5POINT1 :
#if defined( AK_71FROM51MIXER )
			// On some platforms, 5.1 needs to be interleaved into 7.1 channels
			if (!in_bForce71 || AK_PERF_OFFLINE_RENDERING)
			{
				m_Mixer.MixFinal51( &m_BufferOut, &io_rBuffer );
			}
			else
			{
				m_Mixer.MixFinal71From51( &m_BufferOut, &io_rBuffer );
			}
#else
			m_Mixer.MixFinal51( &m_BufferOut, &io_rBuffer );
#endif
			break;

#endif // defined(AK_REARCHANNELS) && defined(AK_LFECENTER)

#ifdef AK_71AUDIO
		case AK_SPEAKER_SETUP_7POINT1 :
			m_Mixer.MixFinal71( &m_BufferOut, &io_rBuffer );
			break;
#endif
#if defined AK_APPLE || defined AK_PS4
		case AK_SPEAKER_SETUP_MONO :	
			m_Mixer.MixFinalMono(&m_BufferOut, &io_rBuffer );
			break;
#endif
		default:
			AKASSERT(!"Unsupported number of channels");
			break;
		}
	}
#endif

	// get volumes ready for next time
	TagPreviousVolumes();

#ifndef AK_OPTIMIZED
	#ifdef AK_PS3
		// PS3 does it in the Final Interleave processing.
	#else
		if( (AkMonitor::GetNotifFilter() & AkMonitorData::MonitorDataMeter) 
			&& m_pMeterCtx )
		{
			MeterBuffer( &m_BufferOut, m_pMeterCtx );
		}
	#endif
#endif

} // MixBuffers

AKRESULT CAkVPLFinalMixNode::SetAllInsertFx()
{
	AKRESULT l_eResult = AK_Success;

	if (CAkOutputMgr::GetDevice(AK_MAIN_OUTPUT_DEVICE)->pFinalMix == this)
	{
		for ( AkUInt32 uFXIndex = 0; uFXIndex < AK_NUM_EFFECTS_PER_OBJ; ++uFXIndex )
			l_eResult = SetInsertFx( g_MasterBusCtx, uFXIndex );
	}

	return l_eResult;
}
