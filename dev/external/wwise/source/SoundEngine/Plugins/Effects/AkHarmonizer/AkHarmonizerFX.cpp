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

#include "AkHarmonizerFX.h"
#include <math.h>
#include <AK/Tools/Common/AkAssert.h>
#include "AkHarmonizerDSPProcess.h"

#ifdef __PPU__
// embedded SPU Job Binary symbols
extern char _binary_AkHarmonizerFXJob_spu_bin_start[];
extern char _binary_AkHarmonizerFXJob_spu_bin_size[];
static AK::MultiCoreServices::BinData AkHarmonizerFXJobBin = { _binary_AkHarmonizerFXJob_spu_bin_start, CELL_SPURS_GET_SIZE_BINARY( _binary_AkHarmonizerFXJob_spu_bin_size ) };
#endif

// Plugin mechanism. Dynamic create function whose address must be registered to the FX manager.
AK::IAkPlugin* CreateHarmonizerFX( AK::IAkPluginMemAlloc * in_pAllocator )
{
	return AK_PLUGIN_NEW( in_pAllocator, CAkHarmonizerFX( ) );
}


// Constructor.
CAkHarmonizerFX::CAkHarmonizerFX() 
	: m_pParams(NULL)
	, m_pAllocator(NULL)
{

}

// Destructor.
CAkHarmonizerFX::~CAkHarmonizerFX()
{

}

// Initializes and allocate memory for the effect
AKRESULT CAkHarmonizerFX::Init(	AK::IAkPluginMemAlloc * in_pAllocator,	// Memory allocator interface.
									AK::IAkEffectPluginContext * in_pFXCtx,	// FX Context
									AK::IAkPluginParam * in_pParams,		// Effect parameters.
									AkAudioFormat &	in_rFormat				// Required audio input format.
									)
{
	m_pParams = (CAkHarmonizerFXParams*)(in_pParams);
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
	ComputeWetPathEnabled( eChannelsMask );
	AKRESULT eResult = InitPitchVoices();
	if ( eResult != AK_Success )
		return eResult;
	eResult = InitDryDelay();
	if ( eResult != AK_Success )
		return eResult;

	m_pParams->m_ParamChangeHandler.ResetAllParamChanges();

	AK_PERF_RECORDING_RESET();

	return eResult;
}

void CAkHarmonizerFX::ComputeNumProcessedChannels( AkChannelMask in_eChannelMask )
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
	case AKINPUTTYPE_LEFTONLY:
		m_FXInfo.uNumProcessedChannels = AK::GetNumChannels(AK_SPEAKER_FRONT_LEFT & in_eChannelMask);
		m_FXInfo.eProcessChannelMask = AK_SPEAKER_FRONT_LEFT;
		break;
	}

	if ( m_FXInfo.Params.bProcessLFE && (in_eChannelMask & AK_SPEAKER_LOW_FREQUENCY) )
	{
		m_FXInfo.uNumProcessedChannels++;
		m_FXInfo.eProcessChannelMask |= AK_SPEAKER_LOW_FREQUENCY;
	}
}

void CAkHarmonizerFX::ComputeWetPathEnabled( AkChannelMask in_eChannelMask )
{
	bool bHasOneVoiceEnabled = false;
	for (AkUInt32 i = 0; i < AKHARMONIZER_NUMVOICES; i++ )
	{	
		if ( m_FXInfo.Params.Voice[i].bEnable )
			bHasOneVoiceEnabled = true;
	}
	m_FXInfo.bWetPathEnabled = bHasOneVoiceEnabled && ((in_eChannelMask & m_FXInfo.eProcessChannelMask) != 0);
}

AKRESULT CAkHarmonizerFX::InitPitchVoices()
{
	for (AkUInt32 i = 0; i < AKHARMONIZER_NUMVOICES; i++ )
	{
		if ( m_FXInfo.Params.Voice[i].bEnable )
		{	
			AKRESULT eResult = m_FXInfo.PhaseVocoder[i].Init( 
				m_pAllocator, 
				m_FXInfo.uNumProcessedChannels,
				m_FXInfo.uSampleRate,
				m_FXInfo.Params.uWindowSize,
				false );	
			if ( eResult != AK_Success )
				return eResult;

			if ( m_FXInfo.Params.Voice[i].Filter.eFilterType != AKFILTERTYPE_NONE )
			{
				m_FXInfo.Filter[i].ComputeCoefs(	
					(AK::DSP::MultiChannelBiquadFilter<AK_VOICE_MAX_NUM_CHANNELS>::FilterType)(m_FXInfo.Params.Voice[i].Filter.eFilterType-1),
					m_FXInfo.uSampleRate,
					m_FXInfo.Params.Voice[i].Filter.fFilterFrequency,
					m_FXInfo.Params.Voice[i].Filter.fFilterGain,
					m_FXInfo.Params.Voice[i].Filter.fFilterQFactor );
			}
		}
	}

	return AK_Success;
}

void CAkHarmonizerFX::TermPitchVoices()
{
	for (AkUInt32 i = 0; i < AKHARMONIZER_NUMVOICES; i++ )
	{
		m_FXInfo.PhaseVocoder[i].Term( m_pAllocator );
	}
}

void CAkHarmonizerFX::ResetPitchVoices()
{
	for ( AkUInt32 i = 0; i < AKHARMONIZER_NUMVOICES; i++ )
	{
		if ( m_FXInfo.Params.Voice[i].bEnable )
		{	
			m_FXInfo.PhaseVocoder[i].Reset();
			m_FXInfo.PhaseVocoder[i].ResetInputFill();
			m_FXInfo.Filter[i].Reset();
		}
	}
}

AKRESULT CAkHarmonizerFX::InitDryDelay()
{
	const AkUInt32 uDryDelay = m_FXInfo.Params.uWindowSize;
	if ( m_FXInfo.Params.bSyncDry && m_FXInfo.bWetPathEnabled )
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

void CAkHarmonizerFX::TermDryDelay()
{
	for ( AkUInt32 j = 0; j < m_FXInfo.uTotalNumChannels; j++ )
	{
		m_FXInfo.DryDelay[j].Term( m_pAllocator );
	}
}

void CAkHarmonizerFX::ResetDryDelay()
{
	for ( AkUInt32 j = 0; j < m_FXInfo.uTotalNumChannels; j++ )
		m_FXInfo.DryDelay[j].Reset( );
}

// Terminates.
AKRESULT CAkHarmonizerFX::Term( AK::IAkPluginMemAlloc * in_pAllocator )
{
	TermPitchVoices();
	TermDryDelay();
	AK_PLUGIN_DELETE( in_pAllocator, this );
	return AK_Success;
}

// Reset or seek to start.
AKRESULT CAkHarmonizerFX::Reset( )
{
	ResetPitchVoices();
	ResetDryDelay();
	return AK_Success;
}


// Effect info query.
AKRESULT CAkHarmonizerFX::GetPluginInfo( AkPluginInfo & out_rPluginInfo )
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

void CAkHarmonizerFX::Execute(	AkAudioBuffer * io_pBuffer			
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
		// These values cannot support RTPC because this could invalidate the constant global data flow requirement during the changes
		// These losses in input buffering could never be compensated and this would in turn lead to starvation
		// of the algorithm, which is handled through... infinite looping. Consider using an out-of-place node
		// if RTPC on pitch is required.
		if ( m_pParams->m_ParamChangeHandler.HasChanged( AKHARMONIZERPARAMID_VOICE1PITCH ) ||
			 m_pParams->m_ParamChangeHandler.HasChanged( AKHARMONIZERPARAMID_VOICE2PITCH ) )
		{
			TermPitchVoices();
			AKRESULT eResult = InitPitchVoices(); 
			if ( eResult != AK_Success )
				return; // passthrough
			ResetPitchVoices();
		}

		if ( m_pParams->m_ParamChangeHandler.HasChanged( AKHARMONIZERPARAMID_WINDOWSIZE ) ||
			 m_pParams->m_ParamChangeHandler.HasChanged( AKHARMONIZERPARAMID_VOICE1ENABLE ) ||
			 m_pParams->m_ParamChangeHandler.HasChanged( AKHARMONIZERPARAMID_VOICE2ENABLE ) ||
			 m_pParams->m_ParamChangeHandler.HasChanged( AKHARMONIZERPARAMID_INPUT ) ||
			 m_pParams->m_ParamChangeHandler.HasChanged( AKHARMONIZERPARAMID_PROCESSLFE ) )
		{
			TermPitchVoices();
			TermDryDelay();
			ComputeNumProcessedChannels( io_pBuffer->GetChannelMask() );
			ComputeWetPathEnabled( io_pBuffer->GetChannelMask() );
			AKRESULT eResult = InitPitchVoices();
			if ( eResult != AK_Success )
				return; // passthrough
			eResult = InitDryDelay();
			if ( eResult != AK_Success )
				return; // passthrough
			ResetPitchVoices();
			ResetDryDelay();
		}

		if ( m_pParams->m_ParamChangeHandler.HasChanged( AKHARMONIZERPARAMID_SYNCDRY ) )
		{
			TermDryDelay( );
			AKRESULT eResult = InitDryDelay( ); 
			if ( eResult != AK_Success )
				return;
			ResetDryDelay();
		}

		if ( m_pParams->m_ParamChangeHandler.HasChanged( AKHARMONIZERPARAMID_VOICE1FILTERTYPE ) ||
			 m_pParams->m_ParamChangeHandler.HasChanged( AKHARMONIZERPARAMID_VOICE1FILTERFREQUENCY ) ||
			 m_pParams->m_ParamChangeHandler.HasChanged( AKHARMONIZERPARAMID_VOICE1FILTERGAIN ) ||
			 m_pParams->m_ParamChangeHandler.HasChanged( AKHARMONIZERPARAMID_VOICE1FILTERQFACTOR ) )
		{
			if ( m_FXInfo.Params.Voice[0].Filter.eFilterType != AKFILTERTYPE_NONE )
			{
				m_FXInfo.Filter[0].ComputeCoefs(	
					(AK::DSP::MultiChannelBiquadFilter<AK_VOICE_MAX_NUM_CHANNELS>::FilterType)(m_FXInfo.Params.Voice[0].Filter.eFilterType-1),
					m_FXInfo.uSampleRate,
					m_FXInfo.Params.Voice[0].Filter.fFilterFrequency,
					m_FXInfo.Params.Voice[0].Filter.fFilterGain,
					m_FXInfo.Params.Voice[0].Filter.fFilterQFactor );
			}
		}

		if ( m_pParams->m_ParamChangeHandler.HasChanged( AKHARMONIZERPARAMID_VOICE2FILTERTYPE ) ||
			 m_pParams->m_ParamChangeHandler.HasChanged( AKHARMONIZERPARAMID_VOICE2FILTERFREQUENCY ) ||
			 m_pParams->m_ParamChangeHandler.HasChanged( AKHARMONIZERPARAMID_VOICE2FILTERGAIN ) ||
			 m_pParams->m_ParamChangeHandler.HasChanged( AKHARMONIZERPARAMID_VOICE2FILTERQFACTOR ) )
		{
			if ( m_FXInfo.Params.Voice[1].Filter.eFilterType != AKFILTERTYPE_NONE )
			{
				m_FXInfo.Filter[1].ComputeCoefs(	
					(AK::DSP::MultiChannelBiquadFilter<AK_VOICE_MAX_NUM_CHANNELS>::FilterType)(m_FXInfo.Params.Voice[1].Filter.eFilterType-1),
					m_FXInfo.uSampleRate,
					m_FXInfo.Params.Voice[1].Filter.fFilterFrequency,
					m_FXInfo.Params.Voice[1].Filter.fFilterGain,
					m_FXInfo.Params.Voice[1].Filter.fFilterQFactor );
			}
		}
	}
	m_pParams->m_ParamChangeHandler.ResetAllParamChanges();

	// Temporary storage: Pitch out buffer + wet channel accumulator buffer 
	AkUInt32 uVoiceMgmtBufferSize = 2 * io_pBuffer->MaxFrames() * sizeof(AkReal32);
	// Temp storage for phase vocoder
	AkUInt32 uPhaseVocoderTempBufferSize = 0;
	// All voices have same requirement, processed sequentially in same memory
	for ( AkUInt32 i = 0; i < AKHARMONIZER_NUMVOICES; i++ )
	{
		if ( m_FXInfo.Params.Voice[i].bEnable )
			uPhaseVocoderTempBufferSize = m_FXInfo.PhaseVocoder[i].GetTDStorageSize();
	}

#ifdef __PPU__
	// Prepare for SPU job and reserve scratch memory size
	m_DspProcess.ResetDspProcess( true );
	m_DspProcess.SetDspProcess( AkHarmonizerFXJobBin );
	// All voices have same requirement, processed sequentially in same memory
	if ( m_FXInfo.bWetPathEnabled )
	{
		// Streaming storage for sync delay requires an extra buffer
		AkUInt32 uSyncDelayStreamSize = m_FXInfo.Params.bSyncDry ? (io_pBuffer->MaxFrames()*sizeof(AkReal32)) : 0;
		// All voices share the same scratch size but they are not necessarily activated
		AkUInt32 uPhaseVocoderScratchSize = 0;
		for ( AkUInt32 i = 0; i < AKHARMONIZER_NUMVOICES; i++ )
		{
			if ( m_FXInfo.Params.Voice[i].bEnable )
				uPhaseVocoderScratchSize = m_FXInfo.PhaseVocoder[i].GetScratchMemRequirement();
		}
		AkUInt32 uTotalScratch = uVoiceMgmtBufferSize + uPhaseVocoderTempBufferSize + uSyncDelayStreamSize + uPhaseVocoderScratchSize;
		m_DspProcess.SetScratchSize( uTotalScratch ); 
		m_DspProcess.SetStackSize( 12*1024 ); // Max stack just above 8k
	}

	// Schedule DMA transfers
	m_DspProcess.AddDspProcessSmallDma( io_pBuffer, sizeof(AkAudioBuffer) );
	m_DspProcess.AddDspProcessSmallDma( &m_FXInfo, sizeof( m_FXInfo ) );
	m_DspProcess.AddDspProcessDma( io_pBuffer->GetDataStartDMA(), sizeof(AkReal32) * io_pBuffer->MaxFrames() * io_pBuffer->NumChannels() );

	out_pDspProcess = &m_DspProcess;
#else

	AK_PERF_RECORDING_START( "Harmonizer", 25, 100 );

	// Temporary storage: Voice mgmt + Phase vocoder time domain window
	AkUInt32 uTotalTempStorageSize = uVoiceMgmtBufferSize + uPhaseVocoderTempBufferSize;
	AkReal32 * pfTempStorage  = (AkReal32*) AK_PLUGIN_ALLOC( m_pAllocator, uTotalTempStorageSize );
	if ( !pfTempStorage )
		return; // Pipeline cannot handle errors here, just play dry signal when out of memory

	AkHarmonizerDSPProcess( io_pBuffer, m_FXInfo, pfTempStorage );

	if ( pfTempStorage )
		AK_PLUGIN_FREE( m_pAllocator, pfTempStorage );
	
	AK_PERF_RECORDING_STOP( "Harmonizer", 25, 100 );

#endif // __PPU__
}

