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
// AkSrcLpFilter.h
//
// Single Butterworth low pass filter section (IIR). 
// The input control parameter in range (0,100) is non -linearly mapped to cutoff frequency (Hz)
// Assumes same thread will call both SetLPFPar and Execute (not locking)
//
//////////////////////////////////////////////////////////////////////

#ifndef _AK_SRCLPFILTER_H_
#define _AK_SRCLPFILTER_H_

#include "AkInternalLPFState.h"
#include <AK/SoundEngine/Common/AkCommonDefs.h>

//#define PERFORMANCE_BENCHMARK

class CAkSrcLpFilter
{
public:  
	CAkSrcLpFilter();
    virtual ~CAkSrcLpFilter();

    AKRESULT Init( AkChannelMask in_uChannelMask, bool in_bComputeCoefsForFeedback = false );
	void Term();

	AkForceInline void SetLPFPar( AkReal32 in_fTargetLPFPar )
	{
		in_fTargetLPFPar = AkClamp( in_fTargetLPFPar, 0.0f, 100.0f );
		m_InternalLPFState.bTargetDirty |= in_fTargetLPFPar != m_InternalLPFState.fTargetLPFPar;
		m_InternalLPFState.fTargetLPFPar = in_fTargetLPFPar;
	}

	void ResetRamp();

	AkForceInline AkReal32 GetLPFPar() const { return m_InternalLPFState.fTargetLPFPar; }

	AkForceInline bool IsInitialized() { return m_InternalLPFState.m_LPF.IsInitialized(); }

#ifdef AK_PS3
	void ExecutePS3( AkAudioBuffer * io_pBuffer, AKRESULT &io_result );
#else
    void Execute( AkAudioBuffer * io_pBuffer ); 	// Input/output audio buffer 
#endif

#ifndef AK_PS3
private:
#endif
	AkInternalLPFState m_InternalLPFState;	// These only needs to be public on PS3
private:

#ifdef PERFORMANCE_BENCHMARK
	AkUInt32			m_uNumberCalls;
	AkReal32			m_fTotalTime;
#endif
};

#endif  // _AK_SRCLPFILTER_H_
