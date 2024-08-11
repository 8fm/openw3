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
// AkExpanderFX.cpp
//
// Expander processing FX implementation.
// 
// Note that Attack time here corresponds to time taken to reach NO gain reduction, 
// and release time corresponds to time take to get to 90 % of target gain reduction
//
// Copyright 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "AkExpanderFX.h"
#include <AK/DSP/AkApplyGain.h>
#include "AkMath.h"
#include "AkDSPUtils.h"

static const AkReal32 RMSWINDOWLENGTH = 0.02322f;	// ~ 1 audio buffer
static const AkReal32 SMALL_DC_OFFSET = 1.e-25f;	// DC offset to avoid log(0)
// Note: Scaling factor is such that time specified corresponds to time to reach 90% of target value
static const AkReal32 SCALE_RAMP_TIME = 2.2f;		// Correction factor to make attack/release times accurate
static const AkUInt32 MAXCHANNELS = AK_VOICE_MAX_NUM_CHANNELS;

// Plugin mechanism. Dynamic create function whose address must be registered to the FX manager.
AK::IAkPlugin* CreateExpanderFX( AK::IAkPluginMemAlloc * in_pAllocator )
{
	AKASSERT( in_pAllocator != NULL );
	return AK_PLUGIN_NEW( in_pAllocator, CAkExpanderFX( ) );
}

// Constructor.
CAkExpanderFX::CAkExpanderFX()
{
	m_pSharedParams = NULL;
	m_fpPerformDSP = NULL;
	m_pSideChain = NULL;
}

// Destructor.
CAkExpanderFX::~CAkExpanderFX()
{

}

// Initializes and allocate memory for the effect
AKRESULT CAkExpanderFX::Init(	AK::IAkPluginMemAlloc * in_pAllocator,		// Memory allocator interface.
								AK::IAkEffectPluginContext * in_pFXCtx,		// FX Context
								AK::IAkPluginParam * in_pParams,			// Effect parameters.
								AkAudioFormat &	in_rFormat					// Required audio input format.
								)
{
	// Set parameters interface and retrieve init params.
	m_pSharedParams = static_cast<CAkExpanderFXParams*>(in_pParams);
	AkExpanderFXParams Params;
	m_pSharedParams->GetParams( &Params );

	// Should not be able to change those at run-time (Wwise only problem)
	m_bProcessLFE = Params.bProcessLFE;

	// Allocate sidechain resources
	m_uNumChannels = in_rFormat.GetNumChannels( );
	m_uSampleRate = in_rFormat.uSampleRate;

	// Initialized cached values
	m_fCachedAttack = Params.fAttack;
	m_fCachedAttackCoef = exp( -SCALE_RAMP_TIME / ( Params.fAttack * m_uSampleRate ) );
	m_fCachedRelease = Params.fRelease;
	m_fCachedReleaseCoef = exp( -SCALE_RAMP_TIME / ( Params.fRelease * m_uSampleRate ) );

	// Note: 1 channel case can be handled by both routines, faster with unlinked process
	AkUInt32 uNumProcessedChannels = m_uNumChannels;
	if ( Params.bChannelLink == false || m_uNumChannels == 1 )
		m_fpPerformDSP = &CAkExpanderFX::Process;
	else
	{
		m_fpPerformDSP = &CAkExpanderFX::ProcessLinked;
		if ( in_rFormat.HasLFE() && !Params.bProcessLFE )
			--uNumProcessedChannels;
	}
	m_uNumSideChain = Params.bChannelLink ? 1 : uNumProcessedChannels;

	// Algorithm specific initializations
	m_pSideChain = (AkExpanderSideChain*)AK_PLUGIN_ALLOC( in_pAllocator, sizeof(AkExpanderSideChain)*m_uNumSideChain );	
	if ( m_pSideChain == NULL )
		return AK_InsufficientMemory;

	// Sidechain initialization
	m_fRMSFilterCoef = exp( -1.f / ( RMSWINDOWLENGTH * m_uSampleRate ) );

	// Gain ramp initialization for Output level
	m_fCurrentGain = Params.fOutputLevel;

	AK_PERF_RECORDING_RESET();

	return AK_Success;
}

// Terminates.
AKRESULT CAkExpanderFX::Term( AK::IAkPluginMemAlloc * in_pAllocator )
{
	if ( m_pSideChain )
		AK_PLUGIN_FREE( in_pAllocator, m_pSideChain );
	AK_PLUGIN_DELETE( in_pAllocator, this );
	return AK_Success;
}

// Reset
AKRESULT CAkExpanderFX::Reset( )
{
	// Sidechain initialization
	for (AkUInt32 i = 0; i < m_uNumSideChain; ++i )
	{
		m_pSideChain[i].fGainDb = 0.f;
		m_pSideChain[i].fMem = 0.f;
	}
	return AK_Success;
}

// Effect info query.
AKRESULT CAkExpanderFX::GetPluginInfo( AkPluginInfo & out_rPluginInfo )
{
	out_rPluginInfo.eType = AkPluginTypeEffect;
	out_rPluginInfo.bIsInPlace = true;
	out_rPluginInfo.bIsAsynchronous = false;
	return AK_Success;
}

//-----------------------------------------------------------------------------
// Name: Execute
// Desc: Execute dynamics processing effect.
//-----------------------------------------------------------------------------
void CAkExpanderFX::Execute( AkAudioBuffer * io_pBuffer )
{
	if ( io_pBuffer->uValidFrames == 0 )
		return;

	AK_PERF_RECORDING_START( "Expander", 25, 30 );

	AkExpanderFXParams Params;
	m_pSharedParams->GetParams( &Params );

	// Watchout: Params.bChannelLink and bProcessLFE may have been changed by Wwise
	// These parameters don't support run-time change and are ignored by DSP routine
	(this->*m_fpPerformDSP)( io_pBuffer, &Params );

	// Apply output gain
	AK::DSP::ApplyGain( io_pBuffer, m_fCurrentGain, Params.fOutputLevel, m_bProcessLFE );
	m_fCurrentGain = Params.fOutputLevel;

	AK_PERF_RECORDING_STOP( "Expander", 25, 30 );
}

void CAkExpanderFX::Process( AkAudioBuffer * io_pBufferIn, AkExpanderFXParams * in_pParams )
{
	AKASSERT( m_uNumChannels == m_uNumSideChain ); // Unlinked processing
	AkReal32 fThresh = in_pParams->fThreshold;
	AkReal32 fRatioFactor = in_pParams->fRatio - 1.f;
	if ( in_pParams->fAttack != m_fCachedAttack )
	{
		m_fCachedAttack = in_pParams->fAttack;
		m_fCachedAttackCoef = exp( -SCALE_RAMP_TIME / ( in_pParams->fAttack * m_uSampleRate ) );
	}
	AkReal32 fAttackCoef = m_fCachedAttackCoef;
	if ( in_pParams->fRelease != m_fCachedRelease )
	{
		m_fCachedRelease = in_pParams->fRelease;
		m_fCachedReleaseCoef = exp( -SCALE_RAMP_TIME / ( in_pParams->fRelease * m_uSampleRate ) );
	}
	AkReal32 fReleaseCoef = m_fCachedReleaseCoef;
	AkReal32 fRMSFilterCoef = m_fRMSFilterCoef;

	// Skip LFE if necessary (LFE is always the last channel).
	AkUInt32 uNumProcessedChannels = ( io_pBufferIn->HasLFE() && !m_bProcessLFE ) ? m_uNumChannels - 1 : m_uNumChannels;
	for ( AkUInt32 uChan = 0; uChan < uNumProcessedChannels; ++uChan )
	{
		AkReal32 * AK_RESTRICT pfBuf = io_pBufferIn->GetChannel(uChan);	
		AkReal32 * AK_RESTRICT pfEnd = pfBuf + io_pBufferIn->uValidFrames;

		// Optimization: Local variables
		AkReal32 fLocRMSFilterMem = m_pSideChain[uChan].fMem;
		AkReal32 fLocGainDb = m_pSideChain[uChan].fGainDb;

		while ( pfBuf < pfEnd )
		{
			// Sidechain computations
			AkReal32 fSqIn = *pfBuf * *pfBuf;					// x[n]^2 (rectification)
			fSqIn += SMALL_DC_OFFSET;							// Avoid log 0
			fLocRMSFilterMem = fSqIn + fRMSFilterCoef * ( fLocRMSFilterMem - fSqIn );	// Run one-pole filter
			
			AkReal32 fPowerDb = 10.f*AK::FastLog10( fLocRMSFilterMem );	// Convert power estimation to dB
			AkReal32 fDbUnderThresh = fThresh - fPowerDb;		// Offset into non-linear range (under threshold)
			fDbUnderThresh = AK_FPMax( fDbUnderThresh, 0.f );

			// Attack and release smoothing
			AkReal32 fCoef = AK_FSEL(fDbUnderThresh - fLocGainDb, fReleaseCoef, fAttackCoef );
			fLocGainDb = fDbUnderThresh + fCoef * ( fLocGainDb - fDbUnderThresh);
			// Static transfer function evaluation
			AkReal32 fGainReductiondB = -fLocGainDb * fRatioFactor;				// Gain reduction (dB)
			AkReal32 fGainReduction = AK::dBToLin( fGainReductiondB );			// Convert to linear

			// Apply compression gain
			*pfBuf++ *= fGainReduction;
		}

// Possibly fix this if there are denormal problems on PS3
#ifdef AK_XBOX360	
		// Save local variables
		RemoveDenormal( fLocRMSFilterMem );
		RemoveDenormal( fLocGainDb );
#endif

		// Save local variables
		m_pSideChain[uChan].fMem = fLocRMSFilterMem;
		m_pSideChain[uChan].fGainDb = fLocGainDb;
	}
}

void CAkExpanderFX::ProcessLinked( AkAudioBuffer * io_pBufferIn, AkExpanderFXParams * in_pParams )
{
	AKASSERT( m_uNumSideChain == 1); // Linked processing
	// Note: Using average power from all processed channels to determine whether to compress or not
	// Skip LFE if necessary (LFE is always the last channel).
	AkUInt32 uNumProcessedChannels = ( io_pBufferIn->HasLFE() && !m_bProcessLFE ) ? m_uNumChannels - 1 : m_uNumChannels;
	const AkReal32 fOneOverNumProcessedChannels = 1.f/uNumProcessedChannels;
	AkReal32 fThresh = in_pParams->fThreshold;
	AkReal32 fRatioFactor = in_pParams->fRatio - 1.f;
	if ( in_pParams->fAttack != m_fCachedAttack )
	{
		m_fCachedAttack = in_pParams->fAttack;
		m_fCachedAttackCoef = exp( -SCALE_RAMP_TIME / ( in_pParams->fAttack * m_uSampleRate ) );
	}
	AkReal32 fAttackCoef = m_fCachedAttackCoef;
	if ( in_pParams->fRelease != m_fCachedRelease )
	{
		m_fCachedRelease = in_pParams->fRelease;
		m_fCachedReleaseCoef = exp( -SCALE_RAMP_TIME / ( in_pParams->fRelease * m_uSampleRate ) );
	}
	AkReal32 fReleaseCoef = m_fCachedReleaseCoef;
	AkReal32 fRMSFilterCoef = m_fRMSFilterCoef;
	AkReal32 fLocRMSFilterMem = m_pSideChain[0].fMem;
	AkReal32 fLocGainDb = m_pSideChain[0].fGainDb;

	// Setup pointers to all channels
	AkReal32 * AK_RESTRICT pfBuf[MAXCHANNELS];
	for ( AkUInt32 uChan = 0; uChan < uNumProcessedChannels; ++uChan )
	{
		pfBuf[uChan]= io_pBufferIn->GetChannel(uChan);
	}
	AkInt32 iNumFrames = io_pBufferIn->uValidFrames;

	while ( --iNumFrames >= 0 )
	{
		AkReal32 fAvgPower = 0.f;
		for ( AkUInt32 uChan = 0; uChan < uNumProcessedChannels; ++uChan )
		{
			AkReal32 fIn = *(pfBuf[uChan]);
			fAvgPower += fIn * fIn;			// x[n]^2 (rectification)		
		}
		fAvgPower *= fOneOverNumProcessedChannels;
		fAvgPower += SMALL_DC_OFFSET;							// Avoid log 0 problems
		fLocRMSFilterMem = fAvgPower + fRMSFilterCoef * ( fLocRMSFilterMem - fAvgPower );	// Run one-pole filter
		AkReal32 fPowerDb = 10.f*AK::FastLog10( fLocRMSFilterMem );	// Convert power estimation to dB (sqrt taken out)
		AkReal32 fDbUnderThresh = fThresh - fPowerDb;		// Offset into non-linear range (under threshold)
		fDbUnderThresh = AK_FPMax( fDbUnderThresh, 0.f );

		// Attack and release smoothing
		AkReal32 fCoef = AK_FSEL(fDbUnderThresh - fLocGainDb, fReleaseCoef, fAttackCoef );
		fLocGainDb = fDbUnderThresh + fCoef * ( fLocGainDb - fDbUnderThresh);
		// Static transfer function evaluation
		AkReal32 fGainReductiondB = -fLocGainDb * fRatioFactor;				// Gain reduction (dB)
		AkReal32 fGainReduction = AK::dBToLin( fGainReductiondB );			// Convert to linear

		// Apply compression gain to all channels
		for ( AkUInt32 uChan = 0; uChan < uNumProcessedChannels; ++uChan )
		{
			*(pfBuf[uChan]) *= fGainReduction;
			pfBuf[uChan]++;
		}
	}

// Possibly fix this if there are denormal problems on PS3
#ifdef AK_XBOX360	
		// Save local variables
		RemoveDenormal( fLocRMSFilterMem );
		RemoveDenormal( fLocGainDb );
#endif

	// Save local variables
	m_pSideChain[0].fMem = fLocRMSFilterMem;
	m_pSideChain[0].fGainDb = fLocGainDb;
}
