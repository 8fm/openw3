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
#include <AK/Tools/Common/AkObject.h> // Placement new
#include "AkFlangerFX.h"
#include "Mix2Interp.h"
#include "UniComb.h"
#include "LFO.h"

#ifdef __PPU__
// embedded SPU Job Binary symbols
extern char _binary_AkFlangerFXJob_spu_bin_start[];
extern char _binary_AkFlangerFXJob_spu_bin_size[];
static AK::MultiCoreServices::BinData AkFlangerFXJobBin = { _binary_AkFlangerFXJob_spu_bin_start, CELL_SPURS_GET_SIZE_BINARY( _binary_AkFlangerFXJob_spu_bin_size ) };
#endif

// Plugin mechanism. Dynamic create function whose address must be registered to the FX manager.
AK::IAkPlugin* CreateFlangerFX( AK::IAkPluginMemAlloc * in_pAllocator )
{
	return AK_PLUGIN_NEW( in_pAllocator, CAkFlangerFX( ) );
}

// Constructor.
CAkFlangerFX::CAkFlangerFX() 
: m_pUniCombs(0)
, m_pLFO(0)
, m_pSharedParams(0)
, m_pAllocator(0)
, m_pFXCtx(0)
{
	AKPLATFORM::AkMemSet( &m_FXInfo.Params, 0, sizeof(m_FXInfo.Params) );
	AKPLATFORM::AkMemSet( &m_FXInfo.PrevParams, 0, sizeof(m_FXInfo.PrevParams) );
}

// Destructor.
CAkFlangerFX::~CAkFlangerFX()
{
	
}


#define RETURNIFNOTSUCCESS( __FONCTIONEVAL__ )	\
{												\
	AKRESULT __result__ = (__FONCTIONEVAL__);	\
	if ( __result__ != AK_Success )				\
		return __result__;						\
}												\

// Initializes and allocate memory for the effect
AKRESULT CAkFlangerFX::Init(	AK::IAkPluginMemAlloc * in_pAllocator,		// Memory allocator interface.
								AK::IAkEffectPluginContext * in_pFXCtx,		// FX Context
								AK::IAkPluginParam * in_pParams,			// Effect parameters.
								AkAudioFormat &	in_rFormat					// Required audio input format.
							   )
{	
	// get parameter state
	m_pSharedParams = static_cast<CAkFlangerFXParams*>(in_pParams);
	m_pSharedParams->GetParams( &m_FXInfo.Params );
	if ( !m_FXInfo.Params.NonRTPC.bEnableLFO )
		m_FXInfo.Params.RTPC.fModDepth = 0.f;
	m_FXInfo.PrevParams = m_FXInfo.Params;
	m_pFXCtx = in_pFXCtx;

	// LFE and true center (not mono) may or may not be processed.
	AkChannelMask uChannelMask = AdjustEffectiveChannelMask( in_rFormat.GetChannelMask() );
	m_FXInfo.uNumProcessedChannels = AK::GetNumChannels( uChannelMask );
	m_FXInfo.uSampleRate = in_rFormat.uSampleRate;
	m_pAllocator = in_pAllocator;

	RETURNIFNOTSUCCESS( InitUniCombs( uChannelMask ) );
	RETURNIFNOTSUCCESS( InitLFO( uChannelMask ) );


	AK_PERF_RECORDING_RESET();

	// all set. RETURNIFNOTSUCCESS would have earlied us out if we didn't get here.
	return AK_Success;
}

AkChannelMask CAkFlangerFX::AdjustEffectiveChannelMask( AkChannelMask in_uChannelMask )
{
	if ( !m_FXInfo.Params.NonRTPC.bProcessLFE )
		in_uChannelMask &= ~AK_SPEAKER_LOW_FREQUENCY;

	if ( ((in_uChannelMask & AK_SPEAKER_SETUP_3_0) == AK_SPEAKER_SETUP_3_0) && !m_FXInfo.Params.NonRTPC.bProcessCenter )
		in_uChannelMask &= ~AK_SPEAKER_FRONT_CENTER;

	return in_uChannelMask;
}

AKRESULT CAkFlangerFX::InitUniCombs( AkChannelMask in_uChannelMask )
{
	const AkUInt32 uNumProcessedChannels = AK::GetNumChannels( in_uChannelMask );
	if ( uNumProcessedChannels )
	{
		// allocate per-channel unicombs
		m_pUniCombs = (DSP::UniComb *) AK_PLUGIN_ALLOC( m_pAllocator,  AK_ALIGN_SIZE_FOR_DMA( sizeof(DSP::UniComb) * uNumProcessedChannels ) );
		if ( !m_pUniCombs )
			return AK_InsufficientMemory;
		for(AkUInt32 i = 0; i < uNumProcessedChannels; ++i)
		{
			AkPlacementNew( &m_pUniCombs[i] ) DSP::UniComb() ;
		}

		// compute delay line length from delay time (ms)
		AkUInt32 uDelayLength = (AkUInt32)( m_FXInfo.Params.NonRTPC.fDelayTime / 1000.f * m_FXInfo.uSampleRate );

		// initialize unicombs for each processed channel
		for(AkUInt32 i = 0; i < uNumProcessedChannels; ++i)
		{
			RETURNIFNOTSUCCESS( m_pUniCombs[i].Init(	
				m_pAllocator, 
				uDelayLength,
				m_pFXCtx->GetMaxBufferLength(),
				m_FXInfo.Params.RTPC.fFbackLevel,
				m_FXInfo.Params.RTPC.fFfwdLevel,
				m_FXInfo.Params.RTPC.fDryLevel,
				m_FXInfo.Params.RTPC.fModDepth) );
		}
	}

	return AK_Success;
}

AKRESULT CAkFlangerFX::InitLFO( AkChannelMask in_uChannelMask )
{
	if ( m_FXInfo.Params.NonRTPC.bEnableLFO && AK::GetNumChannels( in_uChannelMask ) )
	{
		// allocate lfo module
		m_pLFO = (FlangerLFO *) AK_PLUGIN_NEW( m_pAllocator, FlangerLFO() );
		if ( !m_pLFO )
			return AK_InsufficientMemory;

		m_pLFO->Setup( 
			in_uChannelMask,
			m_FXInfo.uSampleRate,
			m_FXInfo.Params.RTPC.modParams
			);
	}

	return AK_Success;
}

AKRESULT CAkFlangerFX::Term( AK::IAkPluginMemAlloc * in_pAllocator )
{
	TermUniCombs();
	TermLFO();

	AK_PLUGIN_DELETE( in_pAllocator, this );
	return AK_Success;
}

void CAkFlangerFX::TermUniCombs( )
{
	if ( m_pUniCombs )
	{
		for(AkUInt32 i = 0; i < m_FXInfo.uNumProcessedChannels; ++i)
			m_pUniCombs[i].Term( m_pAllocator );

		AK_PLUGIN_FREE( m_pAllocator, m_pUniCombs ); // DSP::Unicomb::~Unicomb is trivial.
		m_pUniCombs = NULL;
	}
}

void CAkFlangerFX::TermLFO( )
{
	if ( m_pLFO )
	{
		AK_PLUGIN_DELETE( m_pAllocator, m_pLFO );
		m_pLFO = NULL;
	}
}

// Effect info query.
AKRESULT CAkFlangerFX::GetPluginInfo( AkPluginInfo & out_rPluginInfo )
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
AKRESULT CAkFlangerFX::LiveParametersUpdate( AkAudioBuffer* io_pBuffer )
{
	AkChannelMask uChannelMask = AdjustEffectiveChannelMask( io_pBuffer->GetChannelMask() );
	AkUInt32 uNewNumChannels = AK::GetNumChannels( uChannelMask );
	bool bNumProcessChannelsChanged = uNewNumChannels != m_FXInfo.uNumProcessedChannels;
	if ( (m_FXInfo.PrevParams.NonRTPC.bEnableLFO != m_FXInfo.Params.NonRTPC.bEnableLFO) || bNumProcessChannelsChanged )
	{
		TermLFO();
		AKRESULT eResult = InitLFO( uChannelMask );
		if ( eResult != AK_Success )
			return eResult;
	}

	if ( (m_FXInfo.PrevParams.NonRTPC.fDelayTime != m_FXInfo.Params.NonRTPC.fDelayTime) || bNumProcessChannelsChanged )
	{
		TermUniCombs();
		AKRESULT eResult = InitUniCombs( uChannelMask );
		if ( eResult != AK_Success )
			return eResult;
		ResetUniCombs( uNewNumChannels );
	}

	m_FXInfo.uNumProcessedChannels = uNewNumChannels;

	return AK_Success; // No errors
}

// parameter updates from the game engine
void CAkFlangerFX::RTPCParametersUpdate( )
{
	for(AkUInt32 i = 0; i < m_FXInfo.uNumProcessedChannels; ++i)
	{
		m_pUniCombs[i].SetParams(	m_FXInfo.Params.RTPC.fFbackLevel,
									m_FXInfo.Params.RTPC.fFfwdLevel,
									m_FXInfo.Params.RTPC.fDryLevel,
									m_FXInfo.Params.RTPC.fModDepth );
	}

	if ( m_pLFO && m_FXInfo.Params.NonRTPC.bEnableLFO )
	{
		m_pLFO->SetParams( 
			m_FXInfo.uSampleRate,
			m_FXInfo.Params.RTPC.modParams.lfoParams
			);
	}
}

// main processing routine
void CAkFlangerFX::Execute( AkAudioBuffer * io_pBuffer
#ifdef AK_PS3
							, AK::MultiCoreServices::DspProcess*&	out_pDspProcess
#endif						
								)
{
	m_pSharedParams->GetParams( &m_FXInfo.Params );
	if ( !m_FXInfo.Params.NonRTPC.bEnableLFO )
		m_FXInfo.Params.RTPC.fModDepth = 0.f;

	// Support live change of non-RTPC parameters when connected to Wwise
	if(m_FXInfo.Params.NonRTPC.bHasChanged)
	{
		AKRESULT bError = LiveParametersUpdate( io_pBuffer );
		if ( bError != AK_Success )
			return;
		m_FXInfo.Params.NonRTPC.bHasChanged = false;
	}

	if(m_FXInfo.Params.RTPC.bHasChanged)
	{
		// RTPC parameter updates
		RTPCParametersUpdate( );
		m_FXInfo.Params.RTPC.bHasChanged = false;
	}

	if ( m_FXInfo.uNumProcessedChannels == 0 )
		return;

#ifdef __PPU__
	// Prepare for SPU job and reserve scratch memory size
	m_DspProcess.ResetDspProcess( true );
	// Necessary space to stream in current UniComb delay memory and to process a LFO buffer on SPU
	const AkUInt32 uMaxFrames = io_pBuffer->MaxFrames();
	AkUInt32  uScratchBytes = uMaxFrames * sizeof(AkReal32); // Dry storage
	if ( m_FXInfo.Params.NonRTPC.bEnableLFO )
		uScratchBytes += uMaxFrames * sizeof(AkReal32); // LFO buffer
	uScratchBytes += m_pUniCombs[0].GetScratchSize(  );
	m_DspProcess.SetScratchSize( uScratchBytes );
	m_DspProcess.SetDspProcess( AkFlangerFXJobBin );

	// Schedule DMA transfers
	m_DspProcess.AddDspProcessSmallDma( io_pBuffer, sizeof(AkAudioBuffer) );
	m_DspProcess.AddDspProcessSmallDma( &m_FXInfo, sizeof( m_FXInfo ) );
	m_DspProcess.AddDspProcessSmallDma( m_pUniCombs, AK_ALIGN_SIZE_FOR_DMA( sizeof(DSP::UniComb) * m_FXInfo.uNumProcessedChannels ) );

	if ( m_pLFO )
		m_DspProcess.AddDspProcessSmallDma( m_pLFO, sizeof(FlangerLFO) );

	m_DspProcess.AddDspProcessDma( io_pBuffer->GetDataStartDMA(), sizeof(AkReal32) * io_pBuffer->MaxFrames() * io_pBuffer->NumChannels() );

	out_pDspProcess = &m_DspProcess;
#else

	AK_PERF_RECORDING_START( "Flanger", 25, 30 );

	AkUInt32 uDelayLength = (AkUInt32)( m_FXInfo.Params.NonRTPC.fDelayTime / 1000.f * m_FXInfo.uSampleRate );
	m_FXInfo.FXTailHandler.HandleTail( io_pBuffer, uDelayLength );

	if ( io_pBuffer->uValidFrames == 0 )
		return;

	const AkUInt32 uNumFrames = io_pBuffer->uValidFrames;
	AkUInt32 uChanIndex = 0;

	AkChannelMask uChannelMask = io_pBuffer->GetChannelMask();
	if ( !m_FXInfo.Params.NonRTPC.bProcessLFE )
		uChannelMask &= ~AK_SPEAKER_LOW_FREQUENCY;
	AkUInt32 uNumChannels = AK::GetNumChannels(uChannelMask);

	// Skip center channel if required.
	bool bSkipCenterChannel = ( !m_FXInfo.Params.NonRTPC.bProcessCenter && ((uChannelMask & AK_SPEAKER_SETUP_3_0) == AK_SPEAKER_SETUP_3_0) );
		
	AkReal32 * pfDryBuf = (AkReal32*)AK_PLUGIN_ALLOC( m_pAllocator,  sizeof(AkReal32) * io_pBuffer->MaxFrames() );
	if ( pfDryBuf == NULL )
		return;

	const AkReal32 fCurrentDryGain = (100.f-m_FXInfo.PrevParams.RTPC.fWetDryMix)*0.01f;
	const AkReal32 fTargetDryGain = (100.f-m_FXInfo.Params.RTPC.fWetDryMix)*0.01f;
	const AkReal32 fCurrentWetGain = 1.f-fCurrentDryGain;
	const AkReal32 fTargetWetGain = 1.f-fTargetDryGain;

	if ( m_FXInfo.Params.NonRTPC.bEnableLFO )
	{
		// With LFO
		AkReal32 * pfLFOBuf = (AkReal32*)AK_PLUGIN_ALLOC( m_pAllocator,  sizeof(AkReal32) * uNumFrames );
		// Note: If allocation failed, proceed, but just avoid calling LFO::ProduceBuffer().
		
		const AkReal32 fPWM = m_FXInfo.Params.RTPC.modParams.lfoParams.fPWM;
		const AkReal32 fLFODepth = m_FXInfo.Params.RTPC.fModDepth;
		const AkReal32 fPrevLFODepth = m_FXInfo.PrevParams.RTPC.fModDepth;

		FlangerOutputPolicy dummy;	//Empty obj.

		for(AkUInt32 i = 0; i < uNumChannels; ++i)
		{
			// Skip over (true) center channel if present and not being processed
			if ( bSkipCenterChannel && i == AK_IDX_SETUP_3_CENTER )
				continue;

			// Produce a buffer of LFO data.
			if ( pfLFOBuf )
			{
				m_pLFO->GetChannel(uChanIndex).ProduceBuffer( 
					pfLFOBuf, 
					uNumFrames, 
					fLFODepth, 
					fPrevLFODepth,
					fPWM,
					dummy);
			}

			AkReal32 * pfChan = io_pBuffer->GetChannel(i);

			// Copy dry path
			AKPLATFORM::AkMemCpy(pfDryBuf, pfChan, uNumFrames*sizeof(AkReal32) ); 

			// Process unicomb, passing it the buffer with LFO data.
			m_pUniCombs[uChanIndex].ProcessBuffer(pfChan, uNumFrames, pfLFOBuf);
			uChanIndex++;

			// Wet/dry mix
			DSP::Mix2Interp(	
				pfChan, pfDryBuf, 
				fCurrentWetGain*m_FXInfo.PrevParams.RTPC.fOutputLevel, fTargetWetGain*m_FXInfo.Params.RTPC.fOutputLevel, 
				fCurrentDryGain*m_FXInfo.PrevParams.RTPC.fOutputLevel, fTargetDryGain*m_FXInfo.Params.RTPC.fOutputLevel, 
				uNumFrames );

///#define TEST_LFO
#ifdef TEST_LFO
			memcpy( pfChan, pfLFOBuf, uNumFrames * sizeof( AkReal32 ) );
#endif
		}

		if ( pfLFOBuf )
		{
			AK_PLUGIN_FREE( m_pAllocator, pfLFOBuf );
			pfLFOBuf = NULL;
		}
	}
	else
	{
		// No LFO (static filtering)
		for(AkUInt32 i = 0; i < uNumChannels; ++i)
		{
			// Skip over (true) center channel if present and not being processed
			if ( bSkipCenterChannel && i == AK_IDX_SETUP_3_CENTER )
				continue;

			AkReal32 * pfChan = io_pBuffer->GetChannel(i);

			// Copy dry path
			AKPLATFORM::AkMemCpy(pfDryBuf, pfChan, uNumFrames*sizeof(AkReal32) ); 

			m_pUniCombs[uChanIndex].ProcessBuffer(pfChan, uNumFrames, NULL);
			uChanIndex++;

			// Wet/dry mix
			DSP::Mix2Interp(	
				pfChan, pfDryBuf, 
				fCurrentWetGain*m_FXInfo.PrevParams.RTPC.fOutputLevel, fTargetWetGain*m_FXInfo.Params.RTPC.fOutputLevel, 
				fCurrentDryGain*m_FXInfo.PrevParams.RTPC.fOutputLevel, fTargetDryGain*m_FXInfo.Params.RTPC.fOutputLevel, 
				uNumFrames );
		}
	}

	AK_PLUGIN_FREE( m_pAllocator, pfDryBuf );
	pfDryBuf = NULL;

	m_FXInfo.PrevParams = m_FXInfo.Params;

	AK_PERF_RECORDING_STOP( "Flanger", 25, 30 );

#endif
}

AKRESULT CAkFlangerFX::Reset( )
{
	ResetUniCombs( m_FXInfo.uNumProcessedChannels );
	return AK_Success;
}

void CAkFlangerFX::ResetUniCombs( AkUInt32 in_uNumProcessedChannels )
{
	if ( m_pUniCombs )
	{
		for(AkUInt32 i = 0; i < in_uNumProcessedChannels; ++i)
			m_pUniCombs[i].Reset();
	}
}
