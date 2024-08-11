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

#ifndef _AK_TIMESTRETCHFX_H_
#define _AK_TIMESTRETCHFX_H_

#include "AkTimeStretchFXParams.h"
#include "AkTimeStretchFXInfo.h"

//-----------------------------------------------------------------------------
// Name: class CAkTimeStretchFX
//-----------------------------------------------------------------------------
class CAkTimeStretchFX : public AK::IAkOutOfPlaceEffectPlugin
{
public:

    // Constructor/destructor
    CAkTimeStretchFX();
    ~CAkTimeStretchFX();

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
	void Execute(	AkAudioBuffer * in_pBuffer,
					AkUInt32		in_uInOffset,
					AkAudioBuffer * out_pBuffer
#ifdef AK_PS3
					, AK::MultiCoreServices::DspProcess*&	out_pDspProcess
#endif	
								);

private:

	void Bypass( 
		AkAudioBuffer * in_pBuffer, 
		AkUInt32		in_uInOffset, 
		AkAudioBuffer * out_pBuffer );

	AKRESULT TimeSkip(AkUInt32 &io_uFrames);

	CAkTimeStretchFXParams *			m_pSharedParams;
	AK::IAkPluginMemAlloc * 			m_pAllocator;
	AkTimeStretchFXInfo					m_FXInfo;
	AkReal32							m_fSkippedFrames;

#ifdef AK_PS3
	AK::MultiCoreServices::DspProcess	m_DspProcess;
#endif
};

#endif // _AK_TIMESTRETCHFX_H_
