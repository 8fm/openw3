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

#include "AkVPLFilterNodeOutOfPlace.h"
#include "AudiolibDefs.h"
#include "AkMonitor.h"
#include "AkFXMemAlloc.h"
#include "AkAudioLibTimer.h"
#include "AkSpeakerPan.h"
#include <AK/SoundEngine/Common/IAkPlugin.h>
#include "AkPositionRepository.h"
#include "AkMixer.h"
#include <AK/SoundEngine/Common/AkSimd.h>

#ifdef AK_PS3
#include "AkLEngine.h"
#endif

//Number of frames averaged for rate estimation
#define FRAMES_AVERAGE 8	

void CAkVPLFilterNodeOutOfPlace::GetBuffer( AkVPLState & io_state )
{
	m_usRequestedFrames = io_state.MaxFrames();

	// Upstream node has reported end of data, output effect tail
	if( m_bLast )
	{
		io_state.result = AK_NoMoreData;
		ConsumeBuffer( io_state );
		return;
	}

	// Start going downstream if we still have input remaining
	if( m_BufferIn.uValidFrames != 0 )
	{
		ConsumeBuffer( io_state );
	}
}

void CAkVPLFilterNodeOutOfPlace::ConsumeBuffer( AkVPLState & io_state )
{
	if ( io_state.result == AK_NoMoreData )
		m_bLast = true;
        
	if ( m_BufferIn.uValidFrames == 0 )
	{
		m_uInOffset = 0;
		InitInputBuffer(io_state);
	}
	//m_BufferIn.eState = !m_bLast ? io_state.result : AK_NoMoreData;
	m_BufferIn.eState = io_state.result;
	
	if ( !m_BufferOut.HasData() )
	{
		AkUInt8* pData = (AkUInt8*)AkAlloc( g_LEngineDefaultPoolId, m_usRequestedFrames*sizeof(AkReal32)*m_BufferOut.NumChannels() );
		if ( pData )
		{
			((AkPipelineBufferBase*)&m_BufferOut)->AttachInterleavedData( pData, m_usRequestedFrames, 0, m_BufferOut.GetChannelMask() );
		}
		else
		{
			io_state.result = AK_Fail;
			return;
		}
	}

	m_InputFramesBeforeExec = m_BufferIn.uValidFrames;

	// Bypass FX if necessary
	if( m_bBypassed || m_pCtx->GetBypassAllFX() )
	{
		if ( !m_LastBypassed )	// Reset FX if not bypassed last buffer
			m_pEffect->Reset( );

		m_LastBypassed = true;

		AkUInt32 uNumFrames = AkMin( m_BufferIn.uValidFrames, m_BufferOut.MaxFrames() );

		if( uNumFrames > 0 )
		{
			AkChannelMask uChannelMaskIn = m_BufferIn.GetChannelMask();
			AkChannelMask uChannelMaskOut = m_BufferOut.GetChannelMask();

			AkUInt32 uNumChannelsOut = AK::GetNumChannels( uChannelMaskOut ); 
			for ( AkUInt32 uChan = 0; uChan < uNumChannelsOut; uChan++ )
			{
				AkReal32 * pfOut = m_BufferOut.GetChannel( uChan );
				memset( pfOut, 0, uNumFrames*sizeof(AkReal32) );
			}
			
			/// REVIEW: In "no panner" mode, this merely returns the identity matrix.
			AkSIMDSpeakerVolumes volumes[AK_VOICE_MAX_NUM_CHANNELS];
			CAkSpeakerPan::GetSpeakerVolumes2DPan( 0, 0, 1.f, false, uChannelMaskIn, uChannelMaskOut, volumes );

			CAkMixer::DownMix( &m_BufferIn, &m_BufferOut, volumes, uNumFrames );
		}

		m_BufferIn.uValidFrames = 0;
		if ( m_bLast )
			m_BufferOut.eState = AK_NoMoreData;
		else if ( m_BufferOut.uValidFrames == m_BufferOut.MaxFrames() )
			m_BufferOut.eState = AK_DataReady;
		else
			m_BufferOut.eState = AK_DataNeeded;
	}
	else
	{
		m_LastBypassed = false;

#ifndef AK_PS3
		AK_START_PLUGIN_TIMER( m_FXID );
 		m_pEffect->Execute( &m_BufferIn, m_uInOffset, &m_BufferOut );
		AK_STOP_PLUGIN_TIMER( m_FXID );
#else	
		AK::MultiCoreServices::DspProcess* pDspProcess = NULL;
		m_pEffect->Execute( &m_BufferIn, m_uInOffset, &m_BufferOut , pDspProcess );
		if ( pDspProcess )
		{
			io_state.result = AK_ProcessNeeded;
			CAkLEngine::QueueDspProcess(pDspProcess);
		}
		else
		{
			// Input buffer state is ignored by pipeline
			io_state.result = m_BufferOut.eState;
		}

		return;
#endif
	}

	ProcessDone(io_state);
	
	AKSIMD_ASSERTFLUSHZEROMODE;
}

void CAkVPLFilterNodeOutOfPlace::ProcessDone( AkVPLState & io_state )
{
	AkUInt32 uConsumedInputFrames = m_InputFramesBeforeExec - m_BufferIn.uValidFrames;
	CopyRelevantMarkers(&m_BufferIn, &m_BufferOut, m_uInOffset, uConsumedInputFrames);

	if ((m_pCtx->GetRegisteredNotif() & AK_EnableGetSourcePlayPosition))
	{	
		m_uConsumedSinceLastOutput += uConsumedInputFrames;

		if (m_BufferOut.eState == AK_DataReady || m_BufferOut.eState == AK_NoMoreData)
		{
			m_fAveragedInput = (m_fAveragedInput * (FRAMES_AVERAGE - 1) + m_uConsumedSinceLastOutput) / FRAMES_AVERAGE;
			m_fAveragedOutput = (m_fAveragedOutput * (FRAMES_AVERAGE - 1) + m_BufferOut.uValidFrames) / FRAMES_AVERAGE;

			m_uConsumedSinceLastOutput = 0;

			io_state.posInfo.uSampleRate = m_BufferIn.posInfo.uSampleRate;
			io_state.posInfo.uStartPos = m_BufferIn.posInfo.uStartPos + m_uInOffset;
			io_state.posInfo.uFileEnd = m_BufferIn.posInfo.uFileEnd;
			AKASSERT(m_fAveragedOutput != 0.f);
			io_state.posInfo.fLastRate = m_fAveragedInput/m_fAveragedOutput;			
		}
	}

	m_uInOffset += uConsumedInputFrames;
	m_uRequestedInputFrames += uConsumedInputFrames;
	m_uConsumedInputFrames = m_uRequestedInputFrames;
	
	// Input entirely consumed release it.
	if( m_BufferIn.uValidFrames == 0 )
	{
		m_pInput->ReleaseBuffer();

		m_BufferIn.ClearAndFreeMarkers();
		
		// Note. Technically we should reassign our cleared input buffer to the
		// pipeline buffer (in this case the pipeline should only hold a reference 
		// of our input buffer), but just clearing the data does the trick: the
		// request is reset in RunVPL().
		//io_state = m_BufferIn;
		io_state.ClearData();
		io_state.uValidFrames = 0;
	}

	if ( m_BufferOut.eState == AK_DataReady || m_BufferOut.eState == AK_NoMoreData )
	{
		/////////////////////////
		// Patch for WG-17598 
		m_BufferOut.posInfo = io_state.posInfo; // Quick fix for propagating position information when processing out of place effects.
		/////////////////////////
		
		*((AkPipelineBuffer *) &io_state) = m_BufferOut;
	}

	io_state.result = m_BufferOut.eState;
	AKASSERT( m_BufferOut.uValidFrames <= io_state.MaxFrames() );	// Produce <= than requested
}

//-----------------------------------------------------------------------------
// Name: ReleaseBuffer
// Desc: Releases a processed buffer.
//-----------------------------------------------------------------------------
void CAkVPLFilterNodeOutOfPlace::ReleaseBuffer()
{
	// Assume output buffer was entirely consumed by client.
	if( m_BufferOut.HasData() )
	{
		AkFree( g_LEngineDefaultPoolId, m_BufferOut.GetInterleavedData() );
		((AkPipelineBufferBase*)&m_BufferOut)->DetachData();

		m_BufferOut.Clear();
	}
} // ReleaseBuffer

AKRESULT CAkVPLFilterNodeOutOfPlace::Seek()
{
	m_pEffect->Reset();

	// Clear the input buffer first, then propagate to the input.
	m_pInput->ReleaseBuffer();

	m_BufferIn.ClearAndFreeMarkers();

	// Seek anywhere: reset m_bLast!
	m_bLast = false;

	return m_pInput->Seek();
}

void CAkVPLFilterNodeOutOfPlace::VirtualOn( AkVirtualQueueBehavior eBehavior )
{
	if ( eBehavior != AkVirtualQueueBehavior_Resume )
	{
		m_pEffect->Reset();
		ReleaseInputBuffer();
	}

	CAkVPLFilterNodeBase::VirtualOn( eBehavior );
}

AKRESULT CAkVPLFilterNodeOutOfPlace::Init( 
		IAkPlugin * in_pPlugin,
		const AkFXDesc & in_fxDesc,
		AkUInt32 in_uFXIndex,
		CAkPBI * in_pCtx,
		AkAudioFormat &	io_format )
{
	m_pEffect				= (IAkOutOfPlaceEffectPlugin *) in_pPlugin;
	m_BufferIn.Clear();
	m_BufferOut.Clear();

	m_uRequestedInputFrames = 0;
	m_uConsumedInputFrames = 0;
	m_fAveragedInput = 1.f;
	m_fAveragedOutput = 1.f;
	m_uConsumedSinceLastOutput = 0;

	AKRESULT eResult = CAkVPLFilterNodeBase::Init( 
		in_pPlugin,
		in_fxDesc,
		in_uFXIndex,
		in_pCtx,
		io_format );
	if ( eResult == AK_Success )
	{
		eResult = m_pEffect->Init(
			AkFXMemAlloc::GetLower(),
			m_pInsertFXContext,	
			m_pParam,
			io_format
			);

		if ( eResult == AK_Success )
		{
			m_BufferOut.SetChannelMask( io_format.GetChannelMask() ); // preserve output format selection for buffer allocation

			eResult = m_pEffect->Reset( );
		}
		else
		{
			switch ( eResult )
			{ 
			case AK_UnsupportedChannelConfig:
				MONITOR_PLUGIN_ERROR( AK::Monitor::ErrorCode_PluginUnsupportedChannelConfiguration, in_pCtx, m_FXID );	
				break;
			case AK_PluginMediaNotAvailable:
				MONITOR_PLUGIN_ERROR( AK::Monitor::ErrorCode_PluginMediaUnavailable, in_pCtx, m_FXID );	
				break;
			default:
				MONITOR_PLUGIN_ERROR( AK::Monitor::ErrorCode_PluginInitialisationFailed, in_pCtx, m_FXID );	
				break;
			}
		}
	}
	return eResult;

} // Init

void CAkVPLFilterNodeOutOfPlace::Term()
{
	ReleaseMemory();
	CAkVPLFilterNodeBase::Term();

	if( m_BufferOut.HasData() )
	{
		m_BufferOut.FreeMarkers();
	
		AkFree( g_LEngineDefaultPoolId, m_BufferOut.GetInterleavedData() );
		((AkPipelineBufferBase*)&m_BufferOut)->DetachData();
	}

	// Release any input markers that could have been left there.
	m_BufferIn.FreeMarkers();
} // Term

void CAkVPLFilterNodeOutOfPlace::ReleaseMemory()
{
	if( m_pEffect != NULL )
	{
		m_pEffect->Term( AkFXMemAlloc::GetLower() );
		AK_DECREMENT_PLUGIN_COUNT( m_FXID );
		m_pEffect = NULL;
	}
}

AKRESULT CAkVPLFilterNodeOutOfPlace::TimeSkip( AkUInt32 & io_uFrames )
{
	if ( m_bLast )
		return AK_NoMoreData;

	AkUInt32 uSrcRequest = io_uFrames;
	
	AKRESULT eResult = m_pEffect->TimeSkip( uSrcRequest );
	m_uRequestedInputFrames += uSrcRequest;
	uSrcRequest = m_uRequestedInputFrames - m_uConsumedInputFrames;

	//Request frames from the input node.  The request must be AK_NUM_VOICE_REFILL_FRAMES so we accumulate enough requests to make that.
	//This means that upon returning from virtual mode, the synchronization will may be lost by a few buffers if time was slowed down.
	while ( uSrcRequest >= AK_NUM_VOICE_REFILL_FRAMES && eResult == AK_DataReady )
	{
		AkUInt32 uFramesToProduce = AK_NUM_VOICE_REFILL_FRAMES;
		eResult = m_pInput->TimeSkip(uFramesToProduce);
		uSrcRequest -= uFramesToProduce;
		m_uConsumedInputFrames += uFramesToProduce;

		//Out of place effects can not produce data if the input doesn't have data.
		//Effects with tails effectively can't compute the tail if it hasn't normally computed previous audio samples.
		//So when virtual, if the input is exhausted, there can't be any output.
		m_bLast = (eResult == AK_NoMoreData);
	}
	
	return eResult;
}

void CAkVPLFilterNodeOutOfPlace::InitInputBuffer(AkPipelineBuffer &in_buffer)
{
	m_BufferIn = in_buffer;
	CopyRelevantMarkers(&in_buffer, &m_BufferIn, 0, in_buffer.uValidFrames);	
};

bool CAkVPLFilterNodeOutOfPlace::ReleaseInputBuffer()
{
	if ( m_pInput )
		m_pInput->ReleaseBuffer();

	m_BufferIn.ClearAndFreeMarkers();

	return true;
}

AkChannelMask CAkVPLFilterNodeOutOfPlace::GetOutputChannelMask()
{
	return m_BufferOut.GetChannelMask();
}
