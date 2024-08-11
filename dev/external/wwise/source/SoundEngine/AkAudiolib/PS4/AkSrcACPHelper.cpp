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
#include "AkCommon.h"

#include "AkSrcACPHelper.h"

CAkSkipDecodedFrames::CAkSkipDecodedFrames() :
uSkipStartBufferSamples(0)
, uSkipSamplesLength(0)
, bReadBeforeSkip(false)
{
}

AkUInt32 CAkSkipDecodedFrames::SetSkipFrames(AkUInt32 in_uSkipStartIndex, AkUInt32 in_uSkipLen, bool in_bReadBeforeSkip)
{
	if (uSkipStartBufferSamples == in_uSkipStartIndex && 
		uSkipSamplesLength == in_uSkipLen )
	{
		return in_uSkipLen;
	}
	AKASSERT(uSkipSamplesLength == 0);

#ifdef AT9_STREAM_DEBUG_OUTPUT
		char msg[256];
		sprintf( msg, "-- -- -- -- SetSkipFrames %p in_uSkipStartIndex:%i, in_uSkipLen:%i \n", this, in_uSkipStartIndex, in_uSkipLen);
		AKPLATFORM::OutputDebugMsg( msg );
#endif
	
	uSkipStartBufferSamples = in_uSkipStartIndex % AKAT9_RINGBUFFER_SAMPLES;
	uSkipSamplesLength = in_uSkipLen;

	// Setting skipdata while valid data is already located there in the ringbuffer, read once before skipping
	bReadBeforeSkip = in_bReadBeforeSkip;

	return in_uSkipLen;
}

void CAkSkipDecodedFrames::Reset()
{
	uSkipStartBufferSamples = 0;
	uSkipSamplesLength = 0;
	bReadBeforeSkip = false;
}

CAkDecodingRingbuffer::CAkDecodingRingbuffer() :
m_pData(NULL)
, m_WriteSampleIndex(0)
, m_ReadSampleIndex(0)
, m_SamplesBeingConsumed(0)
, m_RingbufferUsedSample(0)
{
}

bool CAkDecodingRingbuffer::Create(AkUInt32 in_size)
{
	m_pData = (AkUInt16*)AkAlloc( g_LEngineDefaultPoolId, in_size ); 

	return m_pData != NULL;
}

void CAkDecodingRingbuffer::Destroy()
{
	if (m_pData != NULL)
	{
		AkFree( g_LEngineDefaultPoolId, m_pData ); 
		m_pData = NULL;
	}
}

void CAkDecodingRingbuffer::Reset()
{
	m_WriteSampleIndex = 0;
	m_ReadSampleIndex = 0;
	m_SamplesBeingConsumed = 0;
	m_RingbufferUsedSample = 0;
	m_SkipFramesStart.Reset();
	m_SkipFrames.Reset();
}

AkUInt32 CAkDecodingRingbuffer::SkipFramesIfNeeded(const AkUInt32 in_uReadLength)
{
	AkUInt32 framesToRead = in_uReadLength;
	if ( m_SkipFramesStart.uSkipSamplesLength != 0 )
	{
		if ( m_ReadSampleIndex == m_SkipFramesStart.uSkipStartBufferSamples )
		{
#ifdef AT9_STREAM_DEBUG_OUTPUT
			char msg[256];
			sprintf( msg, "-- -- -- -- [%p] Skipping A. %i \n", this, m_SkipFramesStart.uSkipSamplesLength);
			AKPLATFORM::OutputDebugMsg( msg );
#endif
			if (m_SkipFramesStart.bReadBeforeSkip)
			{
				m_SkipFramesStart.bReadBeforeSkip = false;
				return framesToRead;
			}
			// skip at the begining
			m_ReadSampleIndex = ((m_ReadSampleIndex + m_SkipFramesStart.uSkipSamplesLength) % AKAT9_RINGBUFFER_SAMPLES);
			m_RingbufferUsedSample -= m_SkipFramesStart.uSkipSamplesLength;
			
			if (m_RingbufferUsedSample < framesToRead)
			{
				framesToRead = m_RingbufferUsedSample;
			}

			m_SkipFramesStart.uSkipStartBufferSamples = 0;
			m_SkipFramesStart.uSkipSamplesLength = 0;
		}
		else if( m_ReadSampleIndex < m_SkipFramesStart.uSkipStartBufferSamples && m_ReadSampleIndex + framesToRead >=  m_SkipFramesStart.uSkipStartBufferSamples )
		{
#ifdef AT9_STREAM_DEBUG_OUTPUT
			char msg[256];
			sprintf( msg, "-- -- -- -- [%p] Skipping B. %i \n", this, m_SkipFramesStart.uSkipSamplesLength);
			AKPLATFORM::OutputDebugMsg( msg );
#endif
			if (m_SkipFramesStart.bReadBeforeSkip)
			{
				m_SkipFramesStart.bReadBeforeSkip = false;
				return framesToRead;
			}
			 // skip at the end
			framesToRead = ( m_SkipFramesStart.uSkipStartBufferSamples - m_ReadSampleIndex );

			m_SamplesBeingConsumed += m_SkipFramesStart.uSkipSamplesLength;

			m_SkipFramesStart.uSkipStartBufferSamples = 0;
			m_SkipFramesStart.uSkipSamplesLength = 0;
		}
	}
	
	if ( m_SkipFrames.uSkipSamplesLength != 0 )
	{
		if ( m_ReadSampleIndex == m_SkipFrames.uSkipStartBufferSamples )
		{
#ifdef AT9_STREAM_DEBUG_OUTPUT
			char msg[256];
			sprintf( msg, "-- -- -- -- [%p] Skipping C. %i \n", this, m_SkipFrames.uSkipSamplesLength);
			AKPLATFORM::OutputDebugMsg( msg );
#endif

			if (m_SkipFrames.bReadBeforeSkip)
			{
				m_SkipFrames.bReadBeforeSkip = false;
				return framesToRead;
			}

			// skip at the begining
			m_ReadSampleIndex = ((m_ReadSampleIndex + m_SkipFrames.uSkipSamplesLength) % AKAT9_RINGBUFFER_SAMPLES);
			m_RingbufferUsedSample -= m_SkipFrames.uSkipSamplesLength;
			
			if (m_RingbufferUsedSample < framesToRead)
			{
				framesToRead = m_RingbufferUsedSample;
			}

			m_SkipFrames.uSkipStartBufferSamples = 0;
			m_SkipFrames.uSkipSamplesLength = 0;
		}
		else if( m_ReadSampleIndex < m_SkipFrames.uSkipStartBufferSamples && m_ReadSampleIndex + framesToRead >=  m_SkipFrames.uSkipStartBufferSamples )
		{
#ifdef AT9_STREAM_DEBUG_OUTPUT
			char msg[256];
			sprintf( msg, "-- -- -- -- [%p] Skipping D. %i \n", this, m_SkipFrames.uSkipSamplesLength);
			AKPLATFORM::OutputDebugMsg( msg );
#endif
			if (m_SkipFrames.bReadBeforeSkip)
			{
				m_SkipFrames.bReadBeforeSkip = false;
				return framesToRead;
			}

			// skip at the end
			framesToRead = ( m_SkipFrames.uSkipStartBufferSamples - m_ReadSampleIndex );

			m_SamplesBeingConsumed += m_SkipFrames.uSkipSamplesLength;

			m_SkipFrames.uSkipStartBufferSamples = 0;
			m_SkipFrames.uSkipSamplesLength = 0;
		}
	}
	
	return framesToRead;
}
