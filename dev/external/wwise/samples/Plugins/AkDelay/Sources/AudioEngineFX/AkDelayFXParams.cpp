//////////////////////////////////////////////////////////////////////
//
// AkDelayFXParams.cpp
//
// Delay FX parameters sample implementation
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include <AK/Tools/Common/AkAssert.h>
#include <AK/Tools/Common/AkBankReadHelpers.h>
#include <math.h>
#include "AkDelayFXParams.h"

/// Plugin mechanism. Instantiation method that must be registered to the plug-in manager.
AK::IAkPluginParam * CreateDelayFXParams( AK::IAkPluginMemAlloc * in_pAllocator )
{
    return AK_PLUGIN_NEW( in_pAllocator, CAkDelayFXParams( ) );
}

// Constructor.
CAkDelayFXParams::CAkDelayFXParams( )
{
}

// Destructor.
CAkDelayFXParams::~CAkDelayFXParams( )
{
}

// Copy constructor.
CAkDelayFXParams::CAkDelayFXParams( const CAkDelayFXParams & in_rCopy )
{
	RTPC = in_rCopy.RTPC;
	RTPC.bHasChanged = true;
	NonRTPC = in_rCopy.NonRTPC;
	NonRTPC.bHasChanged = true;
}

// Create parameter node duplicate.
AK::IAkPluginParam * CAkDelayFXParams::Clone( AK::IAkPluginMemAlloc * in_pAllocator )
{
    return AK_PLUGIN_NEW( in_pAllocator, CAkDelayFXParams( *this ) );
}

// Parameter node initialization.
AKRESULT CAkDelayFXParams::Init(	
								AK::IAkPluginMemAlloc *	in_pAllocator,									   
								const void *			in_pParamsBlock, 
								AkUInt32				in_ulBlockSize  )
{
    if ( in_ulBlockSize == 0)
    {
		// Init default parameters.
		NonRTPC.fDelayTime = DELAYFXPARAM_DELAYTIME_DEF;
		RTPC.fFeedback = DELAYFXPARAM_FEEDBACK_DEF * ONEOVER_DELAYFXPARAM_PERCENT_MAX;
		RTPC.fWetDryMix = DELAYFXPARAM_WETDRYMIX_DEF * ONEOVER_DELAYFXPARAM_PERCENT_MAX;
		RTPC.fOutputLevel = powf( 10.f, DELAYFXPARAM_OUTPUTLEVEL_DEF * 0.05f );
		RTPC.bFeedbackEnabled = DELAYFXPARAM_FEEDBACKENABLED_DEF;
		RTPC.bHasChanged = true;
		NonRTPC.bProcessLFE = DELAYFXPARAM_PROCESSLFE_DEF;
		NonRTPC.bHasChanged = true;
		return AK_Success;
    }
    return SetParamsBlock( in_pParamsBlock, in_ulBlockSize );
}

// Parameter interface termination.
AKRESULT CAkDelayFXParams::Term( AK::IAkPluginMemAlloc * in_pAllocator )
{
    AK_PLUGIN_DELETE( in_pAllocator, this );
    return AK_Success;
}

// Parameter block set.
AKRESULT CAkDelayFXParams::SetParamsBlock( 
	const void * in_pParamsBlock, 
	AkUInt32 in_ulBlockSize )
{
	AKRESULT eResult = AK_Success;
	AkUInt8 * pParamsBlock = (AkUInt8 *)in_pParamsBlock;
	NonRTPC.fDelayTime = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );
	RTPC.fFeedback = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );
	RTPC.fWetDryMix = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );
	RTPC.fOutputLevel = AK_DBTOLIN( READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize ) );
	RTPC.bFeedbackEnabled = READBANKDATA( bool, pParamsBlock, in_ulBlockSize );
	NonRTPC.bProcessLFE = READBANKDATA( bool, pParamsBlock, in_ulBlockSize );
	CHECKBANKDATASIZE( in_ulBlockSize, eResult );

	// Range translation
	RTPC.fFeedback *= ONEOVER_DELAYFXPARAM_PERCENT_MAX;					// From percentage to linear gain
	RTPC.fWetDryMix *= ONEOVER_DELAYFXPARAM_PERCENT_MAX;				// From percentage to linear gain

	RTPC.bHasChanged = true;
	NonRTPC.bHasChanged = true;

    return eResult;
}

// Update a single parameter.
AKRESULT CAkDelayFXParams::SetParam(	
									AkPluginParamID in_ParamID,
									const void * in_pValue, 
									AkUInt32 in_ulParamSize )
{
	AKRESULT eResult = AK_Success;

	switch ( in_ParamID )
	{
	case AK_DELAYFXPARAM_DELAYTIME_ID:	
		NonRTPC.fDelayTime = *(AkReal32*)(in_pValue);
		NonRTPC.bHasChanged = true;
		break;
	case AK_DELAYFXPARAM_FEEDBACK_ID:	// RTPC
		RTPC.fFeedback = *(AkReal32*)(in_pValue);
		RTPC.fFeedback *= ONEOVER_DELAYFXPARAM_PERCENT_MAX;
		RTPC.bHasChanged = true;
		break;
	case AK_DELAYFXPARAM_WETDRYMIX_ID:	// RTPC
		RTPC.fWetDryMix = *(AkReal32*)(in_pValue);
		RTPC.fWetDryMix *= ONEOVER_DELAYFXPARAM_PERCENT_MAX;	
		break;
	case AK_DELAYFXPARAM_OUTPUTGAIN_ID:	// RTPC
		RTPC.fOutputLevel = *(AkReal32*)(in_pValue);	
		RTPC.fOutputLevel = powf( 10.f, ( RTPC.fOutputLevel * 0.05f ) ); // Make it a linear value	
		break;
	case AK_DELAYFXPARAM_FEEDBACKENABLED_ID:
		// Note RTPC parameters are always of type float regardless of property type in XML plugin description
		RTPC.bFeedbackEnabled = (*(AkReal32*)(in_pValue)) != 0;
		RTPC.bHasChanged = true;
		break;
	case AK_DELAYFXPARAM_PROCESSLFE_ID:
		NonRTPC.bProcessLFE = *(bool*)(in_pValue);
		NonRTPC.bHasChanged = true;
		break;
	default:
		AKASSERT(!"Invalid parameter.");
		eResult = AK_InvalidParameter;
	}

	return eResult;
}
