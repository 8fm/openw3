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

#include "AkPeakLimiterFXParams.h"
#include <math.h>
#include <AK/Tools/Common/AkBankReadHelpers.h>

// Creation function
AK::IAkPluginParam * CreatePeakLimiterFXParams( AK::IAkPluginMemAlloc * in_pAllocator )
{
	return AK_PLUGIN_NEW( in_pAllocator, CAkPeakLimiterFXParams( ) );
}

// Constructor/destructor.
CAkPeakLimiterFXParams::CAkPeakLimiterFXParams( )
{
}

CAkPeakLimiterFXParams::~CAkPeakLimiterFXParams( )
{
}

// Copy constructor.
CAkPeakLimiterFXParams::CAkPeakLimiterFXParams( const CAkPeakLimiterFXParams & in_rCopy )
{
	RTPC = in_rCopy.RTPC;
	RTPC.bDirty = true;
	NonRTPC = in_rCopy.NonRTPC;
	NonRTPC.bDirty = true;
}

// Create duplicate.
AK::IAkPluginParam * CAkPeakLimiterFXParams::Clone( AK::IAkPluginMemAlloc * in_pAllocator )
{
	return AK_PLUGIN_NEW( in_pAllocator, CAkPeakLimiterFXParams( *this ) );
}

// Init.
AKRESULT CAkPeakLimiterFXParams::Init(	AK::IAkPluginMemAlloc *	in_pAllocator,									   
										const void *			in_pParamsBlock, 
										AkUInt32				in_ulBlockSize )
{
	if ( in_ulBlockSize == 0)
	{
		// Init default parameters.
		RTPC.fThreshold = AK_PEAKLIMITER_THRESHOLD_DEF;
		RTPC.fRatio = AK_PEAKLIMITER_RATIO_DEF;
		RTPC.fRelease = AK_PEAKLIMITER_RELEASE_DEF;
		RTPC.fOutputLevel = AK_PEAKLIMITER_GAIN_DEF;
		RTPC.bDirty = true;

		NonRTPC.fLookAhead = AK_PEAKLIMITER_LOOKAHEAD_DEF;
		NonRTPC.bProcessLFE = AK_PEAKLIMITER_PROCESSLFE_DEF;
		NonRTPC.bChannelLink = AK_PEAKLIMITER_CHANNELLINK_DEF;
		NonRTPC.bDirty = true;

		return AK_Success;
	}
	return SetParamsBlock( in_pParamsBlock, in_ulBlockSize );
}

// Term.
AKRESULT CAkPeakLimiterFXParams::Term( AK::IAkPluginMemAlloc * in_pAllocator )
{
	AK_PLUGIN_DELETE( in_pAllocator, this );
	return AK_Success;
}

// Blob set.
AKRESULT CAkPeakLimiterFXParams::SetParamsBlock(	const void * in_pParamsBlock, 
													AkUInt32 in_ulBlockSize
												)
{  
	AKRESULT eResult = AK_Success;
	AkUInt8 * pParamsBlock = (AkUInt8 *)in_pParamsBlock;

	RTPC.fThreshold = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );	
	RTPC.fRatio = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );	
	NonRTPC.fLookAhead = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );	
	RTPC.fRelease = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );	
	RTPC.fOutputLevel  = AK_DBTOLIN( READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize ) );
	NonRTPC.bProcessLFE = READBANKDATA( bool, pParamsBlock, in_ulBlockSize );	
	NonRTPC.bChannelLink = READBANKDATA( bool, pParamsBlock, in_ulBlockSize );	

	RTPC.bDirty = true;
	NonRTPC.bDirty = true;

	CHECKBANKDATASIZE( in_ulBlockSize, eResult );
	return eResult;
}

// Update one parameter.
AKRESULT CAkPeakLimiterFXParams::SetParam(	AkPluginParamID in_ParamID,
											const void * in_pValue, 
											AkUInt32 in_ulParamSize )
{
	AKRESULT eResult = AK_Success;

	switch ( in_ParamID )
	{
	case AK_PEAKLIMITERFXPARAM_THRESHOLD_ID:
		RTPC.fThreshold = *reinterpret_cast<const AkReal32*>(in_pValue);
		RTPC.bDirty = true;
		break;
	case AK_PEAKLIMITERFXPARAM_RATIO_ID:
		RTPC.fRatio = *reinterpret_cast<const AkReal32*>(in_pValue);
		RTPC.bDirty = true;
		break;
	case AK_PEAKLIMITERFXPARAM_LOOKAHEAD_ID:
		NonRTPC.fLookAhead = *reinterpret_cast<const AkReal32*>(in_pValue);
		NonRTPC.bDirty = true;
		break;
	case AK_PEAKLIMITERFXPARAM_RELEASE_ID:
		RTPC.fRelease = *reinterpret_cast<const AkReal32*>(in_pValue);
		RTPC.bDirty = true;
		break;
	case AK_PEAKLIMITERFXPARAM_GAIN_ID:
		{
			AkReal32 fDbVal = *reinterpret_cast<const AkReal32*>(in_pValue);
			RTPC.fOutputLevel  = powf( 10.f, fDbVal * 0.05f );
			RTPC.bDirty = true;
		}
		break;
	case AK_PEAKLIMITERFXPARAM_PROCESSLFE_ID:
		NonRTPC.bProcessLFE = *reinterpret_cast<const bool*>(in_pValue);
		NonRTPC.bDirty = true;
		break;
	case AK_PEAKLIMITERFXPARAM_CHANNELLINK_ID:
		NonRTPC.bChannelLink = *reinterpret_cast<const bool*>(in_pValue);
		NonRTPC.bDirty = true;
		break;
	default:
		AKASSERT(!"Invalid parameter.");
		eResult = AK_InvalidParameter;
	}

	return eResult;
}