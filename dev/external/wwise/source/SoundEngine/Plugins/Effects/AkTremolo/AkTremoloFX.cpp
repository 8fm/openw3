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

#include <AK/Tools/Common/AkAssert.h>
///#include <AK/Tools/Common/AkObject.h> // Placement new
#include "AkTremoloFX.h"
#include "LFO.h"

#ifdef __PPU__
// embedded SPU Job Binary symbols
extern char _binary_AkTremoloFXJob_spu_bin_start[];
extern char _binary_AkTremoloFXJob_spu_bin_size[];
static AK::MultiCoreServices::BinData AkTremoloFXJobBin = { _binary_AkTremoloFXJob_spu_bin_start, CELL_SPURS_GET_SIZE_BINARY( _binary_AkTremoloFXJob_spu_bin_size ) };
#endif

// Plugin mechanism. Dynamic create function whose address must be registered to the FX manager.
AK::IAkPlugin* CreateTremoloFX( AK::IAkPluginMemAlloc * in_pAllocator )
{
	return AK_PLUGIN_NEW( in_pAllocator, CAkTremoloFX() );
}

// Constructor.
CAkTremoloFX::CAkTremoloFX() 
: m_pSharedParams( 0 )
, m_pAllocator( 0 )
{
	AKPLATFORM::AkMemSet( &m_FXInfo.Params, 0, sizeof(m_FXInfo.Params) );
	AKPLATFORM::AkMemSet( &m_FXInfo.PrevParams, 0, sizeof(m_FXInfo.PrevParams) );
}

// Destructor.
CAkTremoloFX::~CAkTremoloFX()
{
	
}

// Initializes and allocate memory for the effect
AKRESULT CAkTremoloFX::Init(	AK::IAkPluginMemAlloc * in_pAllocator,		// Memory allocator interface.
								AK::IAkEffectPluginContext * in_pFXCtx,		// FX Context
								AK::IAkPluginParam * in_pParams,			// Effect parameters.
								AkAudioFormat &	in_rFormat					// Required audio input format.
							   )
{	
	// get parameter state
	m_pSharedParams = static_cast<CAkTremoloFXParams*>(in_pParams);
	m_pSharedParams->GetParams( &m_FXInfo.Params );
	m_FXInfo.PrevParams = m_FXInfo.Params;
	
	m_FXInfo.uSampleRate = in_rFormat.uSampleRate;
	m_pAllocator = in_pAllocator;

	SetupLFO( in_rFormat.GetChannelMask() );

	AK_PERF_RECORDING_RESET();

	return AK_Success;
}

void CAkTremoloFX::SetupLFO( AkChannelMask in_uChannelMask )
{
	// Exclude Center channel if applicable.
	if ( !m_FXInfo.Params.NonRTPC.bProcessCenter && ((in_uChannelMask & AK_SPEAKER_SETUP_3_0) == AK_SPEAKER_SETUP_3_0) )
		in_uChannelMask &= ~AK_SPEAKER_FRONT_CENTER;

	// Exclude LFE if applicable.
	if ( !m_FXInfo.Params.NonRTPC.bProcessLFE )
		in_uChannelMask &= ~AK_SPEAKER_LOW_FREQUENCY;

	m_lfo.Setup( 
		in_uChannelMask, 
		m_FXInfo.uSampleRate,
		m_FXInfo.Params.RTPC.modParams );
}

AKRESULT CAkTremoloFX::Term( AK::IAkPluginMemAlloc * in_pAllocator )
{
	AK_PLUGIN_DELETE( in_pAllocator, this );
	return AK_Success;
}


// Effect info query.
AKRESULT CAkTremoloFX::GetPluginInfo( AkPluginInfo & out_rPluginInfo )
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

// parameter updates from wwise
void CAkTremoloFX::LiveParametersUpdate( AkAudioBuffer* io_pBuffer )
{
	// Handle live changes
	if ( (m_FXInfo.PrevParams.NonRTPC.bProcessLFE != m_FXInfo.Params.NonRTPC.bProcessLFE) || (m_FXInfo.PrevParams.NonRTPC.bProcessCenter != m_FXInfo.Params.NonRTPC.bProcessCenter) )
	{
		AkChannelMask uChannelMask = io_pBuffer->GetChannelMask();
		SetupLFO( uChannelMask );
	}
}

// parameter updates from the game engine
void CAkTremoloFX::RTPCParametersUpdate()
{
	m_lfo.SetParams(
		m_FXInfo.uSampleRate,
		m_FXInfo.Params.RTPC.modParams.lfoParams
		);
}


// main processing routine
void CAkTremoloFX::Execute( AkAudioBuffer * io_pBuffer
#ifdef AK_PS3
							, AK::MultiCoreServices::DspProcess*&	out_pDspProcess
#endif						
								)
{
	if ( m_lfo.GetNumChannels() == 0 )
		return;

	m_pSharedParams->GetParams( &m_FXInfo.Params );

	// Support live change of non-RTPC parameters when connected to Wwise
	if(m_FXInfo.Params.NonRTPC.bHasChanged)
		LiveParametersUpdate( io_pBuffer );

	if(m_FXInfo.Params.RTPC.bHasChanged)
	{
		// RTPC parameter updates
		RTPCParametersUpdate();
	}

#ifdef __PPU__
	// Prepare for SPU job and reserve scratch memory size
	m_DspProcess.ResetDspProcess( true );
	m_DspProcess.SetDspProcess( AkTremoloFXJobBin );
	

	// Schedule DMA transfers
	m_DspProcess.AddDspProcessSmallDma( io_pBuffer, sizeof(AkAudioBuffer) );
	m_DspProcess.AddDspProcessSmallDma( &m_FXInfo, sizeof(m_FXInfo) );
	m_DspProcess.AddDspProcessSmallDma( &m_lfo, sizeof(TremoloLFO) );
	
	// Note: Do not send the LFE channel if !bProcessLFE
	AkChannelMask uChannelMask = io_pBuffer->GetChannelMask();
	if ( !m_FXInfo.Params.NonRTPC.bProcessLFE )
		uChannelMask &= ~AK_SPEAKER_LOW_FREQUENCY;
	
	m_DspProcess.AddDspProcessDma( io_pBuffer->GetDataStartDMA(), sizeof(AkReal32) * io_pBuffer->MaxFrames() * AK::GetNumChannels( uChannelMask ) );

	out_pDspProcess = &m_DspProcess;
#else

	AK_PERF_RECORDING_START( "Tremolo", 25, 30 );
	
	AkChannelMask uChannelMask = io_pBuffer->GetChannelMask();

	// Skip LFE if required.
	if ( !m_FXInfo.Params.NonRTPC.bProcessLFE )
		uChannelMask &= ~AK_SPEAKER_LOW_FREQUENCY;

	// Skip center channel if required.
	bool bSkipCenterChannel = ( !m_FXInfo.Params.NonRTPC.bProcessCenter && ((uChannelMask & AK_SPEAKER_SETUP_3_0) == AK_SPEAKER_SETUP_3_0) );

	// Prepare variables for ramp.
	const AkUInt32 uNumFrames = io_pBuffer->uValidFrames;

	const AkReal32 fDepth = m_FXInfo.Params.RTPC.fModDepth;
	const AkReal32 fPrevDepth = m_FXInfo.PrevParams.RTPC.fModDepth;

	const AkReal32 fOutputGain = m_FXInfo.Params.RTPC.fOutputGain;
	const AkReal32 fPrevOutputGain = m_FXInfo.PrevParams.RTPC.fOutputGain;
	const AkReal32 fOutInc = ( fOutputGain - fPrevOutputGain ) / uNumFrames;
	const AkReal32 fPWM = m_FXInfo.Params.RTPC.modParams.lfoParams.fPWM;

	AkUInt32 uNumChannels = AK::GetNumChannels( uChannelMask );
	for(AkUInt32 i = 0; i < uNumChannels; ++i)
	{
		// Skip center channel.
		if ( bSkipCenterChannel && i == AK_IDX_SETUP_3_CENTER )
			continue;

		TremoloOutputPolicy oOutputParams;
		oOutputParams.fGain = fOutputGain;
		oOutputParams.fGainInc = fOutInc;

		m_lfo.GetChannel( i ).ProduceBuffer(io_pBuffer->GetChannel(i), uNumFrames, fDepth, fPrevDepth, fPWM, oOutputParams);
	}

	m_FXInfo.PrevParams = m_FXInfo.Params;

	AK_PERF_RECORDING_STOP( "Tremolo", 25, 30 );
	
#endif
}

AKRESULT CAkTremoloFX::Reset()
{
	return AK_Success;
}
