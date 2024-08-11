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

#ifndef _AK_PITCHSHIFTERPARAMS_H_
#define _AK_PITCHSHIFTERPARAMS_H_

#include <AK/Plugin/AkPitchShifterFXFactory.h>
#include <AK/Plugin/PluginServices/AkFXParameterChangeHandler.h>
#include <math.h>

#define AK_PITCH2FACTOR( __Pitch__ ) (powf(2.f, (__Pitch__) * 8.33333334e-4f )) // /1200.f

// Parameter IDs bindings
enum AkPitchShifterParamID
{
	AKPITCHSHIFTERPARAMID_INPUT = 0,
	AKPITCHSHIFTERPARAMID_PROCESSLFE,
	AKPITCHSHIFTERPARAMID_SYNCDRY,
	AKPITCHSHIFTERPARAMID_DRYLEVEL,
	AKPITCHSHIFTERPARAMID_WETLEVEL, 
	AKPITCHSHIFTERPARAMID_DELAYTIME,
	AKPITCHSHIFTERPARAMID_PITCH,
	AKPITCHSHIFTERPARAMID_FILTERTYPE,
	AKPITCHSHIFTERPARAMID_FILTERGAIN,
	AKPITCHSHIFTERPARAMID_FILTERFREQUENCY,
	AKPITCHSHIFTERPARAMID_FILTERQFACTOR,
	AKPITCHSHIFTERPARAMID_NUM_PARAMS	// KEEP LAST
};

// Same order as DSP:BiquadFilter::FilterType (offset by none option)
enum AkFilterType
{
	AKFILTERTYPE_NONE		= 0,
	AKFILTERTYPE_LOWSHELF,	
	AKFILTERTYPE_PEAKINGEQ,
	AKFILTERTYPE_HIGHSHELF,
	AKFILTERTYPE_LOWPASS,
	AKFILTERTYPE_HIGHPASS,
	AKFILTERTYPE_BANDPASS,
	AKFILTERTYPE_NOTCH
};

enum AkInputType
{
	AKINPUTTYPE_ASINPUT =  0,		// Process all channels
	AKINPUTTYPE_CENTER,				// Treat center in both sides
	AKINPUTTYPE_STEREO,				// Only process front L,R
	AKINPUTCHANNELTYPE_3POINT0,		
	AKINPUTTYPE_4POINT0,			
	AKINPUTTYPE_5POINT0
};

struct AkVoiceFilterParams
{
	AkFilterType		eFilterType;
	AkReal32			fFilterGain;
	AkReal32			fFilterFrequency;
	AkReal32			fFilterQFactor;

	AkVoiceFilterParams()
	{
		eFilterType = AKFILTERTYPE_NONE; 
		fFilterGain = 1.f; 
		fFilterFrequency = 1000.f;
		fFilterQFactor = 1.f;
	}
};

struct AkPitchVoiceParams
{
	AkVoiceFilterParams	Filter;
	AkReal32			fPitchFactor;

	AkPitchVoiceParams() 
	{
		fPitchFactor = 1.f;
	}
};

struct AkPitchShifterFXParams
{
	AkPitchVoiceParams Voice;
	AkInputType eInputType;
	AkReal32	fDryLevel;
	AkReal32	fWetLevel;
	AkReal32	fDelayTime;
	bool		bProcessLFE;
	bool		bSyncDry;

	AkPitchShifterFXParams() 
	{
		eInputType = AKINPUTTYPE_ASINPUT;
		fDryLevel =  1.f;
		fWetLevel = 1.f;
		fDelayTime = 50.f;
		bProcessLFE = false;
		bSyncDry = false;
	}
};

#ifndef __SPU__

class CAkPitchShifterFXParams : public AK::IAkPluginParam
{
public:
 
    // Constructor/destructor.
    CAkPitchShifterFXParams( );
    ~CAkPitchShifterFXParams( );
	CAkPitchShifterFXParams( const CAkPitchShifterFXParams & in_rCopy );

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
	void GetParams( AkPitchShifterFXParams* out_pParams );

	AK::AkFXParameterChangeHandler<AKPITCHSHIFTERPARAMID_NUM_PARAMS> m_ParamChangeHandler;
	
private:

    AkPitchShifterFXParams	m_Params;
};

#endif

#endif // _AK_PITCHSHIFTERPARAMS_H_
