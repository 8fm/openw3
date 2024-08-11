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

#include "AkHarmonizerDSPProcess.h"
#include <AK/Tools/Common/AkAssert.h>
#include "Mix2Interp.h"
#include <AK/DSP/AkApplyGain.h>

#if defined(__SPU__) && defined(_DEBUG)
#include "libsn_spu.h"
#endif

#ifndef __PPU__

void AkHarmonizerDSPProcessVoice( 
	AkReal32 * in_pfCurChan,
	AkHarmonizerFXInfo & io_FXInfo, 
	AkUInt32 in_uChannelIndex,
	AkUInt32 in_uVoiceIndex,
	AkReal32 * in_pfMonoPitchedVoice,
	AkReal32 * in_pfWet,
	AkUInt32 in_uNumFrames,
	bool	 in_bNoMoreData,
	AkReal32 in_fResamplingFactor,
	AkReal32 * in_pfPVTDWindow
#ifdef __SPU__
	, AkUInt8 * in_pScratchMem
	, AkUInt32 in_uDMATag
#endif
	)
{
	AKASSERT( in_uVoiceIndex < AKHARMONIZER_NUMVOICES );
	if ( io_FXInfo.Params.Voice[in_uVoiceIndex].bEnable )
	{
		io_FXInfo.PhaseVocoder[in_uVoiceIndex].ProcessPitchChannel( 
			in_pfCurChan, 
			in_uNumFrames, 
			in_bNoMoreData, 
			in_uChannelIndex, 
			in_pfMonoPitchedVoice, 
			in_fResamplingFactor, 
			in_pfPVTDWindow
#ifdef __SPU__
			, in_pScratchMem
			, in_uDMATag
#endif
			);

		if ( io_FXInfo.Params.Voice[in_uVoiceIndex].Filter.eFilterType != AKFILTERTYPE_NONE )
			io_FXInfo.Filter[in_uVoiceIndex].ProcessChannel( in_pfMonoPitchedVoice, in_uNumFrames, in_uChannelIndex );
		::DSP::Mix2Interp( in_pfWet, in_pfMonoPitchedVoice, 
							1.f, 1.f,
							io_FXInfo.PrevParams.Voice[in_uVoiceIndex].fGain, io_FXInfo.Params.Voice[in_uVoiceIndex].fGain,
							in_uNumFrames );
	}
}

void AkHarmonizerDSPProcess(	AkAudioBuffer * io_pBuffer, 
								AkHarmonizerFXInfo & io_FXInfo, 
								AkReal32 * in_pfTempStorage
#ifdef __SPU__
								, AkUInt8 * in_pScratchMem
								, AkUInt32 in_uDMATag
#endif
								)
{
// Maximum resampling of input buffer is 4x (2 octaves), consider input buffer 5/4 window size and output buffer 1 window size (rounded upwards)
	io_FXInfo.FXTailHandler.HandleTail( io_pBuffer, 4*3*io_FXInfo.Params.uWindowSize );
	if ( io_pBuffer->uValidFrames == 0 )
		return;
	const AkUInt32 uNumFrames = io_pBuffer->uValidFrames;
	AkReal32 * pfMonoPitchedVoice = in_pfTempStorage;
	AkReal32 * pfWet = &in_pfTempStorage[uNumFrames];
	AkReal32 * pfPhaseVocoderWindow = &in_pfTempStorage[2*uNumFrames];
#ifdef __SPU__
	AkReal32 * pfDryDelayLS = NULL;
	if ( io_FXInfo.Params.bSyncDry )
	{
		pfDryDelayLS = (AkReal32*)in_pScratchMem;
		in_pScratchMem += uNumFrames*sizeof(AkReal32);
	}
	AkUInt8 * pPhaseVocoderScratchMem = in_pScratchMem;
#endif	

	const AkChannelMask eBufferChannelMask = io_pBuffer->GetChannelMask();
	AkUInt32 uDryChannelIndex = 0;
	AkUInt32 uDryProcessedChan = 0;
	AkUInt32 uWetProcessedChan = 0;
	AkChannelMask eChannelMaskToProcess = eBufferChannelMask;
	AkUInt32 i = 0;
	while ( eChannelMaskToProcess )
	{
		AkUInt32 uCurrentChannelMask = 1<<i;
		++i;
		
		// if existent channels in AkAudiobuffer only
		if ( uCurrentChannelMask & eBufferChannelMask  )
		{
			AkReal32 * pfCurrentChan;
			if ( (uCurrentChannelMask & AK_SPEAKER_LOW_FREQUENCY) == 0 )
				pfCurrentChan = io_pBuffer->GetChannel( uDryChannelIndex++ );
			else
				pfCurrentChan = io_pBuffer->GetLFE( );
			AKASSERT( pfCurrentChan );
			 
			// Note: no distinction made to determine if center is true center (i.e not mono)
			// For processed channels (according to user parameter) only
			if ( io_FXInfo.bWetPathEnabled && 
				(uCurrentChannelMask & io_FXInfo.eProcessChannelMask) )
			{
				AkZeroMemLarge( pfWet, uNumFrames*sizeof(AkReal32) );

				AkHarmonizerDSPProcessVoice(	pfCurrentChan,
												io_FXInfo, 
												uWetProcessedChan, 
												0, 
												pfMonoPitchedVoice, 
												pfWet, 
												uNumFrames, 
												io_pBuffer->eState == AK_NoMoreData,
												io_FXInfo.Params.Voice[0].fPitchFactor,
												pfPhaseVocoderWindow
											#ifdef __SPU__
												, pPhaseVocoderScratchMem
												, in_uDMATag
											#endif
												);
				
				AkHarmonizerDSPProcessVoice(	pfCurrentChan,
												io_FXInfo, 
												uWetProcessedChan, 
												1, 
												pfMonoPitchedVoice, 
												pfWet, 
												uNumFrames, 
												io_pBuffer->eState == AK_NoMoreData,
												io_FXInfo.Params.Voice[1].fPitchFactor,
												pfPhaseVocoderWindow
											#ifdef __SPU__
												, pPhaseVocoderScratchMem
												, in_uDMATag
											#endif
												);
				uWetProcessedChan++;
			}

			// Delay dry to sync with wet path (even unprocessed channels)
			if ( io_FXInfo.Params.bSyncDry )
			{
				io_FXInfo.DryDelay[uDryProcessedChan].ProcessBuffer( pfCurrentChan, uNumFrames
#ifdef __SPU__
					, pfDryDelayLS, in_uDMATag
#endif
					);
				uDryProcessedChan++;
			}
			
			// For processed channels (according to user parameter) only
			if ( (io_FXInfo.bWetPathEnabled && (uCurrentChannelMask & io_FXInfo.eProcessChannelMask)) || 
				 ((io_FXInfo.Params.eInputType == AKINPUTTYPE_LEFTONLY) && 
					(uCurrentChannelMask == AK_SPEAKER_FRONT_RIGHT) && 
					 (eBufferChannelMask & AK_SPEAKER_FRONT_RIGHT)) )
			{
				// Apply wet level and mix with dry
				DSP::Mix2Interp( 
					pfCurrentChan, pfWet, 
					io_FXInfo.PrevParams.fDryLevel, io_FXInfo.Params.fDryLevel, 
					io_FXInfo.PrevParams.fWetLevel, io_FXInfo.Params.fWetLevel,
					uNumFrames );
			}
			else
			{	
				// Channel exist but no associated wet path, just apply dry gain
				AK::DSP::ApplyGain( pfCurrentChan, io_FXInfo.PrevParams.fDryLevel, io_FXInfo.Params.fDryLevel, uNumFrames );
			}

			eChannelMaskToProcess = eChannelMaskToProcess & ~uCurrentChannelMask;
		}	// if existent channels in AkAudiobuffer only
	} // for maximum channels


	io_FXInfo.PrevParams = io_FXInfo.Params;
}

#endif // #ifndef __PPU__
