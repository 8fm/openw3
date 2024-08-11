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

#include "AkTimeStretchFXParams.h"
#include <AK/Tools/Common/AkAssert.h>
#include <AK/Tools/Common/AkBankReadHelpers.h>


// Creation function
AK::IAkPluginParam * CreateTimeStretchFXParams( AK::IAkPluginMemAlloc * in_pAllocator )
{
	return AK_PLUGIN_NEW( in_pAllocator, CAkTimeStretchFXParams( ) );
}

// Constructor/destructor.
CAkTimeStretchFXParams::CAkTimeStretchFXParams( )
{
}

CAkTimeStretchFXParams::~CAkTimeStretchFXParams( )
{
}

// Copy constructor.
CAkTimeStretchFXParams::CAkTimeStretchFXParams( const CAkTimeStretchFXParams & in_rCopy )
{
	m_Params = in_rCopy.m_Params;
}

// Create duplicate.
AK::IAkPluginParam * CAkTimeStretchFXParams::Clone( AK::IAkPluginMemAlloc * in_pAllocator )
{
	return AK_PLUGIN_NEW( in_pAllocator, CAkTimeStretchFXParams( *this ) );
}

// Init/Term.
AKRESULT CAkTimeStretchFXParams::Init(	AK::IAkPluginMemAlloc *	in_pAllocator,									   
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

AKRESULT CAkTimeStretchFXParams::Term( AK::IAkPluginMemAlloc * in_pAllocator )
{
	AK_PLUGIN_DELETE( in_pAllocator, this );
	return AK_Success;
}

// Blob set.
AKRESULT CAkTimeStretchFXParams::SetParamsBlock(	const void * in_pParamsBlock, 
														AkUInt32 in_ulBlockSize
												)
{  
	AKRESULT eResult = AK_Success;
	AkUInt8 * pParamsBlock = (AkUInt8 *)in_pParamsBlock;

	m_Params.uWindowSize = READBANKDATA( AkUInt32, pParamsBlock, in_ulBlockSize );	
	m_Params.fTimeStretch = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );	
	m_Params.fTimeStretchRandom = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );	
	m_Params.fOutputGain = AK_DBTOLIN( READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize ) );	
	
	CHECKBANKDATASIZE( in_ulBlockSize, eResult );
	return eResult;
}

// Update one parameter.
AKRESULT CAkTimeStretchFXParams::SetParam(	AkPluginParamID in_ParamID,
												const void * in_pValue, 
												AkUInt32 in_ulParamSize )
{
	if ( in_pValue == NULL )
	{
		return AK_InvalidParameter;
	}

	switch ( in_ParamID )
	{
	case AK_TIMESTRETCHFXPARAM_WINDOWSIZE_ID:
		m_Params.uWindowSize = *((AkUInt32*)in_pValue);
		break;
	case AK_TIMESTRETCHFXPARAM_TIMSTRETCH_ID:
		m_Params.fTimeStretch = *((AkReal32*)in_pValue);
		break;
	case AK_TIMESTRETCHFXPARAM_OUTPUTGAIN_ID:
		m_Params.fOutputGain = AK_DBTOLIN( *((AkReal32*)in_pValue) );
		break;
	case AK_TIMESTRETCHFXPARAM_TIMSTRETCHRANDOM_ID:
		m_Params.fTimeStretchRandom = *((AkReal32*)in_pValue);
		break;
	}

	return AK_Success;
}

// Retrieve all parameters at once
void CAkTimeStretchFXParams::GetParams( AkTimeStretchFXParams* out_pParams )
{
	*out_pParams = m_Params;
}
