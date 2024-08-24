//////////////////////////////////////////////////////////////////////
//
// AkDelayFX.cpp
//
// Sample delay FX implementation.
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "AkDelayFX.h"
#include <AK/Tools/Common/AkAssert.h>

/// Plugin mechanism. Instanciation method that must be registered to the plug-in manager.
AK::IAkPlugin* CreateDelayFX( AK::IAkPluginMemAlloc * in_pAllocator )
{
	return AK_PLUGIN_NEW( in_pAllocator, CAkDelayFX( ) );
}

/// Constructor.
CAkDelayFX::CAkDelayFX()
	: m_pParams( NULL )
	, m_pAllocator( NULL )
{
}

/// Destructor.
CAkDelayFX::~CAkDelayFX()
{
}

/// Initializes and allocate memory for the effect.
AKRESULT CAkDelayFX::Init(	AK::IAkPluginMemAlloc *			in_pAllocator,		/// Memory allocator interface.
							AK::IAkEffectPluginContext *	in_pFXCtx,			/// Sound engine plug-in execution context.
							AK::IAkPluginParam *			in_pParams,			/// Associated effect parameters node.
							AkAudioFormat &					in_rFormat			/// Input/output audio format.
							)
{
	m_pParams = (CAkDelayFXParams*)in_pParams;
	m_pAllocator = in_pAllocator;
 
	m_FXState.Setup( m_pParams, in_pFXCtx->IsSendModeEffect(), in_rFormat.uSampleRate );
	AKRESULT eResult = m_FXState.InitDelay( in_pAllocator, m_pParams, in_rFormat.GetChannelMask() );
	m_FXState.ComputeTailLength( m_pParams->RTPC.bFeedbackEnabled, m_pParams->RTPC.fFeedback );
	m_pParams->NonRTPC.bHasChanged = false; 
	m_pParams->RTPC.bHasChanged = false;

	AK_PERF_RECORDING_RESET();

	return eResult;
}

/// Effect termination.
AKRESULT CAkDelayFX::Term( AK::IAkPluginMemAlloc * in_pAllocator )
{
	m_FXState.TermDelay( in_pAllocator );
	AK_PLUGIN_DELETE( in_pAllocator, this ); /// Effect must delete itself
	return AK_Success;
}

/// Actions to perform on FX reset (example on bypass)
AKRESULT CAkDelayFX::Reset( )
{
	m_FXState.ResetDelay();
	return AK_Success;
}

/// Effect info query.
AKRESULT CAkDelayFX::GetPluginInfo( AkPluginInfo & out_rPluginInfo )
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
// embedded SPU Job Binary symbols
extern char _binary_DelayFX_spu_bin_start[];
extern char _binary_DelayFX_spu_bin_size[];
static AK::MultiCoreServices::BinData DelayFxJobBin = { _binary_DelayFX_spu_bin_start, CELL_SPURS_GET_SIZE_BINARY( _binary_DelayFX_spu_bin_size ) };
#endif

/// Effect plug-in DSP processing
void CAkDelayFX::Execute(	AkAudioBuffer * io_pBuffer							/// Input/Output audio buffer structure.		
#ifdef AK_PS3
						 , AK::MultiCoreServices::DspProcess*& out_pDspProcess	/// SPU job setup information
#endif
						 )
{
	if ( AK_EXPECT_FALSE( m_pParams->NonRTPC.bHasChanged ) ) 
	{
		AKRESULT eResult = m_FXState.InitDelay( m_pAllocator, m_pParams, io_pBuffer->GetChannelMask() );
		if ( eResult != AK_Success )
			return; // passthrough
		m_FXState.ResetDelay();
		m_pParams->NonRTPC.bHasChanged = false; 
	}

	if ( AK_EXPECT_FALSE( m_pParams->RTPC.bHasChanged ) )
	{
		m_FXState.ComputeTailLength( m_pParams->RTPC.bFeedbackEnabled, m_pParams->RTPC.fFeedback );
		m_pParams->RTPC.bHasChanged = false;
	}
	
#ifdef AK_PS3
	// Schedule DSP to be done asynchronously on SPU
	m_DspProcess.ResetDspProcess( true );
	m_DspProcess.SetDspProcess( DelayFxJobBin );

	AkReal32 * pDelayMemDMA[2];
	AkUInt32 uDelayMemDMASize[2];
	AkUInt32 uOutputBufferSize;
	AkUInt32 uNumDelayMemDMAs = m_FXState.GetPrimeDelayMemory( io_pBuffer->MaxFrames(), pDelayMemDMA, uDelayMemDMASize, uOutputBufferSize );

	// Important note: Asynchronous effects must ALWAYS DMA AkAudioBuffer structure because this is the handshake mechanism 
	// between plug-ins in FX chains when there is no return to PPU between plug-in executions. io_pBuffer reprensents an address
	// that is persistent (held by the sound engine) that can be safely DMAed (i.e. not held on the stack).
	m_DspProcess.AddDspProcessSmallDma( io_pBuffer, sizeof(AkAudioBuffer) ); 
	m_DspProcess.AddDspProcessSmallDma( &m_FXState, sizeof(CAkDelayFXDSP) );
	m_DspProcess.AddDspProcessSmallDma( static_cast<AkDelayFXParams*>( m_pParams ), sizeof(AkDelayFXParams) );

	// Input data 
	// Note: AddDspProcessDma will split in several DMAs if data size exceeds 16 kB
	// Note: Because all channels in the sound engine are contiguous in memory sending all channels in a combined DMA as below is acceptable
	// Note: FX tail handling requires plug-in to make audio samples valid in ALL channels when modifying the uValidFrames field 
	// Note: FX tail handling may increase the number of valid frames up to max frames. Always send maxFrames to cover these situation as well
	m_DspProcess.AddDspProcessDma( io_pBuffer->GetDataStartDMA(), sizeof(AkReal32) * io_pBuffer->MaxFrames() * io_pBuffer->NumChannels() );

	// Delay memory
	for ( AkUInt32 i = 0; i < uNumDelayMemDMAs; i++ )
		m_DspProcess.AddDspProcessSmallDma( pDelayMemDMA[i], uDelayMemDMASize[i] );
	// Job-persistent output storage for triple buffering (combine with above I/O buffer)
	m_DspProcess.SetOutputBufferSize( uOutputBufferSize ); 
	
	out_pDspProcess = &m_DspProcess; 
#else
	AK_PERF_RECORDING_START( "Delay", 25, 30 );
	// Execute DSP processing synchronously here
	m_FXState.Process( io_pBuffer, m_pParams );
	AK_PERF_RECORDING_STOP( "Delay", 25, 30 );
#endif
}
