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

#ifndef _AK_TREMOLOFXPARAMS_H_
#define _AK_TREMOLOFXPARAMS_H_

#include <AK/SoundEngine/Common/AkTypes.h>
#include <AK/Plugin/AkTremoloFXFactory.h>
#include "LFO.h"

struct AkTremoloRTPCParams
{
	// Modulation.
	AkReal32		fModDepth;
	DSP::LFO::MultiChannel::AllParams modParams;
	
	// Output
	AkReal32		fOutputGain;

	// Dirty flag
	bool			bHasChanged;
	void SetDirty( bool in_bDirty )
	{
		bHasChanged = in_bDirty;
	}
};

struct AkTremoloNonRTPCParams
{
	bool		bProcessCenter;
	bool		bProcessLFE;

	// Dirty flag
	bool		bHasChanged;
	void SetDirty( bool in_bDirty )
	{
		bHasChanged = in_bDirty;
	}
};

struct AkTremoloFXParams
{
	AkTremoloRTPCParams	   RTPC;
	AkTremoloNonRTPCParams NonRTPC;
} AK_ALIGN_DMA;

#ifndef __SPU__

#include <math.h>
#include <AK/Tools/Common/AkAssert.h>

// Parameters IDs for Wwise or RTPC.

// LFO params

static const AkPluginParamID AK_TREMOLOFXPARAM_MODDEPTH_ID				= 		1;
static const AkPluginParamID AK_TREMOLOFXPARAM_MODFREQUENCY_ID			= 		2;
static const AkPluginParamID AK_TREMOLOFXPARAM_MODWAVEFORM_ID			= 		3;
static const AkPluginParamID AK_TREMOLOFXPARAM_MODSMOOTH_ID				= 		4;
static const AkPluginParamID AK_TREMOLOFXPARAM_MODPWM_ID				= 		5;
static const AkPluginParamID AK_TREMOLOFXPARAM_MODPHASEOFFSET_ID		= 		6;
static const AkPluginParamID AK_TREMOLOFXPARAM_MODPHASEMODE_ID			= 		7;
static const AkPluginParamID AK_TREMOLOFXPARAM_MODPHASESPREAD_ID		= 		8;

// Output params
static const AkPluginParamID AK_TREMOLOFXPARAM_OUTPUTLEVEL_ID		= 		9;	
static const AkPluginParamID AK_TREMOLOFXPARAM_PROCESSCENTER_ID		= 		10;	
static const AkPluginParamID AK_TREMOLOFXPARAM_PROCESSLFE_ID		= 		11;
	
// Parameters ID defaults for Wwise or RTPC.

// LFO params
static const DSP::LFO::Waveform AK_TREMOLOFXPARAM_MODWAVEFORM_DEF	= 		DSP::LFO::WAVEFORM_SINE;
static const AkReal32 AK_TREMOLOFXPARAM_MODFREQUENCY_DEF			= 		1.0f;
static const AkReal32 AK_TREMOLOFXPARAM_MODPHASEOFFSET_DEF			= 		0.0f;
static const AkReal32 AK_TREMOLOFXPARAM_MODSMOOTH_DEF				= 		0.0f;
static const AkReal32 AK_TREMOLOFXPARAM_MODPWM_DEF					= 		0.5f;
static const DSP::LFO::MultiChannel::PhaseMode AK_TREMOLOFXPARAM_MODPHASEMODE_DEF	= DSP::LFO::MultiChannel::PHASE_MODE_LEFT_RIGHT;
static const AkReal32 AK_TREMOLOFXPARAM_MODPHASESPREAD_DEF			= 		0.0f;

static const AkReal32 AK_TREMOLOFXPARAM_MODDEPTH_DEF				= 		1.0f;

// Output params
static const AkReal32 AK_TREMOLOFXPARAM_OUTPUTLEVEL_DEF				= 		1.0f;	
static const bool	  AK_TREMOLOFXPARAM_PROCESSCENTER_DEF			=		true;
static const bool	  AK_TREMOLOFXPARAM_PROCESSLFE_DEF				=		true;
	
//-----------------------------------------------------------------------------
// Name: class CAkTremoloFXParams
// Desc: Shared Tremolo FX parameters implementation.
//-----------------------------------------------------------------------------
class CAkTremoloFXParams : public AK::IAkPluginParam
{
public:

	friend class CAkTremoloFX;
    
    // Constructor/destructor.
    CAkTremoloFXParams();
    ~CAkTremoloFXParams();
	CAkTremoloFXParams( const CAkTremoloFXParams & in_rCopy );

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
	inline void GetParams(AkTremoloFXParams* in_pParams)
	{
		*in_pParams = m_Params;
		SetDirty( false ); // Once parameter are read by the FX, the computations will be in sync with the parameters
	}

	void SetDirty( bool in_bDirty );

    // Parameter blob.
    AkTremoloFXParams m_Params;
};

#endif // __SPU__

#endif // _AK_TREMOLOFXPARAMS_H_
