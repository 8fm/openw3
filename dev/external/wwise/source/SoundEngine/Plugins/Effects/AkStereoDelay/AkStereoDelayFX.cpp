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

#include "AkStereoDelayFX.h"
#include <math.h>
#include <AK/Tools/Common/AkAssert.h>
#include "AkStereoDelayDSPProcess.h"

#ifdef __PPU__
// embedded SPU Job Binary symbols
extern char _binary_AkStereoDelayFXJob_spu_bin_start[];
extern char _binary_AkStereoDelayFXJob_spu_bin_size[];
static AK::MultiCoreServices::BinData AkStereoDelayFXJobBin = { _binary_AkStereoDelayFXJob_spu_bin_start, CELL_SPURS_GET_SIZE_BINARY( _binary_AkStereoDelayFXJob_spu_bin_size ) };
#endif

// Plugin mechanism. Dynamic create function whose address must be registered to the FX manager.
AK::IAkPlugin* CreateStereoDelayFX( AK::IAkPluginMemAlloc * in_pAllocator )
{
	return AK_PLUGIN_NEW( in_pAllocator, CAkStereoDelayFX( ) );
}


// Constructor.
CAkStereoDelayFX::CAkStereoDelayFX() 
	: m_pParams(NULL)
	, m_pAllocator(NULL)
{

}

// Destructor.
CAkStereoDelayFX::~CAkStereoDelayFX()
{

}

// Initializes and allocate memory for the effect
AKRESULT CAkStereoDelayFX::Init(	AK::IAkPluginMemAlloc * in_pAllocator,	// Memory allocator interface.
									AK::IAkEffectPluginContext * in_pFXCtx,	// FX Context
									AK::IAkPluginParam * in_pParams,		// Effect parameters.
									AkAudioFormat &	in_rFormat				// Required audio input format.
									)
{
	m_pParams = static_cast<CAkStereoDelayFXParams*>(in_pParams);
	m_pAllocator = in_pAllocator;
	
#define RETURNIFNOTSUCCESS( __FONCTIONEVAL__ )	\
{												\
	AKRESULT __result__ = (__FONCTIONEVAL__);	\
	if ( __result__ != AK_Success )				\
		return __result__;						\
}												\

	m_FXInfo.uSampleRate = in_rFormat.uSampleRate;
	m_FXInfo.bSendMode = in_pFXCtx->IsSendModeEffect();

	m_FXInfo.uMaxBufferSize = in_pFXCtx->GetMaxBufferLength();

	m_pParams->GetParams( &m_FXInfo.Params );
	SanitizeParams( m_FXInfo.Params );
	m_FXInfo.PrevParams = m_FXInfo.Params;
	m_FXInfo.bRecomputeFilterCoefs = true;

	AkReal32 fDelayTimes[2] = { m_FXInfo.Params.StereoDelayParams[0].fDelayTime, m_FXInfo.Params.StereoDelayParams[1].fDelayTime };
	for ( AkUInt32 uLine = 0; uLine < AK_NUM_STEREO_DELAY_LINES; uLine++ )
	{
		RETURNIFNOTSUCCESS( m_FXInfo.StereoDelay[uLine].Init( in_pAllocator, fDelayTimes, m_FXInfo.uSampleRate ) );
	}
	m_pParams->m_ParamChangeHandler.ResetParamChange( AKSTEREODELAYPARAMID_LEFTDELAYTIME );
	m_pParams->m_ParamChangeHandler.ResetParamChange( AKSTEREODELAYPARAMID_RIGHTDELAYTIME );

	
	AK_PERF_RECORDING_RESET();

	return AK_Success;
}

void CAkStereoDelayFX::ComputeTailLength( )
{
	// This is a guestimate of the tail length based on crossfeed, feedback and delay time parameters
	static const AkReal32 DYNAMICRANGE = 60.f;		// Level (in dB) at the end of the effect tail (effect shut off after this)
	static const AkReal32 MAXFEEDBACKGAIN = -0.1f;	// Cap feedback and crossfeed (dB)
	static const AkUInt32 MAXTAILTIME = 60;			// Effect shut off after this time (in secs), regardless of feedback and crossfeed values

	AkReal32 fTailTime = 0.f;
	if ( m_FXInfo.Params.bEnableFeedback )
	{
		// Left feedback line 
		AKASSERT( m_FXInfo.Params.StereoDelayParams[0].fFeedback > 0.f );
		AkReal32 fCappedFeedback = AkMin( 20.f*log10f( m_FXInfo.Params.StereoDelayParams[0].fFeedback), MAXFEEDBACKGAIN );
		AkReal32 fNumIter = DYNAMICRANGE / -fCappedFeedback;
		AkReal32 fTailTimeFbkL = fNumIter*m_FXInfo.Params.StereoDelayParams[0].fDelayTime;
		// Right feedback line 
		AKASSERT( m_FXInfo.Params.StereoDelayParams[1].fFeedback > 0.f );
		fCappedFeedback = AkMin( 20.f*log10f(m_FXInfo.Params.StereoDelayParams[1].fFeedback), MAXFEEDBACKGAIN );
		fNumIter = DYNAMICRANGE / -fCappedFeedback;
		AkReal32 fTailTimeFbkR = fNumIter*m_FXInfo.Params.StereoDelayParams[1].fDelayTime;
		fTailTime = AkMax( fTailTimeFbkL, fTailTimeFbkR );
	}
	else
	{
		fTailTime = AkMax( m_FXInfo.Params.StereoDelayParams[0].fDelayTime,m_FXInfo.Params.StereoDelayParams[1].fDelayTime );
	}

	if ( m_FXInfo.Params.bEnableCrossFeed )
	{
		// Crossfeed feedback line
		AkReal32 fCombinedCrossFeed = m_FXInfo.Params.StereoDelayParams[0].fCrossFeed*m_FXInfo.Params.StereoDelayParams[1].fCrossFeed;
		AKASSERT( fCombinedCrossFeed > 0.f );
		AkReal32 fCappedFeedback = AkMin( 20.f*log10f(fCombinedCrossFeed), MAXFEEDBACKGAIN );
		AkReal32 fNumIter = DYNAMICRANGE / -fCappedFeedback;
		AkReal32 fCombinedFbkCfdFudge = m_FXInfo.Params.bEnableFeedback ? 2.f : 1.f;
		AkReal32 fTailTimeCfd = fCombinedFbkCfdFudge*fNumIter*(m_FXInfo.Params.StereoDelayParams[0].fDelayTime+m_FXInfo.Params.StereoDelayParams[1].fDelayTime);
		fTailTime += fTailTimeCfd;
	}

	fTailTime = AkMin( fTailTime, MAXTAILTIME );
	m_FXInfo.uTailLength = (AkUInt32)(fTailTime*m_FXInfo.uSampleRate);
}

void CAkStereoDelayFX::SanitizeParams( AkStereoDelayFXParams & io_FXParams )
{	
	// No dry signal in send mode (environmental)
	if ( m_FXInfo.bSendMode )
	{
		io_FXParams.fDryLevel = 0.f;
	}

	// Feedback disabled means no feedback
	if ( !io_FXParams.bEnableFeedback )
	{
		io_FXParams.StereoDelayParams[0].fFeedback = 0.f;
		io_FXParams.StereoDelayParams[1].fFeedback = 0.f;
	}

	// Crossfeed disabled means no crossfeed
	if ( !io_FXParams.bEnableCrossFeed )
	{
		io_FXParams.StereoDelayParams[0].fCrossFeed = 0.f;
		io_FXParams.StereoDelayParams[1].fCrossFeed = 0.f;
	}

	//The algorithm assumes the delay is not smaller than the size of the buffers.  Extra code would be needed otherwise.
	const AkReal32 c_fMinDelay = (AkReal32)m_FXInfo.uMaxBufferSize/m_FXInfo.uSampleRate;
	if (io_FXParams.StereoDelayParams[0].fDelayTime < c_fMinDelay)
		io_FXParams.StereoDelayParams[0].fDelayTime = c_fMinDelay;
	if (io_FXParams.StereoDelayParams[1].fDelayTime < c_fMinDelay)
		io_FXParams.StereoDelayParams[1].fDelayTime = c_fMinDelay;
}


// Terminates.
AKRESULT CAkStereoDelayFX::Term( AK::IAkPluginMemAlloc * in_pAllocator )
{
	for ( AkUInt32 uLine = 0; uLine < AK_NUM_STEREO_DELAY_LINES; uLine++ )
		m_FXInfo.StereoDelay[uLine].Term( in_pAllocator );
	AK_PLUGIN_DELETE( in_pAllocator, this );
	return AK_Success;
}

// Reset.
AKRESULT CAkStereoDelayFX::Reset( )
{
	for ( AkUInt32 uLine = 0; uLine < AK_NUM_STEREO_DELAY_LINES; uLine++ )
		m_FXInfo.StereoDelay[uLine].Reset();
	return AK_Success;
}


// Effect info query.
AKRESULT CAkStereoDelayFX::GetPluginInfo( AkPluginInfo & out_rPluginInfo )
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

//  Execute stereo delay effect.
void CAkStereoDelayFX::Execute( AkAudioBuffer * io_pBuffer
#ifdef AK_PS3
								, AK::MultiCoreServices::DspProcess*&	out_pDspProcess
#endif						
								)
{
	// Handle parameter changes
	m_pParams->GetParams( &m_FXInfo.Params );
	SanitizeParams( m_FXInfo.Params );

	if ( m_pParams->m_ParamChangeHandler.HasChanged( AKSTEREODELAYPARAMID_ENABLECROSSFEED ) ||
		 m_pParams->m_ParamChangeHandler.HasChanged( AKSTEREODELAYPARAMID_ENABLEFEEDBACK ) ||
		 m_pParams->m_ParamChangeHandler.HasChanged( AKSTEREODELAYPARAMID_LEFTDELAYTIME ) ||
		 m_pParams->m_ParamChangeHandler.HasChanged( AKSTEREODELAYPARAMID_LEFTFEEDBACK ) ||
		 m_pParams->m_ParamChangeHandler.HasChanged( AKSTEREODELAYPARAMID_LEFTCROSSFEED ) ||
		 m_pParams->m_ParamChangeHandler.HasChanged( AKSTEREODELAYPARAMID_RIGHTDELAYTIME ) ||
		 m_pParams->m_ParamChangeHandler.HasChanged( AKSTEREODELAYPARAMID_RIGHTFEEDBACK ) || 
		 m_pParams->m_ParamChangeHandler.HasChanged( AKSTEREODELAYPARAMID_RIGHTCROSSFEED )
		)
	{
		ComputeTailLength();
	}

	if (  m_pParams->m_ParamChangeHandler.HasChanged( AKSTEREODELAYPARAMID_RIGHTDELAYTIME ) ||
		 m_pParams->m_ParamChangeHandler.HasChanged( AKSTEREODELAYPARAMID_LEFTDELAYTIME ) )
	{
		AkReal32 fDelayTimes[2] = { m_FXInfo.Params.StereoDelayParams[0].fDelayTime, m_FXInfo.Params.StereoDelayParams[1].fDelayTime };
		for ( AkUInt32 uLine = 0; uLine < AK_NUM_STEREO_DELAY_LINES; uLine++ )
		{
			m_FXInfo.StereoDelay[uLine].Term( m_pAllocator );
			AKRESULT eResult = m_FXInfo.StereoDelay[uLine].Init( m_pAllocator, fDelayTimes, m_FXInfo.uSampleRate );
			if ( eResult != AK_Success )
				return; // passthrough
			m_FXInfo.StereoDelay[uLine].Reset();
		}
	}


	if ( m_pParams->m_ParamChangeHandler.HasChanged( AKSTEREODELAYPARAMID_FILTERTYPE ) ||
		 m_pParams->m_ParamChangeHandler.HasChanged( AKSTEREODELAYPARAMID_FILTERGAIN ) ||
		 m_pParams->m_ParamChangeHandler.HasChanged( AKSTEREODELAYPARAMID_FILTERFREQUENCY ) ||
		 m_pParams->m_ParamChangeHandler.HasChanged( AKSTEREODELAYPARAMID_FILTERQFACTOR ) )
	{
		m_FXInfo.bRecomputeFilterCoefs = true;
	}
	else
		m_FXInfo.bRecomputeFilterCoefs = false;
	m_pParams->m_ParamChangeHandler.ResetAllParamChanges( );
	
#ifdef __PPU__
	// Prepare for SPU job and reserve scratch memory size
	m_DspProcess.ResetDspProcess( true );
	m_DspProcess.SetScratchSize( io_pBuffer->MaxFrames() * (4+2) * sizeof(AkReal32) );
	m_DspProcess.SetDspProcess( AkStereoDelayFXJobBin );

	// Schedule DMA transfers
	m_DspProcess.AddDspProcessSmallDma( io_pBuffer, sizeof(AkAudioBuffer) );
	m_DspProcess.AddDspProcessSmallDma( &m_FXInfo, sizeof( m_FXInfo ) );
	m_DspProcess.AddDspProcessDma( io_pBuffer->GetDataStartDMA(), sizeof(AkReal32) * io_pBuffer->MaxFrames() * io_pBuffer->NumChannels() );
	
	out_pDspProcess = &m_DspProcess;
#else

	AK_PERF_RECORDING_START( "StereoDelay", 25, 30 );

	// Allocate temporary storage for wet path
	AkReal32 * pfStereoBufferStorage  = (AkReal32*) AK_PLUGIN_ALLOC( m_pAllocator, io_pBuffer->MaxFrames() * 4 * sizeof(AkReal32) );
	if ( !pfStereoBufferStorage )
		return; // Pipeline cannot handle errors here, just play dry signal when out of memory

	AkStereoDelayDSPProcess( io_pBuffer, m_FXInfo, pfStereoBufferStorage );

	if ( pfStereoBufferStorage )
	{
		AK_PLUGIN_FREE( m_pAllocator, pfStereoBufferStorage );
		pfStereoBufferStorage = NULL;
	}

	AK_PERF_RECORDING_STOP( "StereoDelay", 25, 30 );
#endif
}
