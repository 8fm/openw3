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

#ifndef _AK_HARMONIZERPARAMS_H_
#define _AK_HARMONIZERPARAMS_H_

#include <AK/Plugin/AkHarmonizerFXFactory.h>
#include <AK/Plugin/PluginServices/AkFXParameterChangeHandler.h>
#include <math.h>

#define AKHARMONIZER_NUMVOICES (2)
#define AK_PITCH2FACTOR( __Pitch__ ) (powf(2.f, (__Pitch__) * 8.33333334e-4f ))

// Parameter IDs bindings
enum AkHarmonizerID
{
	AKHARMONIZERPARAMID_INPUT = 0,
	AKHARMONIZERPARAMID_PROCESSLFE,
	AKHARMONIZERPARAMID_SYNCDRY,
	AKHARMONIZERPARAMID_DRYLEVEL,
	AKHARMONIZERPARAMID_WETLEVEL, 
	AKHARMONIZERPARAMID_WINDOWSIZE,
	AKHARMONIZERPARAMID_VOICE1ENABLE,
	AKHARMONIZERPARAMID_VOICE1PITCH,
	AKHARMONIZERPARAMID_VOICE1GAIN,
	AKHARMONIZERPARAMID_VOICE1FILTERTYPE,
	AKHARMONIZERPARAMID_VOICE1FILTERGAIN,
	AKHARMONIZERPARAMID_VOICE1FILTERFREQUENCY,
	AKHARMONIZERPARAMID_VOICE1FILTERQFACTOR,
	AKHARMONIZERPARAMID_VOICE2ENABLE,
	AKHARMONIZERPARAMID_VOICE2PITCH,
	AKHARMONIZERPARAMID_VOICE2GAIN,
	AKHARMONIZERPARAMID_VOICE2FILTERTYPE,
	AKHARMONIZERPARAMID_VOICE2FILTERGAIN,
	AKHARMONIZERPARAMID_VOICE2FILTERFREQUENCY,
	AKHARMONIZERPARAMID_VOICE2FILTERQFACTOR,
	AKHARMONIZERPARAMID_NUM_PARAMS	// KEEP LAST
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
	AKINPUTTYPE_5POINT0,
	AKINPUTTYPE_LEFTONLY
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
	AkReal32			fGain;
	bool				bEnable;

	AkPitchVoiceParams() 
	{
		fPitchFactor = 1.f;
		fGain = 1.f;
		bEnable = false;
	}
};

struct AkHarmonizerFXParams
{
	AkPitchVoiceParams Voice[AKHARMONIZER_NUMVOICES];
	AkInputType eInputType;
	AkReal32	fDryLevel;
	AkReal32	fWetLevel;
	AkUInt32	uWindowSize;
	bool		bProcessLFE;
	bool		bSyncDry;

	AkHarmonizerFXParams() 
	{
		eInputType = AKINPUTTYPE_ASINPUT;
		fDryLevel =  1.f;
		fWetLevel = 1.f;
		uWindowSize = 1024;
		bProcessLFE = false;
		bSyncDry = false;
	}
};


#ifndef __SPU__

class CAkHarmonizerFXParams : public AK::IAkPluginParam
{
public:
 
    // Constructor/destructor.
    CAkHarmonizerFXParams( );
    ~CAkHarmonizerFXParams( );
	CAkHarmonizerFXParams( const CAkHarmonizerFXParams & in_rCopy );

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
	void GetParams( AkHarmonizerFXParams* out_pParams );

	AK::AkFXParameterChangeHandler<AKHARMONIZERPARAMID_NUM_PARAMS> m_ParamChangeHandler;
	
private:

    AkHarmonizerFXParams	m_Params;
};

#endif

#endif // _AK_HARMONIZERPARAMS_H_
