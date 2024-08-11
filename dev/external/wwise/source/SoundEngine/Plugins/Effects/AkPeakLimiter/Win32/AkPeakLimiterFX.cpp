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

#include "AkPeakLimiterFX.h"
#include <AK/DSP/AkApplyGain.h>
#include "AkMath.h"
#include <AK/Tools/Common/AkAssert.h>
#include "AkDSPUtils.h"

// Note: Scaling factor is such that time specified corresponds to time to reach 90% of target value
static const AkReal32 SCALE_RAMP_TIME = 2.2f;		// Correction factor to make attack/release times accurate

// Plugin mechanism. Dynamic create function whose address must be registered to the FX manager.
AK::IAkPlugin* CreatePeakLimiterFX( AK::IAkPluginMemAlloc * in_pAllocator )
{
	return AK_PLUGIN_NEW( in_pAllocator, CAkPeakLimiterFX( ) );
}

// Constructor.
CAkPeakLimiterFX::CAkPeakLimiterFX()
	: m_fpPerformDSP( NULL )
	, m_pParams( NULL )
	, m_pAllocator( NULL )
#ifndef AK_OPTIMIZED
	, m_pCtx( NULL )
#endif
	, m_SideChains( NULL )
	, m_pfDelayBuffer( NULL )
{
}

// Destructor.
CAkPeakLimiterFX::~CAkPeakLimiterFX()
{

}

// Initializes and allocate memory for the effect
AKRESULT CAkPeakLimiterFX::Init(	AK::IAkPluginMemAlloc * in_pAllocator,		// Memory allocator interface.
									AK::IAkEffectPluginContext * in_pFXCtx,		// FX Context
									AK::IAkPluginParam * in_pParams,			// Effect parameters.
									AkAudioFormat &	in_rFormat					// Required audio input format.
								)
{
	// Set parameters interface and retrieve init params.
	m_pParams = static_cast<CAkPeakLimiterFXParams*>(in_pParams);
	m_pAllocator = in_pAllocator;
#ifndef AK_OPTIMIZED
	m_pCtx = in_pFXCtx;
#endif

	m_format = in_rFormat;

	// Gain ramp initialization for Output level
	m_fCurrentGain = m_pParams->RTPC.fOutputLevel;

	AK_PERF_RECORDING_RESET();

	AKRESULT eResult = InitDelayLine();
	return eResult;
}

// Deallocates and kill effect instance.
AKRESULT CAkPeakLimiterFX::Term( AK::IAkPluginMemAlloc * in_pAllocator )
{
	if ( m_pfDelayBuffer )
		AK_PLUGIN_FREE( in_pAllocator, m_pfDelayBuffer );	

	if ( m_SideChains )
		AK_PLUGIN_FREE( in_pAllocator, m_SideChains );

	AK_PLUGIN_DELETE( in_pAllocator, this );
	return AK_Success;
}

// Clear peak hold, delay lines and side chain states
AKRESULT CAkPeakLimiterFX::Reset( )
{
	if ( m_pfDelayBuffer )
	{
		memset( m_pfDelayBuffer, 0, sizeof(AkReal32)*m_uLookAheadFrames*m_format.GetNumChannels() );	
	}

	if ( m_SideChains )
	{
		for ( AkUInt32 i = 0; i < m_uNumSideChain; ++i )
		{
			m_SideChains[i].fCurrentPeak = 0.f;
			m_SideChains[i].fGainDb = 0.f;
			m_SideChains[i].uPeakTimer = 0;
		}
	}

	// Go through next buffer to find peak
	m_bFirstTime = true;

	return AK_Success;
}

// Effect info query.
AKRESULT CAkPeakLimiterFX::GetPluginInfo( AkPluginInfo & out_rPluginInfo )
{
	out_rPluginInfo.eType = AkPluginTypeEffect;
	out_rPluginInfo.bIsInPlace = true;
	out_rPluginInfo.bIsAsynchronous = false;
	return AK_Success;
}

// Helper function for monitoring
#ifndef AK_OPTIMIZED
static void _FindPeaks( AkAudioBuffer* io_pBuffer, AkReal32 * out_pPeaks, AkUInt32 in_cChannels )
{
	for ( AkUInt32 uChan = 0; uChan < in_cChannels; ++uChan )
	{
		AkReal32 * AK_RESTRICT pfBuf = io_pBuffer->GetChannel(uChan);	
		AkReal32 * AK_RESTRICT pfEnd = pfBuf + io_pBuffer->uValidFrames;

		AkReal32 fPeak = 0;

		while ( pfBuf < pfEnd )
		{
			AkReal32 fIn = fabs( *pfBuf++ );
			fPeak = AK_FPMax( fIn, fPeak );
		}

		out_pPeaks[ uChan ] = fPeak;
	}
}
#endif

//-----------------------------------------------------------------------------
// Name: Execute
// Desc: Execute dynamics processing effect.
//-----------------------------------------------------------------------------
void CAkPeakLimiterFX::Execute(	AkAudioBuffer* io_pBuffer )
{
	if ( m_pParams->RTPC.bDirty )
	{
		m_fReleaseCoef = exp( -SCALE_RAMP_TIME / ( m_pParams->RTPC.fRelease * m_format.uSampleRate ) );
		m_pParams->RTPC.bDirty = false;
	}

	if ( m_pParams->NonRTPC.bDirty ) 
	{
		AKRESULT eResult = InitDelayLine();
		if ( eResult != AK_Success )
			return; // passthrough
		Reset();
	}

	m_FXTailHandler.HandleTail( io_pBuffer, m_uLookAheadFrames );

	if ( io_pBuffer->uValidFrames == 0 )
		return;

#ifndef AK_OPTIMIZED
	char * pMonitorData = NULL;
	int sizeofData = 0;
	if ( m_pCtx->CanPostMonitorData() )
	{
		sizeofData = sizeof( AkUInt16 ) * 2 
			+ sizeof( AkReal32 ) * m_uNumPeakLimitedChannels * 2
			+ sizeof( AkReal32 ) * m_uNumSideChain;
		pMonitorData = (char *) AkAlloca( sizeofData );
		*((AkUInt16 *) pMonitorData ) = (AkUInt16) io_pBuffer->GetChannelMask() & ~(m_pParams->NonRTPC.bProcessLFE ? 0 : AK_SPEAKER_LOW_FREQUENCY );
		*((AkUInt16 *) (pMonitorData+2) ) = (AkUInt16) m_uNumSideChain;

		_FindPeaks( io_pBuffer, (AkReal32 *) pMonitorData + 1, m_uNumPeakLimitedChannels );
	}
#endif

	AK_PERF_RECORDING_START( "PeakLimiter", 25, 30 );

	(this->*m_fpPerformDSP)( io_pBuffer );

	// Apply output gain
	AK::DSP::ApplyGain( io_pBuffer, m_fCurrentGain, m_pParams->RTPC.fOutputLevel, m_pParams->NonRTPC.bProcessLFE );
	m_fCurrentGain = m_pParams->RTPC.fOutputLevel;

	AK_PERF_RECORDING_STOP( "PeakLimiter", 25, 30 );

#ifndef AK_OPTIMIZED
	if ( pMonitorData )
	{
		_FindPeaks( io_pBuffer, (AkReal32 *) pMonitorData + m_uNumPeakLimitedChannels + 1, m_uNumPeakLimitedChannels );

		AkReal32 fRatioFactor = (1.f / m_pParams->RTPC.fRatio) - 1.f;
		for ( AkUInt32 uChan = 0; uChan < m_uNumSideChain; ++uChan )
			((AkReal32 *) pMonitorData )[ m_uNumPeakLimitedChannels * 2 + uChan + 1 ] = m_SideChains[ uChan ].fGainDb * fRatioFactor;

		m_pCtx->PostMonitorData( pMonitorData, sizeofData );
	}
#endif
}

AKRESULT CAkPeakLimiterFX::InitDelayLine()
{
	if ( m_pfDelayBuffer )
	{
		AK_PLUGIN_FREE( m_pAllocator, m_pfDelayBuffer );
		m_pfDelayBuffer = NULL;
	}

	if ( m_SideChains )
	{
		AK_PLUGIN_FREE( m_pAllocator, m_SideChains );
		m_SideChains = NULL;
	}

	// Should not be able to change those at run-time (Wwise only problem)
	AkUInt32 uNumChannels = m_format.GetNumChannels();
	m_uNumPeakLimitedChannels = uNumChannels;
	// Gain and peak limiting is not applied to LFE channel
	if ( !m_pParams->NonRTPC.bProcessLFE && m_format.HasLFE() )
		--m_uNumPeakLimitedChannels;

	if ( m_pParams->NonRTPC.bChannelLink )
		m_uNumSideChain = 1;
	else
	{
		// No need to have side chain for LFE if we are not processing it (still delayed however)
		m_uNumSideChain = m_uNumPeakLimitedChannels;
	}

	m_uLookAheadFrames = static_cast<AkUInt32>( m_pParams->NonRTPC.fLookAhead * m_format.uSampleRate );
	// Note: Attack time is hard coded to half the look ahead time
	m_fAttackCoef = exp( -SCALE_RAMP_TIME / ( m_uLookAheadFrames/2.f ) );

	// Note: LFE channels is always delayed as well
	m_pfDelayBuffer = (AkReal32*)AK_PLUGIN_ALLOC( m_pAllocator, sizeof(AkReal32)*m_uLookAheadFrames*uNumChannels );	
	if ( m_pfDelayBuffer == NULL )
		return AK_InsufficientMemory;
	m_uFramePos = 0;

	// Note: Mono case can be handled by both routines, faster with unlinked process	
	if ( !m_pParams->NonRTPC.bChannelLink || m_format.GetChannelMask() == AK_SPEAKER_SETUP_MONO )
	{
		m_fpPerformDSP = &CAkPeakLimiterFX::Process;
	}
	else if ( !m_format.HasLFE() || m_pParams->NonRTPC.bProcessLFE ) // bChannelLink == true
	{
		m_fpPerformDSP = &CAkPeakLimiterFX::ProcessLinked;
	}
	else // bChannelLink == true && m_format.HasLFE() == true && m_pParams->NonRTPC.bProcessLFE == false
	{
		m_fpPerformDSP = &CAkPeakLimiterFX::ProcessLinkedNoLFE; // Delay LFE without peak limiting it
	}

	// Side chains alloc	
	if ( m_uNumSideChain )
	{
		m_SideChains = (AkPeakLimiterSideChain*)AK_PLUGIN_ALLOC( m_pAllocator, sizeof(AkPeakLimiterSideChain)*m_uNumSideChain );	
		if ( m_SideChains == NULL )
			return AK_InsufficientMemory;
	}

	m_pParams->NonRTPC.bDirty = false;

	return AK_Success;
}

void CAkPeakLimiterFX::Process( AkAudioBuffer * io_pBufferIn )
{
	AkReal32 fThresh = m_pParams->RTPC.fThreshold;
	AkReal32 fRatioFactor = (1.f / m_pParams->RTPC.fRatio) - 1.f;
	fRatioFactor *= 0.05;	//Precompute factor for dbToLin (log2(10)/20)
	AkReal32 fAttackCoef = m_fAttackCoef;
	AkReal32 fReleaseCoef = m_fReleaseCoef;
	AkReal32 * AK_RESTRICT pfDelayBufferRWPtr = NULL;
	AkReal32 * AK_RESTRICT pfDelayBuffer = NULL;
	AkUInt32 uLookAheadFrames = m_uLookAheadFrames;
	AkUInt32 uFramePos = m_uFramePos;

	AkUInt32 uNumPeakLimitedChannels = m_uNumPeakLimitedChannels;

	// Just delay LFE if necessary
	if ( !m_pParams->NonRTPC.bProcessLFE && io_pBufferIn->HasLFE() )
	{
		AkReal32 * AK_RESTRICT pfBuf = io_pBufferIn->GetLFE();
		AkReal32 * AK_RESTRICT pfEnd = pfBuf + io_pBufferIn->uValidFrames;

		AkUInt32 uChannelOffset = uNumPeakLimitedChannels*uLookAheadFrames;
		pfDelayBufferRWPtr = m_pfDelayBuffer + uChannelOffset + uFramePos;
		const AkReal32 * AK_RESTRICT pfDelayBufferEnd = m_pfDelayBuffer + uChannelOffset + uLookAheadFrames;
		pfDelayBuffer = m_pfDelayBuffer + uChannelOffset;

		while ( pfBuf < pfEnd )
		{
			AkUInt32 uFramesBeforeBoundary = (AkUInt32)AkMin( pfEnd - pfBuf, pfDelayBufferEnd - pfDelayBufferRWPtr );
			while ( uFramesBeforeBoundary-- )
			{
				// Read delay sample
				AkReal32 fDelayedSample = *pfDelayBufferRWPtr;
				// Write new input to delay line
				*pfDelayBufferRWPtr++ = *pfBuf;				
				// Output delayed LFE
				*pfBuf++ = fDelayedSample;
			}
			// Wrap delay line
			if ( pfDelayBufferRWPtr == pfDelayBufferEnd )
				pfDelayBufferRWPtr = pfDelayBuffer;
		}
	}

	// Process all channels (except LFE if it was handled above).
	for ( AkUInt32 uChan = 0; uChan < uNumPeakLimitedChannels; ++uChan )
	{
		AkReal32 * AK_RESTRICT pfBuf = io_pBufferIn->GetChannel(uChan);	
		AkReal32 * AK_RESTRICT pfEnd = pfBuf + io_pBufferIn->uValidFrames;

		AkUInt32 uChannelOffset = uChan*uLookAheadFrames;
		pfDelayBufferRWPtr = m_pfDelayBuffer + uChannelOffset + uFramePos;
		const AkReal32 * AK_RESTRICT pfDelayBufferEnd = m_pfDelayBuffer + uChannelOffset + uLookAheadFrames;
		pfDelayBuffer = m_pfDelayBuffer + uChannelOffset;

		// Local variables for performance
		AkReal32 fLocGainDb = m_SideChains[uChan].fGainDb;
		AkReal32 fCurrentPeak = m_SideChains[uChan].fCurrentPeak;
		AkUInt32 uPeakTimer = m_SideChains[uChan].uPeakTimer;		

		// If first buffer received, process a whole buffer to find peak ( reset() reinitializes this flag)
		if ( m_bFirstTime )
		{
			AkUInt32 uLoopFrames = AkMin( uLookAheadFrames, io_pBufferIn->uValidFrames );
			for ( AkUInt32 i = 0; i < uLoopFrames; ++i )
			{
				AkReal32 fAbsX = fabs( pfBuf[i]);
				if ( fAbsX >= fCurrentPeak )
				{
					fCurrentPeak = fAbsX;
					uPeakTimer = uLoopFrames - i;
				}
			}
			if ( uChan == uNumPeakLimitedChannels-1 )
				m_bFirstTime = false;
		}

		AkReal32 fPowerDb = 20.f*AK::FastLog10( fCurrentPeak );	// Convert power estimation to dB
		AkReal32 fDbOverThresh = AK_FPMax(fPowerDb - fThresh, 0.f);			// Offset into non-linear range (over threshold)
		while ( pfBuf < pfEnd )
		{
			AkUInt32 uFramesBeforeBoundary = (AkUInt32)AkMin( pfEnd - pfBuf, pfDelayBufferEnd - pfDelayBufferRWPtr );
			while ( uFramesBeforeBoundary-- )
			{
				// Read delay sample
				AkReal32 fDelayedSample = *pfDelayBufferRWPtr;
				// Write new input to delay line
				AkReal32 fIn = *pfBuf;
				*pfDelayBufferRWPtr++ = fIn;

				// IMPORTANT NOTE: We know that simply getting next value as the next peak once the old one times out is not ideal.
				// It most likely will not be the highest sample of the lookahead buffer. This on the other hand allows
				// tremendous memory and CPU optimizations. In practice release times used are long enough to avoid problems
				// related to this.

				// Get a new peak value if higher than current or if old peak has timed out.
				fIn = fabs( fIn );	// |x[n]| (rectification)		
				if ( !uPeakTimer || fIn > fCurrentPeak )
				{
					fCurrentPeak = fIn;					// New peak value
					uPeakTimer = uLookAheadFrames;		// Reset timer

					// Find dB over threshold
					fPowerDb = 20.f*AK::FastLog10( fCurrentPeak );		// Convert power estimation to dB
					fDbOverThresh = AK_FPMax(fPowerDb - fThresh, 0.f);	// Offset into non-linear range (over threshold)
				}
				else
					uPeakTimer--;		

				// Attack and release smoothing
				AkReal32 fCoef = AK_FSEL(fDbOverThresh - fLocGainDb, fAttackCoef, fReleaseCoef );
				fLocGainDb = fDbOverThresh + fCoef * ( fLocGainDb - fDbOverThresh );

				// Static transfer function evaluation
				AkReal32 fGainReduction = AkMath::FastPow10(fLocGainDb * fRatioFactor);	// Gain reduction (dB) and convert to linear

				// Apply compression gain
				*pfBuf++ = fDelayedSample * fGainReduction;
			}
			// Wrap delay line
			if ( pfDelayBufferRWPtr == pfDelayBufferEnd )
				pfDelayBufferRWPtr = pfDelayBuffer;
		}

		// Local variables for performance
		m_SideChains[uChan].fGainDb = fLocGainDb;
		m_SideChains[uChan].fCurrentPeak = fCurrentPeak;
		m_SideChains[uChan].uPeakTimer = uPeakTimer;
	}

	// Update frame position within delay lines
	m_uFramePos = (AkUInt32) (pfDelayBufferRWPtr - pfDelayBuffer);
	AKASSERT( m_uFramePos <= m_uLookAheadFrames );
}

// Note: delay line buffer is written INTERLEAVED in this mode of processing
// Note: The LFE (if present) is not treated differently than other channels in this routine 
void CAkPeakLimiterFX::ProcessLinked( AkAudioBuffer * io_pBufferIn )
{
	// Note assuming all channels are the same threshold needs to be scaled by sqrt(numChannels) when adding power of
	// many channels
	AkReal32 fThresh = m_pParams->RTPC.fThreshold;
	AkReal32 fRatioFactor = (1.f / m_pParams->RTPC.fRatio) - 1.f;
	fRatioFactor *= 0.05;	//Precompute factor for dbToLin (log2(10)/20)
	AkReal32 fAttackCoef = m_fAttackCoef;
	AkReal32 fReleaseCoef = m_fReleaseCoef;

	// Local variables for performance
	AkUInt32 uNumChannels = m_format.GetNumChannels();
	AkReal32 * AK_RESTRICT pfDelayBufferRWPtr = m_pfDelayBuffer + m_uFramePos*uNumChannels;
	const AkReal32 * pfDelayBufferEnd = m_pfDelayBuffer + m_uLookAheadFrames*uNumChannels;
	AkUInt32 uLookAheadFrames = m_uLookAheadFrames;
	AkUInt32 uNumFrames = io_pBufferIn->uValidFrames;
	AkUInt32 uMaxFrames = io_pBufferIn->MaxFrames();
	AkReal32 fLocGainDb = m_SideChains->fGainDb;
	AkReal32 fCurrentPeak = m_SideChains->fCurrentPeak;
	AkUInt32 uPeakTimer = m_SideChains->uPeakTimer;

	// Setup pointers to all channels
	AkReal32 * AK_RESTRICT pfBuf = io_pBufferIn->GetChannel(0);
	AkReal32 fDelayedSamples[MAXCHANNELS];

	// If first buffer received, process a whole buffer to find peak ( reset() reinitializes this flag)
	if ( m_bFirstTime )
	{
		AkUInt32 uLoopFrames = AkMin( uLookAheadFrames, io_pBufferIn->uValidFrames );
		AkReal32 * AK_RESTRICT pChanBuf = pfBuf;
		for ( AkUInt32 uChan = 0; uChan < uNumChannels; ++uChan )
		{
			for ( AkUInt32 i = 0; i < uLoopFrames; ++i )
			{
				AkReal32 fAbsX = fabs( pChanBuf[i]);
				if ( fAbsX > fCurrentPeak )
				{
					fCurrentPeak = fAbsX;
					uPeakTimer = uLoopFrames - i;
				}
			}			
			pChanBuf += uMaxFrames;
		}
		m_bFirstTime = false;
	}

	AkReal32 fPowerDb = 20.f*AK::FastLog10( fCurrentPeak );	// Convert power estimation to dB
	AkReal32 fDbOverThresh = AK_FPMax(fPowerDb - fThresh, 0.f);			// Offset into non-linear range (over threshold)

	AkUInt32 uIndex = 0;
	while ( uIndex < uNumFrames )
	{
		AkUInt32 uFramesBeforeBoundary = (AkUInt32)AkMin( uNumFrames-uIndex, (pfDelayBufferEnd - pfDelayBufferRWPtr) / uNumChannels );

		while ( uFramesBeforeBoundary-- )
		{
			// Find peak value in all channels
			AkReal32 fChannelsPeak = 0.f;
			AkReal32 * AK_RESTRICT pChanBuf = pfBuf;
			for ( AkUInt32 uChan = 0; uChan < uNumChannels; ++uChan )
			{
				// Read delay samples to local storage
				fDelayedSamples[uChan] = pfDelayBufferRWPtr[uChan];
				// Write new input to delay lines
				AkReal32 fIn = pChanBuf[uIndex];
				pfDelayBufferRWPtr[uChan] = fIn;
				fIn = fabs( fIn );	// |x[n]| (rectification)	
				fChannelsPeak = AK_FPMax( fIn, fChannelsPeak );			
				pChanBuf += uMaxFrames;
			}
			pfDelayBufferRWPtr += uNumChannels;

			// IMPORTANT NOTE: We know that simply getting next value as the next peak once the old one times out is not ideal.
			// It most likely will not be the highest sample of the lookahead buffer. This on the other hand allows
			// tremendous memory and CPU optimizations. In practice release times used are long enough to avoid problems
			// related to this.

			// Get a new peak value if higher than current or if old peak has timed out.
			if ( !uPeakTimer || fChannelsPeak > fCurrentPeak )
			{
				fCurrentPeak = fChannelsPeak;		// New peak value
				uPeakTimer = uLookAheadFrames;		// Reset timer

				// Find dB over threshold
				fPowerDb = 20.f*AK::FastLog10( fCurrentPeak );		// Convert power estimation to dB
				fDbOverThresh = AK_FPMax(fPowerDb - fThresh, 0.f);		// Offset into non-linear range (over threshold)
			}
			else
				uPeakTimer--;

			// Attack and release smoothing
			AkReal32 fCoef = AK_FSEL(fDbOverThresh - fLocGainDb, fAttackCoef, fReleaseCoef );
			fLocGainDb = fDbOverThresh + fCoef * ( fLocGainDb - fDbOverThresh );

			// Static transfer function evaluation
			AkReal32 fGainReduction = AkMath::FastPow10(fLocGainDb * fRatioFactor);	// Gain reduction (dB) and convert to linear

			// Apply compression gain to all channels
			pChanBuf = pfBuf;
			for ( AkUInt32 uChan = 0; uChan < uNumChannels; ++uChan )
			{
				pChanBuf[uIndex] = fDelayedSamples[uChan] * fGainReduction;
				pChanBuf += uMaxFrames;
			}
			++uIndex;
		}

		if ( pfDelayBufferRWPtr == pfDelayBufferEnd )
			pfDelayBufferRWPtr = m_pfDelayBuffer;
	}

	// Save local variables
	m_SideChains->fGainDb = fLocGainDb;
	m_SideChains->fCurrentPeak = fCurrentPeak;
	m_SideChains->uPeakTimer = uPeakTimer;
	m_uFramePos = (AkUInt32) (pfDelayBufferRWPtr - m_pfDelayBuffer) / uNumChannels;
	AKASSERT( m_uFramePos <= m_uLookAheadFrames );
}

// This routine is called when the processing is linked AND there is a LFE present that must not be peak limited but only delayed
// Note: delay line buffer is written INTERLEAVED in this mode of processing
void CAkPeakLimiterFX::ProcessLinkedNoLFE( AkAudioBuffer * io_pBufferIn )
{
	// Note assuming all channels are the same threshold needs to be scaled by sqrt(numChannels) when adding power of
	// many channels
	AkReal32 fThresh = m_pParams->RTPC.fThreshold;
	AkReal32 fRatioFactor = (1.f / m_pParams->RTPC.fRatio) - 1.f;
	fRatioFactor *= 0.05;	//Precompute factor for dbToLin (log2(10)/20)
	AkReal32 fAttackCoef = m_fAttackCoef;
	AkReal32 fReleaseCoef = m_fReleaseCoef;

	// Local variables for performance
	const AkUInt32 uNumChannels = m_format.GetNumChannels();
	AkReal32 * AK_RESTRICT pfDelayBufferRWPtr = m_pfDelayBuffer + m_uFramePos*uNumChannels;
	const AkReal32 * pfDelayBufferEnd = m_pfDelayBuffer + m_uLookAheadFrames*uNumChannels;
	AkUInt32 uLookAheadFrames = m_uLookAheadFrames;
	AkUInt32 uNumFrames = io_pBufferIn->uValidFrames;
	AkUInt32 uMaxFrames = io_pBufferIn->MaxFrames();
	AkReal32 fLocGainDb = m_SideChains->fGainDb;
	AkReal32 fCurrentPeak = m_SideChains->fCurrentPeak;
	AkUInt32 uPeakTimer = m_SideChains->uPeakTimer;
	const AkUInt32 uNumPeakLimitedChannels = m_uNumPeakLimitedChannels;

	// Setup pointers to all channels
	AKASSERT( io_pBufferIn->HasLFE() );
	AkReal32 * AK_RESTRICT pfBuf = io_pBufferIn->GetChannel(0);
	AkReal32 fDelayedSamples[MAXCHANNELS];

	// If first buffer received, process a whole buffer to find peak ( reset() reinitializes this flag)
	if ( m_bFirstTime )
	{
		AkUInt32 uLoopFrames = AkMin( uLookAheadFrames, io_pBufferIn->uValidFrames );

		AkReal32 * AK_RESTRICT pChanBuf = pfBuf;
		// Skip LFE in this loop (we know that it is present and that it is the last channel).
		for ( AkUInt32 uChan = 0; uChan < uNumPeakLimitedChannels; ++uChan )
		{
			for ( AkUInt32 i = 0; i < uLoopFrames; ++i )
			{
				AkReal32 fAbsX = fabs( pChanBuf[i]);
				if ( fAbsX > fCurrentPeak )
				{
					fCurrentPeak = fAbsX;
					uPeakTimer = uLoopFrames - i;
				}
			}			
			pChanBuf += uMaxFrames;
		}
		m_bFirstTime = false;
	}

	AkReal32 fPowerDb = 20.f*AK::FastLog10( fCurrentPeak );	// Convert power estimation to dB
	AkReal32 fDbOverThresh = AK_FPMax(fPowerDb - fThresh, 0.f);			// Offset into non-linear range (over threshold)

	AkUInt32 uIndex = 0;
	while ( uIndex < uNumFrames )
	{
		AkUInt32 uFramesBeforeBoundary = (AkUInt32)AkMin( uNumFrames-uIndex, (pfDelayBufferEnd - pfDelayBufferRWPtr) / uNumChannels );

		while ( uFramesBeforeBoundary-- )
		{
			// Find peak value in all processed channels
			AkReal32 fChannelsPeak = 0.f;
			{
				AkReal32 * AK_RESTRICT pChanBuf = pfBuf;
				AkUInt32 uChan = 0;
				for ( ; uChan < uNumPeakLimitedChannels; ++uChan )
				{
					// Read delay samples to local storage
					fDelayedSamples[uChan] = pfDelayBufferRWPtr[uChan];
					// Write new input to delay lines
					AkReal32 fIn = pChanBuf[uIndex];
					pfDelayBufferRWPtr[uChan] = fIn;
					fIn = fabs( fIn );	// |x[n]| (rectification)	
					fChannelsPeak = AK_FPMax( fIn, fChannelsPeak );		
					pChanBuf += uMaxFrames;
				}

				// Handle LFE channel separately
				fDelayedSamples[uChan] = pfDelayBufferRWPtr[uChan];
				AkReal32 fIn = pChanBuf[uIndex];
				pfDelayBufferRWPtr[uChan] = fIn;
				pChanBuf[uIndex] = fDelayedSamples[uChan];

				pfDelayBufferRWPtr += uNumChannels;
			}

			// IMPORTANT NOTE: We know that simply getting next value as the next peak once the old one times out is not ideal.
			// It most likely will not be the highest sample of the lookahead buffer. This on the other hand allows
			// tremendous memory and CPU optimizations. In practice release times used are long enough to avoid problems
			// related to this.

			// Get a new peak value if higher than current or if old peak has timed out.
			if ( !uPeakTimer || fChannelsPeak > fCurrentPeak )
			{
				fCurrentPeak = fChannelsPeak;		// New peak value
				uPeakTimer = uLookAheadFrames;		// Reset timer

				// Find dB over threshold
				fPowerDb = 20.f*AK::FastLog10( fCurrentPeak );		// Convert power estimation to dB
				fDbOverThresh = AK_FPMax(fPowerDb - fThresh, 0.f);		// Offset into non-linear range (over threshold)
			}
			else
				uPeakTimer--;

			// Attack and release smoothing
			AkReal32 fCoef = AK_FSEL(fDbOverThresh - fLocGainDb, fAttackCoef, fReleaseCoef );
			fLocGainDb = fDbOverThresh + fCoef * ( fLocGainDb - fDbOverThresh );

			// Static transfer function evaluation
			AkReal32 fGainReduction = AkMath::FastPow10(fLocGainDb * fRatioFactor);	// Gain reduction (dB) and convert to linear

			// Apply compression gain to all processed channels
			AkReal32 * AK_RESTRICT pChanBuf = pfBuf;
			for ( AkUInt32 uChan = 0; uChan < uNumPeakLimitedChannels; ++uChan )
			{
				pChanBuf[uIndex] = fDelayedSamples[uChan] * fGainReduction;
				pChanBuf += uMaxFrames;
			}
			++uIndex;
		}

		if ( pfDelayBufferRWPtr == pfDelayBufferEnd )
			pfDelayBufferRWPtr = m_pfDelayBuffer;
	}

	// Save local variables
	m_SideChains->fGainDb = fLocGainDb;
	m_SideChains->fCurrentPeak = fCurrentPeak;
	m_SideChains->uPeakTimer = uPeakTimer;
	m_uFramePos = (AkUInt32) (pfDelayBufferRWPtr - m_pfDelayBuffer) / uNumChannels;
	AKASSERT( m_uFramePos <= m_uLookAheadFrames );
}