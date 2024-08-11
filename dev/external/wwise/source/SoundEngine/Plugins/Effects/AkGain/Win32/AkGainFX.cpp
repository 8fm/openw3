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
// AkGainFX.cpp
//
// Gain FX implementation.
// 
//
// Copyright 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "AkGainFX.h"
#include <AK/DSP/AkApplyGain.h>
#include <math.h>
#include <AK/Tools/Common/AkAssert.h>

#ifdef AK_PS3
// embedded SPU Job Binary symbols
extern char _binary_GainFX_spu_bin_start[];
extern char _binary_GainFX_spu_bin_size[];
static AK::MultiCoreServices::BinData GainFXJobBin = { _binary_GainFX_spu_bin_start, CELL_SPURS_GET_SIZE_BINARY( _binary_GainFX_spu_bin_size ) };
#endif

// Plugin mechanism. Dynamic create function whose address must be registered to the FX manager.
AK::IAkPlugin* CreateGainFX( AK::IAkPluginMemAlloc * in_pAllocator )
{
	AKASSERT( in_pAllocator != NULL );
	return AK_PLUGIN_NEW( in_pAllocator, CAkGainFX( ) );
}


// Constructor.
CAkGainFX::CAkGainFX()
{
}

// Destructor.
CAkGainFX::~CAkGainFX()
{

}

// Initializes and allocate memory for the effect
AKRESULT CAkGainFX::Init(	AK::IAkPluginMemAlloc * in_pAllocator,		// Memory allocator interface.
									AK::IAkEffectPluginContext * in_pFXCtx,	// FX Context
									AK::IAkPluginParam * in_pParams,		// Effect parameters.
									AkAudioFormat &	in_rFormat				// Required audio input format.
								)
{
	// Save format internally
	m_uNumProcessedChannels = in_rFormat.GetNumChannels( );
	m_uSampleRate = in_rFormat.uSampleRate;

	// Set parameters.
	m_pSharedParams = static_cast<CAkGainFXParams*>(in_pParams);

	// Gain ramp initialization Gains level
	m_fCurrentFullBandGain = m_pSharedParams->GetFullbandGain();
	m_fCurrentLFEGain = m_pSharedParams->GetLFEGain();

	AK_PERF_RECORDING_RESET();

	return AK_Success;
}

// Terminates.
AKRESULT CAkGainFX::Term( AK::IAkPluginMemAlloc * in_pAllocator )
{
	// Effect's deletion
	AK_PLUGIN_DELETE( in_pAllocator, this );
	return AK_Success;
}

// Reset or seek to start.
AKRESULT CAkGainFX::Reset( )
{
	return AK_Success;
}

// Effect info query.
AKRESULT CAkGainFX::GetPluginInfo( AkPluginInfo & out_rPluginInfo )
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

#ifdef AK_PS3
// Execute Gain effect.
void CAkGainFX::Execute(	AkAudioBuffer*	io_pBuffer,							// Input buffer interface.
									AK::MultiCoreServices::DspProcess*&	out_pDspProcess	// the process that needs to run
									)
{
	if ( m_uNumProcessedChannels == 0 )
	{
		out_pDspProcess = NULL;
		return;
	}

	m_DspProcess.ResetDspProcess( true );
	m_DspProcess.SetDspProcess( GainFXJobBin );

	// Send PluginAudioBuffer information
	// Watchout: uValidFrames may not be right at this time. It will be before the DMA actually goes to SPU.
	m_DspProcess.AddDspProcessSmallDma( io_pBuffer, sizeof(AkAudioBuffer) );

	// Note: Subsequent DMAs are contiguous in memory on SPU side.
	m_DspProcess.AddDspProcessDma( io_pBuffer->GetDataStartDMA(), sizeof(AkReal32) * io_pBuffer->MaxFrames() * m_uNumProcessedChannels );
	
	AkReal32 fTargetFullBandGain = m_pSharedParams->GetFullbandGain();
	AkReal32 fTargetLFEGain = m_pSharedParams->GetLFEGain();

 	m_DspProcess.SetUserData( 5, *((AkUInt32 *) &m_fCurrentFullBandGain ) );
	m_DspProcess.SetUserData( 6, *((AkUInt32 *) &fTargetFullBandGain ) );
	m_DspProcess.SetUserData( 7, *((AkUInt32 *) &m_fCurrentLFEGain ) );
	m_DspProcess.SetUserData( 8, *((AkUInt32 *) &fTargetLFEGain ) );

	out_pDspProcess = &m_DspProcess;

	m_fCurrentFullBandGain = fTargetFullBandGain;
	m_fCurrentLFEGain = fTargetLFEGain;
}

#else
//  Execute Gain effect.
void CAkGainFX::Execute( AkAudioBuffer* io_pBuffer	)
{
	if ( m_uNumProcessedChannels == 0 || io_pBuffer->uValidFrames == 0 )
		return;

	AK_PERF_RECORDING_START( "Gain", 25, 30 );
	
	AkReal32 fTargetFullBandGain = m_pSharedParams->GetFullbandGain();
	AkReal32 fTargetLFEGain = m_pSharedParams->GetLFEGain();

	AK::DSP::ApplyGain( io_pBuffer, m_fCurrentFullBandGain, fTargetFullBandGain, false/*Don't process LFE*/ );
	AK::DSP::ApplyGainLFE( io_pBuffer, m_fCurrentLFEGain, fTargetLFEGain );
	m_fCurrentFullBandGain = fTargetFullBandGain;
	m_fCurrentLFEGain = fTargetLFEGain;

	AK_PERF_RECORDING_STOP( "Gain", 25, 30 );
}
#endif

