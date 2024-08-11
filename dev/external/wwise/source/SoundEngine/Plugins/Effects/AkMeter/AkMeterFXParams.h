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
// AkMeterFXParams.h
//
// Shared parameter implementation for compressor FX.
//
// Copyright 2010 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#ifndef _AK_METERPARAMS_H_
#define _AK_METERPARAMS_H_

#include <AK/Plugin/AkMeterFXFactory.h>

enum AkMeterMode
{
	AkMeterMode_Peak = 0,
	AkMeterMode_RMS
};

struct AkMeterRTPCParams
{
	AkReal32		fAttack;
	AkReal32		fRelease;
	AkReal32		fMin;
	AkReal32		fMax;
	AkReal32		fHold;
};

struct AkMeterNonRTPCParams
{
	AkMeterMode		eMode;
	AkUniqueID		uGameParamID;
};

	// Structure of MeterFX parameters
struct AkMeterFXParams
{
	AkMeterRTPCParams		RTPC;
	AkMeterNonRTPCParams	NonRTPC;
} AK_ALIGN_DMA;

#define HOLD_MEMORY_SIZE 8

struct AkMeterState
{
	AkUInt32	uSampleRate;
	AkReal32	fHoldTime;
	AkReal32	fReleaseTarget;
	AkReal32	fLastValue;
	AkReal32	fHoldMemory[ HOLD_MEMORY_SIZE ];

#if defined(AK_PS3) && !defined(AK_OPTIMIZED)
	void *		pMeterData;
#endif
} AK_ALIGN_DMA;

#ifndef __SPU__

#include <AK/Tools/Common/AkAssert.h>

// Parameters IDs for the Wwise or RTPC.
static const AkPluginParamID AK_METERFXPARAM_ATTACK_ID	= 0;	
static const AkPluginParamID AK_METERFXPARAM_RELEASE_ID	= 1;
static const AkPluginParamID AK_METERFXPARAM_MODE_ID	= 2;
static const AkPluginParamID AK_METERFXPARAM_MIN_ID = 4;
static const AkPluginParamID AK_METERFXPARAM_MAX_ID = 5;
static const AkPluginParamID AK_METERFXPARAM_HOLD_ID = 6;

// Default values in case a bad parameter block is provided
#define AK_METER_ATTACK_DEF		(0.0f)
#define AK_METER_RELEASE_DEF	(0.1f)
#define AK_METER_MODE_DEF		(AkMeterMode_Peak)
#define AK_METER_GAMEPARAM_DEF	(AK_INVALID_UNIQUE_ID)
#define AK_METER_MIN_DEF		(-48.0f)
#define AK_METER_MAX_DEF		(6.0f)
#define AK_METER_HOLD_DEF		(0.0f)

class CAkMeterFXParams 
	: public AK::IAkPluginParam
	, public AkMeterFXParams
{
public:

	friend class CAkMeterFX;
    
    // Constructor/destructor.
    CAkMeterFXParams( );
    ~CAkMeterFXParams( );
	CAkMeterFXParams( const CAkMeterFXParams & in_rCopy );

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

#endif // _AK_METERPARAMS_H_
