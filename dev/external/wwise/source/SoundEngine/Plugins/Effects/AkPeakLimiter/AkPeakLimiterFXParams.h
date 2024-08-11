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

//////////////////////////////////////////////////////////////////////
//
// AkPeakLimiterFXParams.h
//
// Shared parameter implementation for peak limiter FX.
//
// Copyright 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#ifndef _AK_PEAKLIMITERPARAMS_H_
#define _AK_PEAKLIMITERPARAMS_H_

#include <AK/Plugin/AkPeakLimiterFXFactory.h>

struct AkPeakLimiterRTPCParams
{
	AkReal32	fThreshold;
	AkReal32	fRatio;
	AkReal32	fRelease;
	AkReal32	fOutputLevel;
	bool		bDirty;
};

struct AkPeakLimiterNonRTPCParams
{
	AkReal32	fLookAhead;
	bool		bProcessLFE;
	bool		bChannelLink;
	bool		bDirty;
};

struct AkPeakLimiterFXParams
{
	AkPeakLimiterRTPCParams RTPC;
	AkPeakLimiterNonRTPCParams NonRTPC;
} AK_ALIGN_DMA;

#ifndef __SPU__	// The rest need not to be known on SPU side

#include <AK/Tools/Common/AkAssert.h>

// Parameters IDs for the Wwise or RTPC.
static const AkPluginParamID AK_PEAKLIMITERFXPARAM_THRESHOLD_ID		= 0;	
static const AkPluginParamID AK_PEAKLIMITERFXPARAM_RATIO_ID			= 1;
static const AkPluginParamID AK_PEAKLIMITERFXPARAM_LOOKAHEAD_ID		= 2;	
static const AkPluginParamID AK_PEAKLIMITERFXPARAM_RELEASE_ID		= 3;
static const AkPluginParamID AK_PEAKLIMITERFXPARAM_GAIN_ID			= 4;
static const AkPluginParamID AK_PEAKLIMITERFXPARAM_PROCESSLFE_ID	= 5;	
static const AkPluginParamID AK_PEAKLIMITERFXPARAM_CHANNELLINK_ID	= 6;	

// Default values in case a bad parameter block is provided
#define AK_PEAKLIMITER_THRESHOLD_DEF	(-12.f)
#define AK_PEAKLIMITER_RATIO_DEF		(10.f)
#define AK_PEAKLIMITER_LOOKAHEAD_DEF	(0.01f)
#define AK_PEAKLIMITER_RELEASE_DEF		(0.2f)
#define AK_PEAKLIMITER_GAIN_DEF			(1.f) // linear value
#define AK_PEAKLIMITER_PROCESSLFE_DEF	(true)
#define AK_PEAKLIMITER_CHANNELLINK_DEF	(true)

//-----------------------------------------------------------------------------
// Name: class CAkPeakLimiterFXParams
// Desc: Shared dynamics processing FX parameters implementation.
//-----------------------------------------------------------------------------
class CAkPeakLimiterFXParams 
	: public AK::IAkPluginParam
	, public AkPeakLimiterFXParams
{
public:
    
    // Constructor/destructor.
    CAkPeakLimiterFXParams( );
    ~CAkPeakLimiterFXParams( );
	CAkPeakLimiterFXParams( const CAkPeakLimiterFXParams & in_rCopy );

    // Create duplicate.
    IAkPluginParam * Clone( AK::IAkPluginMemAlloc * in_pAllocator );

    // Init/Term.
    AKRESULT Init(	AK::IAkPluginMemAlloc *	in_pAllocator,						    
					const void *			in_pParamsBlock, 
					AkUInt32				in_ulBlockSize 
                         );
    AKRESULT Term( AK::IAkPluginMemAlloc * in_pAllocator );

    // Blob set.
    AKRESULT SetParamsBlock(	const void * in_pParamsBlock, 
								AkUInt32 in_ulBlockSize
                                );

    // Update one parameter.
    AKRESULT SetParam(	AkPluginParamID in_ParamID,
						const void * in_pValue, 
						AkUInt32 in_ulParamSize
                        );
};

#endif // #ifndef __SPU__

#endif // _AK_PEAKLIMITERPARAMS_H_
