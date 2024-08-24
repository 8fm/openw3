//////////////////////////////////////////////////////////////////////
//
// AkFXSrcMP3Params.cpp
//
// Allows for audio source to come from an external input.
// 
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "AkFXSrcMP3Params.h"
#include <AK/Tools/Common/AkBankReadHelpers.h>
#if defined AK_WII_FAMILY
#include <string.h>
#endif

// Plugin mechanism. Parameter node create function to be registered to the FX manager.
AK::IAkPluginParam * CreateMP3SourceParams( AK::IAkPluginMemAlloc * in_pAllocator )
{
	return AK_PLUGIN_NEW( in_pAllocator, CAkFxSrcMP3Params() );
}

// Constructor.
CAkFxSrcMP3Params::CAkFxSrcMP3Params()
{

}

// Destructor.
CAkFxSrcMP3Params::~CAkFxSrcMP3Params()
{

}

// Copy constructor.
CAkFxSrcMP3Params::CAkFxSrcMP3Params( const CAkFxSrcMP3Params & in_rCopy )
{
	m_Params = in_rCopy.m_Params;	 
}

// Create parameters node duplicate.
AK::IAkPluginParam * CAkFxSrcMP3Params::Clone( AK::IAkPluginMemAlloc * in_pAllocator )
{
	return AK_PLUGIN_NEW( in_pAllocator, CAkFxSrcMP3Params(*this) );
}

// Shared parameters initialization.
AKRESULT CAkFxSrcMP3Params::Init( 
	AK::IAkPluginMemAlloc *	in_pAllocator,								   
	const void * in_pParamsBlock, 
	AkUInt32 in_ulBlockSize )
{
    if ( in_ulBlockSize == 0)
    {
		// Init with default values if we got invalid parameter block.
        m_Params.uToken = 0;
        return AK_Success;
    }

    return SetParamsBlock( in_pParamsBlock, in_ulBlockSize );
}

// Parameter node termination.
AKRESULT CAkFxSrcMP3Params::Term( AK::IAkPluginMemAlloc * in_pAllocator )
{
	AK_PLUGIN_DELETE( in_pAllocator, this );
    return AK_Success;
}

// Set all shared parameters at once.
AKRESULT CAkFxSrcMP3Params::SetParamsBlock( 
	const void * in_pParamsBlock, 
	AkUInt32 in_ulBlockSize )
{
	AKRESULT eResult = AK_Success;
	AkUInt8 * pParamsBlock = (AkUInt8 *)in_pParamsBlock;
	m_Params.uToken = READBANKDATA( AkUInt32, pParamsBlock, in_ulBlockSize );
	CHECKBANKDATASIZE( in_ulBlockSize, eResult );
    return eResult;
}

// Update single parameter.
AKRESULT CAkFxSrcMP3Params::SetParam( 
	AkPluginParamID	in_ParamID,
	const void * in_pValue, 
	AkUInt32 in_ulParamSize )
{
	// Consistency check.
	if ( in_pValue == NULL )
	{
		return AK_InvalidParameter;
	}

    // Set parameter value.
    switch ( in_ParamID )
    {
	case AK_SRCMP3_FXPARAM_TOKEN_ID:
		m_Params.uToken = *reinterpret_cast<const AkUInt32*>(in_pValue);
		break;
	default:
		return AK_InvalidParameter;
	}

    return AK_Success;
}