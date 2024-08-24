//////////////////////////////////////////////////////////////////////
//
// AkDelayFXParams.h
//
// Shared parameter implementation for Delay FX.
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#ifndef _AK_DELAYFXPARAMS_H_
#define _AK_DELAYFXPARAMS_H_

#include <AK/SoundEngine/Common/IAkPlugin.h>

// Parameters IDs for the Wwise or RTPC.
// Those IDs map to the AudioEnginePropertyID attributes in the XML property definition
static const AkPluginParamID AK_DELAYFXPARAM_DELAYTIME_ID			= 0;
static const AkPluginParamID AK_DELAYFXPARAM_FEEDBACK_ID			= 1;	// RTPC
static const AkPluginParamID AK_DELAYFXPARAM_WETDRYMIX_ID			= 2;	// RTPC
static const AkPluginParamID AK_DELAYFXPARAM_OUTPUTGAIN_ID			= 3;	// RTPC
static const AkPluginParamID AK_DELAYFXPARAM_FEEDBACKENABLED_ID		= 4;	// RTPC
static const AkPluginParamID AK_DELAYFXPARAM_PROCESSLFE_ID			= 5;	

static const AkPluginParamID AK_DELAYFXPARAM_NUM					= 6;

// Default parameter values
#define DELAYFXPARAM_DELAYTIME_DEF			(0.5f)		// secs
#define DELAYFXPARAM_FEEDBACK_DEF			(0.f)		// Percent
#define DELAYFXPARAM_WETDRYMIX_DEF			(50.f)		// Percent
#define DELAYFXPARAM_OUTPUTLEVEL_DEF		(0.f)		// dBFS
#define DELAYFXPARAM_FEEDBACKENABLED_DEF	(false)		// Disabled
#define DELAYFXPARAM_PROCESSLFE_DEF			(true)

static const AkReal32 ONEOVER_DELAYFXPARAM_PERCENT_MAX = 0.01f;

struct AkDelayRTPCParams
{
	AkReal32	fFeedback;
	AkReal32	fWetDryMix;
	AkReal32	fOutputLevel;
	bool		bFeedbackEnabled;
	bool		bHasChanged;
};

struct AkDelayNonRTPCParams
{
	AkReal32	fDelayTime;
	bool		bProcessLFE;
	bool		bHasChanged;
};

struct AkDelayFXParams
{
	AkDelayRTPCParams RTPC;
	AkDelayNonRTPCParams NonRTPC;
} AK_ALIGN_DMA;

#ifndef __SPU__

class CAkDelayFXParams 
	: public AK::IAkPluginParam
	, public AkDelayFXParams
{
public:

    /// Constructor.
    CAkDelayFXParams( );

	/// Copy constructor.
	CAkDelayFXParams( const CAkDelayFXParams & in_rCopy );

	/// Destructor.
    ~CAkDelayFXParams();

    /// Create parameter node duplicate.
    IAkPluginParam * Clone( AK::IAkPluginMemAlloc * in_pAllocator );

    /// Parameter node initialization.
	AKRESULT Init(	
		AK::IAkPluginMemAlloc *	in_pAllocator,						    
		const void *			in_pParamsBlock, 
		AkUInt32				in_ulBlockSize 
		);

	/// Parameter node termination.
    AKRESULT Term( AK::IAkPluginMemAlloc * in_pAllocator );

    /// Parameter block set.
    AKRESULT SetParamsBlock(	
		const void *in_pParamsBlock, 
		AkUInt32	in_ulBlockSize
		);

    /// Update a single parameter.
	AKRESULT SetParam(	
		AkPluginParamID in_ParamID,
		const void *	in_pValue, 
		AkUInt32		in_ulParamSize
		);
};

#endif // __SPU__
#endif // _AK_DELAYFXPARAMS_H_
