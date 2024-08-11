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

#include "AkRoomVerbFX.h"
#include <AK/Tools/Common/AkAssert.h>
#include <math.h>
#include <AK/Tools/Common/AkPlatformFuncs.h>
#include "AlgoTunings.h"
#include "DelayLengths.h"
#include "ReverbState.h"
#include "ConstantPowerChannelMixdown.h"
#include "Mix3Interp.h"
#include <AK/DSP/AkApplyGain.h>
#include "BiquadFilter.h"
#include "FDN4.h"
#include "ERTables.h"
#include <AK/Tools/Common/AkObject.h>

using namespace DSP;
using namespace AK::DSP;

#if !defined(AK_PS3) && defined(_DEBUG)
//#define MEMORYOUTPUT
#endif
#if defined(MEMORYOUTPUT)
#include <tchar.h> //test traces
#endif

#ifdef AK_PS3
// embedded SPU Job Binary symbols
extern char _binary_AkRoomVerbFXJob_spu_bin_start[];
extern char _binary_AkRoomVerbFXJob_spu_bin_size[];
extern char _binary_AkRoomVerbFXJob2_spu_bin_start[];
extern char _binary_AkRoomVerbFXJob2_spu_bin_size[];
extern char _binary_AkRoomVerbFXJob3_spu_bin_start[];
extern char _binary_AkRoomVerbFXJob3_spu_bin_size[];
static AK::MultiCoreServices::BinData AkRoomVerbFXJobBin = { _binary_AkRoomVerbFXJob_spu_bin_start, CELL_SPURS_GET_SIZE_BINARY( _binary_AkRoomVerbFXJob_spu_bin_size ) };
static AK::MultiCoreServices::BinData AkRoomVerbFXJobBin2 = { _binary_AkRoomVerbFXJob2_spu_bin_start, CELL_SPURS_GET_SIZE_BINARY( _binary_AkRoomVerbFXJob2_spu_bin_size ) };
static AK::MultiCoreServices::BinData AkRoomVerbFXJobBin3 = { _binary_AkRoomVerbFXJob3_spu_bin_start, CELL_SPURS_GET_SIZE_BINARY( _binary_AkRoomVerbFXJob3_spu_bin_size ) };
#endif

// Plugin mechanism. Dynamic create function whose address must be registered to the FX manager.
AK::IAkPlugin* CreateRoomVerbFX( AK::IAkPluginMemAlloc * in_pAllocator )
{
	return AK_PLUGIN_NEW( in_pAllocator, CAkRoomVerbFX( ) );
}


// Constructor.
CAkRoomVerbFX::CAkRoomVerbFX()
{
	m_pParams = NULL;
	m_pAllocator = NULL;
	m_pReverbUnitsState = NULL;
	m_pTCFiltersState = NULL;
	m_pERUnit = NULL;
	AKPLATFORM::AkMemSet( &m_PrevRTPCParams, 0, sizeof(m_PrevRTPCParams) );
	AKPLATFORM::AkMemSet( &m_PrevInvariantParams, 0, sizeof(m_PrevInvariantParams) );
}

// Destructor.
CAkRoomVerbFX::~CAkRoomVerbFX()
{
	
}

// Initializes and allocate memory for the effect
AKRESULT CAkRoomVerbFX::Init(	AK::IAkPluginMemAlloc * in_pAllocator,		// Memory allocator interface.
								AK::IAkEffectPluginContext * in_pFXCtx,		// FX Context
								AK::IAkPluginParam * in_pParams,			// Effect parameters.
								AkAudioFormat &	in_rFormat					// Required audio input format.
							   )
{
#define RETURNIFNOTSUCCESS( __FONCTIONEVAL__ )	\
{												\
	AKRESULT __result__ = (__FONCTIONEVAL__);	\
	if ( __result__ != AK_Success )				\
		return __result__;						\
}												\

	m_Reverb.uSampleRate = in_rFormat.uSampleRate;	
	m_pParams = static_cast<CAkRoomVerbFXParams*>(in_pParams);
	m_pAllocator = in_pAllocator;

	m_Reverb.uNumReverbUnits = m_pParams->sInvariantParams.uNumReverbUnits;
	AKASSERT( m_Reverb.uNumReverbUnits % 2 == 0 );

	m_Reverb.bIsSentMode = in_pFXCtx->IsSendModeEffect();
	// Disable dry level for sent mode path (wet still active for flexibility)
	if ( m_Reverb.bIsSentMode )
		m_pParams->sRTPCParams.fDryLevel = 0.f;

	// Cache value initial RTPC values
	m_PrevRTPCParams = m_pParams->sRTPCParams;
#ifndef AK_OPTIMIZED
	m_PrevInvariantParams = m_pParams->sInvariantParams;
#endif

	// Compute the number of filters required
	m_Reverb.uNumERSignals = m_pParams->sInvariantParams.bEnableEarlyReflections ? 2 : 0;
	if ( in_rFormat.GetChannelMask() == AK_SPEAKER_SETUP_0POINT1 )
		m_Reverb.uNumERSignals = 0;

	SetupDCFilters( );

	RETURNIFNOTSUCCESS( SetupToneControlFilters( ) );

	RETURNIFNOTSUCCESS( SetupFDNs(in_pAllocator) );

	RETURNIFNOTSUCCESS( SetupERDelay(in_pAllocator) );

	RETURNIFNOTSUCCESS( SetupReverbDelay(in_pAllocator) );

	RETURNIFNOTSUCCESS( SetupERUnit(in_pAllocator) );

	RETURNIFNOTSUCCESS( SetupERFrontBackDelay(in_pAllocator,in_rFormat.GetChannelMask()) );

	RETURNIFNOTSUCCESS( SetupDiffusionAPF(in_pAllocator) );
	
	m_Reverb.uTailLength = (AkUInt32)( ( m_pParams->sInvariantParams.fReverbDelay / 1000.0f + m_pParams->sRTPCParams.fDecayTime ) * m_Reverb.uSampleRate );

#if defined(MEMORYOUTPUT)
	AkReal32 fModalDensity,fEchoDensity;
	AkReal32 fERLongestTapTime = 0.f;
	if ( m_pParams->sInvariantParams.bEnableEarlyReflections )
	{
		AkUInt32 uERPattern = m_pParams->sInvariantParams.uERPattern;
		AkUInt32 uLongestTapIndexL = AkRoomVerb::g_ERPatternsList[uERPattern].uNumTapsLeft-1;
		AkReal32 fLongestTapL = AkRoomVerb::g_ERPatternsList[uERPattern].TapInfoLeft[uLongestTapIndexL].fTapTime;
		AkUInt32 uLongestTapIndexR = AkRoomVerb::g_ERPatternsList[uERPattern].uNumTapsRight-1;
		AkReal32 fLongestTapR = AkRoomVerb::g_ERPatternsList[uERPattern].TapInfoRight[uLongestTapIndexR].fTapTime;
		AkReal32 fRoomSizeScaleFactor = pow( 2.f, m_pParams->sInvariantParams.fRoomSize/100.f );
		fERLongestTapTime = AK_FPMax( fLongestTapL, fLongestTapR ) * fRoomSizeScaleFactor;
	}
	AkUInt32 uMemUsed = DelayLengths::ComputeMemoryStats(	m_pParams->sInvariantParams.fDensity, 
															m_pParams->sInvariantParams.fRoomShape, 
															m_Reverb.uNumReverbUnits, 
															fERLongestTapTime,
															m_pParams->sInvariantParams.fReverbDelay, 
															m_pParams->sInvariantParams.fERFrontBackDelay,
															m_Reverb.uSampleRate,
															m_pParams->sAlgoTunings,
															&fModalDensity,
															&fEchoDensity );
	TCHAR str[256];
	swprintf_s(str, 256, L"\nRoomVerb statistics\n" );
	OutputDebugString( str );
	swprintf_s(str, 256, L"Modal density: %f\n", fModalDensity );
	OutputDebugString( str );
	swprintf_s(str, 256, L"Echo density: %f\n", fEchoDensity );
	OutputDebugString( str );
	swprintf_s(str, 256, L"Memory used: %d\n", uMemUsed );
	OutputDebugString( str );
#endif

	AK_PERF_RECORDING_RESET();

	return AK_Success;
}

void CAkRoomVerbFX::SetupDCFilters()
{
	// Need to setup all in case stereo width shifts to different Execute routine
	for ( AkUInt32 i = 0; i < MAXNUMOUTPUTSIGNALS; i++ )
	{
		m_Reverb.DCFilter[i].ComputeCoefs( m_pParams->sAlgoTunings.fDCFilterCutFreq, m_Reverb.uSampleRate );
	}
}

void CAkRoomVerbFX::ComputeTCCoefs1( )
{
	// Compute coefs for filter 1
	if ( m_pParams->sInvariantParams.bEnableToneControls && 
		(m_pParams->sInvariantParams.eFilter1Pos != FILTERINSERTTYPE_OFF) &&
		!((m_pParams->sInvariantParams.eFilter1Pos == FILTERINSERTTYPE_ERONLY) && (m_Reverb.uNumERSignals == 0)) )
	{
		m_pTCFiltersState[m_Reverb.uTCFilterIndex[1]].Filter.ComputeCoefs(	(DSP::BiquadFilterMono::FilterType)m_pParams->sInvariantParams.eFilter1Curve, 
																			(AkReal32)m_Reverb.uSampleRate, 
																			m_pParams->sRTPCParams.fFilter1Freq, 
																			m_pParams->sRTPCParams.fFilter1Gain,
																			m_pParams->sRTPCParams.fFilter1Q );
		m_pTCFiltersState[m_Reverb.uTCFilterIndex[1]].FilterPos = m_pParams->sInvariantParams.eFilter1Pos;
		if ( (m_pParams->sInvariantParams.eFilter1Pos == FILTERINSERTTYPE_ERONLY) && (m_Reverb.uNumERSignals == 2) )
		{
			m_pTCFiltersState[m_Reverb.uTCFilterIndex[1]+1].Filter.ComputeCoefs(	(DSP::BiquadFilterMono::FilterType)m_pParams->sInvariantParams.eFilter1Curve, 
																					(AkReal32)m_Reverb.uSampleRate, 
																					m_pParams->sRTPCParams.fFilter1Freq, 
																					m_pParams->sRTPCParams.fFilter1Gain,
																					m_pParams->sRTPCParams.fFilter1Q );
			m_pTCFiltersState[m_Reverb.uTCFilterIndex[1]+1].FilterPos = m_pParams->sInvariantParams.eFilter1Pos;
		}
	}
}

void CAkRoomVerbFX::ComputeTCCoefs2( )
{
	// Compute coefs for filter 2
	if ( m_pParams->sInvariantParams.bEnableToneControls && 
		(m_pParams->sInvariantParams.eFilter2Pos != FILTERINSERTTYPE_OFF) &&
		!((m_pParams->sInvariantParams.eFilter2Pos == FILTERINSERTTYPE_ERONLY) && (m_Reverb.uNumERSignals == 0)) )
	{
		m_pTCFiltersState[m_Reverb.uTCFilterIndex[2]].Filter.ComputeCoefs(	(DSP::BiquadFilterMono::FilterType)m_pParams->sInvariantParams.eFilter2Curve, 
																			(AkReal32)m_Reverb.uSampleRate, 
																			m_pParams->sRTPCParams.fFilter2Freq, 
																			m_pParams->sRTPCParams.fFilter2Gain,
																			m_pParams->sRTPCParams.fFilter2Q );
		m_pTCFiltersState[m_Reverb.uTCFilterIndex[2]].FilterPos = m_pParams->sInvariantParams.eFilter2Pos;
		if ( (m_pParams->sInvariantParams.eFilter2Pos == FILTERINSERTTYPE_ERONLY) && (m_Reverb.uNumERSignals == 2) )
		{
			m_pTCFiltersState[m_Reverb.uTCFilterIndex[2]+1].Filter.ComputeCoefs(	(DSP::BiquadFilterMono::FilterType)m_pParams->sInvariantParams.eFilter2Curve, 
																					(AkReal32)m_Reverb.uSampleRate, 
																					m_pParams->sRTPCParams.fFilter2Freq, 
																					m_pParams->sRTPCParams.fFilter2Gain,
																					m_pParams->sRTPCParams.fFilter2Q );
			m_pTCFiltersState[m_Reverb.uTCFilterIndex[2]+1].FilterPos = m_pParams->sInvariantParams.eFilter2Pos;
		}
	}
}

void CAkRoomVerbFX::ComputeTCCoefs3( )
{
	// Compute coefs for filter 3
	if ( m_pParams->sInvariantParams.bEnableToneControls && 
		(m_pParams->sInvariantParams.eFilter3Pos != FILTERINSERTTYPE_OFF) &&
		!((m_pParams->sInvariantParams.eFilter3Pos == FILTERINSERTTYPE_ERONLY) && (m_Reverb.uNumERSignals == 0)) )
	{
		m_pTCFiltersState[m_Reverb.uTCFilterIndex[3]].Filter.ComputeCoefs(	(DSP::BiquadFilterMono::FilterType)m_pParams->sInvariantParams.eFilter3Curve, 
																			(AkReal32)m_Reverb.uSampleRate, 
																			m_pParams->sRTPCParams.fFilter3Freq, 
																			m_pParams->sRTPCParams.fFilter3Gain,
																			m_pParams->sRTPCParams.fFilter3Q );
		m_pTCFiltersState[m_Reverb.uTCFilterIndex[3]].FilterPos = m_pParams->sInvariantParams.eFilter3Pos;
		if ( (m_pParams->sInvariantParams.eFilter3Pos == FILTERINSERTTYPE_ERONLY) && (m_Reverb.uNumERSignals == 2) )
		{
			m_pTCFiltersState[m_Reverb.uTCFilterIndex[3]+1].Filter.ComputeCoefs(	(DSP::BiquadFilterMono::FilterType)m_pParams->sInvariantParams.eFilter3Curve, 
																					(AkReal32)m_Reverb.uSampleRate, 
																					m_pParams->sRTPCParams.fFilter3Freq, 
																					m_pParams->sRTPCParams.fFilter3Gain,
																					m_pParams->sRTPCParams.fFilter3Q );
			m_pTCFiltersState[m_Reverb.uTCFilterIndex[3]+1].FilterPos = m_pParams->sInvariantParams.eFilter3Pos;
		}
	}
}

AKRESULT CAkRoomVerbFX::SetupToneControlFilters( )
{	
	AkUInt32 uTotalFilters = 0;
	if ( m_pParams->sInvariantParams.bEnableToneControls )
	{
		m_Reverb.uTCFilterIndex[1] = 0;
		switch (m_pParams->sInvariantParams.eFilter1Pos)
		{
		case FILTERINSERTTYPE_REVERBONLY:
		case FILTERINSERTTYPE_ERANDREVERB:
			uTotalFilters += 1;
			break;
		case FILTERINSERTTYPE_ERONLY:
			uTotalFilters += m_Reverb.uNumERSignals;
			break;
		}
		
		m_Reverb.uTCFilterIndex[2] = (AkUInt8)uTotalFilters;
		switch (m_pParams->sInvariantParams.eFilter2Pos)
		{
		case FILTERINSERTTYPE_REVERBONLY:
		case FILTERINSERTTYPE_ERANDREVERB:
			uTotalFilters += 1;
			break;
		case FILTERINSERTTYPE_ERONLY:
			uTotalFilters += m_Reverb.uNumERSignals;
			break;
		}

		m_Reverb.uTCFilterIndex[3] = (AkUInt8)uTotalFilters;
		switch (m_pParams->sInvariantParams.eFilter3Pos)
		{
		case FILTERINSERTTYPE_REVERBONLY:
		case FILTERINSERTTYPE_ERANDREVERB:
			uTotalFilters += 1;
			break;
		case FILTERINSERTTYPE_ERONLY:
			uTotalFilters += m_Reverb.uNumERSignals;
			break;
		}
	}

	AKASSERT( uTotalFilters <= 6 );
	m_Reverb.uTCFilterIndex[0] = (AkUInt8)uTotalFilters;

	// Allocate filters
	if ( uTotalFilters )
	{
		m_pTCFiltersState = (ToneControlsState*) AK_PLUGIN_ALLOC( m_pAllocator, AK_ALIGN_SIZE_FOR_DMA( sizeof(ToneControlsState) * uTotalFilters ) );
		if ( !m_pTCFiltersState )
			return AK_InsufficientMemory;

		for ( AkUInt32 i = 0; i < uTotalFilters; i++ )
		{
			AkPlacementNew( &m_pTCFiltersState[i]) ToneControlsState();
		}
	}

	ComputeTCCoefs1( );
	ComputeTCCoefs2( );
	ComputeTCCoefs3( );

	return AK_Success;
}

AKRESULT CAkRoomVerbFX::SetupERDelay( AK::IAkPluginMemAlloc * in_pAllocator )
{
	AKRESULT eResult = AK_Success;
	if ( m_Reverb.uNumERSignals )
	{
		AkReal32 fRoomSizeScaleFactor = pow( 2.f, m_pParams->sInvariantParams.fRoomSize/100.f );
		AkUInt32 uERPatternSel = m_pParams->sInvariantParams.uERPattern;
		AKASSERT( AkRoomVerb::g_ERPatternsList[uERPatternSel].uNumTapsLeft );
		AkReal32 fMinTapTimeL = AkRoomVerb::g_ERPatternsList[uERPatternSel].TapInfoLeft[0].fTapTime*fRoomSizeScaleFactor;
		AKASSERT( AkRoomVerb::g_ERPatternsList[uERPatternSel].uNumTapsRight );
		AkReal32 fMinTapTimeR = AkRoomVerb::g_ERPatternsList[uERPatternSel].TapInfoRight[0].fTapTime*fRoomSizeScaleFactor;
		AkReal32 fMintapTime = AK_FPMin( fMinTapTimeL, fMinTapTimeR );
		AkUInt32 uERDelayLength = (AkUInt32) (fMintapTime/1000.f * m_Reverb.uSampleRate);
		if ( uERDelayLength  > 0 )
		{	
			eResult = m_Reverb.ERDelay.Init( in_pAllocator, uERDelayLength  );
		}
	}
	return eResult;
}

AKRESULT CAkRoomVerbFX::SetupReverbDelay( AK::IAkPluginMemAlloc * in_pAllocator )
{
	AKRESULT eResult = AK_Success;
	AkUInt32 uReverbDelayLength = (AkUInt32) (m_pParams->sInvariantParams.fReverbDelay/1000.f * m_Reverb.uSampleRate);
	if ( uReverbDelayLength > 0 )
	{	
		eResult = m_Reverb.ReverbDelay.Init( in_pAllocator, uReverbDelayLength );
	}
	return eResult;
}

AKRESULT CAkRoomVerbFX::SetupERFrontBackDelay( AK::IAkPluginMemAlloc * in_pAllocator, AkChannelMask in_uChannelMask )
{
	if ( m_Reverb.uNumERSignals )
	{
		AkUInt32 uERFBDelayLength = (AkUInt32) (m_pParams->sInvariantParams.fERFrontBackDelay/1000.f * m_Reverb.uSampleRate);
		if ( uERFBDelayLength > 0 )
		{	
			if ( in_uChannelMask & AK_SPEAKER_BACK_LEFT )
			{
				AKRESULT eResult = m_Reverb.ERFrontBackDelay[0].Init( in_pAllocator, uERFBDelayLength );
				if ( eResult != AK_Success )
					return eResult;
			}
			if ( in_uChannelMask & AK_SPEAKER_BACK_RIGHT )
			{
				AKRESULT eResult = m_Reverb.ERFrontBackDelay[1].Init( in_pAllocator, uERFBDelayLength );
				if ( eResult != AK_Success )
					return eResult;
			}
		}
	}
	return AK_Success;
}

AKRESULT CAkRoomVerbFX::SetupERUnit( AK::IAkPluginMemAlloc * in_pAllocator )
{
	AKRESULT eResult = AK_Success;
	AkUInt32 uERPatternSel = m_pParams->sInvariantParams.uERPattern;
	if ( m_Reverb.uNumERSignals )
	{
		m_pERUnit = AK_PLUGIN_NEW( in_pAllocator, ERUnitDual() );
		if ( !m_pERUnit )
			return AK_InsufficientMemory;
		eResult = m_pERUnit->Init(	in_pAllocator, 
								m_pParams->sInvariantParams.fRoomSize,
								AkRoomVerb::g_ERPatternsList[uERPatternSel].TapInfoLeft, 
								AkRoomVerb::g_ERPatternsList[uERPatternSel].TapInfoRight, 
								AkRoomVerb::g_ERPatternsList[uERPatternSel].uNumTapsLeft, 	
								AkRoomVerb::g_ERPatternsList[uERPatternSel].uNumTapsRight, 
								m_Reverb.uSampleRate );
	}

	return eResult;
}

AKRESULT CAkRoomVerbFX::SetupFDNs( AK::IAkPluginMemAlloc * in_pAllocator )
{
	m_pReverbUnitsState = (ReverbUnitState*) AK_PLUGIN_ALLOC( in_pAllocator, AK_ALIGN_SIZE_FOR_DMA( sizeof(ReverbUnitState) * m_Reverb.uNumReverbUnits ) );
	if ( !m_pReverbUnitsState )
		return AK_InsufficientMemory;

	for ( AkUInt32 i = 0; i < m_Reverb.uNumReverbUnits; i++ )
	{
		AkPlacementNew( &m_pReverbUnitsState[i]) ReverbUnitState();
	}

	// Compute delay times based on parameters and tunings
	m_Reverb.fReverbUnitsMixGain = 1.f;
	if ( m_Reverb.uNumReverbUnits > 1 )
		m_Reverb.fReverbUnitsMixGain = 1.f/sqrt((AkReal32)m_Reverb.uNumReverbUnits); // Constant power mix

	AkReal32 fFDNDelayTimes[MAXNUMREVERBUNITS*4];
	DelayLengths::ComputeFDNDelayTimes(	m_pParams->sInvariantParams.fDensity, 
										m_pParams->sInvariantParams.fRoomShape, 
										m_Reverb.uNumReverbUnits*4, 
										m_pParams->sAlgoTunings,
										fFDNDelayTimes );
														
	// Convert to prime delay line lengths
	AkUInt32 uFDNDelayLengths[MAXNUMREVERBUNITS*4];
	DelayLengths::ComputePrimeDelayLengths( fFDNDelayTimes, 
											m_Reverb.uSampleRate,
											m_Reverb.uNumReverbUnits*4,
											uFDNDelayLengths );
	
	// Reorder so that each FDN has delays of a wider range of sizes
	AkUInt32 uReorderedFDNDelayLengths[MAXNUMREVERBUNITS*4];
	for ( AkUInt32 i = 0; i < m_Reverb.uNumReverbUnits; i++ )
	{
		uReorderedFDNDelayLengths[i*4] = uFDNDelayLengths[i];
		uReorderedFDNDelayLengths[i*4+1] = uFDNDelayLengths[m_Reverb.uNumReverbUnits+i];
		uReorderedFDNDelayLengths[i*4+2] = uFDNDelayLengths[2*m_Reverb.uNumReverbUnits+i];
		uReorderedFDNDelayLengths[i*4+3] = uFDNDelayLengths[3*m_Reverb.uNumReverbUnits+i];
	}

	// Determine reverb units input delay lengths
	AkReal32 fFDNInputDelayTimes[MAXNUMREVERBUNITS];
	DelayLengths::ComputeFDNInputDelayTimes( m_Reverb.uNumReverbUnits, m_pParams->sAlgoTunings, fFDNInputDelayTimes );
	AkUInt32 uFDNInputDelayLengths[MAXNUMREVERBUNITS];
	for ( AkUInt32 i = 0; i < m_Reverb.uNumReverbUnits; i++ )
	{
		uFDNInputDelayLengths[i] = (AkUInt32)(fFDNInputDelayTimes[i]/1000.f*m_Reverb.uSampleRate);
	}

	// SETUP FDNs 
	for ( AkUInt32 i = 0; i < m_Reverb.uNumReverbUnits; i++ )
	{
		AKRESULT eResult = m_pReverbUnitsState[i].ReverbUnits.Init(	in_pAllocator, 
																	&uReorderedFDNDelayLengths[i*4], 
																	m_pParams->sRTPCParams.fDecayTime, 
																	m_pParams->sRTPCParams.fHFDamping, 
																	m_Reverb.uSampleRate );
		if ( eResult != AK_Success )
			return eResult;

		eResult = m_pReverbUnitsState[i].RUInputDelay.Init(in_pAllocator, uFDNInputDelayLengths[i] ); 
		if ( eResult != AK_Success )
			return eResult;
	}

	return AK_Success;
}

AKRESULT CAkRoomVerbFX::SetupDiffusionAPF( AK::IAkPluginMemAlloc * in_pAllocator )
{
	AkReal32 fAPFDelayTimes[NUMDIFFUSIONALLPASSFILTERS];
	DelayLengths::ComputeDiffusionFiltersDelayTimes(	NUMDIFFUSIONALLPASSFILTERS, 
														m_pParams->sAlgoTunings, 
														fAPFDelayTimes );

	AkUInt32 uAPFDelayLengths[NUMDIFFUSIONALLPASSFILTERS];
	DelayLengths::ComputePrimeDelayLengths( fAPFDelayTimes, 
											m_Reverb.uSampleRate, 
											NUMDIFFUSIONALLPASSFILTERS, 
											uAPFDelayLengths );

	for ( AkUInt32 i = 0; i < NUMDIFFUSIONALLPASSFILTERS; i++ )
	{
		AkReal32 fDiffusionAPFGain = ComputeDiffusionAPFGain( i, m_pParams->sRTPCParams.fDiffusion );
		AKRESULT eResult = m_Reverb.DiffusionFilters[i].Init(	in_pAllocator, 
																uAPFDelayLengths[i],
																fDiffusionAPFGain );
		if ( eResult != AK_Success )
			return eResult;
	}

	return AK_Success;
}

// Terminates.
AKRESULT CAkRoomVerbFX::Term( AK::IAkPluginMemAlloc * in_pAllocator )
{
	m_Reverb.ERDelay.Term( in_pAllocator );
	m_Reverb.ReverbDelay.Term( in_pAllocator );
	m_Reverb.ERFrontBackDelay[0].Term( in_pAllocator );
	m_Reverb.ERFrontBackDelay[1].Term( in_pAllocator );
	TermERUnit( in_pAllocator );
	TermToneControlFilters( in_pAllocator );
	TermFDNs( in_pAllocator );
	TermDiffusionAPF( in_pAllocator );

	AK_PLUGIN_DELETE( in_pAllocator, this );
	return AK_Success;
}

void CAkRoomVerbFX::TermToneControlFilters( AK::IAkPluginMemAlloc * in_pAllocator )
{
	if ( m_pTCFiltersState )
	{
		for ( AkUInt32 i = 0; i < m_Reverb.uTCFilterIndex[0]; i++ )
		{
			m_pTCFiltersState[i].~ToneControlsState();
		}

		AK_PLUGIN_FREE( in_pAllocator, m_pTCFiltersState );
		m_pTCFiltersState = NULL;
	}
}

void CAkRoomVerbFX::TermFDNs( AK::IAkPluginMemAlloc * in_pAllocator )
{
	if ( m_pReverbUnitsState )
	{
		for ( AkUInt32 i = 0; i < m_Reverb.uNumReverbUnits; i++ )
		{
			m_pReverbUnitsState[i].ReverbUnits.Term( in_pAllocator );
			m_pReverbUnitsState[i].RUInputDelay.Term( in_pAllocator );
			m_pReverbUnitsState[i].~ReverbUnitState();
		}

		AK_PLUGIN_FREE( in_pAllocator, m_pReverbUnitsState );
		m_pReverbUnitsState = NULL;
	}
}

void CAkRoomVerbFX::TermDiffusionAPF( AK::IAkPluginMemAlloc * in_pAllocator )
{
	for ( AkUInt32 i = 0; i < NUMDIFFUSIONALLPASSFILTERS; i++ )
	{
		m_Reverb.DiffusionFilters[i].Term( in_pAllocator ); 	
	}
}

void CAkRoomVerbFX::TermERUnit( AK::IAkPluginMemAlloc * in_pAllocator )
{
	if ( m_Reverb.uNumERSignals )
	{
		if ( m_pERUnit )
		{
			m_pERUnit->Term( in_pAllocator );
			AK_PLUGIN_DELETE( in_pAllocator, m_pERUnit );
			m_pERUnit = NULL;
		}
	}
}

// Reset state
AKRESULT CAkRoomVerbFX::Reset( )
{
	m_Reverb.ERDelay.Reset();
	m_Reverb.ReverbDelay.Reset();
	m_Reverb.ERFrontBackDelay[0].Reset();
	m_Reverb.ERFrontBackDelay[1].Reset();
	ResetERUnit();
	ResetDCFilters();
	ResetToneControlFilters();
	ResetFDNs();
	ResetDiffusionAPF();
	return AK_Success;
}

void CAkRoomVerbFX::ResetFDNs()
{
	if ( m_pReverbUnitsState )
	{
		for ( AkUInt32 i = 0; i < m_Reverb.uNumReverbUnits; i++ )
		{
			m_pReverbUnitsState[i].ReverbUnits.Reset();	
			m_pReverbUnitsState[i].RUInputDelay.Reset();
		}
	}
}

void CAkRoomVerbFX::ResetDiffusionAPF()
{
	for ( AkUInt32 i = 0; i < NUMDIFFUSIONALLPASSFILTERS; i++ )
	{
		m_Reverb.DiffusionFilters[i].Reset();
	}
}

void CAkRoomVerbFX::ResetDCFilters()
{
	for ( AkUInt32 i = 0; i < MAXNUMOUTPUTSIGNALS; i++ )
	{
		m_Reverb.DCFilter[i].Reset( );
	}
}

void CAkRoomVerbFX::ResetToneControlFilters()
{
	if ( m_pParams->sInvariantParams.bEnableToneControls )
	{
		for ( AkUInt32 i = 0; i < m_Reverb.uTCFilterIndex[0]; ++i )
		{
			m_pTCFiltersState[i].Filter.Reset();
		}
	}
}

void CAkRoomVerbFX::ResetERUnit()
{
	if ( m_pERUnit )
	{
		m_pERUnit->Reset();
	}
}

// Effect info query.
AKRESULT CAkRoomVerbFX::GetPluginInfo( AkPluginInfo & out_rPluginInfo )
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

bool CAkRoomVerbFX::LiveParametersUpdate( AkAudioBuffer* io_pBuffer )
{
	#define RETURNSTATEIFNOTSUCCESS( __result__ )	\
	if ( (__result__) != AK_Success )				\
	{												\
		return true;								\
	}												\

	if (	(m_PrevInvariantParams.uNumReverbUnits != m_pParams->sInvariantParams.uNumReverbUnits) ||
			(m_PrevInvariantParams.fRoomShape != m_pParams->sInvariantParams.fRoomShape) ||
			(m_PrevInvariantParams.fDensity != m_pParams->sInvariantParams.fDensity) )
	{
		TermFDNs( m_pAllocator );
		// Protect ourself from subsequent set of param from Wwise thread by using our own from now on
		m_Reverb.uNumReverbUnits = m_pParams->sInvariantParams.uNumReverbUnits;
		AKASSERT( m_Reverb.uNumReverbUnits % 2 == 0 );
		RETURNSTATEIFNOTSUCCESS( SetupFDNs( m_pAllocator ) );
		ResetFDNs( );
	}

	if ( m_PrevInvariantParams.bEnableEarlyReflections != m_pParams->sInvariantParams.bEnableEarlyReflections )
	{
		m_Reverb.ERFrontBackDelay[0].Term( m_pAllocator );
		m_Reverb.ERFrontBackDelay[1].Term( m_pAllocator );	
		m_Reverb.ERDelay.Term( m_pAllocator );
		TermERUnit( m_pAllocator );
		if ( m_pParams->sInvariantParams.bEnableToneControls &&
			((m_pParams->sInvariantParams.eFilter1Pos != FILTERINSERTTYPE_OFF) || 
			(m_pParams->sInvariantParams.eFilter2Pos != FILTERINSERTTYPE_OFF) ||
			(m_pParams->sInvariantParams.eFilter3Pos != FILTERINSERTTYPE_OFF)) )
		{
			TermToneControlFilters( m_pAllocator );
		}

		// Compute the number of filters required
		m_Reverb.uNumERSignals = m_pParams->sInvariantParams.bEnableEarlyReflections ? 2 : 0;
		if ( io_pBuffer->GetChannelMask()  == AK_SPEAKER_SETUP_0POINT1 )
			m_Reverb.uNumERSignals = 0;
	
		RETURNSTATEIFNOTSUCCESS( SetupERFrontBackDelay( m_pAllocator, io_pBuffer->GetChannelMask() ) );		
		m_Reverb.ERFrontBackDelay[0].Reset();
		m_Reverb.ERFrontBackDelay[1].Reset();
		RETURNSTATEIFNOTSUCCESS( SetupERDelay( m_pAllocator ) );
		m_Reverb.ERDelay.Reset();			
		RETURNSTATEIFNOTSUCCESS( SetupERUnit( m_pAllocator ) );
		ResetERUnit();

		if ( m_pParams->sInvariantParams.bEnableToneControls &&
			((m_pParams->sInvariantParams.eFilter1Pos != FILTERINSERTTYPE_OFF) || 
			(m_pParams->sInvariantParams.eFilter2Pos != FILTERINSERTTYPE_OFF) ||
			(m_pParams->sInvariantParams.eFilter3Pos != FILTERINSERTTYPE_OFF)) )
		{
			RETURNSTATEIFNOTSUCCESS( SetupToneControlFilters( ) );
			ResetToneControlFilters( );
		}
	}

	if ( m_PrevInvariantParams.fERFrontBackDelay != m_pParams->sInvariantParams.fERFrontBackDelay )
	{
		m_Reverb.ERFrontBackDelay[0].Term( m_pAllocator );
		m_Reverb.ERFrontBackDelay[1].Term( m_pAllocator );	
		RETURNSTATEIFNOTSUCCESS( SetupERFrontBackDelay( m_pAllocator, io_pBuffer->GetChannelMask() ) );		
		m_Reverb.ERFrontBackDelay[0].Reset();
		m_Reverb.ERFrontBackDelay[1].Reset();
	}

	if (m_PrevInvariantParams.fReverbDelay != m_pParams->sInvariantParams.fReverbDelay) 
	{
		m_Reverb.ReverbDelay.Term( m_pAllocator );	
		RETURNSTATEIFNOTSUCCESS( SetupReverbDelay( m_pAllocator ) );		
		m_Reverb.ReverbDelay.Reset();	
		m_Reverb.uTailLength = (AkUInt32)( ( m_pParams->sInvariantParams.fReverbDelay / 1000.0f + m_pParams->sRTPCParams.fDecayTime ) * m_Reverb.uSampleRate );
	}

	if (	(m_PrevInvariantParams.uERPattern != m_pParams->sInvariantParams.uERPattern) ||
			(m_PrevInvariantParams.fRoomSize != m_pParams->sInvariantParams.fRoomSize) )
	{
		m_Reverb.ERDelay.Term( m_pAllocator );
		RETURNSTATEIFNOTSUCCESS( SetupERDelay( m_pAllocator ) );
		m_Reverb.ERDelay.Reset();	
 		TermERUnit( m_pAllocator );
		RETURNSTATEIFNOTSUCCESS( SetupERUnit( m_pAllocator ) );
		ResetERUnit();
	}

	if ( m_PrevInvariantParams.bEnableToneControls != m_pParams->sInvariantParams.bEnableToneControls ||		 
		 m_PrevInvariantParams.eFilter1Pos != m_pParams->sInvariantParams.eFilter1Pos ||
		 m_PrevInvariantParams.eFilter2Pos != m_pParams->sInvariantParams.eFilter2Pos ||
		 m_PrevInvariantParams.eFilter3Pos != m_pParams->sInvariantParams.eFilter3Pos  )
	{
		if ( m_pParams->sInvariantParams.bEnableToneControls &&
			((m_pParams->sInvariantParams.eFilter1Pos != FILTERINSERTTYPE_OFF) || 
			(m_pParams->sInvariantParams.eFilter2Pos != FILTERINSERTTYPE_OFF) ||
			(m_pParams->sInvariantParams.eFilter3Pos != FILTERINSERTTYPE_OFF)) )
		{
			TermToneControlFilters( m_pAllocator );
			RETURNSTATEIFNOTSUCCESS( SetupToneControlFilters( ) );
			ResetToneControlFilters( );
		}
	}

	if ( m_PrevInvariantParams.eFilter1Curve != m_pParams->sInvariantParams.eFilter1Curve  )
	{
		ComputeTCCoefs1( );
	}
	if ( m_PrevInvariantParams.eFilter2Curve != m_pParams->sInvariantParams.eFilter2Curve )
	{	
		ComputeTCCoefs2( );
	}
	if ( m_PrevInvariantParams.eFilter3Curve != m_pParams->sInvariantParams.eFilter3Curve )
	{
		ComputeTCCoefs3( );
	} 

	// nothing to do when fInputLFELevel or fInputCenter level changes

	m_PrevInvariantParams = m_pParams->sInvariantParams;
	m_pParams->sInvariantParams.bDirty = false;

	return false; // No errors
}

void CAkRoomVerbFX::RTPCParametersUpdate( )
{
	// Update RTPC values as necessary
	if ( m_pParams->sRTPCParams.fDecayTime != m_PrevRTPCParams.fDecayTime || 
		 m_pParams->sRTPCParams.fHFDamping != m_PrevRTPCParams.fHFDamping )
	{
		ChangeDecay();
		m_Reverb.uTailLength = (AkUInt32)( ( m_pParams->sInvariantParams.fReverbDelay / 1000.0f + m_pParams->sRTPCParams.fDecayTime ) * m_Reverb.uSampleRate );
	}

	if ( m_pParams->sRTPCParams.fDiffusion != m_PrevRTPCParams.fDiffusion )
	{
		ChangeDiffusion();
	}

	if ( (m_pParams->sRTPCParams.fFilter1Gain != m_PrevRTPCParams.fFilter1Gain) || 
	     (m_pParams->sRTPCParams.fFilter1Freq != m_PrevRTPCParams.fFilter1Freq) ||
		 (m_pParams->sRTPCParams.fFilter1Q != m_PrevRTPCParams.fFilter1Q) )
	{
		ComputeTCCoefs1( );
	}

	if ( (m_pParams->sRTPCParams.fFilter2Gain != m_PrevRTPCParams.fFilter2Gain) || 
	     (m_pParams->sRTPCParams.fFilter2Freq != m_PrevRTPCParams.fFilter2Freq) ||
		 (m_pParams->sRTPCParams.fFilter2Q != m_PrevRTPCParams.fFilter2Q) )
	{
		ComputeTCCoefs2( );
	}

	if ( (m_pParams->sRTPCParams.fFilter3Gain != m_PrevRTPCParams.fFilter3Gain) || 
	     (m_pParams->sRTPCParams.fFilter3Freq != m_PrevRTPCParams.fFilter3Freq) ||
		 (m_pParams->sRTPCParams.fFilter3Q != m_PrevRTPCParams.fFilter3Q) )
	{
		ComputeTCCoefs3( );
	}

	// Disable dry level for sent mode path (wet still active for flexibility)
	if ( m_Reverb.bIsSentMode )
	{
		// Send mode has no dry path
		m_pParams->sRTPCParams.fDryLevel = 0.f;
	}

	m_pParams->sRTPCParams.bDirty = false;
}

#ifndef AK_PS3

// Main processing routine
void CAkRoomVerbFX::Execute( AkAudioBuffer* io_pBuffer )
{
	AK_PERF_RECORDING_START( "RoomVerb", 25, 30 );

	if ( AK_EXPECT_FALSE( m_pParams->sInvariantParams.bDirty ) )
	{
		// Support live change of non-RTPC parameters
		bool bError = LiveParametersUpdate( io_pBuffer );
		if ( bError )
			return;
	}

	if ( AK_EXPECT_FALSE( m_pParams->sRTPCParams.bDirty ) )
	{
		// RTPC parameter updates
		RTPCParametersUpdate( );
	}
	
	m_Reverb.FXTailHandler.HandleTail( io_pBuffer, m_Reverb.uTailLength ); 
	AKASSERT( io_pBuffer->uValidFrames <= io_pBuffer->MaxFrames() );
	if ( io_pBuffer->uValidFrames == 0 )
		return;

#ifdef AK_WIIU
	LCEnableDMA();
#endif

	// Even if the processing fail, (i.e. memory allocation did not success, 
	// do not fail here as the pipeline won't be able to handle it (by design)
	switch ( io_pBuffer->GetChannelMask() )
	{
	case AK_SPEAKER_SETUP_MONO:
	case AK_SPEAKER_SETUP_0POINT1:
	case AK_SPEAKER_SETUP_STEREO:
	case AK_SPEAKER_SETUP_1POINT1:
	case AK_SPEAKER_SETUP_2POINT1:
		ProcessSpread1Out( io_pBuffer );
		break;
	case AK_SPEAKER_SETUP_3STEREO:
	case AK_SPEAKER_SETUP_3POINT1:
	case AK_SPEAKER_SETUP_4:
		ProcessSpread2Out( io_pBuffer );
		break;
	case AK_SPEAKER_SETUP_4POINT1:
	case AK_SPEAKER_SETUP_5:
	case AK_SPEAKER_SETUP_5POINT1:
		ProcessSpread3Out( io_pBuffer );
		break;
#ifdef AK_71AUDIO
	case AK_SPEAKER_SETUP_6:
	case AK_SPEAKER_SETUP_6POINT1:
	case AK_SPEAKER_SETUP_7:
	case AK_SPEAKER_SETUP_7POINT1:
		ProcessSpread4Out( io_pBuffer );
		break;
#endif
	}

#ifdef AK_WIIU
	LCDisableDMA();
#endif


	m_PrevRTPCParams = m_pParams->sRTPCParams;

	AK_PERF_RECORDING_STOP( "RoomVerb", 25, 30 );
}

#else

void CAkRoomVerbFX::ComputeScratchMemoryRequired( AkChannelMask in_uChannelMask )
{
	// Temporary buffers required by routine
	AkUInt32 uNumBackChannels = in_uChannelMask & ( AK_SPEAKER_BACK_LEFT | AK_SPEAKER_BACK_RIGHT ) ? 2 : 0;
	uNumBackChannels = m_pParams->sInvariantParams.bEnableEarlyReflections ? uNumBackChannels : 0;
	AkUInt32 uNumTempBuffers;
	switch ( in_uChannelMask )
	{
	case AK_SPEAKER_SETUP_MONO:
	case AK_SPEAKER_SETUP_0POINT1:
	case AK_SPEAKER_SETUP_STEREO:
	case AK_SPEAKER_SETUP_1POINT1:
	case AK_SPEAKER_SETUP_2POINT1:
		uNumTempBuffers = 4 + m_Reverb.uNumERSignals;
		break;
	case AK_SPEAKER_SETUP_3STEREO:
	case AK_SPEAKER_SETUP_3POINT1:
	case AK_SPEAKER_SETUP_4:
		uNumTempBuffers = 6 + m_Reverb.uNumERSignals + uNumBackChannels;
		break;
	case AK_SPEAKER_SETUP_4POINT1:
	case AK_SPEAKER_SETUP_5:
	case AK_SPEAKER_SETUP_5POINT1:
		uNumTempBuffers = 8 + m_Reverb.uNumERSignals + uNumBackChannels;
		break;
	}
	m_uScratchBytes = uNumTempBuffers * sizeof(AkReal32) * NUMPROCESSFRAMES;
	if ( m_Reverb.uNumERSignals )
	{
		m_uScratchBytes += m_pERUnit->GetScratchMemorySizeRequired();
	}
	if ( m_Reverb.uNumERSignals )
	{
		m_uScratchBytes += m_Reverb.ERDelay.GetScratchMemorySizeRequired( NUMPROCESSFRAMES );
		m_uScratchBytes += m_Reverb.ERFrontBackDelay[0].GetScratchMemorySizeRequired( NUMPROCESSFRAMES );
		m_uScratchBytes += m_Reverb.ERFrontBackDelay[1].GetScratchMemorySizeRequired( NUMPROCESSFRAMES );
	}
	m_uScratchBytes += m_Reverb.ReverbDelay.GetScratchMemorySizeRequired( NUMPROCESSFRAMES );
	
	for ( AkUInt32 i = 0; i < NUMDIFFUSIONALLPASSFILTERS; i++ )
	{
		m_uScratchBytes += m_Reverb.DiffusionFilters[i].GetScratchMemorySizeRequired( NUMPROCESSFRAMES );
	}
	AkUInt32 uMaxRUMem = 0;
	for ( AkUInt32 i = 0; i < m_Reverb.uNumReverbUnits; i++ )
	{
		AkUInt32 uRUMem = m_pReverbUnitsState[i].ReverbUnits.GetScratchMemorySizeRequired( NUMPROCESSFRAMES );
		uRUMem += m_pReverbUnitsState[i].RUInputDelay.GetScratchMemorySizeRequired( NUMPROCESSFRAMES );
		if ( uRUMem > uMaxRUMem )
			uMaxRUMem = uRUMem;
	}
	m_uScratchBytes += uMaxRUMem;
}


// Main processing routine for PS3
void CAkRoomVerbFX::Execute( AkAudioBuffer* io_pBuffer,
							 AK::MultiCoreServices::DspProcess*& out_pDspProcess )
{
	if ( AK_EXPECT_FALSE( m_pParams->sInvariantParams.bDirty ) )
	{
		// Support live change of non-RTPC parameters
		bool bError = LiveParametersUpdate( io_pBuffer );
		if ( bError )
			return;
	}

	if ( AK_EXPECT_FALSE( m_pParams->sRTPCParams.bDirty ) )
	{
		// RTPC parameter updates
		RTPCParametersUpdate( );
	}

	// Total amount scratch memory required
	AkChannelMask uChannelMask = io_pBuffer->GetChannelMask();
	ComputeScratchMemoryRequired( uChannelMask );
	
	// Prepare for SPU job
	m_DspProcess.ResetDspProcess( true );
	switch ( uChannelMask )
	{
	case AK_SPEAKER_SETUP_MONO:
	case AK_SPEAKER_SETUP_0POINT1:
	case AK_SPEAKER_SETUP_STEREO:
	case AK_SPEAKER_SETUP_1POINT1:
	case AK_SPEAKER_SETUP_2POINT1:
		m_DspProcess.SetDspProcess( AkRoomVerbFXJobBin );
		break;
	case AK_SPEAKER_SETUP_3STEREO:
	case AK_SPEAKER_SETUP_3POINT1:
	case AK_SPEAKER_SETUP_4:
		m_DspProcess.SetDspProcess( AkRoomVerbFXJobBin2 );
		break;
	case AK_SPEAKER_SETUP_4POINT1:
	case AK_SPEAKER_SETUP_5:
	case AK_SPEAKER_SETUP_5POINT1:
		m_DspProcess.SetDspProcess( AkRoomVerbFXJobBin3 );
		break;
	}
	m_DspProcess.SetScratchSize( m_uScratchBytes );


	// Setup DMA transfers
	// Send plugin audio buffer information
	m_DspProcess.AddDspProcessSmallDma( io_pBuffer, sizeof(AkAudioBuffer) );

	// Send FX params
	m_DspProcess.AddDspProcessSmallDma( static_cast<AkRoomVerbFXParams *>( m_pParams ), sizeof( AkRoomVerbFXParams ) );
	m_DspProcess.AddDspProcessSmallDma( &m_PrevRTPCParams, sizeof( RTPCParams ) );

	// Send FX state
	m_DspProcess.AddDspProcessSmallDma( &m_Reverb, sizeof( ReverbState ) );

	// Send reverb unit state
	AkUInt32 uDMASize = AK_ALIGN_SIZE_FOR_DMA( sizeof( ReverbUnitState ) * m_Reverb.uNumReverbUnits );
	m_DspProcess.AddDspProcessSmallDma( m_pReverbUnitsState, uDMASize ); 

	// Send tone controls state
	if ( m_Reverb.uTCFilterIndex[0] )
	{
		uDMASize = AK_ALIGN_SIZE_FOR_DMA( sizeof( ToneControlsState ) * m_Reverb.uTCFilterIndex[0]);
		m_DspProcess.AddDspProcessSmallDma( m_pTCFiltersState, uDMASize ); 
	}

	// Send early reflections unit state
	if ( m_Reverb.uNumERSignals  )
	{
		m_DspProcess.AddDspProcessSmallDma( m_pERUnit, sizeof( ERUnitDual ) ); 
	}

	// Send input channel information. 
	// Note: Subsequent DMAs are contiguous in memory on SPU side. 
	// AddDspProcessDma splits into more than one DMA if necessary.
	AkUInt32 uSizeInput = AK_ALIGN_SIZE_FOR_DMA( sizeof(AkReal32) * io_pBuffer->MaxFrames() * io_pBuffer->NumChannels() );
	m_DspProcess.AddDspProcessDma( io_pBuffer->GetDataStartDMA(), uSizeInput );

	out_pDspProcess = &m_DspProcess;
}

#endif

// Compute allpass filter gain for a given instance
AkReal32 CAkRoomVerbFX::ComputeDiffusionAPFGain( AkUInt32 in_UnitNum, AkReal32 in_fDiffusion )
{
	AkReal32 fG =  in_fDiffusion/100.f*0.61803f*NUMDIFFUSIONALLPASSFILTERS;
	AkReal32 fTmp = fG-(NUMDIFFUSIONALLPASSFILTERS-1-in_UnitNum)*0.61803f;
	AkReal32 fGi = AkMax(fTmp,0.f);
	fGi = AkMin( fGi, 0.61803f );
	return fGi;
}

// Change diffusion without changing delay lengths (RTPC)
void CAkRoomVerbFX::ChangeDiffusion( )
{
	for ( AkUInt32 i = 0; i < NUMDIFFUSIONALLPASSFILTERS; ++i )
	{
		AkReal32 fAPFGain = ComputeDiffusionAPFGain( i, m_pParams->sRTPCParams.fDiffusion );
		m_Reverb.DiffusionFilters[i].SetGain( fAPFGain );
	}
}

// Change decay time without changing delay lengths (RTPC)
void CAkRoomVerbFX::ChangeDecay( )
{
	for ( AkUInt32 i = 0; i < m_Reverb.uNumReverbUnits; ++i )
	{
		m_pReverbUnitsState[i].ReverbUnits.ChangeDecay(	m_pParams->sRTPCParams.fDecayTime, 
														m_pParams->sRTPCParams.fHFDamping, 
														m_Reverb.uSampleRate );
	}
}

#ifndef AK_PS3

void CAkRoomVerbFX::WetPreProcess( AkAudioBuffer * in_pBuffer, AkReal32 * out_pfWetIn, AkUInt32 in_uNumFrames, AkUInt32 in_uFrameOffset )
{
	AkChannelMask uChannelMask = in_pBuffer->GetChannelMask();
	ConstantPowerChannelMixdown( in_pBuffer, in_uNumFrames, in_uFrameOffset, out_pfWetIn, uChannelMask, m_pParams->sInvariantParams.fInputCenterLevel , m_pParams->sInvariantParams.fInputLFELevel );
	
	if ( m_pParams->sInvariantParams.bEnableToneControls )
	{
		for ( AkUInt8 i = 0; i < m_Reverb.uTCFilterIndex[0]; ++i )
		{
			if ( m_pTCFiltersState[i].FilterPos == FILTERINSERTTYPE_ERANDREVERB )
				m_pTCFiltersState[i].Filter.ProcessBuffer( out_pfWetIn, in_uNumFrames );
		}
	}
}

void CAkRoomVerbFX::ReverbPreProcess( AkReal32 * io_pfBuffer, AkUInt32 in_uNumFrames )
{
	if ( m_pParams->sInvariantParams.bEnableToneControls )
	{
		for ( AkUInt8 i = 0; i < m_Reverb.uTCFilterIndex[0]; ++i )
		{
			if ( m_pTCFiltersState[i].FilterPos == FILTERINSERTTYPE_REVERBONLY )
				m_pTCFiltersState[i].Filter.ProcessBuffer( io_pfBuffer, in_uNumFrames );
		}
	}
}

void CAkRoomVerbFX::ReverbPostProcess( AkReal32 ** out_ppfReverb, AkUInt32 in_uNumOutSignals, AkReal32 in_fGain, AkUInt32 in_uNumFrames )
{
	for ( AkUInt32 i = 0; i < in_uNumOutSignals; i++ )
	{
		m_Reverb.DCFilter[i].ProcessBufferWithGain( out_ppfReverb[i], in_uNumFrames, in_fGain );
	}
}


#define MAXSTEREOWIDTH (180.f)
#define HALFPOWERGAIN (0.707106f)
// LR mix based on spread factor (constant power)
// 0 Stereo width corresponds to half power from each unit mix in each channel
// MAXSTEREOWIDTH maps full power to each discrete signal with no contribution from the other on each side
#define COMPUTELRMIX( __PrevStereoWidth__, __TargetStereoWidth__ )								 \
	AkReal32 fPrevGain1 = ((__PrevStereoWidth__)/MAXSTEREOWIDTH)*(1.f-HALFPOWERGAIN)+HALFPOWERGAIN;		 \
	AkReal32 fOneMinuseSquaredGain = 1.f-fPrevGain1*fPrevGain1;									 \
	AkReal32 fPrevGain2;																		 \
	if ( fOneMinuseSquaredGain > 0.f )															 \
		fPrevGain2 = sqrt(fOneMinuseSquaredGain);												 \
	else																						 \
		fPrevGain2 = 0.f;																		 \
	const AkReal32 fGain1 = ((__TargetStereoWidth__)/MAXSTEREOWIDTH)*(1.f-HALFPOWERGAIN)+HALFPOWERGAIN;			 \
	fOneMinuseSquaredGain = 1.f-fGain1*fGain1;													 \
	AkReal32 fGain2;																			 \
	if ( fOneMinuseSquaredGain > 0.f )															 \
		fGain2 = sqrt(fOneMinuseSquaredGain);													 \
	else																						 \
		fGain2 = 0.f;																			 \

// This routine handles 0.1, 1.0, 1.1, 2.0, 2.1
AKRESULT CAkRoomVerbFX::ProcessSpread1Out( AkAudioBuffer * io_pBuffer )
{
	// Allocate all temporary processing buffer in one shot and dispatch
	AkUInt32 uNumTempBuffers = 4 + m_Reverb.uNumERSignals;
	AkReal32 * pfTempStorage  = (AkReal32*) AK_PLUGIN_ALLOC( m_pAllocator, uNumTempBuffers * sizeof(AkReal32) * NUMPROCESSFRAMES );
	if ( !pfTempStorage )
		return AK_InsufficientMemory;

	AkReal32 * pfERIn  = &pfTempStorage[0]; 	
	AkReal32 * pfReverbIn = &pfTempStorage[NUMPROCESSFRAMES];
	AkReal32 * pfReverbOut[2];
	pfReverbOut[0] = &pfTempStorage[2*NUMPROCESSFRAMES];
	pfReverbOut[1] = &pfTempStorage[3*NUMPROCESSFRAMES];
	AkReal32 * pfEROutL = NULL;
	AkReal32 * pfEROutR = NULL;
	if ( m_Reverb.uNumERSignals )
	{
		pfEROutL = &pfTempStorage[4*NUMPROCESSFRAMES];
		pfEROutR = &pfTempStorage[5*NUMPROCESSFRAMES];
	}

	COMPUTELRMIX( m_PrevRTPCParams.fStereoWidth, m_pParams->sRTPCParams.fStereoWidth );

	AkUInt32 uFrameOffset = 0;
	AkUInt32 uNumFramesRemaining = io_pBuffer->uValidFrames;
	while ( uNumFramesRemaining )
	{
		AkUInt32 uNumFrames = AkMin( uNumFramesRemaining, NUMPROCESSFRAMES );

		// Mixdown, tone controls and early reflections delay
		WetPreProcess( io_pBuffer, pfERIn, uNumFrames, uFrameOffset );

		// Split result into ER and reverb feed
		if ( m_Reverb.ReverbDelay.GetDelayLength() > 0 )
			m_Reverb.ReverbDelay.ProcessBuffer( pfERIn, pfReverbIn, uNumFrames );
		else
			AKPLATFORM::AkMemCpy(pfReverbIn, pfERIn, uNumFrames*sizeof(AkReal32) ); 

		if ( m_Reverb.ERDelay.GetDelayLength() > 0 )
			m_Reverb.ERDelay.ProcessBuffer( pfERIn, uNumFrames );

		if ( m_Reverb.uNumERSignals )
		{
			// Early reflections process
			m_pERUnit->ProcessBuffer( pfERIn, pfEROutL, pfEROutR, uNumFrames );
		
			// ER tone controls
			if ( m_pParams->sInvariantParams.bEnableToneControls )
			{
				for ( AkUInt8 i = 0; i < m_Reverb.uTCFilterIndex[0]; ++i )
				{
					if ( m_pTCFiltersState[i].FilterPos == FILTERINSERTTYPE_ERONLY )
					{
						m_pTCFiltersState[i].Filter.ProcessBuffer( pfEROutL, uNumFrames );
						m_pTCFiltersState[i+1].Filter.ProcessBuffer( pfEROutR, uNumFrames );
						++i;
					}
				}
			}
		}

		// Diffusion tank
		for ( AkUInt32 i = 0; i < NUMDIFFUSIONALLPASSFILTERS; i++ )
		{
			m_Reverb.DiffusionFilters[i].ProcessBuffer( pfReverbIn, uNumFrames );
		}

		// Reverb EQ
		ReverbPreProcess( pfReverbIn, uNumFrames );

		// Even units go into left tank
		// Odd units go into right tank
		AkZeroMemLarge( pfReverbOut[0], 2 * NUMPROCESSFRAMES * sizeof(AkReal32) );
		for ( AkUInt32 i = 0; i < m_Reverb.uNumReverbUnits; i++ )
		{	
			m_pReverbUnitsState[i].RUInputDelay.ProcessBuffer( pfReverbIn, uNumFrames );
			m_pReverbUnitsState[i].ReverbUnits.ProcessBufferAccum( pfReverbIn, pfReverbOut[(i&1)], uNumFrames );
		}

		// Reverb unit gain consider that it was split into 2 banks
		ReverbPostProcess( pfReverbOut, 2, 1.41421356f*m_Reverb.fReverbUnitsMixGain, uNumFrames );
	
		AkChannelMask uChannelMask = io_pBuffer->GetChannelMask();
		AkUInt32 uCurChannel = 0;
		// Front left
		if ( uChannelMask & AK_SPEAKER_FRONT_LEFT )
		{
			AkReal32 * pfDryChannel = io_pBuffer->GetChannel(uCurChannel) + uFrameOffset;
			Mix3Interp( pfDryChannel, 
						pfReverbOut[0],
						pfReverbOut[1], 
						m_PrevRTPCParams.fDryLevel,
						m_pParams->sRTPCParams.fDryLevel,
						m_PrevRTPCParams.fReverbLevel*fPrevGain1,
						m_pParams->sRTPCParams.fReverbLevel*fGain1,
						m_PrevRTPCParams.fReverbLevel*fPrevGain2,
						m_pParams->sRTPCParams.fReverbLevel*fGain2,
						uNumFrames );
			if ( m_Reverb.uNumERSignals )
			{
				Mix3Interp( pfDryChannel,
							pfEROutL,
							pfEROutR,
							1.f,
							1.f,
							m_PrevRTPCParams.fERLevel*fPrevGain1,
							m_pParams->sRTPCParams.fERLevel*fGain1,
							m_PrevRTPCParams.fERLevel*fPrevGain2,
							m_pParams->sRTPCParams.fERLevel*fGain2,
							uNumFrames);
			}
			
			uCurChannel++;
		}

		// Front right
		if  ( uChannelMask & AK_SPEAKER_FRONT_RIGHT ) 
		{
			AkReal32 * pfDryChannel = io_pBuffer->GetChannel(uCurChannel) + uFrameOffset;
			Mix3Interp( pfDryChannel, 
						pfReverbOut[0],
						pfReverbOut[1], 
						m_PrevRTPCParams.fDryLevel,
						m_pParams->sRTPCParams.fDryLevel,
						m_PrevRTPCParams.fReverbLevel*fPrevGain2,
						m_pParams->sRTPCParams.fReverbLevel*fGain2,
						m_PrevRTPCParams.fReverbLevel*fPrevGain1,
						m_pParams->sRTPCParams.fReverbLevel*fGain1,
						uNumFrames );
			if ( m_Reverb.uNumERSignals )
			{
				Mix3Interp( pfDryChannel,
							pfEROutL,
							pfEROutR,
							1.f,
							1.f,
							m_PrevRTPCParams.fERLevel*fPrevGain2,
							m_pParams->sRTPCParams.fERLevel*fGain2,
							m_PrevRTPCParams.fERLevel*fPrevGain1,
							m_pParams->sRTPCParams.fERLevel*fGain1,
							uNumFrames);
			}
			uCurChannel++;
		}

		// Center is the mono channel in this case
		if ( uChannelMask & AK_SPEAKER_FRONT_CENTER )
		{	
			AKASSERT( !((uChannelMask & AK_SPEAKER_FRONT_LEFT) || (uChannelMask & AK_SPEAKER_FRONT_RIGHT) ) );
			AkReal32 * pfDryChannel = io_pBuffer->GetChannel(uCurChannel) + uFrameOffset;
			Mix3Interp( pfDryChannel, 
						pfReverbOut[0],
						pfReverbOut[1], 
						m_PrevRTPCParams.fDryLevel,
						m_pParams->sRTPCParams.fDryLevel,
						m_PrevRTPCParams.fReverbLevel*HALFPOWERGAIN,
						m_pParams->sRTPCParams.fReverbLevel*HALFPOWERGAIN,
						m_PrevRTPCParams.fReverbLevel*HALFPOWERGAIN,
						m_pParams->sRTPCParams.fReverbLevel*HALFPOWERGAIN,
						uNumFrames );
			if ( m_Reverb.uNumERSignals )
			{
				Mix3Interp( pfDryChannel,
							pfEROutL,
							pfEROutR,
							1.f,
							1.f,
							m_PrevRTPCParams.fERLevel*HALFPOWERGAIN,
							m_pParams->sRTPCParams.fERLevel*HALFPOWERGAIN,
							m_PrevRTPCParams.fERLevel*HALFPOWERGAIN,
							m_pParams->sRTPCParams.fERLevel*HALFPOWERGAIN,
							uNumFrames);
			}
			uCurChannel++;
		}

		// LFE
		if ( uChannelMask & AK_SPEAKER_LOW_FREQUENCY )
		{
			// Never any ER on LFE
			AkReal32 * pfDryChannel = io_pBuffer->GetChannel(uCurChannel) + uFrameOffset;
			Mix3Interp( pfDryChannel, 
						pfReverbOut[0],
						pfReverbOut[1], 
						m_PrevRTPCParams.fDryLevel,
						m_pParams->sRTPCParams.fDryLevel,
						m_PrevRTPCParams.fLFELevel*m_PrevRTPCParams.fReverbLevel*HALFPOWERGAIN,
						m_pParams->sRTPCParams.fLFELevel*m_pParams->sRTPCParams.fReverbLevel*HALFPOWERGAIN,
						m_PrevRTPCParams.fLFELevel*m_PrevRTPCParams.fReverbLevel*HALFPOWERGAIN,
						m_pParams->sRTPCParams.fLFELevel*m_pParams->sRTPCParams.fReverbLevel*HALFPOWERGAIN,
						uNumFrames );
			uCurChannel++;
		}

		fPrevGain1 = fGain1;
		fPrevGain2 = fGain2;
		m_PrevRTPCParams.fDryLevel = m_pParams->sRTPCParams.fDryLevel;
		m_PrevRTPCParams.fERLevel = m_pParams->sRTPCParams.fERLevel;
		m_PrevRTPCParams.fReverbLevel = m_pParams->sRTPCParams.fReverbLevel;
		m_PrevRTPCParams.fLFELevel = m_pParams->sRTPCParams.fLFELevel;

		// Advance pointers
		uFrameOffset += uNumFrames;
		uNumFramesRemaining -= uNumFrames;
	}

	if ( pfTempStorage )
	{
		AK_PLUGIN_FREE( m_pAllocator, pfTempStorage );
		pfTempStorage = NULL;
	}

	return AK_Success;
}

// This routine handles 3.0, 3.1 and 4.0
AKRESULT CAkRoomVerbFX::ProcessSpread2Out( AkAudioBuffer * io_pBuffer )
{
	// Allocate all temporary processing buffer in one shot and dispatch
	AkChannelMask uChannelMask = io_pBuffer->GetChannelMask();
	AkUInt32 uNumBackChannels = uChannelMask & ( AK_SPEAKER_BACK_LEFT | AK_SPEAKER_BACK_RIGHT ) ? 2 : 0;
	uNumBackChannels = m_pParams->sInvariantParams.bEnableEarlyReflections ? uNumBackChannels : 0;
	AkUInt32 uNumTempBuffers = 6 + m_Reverb.uNumERSignals + uNumBackChannels;
	AkReal32 * pfTempStorage  = (AkReal32*) AK_PLUGIN_ALLOC( m_pAllocator, uNumTempBuffers * sizeof(AkReal32) * NUMPROCESSFRAMES );
	if ( !pfTempStorage )
		return AK_InsufficientMemory;

	AkReal32 * pfERIn  = &pfTempStorage[0]; 	
	AkReal32 * pfReverbIn = &pfTempStorage[NUMPROCESSFRAMES];
	AkReal32 * pfReverbOut[4];
	pfReverbOut[0] = &pfTempStorage[2*NUMPROCESSFRAMES];
	pfReverbOut[1] = &pfTempStorage[3*NUMPROCESSFRAMES];
	pfReverbOut[2] = &pfTempStorage[4*NUMPROCESSFRAMES];
	pfReverbOut[3] = &pfTempStorage[5*NUMPROCESSFRAMES];
	AkReal32 * pfEROut[4] = { NULL, NULL, NULL, NULL };
	if ( m_Reverb.uNumERSignals )
	{
		pfEROut[0] = &pfTempStorage[6*NUMPROCESSFRAMES];
		pfEROut[1] = &pfTempStorage[7*NUMPROCESSFRAMES];
		if ( uNumBackChannels == 2 )
		{
			pfEROut[2] =&pfTempStorage[8*NUMPROCESSFRAMES]; 
			pfEROut[3] =&pfTempStorage[9*NUMPROCESSFRAMES];
		}	
	}

	COMPUTELRMIX( m_PrevRTPCParams.fStereoWidth, m_pParams->sRTPCParams.fStereoWidth );

	// Front/back control is ignored when surround channels are not present
	if ( !(uChannelMask & ( AK_SPEAKER_BACK_LEFT | AK_SPEAKER_BACK_RIGHT )) )
	{
		m_pParams->sRTPCParams.fFrontLevel = 1.f; 
		m_pParams->sRTPCParams.fRearLevel = 1.f;
	}

	// Center is always a 'true' center  (not mono)
	AKASSERT( !(uChannelMask & AK_SPEAKER_FRONT_CENTER) || ((uChannelMask & AK_SPEAKER_FRONT_CENTER) && (uChannelMask & ( AK_SPEAKER_FRONT_LEFT | AK_SPEAKER_FRONT_RIGHT )) ) ); 

	AkUInt32 uFrameOffset = 0;
	AkUInt32 uNumFramesRemaining = io_pBuffer->uValidFrames;
	while ( uNumFramesRemaining )
	{
		AkUInt32 uNumFrames = AkMin( uNumFramesRemaining, NUMPROCESSFRAMES );

		// Mixdown, tone controls and pre-delay
		WetPreProcess( io_pBuffer, pfERIn, uNumFrames, uFrameOffset );

		// Split result into ER and reverb feed
		if ( m_Reverb.ReverbDelay.GetDelayLength() > 0 )
			m_Reverb.ReverbDelay.ProcessBuffer( pfERIn, pfReverbIn, uNumFrames );
		else
			AKPLATFORM::AkMemCpy(pfReverbIn, pfERIn, uNumFrames*sizeof(AkReal32) ); 

		if ( m_Reverb.ERDelay.GetDelayLength() > 0 )
			m_Reverb.ERDelay.ProcessBuffer( pfERIn, uNumFrames );

		if ( m_Reverb.uNumERSignals )
		{
			// Early reflections process
			m_pERUnit->ProcessBuffer( pfERIn, pfEROut[0], pfEROut[1], uNumFrames );

			// ER tone controls
			if ( m_pParams->sInvariantParams.bEnableToneControls )
			{
				for ( AkUInt8 i = 0; i < m_Reverb.uTCFilterIndex[0]; ++i )
				{
					if ( m_pTCFiltersState[i].FilterPos == FILTERINSERTTYPE_ERONLY )
					{
						m_pTCFiltersState[i].Filter.ProcessBuffer( pfEROut[0], uNumFrames );
						m_pTCFiltersState[i+1].Filter.ProcessBuffer( pfEROut[1], uNumFrames );
						++i;
					}
				}
			}

			if ( uNumBackChannels == 2 ) 
			{
				if ( m_Reverb.ERFrontBackDelay[0].GetDelayLength() > 0 )
				{
					m_Reverb.ERFrontBackDelay[0].ProcessBuffer( pfEROut[0], pfEROut[2], uNumFrames );
					m_Reverb.ERFrontBackDelay[1].ProcessBuffer( pfEROut[1], pfEROut[3], uNumFrames );
				}
				else
				{
					pfEROut[2] = pfEROut[0];
					pfEROut[3] = pfEROut[1];
				}
			}
		}

		// Diffusion tank
		for ( AkUInt32 i = 0; i < NUMDIFFUSIONALLPASSFILTERS; i++ )
		{
			m_Reverb.DiffusionFilters[i].ProcessBuffer( pfReverbIn, uNumFrames );
		}

		// Reverb EQ
		ReverbPreProcess( pfReverbIn, uNumFrames );

		// Even units go into left tanks
		// Odd units go into right tanks
		AkZeroMemLarge( pfReverbOut[0], 4 * NUMPROCESSFRAMES * sizeof(AkReal32) );
		for ( AkUInt32 i = 0; i < m_Reverb.uNumReverbUnits; i++ )
		{	
			m_pReverbUnitsState[i].RUInputDelay.ProcessBuffer( pfReverbIn, uNumFrames );
			m_pReverbUnitsState[i].ReverbUnits.ProcessBufferAccum( pfReverbIn, pfReverbOut[(i&1)], pfReverbOut[(i&1)+2], uNumFrames );
		}

		ReverbPostProcess( pfReverbOut, 4, 1.41421356f*m_Reverb.fReverbUnitsMixGain, uNumFrames );	

		AkUInt32 uCurChannel = 0;
		// Front left	
		if ( uChannelMask & AK_SPEAKER_FRONT_LEFT )
		{
			AkReal32 * pfDryChannel = io_pBuffer->GetChannel(uCurChannel) + uFrameOffset;			
			Mix3Interp( pfDryChannel, 
						pfReverbOut[0],
						pfReverbOut[1], 
						m_PrevRTPCParams.fDryLevel,
						m_pParams->sRTPCParams.fDryLevel,
						m_PrevRTPCParams.fFrontLevel*m_PrevRTPCParams.fReverbLevel*fPrevGain1,
						m_pParams->sRTPCParams.fFrontLevel*m_pParams->sRTPCParams.fReverbLevel*fGain1,
						m_PrevRTPCParams.fFrontLevel*m_PrevRTPCParams.fReverbLevel*fPrevGain2,
						m_pParams->sRTPCParams.fFrontLevel*m_pParams->sRTPCParams.fReverbLevel*fGain2,
						uNumFrames );
			if ( m_Reverb.uNumERSignals )
			{
				Mix3Interp( pfDryChannel, 
							pfEROut[0],
							pfEROut[1], 
							1.f,
							1.f,
							m_PrevRTPCParams.fFrontLevel*m_PrevRTPCParams.fERLevel*fPrevGain1,
							m_pParams->sRTPCParams.fFrontLevel*m_pParams->sRTPCParams.fERLevel*fGain1,
							m_PrevRTPCParams.fFrontLevel*m_PrevRTPCParams.fERLevel*fPrevGain2,
							m_pParams->sRTPCParams.fFrontLevel*m_pParams->sRTPCParams.fERLevel*fGain2,
							uNumFrames );
			}
			uCurChannel++;
		}

		// Front right
		if ( uChannelMask & AK_SPEAKER_FRONT_RIGHT )
		{
			AkReal32 * pfDryChannel = io_pBuffer->GetChannel(uCurChannel) + uFrameOffset;
			Mix3Interp( pfDryChannel, 
						pfReverbOut[0],
						pfReverbOut[1], 
						m_PrevRTPCParams.fDryLevel,
						m_pParams->sRTPCParams.fDryLevel,
						m_PrevRTPCParams.fFrontLevel*m_PrevRTPCParams.fReverbLevel*fPrevGain2,
						m_pParams->sRTPCParams.fFrontLevel*m_pParams->sRTPCParams.fReverbLevel*fGain2,
						m_PrevRTPCParams.fFrontLevel*m_PrevRTPCParams.fReverbLevel*fPrevGain1,
						m_pParams->sRTPCParams.fFrontLevel*m_pParams->sRTPCParams.fReverbLevel*fGain1,
						uNumFrames );
			if ( m_Reverb.uNumERSignals )
			{
				Mix3Interp( pfDryChannel, 
							pfEROut[0],
							pfEROut[1], 
							1.f,
							1.f,
							m_PrevRTPCParams.fFrontLevel*m_PrevRTPCParams.fERLevel*fPrevGain2,
							m_pParams->sRTPCParams.fFrontLevel*m_pParams->sRTPCParams.fERLevel*fGain2,
							m_PrevRTPCParams.fFrontLevel*m_PrevRTPCParams.fERLevel*fPrevGain1,
							m_pParams->sRTPCParams.fFrontLevel*m_pParams->sRTPCParams.fERLevel*fGain1,
							uNumFrames );
			}
			uCurChannel++;
		}

		if ( uChannelMask & AK_SPEAKER_FRONT_CENTER )
		{	
			AkReal32 * pfDryChannel = io_pBuffer->GetChannel(uCurChannel) + uFrameOffset;
			Mix3Interp( pfDryChannel, 
						pfReverbOut[2],
						pfReverbOut[3], 
						m_PrevRTPCParams.fDryLevel,
						m_pParams->sRTPCParams.fDryLevel,
						m_PrevRTPCParams.fCenterLevel*m_PrevRTPCParams.fReverbLevel*HALFPOWERGAIN,
						m_pParams->sRTPCParams.fCenterLevel*m_pParams->sRTPCParams.fReverbLevel*HALFPOWERGAIN,
						m_PrevRTPCParams.fCenterLevel*m_PrevRTPCParams.fReverbLevel*HALFPOWERGAIN,
						m_pParams->sRTPCParams.fCenterLevel*m_pParams->sRTPCParams.fReverbLevel*HALFPOWERGAIN,
						uNumFrames );
			uCurChannel++;
		}

		if ( uChannelMask & AK_SPEAKER_BACK_LEFT )
		{
			AkReal32 * pfDryChannel = io_pBuffer->GetChannel(uCurChannel) + uFrameOffset;
			Mix3Interp( pfDryChannel, 
						pfReverbOut[2],
						pfReverbOut[3], 
						m_PrevRTPCParams.fDryLevel,
						m_pParams->sRTPCParams.fDryLevel,
						m_PrevRTPCParams.fRearLevel*m_PrevRTPCParams.fReverbLevel*fPrevGain1,
						m_pParams->sRTPCParams.fRearLevel*m_pParams->sRTPCParams.fReverbLevel*fGain1,
						m_PrevRTPCParams.fRearLevel*m_PrevRTPCParams.fReverbLevel*fPrevGain2,
						m_pParams->sRTPCParams.fRearLevel*m_pParams->sRTPCParams.fReverbLevel*fGain2,
						uNumFrames );
			if ( m_Reverb.uNumERSignals )
			{
				Mix3Interp( pfDryChannel, 
							pfEROut[2],
							pfEROut[3], 
							1.f,
							1.f,
							m_PrevRTPCParams.fRearLevel*m_PrevRTPCParams.fERLevel*fPrevGain1,
							m_pParams->sRTPCParams.fRearLevel*m_pParams->sRTPCParams.fERLevel*fGain1,
							m_PrevRTPCParams.fRearLevel*m_PrevRTPCParams.fERLevel*fPrevGain2,
							m_pParams->sRTPCParams.fRearLevel*m_pParams->sRTPCParams.fERLevel*fGain2,
							uNumFrames );	
			}
			uCurChannel++;
		}
		
		if ( uChannelMask & AK_SPEAKER_BACK_RIGHT )
		{
			AkReal32 * pfDryChannel = io_pBuffer->GetChannel(uCurChannel) + uFrameOffset;
			Mix3Interp( pfDryChannel, 
						pfReverbOut[2],
						pfReverbOut[3], 
						m_PrevRTPCParams.fDryLevel,
						m_pParams->sRTPCParams.fDryLevel,
						m_PrevRTPCParams.fRearLevel*m_PrevRTPCParams.fReverbLevel*fPrevGain2,
						m_pParams->sRTPCParams.fRearLevel*m_pParams->sRTPCParams.fReverbLevel*fGain2,
						m_PrevRTPCParams.fRearLevel*m_PrevRTPCParams.fReverbLevel*fPrevGain1,
						m_pParams->sRTPCParams.fRearLevel*m_pParams->sRTPCParams.fReverbLevel*fGain1,
						uNumFrames );
			if ( m_Reverb.uNumERSignals )
			{
				Mix3Interp( pfDryChannel, 
							pfEROut[2],
							pfEROut[3], 
							1.f,
							1.f,
							m_PrevRTPCParams.fRearLevel*m_PrevRTPCParams.fERLevel*fPrevGain2,
							m_pParams->sRTPCParams.fRearLevel*m_pParams->sRTPCParams.fERLevel*fGain2,
							m_PrevRTPCParams.fRearLevel*m_PrevRTPCParams.fERLevel*fPrevGain1,
							m_pParams->sRTPCParams.fRearLevel*m_pParams->sRTPCParams.fERLevel*fGain1,
							uNumFrames );
			}
			uCurChannel++;
		}

		if ( uChannelMask & AK_SPEAKER_LOW_FREQUENCY )
		{
			// No ER
			AkReal32 * pfDryChannel = io_pBuffer->GetChannel(uCurChannel) + uFrameOffset;
			Mix3Interp( pfDryChannel, 
						pfReverbOut[2],
						pfReverbOut[3], 
						m_PrevRTPCParams.fDryLevel,
						m_pParams->sRTPCParams.fDryLevel,
						m_PrevRTPCParams.fLFELevel*m_PrevRTPCParams.fReverbLevel*HALFPOWERGAIN,
						m_pParams->sRTPCParams.fLFELevel*m_pParams->sRTPCParams.fReverbLevel*HALFPOWERGAIN,
						m_PrevRTPCParams.fLFELevel*m_PrevRTPCParams.fReverbLevel*HALFPOWERGAIN,
						m_pParams->sRTPCParams.fLFELevel*m_pParams->sRTPCParams.fReverbLevel*HALFPOWERGAIN,
						uNumFrames );
			uCurChannel++;
		}

		fPrevGain1 = fGain1;
		fPrevGain2 = fGain2;
		m_PrevRTPCParams.fDryLevel = m_pParams->sRTPCParams.fDryLevel;
		m_PrevRTPCParams.fERLevel = m_pParams->sRTPCParams.fERLevel;
		m_PrevRTPCParams.fReverbLevel = m_pParams->sRTPCParams.fReverbLevel;
		m_PrevRTPCParams.fFrontLevel = m_pParams->sRTPCParams.fFrontLevel;
		m_PrevRTPCParams.fCenterLevel = m_pParams->sRTPCParams.fCenterLevel;
		m_PrevRTPCParams.fRearLevel = m_pParams->sRTPCParams.fRearLevel;
		m_PrevRTPCParams.fLFELevel = m_pParams->sRTPCParams.fLFELevel;

		// Advance pointers
		uFrameOffset += uNumFrames;
		uNumFramesRemaining -= uNumFrames;
	}
		
	if ( pfTempStorage )
	{
		AK_PLUGIN_FREE( m_pAllocator, pfTempStorage );
		pfTempStorage = NULL;
	}

	return AK_Success;
}

// This routine handles 4.1, 5.0, and 5.1
AKRESULT CAkRoomVerbFX::ProcessSpread3Out( AkAudioBuffer * io_pBuffer )
{
	// Allocate all temporary processing buffer in one shot and dispatch
	AkChannelMask uChannelMask = io_pBuffer->GetChannelMask();
	AkUInt32 uNumTempBuffers = 8 + 2*m_Reverb.uNumERSignals;
	AkReal32 * pfTempStorage  = (AkReal32*) AK_PLUGIN_ALLOC( m_pAllocator, uNumTempBuffers * sizeof(AkReal32) * NUMPROCESSFRAMES );
	if ( !pfTempStorage )
		return AK_InsufficientMemory;

	AkReal32 * pfERIn  = &pfTempStorage[0]; 	
	AkReal32 * pfReverbIn = &pfTempStorage[NUMPROCESSFRAMES];
	AkReal32 * pfReverbOut[6];
	pfReverbOut[0] = &pfTempStorage[2*NUMPROCESSFRAMES];
	pfReverbOut[1] = &pfTempStorage[3*NUMPROCESSFRAMES];
	pfReverbOut[2] = &pfTempStorage[4*NUMPROCESSFRAMES];
	pfReverbOut[3] = &pfTempStorage[5*NUMPROCESSFRAMES];
	pfReverbOut[4] = &pfTempStorage[6*NUMPROCESSFRAMES];
	pfReverbOut[5] = &pfTempStorage[7*NUMPROCESSFRAMES];
	// Possible memory optimization: If eearly reflection signals are mixed immediately with dry signal,
	// ReverbOut buffers could reused them (4 * NUMPROCESSFRAMES * sizeof(Akreal32) bytes saving
	// Doing so will imply more memory access and more mixing operations on the other hand.
	AkReal32 * pfEROut[4] = { NULL, NULL, NULL, NULL };
	if ( m_Reverb.uNumERSignals ) 
	{
		pfEROut[0] = &pfTempStorage[8*NUMPROCESSFRAMES];
		pfEROut[1] = &pfTempStorage[9*NUMPROCESSFRAMES];
		pfEROut[2] = &pfTempStorage[10*NUMPROCESSFRAMES];
		pfEROut[3] = &pfTempStorage[11*NUMPROCESSFRAMES];
	}

	COMPUTELRMIX( m_PrevRTPCParams.fStereoWidth, m_pParams->sRTPCParams.fStereoWidth );

	AkUInt32 uFrameOffset = 0;
	AkUInt32 uNumFramesRemaining = io_pBuffer->uValidFrames;
	while ( uNumFramesRemaining )
	{
		AkUInt32 uNumFrames = AkMin( uNumFramesRemaining, NUMPROCESSFRAMES );

		// Mixdown, tone controls and pre-delay
		WetPreProcess( io_pBuffer, pfERIn, uNumFrames, uFrameOffset );

		// Split result into ER and reverb feed
		if ( m_Reverb.ReverbDelay.GetDelayLength() > 0 )
			m_Reverb.ReverbDelay.ProcessBuffer( pfERIn, pfReverbIn, uNumFrames );
		else
			AKPLATFORM::AkMemCpy(pfReverbIn, pfERIn, uNumFrames*sizeof(AkReal32) ); 

		if ( m_Reverb.ERDelay.GetDelayLength() > 0 )
			m_Reverb.ERDelay.ProcessBuffer( pfERIn, uNumFrames );

		if ( m_Reverb.uNumERSignals ) 
		{
			// Early reflections process
			m_pERUnit->ProcessBuffer( pfERIn, pfEROut[0], pfEROut[1], uNumFrames );

			// ER tone controls
			if ( m_pParams->sInvariantParams.bEnableToneControls )
			{
				for ( AkUInt8 i = 0; i < m_Reverb.uTCFilterIndex[0]; ++i )
				{
					if ( m_pTCFiltersState[i].FilterPos == FILTERINSERTTYPE_ERONLY )
					{
						m_pTCFiltersState[i].Filter.ProcessBuffer( pfEROut[0], uNumFrames );
						m_pTCFiltersState[i+1].Filter.ProcessBuffer( pfEROut[1], uNumFrames );
						++i;
					}
				}
			}

			if ( m_Reverb.ERFrontBackDelay[0].GetDelayLength() > 0 )
			{
				m_Reverb.ERFrontBackDelay[0].ProcessBuffer( pfEROut[0], pfEROut[2], uNumFrames );
				m_Reverb.ERFrontBackDelay[1].ProcessBuffer( pfEROut[1], pfEROut[3], uNumFrames );}
			else
			{
				pfEROut[2] = pfEROut[0];
				pfEROut[3] = pfEROut[1];
			}
		}

		// Diffusion tank
		for ( AkUInt32 i = 0; i < NUMDIFFUSIONALLPASSFILTERS; i++ )
		{
			m_Reverb.DiffusionFilters[i].ProcessBuffer( pfReverbIn, uNumFrames );
		}

		// Reverb EQ
		ReverbPreProcess( pfReverbIn, uNumFrames );
	
		// Even units go into left tanks
		// Odd units go into right tanks
		AkZeroMemLarge( pfReverbOut[0], 6 * NUMPROCESSFRAMES * sizeof(AkReal32) );
		for ( AkUInt32 i = 0; i < m_Reverb.uNumReverbUnits; i++ )
		{	
			m_pReverbUnitsState[i].RUInputDelay.ProcessBuffer( pfReverbIn, uNumFrames );
			m_pReverbUnitsState[i].ReverbUnits.ProcessBufferAccum( pfReverbIn, pfReverbOut[i%2], pfReverbOut[(i&1)+2], pfReverbOut[(i&1)+4], uNumFrames );
		}

		ReverbPostProcess( pfReverbOut, 6, 1.41421356f*m_Reverb.fReverbUnitsMixGain, uNumFrames );
		
		AkUInt32 uCurChannel = 0;
		// Front left
		{
			AkReal32 * pfDryChannel = io_pBuffer->GetChannel(uCurChannel) + uFrameOffset;
			Mix3Interp( pfDryChannel, 
						pfReverbOut[0],
						pfReverbOut[1], 
						m_PrevRTPCParams.fDryLevel,
						m_pParams->sRTPCParams.fDryLevel,
						m_PrevRTPCParams.fFrontLevel*m_PrevRTPCParams.fReverbLevel*fPrevGain1,
						m_pParams->sRTPCParams.fFrontLevel*m_pParams->sRTPCParams.fReverbLevel*fGain1,
						m_PrevRTPCParams.fFrontLevel*m_PrevRTPCParams.fReverbLevel*fPrevGain2,
						m_pParams->sRTPCParams.fFrontLevel*m_pParams->sRTPCParams.fReverbLevel*fGain2,
						uNumFrames );
			if ( m_Reverb.uNumERSignals )
			{
				Mix3Interp( pfDryChannel, 
							pfEROut[0],
							pfEROut[1], 
							1.f,
							1.f,
							m_PrevRTPCParams.fFrontLevel*m_PrevRTPCParams.fERLevel*fPrevGain1,
							m_pParams->sRTPCParams.fFrontLevel*m_pParams->sRTPCParams.fERLevel*fGain1,
							m_PrevRTPCParams.fFrontLevel*m_PrevRTPCParams.fERLevel*fPrevGain2,
							m_pParams->sRTPCParams.fFrontLevel*m_pParams->sRTPCParams.fERLevel*fGain2,
							uNumFrames );	
			}
			uCurChannel++;
		}

		// Front right
		{
			AkReal32 * pfDryChannel = io_pBuffer->GetChannel(uCurChannel) + uFrameOffset;
			Mix3Interp( pfDryChannel, 
						pfReverbOut[0],
						pfReverbOut[1], 
						m_PrevRTPCParams.fDryLevel,
						m_pParams->sRTPCParams.fDryLevel,
						m_PrevRTPCParams.fFrontLevel*m_PrevRTPCParams.fReverbLevel*fPrevGain2,
						m_pParams->sRTPCParams.fFrontLevel*m_pParams->sRTPCParams.fReverbLevel*fGain2,
						m_PrevRTPCParams.fFrontLevel*m_PrevRTPCParams.fReverbLevel*fPrevGain1,
						m_pParams->sRTPCParams.fFrontLevel*m_pParams->sRTPCParams.fReverbLevel*fGain1,
						uNumFrames );
			if ( m_Reverb.uNumERSignals )
			{
				Mix3Interp( pfDryChannel, 
							pfEROut[0],
							pfEROut[1], 
							1.f,
							1.f,
							m_PrevRTPCParams.fFrontLevel*m_PrevRTPCParams.fERLevel*fPrevGain2,
							m_pParams->sRTPCParams.fFrontLevel*m_pParams->sRTPCParams.fERLevel*fGain2,
							m_PrevRTPCParams.fFrontLevel*m_PrevRTPCParams.fERLevel*fPrevGain1,
							m_pParams->sRTPCParams.fFrontLevel*m_pParams->sRTPCParams.fERLevel*fGain1,
							uNumFrames );	
			}
			uCurChannel++;
		}

		if ( uChannelMask & AK_SPEAKER_FRONT_CENTER )
		{
			// No ER
			AkReal32 * pfDryChannel = io_pBuffer->GetChannel(uCurChannel) + uFrameOffset;
			Mix3Interp( pfDryChannel, 
						pfReverbOut[2],
						pfReverbOut[3], 
						m_PrevRTPCParams.fDryLevel,
						m_pParams->sRTPCParams.fDryLevel,
						m_PrevRTPCParams.fCenterLevel*m_PrevRTPCParams.fReverbLevel*HALFPOWERGAIN,
						m_pParams->sRTPCParams.fCenterLevel*m_pParams->sRTPCParams.fReverbLevel*HALFPOWERGAIN,
						m_PrevRTPCParams.fCenterLevel*m_PrevRTPCParams.fReverbLevel*HALFPOWERGAIN,
						m_pParams->sRTPCParams.fCenterLevel*m_pParams->sRTPCParams.fReverbLevel*HALFPOWERGAIN,
						uNumFrames );
			uCurChannel++;
		}

		// Rear left		
		{
			AkReal32 * pfDryChannel = io_pBuffer->GetChannel(uCurChannel) + uFrameOffset;
			Mix3Interp( pfDryChannel, 
						pfReverbOut[4],
						pfReverbOut[5], 
						m_PrevRTPCParams.fDryLevel,
						m_pParams->sRTPCParams.fDryLevel,
						m_PrevRTPCParams.fRearLevel*m_PrevRTPCParams.fReverbLevel*fPrevGain1,
						m_pParams->sRTPCParams.fRearLevel*m_pParams->sRTPCParams.fReverbLevel*fGain1,
						m_PrevRTPCParams.fRearLevel*m_PrevRTPCParams.fReverbLevel*fPrevGain2,
						m_pParams->sRTPCParams.fRearLevel*m_pParams->sRTPCParams.fReverbLevel*fGain2,
						uNumFrames );
			if ( m_Reverb.uNumERSignals )
			{
				Mix3Interp( pfDryChannel, 
							pfEROut[2],
							pfEROut[3], 
							1.f,
							1.f,
							m_PrevRTPCParams.fRearLevel*m_PrevRTPCParams.fERLevel*fPrevGain1,
							m_pParams->sRTPCParams.fRearLevel*m_pParams->sRTPCParams.fERLevel*fGain1,
							m_PrevRTPCParams.fRearLevel*m_PrevRTPCParams.fERLevel*fPrevGain2,
							m_pParams->sRTPCParams.fRearLevel*m_pParams->sRTPCParams.fERLevel*fGain2,
							uNumFrames );
			}
			uCurChannel++;
		}
		
		// Rear right
		{
			AkReal32 * pfDryChannel = io_pBuffer->GetChannel(uCurChannel) + uFrameOffset;
			Mix3Interp( pfDryChannel, 
						pfReverbOut[4],
						pfReverbOut[5], 
						m_PrevRTPCParams.fDryLevel,
						m_pParams->sRTPCParams.fDryLevel,
						m_PrevRTPCParams.fRearLevel*m_PrevRTPCParams.fReverbLevel*fPrevGain2,
						m_pParams->sRTPCParams.fRearLevel*m_pParams->sRTPCParams.fReverbLevel*fGain2,
						m_PrevRTPCParams.fRearLevel*m_PrevRTPCParams.fReverbLevel*fPrevGain1,
						m_pParams->sRTPCParams.fRearLevel*m_pParams->sRTPCParams.fReverbLevel*fGain1,
						uNumFrames );
			if ( m_Reverb.uNumERSignals )
			{
				Mix3Interp( pfDryChannel, 
							pfEROut[2],
							pfEROut[3], 
							1.f,
							1.f,
							m_PrevRTPCParams.fRearLevel*m_PrevRTPCParams.fERLevel*fPrevGain2,
							m_pParams->sRTPCParams.fRearLevel*m_pParams->sRTPCParams.fERLevel*fGain2,
							m_PrevRTPCParams.fRearLevel*m_PrevRTPCParams.fERLevel*fPrevGain1,
							m_pParams->sRTPCParams.fRearLevel*m_pParams->sRTPCParams.fERLevel*fGain1,
							uNumFrames );	
			}
			uCurChannel++;
		}

		if ( uChannelMask & AK_SPEAKER_LOW_FREQUENCY )
		{
			// No ER
			AkReal32 * pfDryChannel = io_pBuffer->GetChannel(uCurChannel) + uFrameOffset;	
			Mix3Interp( pfDryChannel, 
						pfReverbOut[2],
						pfReverbOut[3], 
						m_PrevRTPCParams.fDryLevel,
						m_pParams->sRTPCParams.fDryLevel,
						m_PrevRTPCParams.fLFELevel*m_PrevRTPCParams.fReverbLevel*HALFPOWERGAIN,
						m_pParams->sRTPCParams.fLFELevel*m_pParams->sRTPCParams.fReverbLevel*HALFPOWERGAIN,
						m_PrevRTPCParams.fLFELevel*m_PrevRTPCParams.fReverbLevel*HALFPOWERGAIN,
						m_pParams->sRTPCParams.fLFELevel*m_pParams->sRTPCParams.fReverbLevel*HALFPOWERGAIN,
						uNumFrames );
			uCurChannel++;
		}

		fPrevGain1 = fGain1;
		fPrevGain2 = fGain2;
		m_PrevRTPCParams.fDryLevel = m_pParams->sRTPCParams.fDryLevel;
		m_PrevRTPCParams.fERLevel = m_pParams->sRTPCParams.fERLevel;
		m_PrevRTPCParams.fReverbLevel = m_pParams->sRTPCParams.fReverbLevel;
		m_PrevRTPCParams.fFrontLevel = m_pParams->sRTPCParams.fFrontLevel;
		m_PrevRTPCParams.fCenterLevel = m_pParams->sRTPCParams.fCenterLevel;
		m_PrevRTPCParams.fRearLevel = m_pParams->sRTPCParams.fRearLevel;
		m_PrevRTPCParams.fLFELevel = m_pParams->sRTPCParams.fLFELevel;

		// Advance pointers
		uFrameOffset += uNumFrames;
		uNumFramesRemaining -= uNumFrames;
	}
	
	if ( pfTempStorage )
	{
		AK_PLUGIN_FREE( m_pAllocator, pfTempStorage );
		pfTempStorage = NULL;
	}

	return AK_Success;
}

#ifdef AK_71AUDIO
// This routine handles 6.0 to 7.1
/// LX NOTE: Currently, this routine is identical as Spread3 except that it duplicates surround channels.
/// Use it as a starting point to experiment with other schemes: larger sub-FDN, decorrelation filter,
/// virtual microphones out of 2 perfectly decorrelated ambisonic stream.
AKRESULT CAkRoomVerbFX::ProcessSpread4Out( AkAudioBuffer * io_pBuffer )
{
	// Allocate all temporary processing buffer in one shot and dispatch
	AkChannelMask uChannelMask = io_pBuffer->GetChannelMask();
	AkUInt32 uNumTempBuffers = 8 + 2*m_Reverb.uNumERSignals;
	AkReal32 * pfTempStorage  = (AkReal32*) AK_PLUGIN_ALLOC( m_pAllocator, uNumTempBuffers * sizeof(AkReal32) * NUMPROCESSFRAMES );
	if ( !pfTempStorage )
		return AK_InsufficientMemory;

	AkReal32 * pfERIn  = &pfTempStorage[0]; 	
	AkReal32 * pfReverbIn = &pfTempStorage[NUMPROCESSFRAMES];
	AkReal32 * pfReverbOut[6];
	pfReverbOut[0] = &pfTempStorage[2*NUMPROCESSFRAMES];
	pfReverbOut[1] = &pfTempStorage[3*NUMPROCESSFRAMES];
	pfReverbOut[2] = &pfTempStorage[4*NUMPROCESSFRAMES];
	pfReverbOut[3] = &pfTempStorage[5*NUMPROCESSFRAMES];
	pfReverbOut[4] = &pfTempStorage[6*NUMPROCESSFRAMES];
	pfReverbOut[5] = &pfTempStorage[7*NUMPROCESSFRAMES];
	// Possible memory optimization: If eearly reflection signals are mixed immediately with dry signal,
	// ReverbOut buffers could reused them (4 * NUMPROCESSFRAMES * sizeof(Akreal32) bytes saving
	// Doing so will imply more memory access and more mixing operations on the other hand.
	AkReal32 * pfEROut[4] = { NULL, NULL, NULL, NULL };
	if ( m_Reverb.uNumERSignals ) 
	{
		pfEROut[0] = &pfTempStorage[8*NUMPROCESSFRAMES];
		pfEROut[1] = &pfTempStorage[9*NUMPROCESSFRAMES];
		pfEROut[2] = &pfTempStorage[10*NUMPROCESSFRAMES];
		pfEROut[3] = &pfTempStorage[11*NUMPROCESSFRAMES];
	}

	COMPUTELRMIX( m_PrevRTPCParams.fStereoWidth, m_pParams->sRTPCParams.fStereoWidth );

	AkUInt32 uFrameOffset = 0;
	AkUInt32 uNumFramesRemaining = io_pBuffer->uValidFrames;
	while ( uNumFramesRemaining )
	{
		AkUInt32 uNumFrames = AkMin( uNumFramesRemaining, NUMPROCESSFRAMES );

		// Mixdown, tone controls and pre-delay
		WetPreProcess( io_pBuffer, pfERIn, uNumFrames, uFrameOffset );

		// Split result into ER and reverb feed
		if ( m_Reverb.ReverbDelay.GetDelayLength() > 0 )
			m_Reverb.ReverbDelay.ProcessBuffer( pfERIn, pfReverbIn, uNumFrames );
		else
			AKPLATFORM::AkMemCpy(pfReverbIn, pfERIn, uNumFrames*sizeof(AkReal32) ); 

		if ( m_Reverb.ERDelay.GetDelayLength() > 0 )
			m_Reverb.ERDelay.ProcessBuffer( pfERIn, uNumFrames );

		if ( m_Reverb.uNumERSignals ) 
		{
			// Early reflections process
			m_pERUnit->ProcessBuffer( pfERIn, pfEROut[0], pfEROut[1], uNumFrames );

			// ER tone controls
			if ( m_pParams->sInvariantParams.bEnableToneControls )
			{
				for ( AkUInt8 i = 0; i < m_Reverb.uTCFilterIndex[0]; ++i )
				{
					if ( m_pTCFiltersState[i].FilterPos == FILTERINSERTTYPE_ERONLY )
					{
						m_pTCFiltersState[i].Filter.ProcessBuffer( pfEROut[0], uNumFrames );
						m_pTCFiltersState[i+1].Filter.ProcessBuffer( pfEROut[1], uNumFrames );
						++i;
					}
				}
			}

			if ( m_Reverb.ERFrontBackDelay[0].GetDelayLength() > 0 )
			{
				m_Reverb.ERFrontBackDelay[0].ProcessBuffer( pfEROut[0], pfEROut[2], uNumFrames );
				m_Reverb.ERFrontBackDelay[1].ProcessBuffer( pfEROut[1], pfEROut[3], uNumFrames );}
			else
			{
				pfEROut[2] = pfEROut[0];
				pfEROut[3] = pfEROut[1];
			}
		}

		// Diffusion tank
		for ( AkUInt32 i = 0; i < NUMDIFFUSIONALLPASSFILTERS; i++ )
		{
			m_Reverb.DiffusionFilters[i].ProcessBuffer( pfReverbIn, uNumFrames );
		}

		// Reverb EQ
		ReverbPreProcess( pfReverbIn, uNumFrames );
	
		// Even units go into left tanks
		// Odd units go into right tanks
		AkZeroMemLarge( pfReverbOut[0], 6 * NUMPROCESSFRAMES * sizeof(AkReal32) );
		for ( AkUInt32 i = 0; i < m_Reverb.uNumReverbUnits; i++ )
		{	
			m_pReverbUnitsState[i].RUInputDelay.ProcessBuffer( pfReverbIn, uNumFrames );
			m_pReverbUnitsState[i].ReverbUnits.ProcessBufferAccum( pfReverbIn, pfReverbOut[i%2], pfReverbOut[(i&1)+2], pfReverbOut[(i&1)+4], uNumFrames );
		}

		ReverbPostProcess( pfReverbOut, 6, 1.41421356f*m_Reverb.fReverbUnitsMixGain, uNumFrames );
		
		AkUInt32 uCurChannel = 0;
		// Front left
		{
			AkReal32 * pfDryChannel = io_pBuffer->GetChannel(uCurChannel) + uFrameOffset;
			Mix3Interp( pfDryChannel, 
						pfReverbOut[0],
						pfReverbOut[1], 
						m_PrevRTPCParams.fDryLevel,
						m_pParams->sRTPCParams.fDryLevel,
						m_PrevRTPCParams.fFrontLevel*m_PrevRTPCParams.fReverbLevel*fPrevGain1,
						m_pParams->sRTPCParams.fFrontLevel*m_pParams->sRTPCParams.fReverbLevel*fGain1,
						m_PrevRTPCParams.fFrontLevel*m_PrevRTPCParams.fReverbLevel*fPrevGain2,
						m_pParams->sRTPCParams.fFrontLevel*m_pParams->sRTPCParams.fReverbLevel*fGain2,
						uNumFrames );
			if ( m_Reverb.uNumERSignals )
			{
				Mix3Interp( pfDryChannel, 
							pfEROut[0],
							pfEROut[1], 
							1.f,
							1.f,
							m_PrevRTPCParams.fFrontLevel*m_PrevRTPCParams.fERLevel*fPrevGain1,
							m_pParams->sRTPCParams.fFrontLevel*m_pParams->sRTPCParams.fERLevel*fGain1,
							m_PrevRTPCParams.fFrontLevel*m_PrevRTPCParams.fERLevel*fPrevGain2,
							m_pParams->sRTPCParams.fFrontLevel*m_pParams->sRTPCParams.fERLevel*fGain2,
							uNumFrames );	
			}
			uCurChannel++;
		}

		// Front right
		{
			AkReal32 * pfDryChannel = io_pBuffer->GetChannel(uCurChannel) + uFrameOffset;
			Mix3Interp( pfDryChannel, 
						pfReverbOut[0],
						pfReverbOut[1], 
						m_PrevRTPCParams.fDryLevel,
						m_pParams->sRTPCParams.fDryLevel,
						m_PrevRTPCParams.fFrontLevel*m_PrevRTPCParams.fReverbLevel*fPrevGain2,
						m_pParams->sRTPCParams.fFrontLevel*m_pParams->sRTPCParams.fReverbLevel*fGain2,
						m_PrevRTPCParams.fFrontLevel*m_PrevRTPCParams.fReverbLevel*fPrevGain1,
						m_pParams->sRTPCParams.fFrontLevel*m_pParams->sRTPCParams.fReverbLevel*fGain1,
						uNumFrames );
			if ( m_Reverb.uNumERSignals )
			{
				Mix3Interp( pfDryChannel, 
							pfEROut[0],
							pfEROut[1], 
							1.f,
							1.f,
							m_PrevRTPCParams.fFrontLevel*m_PrevRTPCParams.fERLevel*fPrevGain2,
							m_pParams->sRTPCParams.fFrontLevel*m_pParams->sRTPCParams.fERLevel*fGain2,
							m_PrevRTPCParams.fFrontLevel*m_PrevRTPCParams.fERLevel*fPrevGain1,
							m_pParams->sRTPCParams.fFrontLevel*m_pParams->sRTPCParams.fERLevel*fGain1,
							uNumFrames );	
			}
			uCurChannel++;
		}

		if ( uChannelMask & AK_SPEAKER_FRONT_CENTER )
		{
			// No ER
			AkReal32 * pfDryChannel = io_pBuffer->GetChannel(uCurChannel) + uFrameOffset;
			Mix3Interp( pfDryChannel, 
						pfReverbOut[2],
						pfReverbOut[3], 
						m_PrevRTPCParams.fDryLevel,
						m_pParams->sRTPCParams.fDryLevel,
						m_PrevRTPCParams.fCenterLevel*m_PrevRTPCParams.fReverbLevel*HALFPOWERGAIN,
						m_pParams->sRTPCParams.fCenterLevel*m_pParams->sRTPCParams.fReverbLevel*HALFPOWERGAIN,
						m_PrevRTPCParams.fCenterLevel*m_PrevRTPCParams.fReverbLevel*HALFPOWERGAIN,
						m_pParams->sRTPCParams.fCenterLevel*m_pParams->sRTPCParams.fReverbLevel*HALFPOWERGAIN,
						uNumFrames );
			uCurChannel++;
		}

		float fCurGainRvbBL = m_PrevRTPCParams.fRearLevel*m_PrevRTPCParams.fReverbLevel*fPrevGain1;
		float fTgtGainRvbBL = m_pParams->sRTPCParams.fRearLevel*m_pParams->sRTPCParams.fReverbLevel*fGain1;
		float fCurGainRvbBR = m_PrevRTPCParams.fRearLevel*m_PrevRTPCParams.fReverbLevel*fPrevGain2;
		float fTgtGainRvbBR = m_pParams->sRTPCParams.fRearLevel*m_pParams->sRTPCParams.fReverbLevel*fGain2;

		float fCurGainErBL = m_PrevRTPCParams.fRearLevel*m_PrevRTPCParams.fERLevel*fPrevGain1;
		float fTgtGainErBL = m_pParams->sRTPCParams.fRearLevel*m_pParams->sRTPCParams.fERLevel*fGain1;
		float fCurGainErBR = m_PrevRTPCParams.fRearLevel*m_PrevRTPCParams.fERLevel*fPrevGain2;
		float fTgtGainErBR = m_pParams->sRTPCParams.fRearLevel*m_pParams->sRTPCParams.fERLevel*fGain2;

		// Rear left
		{
			AkReal32 * pfDryChannel = io_pBuffer->GetChannel(uCurChannel) + uFrameOffset;
			Mix3Interp( pfDryChannel, 
						pfReverbOut[4],
						pfReverbOut[5], 
						m_PrevRTPCParams.fDryLevel,
						m_pParams->sRTPCParams.fDryLevel,
						fCurGainRvbBL,
						fTgtGainRvbBL,
						fCurGainRvbBR,
						fTgtGainRvbBR,
						uNumFrames );
			if ( m_Reverb.uNumERSignals )
			{
				Mix3Interp( pfDryChannel, 
							pfEROut[2],
							pfEROut[3], 
							1.f,
							1.f,
							fCurGainErBL,
							fTgtGainErBL,
							fCurGainErBR,
							fTgtGainErBR,
							uNumFrames );
			}
			uCurChannel++;
		}
		
		// Rear right
		{
			AkReal32 * pfDryChannel = io_pBuffer->GetChannel(uCurChannel) + uFrameOffset;
			Mix3Interp( pfDryChannel, 
						pfReverbOut[4],
						pfReverbOut[5], 
						m_PrevRTPCParams.fDryLevel,
						m_pParams->sRTPCParams.fDryLevel,
						fCurGainRvbBR,
						fTgtGainRvbBR,
						fCurGainRvbBL,
						fTgtGainRvbBL,
						uNumFrames );
			if ( m_Reverb.uNumERSignals )
			{
				Mix3Interp( pfDryChannel, 
							pfEROut[2],
							pfEROut[3], 
							1.f,
							1.f,
							fCurGainErBR,
							fTgtGainErBR,
							fCurGainErBL,
							fTgtGainErBL,
							uNumFrames );	
			}
			uCurChannel++;
		}

		// Side left		
		{
			AkReal32 * pfDryChannel = io_pBuffer->GetChannel(uCurChannel) + uFrameOffset;
			Mix3Interp( pfDryChannel, 
				pfReverbOut[4],
				pfReverbOut[5], 
				m_PrevRTPCParams.fDryLevel,
				m_pParams->sRTPCParams.fDryLevel,
				fCurGainRvbBL,
				fTgtGainRvbBL,
				fCurGainRvbBR,
				fTgtGainRvbBR,
				uNumFrames );
			if ( m_Reverb.uNumERSignals )
			{
				Mix3Interp( pfDryChannel, 
					pfEROut[2],
					pfEROut[3], 
					1.f,
					1.f,
					fCurGainErBL,
					fTgtGainErBL,
					fCurGainErBR,
					fTgtGainErBR,
					uNumFrames );
			}
			uCurChannel++;
		}

		// Side right
		{
			AkReal32 * pfDryChannel = io_pBuffer->GetChannel(uCurChannel) + uFrameOffset;
			Mix3Interp( pfDryChannel, 
				pfReverbOut[4],
				pfReverbOut[5], 
				m_PrevRTPCParams.fDryLevel,
				m_pParams->sRTPCParams.fDryLevel,
				fCurGainRvbBR,
				fTgtGainRvbBR,
				fCurGainRvbBL,
				fTgtGainRvbBL,
				uNumFrames );
			if ( m_Reverb.uNumERSignals )
			{
				Mix3Interp( pfDryChannel, 
					pfEROut[2],
					pfEROut[3], 
					1.f,
					1.f,
					fCurGainErBR,
					fTgtGainErBR,
					fCurGainErBL,
					fTgtGainErBL,
					uNumFrames );	
			}
			uCurChannel++;
		}

		if ( uChannelMask & AK_SPEAKER_LOW_FREQUENCY )
		{
			// No ER
			AkReal32 * pfDryChannel = io_pBuffer->GetChannel(uCurChannel) + uFrameOffset;	
			Mix3Interp( pfDryChannel, 
						pfReverbOut[2],
						pfReverbOut[3], 
						m_PrevRTPCParams.fDryLevel,
						m_pParams->sRTPCParams.fDryLevel,
						m_PrevRTPCParams.fLFELevel*m_PrevRTPCParams.fReverbLevel*HALFPOWERGAIN,
						m_pParams->sRTPCParams.fLFELevel*m_pParams->sRTPCParams.fReverbLevel*HALFPOWERGAIN,
						m_PrevRTPCParams.fLFELevel*m_PrevRTPCParams.fReverbLevel*HALFPOWERGAIN,
						m_pParams->sRTPCParams.fLFELevel*m_pParams->sRTPCParams.fReverbLevel*HALFPOWERGAIN,
						uNumFrames );
			uCurChannel++;
		}

		fPrevGain1 = fGain1;
		fPrevGain2 = fGain2;
		m_PrevRTPCParams.fDryLevel = m_pParams->sRTPCParams.fDryLevel;
		m_PrevRTPCParams.fERLevel = m_pParams->sRTPCParams.fERLevel;
		m_PrevRTPCParams.fReverbLevel = m_pParams->sRTPCParams.fReverbLevel;
		m_PrevRTPCParams.fFrontLevel = m_pParams->sRTPCParams.fFrontLevel;
		m_PrevRTPCParams.fCenterLevel = m_pParams->sRTPCParams.fCenterLevel;
		m_PrevRTPCParams.fRearLevel = m_pParams->sRTPCParams.fRearLevel;
		m_PrevRTPCParams.fLFELevel = m_pParams->sRTPCParams.fLFELevel;

		// Advance pointers
		uFrameOffset += uNumFrames;
		uNumFramesRemaining -= uNumFrames;
	}
	
	if ( pfTempStorage )
	{
		AK_PLUGIN_FREE( m_pAllocator, pfTempStorage );
		pfTempStorage = NULL;
	}

	return AK_Success;
}
#endif // 71

#endif
