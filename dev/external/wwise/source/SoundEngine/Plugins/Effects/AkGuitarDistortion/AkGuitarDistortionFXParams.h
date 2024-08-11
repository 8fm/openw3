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

#ifndef _AK_GUITARDISTORTIONPARAMS_H_
#define _AK_GUITARDISTORTIONPARAMS_H_

#include <AK/Plugin/AkGuitarDistortionFXFactory.h>
#include <math.h>
#include <AK/Tools/Common/AkAssert.h>


// Parameter IDs 

// EQ band parameter definitions

#define NUMPREDISTORTIONEQBANDS (3)
#define NUMPOSTDISTORTIONEQBANDS (3)
#define NUMDISTORTIONEQBANDS (NUMPREDISTORTIONEQBANDS + NUMPOSTDISTORTIONEQBANDS)
#define PREEQBANDPARAMETERIDSTART (0)
#define PREEQBANDPARAMETERIDEND (PREEQBANDPARAMETERIDSTART+NUMPREDISTORTIONEQBANDS*10)
#define POSTEQBANDPARAMETERIDSTART (PREEQBANDPARAMETERIDEND)
#define POSTEQBANDPARAMETERIDEND (POSTEQBANDPARAMETERIDSTART+NUMPOSTDISTORTIONEQBANDS*10)
#define DISTORTIONPARAMETERSTARTID (NUMDISTORTIONEQBANDS*10)
static const AkPluginParamID AK_GUITARDISTORTIONFXPARAM_FILTERTYPE_ID	= 0;	
static const AkPluginParamID AK_GUITARDISTORTIONFXPARAM_GAIN_ID			= 1;
static const AkPluginParamID AK_GUITARDISTORTIONFXPARAM_FREQUENCY_ID	= 2;	
static const AkPluginParamID AK_GUITARDISTORTIONFXPARAM_QFACTOR_ID		= 3;
static const AkPluginParamID AK_GUITARDISTORTIONFXPARAM_ONOFF_ID		= 4;

// Other parameters
static const AkPluginParamID AK_GUITARDISTORTIONFXPARAM_DISTORTIONTYPE_ID	= DISTORTIONPARAMETERSTARTID;
static const AkPluginParamID AK_GUITARDISTORTIONFXPARAM_DRIVE_ID			= DISTORTIONPARAMETERSTARTID+1;
static const AkPluginParamID AK_GUITARDISTORTIONFXPARAM_TONE_ID				= DISTORTIONPARAMETERSTARTID+2;
static const AkPluginParamID AK_GUITARDISTORTIONFXPARAM_RECTIFICATION_ID	= DISTORTIONPARAMETERSTARTID+3;
static const AkPluginParamID AK_GUITARDISTORTIONFXPARAM_OUTPUTLEVEL_ID		= DISTORTIONPARAMETERSTARTID+4;
static const AkPluginParamID AK_GUITARDISTORTIONFXPARAM_WETDRYMIX_ID		= DISTORTIONPARAMETERSTARTID+5;

// Same order as DSP:BiquadFilter::FilterType
enum AkFilterType
{
	AKFILTERTYPE_LOWSHELF	=  0,	
	AKFILTERTYPE_PEAKINGEQ	=  1,
	AKFILTERTYPE_HIGHSHELF	=  2,
	AKFILTERTYPE_LOWPASS	=  3,
	AKFILTERTYPE_HIGHPASS	=  4,
	AKFILTERTYPE_BANDPASS	=  5,
	AKFILTERTYPE_NOTCH		=  6
};

// Filter band parameters
struct AkFilterBand
{
	AkFilterType	eFilterType;
	AkReal32		fGain;
	AkReal32		fFrequency;
	AkReal32		fQFactor;
	bool			bOnOff;
	bool			bHasChanged; 

	// Default values
	AkFilterBand() : eFilterType(AKFILTERTYPE_LOWSHELF), fGain(0.f), fFrequency(1000.f), fQFactor(1.f), bOnOff(false), bHasChanged(true){}

	// Used to determine when coefficient computations are necessary
	void SetDirty( bool in_bDirty )
	{
		bHasChanged = in_bDirty;
	}
};

// Type of waveshaping transfer functions used
enum AkDistortionType
{
	AKDISTORTIONTYPE_NONE		=  0,	// No distortion
	AKDISTORTIONTYPE_OVERDRIVE	=  1,	// Linear in most of the range with symmetrical soft clipping
	AKDISTORTIONTYPE_HEAVY		=  2,	// Non-linear and symmetrical hard(er) clipping
	AKDISTORTIONTYPE_FUZZ		=  3,	// Assymetrical (soft+hard) clipping
	AKDISTORTIONTYPE_CLIP		=  4	// Symmetrical hard clipping
};

// Distortion parameters
struct AkDistortionParams
{
	AkDistortionType	eDistortionType;
	AkReal32			fDrive;
	AkReal32			fTone;
	AkReal32			fRectification;
	bool				bHasChanged; 

	// Default values
	AkDistortionParams() : eDistortionType(AKDISTORTIONTYPE_NONE), fDrive(50.f), fTone(50.f), fRectification(0.f), bHasChanged(true){}

	// Used to determine when transfer function computations are necessary
	void SetDirty( bool in_bDirty )
	{
		bHasChanged = in_bDirty;
	}
};

// All guitar distortion parameters
struct AkGuitarDistortionFXParams
{
	AkFilterBand			PreEQ[NUMPREDISTORTIONEQBANDS];
	AkFilterBand			PostEQ[NUMPOSTDISTORTIONEQBANDS];
	AkDistortionParams		Distortion;
	AkReal32				fOutputLevel;
	AkReal32				fWetDryMix;

	AkGuitarDistortionFXParams() 
		: fOutputLevel( 1.f ),
		fWetDryMix( 100.f ) {}
};

#ifndef __SPU__


class CAkGuitarDistortionFXParams : public AK::IAkPluginParam
{
public:
 
    // Constructor/destructor.
    CAkGuitarDistortionFXParams( );
    ~CAkGuitarDistortionFXParams( );
	CAkGuitarDistortionFXParams( const CAkGuitarDistortionFXParams & in_rCopy );

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
	void GetParams( AkGuitarDistortionFXParams* out_pParams );
	
private:

	// Parameters have been read by client
	inline void SetDirty( bool in_bDirty );

    AkGuitarDistortionFXParams	m_Params;
};

#endif

#endif // _AK_GUITARDISTORTIONPARAMS_H_
