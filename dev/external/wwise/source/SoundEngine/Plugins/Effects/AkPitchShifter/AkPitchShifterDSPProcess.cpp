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

#include "AkPitchShifterDSPProcess.h"
#include <AK/Tools/Common/AkAssert.h>
#include "Mix2Interp.h"
#include <AK/DSP/AkApplyGain.h>

#if defined(__SPU__) && defined(_DEBUG)
#include "libsn_spu.h"
#endif

void AkPitchShifterDSPProcess(	AkAudioBuffer * io_pBuffer, 
								AkPitchShifterFXInfo & io_FXInfo, 
								AkReal32 * in_pfBufferStorage,
								void * pTwoPassStorage
	#ifdef __SPU__
								, AkUInt32 in_uDMATag
#endif
							  )
{
	io_FXInfo.FXTailHandler.HandleTail( io_pBuffer, io_FXInfo.uTailLength );
	if ( io_pBuffer->uValidFrames == 0 )
		return;

	const AkUInt32 uNumFrames = io_pBuffer->uValidFrames;

	
#ifdef __SPU__
	AkReal32 * pfDryDelayLS = NULL;
	AkUInt8 * pParseStorage = (AkUInt8*)in_pfBufferStorage;
	if ( io_FXInfo.Params.bSyncDry )
	{
		pfDryDelayLS = (AkReal32*)pParseStorage;
		pParseStorage += uNumFrames*sizeof(AkReal32);
	}
	AkReal32 * pfWet = NULL;
	AkReal32 * pfPitchShiftDelay = NULL;
	if ( io_FXInfo.uNumProcessedChannels > 0 )
	{
		pfWet = (AkReal32*)pParseStorage;
		pParseStorage += uNumFrames*sizeof(AkReal32);
		pfPitchShiftDelay = (AkReal32*)pParseStorage;
	}
#else
	AkReal32 * pfWet = in_pfBufferStorage;
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
			if ( io_FXInfo.uNumProcessedChannels > 0 && 
				(uCurrentChannelMask & io_FXInfo.eProcessChannelMask) )
			{
#ifdef AKDELAYPITCHSHIFT_USETWOPASSALGO
				AKASSERT( pTwoPassStorage != NULL );
				io_FXInfo.PitchShifter.ProcessChannel( pfCurrentChan, pfWet, pTwoPassStorage, uNumFrames, uWetProcessedChan
#ifdef __SPU__
					, pfPitchShiftDelay, in_uDMATag
#endif
					);
#else
				io_FXInfo.PitchShifter.ProcessChannel( pfCurrentChan, pfWet, uNumFrames, uWetProcessedChan
#ifdef __SPU__
					, pfPitchShiftDelay, in_uDMATag
#endif
					);
#endif

				if ( io_FXInfo.Params.Voice.Filter.eFilterType != AKFILTERTYPE_NONE )
					io_FXInfo.Filter.ProcessChannel( pfWet, uNumFrames, uWetProcessedChan );
			
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
			if ( io_FXInfo.uNumProcessedChannels > 0 && 
				(uCurrentChannelMask & io_FXInfo.eProcessChannelMask) )
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
