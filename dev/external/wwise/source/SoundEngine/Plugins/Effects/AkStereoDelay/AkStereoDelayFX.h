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

#ifndef _AK_STEREODELAYFX_H_
#define _AK_STEREODELAYFX_H_

#include "AkStereoDelayFXParams.h"
#include "AkStereoDelayFXInfo.h"

//-----------------------------------------------------------------------------
// Name: class CAkStereoDelayFX
//-----------------------------------------------------------------------------
class CAkStereoDelayFX : public AK::IAkInPlaceEffectPlugin
{
public:

    // Constructor/destructor
    CAkStereoDelayFX();
    ~CAkStereoDelayFX();

	// Allocate memory needed by effect and other initializations
    AKRESULT Init(	AK::IAkPluginMemAlloc *	in_pAllocator,		// Memory allocator interface.
					AK::IAkEffectPluginContext * in_pFXCtx,		// FX Context
				    AK::IAkPluginParam * in_pParams,			// Effect parameters.
                    AkAudioFormat &	in_rFormat					// Required audio input format.
				);
    
	// Free memory used by effect and effect termination
	AKRESULT Term( AK::IAkPluginMemAlloc * in_pAllocator );

	// Reset or seek to start (looping).
	AKRESULT Reset( );

    // Effect type query.
    AKRESULT GetPluginInfo( AkPluginInfo & out_rPluginInfo );

    // Execute effect processing. 
	void Execute(	AkAudioBuffer * io_pBuffer
#ifdef AK_PS3
					, AK::MultiCoreServices::DspProcess*&	out_pDspProcess
#endif	
								);

	// Skips execution of some frames, when the voice is virtual.  Nothing special to do for this effect.
	AKRESULT TimeSkip(AkUInt32 in_uFrames) {return AK_DataReady;}	

private:

	void ComputeTailLength( );
	void SanitizeParams( AkStereoDelayFXParams & io_FXParams );

	CAkStereoDelayFXParams *	m_pParams;
	AK::IAkPluginMemAlloc * 	m_pAllocator;
	AkStereoDelayFXInfo			m_FXInfo;

#ifdef AK_PS3
	AK::MultiCoreServices::DspProcess	m_DspProcess;
#endif
};

#endif // _AK_STEREODELAYFX_H_