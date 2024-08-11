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

#include "AkMath.h"
#include "AkMonitor.h"
#include "AkProfile.h"
#include "AkSettings.h"
#include "AkCritical.h"
#include "AkAudioMgr.h"

#include <AK/SoundEngine/Common/IAkStreamMgr.h>
#include <AK/SoundEngine/Common/AkSimd.h>

#include "AkAudioLibTimer.h"

#include <audioout.h>
#include <user_service.h>

#ifdef AK_71FROM51MIXER
#include "AkProfile.h"
#endif

extern AkPlatformInitSettings g_PDSettings;

AkThread					CAkSinkPS4::s_hAudioOutThread = AK_NULL_THREAD;		// Thread for all libAudio sinks.
CAkSinkPS4::LibAudioSinks	CAkSinkPS4::s_listSinks;
CAkLock						CAkSinkPS4::s_lockSinks;
AkUInt32					CAkSinkPS4::s_uSinkCount = 0;


//====================================================================================================
//====================================================================================================

CAkSink::CAkSink(AkChannelMask uChannelMask)
	: m_SpeakersConfig( uChannelMask )
	, m_ulRefillSamples( 0 )
{
	m_ulNumChannels = AK::GetNumChannels( m_SpeakersConfig );
	// Note: PS4 sink does not natively support 6 channels.
#if defined( AK_71FROM51MIXER )
	AkUInt32 uNumChannels = m_ulNumChannels;
	if (!AK_PERF_OFFLINE_RENDERING && m_ulNumChannels == 6)
	{
		uNumChannels = 8;
	}
#else
	AkUInt32 uNumChannels = m_ulNumChannels;
#endif
	m_ulRefillSamples = uNumChannels * AK_NUM_VOICE_REFILL_FRAMES;

#ifndef AK_OPTIMIZED
	m_stats.m_fOutMin = (AkReal32) INT_MAX;
	m_stats.m_fOutMax = (AkReal32) INT_MIN;
	m_stats.m_fOutSum = 0;
	m_stats.m_fOutSumOfSquares = 0;
	m_stats.m_uOutNum = 0;
#endif

	m_pCapture = NULL;

	m_MasterOut.Clear();
	m_MasterOut.SetChannelMask( uChannelMask );

	m_unBufferSize = m_ulRefillSamples * sizeof(AK_PRIMARY_BUFFER_SAMPLE_TYPE);
}

CAkSink * CAkSink::Create( AkOutputSettings & in_settings, AkSinkType in_eType, AkUInt32 in_uInstance )
{
	CAkSink* pSink = NULL;

	bool bIsOutputRecordable = true;
	
	// Create sink
	if ( in_eType != AkSink_Dummy )
	{
		AkUInt32 uType = SCE_AUDIO_OUT_PORT_TYPE_MAIN;
		switch(in_eType)
		{
		case AkSink_Main_NonRecordable:
			bIsOutputRecordable = false;
		case AkSink_Main:
			// Default:
			if (in_settings.uChannelMask == AK_CHANNEL_MASK_DETECT)
				in_settings.uChannelMask = AK_SPEAKER_SETUP_7_1;

			uType = SCE_AUDIO_OUT_PORT_TYPE_MAIN; 
			in_uInstance = SCE_USER_SERVICE_USER_ID_SYSTEM;
			break;
		case AkSink_BGM_NonRecordable:
			bIsOutputRecordable = false;
		case AkSink_BGM:
			if (in_settings.uChannelMask == AK_CHANNEL_MASK_DETECT)
				in_settings.uChannelMask = AK_SPEAKER_SETUP_7_1;

			uType = SCE_AUDIO_OUT_PORT_TYPE_BGM;
			in_uInstance = SCE_USER_SERVICE_USER_ID_SYSTEM;
			break;
		case AkSink_Voice:
			uType = SCE_AUDIO_OUT_PORT_TYPE_VOICE; 
			in_settings.uChannelMask = AK_SPEAKER_SETUP_STEREO;
			break;
		case AkSink_Personal:
			uType = SCE_AUDIO_OUT_PORT_TYPE_PERSONAL; 
			in_settings.uChannelMask = AK_SPEAKER_SETUP_STEREO;
			break;
		case AkSink_PAD:
			uType = SCE_AUDIO_OUT_PORT_TYPE_PADSPK;
			in_settings.uChannelMask = AK_SPEAKER_SETUP_MONO;
			break;
		};

		pSink = AkNew( g_LEngineDefaultPoolId, CAkSinkPS4(in_settings.uChannelMask, uType, in_uInstance) );		
	}
#ifndef AK_OPTIMIZED
	else
	{
		pSink = AkNew( g_LEngineDefaultPoolId, CAkSinkDummy(in_settings.uChannelMask) );
	}
#endif
	if ( !pSink )
		return NULL;

	if ( pSink->Init( bIsOutputRecordable ) != AK_Success ) 
	{
		pSink->Term();
		AkDelete( g_LEngineDefaultPoolId, pSink );
		return NULL;
	}

	return pSink;
}

void CAkSink::StartOutputCapture(const AkOSChar* in_CaptureFileName)
{
	if (m_pCapture == NULL)
	{
		m_pCapture = AkCaptureMgr::Instance()->StartCapture(in_CaptureFileName, m_ulNumChannels, AK_CORE_SAMPLERATE, AK_PRIMARY_BUFFER_BITS_PER_SAMPLE, AkCaptureFile::Float);	
	}
}
void CAkSink::StopOutputCapture()
{
	if (m_pCapture != NULL)
	{
		m_pCapture->StopCapture();
		m_pCapture = NULL;	//Instance is deleted by StopCapture
	}
}

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

	m_stats.m_fOutMin = AkMath::Min( m_stats.m_fOutMin, mMin[ 0 ] );
	m_stats.m_fOutMin = AkMath::Min( m_stats.m_fOutMin, mMin[ 1 ] );
	m_stats.m_fOutMin = AkMath::Min( m_stats.m_fOutMin, mMin[ 2 ] );
	m_stats.m_fOutMin = AkMath::Min( m_stats.m_fOutMin, mMin[ 3 ] );

	m_stats.m_fOutMax = AkMath::Max( m_stats.m_fOutMax, mMax[ 0 ] );
	m_stats.m_fOutMax = AkMath::Max( m_stats.m_fOutMax, mMax[ 1 ] );
	m_stats.m_fOutMax = AkMath::Max( m_stats.m_fOutMax, mMax[ 2 ] );
	m_stats.m_fOutMax = AkMath::Max( m_stats.m_fOutMax, mMax[ 3 ] );

	m_stats.m_fOutSum += mSum[0] + mSum[1] + mSum[2] + mSum[3];
	m_stats.m_fOutSumOfSquares += mSumSquares[0] + mSumSquares[1] + mSumSquares[2] + mSumSquares[3];

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
//====================================================================================================


CAkSinkPS4::CAkSinkPS4(AkChannelMask uChannelMask, AkUInt32 in_uType, AkUInt32 in_userId)
	: CAkSink(uChannelMask)	
	, m_port( 0 )
	, m_uType(in_uType)	
	, m_uUserId(in_userId)
	, m_pvAudioBuffer( NULL )
	, m_uReadBufferIndex( 0 )
	, m_uWriteBufferIndex( 0 )	
	, m_bEmpty( true )
	, m_bStarved( false )
	, m_ManagedAudioOutInit( true )
	, m_bPendingPortClose( false )
	, m_bForce71( false )
{
	for ( AkUInt32 i = 0; i < AK_NUM_VOICE_BUFFERS; ++i )
		m_ppvRingBuffer[i] = NULL;
}

AKRESULT CAkSinkPS4::Init(bool in_bIsOutputRecordable)
{
	int ret = sceAudioOutInit();
	if( ret == SCE_AUDIO_OUT_ERROR_ALREADY_INIT )
	{
		m_ManagedAudioOutInit = false;;
	}
	else if( ret != SCE_OK )
	{
		return AK_Fail; 
	}

	int32_t vol[SCE_AUDIO_OUT_CHANNEL_MAX];
	for (int i = 0; i < SCE_AUDIO_OUT_CHANNEL_MAX; i ++) 
	{
		vol[i] = SCE_AUDIO_VOLUME_0DB;
	}

	// From documentation: 2 7.1 format available:
	// SCE_AUDIO_OUT_PARAM_FORMAT_FLOAT_8CH ==> Float 32-bit 7.1 multi-channel (L-R-C-LFE-Lsurround-Rsurround-Lextend-Rextend interleaved)
	// SCE_AUDIO_OUT_PARAM_FORMAT_FLOAT_8CH_STD ==> Float 32bit 7.1ch multi-channel (L-R-C-LFE-Lextend-Rextend-Lsurround-Rsurround interleaved)
	uint32_t channelParam = 0;
	switch(m_ulNumChannels)
	{
	case 1: 
		/// REVIEW: Compatibility with main.
		channelParam = SCE_AUDIO_OUT_PARAM_FORMAT_FLOAT_MONO; 
		break;
	case 2: 
		channelParam = SCE_AUDIO_OUT_PARAM_FORMAT_FLOAT_STEREO; 
		break;
#if defined( AK_71FROM51MIXER )
	case 6:
		// Sink does not support 6 channels, just 8; set up output in SCE_AUDIO_OUT_PARAM_FORMAT_FLOAT_8CH
		// and let the final mix node fill the "extend" channels with zeros.
		channelParam = SCE_AUDIO_OUT_PARAM_FORMAT_FLOAT_8CH; 
		vol[SCE_AUDIO_OUT_CHANNEL_LE] = 0;
		vol[SCE_AUDIO_OUT_CHANNEL_RE] = 0;
		break;
#else
	case 6:
		// unsupported
		AKASSERT(false);
		break;
#endif
	case 8: 
		channelParam = SCE_AUDIO_OUT_PARAM_FORMAT_FLOAT_8CH_STD; 
		break;
	default:
		return AK_Fail;
	}

	uint32_t recordingRestrictionParam = 0;
	if (!in_bIsOutputRecordable)
	{
		recordingRestrictionParam = SCE_AUDIO_OUT_PARAM_ATTR_RESTRICTED;
	}

	// Get an audio out port
	m_port = sceAudioOutOpen(
		m_uUserId,
		m_uType,
		0,
		AK_NUM_VOICE_REFILL_FRAMES,
		48000, // Must always be 48000 for SCE_AUDIO_OUT_PORT_TYPE_MAIN
		channelParam | recordingRestrictionParam );

	if ( m_port < 1 )
		return AK_Fail; // Keep m_port as-is, so you can look at the error value later in the debugger

	if ( m_ulNumChannels == 8 )
	{
		// Check _real_ output config.
		SceAudioOutPortState state;
		AKVERIFY( sceAudioOutGetPortState( m_port, &state) == SCE_OK );

		switch ( state.channel )
		{
		case SCE_AUDIO_OUT_STATE_CHANNEL_1:
			// Mono to 7.1 not supported. This should only be used for mono controllers, which is already mono.
			AKASSERT( m_SpeakersConfig == AK_SPEAKER_SETUP_MONO );
			break;
		case SCE_AUDIO_OUT_STATE_CHANNEL_2:
			m_SpeakersConfig = AK_SPEAKER_SETUP_STEREO;
			vol[SCE_AUDIO_OUT_CHANNEL_C] = 0;
			vol[SCE_AUDIO_OUT_CHANNEL_LS] = 0;
			vol[SCE_AUDIO_OUT_CHANNEL_RS] = 0;
			vol[SCE_AUDIO_OUT_CHANNEL_LE] = 0;
			vol[SCE_AUDIO_OUT_CHANNEL_RE] = 0;
			vol[SCE_AUDIO_OUT_CHANNEL_LFE] = 0;
			break;
		case SCE_AUDIO_OUT_STATE_CHANNEL_6:
			m_SpeakersConfig = AK_SPEAKER_SETUP_5POINT1;
			vol[SCE_AUDIO_OUT_CHANNEL_LE] = 0;
			vol[SCE_AUDIO_OUT_CHANNEL_RE] = 0;
			break;
		case SCE_AUDIO_OUT_STATE_CHANNEL_8:
			// Default case.
			AKASSERT( m_SpeakersConfig == AK_SPEAKER_SETUP_7POINT1 );
			break;
		default:
			AKASSERT( !"Invalid channel config" );
			return AK_Fail;
		}

		m_bForce71 = ( state.channel != SCE_AUDIO_OUT_STATE_CHANNEL_8 );
	}

	m_ulNumChannels = AK::GetNumChannels( m_SpeakersConfig );

	AKVERIFY( sceAudioOutSetVolume(m_port, ( SCE_AUDIO_VOLUME_FLAG_L_CH | SCE_AUDIO_VOLUME_FLAG_R_CH
              					  | SCE_AUDIO_VOLUME_FLAG_C_CH | SCE_AUDIO_VOLUME_FLAG_LFE_CH 
								  | SCE_AUDIO_VOLUME_FLAG_LS_CH  | SCE_AUDIO_VOLUME_FLAG_RS_CH
								  | SCE_AUDIO_VOLUME_FLAG_LE_CH  | SCE_AUDIO_VOLUME_FLAG_RE_CH ), vol) == SCE_OK);

	// Allocate ring buffer
    m_pvAudioBuffer = AkAlloc( g_LEngineDefaultPoolId, AK_NUM_VOICE_BUFFERS*m_unBufferSize );
    if ( m_pvAudioBuffer == NULL )
    {
        return AK_Fail;
    }

	memset( m_pvAudioBuffer, 0, AK_NUM_VOICE_BUFFERS*m_unBufferSize );

	// Initialize ring buffer ptrs
	for ( AkUInt32 i = 0; i < AK_NUM_VOICE_BUFFERS; ++i )
		m_ppvRingBuffer[i] = (AkUInt8*)m_pvAudioBuffer + i*m_unBufferSize;

	//Synchronize with the main sink.  This is important to avoid false starvation notifications.
	if (!s_listSinks.IsEmpty())
	{
		m_uReadBufferIndex = s_listSinks[0].pSink->m_uReadBufferIndex;
		m_uWriteBufferIndex = s_listSinks[0].pSink->m_uWriteBufferIndex;
		m_bEmpty = s_listSinks[0].pSink->m_bEmpty;
	}

	// Important: must register first sink to thread before creating thread.
	if (RegisterSink( this ) != AK_Success)
	{
		_Term();
		return AK_Fail;
	}

	// Ensure LibAudio thread has been created.
	if ( !AKPLATFORM::AkIsValidThread(&s_hAudioOutThread) )
	{
		// Thread does not exist yet. Create it.
		AKPLATFORM::AkCreateThread(	
			AudioOutThreadFunc,					// Start address
			this,								// Parameter
			g_PDSettings.threadLEngine,			// Properties 
			&s_hAudioOutThread,					// AkHandle
			"AK::LibAudioOut" );				// Debug name
		if ( !AKPLATFORM::AkIsValidThread(&s_hAudioOutThread) )
		{
			// Thread creation failed. 
			return AK_Fail;
		}
	}
	return AK_Success;
}

void CAkSinkPS4::Term( )
{
	StopOutputCapture();
	bool bIsLast;
	UnregisterSink( this, bIsLast );	

	// If we were the last sink and the audio out thread was running, wait until it joins.
	if ( bIsLast && AKPLATFORM::AkIsValidThread( &s_hAudioOutThread ) )
	{
		// No more sinks. Bail out.
		AKPLATFORM::AkWaitForSingleThread( &s_hAudioOutThread );
		AKPLATFORM::AkCloseThread( &s_hAudioOutThread );
		AKPLATFORM::AkClearThread( &s_hAudioOutThread );		
	}
}

void CAkSinkPS4::_Term()
{
	
	if ( m_port > 0 )
	{		
		sceAudioOutClose( m_port );
		m_port = 0;

		if ( m_pvAudioBuffer )
		{
			AkFree( g_LEngineDefaultPoolId, m_pvAudioBuffer );
			m_pvAudioBuffer = NULL;
		}	
	}

	for ( AkUInt32 i = 0; i < AK_NUM_VOICE_BUFFERS; ++i )
		m_ppvRingBuffer[i] = NULL;	
}

bool CAkSinkPS4::IsStarved()
{
	return m_bStarved;
}

void CAkSinkPS4::ResetStarved()
{
	AkAutoLock<CAkLock> lock( m_lockRingBuffer );
	m_bStarved = false;
}

AKRESULT CAkSinkPS4::IsDataNeeded( AkUInt32 & out_uBuffersNeeded )
{
	AkAutoLock<CAkLock> lock( m_lockRingBuffer );
	if ( m_bEmpty )
		out_uBuffersNeeded = 2;
	else if ( m_uReadBufferIndex != m_uWriteBufferIndex ) 
		out_uBuffersNeeded = 1;
	else
		out_uBuffersNeeded = 0;
	return AK_Success;
}

AKRESULT CAkSinkPS4::PassData()
{
	AKASSERT( m_port > 0 );
	AKASSERT( m_MasterOut.HasData() && m_MasterOut.uValidFrames == AK_NUM_VOICE_REFILL_FRAMES );

	AkReal32 * pBufferOut = (AkReal32 *)GetRefillPosition();	
	
#ifndef AK_OPTIMIZED
	{
		AKAutoRazorMarker marker( "UpdateProfileData" );
		UpdateProfileData( pBufferOut, m_ulRefillSamples );
	}
#endif
	if ( m_pCapture != NULL )
		m_pCapture->PassSampleData( pBufferOut, m_unBufferSize );

	// Increment write buffer index.
	AdvanceWritePointer();
	

	return AK_Success;
}


AKRESULT CAkSinkPS4::PassSilence()
{
	AKASSERT( m_port >= 1 );

	void* pvBuffer = GetRefillPosition();	

#ifndef AK_OPTIMIZED
	{
		AKAutoRazorMarker marker( "UpdateProfileSilence" );
		UpdateProfileSilence( m_ulRefillSamples );
	}
#endif
	
	if (m_pCapture != NULL)
		m_pCapture->PassSampleData( pvBuffer, m_unBufferSize );

	// Increment write buffer index.
	AdvanceWritePointer();	

	return AK_Success;
}

void * CAkSinkPS4::GetAndAdvanceReadPointer()
{
	AkAutoLock<CAkLock> lock( m_lockRingBuffer );
	void * pBuffer = m_ppvRingBuffer[m_uReadBufferIndex];

	//printf("%u Get() from %u, s: %u\n", m_uUserId, m_uReadBufferIndex, m_bStarved);

	// If we were starving, need to change buffer or else Flush() will not block. 
	// So change write buffer too remain consistent.
	// Otherwise advance.
	m_uReadBufferIndex = m_uReadBufferIndex ^ 1;
	if ( m_bEmpty )			
	{
		m_bStarved = true;		
		m_uWriteBufferIndex = m_uWriteBufferIndex ^ 1;
	}
	else
		m_bEmpty = ( m_uReadBufferIndex == m_uWriteBufferIndex );	
	
	return pBuffer;
}

void CAkSinkPS4::AdvanceWritePointer()
{
	AkAutoLock<CAkLock> lock( m_lockRingBuffer );
	//printf("%u Put() in %u\n", m_uUserId, m_uWriteBufferIndex);
	m_uWriteBufferIndex = m_uWriteBufferIndex ^ 1;
	m_bEmpty = false;
}

AKRESULT CAkSinkPS4::FillOutputParam( SceAudioOutOutputParam& out_rParam )
{
	out_rParam.handle = m_port;
	out_rParam.ptr = GetAndAdvanceReadPointer();
	return AK_Success;
}

AKRESULT CAkSinkPS4::RegisterSink( CAkSinkPS4 * in_pSink )
{
	AkAutoLock<CAkLock> lock( s_lockSinks );
	LibAudioPort* pAudioPort = s_listSinks.AddLast();
	if (pAudioPort)
	{
		pAudioPort->bPendingClose = false;
		pAudioPort->port = in_pSink->m_port;
		pAudioPort->pSink = in_pSink;
		pAudioPort->pRingBuffer = in_pSink->m_pvAudioBuffer;
		s_uSinkCount++;
		return AK_Success;
	}

	return AK_InsufficientMemory;
}

bool CAkSinkPS4::UnregisterSink( CAkSinkPS4 * in_pSink, bool & out_bIsLast )
{
	out_bIsLast = false;
	AkAutoLock<CAkLock> lock( s_lockSinks );
	LibAudioSinks::Iterator it = s_listSinks.Begin();
	for(; it != s_listSinks.End() && (*it).pSink != in_pSink; ++it)
		/*Searching*/;
	
	if ( it != s_listSinks.End() )
	{
		// Found it. Mark as ready to close.	
		(*it).bPendingClose = true;
		s_uSinkCount--;
		out_bIsLast = s_uSinkCount == 0;
		return true;
	}
	
	in_pSink->m_port = 0;	//The port will be closed by the audio output thread.
	return false;
}

AK_DECLARE_THREAD_ROUTINE(CAkSinkPS4::AudioOutThreadFunc)
{
	//Send all the buffers to the HW in one pass.
	// get our info from the parameter
	AkUInt32 uOutputs = 0;
	do
	{
		s_lockSinks.Lock();
		AkUInt32 uMaxOutputs = s_listSinks.Length();
		SceAudioOutOutputParam pOutputs[uMaxOutputs];
					
		uOutputs = 0;
		while ( uMaxOutputs )
		{
			uMaxOutputs--;
			LibAudioPort& rPort = s_listSinks[uMaxOutputs];
			if (rPort.bPendingClose)
			{
				int ret = sceAudioOutOutput( rPort.port, NULL );
				AKASSERT( ret >= SCE_OK );

				ret = sceAudioOutClose( rPort.port );

				AkFree( g_LEngineDefaultPoolId, rPort.pRingBuffer );
				s_listSinks.Erase(uMaxOutputs);	//Will always be ok because we do it from the end.

			}
			else if ( rPort.pSink->FillOutputParam(pOutputs[uOutputs]) == AK_Success)
				uOutputs++;				
		}		
				
		s_lockSinks.Unlock();
		if ( uOutputs > 0 )
		{
			AKAutoRazorMarker marker( "sceAudioOutOutputs" );
			int res = sceAudioOutOutputs(pOutputs, uOutputs);
			AKASSERT( res >= SCE_OK );			
			if ( g_pAudioMgr )
				g_pAudioMgr->WakeupEventsConsumer();
		}		
	}
	while ( uOutputs > 0 );
	
	s_listSinks.Term();
	AkExitThread( AK_RETURN_THREAD_OK );
	
} // Perform

//====================================================================================================
//====================================================================================================

//====================================================================================================
//====================================================================================================


#ifndef AK_OPTIMIZED
CAkSinkDummy::CAkSinkDummy(AkChannelMask uChannelMask)
	: CAkSink(uChannelMask ? uChannelMask : AK_SPEAKER_SETUP_STEREO)
	, m_pCaptureBuffer( NULL )
{	
}

AKRESULT CAkSinkDummy::Init(  bool /*in_bIsOutputRecordable IGNORED*/ ) 
{
	// Allocate capture buffer
    m_pCaptureBuffer = (AK_PRIMARY_BUFFER_SAMPLE_TYPE*)AkAlloc( g_LEngineDefaultPoolId, m_unBufferSize );
    if ( m_pCaptureBuffer == NULL )
    {
        return AK_Fail;
    }

	memset( m_pCaptureBuffer, 0, m_unBufferSize );

	return AK_Success; 
}

void CAkSinkDummy::Term() 
{
	StopOutputCapture();

	if ( m_pCaptureBuffer )
	{
		AkFree( g_LEngineDefaultPoolId, m_pCaptureBuffer );
		m_pCaptureBuffer = NULL;
	}
}

AKRESULT CAkSinkDummy::IsDataNeeded( AkUInt32 & out_uBuffersNeeded )
{
	out_uBuffersNeeded = 1;
	return AK_Success;
}

//====================================================================================================
// PassData
//====================================================================================================
AKRESULT CAkSinkDummy::PassData()
{
	AKASSERT( m_MasterOut.HasData() && m_MasterOut.uValidFrames == AK_NUM_VOICE_REFILL_FRAMES ); // ?????

	void* pbSourceData = m_MasterOut.GetInterleavedData();

	// copy the buffer
	if ( pbSourceData != NULL )
	{
		AKASSERT( m_MasterOut.GetChannelMask() == GetSpeakerConfig() );
		UpdateProfileData( (float*)pbSourceData, m_ulRefillSamples );

		if ( m_pCapture != NULL )
		{
			m_pCapture->PassSampleData( pbSourceData, m_unBufferSize );
		}
	}

	return AK_Success; // ???
}
//====================================================================================================
// PassSilence
//====================================================================================================
AKRESULT CAkSinkDummy::PassSilence()
{
	UpdateProfileSilence( m_ulRefillSamples );

	if ( m_pCapture != NULL )
	{
		memset( m_pCaptureBuffer, 0, m_unBufferSize );
		m_pCapture->PassSampleData( m_pCaptureBuffer, m_unBufferSize );
	}

	return AK_Success;
}


#endif
