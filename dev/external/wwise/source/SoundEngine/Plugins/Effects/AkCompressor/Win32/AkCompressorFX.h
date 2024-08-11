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

#ifndef _AK_COMPRESSORFX_H_
#define _AK_COMPRESSORFX_H_

#include "AkCompressorFXParams.h"

struct AkCompressorSideChain
{
	AkReal32 fGainDb;
	AkReal32 fMem;
};

class CAkCompressorFX : public AK::IAkInPlaceEffectPlugin
{
public:

    // Constructor/destructor
    CAkCompressorFX();
    ~CAkCompressorFX();

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
    void Execute( AkAudioBuffer * io_pBuffer );

	// Skips execution of some frames, when the voice is virtual.  Nothing special to do for this effect.
	AKRESULT TimeSkip(AkUInt32 in_uFrames) {return AK_DataReady;}	

private:

	void Process( AkAudioBuffer * io_pBufferIn, AkCompressorFXParams * in_pParams );
	void ProcessLinked( AkAudioBuffer * io_pBufferIn, AkCompressorFXParams * in_pParams );

private:

	// Shared parameter interface
    CAkCompressorFXParams * m_pSharedParams;

	// Function ptr to the appropriate DSP execution routine
	void (CAkCompressorFX::*m_fpPerformDSP)  ( AkAudioBuffer * io_pBufferIn, AkCompressorFXParams * in_pParams );

	AkReal32	m_fCurrentGain;			// Current gain value	
	
	// Audio format information
	AkUInt32	m_uNumChannels;
	AkUInt32	m_uSampleRate;
	
	// Side chain
	AkUInt32	m_uNumSideChain;
	AkReal32	m_fRMSFilterCoef;			// RMS filter coefficient (same for all sidechains)
	AkCompressorSideChain *	m_pSideChain;	// RMS evaluation sidechain

	// Cached values for optimization
	AkReal32 m_fCachedAttack;
	AkReal32 m_fCachedAttackCoef;
	AkReal32 m_fCachedRelease;
	AkReal32 m_fCachedReleaseCoef;

	bool 	m_bProcessLFE;				// LFE behavior
};

#endif // _AK_COMPRESSORFX_H_