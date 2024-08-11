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
// AkMeterFX.cpp
//
// Meter processing FX implementation.
// 
//
// Copyright 2010 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "AkMeterFX.h"
#include "AkMeterFXHelpers.h"
#include "AkMeterManager.h"

// Plugin mechanism. Dynamic create function whose address must be registered to the FX manager.
AK::IAkPlugin* CreateMeterFX( AK::IAkPluginMemAlloc * in_pAllocator )
{
	AK::IAkPlugin* pPlugin = AK_PLUGIN_NEW( in_pAllocator, CAkMeterFX( ) );
	return pPlugin;
}

// Constructor.
CAkMeterFX::CAkMeterFX()
	: m_pParams( NULL )
	, m_pCtx( NULL )
	, m_pAllocator( NULL )
	, m_pMeterManager( NULL )
	, m_bTerminated( false )
{
#if defined(AK_PS3) && !defined(AK_OPTIMIZED)
	m_state.pMeterData = NULL;
#endif
}

// Destructor.
CAkMeterFX::~CAkMeterFX()
{
#if defined(AK_PS3) && !defined(AK_OPTIMIZED)
	if ( m_state.pMeterData )
	{
		AK_PLUGIN_FREE( m_pAllocator, m_state.pMeterData );
		m_state.pMeterData = NULL;
	}
#endif
}

// Initializes and allocate memory for the effect
AKRESULT CAkMeterFX::Init(	
	AK::IAkPluginMemAlloc * in_pAllocator,		// Memory allocator interface.
	AK::IAkEffectPluginContext * in_pFXCtx,		// FX Context
	AK::IAkPluginParam * in_pParams,			// Effect parameters.
	AkAudioFormat &	in_rFormat					// Required audio input format.
	)
{
	m_pParams = static_cast<CAkMeterFXParams*>(in_pParams);
	m_pAllocator = in_pAllocator;
	m_pCtx = in_pFXCtx;
	m_state.uSampleRate = in_rFormat.uSampleRate;

	m_pMeterManager = CAkMeterManager::Instance( in_pAllocator );
	if ( !m_pMeterManager )
		return AK_Fail;

	m_pMeterManager->Register( this );

	// Copy parameters to members for meter manager processing (which might occur after m_pParams is deleted)
	m_fMin = m_pParams->RTPC.fMin;
	m_uGameParamID = m_pParams->NonRTPC.uGameParamID;

	AK_PERF_RECORDING_RESET();

	return AK_Success;
}

// Terminates.
AKRESULT CAkMeterFX::Term( AK::IAkPluginMemAlloc * in_pAllocator )
{
	if ( m_pMeterManager )
	{
		m_bTerminated = true;
	}
	else // if we didn't successfully acquire meter manager, manager can't kill us.
	{
		AK_PLUGIN_DELETE( in_pAllocator, this );
	}

	return AK_Success;
}

// Reset
AKRESULT CAkMeterFX::Reset()
{
	m_state.fHoldTime = 0.0f;
	m_state.fReleaseTarget = m_pParams->RTPC.fMin;
	m_state.fLastValue = m_pParams->RTPC.fMin;
	for ( int i = 0; i < HOLD_MEMORY_SIZE; ++i )
		m_state.fHoldMemory[ i ] = m_pParams->RTPC.fMin;

	return AK_Success;
}

// Effect info query.
AKRESULT CAkMeterFX::GetPluginInfo( AkPluginInfo & out_rPluginInfo )
{
	out_rPluginInfo.eType = AkPluginTypeEffect;
	out_rPluginInfo.bIsInPlace = true;
#ifdef AK_PS3
	out_rPluginInfo.bIsAsynchronous = true;
#else
	out_rPluginInfo.bIsAsynchronous = false;
#endif
	return AK_Success;
}

#ifndef AK_PS3

void CAkMeterFX::Execute( AkAudioBuffer * io_pBuffer ) 
{
	AkUInt32 uNumChannels = io_pBuffer->NumChannels();

	AkReal32 * pfInChannel = NULL;
#ifndef AK_OPTIMIZED
	char * pMonitorData = NULL;
	int sizeofData = 0;
	if ( m_pCtx->CanPostMonitorData() )
	{
		sizeofData = sizeof( AkUInt32 ) * 1 
			+ sizeof( AkReal32 ) * uNumChannels
			+ sizeof( AkReal32 );
		pMonitorData = (char *) AkAlloca( sizeofData );
		*((AkUInt32 *) pMonitorData ) = (AkUInt32) io_pBuffer->GetChannelMask();
		pfInChannel = (AkReal32 *) ( pMonitorData + sizeof( AkUInt32 ) );
	}
#endif

	AK_PERF_RECORDING_START( "Meter", 25, 30 );

	AkReal32 fValue = AkMeterGetValue( io_pBuffer, m_pParams, pfInChannel );
	AkMeterApplyBallistics( fValue, io_pBuffer->MaxFrames(), m_pParams, &m_state );

	// Although we don't modify the output, the meter ballistics do have a tail
	bool bPreStop = io_pBuffer->eState == AK_NoMoreData;
	if ( bPreStop )
	{	
		bool bTailEndReached = ( m_state.fLastValue <= m_pParams->RTPC.fMin );
		if ( !bTailEndReached )
		{
			io_pBuffer->ZeroPadToMaxFrames();
			io_pBuffer->eState = AK_DataReady;
		}
	}

	// Copy parameters to members for meter manager processing (which might occur after m_pParams is deleted)
	m_fMin = m_pParams->RTPC.fMin;
	m_uGameParamID = m_pParams->NonRTPC.uGameParamID;

	AK_PERF_RECORDING_STOP( "Meter", 25, 30 );

#ifndef AK_OPTIMIZED
	if ( pfInChannel )
	{
		pfInChannel[ uNumChannels ] = m_state.fLastValue;

		m_pCtx->PostMonitorData( pMonitorData, sizeofData );
	}
#endif
}

#else

// embedded SPU Job Binary symbols
extern char _binary_MeterFX_spu_bin_start[];
extern char _binary_MeterFX_spu_bin_size[];
static AK::MultiCoreServices::BinData MeterFXJobBin = { _binary_MeterFX_spu_bin_start, CELL_SPURS_GET_SIZE_BINARY( _binary_MeterFX_spu_bin_size ) };

void CAkMeterFX::Execute( 
	AkAudioBuffer * io_pBuffer,
	AK::MultiCoreServices::DspProcess*& out_pDspProcess
	) 
{
	AkUInt32 uNumChannels = io_pBuffer->NumChannels();

#ifndef AK_OPTIMIZED
	if ( m_pCtx->CanPostMonitorData() )
	{
		// Prepare pMeterData buffer. SPU job will recognize this and fill it appropriately, so that we
		// can send it at the next Execute().

		AkUInt32 sizeofData = sizeof( AkUInt32 ) * 1 
			+ sizeof( AkReal32 ) * uNumChannels
			+ sizeof( AkReal32 );

		if ( m_state.pMeterData )
			m_pCtx->PostMonitorData( m_state.pMeterData, sizeofData );
		else
		{
			m_state.pMeterData = AK_PLUGIN_ALLOC( m_pAllocator, AK_ALIGN_SIZE_FOR_DMA( sizeofData ) );
			*((AkUInt32 *) m_state.pMeterData) = 0; // Make sure there is no way we send garbage as the size values
		}
	}
	else
	{
		if ( m_state.pMeterData )
		{
			AK_PLUGIN_FREE( m_pAllocator, m_state.pMeterData );
			m_state.pMeterData = NULL;
		}
	}
#endif

	// Copy parameters to members for meter manager processing (which might occur after m_pParams is deleted)
	m_fMin = m_pParams->RTPC.fMin;
	m_uGameParamID = m_pParams->NonRTPC.uGameParamID;

	// Prepare for SPU job
	m_DspProcess.ResetDspProcess( true );
	m_DspProcess.SetDspProcess( MeterFXJobBin ); 

	// Send plugin audio buffer information
	m_DspProcess.AddDspProcessSmallDma( io_pBuffer, sizeof(AkAudioBuffer) );
	m_DspProcess.AddDspProcessSmallDma( &m_state, sizeof(AkMeterState) );
	m_DspProcess.AddDspProcessSmallDma( (AkMeterFXParams *) m_pParams, sizeof(AkMeterFXParams) );

	// Note: Subsequent DMAs are contiguous in memory on SPU side. 
	m_DspProcess.AddDspProcessDma( io_pBuffer->GetDataStartDMA(), sizeof(AkReal32) * io_pBuffer->MaxFrames() * uNumChannels );

	out_pDspProcess = &m_DspProcess;
}

#endif