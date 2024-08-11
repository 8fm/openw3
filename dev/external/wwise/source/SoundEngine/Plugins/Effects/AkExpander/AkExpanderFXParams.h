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
// AkExpanderFXParams.h
//
// Shared parameter implementation for expander FX.
//
// Copyright 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#ifndef _AK_EXPANDERPARAMS_H_
#define _AK_EXPANDERPARAMS_H_

#include <AK/Plugin/AkExpanderFXFactory.h>

// Structure of ExpanderFX parameters
struct AkExpanderFXParams
{
	AkReal32		fThreshold;
	AkReal32		fRatio;
	AkReal32		fAttack;
	AkReal32		fRelease;
	AkReal32		fOutputLevel;
	bool			bProcessLFE;
	bool			bChannelLink;
} AK_ALIGN_DMA;

#ifndef __SPU__

#include <AK/Tools/Common/AkAssert.h>
// Parameters IDs for the Wwise or RTPC.
static const AkPluginParamID AK_EXPANDERFXPARAM_THRESHOLD_ID		= 0;	
static const AkPluginParamID AK_EXPANDERFXPARAM_RATIO_ID			= 1;
static const AkPluginParamID AK_EXPANDERFXPARAM_ATTACK_ID			= 2;	
static const AkPluginParamID AK_EXPANDERFXPARAM_RELEASE_ID			= 3;
static const AkPluginParamID AK_EXPANDERFXPARAM_GAIN_ID				= 4;
static const AkPluginParamID AK_EXPANDERFXPARAM_PROCESSLFE_ID		= 5;	
static const AkPluginParamID AK_EXPANDERFXPARAM_CHANNELLINK_ID		= 6;	

// Default values in case a bad parameter block is provided
#define AK_EXPANDER_THRESHOLD_DEF		(-30.f)
#define AK_EXPANDER_RATIO_DEF			(4.f)
#define AK_EXPANDER_ATTACK_DEF			(0.1f)
#define AK_EXPANDER_RELEASE_DEF			(0.01f)
#define AK_EXPANDER_GAIN_DEF			(1.f)
#define AK_EXPANDER_PROCESSLFE_DEF		(true)
#define AK_EXPANDER_CHANNELLINK_DEF		(true)

//-----------------------------------------------------------------------------
// Name: class CAkExpanderFXParams
// Desc: Shared dynamics processing FX parameters implementation.
//-----------------------------------------------------------------------------
class CAkExpanderFXParams : public AK::IAkPluginParam
{
public:

	friend class CAkExpanderFX;
    
    // Constructor/destructor.
    CAkExpanderFXParams( );
    ~CAkExpanderFXParams( );
	CAkExpanderFXParams( const CAkExpanderFXParams & in_rCopy );

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

private:

	// Get all parameters at once
	inline void GetParams( AkExpanderFXParams * out_pParams )
	{
		*out_pParams = m_Params;
	}

	// Get all RTPC parameters at once
	inline void GetRTPCParams( AkExpanderFXParams * out_pParams )
	{
		out_pParams->fOutputLevel = m_Params.fOutputLevel;
		out_pParams->fRatio = m_Params.fRatio;
		out_pParams->fAttack = m_Params.fAttack;
		out_pParams->fRelease = m_Params.fRelease;
		out_pParams->fThreshold = m_Params.fThreshold;
	}
private:

	AkExpanderFXParams	m_Params;						// Parameter structure.
};

#endif // #ifndef __SPU__

#endif // _AK_EXPANDERPARAMS_H_
