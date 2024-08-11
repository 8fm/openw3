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

////////////////////////////////////////////////////////////////////////
// RumbleBus.cpp
//
// Final mix bus for the rumble function of the game controllers.
//
// Copyright 2007 Audiokinetic Inc.
//
// Author:  mjean
// Version: 1.0
//
///////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "RumbleBusBase.h"
#include "math.h"

RumbleBusBase::RumbleBusBase() 
{
	memset(m_pData, 0, sizeof(AkReal32) * BUFFER_SIZE);
	m_usWriteBuffer = 0;
	m_usReadBuffer = 0;
	m_fPeak = 0;
	m_iPlayer = 0;	
	m_bGotData = false;
	m_iDrift = 0;
	m_fVolume = 1.0f;

	m_pCapture = NULL;

	m_fAverageSpeed = 0;
	m_bStopped = true;
}

AKRESULT RumbleBusBase::Term( AK::IAkPluginMemAlloc * in_pAllocator )
{
	StopOutputCapture();

	AK_PLUGIN_DELETE(in_pAllocator, this);

	return AK_Success;
}

AKRESULT RumbleBusBase::Reset()
{
	return AK_Success;
}

AKRESULT RumbleBusBase::GetPluginInfo( AkPluginInfo & out_rPluginInfo )
{
	return AK_NotImplemented;
}

AKRESULT RumbleBusBase::Init(AK::IAkPluginMemAlloc * in_pAllocator, AkPlatformInitSettings * io_pPDSettings, AkUInt8 in_iPlayer, void * in_pDevice )
{
	m_iPlayer = in_iPlayer;
	return AK_Success;
}


AKRESULT RumbleBusBase::MixAudioBuffer( AkAudioBuffer &io_rBuffer )
{
	//Don't process the LFE
	AkUInt32 uNumChannel = AK::GetNumChannels(io_rBuffer.GetChannelMask() & ~AK_SPEAKER_LOW_FREQUENCY);
	for (AkUInt16 iSample = 0; iSample < SAMPLE_COUNT; ++iSample)
	{
		AkReal32 fMax = 0;
		AkUInt16 iSamplesToProcess = io_rBuffer.uValidFrames / SAMPLE_COUNT;

		for(AkUInt32 iChannel = 0; iChannel < uNumChannel; ++iChannel)
		{
			AkReal32 *pData = (AkReal32*)io_rBuffer.GetChannel(iChannel) + iSample * iSamplesToProcess;
			for(AkUInt16 i = 0; i < iSamplesToProcess; ++i)
			{
				fMax = AkMax(*pData, fMax);
				pData++;
			}
		}

		m_pData[m_usWriteBuffer + iSample * CHANNEL_COUNT] += fMax * m_fVolume;		//Left motor
		m_pData[m_usWriteBuffer + iSample * CHANNEL_COUNT + 1] += fMax * m_fVolume;	//Right motor		
	}

	m_bGotData = true;

	return AK_Success;
}

AKRESULT RumbleBusBase::MixFeedbackBuffer( AkAudioBuffer &io_rBuffer )
{
	if (io_rBuffer.uValidFrames == 0)
		return AK_Success;

	//Grab only the two samples, the first one and the middle one
	AkReal32 * pData;

	for(AkUInt16 i = 0; i < io_rBuffer.NumChannels(); i++)
	{
		pData = (AkReal32*)io_rBuffer.GetChannel(i);

		for(AkUInt16 s = 0; s < SAMPLE_COUNT; s++)
		{
			//Interleave the samples in the final buffer
			AkUInt32 uOffset = (m_usWriteBuffer + (i + s*CHANNEL_COUNT)) & BUFFER_MASK;
			AkReal32 fSample = pData[0] * m_fVolume;
			// Note: values can slightly undershoot 0 because of curve evaluation.
			if ( fSample < 0.f ) fSample = 0.f;
			m_pData[uOffset] += fSample;
			pData += io_rBuffer.MaxFrames()/SAMPLE_COUNT;
		}
	}
	m_bGotData = true;
	return AK_Success;
}

AKRESULT RumbleBusBase::RenderData()
{
	AkUInt16 i = 0;
	if (!m_bGotData)
	{
		if ( !m_bStopped )
		{
			//Put zeros to send next.
			for(i = 0; i < SAMPLE_COUNT*CHANNEL_COUNT; i++)
				m_pData[m_usWriteBuffer + i] = 0.0f;

			//Advance the write pointer
			m_usWriteBuffer = (m_usWriteBuffer + SAMPLE_COUNT*CHANNEL_COUNT) & BUFFER_MASK;

			//Clear the buffer for the next mix.
			for(i = 0; i < SAMPLE_COUNT*CHANNEL_COUNT; i++)
				m_pData[m_usWriteBuffer + i] = 0.0f;

			m_iDrift = 0;
		}
		return AK_Success; 
	}

	for(i = 0; i < SAMPLE_COUNT*CHANNEL_COUNT; i++)
	{
		//Compute peak
		m_fPeak = AkMax(m_pData[m_usWriteBuffer+i ], m_fPeak);
	}

	//Advance the write pointer
	m_usWriteBuffer = (m_usWriteBuffer + SAMPLE_COUNT*CHANNEL_COUNT) & BUFFER_MASK;

	for(i = 0; i < SAMPLE_COUNT*CHANNEL_COUNT; i++)
	{
		//Clear the buffer for the next mix.
		m_pData[m_usWriteBuffer + i] = 0.0f;
	}

	m_bGotData = false;

	//Added 2 samples
	m_iDrift += SAMPLE_COUNT;

	//Because we don't really control the number of buffers processed per slice (we're piggy-backing the
	//audio thread's clock), we need to make sure we don't lag behind too much.  It is normal that audio buffer
	//be processed 2 or 3 in a row.  It is possible that we receive only one clock tick during that time.
	//Our clock ticks are variable because of this which induces a lag.  Fortunately, it won't be felt if we 
	//skip a sample once in a while.  We just need to make sure the write head don't overwrite the read head.
	if(m_iDrift > SAMPLE_COUNT*BUFFER_COUNT / 2)
	{
		m_usReadBuffer = (m_usReadBuffer + CHANNEL_COUNT) & BUFFER_MASK;
		m_iDrift -= 1;
	}

	return AK_Success;
}

void RumbleBusBase::CommandTick()
{
	//Don't send anything if there is nothing in the buffer.  We don't care about starvation.
	if (m_usReadBuffer == m_usWriteBuffer)
	{
		if (m_pCapture != NULL)
		{
			AkInt16 uiOutput[] = {0,0};
			m_pCapture->PassSampleData(uiOutput, CHANNEL_COUNT*sizeof(AkInt16));
		}

		m_iDrift = 0;
		return;
	}

	//Send the next sample
	SendSample();
	m_usReadBuffer = (m_usReadBuffer + CHANNEL_COUNT) & BUFFER_MASK;

	//Sent one sample
	m_iDrift--;
}

AkReal32 RumbleBusBase::GetPeak()
{
	AkReal32 fReturned = m_fPeak;
	m_fPeak = 0.0f;
	return fReturned;
}

bool RumbleBusBase::IsStarving()
{
	return false;
}

AkChannelMask RumbleBusBase::GetMixingFormat()
{
	return AK_SPEAKER_SETUP_STEREO;
}

void RumbleBusBase::SetMasterVolume( AkReal32 in_fVol )
{
	m_fVolume = in_fVol;
}

/*****************************************************/
// Capturing the rumble output is done on demand, for unit tests.

//====================================================================================================
// StartOutputCapture
//====================================================================================================
void RumbleBusBase::StartOutputCapture(const AkOSChar* in_CaptureFileName)
{
	if (m_pCapture != NULL)
		return;

	size_t uStrLen = AKPLATFORM::OsStrLen(in_CaptureFileName)+1;

	//Reserve 2 chars for "P1" suffix to identify the player  
	AkOSChar* pNameForPlayer = (AkOSChar*)AkAlloca((uStrLen + 2)*sizeof(AkOSChar));
	AKPLATFORM::SafeStrCpy( pNameForPlayer, in_CaptureFileName, uStrLen );

	//Find the extension, and copy it farther to make space.
	size_t iExt = 0;
	for(iExt = uStrLen - 1; pNameForPlayer[iExt] != (AkOSChar)'.' && iExt > 0; iExt--)
		pNameForPlayer[iExt + 2] = pNameForPlayer[iExt];

	pNameForPlayer[iExt+2] = (AkOSChar)'.';
	pNameForPlayer[iExt+1] = (AkOSChar)'1' + m_iPlayer;
	pNameForPlayer[iExt] = (AkOSChar)'P';

	m_pCapture = AkCaptureMgr::Instance()->StartCapture(pNameForPlayer, CHANNEL_COUNT, AK_FEEDBACK_SAMPLE_RATE, 16, AkCaptureFile::Int16);
}

//====================================================================================================
// StopOutputCapture
//====================================================================================================
void RumbleBusBase::StopOutputCapture()
{
	if (m_pCapture != NULL)
	{
		m_pCapture->StopCapture();
		m_pCapture = NULL;
	}
}