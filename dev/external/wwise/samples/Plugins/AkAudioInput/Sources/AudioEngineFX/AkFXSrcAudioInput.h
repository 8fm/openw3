//////////////////////////////////////////////////////////////////////
//
// AkFXSrcAudioInput.h
//
// Allow the game to set data for a source.
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#ifndef _AKFXSRC_AUDIOINPUT_H_
#define _AKFXSRC_AUDIOINPUT_H_

#include "AkFXSrcAudioInputParams.h"
#include <AK/Plugin/PluginServices/AkValueRamp.h>

//-----------------------------------------------------------------------------
// Name: class CAkFXSrcAudioInput
// Desc: Demo implementation of audio input
//-----------------------------------------------------------------------------
class CAkFXSrcAudioInput : public AK::IAkSourcePlugin
{
public:

    // Constructor.
    CAkFXSrcAudioInput();

	// Destructor.
    ~CAkFXSrcAudioInput();

    // Initialize.
    AKRESULT Init(	AK::IAkPluginMemAlloc *			in_pAllocator,		// Memory allocator interface.
					AK::IAkSourcePluginContext *	in_pSourceFXContext,// Source FX context
                    AK::IAkPluginParam *			in_pParams,			// Effect parameters.
                    AkAudioFormat &					io_rFormat			// Supported audio output format.
                    );

    // Effect termination.
    AKRESULT Term( AK::IAkPluginMemAlloc * in_pAllocator );

	// Reset.
	AKRESULT Reset( );

    // Effect info query.
    AKRESULT GetPluginInfo( AkPluginInfo & out_rPluginInfo );

    // Execute effect processing.
	void Execute(	AkAudioBuffer *							io_pBuffer
#ifdef AK_PS3	
					, AK::MultiCoreServices::DspProcess*&	out_pDspProcess
#endif					
					);

	AkReal32 GetDuration( ) const {return 0;}

	virtual AKRESULT StopLooping();

    ////////////////////////////////////////////////////////////////////////////////////////////
    // API external to the plug-in, to be used by the game.

    // This function should be called at the same place the AudioInput plug-in is being registered.
    static void SetAudioInputCallbacks(
                    AkAudioInputPluginExecuteCallbackFunc in_pfnExecCallback, 
                    AkAudioInputPluginGetFormatCallbackFunc in_pfnGetFormatCallback = NULL,
                    AkAudioInputPluginGetGainCallbackFunc in_pfnGetGainCallback = NULL
                    );
    ////////////////////////////////////////////////////////////////////////////////////////////

private:

    AkReal32 GetGain();

    // Shared parameters structure
    CAkFxSrcAudioInputParams * m_pSharedParams;

	// Source FX context interface
	AK::IAkSourcePluginContext * m_pSourceFXContext;

	// Gain ramp interpolator
	AK::CAkValueRamp	m_GainRamp;

    AkAudioFormat m_Format;

    static AkAudioInputPluginExecuteCallbackFunc m_pfnExecCallback;
    static AkAudioInputPluginGetFormatCallbackFunc m_pfnGetFormatCallback;
    static AkAudioInputPluginGetGainCallbackFunc m_pfnGetGainCallback;
};

#endif // _AKFXSRC_AUDIOINPUT_H_
