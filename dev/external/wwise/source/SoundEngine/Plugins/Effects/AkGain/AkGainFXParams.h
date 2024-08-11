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
// AkGainFXParams.h
//
// Shared parameter implementation for Gain FX.
//
// Copyright 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#ifndef _AK_GAINPARAMS_H_
#define _AK_GAINPARAMS_H_

#include <AK/Plugin/AkGainFXFactory.h>
#include <math.h>
#include <AK/Tools/Common/AkAssert.h>

// Parameters IDs for the Wwise or RTPC.
static const AkPluginParamID AK_GAINFXPARAM_FULLBANDGAIN_ID	= 0;	
static const AkPluginParamID AK_GAINFXPARAM_LFEGAIN_ID		= 1;

//-----------------------------------------------------------------------------
// Structures.
//-----------------------------------------------------------------------------

#define GAIN_FULLBAND_DEF			(0.f)
#define GAIN_LFE_DEF				(0.f)

// Structure of Gain parameters
struct AkGainFXParams
{
	AkReal32		fFullbandGain;
	AkReal32		fLFEGain;
};

//-----------------------------------------------------------------------------
// Name: class CAkGainFXParams
// Desc: Shared Gain FX parameters implementation.
//-----------------------------------------------------------------------------
class CAkGainFXParams : public AK::IAkPluginParam
{
public:

	friend class CAkGainFX;
 
    // Constructor/destructor.
    CAkGainFXParams( );
    ~CAkGainFXParams( );
	CAkGainFXParams( const CAkGainFXParams & in_rCopy );

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

	//// Internal API functions ////
	AkReal32 GetFullbandGain( );
	AkReal32 GetLFEGain( );

private:

    AkGainFXParams			m_Params;	// Parameter structure.
};

// GetFullbandGain
inline AkReal32 CAkGainFXParams::GetFullbandGain( )
{
	AkReal32 fOutputLevel = m_Params.fFullbandGain;
	fOutputLevel = powf( 10.f, ( fOutputLevel * 0.05f ) );
	return fOutputLevel;
}

// GetLFEGain
inline AkReal32 CAkGainFXParams::GetLFEGain( )
{
	AkReal32 fOutputLevel = m_Params.fLFEGain;
	fOutputLevel = powf( 10.f, ( fOutputLevel * 0.05f ) );
	return fOutputLevel;
}

#endif // _AK_GAINPARAMS_H_
