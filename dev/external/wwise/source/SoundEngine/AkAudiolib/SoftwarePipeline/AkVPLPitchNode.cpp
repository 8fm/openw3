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
// AkVPLPitchNode.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkLEngine.h"
#include "AkVPLPitchNode.h"
#include "AkVPLSrcCbxNode.h"
#include "AudiolibDefs.h"     // Pool IDs definition.
#include "AkMonitor.h"
#include "math.h"

void CAkVPLPitchNode::GetBuffer( AkVPLState & io_state )
{
	AKASSERT( m_pInput != NULL );
	AKASSERT( m_pPBI != NULL );

#ifdef AK_MOTION
	AKASSERT( io_state.MaxFrames() == AK_FEEDBACK_MAX_FRAMES_PER_BUFFER
		|| io_state.MaxFrames() == AK_NUM_VOICE_REFILL_FRAMES );
#else
	AKASSERT( io_state.MaxFrames() == AK_NUM_VOICE_REFILL_FRAMES );
#endif // AK_MOTION

	m_Pitch.SetRequestedFrames( io_state.MaxFrames() );
	m_bStartPosInfoUpdated = false;

	// Apply new pitch from context
	m_Pitch.SetPitch( m_pInput->GetPitch() );

	// Consume what we have
	if( m_BufferIn.uValidFrames != 0 )
	{
		ConsumeBuffer( io_state );
		return;
	}

	if ( m_bLast )
	{
		// Upstream node (source) had already finished and the pitch node has no 
		// data in its input buffer. This can occur when the voice becomes physical
		// FromElapsedTime just after the source finished, but the pitch node still 
		// has "virtual buffered samples". Simply continue processing downstream
		// (with 0 valid input samples). WG-9004.
		io_state.result = AK_NoMoreData;
	}
}

void CAkVPLPitchNode::ConsumeBuffer( AkVPLState & io_state )
{
	if ( io_state.result == AK_NoMoreData )
		m_bLast = true;

	if ( m_BufferIn.uValidFrames == 0 )
	{
		//AKASSERT( io_state.uValidFrames != 0 );
		// WG-9121 Source plug-ins with RTPCable duration may end up (legitimally) in this condition

		if ( AK_EXPECT_FALSE( io_state.uValidFrames == 0 
			&& io_state.result == AK_DataReady ) )
		{
			// Handle the case where sources return AK_DataReady with 0 frames.
			// This is legal: need to ask it data again.
			io_state.result = AK_DataNeeded;
			return;
		}

		m_BufferIn = io_state;
	}

	if ( !m_BufferOut.HasData() )
	{
		// output same channel config as input
#ifndef AK_XBOX360
		if ( m_BufferOut.GetCachedBuffer( (AkUInt16) m_Pitch.GetRequestedFrames(),
										  m_BufferIn.GetChannelMask() 
										  ) == AK_Success )
		{
#else
		AkUInt8* pData = (AkUInt8*)AkMalign( g_LEngineDefaultPoolId, (m_Pitch.GetRequestedFrames()+4)*sizeof(AkReal32)*m_BufferIn.NumChannels(), AK_BUFFER_ALIGNMENT );
		if ( pData )
		{
			((AkPipelineBufferBase*)&m_BufferOut)->AttachInterleavedData( pData, (AkUInt16) m_Pitch.GetRequestedFrames(), 0, m_BufferIn.GetChannelMask() );
#endif
			AkPrefetchZero(m_BufferOut.GetInterleavedData(), m_BufferOut.NumChannels()*m_Pitch.GetRequestedFrames()*sizeof(AkReal32));

			// IM
			// Force to start the sound in a specific sample in the buffer.
			// Allows to start sound on non-buffer boundaries.
			if ( m_bPadFrameOffset )
			{
				AKASSERT( !m_Pitch.HasOffsets() );

				AkInt32 l_iFrameOffset = m_pPBI->GetFrameOffsetBeforeFrame();
				if( l_iFrameOffset > 0 )
				{
					AKASSERT( l_iFrameOffset < m_BufferOut.MaxFrames() );

#ifndef AK_PS3
					if( m_Pitch.IsPostDeInterleaveRequired() )
						ZeroPrePadBufferInterleaved( &m_BufferOut, l_iFrameOffset );// Set the skipped samples to zero
					else
#endif
						ZeroPrePadBuffer( &m_BufferOut, l_iFrameOffset );// Set the skipped samples to zero

					m_Pitch.SetOutputBufferOffset( l_iFrameOffset );// set the starting offset of the output buffer
				}
				m_bPadFrameOffset = false;
			}
		}
		else
		{
			io_state.result = AK_Fail;
			return;
		}
	}

	// If the source was not starting at the beginning of the file and the format was not decoded sample accurately, there
	// may be a remaining number of samples to skip so that the source starts accurately to the right sample.
	// Note: The pitch node does not care about the source "really" requires a source offset (seek). When this 
	// code executes, the PBI's source offset only represents the remainder of the original source seeking value.
	// The task of the pitch node is to correct the error if the source doesn't have sample-accurate seeking ability.
	AkInt32 l_iSourceFrameOffset = m_pPBI->GetSourceOffsetRemainder();
	if( l_iSourceFrameOffset )
	{
		if( m_BufferIn.uValidFrames > l_iSourceFrameOffset)
		{
			m_Pitch.SetInputBufferOffset( l_iSourceFrameOffset );
			m_BufferIn.uValidFrames -= (AkUInt16)l_iSourceFrameOffset;
			m_pPBI->SetSourceOffsetRemainder( 0 );
		}
		else
		{
			m_pPBI->SetSourceOffsetRemainder( l_iSourceFrameOffset - m_BufferIn.uValidFrames );
			m_BufferIn.uValidFrames = 0;
			io_state.uValidFrames = 0;
			ReleaseInputBuffer( io_state );
			io_state.result = m_bLast ? AK_NoMoreData : AK_DataNeeded;
			return;
		}
	}

	AKASSERT( m_BufferIn.HasData() || m_BufferIn.uValidFrames == 0 );

	AkUInt32 l_ulInputFrameOffset = m_Pitch.GetInputFrameOffset();
	AkUInt16 l_usConsumedInputFrames = m_BufferIn.uValidFrames;
	
#ifdef AK_PS3
	m_uInputFrameOffsetBefore = (AkUInt16) l_ulInputFrameOffset;

	if ( AK_EXPECT_FALSE( m_BufferIn.uValidFrames == 0 ) )
	{
		m_Pitch.m_InternalPitchState.uInValidFrames = 0;
		m_Pitch.m_InternalPitchState.uOutValidFrames = m_BufferOut.uValidFrames;
		ProcessDone( io_state ); // WG-15893
		return;
	}

	m_Pitch.ExecutePS3( &m_BufferIn, &m_BufferOut, io_state );
#else

	// Note. The number of frames already present in the output buffer must NEVER exceed the 
	// number of requested frames. This situation should have been caught in VPLPitchNode::GetBuffer().
	AKRESULT eResult = m_Pitch.Execute( &m_BufferIn, &m_BufferOut );

	l_usConsumedInputFrames -= m_BufferIn.uValidFrames;

	CopyRelevantMarkers( &m_BufferIn, &m_BufferOut, l_ulInputFrameOffset, l_usConsumedInputFrames );
	
	// Adjust position information
	if( ( m_BufferIn.posInfo.uStartPos != (AkUInt32) -1 ) && !m_bStartPosInfoUpdated )
	{
		m_BufferOut.posInfo = m_BufferIn.posInfo;
		m_BufferOut.posInfo.uStartPos = m_BufferIn.posInfo.uStartPos + l_ulInputFrameOffset;
		m_bStartPosInfoUpdated = true;
	}
	m_BufferOut.posInfo.fLastRate = m_Pitch.GetLastRate();

	// Input entirely consumed release it.
	if( m_BufferIn.uValidFrames == 0 )
	{
		ReleaseInputBuffer( io_state );
		
		if( m_bLast == true )
		{
			if ( m_pCbx->m_pSources[ 1 ] )
				eResult = SwitchToNextSrc();
			else
				eResult = AK_NoMoreData;
		}
	}

	AKASSERT( m_BufferOut.MaxFrames() == m_Pitch.GetRequestedFrames() );

	if ( eResult == AK_DataReady || eResult == AK_NoMoreData )
	{
		if ( m_Pitch.IsPostDeInterleaveRequired() )
			m_Pitch.DeinterleaveAndSwapOutput( &m_BufferOut );
		*((AkPipelineBuffer *) &io_state) = m_BufferOut;
	}

	io_state.result = eResult;
#endif
}

#ifdef AK_PS3
void CAkVPLPitchNode::ProcessDone( AkVPLState & io_state )
{
	m_BufferOut.uValidFrames = m_Pitch.m_InternalPitchState.uOutValidFrames; 
	io_state.result = m_Pitch.m_InternalPitchState.eState;

	AkUInt32 uInputFramesBefore = m_BufferIn.uValidFrames;
	m_BufferIn.uValidFrames = m_Pitch.m_InternalPitchState.uInValidFrames;
	AkUInt32 uConsumedInputFrames = uInputFramesBefore - m_BufferIn.uValidFrames;

	if ( m_Pitch.m_PitchOperationMode == PitchOperatingMode_Interpolating && m_Pitch.m_InternalPitchState.uInterpolationRampCount >= PITCHRAMPLENGTH )
	{
		m_Pitch.m_InternalPitchState.uCurrentFrameSkip = m_Pitch.m_InternalPitchState.uTargetFrameSkip;
		m_Pitch.m_PitchOperationMode = PitchOperatingMode_Fixed;
		// Note: It is ok to go to fixed mode (even if it should have gone to bypass mode) for the remainder of this buffer
		// It will go back to bypass mode after next SetPitch() is called
	}

	// Handle the case of the interpolating pitch finishing ramping before the end of output
	if ( m_BufferIn.uValidFrames > 0 && 
		m_BufferOut.uValidFrames < m_Pitch.GetRequestedFrames() )
	{
		m_Pitch.ExecutePS3( &m_BufferIn, &m_BufferOut, io_state );
		return;
	}

	CopyRelevantMarkers( &m_BufferIn, &m_BufferOut, m_uInputFrameOffsetBefore, uConsumedInputFrames );
	
	// Adjust position information
	if( ( m_BufferIn.posInfo.uStartPos != -1 ) && !m_bStartPosInfoUpdated )
	{
		m_BufferOut.posInfo = m_BufferIn.posInfo;
		m_BufferOut.posInfo.uStartPos = m_BufferIn.posInfo.uStartPos + m_uInputFrameOffsetBefore;
		m_bStartPosInfoUpdated = true;
	}
	m_BufferOut.posInfo.fLastRate = m_Pitch.GetLastRate();

	// Input entirely consumed release it.
	if( m_BufferIn.uValidFrames == 0 )
	{
		ReleaseInputBuffer( io_state );

		if( m_bLast == true )
		{
			if ( m_pCbx->m_pSources[ 1 ] )
				io_state.result = SwitchToNextSrc();
			else
				io_state.result = AK_NoMoreData;
		}
	}
	if ( io_state.result == AK_DataReady || io_state.result == AK_NoMoreData )
	{
		*((AkPipelineBuffer *) &io_state) = m_BufferOut;
	}
}
#endif // PS3

// Switch to next source on the combiner in the sample-accurate transition scenario
AKRESULT CAkVPLPitchNode::SwitchToNextSrc()
{
	CAkPBI * pPBINew = m_pCbx->m_pSources[ 1 ]->GetContext();

	// Check if there is a frame offset; we can honor it if it is larger than the space remaining in the buffer.
	AkInt32 iFrameOffset = pPBINew->GetFrameOffset();
	if( iFrameOffset > 0 )
	{
		AkInt32 iNextRequestFrames = m_BufferOut.MaxFrames() - m_BufferOut.uValidFrames;
		pPBINew->ConsumeFrameOffset( AkMin( iNextRequestFrames, iFrameOffset ) );

		return AK_NoMoreData; // Then catch nomoredata in runvpl and do an old-fashioned addpipeline with m_pSources[ 1 ]
	}

	AKRESULT eResultFetch = m_pCbx->m_pSources[ 1 ]->FetchStreamedData();
	if ( eResultFetch == AK_FormatNotReady )
	{
		return AK_NoMoreData;
	}
	else if ( eResultFetch != AK_Success )
	{
		return AK_Fail;
	}

	// Only now that the next source has 'fetched' can we get the format
	AkAudioFormat fmtOld = m_pPBI->GetMediaFormat();
	AkAudioFormat fmtNew = pPBINew->GetMediaFormat();
	if ( fmtOld.GetChannelMask() != fmtNew.GetChannelMask() ) 
	{
		MONITOR_SOURCE_ERROR( AK::Monitor::ErrorCode_TransitionNotAccurateChannel, m_pPBI );
		return AK_NoMoreData;
	}

	m_pCbx->SwitchToNextSrc(); // This performs the change on all other nodes

	m_pPBI = pPBINew;
	m_pInput = m_pCbx->m_pSources[ 0 ];

	pPBINew->CalcEffectiveParams();

	m_Pitch.SwitchTo( fmtNew, m_pInput->GetPitch(), &m_BufferOut, m_pCbx->GetSampleRate() );

	m_bLast = false;
	AKRESULT eResult = m_BufferOut.uValidFrames != m_Pitch.GetRequestedFrames() ? AK_DataNeeded : AK_DataReady;

	return eResult;
}

//-----------------------------------------------------------------------------
// Name: ReleaseBuffer
// Desc: Releases a processed buffer.
//-----------------------------------------------------------------------------
void CAkVPLPitchNode::ReleaseBuffer()
{
	AKASSERT( m_pInput != NULL );

	// Assume output buffer was entirely consumed by client.
	if( m_BufferOut.HasData() )
	{
#ifndef AK_XBOX360
		m_BufferOut.ReleaseCachedBuffer();
#else
		AkFalign( g_LEngineDefaultPoolId, m_BufferOut.GetInterleavedData() );
		((AkPipelineBufferBase*)&m_BufferOut)->DetachData();
#endif
		
		m_BufferOut.Clear();

		m_Pitch.SetOutputBufferOffset( 0 );
	}
} // ReleaseBuffer

// io_buffer is the VPL state's buffer.
void CAkVPLPitchNode::ReleaseInputBuffer( AkPipelineBuffer & io_buffer )
{
	m_pInput->ReleaseBuffer();
	m_BufferIn.ClearAndFreeMarkers();

	// Note. Technically we should reassign our cleared input buffer to the
	// pipeline buffer (in this case the pipeline should only hold a reference 
	// of our input buffer), but just clearing the data does the trick: the
	// request is reset in RunVPL().
	//io_buffer = m_BufferIn;
	io_buffer.ClearData();
	// WG-21537 Don't forget to clear references to markers that were just freed.
	io_buffer.ResetMarkerPointer();
}

AKRESULT CAkVPLPitchNode::TimeSkip( AkUInt32 & io_uFrames )
{
	// Apply new pitch from context
 	m_Pitch.SetPitchForTimeSkip( m_pInput->GetPitch() );

	AkUInt32 uFramesToProduce = io_uFrames;
	m_Pitch.TimeOutputToInput( uFramesToProduce );

	AkUInt32 uProducedFrames = 0;
	AKRESULT eResult = AK_DataReady;

	while ( uFramesToProduce )
	{
		// Need to get 'more data' from source ?

		if ( !m_BufferIn.uValidFrames && 
             !m_bLast )
		{
			AkUInt32 uSrcRequest = io_uFrames;

			AKRESULT eThisResult = m_pInput->TimeSkip( uSrcRequest );

			if( eThisResult != AK_DataReady && eThisResult != AK_NoMoreData )
				return eThisResult;
			else if ( eThisResult == AK_NoMoreData )
				m_bLast = true;

			// Consume source offset remainder stored in the PBI if any.
			AkUInt32 uSrcOffsetToConsume = m_pPBI->GetSourceOffsetRemainder();
			AkUInt32 uRemainingSrcOffsetToConsume = 0;
			if ( uSrcOffsetToConsume > uSrcRequest )
			{
				// Cannot consume source offset completely from input's 'data'. Clamp and push the rest to PBI.
				uRemainingSrcOffsetToConsume = uSrcOffsetToConsume - uSrcRequest;
				uSrcOffsetToConsume = uSrcRequest;
			}
			uSrcRequest -= uSrcOffsetToConsume;
			m_pPBI->SetSourceOffsetRemainder( uRemainingSrcOffsetToConsume );

			m_BufferIn.uValidFrames = (AkUInt16) uSrcRequest;
		}

		// Enable this code to activate IM to use Virtual voices.
		// IM
		// Force to start the sound in a specific sample in the actual buffer.
		// Allows to start sound on non-buffer boundaries.
		//if( !m_bFirstBufferProcessed )
		//{
		//	AKASSERT( m_PBI->GetNativeSampleOffsetInBuffer() < uFramesToProduce );
		//	//if not starting at first sample, the real number to produce is less than initially demanded.
		//	uFramesToProduce -= m_PBI->GetNativeSampleOffsetInBuffer();
		//	m_bFirstBufferProcessed = true;
		//}

		AkUInt32 uFrames = AkMin( uFramesToProduce, m_BufferIn.uValidFrames );

		uProducedFrames += uFrames;
		uFramesToProduce -= uFrames;

		m_BufferIn.uValidFrames -= (AkUInt16) uFrames;

		if ( !m_BufferIn.uValidFrames ) 
		{
			if ( m_bLast ) 
			{
				eResult = AK_NoMoreData;
				break;
			}
		}
	}

	m_Pitch.TimeInputToOutput( uProducedFrames );

	io_uFrames = uProducedFrames;

	return eResult;
}

void CAkVPLPitchNode::VirtualOn( AkVirtualQueueBehavior eBehavior )
{
	if ( eBehavior != AkVirtualQueueBehavior_Resume )
	{
		// we do not support skipping some data in the input buffer and then coming back:
		// flush what's left.

		if ( m_BufferIn.HasData() )
			m_pInput->ReleaseBuffer();

		m_BufferIn.ClearAndFreeMarkers();
		m_Pitch.ResetOffsets();
	}

	if( eBehavior == AkVirtualQueueBehavior_FromBeginning )
	{
		// WG-5935 
		// If the last buffer was drained and we restart from beginning, the flag must be resetted.
		m_bLast = false;
	}

    if ( !m_bLast )
	{
		m_pInput->VirtualOn( eBehavior );
	}
}

AKRESULT CAkVPLPitchNode::VirtualOff( AkVirtualQueueBehavior eBehavior, bool in_bUseSourceOffset )
{
	if ( eBehavior == AkVirtualQueueBehavior_FromElapsedTime )
	{
		// We used this as a marker for elapsed frames, flush it now.
		m_BufferIn.uValidFrames = 0;
	}

    if ( !m_bLast )
		return m_pInput->VirtualOff( eBehavior, in_bUseSourceOffset );

	return AK_Success;
}

AKRESULT CAkVPLPitchNode::Seek()
{
	// Clear the input buffer first, then propagate to the input.
	m_pInput->ReleaseBuffer();
	m_BufferIn.ClearAndFreeMarkers();

	// Reset resampler.
	m_Pitch.ResetOffsets();

	// Seek anywhere: reset m_bLast!
	m_bLast = false;

	return m_pInput->Seek();
}


//-----------------------------------------------------------------------------
// Name: Init
// Desc: Initializes the node.
//
// Parameters:
//	AkSoundParams* in_pSoundParams		   : Pointer to the sound parameters.
//	AkAudioFormat* io_pFormat			   : Pointer to the format of the sound.
//-----------------------------------------------------------------------------
void CAkVPLPitchNode::Init( AkAudioFormat * io_pFormat,			// Format.
								CAkPBI* in_pPBI, // PBI, to access the initial pbi that created the pipeline.
								AkUInt32 in_usSampleRate
								)
{
	m_pPBI = in_pPBI;
	m_bLast						= false;
	m_bPadFrameOffset			= true;

	m_Pitch.Init( io_pFormat, in_usSampleRate );
} // Init

//-----------------------------------------------------------------------------
// Name: Term
// Desc: Term.
//
// Parameters:
//
// Return:
//	AK_Success : Terminated correctly.
//	AK_Fail    : Failed to terminate correctly.
//-----------------------------------------------------------------------------
void CAkVPLPitchNode::Term()
{
	if( m_BufferOut.HasData() )
	{
		// WG-14924: Markers may be left here when stopping a just-starved voice.
		m_BufferOut.FreeMarkers();
	
#ifndef AK_XBOX360
		m_BufferOut.ReleaseCachedBuffer();
#else
		AkFalign( g_LEngineDefaultPoolId, m_BufferOut.GetInterleavedData() );
		((AkPipelineBufferBase*)&m_BufferOut)->DetachData();
#endif
	}

	// Release any input markers that could have been left there.
	m_BufferIn.FreeMarkers();

	m_pPBI = NULL;

	m_Pitch.Term();
} // Term

void CAkVPLPitchNode::RelocateMedia( AkUInt8* in_pNewMedia,  AkUInt8* in_pOldMedia )
{
	if( m_BufferIn.HasData() )
	{
		m_BufferIn.RelocateMedia( in_pNewMedia, in_pOldMedia );
	}
}
