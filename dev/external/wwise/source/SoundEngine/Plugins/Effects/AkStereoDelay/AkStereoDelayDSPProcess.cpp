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

#include "AkStereoDelayDSPProcess.h"
#include "Mix2Interp.h"
#include "Mix3Interp.h"
#include <AK/DSP/AkApplyGain.h>

#if defined(__SPU__) && defined(_DEBUG)
#include "libsn_spu.h"
#endif

#ifdef AK_REARCHANNELS
void ProcessSurroundChannels( 
	AkAudioBuffer & StereoInputBuffer,
	AkAudioBuffer & StereoOutputBuffer,
	AkAudioBuffer * io_pBuffer, 
	AkStereoDelayFXInfo & io_FXInfo, 
	AkReal32 in_fPrevGain,
	AkReal32 in_fCurGain,
	AkUInt8 in_uIndexSurroundLeft,
	AkUInt8 in_uDelayLineIdx
#ifdef __SPU__
	, AkReal32 * in_pfStereoBufferStorage
	, AkUInt32 in_uDMATag
#endif
	)
{
	const AkUInt32 uNumFrames = io_pBuffer->uValidFrames;
	AkReal32 * pfInputL = StereoInputBuffer.GetChannel(0);
	AkReal32 * pfInputR = StereoInputBuffer.GetChannel(1);
	AkReal32 * pfOutputL = StereoOutputBuffer.GetChannel(0);
	AkReal32 * pfOutputR = StereoOutputBuffer.GetChannel(1);
	
	// Left
	switch ( io_FXInfo.Params.eInputType[0] )
	{
	case AKINPUTCHANNELTYPE_LEFT_OR_RIGHT:
	case AKINPUTCHANNELTYPE_DOWNMIX:
		AKPLATFORM::AkMemCpy(pfInputL, io_pBuffer->GetChannel(in_uIndexSurroundLeft), uNumFrames*sizeof(AkReal32) ); 
		break;
	case AKINPUTCHANNELTYPE_CENTER:
	case AKINPUTCHANNELTYPE_NONE:
		AkZeroMemLarge(pfInputL, uNumFrames*sizeof(AkReal32) ); // silenced input
		break;
	}

	// Right
	switch ( io_FXInfo.Params.eInputType[1] )
	{
	case AKINPUTCHANNELTYPE_LEFT_OR_RIGHT:
	case AKINPUTCHANNELTYPE_DOWNMIX:
		AKPLATFORM::AkMemCpy(pfInputR, io_pBuffer->GetChannel(in_uIndexSurroundLeft+1), uNumFrames*sizeof(AkReal32) ); 
		break;
	case AKINPUTCHANNELTYPE_CENTER:
	case AKINPUTCHANNELTYPE_NONE:
		AkZeroMemLarge(pfInputR, uNumFrames*sizeof(AkReal32) ); // silenced input
		break;
	}

	// Process rear stereo delay
	io_FXInfo.StereoDelay[in_uDelayLineIdx].ProcessBuffer( 	
		&StereoInputBuffer, 
		&StereoOutputBuffer,
		io_FXInfo.PrevParams.StereoDelayParams, 
		io_FXInfo.Params.StereoDelayParams,
		io_FXInfo.Params.FilterParams,
		io_FXInfo.bRecomputeFilterCoefs
#ifdef __SPU__
		, &in_pfStereoBufferStorage[4*uNumFrames]
		, in_uDMATag
#endif
		);

	// Wet/dry mix
	DSP::Mix2Interp(	io_pBuffer->GetChannel(in_uIndexSurroundLeft),pfOutputL,  
		io_FXInfo.PrevParams.fDryLevel, io_FXInfo.Params.fDryLevel,
		io_FXInfo.PrevParams.fWetLevel*in_fCurGain, io_FXInfo.Params.fWetLevel*in_fPrevGain,
		uNumFrames );
	DSP::Mix2Interp(	io_pBuffer->GetChannel(in_uIndexSurroundLeft+1),pfOutputR,  
		io_FXInfo.PrevParams.fDryLevel, io_FXInfo.Params.fDryLevel,
		io_FXInfo.PrevParams.fWetLevel*in_fCurGain, io_FXInfo.Params.fWetLevel*in_fPrevGain,
		uNumFrames );
}
#endif // AK_REARCHANNELS

void AkStereoDelayDSPProcess(  	AkAudioBuffer * io_pBuffer, 
								AkStereoDelayFXInfo & io_FXInfo, 
								AkReal32 * in_pfStereoBufferStorage
#ifdef __SPU__
								, AkUInt32 in_uDMATag
#endif
								)
{
	io_FXInfo.FXTailHandler.HandleTail( io_pBuffer, io_FXInfo.uTailLength );
	if ( io_pBuffer->uValidFrames == 0 )
		return;

	const AkUInt32 uNumFrames = io_pBuffer->uValidFrames;

	// Zero out the LFE channel in send effect configuration to avoid doubling effect (since it is not processed), 
	// Note: LFE channel is always the last channel
	AkReal32 * pLFEChan = io_pBuffer->GetLFE( );
	if ( pLFEChan != NULL && io_FXInfo.bSendMode )
	{
		AkZeroMemLarge( pLFEChan, uNumFrames*sizeof(AkReal32) );
	}
	if ( io_pBuffer->GetChannelMask() == AK_SPEAKER_SETUP_0POINT1 )
		return;

	// Create temporary buffers
	AkAudioBuffer StereoInputBuffer;
	StereoInputBuffer.AttachContiguousDeinterleavedData( in_pfStereoBufferStorage, (AkUInt16)uNumFrames, (AkUInt16)uNumFrames, AK_SPEAKER_SETUP_2_0 );
	AkAudioBuffer StereoOutputBuffer;
	StereoOutputBuffer.AttachContiguousDeinterleavedData( &in_pfStereoBufferStorage[2*uNumFrames], (AkUInt16)uNumFrames, (AkUInt16)uNumFrames, AK_SPEAKER_SETUP_2_0 );

	const AkChannelMask InputChannelMask = io_pBuffer->GetChannelMask() & ~AK_SPEAKER_LOW_FREQUENCY; // Disregard LFE for this effect
	const AkUInt32 uNumFullBandChannels = AK::GetNumChannels( InputChannelMask );
	// Front input channel selection / downmix
	// Left
	AkReal32 * pfInputL = StereoInputBuffer.GetChannel(0);
	switch ( io_FXInfo.Params.eInputType[0] )
	{
	case AKINPUTCHANNELTYPE_LEFT_OR_RIGHT:
		// Check whether there is a left
		AKASSERT ( (InputChannelMask & AK_SPEAKER_FRONT_LEFT) || (InputChannelMask == AK_SPEAKER_SETUP_1_0_CENTER) );
		AKPLATFORM::AkMemCpy(pfInputL, io_pBuffer->GetChannel(0), uNumFrames*sizeof(AkReal32) ); 
		break;
	case AKINPUTCHANNELTYPE_CENTER:
		{
			// Use left when there is no true center
			AkUInt8 uCenterChannelIndex = ((InputChannelMask & AK_SPEAKER_FRONT_LEFT) && (InputChannelMask & AK_SPEAKER_FRONT_CENTER)) ? 2 : 0;	
			AKPLATFORM::AkMemCpy(pfInputL, io_pBuffer->GetChannel(uCenterChannelIndex), uNumFrames*sizeof(AkReal32) ); 
		}
		break;
	case AKINPUTCHANNELTYPE_DOWNMIX:
		if ( (InputChannelMask & AK_SPEAKER_FRONT_LEFT) && (InputChannelMask & AK_SPEAKER_FRONT_CENTER) )
			DSP::Mix2Interp( io_pBuffer->GetChannel(0), io_pBuffer->GetChannel(2), pfInputL, 0.707f, 0.707f, 0.707f, 0.707f, uNumFrames );
		else  
		{
			AKASSERT( (InputChannelMask & AK_SPEAKER_FRONT_LEFT) || (InputChannelMask & AK_SPEAKER_FRONT_CENTER) );
			AKPLATFORM::AkMemCpy(pfInputL, io_pBuffer->GetChannel(0), uNumFrames*sizeof(AkReal32) ); 
		}
		break;
	case AKINPUTCHANNELTYPE_NONE:
		AkZeroMemLarge(pfInputL, uNumFrames*sizeof(AkReal32) ); // silenced input
		break;
	}

	// Right
	AkReal32 * pfInputR = StereoInputBuffer.GetChannel(1);
	switch ( io_FXInfo.Params.eInputType[1] )
	{
	case AKINPUTCHANNELTYPE_LEFT_OR_RIGHT:
		// Check whether there is a right
		if ( InputChannelMask & AK_SPEAKER_FRONT_RIGHT )
			AKPLATFORM::AkMemCpy(pfInputR, io_pBuffer->GetChannel(1), uNumFrames*sizeof(AkReal32) ); 
		else 
		{
			AKPLATFORM::AkMemCpy(pfInputR, io_pBuffer->GetChannel(0), uNumFrames*sizeof(AkReal32) ); 
			AKASSERT( (InputChannelMask == AK_SPEAKER_SETUP_1_0_CENTER) );
		}
		break;
	case AKINPUTCHANNELTYPE_CENTER:
		{
			// Use right when there is no true center
			AkUInt32 uCenterChannelIndex = ((InputChannelMask & AK_SPEAKER_FRONT_RIGHT) && (InputChannelMask & AK_SPEAKER_FRONT_CENTER)) ? 2 : AkMin(1,uNumFullBandChannels-1);
			AKPLATFORM::AkMemCpy(pfInputR, io_pBuffer->GetChannel(uCenterChannelIndex), uNumFrames*sizeof(AkReal32) ); 
		}
		break;
	case AKINPUTCHANNELTYPE_DOWNMIX:
		if ( (InputChannelMask & AK_SPEAKER_FRONT_RIGHT) && (InputChannelMask & AK_SPEAKER_FRONT_CENTER) )
			DSP::Mix2Interp( io_pBuffer->GetChannel(1), io_pBuffer->GetChannel(2), pfInputR, 0.707f, 0.707f, 0.707f, 0.707f, uNumFrames );
		else if ( InputChannelMask & AK_SPEAKER_FRONT_RIGHT )
			AKPLATFORM::AkMemCpy(pfInputR, io_pBuffer->GetChannel(1), uNumFrames*sizeof(AkReal32) ); 
		else 
		{
			AKASSERT( InputChannelMask & AK_SPEAKER_FRONT_CENTER );
			AKPLATFORM::AkMemCpy(pfInputR, io_pBuffer->GetChannel(0), uNumFrames*sizeof(AkReal32) ); 
		}
		break;
	case AKINPUTCHANNELTYPE_NONE:
		AkZeroMemLarge(pfInputR, uNumFrames*sizeof(AkReal32) ); // silenced input
		break;
	}

	// Process front stereo delay
	io_FXInfo.StereoDelay[0].ProcessBuffer(  
		&StereoInputBuffer, 
		&StereoOutputBuffer,
		io_FXInfo.PrevParams.StereoDelayParams, 
		io_FXInfo.Params.StereoDelayParams,
		io_FXInfo.Params.FilterParams,
		io_FXInfo.bRecomputeFilterCoefs 
#ifdef __SPU__
		, &in_pfStereoBufferStorage[4*uNumFrames]
		, in_uDMATag
#endif
		);

	// Wet dry mix in front left+right
	AkReal32 fCurFrontGain = 1.f;
	AkReal32 fCurRearGain = 1.f;
	AkReal32 fPrevFrontGain = 1.f;
	AkReal32 fPrevRearGain = 1.f;
	if ( InputChannelMask & AK_SPEAKER_BACK_LEFT )
	{
		// WG-19497 Need double arithmetic here
		// Consider front/rear balance
		AkReal64 fCurRearPower = ((AkReal64)io_FXInfo.Params.fFrontRearBalance + 100.) * 0.005; // remap (-100,100) -> (0,1)
		fCurRearGain = (AkReal32)sqrt(fCurRearPower);
		fCurFrontGain = (AkReal32)sqrt(1.-fCurRearPower);
		AkReal64 fPrevRearPower = ((AkReal64)io_FXInfo.PrevParams.fFrontRearBalance + 100.) * 0.005; // remap (-100,100) -> (0,1)
		fPrevRearGain = (AkReal32)sqrt(fPrevRearPower);
		fPrevFrontGain = (AkReal32)sqrt(1.-fPrevRearPower);
	}

	AkReal32 * pfOutputL = StereoOutputBuffer.GetChannel(0);
	AkReal32 * pfOutputR = StereoOutputBuffer.GetChannel(1);
	if ( InputChannelMask == AK_SPEAKER_SETUP_1_0_CENTER )
	{
		// Downmix delay output to mono + wet/dry mix
		DSP::Mix3Interp(	io_pBuffer->GetChannel(0),pfOutputL, pfOutputR, 
			io_FXInfo.PrevParams.fDryLevel, io_FXInfo.Params.fDryLevel,
			io_FXInfo.PrevParams.fWetLevel, io_FXInfo.Params.fWetLevel,
			io_FXInfo.PrevParams.fWetLevel, io_FXInfo.Params.fWetLevel,
			uNumFrames );
	}
	else if ( InputChannelMask & AK_SPEAKER_FRONT_LEFT )
	{
		// Wet/dry mix
		DSP::Mix2Interp(	io_pBuffer->GetChannel(0),pfOutputL,  
			io_FXInfo.PrevParams.fDryLevel, io_FXInfo.Params.fDryLevel,
			io_FXInfo.PrevParams.fWetLevel*fCurFrontGain, io_FXInfo.Params.fWetLevel*fPrevFrontGain,
			uNumFrames );
		DSP::Mix2Interp(	io_pBuffer->GetChannel(1),pfOutputR,  
			io_FXInfo.PrevParams.fDryLevel, io_FXInfo.Params.fDryLevel,
			io_FXInfo.PrevParams.fWetLevel*fCurFrontGain, io_FXInfo.Params.fWetLevel*fPrevFrontGain,
			uNumFrames );
	}

#ifdef AK_REARCHANNELS
	if ( (InputChannelMask & AK_SPEAKER_BACK_LEFT) && (InputChannelMask & AK_SPEAKER_BACK_RIGHT) )
	{
		// Rear channel selection
		AkUInt8 uIndexRearLeft = (InputChannelMask & AK_SPEAKER_FRONT_CENTER) ? 3 : 2;
		ProcessSurroundChannels( 
			StereoInputBuffer, 
			StereoOutputBuffer, 
			io_pBuffer, 
			io_FXInfo, 
			fCurRearGain, 
			fPrevRearGain, 
			uIndexRearLeft, 
			1
#ifdef __SPU__
			, in_pfStereoBufferStorage
			, in_uDMATag
#endif
			);

#ifdef AK_71AUDIO
		if ( (InputChannelMask & AK_SPEAKER_SIDE_LEFT) && (InputChannelMask & AK_SPEAKER_SIDE_RIGHT) )
		{
			// Side channel selection
			/// 7.1: We use the same gain as the rear speakers, and not an interpolation between front and rear.
			AkUInt8 uIndexSideLeft = uIndexRearLeft + 2;
			ProcessSurroundChannels( 
				StereoInputBuffer, 
				StereoOutputBuffer, 
				io_pBuffer, 
				io_FXInfo, 
				fCurRearGain, 
				fPrevRearGain, 
				uIndexSideLeft, 
				2
#ifdef __SPU__
				, in_pfStereoBufferStorage
				, in_uDMATag
#endif
				);
		}
#endif
	}
#endif // AK_REARCHANNELS

	// Effect never produce any Wet signal to either (true) center and LFE channels. Simply apply dry levels in those cases.	
	if ( (InputChannelMask & AK_SPEAKER_FRONT_CENTER) && (InputChannelMask != AK_SPEAKER_SETUP_1_0_CENTER) )
		AK::DSP::ApplyGain( io_pBuffer->GetChannel(2), io_FXInfo.PrevParams.fDryLevel, io_FXInfo.Params.fDryLevel, uNumFrames );

	AkReal32 * pLFE = io_pBuffer->GetLFE();
	if ( pLFE )
		AK::DSP::ApplyGain( pLFE, io_FXInfo.PrevParams.fDryLevel, io_FXInfo.Params.fDryLevel, uNumFrames );

	// Gain ramps have been reached
	io_FXInfo.PrevParams = io_FXInfo.Params;

}
