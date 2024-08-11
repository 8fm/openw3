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

#include "AkRoomVerbFXParams.h"
#include <AK/Tools/Common/AkBankReadHelpers.h>

static const AkReal32 REVERBLEVEL_OFFSET = -3.f;

// Creation function
AK::IAkPluginParam * CreateRoomVerbFXParams( AK::IAkPluginMemAlloc * in_pAllocator )
{
    return AK_PLUGIN_NEW( in_pAllocator, CAkRoomVerbFXParams( ) );
}

// Constructor/destructor.
CAkRoomVerbFXParams::CAkRoomVerbFXParams( )
{
}

CAkRoomVerbFXParams::~CAkRoomVerbFXParams( )
{
}

// Copy constructor.
CAkRoomVerbFXParams::CAkRoomVerbFXParams( const CAkRoomVerbFXParams & in_rCopy )
{
	sRTPCParams = in_rCopy.sRTPCParams;
	sRTPCParams.bDirty = true;
	sInvariantParams = in_rCopy.sInvariantParams;
	sInvariantParams.bDirty = true;
	sAlgoTunings = in_rCopy.sAlgoTunings;
}

// Create duplicate.
AK::IAkPluginParam * CAkRoomVerbFXParams::Clone( AK::IAkPluginMemAlloc * in_pAllocator )
{
    return AK_PLUGIN_NEW( in_pAllocator, CAkRoomVerbFXParams( *this ) );
}

// Init/Term.
AKRESULT CAkRoomVerbFXParams::Init(	AK::IAkPluginMemAlloc *	in_pAllocator,									   
										const void *			in_pParamsBlock, 
										AkUInt32				in_ulBlockSize 
                                     )
{
	AKASSERT( in_pAllocator != NULL );
    if ( in_ulBlockSize == 0)
    {
        // Init default parameters.
		sRTPCParams.fDecayTime = AK_ROOMVERBFXPARAM_DECAYTIME_DEF;
		sRTPCParams.fHFDamping = AK_ROOMVERBFXPARAM_HFDAMPING_DEF;
		sRTPCParams.fDiffusion = AK_ROOMVERBFXPARAM_DIFFUSION_DEF;
		sRTPCParams.fStereoWidth = AK_ROOMVERBFXPARAM_STEREOWIDTH_DEF;	
			
		sRTPCParams.fFilter1Gain = AK_ROOMVERBFXPARAM_FILTER1GAIN_DEF;	
		sRTPCParams.fFilter1Freq = AK_ROOMVERBFXPARAM_FILTER1FREQ_DEF;
		sRTPCParams.fFilter1Q = AK_ROOMVERBFXPARAM_FILTER1Q_DEF;
		sRTPCParams.fFilter2Gain = AK_ROOMVERBFXPARAM_FILTER2GAIN_DEF;
		sRTPCParams.fFilter2Freq = AK_ROOMVERBFXPARAM_FILTER2FREQ_DEF;
		sRTPCParams.fFilter2Q = AK_ROOMVERBFXPARAM_FILTER2Q_DEF;
		sRTPCParams.fFilter3Gain = AK_ROOMVERBFXPARAM_FILTER3GAIN_DEF;
		sRTPCParams.fFilter3Freq = AK_ROOMVERBFXPARAM_FILTER3FREQ_DEF;	
		sRTPCParams.fFilter3Q = AK_ROOMVERBFXPARAM_FILTER3Q_DEF;
			
		sRTPCParams.fFrontLevel = AK_DBTOLIN( AK_ROOMVERBFXPARAM_FRONTLEVEL_DEF );
		sRTPCParams.fRearLevel = AK_DBTOLIN( AK_ROOMVERBFXPARAM_REARLEVEL_DEF );
		sRTPCParams.fCenterLevel = AK_DBTOLIN( AK_ROOMVERBFXPARAM_CENTERLEVEL_DEF );	
		sRTPCParams.fLFELevel = AK_DBTOLIN( AK_ROOMVERBFXPARAM_LFELEVEL_DEF );

		sRTPCParams.fDryLevel = AK_DBTOLIN( AK_ROOMVERBFXPARAM_DRYLEVEL_DEF );	
		sRTPCParams.fERLevel = AK_DBTOLIN( AK_ROOMVERBFXPARAM_ERLEVEL_DEF );
		sRTPCParams.fReverbLevel = AK_DBTOLIN( AK_ROOMVERBFXPARAM_REVERBLEVEL_DEF + REVERBLEVEL_OFFSET );

	
		sInvariantParams.uERPattern = AK_ROOMVERBFXPARAM_ERPATTERN_DEF;
		sInvariantParams.fReverbDelay = AK_ROOMVERBFXPARAM_REVERBDELAY_DEF;
		sInvariantParams.fRoomSize = AK_ROOMVERBFXPARAM_ROOMSIZE_DEF;
		sInvariantParams.fERFrontBackDelay = AK_ROOMVERBFXPARAM_ERFRONTBACKDELAY_DEF;
		sInvariantParams.bEnableEarlyReflections = AK_ROOMVERBFXPARAM_ENABLEEARLYREFLECTIONS_DEF;

		sInvariantParams.fDensity = AK_ROOMVERBFXPARAM_DENSITY_DEF;
		sInvariantParams.fRoomShape = AK_ROOMVERBFXPARAM_ROOMSHAPE_DEF;
		sInvariantParams.uNumReverbUnits = AK_ROOMVERBFXPARAM_NUMREVERBUNITS_DEF;

		sInvariantParams.fInputLFELevel = AK_DBTOLIN( AK_ROOMVERBFXPARAM_INPUTLFELEVEL_DEF );	
		sInvariantParams.fInputCenterLevel = AK_DBTOLIN( AK_ROOMVERBFXPARAM_INPUTCENTERLEVEL_DEF );	

		sInvariantParams.eFilter1Pos = AK_ROOMVERBFXPARAM_FILTER1INSERT_DEF;
		sInvariantParams.eFilter1Curve = AK_ROOMVERBFXPARAM_FILTER1CURVE_DEF;
		sInvariantParams.eFilter2Pos = AK_ROOMVERBFXPARAM_FILTER2INSERT_DEF;
		sInvariantParams.eFilter2Curve = AK_ROOMVERBFXPARAM_FILTER2CURVE_DEF;
		sInvariantParams.eFilter3Pos = AK_ROOMVERBFXPARAM_FILTER3INSERT_DEF;
		sInvariantParams.eFilter3Curve = AK_ROOMVERBFXPARAM_FILTER3CURVE_DEF;
		sInvariantParams.bEnableToneControls = AK_ROOMVERBFXPARAM_ENABLETONECONTROLS_DEF;
		
		// Tunings
		sAlgoTunings = g_AlgoTunings;

		sRTPCParams.bDirty = true;
		sInvariantParams.bDirty = true;

        return AK_Success;
    }
    return SetParamsBlock( in_pParamsBlock, in_ulBlockSize );
}

AKRESULT CAkRoomVerbFXParams::Term( AK::IAkPluginMemAlloc * in_pAllocator )
{
    AK_PLUGIN_DELETE( in_pAllocator, this );
    return AK_Success;
}

// Blob set.
AKRESULT CAkRoomVerbFXParams::SetParamsBlock(	const void * in_pParamsBlock, 
                                                AkUInt32 in_ulBlockSize
                                               )
{
	AKRESULT eResult = AK_Success;
	AkUInt8 * pParamsBlock = (AkUInt8 *)in_pParamsBlock;
	// RTPC parameters
	sRTPCParams.fDecayTime = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );	
	sRTPCParams.fHFDamping = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );	
	sRTPCParams.fDiffusion = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );	
	sRTPCParams.fStereoWidth = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );	

	sRTPCParams.fFilter1Gain = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );	
	sRTPCParams.fFilter1Freq = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );	
	sRTPCParams.fFilter1Q = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );	
	sRTPCParams.fFilter2Gain = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );	
	sRTPCParams.fFilter2Freq = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );	
	sRTPCParams.fFilter2Q = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );	
	sRTPCParams.fFilter3Gain = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );	
	sRTPCParams.fFilter3Freq = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );	
	sRTPCParams.fFilter3Q = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );	
	
	sRTPCParams.fFrontLevel = AK_DBTOLIN( READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize ) );	
	sRTPCParams.fRearLevel = AK_DBTOLIN( READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize ) );	
	sRTPCParams.fCenterLevel = AK_DBTOLIN( READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize ) );	
	sRTPCParams.fLFELevel = AK_DBTOLIN( READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize ) );	

	sRTPCParams.fDryLevel = AK_DBTOLIN( READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize ) );	
	sRTPCParams.fERLevel = AK_DBTOLIN( READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize ) );	
	sRTPCParams.fReverbLevel = AK_DBTOLIN( READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize ) + REVERBLEVEL_OFFSET );		

	// Non-RTPC parameters
	sInvariantParams.bEnableEarlyReflections = READBANKDATA( bool, pParamsBlock, in_ulBlockSize );
	sInvariantParams.uERPattern = READBANKDATA( AkUInt32, pParamsBlock, in_ulBlockSize );
	sInvariantParams.fReverbDelay = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );
	sInvariantParams.fRoomSize = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );
	sInvariantParams.fERFrontBackDelay = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );
	
	sInvariantParams.fDensity = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );
	sInvariantParams.fRoomShape = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );
	sInvariantParams.uNumReverbUnits = READBANKDATA( AkUInt32, pParamsBlock, in_ulBlockSize );

	sInvariantParams.bEnableToneControls = READBANKDATA( bool, pParamsBlock, in_ulBlockSize );
	sInvariantParams.eFilter1Pos = READBANKDATA( FilterInsertType, pParamsBlock, in_ulBlockSize );
	sInvariantParams.eFilter1Curve = READBANKDATA( FilterCurveType, pParamsBlock, in_ulBlockSize );
	sInvariantParams.eFilter2Pos = READBANKDATA( FilterInsertType, pParamsBlock, in_ulBlockSize );
	sInvariantParams.eFilter2Curve = READBANKDATA( FilterCurveType, pParamsBlock, in_ulBlockSize );
	sInvariantParams.eFilter3Pos = READBANKDATA( FilterInsertType, pParamsBlock, in_ulBlockSize );
	sInvariantParams.eFilter3Curve = READBANKDATA( FilterCurveType, pParamsBlock, in_ulBlockSize );

	sInvariantParams.fInputCenterLevel = AK_DBTOLIN( READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize ) );
	sInvariantParams.fInputLFELevel = AK_DBTOLIN( READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize ) );

	// Tunings
	sAlgoTunings.fDensityDelayMin = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );
	sAlgoTunings.fDensityDelayMax = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );
	sAlgoTunings.fDensityDelayRdmPerc = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );
	sAlgoTunings.fRoomShapeMin = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );
	sAlgoTunings.fRoomShapeMax = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );
	sAlgoTunings.fDiffusionDelayScalePerc = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );
	sAlgoTunings.fDiffusionDelayMax = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );
	sAlgoTunings.fDiffusionDelayRdmPerc = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );
	sAlgoTunings.fDCFilterCutFreq = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );
	sAlgoTunings.fReverbUnitInputDelay = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );
	sAlgoTunings.fReverbUnitInputDelayRmdPerc = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );

	sRTPCParams.bDirty = true;
	sInvariantParams.bDirty = true;

    CHECKBANKDATASIZE( in_ulBlockSize, eResult );
	return eResult;
}

// Update one parameter.
AKRESULT CAkRoomVerbFXParams::SetParam(	AkPluginParamID in_ParamID,
										const void * in_pValue, 
										AkUInt32 in_ulParamSize
                                         )
{
	AKASSERT( in_pValue != NULL );
	if ( in_pValue == NULL )
	{
		return AK_InvalidParameter;
	}

	switch ( in_ParamID )
	{
	// RTPC params
	case AK_ROOMVERBFXPARAM_DECAYTIME_ID:
		sRTPCParams.fDecayTime = *((AkReal32*)in_pValue);
		sRTPCParams.bDirty = true;
		break;
	case AK_ROOMVERBFXPARAM_HFDAMPING_ID:
		sRTPCParams.fHFDamping = *((AkReal32*)in_pValue);
		sRTPCParams.bDirty = true;
		break;
	case AK_ROOMVERBFXPARAM_DIFFUSION_ID:
		sRTPCParams.fDiffusion = *((AkReal32*)in_pValue);
		sRTPCParams.bDirty = true;
		break;
	case AK_ROOMVERBFXPARAM_STEREOWIDTH_ID:	
		sRTPCParams.fStereoWidth = *((AkReal32*)in_pValue);
		sRTPCParams.bDirty = true;
		break;

	case AK_ROOMVERBFXPARAM_FILTER1GAIN_ID:
		sRTPCParams.fFilter1Gain = *((AkReal32*)in_pValue);
		sRTPCParams.bDirty = true;
		break;
	case AK_ROOMVERBFXPARAM_FILTER1FREQ_ID:
		sRTPCParams.fFilter1Freq = *((AkReal32*)in_pValue);
		sRTPCParams.bDirty = true;
		break;
	case AK_ROOMVERBFXPARAM_FILTER1Q_ID:
		sRTPCParams.fFilter1Q = *((AkReal32*)in_pValue);
		sRTPCParams.bDirty = true;
		break;
	case AK_ROOMVERBFXPARAM_FILTER2GAIN_ID:
		sRTPCParams.fFilter2Gain = *((AkReal32*)in_pValue);
		sRTPCParams.bDirty = true;
		break;
	case AK_ROOMVERBFXPARAM_FILTER2FREQ_ID:
		sRTPCParams.fFilter2Freq = *((AkReal32*)in_pValue);
		sRTPCParams.bDirty = true;
		break;
	case AK_ROOMVERBFXPARAM_FILTER2Q_ID:
		sRTPCParams.fFilter2Q = *((AkReal32*)in_pValue);
		sRTPCParams.bDirty = true;
		break;
	case AK_ROOMVERBFXPARAM_FILTER3GAIN_ID:	
		sRTPCParams.fFilter3Gain = *((AkReal32*)in_pValue);
		sRTPCParams.bDirty = true;
		break;
	case AK_ROOMVERBFXPARAM_FILTER3FREQ_ID:
		sRTPCParams.fFilter3Freq = *((AkReal32*)in_pValue);
		sRTPCParams.bDirty = true;
		break;
	case AK_ROOMVERBFXPARAM_FILTER3Q_ID:
		sRTPCParams.fFilter3Q = *((AkReal32*)in_pValue);
		sRTPCParams.bDirty = true;
		break;
	
	case AK_ROOMVERBFXPARAM_FRONTLEVEL_ID:
		sRTPCParams.fFrontLevel = AK_DBTOLIN( *((AkReal32*)in_pValue) );
		sRTPCParams.bDirty = true;
		break;
	case AK_ROOMVERBFXPARAM_REARLEVEL_ID:
		sRTPCParams.fRearLevel = AK_DBTOLIN( *((AkReal32*)in_pValue) );
		sRTPCParams.bDirty = true;
		break;
	case AK_ROOMVERBFXPARAM_CENTERLEVEL_ID:	
		sRTPCParams.fCenterLevel = AK_DBTOLIN( *((AkReal32*)in_pValue) );
		sRTPCParams.bDirty = true;
		break;
	case AK_ROOMVERBFXPARAM_LFELEVEL_ID:	
		sRTPCParams.fLFELevel = AK_DBTOLIN( *((AkReal32*)in_pValue) );
		sRTPCParams.bDirty = true;
		break;
	
	case AK_ROOMVERBFXPARAM_DRYLEVEL_ID:
		sRTPCParams.fDryLevel = AK_DBTOLIN( *((AkReal32*)in_pValue) );
		sRTPCParams.bDirty = true;
		break;
	case AK_ROOMVERBFXPARAM_ERLEVEL_ID:
		sRTPCParams.fERLevel = AK_DBTOLIN( *((AkReal32*)in_pValue) );
		sRTPCParams.bDirty = true;
		break;
	case AK_ROOMVERBFXPARAM_REVERBLEVEL_ID:	
		sRTPCParams.fReverbLevel = AK_DBTOLIN( *((AkReal32*)in_pValue) + REVERBLEVEL_OFFSET );
		sRTPCParams.bDirty = true;
		break;

	// Non-RTPC params
	case AK_ROOMVERBFXPARAM_ENABLEEARLYREFLECTIONS_ID:
		sInvariantParams.bEnableEarlyReflections = *((bool*)in_pValue) ? 1 : 0;
		sInvariantParams.bDirty = true;
		break;
	case AK_ROOMVERBFXPARAM_ERPATTERN_ID:
		sInvariantParams.uERPattern = *((AkUInt32*)in_pValue);
		sInvariantParams.bDirty = true;
		break;
	case AK_ROOMVERBFXPARAM_REVERBDELAY_ID:
		sInvariantParams.fReverbDelay = *((AkReal32*)in_pValue);
		sInvariantParams.bDirty = true;
		break;
	case AK_ROOMVERBFXPARAM_ROOMSIZE_ID:
		sInvariantParams.fRoomSize = *((AkReal32*)in_pValue);
		sInvariantParams.bDirty = true;
		break;
	case AK_ROOMVERBFXPARAM_ERFRONTBACKDELAY_ID:
		sInvariantParams.fERFrontBackDelay = *((AkReal32*)in_pValue);
		sInvariantParams.bDirty = true;
		break;

	case AK_ROOMVERBFXPARAM_DENSITY_ID:
		sInvariantParams.fDensity = *((AkReal32*)in_pValue);
		sInvariantParams.bDirty = true;
		break;
	case AK_ROOMVERBFXPARAM_ROOMSHAPE_ID:
		sInvariantParams.fRoomShape = *((AkReal32*)in_pValue);
		sInvariantParams.bDirty = true;
		break;
	case AK_ROOMVERBFXPARAM_NUMREVERBUNITS_ID:
		sInvariantParams.uNumReverbUnits = *((AkUInt32*)in_pValue);
		sInvariantParams.bDirty = true;
		break;

	case AK_ROOMVERBFXPARAM_INPUTCENTERLEVEL_ID:	
		sInvariantParams.fInputCenterLevel = AK_DBTOLIN( *((AkReal32*)in_pValue) );
		sInvariantParams.bDirty = true;
		break;
	case AK_ROOMVERBFXPARAM_INPUTLFELEVEL_ID:	
		sInvariantParams.fInputLFELevel = AK_DBTOLIN( *((AkReal32*)in_pValue) );
		sInvariantParams.bDirty = true;
		break;

	case AK_ROOMVERBFXPARAM_FILTER1INSERT_ID:
		sInvariantParams.eFilter1Pos = *((FilterInsertType*)in_pValue);
		sInvariantParams.bDirty = true;
		break;
	case AK_ROOMVERBFXPARAM_FILTER1CURVE_ID:
		sInvariantParams.eFilter1Curve = *((FilterCurveType*)in_pValue);
		sInvariantParams.bDirty = true;
		break;
	case AK_ROOMVERBFXPARAM_FILTER2INSERT_ID:
		sInvariantParams.eFilter2Pos = *((FilterInsertType*)in_pValue);
		sInvariantParams.bDirty = true;
		break;
	case AK_ROOMVERBFXPARAM_FILTER2CURVE_ID:
		sInvariantParams.eFilter2Curve = *((FilterCurveType*)in_pValue);
		sInvariantParams.bDirty = true;
		break;
	case AK_ROOMVERBFXPARAM_FILTER3INSERT_ID:
		sInvariantParams.eFilter3Pos = *((FilterInsertType*)in_pValue);
		sInvariantParams.bDirty = true;
		break;
	case AK_ROOMVERBFXPARAM_FILTER3CURVE_ID:
		sInvariantParams.eFilter3Curve = *((FilterCurveType*)in_pValue);
		sInvariantParams.bDirty = true;
		break;

	case AK_ROOMVERBFXPARAM_ENABLETONECONTROLS_ID:
		sInvariantParams.bEnableToneControls = *((bool*)in_pValue) ? 1 : 0;
		sInvariantParams.bDirty = true;
		break;

	// Tunings
	case AK_ROOMVERBFXTUNING_DENSITYDELAYMIN_ID:
		sAlgoTunings.fDensityDelayMin = *((AkReal32*)in_pValue);
		break;
	case AK_ROOMVERBFXTUNING_DENSITYDELAYMAX_ID:
		sAlgoTunings.fDensityDelayMax = *((AkReal32*)in_pValue);
		break;
	case AK_ROOMVERBFXTUNING_DENSITYDELAYRDMPERC_ID:
		sAlgoTunings.fDensityDelayRdmPerc = *((AkReal32*)in_pValue);
		break;
	case AK_ROOMVERBFXTUNING_ROOMSHAPEMIN_ID:
		sAlgoTunings.fRoomShapeMin = *((AkReal32*)in_pValue);
		break;
	case AK_ROOMVERBFXTUNING_ROOMSHAPEMAX_ID:	
		sAlgoTunings.fRoomShapeMax = *((AkReal32*)in_pValue);
		break;
	case AK_ROOMVERBFXTUNING_DIFFUSIONDELAYSCALEPERC_ID:
		sAlgoTunings.fDiffusionDelayScalePerc = *((AkReal32*)in_pValue);
		break;
	case AK_ROOMVERBFXTUNING_DIFFUSIONDELAYMAX_ID:
		sAlgoTunings.fDiffusionDelayMax = *((AkReal32*)in_pValue);
		break;
	case AK_ROOMVERBFXTUNING_DIFFUSIONDELAYRDMPERC_ID:
		sAlgoTunings.fDiffusionDelayRdmPerc = *((AkReal32*)in_pValue);
		break;
	case AK_ROOMVERBFXTUNING_DCFILTERCUTFREQ_ID:
		sAlgoTunings.fDCFilterCutFreq = *((AkReal32*)in_pValue);
		break;
	case AK_ROOMVERBFXTUNING_REVERBUNITINPUTDELAY_ID:
		sAlgoTunings.fReverbUnitInputDelay = *((AkReal32*)in_pValue);
		break;
	case AK_ROOMVERBFXTUNING_REVERBUNITINPUTDELAYRMDPERC_ID:
		sAlgoTunings.fReverbUnitInputDelayRmdPerc = *((AkReal32*)in_pValue);
		break;
	}

	return AK_Success;
}
