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

#ifndef _AK_TIMESTRETCHPARAMS_H_
#define _AK_TIMESTRETCHPARAMS_H_

#include <AK/Plugin/AkTimeStretchFXFactory.h>

// Parameter IDs 
static const AkPluginParamID AK_TIMESTRETCHFXPARAM_WINDOWSIZE_ID		= 0;
static const AkPluginParamID AK_TIMESTRETCHFXPARAM_TIMSTRETCH_ID		= 1;
static const AkPluginParamID AK_TIMESTRETCHFXPARAM_OUTPUTGAIN_ID		= 2;
static const AkPluginParamID AK_TIMESTRETCHFXPARAM_TIMSTRETCHRANDOM_ID	= 3;

// Time stretch parameters
struct AkTimeStretchFXParams
{
	AkUInt32	uWindowSize;
	AkReal32	fTimeStretch;
	AkReal32	fTimeStretchRandom;
	AkReal32	fOutputGain;

	// Default values
	AkTimeStretchFXParams() : uWindowSize( 2048 ), fTimeStretch(0.f), fTimeStretchRandom(0.f), fOutputGain(1.f) {}
};

#ifndef __SPU__

class CAkTimeStretchFXParams : public AK::IAkPluginParam
{
public:
 
    // Constructor/destructor.
    CAkTimeStretchFXParams( );
    ~CAkTimeStretchFXParams( );
	CAkTimeStretchFXParams( const CAkTimeStretchFXParams & in_rCopy );

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

	// Retrieve all parameters at once
	void GetParams( AkTimeStretchFXParams* out_pParams );
	
private:

    AkTimeStretchFXParams	m_Params;
};

#endif

#endif // _AK_TIMESTRETCHPARAMS_H_
