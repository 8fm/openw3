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

#include "AkTimeStretchFX.h"
#include <math.h>
#include <AK/DSP/AkApplyGain.h>
#include <AK/Tools/Common/AkAssert.h>
#include "AkRandomValues.h"

#ifdef __PPU__
// embedded SPU Job Binary symbols
extern char _binary_AkTimeStretchFXJob_spu_bin_start[];
extern char _binary_AkTimeStretchFXJob_spu_bin_size[];
static AK::MultiCoreServices::BinData AkTimeStretchFXJobBin = { _binary_AkTimeStretchFXJob_spu_bin_start, CELL_SPURS_GET_SIZE_BINARY( _binary_AkTimeStretchFXJob_spu_bin_size ) };
#endif

// Plugin mechanism. Dynamic create function whose address must be registered to the FX manager.
AK::IAkPlugin* CreateTimeStretchFX( AK::IAkPluginMemAlloc * in_pAllocator )
{
	return AK_PLUGIN_NEW( in_pAllocator, CAkTimeStretchFX( ) );
}


// Constructor.
CAkTimeStretchFX::CAkTimeStretchFX() 
	: m_pSharedParams(NULL)
	, m_pAllocator(NULL)
{

}

// Destructor.
CAkTimeStretchFX::~CAkTimeStretchFX()
{

}

// Initializes and allocate memory for the effect
AKRESULT CAkTimeStretchFX::Init(	AK::IAkPluginMemAlloc * in_pAllocator,	// Memory allocator interface.
									AK::IAkEffectPluginContext * in_pFXCtx,	// FX Context
									AK::IAkPluginParam * in_pParams,		// Effect parameters.
									AkAudioFormat &	in_rFormat				// Required audio input format.
									)
{
	m_pSharedParams = (CAkTimeStretchFXParams*)(in_pParams);
	m_pAllocator = in_pAllocator;

	// Initialize components
	m_FXInfo.uNumChannels = in_rFormat.GetNumChannels();
	m_FXInfo.uSampleRate = in_rFormat.uSampleRate;

	// Initial computations
	m_pSharedParams->GetParams( &m_FXInfo.Params );
	m_FXInfo.PrevParams = m_FXInfo.Params;

	// Compute random offset value to be applied over time stretch value for whole duration of this instance
	DSP::CAkRandomValues Rand;
	m_FXInfo.fTSRandomOffset = Rand.RandomRange( -m_FXInfo.Params.fTimeStretchRandom, m_FXInfo.Params.fTimeStretchRandom );

	AKRESULT eResult = AK_Success;
#ifndef USE_SIMPLE_RESAMPLER
	eResult = m_FXInfo.PhaseVocoder.Init( 
		in_pAllocator, 
		in_rFormat.GetNumChannels(), 
		in_rFormat.uSampleRate, 
		m_FXInfo.Params.uWindowSize );	

	m_FXInfo.bCanEnterNoTSMode = false;
	m_FXInfo.uNoTSModeCount = 0;
#endif

	m_fSkippedFrames = 0.f;

	AK_PERF_RECORDING_RESET();

	return eResult;
}

// Terminates.
AKRESULT CAkTimeStretchFX::Term( AK::IAkPluginMemAlloc * in_pAllocator )
{
#ifndef USE_SIMPLE_RESAMPLER
	m_FXInfo.PhaseVocoder.Term( in_pAllocator );
#endif

	AK_PLUGIN_DELETE( in_pAllocator, this );
	return AK_Success;
}

// Reset or seek to start.
AKRESULT CAkTimeStretchFX::Reset( )
{
#ifdef USE_SIMPLE_RESAMPLER
	m_FXInfo.LinearResampler.Reset();
#else
	m_FXInfo.PhaseVocoder.Reset();
#endif

	return AK_Success;
}


// Effect info query.
AKRESULT CAkTimeStretchFX::GetPluginInfo( AkPluginInfo & out_rPluginInfo )
{
	out_rPluginInfo.eType = AkPluginTypeEffect;
	out_rPluginInfo.bIsInPlace = false;
#ifdef AK_PS3
	out_rPluginInfo.bIsAsynchronous = true;
#else	
	out_rPluginInfo.bIsAsynchronous = false;
#endif	
	return AK_Success;
}

void CAkTimeStretchFX::Execute( AkAudioBuffer * in_pBuffer,
								AkUInt32		in_uInOffset,
								AkAudioBuffer * out_pBuffer
#ifdef AK_PS3
								, AK::MultiCoreServices::DspProcess*&	out_pDspProcess
#endif						
								)
{
	AKASSERT( m_FXInfo.uNumChannels == in_pBuffer->NumChannels() );
	AKASSERT( m_FXInfo.uNumChannels == out_pBuffer->NumChannels() );

	// Reset skipped frames, we now output something.
	m_fSkippedFrames = 0.f;

	// Compute changes if parameters have changed
	m_pSharedParams->GetParams( &m_FXInfo.Params );
	
#ifndef USE_SIMPLE_RESAMPLER
	if ( m_FXInfo.Params.uWindowSize != m_FXInfo.PrevParams.uWindowSize )
	{
		m_FXInfo.PhaseVocoder.Term( m_pAllocator );
		AKRESULT eResult = m_FXInfo.PhaseVocoder.Init( 
			m_pAllocator, 
			m_FXInfo.uNumChannels, 
			m_FXInfo.uSampleRate, 
			m_FXInfo.Params.uWindowSize );
		if ( eResult != AK_Success )
		{
			Bypass( in_pBuffer, in_uInOffset, out_pBuffer );
			return;
		}
		m_FXInfo.PhaseVocoder.Reset();
		m_FXInfo.PrevParams.uWindowSize = m_FXInfo.Params.uWindowSize;
	}
#endif

	AkUInt32 uTempStorageSize = m_FXInfo.PhaseVocoder.GetTDStorageSize(); // Temporary TD/FD domain buffer for phase vocoder

#ifdef __PPU__
	// Prepare for SPU job and reserve scratch memory size
	m_DspProcess.ResetDspProcess( true );
	m_DspProcess.SetDspProcess( AkTimeStretchFXJobBin );
#ifndef USE_SIMPLE_RESAMPLER
	AkUInt32 uTotalScracthSize = m_FXInfo.PhaseVocoder.GetScratchMemRequirement() + uTempStorageSize;
	m_DspProcess.SetScratchSize( uTotalScracthSize );
#endif

	// Schedule DMA transfers
	m_DspProcess.AddDspProcessSmallDma( in_pBuffer, sizeof(AkAudioBuffer) );
	m_DspProcess.AddDspProcessSmallDma( out_pBuffer, sizeof(AkAudioBuffer) );
	m_DspProcess.AddDspProcessSmallDma( &m_FXInfo, sizeof( m_FXInfo ) );
	m_DspProcess.AddDspProcessDma( out_pBuffer->GetDataStartDMA(), sizeof(AkReal32) * out_pBuffer->MaxFrames() * out_pBuffer->NumChannels() );
	// Possible to have null input buffer data in tail
	AkReal32 * pfInputData = (AkReal32*)in_pBuffer->GetDataStartDMA();
	if ( pfInputData )
		m_DspProcess.AddDspProcessDma( in_pBuffer->GetDataStartDMA(), sizeof(AkReal32) * in_pBuffer->MaxFrames() * in_pBuffer->NumChannels() );
	
	  // The rest is pulled from the SPU job as required
	m_DspProcess.SetUserData( 9, (AkUInt64)in_uInOffset );

	out_pDspProcess = &m_DspProcess;
#else

	AK_PERF_RECORDING_START( "TimeStretch", 25, 30 );

	AkReal32 fCurrentTSFactor = m_FXInfo.Params.fTimeStretch + m_FXInfo.fTSRandomOffset;
	fCurrentTSFactor = AkMin( fCurrentTSFactor, 1600.f ); 
	fCurrentTSFactor = AkMax( fCurrentTSFactor, 25.f ); 

	// Special case to recover minimum artefacts when back to no time stretch
	// Note: Note bypassing the algorithm here to change TS value without glitch
	if ( fCurrentTSFactor == 100.f )
	{
		AkReal32 fPrevTSFactor = m_FXInfo.PrevParams.fTimeStretch + m_FXInfo.fTSRandomOffset;
		if ( fPrevTSFactor != 100.f )
		{
			m_FXInfo.bCanEnterNoTSMode = true;
			m_FXInfo.uNoTSModeCount = 0;
		}

		if ( m_FXInfo.bCanEnterNoTSMode )
		{
			m_FXInfo.uNoTSModeCount++;
		}
	}
	else
	{
		m_FXInfo.bCanEnterNoTSMode = false;
		m_FXInfo.uNoTSModeCount = 0;
	}
	bool bEnterNoTSMode = m_FXInfo.uNoTSModeCount == 8; // last 8 passes had TS factor of 100
	
#ifdef USE_SIMPLE_RESAMPLER
	// Simple linear resampler execution
	m_FXInfo.LinearResampler.Execute( in_pBuffer, in_uInOffset, out_pBuffer, fCurrentTSFactor );
#else

	// Phase vocoder execution
	AkReal32 * pfTempStorage = (AkReal32 *)AK_PLUGIN_ALLOC( m_pAllocator, uTempStorageSize );
	if ( pfTempStorage == NULL  )
	{
		Bypass( in_pBuffer, in_uInOffset, out_pBuffer );
		// Pipeline cannot properly handle errors here, just play dry signal when out of memory
		return; 
	}

	m_FXInfo.PhaseVocoder.Execute( 
		in_pBuffer, 
		in_uInOffset, 
		out_pBuffer, 
		fCurrentTSFactor, 
		bEnterNoTSMode,
		pfTempStorage);

	if ( bEnterNoTSMode )
	{
		m_FXInfo.bCanEnterNoTSMode = false;
		m_FXInfo.uNoTSModeCount = 0;
	}


	AKASSERT( pfTempStorage );
	AK_PLUGIN_FREE( m_pAllocator, pfTempStorage );

#endif // USE_SIMPLE_RESAMPLER

	if ( out_pBuffer->eState == AK_DataReady || out_pBuffer->eState == AK_NoMoreData )
	{
		AK::DSP::ApplyGain( out_pBuffer, m_FXInfo.PrevParams.fOutputGain, m_FXInfo.Params.fOutputGain );
		m_FXInfo.PrevParams = m_FXInfo.Params;
	}

	AK_PERF_RECORDING_STOP( "TimeStretch", 25, 30 );

#endif // __PPU__
}

void CAkTimeStretchFX::Bypass( 
	  AkAudioBuffer * in_pBuffer, 
	  AkUInt32		in_uInOffset, 
	  AkAudioBuffer * out_pBuffer )
{
		// Copy all data from input to output and set the output eState accordingly
		AkUInt32 uNumChannels = AkMin( in_pBuffer->NumChannels(), out_pBuffer->NumChannels() ); 
		AkUInt32 uNumFrames = AkMin( in_pBuffer->uValidFrames, out_pBuffer->MaxFrames()-out_pBuffer->uValidFrames );
		for ( AkUInt32 i = 0; i < uNumChannels; i++ )
		{
			AkReal32 * pfIn = in_pBuffer->GetChannel( i ) + in_uInOffset;
			AkReal32 * pfOut = out_pBuffer->GetChannel( i ) + out_pBuffer->uValidFrames;
			AKPLATFORM::AkMemCpy( pfOut, pfIn, uNumFrames*sizeof(AkReal32) );
		}
		out_pBuffer->uValidFrames += (AkUInt16)uNumFrames;
		in_pBuffer->uValidFrames -= (AkUInt16)uNumFrames;
		if ( in_pBuffer->eState == AK_NoMoreData && in_pBuffer->uValidFrames == 0 )
			out_pBuffer->eState = AK_NoMoreData;
		else if ( out_pBuffer->uValidFrames == out_pBuffer->MaxFrames() )
			out_pBuffer->eState = AK_DataReady;
		else
			out_pBuffer->eState = AK_DataNeeded;
}

AKRESULT CAkTimeStretchFX::TimeSkip( AkUInt32 &io_uFrames )
{
	if (io_uFrames == 0)
		return AK_NoMoreData;

	// Compute changes if parameters have changed
	m_pSharedParams->GetParams( &m_FXInfo.Params );

	AkReal32 fCurrentTSFactor = m_FXInfo.Params.fTimeStretch + m_FXInfo.fTSRandomOffset;
	fCurrentTSFactor = AkMin( fCurrentTSFactor, 1600.f ); 
	fCurrentTSFactor = AkMax( fCurrentTSFactor, 25.f ); 

	//Timeskip must provide the number of frames needed from the input to produce io_uFrames.
	//Do the math in float to avoid drifting over long periods.
	AkReal32 fCurrentSkip = m_fSkippedFrames;
	m_fSkippedFrames += io_uFrames * 100.f / fCurrentTSFactor;
	io_uFrames = (AkUInt32)(m_fSkippedFrames - fCurrentSkip);

	return AK_DataReady;
}
