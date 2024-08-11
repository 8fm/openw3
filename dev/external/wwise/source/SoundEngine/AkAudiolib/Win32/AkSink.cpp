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
// AkSink.cpp
//
// Platform dependent part
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "AkSink.h"
#include "AkSinkDirectSound.h"
#include "AkSinkXAudio2.h"


#include "AkMath.h"
#include "AkMonitor.h"
#include "AkProfile.h"
#include "AkRuntimeEnvironmentMgr.h"
#include "AkSettings.h"
#include "AkCaptureMgr.h"
#include "AkOutputMgr.h"
#include "AkLEngine.h"

#ifdef AK_WASAPI
#include "AkSinkWasapi.h"
#endif
extern AkInitSettings g_settings;
extern AkPlatformInitSettings g_PDSettings;

CAkSink::ListOfMergingSinks CAkSink::s_MergingSinks;

CAkSink::CAkSink(AkSinkType in_eType)
	: m_pCapture( NULL )
	, m_eType(in_eType)
	, m_SpeakersConfig( 0 ) // speakers config is unknown	
	, m_bWaitForMerge(true)
{
#ifndef AK_OPTIMIZED
	m_stats.m_fOutMin = (AkReal32) INT_MAX;
	m_stats.m_fOutMax = (AkReal32) INT_MIN;
	m_stats.m_fOutSum = 0;
	m_stats.m_fOutSumOfSquares = 0;
	m_stats.m_uOutNum = 0;	
#endif
}

AKRESULT CAkSink::AllocBuffer()
{
	AkUInt32 uMasterOutSize = LE_MAX_FRAMES_PER_BUFFER * AK::GetNumChannels(m_SpeakersConfig) * sizeof(AkReal32);
	void * pData = AkMalign( g_LEngineDefaultPoolId, uMasterOutSize, AK_BUFFER_ALIGNMENT );
	if ( !pData )
		return AK_InsufficientMemory;

	AkZeroMemAligned( pData, uMasterOutSize );

	m_MasterOut.Clear();
	m_MasterOut.AttachInterleavedData( 
		pData,						// Buffer.
		LE_MAX_FRAMES_PER_BUFFER,	// Buffer size (in sample frames).
		0,							// Valid frames.
		m_SpeakersConfig );		// Chan config.

	return AK_Success;
}

CAkSink * CAkSink::CreateInternal( AkOutputSettings & in_settings, AkSinkType in_eType, AkUInt32 in_uInstance )
{
	// Detect: 
	// - XboxOne and Wasapi: Wasapi first, then XAudio2, , then directsound if available.
	// - Non-Wasapi Windows: XAudio2 first, then directsound if available.
	AkSinkType eAPIType = in_eType;
#if defined AK_XBOXONE
	if (in_eType == AkSink_BGM || in_eType == AkSink_Communication)
	{
		//For BGM and Communication channels, use the same audio API as the main device.
		AkDevice * pMainDevice = CAkOutputMgr::GetDevice(AK_MAIN_OUTPUT_DEVICE);
		if (pMainDevice != NULL)
			eAPIType = pMainDevice->pSink->GetType();
	}
#endif

#ifdef AK_WASAPI
	if ( eAPIType == AkSink_Main || eAPIType == AkSink_Main_Wasapi )
	{
		// Try to create Wasapi sink
		IMMDeviceEnumerator * pEnumerator = NULL;
		CoCreateInstance(
			__uuidof(MMDeviceEnumerator), NULL,
			CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
			(void**)&pEnumerator);
		if ( pEnumerator )
		{
			CAkSinkWasapi * pSinkWasapi = AkNew( g_LEngineDefaultPoolId, CAkSinkWasapi() );
			if ( !pSinkWasapi )
				return NULL;

			if ( pSinkWasapi->Init( pEnumerator, in_settings.uChannelMask, in_eType ) != AK_Success )
			{
				pSinkWasapi->Term();
				AkDelete( g_LEngineDefaultPoolId, pSinkWasapi );
				pSinkWasapi = NULL;
			}
			else
			{
				// Success
				pEnumerator->Release();
				return pSinkWasapi;
			}
			pEnumerator->Release();
		}
	}
#endif
	// Try to create DirectSound sink

#ifdef AK_XAUDIO2
	if ( eAPIType == AkSink_Main || eAPIType == AkSink_Main_XAudio2 )
	{
		CAkSinkXAudio2 * pSinkXAudio2 = AkNew( g_LEngineDefaultPoolId, CAkSinkXAudio2() );
		if ( !pSinkXAudio2 )
			return NULL;

		if ( pSinkXAudio2->Init( g_PDSettings, in_settings.uChannelMask, in_eType ) == AK_Success ) 
			return pSinkXAudio2;

		pSinkXAudio2->Term();
		AkDelete( g_LEngineDefaultPoolId, pSinkXAudio2 );
	}
#endif

#ifdef AK_DIRECTSOUND
	if ( in_eType == AkSink_Main || in_eType == AkSink_Main_DirectSound )
	{
		CAkSinkDirectSound * pSinkDS = AkNew( g_LEngineDefaultPoolId, CAkSinkDirectSound() );
		if ( !pSinkDS )
			return NULL;

		if ( pSinkDS->Init( g_PDSettings, in_settings.uChannelMask ) == AK_Success ) 
			return pSinkDS;

		pSinkDS->Term();
		AkDelete( g_LEngineDefaultPoolId, pSinkDS );
	}
#endif

	if (in_eType == AkSink_Dummy)
	{
		if(in_settings.uChannelMask == AK_CHANNEL_MASK_DETECT && !AK_PERF_OFFLINE_RENDERING)
		{
			//If we must let the hardware decide, take the main output config.
			in_settings.uChannelMask = g_pAkSink ? g_pAkSink->GetSpeakerConfig() : AK_SPEAKER_SETUP_STEREO;
		}

		return CreateDummy(in_settings.uChannelMask);
	}

#if !defined AK_OPTIMIZED && defined AK_WIN
	if (in_eType == AkSink_MergeToMain)
	{
		CAkMergingSink *pSink = AkNew( g_LEngineDefaultPoolId, CAkMergingSink() );
		if (pSink)
		{
			if(in_settings.uChannelMask == AK_CHANNEL_MASK_DETECT)
				in_settings.uChannelMask = g_pAkSink->GetSpeakerConfig();	//If we must let the hardware decide, take the main output config.

			if (pSink->Init(in_settings.uChannelMask, CAkOutputMgr::GetDevice(AK_MAIN_OUTPUT_DEVICE)->pSink, &CAkOutputMgr::GetDevice(AK_MAIN_OUTPUT_DEVICE)->pSink->m_MasterOut) != AK_Success)
			{
				pSink->Term();
				AkDelete(g_LEngineDefaultPoolId, pSink);
				return NULL;
			}
			return pSink;
		}
	}
#endif

	return NULL;
}


CAkSink * CAkSink::Create( AkOutputSettings & in_settings, AkSinkType in_eType, AkUInt32 in_uInstance )
{
	CAkSink* pSink = CreateInternal(in_settings, in_eType, in_uInstance);
	if (pSink != NULL && in_eType != AkSink_MergeToMain)
	{
		//Were going to replace the main sink.  Advise the merging sinks
		RerouteMerges(pSink);	
	}

	return pSink;
}

CAkSink * CAkSink::CreateDummy( AkChannelMask in_uChannelMask )
{
	CAkSinkDummy * pSinkDummy = AkNew( g_LEngineDefaultPoolId, CAkSinkDummy() );
	if ( !pSinkDummy )
		return NULL;	// NOTE: this should NEVER happen
		
	if (pSinkDummy->Init(in_uChannelMask) != AK_Success)
	{
		pSinkDummy->Term();
		AkDelete(g_LEngineDefaultPoolId, pSinkDummy);
		pSinkDummy = NULL;
	}

	return pSinkDummy;
}

void CAkSink::Term()
{
	//Only "main" sinks should end here.	
	RerouteMerges(NULL);
}

void CAkSink::RerouteMerges(CAkSink* in_pNewMainSink)
{
	if (s_MergingSinks.IsEmpty())
		return;

	CAkSink *pSink = NULL;
	AkPipelineBufferBase* pBuffer = NULL;
	if (in_pNewMainSink != NULL)
	{
		pSink = in_pNewMainSink;
		pBuffer = &in_pNewMainSink->NextWriteBuffer();
	}

	for(ListOfMergingSinks::Iterator it = s_MergingSinks.Begin(); it != s_MergingSinks.End(); ++it)
		(*it)->ReplaceSink(pSink, pBuffer);
}

void CAkSink::FinishMix()
{
	if (s_MergingSinks.IsEmpty())
		return;

	m_bWaitForMerge = false;
	if (m_MasterOut.uValidFrames)
		PassData();
	else
		PassSilence();
	m_bWaitForMerge = true;
};

#ifndef AK_OPTIMIZED
void CAkSink::UpdateProfileData( AkReal32 * in_pfSamples, AkUInt32 in_uNumSamples )
{
	if ( !( AkMonitor::GetNotifFilter() & AkMonitorData::MonitorDataOutput ) )
		return;

	AKSIMD_V4F32 * pmIn = (AKSIMD_V4F32 *) in_pfSamples;
	AKSIMD_V4F32 * pmEnd = (AKSIMD_V4F32 *) ( in_pfSamples + in_uNumSamples );

	AKSIMD_V4F32 mMin = AKSIMD_SET_V4F32( (AkReal32) INT_MAX ); 
	AKSIMD_V4F32 mMax = AKSIMD_SET_V4F32( (AkReal32) INT_MIN );
	AKSIMD_V4F32 mSum = AKSIMD_SET_V4F32( 0.0f );
	AKSIMD_V4F32 mSumSquares = AKSIMD_SET_V4F32( 0.0f );

	do
	{
		AKSIMD_V4F32 mTmp = *pmIn;

		mMin = AKSIMD_MIN_V4F32( mTmp, mMin );
		mMax = AKSIMD_MAX_V4F32( mTmp, mMax );
		mSum = AKSIMD_ADD_V4F32( mTmp, mSum );

		mTmp = AKSIMD_MUL_V4F32( mTmp, mTmp );
		mSumSquares = AKSIMD_ADD_V4F32( mTmp, mSumSquares );
		
		pmIn++;
	}
	while ( pmIn < pmEnd );

	m_stats.m_fOutMin = AkMath::Min( m_stats.m_fOutMin, AKSIMD_GETELEMENT_V4F32( mMin, 0 ) );
	m_stats.m_fOutMin = AkMath::Min( m_stats.m_fOutMin, AKSIMD_GETELEMENT_V4F32( mMin, 1 ) );
	m_stats.m_fOutMin = AkMath::Min( m_stats.m_fOutMin, AKSIMD_GETELEMENT_V4F32( mMin, 2 ) );
	m_stats.m_fOutMin = AkMath::Min( m_stats.m_fOutMin, AKSIMD_GETELEMENT_V4F32( mMin, 3 ) );

	m_stats.m_fOutMax = AkMath::Max( m_stats.m_fOutMax, AKSIMD_GETELEMENT_V4F32( mMax, 0 ) );
	m_stats.m_fOutMax = AkMath::Max( m_stats.m_fOutMax, AKSIMD_GETELEMENT_V4F32( mMax, 1 ) );
	m_stats.m_fOutMax = AkMath::Max( m_stats.m_fOutMax, AKSIMD_GETELEMENT_V4F32( mMax, 2 ) );
	m_stats.m_fOutMax = AkMath::Max( m_stats.m_fOutMax, AKSIMD_GETELEMENT_V4F32( mMax, 3 ) );

	m_stats.m_fOutSum += AKSIMD_GETELEMENT_V4F32( mSum, 0 ) 
		+ AKSIMD_GETELEMENT_V4F32( mSum, 1 ) 
		+ AKSIMD_GETELEMENT_V4F32( mSum, 2 ) 
		+ AKSIMD_GETELEMENT_V4F32( mSum, 3 );

	m_stats.m_fOutSumOfSquares += AKSIMD_GETELEMENT_V4F32( mSumSquares, 0 )
		+ AKSIMD_GETELEMENT_V4F32( mSumSquares, 1 )
		+ AKSIMD_GETELEMENT_V4F32( mSumSquares, 2 )
		+ AKSIMD_GETELEMENT_V4F32( mSumSquares, 3 );

	m_stats.m_uOutNum += in_uNumSamples;
}

void CAkSink::UpdateProfileSilence( AkUInt32 in_uNumSamples )
{
	if ( !( AkMonitor::GetNotifFilter() & AkMonitorData::MonitorDataOutput ) )
		return;

	m_stats.m_fOutMin = AkMath::Min( m_stats.m_fOutMin, 0 );
	m_stats.m_fOutMax = AkMath::Max( m_stats.m_fOutMax, 0 );
	m_stats.m_uOutNum += in_uNumSamples;
}
#endif

//====================================================================================================
// StartOutputCapture
//====================================================================================================
void CAkSink::StartOutputCapture(const AkOSChar* in_CaptureFileName)
{
	if (m_pCapture == NULL)
	{
		AkUInt16 uNumChannels = (AkInt16)AK::GetNumChannels( m_SpeakersConfig );
		m_pCapture = AkCaptureMgr::Instance()->StartCapture(
			in_CaptureFileName, 
			uNumChannels, 
			AK_CORE_SAMPLERATE,
			16,// Hardcoded down so we can compare both sinks in 16 bits samples.
			AkCaptureFile::Int16
			);	
	}
}

//====================================================================================================
// StopOutputCapture
//====================================================================================================
void CAkSink::StopOutputCapture()
{
	if (m_pCapture != NULL)
	{
		m_pCapture->StopCapture();
		m_pCapture = NULL;	//Instance is deleted by StopCapture
	}
}

//----------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------
//
// In the event that there is no sound card or driver present, the application will revert to creating
// a dummy sink
//
//----------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------
AKRESULT CAkSinkDummy::Init(AkChannelMask in_uChannels)
{
#ifdef AK_USE_METRO_API
	m_Timer = (DWORD) ::GetTickCount64();
#else
	m_Timer = ::GetTickCount();
#endif
	m_SpeakersConfig = in_uChannels;
	m_dwMSPerBuffer = (DWORD) ( 1000.0 / ( (double) AK_CORE_SAMPLERATE / AK_NUM_VOICE_REFILL_FRAMES ) ); // duration of buffer in milliseconds 
	m_usBlockAlign = (AkUInt16) ( AK::GetNumChannels( m_SpeakersConfig ) * sizeof(AkInt16) );

	AkUInt32 uMasterOutSize = LE_MAX_FRAMES_PER_BUFFER * AK::GetNumChannels( m_SpeakersConfig ) * sizeof(AkReal32);
	void * pData = AkMalign( g_LEngineDefaultPoolId, uMasterOutSize, AK_BUFFER_ALIGNMENT );
	if ( !pData )
		return AK_InsufficientMemory;

	AkZeroMemAligned( pData, uMasterOutSize );

	m_MasterOut.Clear();
	m_MasterOut.AttachInterleavedData( 
		pData,						// Buffer.
		LE_MAX_FRAMES_PER_BUFFER,	// Buffer size (in sample frames).
		0,							// Valid frames.
		m_SpeakersConfig );		// Chan config.

	return AK_Success;
}

AKRESULT CAkSinkDummy::Play()
{
	return AK_Success;
}

void CAkSinkDummy::Term()
{
	if( m_MasterOut.HasData() )
	{
		AkFalign( g_LEngineDefaultPoolId, m_MasterOut.GetContiguousDeinterleavedData() );
		m_MasterOut.ClearData();
	}

	StopOutputCapture();
	return;
}

DWORD CAkSinkDummy::GetThreadWaitTime()
{
	return AK_PC_WAIT_TIME;
}

bool CAkSinkDummy::IsStarved()
{
	return false;
}

void CAkSinkDummy::ResetStarved()
{
	return;	
}

AKRESULT CAkSinkDummy::IsDataNeeded( AkUInt32 & out_uBuffersNeeded )
{
#ifdef AK_USE_METRO_API
	DWORD tmp_Timer  = (DWORD) ::GetTickCount64();
#else
	DWORD tmp_Timer  = ::GetTickCount();
#endif	
	if(tmp_Timer < m_Timer) // in case the timer wraps
	{
		out_uBuffersNeeded = 1;
	}
	else 
	{
		out_uBuffersNeeded = (tmp_Timer - m_Timer) / m_dwMSPerBuffer;
	}

	if ( out_uBuffersNeeded )
		m_Timer = tmp_Timer;

	return AK_Success;
}

AKRESULT CAkSinkDummy::PassData()
{
	void* pvFrom = m_MasterOut.GetInterleavedData();
	AkUInt32 ulToRefillSamples = m_MasterOut.uValidFrames * m_MasterOut.NumChannels();
	AkUInt32 ulToRefillBytes = ulToRefillSamples * sizeof( AkInt16 );

#ifndef AK_OPTIMIZED
//	UpdateProfileData( (float *) pvFrom, ulToRefillSamples );
#endif

	if ( m_pCapture != NULL )
	{
		void * pBuf = alloca( ulToRefillBytes );

		FloatsToShorts( (AkInt16 *) pBuf, (float *) pvFrom, ulToRefillSamples );

		m_pCapture->PassSampleData(pBuf, ulToRefillBytes);
	}

	return AK_Success;
}

AKRESULT CAkSinkDummy::PassSilence()
{
	AkUInt32 ulToRefillBytes = AK_NUM_VOICE_REFILL_FRAMES * m_usBlockAlign;

#ifndef AK_OPTIMIZED
//	UpdateProfileSilence( ulToRefillBytes / sizeof( AkReal32 ) );
#endif

	if (m_pCapture != NULL)
	{
		void * pBuf = alloca( ulToRefillBytes );
		memset( pBuf, 0, ulToRefillBytes );

		m_pCapture->PassSampleData(pBuf, ulToRefillBytes);
	}

	return AK_Success;
}

// Used by Create 
AKRESULT CAkMergingSink::Init(AkChannelMask in_uChannels, CAkSink* in_pOther, AkPipelineBufferBase* in_pBuffer)
{
	ReplaceSink(in_pOther, in_pBuffer);
	s_MergingSinks.AddFirst(this);
	return CAkSinkDummy::Init(in_uChannels);
}

void CAkMergingSink::ReplaceSink(CAkSink* in_pTarget, AkPipelineBufferBase* in_pBuffer)
{
	m_pTarget = in_pTarget;
	m_pMixingBuffer = in_pBuffer;
}

void CAkMergingSink::Term()
{
	s_MergingSinks.Remove(this);
	CAkSinkDummy::Term();
}

AKRESULT CAkMergingSink::PassData()
{
	if (m_pTarget == NULL)
		return AK_Fail;

	AkReal32* pbSourceData = (AkReal32*)m_MasterOut.GetInterleavedData();

	AkInt32 lNumSamples = m_MasterOut.uValidFrames * m_MasterOut.NumChannels();
	AKASSERT( !( lNumSamples % 4 ) ); // we operate 4 samples at a time.

#ifndef AK_OPTIMIZED
	UpdateProfileData( (float *) pbSourceData, lNumSamples );
#endif
	
	//Mix this buffer into the final mixing buffer.  Note that both input and the output are interleaved at this stage.
	AkUInt32 uDestChannels = AK::GetNumChannels(m_pMixingBuffer->GetChannelMask());
	if (m_MasterOut.GetChannelMask() == m_pMixingBuffer->GetChannelMask())
	{
		//Easy, just mix normally
		AKSIMD_V4F32 * AK_RESTRICT pvFrom = (AKSIMD_V4F32 *) pbSourceData;
		AKSIMD_V4F32 * AK_RESTRICT pvEnd = (AKSIMD_V4F32 *) ( pbSourceData + lNumSamples );
		AKSIMD_V4F32 * AK_RESTRICT pvTo = (AKSIMD_V4F32 *) m_pMixingBuffer->GetInterleavedData();

		for ( ; pvFrom < pvEnd; pvFrom++, pvTo++ )
			*pvTo = AKSIMD_ADD_V4F32( *pvTo, *pvFrom );
	}
	//Mix stereo into anything larger
	else if ((m_MasterOut.GetChannelMask() & m_pMixingBuffer->GetChannelMask()) == AK_SPEAKER_SETUP_STEREO)
	{
		AkReal32 * AK_RESTRICT pvFrom = pbSourceData;
		AkReal32 * AK_RESTRICT pvEnd = pbSourceData + lNumSamples;
		AkReal32 * AK_RESTRICT pvTo = (AkReal32*)m_pMixingBuffer->GetInterleavedData();

		for(;pvFrom != pvEnd; pvFrom += 2, pvTo += uDestChannels)
		{
			pvTo[0] += pvFrom[0];
			pvTo[1] += pvFrom[1];
		}
	}
	//Mix mono into anything larger
	else if (m_MasterOut.NumChannels() == 1)
	{
		AkReal32 * AK_RESTRICT pvFrom = pbSourceData;
		AkReal32 * AK_RESTRICT pvEnd = pbSourceData + lNumSamples;
		AkReal32 * AK_RESTRICT pvTo = (AkReal32*)m_pMixingBuffer->GetInterleavedData();

		for(;pvFrom != pvEnd; pvFrom++, pvTo += uDestChannels)
		{
			pvTo[0] += 0.707f * pvFrom[0];
			pvTo[1] += 0.707f * pvFrom[0];
		}
	}
	else
		AKASSERT(!"Unsupported secondary device configuration.");	//Add a new mixing mode.

	m_pMixingBuffer->uValidFrames = m_MasterOut.uValidFrames;

	return CAkSinkDummy::PassData();	//Let the capture run normally.
}

AKRESULT CAkMergingSink::PassSilence()
{
	CAkSinkDummy::PassSilence();
	return AK_Success;
}
