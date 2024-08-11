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
// AkParametricEQFX.cpp
//
// ParametricEQ FX implementation.
// 
// Based on evaluation of bilinear transform equations for use with biquad
// y[n] = (b0/a0)*x[n] + (b1/a0)*x[n-1] + (b2/a0)*x[n-2]
//                     - (a1/a0)*y[n-1] - (a2/a0)*y[n-2]
//
// Copyright 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "AkParametricEQFX.h"
#include <AK/DSP/AkApplyGain.h>
#include <math.h>
#include <AK/Tools/Common/AkAssert.h>

#define PI			(3.14159265358979323846f)
#define SQRT2		(1.41421356237309504880f)
#define SHELFSLOPE	(1.f) // Maximum shelf slope

#ifdef AK_PS3
// embedded SPU Job Binary symbols
extern char _binary_ParametricEQFX_spu_bin_start[];
extern char _binary_ParametricEQFX_spu_bin_size[];
static AK::MultiCoreServices::BinData ParametricEQFXJobBin = { _binary_ParametricEQFX_spu_bin_start, CELL_SPURS_GET_SIZE_BINARY( _binary_ParametricEQFX_spu_bin_size ) };
#endif

// Plugin mechanism. Dynamic create function whose address must be registered to the FX manager.
AK::IAkPlugin* CreateParametricEQFX( AK::IAkPluginMemAlloc * in_pAllocator )
{
	AKASSERT( in_pAllocator != NULL );
	return AK_PLUGIN_NEW( in_pAllocator, CAkParametricEQFX( ) );
}


// Constructor.
CAkParametricEQFX::CAkParametricEQFX()
{
	m_pSharedParams = NULL;
	m_pfAllocatedMem = NULL;
}

// Destructor.
CAkParametricEQFX::~CAkParametricEQFX()
{

}

// Initializes and allocate memory for the effect
AKRESULT CAkParametricEQFX::Init(	AK::IAkPluginMemAlloc * in_pAllocator,		// Memory allocator interface.
									AK::IAkEffectPluginContext * in_pFXCtx,			// FX Context
									AK::IAkPluginParam * in_pParams,		// Effect parameters.
									AkAudioFormat &	in_rFormat				// Required audio input format.
								)
{
	// Save format internally
	m_uNumProcessedChannels = in_rFormat.GetNumChannels( );
	m_uSampleRate = in_rFormat.uSampleRate;

	// Set parameters.
	m_pSharedParams = static_cast<CAkParameterEQFXParams*>(in_pParams);

	// LFE passthrough behavior
	if( in_rFormat.HasLFE() && !m_pSharedParams->GetProcessLFE( ) )
	{
		--m_uNumProcessedChannels;
	}
	
	// Allocate biquad mem locations e.g L||x[n-1]|x[n-2]|y[n-1]|y[n-2]|| R||x[n-1]|x[n-2]|y[n-1]|y[n-2]||
	if ( m_uNumProcessedChannels )
	{
		AkUInt32 uAllocSize = sizeof(AkReal32)* NUMBER_FILTER_MODULES * NUMBER_FILTER_MEMORY * m_uNumProcessedChannels;
#ifdef AK_PS3
		uAllocSize = AK_ALIGN_SIZE_FOR_DMA( uAllocSize );
#endif
		m_pfAllocatedMem = (AkReal32*) AK_PLUGIN_ALLOC( in_pAllocator, uAllocSize );
		if ( m_pfAllocatedMem == NULL )
			return AK_InsufficientMemory;
	}
	
	// Coefficients for 3 biquad units
	memset( m_pfFiltCoefs, 0, NUMBER_FILTER_MODULES * NUMBER_COEFFICIENTS_PER_BAND * sizeof(AkReal32) );

	// Set parameters as dirty to trigger coef compute on first Execute()
	m_pSharedParams->SetDirty( BAND1, true );
	m_pSharedParams->SetDirty( BAND2, true );
	m_pSharedParams->SetDirty( BAND3, true );

	// Gain ramp initialization for Output level
	m_fCurrentGain = m_pSharedParams->GetOutputLevel( );

	AK_PERF_RECORDING_RESET();

	return AK_Success;
}

// Terminates.
AKRESULT CAkParametricEQFX::Term( AK::IAkPluginMemAlloc * in_pAllocator )
{
	// Free filter memory
	if ( m_pfAllocatedMem != NULL )
	{
		AK_PLUGIN_FREE( in_pAllocator, m_pfAllocatedMem );
		m_pfAllocatedMem = NULL;
	}

	// Effect's deletion
	AK_PLUGIN_DELETE( in_pAllocator, this );
	return AK_Success;
}

// Reset or seek to start.
AKRESULT CAkParametricEQFX::Reset( )
{
	// Clear filter memory
	if ( m_pfAllocatedMem != NULL )
	{
		AkUInt32 uAllocSize = sizeof(AkReal32)* NUMBER_FILTER_MODULES * NUMBER_FILTER_MEMORY * m_uNumProcessedChannels;
#ifdef AK_PS3
		uAllocSize = AK_ALIGN_SIZE_FOR_DMA( uAllocSize );
#endif		
		memset( m_pfAllocatedMem, 0, uAllocSize );
	}
	return AK_Success;
}

// Effect info query.
AKRESULT CAkParametricEQFX::GetPluginInfo( AkPluginInfo & out_rPluginInfo )
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
// Execute parametric EQ effect.
void CAkParametricEQFX::Execute(	AkAudioBuffer*	io_pBuffer,							// Input buffer interface.
									AK::MultiCoreServices::DspProcess*&	out_pDspProcess	// the process that needs to run
									)
{
	if ( m_uNumProcessedChannels == 0 )
	{
		out_pDspProcess = NULL;
		return;
	}

	// Retrieve parameter for each filter
	AkUInt8 uOnOff = 0;

	// Compute filter coefficients (bypassed if no changes)

	EQModuleParams * pFilterParams;
	pFilterParams = m_pSharedParams->GetFilterModuleParams( BAND1 );
	if ( pFilterParams->bOnOff ) uOnOff |= 1<<BAND1;
	if ( m_pSharedParams->GetDirty(BAND1) )
	{
		ComputeBiquadCoefs( BAND1, pFilterParams );
		m_pSharedParams->SetDirty( BAND1, false );
	}
	pFilterParams = m_pSharedParams->GetFilterModuleParams( BAND2 );
	if ( pFilterParams->bOnOff ) uOnOff |= 1<<BAND2;
	if ( m_pSharedParams->GetDirty(BAND2) )
	{
		ComputeBiquadCoefs( BAND2, pFilterParams );
		m_pSharedParams->SetDirty( BAND2, false );
	}
	pFilterParams = m_pSharedParams->GetFilterModuleParams( BAND3 );
	if ( pFilterParams->bOnOff ) uOnOff |= 1<<BAND3;
	if ( m_pSharedParams->GetDirty(BAND3) )
	{		
		ComputeBiquadCoefs( BAND3, pFilterParams );
		m_pSharedParams->SetDirty( BAND3, false );
	}
	
	AkReal32 fTargetGain = m_pSharedParams->GetOutputLevel( );

	m_DspProcess.ResetDspProcess( true );
	m_DspProcess.SetDspProcess( ParametricEQFXJobBin );

	AkUInt32 uFiltMemSize = AK_ALIGN_SIZE_FOR_DMA( sizeof(AkReal32)* NUMBER_FILTER_MODULES * NUMBER_FILTER_MEMORY * m_uNumProcessedChannels );

	// Send PluginAudioBuffer information
	// Watchout: uValidFrames may not be right at this time. It will be before the DMA actually goes to SPU.
	m_DspProcess.AddDspProcessSmallDma( io_pBuffer, sizeof(AkAudioBuffer) );
	// Send filter coefficients and memories
	m_DspProcess.AddDspProcessSmallDma( m_pfFiltCoefs, AK_ALIGN_SIZE_FOR_DMA( sizeof( m_pfFiltCoefs ) ) );
	m_DspProcess.AddDspProcessSmallDma( m_pfAllocatedMem, uFiltMemSize );

	// Note: Subsequent DMAs are contiguous in memory on SPU side. 
	// Note: LFE channel (last), may not be sent if ProcessLFE was selected and m_uNumProcessedChannels decremented.
	m_DspProcess.AddDspProcessDma( io_pBuffer->GetDataStartDMA(), sizeof(AkReal32) * io_pBuffer->MaxFrames() * m_uNumProcessedChannels );
	
	m_DspProcess.SetUserData( 6, (AkUInt64)uOnOff );
 	m_DspProcess.SetUserData( 7, *((AkUInt32 *) &m_fCurrentGain) );
	m_DspProcess.SetUserData( 8, *((AkUInt32 *) &fTargetGain) );
	m_DspProcess.SetUserData( 9, m_uNumProcessedChannels );

	out_pDspProcess = &m_DspProcess;

	m_fCurrentGain = fTargetGain;
}

#else
//  Execute parametric EQ effect.
void CAkParametricEQFX::Execute( AkAudioBuffer* io_pBuffer	)
{
	if ( m_uNumProcessedChannels == 0 || io_pBuffer->uValidFrames == 0 )
		return;

	AK_PERF_RECORDING_START( "ParametricEQ", 25, 30 );

	// Compute filter coefficients (bypassed if no changes)
	bool bOnOff[ NUMBER_FILTER_MODULES ];

	EQModuleParams * pFilterParams;
	pFilterParams = m_pSharedParams->GetFilterModuleParams( BAND1 );
	bOnOff[ BAND1 ] = pFilterParams->bOnOff;
	if ( m_pSharedParams->GetDirty(BAND1) )
	{
		ComputeBiquadCoefs( BAND1, pFilterParams );
		m_pSharedParams->SetDirty( BAND1, false );
	}
	pFilterParams = m_pSharedParams->GetFilterModuleParams( BAND2 );
	bOnOff[ BAND2 ] = pFilterParams->bOnOff;
	if ( m_pSharedParams->GetDirty(BAND2) )
	{
		ComputeBiquadCoefs( BAND2, pFilterParams );
		m_pSharedParams->SetDirty( BAND2, false );
	}
	pFilterParams = m_pSharedParams->GetFilterModuleParams( BAND3 );
	bOnOff[ BAND3 ] = pFilterParams->bOnOff;
	if ( m_pSharedParams->GetDirty(BAND3) )
	{		
		ComputeBiquadCoefs( BAND3, pFilterParams );
		m_pSharedParams->SetDirty( BAND3, false );
	}
	
	AkReal32 fTargetGain = m_pSharedParams->GetOutputLevel( );

	if ( bOnOff[BAND1] )
		Process( io_pBuffer, m_pfFiltCoefs[BAND1], &m_pfAllocatedMem[0], m_uNumProcessedChannels );
	if ( bOnOff[BAND2] )
		Process( io_pBuffer, m_pfFiltCoefs[BAND2], &m_pfAllocatedMem[BAND2*NUMBER_FILTER_MEMORY*m_uNumProcessedChannels], m_uNumProcessedChannels );
	if ( bOnOff[BAND3] )
		Process( io_pBuffer, m_pfFiltCoefs[BAND3], &m_pfAllocatedMem[BAND3*NUMBER_FILTER_MEMORY*m_uNumProcessedChannels], m_uNumProcessedChannels );
	
	AK::DSP::ApplyGain( io_pBuffer, m_fCurrentGain, fTargetGain, m_pSharedParams->GetProcessLFE( ) );
	m_fCurrentGain = fTargetGain;

	AK_PERF_RECORDING_STOP( "ParametricEQ", 25, 30 );
}
#endif

void CAkParametricEQFX::ComputeBiquadCoefs( AkBandNumber in_eBandNum, 
											EQModuleParams * in_sModuleParams )
{
	AkReal32 fb0,fb1,fb2;	// Feed forward coefficients
	AkReal32 fa0,fa1,fa2;	// Feed back coefficients

	// Note: Q and "bandwidth" linked by:
    // 1/Q = 2*sinh(ln(2)/2*BandWidth*fOmega/sin(fOmega))

	AkReal32 fFrequency = in_sModuleParams->fFrequency;
	
	// Frequency must be less or equal to the half of the sample rate
	AkReal32 fMaxFrequency = m_uSampleRate * 0.5f * 0.9f;
	if ( fFrequency >= fMaxFrequency )
		fFrequency = fMaxFrequency; 

	switch ( in_sModuleParams->eFilterType )
	{
	case AKFILTERTYPE_LOWPASS:
		{
			// Butterworth low pass for flat pass band
			AkReal32 PiFcOSr	= PI * fFrequency / m_uSampleRate;
			AkReal32 fTanPiFcSr = tanf(PiFcOSr);
			AkReal32 fIntVal	= 1.0f / fTanPiFcSr;
			AkReal32 fRootTwoxIntVal	= SQRT2 * fIntVal;
			AkReal32 fSqIntVal =	fIntVal	* fIntVal;
			AkReal32 fOnePlusSqIntVal = 1.f + fSqIntVal;
			// Coefficient formulas
			fb0 = (1.0f / ( fRootTwoxIntVal + fOnePlusSqIntVal));
			fb1 = 2.f * fb0;
			fb2 = fb0;
			fa0 = 1.f;
			fa1 = fb1 * ( 1.0f - fSqIntVal);
			fa2 = ( fOnePlusSqIntVal - fRootTwoxIntVal) * fb0;
			break;
		}
	case AKFILTERTYPE_HIPASS:
		{
			// Butterworth high pass for flat pass band
			AkReal32 PiFcOSr	= PI * fFrequency / m_uSampleRate;
			AkReal32 fTanPiFcSr = tanf(PiFcOSr);
			AkReal32 fRootTwoxIntVal	= SQRT2 * fTanPiFcSr;
			AkReal32 fSqIntVal =	fTanPiFcSr	* fTanPiFcSr;
			AkReal32 fOnePlusSqIntVal = 1.f + fSqIntVal;
			// Coefficient formulas
			fb0 = (1.0f / ( fRootTwoxIntVal + fOnePlusSqIntVal));
			fb1 = -2.f * fb0;
			fb2 = fb0;
			fa0 = 1.f;
			fa1 = -fb1 * ( fSqIntVal - 1.0f );
			fa2 = ( fOnePlusSqIntVal - fRootTwoxIntVal ) * fb0;
			break;
		}
	case AKFILTERTYPE_BANDPASS:
		{
			AkReal32 fOmega = 2.f * PI * fFrequency / m_uSampleRate;	// Normalized angular frequency
			AkReal32 fCosOmega = cosf(fOmega);											// Cos omega
			AkReal32 fSinOmega = sinf(fOmega);											// Sin omega
			// 0 dB peak (normalized passband gain)
			// alpha = sin(w0)/(2*Q)
			AkReal32 fAlpha = fSinOmega/(2.f*in_sModuleParams->fQFactor);
			// Coefficient formulas
            fb0 = fAlpha;
            fb1 = 0.f;
            fb2 = -fAlpha;
            fa0 = 1.f + fAlpha;
            fa1 = -2.f*fCosOmega;
            fa2 = 1.f - fAlpha;    
			break;
		}
	case AKFILTERTYPE_NOTCH:
		{
			AkReal32 fOmega = 2.f * PI * fFrequency / m_uSampleRate;	// Normalized angular frequency
			AkReal32 fCosOmega = cosf(fOmega);											// Cos omega
			AkReal32 fSinOmega = sinf(fOmega);											// Sin omega
			// Normalized passband gain
			AkReal32 fAlpha = fSinOmega/(2.f*in_sModuleParams->fQFactor);
			// Coefficient formulas
			fb0 = 1.f;
            fb1 = -2.f*fCosOmega;
            fb2 = 1.f;
            fa0 = 1.f + fAlpha;
            fa1 = fb1;
            fa2 = 1.f - fAlpha;
			break;
		}
	case AKFILTERTYPE_LOWSHELF:
		{
			AkReal32 fLinAmp = powf(10.f,in_sModuleParams->fGain*0.025f);
			AkReal32 fOmega = 2.f * PI * fFrequency / m_uSampleRate;	// Normalized angular frequency
			AkReal32 fAlpha = sinf(fOmega)/2.f * sqrtf( (fLinAmp + 1.f/fLinAmp)*(1.f/SHELFSLOPE - 1.f) + 2.f );
			AkReal32 fCosOmega = cosf(fOmega);
			AkReal32 fLinAmpPlusOne = fLinAmp+1.f;
			AkReal32 fLinAmpMinusOne = fLinAmp-1.f;
			AkReal32 fTwoSqrtATimesAlpha = 2.f*sqrt(fLinAmp)*fAlpha;
			AkReal32 fLinAmpMinusOneTimesCosOmega = fLinAmpMinusOne*fCosOmega;
			AkReal32 fLinAmpPlusOneTimesCosOmega = fLinAmpPlusOne*fCosOmega;

			fb0 = fLinAmp*( fLinAmpPlusOne - fLinAmpMinusOneTimesCosOmega + fTwoSqrtATimesAlpha );
            fb1 = 2.f*fLinAmp*( fLinAmpMinusOne - fLinAmpPlusOneTimesCosOmega );
            fb2 = fLinAmp*( fLinAmpPlusOne - fLinAmpMinusOneTimesCosOmega - fTwoSqrtATimesAlpha );
            fa0 = fLinAmpPlusOne + fLinAmpMinusOneTimesCosOmega + fTwoSqrtATimesAlpha;
            fa1 = -2.f*( fLinAmpMinusOne + fLinAmpPlusOneTimesCosOmega );
            fa2 = fLinAmpPlusOne + fLinAmpMinusOneTimesCosOmega - fTwoSqrtATimesAlpha;
			break;
		}
	case AKFILTERTYPE_HISHELF:
		{
			AkReal32 fLinAmp = powf(10.f,in_sModuleParams->fGain*0.025f);
			AkReal32 fOmega = 2.f * PI * fFrequency / m_uSampleRate;	// Normalized angular frequency
			AkReal32 fAlpha = sinf(fOmega)/2.f * sqrtf( (fLinAmp + 1.f/fLinAmp)*(1.f/SHELFSLOPE - 1.f) + 2.f );
			AkReal32 fCosOmega = cosf(fOmega);
			AkReal32 fLinAmpPlusOne = fLinAmp+1.f;
			AkReal32 fLinAmpMinusOne = fLinAmp-1.f;
			AkReal32 fTwoSqrtATimesAlpha = 2.f*sqrt(fLinAmp)*fAlpha;
			AkReal32 fLinAmpMinusOneTimesCosOmega = fLinAmpMinusOne*fCosOmega;
			AkReal32 fLinAmpPlusOneTimesCosOmega = fLinAmpPlusOne*fCosOmega;

			fb0 = fLinAmp*( fLinAmpPlusOne + fLinAmpMinusOneTimesCosOmega + fTwoSqrtATimesAlpha );
            fb1 = -2.f*fLinAmp*( fLinAmpMinusOne + fLinAmpPlusOneTimesCosOmega );
            fb2 = fLinAmp*( fLinAmpPlusOne + fLinAmpMinusOneTimesCosOmega - fTwoSqrtATimesAlpha );
            fa0 = fLinAmpPlusOne - fLinAmpMinusOneTimesCosOmega + fTwoSqrtATimesAlpha;
            fa1 = 2.f*( fLinAmpMinusOne - fLinAmpPlusOneTimesCosOmega );
            fa2 = fLinAmpPlusOne - fLinAmpMinusOneTimesCosOmega - fTwoSqrtATimesAlpha;
			break;
		}
	case AKFILTERTYPE_PEAKINGEQ:
		{
			AkReal32 fOmega = 2.f * PI * fFrequency / m_uSampleRate;	// Normalized angular frequency
			AkReal32 fCosOmega = cosf(fOmega);											// Cos omega
			AkReal32 fLinAmp = powf(10.f,in_sModuleParams->fGain*0.025f);
			// alpha = sin(w0)/(2*Q)
			AkReal32 fAlpha = sinf(fOmega)/(2.f*in_sModuleParams->fQFactor);
			// Coefficient formulas
			fb0 = 1.f + fAlpha*fLinAmp;
			fb1 = -2.f*fCosOmega;
			fb2 = 1.f - fAlpha*fLinAmp;
			fa0 = 1.f + fAlpha/fLinAmp;
			fa1 = -2.f*fCosOmega;
			fa2 = 1.f - fAlpha/fLinAmp;
			break;
		}
	default:
		AKASSERT( !"Invalid filter type." );
	}

	// Normalize all 6 coefficients into 5 and flip sign of recursive coefficients 
	// to add in difference equation instead
	// Note keep gain separate from fb0 coefficients to avoid 
	// recomputing coefficient on output volume changes
	m_pfFiltCoefs[in_eBandNum][0] = fb0/fa0;
	m_pfFiltCoefs[in_eBandNum][1] = fb1/fa0;
	m_pfFiltCoefs[in_eBandNum][2] = fb2/fa0;
	m_pfFiltCoefs[in_eBandNum][3] = -fa1/fa0;
	m_pfFiltCoefs[in_eBandNum][4] = -fa2/fa0;
}
