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

#include "AkPhaseVocoder.h"
#include <AK/Tools/Common/AkAssert.h>
#ifdef __SPU__
#include <AK/Plugin/PluginServices/PS3/SPUServices.h>
#endif

namespace DSP
{

namespace BUTTERFLYSET_NAMESPACE
{

#ifndef __SPU__

	CAkPhaseVocoder::CAkPhaseVocoder() 
		: m_pFFTState(NULL)
		, m_pIFFTState(NULL)
		, m_uNumChannels( 0 )
	{
		for ( AkUInt32 i = 0; i < AK_VOICE_MAX_NUM_CHANNELS; i++ )
		{
			m_pfPrevSynthesisPhase[i] = NULL;
		}
	}

	CAkPhaseVocoder::~CAkPhaseVocoder()
	{

	}

	AKRESULT CAkPhaseVocoder::Init(	
		AK::IAkPluginMemAlloc *	in_pAllocator,	
		AkUInt32				in_uNumChannels,
		AkUInt32				in_uSampleRate,
		AkUInt32				in_uFFTSize,
		bool					in_bUseInputBuffer /* = true */
		)
	{	
		AKASSERT( in_uFFTSize % PV_OVERLAP_FACTOR == 0 );

		m_uFFTSize = in_uFFTSize;
		m_uNumChannels = in_uNumChannels;
		m_uSampleRate = in_uSampleRate;

		// FFT configuration are the same for all channels and thus can be shared
		// Allocate FFT state
		RESOLVEUSEALLBUTTERFLIES( ak_fftr_alloc(m_uFFTSize, 0, NULL, &m_uFFTSpaceRequirements) );
		m_uFFTSpaceRequirements = AK_ALIGN_SIZE_FOR_DMA(m_uFFTSpaceRequirements);
		m_pFFTState = (ak_fftr_state *)AK_PLUGIN_ALLOC( in_pAllocator, m_uFFTSpaceRequirements );
		if ( m_pFFTState == NULL )
			return AK_InsufficientMemory;
		// Allocate iFFT state
		RESOLVEUSEALLBUTTERFLIES( ak_fftr_alloc(m_uFFTSize, 1, NULL, &m_uIFFTSpaceRequirements) );
		m_uIFFTSpaceRequirements = AK_ALIGN_SIZE_FOR_DMA(m_uIFFTSpaceRequirements);
		m_pIFFTState = (ak_fftr_state *)AK_PLUGIN_ALLOC( in_pAllocator, m_uIFFTSpaceRequirements );
		if ( m_pIFFTState == NULL )
			return AK_InsufficientMemory;

		// Configure  FFT and IFFT transforms
		// Note: These routines don't really allocate memory, they use the one allocated above
		RESOLVEUSEALLBUTTERFLIES( ak_fftr_alloc(m_uFFTSize,0,(void*)m_pFFTState,&m_uFFTSpaceRequirements) );
		RESOLVEUSEALLBUTTERFLIES( ak_fftr_alloc(m_uFFTSize,1,(void*)m_pIFFTState,&m_uIFFTSpaceRequirements) );
		AKASSERT( (m_pFFTState != NULL) && (m_pIFFTState != NULL) );

		// Setup windowing function
		AKRESULT eResult = m_TimeWindow.Init( in_pAllocator, m_uFFTSize, DSP::CAkTimeWindow::WINDOWTYPE_HANN, true );
		if ( eResult != AK_Success )
			return eResult;

		// Setup past and current spectral window storage
		for ( AkUInt32 i = 0; i < m_uNumChannels; i++ )
		{
			eResult = m_FreqWindow[0][i].Alloc( in_pAllocator, m_uFFTSize );
			if ( eResult != AK_Success )
				return eResult;
			eResult = m_FreqWindow[1][i].Alloc( in_pAllocator, m_uFFTSize );
			if ( eResult != AK_Success )
				return eResult;
			eResult = m_VocoderWindow[i].Alloc( in_pAllocator, m_uFFTSize );
			if ( eResult != AK_Success )
				return eResult;
			m_pfPrevSynthesisPhase[i] = (PhaseProcessingType *)AK_PLUGIN_ALLOC( in_pAllocator, AK_ALIGN_SIZE_FOR_DMA( ((m_uFFTSize/2)+1)*sizeof(PhaseProcessingType) ) );
			if ( m_pfPrevSynthesisPhase[i] == NULL )
				return AK_InsufficientMemory;
			
			m_uFreqWindowIndex[i] = 0;
		}

		
		// Minimum supported buffering is m_uFFTSize for input and output OLA buffers
		// Additional buffering will give more flexibility to the algorithm to reduce the algorithm logic overhead 
		// On PS3 this may be significant as it will control the number of PPU/SPU roundtrips
		// Setup output circular buffer for overlapped time domain data
		m_bUseInputBuffer = in_bUseInputBuffer;
		for ( AkUInt32 i = 0; i < m_uNumChannels; i++ )
		{	
			if ( in_bUseInputBuffer )
			{
				eResult = m_InputAccumBuf[i].Init( in_pAllocator, m_uFFTSize+m_uFFTSize/4 );
				if ( eResult != AK_Success )
					return eResult;
			}
			eResult = m_OLAOutCircBuf[i].Init( in_pAllocator, m_uFFTSize, m_uFFTSize );
			if ( eResult != AK_Success )
				return eResult;
		}

		return AK_Success;
	}

	void CAkPhaseVocoder::Term( AK::IAkPluginMemAlloc * in_pAllocator )
	{
		if ( m_pFFTState )
		{	
			AK_PLUGIN_FREE( in_pAllocator, m_pFFTState );
			m_pFFTState = NULL;
		}
		if ( m_pIFFTState )
		{	
			AK_PLUGIN_FREE( in_pAllocator, m_pIFFTState );
			m_pIFFTState = NULL;
		}
		m_TimeWindow.Term( in_pAllocator );
		for ( AkUInt32 i = 0; i < AK_VOICE_MAX_NUM_CHANNELS; i++ )
		{
			m_FreqWindow[0][i].Free( in_pAllocator );
			m_FreqWindow[1][i].Free( in_pAllocator );
			m_VocoderWindow[i].Free( in_pAllocator );
			if ( m_bUseInputBuffer )
				m_InputAccumBuf[i].Term( in_pAllocator );
			m_OLAOutCircBuf[i].Term( in_pAllocator );
			if ( m_pfPrevSynthesisPhase[i] )
			{
				AK_PLUGIN_FREE( in_pAllocator, m_pfPrevSynthesisPhase[i] );
				m_pfPrevSynthesisPhase[i] = NULL;
			}
		}
	}

	void CAkPhaseVocoder::Reset( )
	{
		for ( AkUInt32 i = 0; i < m_uNumChannels; i++ )
		{
			m_FreqWindow[0][i].SetReady( false );
			m_FreqWindow[1][i].SetReady( false );
			m_VocoderWindow[i].SetReady( false );
			if ( m_bUseInputBuffer )
				m_InputAccumBuf[i].Reset();
			m_OLAOutCircBuf[i].Reset();
			if ( m_pfPrevSynthesisPhase[i] )
			{
				AkZeroMemLarge( m_pfPrevSynthesisPhase[i], AK_ALIGN_SIZE_FOR_DMA( ((m_uFFTSize/2)+1)*sizeof(PhaseProcessingType) ) );
			}
		}
		m_fInterpPos = 0.f;
		m_uInputFramesToDiscard = 0;
		m_bInitPhases = true;
		m_bInputStartFill = true;
	}

#endif // #ifndef __SPU__

#ifndef __PPU__

	void CAkPhaseVocoder::Execute( 
		AkAudioBuffer * io_pInBuffer, 
		AkUInt32		in_uInOffset,
		AkAudioBuffer * io_pOutBuffer,
		AkReal32		in_fTSFactor,
		bool			in_bEnterNoTSMode,
		AkReal32 *		in_pfTempStorage
#ifdef __SPU__
		, AkUInt8*		in_pScratchMem
		, AkUInt32		in_uDMATag
#endif		
		)
	{
		AKASSERT( m_bUseInputBuffer );
		
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
			m_InputAccumBuf[0].Get( &uInputAccumBufSize );
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
			// pScratchMem += uFreqWindowBufSize;
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

		if ( in_bEnterNoTSMode )
		{
			m_fInterpPos = 0.f;
			m_bInitPhases = true;
		}

		AKASSERT( m_uFFTSize % PV_OVERLAP_FACTOR == 0 );
		const AkUInt32 uHopSize = m_uFFTSize / PV_OVERLAP_FACTOR;
		const AkReal32 fTimeScale = 100.f/in_fTSFactor;

		// Compute overall system gain and compensate for windowing induced gain
		// Window factor = WindowSize / WindowCummulativeSum
		// Analysis overlap gain = overlap factor / Window factor
		// Compensation gain = 1.f / Analysis overlap gain 
		const AkReal32 fAnalysisGain = (m_TimeWindow.GetCummulativeSum() * PV_OVERLAP_FACTOR) / (AkReal32)m_uFFTSize;
		const AkReal32 fCompGain = 1.f/fAnalysisGain;

		const AkUInt32 uFFTSize = m_uFFTSize;
		// Modify those locally for each channel and set them just before returning at the end
		AkUInt32 uInputValidFrames;
		AkUInt32 uOutputValidFrames;
		AkReal32 fInterpPos;
		AkUInt32 uInputFramesToDiscard;
		AkUInt32 uInOffset;
		bool bInitPhases;
		bool bInputStartFill;

		AKASSERT( m_uNumChannels > 0 );

		AkUInt32 i = 0;
		do
		{
#ifdef __SPU__
			// Retrieve data from main memory if necessary (PIC code)
			// pull input circular buffer
			AkReal32 * pfPPUInputAccumBuf = m_InputAccumBuf[i].Get();
			DMAPull("PV::InputCircBuf",pfInputCircularBuffer, (std::uint64_t)pfPPUInputAccumBuf, uInputAccumBufSize );
			// pull output circular buffer
			AkReal32 * pfPPUOutputAccumBuf = m_OLAOutCircBuf[i].Get();
			DMAPull("PV::OutputCircBuf",pfOutputCircularBuffer, (std::uint64_t)pfPPUOutputAccumBuf, uOutputAccumBufSize );
#endif

			// Modify those locally for each channel and set them just before returning at the end
			uInputValidFrames = io_pInBuffer->uValidFrames;
			uOutputValidFrames = io_pOutBuffer->uValidFrames;
			fInterpPos = m_fInterpPos;
			uInputFramesToDiscard = m_uInputFramesToDiscard;
			bInitPhases = m_bInitPhases; 
			uInOffset = in_uInOffset;
			bInputStartFill = m_bInputStartFill;

			AkReal32 * AK_RESTRICT pInBuf = (AkReal32 * AK_RESTRICT) io_pInBuffer->GetChannel( i ); 
			AkReal32 * AK_RESTRICT pfOutBuf = (AkReal32 * AK_RESTRICT) io_pOutBuffer->GetChannel( i );

			while ( true )
			{				
				// If there are frames remaining to discard, the input buffer should be empty
				AKASSERT( uInputFramesToDiscard == 0 || m_InputAccumBuf[i].FramesReady() == 0 );
				AkUInt32 uFramesDiscarded = AkMin( uInputValidFrames, uInputFramesToDiscard );
				uInputFramesToDiscard -= uFramesDiscarded;
				uInputValidFrames -= uFramesDiscarded;
				uInOffset += uFramesDiscarded;
				if ( bInputStartFill && uInputValidFrames == 0 && io_pInBuffer->eState != AK_NoMoreData )
				{
					io_pOutBuffer->eState = AK_DataNeeded;
					break;
				}

				// Once we reached the position, we can start filling the accumulation buffer with enough data for FFTs
#ifdef __SPU__
				// Catch any TDWindow func, and input buffer pending DMAs
				AkDmaWait(1<<in_uDMATag); 
				AkUInt32 uFramesConsumed = m_InputAccumBuf[i].PushFrames( pInBuf+uInOffset, uInputValidFrames, pfInputCircularBuffer );
#else
				AkUInt32 uFramesConsumed = m_InputAccumBuf[i].PushFrames( pInBuf+uInOffset, uInputValidFrames );
#endif
				if ( m_InputAccumBuf[i].FramesEmpty() == 0 )
					bInputStartFill = false;

				uInputValidFrames -= (AkUInt16)uFramesConsumed;	
				uInOffset += uFramesConsumed;
				if ( bInputStartFill && m_InputAccumBuf[i].FramesEmpty() != 0 && uInputValidFrames == 0 && io_pInBuffer->eState != AK_NoMoreData )
				{
					io_pOutBuffer->eState = AK_DataNeeded;
					break;
				}	

				bool bNoMoreInputData = io_pInBuffer->eState == AK_NoMoreData && uInputValidFrames == 0; 
				
				// Compute spectrum of previous FFT frame
				if ( !m_FreqWindow[FREQWIN_PREV][i].IsReady() )
				{
#ifdef __SPU__
					bool bSuccess = m_InputAccumBuf[i].ReadFrameBlock( pfTDWindowStorage, uFFTSize, bNoMoreInputData, pfInputCircularBuffer );
					if ( bSuccess )
					{
						AkUInt32 uFrameAdvance = m_InputAccumBuf[i].AdvanceFrames( uHopSize );
						AKASSERT( bNoMoreInputData || uFrameAdvance == uHopSize );

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
					bool bSuccess = m_InputAccumBuf[i].ReadFrameBlock( pfTDWindowStorage, uFFTSize, bNoMoreInputData );
					if ( bSuccess )
					{
						AkUInt32 uFrameAdvance = m_InputAccumBuf[i].AdvanceFrames( uHopSize );
						AKASSERT( bNoMoreInputData || uFrameAdvance == uHopSize );
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
					bool bSuccess = m_InputAccumBuf[i].ReadFrameBlock( pfTDWindowStorage, uFFTSize, bNoMoreInputData, pfInputCircularBuffer );
					if ( bSuccess )
					{
						AkUInt32 uFrameAdvance = m_InputAccumBuf[i].AdvanceFrames( uHopSize );
						AKASSERT( bNoMoreInputData || uFrameAdvance == uHopSize );

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
					bool bSuccess = m_InputAccumBuf[i].ReadFrameBlock( pfTDWindowStorage, uFFTSize, bNoMoreInputData);
					if ( bSuccess )
					{
						AkUInt32 uFrameAdvance = m_InputAccumBuf[i].AdvanceFrames( uHopSize );
						AKASSERT( bNoMoreInputData || uFrameAdvance == uHopSize );
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

					fInterpPos += fTimeScale;
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
							AkUInt32 uFramesSkipped = m_InputAccumBuf[i].AdvanceFrames( uFramesToFlushFromInput );
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
				// Note: Do we need to consider spectral (prev and next) windows as well?
				bool bFlushingOLABuffer =	(bNoMoreInputData &&
											m_InputAccumBuf[i].FramesReady() == 0 &&
											!m_VocoderWindow[i].IsReady() &&
											m_OLAOutCircBuf[i].FramesReady() == 0 );

				AkUInt32 uFramesProduced = m_OLAOutCircBuf[i].PopFrames( 
					pfOutBuf+uOutputValidFrames, 
					io_pOutBuffer->MaxFrames() - uOutputValidFrames,
					bFlushingOLABuffer 
#ifdef __SPU__
					, pfOutputCircularBuffer
#endif
					);
				uOutputValidFrames += (AkUInt16)uFramesProduced;

				if (	bFlushingOLABuffer &&
						m_OLAOutCircBuf[i].IsDoneTail() )
				{
					io_pOutBuffer->eState = AK_NoMoreData;
					break;
				}
				else if ( uOutputValidFrames == io_pOutBuffer->MaxFrames() ) 
				{
					io_pOutBuffer->eState = AK_DataReady;
					break;
				}
				else if ( !bNoMoreInputData && uInputValidFrames == 0 )
				{
					io_pOutBuffer->eState = AK_DataNeeded;
					break;
				}

			} // While

#ifdef __SPU__
			// Put back data to main memory if necessary (PIC code)
			// push input circular buffer
			DMAPush("PV::InCircBuf",pfInputCircularBuffer, (std::uint64_t)pfPPUInputAccumBuf, uInputAccumBufSize );
			// push output circular buffer
			DMAPush("PV::OutCircBuf",pfOutputCircularBuffer, (std::uint64_t)pfPPUOutputAccumBuf, uOutputAccumBufSize );
			AkDmaWait(1<<in_uDMATag); 
#endif
		} // Channel loop
		while ( ++i < m_uNumChannels );
	
		io_pInBuffer->uValidFrames = (AkUInt16)uInputValidFrames;
		io_pOutBuffer->uValidFrames = (AkUInt16)uOutputValidFrames;
		m_fInterpPos = fInterpPos;
		m_uInputFramesToDiscard = uInputFramesToDiscard;
		m_bInitPhases = bInitPhases;
		m_bInputStartFill = bInputStartFill;

#ifdef __SPU__
		AkDmaWait(1<<in_uDMATag); 
#endif
	}

#endif // #ifndef __PPU__ 

#ifdef AK_PS3

	AkUInt32 CAkPhaseVocoder::GetScratchMemRequirement( )
	{
		// Note: Some parts of scratch memory are used for different purposes to reduce scratch memory requirements
		// Possible configurations of scratch mem are as follows:

		// Storage always available
		// TD window function
		AkUInt32 uTDWindowFuncSize;
		m_TimeWindow.Get( &uTDWindowFuncSize );
		AkUInt32 uScratchMemRequired = uTDWindowFuncSize;
		if ( m_bUseInputBuffer )
		{
			// Input circular buffer (one channel at a time)
			AkUInt32 uInputAccumBufSize;
			m_InputAccumBuf[0].Get( &uInputAccumBufSize );
			uScratchMemRequired += uInputAccumBufSize;
		}
		// Output circular buffer (one channel at a time)
		AkUInt32 uOutputAccumBufSize;
		m_OLAOutCircBuf[0].Get( &uOutputAccumBufSize );
		uScratchMemRequired += uOutputAccumBufSize; 

		// Storage reused between 2 scenarios:

		// FFT/IFFT storage config
		AkUInt32 uFFTStorageConfigSize = 0;
		// FFT state
		AKASSERT( m_uFFTSpaceRequirements == m_uIFFTSpaceRequirements );
		uFFTStorageConfigSize += (AkUInt32)m_uFFTSpaceRequirements;
		// Freq win storage (prev or cur or vocoder)
		// Note: All freq windows have the same sizes
		AkUInt32 uFreqWindowBufSize;
		m_FreqWindow[0][0].Get( &uFreqWindowBufSize );
		uFFTStorageConfigSize += uFreqWindowBufSize;

		// OR

		// Vocoder storage config
		// Freq win storage (prev,cur,vocoder)
		AkUInt32 uVocoderStorageConfigSize = 3 * uFreqWindowBufSize;
		// Previous phases
		AkUInt32 uPreviousPhaseBufferSize = AK_ALIGN_SIZE_FOR_DMA( ((m_uFFTSize/2)+1)*sizeof(PhaseProcessingType) );
		uVocoderStorageConfigSize += uPreviousPhaseBufferSize;

		uScratchMemRequired += AkMax( uFFTStorageConfigSize, uVocoderStorageConfigSize ); 
		return uScratchMemRequired;
	}

#endif // #ifdef AK_PS3

} // namespace BUTTERFLYSET_NAMESPACE

} // namespace DSP
