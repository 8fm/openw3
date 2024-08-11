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
// AkSrcPhysModel.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"

#include "AkSrcPhysModel.h"

#include "AkLEngine.h"

#include "AkFxBase.h"
#include "AkEffectsMgr.h"
#include "AudiolibDefs.h"
#include "AkFXMemAlloc.h"

#include "AkAudioLibTimer.h"
#include "AkMonitor.h"
#include "AkSoundBase.h"
#include "AkURenderer.h"
#include <AK/SoundEngine/Common/AkSimd.h>

//-----------------------------------------------------------------------------
// Defines.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Name: CAkSrcPhysModel
// Desc: Constructor.
//
// Return: None.
//-----------------------------------------------------------------------------
CAkSrcPhysModel::CAkSrcPhysModel( CAkPBI * in_pCtx )
	: CAkVPLSrcNode( in_pCtx )
	, m_FXID( AK_INVALID_PLUGINID )
	, m_pParam( NULL )
	, m_pEffect( NULL )
	, m_pSourceFXContext( NULL )
{
	m_pluginBuffer.Clear();
}

//-----------------------------------------------------------------------------
// Name: ~CAkSrcPhysModel
// Desc: Destructor.
//
// Return: None.
//-----------------------------------------------------------------------------
CAkSrcPhysModel::~CAkSrcPhysModel()
{
	StopStream();
}

//-----------------------------------------------------------------------------
// Name: StartStream
// Desc: Start to stream data.
//
// Return: Ak_Success:          Stream was started.
//         AK_InvalidParameter: Invalid parameters.
//         AK_Fail:             Failed to start streaming data.
//-----------------------------------------------------------------------------
AKRESULT CAkSrcPhysModel::StartStream()
{
	AK::Monitor::ErrorCode errCode = AK::Monitor::ErrorCode_PluginInitialisationFailed;
	AKRESULT l_eResult = AK_Fail;
	AK::IAkPlugin * l_pIEffect = NULL;
	CAkFxCustom * pFx = NULL;
	AK::IAkPluginParam * pMaster = NULL;

	// Set source info
	{
		AkSrcTypeInfo * pSrcType = m_pCtx->GetSrcTypeInfo();

		// Source plug-in are always custom for now
		pFx = g_pIndex->m_idxFxCustom.GetPtrAndAddRef( pSrcType->mediaInfo.sourceID );
		if ( !pFx )
		{
			errCode = AK::Monitor::ErrorCode_PluginInitialisationFailed;
			goto failure;
		}
	}

	m_FXID = pFx->GetFXID();

	// Get an instance of the physical modelling source plugin.
	l_eResult = CAkEffectsMgr::Alloc( AkFXMemAlloc::GetLower(), m_FXID, l_pIEffect );
	if( l_eResult != AK_Success )
	{
		errCode = AK::Monitor::ErrorCode_PluginAllocationFailed;
		goto failure;
	}

	m_pEffect = reinterpret_cast<IAkSourcePlugin*>(l_pIEffect);
	AK_INCREMENT_PLUGIN_COUNT( m_FXID );

	pMaster = reinterpret_cast<IAkPluginParam*>( pFx->GetFXParam() );
	if( pMaster != NULL )
	{
		m_pParam = pMaster->Clone( AkFXMemAlloc::GetLower() );

		if( !m_pParam )
		{
			errCode = AK::Monitor::ErrorCode_PluginAllocationFailed;
			goto failure;
		}

		// Subscribe to RTPC
		pFx->SubscribeRTPC( m_pParam, m_pCtx->GetGameObjectPtr() );
	}

	m_pSourceFXContext = AkNew( g_LEngineDefaultPoolId, CAkSourceFXContext( m_pCtx ) );
	if ( !m_pSourceFXContext )
	{
		errCode = AK::Monitor::ErrorCode_PluginAllocationFailed;
		goto failure;
	}

	// Suggested format for sources is mono native for platform, they can change it if they wish
	m_AudioFormat.SetAll( 
#ifdef AK_MOTION
		m_pCtx->IsForFeedbackPipeline() ? AK_FEEDBACK_SAMPLE_RATE : AK_CORE_SAMPLERATE,
#else // AK_MOTION
		AK_CORE_SAMPLERATE,
#endif // AK_MOTION
		AK_SPEAKER_SETUP_MONO,
		AK_LE_NATIVE_BITSPERSAMPLE,
		sizeof(AkReal32),	
		AK_LE_NATIVE_SAMPLETYPE,
		AK_LE_NATIVE_INTERLEAVE
		);

	AkPluginInfo PluginInfo;
	m_pEffect->GetPluginInfo( PluginInfo );

    // Initialise.
    l_eResult = m_pEffect->Init( 
		AkFXMemAlloc::GetLower(),
		m_pSourceFXContext,
		m_pParam,
		m_AudioFormat
		);

	AKASSERT( ((m_AudioFormat.GetInterleaveID() == AK_INTERLEAVED) ^ (m_AudioFormat.GetTypeID() == AK_FLOAT))  
		|| !"Invalid output format" );

	if ( !m_AudioFormat.IsChannelConfigSupported() )
	{
		errCode = AK::Monitor::ErrorCode_PluginUnsupportedChannelConfiguration;
		goto failure;
	}
		
    if( l_eResult != AK_Success )
	{
		errCode = AK::Monitor::ErrorCode_PluginInitialisationFailed;
		goto failure;
	}

	l_eResult = m_pEffect->Reset( );
	if( l_eResult != AK_Success )
	{
		errCode = AK::Monitor::ErrorCode_PluginInitialisationFailed;
		goto failure;
	}

	m_pCtx->SetMediaFormat( m_AudioFormat );

	pFx->Release();

	return AK_Success;

failure:
	MONITOR_PLUGIN_ERROR( errCode, m_pCtx, m_FXID );	
	StopStream();

	if ( pFx )
		pFx->Release();

	return AK_Fail;
}

//-----------------------------------------------------------------------------
// Name: StopStream
// Desc: Stop streaming data.
//
// Return: Ak_Success:          Stream was stopped.
//         AK_InvalidParameter: Invalid parameters.
//         AK_Fail:             Failed to stop the stream.
//-----------------------------------------------------------------------------
void CAkSrcPhysModel::StopStream()
{
	ReleaseBuffer();

	if( m_pEffect != NULL )
    {
		AKVERIFY( m_pEffect->Term( AkFXMemAlloc::GetLower() ) == AK_Success );
		m_pEffect = NULL;
		AK_DECREMENT_PLUGIN_COUNT( m_FXID );
    }

	if ( m_pSourceFXContext )
	{
		AkDelete( g_LEngineDefaultPoolId, m_pSourceFXContext );
		m_pSourceFXContext = NULL;
	}

	if ( m_pParam )
	{
		g_pRTPCMgr->UnSubscribeRTPC( m_pParam );

		AKRESULT eResult = m_pParam->Term( AkFXMemAlloc::GetLower( ) );
		AKASSERT( eResult == AK_Success );
		m_pParam = NULL;
	}
}

void CAkSrcPhysModel::GetBuffer( AkVPLState & io_state )
{
	if( m_pEffect != NULL )
	{
		if( io_state.MaxFrames() == 0 )
		{
			AKASSERT( !"Physical modeling source called with zeros size request" );
			io_state.result = AK_NoMoreData;
			return;
		}

		AKRESULT eResult = AK_Success;
		AkChannelMask uChannelMask = m_AudioFormat.GetChannelMask();
		if ( !m_pluginBuffer.HasData() )
		{
			// Allocate working buffer for effects.
			if ( m_AudioFormat.GetInterleaveID() == AK_NONINTERLEAVED )
			{		
				eResult = io_state.GetCachedBuffer( io_state.MaxFrames(), uChannelMask );
			}
			else
			{
				// Note: With interleaved data, the plugin source is free to use the sample size 
				// it wants. 
				AkUInt32 uSampleFrameSize = m_AudioFormat.GetBlockAlign();
				AKASSERT( uSampleFrameSize >= GetNumChannels(uChannelMask) * m_AudioFormat.GetBitsPerSample() / 8 );
				void * pBuffer = CAkLEngine::GetCachedAudioBuffer( uSampleFrameSize * io_state.MaxFrames() );
				if ( pBuffer )
				{
					io_state.AttachInterleavedData( pBuffer, io_state.MaxFrames(), 0, uChannelMask );
				}
				else
				{
					eResult = AK_Fail;
				}
			}
		}
		else
		{
			io_state.AttachInterleavedData( m_pluginBuffer.GetInterleavedData(), io_state.MaxFrames(), 0, uChannelMask );
		}

		if ( eResult != AK_Success )
		{
			MONITOR_PLUGIN_ERROR( AK::Monitor::ErrorCode_PluginProcessingFailed, m_pCtx, m_FXID );
			io_state.result = AK_Fail;
			return;
		}


		// Ensure SIMD processing is possible without extra considerations
		AKASSERT( io_state.MaxFrames() % 4 == 0 );	
		
		io_state.eState = AK_DataNeeded;

		//////////////////////////////////////////////////////////
		AK_START_PLUGIN_TIMER( m_FXID );
#ifdef AK_PS3		
		AK::MultiCoreServices::DspProcess* pDspProcess = NULL;
		m_pEffect->Execute( &io_state, pDspProcess );
		if ( pDspProcess )
		{
			io_state.result = AK_ProcessNeeded;
			CAkLEngine::QueueDspProcess(pDspProcess);
		}
		else
		{
			io_state.result = io_state.eState;
		}
#else
		AkPrefetchZero(io_state.GetInterleavedData(), io_state.MaxFrames() * m_AudioFormat.GetBlockAlign() );

		m_pEffect->Execute( &io_state );
		io_state.result = io_state.eState;
#endif	
		AK_STOP_PLUGIN_TIMER( m_FXID );

		AKSIMD_ASSERTFLUSHZEROMODE;

		//////////////////////////////////////////////////////////

		// Copy buffer data for later release.
		m_pluginBuffer = io_state;
	}
	else
	{
		io_state.Clear();
		io_state.result = AK_Fail;
	}
	
	// Graceful plugin error handling.
	if ( io_state.result == AK_Fail )
	{
		MONITOR_PLUGIN_ERROR( AK::Monitor::ErrorCode_PluginProcessingFailed, m_pCtx, m_FXID );
		return;
	}
}

//-----------------------------------------------------------------------------
// Name: ReleaseBuffer
// Desc: Release a specified buffer.
//-----------------------------------------------------------------------------
void CAkSrcPhysModel::ReleaseBuffer()
{
	if( m_pluginBuffer.HasData() )
	{
		if ( m_AudioFormat.GetInterleaveID() == AK_NONINTERLEAVED )
		{
			m_pluginBuffer.ReleaseCachedBuffer();
		}
		else
		{
			CAkLEngine::ReleaseCachedAudioBuffer( m_AudioFormat.GetBlockAlign() * LE_MAX_FRAMES_PER_BUFFER, m_pluginBuffer.GetInterleavedData() );
			m_pluginBuffer.ClearData();
		}
	}
}

#ifdef AK_PS3
void CAkSrcPhysModel::ProcessDone( AkVPLState & io_state )
{
	AKASSERT( io_state.uValidFrames <= io_state.MaxFrames() );	// Produce <= than requested
	io_state.result = io_state.eState;
}
#endif

void CAkSrcPhysModel::VirtualOn( AkVirtualQueueBehavior eBehavior )
{
	if( eBehavior == AkVirtualQueueBehavior_FromBeginning )
	{
		if ( m_pEffect != NULL ) 
			m_pEffect->Reset( );
	}
}

AKRESULT CAkSrcPhysModel::TimeSkip( AkUInt32 & io_uFrames )
{
	AKRESULT eResult = AK_DataReady;
	if ( m_pEffect != NULL ) 
	{
		eResult = m_pEffect->TimeSkip( io_uFrames );
		if ( eResult == AK_NotImplemented )
		{
#ifndef AK_PS3 
			return CAkVPLSrcNode::TimeSkip( io_uFrames );
#else
			AkPluginInfo pluginInfo;
			m_pEffect->GetPluginInfo( pluginInfo );
			if ( !pluginInfo.bIsAsynchronous )
				return CAkVPLSrcNode::TimeSkip( io_uFrames );
			else
			{
				// Note: TimeSkip should be implemented on a aynchronous plug-ins to obtain proper virtual voice behavior from elapsed time
				// otherwise a resume behavior will occur
				eResult = AK_DataReady;
			}
#endif
		}
	}
	return eResult;
}

//-----------------------------------------------------------------------------
// Name: GetDuration()
// Desc: Get the duration of the source.
//
// Return: AkReal32 : duration of the source.
//
//-----------------------------------------------------------------------------
AkReal32 CAkSrcPhysModel::GetDuration() const
{
	if ( m_pEffect )
		return m_pEffect->GetDuration();
	else
		return 0;
}

// Returns estimate of relative loudness at current position, compared to the loudest point of the sound, in dBs (always negative).
AkReal32 CAkSrcPhysModel::GetAnalyzedEnvelope( AkUInt32 /*in_uBufferedFrames*/ ) 
{ 
	if ( m_pEffect )
		return AkMath::FastLinTodB( m_pEffect->GetEnvelope() + AK_EPSILON ); 
	else
		return 0;
}


//-----------------------------------------------------------------------------
// Name: StopLooping()
// Desc: Called on actions of type break
//-----------------------------------------------------------------------------
AKRESULT CAkSrcPhysModel::StopLooping()
{
	if ( m_pEffect )
		return m_pEffect->StopLooping();
	else
		return AK_Fail;
}

//-----------------------------------------------------------------------------
// Name: ChangeSourcePosition()
// Desc: Called on actions of type Seek.
//-----------------------------------------------------------------------------
AKRESULT CAkSrcPhysModel::ChangeSourcePosition()
{
	// Get the source offset from the PBI and transform it to the plugin's sample rate.

	AKASSERT( m_pCtx->RequiresSourceSeek() );

	AkUInt32 uSourceOffset;
	bool bSnapToMarker;	// Ignored with source plugins.

	if ( m_pCtx->IsSeekRelativeToDuration() )
	{
		uSourceOffset = m_pCtx->GetSeekPosition( 
			m_pEffect->GetDuration(), 
			bSnapToMarker );
	}
	else
	{
		uSourceOffset = m_pCtx->GetSeekPosition( 
			bSnapToMarker );
	}

	// Consume seeking value on PBI.
	m_pCtx->SetSourceOffsetRemainder( 0 );

	return m_pEffect->Seek( uSourceOffset );
}
