//////////////////////////////////////////////////////////////////////
//
// AkFXSrcAudioInputParams.h
//
// Allows for audio source to come from an external input.
// 
// Note: Target output format is currently determined by the source itself.
// 
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#ifndef _AKFXSRC_AUDIOINPUTPARAMS_H_
#define _AKFXSRC_AUDIOINPUTPARAMS_H_

#include <AK/Plugin/AkAudioInputSourceFactory.h>
#include <AK/Tools/Common/AkAssert.h>
#include <math.h>

// Parameters IDs.
const AkPluginParamID AK_SRCAUDIOINPUT_FXPARAM_GAIN_ID		= 1;

// Parameter range values
const AkReal32 INPUT_GAIN_MIN			= -96.3f;	// dB FS
const AkReal32 INPUT_GAIN_MAX			= 0.f;		// db FS

// Parameters struture for this effect.
struct AkFXSrcAudioInputParams
{
	AkReal32     fGain;         // Gain (in dBFS).
};


//-----------------------------------------------------------------------------
// Name: class CAkFXSrcAudioInputParams
// Desc: Sample implementation the audio input source shared parameters.
//-----------------------------------------------------------------------------
class CAkFxSrcAudioInputParams : public AK::IAkPluginParam
{
public:

	// Allow effect to call accessor functions for retrieving parameter values.
	friend class CAkFXSrcAudioInput;

    // Constructor.
    CAkFxSrcAudioInputParams();
	
	// Copy constructor.
    CAkFxSrcAudioInputParams( const CAkFxSrcAudioInputParams & in_rCopy );

	// Destructor.
    ~CAkFxSrcAudioInputParams();

    // Create parameter object duplicate.
	AK::IAkPluginParam * Clone( AK::IAkPluginMemAlloc * in_pAllocator );

    // Initialization.
    AKRESULT Init( AK::IAkPluginMemAlloc *	in_pAllocator,		// Memory allocator.						    
                   const void *				in_pParamsBlock,	// Pointer to param block.
                   AkUInt32					in_ulBlockSize		// Sise of the parm block.
                   );
   
	// Termination.
	AKRESULT Term( AK::IAkPluginMemAlloc * in_pAllocator );

    // Set all parameters at once.
    AKRESULT SetParamsBlock( const void * in_pParamsBlock, 
                             AkUInt32 in_ulBlockSize
                             );

    // Update one parameter.
    AKRESULT SetParam( AkPluginParamID in_ParamID,
                       const void * in_pValue, 
                       AkUInt32 in_ulParamSize
                       );

private:

    AkUInt32	GetInput();
    AkReal32	GetGain();

private:

    // Parameter structure.
    AkFXSrcAudioInputParams	m_Params;
};

// Safely retrieve gain.
inline AkReal32 CAkFxSrcAudioInputParams::GetGain( )
{
    AkReal32 fGain = m_Params.fGain;
	AKASSERT( fGain >= INPUT_GAIN_MIN && fGain <= INPUT_GAIN_MAX );
	fGain = powf( 10.f, ( fGain / 20.f ) ); // Make it a linear value	
    return fGain;
}

#endif // _AKFXSRC_AUDIOINPUTPARAMS_H_
