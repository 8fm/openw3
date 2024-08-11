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
#include "AkSinkDirectSound.h"

#ifdef AK_DIRECTSOUND

#include "AkSettings.h"

extern AkPlatformInitSettings g_PDSettings;
#define AK_NUM_VOICE_BUFFERS (g_PDSettings.uNumRefillsInVoice)
#define AK_HARD_OUT_SUB_BUFFER_SIZE (1024)

#define AK_PRIMARY_BUFFER_FORMAT				WAVE_FORMAT_PCM
#define	AK_PRIMARY_BUFFER_BITS_PER_SAMPLE		(16)
#define	AK_PRIMARY_BUFFER_SAMPLE_TYPE			AkInt16

LPDIRECTSOUND8 CAkSinkDirectSound::m_pDirectSound = NULL;

CAkSinkDirectSound::CAkSinkDirectSound()
: CAkSink(AkSink_Main_DirectSound)
{
	// no direct sound
	m_pDirectSound = NULL;

	// memory
	m_ulBufferSize = 0;
	m_lRefillToEnd = 0;
	m_ulRefillOffset = 0;
	m_uFreeRefillFrames = 0;
	m_uPlay = 0;
	m_uWrite = 0;

	// DS
	m_pDSBuffer = NULL;

	// flags
	m_bStarved = false;
}

CAkSinkDirectSound::~CAkSinkDirectSound()
{
}

AKRESULT CAkSinkDirectSound::Init(AkPlatformInitSettings & in_settings, AkChannelMask in_uChannelMask)
{
	DSBUFFERDESC	dsbd;

	// initialise DS

	HRESULT result = CoCreateInstance(CLSID_DirectSound8,NULL, CLSCTX_INPROC_SERVER, IID_IDirectSound8, (void**)&m_pDirectSound);
	if (FAILED(result))
	{
#if defined (_DEBUG)
		OutputDebugString(AKTEXT("WARNING: Unable to Create DirectSound Instance"));
#endif
		return AK_Fail;
	}

	result = m_pDirectSound->Initialize(NULL);

	if(FAILED(result))
	{
#if defined (_DEBUG)
		OutputDebugString(AKTEXT("WARNING: Unable to initialize DirectSound"));
#endif
		return AK_Fail;
	}

	if(FAILED(m_pDirectSound->SetCooperativeLevel(in_settings.hWnd, DSSCL_PRIORITY)))
	{
		AKASSERT(!"Unable to set cooperative level");
		return AK_Fail;
	}

	// create the primary buffer
	memset(&dsbd, 0, sizeof(dsbd));
	dsbd.dwSize = sizeof(dsbd);
	dsbd.dwFlags = DSBCAPS_PRIMARYBUFFER;

	if(FAILED(m_pDirectSound->CreateSoundBuffer(&dsbd, &m_pPrimaryBuffer, NULL)))
	{
		AKASSERT(!"Unable to create the primary buffer");
		return AK_Fail;
	}

	m_SpeakersConfig = in_uChannelMask;

	if( m_SpeakersConfig == AK_CHANNEL_MASK_DETECT )
	{
		m_SpeakersConfig = GetDSSpeakerConfig();
	}

	if( m_SpeakersConfig != AK_SPEAKER_SETUP_7POINT1 && 
		m_SpeakersConfig != AK_SPEAKER_SETUP_5POINT1 && 
		 m_SpeakersConfig != AK_SPEAKER_SETUP_STEREO )
	{
		// Default to stereo
		m_SpeakersConfig = AK_SPEAKER_SETUP_STEREO;
	}

	AKRESULT eResult = SetPrimaryBufferFormat();
	if ( eResult != AK_Success )
		return eResult;

	return AllocBuffer();
}


AkChannelMask CAkSinkDirectSound::GetDSSpeakerConfig()
{
	AkChannelMask channelConfig = AK_SPEAKER_SETUP_STEREO; // Return stereo by default

	AkUInt32 dwSpeakerConfig = 0;

	HRESULT hr =  m_pDirectSound->GetSpeakerConfig( &dwSpeakerConfig );
	if( SUCCEEDED( hr ) )
	{
		switch(DSSPEAKER_CONFIG(dwSpeakerConfig))
		{
		case DSSPEAKER_DIRECTOUT: // DirectOut: we don't know how many channels the device accepts: start at 7.1 and go down from there.
		case DSSPEAKER_7POINT1:
#if (DIRECTSOUND_VERSION >= 0x1000)
		case DSSPEAKER_7POINT1_SURROUND:
#else
		case 0x00000008: // DSSPEAKER_7POINT1_SURROUND
#endif
			channelConfig = AK_SPEAKER_SETUP_7POINT1;
			break;

		case DSSPEAKER_QUAD:
		case DSSPEAKER_5POINT1:
		case DSSPEAKER_SURROUND:
	// For some reason DSSPEAKER_5POINT1_SURROUND is not defined before version 10 whereas DSSPEAKER_7POINT1_SURROUND is.
#if (DIRECTSOUND_VERSION >= 0x1000)
		case DSSPEAKER_5POINT1_SURROUND:
#else
		case 0x00000009: // DSSPEAKER_5POINT1_SURROUND
#endif
			channelConfig = AK_SPEAKER_SETUP_5POINT1;
			break;
		
		case DSSPEAKER_STEREO:
		case DSSPEAKER_HEADPHONE:
		default: //Default, Use stereo.
			channelConfig = AK_SPEAKER_SETUP_STEREO;
			break;
		}
	}
	
	return channelConfig;
}

void CAkSinkDirectSound::Term()
{
	if( m_MasterOut.HasData() )
	{
		AkFalign( g_LEngineDefaultPoolId, m_MasterOut.GetContiguousDeinterleavedData() );
		m_MasterOut.ClearData();
	}

	if ( m_pDSBuffer )
	{
		// stop what's playing if needed
		m_pDSBuffer->Stop();

		// release the DS buffer
		m_pDSBuffer->Release();
		m_pDSBuffer = NULL;
	}

	if( m_pDirectSound )
	{
		m_pDirectSound->Release();
		m_pDirectSound = NULL;
	}

	StopOutputCapture();

	CAkSink::Term();
}

// Prepare a buffer format for DirectSound.
void CAkSinkDirectSound::PrepareFormat( AkChannelMask in_eChannelMask, WAVEFORMATEXTENSIBLE & out_wfExt )
{
	memset( &out_wfExt, 0, sizeof( WAVEFORMATEXTENSIBLE ) );

	AkUInt16 uNumChannels = (AkInt16)AK::GetNumChannels( in_eChannelMask );
	out_wfExt.Format.nChannels			= uNumChannels;
	out_wfExt.Format.nSamplesPerSec		= AK_CORE_SAMPLERATE;
	out_wfExt.Format.wBitsPerSample		= AK_PRIMARY_BUFFER_BITS_PER_SAMPLE;
	out_wfExt.Format.nBlockAlign		= sizeof(AK_PRIMARY_BUFFER_SAMPLE_TYPE) * uNumChannels;
	out_wfExt.Format.nAvgBytesPerSec	= AK_CORE_SAMPLERATE * out_wfExt.Format.nBlockAlign;

	if ( in_eChannelMask == AK_SPEAKER_SETUP_STEREO )
	{
		out_wfExt.Format.cbSize		= 0;
		out_wfExt.Format.wFormatTag	= AK_PRIMARY_BUFFER_FORMAT;
	}
	else // Use WAVEFORMATEXTENSIBLE for surround, as Vista+ requires it.
	{
		out_wfExt.Format.cbSize		= sizeof( WAVEFORMATEXTENSIBLE ) - sizeof( WAVEFORMATEX );
		out_wfExt.Format.wFormatTag	= WAVE_FORMAT_EXTENSIBLE;

		out_wfExt.dwChannelMask					= in_eChannelMask;
		out_wfExt.SubFormat						= KSDATAFORMAT_SUBTYPE_PCM;
		out_wfExt.Samples.wValidBitsPerSample	= out_wfExt.Format.wBitsPerSample;
	}
}

HRESULT CAkSinkDirectSound::CreateSecondaryBuffer()
{
	// Create a DSBuffer to play this sound
	DSBUFFERDESC desc;

	memset(&desc, 0, sizeof(desc));
	desc.dwSize = sizeof(desc);

	//TODO(alessard) check to not enable DSBCAPS if it is possible that it might not be used
	desc.dwFlags |= DSBCAPS_GETCURRENTPOSITION2;
	if( g_PDSettings.bGlobalFocus )
	{
		desc.dwFlags |= DSBCAPS_GLOBALFOCUS;
	}

	WAVEFORMATEXTENSIBLE wfExt;
	desc.lpwfxFormat = (WAVEFORMATEX *) &wfExt;

	AkChannelMask uChannelMask = GetSpeakerConfig();
	
	AkUInt16 uChannels = (AkUInt16)AK::GetNumChannels( uChannelMask );
	desc.dwBufferBytes = AK_NUM_VOICE_BUFFERS * AK_HARD_OUT_SUB_BUFFER_SIZE * uChannels * sizeof(AkInt16);
	m_usBlockAlign = uChannels*sizeof(AkInt16);
	
	PrepareFormat( uChannelMask, wfExt );

	// this is the size of our buffer
	m_ulBufferSize = desc.dwBufferBytes;

	// all is free to be re-filled
	m_lRefillToEnd = m_ulBufferSize;

	IDirectSoundBuffer* pDSB = NULL;
	void *		pData1 = NULL;
	AkUInt32	ulData1Bytes;
	void *		pData2 = NULL;
	AkUInt32	ulData2Bytes;

	HRESULT hr = m_pDirectSound->CreateSoundBuffer( &desc, &pDSB, NULL );
	if ( FAILED( hr ) )
	{
		return hr;
	}
	
	hr = pDSB->QueryInterface(IID_IDirectSoundBuffer8, (void**)(&m_pDSBuffer));
	if ( FAILED( hr ) )
	{
		AKASSERT(!"Could not get DS buffer");
		goto Failed;
	}

	hr = m_pDSBuffer->Lock(0, m_ulBufferSize,
									&pData1, &ulData1Bytes,
									&pData2, &ulData2Bytes,
									DSBLOCK_FROMWRITECURSOR );
	if ( FAILED( hr ) )
	{
		AKASSERT(!"Could not lock buffer");
		goto Failed;
	}

	::ZeroMemory( pData1, ulData1Bytes );

	hr = m_pDSBuffer->Unlock(pData1, ulData1Bytes, pData2, ulData2Bytes);
	if ( FAILED( hr ) )
	{
		AKASSERT(!"Could not unlock buffer");
		goto Failed;
	}

	pDSB->Release();
	pDSB = NULL;

	return S_OK;

Failed:
//	AssertHR(hr);

	if(m_pDSBuffer != NULL)
	{
		m_pDSBuffer->Release();
		m_pDSBuffer = NULL;
	}

	if(pDSB)
	{
		pDSB->Release();
	}

	return E_FAIL;
}

DWORD CAkSinkDirectSound::GetThreadWaitTime()
{
	return AK_PC_WAIT_TIME;
}

bool CAkSinkDirectSound::IsStarved()
{
	return m_bStarved;
}

void CAkSinkDirectSound::ResetStarved()
{
	m_bStarved = false;
}

AKRESULT CAkSinkDirectSound::Play()
{
	HRESULT hr = m_pDSBuffer->Play(0,0,DSBPLAY_LOOPING);
	return hr == S_OK ? AK_Success : AK_Fail;
}
//====================================================================================================
// set the primary buffer's format
// only the ones the mixer supports are allowed
//====================================================================================================
AKRESULT CAkSinkDirectSound::SetPrimaryBufferFormat()
{
	AKASSERT(m_pPrimaryBuffer != NULL);

	HRESULT hr;
	do
	{
		AkUInt32 numChannels = AK::GetNumChannels( m_SpeakersConfig );

		m_pPrimaryBuffer->Stop();

		WAVEFORMATEXTENSIBLE wfExt;
		PrepareFormat( m_SpeakersConfig, wfExt );

		hr = m_pPrimaryBuffer->SetFormat( (WAVEFORMATEX *) &wfExt );
		if( SUCCEEDED( hr ) )
		{
			hr = m_pPrimaryBuffer->Play( 0, 0, DSBPLAY_LOOPING );
			if( SUCCEEDED( hr ) )
				hr = CreateSecondaryBuffer();
		}

		if( FAILED( hr ) )
		{
			switch( m_SpeakersConfig )
			{
			case AK_SPEAKER_SETUP_7POINT1:
				m_SpeakersConfig = AK_SPEAKER_SETUP_5POINT1;
				break;
			case AK_SPEAKER_SETUP_5POINT1:
				m_SpeakersConfig = AK_SPEAKER_SETUP_STEREO;
				break;
			default:
				return AK_Fail;
			}
		}
	}
	while ( FAILED( hr ) );

	return AK_Success;
}
//====================================================================================================
// get buffer ready for being written to
//====================================================================================================
AKRESULT CAkSinkDirectSound::GetBuffer(BufferUpdateParams& io_Params)
{
	AKRESULT eResult = AK_Success;

	HRESULT hResult = m_pDSBuffer->Lock(io_Params.ulOffset,io_Params.ulBytes,
								&io_Params.pvAudioPtr1,&io_Params.ulAudioBytes1,
								&io_Params.pvAudioPtr2,&io_Params.ulAudioBytes2,
								io_Params.ulFlags);

	if(FAILED(hResult))
	{
		eResult = AK_Fail;
	}

	return eResult;
}
//====================================================================================================
// we are done writting the buffer
//====================================================================================================
AKRESULT CAkSinkDirectSound::ReleaseBuffer(const BufferUpdateParams& in_rParams)
{
	AKRESULT eResult = AK_Success;

	HRESULT hResult = m_pDSBuffer->Unlock(in_rParams.pvAudioPtr1,
								in_rParams.ulAudioBytes1,
								in_rParams.pvAudioPtr2,
								in_rParams.ulAudioBytes2);
	if(FAILED(hResult))
	{
		eResult = AK_Fail;
	}

	return eResult;
}
//====================================================================================================
// figure out if anything needs to be re-filled
//====================================================================================================
AKRESULT CAkSinkDirectSound::IsDataNeeded( AkUInt32 & out_uBuffersNeeded )
{
	AKRESULT eResult = GetRefillSize( m_uFreeRefillFrames );
	if ( eResult != AK_Success )
		return eResult;

	out_uBuffersNeeded = m_uFreeRefillFrames / AK_NUM_VOICE_REFILL_FRAMES;

	return AK_Success;
}


//====================================================================================================
//====================================================================================================
AKRESULT CAkSinkDirectSound::PassData()
{
	if ( !m_pDSBuffer ) 
		return AK_Fail;
	
	if (m_bWaitForMerge && !s_MergingSinks.IsEmpty())
		return AK_Success;

	AKRESULT eResult = AK_Success;

	if(m_MasterOut.uValidFrames != 0)
	{
		BufferUpdateParams	Params;

		memset(&Params,0,sizeof(Params));
		Params.ulOffset = m_ulRefillOffset;
		Params.ulBytes = AK_NUM_VOICE_REFILL_FRAMES * AK::GetNumChannels( GetSpeakerConfig() ) * sizeof( AkInt16 );

		// lock the buffer before trying to write there
		eResult = GetBuffer(Params);
		if(eResult == AK_Success)
		{
			// pass what we have
			DoRefill(m_MasterOut, Params);

			// try to release the buffer
			eResult = ReleaseBuffer(Params);
		}
	}

	return eResult;
}
//====================================================================================================
//====================================================================================================
AKRESULT CAkSinkDirectSound::PassSilence()
{
	if ( !m_pDSBuffer ) 
		return AK_Fail;

	if (m_bWaitForMerge && !s_MergingSinks.IsEmpty())
	{			
		AkZeroMemLarge(m_MasterOut.GetInterleavedData(), AK_NUM_VOICE_REFILL_FRAMES * m_MasterOut.NumChannels() * sizeof(AkReal32));
		return AK_Success;
	}

	BufferUpdateParams	Params;

	memset(&Params,0,sizeof(Params));
	Params.ulOffset = m_ulRefillOffset;
	Params.ulBytes = AK_NUM_VOICE_REFILL_FRAMES * m_usBlockAlign;

	// lock the buffer before trying to write there
	AKRESULT eResult = GetBuffer(Params);

	if(eResult == AK_Success)
	{
		// pass what we have

		DoRefillSilence( AK_NUM_VOICE_REFILL_FRAMES, Params );

		// try to release the buffer
		eResult = ReleaseBuffer(Params);
	}

	return eResult;
}
//====================================================================================================
//====================================================================================================

static AkForceInline void FloatsToShorts( AkInt16 * dst, float * src, int nFloats, int nStrideSrc, int nStrideDst )
{
	do
	{
		AkReal32 fSample = *src;

		if ( fSample > AUDIOSAMPLE_FLOAT_MAX )
			fSample = AUDIOSAMPLE_FLOAT_MAX;
		else if ( fSample < AUDIOSAMPLE_FLOAT_MIN )
			fSample = AUDIOSAMPLE_FLOAT_MIN;

		*dst = (AkUInt16) ( fSample * AUDIOSAMPLE_SHORT_MAX );

		src += nStrideSrc;
		dst += nStrideDst;
	}
	while( --nFloats );
}

static AkForceInline void AddFloats( float * dst, float * src, int nFloats, float fMul, int nStride )
{
	do
	{
		AkReal32 fSample = *src;
		*dst += fSample * fMul;

		src += nStride;
		dst += nStride;
	}
	while( --nFloats );
}

#define MINUS_3DB 0.707

void CAkSinkDirectSound::DoRefill(AkPipelineBufferBase& in_AudioBuffer, const BufferUpdateParams& in_rParams)
{
	AKASSERT( m_uFreeRefillFrames >= AK_NUM_VOICE_REFILL_FRAMES );
	AKASSERT( in_AudioBuffer.uValidFrames == AK_NUM_VOICE_REFILL_FRAMES );

	AkUInt32 uNumChannelDS = AK::GetNumChannels( GetSpeakerConfig() );

	void*	pvFrom = in_AudioBuffer.GetInterleavedData();
	AkUInt32 ulToRefillSamples = AK_NUM_VOICE_REFILL_FRAMES * uNumChannelDS;
	AKASSERT( (ulToRefillSamples * sizeof( AkInt16 )) == in_rParams.ulAudioBytes1 );
	void* pvRefillPosition = in_rParams.pvAudioPtr1;

	if ( in_AudioBuffer.GetChannelMask() == GetSpeakerConfig() )
	{
		FloatsToShorts((AkInt16 *) pvRefillPosition, (float *) pvFrom, ulToRefillSamples );
	}
	else // Mismatch in channel config: happens when hardware device has changed
	{
		AkUInt32 uNumChannelSrc = AK::GetNumChannels( in_AudioBuffer.GetChannelMask() );
		if ( uNumChannelSrc < uNumChannelDS )
		{
			// clear output
			memset( (AkInt16 *) pvRefillPosition, 0, ulToRefillSamples * sizeof( AkInt16 ) );
	
			// Copy source channels, leave all other channels to zero
			for ( AkUInt32 uSrcChannel = 0; uSrcChannel < uNumChannelSrc; ++uSrcChannel )
				FloatsToShorts( (AkInt16 *) pvRefillPosition + uSrcChannel, (float *) pvFrom + uSrcChannel, AK_NUM_VOICE_REFILL_FRAMES, uNumChannelSrc, uNumChannelDS );
		}
		else
		{
			AKASSERT( GetSpeakerConfig() == AK_SPEAKER_SETUP_STEREO );

			// Downmix into LR. NOTE: speaker indices are windows' (with LFE before rear channels)
			float * pL = (float *) pvFrom;
			float * pR = (float *) pvFrom + 1;
			float * pC = (float *) pvFrom + 2;
			float * pRL = (float *) pvFrom + 4;
			float * pRR = (float *) pvFrom + 5;

			AddFloats( pL, pC, AK_NUM_VOICE_REFILL_FRAMES, MINUS_3DB, uNumChannelSrc );
			AddFloats( pR, pC, AK_NUM_VOICE_REFILL_FRAMES, MINUS_3DB, uNumChannelSrc );
			AddFloats( pL, pRL, AK_NUM_VOICE_REFILL_FRAMES, MINUS_3DB, uNumChannelSrc );
			AddFloats( pR, pRR, AK_NUM_VOICE_REFILL_FRAMES, MINUS_3DB, uNumChannelSrc );

			FloatsToShorts( (AkInt16 *) pvRefillPosition, (float *) pvFrom, AK_NUM_VOICE_REFILL_FRAMES, uNumChannelSrc, uNumChannelDS );
			FloatsToShorts( (AkInt16 *) pvRefillPosition + 1, (float *) pvFrom + 1, AK_NUM_VOICE_REFILL_FRAMES, uNumChannelSrc, uNumChannelDS );
		}
	}

#ifndef AK_OPTIMIZED
	UpdateProfileData( (float *) pvFrom, ulToRefillSamples );
#endif
	if (m_pCapture != NULL)
		m_pCapture->PassSampleData(pvRefillPosition, ulToRefillSamples * sizeof(AK_PRIMARY_BUFFER_SAMPLE_TYPE));

	// move refill offset
	m_ulRefillOffset += ulToRefillSamples * sizeof( AkInt16 );

	// over the limit ?
	if(m_ulRefillOffset >= m_ulBufferSize)
	{
		// get them back within the buffer's limits
		m_ulRefillOffset = 0;
	}
}

void CAkSinkDirectSound::DoRefillSilence( AkUInt32 in_uNumFrames, const BufferUpdateParams& in_rParams )
{
	AKASSERT( m_uFreeRefillFrames >= AK_NUM_VOICE_REFILL_FRAMES );
	AKASSERT( in_uNumFrames == AK_NUM_VOICE_REFILL_FRAMES );

	AkUInt32 ulToRefillBytes = in_uNumFrames * m_usBlockAlign;
	void* pvRefillPosition = in_rParams.pvAudioPtr1;

#ifndef AK_OPTIMIZED
	UpdateProfileSilence( ulToRefillBytes / sizeof( AkReal32 ) );
#endif

	::ZeroMemory( pvRefillPosition, ulToRefillBytes );

	// write silence to capture file
	if (m_pCapture != NULL)
		m_pCapture->PassSampleData(pvRefillPosition, ulToRefillBytes);

	{
		// move refill offset
		m_ulRefillOffset += ulToRefillBytes;

		// over the limit ?
		if(m_ulRefillOffset >= m_ulBufferSize)
		{
			// get them back within the buffer's limits
			m_ulRefillOffset = 0;
		}
	}
}

//====================================================================================================
//====================================================================================================
AKRESULT CAkSinkDirectSound::GetRefillSize( AkUInt32& out_uRefillFrames )
{
	// figure out where the play position is
	AkUInt32 uOldPlay = m_uPlay;
	HRESULT hr = m_pDSBuffer->GetCurrentPosition(&m_uPlay, &m_uWrite);
	if ( hr != DS_OK )
		return AK_Fail;

	m_lRefillToEnd = m_ulBufferSize - m_ulRefillOffset;

	// handle cases where play and write are 0
	if((m_uPlay + m_uWrite) == 0)
	{
		// return a number of frames
		out_uRefillFrames = m_ulBufferSize / m_usBlockAlign;
		return AK_Success;
	}

	//Check 3 cases for starvation
	//OP -> Old Play head
	//RO -> Refill Offset
	//SW -> Safe Write pointer
	//+------OP---RO---SW----+
	//+---SW------OP---RO----+
	//+--RO------SW------OP--+
	m_bStarved = (uOldPlay < m_ulRefillOffset && m_ulRefillOffset < m_uWrite) ||
		(m_uWrite < uOldPlay && uOldPlay < m_ulRefillOffset) ||
		(m_ulRefillOffset < m_uWrite && m_uWrite < uOldPlay);

	AkUInt32 ulFreeRefillSize = 0;
	if(m_ulRefillOffset > m_uPlay)
	{
		ulFreeRefillSize = m_ulBufferSize - m_ulRefillOffset + m_uPlay;
	}
	else
	{
		ulFreeRefillSize = m_uPlay - m_ulRefillOffset;
	}

	// return a number of frames
	out_uRefillFrames = (AkUInt32)ulFreeRefillSize / m_usBlockAlign;

	return AK_Success;
}


#if defined (_DEBUG) || (_PROFILE)
void CAkSinkDirectSound::AssertHR(HRESULT hr)
{
	switch(hr)
	{
	case S_OK:
		break;
	case DSERR_ALLOCATED:
	AKASSERT(!"DSERR_ALLOCATED");
		break;
	case DSERR_CONTROLUNAVAIL:
	AKASSERT(!"DSERR_CONTROLUNAVAIL");
		break;
	case DSERR_INVALIDPARAM:
	AKASSERT(!"DSERR_INVALIDPARAM");
		break;
	case DSERR_INVALIDCALL:
	AKASSERT(!"DSERR_INVALIDCALL");
		break;
	case DSERR_GENERIC:
	AKASSERT(!"DSERR_GENERIC");
		break;
	case DSERR_PRIOLEVELNEEDED:
	AKASSERT(!"DSERR_PRIOLEVELNEEDED");
		break;
	case DSERR_OUTOFMEMORY:
	AKASSERT(!"DSERR_OUTOFMEMORY");
		break;
	case DSERR_BADFORMAT:
	AKASSERT(!"DSERR_BADFORMAT");
		break;
	case DSERR_UNSUPPORTED:
	AKASSERT(!"DSERR_UNSUPPORTED");
		break;
	case DSERR_NODRIVER:
	AKASSERT(!"DSERR_NODRIVER");
		break;
	case DSERR_ALREADYINITIALIZED:
	AKASSERT(!"DSERR_ALREADYINITIALIZED");
		break;
	case DSERR_NOAGGREGATION:
	AKASSERT(!"DSERR_NOAGGREGATION");
		break;
	case DSERR_BUFFERLOST:
	AKASSERT(!"DSERR_BUFFERLOST");
		break;
	case DSERR_OTHERAPPHASPRIO:
	AKASSERT(!"DSERR_OTHERAPPHASPRIO");
		break;
	case DSERR_UNINITIALIZED:
	AKASSERT(!"DSERR_UNINITIALIZED");
		break;
	case DSERR_NOINTERFACE:
	AKASSERT(!"DSERR_NOINTERFACE");
		break;
	case DSERR_ACCESSDENIED:
	AKASSERT(!"DSERR_ACCESSDENIED");
		break;
	case DSERR_BUFFERTOOSMALL:
	AKASSERT(!"DSERR_BUFFERTOOSMALL");
		break;
	case DSERR_DS8_REQUIRED:
	AKASSERT(!"DSERR_DS8_REQUIRED");
		break;
	case DSERR_SENDLOOP:
	AKASSERT(!"DSERR_SENDLOOP");
		break;
	case DSERR_BADSENDBUFFERGUID:
	AKASSERT(!"DSERR_BADSENDBUFFERGUID");
		break;
	case DSERR_OBJECTNOTFOUND:
	AKASSERT(!"DSERR_OBJECTNOTFOUND");
		break;
	case DSERR_FXUNAVAILABLE:
	AKASSERT(!"DSERR_FXUNAVAILABLE");
		break;
	default:
		AKASSERT(!"UNKNOWN HRESULT");
		break;
	}
}
#endif
#endif
