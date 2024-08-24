//////////////////////////////////////////////////////////////////////
//
// AkFXSrcAudioInputParams.cpp
//
// Allows for audio source to come from an external input.
// 
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "AkFXSrcAudioInputParams.h"
#include <AK/Tools/Common/AkBankReadHelpers.h>
#if defined(AK_WII) || defined (AK_WIIU) || defined(AK_APPLE)
#include <string.h>
#endif

// Plugin mechanism. Parameter node create function to be registered to the FX manager.
AK::IAkPluginParam * CreateAudioInputSourceParams( AK::IAkPluginMemAlloc * in_pAllocator )
{
	return AK_PLUGIN_NEW( in_pAllocator, CAkFxSrcAudioInputParams() );
}

// Constructor.
CAkFxSrcAudioInputParams::CAkFxSrcAudioInputParams()
{

}

// Destructor.
CAkFxSrcAudioInputParams::~CAkFxSrcAudioInputParams()
{

}

// Copy constructor.
CAkFxSrcAudioInputParams::CAkFxSrcAudioInputParams( const CAkFxSrcAudioInputParams & in_rCopy )
{
	m_Params = in_rCopy.m_Params;	 
}

// Create parameters node duplicate.
AK::IAkPluginParam * CAkFxSrcAudioInputParams::Clone( AK::IAkPluginMemAlloc * in_pAllocator )
{
	return AK_PLUGIN_NEW( in_pAllocator, CAkFxSrcAudioInputParams(*this) );
}

// Shared parameters initialization.
AKRESULT CAkFxSrcAudioInputParams::Init( AK::IAkPluginMemAlloc *	in_pAllocator,								   
										 const void *				in_pParamsBlock, 
										 AkUInt32					in_ulBlockSize 
                                 )
{
    if ( in_ulBlockSize == 0)
    {
		// Init with default values if we got invalid parameter block.
        m_Params.fGain  = 0.f; // Gain (in dB FS)
        return AK_Success;
    }

    return SetParamsBlock( in_pParamsBlock, in_ulBlockSize );
}

// Parameter node termination.
AKRESULT CAkFxSrcAudioInputParams::Term( AK::IAkPluginMemAlloc * in_pAllocator )
{
	AK_PLUGIN_DELETE( in_pAllocator, this );
    return AK_Success;
}

// Set all shared parameters at once.
AKRESULT CAkFxSrcAudioInputParams::SetParamsBlock( const void * in_pParamsBlock, 
												   AkUInt32 in_ulBlockSize
                                           )
{
	AKRESULT eResult = AK_Success;
	AkUInt8 * pParamsBlock = (AkUInt8 *)in_pParamsBlock;
	m_Params.fGain = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );
	CHECKBANKDATASIZE( in_ulBlockSize, eResult );
    return eResult;
}

// Update single parameter.
AKRESULT CAkFxSrcAudioInputParams::SetParam( AkPluginParamID	in_ParamID,
											 const void *		in_pValue, 
											 AkUInt32			in_ulParamSize
                                     )
{
	// Consistency check.
	if ( in_pValue == NULL )
		return AK_InvalidParameter;

    // Set parameter value.
    switch ( in_ParamID )
    {
	case AK_SRCAUDIOINPUT_FXPARAM_GAIN_ID:
		m_Params.fGain = *reinterpret_cast<const AkReal32*>(in_pValue);
		break;
	default:
		return AK_InvalidParameter;
	}

    return AK_Success;
}