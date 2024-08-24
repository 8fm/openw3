//////////////////////////////////////////////////////////////////////
//
// AkFXSrcSineParams.h
//
// Sample physical modelling source. Implements a simple sine tone source.
// 
// Note: Target output format is currently determined by the source itself.
// 
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#ifndef _AKFXSRC_SINEPARAMS_H_
#define _AKFXSRC_SINEPARAMS_H_

#include <AK/Plugin/AkSineSourceFactory.h>
#include <AK/Tools/Common/AkAssert.h>
#include <AK/Plugin/PluginServices/AkFXParameterChangeHandler.h>
#include <math.h>

// Parameters IDs.
static const AkPluginParamID AK_SINE_FXPARAM_FREQ_ID			= 0;
static const AkPluginParamID AK_SINE_FXPARAM_GAIN_ID			= 1;
static const AkPluginParamID AK_SINE_FXPARAM_DURATION_ID		= 2;
static const AkPluginParamID AK_SINE_FXPARAM_CHANNELMASK_ID		= 3;
static const AkUInt32		 AK_SINE_NUMFXPARAMS				= 4;

// Parameters struture for this effect.
struct AkSineFXParams
{
    AkReal32		fFrequency;    // Frequency (in Hertz).
    AkReal32		fGain;         // Gain (in dBFS).
    AkReal32		fDuration;		// Sustain duration (only valid if finite).
	AkChannelMask	uChannelMask;	// Output channel mask.
} AK_ALIGN_DMA;

#ifndef __SPU__

class CAkFxSrcSineParams 
	: public AK::IAkPluginParam
	, public AkSineFXParams
{
public:

    // Constructor.
    CAkFxSrcSineParams();
	
	// Copy constructor.
    CAkFxSrcSineParams( const CAkFxSrcSineParams & in_rCopy );

	// Destructor.
    virtual ~CAkFxSrcSineParams();

    // Create parameter object duplicate.
	virtual AK::IAkPluginParam * Clone( AK::IAkPluginMemAlloc * in_pAllocator );

    // Initialization.
    virtual AKRESULT Init( AK::IAkPluginMemAlloc *	in_pAllocator,		// Memory allocator.						    
                   const void *				in_pParamsBlock,	// Pointer to param block.
                   AkUInt32					in_ulBlockSize		// Sise of the parm block.
                   );
   
	// Termination.
	virtual AKRESULT Term( AK::IAkPluginMemAlloc * in_pAllocator );

    // Set all parameters at once.
    virtual AKRESULT SetParamsBlock( const void * in_pParamsBlock, 
                             AkUInt32 in_ulBlockSize
                             );

    // Update one parameter.
    virtual AKRESULT SetParam( AkPluginParamID in_ParamID,
                       const void * in_pValue, 
                       AkUInt32 in_ulParamSize
                       );
public:

	AK::AkFXParameterChangeHandler<AK_SINE_NUMFXPARAMS> m_ParamChangeHandler;
};

#endif // __SPU__
#endif // _AKFXSRC_SINEPARAMS_H_
