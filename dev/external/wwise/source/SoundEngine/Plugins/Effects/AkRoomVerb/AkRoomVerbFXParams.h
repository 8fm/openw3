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

#ifndef _AK_ROOMVERBFXPARAMS_H_
#define _AK_ROOMVERBFXPARAMS_H_

#include <AK/SoundEngine/Common/AkTypes.h>
#include <AK/Plugin/AkRoomVerbFXFactory.h>
#include "AlgoTunings.h"

// Structure of time-varying parameters of the effect.
struct RTPCParams
{
	// Reverb Controls
	AkReal32	fDecayTime;
	AkReal32	fHFDamping;
	AkReal32	fDiffusion;
	AkReal32	fStereoWidth;
	// Tone controls
	AkReal32	fFilter1Gain;
	AkReal32	fFilter1Freq;
	AkReal32	fFilter1Q;
	AkReal32	fFilter2Gain;
	AkReal32	fFilter2Freq;
	AkReal32	fFilter2Q;
	AkReal32	fFilter3Gain;
	AkReal32	fFilter3Freq;
	AkReal32	fFilter3Q;
	// Reverb levels
	AkReal32	fFrontLevel;
	AkReal32	fRearLevel;
	AkReal32	fCenterLevel;
	AkReal32	fLFELevel;
	// Output levels
	AkReal32	fDryLevel;
	AkReal32	fERLevel;
	AkReal32	fReverbLevel;

	bool		bDirty;
} AK_ALIGN_DMA;

enum FilterInsertType
{
	FILTERINSERTTYPE_OFF			=  0,	// No filter
	FILTERINSERTTYPE_ERONLY			=  1,	// Affect early reflection only
	FILTERINSERTTYPE_REVERBONLY		=  2,	// Affect reverb tail only
	FILTERINSERTTYPE_ERANDREVERB	=  3	// Affect both ER and reverb tail
};

enum FilterCurveType
{
	FILTERCURVETYPE_LOWSHELF		=  0,	
	FILTERCURVETYPE_PEAKING			=  1,	
	FILTERCURVETYPE_HIGHSHELF		=  2
};

// Structure of parameters that remain the same for the whole lifespan of the effect.
struct InvariantParams
{
	AkUInt32	uERPattern;
	AkReal32	fReverbDelay;
	AkReal32	fRoomSize;
	AkReal32	fERFrontBackDelay;

	AkReal32	fDensity;
	AkReal32	fRoomShape;
	AkUInt32	uNumReverbUnits; // Quality
	
	AkReal32	fInputCenterLevel;
	AkReal32	fInputLFELevel;

	FilterInsertType	eFilter1Pos;
	FilterCurveType		eFilter1Curve;
	FilterInsertType	eFilter2Pos;
	FilterCurveType		eFilter2Curve;
	FilterInsertType	eFilter3Pos;
	FilterCurveType		eFilter3Curve;
	bool				bEnableToneControls;
	bool				bEnableEarlyReflections;

	bool		bDirty;
};

struct AkRoomVerbFXParams
{
	RTPCParams			sRTPCParams;
	InvariantParams		sInvariantParams;
	AlgorithmTunings	sAlgoTunings;
} AK_ALIGN_DMA;

#ifndef __SPU__

#include <AK/Tools/Common/AkAssert.h>
#include <math.h>

// Parameters IDs for Wwise or RTPC.
static const AkPluginParamID AK_ROOMVERBFXPARAM_ERPATTERN_ID				= 0;
static const AkPluginParamID AK_ROOMVERBFXPARAM_REVERBDELAY_ID				= 1;
static const AkPluginParamID AK_ROOMVERBFXPARAM_ROOMSIZE_ID					= 2;	
static const AkPluginParamID AK_ROOMVERBFXPARAM_ERFRONTBACKDELAY_ID			= 3;
static const AkPluginParamID AK_ROOMVERBFXPARAM_ENABLEEARLYREFLECTIONS_ID	= 4;

static const AkPluginParamID AK_ROOMVERBFXPARAM_DECAYTIME_ID				= 10;
static const AkPluginParamID AK_ROOMVERBFXPARAM_HFDAMPING_ID				= 11;
static const AkPluginParamID AK_ROOMVERBFXPARAM_DENSITY_ID					= 12;
static const AkPluginParamID AK_ROOMVERBFXPARAM_ROOMSHAPE_ID				= 13;
static const AkPluginParamID AK_ROOMVERBFXPARAM_NUMREVERBUNITS_ID			= 14;
static const AkPluginParamID AK_ROOMVERBFXPARAM_DIFFUSION_ID				= 15;
static const AkPluginParamID AK_ROOMVERBFXPARAM_STEREOWIDTH_ID				= 16;

static const AkPluginParamID AK_ROOMVERBFXPARAM_ENABLETONECONTROLS_ID		= 20;
static const AkPluginParamID AK_ROOMVERBFXPARAM_FILTER1INSERT_ID			= 21;
static const AkPluginParamID AK_ROOMVERBFXPARAM_FILTER1CURVE_ID				= 22;
static const AkPluginParamID AK_ROOMVERBFXPARAM_FILTER1GAIN_ID				= 23;
static const AkPluginParamID AK_ROOMVERBFXPARAM_FILTER1FREQ_ID				= 24;
static const AkPluginParamID AK_ROOMVERBFXPARAM_FILTER1Q_ID					= 25;
static const AkPluginParamID AK_ROOMVERBFXPARAM_FILTER2INSERT_ID			= 26;
static const AkPluginParamID AK_ROOMVERBFXPARAM_FILTER2CURVE_ID				= 27;
static const AkPluginParamID AK_ROOMVERBFXPARAM_FILTER2GAIN_ID				= 28;
static const AkPluginParamID AK_ROOMVERBFXPARAM_FILTER2FREQ_ID				= 29;
static const AkPluginParamID AK_ROOMVERBFXPARAM_FILTER2Q_ID					= 30;
static const AkPluginParamID AK_ROOMVERBFXPARAM_FILTER3INSERT_ID			= 31;
static const AkPluginParamID AK_ROOMVERBFXPARAM_FILTER3CURVE_ID				= 32;
static const AkPluginParamID AK_ROOMVERBFXPARAM_FILTER3GAIN_ID				= 33;
static const AkPluginParamID AK_ROOMVERBFXPARAM_FILTER3FREQ_ID				= 34;	
static const AkPluginParamID AK_ROOMVERBFXPARAM_FILTER3Q_ID					= 35;

static const AkPluginParamID AK_ROOMVERBFXPARAM_INPUTCENTERLEVEL_ID			= 40;	
static const AkPluginParamID AK_ROOMVERBFXPARAM_INPUTLFELEVEL_ID			= 41;	
	
static const AkPluginParamID AK_ROOMVERBFXPARAM_FRONTLEVEL_ID				= 50;
static const AkPluginParamID AK_ROOMVERBFXPARAM_REARLEVEL_ID				= 51;
static const AkPluginParamID AK_ROOMVERBFXPARAM_CENTERLEVEL_ID				= 52;	
static const AkPluginParamID AK_ROOMVERBFXPARAM_LFELEVEL_ID					= 53;	

static const AkPluginParamID AK_ROOMVERBFXPARAM_DRYLEVEL_ID					= 60;	
static const AkPluginParamID AK_ROOMVERBFXPARAM_ERLEVEL_ID					= 61;	
static const AkPluginParamID AK_ROOMVERBFXPARAM_REVERBLEVEL_ID				= 62;	

// Algorithm tunings IDs
static const AkPluginParamID AK_ROOMVERBFXTUNING_DENSITYDELAYMIN_ID				= 100;
static const AkPluginParamID AK_ROOMVERBFXTUNING_DENSITYDELAYMAX_ID				= 101;
static const AkPluginParamID AK_ROOMVERBFXTUNING_DENSITYDELAYRDMPERC_ID			= 102;
static const AkPluginParamID AK_ROOMVERBFXTUNING_ROOMSHAPEMIN_ID				= 103;	
static const AkPluginParamID AK_ROOMVERBFXTUNING_ROOMSHAPEMAX_ID				= 104;	
static const AkPluginParamID AK_ROOMVERBFXTUNING_DIFFUSIONDELAYSCALEPERC_ID		= 105;
static const AkPluginParamID AK_ROOMVERBFXTUNING_DIFFUSIONDELAYMAX_ID			= 106;
static const AkPluginParamID AK_ROOMVERBFXTUNING_DIFFUSIONDELAYRDMPERC_ID		= 107;
static const AkPluginParamID AK_ROOMVERBFXTUNING_DCFILTERCUTFREQ_ID				= 108;
static const AkPluginParamID AK_ROOMVERBFXTUNING_REVERBUNITINPUTDELAY_ID		= 109;
static const AkPluginParamID AK_ROOMVERBFXTUNING_REVERBUNITINPUTDELAYRMDPERC_ID	= 110;

// Default parameters values
// TODO: Use medium room preset to determine those
static const AkUInt32 AK_ROOMVERBFXPARAM_ERPATTERN_DEF						= 23;
static const AkReal32 AK_ROOMVERBFXPARAM_REVERBDELAY_DEF					= 25.f;
static const AkReal32 AK_ROOMVERBFXPARAM_ROOMSIZE_DEF						= 0.f;
static const AkReal32 AK_ROOMVERBFXPARAM_ERFRONTBACKDELAY_DEF				= 40.f;
static const bool	  AK_ROOMVERBFXPARAM_ENABLEEARLYREFLECTIONS_DEF			= true;

static const AkReal32 AK_ROOMVERBFXPARAM_DECAYTIME_DEF						= 1.2f;
static const AkReal32 AK_ROOMVERBFXPARAM_HFDAMPING_DEF						= 2.25f;
static const AkReal32 AK_ROOMVERBFXPARAM_DENSITY_DEF						= 80.f;
static const AkReal32 AK_ROOMVERBFXPARAM_ROOMSHAPE_DEF						= 100.f;
static const AkUInt32 AK_ROOMVERBFXPARAM_NUMREVERBUNITS_DEF					= 8;
static const AkReal32 AK_ROOMVERBFXPARAM_DIFFUSION_DEF						= 100.f;
static const AkReal32 AK_ROOMVERBFXPARAM_STEREOWIDTH_DEF					= 180.f;

static const bool				AK_ROOMVERBFXPARAM_ENABLETONECONTROLS_DEF	= false;
static const FilterInsertType	AK_ROOMVERBFXPARAM_FILTER1INSERT_DEF		= FILTERINSERTTYPE_ERANDREVERB;
static const FilterCurveType	AK_ROOMVERBFXPARAM_FILTER1CURVE_DEF			= FILTERCURVETYPE_LOWSHELF;
static const AkReal32			AK_ROOMVERBFXPARAM_FILTER1GAIN_DEF			= 0.f;
static const AkReal32			AK_ROOMVERBFXPARAM_FILTER1FREQ_DEF			= 100.f;
static const AkReal32			AK_ROOMVERBFXPARAM_FILTER1Q_DEF				= 1.f;
static const FilterInsertType	AK_ROOMVERBFXPARAM_FILTER2INSERT_DEF		= FILTERINSERTTYPE_ERANDREVERB;
static const FilterCurveType	AK_ROOMVERBFXPARAM_FILTER2CURVE_DEF			= FILTERCURVETYPE_PEAKING;
static const AkReal32			AK_ROOMVERBFXPARAM_FILTER2GAIN_DEF			= 0.f;
static const AkReal32			AK_ROOMVERBFXPARAM_FILTER2FREQ_DEF			= 1000.f;
static const AkReal32			AK_ROOMVERBFXPARAM_FILTER2Q_DEF				= 1.f;
static const FilterInsertType	AK_ROOMVERBFXPARAM_FILTER3INSERT_DEF		= FILTERINSERTTYPE_ERANDREVERB;
static const FilterCurveType	AK_ROOMVERBFXPARAM_FILTER3CURVE_DEF			= FILTERCURVETYPE_HIGHSHELF;
static const AkReal32			AK_ROOMVERBFXPARAM_FILTER3GAIN_DEF			= 0.f;	
static const AkReal32			AK_ROOMVERBFXPARAM_FILTER3FREQ_DEF			= 10000.f;
static const AkReal32			AK_ROOMVERBFXPARAM_FILTER3Q_DEF				= 1.f;

static const AkReal32 AK_ROOMVERBFXPARAM_INPUTCENTERLEVEL_DEF				= 0.f;	
static const AkReal32 AK_ROOMVERBFXPARAM_INPUTLFELEVEL_DEF					= -96.f;

static const AkReal32 AK_ROOMVERBFXPARAM_FRONTLEVEL_DEF						= 0.f;
static const AkReal32 AK_ROOMVERBFXPARAM_REARLEVEL_DEF						= 0.f;
static const AkReal32 AK_ROOMVERBFXPARAM_CENTERLEVEL_DEF					= 0.f;	
static const AkReal32 AK_ROOMVERBFXPARAM_LFELEVEL_DEF						= -96.f;	

static const AkReal32 AK_ROOMVERBFXPARAM_DRYLEVEL_DEF						= 0.f;	
static const AkReal32 AK_ROOMVERBFXPARAM_ERLEVEL_DEF						= -20.f;	
static const AkReal32 AK_ROOMVERBFXPARAM_REVERBLEVEL_DEF					= -20.f;

//-----------------------------------------------------------------------------
// Name: class CAkRoomVerbFXParams
// Desc: Shared RoomVerb FX parameters implementation.
//-----------------------------------------------------------------------------
class CAkRoomVerbFXParams 
	: public AK::IAkPluginParam
	, public AkRoomVerbFXParams
{
public:
    
    // Constructor/destructor.
    CAkRoomVerbFXParams( );
    ~CAkRoomVerbFXParams();
	CAkRoomVerbFXParams( const CAkRoomVerbFXParams & in_rCopy );

    // Create duplicate.
    IAkPluginParam * Clone( AK::IAkPluginMemAlloc * in_pAllocator );

    // Init/Term.
    AKRESULT Init( AK::IAkPluginMemAlloc *	in_pAllocator,						    
                   const void *				in_pParamsBlock, 
                   AkUInt32					in_ulBlockSize 
                   );
    AKRESULT Term( AK::IAkPluginMemAlloc * in_pAllocator );

    // Blob set.
    AKRESULT SetParamsBlock( const void * in_pParamsBlock, 
                             AkUInt32 in_ulBlockSize
                             );

    // Update one parameter.
    AKRESULT SetParam(	AkPluginParamID in_ParamID,
                        const void * in_pValue, 
                        AkUInt32 in_ulParamSize
                        );
};

#endif // __SPU__

#endif // _AK_ROOMVERBFXPARAMS_H_
