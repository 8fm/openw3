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

#include "AkResamplingPhaseVocoder.h"
#include <AK/Tools/Common/AkAssert.h>
#ifdef __SPU__
#include <AK/Plugin/PluginServices/PS3/SPUServices.h>
#endif

namespace DSP
{

namespace BUTTERFLYSET_NAMESPACE
{

#ifndef __SPU__
	void CAkResamplingPhaseVocoder::ResetInputFill( )
	{
		for ( AkUInt32 i = 0; i < m_uNumChannels; i++ )
		{
			m_ResamplingInputAccumBuf[i].ForceFullBuffer( );
		}
	}

	AKRESULT CAkResamplingPhaseVocoder::Init(	
		AK::IAkPluginMemAlloc *	in_pAllocator,	
		AkUInt32				in_uNumChannels,
		AkUInt32				in_uSampleRate,
		AkUInt32				in_uFFTSize,
		bool					in_bUseInputBuffer /* = false */
		)
	{
		CAkPhaseVocoder::Init( in_pAllocator, in_uNumChannels, in_uSampleRate, in_uFFTSize, in_bUseInputBuffer );
		for ( AkUInt32 i = 0; i < m_uNumChannels; i++ )
		{	
			AKRESULT eResult = m_ResamplingInputAccumBuf[i].Init( in_pAllocator, m_uFFTSize+m_uFFTSize/4 );
			if ( eResult != AK_Success )
					return eResult;
		}
		return AK_Success;
	}

	void CAkResamplingPhaseVocoder::Term( AK::IAkPluginMemAlloc * in_pAllocator )
	{
		for ( AkUInt32 i = 0; i < m_uNumChannels; i++ )
			m_ResamplingInputAccumBuf[i].Term( in_pAllocator );
		CAkPhaseVocoder::Term( in_pAllocator );
	}

	void CAkResamplingPhaseVocoder::Reset( )
	{
		CAkPhaseVocoder::Reset( );
		for ( AkUInt32 i = 0; i < m_uNumChannels; i++ )
			m_ResamplingInputAccumBuf[i].Reset();
	}
#endif // #ifndef __SPU__

#ifndef __PPU__

	// Phase vocoder which resamples the content of the input by a given factor, a time stretch of the inverse factor is applied to get pitch shift
	// Channels are processed sequentially by caller
	AKRESULT CAkResamplingPhaseVocoder::ProcessPitchChannel( 
		AkReal32 *		in_pfInBuf, 
		AkUInt32		in_uNumFrames,
		bool			in_bNoMoreData,
		AkUInt32		i, // Channel index
		AkReal32 *		io_pfOutBuf,
		AkReal32		in_fResamplingFactor,
		AkReal32 *		in_pfTempStorage
#ifdef __SPU__
		, AkUInt8 *		in_pScratchMem
		, AkUInt32		in_uDMATag
#endif		
		)
	{	
#ifdef __SPU__

#define DMAPull(__str__, __LSAddr__, __PPUAddr__, __Size__ ) AkDmaLargeGet( __str__, __LSAddr__, __PPUAddr__, __Size__, in_uDMATag, 0, 0 ); AkDmaWait(1<<in_uDMATag); 
#define DMAPush(__str__, __LSAddr__, __PPUAddr__, __Size__ ) AkDmaLargePut( __str__, __LSAddr__, __PPUAddr__, __Size__, in_uDMATag, 0, 0 ); AkDmaWait(1<<in_uDMATag); 

		// Retrieve data from main memory if necessary (PIC code)
		AkUInt8 * pScratchMem = (AkUInt8 *)in_pScratchMem;
		// Note: Some parts of scratch memory are used for different purposes to reduce scratch memory requirements
		// Possible configurations of scratch mem are as follows:
		// Storage always available
			// TD window function
			AkReal32 * pfWindowFunction = (AkReal32 *)pScratchMem;
			AkUInt32 uTDWindowFuncSize;
			AkReal32 * pfPPUWindowFunc = m_TimeWindow.Get( &uTDWindowFuncSize );
			pScratchMem += uTDWindowFuncSize;
			// Input circular buffer (one channel at a time)
			AkReal32 * pfInputCircularBuffer = (AkReal32 *)pScratchMem;
			AkUInt32 uInputAccumBufSize;
			m_ResamplingInputAccumBuf[0].Get( &uInputAccumBufSize );
			pScratchMem += uInputAccumBufSize; 
			// Output circular buffer (one channel at a time)
			AkReal32 * pfOutputCircularBuffer = (AkReal32 *)pScratchMem;
			AkUInt32 uOutputAccumBufSize;
			m_OLAOutCircBuf[0].Get( &uOutputAccumBufSize );
			pScratchMem += uOutputAccumBufSize; 
		// Storage reused between 2 scenarios:
			// Note: PPU addresses are computed on the fly for variable scenarios
			AkUInt8 * pVariableStorageStart = pScratchMem;
		// (FFT/IFFT storage config
			// FFT state
			ak_fftr_state * pFFTState = (ak_fftr_state *)pScratchMem;
			AKASSERT( m_uFFTSpaceRequirements == m_uIFFTSpaceRequirements );
			pScratchMem += m_uFFTSpaceRequirements;
			// Freq win storage (prev or cur or vocoder)
			ak_fft_cpx * pfComputedFreqWinBuffer = (ak_fft_cpx *)pScratchMem;
			// Note: All freq windows have the same sizes
			AkUInt32 uFreqWindowBufSize;
			m_FreqWindow[0][0].Get( &uFreqWindowBufSize );
			pScratchMem += uFreqWindowBufSize;
		pScratchMem = pVariableStorageStart;
		// OR
		// Vocoder storage config
			// Freq win storage (prev)
			AkPolar * pfPrevFreqWinBuffer = (AkPolar *)pScratchMem;
			pScratchMem += uFreqWindowBufSize;
			// Freq win storage (cur)
			AkPolar * pfCurFreqWinBuffer = (AkPolar *)pScratchMem;
			pScratchMem += uFreqWindowBufSize;
			// Freq win storage (vocoder)
			AkPolar * pfVocoderFreqWinBuffer = (AkPolar *)pScratchMem;
			pScratchMem += uFreqWindowBufSize;
			// Freq win storage phases
			PhaseProcessingType * pfPrevSynthPhases = (PhaseProcessingType *)pScratchMem;
			AkUInt32 uPreviousPhaseBufferSize = AK_ALIGN_SIZE_FOR_DMA( ((m_uFFTSize/2)+1)*sizeof(PhaseProcessingType) );
			pScratchMem += uPreviousPhaseBufferSize;
		
		// Pull time domain window that will be used accross all channels (analysis and synthesis)
			DMAPull("PV::WindowFunc",pfWindowFunction, (std::uint64_t)pfPPUWindowFunc, uTDWindowFuncSize );
#endif // #ifdef __SPU__

		AkReal32 * pfTDWindowStorage = in_pfTempStorage;

		AKRESULT eState = AK_DataReady;
		const AkUInt32 uFFTSize = m_uFFTSize;
		AKASSERT( uFFTSize % PV_OVERLAP_FACTOR == 0 );
		const AkUInt32 uHopSize = uFFTSize / PV_OVERLAP_FACTOR;

		// Compute overall system gain and compensate for windowing induced gain
		// Window factor = WindowSize / WindowCummulativeSum
		// Analysis overlap gain = overlap factor / Window factor
		// Compensation gain = 1.f / Analysis overlap gain 
		const AkReal32 fAnalysisGain = (m_TimeWindow.GetCummulativeSum() * PV_OVERLAP_FACTOR) / (AkReal32)m_uFFTSize;
		const AkReal32 fCompGain = 1.f/fAnalysisGain;
		const AkReal32 fTSFactor = 1.f/in_fResamplingFactor;

#ifdef __SPU__
			// Retrieve data from main memory if necessary (PIC code)
			// pull input circular buffer
			AkReal32 * pfPPUInputAccumBuf = m_ResamplingInputAccumBuf[i].Get();
			DMAPull("PV::InputCircBuf",pfInputCircularBuffer, (std::uint64_t)pfPPUInputAccumBuf, uInputAccumBufSize );
			// pull output circular buffer
			AkReal32 * pfPPUOutputAccumBuf = m_OLAOutCircBuf[i].Get();
			DMAPull("PV::OutputCircBuf",pfOutputCircularBuffer, (std::uint64_t)pfPPUOutputAccumBuf, uOutputAccumBufSize );
#endif

			AkUInt32 uInputValidFrames = in_uNumFrames;
			AkUInt32 uOutputValidFrames = 0;
			AkReal32 fInterpPos = m_fInterpPos;
			AkUInt32 uInputFramesToDiscard = m_uInputFramesToDiscard;
			AkUInt32 uInOffset = 0;
			bool bInitPhases = m_bInitPhases; 

			AkReal32 * AK_RESTRICT pfInBuf = (AkReal32 * AK_RESTRICT) in_pfInBuf; 
			AkReal32 * AK_RESTRICT pfOutBuf = (AkReal32 * AK_RESTRICT) io_pfOutBuf;

			AkUInt32 uMasterBreaker = 0;
			
			while ( (uInputValidFrames > 0) || (uOutputValidFrames < in_uNumFrames) )
			{
				// Safety mechanism to avoid infinite loops in case things go horribly wrong. Should never happen.
				if ( uMasterBreaker >= 100 )
				{
					AKASSERT( false && "AkHarmonizer: Infinite loop condition detected." );
					uMasterBreaker = 0xFFFFFFFF;
					break;
				}
				uMasterBreaker++;

				// If there are frames remaining to discard, the input buffer should be empty
				AKASSERT( uInputFramesToDiscard == 0 || m_ResamplingInputAccumBuf[i].FramesReady() == 0 );
				AkUInt32 uFramesDiscarded = AkMin( uInputValidFrames, uInputFramesToDiscard );
				uInputFramesToDiscard -= uFramesDiscarded;
				uInputValidFrames -= uFramesDiscarded;
				uInOffset += uFramesDiscarded;
				AKASSERT( uInputFramesToDiscard == 0 );

				// Once we reached the position, we can start filling the accumulation buffer with enough data for FFTs
				AkUInt32 uFramesConsumed = 0;

#ifdef __SPU__
				// Catch any TDWindow func, and input buffer pending DMAs
				AkDmaWait(1<<in_uDMATag); 
				if ( pfInBuf != NULL && uInputValidFrames > 0 )
					uFramesConsumed = m_ResamplingInputAccumBuf[i].PushFrames( pfInBuf+uInOffset, uInputValidFrames, pfInputCircularBuffer, in_fResamplingFactor );
#else
				if ( pfInBuf != NULL && uInputValidFrames > 0 )
					uFramesConsumed = m_ResamplingInputAccumBuf[i].PushFrames( pfInBuf+uInOffset, uInputValidFrames, in_fResamplingFactor );
#endif
				uInputValidFrames -= (AkUInt16)uFramesConsumed;	
				uInOffset += uFramesConsumed;

				// Compute spectrum of previous FFT frame
				if ( !m_FreqWindow[FREQWIN_PREV][i].IsReady() )
				{
#ifdef __SPU__
					bool bSuccess = m_ResamplingInputAccumBuf[i].ReadFrameBlock( pfTDWindowStorage, uFFTSize, in_bNoMoreData, pfInputCircularBuffer );
					if ( bSuccess )
					{
						AkUInt32 uFrameAdvance = m_ResamplingInputAccumBuf[i].AdvanceFrames( uHopSize );
						AKASSERT( in_bNoMoreData || uFrameAdvance == uHopSize );

						// pull FFT state				
						DMAPull("PV::PrevFreqFFTState",pFFTState, (std::uint64_t)m_pFFTState, m_uFFTSpaceRequirements );
						// pull previous frequency window
						ak_fft_cpx * pPPUPrevFFTFreqWinBuf =  m_FreqWindow[FREQWIN_PREV][i].Get( );
						DMAPull("PV::PrevFreqWin",pfComputedFreqWinBuffer, (std::uint64_t)pPPUPrevFFTFreqWinBuf, uFreqWindowBufSize );
						AkDmaWait(1<<in_uDMATag); 
						pFFTState->FixPointersToLS();

						m_TimeWindow.Apply( pfTDWindowStorage, uFFTSize, pfWindowFunction );
						m_FreqWindow[FREQWIN_PREV][i].Compute( pfTDWindowStorage, uFFTSize, pFFTState, pfComputedFreqWinBuffer );
						m_FreqWindow[FREQWIN_PREV][i].CartToPol( pfComputedFreqWinBuffer );

						DMAPush("PV::PrevFreqWin",pfComputedFreqWinBuffer, (std::uint64_t)pPPUPrevFFTFreqWinBuf, uFreqWindowBufSize );
						AkDmaWait(1<<in_uDMATag); 
					}
#else
					bool bSuccess = m_ResamplingInputAccumBuf[i].ReadFrameBlock( pfTDWindowStorage, uFFTSize, in_bNoMoreData );
					if ( bSuccess )
					{
						AkUInt32 uFrameAdvance = m_ResamplingInputAccumBuf[i].AdvanceFrames( uHopSize );
						AKASSERT( in_bNoMoreData || uFrameAdvance == uHopSize );
						m_TimeWindow.Apply( pfTDWindowStorage, uFFTSize );
						m_FreqWindow[FREQWIN_PREV][i].Compute( pfTDWindowStorage, uFFTSize, m_pFFTState );
						m_FreqWindow[FREQWIN_PREV][i].CartToPol();
					}
#endif
				}

				// Compute spectrum of next FFT frame
				if ( !m_FreqWindow[FREQWIN_CUR][i].IsReady() )
				{
#ifdef __SPU__
					bool bSuccess = m_ResamplingInputAccumBuf[i].ReadFrameBlock( pfTDWindowStorage, uFFTSize, in_bNoMoreData, pfInputCircularBuffer );
					if ( bSuccess )
					{
						AkUInt32 uFrameAdvance = m_ResamplingInputAccumBuf[i].AdvanceFrames( uHopSize );
						AKASSERT( in_bNoMoreData || uFrameAdvance == uHopSize );

						// pull FFT state				
						DMAPull("PV::CurFreqFFTState",pFFTState, (std::uint64_t)m_pFFTState, m_uFFTSpaceRequirements );
						// pull current frequency window
						ak_fft_cpx * pPPUCurFFTFreqWinBuf = m_FreqWindow[FREQWIN_CUR][i].Get( );
						DMAPull("PV::CurFreqWin",pfComputedFreqWinBuffer, (std::uint64_t)pPPUCurFFTFreqWinBuf, uFreqWindowBufSize );
						AkDmaWait(1<<in_uDMATag); 
						pFFTState->FixPointersToLS();
	
						m_TimeWindow.Apply( pfTDWindowStorage, uFFTSize, pfWindowFunction );
						m_FreqWindow[FREQWIN_CUR][i].Compute( pfTDWindowStorage, uFFTSize, pFFTState, pfComputedFreqWinBuffer );
						m_FreqWindow[FREQWIN_CUR][i].CartToPol( pfComputedFreqWinBuffer );

						DMAPush("PV::CurFreqWin",pfComputedFreqWinBuffer, (std::uint64_t)pPPUCurFFTFreqWinBuf, uFreqWindowBufSize );
						AkDmaWait(1<<in_uDMATag); 
					}
#else
					bool bSuccess = m_ResamplingInputAccumBuf[i].ReadFrameBlock( pfTDWindowStorage, uFFTSize, in_bNoMoreData);
					if ( bSuccess )
					{
						AkUInt32 uFrameAdvance = m_ResamplingInputAccumBuf[i].AdvanceFrames( uHopSize );
						AKASSERT( in_bNoMoreData || uFrameAdvance == uHopSize );
						m_TimeWindow.Apply( pfTDWindowStorage, uFFTSize );
						m_FreqWindow[FREQWIN_CUR][i].Compute( pfTDWindowStorage, uFFTSize, m_pFFTState );
						m_FreqWindow[FREQWIN_CUR][i].CartToPol();				
					}
#endif
				}

				// Compute interpolated spectrum window for this synthesis frame
				if ( m_FreqWindow[FREQWIN_CUR][i].IsReady() && 
					 m_FreqWindow[FREQWIN_PREV][i].IsReady() && 
					 !m_VocoderWindow[i].IsReady() )
				{
#ifdef __SPU__
					// pull previous frequency window
					ak_fft_cpx * pPPUPrevFFTFreqWinBuf =  m_FreqWindow[FREQWIN_PREV][i].Get( );
					DMAPull("PV::VocodePrevFreqWin",pfPrevFreqWinBuffer, (std::uint64_t)pPPUPrevFFTFreqWinBuf, uFreqWindowBufSize );
					// pull current frequency window
					ak_fft_cpx * pPPUCurFFTFreqWinBuf = m_FreqWindow[FREQWIN_CUR][i].Get( );
					DMAPull("PV::VocodeCurFreqWin",pfCurFreqWinBuffer, (std::uint64_t)pPPUCurFFTFreqWinBuf, uFreqWindowBufSize );
					// pull vocoder frequency window
					ak_fft_cpx * pPPUVocoderFreqWinBuf = m_VocoderWindow[i].Get( );
					DMAPull("PV::VocodeFreqWin",pfVocoderFreqWinBuffer, (std::uint64_t)pPPUVocoderFreqWinBuf, uFreqWindowBufSize );
					// pull previous synthesis phases
					DMAPull("PV::PrevSynthPhases",pfPrevSynthPhases, (std::uint64_t)m_pfPrevSynthesisPhase[i], uPreviousPhaseBufferSize );	
					AkDmaWait(1<<in_uDMATag); 

					m_VocoderWindow[i].ComputeVocoderSpectrum( 
						pfPrevFreqWinBuffer, 
						pfCurFreqWinBuffer,
						pfPrevSynthPhases,
						uHopSize,
						fInterpPos,
						bInitPhases,
						pfVocoderFreqWinBuffer );

					// push vocoder frequency window
					DMAPush("PV::VocodeCurFreqWin",pfVocoderFreqWinBuffer, (std::uint64_t)pPPUVocoderFreqWinBuf, uFreqWindowBufSize );
					// push previous synthesis phases
					DMAPush("PV::PrevSynthPhases",pfPrevSynthPhases, (std::uint64_t)m_pfPrevSynthesisPhase[i], uPreviousPhaseBufferSize );	
					AkDmaWait(1<<in_uDMATag); 
#else
					m_VocoderWindow[i].ComputeVocoderSpectrum( 
						(AkPolar *)m_FreqWindow[FREQWIN_PREV][i].Get(), 
						(AkPolar *)m_FreqWindow[FREQWIN_CUR][i].Get(),
						m_pfPrevSynthesisPhase[i],
						uHopSize,
						fInterpPos,
						bInitPhases );
#endif
					bInitPhases = false;
				}

				// Output to OLA buffer when space is available and advance time
				if ( m_VocoderWindow[i].IsReady() &&
					m_OLAOutCircBuf[i].FramesEmpty() >= uFFTSize )
				{
#ifdef __SPU__
					// pull IFFT state				
					DMAPull("PV::IFFState",pFFTState, (std::uint64_t)m_pIFFTState, m_uIFFTSpaceRequirements );
					// pull vocoder frequency window
					ak_fft_cpx * pPPUVocoderFreqWinBuf = m_VocoderWindow[i].Get( );
					DMAPull("PV::SynthVocodeFreqWin",pfComputedFreqWinBuffer, (std::uint64_t)pPPUVocoderFreqWinBuf, uFreqWindowBufSize );
					AkDmaWait(1<<in_uDMATag); 
					pFFTState->FixPointersToLS();

					// IFFT
					m_VocoderWindow[i].ConvertToTimeDomain( pfTDWindowStorage, uFFTSize, pFFTState, pfComputedFreqWinBuffer );
					// Apply synthesis window
					m_TimeWindow.Apply( pfTDWindowStorage, uFFTSize, fCompGain, pfWindowFunction );
					// OLA
					m_OLAOutCircBuf[i].PushOverlappedWindow( pfTDWindowStorage, uHopSize, pfOutputCircularBuffer );	
#else
					// IFFT
					m_VocoderWindow[i].ConvertToTimeDomain( pfTDWindowStorage, uFFTSize, m_pIFFTState );
					// Apply synthesis window
					m_TimeWindow.Apply( pfTDWindowStorage, uFFTSize, fCompGain );
					// OLA
					m_OLAOutCircBuf[i].PushOverlappedWindow( pfTDWindowStorage, uHopSize );	
#endif
									
					m_VocoderWindow[i].SetReady( false ); // Even with same 2 spectral window, a new phase vocoded window is necessary

					fInterpPos += fTSFactor;
					// Advance spectral windows if necessary
					if ( fInterpPos >= 1.f )
					{
						m_FreqWindow[FREQWIN_PREV][i].SetReady( false ); 
						AkReal32 fInterpPosIntegerPart = floor(fInterpPos);
						AkUInt32 uNumHopSkip = (AkUInt32) fInterpPosIntegerPart;
						if ( uNumHopSkip >= 2 )
						{
							m_FreqWindow[FREQWIN_CUR][i].SetReady( false ); // Time compression can skip such that both windows are invalidated
							// Time compression may require skipping over some data: 
							// 1) If already in input buffer, simply flush these frames
							// 2) Set the remainder of what needs to be skipped (but not yet cached in input buffer)
							// so that it can be discarded when new input arrives
							AkUInt32 uFramesToFlushFromInput = (uNumHopSkip-2)*uHopSize;
							AkUInt32 uFramesSkipped = m_ResamplingInputAccumBuf[i].AdvanceFrames( uFramesToFlushFromInput );
							AKASSERT( uInputFramesToDiscard == 0 );
							uInputFramesToDiscard = uFramesToFlushFromInput - uFramesSkipped; 
						}
						else
							m_uFreqWindowIndex[i]++;	// Past == Current, switch the 2 windows
						
						fInterpPos -= fInterpPosIntegerPart;
					}	
					AKASSERT( fInterpPos >= 0.f && fInterpPos < 1.f );
				}

				// Consume what is available from the OLA buffer to put in pipeline's output buffer
				bool bFlushingOLABuffer =	(in_bNoMoreData &&
											m_ResamplingInputAccumBuf[i].FramesReady() == 0 &&
											!m_VocoderWindow[i].IsReady() &&
											m_OLAOutCircBuf[i].FramesReady() == 0 );

				AkUInt32 uFramesProduced = m_OLAOutCircBuf[i].PopFrames( 
					pfOutBuf+uOutputValidFrames, 
					in_uNumFrames - uOutputValidFrames,
					bFlushingOLABuffer 
#ifdef __SPU__
					, pfOutputCircularBuffer
#endif
					);
				uOutputValidFrames += (AkUInt16)uFramesProduced;

				if ( bFlushingOLABuffer && m_OLAOutCircBuf[i].IsDoneTail() )
					eState = AK_NoMoreData;

			} // While

#ifdef __SPU__
			// Put back data to main memory if necessary (PIC code)
			// push input circular buffer
			DMAPush("PV::InCircBuf",pfInputCircularBuffer, (std::uint64_t)pfPPUInputAccumBuf, uInputAccumBufSize );
			// push output circular buffer
			DMAPush("PV::OutCircBuf",pfOutputCircularBuffer, (std::uint64_t)pfPPUOutputAccumBuf, uOutputAccumBufSize );
			AkDmaWait(1<<in_uDMATag); 
#endif

			// Post processing
			if ( i == (m_uNumChannels-1) )
			{
				m_fInterpPos = fInterpPos;
				m_uInputFramesToDiscard = uInputFramesToDiscard;
				m_bInitPhases = bInitPhases;
			}

#ifdef __SPU__
		AkDmaWait(1<<in_uDMATag); 
#endif

		return eState;
	}

#endif // #ifndef __PPU__

#ifdef AK_PS3
	AkUInt32 CAkResamplingPhaseVocoder::GetScratchMemRequirement( )
	{
		// Resampling input circular buffer (one channel at a time)
		AkUInt32 uResamplingInputAccumBufSize;
		m_ResamplingInputAccumBuf[0].Get( &uResamplingInputAccumBufSize );
		AkUInt32 uScratchMemRequired = uResamplingInputAccumBufSize;
		uScratchMemRequired += CAkPhaseVocoder::GetScratchMemRequirement();
		return uScratchMemRequired;
	}

#endif // #ifdef AK_PS3

} // namespace BUTTERFLYSET_NAMESPACE

} // namespace DSP
