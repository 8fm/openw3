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
#include "AkSinkXAudio2.h"

#ifdef AK_XAUDIO2

#include "process.h"
#include "AkMath.h"
#include "AkMonitor.h"
#include "AkProfile.h"
#include "AudiolibDefs.h"
#include "AkSettings.h"
#include "AkAudioMgr.h"
#include "AkOutputMgr.h"

#include <AK/SoundEngine/Common/AkSimd.h>

#ifdef AK_XBOX360
	#include "XMAHardwareAbstraction.h"
#endif

#include <AK/SoundEngine/Common/IAkStreamMgr.h>

extern AkPlatformInitSettings g_PDSettings;
#define AK_NUM_VOICE_BUFFERS (g_PDSettings.uNumRefillsInVoice)

#define	AK_PRIMARY_BUFFER_SAMPLE_TYPE			AkReal32

#if defined AK_CPU_X86 || defined AK_CPU_X86_64
static const AKSIMD_V4F32 vkMin = { (AkReal32) INT_MIN, (AkReal32) INT_MIN, (AkReal32) INT_MIN, (AkReal32) INT_MIN };
static const AKSIMD_V4F32 vkMax = { (AkReal32) INT_MAX, (AkReal32) INT_MAX, (AkReal32) INT_MAX, (AkReal32) INT_MAX };
#endif

#if defined AK_CPU_ARM_NEON
static const DirectX::XMVECTORF32 vkMin = { (AkReal32) INT_MIN, (AkReal32) INT_MIN, (AkReal32) INT_MIN, (AkReal32) INT_MIN };
static const DirectX::XMVECTORF32 vkMax = { (AkReal32) INT_MAX, (AkReal32) INT_MAX, (AkReal32) INT_MAX, (AkReal32) INT_MAX };
#endif

XAudio2Sinks CAkSinkXAudio2::s_XAudio2List;

CAkSinkXAudio2::CAkSinkXAudio2()
	: CAkSink(AkSink_Main_XAudio2)
    , m_ulNumChannels( 0 )
	, m_pXAudio2( NULL )
	, m_pMasteringVoice( NULL )
	, m_pSourceVoice( NULL )
	, m_pvAudioBuffer( NULL )
	, m_uWriteBufferIndex( 0 )
	, m_uReadBufferIndex( 0 )
	, m_uNbBuffersRB( 0 )
	, m_usBlockAlign( 0 )
	, m_bStarved( false )
	, m_bCriticalError( false )
	, pNextLightItem( NULL )
{
	for ( AkUInt32 i = 0; i < AK_MAX_NUM_VOICE_BUFFERS; ++i )
		m_ppvRingBuffer[i] = NULL;
}

CAkSinkXAudio2::~CAkSinkXAudio2()
{
}

AKRESULT CAkSinkXAudio2::Init( AkPlatformInitSettings & in_settings, AkChannelMask in_uChannelMask, AkSinkType in_eType )
{
	// Create XAudio2 engine with default options
	UINT32 Flags = AK_XAUDIO2_FLAGS;
	HRESULT hr;
	if (in_settings.pXAudio2 == NULL)
	{
		hr = XAudio2Create( &m_pXAudio2, Flags, XAUDIO2_DEFAULT_PROCESSOR );
		if(  hr != S_OK )
		{
			// Failure here usually means that XAudio2 is not installed on the system.
			return AK_Fail;
		}
	}
	else
	{
		m_pXAudio2 = in_settings.pXAudio2;
		m_pXAudio2->AddRef();
	}

	AKASSERT( m_pXAudio2 );

	// Create a 6-channel 48 kHz mastering voice feeding the default device
#ifdef AK_XBOXONE
	AUDIO_STREAM_CATEGORY eCategory = (in_eType == AkSink_Communication ? AudioCategory_Communications : AudioCategory_SoundEffects);	
	UINT flags = 0;

#ifndef AK_XBOXONE_ADK
	if (in_eType == AkSink_BGM || in_eType == AkSink_Communication)
		flags = XAUDIO2_EXCLUDE_FROM_GAME_DVR_CAPTURE ;
#endif
	
	if( !SUCCEEDED( m_pXAudio2->CreateMasteringVoice( &m_pMasteringVoice, XAUDIO2_DEFAULT_CHANNELS, AK_CORE_SAMPLERATE, flags, NULL, NULL, eCategory) ) )
#else
	if( !SUCCEEDED( m_pXAudio2->CreateMasteringVoice( &m_pMasteringVoice, XAUDIO2_DEFAULT_CHANNELS, AK_CORE_SAMPLERATE) ) )
#endif
	{
#ifdef _DEBUG
		AKPLATFORM::OutputDebugMsg( "Audiokinetic Wwise: Unable to create XAudio2 mastering voice\n" );
#endif
		return AK_Fail;
	}

	m_pXAudio2->RegisterForCallbacks( this );

#ifdef AK_XBOX360

	XMAPlaybackInitialize();// Required to be called before any call to XMA XDK. We simply always initialize it.

	// At least PL2 is always present on XBox360
	m_SpeakersConfig = AK_SPEAKER_SETUP_5POINT1;

#else

	m_SpeakersConfig = in_uChannelMask;

	// Get current channel mask from xaudio2 device
	if( m_SpeakersConfig == AK_CHANNEL_MASK_DETECT )
	{
#if defined(AK_USE_METRO_API)
		DWORD dwChannelMask;
		if ( !SUCCEEDED( m_pMasteringVoice->GetChannelMask( &dwChannelMask ) ) )
		{
			AKASSERT( !"Unable to get mastering voice channel mask" );
			return AK_Fail;
		}
		AkUInt32 uChannelCount = AK::GetNumChannels( dwChannelMask );
#else
		XAUDIO2_DEVICE_DETAILS deviceDetails;
		if( !SUCCEEDED( m_pXAudio2->GetDeviceDetails( 0, &deviceDetails ) ) )
		{
			AKASSERT(!"Unable to query XAudio2 mastering voice");
			return AK_Fail;
		}
		AkUInt32 uChannelCount = deviceDetails.OutputFormat.Format.nChannels;
#endif	
#ifdef AK_71AUDIO
		if ( uChannelCount >= 7 )
			m_SpeakersConfig = AK_SPEAKER_SETUP_7POINT1;
		else
#endif
		if ( uChannelCount >= 4 )
			m_SpeakersConfig = AK_SPEAKER_SETUP_5POINT1;
		else
			m_SpeakersConfig = AK_SPEAKER_SETUP_STEREO;
	}
#endif
	
	m_ulNumChannels = AK::GetNumChannels( m_SpeakersConfig );

	if (AllocBuffer() != AK_Success)
		return AK_Fail;

	// get things ready for outputs routing

	if ( m_SpeakersConfig != AK_SPEAKER_SETUP_STEREO 
		&& m_SpeakersConfig != AK_SPEAKER_SETUP_5POINT1
#ifdef AK_71AUDIO
		&& m_SpeakersConfig != AK_SPEAKER_SETUP_7POINT1 
#endif
		)
	{
		AKASSERT(!"Unsupported number of channels");
		return AK_Fail;
	}	
	m_usBlockAlign = (AkUInt16) ( m_ulNumChannels * sizeof(AkReal32) );
	AkUInt32 uBufferBytes = AK_NUM_VOICE_REFILL_FRAMES * m_usBlockAlign;

	// Allocate ring buffer 
	m_pvAudioBuffer = AkAlloc( g_LEngineDefaultPoolId, AK_MAX_NUM_VOICE_BUFFERS*uBufferBytes );
	if ( m_pvAudioBuffer == NULL )
	{
		return AK_Fail;
	}

	::ZeroMemory( m_pvAudioBuffer, AK_MAX_NUM_VOICE_BUFFERS*uBufferBytes );

	// Initialize ring buffer ptrs
	for ( AkUInt32 i = 0; i < AK_MAX_NUM_VOICE_BUFFERS; ++i )
		m_ppvRingBuffer[i] = (AkUInt8*)m_pvAudioBuffer + i*uBufferBytes;

	// try to create the source voice
	WAVEFORMATEXTENSIBLE formatX;
	formatX.dwChannelMask = m_SpeakersConfig; 
	formatX.SubFormat = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
	formatX.Samples.wValidBitsPerSample = 32;
	formatX.Format.nChannels = (WORD)m_ulNumChannels;
	formatX.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
	formatX.Format.nSamplesPerSec = AK_CORE_SAMPLERATE;
	formatX.Format.nBlockAlign = formatX.Format.nChannels * sizeof( AkReal32 );
	formatX.Format.nAvgBytesPerSec = formatX.Format.nSamplesPerSec * formatX.Format.nBlockAlign;
	formatX.Format.wBitsPerSample = sizeof( AkReal32 ) * 8;
	formatX.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);

	if( FAILED( m_pXAudio2->CreateSourceVoice( &m_pSourceVoice, (WAVEFORMATEX*)&(formatX), 0, 1.0, this ) ) )
		return AK_Fail;

	m_pSourceVoice->SetVolume(1.0f);

	//Synchronize this sink with the main sink too.  This is necessary to avoid starvation of the slave sink in a multi-sink environment
	//So start the ring buffer *exactly* at the same place as the main sink is.
	AkDevice * pDevice = CAkOutputMgr::GetDevice(AK_MAIN_OUTPUT_DEVICE);
	if (pDevice && pDevice->pSink && pDevice->pSink->GetType() == AkSink_Main_XAudio2)
	{
		CAkSinkXAudio2* pMainSink = (CAkSinkXAudio2*)pDevice->pSink;
		m_uWriteBufferIndex = pMainSink->m_uWriteBufferIndex;		
		m_uNbBuffersRB = pMainSink->m_uNbBuffersRB;
		m_uReadBufferIndex = 0;
		
		for(AkInt32 i = 0; i < m_uNbBuffersRB; i++)
			SubmitPacketRB();
	}

	s_XAudio2List.AddFirst(this);
	
	return AK_Success;
}

void CAkSinkXAudio2::Term()
{
	// anyone got lost in here ?
	Stop();

	if( m_MasterOut.HasData() )
	{
		AkFalign( g_LEngineDefaultPoolId, m_MasterOut.GetContiguousDeinterleavedData() );
		m_MasterOut.ClearData();
	}

	if( m_pMasteringVoice )
	{
		m_pMasteringVoice->DestroyVoice();
		m_pMasteringVoice = NULL;
	}

	if( m_pXAudio2 )
	{
		m_pXAudio2->UnregisterForCallbacks( this );
		m_pXAudio2->Release();
		m_pXAudio2 = NULL;
	}

	if ( m_pvAudioBuffer )
	{
		AkFree( g_LEngineDefaultPoolId, m_pvAudioBuffer );
		m_pvAudioBuffer = NULL;
	}

	StopOutputCapture();

	CAkSink::Term();

	s_XAudio2List.Remove(this);	
}

AKRESULT CAkSinkXAudio2::Play()
{
	HRESULT hr = m_pSourceVoice->Start(0);
	return SUCCEEDED( hr ) ? AK_Success : AK_Fail;
}

void CAkSinkXAudio2::Stop()
{
	if( m_pSourceVoice )//must check since Stop may be called on Term after the Init Failed
	{
		m_pSourceVoice->DestroyVoice(); // call this before clearing buffer to avoid race with voice callback
		m_pSourceVoice = NULL;
	}

	// Clear our ring buffer
	m_uWriteBufferIndex = 0;
	m_uReadBufferIndex = 0;
	m_uNbBuffersRB = 0;
	for ( AkUInt32 i = 0; i < AK_MAX_NUM_VOICE_BUFFERS; ++i )
		m_ppvRingBuffer[i] = NULL;
}

DWORD CAkSinkXAudio2::GetThreadWaitTime()
{
	return AK_INFINITE; // XAudio2 callback will wake up the audio thread when the time comes.
}

bool CAkSinkXAudio2::IsStarved()
{
	return m_bStarved;
}

void CAkSinkXAudio2::ResetStarved()
{
	m_bStarved = false;
}

AkUInt16 CAkSinkXAudio2::IsDataNeededRB( )
{
	return (AkUInt16) ( AK_NUM_VOICE_BUFFERS - m_uNbBuffersRB );
}

void CAkSinkXAudio2::SubmitPacketRB( )
{
	// Build and submit the packet
	XAUDIO2_BUFFER XAudio2Buffer = { 0 };
	XAudio2Buffer.AudioBytes = AK_NUM_VOICE_REFILL_FRAMES * m_usBlockAlign;
	XAudio2Buffer.pAudioData = (BYTE*)( m_ppvRingBuffer[m_uReadBufferIndex] );
	++m_uReadBufferIndex;

	// Reset the index when required.
	if ( m_uReadBufferIndex == AK_MAX_NUM_VOICE_BUFFERS )
		m_uReadBufferIndex = 0;

	HRESULT hResult = m_pSourceVoice->SubmitSourceBuffer(&XAudio2Buffer);
	AKASSERT( SUCCEEDED(hResult) );

	// Do not release the buffer in ring buffer here, as SubmitSourceBuffer holds the pointer
	// until OnBufferEnd.
}

//====================================================================================================
// this is the entry point for passing buffers
//====================================================================================================
AKRESULT CAkSinkXAudio2::PassData()
{
	AKASSERT( m_pSourceVoice );

	AkUInt32 uRefilledFrames = m_MasterOut.uValidFrames;
	AKASSERT( uRefilledFrames != 0 );

	void* pvBuffer = m_ppvRingBuffer[m_uWriteBufferIndex];

	AkUInt8* pbSourceData = (AkUInt8*)m_MasterOut.GetInterleavedData();

	AkInt32 lNumSamples = uRefilledFrames * m_ulNumChannels;
	AKASSERT( !( lNumSamples % 4 ) ); // we operate 4 samples at a time.

#ifndef AK_OPTIMIZED
	UpdateProfileData( (float *) pbSourceData, lNumSamples );
#endif

	if (m_bWaitForMerge && !s_MergingSinks.IsEmpty())
		return AK_Success;

	// Clip samples to [-1.0, 1.0]
	AKSIMD_V4F32 * AK_RESTRICT pvFrom = (AKSIMD_V4F32 *) pbSourceData;
	AKSIMD_V4F32 * AK_RESTRICT pvEnd = (AKSIMD_V4F32 *) ( (AkReal32 *) pbSourceData + lNumSamples );
	AKSIMD_V4F32 * AK_RESTRICT pvTo = (AKSIMD_V4F32 *) pvBuffer;

	for ( ; pvFrom < pvEnd; pvFrom++, pvTo++ )
		*pvTo = AKSIMD_MAX_V4F32( vkMin, AKSIMD_MIN_V4F32( vkMax, *pvFrom ) );

	// write to output file 
	if (m_pCapture != NULL)
	{
#ifdef AK_XBOX360
		m_pCapture->PassSampleData(pvBuffer, lNumSamples * sizeof(AK_PRIMARY_BUFFER_SAMPLE_TYPE));
#else
		// Currently sampling down to int16 so we can use the same capture.
		void * pBuf = alloca( lNumSamples * sizeof(AkInt16) );

		FloatsToShorts( (AkInt16 *) pBuf, (float *) pvBuffer, lNumSamples );

		m_pCapture->PassSampleData(pBuf, lNumSamples * sizeof(AkInt16));
#endif //XBOX360
	}

	++m_uWriteBufferIndex;
	if ( m_uWriteBufferIndex == AK_MAX_NUM_VOICE_BUFFERS  )
		m_uWriteBufferIndex = 0;

	LONG lNbBuffers = ::InterlockedIncrement( &m_uNbBuffersRB );
	AKASSERT( lNbBuffers <= AK_NUM_VOICE_BUFFERS );

	return AK_Success;
}

AKRESULT CAkSinkXAudio2::PassSilence()
{
	AKASSERT( m_pSourceVoice );

	void* pvBuffer = m_ppvRingBuffer[m_uWriteBufferIndex];

#ifndef AK_OPTIMIZED
	UpdateProfileSilence( AK_NUM_VOICE_REFILL_FRAMES * m_usBlockAlign / sizeof( AkReal32 ) );
#endif
	if (m_bWaitForMerge && !s_MergingSinks.IsEmpty())
	{
		AkZeroMemLarge(m_MasterOut.GetInterleavedData(), AK_NUM_VOICE_REFILL_FRAMES * m_MasterOut.NumChannels() * sizeof(AkReal32));
		return AK_Success;
	}

	AkInt32 lNumSamples = AK_NUM_VOICE_REFILL_FRAMES * m_ulNumChannels;

	AKSIMD_V4F32 * AK_RESTRICT pvTo = (AKSIMD_V4F32 *) pvBuffer;
	AKSIMD_V4F32 * AK_RESTRICT pvEnd = (AKSIMD_V4F32 *) ( (AkReal32 *) pvBuffer + lNumSamples );

	AKSIMD_V4F32 vZero = AKSIMD_SETZERO_V4F32(); 
	for ( ; pvTo < pvEnd; pvTo++ )
		*pvTo = vZero;

	int dataSize = AK_NUM_VOICE_REFILL_FRAMES * m_usBlockAlign;

	if (m_pCapture != NULL)
	{
#ifndef XBOX360
		// Hardcoded down so we can compare both XAudio2 and DirectSound sinks in 16 bits samples.
		// Pushing int16 instead of floats, half the size.
		m_pCapture->PassSampleData(pvBuffer, dataSize/2);
#else
		m_pCapture->PassSampleData(pvBuffer, dataSize);
#endif
	}

	++m_uWriteBufferIndex;
	if ( m_uWriteBufferIndex == AK_MAX_NUM_VOICE_BUFFERS )
		m_uWriteBufferIndex = 0;

	LONG lNbBuffers = ::InterlockedIncrement( &m_uNbBuffersRB );
	AKASSERT( lNbBuffers <= AK_NUM_VOICE_BUFFERS );

	return AK_Success;
}

AKRESULT CAkSinkXAudio2::IsDataNeeded( AkUInt32 & out_uBuffersNeeded )
{
	// All the XAudio sinks need to be synchronized with the rendering thread.  Due to threading timings, we need to query all the sinks at the same time.
	// Normally, IsDataNeededRB should return the same thing, except when the threads are switching at the wrong time...  
	// Therefore, only return the number of buffers we are sure we have space for in all sinks.
	out_uBuffersNeeded = 1000;
	for(XAudio2Sinks::Iterator it = s_XAudio2List.Begin(); it != s_XAudio2List.End(); ++it)
		out_uBuffersNeeded = min(out_uBuffersNeeded, (*it)->IsDataNeededRB());

	// WG-19498: do not submit data from this thread, as this can cause a deadlock.
	return m_bCriticalError ? AK_Fail : AK_Success;
}

//=================================================
//	IXAudio2EngineCallback interface implementation
//=================================================	 

void CAkSinkXAudio2::OnProcessingPassStart()
{
}

void CAkSinkXAudio2::OnProcessingPassEnd()
{
}

void CAkSinkXAudio2::OnCriticalError( HRESULT Error )
{
	m_bCriticalError = true;
	g_pAudioMgr->WakeupEventsConsumer();
}

//=================================================
//	IXAudio2VoiceCallback interface implementation
//=================================================

// Called just before this voice's processing pass begins.
void CAkSinkXAudio2::OnVoiceProcessingPassStart( UINT32 BytesRequired )
{
	XAUDIO2_VOICE_STATE X2VoiceState;
	m_pSourceVoice->GetState( &X2VoiceState );

	if ( X2VoiceState.BuffersQueued < AK_NUM_VOICE_BUFFERS )
	{
		// set only, don't reset; do not report initial starvation (right after voice start)
		if ( !X2VoiceState.BuffersQueued && X2VoiceState.SamplesPlayed )
			m_bStarved = true;

		// Submit all buffers that have not been submitted yet.
		while ( m_uNbBuffersRB - X2VoiceState.BuffersQueued )
		{
			SubmitPacketRB();
			X2VoiceState.BuffersQueued++;
		}

		if ( X2VoiceState.BuffersQueued < AK_NUM_VOICE_BUFFERS )
			g_pAudioMgr->WakeupEventsConsumer(); // Voice still not 'full': trigger buffer generation now.
	}
}

// Called just after this voice's processing pass ends.
void CAkSinkXAudio2::OnVoiceProcessingPassEnd()
{
	//Not used
}

// Called when this voice has just finished playing a buffer stream
// (as marked with the XAUDIO2_END_OF_STREAM flag on the last buffer).
void CAkSinkXAudio2::OnStreamEnd()
{
	//Not used
}

// Called when this voice is about to start processing a new buffer.
void CAkSinkXAudio2::OnBufferStart( void* pBufferContext)
{
	//Not used
}

// Called when this voice has just finished processing a buffer.
// The buffer can now be reused or destroyed.
void CAkSinkXAudio2::OnBufferEnd( void* pBufferContext)
{
	LONG lNbBuffers = ::InterlockedDecrement( &m_uNbBuffersRB );
	AKASSERT( lNbBuffers >= 0 );

	g_pAudioMgr->WakeupEventsConsumer();
}

// Called when this voice has just reached the end position of a loop.
void CAkSinkXAudio2::OnLoopEnd( void* pBufferContext)
{
	//Not used
}

// Called in the event of a critical error during voice processing,
// such as a failing XAPO or an error from the hardware XMA decoder.
// The voice may have to be destroyed and re-created to recover fromF
// the error.  The callback arguments report which buffer was being
// processed when the error occurred, and its HRESULT code.
void CAkSinkXAudio2::OnVoiceError( void* pBufferContext, HRESULT Error )
{
	//Not used
}

IXAudio2* CAkSinkXAudio2::GetWwiseXAudio2Interface()
{
	return m_pXAudio2;
}

#endif
