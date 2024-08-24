//////////////////////////////////////////////////////////////////////
//
// AkFXSrcMP3.h
//
// Allow the game to set data for a source.
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#ifndef _AKFXSRC_MP3_H_
#define _AKFXSRC_MP3_H_

#include "AkFXSrcMP3Params.h"
#include "AkMP3TagReader.h"
#include <AK/Plugin/PluginServices/AkValueRamp.h>
#include <AK/SoundEngine/Common/IAkStreamMgr.h>

#include <windows.h>
#include <stdio.h>
#include <AK/Tools/Common/AkAssert.h>
#include <mmreg.h>
#include <msacm.h>

//-----------------------------------------------------------------------------
// Name: class CAkFXSrcMP3
// Desc: Demo implementation of audio input
//-----------------------------------------------------------------------------
class CAkFXSrcMP3 : public AK::IAkSourcePlugin
{
public:

    // Constructor.
    CAkFXSrcMP3();

	// Destructor.
    ~CAkFXSrcMP3();

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
	void Execute( 
		AkAudioBuffer *				io_pBuffer		///< In/Out audio buffer data structure (in-place processing)
#ifdef AK_PS3
		, AK::MultiCoreServices::DspProcess*&	out_pDspProcess	///< Asynchronous DSP process utilities on PS3
#endif
		);

	AkReal32 GetDuration() const;

private:

    // Internal state variables.
	AkUInt16 m_uNumChannels;
	AkUInt16 m_uSampleRate;
	AkTimeMs m_msDuration;

    // Shared parameters structure
    CAkFxSrcMP3Params * m_pSharedParams;

	// Source FX context interface
	AK::IAkSourcePluginContext * m_pSourceFXContext;

	HACMSTREAM		m_hMP3Stream;
	ACMSTREAMHEADER m_hdrACM;

	AkInt16 * m_pDstBufPos;
	AkUInt32 m_uDsBufSize;			// Size left to process in ACM destination buffer

	void * m_pStreamBuffer;
	AkUInt8 * m_pStreamBufferPos;
	AkUInt32  m_uStreamBufferSize;	// Size left to process in stream buffer

    AK::IAkAutoStream * m_pStream;

	AkMP3Callback m_funcCallback;
	void * m_pCallbackCookie;
	AkUInt32 m_uFramePos; // current audio frame
};

#endif  _AKFXSRC_MP3_H_
