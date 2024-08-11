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

#include "AkGainFXParams.h"
#include <AK/Tools/Common/AkBankReadHelpers.h>

// Creation function
AK::IAkPluginParam * CreateGainFXParams( AK::IAkPluginMemAlloc * in_pAllocator )
{
	AKASSERT( in_pAllocator != NULL );
	return AK_PLUGIN_NEW( in_pAllocator, CAkGainFXParams( ) );
}

// Constructor/destructor.
CAkGainFXParams::CAkGainFXParams( )
{
}

CAkGainFXParams::~CAkGainFXParams( )
{
}

// Copy constructor.
CAkGainFXParams::CAkGainFXParams( const CAkGainFXParams & in_rCopy )
{
	m_Params = in_rCopy.m_Params;
}

// Create duplicate.
AK::IAkPluginParam * CAkGainFXParams::Clone( AK::IAkPluginMemAlloc * in_pAllocator )
{
	AKASSERT( in_pAllocator != NULL );
	return AK_PLUGIN_NEW( in_pAllocator, CAkGainFXParams( *this ) );
}

// Init/Term.
AKRESULT CAkGainFXParams::Init(	AK::IAkPluginMemAlloc *	in_pAllocator,									   
										const void *			in_pParamsBlock, 
										AkUInt32				in_ulBlockSize )
{
	if ( in_ulBlockSize == 0)
	{
		// Init default parameters.
		m_Params.fFullbandGain = GAIN_FULLBAND_DEF;
		m_Params.fLFEGain = GAIN_LFE_DEF;

		return AK_Success;
	}
	return SetParamsBlock( in_pParamsBlock, in_ulBlockSize );
}

AKRESULT CAkGainFXParams::Term( AK::IAkPluginMemAlloc * in_pAllocator )
{
	AKASSERT( in_pAllocator != NULL );
	AK_PLUGIN_DELETE( in_pAllocator, this );
	return AK_Success;
}

// Blob set.
AKRESULT CAkGainFXParams::SetParamsBlock(	const void * in_pParamsBlock, 
													AkUInt32 in_ulBlockSize
												)
{  
	AKRESULT eResult = AK_Success;
	AkUInt8 * pParamsBlock = (AkUInt8 *)in_pParamsBlock;

	m_Params.fFullbandGain =	READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );	
	m_Params.fLFEGain =			READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );	

	CHECKBANKDATASIZE( in_ulBlockSize, eResult );
	return eResult;
}

// Update one parameter.
AKRESULT CAkGainFXParams::SetParam(	AkPluginParamID in_ParamID,
											const void * in_pValue, 
											AkUInt32 in_ulParamSize )
{
	AKASSERT( in_pValue != NULL );
	if ( in_pValue == NULL )
	{
		return AK_InvalidParameter;
	}
	AKRESULT eResult = AK_Success;


	switch ( in_ParamID )
	{
	case AK_GAINFXPARAM_FULLBANDGAIN_ID:
		m_Params.fFullbandGain = *reinterpret_cast<const AkReal32*>(in_pValue);
		break;
	case AK_GAINFXPARAM_LFEGAIN_ID:
		m_Params.fLFEGain = *reinterpret_cast<const AkReal32*>(in_pValue);
		break;
	default:
		AKASSERT(!"Invalid parameter.");
		eResult = AK_InvalidParameter;
	}

	return eResult;

}