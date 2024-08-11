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
// AkVPLMixBusNode.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkVPLMixBusNode.h"
#include "AudiolibDefs.h"
#include "AkEffectsMgr.h"
#include "AkMonitor.h"
#include "AkMath.h"
#include "AkFxBase.h"
#include "AkFXMemAlloc.h"
#include "AkAudioLibTimer.h"
#include "AkVPLSrcCbxNode.h"
#include "AkLEngine.h"
#include "AkBus.h"
#include "AkEnvironmentsMgr.h"
#include "AkMeterTools.h"
#ifdef AK_VITA_HW
#include "AkLEngineHw.h"
#endif
#include "AkParameterNodeBase.h"
#include "AkPlayingMgr.h"

inline void CAkBusVolumes::TagPreviousPan()
{
	AkUInt32 uNumChannels = AK::GetNumChannels( m_uChannelMask );
	unsigned int iChannel=0;
	do {
		m_PanningVolumes[iChannel].Previous = m_PanningVolumes[iChannel].Next;
	}while ( ++iChannel < uNumChannels );
}

void CAkBusVolumes::UpdateFinalVolumes()
{
	AkUInt32 uNumChannels = AK::GetNumChannels( m_uChannelMask );
	unsigned int iChannel=0;

	do {
		m_FinalVolumes[iChannel].Previous = m_PanningVolumes[iChannel].Previous;
		m_FinalVolumes[iChannel].Next = m_PanningVolumes[iChannel].Next;
		m_FinalVolumes[iChannel].Previous.Mul( m_fPreviousVolume );
		m_FinalVolumes[iChannel].Next.Mul( m_fNextVolume );
	}while ( ++iChannel < uNumChannels );
}


void CAkBusVolumes::UpdatePanningVolumes()
{
	TagPreviousPan();

	if( !(m_BusPosParams == m_PrevPosParams) )
	{
		AKASSERT( m_uChannelMask != 0 );//check since init to zero by constructor.

		AkPanningConversion Pan( m_BusPosParams );

		AkSIMDSpeakerVolumes volumes[AK_VOICE_MAX_NUM_CHANNELS];

		if( !m_bIsPositioningEnabled )
			Pan.fCenter = 1.f;

		CAkSpeakerPan::GetSpeakerVolumes2DPan( 
			Pan.fX, Pan.fY, Pan.fCenter, 
			m_BusPosParams.bIsPannerEnabled, 
			m_uChannelMask, 
			m_uParentMask,
			volumes );

		AkUInt32 uNumChannels = AK::GetNumChannels( m_uChannelMask );
		unsigned int iChannel=0;
		do {
			m_PanningVolumes[iChannel].Next = volumes[iChannel];
		}while ( ++iChannel < uNumChannels );

#ifdef AK_LFECENTER
		if( m_uChannelMask & AK_SPEAKER_LOW_FREQUENCY )
		{
			m_PanningVolumes[uNumChannels-1].Next.Zero();
			m_PanningVolumes[uNumChannels-1].Next.volumes.fLfe = 1.f;
		}
#endif

		if( m_bCallbackEnabled && m_busID != AK_INVALID_UNIQUE_ID )
		{
			AkSpeakerVolumeMatrixBusCallbackInfo callbackParams;
			callbackParams.busID = m_busID;
			callbackParams.uChannelMask = m_uChannelMask;

			AkUInt32 inChannel = 0;
			for( ; inChannel < uNumChannels; inChannel++ )
				callbackParams.pVolumes[ inChannel ] = &( m_PanningVolumes[inChannel].Next.volumes );

			for( ; inChannel < AK_VOICE_MAX_NUM_CHANNELS; inChannel++ )
				callbackParams.pVolumes[ inChannel ] = NULL;
			
			bool bHadCallback = g_pBusCallbackMgr->DoCallback( callbackParams );
			if( !bHadCallback )
			{
				m_PrevPosParams = m_BusPosParams;
			}
		}
		else
		{
			m_PrevPosParams = m_BusPosParams;
		}
	}
}

void CAkBusVolumes::InitPan( CAkParameterNodeBase* in_pBus, AkChannelMask in_ChanMsk, AkChannelMask in_uParentMask )
{
	m_uChannelMask = in_ChanMsk;
	m_uParentMask = in_uParentMask;
	
	Update2DParams( in_pBus );
	UpdatePanningVolumes();
	TagPreviousPan();// so that first next is also first previous...
}

void CAkBusVolumes::Update2DParams( CAkParameterNodeBase* in_pBus )
{
	if( in_pBus && in_pBus->IsPositioningEnabled() )
	{
		m_bIsPositioningEnabled = true;
		in_pBus->Get2DParams( NULL, &m_BusPosParams );
	}
	else
	{
		m_bIsPositioningEnabled = false;
		m_BusPosParams.Init();
	}
}

void CAkBusVolumes::PositioningChangeNotification(
		AkReal32			in_RTPCValue,
		AkRTPC_ParameterID	in_ParameterID	// RTPC ParameterID, must be a Positioning ID.
		)
{
	switch ( in_ParameterID )
	{
		case POSID_Position_PAN_X_2D:
			m_BusPosParams.m_fPAN_X_2D = in_RTPCValue;
			return;
		case POSID_Position_PAN_Y_2D:
			m_BusPosParams.m_fPAN_Y_2D = in_RTPCValue;
			return;
		case POSID_Positioning_Divergence_Center_PCT:
			m_BusPosParams.m_fCenterPCT = in_RTPCValue;
			return;
		case POSID_2DPannerEnabled:
			m_BusPosParams.bIsPannerEnabled = ( in_RTPCValue > 0 )? true : false;
			return;
	}
}

CAkBusFX::CAkBusFX()
{
	for ( AkUInt32 uFXIndex = 0; uFXIndex < AK_NUM_EFFECTS_PER_OBJ; ++uFXIndex )
	{
		FX & fx = m_aFX[ uFXIndex ];
		fx.id = AK_INVALID_PLUGINID;
		fx.pParam = NULL;
		fx.pEffect = NULL;
		fx.pBusFXContext = NULL;
		fx.bBypass = 0;
		fx.bLastBypass = 0;
	}

	m_bBypassAllFX = 0;
	m_bLastBypassAllFX = 0;
}

CAkBusFX::~CAkBusFX()
{
#ifndef AK_OPTIMIZED
	if ( m_pMeterCtx )
		AkDelete( g_LEngineDefaultPoolId, m_pMeterCtx );
#endif
}

AKRESULT CAkBusFX::SetInsertFx( const CAkBusCtx & in_busCtx, AkUInt32 in_uFXIndex )
{
	AKRESULT l_eResult = AK_Success;

	DropFx( in_uFXIndex );

	AkFXDesc fxDesc;
	in_busCtx.GetFX( in_uFXIndex, fxDesc );

	if( !fxDesc.pFx )
		return AK_Success; // no new effect

	FX & fx = m_aFX[ in_uFXIndex ];

	fx.id = fxDesc.pFx->GetFXID();

	AK::IAkPluginParam * pParam = fxDesc.pFx->GetFXParam();
	if ( !pParam )
		return AK_Fail;

	fx.pParam = pParam->Clone( AkFXMemAlloc::GetLower() );
	if ( fx.pParam == NULL )
	{
		l_eResult = AK_Fail;
	}
	
	if( l_eResult == AK_Success )
	{
		fxDesc.pFx->SubscribeRTPC( fx.pParam, NULL );

		fx.pBusFXContext = AkNew( g_LEngineDefaultPoolId, CAkBusFXContext( this, in_uFXIndex, in_busCtx ) );
		l_eResult = AK_Fail;
		if ( fx.pBusFXContext != NULL)
		{
			fx.bBypass = fxDesc.bIsBypassed;
			
			l_eResult = CAkEffectsMgr::Alloc( AkFXMemAlloc::GetLower(), fx.id, (IAkPlugin*&) fx.pEffect );
			if( l_eResult == AK_Success )
			{
				// fx.pEffect is set: increment plugin count.
				AK_INCREMENT_PLUGIN_COUNT( fx.id );

				AkPluginInfo PluginInfo;
				l_eResult = fx.pEffect->GetPluginInfo( PluginInfo );
				if ( PluginInfo.bIsAsynchronous != 
	#ifdef AK_PS3
					true
	#else
					false
	#endif
				)
				{
					// Invalid plugin implementation. 
					// Review. This failure is potentially dangerous.
					l_eResult = AK_Fail;
					MONITOR_ERROR( AK::Monitor::ErrorCode_PluginExecutionInvalid ); 			
				}
			}
			else
			{
				MONITOR_ERROR( AK::Monitor::ErrorCode_PluginAllocationFailed );	
				return l_eResult;
			}

			if ( l_eResult == AK_Success )
			{
				AkAudioFormat l_Format;
				l_Format.SetAll( AK_CORE_SAMPLERATE,
						m_BufferOut.GetChannelMask(), 
						AK_LE_NATIVE_BITSPERSAMPLE,
						AK_LE_NATIVE_BITSPERSAMPLE/8*GetNumChannels(m_BufferOut.GetChannelMask()),
						AK_FLOAT,
						AK_NONINTERLEAVED );
#ifdef AK_VITA_HW
				m_ProcessingRackList.InsertFx( fx.pEffect, in_uFXIndex );
#endif
				l_eResult = fx.pEffect->Init( AkFXMemAlloc::GetLower(),		// Memory allocator.
												fx.pBusFXContext,			// Bus FX context.
												fx.pParam,
												l_Format );

				if ( l_eResult == AK_Success )
				{
					l_eResult = fx.pEffect->Reset( );
				}
				else
				{
					switch ( l_eResult )
					{ 
					case AK_UnsupportedChannelConfig:
						MONITOR_ERROR( AK::Monitor::ErrorCode_PluginUnsupportedChannelConfiguration );	
						break;
					case AK_PluginMediaNotAvailable:
						MONITOR_ERROR( AK::Monitor::ErrorCode_PluginMediaUnavailable );	
						break;
					default:
						MONITOR_ERROR( AK::Monitor::ErrorCode_PluginInitialisationFailed );	
						break;
					}
				}		
			}
		}
	}

	if( l_eResult != AK_Success )
	{
		DropFx(in_uFXIndex); 
	}
	else
	{
#ifdef AK_VITA_HW
		m_TailDuration = fx.pEffect->GetTailTime();
		m_TimeToLive = m_TailDuration;
#endif
	}

	return l_eResult;
}

void CAkBusFX::SetInsertFxBypass( AkUInt32 in_bitsFXBypass, AkUInt32 in_uTargetMask )
{
	for ( AkUInt32 uFXIndex = 0; uFXIndex < AK_NUM_EFFECTS_PER_OBJ; ++uFXIndex )
	{
		if ( in_uTargetMask & ( 1 << uFXIndex ) )
		{
			m_aFX[ uFXIndex ].bBypass = ( in_bitsFXBypass & ( 1 << uFXIndex ) ) != 0;
		}
	}

	if ( in_uTargetMask & ( 1 << AK_NUM_EFFECTS_BYPASS_ALL_FLAG ) )
	{
		m_bBypassAllFX = ( in_bitsFXBypass & ( 1 << AK_NUM_EFFECTS_BYPASS_ALL_FLAG ) ) != 0;
	}

#ifdef AK_VITA_HW
		for ( AkUInt32 uFXIndex = 0; uFXIndex < AK_NUM_EFFECTS_PER_OBJ; ++uFXIndex )
		{
			if (m_aFX[ uFXIndex ].pEffect)
			{
				m_aFX[ uFXIndex ].pEffect->SetBypass( ( m_aFX[ uFXIndex ].bBypass | m_bBypassAllFX ) ? SCE_NGS_MODULE_FLAG_BYPASSED : SCE_NGS_MODULE_FLAG_NOT_BYPASSED );
			}
		}
#endif

}

void CAkBusFX::DropFx( )
{
	for ( AkUInt32 uFXIndex = 0; uFXIndex < AK_NUM_EFFECTS_PER_OBJ; ++uFXIndex )
	{
		DropFx(uFXIndex); 
	}
}

void CAkBusFX::DropFx( AkUInt32 in_uFXIndex )
{
	FX & fx = m_aFX[ in_uFXIndex ];

	// Delete any existing effect
	if( fx.pEffect != NULL )
	{
		fx.pEffect->Term( AkFXMemAlloc::GetLower() );
		fx.pEffect = NULL;

		AK_DECREMENT_PLUGIN_COUNT( fx.id );
	}

	if ( fx.pBusFXContext )
	{
		AkDelete( g_LEngineDefaultPoolId, fx.pBusFXContext );
		fx.pBusFXContext = NULL;
	}

	fx.id = AK_INVALID_PLUGINID;

	if( fx.pParam )
	{
		g_pRTPCMgr->UnSubscribeRTPC( fx.pParam );
		fx.pParam->Term( AkFXMemAlloc::GetLower( ) );
		fx.pParam = NULL;
	}	

#ifdef AK_VITA_HW
	m_ProcessingRackList.UnPatch( in_uFXIndex );
#endif
}

#ifndef AK_OPTIMIZED
/// TODO Remove argument and get bus ID from inside. Currently this way in order to work around final bus specificity.
void CAkBusFX::RefreshMeterWatch( AkUniqueID in_busID )
{
	AkUInt8 types = AkMonitor::GetMeterWatchDataTypes( in_busID );
	types = types & AkMonitorData::BusMeterMask_RequireContext;

	// Destroy if it metering data exists and types changed.
	if ( m_pMeterCtx 
		&& m_pMeterCtx->MeterTypes() != types )
	{
		AkDelete( g_LEngineDefaultPoolId, m_pMeterCtx );
		m_pMeterCtx = NULL;
	}
	
	// Create a metering context if there are watches. 
	// Ignore if m_pMeterCtx, it would have been destroyed if types had changed.
	if ( types 
		&& !m_pMeterCtx )
	{
		m_pMeterCtx = AkNew( g_LEngineDefaultPoolId, AkMeterCtx( m_BufferOut.NumChannels(), AK_CORE_SAMPLERATE, types ) );
	}
}
#endif

//-----------------------------------------------------------------------------
// Name: Init
// Desc: Initialize the source.
//-----------------------------------------------------------------------------
#ifdef AK_VITA_HW
AKRESULT CAkVPLMixBusNode::Init( AkChannelMask in_uChannelMask, AkChannelMask in_uParentMask, AkUInt16 in_uMaxFrames, CAkBusCtx in_busContext, CAkBusFX * in_parentBus )
#else
AKRESULT CAkVPLMixBusNode::Init( AkChannelMask in_uChannelMask, AkChannelMask in_uParentMask, AkUInt16 in_uMaxFrames, CAkBusCtx in_busContext )
#endif
{	
#ifdef AK_VITA_HW
	m_ProcessingRackList.Init( in_parentBus );
	m_bUpdateVolumeOnNextFrame = true;
#endif

	m_BusContext = in_busContext;
	m_busID = GetBusID();

	m_bCallbackEnabled = g_pBusCallbackMgr->IsCallbackEnabled( m_busID );
	m_bEffectCreated = false;

	InitPan( in_busContext.GetBus(), in_uChannelMask, in_uParentMask );
	m_Mixer.Init( in_uMaxFrames );

	if ( m_BusContext.GetBus() )
	{
		m_BusContext.GetBus()->AddRef();
		bool bIsIncrementSuccessful = m_BusContext.GetBus()->IncrementActivityCount();
		if( !bIsIncrementSuccessful )
			return AK_Fail;
	}

#ifdef AK_VITA_HW
	m_bIsActive							= false;
#endif
	m_eState							= NodeStateIdle;

	ResetVolumes();
	m_ulBufferOutSize					= in_uMaxFrames * GetNumChannels( in_uChannelMask ) * sizeof(AkReal32);
	m_uConnectCount						= 0;

	m_BufferOut.Clear();
	m_BufferOut.eState					= AK_NoMoreData;
	void * pData = AkMalign( g_LEngineDefaultPoolId, m_ulBufferOutSize, AK_BUFFER_ALIGNMENT );
	if ( !pData )
		return AK_InsufficientMemory;

	AkZeroMemLarge( pData, m_ulBufferOutSize );
	m_BufferOut.AttachContiguousDeinterleavedData( 
		pData,						// Buffer.
		in_uMaxFrames,				// Buffer size (in sample frames).
		0,							// Valid frames.
		in_uChannelMask );			// Chan config.
	
	m_BufferOut.m_fNextVolume = 0.f;
	m_BufferOut.m_fPreviousVolume = 0.f;


#ifndef AK_OPTIMIZED
	m_uMixingVoiceCount = 0;
	
	// NOTE: Remove this line once final bus specificity is resolved.
	if (g_MasterBusCtx.GetBus())
	{
	AkUniqueID busID = ( m_busID != AK_INVALID_UNIQUE_ID ) ? m_busID : g_MasterBusCtx.GetBus()->ID();
	RefreshMeterWatch( busID );
	}
#endif

#ifdef AK_PS3
	m_pLastItemMix = NULL;
#endif

#ifdef AK_VITA_HW
	m_TailDuration = 0.0f;
	m_TimeToLive = 0.0f;
	SetAllInsertFx();
#endif

	return AK_Success;
} // Init


//-----------------------------------------------------------------------------
// Name: Term
// Desc: Terminate.
//
// Parameters:
//
// Return:
//	AKRESULT
//		AK_Success : terminated successfully.
//		AK_Fail    : failed.
//-----------------------------------------------------------------------------
CAkVPLMixBusNode::~CAkVPLMixBusNode()
{
	if ( m_BusContext.GetBus() )
	{
		m_BusContext.GetBus()->DecrementActivityCount();
		m_BusContext.GetBus()->Release();
	}

	DropFx();

#ifdef AK_VITA_HW
	m_ProcessingRackList.Release();
#endif

	if( m_BufferOut.HasData() )
	{
		AkFalign( g_LEngineDefaultPoolId, m_BufferOut.GetContiguousDeinterleavedData() );
		m_BufferOut.ClearData();
	}
} // Term

//-----------------------------------------------------------------------------
// Name: ReleaseBuffer
// Desc: Releases a processed buffer.
//
// Parameters:
//	AkAudioBuffer* io_pBuffer : Pointer to the buffer object to release.
//
// Return:
//	Ak_Success: Buffer was relesed.
//  AK_Fail:    Failed to release the buffer.
//-----------------------------------------------------------------------------
void CAkVPLMixBusNode::ReleaseBuffer()
{
	ResetStateForNextPass();
	
#ifndef AK_PS3
	// Also clean the data for next iteration.
	AkZeroMemLarge( m_BufferOut.GetContiguousDeinterleavedData(), m_ulBufferOutSize );
#endif
} // ReleaseBuffer

bool CAkVPLMixBusNode::IsPanning()
{
	CAkBus* pBus = m_BusContext.GetBus();
	if( pBus )
	{
		return pBus->IsPositioningEnabled();
	}
	else
	{
		return false;
	}
}

void CAkVPLMixBusNode::ResetNextVolume( AkReal32 in_dBVolume )
{
	SetNextVolume( in_dBVolume );
	Update2DParams( m_BusContext.GetBus() );
}

void CAkVPLMixBusNode::ResetStateForNextPass()
{
	// Assume output buffer was entirely consumed by client.
	if( m_BufferOut.HasData() )
	{
		if ( m_BufferOut.eState == AK_NoMoreData )
		{
  			m_eState = NodeStateIdle;
		}
		else
		{
			m_eState = NodeStatePlay;
		}

		// Reset state in case //ConsumeBuffer does not get called again
		m_BufferOut.eState = AK_NoMoreData;

		// Assume output buffer was entirely consumed by client. Do not actually release the memory here.
		m_BufferOut.uValidFrames = 0;
	}
}


//-----------------------------------------------------------------------------
// Name: Connect
// Desc: Connects the specified effect as input.
//-----------------------------------------------------------------------------
void CAkVPLMixBusNode::Connect(  )
{
	InitVolumes();

	++m_uConnectCount;
} // Connect

//-----------------------------------------------------------------------------
// Name: Disconnect
// Desc: Disconnects the specified effect as input.
//
// Parameters:
//	CAkVPLNode * in_pInput : Pointer to the effect to disconnect.
//-----------------------------------------------------------------------------
void CAkVPLMixBusNode::Disconnect( )
{
	AKASSERT( m_uConnectCount != 0 );
	--m_uConnectCount;
} // Disconnect

void CAkVPLMixBusNode::InitVolumes()
{
	if ( m_eState != NodeStatePlay ) // can only do this if node is not currently outputting
	{
		SetNextVolume( m_BusContext.GetVolume( BusVolumeType_ToNextBusWithEffect ) );
		TagPreviousVolumes();

#ifdef AK_VITA_HW
		m_ProcessingRackList.SetOutputVolume(GetNextVolume());
#endif
	}
}

AKRESULT CAkVPLMixBusNode::SetAllInsertFx()
{
	AKRESULT l_eResult = AK_Success;

	for ( AkUInt32 uFXIndex = 0; uFXIndex < AK_NUM_EFFECTS_PER_OBJ; ++uFXIndex )
	{
		l_eResult = SetInsertFx( m_BusContext, uFXIndex );
	}

	m_bEffectCreated = true;
	UpdateBypassFx();
	return l_eResult;

}

void CAkVPLMixBusNode::UpdateBypassFx()
{
	m_bBypassAllFX = m_BusContext.GetBypassAllFX();
} 

void CAkVPLMixBusNode::ConsumeBuffer( 
			AkAudioBufferBus&		io_rBuffer,
			bool					in_bPan,
			AkAudioMix				in_PanMix[]
			)
{
	// Now, send it to its parent bus (N-level mixing)
	// WG-18447 PS3 - Effect tails not playing when parent bus also has an effect
#ifndef AK_PS3
	if( io_rBuffer.uValidFrames > 0 )
#endif	
	{
		if (!m_bEffectCreated)
			SetAllInsertFx();

#ifndef AK_OPTIMIZED
		IncrementMixingVoiceCount();
#endif
		// Set plugin audio buffer eState before execution (it will get updated by the effect chain)
		m_BufferOut.eState = AK_DataReady;
		if ( m_eState == NodeStateIdle )
			m_eState = NodeStatePlay; // Idle mode due to virtual voices ? bring it back to play

#ifdef AK_PS3
		if ( m_pLastItemMix == NULL )
		{
			if ( !CAkLEngine::AddExecutingBus( this ) )
				return;

			// First mixer to be added to final mix: push job (m_Mixer.FinalExecuteSPU() blocks on job chain).
			m_Mixer.ExecuteSPU(&io_rBuffer, m_BufferOut, in_PanMix);
		}
		else
		{
			m_pLastItemMix->pNextItemMix = &io_rBuffer;
			m_pLastItemMix->pNextItemVolumes = in_PanMix;
			m_pLastItemMix->pNextVolumeAttenuation = NULL;
		}

		m_pLastItemMix = &io_rBuffer;
		io_rBuffer.pNextItemMix = NULL;
		io_rBuffer.pNextItemVolumes = NULL;
		io_rBuffer.pNextVolumeAttenuation = NULL;
#else
		// Mixer requires that buffer is zeroed out if buffer is not complete.
		ZeroPadBuffer( &io_rBuffer );
		io_rBuffer.uValidFrames = io_rBuffer.MaxFrames();

		// apply the volumes and final mix with previous buffer (if any)
		m_Mixer.Mix( &io_rBuffer, &m_BufferOut, in_bPan, in_PanMix );
#endif
	}
}

void CAkVPLMixBusNode::ConsumeBuffer(     AkVPLState & io_rVPLState
										, AkAudioMix in_arMix[]
#ifdef AK_PS3
										, AkInt8 in_iVPLEnvironmentIndex
										, AkVolumeOffset* in_pAttenuation
#endif
)
{
	// Now, send it to its connected bus (the dry bus in the case of an environmental)
	if( io_rVPLState.uValidFrames > 0 )
	{
		if (!m_bEffectCreated)
			SetAllInsertFx();

#ifndef AK_OPTIMIZED
		IncrementMixingVoiceCount();
#endif
		// Set plugin audio buffer eState before execution (it will get updated by the effect chain)
		m_BufferOut.eState = AK_DataReady;
		if ( m_eState == NodeStateIdle )
			m_eState = NodeStatePlay; // Idle mode due to virtual voices ? bring it back to play

#ifdef AK_PS3
		// WG-6243  PS3: game will freeze if sound is played on a game object in more than 1 environment  
		if( io_rVPLState.result != AK_ProcessNeeded )
			io_rVPLState.resultPrevious = io_rVPLState.result;

		AKASSERT(((uintptr_t)in_pAttenuation & 0x0000000F) == 0);
		if ( m_pLastItemMix == NULL )
		{
			if ( !CAkLEngine::AddExecutingBus( this ) )
			{
				io_rVPLState.result = AK_Fail;
				return;
			}

			m_Mixer.ExecuteSPU(&io_rVPLState, m_BufferOut, in_arMix, in_iVPLEnvironmentIndex, in_pAttenuation ); // Only one SPU job is needed for the mix list

			AKASSERT( AK_ProcessNeeded == io_rVPLState.result );
		}
		else
		{
			m_pLastItemMix->pNextItemMix = &io_rVPLState;
			m_pLastItemMix->pNextItemVolumes = in_arMix;
			m_pLastItemMix->pNextVolumeAttenuation = in_pAttenuation;
			io_rVPLState.result = AK_ProcessNeeded;
		}

		m_pLastItemMix = &io_rVPLState;
		io_rVPLState.pNextItemMix = NULL;
		io_rVPLState.pNextItemVolumes = NULL;
		io_rVPLState.pNextVolumeAttenuation = NULL;
#else
		// Mixer requires that buffer is zeroed out if buffer is not complete.
		ZeroPadBuffer( &io_rVPLState );
		io_rVPLState.uValidFrames = io_rVPLState.MaxFrames();

		// Mix with previous buffer (if any)
		m_Mixer.Mix3D( &io_rVPLState, &m_BufferOut, in_arMix );
#endif
	}
}

// Just update state to "playing" without mixing anything.
#ifndef AK_OPTIMIZED
void CAkVPLMixBusNode::ConsumeBufferMute( AkVPLState & 
#ifdef AK_PS3
	io_state 
#endif
	)
{
	IncrementMixingVoiceCount();

	// Set plugin audio buffer eState before execution (it will get updated by the effect chain)
	m_BufferOut.eState = AK_DataReady;
	if ( m_eState == NodeStateIdle )
		m_eState = NodeStatePlay; // Idle mode due to virtual voices ? bring it back to play

#ifdef AK_PS3
	io_state.result = io_state.resultPrevious;
#endif
}
#endif

void CAkVPLMixBusNode::ProcessDone( AkVPLState & io_state )
{
#ifdef AK_PS3
	io_state.result = io_state.resultPrevious;
#endif
}

//-----------------------------------------------------------------------------
// Name: GetResultingBuffer
// Desc: Returns the resulting mixed buffer.
//
// Parameters:
//	AkAudioBuffer* io_pBuffer : Pointer to a buffer object.
//-----------------------------------------------------------------------------
void CAkVPLMixBusNode::GetResultingBuffer( AkAudioBufferBus*& io_rpBuffer )
{
#ifndef AK_PS3
	ProcessAllFX();
	PostProcessFx( io_rpBuffer );

#else

	// Setup our buffer for FinalMix job.
	m_BufferOut.pNextItemMix = NULL; 
	
	// Return resulting buffer.
	io_rpBuffer = &m_BufferOut;
#endif

#ifndef AK_OPTIMIZED
#ifdef AK_PS3
	if( AkMonitor::GetNotifFilter() & AkMonitorData::MonitorDataMeter )
		io_rpBuffer->pMeterCtx = m_pMeterCtx;
	else
		io_rpBuffer->pMeterCtx = NULL;
#else
	if( ( m_pMeterCtx )
		&& (AkMonitor::GetNotifFilter() & AkMonitorData::MonitorDataMeter) )
	{
		MeterBuffer( io_rpBuffer, m_pMeterCtx );
	}
#endif
#endif

	UpdatePanningVolumes();
} // GetResultingBuffer

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void CAkVPLMixBusNode::ProcessAllFX()
{
	bool bFxProcessedUnused;
	if ( m_eState == NodeStatePlay )
	{
		for ( AkUInt32 uFXIndex = 0; uFXIndex < AK_NUM_EFFECTS_PER_OBJ; ++uFXIndex )
		{
			ProcessFX( uFXIndex, bFxProcessedUnused );
		}
	}
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void CAkVPLMixBusNode::ProcessFX( AkUInt32 in_fxIndex, bool &io_bfxProcessed )
{
	if ( m_eState == NodeStatePlay 
#ifdef AK_PS3
		|| m_eState == NodeStateProcessing
#endif
		)
	{
		FX & fx = m_aFX[ in_fxIndex ];
		if( fx.pEffect != NULL )
		{
			if ( ( fx.bBypass | m_bBypassAllFX ) == 0 )
			{
				// Ensure SIMD can be used without additional considerations
				AKASSERT( m_BufferOut.MaxFrames() % 4 == 0 );
							
#ifdef AK_PS3
				AK::MultiCoreServices::DspProcess* pDspProcess = NULL;
				fx.pEffect->Execute( &m_BufferOut, pDspProcess );

				if ( pDspProcess )
				{
					CAkLEngine::QueueDspProcess(pDspProcess);
					m_eState = NodeStateProcessing;
					io_bfxProcessed = true;
				}
#else
				AK_START_PLUGIN_TIMER( fx.id );
				fx.pEffect->Execute( &m_BufferOut );
				AK_STOP_PLUGIN_TIMER( fx.id );
				AKASSERT( m_BufferOut.uValidFrames <= m_BufferOut.MaxFrames() );	// Produce <= than requested
#endif

				AKSIMD_ASSERTFLUSHZEROMODE;
			}
			else
			{
				if ( ( fx.bLastBypass | m_bLastBypassAllFX ) == 0 )
				{
					fx.pEffect->Reset( );
				}
			}

			fx.bLastBypass = fx.bBypass;
		}
	}
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void CAkVPLMixBusNode::PostProcessFx( AkAudioBufferBus*& io_rpBuffer )
{
	if ( m_eState == NodeStatePlay 
#ifdef AK_PS3
		|| m_eState == NodeStateProcessing
#endif
		)
	{
		m_bLastBypassAllFX = m_bBypassAllFX;
	}

	m_BufferOut.m_fNextVolume = GetNextVolume();
	m_BufferOut.m_fPreviousVolume = GetPreviousVolume();

	TagPreviousVolumes();

	// Watchout uValidFrames is not known yet for PS3 asynchronous processing if there is a bus effect
	// Note: Effects eState result are directly passed to the pipeline by the following
	io_rpBuffer = &m_BufferOut;
}

#ifdef AK_VITA_HW
bool CAkVPLMixBusNode::IsExpired( AkReal32 in_uElapsedTime )
{
	bool bExpired = false;
	// The environmental FX gets expired ($m_TailDuration)ms time after no sound is using it.
	if ( !m_bIsActive )
	{
		if ( in_uElapsedTime >= m_TimeToLive )
		{
			bExpired = true;
		}
		else
		{
			m_TimeToLive -= in_uElapsedTime;
		}
	}
	else
	{
		// Reset.
		m_bIsActive = false;
		m_TimeToLive = m_TailDuration;
	}
	return bExpired;
}
#endif