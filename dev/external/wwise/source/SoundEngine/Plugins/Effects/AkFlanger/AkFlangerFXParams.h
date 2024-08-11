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

#ifndef _AK_FLANGERFXPARAMS_H_
#define _AK_FLANGERFXPARAMS_H_

#include <AK/SoundEngine/Common/AkTypes.h>
#include <AK/Plugin/AkFlangerFXFactory.h>
#include "LFO.h"

struct AkFlangerRTPCParams
{
	// Color Controls
	AkReal32		fDryLevel;
	AkReal32		fFfwdLevel;
	AkReal32		fFbackLevel;
	// LFO Controls
	AkReal32		fModDepth;
	DSP::LFO::MultiChannel::AllParams modParams;
	// Output levels
	AkReal32		fOutputLevel;
	AkReal32		fWetDryMix;

	// Dirty flag
	bool			bHasChanged;
	void SetDirty( bool in_bDirty )
	{
		bHasChanged = in_bDirty;
	}
};

struct AkFlangerNonRTPCParams
{
	AkReal32	fDelayTime;
	bool		bEnableLFO;
	bool		bProcessCenter;
	bool		bProcessLFE;

	// Dirty flag
	bool		bHasChanged;
	void SetDirty( bool in_bDirty )
	{
		bHasChanged = in_bDirty;
	}
};

struct AkFlangerFXParams
{
	AkFlangerRTPCParams	   RTPC;
	AkFlangerNonRTPCParams NonRTPC;
} AK_ALIGN_DMA;

#ifndef __SPU__

#include <math.h>
#include <AK/Tools/Common/AkAssert.h>

// Parameters IDs for Wwise or RTPC.
// Global params
static const AkPluginParamID AK_FLANGERFXPARAM_DELAYTIME_ID			= 0;

// Color params
static const AkPluginParamID AK_FLANGERFXPARAM_DRYLEVEL_ID			= 5;
static const AkPluginParamID AK_FLANGERFXPARAM_FFWDLEVEL_ID			= 6;
static const AkPluginParamID AK_FLANGERFXPARAM_FBACKLEVEL_ID		= 7;

// LFO params
static const AkPluginParamID AK_FLANGERFXPARAM_MODWAVEFORM_ID		= 		8;
static const AkPluginParamID AK_FLANGERFXPARAM_MODFREQUENCY_ID		= 		9;
static const AkPluginParamID AK_FLANGERFXPARAM_MODSMOOTHING_ID		= 		18;
static const AkPluginParamID AK_FLANGERFXPARAM_MODPWM_ID			= 		19;
static const AkPluginParamID AK_FLANGERFXPARAM_MODPHASEOFFSET_ID	= 		20;
static const AkPluginParamID AK_FLANGERFXPARAM_MODPHASEMODE_ID		= 		21;
static const AkPluginParamID AK_FLANGERFXPARAM_MODPHASESPREAD_ID	= 		22;

static const AkPluginParamID AK_FLANGERFXPARAM_MODDEPTH_ID				= 		10;
static const AkPluginParamID AK_FLANGERFXPARAM_ENABLELFO_ID				= 		14;

// Output params
static const AkPluginParamID AK_FLANGERFXPARAM_OUTPUTLEVEL_ID		= 16;	
static const AkPluginParamID AK_FLANGERFXPARAM_WETDRYMIX_ID			= 1;	
static const AkPluginParamID AK_FLANGERFXPARAM_PROCESSCENTER_ID		= 17;	
static const AkPluginParamID AK_FLANGERFXPARAM_PROCESSLFE_ID		= 2;	
	
// Parameters ID defaults for Wwise or RTPC.
// Global params
static const AkReal32 AK_FLANGERFXPARAM_DELAYTIME_DEF				= 		3.f;

// Color params
static const AkReal32 AK_FLANGERFXPARAM_DRYLEVEL_DEF				= 		0.0f;
static const AkReal32 AK_FLANGERFXPARAM_FFWDLEVEL_DEF				= 		0.0f;
static const AkReal32 AK_FLANGERFXPARAM_FBACKLEVEL_DEF				= 		0.0f;

// LFO params
static const DSP::LFO::Waveform AK_FLANGERFXPARAM_MODWAVEFORM_DEF	= 		DSP::LFO::WAVEFORM_SINE;
static const AkReal32 AK_FLANGERFXPARAM_MODFREQUENCY_DEF			= 		1.0f;
static const AkReal32 AK_FLANGERFXPARAM_MODPHASEOFFSET_DEF			= 		0.0f;
static const AkReal32 AK_FLANGERFXPARAM_MODSMOOTH_DEF				= 		0.0f;
static const AkReal32 AK_FLANGERFXPARAM_MODPWM_DEF					= 		0.5f;
static const DSP::LFO::MultiChannel::PhaseMode AK_FLANGERFXPARAM_MODPHASEMODE_DEF	= DSP::LFO::MultiChannel::PHASE_MODE_LEFT_RIGHT;
static const AkReal32 AK_FLANGERFXPARAM_MODPHASESPREAD_DEF			= 		0.0f;

static const AkReal32 AK_FLANGERFXPARAM_MODDEPTH_DEF				= 		0.0f;
static const bool AK_FLANGERFXPARAM_ENABLELFO_DEF					= 		true;

// Output params
static const AkReal32 AK_FLANGERFXPARAM_WETDRYMIX_DEF				= 		100.0f;
static const AkReal32 AK_FLANGERFXPARAM_OUTPUTLEVEL_DEF				= 		1.0f;	
static const bool	  AK_FLANGERFXPARAM_PROCESSCENTER_DEF			=		false;
static const bool	  AK_FLANGERFXPARAM_PROCESSLFE_DEF				=		false;

	
//-----------------------------------------------------------------------------
// Name: class CAkFlangerFXParams
// Desc: Shared Flanger FX parameters implementation.
//-----------------------------------------------------------------------------
class CAkFlangerFXParams : public AK::IAkPluginParam
{
public:

	friend class CAkFlangerFX;
    
    // Constructor/destructor.
    CAkFlangerFXParams( );
    ~CAkFlangerFXParams();
	CAkFlangerFXParams( const CAkFlangerFXParams & in_rCopy );

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

private:

	// Get all parameters
	inline void GetParams(AkFlangerFXParams* in_pParams)
	{
		*in_pParams = m_Params;
		SetDirty( false ); // Once parameter are read by the FX, the computations will be in sync with the parameters
	}

	void SetDirty( bool in_bDirty );

    // Parameter blob.
    AkFlangerFXParams m_Params;
};

#endif // __SPU__

#endif // _AK_FLANGERFXPARAMS_H_
