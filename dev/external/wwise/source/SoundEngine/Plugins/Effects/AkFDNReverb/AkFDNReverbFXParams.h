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
// AkFDNReverbFXParams.h
//
// Shared parameter implementation for FDN reverb FX.
//
// Copyright 2007 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#ifndef _AK_FDNREVERBFXPARAMS_H_
#define _AK_FDNREVERBFXPARAMS_H_

#include <AK/SoundEngine/Common/AkTypes.h>
#include <AK/Plugin/AkMatrixReverbFXFactory.h>

#define MAXNUMDELAYS (16)

// Structure of parameters that remain true for the whole lifespan of the tone generator.

struct AkFDNReverbRTPCParams
{
	AkReal32	fReverbTime;
	AkReal32	fHFRatio;
	AkReal32	fDryLevel;
	AkReal32	fWetLevel;
};

struct AkFDNReverbNonRTPCParams
{
	AkUInt32	uNumberOfDelays;
	AkReal32	fPreDelay;
	AkUInt32	uProcessLFE;
	AkUInt32	uDelayLengthsMode;
	AkReal32	fDelayTime[MAXNUMDELAYS];
	bool		bDirty;
};

struct AkFDNReverbFXParams
{
	AkFDNReverbRTPCParams RTPC;
	AkFDNReverbNonRTPCParams NonRTPC;
} AK_ALIGN_DMA;

#ifndef __SPU__

#include <AK/Tools/Common/AkAssert.h>
#include <math.h>

// Parameters IDs for Wwise or RTPC.
static const AkPluginParamID AK_FDNREVERBFXPARAM_REVERBTIME_ID				= 0;	// RTPC
static const AkPluginParamID AK_FDNREVERBFXPARAM_HFRATIO_ID					= 1;	// RTPC
static const AkPluginParamID AK_FDNREVERBFXPARAM_NUMBEROFDELAYS_ID			= 2;	
static const AkPluginParamID AK_FDNREVERBFXPARAM_DRYLEVEL_ID				= 3;	// RTPC
static const AkPluginParamID AK_FDNREVERBFXPARAM_WETLEVEL_ID				= 4;	// RTPC
static const AkPluginParamID AK_FDNREVERBFXPARAM_PREDELAY_ID				= 5;	
static const AkPluginParamID AK_FDNREVERBFXPARAM_PROCESSLFE_ID				= 6;	
static const AkPluginParamID AK_FDNREVERBFXPARAM_DELAYLENGTHSMODE_ID		= 7;
static const AkPluginParamID AK_FDNREVERBFXPARAM_FIRSTDELAYTIME_ID			= 8;
static const AkPluginParamID AK_FDNREVERBFXPARAM_LASTDELAYTIME_ID			= 23;

// Default values in case a bad parameter block is provided
#define AK_FDNREVERB_REVERBTIME_DEF				(4.f)
#define AK_FDNREVERB_HFRATIO_DEF				(2.f)
#define AK_FDNREVERB_NUMBEROFDELAYS_DEF			(8)
#define AK_FDNREVERB_DRYLEVEL_DEF				(-3.f)
#define AK_FDNREVERB_WETLEVEL_DEF				(-10.f)
#define AK_FDNREVERB_PREDELAY_DEF				(0.f)
#define AK_FDNREVERB_PROCESSLFE_DEF				(1)
#define AK_FDNREVERB_DELAYLENGTHSMODE_DEF		(AKDELAYLENGTHSMODE_DEFAULT)

//-----------------------------------------------------------------------------
// Name: class CAkFDNReverbFXParams
// Desc: Shared FDN reverb FX parameters implementation.
//-----------------------------------------------------------------------------
class CAkFDNReverbFXParams 
	: public AK::IAkPluginParam
	, public AkFDNReverbFXParams
{
public:

    // Constructor/destructor.
    CAkFDNReverbFXParams( );
    ~CAkFDNReverbFXParams();
	CAkFDNReverbFXParams( const CAkFDNReverbFXParams & in_rCopy );

    // Create duplicate.
    IAkPluginParam * Clone( AK::IAkPluginMemAlloc * in_pAllocator );

    // Init/Term.
    AKRESULT Init( AK::IAkPluginMemAlloc *	in_pAllocator,						    
                   const void *				in_pParamsBlock, 
                   AkUInt32					in_ulBlockSize 
                   );
    AKRESULT Term( AK::IAkPluginMemAlloc * in_pAllocator );

    // Blob set.
    AKRESULT SetParamsBlock( const void * in_pParamsBlock, 
                             AkUInt32 in_ulBlockSize
                             );

    // Update one parameter.
    AKRESULT SetParam(	AkPluginParamID in_ParamID,
                        const void * in_pValue, 
                        AkUInt32 in_ulParamSize
                        );
};

#endif // __SPU__

#endif // _AK_FDNREVERBFXPARAMS_H_
