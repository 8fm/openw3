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

#include "AkPitchShifterFX.h"
#include <math.h>
#include <AK/Tools/Common/AkAssert.h>
#include "AkPitchShifterDSPProcess.h"


#ifdef __PPU__
// embedded SPU Job Binary symbols
extern char _binary_AkPitchShifterFXJob_spu_bin_start[];
extern char _binary_AkPitchShifterFXJob_spu_bin_size[];
static AK::MultiCoreServices::BinData AkPitchShifterFXJobBin = { _binary_AkPitchShifterFXJob_spu_bin_start, CELL_SPURS_GET_SIZE_BINARY( _binary_AkPitchShifterFXJob_spu_bin_size ) };
#endif

// Plugin mechanism. Dynamic create function whose address must be registered to the FX manager.
AK::IAkPlugin* CreatePitchShifterFX( AK::IAkPluginMemAlloc * in_pAllocator )
{
	return AK_PLUGIN_NEW( in_pAllocator, CAkPitchShifterFX( ) );
}


// Constructor.
CAkPitchShifterFX::CAkPitchShifterFX() 
	: m_pParams(NULL)
	, m_pAllocator(NULL)
{

}

// Destructor.
CAkPitchShifterFX::~CAkPitchShifterFX()
{

}

// Initializes and allocate memory for the effect
AKRESULT CAkPitchShifterFX::Init(	AK::IAkPluginMemAlloc * in_pAllocator,	// Memory allocator interface.
									AK::IAkEffectPluginContext * in_pFXCtx,	// FX Context
									AK::IAkPluginParam * in_pParams,		// Effect parameters.
									AkAudioFormat &	in_rFormat				// Required audio input format.
									)
{
	m_pParams = (CAkPitchShifterFXParams*)(in_pParams);
	m_pAllocator = in_pAllocator;
	m_FXInfo.bSendMode = in_pFXCtx->IsSendModeEffect();
	m_FXInfo.uTotalNumChannels = in_rFormat.GetNumChannels();

	// Initial computations
	m_pParams->GetParams( &m_FXInfo.Params );
	// No dry signal in send mode (environmental)
	if ( m_FXInfo.bSendMode )
		m_FXInfo.Params.fDryLevel = 0.f;
	m_FXInfo.PrevParams = m_FXInfo.Params;
	m_FXInfo.uSampleRate = in_rFormat.uSampleRate;

	AkChannelMask eChannelsMask = in_rFormat.GetChannelMask(); 
	ComputeNumProcessedChannels( eChannelsMask );
	ComputeTailLength();
	AKRESULT eResult = InitPitchVoice();
	if ( eResult != AK_Success )
		return eResult;
	eResult = InitDryDelay();
	if ( eResult != AK_Success )
		return eResult;

	m_pParams->m_ParamChangeHandler.ResetAllParamChanges();

	AK_PERF_RECORDING_RESET();

	return AK_Success;
}

void CAkPitchShifterFX::ComputeNumProcessedChannels( AkChannelMask in_eChannelMask )
{
	switch ( m_FXInfo.Params.eInputType )
	{
	case AKINPUTTYPE_ASINPUT:
		m_FXInfo.uNumProcessedChannels = AK::GetNumChannels( in_eChannelMask & ~AK_SPEAKER_LOW_FREQUENCY );
		m_FXInfo.eProcessChannelMask = in_eChannelMask & ~AK_SPEAKER_LOW_FREQUENCY;
		break;
	case AKINPUTTYPE_CENTER:
		m_FXInfo.uNumProcessedChannels = AK::GetNumChannels(AK_SPEAKER_SETUP_1_0_CENTER & in_eChannelMask );	
		m_FXInfo.eProcessChannelMask = AK_SPEAKER_SETUP_1_0_CENTER;
		break;
	case AKINPUTTYPE_STEREO:
		m_FXInfo.uNumProcessedChannels = AK::GetNumChannels((AK_SPEAKER_SETUP_2_0 & in_eChannelMask));
		m_FXInfo.eProcessChannelMask = AK_SPEAKER_SETUP_2_0;
		break;
	case AKINPUTCHANNELTYPE_3POINT0:
		m_FXInfo.uNumProcessedChannels = AK::GetNumChannels((AK_SPEAKER_SETUP_3_0 & in_eChannelMask));
		m_FXInfo.eProcessChannelMask = AK_SPEAKER_SETUP_3_0;
		break;
	case AKINPUTTYPE_4POINT0:
		m_FXInfo.uNumProcessedChannels = AK::GetNumChannels((AK_SPEAKER_SETUP_4_0 & in_eChannelMask));
		m_FXInfo.eProcessChannelMask = AK_SPEAKER_SETUP_4_0;
		break;
	case AKINPUTTYPE_5POINT0:
		m_FXInfo.uNumProcessedChannels = AK::GetNumChannels(AK_SPEAKER_SETUP_5_0 & in_eChannelMask);
		m_FXInfo.eProcessChannelMask = AK_SPEAKER_SETUP_5_0;
		break;
	}

	if ( m_FXInfo.Params.bProcessLFE && (in_eChannelMask & AK_SPEAKER_LOW_FREQUENCY) )
	{
		m_FXInfo.uNumProcessedChannels++;
		m_FXInfo.eProcessChannelMask |= AK_SPEAKER_LOW_FREQUENCY;
	}
}

void CAkPitchShifterFX::ComputeTailLength()
{
	m_FXInfo.uTailLength = (AkUInt32)(m_FXInfo.Params.fDelayTime*0.001f*m_FXInfo.uSampleRate);
}

AKRESULT CAkPitchShifterFX::InitPitchVoice()
{
	if ( m_FXInfo.uNumProcessedChannels > 0 )
	{
		AKRESULT eResult = m_FXInfo.PitchShifter.Init( m_pAllocator, m_FXInfo.Params.fDelayTime, m_FXInfo.uNumProcessedChannels, m_FXInfo.uSampleRate );
		if ( eResult != AK_Success )
			return eResult;
		m_FXInfo.PitchShifter.SetPitchFactor( m_FXInfo.Params.Voice.fPitchFactor );

		if ( m_FXInfo.Params.Voice.Filter.eFilterType != AKFILTERTYPE_NONE )
		{
			m_FXInfo.Filter.ComputeCoefs(	
				(AK::DSP::MultiChannelBiquadFilter<AK_VOICE_MAX_NUM_CHANNELS>::FilterType)(m_FXInfo.Params.Voice.Filter.eFilterType-1),
				m_FXInfo.uSampleRate,
				m_FXInfo.Params.Voice.Filter.fFilterFrequency,
				m_FXInfo.Params.Voice.Filter.fFilterGain,
				m_FXInfo.Params.Voice.Filter.fFilterQFactor );
		}
	}

	return AK_Success;
}

void CAkPitchShifterFX::TermPitchVoice()
{
	if ( m_FXInfo.uNumProcessedChannels > 0 )
		m_FXInfo.PitchShifter.Term( m_pAllocator );
}

void CAkPitchShifterFX::ResetPitchVoice()
{
	if ( m_FXInfo.uNumProcessedChannels > 0 )
	{
		m_FXInfo.PitchShifter.Reset();
		m_FXInfo.Filter.Reset();
	}
}

AKRESULT CAkPitchShifterFX::InitDryDelay()
{
	const AkUInt32 uDryDelay = m_FXInfo.uTailLength/2;
	if ( m_FXInfo.Params.bSyncDry )
	{
		for ( AkUInt32 j = 0; j < m_FXInfo.uTotalNumChannels; j++ )
		{
			AKRESULT eResult = m_FXInfo.DryDelay[j].Init( m_pAllocator, uDryDelay );
			if ( eResult != AK_Success )
				return eResult;
		}
	}
	return AK_Success;
}

void CAkPitchShifterFX::TermDryDelay()
{
	for ( AkUInt32 j = 0; j < m_FXInfo.uTotalNumChannels; j++ )
		m_FXInfo.DryDelay[j].Term( m_pAllocator );
}

void CAkPitchShifterFX::ResetDryDelay()
{
	for ( AkUInt32 j = 0; j < m_FXInfo.uTotalNumChannels; j++ )
		m_FXInfo.DryDelay[j].Reset( );
}

// Terminates.
AKRESULT CAkPitchShifterFX::Term( AK::IAkPluginMemAlloc * in_pAllocator )
{
	TermPitchVoice();
	TermDryDelay();
	AK_PLUGIN_DELETE( in_pAllocator, this );
	return AK_Success;
}

// Reset or seek to start.
AKRESULT CAkPitchShifterFX::Reset( )
{
	ResetPitchVoice();
	ResetDryDelay();
	return AK_Success;
}


// Effect info query.
AKRESULT CAkPitchShifterFX::GetPluginInfo( AkPluginInfo & out_rPluginInfo )
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


void CAkPitchShifterFX::Execute(	AkAudioBuffer * io_pBuffer			
#ifdef AK_PS3
										, AK::MultiCoreServices::DspProcess*& out_pDspProcess
#endif
						 )
{
	// Compute changes if parameters have changed
	m_pParams->GetParams( &m_FXInfo.Params );
	// No dry signal in send mode (environmental)
	if ( m_FXInfo.bSendMode )
		m_FXInfo.Params.fDryLevel = 0.f;

	if ( m_pParams->m_ParamChangeHandler.HasAnyChanged() )
	{
		if ( m_pParams->m_ParamChangeHandler.HasChanged( AKPITCHSHIFTERPARAMID_DELAYTIME ) ||
			 m_pParams->m_ParamChangeHandler.HasChanged( AKPITCHSHIFTERPARAMID_INPUT ) ||
			 m_pParams->m_ParamChangeHandler.HasChanged( AKPITCHSHIFTERPARAMID_PROCESSLFE ) )
		{
			TermPitchVoice();
			TermDryDelay();
			ComputeTailLength();
			ComputeNumProcessedChannels( io_pBuffer->GetChannelMask() );
			AKRESULT eResult = InitPitchVoice();
			if ( eResult != AK_Success )
				return; // passthrough
			eResult = InitDryDelay();
			if ( eResult != AK_Success )
				return; // passthrough
			ResetPitchVoice();
			ResetDryDelay();
		}

		if ( m_pParams->m_ParamChangeHandler.HasChanged( AKPITCHSHIFTERPARAMID_SYNCDRY ) )
		{
			TermDryDelay( );
			AKRESULT eResult = InitDryDelay( ); 
			if ( eResult != AK_Success )
				return;
			ResetDryDelay();
		}

		if ( m_pParams->m_ParamChangeHandler.HasChanged( AKPITCHSHIFTERPARAMID_PITCH ) )
		{
			m_FXInfo.PitchShifter.SetPitchFactor( m_FXInfo.Params.Voice.fPitchFactor );
		}

		if ( m_pParams->m_ParamChangeHandler.HasChanged( AKPITCHSHIFTERPARAMID_FILTERTYPE ) ||
			 m_pParams->m_ParamChangeHandler.HasChanged( AKPITCHSHIFTERPARAMID_FILTERFREQUENCY ) ||
			 m_pParams->m_ParamChangeHandler.HasChanged( AKPITCHSHIFTERPARAMID_FILTERGAIN ) ||
			 m_pParams->m_ParamChangeHandler.HasChanged( AKPITCHSHIFTERPARAMID_FILTERQFACTOR ) )
		{
			if ( m_FXInfo.Params.Voice.Filter.eFilterType != AKFILTERTYPE_NONE )
			{
				m_FXInfo.Filter.ComputeCoefs(	
					(AK::DSP::MultiChannelBiquadFilter<AK_VOICE_MAX_NUM_CHANNELS>::FilterType)(m_FXInfo.Params.Voice.Filter.eFilterType-1),
					m_FXInfo.uSampleRate,
					m_FXInfo.Params.Voice.Filter.fFilterFrequency,
					m_FXInfo.Params.Voice.Filter.fFilterGain,
					m_FXInfo.Params.Voice.Filter.fFilterQFactor );
			}
		}
	}

	m_pParams->m_ParamChangeHandler.ResetAllParamChanges();
	
#ifdef __PPU__
	// Prepare for SPU job and reserve scratch memory size
	m_DspProcess.ResetDspProcess( true );
	m_DspProcess.SetDspProcess( AkPitchShifterFXJobBin );
	m_FXInfo.uSizeDelayMem = 0;
	// Streaming delay storage
	AkUInt32 uTotalScratch = m_FXInfo.Params.bSyncDry ? io_pBuffer->MaxFrames() * sizeof(AkReal32) : 0;
	if ( m_FXInfo.uNumProcessedChannels > 0 )
	{
		m_FXInfo.uSizeDelayMem = m_FXInfo.PitchShifter.GetScratchSize();		// Pitch shift delay storage
		uTotalScratch += io_pBuffer->MaxFrames() * sizeof(AkReal32);	// Wet path storage
		uTotalScratch += m_FXInfo.uSizeDelayMem;
	#ifdef AKDELAYPITCHSHIFT_USETWOPASSALGO
		uTotalScratch += io_pBuffer->MaxFrames() * (2*sizeof(AkReal32)+4*sizeof(AkInt32));
	#endif	
	}
	m_DspProcess.SetScratchSize( uTotalScratch );

	// Schedule DMA transfers
	m_DspProcess.AddDspProcessSmallDma( io_pBuffer, sizeof(AkAudioBuffer) );
	m_DspProcess.AddDspProcessSmallDma( &m_FXInfo, sizeof( m_FXInfo ) );
	m_DspProcess.AddDspProcessDma( io_pBuffer->GetDataStartDMA(), sizeof(AkReal32) * io_pBuffer->MaxFrames() * io_pBuffer->NumChannels() );

	out_pDspProcess = &m_DspProcess;
#else

	AK_PERF_RECORDING_START( "PitchShifter", 25, 30 );

	AkReal32 * pfBufferStorage = NULL;
	void * pfTwoPassStorage = NULL;
	if ( m_FXInfo.uNumProcessedChannels > 0 )
	{
		// Allocate temporary storage for wet path
		pfBufferStorage = (AkReal32*) AK_PLUGIN_ALLOC( m_pAllocator, io_pBuffer->MaxFrames() * sizeof(AkReal32) );
		if ( !pfBufferStorage )
			return; // Pipeline cannot handle errors here, just play dry signal when out of memory

#ifdef AKDELAYPITCHSHIFT_USETWOPASSALGO
		// Allocate temporary storage for 2-pass computations to avoid LHS
		pfTwoPassStorage  = (void*) AK_PLUGIN_ALLOC( m_pAllocator, io_pBuffer->MaxFrames() * (2*sizeof(AkReal32)+4*sizeof(AkInt32)) );
		if ( !pfTwoPassStorage )
			return; // Pipeline cannot handle errors here, just play dry signal when out of memory
#endif
	}
	
	AkPitchShifterDSPProcess( io_pBuffer, m_FXInfo, pfBufferStorage, pfTwoPassStorage );

	if ( pfBufferStorage )
		AK_PLUGIN_FREE( m_pAllocator, pfBufferStorage );
	if ( pfTwoPassStorage )
		AK_PLUGIN_FREE( m_pAllocator, pfTwoPassStorage );
	
	AK_PERF_RECORDING_STOP( "PitchShifter", 25, 30 );

#endif // __PPU__
}
