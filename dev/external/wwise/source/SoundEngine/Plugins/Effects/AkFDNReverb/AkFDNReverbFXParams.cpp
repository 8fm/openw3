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

#include "AkFDNReverbFXParams.h"
#include <AK/Tools/Common/AkBankReadHelpers.h>

// Creation function
AK::IAkPluginParam * CreateMatrixReverbFXParams( AK::IAkPluginMemAlloc * in_pAllocator )
{
    return AK_PLUGIN_NEW( in_pAllocator, CAkFDNReverbFXParams( ) );
}

// Constructor/destructor.
CAkFDNReverbFXParams::CAkFDNReverbFXParams( )
{
}

CAkFDNReverbFXParams::~CAkFDNReverbFXParams( )
{
}

// Copy constructor.
CAkFDNReverbFXParams::CAkFDNReverbFXParams( const CAkFDNReverbFXParams & in_rCopy )
{
	RTPC = in_rCopy.RTPC;
	NonRTPC = in_rCopy.NonRTPC;
	NonRTPC.bDirty = true;
}

// Create duplicate.
AK::IAkPluginParam * CAkFDNReverbFXParams::Clone( AK::IAkPluginMemAlloc * in_pAllocator )
{
    return AK_PLUGIN_NEW( in_pAllocator, CAkFDNReverbFXParams( *this ) );
}

// Init/Term.
AKRESULT CAkFDNReverbFXParams::Init(	AK::IAkPluginMemAlloc *	in_pAllocator,									   
										const void *			in_pParamsBlock, 
										AkUInt32				in_ulBlockSize 
                                     )
{
    if ( in_ulBlockSize == 0)
    {
        // Init default parameters.
		RTPC.fReverbTime = AK_FDNREVERB_REVERBTIME_DEF;
		RTPC.fHFRatio = AK_FDNREVERB_HFRATIO_DEF;
		RTPC.fDryLevel = powf( 10.f, AK_FDNREVERB_DRYLEVEL_DEF * 0.05f );
		RTPC.fWetLevel = powf( 10.f, AK_FDNREVERB_WETLEVEL_DEF * 0.05f );

		NonRTPC.uNumberOfDelays = AK_FDNREVERB_NUMBEROFDELAYS_DEF;
		NonRTPC.fPreDelay = AK_FDNREVERB_PREDELAY_DEF;
		NonRTPC.uProcessLFE = AK_FDNREVERB_PROCESSLFE_DEF;
		NonRTPC.uDelayLengthsMode = AK_FDNREVERB_DELAYLENGTHSMODE_DEF;
		NonRTPC.bDirty = true;

		// FX instance will use default delay lengths	
        return AK_Success;
    }
    return SetParamsBlock( in_pParamsBlock, in_ulBlockSize );
}

AKRESULT CAkFDNReverbFXParams::Term( AK::IAkPluginMemAlloc * in_pAllocator )
{
    AK_PLUGIN_DELETE( in_pAllocator, this );
    return AK_Success;
}

// Blob set.
AKRESULT CAkFDNReverbFXParams::SetParamsBlock(	const void * in_pParamsBlock, 
                                                AkUInt32 in_ulBlockSize
                                               )
{
	AKRESULT eResult = AK_Success;
	AkUInt8 * pParamsBlock = (AkUInt8 *)in_pParamsBlock;

	RTPC.fReverbTime = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );	
	RTPC.fHFRatio = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );	
	NonRTPC.uNumberOfDelays = READBANKDATA( AkUInt32, pParamsBlock, in_ulBlockSize );	
	RTPC.fDryLevel = AK_DBTOLIN( READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize ) );
	RTPC.fWetLevel = AK_DBTOLIN( READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize ) );
	NonRTPC.fPreDelay = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );	
	NonRTPC.uProcessLFE = *((bool *)pParamsBlock) ? 1 : 0; 
	SKIPBANKDATA( bool, pParamsBlock, in_ulBlockSize );
	NonRTPC.uDelayLengthsMode = READBANKDATA( AkUInt32, pParamsBlock, in_ulBlockSize );	
	// In default mode, FX instance will use default delay lengths
	if ( NonRTPC.uDelayLengthsMode == AKDELAYLENGTHSMODE_CUSTOM ) 
	{
		// Use delay parameters provide
		for ( unsigned int i = 0; i < NonRTPC.uNumberOfDelays; ++i )
		{
			NonRTPC.fDelayTime[i] = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );	
		}
	}
	AKASSERT( NonRTPC.uDelayLengthsMode == AKDELAYLENGTHSMODE_CUSTOM || NonRTPC.uDelayLengthsMode == AK_FDNREVERB_DELAYLENGTHSMODE_DEF );

	NonRTPC.bDirty = true;

    CHECKBANKDATASIZE( in_ulBlockSize, eResult );
    return eResult;
}

// Update one parameter.
AKRESULT CAkFDNReverbFXParams::SetParam(	AkPluginParamID in_ParamID,
											const void * in_pValue, 
											AkUInt32 in_ulParamSize
                                         )
{
	AKRESULT eResult = AK_Success;

	switch ( in_ParamID )
	{
	case AK_FDNREVERBFXPARAM_REVERBTIME_ID:	
		RTPC.fReverbTime = *((AkReal32*)in_pValue);
		break;
	case AK_FDNREVERBFXPARAM_HFRATIO_ID:
		RTPC.fHFRatio = *((AkReal32*)in_pValue);
		break;
	case AK_FDNREVERBFXPARAM_NUMBEROFDELAYS_ID:	
		NonRTPC.uNumberOfDelays = *((AkUInt32*)in_pValue);
		NonRTPC.bDirty = true;
		break;
	case AK_FDNREVERBFXPARAM_DRYLEVEL_ID:
		RTPC.fDryLevel = powf( 10.f, *((AkReal32*)in_pValue) * 0.05f );
		break;
	case AK_FDNREVERBFXPARAM_WETLEVEL_ID:	
		RTPC.fWetLevel = powf( 10.f, *((AkReal32*)in_pValue) * 0.05f );
		break;
	case AK_FDNREVERBFXPARAM_PREDELAY_ID:
		NonRTPC.fPreDelay = *((AkReal32*)in_pValue);
		NonRTPC.bDirty = true;
		break;
	case AK_FDNREVERBFXPARAM_PROCESSLFE_ID:
		NonRTPC.uProcessLFE = *((bool*)in_pValue) ? 1 : 0;
		NonRTPC.bDirty = true;
		break;
	case AK_FDNREVERBFXPARAM_DELAYLENGTHSMODE_ID:
		NonRTPC.uDelayLengthsMode = *((AkUInt32*)in_pValue);
		NonRTPC.bDirty = true;
		break;
	default:
		// Note: These may be ignored by FX instance in default delay mode
		AKASSERT( in_ParamID >= AK_FDNREVERBFXPARAM_FIRSTDELAYTIME_ID && in_ParamID <= AK_FDNREVERBFXPARAM_LASTDELAYTIME_ID );
		NonRTPC.fDelayTime[in_ParamID-AK_FDNREVERBFXPARAM_FIRSTDELAYTIME_ID] = *((AkReal32*)in_pValue);
		NonRTPC.bDirty = true;
	}

	return eResult;

}
