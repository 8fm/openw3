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
// AkParametricEQFXParams.h
//
// Shared parameter implementation for parametric EQ FX.
//
// Copyright 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#ifndef _AK_PARAMETEREQPARAMS_H_
#define _AK_PARAMETEREQPARAMS_H_

#include <AK/Plugin/AkParametricEQFXFactory.h>
#include <math.h>
#include <AK/Tools/Common/AkAssert.h>
#include "AkParametricEQFXCommon.h"

// Parameters IDs for the Wwise or RTPC.
static const AkPluginParamID AK_PARAMETRICEQFXPARAM_FILTERTYPEBAND1_ID	= 0;	
static const AkPluginParamID AK_PARAMETRICEQFXPARAM_GAINBAND1_ID		= 1;
static const AkPluginParamID AK_PARAMETRICEQFXPARAM_FREQUENCYBAND1_ID	= 2;	
static const AkPluginParamID AK_PARAMETRICEQFXPARAM_QFACTORBAND1_ID		= 3;
static const AkPluginParamID AK_PARAMETRICEQFXPARAM_ONOFFBAND1_ID		= 4;

static const AkPluginParamID AK_PARAMETRICEQFXPARAM_FILTERTYPEBAND2_ID	= 5;	
static const AkPluginParamID AK_PARAMETRICEQFXPARAM_GAINBAND2_ID		= 6;
static const AkPluginParamID AK_PARAMETRICEQFXPARAM_FREQUENCYBAND2_ID	= 7;	
static const AkPluginParamID AK_PARAMETRICEQFXPARAM_QFACTORBAND2_ID		= 8;
static const AkPluginParamID AK_PARAMETRICEQFXPARAM_ONOFFBAND2_ID		= 9;

static const AkPluginParamID AK_PARAMETRICEQFXPARAM_FILTERTYPEBAND3_ID	= 10;	
static const AkPluginParamID AK_PARAMETRICEQFXPARAM_GAINBAND3_ID		= 11;
static const AkPluginParamID AK_PARAMETRICEQFXPARAM_FREQUENCYBAND3_ID	= 12;	
static const AkPluginParamID AK_PARAMETRICEQFXPARAM_QFACTORBAND3_ID		= 13;
static const AkPluginParamID AK_PARAMETRICEQFXPARAM_ONOFFBAND3_ID		= 14;

static const AkPluginParamID AK_PARAMETRICEQFXPARAM_OUTPUTLEVEL_ID		= 15;
static const AkPluginParamID AK_PARAMETRICEQFXPARAM_PROCESSLFE_ID		= 16;

static const AkPluginParamID AK_PARAMETRICEQ_NUMPERMODULE				= 5;
static const AkPluginParamID AK_PARAMETRICEQ_NUM						= 17;

//-----------------------------------------------------------------------------
// Structures.
//-----------------------------------------------------------------------------

// Early reflection mode
enum AkFilterType
{
	AKFILTERTYPE_LOWPASS	=  0,	// Low-pass filter
	AKFILTERTYPE_HIPASS		=  1,	// Hi-pass filter
	AKFILTERTYPE_BANDPASS	=  2,	// Band-pass filter
	AKFILTERTYPE_NOTCH		=  3,	// Notch filter
	AKFILTERTYPE_LOWSHELF	=  4,	// Low-shelf filter
	AKFILTERTYPE_HISHELF	=  5,	// Hi-shelf filter
	AKFILTERTYPE_PEAKINGEQ	=  6	// PeakingEQ filter
};

// Default values in case a bad parameter block is provided
#define PARAMETRICEQ_FILTERTYPE_BAND1_DEF		(AKFILTERTYPE_LOWSHELF)
#define PARAMETRICEQ_GAIN_BAND1_DEF				(0.f)
#define PARAMETRICEQ_FREQUENCY_BAND1_DEF		(120.f)
#define PARAMETRICEQ_QFACTOR_BAND1_DEF			(5.f)
#define PARAMETRICEQ_ONOFF_BAND1_DEF			(true)

#define PARAMETRICEQ_FILTERTYPE_BAND2_DEF		(AKFILTERTYPE_PEAKINGEQ)
#define PARAMETRICEQ_GAIN_BAND2_DEF				(0.f)
#define PARAMETRICEQ_FREQUENCY_BAND2_DEF		(2000.f)
#define PARAMETRICEQ_QFACTOR_BAND2_DEF			(5.f)
#define PARAMETRICEQ_ONOFF_BAND2_DEF			(true)

#define PARAMETRICEQ_FILTERTYPE_BAND3_DEF		(AKFILTERTYPE_HISHELF)
#define PARAMETRICEQ_GAIN_BAND3_DEF				(0.f)
#define PARAMETRICEQ_FREQUENCY_BAND3_DEF		(5000.f)
#define PARAMETRICEQ_QFACTOR_BAND3_DEF			(5.f)
#define PARAMETRICEQ_ONOFF_BAND3_DEF			(true)

#define PARAMETRICEQ_OUTOUTLEVEL_DEF			(0.f)
#define PARAMETRICEQ_PROCESSLFE_DEF				(true)

// Structure of a single parametric EQ module
struct EQModuleParams
{
	AkFilterType	eFilterType;
	AkReal32		fGain;
	AkReal32		fFrequency;
	AkReal32		fQFactor;
	bool			bOnOff;
};

// Structure of ParametricEQ parameters
struct AkParametricEQFXParams
{
	EQModuleParams	Band[NUMBER_FILTER_MODULES];
	AkReal32		fOutputLevel;
	bool			bProcessLFE;
};

//-----------------------------------------------------------------------------
// Name: class CAkParameterEQFXParams
// Desc: Shared parametric EQ FX parameters implementation.
//-----------------------------------------------------------------------------
class CAkParameterEQFXParams : public AK::IAkPluginParam
{
public:

	friend class CAkParametricEQFX;
 
    // Constructor/destructor.
    CAkParameterEQFXParams( );
    ~CAkParameterEQFXParams( );
	CAkParameterEQFXParams( const CAkParameterEQFXParams & in_rCopy );

    // Create duplicate.
    IAkPluginParam * Clone( AK::IAkPluginMemAlloc * in_pAllocator );

    // Init/Term.
    AKRESULT Init(	AK::IAkPluginMemAlloc *	in_pAllocator,						    
					const void *			in_pParamsBlock, 
					AkUInt32				in_ulBlockSize 
                         );
    AKRESULT Term( AK::IAkPluginMemAlloc * in_pAllocator );

    // Blob set.
    AKRESULT SetParamsBlock(	const void * in_pParamsBlock, 
								AkUInt32 in_ulBlockSize
                                );

    // Update one parameter.
    AKRESULT SetParam(	AkPluginParamID in_ParamID,
						const void * in_pValue, 
						AkUInt32 in_ulParamSize
                        );

private:

	//// Internal API functions ////
	bool GetDirty( AkBandNumber in_eBandNum );
	void SetDirty( AkBandNumber in_eBandNum, bool in_bDirty );
	EQModuleParams * GetFilterModuleParams(	AkBandNumber in_eBandNum );
	AkReal32 GetOutputLevel( );
	bool GetProcessLFE( );

private:

    AkParametricEQFXParams	m_Params;								// Parameter structure.
	bool					m_bBandDirty[NUMBER_FILTER_MODULES];	// IsDirty flag per filter module (internal parameter)
};

// Determine if parameters in band has changed
inline bool CAkParameterEQFXParams::GetDirty( AkBandNumber in_eBandNum )
{
	AKASSERT( in_eBandNum >= BAND1 && in_eBandNum <= BAND3 );
	bool bIsDirty = m_bBandDirty[in_eBandNum];
	return bIsDirty;
}

// Determine if parameters in band has changed
inline void CAkParameterEQFXParams::SetDirty( AkBandNumber in_eBandNum, bool in_bDirty )
{
	AKASSERT( in_eBandNum >= BAND1 && in_eBandNum <= BAND3 );
	m_bBandDirty[in_eBandNum] = in_bDirty;
}

// Get all filter parameter at once
inline EQModuleParams * CAkParameterEQFXParams::GetFilterModuleParams(	AkBandNumber in_eBandNumber )
{
	AKASSERT( in_eBandNumber >= BAND1 && in_eBandNumber <= BAND3 );
	return &( m_Params.Band[in_eBandNumber] );
}

// GetOutputLevel
inline AkReal32 CAkParameterEQFXParams::GetOutputLevel( )
{
	AkReal32 fOutputLevel = m_Params.fOutputLevel;
	fOutputLevel = powf( 10.f, ( fOutputLevel * 0.05f ) );
	return fOutputLevel;
}

// GetProcessLFE
inline bool CAkParameterEQFXParams::GetProcessLFE( )
{
	return m_Params.bProcessLFE;
}

#endif // _AK_PARAMETEREQPARAMS_H_
