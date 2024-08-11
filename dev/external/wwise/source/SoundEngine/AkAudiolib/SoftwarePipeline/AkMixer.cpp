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
// AkMixer.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"

#include "AkCommon.h"
#include "AkMixer.h"
#include "AudiolibDefs.h"
#include "AkMath.h"
#include "AkLEngine.h"

CAkMixer::CAkMixer()
	: m_usMaxFrames( 0 )
	, m_fOneOverNumFrames( 0 )
{
}

// Note: Needed for PS3 until we implement out-of-place effect bypass on SPUs.
void CAkMixer::DownMix( AkPipelineBufferBase * in_pInputBuffer, AkPipelineBufferBase* in_pOutputBuffer, AkSIMDSpeakerVolumes in_volumes[], AkUInt32 in_uNumSamples )
{
	AKASSERT( in_uNumSamples <= in_pInputBuffer->uValidFrames && in_uNumSamples <= in_pOutputBuffer->MaxFrames() );

	// Always exclude LFE for downmix.
	unsigned int uChannel = 0;
	AkUInt32 uInputConfig = in_pInputBuffer->GetChannelMask() & ~AK_SPEAKER_LOW_FREQUENCY;
	unsigned int uNumInputChannels = AK::GetNumChannels( uInputConfig );
		
	AkUInt32 uOutputConfig = in_pOutputBuffer->GetChannelMask() & ~AK_SPEAKER_LOW_FREQUENCY;
	unsigned int uNumOutputChannels = AK::GetNumChannels( uOutputConfig );
	
	if ( uNumOutputChannels > 0 )	// Handle case 0.1.
	{
		while(uChannel < uNumInputChannels)
		{
			AkReal32* AK_RESTRICT pInSample = in_pInputBuffer->GetChannel( uChannel );

			AkSpeakerDownmixVolumes volumes( in_volumes[uChannel].volumes, uOutputConfig );

			unsigned int uOutChannel = 0;
			do
			{
				MixChannel( pInSample, in_pOutputBuffer->GetChannel( uOutChannel ), volumes.GetChannelVolume(uOutChannel), in_uNumSamples );
			}
			while(++uOutChannel < uNumOutputChannels);
			
			++uChannel;
		}
	}

	// LFE.
#ifdef AK_LFECENTER
	if ( in_pInputBuffer->HasLFE() && in_pOutputBuffer->HasLFE() )
	{
		// LFE is always the last channel, and uNumInputChannels/uNumOutputChannels count fullband channels only.
		MixChannel( 
			in_pInputBuffer->GetChannel( uNumInputChannels ), 
			in_pOutputBuffer->GetChannel( uNumOutputChannels ), 
			in_volumes[ uNumInputChannels ].volumes.fLfe, 
			in_uNumSamples );
	}
#endif

	in_pOutputBuffer->uValidFrames = (AkUInt16)in_uNumSamples;
}

void CAkMixer::MixChannel( 
	AkReal32*	in_pSourceData, 
	AkReal32*	in_pDestData, 
	AkReal32	in_fVolume, 
	AkUInt32	in_uNumSamples 
	)
{
	AkUInt32 uNumSamplesAligned = ( in_uNumSamples & ~0x7 );
	MixChannelSIMD( in_pSourceData, in_pDestData, in_fVolume, 0, uNumSamplesAligned );

	// Mix remaining samples.
	in_uNumSamples -= uNumSamplesAligned;
	if ( in_uNumSamples > 0 )
	{
		AkReal32* AK_RESTRICT pSourceData = in_pSourceData + uNumSamplesAligned;
		AkReal32* AK_RESTRICT pDestData = in_pDestData + uNumSamplesAligned;
		AkReal32* AK_RESTRICT pSourceEnd = pSourceData + in_uNumSamples;
		while ( pSourceData < pSourceEnd )
		{
			*pDestData += *pSourceData * in_fVolume;
			pSourceData++;
			pDestData++;
		}
	}
}

#ifndef AK_PS3

void CAkMixer::Mix3D(	AkPipelineBufferBase*	in_pInputBuffer,
						AkPipelineBufferBase*	in_pOutputBuffer,
						AkAudioMix in_audioMix[]
						)
{
	// Always exclude LFE for downmix.
	unsigned int uChannel = 0;
	AkUInt32 uInputConfig = in_pInputBuffer->GetChannelMask() & ~AK_SPEAKER_LOW_FREQUENCY;
	unsigned int uNumInputChannels = AK::GetNumChannels( uInputConfig );
	
	AkUInt32 uOutputConfig = in_pOutputBuffer->GetChannelMask() & ~AK_SPEAKER_LOW_FREQUENCY;
	unsigned int uNumOutputChannels = AK::GetNumChannels( uOutputConfig );
	AKASSERT( uNumOutputChannels > 0 || !"0.1 downmix not supported" );

	while(uChannel < uNumInputChannels)
	{
		AkReal32* AK_RESTRICT pInSample = in_pInputBuffer->GetChannel( uChannel );
		
		AkSpeakerDownmixVolumesDelta volumes( in_audioMix[uChannel], uOutputConfig, uNumOutputChannels, m_fOneOverNumFrames );

		unsigned int uOutChannel = 0;
		do
		{
			MixChannelSIMD( 
				pInSample, 
				in_pOutputBuffer->GetChannel( uOutChannel ), 
				volumes.GetChannelVolume( uOutChannel ), 
				volumes.GetChannelVolumeDelta( uOutChannel ),
				m_usMaxFrames);
		}
		while(++uOutChannel < uNumOutputChannels);
		++uChannel;
	} 

	// LFE.
#ifdef AK_LFECENTER
	if ( in_pInputBuffer->HasLFE() && in_pOutputBuffer->HasLFE() )
	{
		// LFE is always the last channel, and uNumInputChannels/uNumOutputChannels count fullband channels only.
		AkSpeakerVolumesN volumeLFE( in_audioMix[uNumInputChannels].Previous.volumes.fLfe, in_audioMix[uNumInputChannels].Next.volumes.fLfe, m_fOneOverNumFrames );
		MixChannelSIMD( 
			in_pInputBuffer->GetChannel( uNumInputChannels ), 
			in_pOutputBuffer->GetChannel( uNumOutputChannels ), 
			volumeLFE.fVolume, 
			volumeLFE.fVolumeDelta,
			m_usMaxFrames );
	}
#endif

	// set the number of output bytes
	in_pOutputBuffer->uValidFrames = m_usMaxFrames;
}
//====================================================================================================
//====================================================================================================
void CAkMixer::MixFinalStereo(	AkAudioBufferBus*	in_pInputBuffer,
								AkPipelineBufferBase*	in_pOutputBuffer)
{
	// check that we've got what we need
	AKASSERT(in_pInputBuffer != NULL);
	AKASSERT( in_pInputBuffer->NumChannels() == in_pOutputBuffer->NumChannels() );

	MixAndInterleaveStereo( in_pInputBuffer, in_pOutputBuffer );
}


#ifdef AK_PS4
void CAkMixer::MixFinalMono( AkAudioBufferBus* in_pInputBuffer, AkPipelineBufferBase* in_pOutputBuffer)
{
	// check that we've got what we need
	AKASSERT(in_pInputBuffer != NULL);
	AKASSERT( in_pInputBuffer->NumChannels() == in_pOutputBuffer->NumChannels() );

	AkSpeakerVolumesN Volumes( in_pInputBuffer->m_fPreviousVolume, in_pInputBuffer->m_fNextVolume, m_fOneOverNumFrames );
	ApplyVolume( 
		in_pInputBuffer->GetChannel( 0 ), 
		in_pOutputBuffer->GetChannel( 0 ), 
		Volumes.fVolume, 
		Volumes.fVolumeDelta);
}
#endif
//====================================================================================================
//====================================================================================================
#if defined(AK_REARCHANNELS) && defined(AK_LFECENTER)
void CAkMixer::MixFinal51(	AkAudioBufferBus*	in_pInputBuffer,
							AkPipelineBufferBase*	in_pOutputBuffer)
{
	// check that we've got what we need
	AKASSERT(in_pInputBuffer != NULL);
	AKASSERT( in_pInputBuffer->NumChannels() == in_pOutputBuffer->NumChannels() );

	MixAndInterleave51( in_pInputBuffer, in_pOutputBuffer );
}

#ifdef AK_71FROMSTEREOMIXER
void CAkMixer::MixFinal71FromStereo(	AkAudioBufferBus*	in_pInputBuffer,
							AkPipelineBufferBase*	in_pOutputBuffer)
{
	// check that we've got what we need
	AKASSERT(in_pInputBuffer != NULL);
	AKASSERT( in_pInputBuffer->NumChannels() == 2 
			&& in_pOutputBuffer->NumChannels() == 8 );

	MixAndInterleave71FromStereo( in_pInputBuffer, in_pOutputBuffer );
}
#endif

#ifdef AK_71FROM51MIXER
void CAkMixer::MixFinal71From51(	AkAudioBufferBus*	in_pInputBuffer,
							AkPipelineBufferBase*	in_pOutputBuffer)
{
	// check that we've got what we need
	AKASSERT(in_pInputBuffer != NULL);
	AKASSERT( in_pInputBuffer->NumChannels() == 6 
			&& in_pOutputBuffer->NumChannels() == 8 );

	MixAndInterleave71From51( in_pInputBuffer, in_pOutputBuffer );
}
#endif
#endif // defined(AK_REARCHANNELS) && defined(AK_LFECENTER)
//====================================================================================================
//====================================================================================================
#ifdef AK_71AUDIO
void CAkMixer::MixFinal71(	AkAudioBufferBus*	in_pInputBuffer,
							AkPipelineBufferBase*	in_pOutputBuffer)
{
	// check that we've got what we need
	AKASSERT(in_pInputBuffer != NULL);
	AKASSERT( in_pInputBuffer->NumChannels() == in_pOutputBuffer->NumChannels() );

	MixAndInterleave71( in_pInputBuffer, in_pOutputBuffer );
}
#endif


#endif // !PS3
