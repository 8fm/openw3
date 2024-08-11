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

#include "AkHarmonizerFXParams.h"
#include <AK/Tools/Common/AkAssert.h>
#include <AK/Tools/Common/AkBankReadHelpers.h>


// Creation function
AK::IAkPluginParam * CreateHarmonizerFXParams( AK::IAkPluginMemAlloc * in_pAllocator )
{
	return AK_PLUGIN_NEW( in_pAllocator, CAkHarmonizerFXParams( ) );
}

// Constructor/destructor.
CAkHarmonizerFXParams::CAkHarmonizerFXParams( )
{
}

CAkHarmonizerFXParams::~CAkHarmonizerFXParams( )
{
}

// Copy constructor.
CAkHarmonizerFXParams::CAkHarmonizerFXParams( const CAkHarmonizerFXParams & in_rCopy )
{
	m_Params = in_rCopy.m_Params;
	m_ParamChangeHandler.SetAllParamChanges();
}

// Create duplicate.
AK::IAkPluginParam * CAkHarmonizerFXParams::Clone( AK::IAkPluginMemAlloc * in_pAllocator )
{
	return AK_PLUGIN_NEW( in_pAllocator, CAkHarmonizerFXParams( *this ) );
}

// Init/Term.
AKRESULT CAkHarmonizerFXParams::Init(	AK::IAkPluginMemAlloc *	in_pAllocator,									   
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

AKRESULT CAkHarmonizerFXParams::Term( AK::IAkPluginMemAlloc * in_pAllocator )
{
	AK_PLUGIN_DELETE( in_pAllocator, this );
	return AK_Success;
}

// Blob set.
AKRESULT CAkHarmonizerFXParams::SetParamsBlock(	const void * in_pParamsBlock, 
														AkUInt32 in_ulBlockSize
												)
{  
	AKRESULT eResult = AK_Success;
	AkUInt8 * pParamsBlock = (AkUInt8 *)in_pParamsBlock;

	for ( AkUInt32 i = 0; i < AKHARMONIZER_NUMVOICES; i++ )
	{
		m_Params.Voice[i].bEnable = READBANKDATA( bool, pParamsBlock, in_ulBlockSize );
		m_Params.Voice[i].fPitchFactor = AK_PITCH2FACTOR( READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize ) );
		m_Params.Voice[i].fGain = AK_DBTOLIN( READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize ) );
		m_Params.Voice[i].Filter.eFilterType = (AkFilterType)READBANKDATA( AkUInt32, pParamsBlock, in_ulBlockSize );	
		m_Params.Voice[i].Filter.fFilterGain = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );
		m_Params.Voice[i].Filter.fFilterFrequency = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );
		m_Params.Voice[i].Filter.fFilterQFactor = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );
	}
	m_Params.eInputType = (AkInputType)READBANKDATA( AkUInt32, pParamsBlock, in_ulBlockSize );
	m_Params.fDryLevel = AK_DBTOLIN( READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize ) );	
	m_Params.fWetLevel = AK_DBTOLIN( READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize ) );	
	m_Params.uWindowSize = READBANKDATA( AkUInt32, pParamsBlock, in_ulBlockSize );
	m_Params.bProcessLFE = READBANKDATA( bool, pParamsBlock, in_ulBlockSize );
	m_Params.bSyncDry = READBANKDATA( bool, pParamsBlock, in_ulBlockSize );
	
	CHECKBANKDATASIZE( in_ulBlockSize, eResult );

	m_ParamChangeHandler.SetAllParamChanges();

	return eResult;
}

// Update one parameter.
AKRESULT CAkHarmonizerFXParams::SetParam(	AkPluginParamID in_ParamID,
												const void * in_pValue, 
												AkUInt32 in_ulParamSize )
{
	if ( in_pValue == NULL )
	{
		return AK_InvalidParameter;
	}

		switch ( in_ParamID )
	{
	case AKHARMONIZERPARAMID_INPUT:
		m_Params.eInputType = (AkInputType)(*(AkUInt32*)in_pValue);
		break;
	case AKHARMONIZERPARAMID_PROCESSLFE:
		m_Params.bProcessLFE = *(bool*)(in_pValue);
		break;
	case AKHARMONIZERPARAMID_SYNCDRY:
		m_Params.bSyncDry = *(bool*)(in_pValue);
		break;
	case AKHARMONIZERPARAMID_DRYLEVEL:
		m_Params.fDryLevel = AK_DBTOLIN(*(AkReal32*)in_pValue);
		break;
	case AKHARMONIZERPARAMID_WETLEVEL:
		m_Params.fWetLevel = AK_DBTOLIN(*(AkReal32*)in_pValue);
		break;
	case AKHARMONIZERPARAMID_WINDOWSIZE:
		m_Params.uWindowSize = *(AkUInt32*)in_pValue;
		break;
	case AKHARMONIZERPARAMID_VOICE1ENABLE:
		m_Params.Voice[0].bEnable = *(bool*)(in_pValue);
		break;
	case AKHARMONIZERPARAMID_VOICE1PITCH:
		m_Params.Voice[0].fPitchFactor = AK_PITCH2FACTOR(*((AkReal32*)in_pValue));
		break;
	case AKHARMONIZERPARAMID_VOICE1GAIN:
		m_Params.Voice[0].fGain = AK_DBTOLIN(*(AkReal32*)in_pValue);
		break;
	case AKHARMONIZERPARAMID_VOICE1FILTERTYPE:
		m_Params.Voice[0].Filter.eFilterType = (AkFilterType)( (AkUInt32)*((AkReal32*)in_pValue) );
		break;
	case AKHARMONIZERPARAMID_VOICE1FILTERGAIN:
		m_Params.Voice[0].Filter.fFilterGain = *((AkReal32*)in_pValue);
		break;
	case AKHARMONIZERPARAMID_VOICE1FILTERFREQUENCY:
		m_Params.Voice[0].Filter.fFilterFrequency = *((AkReal32*)in_pValue);
		break;
	case AKHARMONIZERPARAMID_VOICE1FILTERQFACTOR:
		m_Params.Voice[0].Filter.fFilterQFactor = *((AkReal32*)in_pValue);
		break;
	case AKHARMONIZERPARAMID_VOICE2ENABLE:
		m_Params.Voice[1].bEnable = *(bool*)(in_pValue);
		break;
	case AKHARMONIZERPARAMID_VOICE2PITCH:
		m_Params.Voice[1].fPitchFactor = AK_PITCH2FACTOR(*((AkReal32*)in_pValue));
		break;
	case AKHARMONIZERPARAMID_VOICE2GAIN:
		m_Params.Voice[1].fGain = AK_DBTOLIN(*(AkReal32*)in_pValue);
		break;
	case AKHARMONIZERPARAMID_VOICE2FILTERTYPE:
		m_Params.Voice[1].Filter.eFilterType = (AkFilterType)( (AkUInt32)*((AkReal32*)in_pValue) );
		break;
	case AKHARMONIZERPARAMID_VOICE2FILTERGAIN:
		m_Params.Voice[1].Filter.fFilterGain = *((AkReal32*)in_pValue);
		break;
	case AKHARMONIZERPARAMID_VOICE2FILTERFREQUENCY:
		m_Params.Voice[1].Filter.fFilterFrequency = *((AkReal32*)in_pValue);
		break;
	case AKHARMONIZERPARAMID_VOICE2FILTERQFACTOR:
		m_Params.Voice[1].Filter.fFilterQFactor = *((AkReal32*)in_pValue);
		break;
	}
	m_ParamChangeHandler.SetParamChange( in_ParamID );

	return AK_Success;
}

// Retrieve all parameters at once
void CAkHarmonizerFXParams::GetParams( AkHarmonizerFXParams* out_pParams )
{
	*out_pParams = m_Params;
}
