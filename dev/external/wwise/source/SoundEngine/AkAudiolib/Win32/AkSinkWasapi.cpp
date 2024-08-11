
#include "stdafx.h"
#include "AkSinkWasapi.h"
#include "AkSettings.h"
#include "AkLEngine.h"


#ifdef AK_WASAPI

extern CAkAudioMgr *g_pAudioMgr;

CAkSinkWasapi::CAkSinkWasapi()
	: CAkSink(AkSink_Main_Wasapi)
	, m_pDeviceOut( NULL )
	, m_pClientOut( NULL )
	, m_pRenderClient( NULL )
	, m_uBufferFrames( 0 )
	, m_uNumChannels( 0 )
	, m_hEvent(NULL)
{
}

CAkSinkWasapi::~CAkSinkWasapi()
{
	if( m_MasterOut.HasData() )
	{
		AkFalign( g_LEngineDefaultPoolId, m_MasterOut.GetContiguousDeinterleavedData() );
		m_MasterOut.ClearData();
	}
}

AKRESULT CAkSinkWasapi::Init( interface IMMDeviceEnumerator * in_pEnumerator, AkChannelMask in_uChannelMask, AkSinkType in_eType )
{
	AKASSERT( in_pEnumerator );

	HRESULT hr;

	hr = in_pEnumerator->GetDefaultAudioEndpoint( eRender, eConsole, &m_pDeviceOut );
	if ( hr != S_OK )
		return AK_Fail;

	hr = m_pDeviceOut->Activate( __uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**) &m_pClientOut );
	if ( hr != S_OK )
		return AK_Fail;

	REFERENCE_TIME timeDefaultPeriod, timeMinPeriod;
	hr = m_pClientOut->GetDevicePeriod( &timeDefaultPeriod, &timeMinPeriod );

	// first start with the time for a refill.
	REFERENCE_TIME timeBuffer = (REFERENCE_TIME) MFTIMES_PER_MILLISEC * 1000 * AK_NUM_VOICE_REFILL_FRAMES / AK_CORE_SAMPLERATE;

	// now make sure we are going to do at least double-buffering of the system's buffer size
	// and use at least the min period provided by the hardware.

#ifdef AK_XBOXONE
	m_SpeakersConfig = in_uChannelMask;

	if (in_uChannelMask == AK_CHANNEL_MASK_DETECT)
	{
		UINT spdifCount;
		UINT hdmiCount;
		IMMXboxDeviceEnumerator *pXBoxEnum = NULL;
		in_pEnumerator->QueryInterface(__uuidof(IMMXboxDeviceEnumerator), (void**)&pXBoxEnum);
		if (pXBoxEnum)
		{
			pXBoxEnum->GetHdAudioChannelCounts(&hdmiCount, &spdifCount);
			pXBoxEnum->Release();

			switch (AkMax(hdmiCount, spdifCount))
			{
				case 2: m_SpeakersConfig = AK_SPEAKER_SETUP_STEREO; break;
				case 6: //Always use 8 channels.  WASAPI endpoints only supports stereo and 8.
				case 8: m_SpeakersConfig = AK_SPEAKER_SETUP_7_1; break;
				default: AKASSERT(!"Unsupported HW channel config");
					break;
			}
		}

		if (m_SpeakersConfig == AK_CHANNEL_MASK_DETECT)
		{
			//Still in detect mode?  Something failed.  Init in 7.1, the console will downmix anyway.
			m_SpeakersConfig = AK_SPEAKER_SETUP_7_1;
		}
	}
#else
	// FIXME: apparently GetDevicePeriod gives values that are 'off' by a factor of 10
	timeMinPeriod *= 10;
	m_SpeakersConfig = AK_SPEAKER_SETUP_7_1;
#endif

	timeBuffer = max( 2*timeBuffer, timeMinPeriod );
	m_uNumChannels = AK::GetNumChannels(m_SpeakersConfig);

	WAVEFORMATEXTENSIBLE wfExt;
	wfExt.dwChannelMask = m_SpeakersConfig;
	wfExt.Format.nChannels = (WORD)m_uNumChannels;
	wfExt.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
	wfExt.Format.nSamplesPerSec = AK_CORE_SAMPLERATE;
	wfExt.Format.nBlockAlign = wfExt.Format.nChannels * sizeof( AkReal32 );
	wfExt.Format.nAvgBytesPerSec = wfExt.Format.nSamplesPerSec * wfExt.Format.nBlockAlign;
	wfExt.Format.wBitsPerSample = sizeof( AkReal32 ) * 8;
	wfExt.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);

	wfExt.Samples.wValidBitsPerSample = 32;
	wfExt.SubFormat = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;

#if !defined AK_XBOXONE
	WAVEFORMATEX * pWfSupported = NULL;
	hr = m_pClientOut->IsFormatSupported( AUDCLNT_SHAREMODE_SHARED, (WAVEFORMATEX *) &wfExt, &pWfSupported );

	if ( pWfSupported ) 
	{
		// Format is not directly supported; we can handle changes in number of channels and
		// sample rate.
		if( AK_CORE_SAMPLERATE != pWfSupported->nSamplesPerSec )
		{
			// Wasapi sink is supported only if the setup is in AK_CORE_SAMPLERATE.
			// Aparently, it works if the system is in 44100 once you ignore some asserts, but we did not 
			// test it properly yet, so we better not enable it for now.
			// User should set its output to be 48000 to get the best of the latency.
			// Returning a failure code will result in the directSound Sink to be created instead.

			::CoTaskMemFree( pWfSupported );
			pWfSupported = NULL;
			return AK_Fail;
		}
		else
		{
			AkAudioLibSettings::SetSampleFrequency( pWfSupported->nSamplesPerSec );
			wfExt.Format.nSamplesPerSec = pWfSupported->nSamplesPerSec;

			if ( pWfSupported->nChannels == 8 )
			{
				m_SpeakersConfig = AK_SPEAKER_SETUP_7POINT1;
			}
			else if ( pWfSupported->nChannels == 6 )
			{
				m_SpeakersConfig = AK_SPEAKER_SETUP_5POINT1;
			}
			else
			{
				m_SpeakersConfig = AK_SPEAKER_SETUP_STEREO;				
			}

			wfExt.Format.nChannels = (WORD)AK::GetNumChannels( m_SpeakersConfig );

			wfExt.dwChannelMask = m_SpeakersConfig;
			wfExt.Format.nBlockAlign = pWfSupported->nChannels * sizeof( AkReal32 );
			wfExt.Format.nAvgBytesPerSec = wfExt.Format.nSamplesPerSec * pWfSupported->nBlockAlign;

			::CoTaskMemFree( pWfSupported );
			pWfSupported = NULL;
		}
	}
	else // Format was accepted 'as-is'.
	{
		m_SpeakersConfig = wfExt.dwChannelMask;
	}
#else 
	IAudioClient2 *pAudioClient2 = NULL;
	hr = m_pClientOut->QueryInterface(__uuidof(IAudioClient2), (void**)&pAudioClient2);
	{
		AudioClientProperties props;
		props.eCategory = AudioCategory_SoundEffects;
		switch(in_eType)
		{
		case AkSink_BGM: 
			hr = pAudioClient2->ExcludeFromGameDVRCapture();
			AKASSERT(hr == S_OK);	//Not critical
			break;
		case AkSink_Communication: props.eCategory = AudioCategory_Communications; break;
		};

		props.bIsOffload = false;
		pAudioClient2->IsOffloadCapable(props.eCategory, &props.bIsOffload);
		AkUInt32 minSamples = ((AkUInt32)timeMinPeriod * AK_CORE_SAMPLERATE)/(MFTIMES_PER_MILLISEC*1000);
		AKASSERT(minSamples < AK_NUM_VOICE_REFILL_FRAMES * 2);
		props.cbSize = (UINT32)(AK_NUM_VOICE_REFILL_FRAMES * 2 * m_uNumChannels * sizeof(AkReal32));

		hr = pAudioClient2->SetClientProperties(&props);
		AKASSERT(hr == S_OK);
		pAudioClient2->Release();
	}
#endif

	AkUInt32 flags = 0;
	if (in_eType == AkSink_Main || in_eType == AkSink_Main_Wasapi)
		flags = AUDCLNT_STREAMFLAGS_EVENTCALLBACK;

	hr = m_pClientOut->Initialize( AUDCLNT_SHAREMODE_SHARED, flags, timeBuffer, 0, (WAVEFORMATEX *) &wfExt, NULL );
	if ( hr != S_OK )
		return AK_Fail;

	//Only the main output has an event to wakeup the processing thread.  The other sinks will be slaved.
	if (in_eType == AkSink_Main || in_eType == AkSink_Main_Wasapi)
	{
		hr = m_pClientOut->SetEventHandle(CAkLEngine::GetProcessEvent());
		if ( hr != S_OK )
			return AK_Fail;	
	}

	hr = m_pClientOut->GetBufferSize( (UINT32*)&m_uBufferFrames );
	if ( hr != S_OK )
		return AK_Fail;	

	//AKASSERT( !( m_uBufferFrames % 1024 ) ); // result of our calculations should be a multiple of 1024 frames

	hr = m_pClientOut->GetService( __uuidof(IAudioRenderClient), (void **) &m_pRenderClient );
	if ( hr != S_OK )
		return AK_Fail;

	if (AllocBuffer() != AK_Success)
		return AK_Fail;

	return AK_Success;
}

// CAkSink overrides

AKRESULT CAkSinkWasapi::Play()
{
	AKASSERT( m_pClientOut );
	m_pClientOut->Start();
	return AK_Success;
}

void CAkSinkWasapi::Term()
{
	if ( m_pRenderClient )
	{
		m_pRenderClient->Release();
		m_pRenderClient = NULL;
	}

	if ( m_pClientOut )
	{
		m_pClientOut->Release();
		m_pClientOut = NULL;
	}

	if ( m_pDeviceOut )
	{
		m_pDeviceOut->Release();
		m_pDeviceOut = NULL;
	}

	StopOutputCapture();

	CAkSink::Term();
}

bool CAkSinkWasapi::IsStarved()
{
	return false; // FIXME: no starvation detection
}

DWORD CAkSinkWasapi::GetThreadWaitTime()
{
	return AK_INFINITE;
}

void CAkSinkWasapi::ResetStarved()
{
}

AKRESULT CAkSinkWasapi::IsDataNeeded( AkUInt32 & out_uBuffersNeeded )
{
	UINT32 uPaddingFrames;

	HRESULT hr = m_pClientOut->GetCurrentPadding( &uPaddingFrames );
	if ( hr == AK_Fail ) 
		return AK_Fail;

	out_uBuffersNeeded = ( m_uBufferFrames - uPaddingFrames ) / AK_NUM_VOICE_REFILL_FRAMES;

	return AK_Success;
}

static void ClipFloats( float * dst, float * src, int nFloats )
{
	AKASSERT( !( nFloats % 16 ) ); // operates on 16 floats at a time

	__m128 * pmIn = (__m128 *) src;
	__m128 * pmEnd = (__m128 *) ( src + nFloats );

	__m128 mMin = _mm_set_ps1( AUDIOSAMPLE_FLOAT_MIN ); // duplicate multiplier factor 4x
	__m128 mMax = _mm_set_ps1( AUDIOSAMPLE_FLOAT_MAX ); // duplicate multiplier factor 4x

	while ( pmIn < pmEnd )
	{
		__m128 mTmp1 = _mm_min_ps( pmIn[ 0 ], mMax );
		__m128 mTmp2 = _mm_min_ps( pmIn[ 1 ], mMax );
		__m128 mTmp3 = _mm_min_ps( pmIn[ 2 ], mMax );
		__m128 mTmp4 = _mm_min_ps( pmIn[ 3 ], mMax );

		//      looks like we can get an unaligned output buffer from Wasapi, so we can't use these.
		//		_mm_stream_ps( dst, _mm_max_ps( mTmp1, mMin ) );
		//		_mm_stream_ps( dst + 4, _mm_max_ps( mTmp2, mMin ) );
		//		_mm_stream_ps( dst + 8, _mm_max_ps( mTmp3, mMin ) );
		//		_mm_stream_ps( dst + 12, _mm_max_ps( mTmp4, mMin ) );
		_mm_storeu_ps( dst, _mm_max_ps( mTmp1, mMin ) );
		_mm_storeu_ps( dst + 4, _mm_max_ps( mTmp2, mMin ) );
		_mm_storeu_ps( dst + 8, _mm_max_ps( mTmp3, mMin ) );
		_mm_storeu_ps( dst + 12, _mm_max_ps( mTmp4, mMin ) );

		pmIn += 4;
		dst += 16;
	}
}

AKRESULT CAkSinkWasapi::PassData()
{
	BYTE * pData = NULL;

	HRESULT hr = m_pRenderClient->GetBuffer( m_MasterOut.uValidFrames, &pData );
	if ( hr != S_OK || pData == NULL )
		return AK_Fail;

	void*	pvFrom = m_MasterOut.GetInterleavedData();

#ifndef AK_OPTIMIZED
	UpdateProfileData( (float *) pvFrom, m_MasterOut.uValidFrames * m_MasterOut.NumChannels() );
#endif

	AkUInt32 uNumSamplesRefill = m_MasterOut.uValidFrames * m_MasterOut.NumChannels();
	ClipFloats( (float *) pData, (float *) pvFrom, uNumSamplesRefill );

	if (m_pCapture != NULL)
	{
		AkUInt32 uNumBytesRefill = uNumSamplesRefill * sizeof(AkUInt16);
		void * pBuf = alloca( uNumBytesRefill );
		FloatsToShorts( (AkInt16 *) pBuf, (float *) pvFrom, uNumSamplesRefill );
		m_pCapture->PassSampleData(pBuf, uNumBytesRefill);
	}

	hr = m_pRenderClient->ReleaseBuffer( m_MasterOut.uValidFrames, 0 );

	return AK_Success;
}

AKRESULT CAkSinkWasapi::PassSilence()
{
	BYTE * pData = NULL;

	HRESULT hr = m_pRenderClient->GetBuffer( AK_NUM_VOICE_REFILL_FRAMES, &pData );
	if ( hr != S_OK || pData == NULL )
		return AK_Fail;

#ifndef AK_OPTIMIZED
	UpdateProfileSilence( AK_NUM_VOICE_REFILL_FRAMES * m_uNumChannels );
#endif
	if (m_pCapture != NULL)
	{
		AkUInt32 uNumBytesRefill = AK_NUM_VOICE_REFILL_FRAMES * m_uNumChannels * sizeof(AkUInt16);
		void * pBuf = alloca( uNumBytesRefill );
		memset( pBuf, 0, uNumBytesRefill );
		m_pCapture->PassSampleData( pBuf, uNumBytesRefill );
	}

	hr = m_pRenderClient->ReleaseBuffer( AK_NUM_VOICE_REFILL_FRAMES, AUDCLNT_BUFFERFLAGS_SILENT );

	return AK_Success;
}
#endif // AK_WASAPI