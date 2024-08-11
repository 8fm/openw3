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

#ifndef _AK_STEREODELAYPARAMS_H_
#define _AK_STEREODELAYPARAMS_H_

#include <AK/Plugin/AkGuitarDistortionFXFactory.h>
#include <AK/Plugin/PluginServices/AkFXParameterChangeHandler.h>

// Parameter IDs bindings
enum AkStereoDelayParamID
{
	AKSTEREODELAYPARAMID_ENABLECROSSFEED = 0,
	AKSTEREODELAYPARAMID_ENABLEFEEDBACK,
	AKSTEREODELAYPARAMID_DRYLEVEL,
	AKSTEREODELAYPARAMID_WETLEVEL, 
	AKSTEREODELAYPARAMID_FRONTREARBALANCE,
	AKSTEREODELAYPARAMID_FILTERTYPE,
	AKSTEREODELAYPARAMID_FILTERGAIN,
	AKSTEREODELAYPARAMID_FILTERFREQUENCY,
	AKSTEREODELAYPARAMID_FILTERQFACTOR,
	AKSTEREODELAYPARAMID_LEFTINPUTTYPE,
	AKSTEREODELAYPARAMID_LEFTDELAYTIME,
	AKSTEREODELAYPARAMID_LEFTFEEDBACK,
	AKSTEREODELAYPARAMID_LEFTCROSSFEED,
	AKSTEREODELAYPARAMID_RIGHTINPUTTYPE,
	AKSTEREODELAYPARAMID_RIGHTDELAYTIME,
	AKSTEREODELAYPARAMID_RIGHTFEEDBACK,
	AKSTEREODELAYPARAMID_RIGHTCROSSFEED,
	AKSTEREODELAYPARAMID_NUM_PARAMS	// KEEP LAST
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

// Channel selection/downmix option for stereo delay line input 
enum AkInputChannelType
{
	AKINPUTCHANNELTYPE_LEFT_OR_RIGHT =  0,	// Left or right
	AKINPUTCHANNELTYPE_CENTER,				// Use only center channel
	AKINPUTCHANNELTYPE_DOWNMIX,				// Downmix left or right with center
	AKINPUTCHANNELTYPE_NONE,				// Silence input
};

struct AkStereoDelayChannelParams
{
	AkReal32			fDelayTime;
	AkReal32			fFeedback;
	AkReal32			fCrossFeed;

	AkStereoDelayChannelParams() 
	{
		fDelayTime = 0.5f;
		fFeedback = 0.25f;
		fCrossFeed = 0.25f;
	}
};

struct AkStereoDelayFilterParams
{
	AkFilterType		eFilterType;
	AkReal32			fFilterGain;
	AkReal32			fFilterFrequency;
	AkReal32			fFilterQFactor;

	AkStereoDelayFilterParams()
	{
		eFilterType = AKFILTERTYPE_NONE; 
		fFilterGain = 1.f; 
		fFilterFrequency = 1000.f;
		fFilterQFactor = 1.f;
	}
};

struct AkStereoDelayFXParams
{
	AkStereoDelayChannelParams StereoDelayParams[2];
	AkInputChannelType	eInputType[2];
	AkStereoDelayFilterParams FilterParams;
	AkReal32			fDryLevel;
	AkReal32			fWetLevel;
	AkReal32			fFrontRearBalance;
	bool				bEnableFeedback;
	bool				bEnableCrossFeed;

	AkStereoDelayFXParams() 
	{
		eInputType[0] = eInputType[1] = AKINPUTCHANNELTYPE_LEFT_OR_RIGHT;
		fDryLevel =  1.f;
		fWetLevel = 1.f;
		fFrontRearBalance = 0.f;
		bEnableFeedback = false;
		bEnableCrossFeed = false;
	}
};

#ifndef __SPU__

class CAkStereoDelayFXParams : public AK::IAkPluginParam
{
public:
 
    // Constructor/destructor.
    CAkStereoDelayFXParams( );
    ~CAkStereoDelayFXParams( );
	CAkStereoDelayFXParams( const CAkStereoDelayFXParams & in_rCopy );

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
	void GetParams( AkStereoDelayFXParams* out_pParams );

	AK::AkFXParameterChangeHandler<AKSTEREODELAYPARAMID_NUM_PARAMS> m_ParamChangeHandler;
	
private:

    AkStereoDelayFXParams	m_Params;
};

#endif // __SPU__
#endif // _AK_STEREODELAYPARAMS_H_
