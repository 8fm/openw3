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
// AkSrcLpFilter.cpp
//
// Single Butterworth low pass filter section (IIR). 
// The input control parameter in range (0,100) is non -linearly mapped to cutoff frequency (Hz)
// Assumes same thread will call both SetLPFPar and Execute (not locking)
// There is some interpolation done on the LPF control parameter do avoid stepping behavior in transition
// or fast parameter changes.
// 
// We can think of latency / stepping problem by looking at rates at which different thinks occur:
// Control rate: rate at which SetLPFPar are called -> 1x per buffer (linked with buffer time)
// Interpolation rate: rate at which transitional LPFParam values are changed ( coefficient recalculation needed)
// -> NUMBLOCKTOREACHTARGET per buffer, necessary to avoid stepping while introduces up to 1 buffer latency
// Audio rate: rate at which samples are calculated == sample rate
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h" 
#include "AkSrcLpFilter.h"
#include "AkLPFCommon.h"
#include <AK/Tools/Common/AkPlatformFuncs.h>
#include "AkSettings.h"
#ifdef PERFORMANCE_BENCHMARK
#include "AkMonitor.h"
#endif

#include "AkCommon.h"

#ifdef AK_PS3

#include "AkLEngine.h"

extern char _binary_SrcLpFilter_spu_bin_start[];
extern char _binary_SrcLpFilter_spu_bin_size[];

static AK::MultiCoreServices::BinData SrcLpFilterJobsBin =
	{ _binary_SrcLpFilter_spu_bin_start, CELL_SPURS_GET_SIZE_BINARY( _binary_SrcLpFilter_spu_bin_size ) };

#endif


CAkSrcLpFilter::CAkSrcLpFilter()
{
	m_InternalLPFState.fCurrentLPFPar = 0.f;
	m_InternalLPFState.fTargetLPFPar = 0.f;
}

CAkSrcLpFilter::~CAkSrcLpFilter()
{
}

AKRESULT CAkSrcLpFilter::Init( AkChannelMask in_uChannelMask, bool in_bComputeCoefsForFeedback )
{
	// Note: for the moment, the LFE channel is a a full band channel treated no differently than others
	// This means that what really matters here is only the number of channel and not the input channel configuration
	// The number set here may be offset by half the DSP function table size to get the corresponding interpolating routine

	m_InternalLPFState.bComputeCoefsForFeedback = in_bComputeCoefsForFeedback;
	m_InternalLPFState.uNumInterBlocks = NUMBLOCKTOREACHTARGET;
	m_InternalLPFState.uChannelMask = (AkUInt16) in_uChannelMask;
	m_InternalLPFState.bBypassFilter = true;
	m_InternalLPFState.bTargetDirty = true;
	m_InternalLPFState.bFirstSetLPF = true;

#ifdef PERFORMANCE_BENCHMARK
	m_fTotalTime = 0.f;
	m_uNumberCalls = 0;
#endif 

	return m_InternalLPFState.m_LPF.Init(AK::GetNumChannels(in_uChannelMask));
}

//-----------------------------------------------------------------------------
// Name: Term
// Desc: Terminates.
//-----------------------------------------------------------------------------
void CAkSrcLpFilter::Term( )
{
	m_InternalLPFState.m_LPF.Term();
#ifdef PERFORMANCE_BENCHMARK
	if ( m_uNumberCalls )
	{
		AkOSChar szString[64];
		AK_OSPRINTF( szString, 64, AKTEXT("%f\n"), m_fTotalTime/m_uNumberCalls );
		AKPLATFORM::OutputDebugMsg( szString );
		MONITOR_MSG( szString );
	}
#endif 
}

void CAkSrcLpFilter::ResetRamp()
{
	m_InternalLPFState.bFirstSetLPF = true;
	m_InternalLPFState.m_LPF.Reset();
}

static inline void _ManageLPFChange( AkInternalLPFState & io_state )
{
	if ( AK_EXPECT_FALSE( io_state.bTargetDirty ) )
	{
		io_state.bTargetDirty = false;

		if ( AK_EXPECT_FALSE( io_state.bFirstSetLPF ) )
		{
			io_state.bFirstSetLPF = false;

			// No interpolation required upon first SetLPF call (current == target)
			io_state.fCurrentLPFPar = io_state.fTargetLPFPar;
			io_state.uNumInterBlocks = NUMBLOCKTOREACHTARGET;
			io_state.bBypassFilter = EvalLPFBypass( io_state.fTargetLPFPar );
			if ( !io_state.bBypassFilter )
			{
				AkReal32 fCutFreq = EvalLPFCutoff( io_state.fTargetLPFPar, io_state.bComputeCoefsForFeedback );
				ComputeLPFCoefs(io_state.m_LPF, fCutFreq);
			}
		}
		else
		{
			// New LPF interpolation is required (set target)
			bool bBypassFilter = EvalLPFBypass( io_state.fCurrentLPFPar ) && EvalLPFBypass( io_state.fTargetLPFPar );
			io_state.bBypassFilter = bBypassFilter;
			io_state.uNumInterBlocks = bBypassFilter ? NUMBLOCKTOREACHTARGET : 0;
		}
	}
}

#ifdef AK_PS3
void CAkSrcLpFilter::ExecutePS3( AkAudioBuffer * io_pBuffer, AKRESULT &io_result )
{
	_ManageLPFChange( m_InternalLPFState );

	// if we're not in bypass mode then start a job
	if ( AK_EXPECT_FALSE( !m_InternalLPFState.bBypassFilter ) )
	{
		// Set PS3 specific information
		AkUInt32 uNumChannels = AK::GetNumChannels( m_InternalLPFState.uChannelMask );
		m_InternalLPFState.uValidFrames = io_pBuffer->uValidFrames;
		m_InternalLPFState.uMaxFrames = io_pBuffer->MaxFrames();

		AK::MultiCoreServices::DspProcess * pDsp = CAkLEngine::GetDspProcess();
		pDsp->ResetDspProcess( true );
		pDsp->SetDspProcess( SrcLpFilterJobsBin );
		// LPF state DMA
		pDsp->AddDspProcessDma( &m_InternalLPFState, sizeof(AkInternalLPFState) );
		// Filter memory DMA
		AkUInt32 uDmaSize = AK_ALIGN_SIZE_FOR_DMA( sizeof(DSP::Memories)*uNumChannels );
		pDsp->AddDspProcessDma( &m_InternalLPFState.m_LPF.GetMemories(0), uDmaSize );
		// Input data DMA (possibly 2 in multi-channel)
		uDmaSize = AK_ALIGN_SIZE_FOR_DMA(uNumChannels * m_InternalLPFState.uMaxFrames * sizeof(AkReal32));
		pDsp->AddDspProcessDma( io_pBuffer->GetDataStartDMA() , uDmaSize );
		io_result = AK_ProcessNeeded;
	}
}

#else

void CAkSrcLpFilter::Execute( AkAudioBuffer * io_pBuffer )
{
	AKASSERT( io_pBuffer != NULL && io_pBuffer->GetChannel( 0 ) != NULL );
	AKASSERT( io_pBuffer->MaxFrames() != 0 && io_pBuffer->uValidFrames <= io_pBuffer->MaxFrames() );

	_ManageLPFChange( m_InternalLPFState );

#ifdef PERFORMANCE_BENCHMARK
	AkInt64 TimeBefore;
	AKPLATFORM::PerformanceCounter( &TimeBefore ); 
#endif
	// Call appropriate DSP function
	if ( AK_EXPECT_FALSE( !m_InternalLPFState.bBypassFilter ) )
	{
		const AkUInt32 uChannels = io_pBuffer->NumChannels();
		const AkUInt32 uNumFrames = io_pBuffer->uValidFrames;

		if (m_InternalLPFState.IsInterpolating())
		{
			//Interpolation required.
			const AkReal32 fLPFParamStart = m_InternalLPFState.fCurrentLPFPar;
			const AkReal32 fLPFParamDiff = m_InternalLPFState.fTargetLPFPar - fLPFParamStart;
			const bool bBypassFilter = EvalLPFBypass( fLPFParamStart );

			AkUInt32 uFramesProduced = 0;
			if ( bBypassFilter )
			{
				if ( uNumFrames < 2 )
					return;

				for ( AkUInt16 c = 0; c < uChannels; ++c )
				{
					//Reset the memory of the filter to the current signal.  Better than starting from 0.
					AkReal32 * pfBuf = io_pBuffer->GetChannel( c );
					DSP::Memories &rMems = m_InternalLPFState.m_LPF.GetMemories(c);
 					rMems.fFFbk1 = pfBuf[1];
 					rMems.fFFbk2 = pfBuf[0];
 					rMems.fFFwd1 = pfBuf[1];
 					rMems.fFFwd2 = pfBuf[0];
				}

				uFramesProduced = 2;
			}

			while ( uFramesProduced < uNumFrames )
			{
				AkUInt32 uFramesInBlock = AkMin(LPFPARAMUPDATEPERIOD, uNumFrames-uFramesProduced);

				if ( m_InternalLPFState.IsInterpolating() )
				{
					++m_InternalLPFState.uNumInterBlocks;
					m_InternalLPFState.fCurrentLPFPar = fLPFParamStart + (fLPFParamDiff*m_InternalLPFState.uNumInterBlocks)/NUMBLOCKTOREACHTARGET;
					AkReal32 fCutFreq = EvalLPFCutoff( m_InternalLPFState.fCurrentLPFPar, m_InternalLPFState.bComputeCoefsForFeedback );
					ComputeLPFCoefs(m_InternalLPFState.m_LPF, fCutFreq);
				}

				for ( AkUInt32 i = 0; i < uChannels; ++i )
				{
					AkReal32 * AK_RESTRICT pfBuf = io_pBuffer->GetChannel( i ) + uFramesProduced;
					m_InternalLPFState.m_LPF.ProcessBuffer(pfBuf, uFramesInBlock, i);
				}

				uFramesProduced += uFramesInBlock;
			}

			if ( !m_InternalLPFState.IsInterpolating() )	
				m_InternalLPFState.bBypassFilter = EvalLPFBypass( m_InternalLPFState.fTargetLPFPar );
		}
		else
		{
			for(AkUInt32 i = 0; i < uChannels; i++)
				m_InternalLPFState.m_LPF.ProcessBuffer(io_pBuffer->GetChannel(i), uNumFrames, i);
		}

	}

#ifdef PERFORMANCE_BENCHMARK
	AkInt64 TimeAfter;
	AKPLATFORM::PerformanceCounter( &TimeAfter );
	AkReal32 fElapsed = AKPLATFORM::Elapsed( TimeAfter, TimeBefore );
	m_fTotalTime += fElapsed;
	++m_uNumberCalls;
#endif
}
#endif
