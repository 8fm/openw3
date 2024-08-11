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

//////////////////////////////////////////////////////////////////////
//
// AkFDNReverbFX.h
//
// FDN Reverb implementation.
//
// Copyright 2007 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#ifndef _AK_FDNREVERBFX_H_
#define _AK_FDNREVERBFX_H_

#include "AkFDNReverbFXParams.h"
#include <AK/Plugin/PluginServices/AkFXTailHandler.h>
#include <AK/SoundEngine/Common/AkSimd.h>

#define MAXNUMDELAYGROUPS (MAXNUMDELAYS/4)

//-----------------------------------------------------------------------------
// Name: class CAkFDNReverbFX
//-----------------------------------------------------------------------------
class CAkFDNReverbFX : public AK::IAkInPlaceEffectPlugin
{
public:

    // Constructor/destructor
    CAkFDNReverbFX();
    ~CAkFDNReverbFX();

	// Allocate memory needed by effect and other initializations
    AKRESULT Init(	AK::IAkPluginMemAlloc *	in_pAllocator,		// Memory allocator interface.
					AK::IAkEffectPluginContext * in_pFXCtx,		// FX Context
                    AK::IAkPluginParam * in_pParams,			// Effect parameters.
                    AkAudioFormat &	in_rFormat					// Required audio input format.
					);
    
	// Free memory used by effect and effect termination
	AKRESULT Term( AK::IAkPluginMemAlloc * in_pAllocator );

	// Reset
	AKRESULT Reset( );

    // Effect type query.
    AKRESULT GetPluginInfo( AkPluginInfo & out_rPluginInfo );

    // Execute effect processing.
	void Execute( AkAudioBuffer * io_pBuffer );

	// Skips execution of some frames, when the voice is virtual.  Nothing special to do for this effect.
	AKRESULT TimeSkip(AkUInt32 in_uFrames) {return AK_DataReady;}	

private:

	AKRESULT InitDelayLines( AkChannelMask in_uChannelMask );
	void TermDelayLines();

	// Utility functions
	inline void SetDefaultDelayLengths( );
	inline void ComputeIIRLPFCoefs( );
	inline void ComputeFIRLPFCoefs( );
	inline void MakePrimeNumber( AkUInt32 & in_uIn );

	// DSP perform routines without or with delay line modulation
	// Mono processing 
	void ProcessMono4( AkAudioBuffer * io_pBuffer );
	void ProcessMono8( AkAudioBuffer * io_pBuffer );	
	void ProcessMono12( AkAudioBuffer * io_pBuffer );	
	void ProcessMono16( AkAudioBuffer * io_pBuffer );	
	// Stereo processing
	void ProcessStereo4( AkAudioBuffer * io_pBuffer );
	void ProcessStereo8( AkAudioBuffer * io_pBuffer );	
	void ProcessStereo12( AkAudioBuffer * io_pBuffer );	
	void ProcessStereo16( AkAudioBuffer * io_pBuffer );	

#ifdef AK_LFECENTER	
	// 5.0 processing (LFE passthrough)
	void ProcessFivePointZero4( AkAudioBuffer * io_pBuffer );
	void ProcessFivePointZero8( AkAudioBuffer * io_pBuffer );
	void ProcessFivePointZero12( AkAudioBuffer * io_pBuffer );
	void ProcessFivePointZero16( AkAudioBuffer * io_pBuffer );
	// 5.1 processing
	void ProcessFivePointOne4( AkAudioBuffer * io_pBuffer );
	void ProcessFivePointOne8( AkAudioBuffer * io_pBuffer );
	void ProcessFivePointOne12( AkAudioBuffer * io_pBuffer );
	void ProcessFivePointOne16( AkAudioBuffer * io_pBuffer );
#endif

#ifndef AK_AKANDROID		
	// Processing for any number of channels (non-optimal)
	void ProcessN4( AkAudioBuffer * io_pBuffer );
	void ProcessN8( AkAudioBuffer * io_pBuffer );
	void ProcessN12( AkAudioBuffer * io_pBuffer );
	void ProcessN16( AkAudioBuffer * io_pBuffer );
#endif	
	
	// Function ptr to the appropriate DSP execution routine
	void (CAkFDNReverbFX::*m_fpPerformDSP) (AkAudioBuffer * io_pBuffer);
	
	// parameter interface and its local copy
    CAkFDNReverbFXParams *	m_pParams;
	AK::IAkPluginMemAlloc * m_pAllocator;

	// Cached values to avoid recomputations
	AkReal32				m_fCachedReverbTime;
	AkReal32				m_fCachedHFRatio;

	// Pre-delay line
	AkReal32 *				m_pfPreDelayStart;
	AkReal32 *				m_pfPreDelayRW;
	AkReal32 *				m_pfPreDelayEnd;
	AkUInt32				m_uPreDelayLength;
	
	// Tone coloration FIR filter
	AkReal32				m_fFIRLPFB0;
	AkReal32				m_fFIRLPFB1;
	AkReal32				m_fFIRLPFMem;
	
	// Delay information
	AkUInt32				m_uNominalDelayLength[MAXNUMDELAYS];	// Nominal delay length without modulation	
	AkReal32*				m_pfDelayRead[MAXNUMDELAYS];			// Pointer to allocated interleaved delays
	AkReal32*				m_pfDelayStart[MAXNUMDELAYGROUPS];		// Pointer to allocated interleaved delays
	AkReal32*				m_pfDelayWrite[MAXNUMDELAYGROUPS];		// Moving read pointers within interleaved delay lines
	AkReal32*				m_pfDelayEnd[MAXNUMDELAYGROUPS];		// End boundaries for interleaved delay lines
	
	// Feedback path recursive LPF coefficients
	AKSIMD_V4F32			m_vIIRLPFB0[MAXNUMDELAYGROUPS];
	AKSIMD_V4F32			m_vIIRLPFA1[MAXNUMDELAYGROUPS];
	AKSIMD_V4F32			m_vIIRLPFMem[MAXNUMDELAYGROUPS];
	
	// DC Filter
	AkReal32				m_fDCFwdMem;	// first order feedforward
	AkReal32				m_fDCFbkMem;	// first order feedback memories
	AkReal32				m_fDCCoef;		// Feedback coefficient for HP filter
	
	// Interpolation ramps for Wet and Dry levels
	AkReal32				m_fCurrentDry;
	AkReal32				m_fCurrentWet;

	AkFXTailHandler			m_FXTailHandler;	
	
	// Global FX variables
	AkUInt32				m_uSampleRate;
	AkUInt32				m_uTailLength;
	AkUInt32				m_uNumProcessedChannels;
	bool					m_bIsSentMode;
	bool					m_bPrevPreStop;
};

#endif // _AK_FDNREVERBFX_H_