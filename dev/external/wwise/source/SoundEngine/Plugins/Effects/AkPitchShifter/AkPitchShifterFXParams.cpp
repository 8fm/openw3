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

#include "AkPitchShifterFXParams.h"
#include <AK/Tools/Common/AkAssert.h>
#include <AK/Tools/Common/AkBankReadHelpers.h>


// Creation function
AK::IAkPluginParam * CreatePitchShifterFXParams( AK::IAkPluginMemAlloc * in_pAllocator )
{
	return AK_PLUGIN_NEW( in_pAllocator, CAkPitchShifterFXParams( ) );
}

// Constructor/destructor.
CAkPitchShifterFXParams::CAkPitchShifterFXParams( )
{
}

CAkPitchShifterFXParams::~CAkPitchShifterFXParams( )
{
}

// Copy constructor.
CAkPitchShifterFXParams::CAkPitchShifterFXParams( const CAkPitchShifterFXParams & in_rCopy )
{
	m_Params = in_rCopy.m_Params;
	m_ParamChangeHandler.SetAllParamChanges();
}

// Create duplicate.
AK::IAkPluginParam * CAkPitchShifterFXParams::Clone( AK::IAkPluginMemAlloc * in_pAllocator )
{
	return AK_PLUGIN_NEW( in_pAllocator, CAkPitchShifterFXParams( *this ) );
}

// Init/Term.
AKRESULT CAkPitchShifterFXParams::Init(	AK::IAkPluginMemAlloc *	in_pAllocator,									   
											const void *			in_pParamsBlock, 
											AkUInt32				in_ulBlockSize )
{
	if ( in_ulBlockSize != 0)
	{
		return SetParamsBlock( in_pParamsBlock, in_ulBlockSize );
	}
	// Wwise does serialized SetParam()
	return AK_Success;
}

AKRESULT CAkPitchShifterFXParams::Term( AK::IAkPluginMemAlloc * in_pAllocator )
{
	AK_PLUGIN_DELETE( in_pAllocator, this );
	return AK_Success;
}

// Blob set.
AKRESULT CAkPitchShifterFXParams::SetParamsBlock(	const void * in_pParamsBlock, 
														AkUInt32 in_ulBlockSize
												)
{  
	AKRESULT eResult = AK_Success;
	AkUInt8 * pParamsBlock = (AkUInt8 *)in_pParamsBlock;

	m_Params.eInputType = (AkInputType)READBANKDATA( AkUInt32, pParamsBlock, in_ulBlockSize );
	m_Params.fDryLevel = AK_DBTOLIN( READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize ) );	
	m_Params.fWetLevel = AK_DBTOLIN( READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize ) );	
	m_Params.fDelayTime = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );
	m_Params.bProcessLFE = READBANKDATA( bool, pParamsBlock, in_ulBlockSize );
	m_Params.bSyncDry = READBANKDATA( bool, pParamsBlock, in_ulBlockSize );
	m_Params.Voice.fPitchFactor = AK_PITCH2FACTOR( READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize ) );
	m_Params.Voice.Filter.eFilterType = (AkFilterType)READBANKDATA( AkUInt32, pParamsBlock, in_ulBlockSize );	
	m_Params.Voice.Filter.fFilterGain = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );
	m_Params.Voice.Filter.fFilterFrequency = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );
	m_Params.Voice.Filter.fFilterQFactor = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );

	CHECKBANKDATASIZE( in_ulBlockSize, eResult );

	m_ParamChangeHandler.SetAllParamChanges();

	return eResult;
}

// Update one parameter.
AKRESULT CAkPitchShifterFXParams::SetParam(	AkPluginParamID in_ParamID,
												const void * in_pValue, 
												AkUInt32 in_ulParamSize )
{
	if ( in_pValue == NULL )
	{
		return AK_InvalidParameter;
	}

	switch ( in_ParamID )
	{
	case AKPITCHSHIFTERPARAMID_INPUT:
		m_Params.eInputType = (AkInputType)(*(AkUInt32*)in_pValue);
		break;
	case AKPITCHSHIFTERPARAMID_PROCESSLFE:
		m_Params.bProcessLFE = *(bool*)(in_pValue);
		break;
	case AKPITCHSHIFTERPARAMID_SYNCDRY:
		m_Params.bSyncDry = *(bool*)(in_pValue);
		break;
	case AKPITCHSHIFTERPARAMID_DRYLEVEL:
		m_Params.fDryLevel = AK_DBTOLIN(*(AkReal32*)in_pValue);
		break;
	case AKPITCHSHIFTERPARAMID_WETLEVEL:
		m_Params.fWetLevel = AK_DBTOLIN(*(AkReal32*)in_pValue);
		break;
	case AKPITCHSHIFTERPARAMID_DELAYTIME:
		m_Params.fDelayTime = *(AkReal32*)in_pValue;
		break;
	case AKPITCHSHIFTERPARAMID_PITCH:
		m_Params.Voice.fPitchFactor = AK_PITCH2FACTOR(*((AkReal32*)in_pValue));
		break;
	case AKPITCHSHIFTERPARAMID_FILTERTYPE:
		m_Params.Voice.Filter.eFilterType = (AkFilterType)( (AkUInt32)*((AkReal32*)in_pValue) );
		break;
	case AKPITCHSHIFTERPARAMID_FILTERGAIN:
		m_Params.Voice.Filter.fFilterGain = *((AkReal32*)in_pValue);
		break;
	case AKPITCHSHIFTERPARAMID_FILTERFREQUENCY:
		m_Params.Voice.Filter.fFilterFrequency = *((AkReal32*)in_pValue);
		break;
	case AKPITCHSHIFTERPARAMID_FILTERQFACTOR:
		m_Params.Voice.Filter.fFilterQFactor = *((AkReal32*)in_pValue);
		break;
	}
	m_ParamChangeHandler.SetParamChange( in_ParamID );

	return AK_Success;
}

// Retrieve all parameters at once
void CAkPitchShifterFXParams::GetParams( AkPitchShifterFXParams* out_pParams )
{
	*out_pParams = m_Params;
}
