//////////////////////////////////////////////////////////////////////
//
// AkFXSrcSine.cpp
//
// Sample sine tone source. Implements a very basic oscillator.
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "AkFXSrcSine.h"
#include <AK/Tools/Common/AkAssert.h>

/// Plugin mechanism. Instanciation method that must be registered to the plug-in manager.
AK::IAkPlugin* CreateSineSource( AK::IAkPluginMemAlloc * in_pAllocator )
{
	return AK_PLUGIN_NEW( in_pAllocator, CAkFXSrcSine() );
}

/// Constructor.
CAkFXSrcSine::CAkFXSrcSine() : m_pParams(NULL)
{
}

/// Destructor.
CAkFXSrcSine::~CAkFXSrcSine()
{

}

/// Initializes the effect.
AKRESULT CAkFXSrcSine::Init(	
							AK::IAkPluginMemAlloc *			/*in_pAllocator*/, 	/// Memory allocator interface.
							AK::IAkSourcePluginContext *	in_pSourceFXContext,/// Sound engine plug-in execution context.
							AK::IAkPluginParam *			in_pParams,			/// Associated effect parameters node.
							AkAudioFormat &					io_rFormat			/// Output audio format.
                           )
{
    m_pParams = (CAkFxSrcSineParams*)in_pParams;

	// Notify pipeline of chosen output format change.
	// You must also change other format field when changing number of output channel or sample type.
	io_rFormat.uChannelMask = m_pParams->uChannelMask;
	if ( !io_rFormat.IsChannelConfigSupported() )
	{
		// LFE channel is not supported on the some platforms. 
		// Output just mono channel in these case.
		io_rFormat.uChannelMask = AK_SPEAKER_SETUP_MONO;
	}
	
	m_SineState.m_fPreviousGain = m_pParams->fGain;	// Gain ramp initialization

	// Setup helper to handle looping and possibly changing duration of synthesis
	m_SineState.DurationHandler.Setup( m_pParams->fDuration, in_pSourceFXContext->GetNumLoops(), io_rFormat.uSampleRate ); 
	m_SineState.Oscillator.Setup( m_pParams->fFrequency, 0.f, io_rFormat.uSampleRate ); // Prepare sine oscillator DSP

	m_pParams->m_ParamChangeHandler.ResetAllParamChanges(); // All parameter changes have been applied
	AK_PERF_RECORDING_RESET();

    return AK_Success;
}

/// The effect must destroy itself herein.
AKRESULT CAkFXSrcSine::Term( AK::IAkPluginMemAlloc * in_pAllocator )
{
	AK_PLUGIN_DELETE( in_pAllocator, this );
    return AK_Success;
}


/// Reinitialize processing state.
AKRESULT CAkFXSrcSine::Reset( )
{
	m_SineState.DurationHandler.Reset();
    return AK_Success;
}

/// Effect info query.
AKRESULT CAkFXSrcSine::GetPluginInfo( AkPluginInfo & out_rPluginInfo )
{
    out_rPluginInfo.eType = AkPluginTypeSource;
	out_rPluginInfo.bIsInPlace = true;
#ifdef AK_PS3
	out_rPluginInfo.bIsAsynchronous = true; // Set this to false for PPU execution on PS3 (source plug-ins only)
#else
	out_rPluginInfo.bIsAsynchronous = false; // Other platform only support synchronous processing
#endif
    return AK_Success;
}

/// Skip processing of some frames when voice is virtual from elapsed time.
AKRESULT CAkFXSrcSine::TimeSkip( AkUInt32 &io_uFrames )
{
	// Continue keeping track of update of duration parameter
	if ( m_pParams->m_ParamChangeHandler.HasChanged( AK_SINE_FXPARAM_DURATION_ID ) )
		m_SineState.DurationHandler.SetDuration( m_pParams->fDuration );
	m_pParams->m_ParamChangeHandler.ResetParamChange( AK_SINE_FXPARAM_DURATION_ID );
	m_SineState.m_fPreviousGain = m_pParams->fGain;

	AkUInt16 uValidFrames = (AkUInt16)io_uFrames;
	AKRESULT eResult = m_SineState.DurationHandler.ProduceBuffer( (AkUInt16)io_uFrames, uValidFrames );
	io_uFrames = uValidFrames;
	return eResult;
}

#ifdef AK_PS3
// embedded SPU Job Binary symbols
extern char _binary_SineFX_spu_bin_start[];
extern char _binary_SineFX_spu_bin_size[];
static AK::MultiCoreServices::BinData AkSineFXJobBin = { _binary_SineFX_spu_bin_start, CELL_SPURS_GET_SIZE_BINARY( _binary_SineFX_spu_bin_size ) };
#endif

//Effect processing.
void CAkFXSrcSine::Execute(	AkAudioBuffer *							io_pBufferOut		// Output buffer interface.
#ifdef AK_PS3
							, AK::MultiCoreServices::DspProcess*&	out_pDspProcess 	// the process that needs to run
#endif
							)
{
	if ( m_pParams->m_ParamChangeHandler.HasChanged( AK_SINE_FXPARAM_FREQ_ID ) )
		m_SineState.Oscillator.SetFrequency( m_pParams->fFrequency );
	if ( m_pParams->m_ParamChangeHandler.HasChanged( AK_SINE_FXPARAM_DURATION_ID ) )
		m_SineState.DurationHandler.SetDuration( m_pParams->fDuration );
	m_pParams->m_ParamChangeHandler.ResetAllParamChanges();

	m_SineState.DurationHandler.ProduceBuffer( io_pBufferOut ); 
	AKASSERT( io_pBufferOut->uValidFrames % 4 == 0 ); // For SIMD processing
	// uValidFrames now indicates how many frames to produce in this execution
	const AkUInt32 uProcessFrames = io_pBufferOut->uValidFrames;

	if ( uProcessFrames )
	{

#ifdef AK_PS3

		// Setup SPU job and DMAs
		m_DspProcess.ResetDspProcess( false ); // Using only output buffer
		m_DspProcess.SetDspProcess( AkSineFXJobBin );
		AkUInt32 uOutputSize = io_pBufferOut->NumChannels() * io_pBufferOut->uValidFrames * sizeof(AkReal32);
		m_DspProcess.SetOutputBufferSize( uOutputSize );
		// Read-only DMAs
		m_DspProcess.AddDspProcessSmallDma( io_pBufferOut, sizeof(AkAudioBuffer) ); 
		m_DspProcess.AddDspProcessSmallDma( static_cast<AkSineFXParams*>( m_pParams ), sizeof( AkSineFXParams ) ); 
		m_DspProcess.AddDspProcessSmallDma( &m_SineState, sizeof( m_SineState ) ); 
		out_pDspProcess = &m_DspProcess;

#else
		AK_PERF_RECORDING_START( "Sine", 25, 30 );

		AkSampleType * pfBufOut = io_pBufferOut->GetChannel(0);
		m_SineState.Oscillator.Process( pfBufOut, uProcessFrames, m_SineState.m_fPreviousGain, m_pParams->fGain );
		m_SineState.m_fPreviousGain = m_pParams->fGain;

		AK_PERF_RECORDING_STOP( "Sine", 25, 30 );
#endif
	}
}

// Get the duration of the source in mSec.
AkReal32 CAkFXSrcSine::GetDuration() const
{
	return m_SineState.DurationHandler.GetDuration() * 1000.f;
}

// Stop playback after the current loop iteration
AKRESULT CAkFXSrcSine::StopLooping()
{
	m_SineState.DurationHandler.SetLooping( 1 ); 
	return AK_Success;
}
