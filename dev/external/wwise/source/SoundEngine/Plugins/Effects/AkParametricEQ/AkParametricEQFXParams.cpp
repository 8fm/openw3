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

#include "AkParametricEQFXParams.h"
#include <AK/Tools/Common/AkBankReadHelpers.h>

// Creation function
AK::IAkPluginParam * CreateParametricEQFXParams( AK::IAkPluginMemAlloc * in_pAllocator )
{
	AKASSERT( in_pAllocator != NULL );
	return AK_PLUGIN_NEW( in_pAllocator, CAkParameterEQFXParams( ) );
}

// Constructor/destructor.
CAkParameterEQFXParams::CAkParameterEQFXParams( )
{
	m_bBandDirty[BAND1] = true;
	m_bBandDirty[BAND2] = true;
	m_bBandDirty[BAND3] = true;
}

CAkParameterEQFXParams::~CAkParameterEQFXParams( )
{
}

// Copy constructor.
CAkParameterEQFXParams::CAkParameterEQFXParams( const CAkParameterEQFXParams & in_rCopy )
{
	m_Params = in_rCopy.m_Params;
	m_bBandDirty[BAND1] = true;
	m_bBandDirty[BAND2] = true;
	m_bBandDirty[BAND3] = true;
}

// Create duplicate.
AK::IAkPluginParam * CAkParameterEQFXParams::Clone( AK::IAkPluginMemAlloc * in_pAllocator )
{
	AKASSERT( in_pAllocator != NULL );
	return AK_PLUGIN_NEW( in_pAllocator, CAkParameterEQFXParams( *this ) );
}

// Init/Term.
AKRESULT CAkParameterEQFXParams::Init(	AK::IAkPluginMemAlloc *	in_pAllocator,									   
										const void *			in_pParamsBlock, 
										AkUInt32				in_ulBlockSize )
{
	if ( in_ulBlockSize == 0)
	{
		// Init default parameters.
		m_Params.Band[BAND1].eFilterType = PARAMETRICEQ_FILTERTYPE_BAND1_DEF;
		m_Params.Band[BAND1].fGain = PARAMETRICEQ_GAIN_BAND1_DEF;
		m_Params.Band[BAND1].fFrequency = PARAMETRICEQ_FREQUENCY_BAND1_DEF;
		m_Params.Band[BAND1].fQFactor = PARAMETRICEQ_QFACTOR_BAND1_DEF;
		m_Params.Band[BAND1].bOnOff = PARAMETRICEQ_ONOFF_BAND1_DEF;

		m_Params.Band[BAND2].eFilterType = PARAMETRICEQ_FILTERTYPE_BAND2_DEF;
		m_Params.Band[BAND2].fGain = PARAMETRICEQ_GAIN_BAND2_DEF;
		m_Params.Band[BAND2].fFrequency = PARAMETRICEQ_FREQUENCY_BAND2_DEF;
		m_Params.Band[BAND2].fQFactor = PARAMETRICEQ_QFACTOR_BAND2_DEF;
		m_Params.Band[BAND2].bOnOff = PARAMETRICEQ_ONOFF_BAND2_DEF;

		m_Params.Band[BAND3].eFilterType = PARAMETRICEQ_FILTERTYPE_BAND3_DEF;
		m_Params.Band[BAND3].fGain = PARAMETRICEQ_GAIN_BAND3_DEF;
		m_Params.Band[BAND3].fFrequency = PARAMETRICEQ_FREQUENCY_BAND3_DEF;
		m_Params.Band[BAND3].fQFactor = PARAMETRICEQ_QFACTOR_BAND3_DEF;
		m_Params.Band[BAND3].bOnOff = PARAMETRICEQ_ONOFF_BAND3_DEF;

		m_Params.fOutputLevel = PARAMETRICEQ_OUTOUTLEVEL_DEF;
		m_Params.bProcessLFE = PARAMETRICEQ_PROCESSLFE_DEF;

		m_bBandDirty[BAND1] = true;
		m_bBandDirty[BAND2] = true;
		m_bBandDirty[BAND3] = true;

		return AK_Success;
	}
	return SetParamsBlock( in_pParamsBlock, in_ulBlockSize );
}

AKRESULT CAkParameterEQFXParams::Term( AK::IAkPluginMemAlloc * in_pAllocator )
{
	AKASSERT( in_pAllocator != NULL );
	AK_PLUGIN_DELETE( in_pAllocator, this );
	return AK_Success;
}

// Blob set.
AKRESULT CAkParameterEQFXParams::SetParamsBlock(	const void * in_pParamsBlock, 
													AkUInt32 in_ulBlockSize
												)
{  
	AKRESULT eResult = AK_Success;
	AkUInt8 * pParamsBlock = (AkUInt8 *)in_pParamsBlock;

	m_Params.Band[BAND1].eFilterType = READBANKDATA( AkFilterType, pParamsBlock, in_ulBlockSize );	
	m_Params.Band[BAND1].fGain = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );	
	m_Params.Band[BAND1].fFrequency = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );	
	m_Params.Band[BAND1].fQFactor = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );	
	m_Params.Band[BAND1].bOnOff = READBANKDATA( bool, pParamsBlock, in_ulBlockSize );	

	m_Params.Band[BAND2].eFilterType = READBANKDATA( AkFilterType, pParamsBlock, in_ulBlockSize );	
	m_Params.Band[BAND2].fGain = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );	
	m_Params.Band[BAND2].fFrequency = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );	
	m_Params.Band[BAND2].fQFactor = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );	
	m_Params.Band[BAND2].bOnOff = READBANKDATA( bool, pParamsBlock, in_ulBlockSize );	

	m_Params.Band[BAND3].eFilterType = READBANKDATA( AkFilterType, pParamsBlock, in_ulBlockSize );	
	m_Params.Band[BAND3].fGain = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );	
	m_Params.Band[BAND3].fFrequency = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );	
	m_Params.Band[BAND3].fQFactor = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );	
	m_Params.Band[BAND3].bOnOff = READBANKDATA( bool, pParamsBlock, in_ulBlockSize );	

	m_Params.fOutputLevel = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );	
	m_Params.bProcessLFE = READBANKDATA( bool, pParamsBlock, in_ulBlockSize );	

	m_bBandDirty[BAND1] = true;
	m_bBandDirty[BAND2] = true;
	m_bBandDirty[BAND3] = true;

	CHECKBANKDATASIZE( in_ulBlockSize, eResult );
	return eResult;
}

// Update one parameter.
AKRESULT CAkParameterEQFXParams::SetParam(	AkPluginParamID in_ParamID,
											const void * in_pValue, 
											AkUInt32 in_ulParamSize )
{
	AKASSERT( in_pValue != NULL );
	if ( in_pValue == NULL )
	{
		return AK_InvalidParameter;
	}
	AKRESULT eResult = AK_Success;

	// Determine which band its applied on
	AkUInt32 ulBandNum = in_ParamID / AK_PARAMETRICEQ_NUMPERMODULE;

	switch ( in_ParamID )
	{
	case AK_PARAMETRICEQFXPARAM_FILTERTYPEBAND1_ID:
	case AK_PARAMETRICEQFXPARAM_FILTERTYPEBAND2_ID:
	case AK_PARAMETRICEQFXPARAM_FILTERTYPEBAND3_ID:
		m_Params.Band[ulBandNum].eFilterType = static_cast<AkFilterType>(static_cast<int>(*reinterpret_cast<const AkReal32*>( in_pValue )));
		AKASSERT( ulBandNum <= BAND3 );
		m_bBandDirty[ulBandNum] = true;
		break;
	case AK_PARAMETRICEQFXPARAM_GAINBAND1_ID:
	case AK_PARAMETRICEQFXPARAM_GAINBAND2_ID:
	case AK_PARAMETRICEQFXPARAM_GAINBAND3_ID:
		m_Params.Band[ulBandNum].fGain = *reinterpret_cast<const AkReal32*>(in_pValue);
		AKASSERT( ulBandNum <= BAND3 );
		m_bBandDirty[ulBandNum] = true;
		break;
	case AK_PARAMETRICEQFXPARAM_FREQUENCYBAND1_ID:
	case AK_PARAMETRICEQFXPARAM_FREQUENCYBAND2_ID:
	case AK_PARAMETRICEQFXPARAM_FREQUENCYBAND3_ID:
		m_Params.Band[ulBandNum].fFrequency = *reinterpret_cast<const AkReal32*>(in_pValue);
		AKASSERT( ulBandNum <= BAND3 );
		m_bBandDirty[ulBandNum] = true;
		break;
	case AK_PARAMETRICEQFXPARAM_QFACTORBAND1_ID:
	case AK_PARAMETRICEQFXPARAM_QFACTORBAND2_ID:
	case AK_PARAMETRICEQFXPARAM_QFACTORBAND3_ID:
		m_Params.Band[ulBandNum].fQFactor = *reinterpret_cast<const AkReal32*>(in_pValue);
		AKASSERT( ulBandNum <= BAND3 );
		m_bBandDirty[ulBandNum] = true;
		break;
	case AK_PARAMETRICEQFXPARAM_ONOFFBAND1_ID:
	case AK_PARAMETRICEQFXPARAM_ONOFFBAND2_ID:
	case AK_PARAMETRICEQFXPARAM_ONOFFBAND3_ID:
		m_Params.Band[ulBandNum].bOnOff  = ( *reinterpret_cast<const AkReal32*>(in_pValue) ) != 0;
		AKASSERT( ulBandNum <= BAND3 );
		m_bBandDirty[ulBandNum] = true;
		break;
	case AK_PARAMETRICEQFXPARAM_OUTPUTLEVEL_ID:
		m_Params.fOutputLevel = *reinterpret_cast<const AkReal32*>(in_pValue);
		break;
	case AK_PARAMETRICEQFXPARAM_PROCESSLFE_ID:
		m_Params.bProcessLFE = *reinterpret_cast<const bool*>(in_pValue);
		break;
	default:
		AKASSERT(!"Invalid parameter.");
		eResult = AK_InvalidParameter;
	}

	return eResult;

}