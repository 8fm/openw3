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
#include "AkSrcBase.h"
#include "AkCommon.h"

#include "AkACPManager.h"
#include "AkLEngine.h"

#include <ajm/at9_decoder.h>

#include "AkSrcACPHelper.h"

template <class T_BASE>
CAkSrcACPBase<T_BASE>::CAkSrcACPBase( CAkPBI * in_pCtx )
: T_BASE( in_pCtx )
, m_DecoderSamplePosition(0)
, m_MaxOutputFramesPerJob(0)
, m_uDecoderLoopCntAhead(0)
, m_uSampleSkipOnFileSeekStart(0)
, m_nSamplesPerBlock(0)
, m_nBlockSize(0)
, m_DecodingStarted(false)
, m_Initialized(false)
, m_DecodingFailed(false)
, m_IsVirtual(false)
, m_AjmInstanceId(0)
, m_AjmContextId(0)
{
}

template <class T_BASE>
CAkSrcACPBase<T_BASE>::~CAkSrcACPBase()
{
}

template <class T_BASE>
AKRESULT CAkSrcACPBase<T_BASE>::CreateHardwareInstance( const SceAjmContextId in_AjmContextId )
{
	uint64_t uiFlags = SCE_AJM_INSTANCE_FLAG_MAX_CHANNEL( GetNumChannels( T_BASE::m_pCtx->GetMediaFormat().uChannelMask ) ) | SCE_AJM_INSTANCE_FLAG_FORMAT(SCE_AJM_FORMAT_ENCODING_S16);

	int sceResult = sceAjmInstanceCreate(in_AjmContextId, SCE_AJM_CODEC_AT9_DEC, uiFlags, &m_AjmInstanceId);

	m_AjmContextId = in_AjmContextId;

	return (sceResult == SCE_OK) ? AK_Success : AK_Fail;
}

template <class T_BASE>
AKRESULT CAkSrcACPBase<T_BASE>::InitialiseDecoderJob( SceAjmBatchInfo* in_Batch )
{
	AKASSERT(m_At9ConfigData != NULL);
	memcpy(m_InitParam.uiConfigData, &m_At9ConfigData, sizeof(m_InitParam.uiConfigData));

	// Set up the At9 config data
	int sceResult = sceAjmBatchJobInitialize(in_Batch, m_AjmInstanceId, &m_InitParam, sizeof(m_InitParam), &m_InitResult);

	m_Initialized = true;

	AKASSERT( m_AjmContextId > 0 );
	AKASSERT( m_AjmInstanceId > 0 );
	
	// Keep the handle for destruction
	return (sceResult == SCE_OK) ? AK_Success : AK_Fail;
}

template <class T_BASE>
void CAkSrcACPBase<T_BASE>::StopStream()
{
	Unregister();
	T_BASE::StopStream();
}

template <class T_BASE>
void CAkSrcACPBase<T_BASE>::ReleaseBuffer()
{
	m_Ringbuffer.m_ReadSampleIndex = ((m_Ringbuffer.m_ReadSampleIndex + m_Ringbuffer.m_SamplesBeingConsumed) % AKAT9_RINGBUFFER_SAMPLES);
	m_Ringbuffer.m_RingbufferUsedSample -= m_Ringbuffer.m_SamplesBeingConsumed;
	m_Ringbuffer.m_SamplesBeingConsumed = 0;
	
#ifdef AT9_STREAM_DEBUG_OUTPUT
	/*char msg[256];
	sprintf( msg, "-- -- ReleaseBuffer %i \n", m_Ringbuffer.m_ReadSampleIndex);
	AKPLATFORM::OutputDebugMsg( msg );*/
#endif
}

template <class T_BASE>
AKRESULT CAkSrcACPBase<T_BASE>::SeekToSourceOffset()
{
	AKRESULT result = AK_Success;
	T_BASE::m_uCurSample = /*139264;*/ T_BASE::GetSourceOffset();	
	T_BASE::m_pCtx->SetSourceOffsetRemainder( 0 );

	if ( T_BASE::m_uCurSample >= T_BASE::m_uTotalSamples )
	{
		MONITOR_SOURCE_ERROR( AK::Monitor::ErrorCode_SeekAfterEof, T_BASE::m_pCtx );
		result = AK_Fail;
	}

	PrepareRingbuffer();
	return result;
}

template <class T_BASE>
AKRESULT CAkSrcACPBase<T_BASE>::ChangeSourcePosition()
{
	AKASSERT( T_BASE::m_pCtx->RequiresSourceSeek() );
	return SeekToSourceOffset();
}

template <class T_BASE>
bool CAkSrcACPBase<T_BASE>::SupportMediaRelocation() const
{
	return true;
}

template <class T_BASE>
void CAkSrcACPBase<T_BASE>::GetBuffer( AkVPLState & io_state )
{
	if (m_DecodingFailed)
	{
		io_state.result = AK_Fail;
		return;
	}

	if(m_Ringbuffer.m_RingbufferUsedSample == 0)
	{
		io_state.result = AK_NoDataReady;
		return;
	}

	AKASSERT( T_BASE::m_uCurSample == GetCurrentDecodedReadSample() );
	AkUInt32 uNumChannels = GetNumChannels( T_BASE::m_pCtx->GetMediaFormat().uChannelMask );
	AKASSERT(m_Ringbuffer.m_SamplesBeingConsumed == 0);

	AkUInt32 uMaxFrames = (AkUInt16)AkMin( io_state.MaxFrames(), m_Ringbuffer.m_RingbufferUsedSample );

	// going around the ring buffer
	if ( m_Ringbuffer.m_ReadSampleIndex + uMaxFrames > AKAT9_RINGBUFFER_SAMPLES)
	{
		uMaxFrames = (AKAT9_RINGBUFFER_SAMPLES - m_Ringbuffer.m_ReadSampleIndex);
	}
	
	// Trucating at loop end
	AkUInt32 loopEnd = (T_BASE::DoLoop()) ? ( T_BASE::m_uPCMLoopEnd + 1 ) : T_BASE::m_uTotalSamples;
	if ( T_BASE::m_uCurSample + uMaxFrames > loopEnd )
	{
		uMaxFrames = loopEnd - T_BASE::m_uCurSample;
	}

	uMaxFrames = m_Ringbuffer.SkipFramesIfNeeded(uMaxFrames);

	// going around the ring buffer, again
	if ( m_Ringbuffer.m_ReadSampleIndex + uMaxFrames > AKAT9_RINGBUFFER_SAMPLES)
	{
		uMaxFrames = (AKAT9_RINGBUFFER_SAMPLES - m_Ringbuffer.m_ReadSampleIndex);
	}

	m_Ringbuffer.m_SamplesBeingConsumed += uMaxFrames;

	AKASSERT( m_Ringbuffer.m_SamplesBeingConsumed > 0 );

#ifdef AT9_STREAM_DEBUG_OUTPUT
	char msg[256];
	sprintf( msg, "-- -- GetBuffer [%i, %i] \n", T_BASE::m_uCurSample, T_BASE::m_uCurSample + uMaxFrames);
	AKPLATFORM::OutputDebugMsg( msg );
#endif
	AKASSERT(m_Ringbuffer.m_ReadSampleIndex+uMaxFrames <= AKAT9_RINGBUFFER_SAMPLES);
	AKASSERT( T_BASE::m_uCurSample + uMaxFrames <= T_BASE::m_uTotalSamples );
	AKASSERT(io_state.result != AK_NoDataReady);
	T_BASE::SubmitBufferAndUpdate(
			m_Ringbuffer.m_pData + SampleToBufferIndex(m_Ringbuffer.m_ReadSampleIndex),
			uMaxFrames,
			T_BASE::m_pCtx->GetMediaFormat().uSampleRate,
			T_BASE::m_pCtx->GetMediaFormat().uChannelMask,
			io_state );
	AKASSERT(io_state.result != AK_NoDataReady);
	T_BASE::LeavePreBufferingState();

	AKASSERT(io_state.result != AK_NoDataReady);
}

// -------------------------------------------------------------------------------------
// Hardware decoder functions
template <class T_BASE>
bool CAkSrcACPBase<T_BASE>::Register()
{
	AkUInt32 uNumChannels = GetNumChannels( T_BASE::m_pCtx->GetMediaFormat().uChannelMask );
	
	if (!m_Ringbuffer.Create(AKAT9_RINGBUFFER_SAMPLES * uNumChannels * sizeof( AkUInt16 )) )
	{
		return false;
	}

	AkUInt32 maxOutputFramesPerJobFromApiMaxInput = (((SCE_AJM_DEC_AT9_MAX_INPUT_BUFFER_SIZE / sizeof( AkUInt16 )) * m_nSamplesPerBlock / m_nBlockSize) / m_nSamplesPerBlock) * m_nSamplesPerBlock ;
	AkUInt32 maxOutputFramesPerJobFromApiMaxOutput = (((SCE_AJM_DEC_AT9_MAX_OUTPUT_BUFFER_SIZE / sizeof( AkUInt16 )) / uNumChannels ) / m_nSamplesPerBlock) * m_nSamplesPerBlock;
	AkUInt32 maxOutputFramesPerJobFromApi = AkMin(maxOutputFramesPerJobFromApiMaxInput, maxOutputFramesPerJobFromApiMaxOutput);

	m_MaxOutputFramesPerJob = AkMin( maxOutputFramesPerJobFromApi, AKAT9_RINGBUFFER_SAMPLES) ;
	AKASSERT(m_MaxOutputFramesPerJob > 0);

	return true;
}

template <class T_BASE>
void CAkSrcACPBase<T_BASE>::Unregister()
{
	if (m_Initialized)
	{
		AKASSERT( m_AjmContextId > 0 );
		AKASSERT( m_AjmInstanceId > 0 );
		AKVERIFY( sceAjmInstanceDestroy(m_AjmContextId, m_AjmInstanceId) == SCE_OK );
		m_Initialized = false;
	}
	m_Ringbuffer.Destroy();
}

template <class T_BASE>
bool CAkSrcACPBase<T_BASE>::InitIfNeeded(SceAjmBatchInfo* in_Batch)
{
	// ----------------------------------------------------------------------------
	// On first decoding task, add an extra initialized job

	bool initializing = false;
	if( !m_Initialized )
	{
		AKRESULT res = InitialiseDecoderJob(in_Batch);
		if (res != AK_Success)
		{
			MONITOR_SOURCE_ERROR( AK::Monitor::ErrorCode_ATRAC9DecodeFailed, T_BASE::m_pCtx);
			m_DecodingFailed = true;
		}
		initializing = true;
	}

	return initializing;
}


template <class T_BASE>
AKRESULT CAkSrcACPBase<T_BASE>::CreateDecodingJob(SceAjmBatchInfo* in_Batch)
{	
	AKASSERT( m_AjmContextId > 0 );
	AKASSERT( m_AjmInstanceId > 0 );

	if (m_DecodingFailed)
	{
		return AK_Fail;
	}

	AkUInt32 uNumChannels = GetNumChannels( T_BASE::m_pCtx->GetMediaFormat().uChannelMask );
	int numBuffersIn = 0;
	int numBuffersOut = 0;

	size_t numSamplesToDecode = GetNumSamplesToDecode();

#ifdef AT9_STREAM_DEBUG_OUTPUT
/*	char msg[256];
	sprintf( msg, "-- -- CreateDecodingJob %i \n", m_Ringbuffer.m_RingbufferUsedSample);
	AKPLATFORM::OutputDebugMsg( msg );*/
#endif

	AKASSERT(numSamplesToDecode > 0);
	AKASSERT(numSamplesToDecode%m_nSamplesPerBlock == 0);
	
	AkInt32 nextSamplePosition = 0;

	m_InputBufferDesc[0].szSize = 0;
	m_InputBufferDesc[1].szSize = 0;
	m_InputBufferDesc[2].szSize = 0;
	m_OutputBufferDesc[0].szSize = 0;
	m_OutputBufferDesc[1].szSize = 0;

	AkUInt32 remainingSizeToDecode = 0;
	nextSamplePosition = m_DecoderSamplePosition + numSamplesToDecode; // default case
	
	AKASSERT ( m_uSampleSkipOnFileSeekStart == 0 );
	AKASSERT ( m_DecoderSamplePosition % m_nSamplesPerBlock == 0  );
	
	// ------------------------------------------------------------------------------------------------------------------
	// If file is ending on this frame, 
	// remove extra tail required by at9 to be blockSize align
	
	if ( FileEndOnThisFrame( numSamplesToDecode ) )
	{
#ifdef AT9_STREAM_DEBUG_OUTPUT
		char msg[256];
		sprintf( msg, "FileEndOnThisFrame %p\n", this );
		AKPLATFORM::OutputDebugMsg( msg );
#endif
		AKASSERT( !IsLoopOnThisFrame( numSamplesToDecode ) );

		numSamplesToDecode = T_BASE::m_uTotalSamples - m_DecoderSamplePosition;
		size_t newNumSamplesToDecode = FixDecodingLength(numSamplesToDecode);

		// discard added samples
		AkUInt32 uSkipEndSamplesLength = newNumSamplesToDecode - numSamplesToDecode;
		AkUInt32 skipStartIndex = m_Ringbuffer.m_WriteSampleIndex + numSamplesToDecode;
		m_Ringbuffer.m_SkipFrames.SetSkipFrames( skipStartIndex, uSkipEndSamplesLength, (skipStartIndex >= m_Ringbuffer.m_ReadSampleIndex) );
		numSamplesToDecode = newNumSamplesToDecode;

		nextSamplePosition = T_BASE::m_uTotalSamples;
	}

	// ------------------------------------------------------------------------------------------------------------------
	// If looping, split into two segments
	
	int inputBufferIndex = 0;
	if ( IsLoopOnThisFrame( numSamplesToDecode ) )
	{
		AkInt32 iLoopEnd = T_BASE::m_uPCMLoopEnd+1;

		AkInt32 fixedEndOfLoop = (iLoopEnd + (m_nSamplesPerBlock-1)) / m_nSamplesPerBlock * m_nSamplesPerBlock; //round up
		AkInt32 fixedStartOfLoop = FloorSampleToBlockSize( T_BASE::m_uPCMLoopStart ) - m_nSamplesPerBlock;

		AkUInt32 skipEndSamples = (fixedEndOfLoop - iLoopEnd);
		AkUInt32 skipStartSamples = (T_BASE::m_uPCMLoopStart - fixedStartOfLoop);
		
		if (skipEndSamples > 0 || skipStartSamples > 0)
		{
			AkUInt32 skipStartIndex = m_Ringbuffer.m_WriteSampleIndex + (iLoopEnd - m_DecoderSamplePosition);
			m_Ringbuffer.m_SkipFrames.SetSkipFrames( skipStartIndex, skipEndSamples + skipStartSamples, (skipStartIndex >= m_Ringbuffer.m_ReadSampleIndex)  );
			AKASSERT(fixedEndOfLoop >= T_BASE::m_uPCMLoopEnd);
		}

		AKASSERT(numSamplesToDecode >= (fixedEndOfLoop - m_DecoderSamplePosition));
		inputBufferIndex = SetDecodingInputBuffers(m_DecoderSamplePosition, fixedEndOfLoop, 0, remainingSizeToDecode);

		AKASSERT(numSamplesToDecode >= (fixedEndOfLoop - m_DecoderSamplePosition));
		remainingSizeToDecode += numSamplesToDecode - (fixedEndOfLoop - m_DecoderSamplePosition);
		if (remainingSizeToDecode >= m_nSamplesPerBlock) // go to loop start
		{
			AkUInt32 decodeLength = remainingSizeToDecode;
			inputBufferIndex = SetDecodingInputBuffers( fixedStartOfLoop, fixedStartOfLoop+decodeLength, inputBufferIndex, remainingSizeToDecode );
			nextSamplePosition = fixedStartOfLoop + decodeLength - remainingSizeToDecode;
			numSamplesToDecode -= remainingSizeToDecode;
			AKASSERT(nextSamplePosition <= (int)(T_BASE::m_uTotalSamples + m_nSamplesPerBlock) / m_nSamplesPerBlock * m_nSamplesPerBlock);
		}
		else
		{
			numSamplesToDecode -= remainingSizeToDecode;
			nextSamplePosition = fixedStartOfLoop;
		}
		
		m_uDecoderLoopCntAhead++;
	}
	else
	{
		inputBufferIndex = SetDecodingInputBuffers(m_DecoderSamplePosition, m_DecoderSamplePosition+numSamplesToDecode, 0, remainingSizeToDecode);
		numSamplesToDecode -= remainingSizeToDecode;
		nextSamplePosition = m_DecoderSamplePosition + numSamplesToDecode;
		AKASSERT(nextSamplePosition <= (int)(T_BASE::m_uTotalSamples + m_nSamplesPerBlock) / m_nSamplesPerBlock * m_nSamplesPerBlock);
	}
	
	numBuffersIn = inputBufferIndex; //+1;
	numBuffersOut = SetDecodingOutputBuffers(numSamplesToDecode);

	// ------------------------------------------------------------------------------------------------------------------
	// Set job

	const uint64_t DECODE_FLAGS = SCE_AJM_FLAG_RUN_MULTIPLE_FRAMES | SCE_AJM_FLAG_SIDEBAND_STREAM | SCE_AJM_FLAG_SIDEBAND_FORMAT;
	AKASSERT(numBuffersIn <= 3);
	AKASSERT(numBuffersOut <= 2);
	int res = sceAjmBatchJobRunSplit(in_Batch, m_AjmInstanceId, DECODE_FLAGS, m_InputBufferDesc, numBuffersIn,
						   m_OutputBufferDesc, numBuffersOut, &m_DecodeResult, sizeof (m_DecodeResult));

	//AKVERIFY( res == SCE_OK );
	if (res != SCE_OK)
	{ 
		MONITOR_SOURCE_ERROR( AK::Monitor::ErrorCode_ATRAC9DecodeFailed, T_BASE::m_pCtx);
		m_DecodingFailed = true;
		return AK_Fail;
	}

	// ------------------------------------------------------------------------------------------------------------------
	// Increment pointers for next job
	AKASSERT(nextSamplePosition <= (int)(T_BASE::m_uTotalSamples + m_nSamplesPerBlock) / m_nSamplesPerBlock * m_nSamplesPerBlock);
	m_DecoderSamplePosition = nextSamplePosition;
	m_Ringbuffer.m_RingbufferUsedSample += numSamplesToDecode;
	m_Ringbuffer.m_WriteSampleIndex = (m_Ringbuffer.m_WriteSampleIndex + numSamplesToDecode) % AKAT9_RINGBUFFER_SAMPLES;
	m_DecodingStarted = true;

	AKASSERT(m_Ringbuffer.m_RingbufferUsedSample <= AKAT9_RINGBUFFER_SAMPLES);

	return AK_Success;
}

// -------------------------------------------------------------------------------------
// Decoding job helper functions

template <class T_BASE>
AkUInt32 CAkSrcACPBase<T_BASE>::SetDecodingOutputBuffers(size_t in_uSampleLength)
{
	size_t numBuffersOut;
	AkUInt32 uNumChannels = GetNumChannels( this->m_pCtx->GetMediaFormat().uChannelMask );

	if ((m_Ringbuffer.m_WriteSampleIndex + in_uSampleLength) > AKAT9_RINGBUFFER_SAMPLES)
	{
		// Output
		m_OutputBufferDesc[0].pAddress = m_Ringbuffer.m_pData + SampleToBufferIndex(m_Ringbuffer.m_WriteSampleIndex);

		size_t numSamplesA = (AKAT9_RINGBUFFER_SAMPLES - m_Ringbuffer.m_WriteSampleIndex);
		size_t numSamplesB = (in_uSampleLength - (AKAT9_RINGBUFFER_SAMPLES - m_Ringbuffer.m_WriteSampleIndex));

		m_OutputBufferDesc[0].szSize = (size_t)SamplesToOutputBytes( numSamplesA );
		m_OutputBufferDesc[1].pAddress = m_Ringbuffer.m_pData;
		m_OutputBufferDesc[1].szSize = (size_t)SamplesToOutputBytes( numSamplesB );

		AKASSERT( m_OutputBufferDesc[0].szSize % m_nSamplesPerBlock == 0);
		AKASSERT( m_OutputBufferDesc[1].szSize % m_nSamplesPerBlock == 0);
		AKASSERT( (m_OutputBufferDesc[0].szSize + m_OutputBufferDesc[1].szSize) <= SCE_AJM_DEC_AT9_MAX_OUTPUT_BUFFER_SIZE );
		numBuffersOut = 2;
	}
	else
	{
		m_OutputBufferDesc[0].pAddress = m_Ringbuffer.m_pData + SampleToBufferIndex(m_Ringbuffer.m_WriteSampleIndex);
		m_OutputBufferDesc[0].szSize = (size_t)SamplesToOutputBytes(in_uSampleLength);
		m_OutputBufferDesc[1].szSize = 0;
			
		AKASSERT( m_OutputBufferDesc[0].szSize % m_nSamplesPerBlock == 0);
		AKASSERT(m_OutputBufferDesc[0].szSize <= SCE_AJM_DEC_AT9_MAX_OUTPUT_BUFFER_SIZE );
		numBuffersOut = 1;

	}

#ifdef AT9_STREAM_DEBUG_OUTPUT
		char msg[256];
		sprintf( msg, "-- -- -- -- SetDecodingOutputBuffers target len:%i, nb:%i [%p,%i], [%p,%i]\n", in_uSampleLength, numBuffersOut,
			m_OutputBufferDesc[0].pAddress, m_OutputBufferDesc[0].szSize,
			m_OutputBufferDesc[1].pAddress, m_OutputBufferDesc[1].szSize);
		AKPLATFORM::OutputDebugMsg( msg );
#endif

	return numBuffersOut;
}

template <class T_BASE>
void CAkSrcACPBase<T_BASE>::PrepareRingbuffer()
{
	m_Ringbuffer.Reset();
	m_DecoderSamplePosition = T_BASE::m_uCurSample - m_nSamplesPerBlock;
	T_BASE::EnterPreBufferingState();
	m_uDecoderLoopCntAhead = 0;

	AkUInt32 uSkipStartSamplesLength = 0;
	if ( m_DecoderSamplePosition % m_nSamplesPerBlock != 0 )
	{
		AKASSERT( m_uSampleSkipOnFileSeekStart == 0 );
		AkUInt32 newCurrentPos = FloorSampleToBlockSize( m_DecoderSamplePosition );
		uSkipStartSamplesLength += m_DecoderSamplePosition - newCurrentPos;
		m_DecoderSamplePosition = newCurrentPos;
	}
	else
	{
		uSkipStartSamplesLength += m_uSampleSkipOnFileSeekStart;
		m_uSampleSkipOnFileSeekStart = 0;
	}

#ifdef AT9_STREAM_DEBUG_OUTPUT
	char msg[256];
	sprintf( msg, "PrepareRingbuffer \n" );
	AKPLATFORM::OutputDebugMsg( msg );
#endif
		
	uSkipStartSamplesLength += m_nSamplesPerBlock;
	m_Ringbuffer.m_SkipFramesStart.SetSkipFrames( 0, uSkipStartSamplesLength, false );	
}

#ifdef _DEBUG
// Ensures the decoding write index is following the current read sample on every fame, used only for debugging purpose.
template <class T_BASE>
AkInt32 CAkSrcACPBase<T_BASE>::GetCurrentDecodedReadSample() const
{
	//AKASSERT( m_DecoderSamplePosition <= this->m_uPCMLoopEnd );
	AkInt32 currentReadSample;
	if ( m_Ringbuffer.m_SkipFrames.uSkipSamplesLength == 0 && m_Ringbuffer.m_SkipFramesStart.uSkipSamplesLength == 0 )
	{
		currentReadSample = m_DecoderSamplePosition - m_Ringbuffer.m_RingbufferUsedSample;
	}
	else
	{
		currentReadSample = this->m_uCurSample;
	}

	/*if (this->m_uCurSample < m_DecoderSamplePosition)
	{
		AKASSERT((m_Ringbuffer.m_RingbufferUsedSample - m_Ringbuffer.m_SkipFramesStart.uSkipSamplesLength) <= m_DecoderSamplePosition);
	}*/


	// for debugging
	/*if( this->m_uCurSample != currentReadSample )
	{
		return currentReadSample;
	}*/
	return currentReadSample;
}
#endif