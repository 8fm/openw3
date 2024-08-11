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
// AkMixer.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _AK_MIXER_H_
#define _AK_MIXER_H_

// Comment this line to disable SIMD optimizations
#define USE_SIMD

#include "AkCommon.h"					// AkSpeakerVolumes
#include "AkDownmix.h"

#ifdef AK_PS3
#include "AkVPLNode.h"
#endif

class	AkAudioBuffer;
struct	AkSpeakerVolumes;

struct AkSpeakerVolumesN
{
	AkReal32 fVolume;
	AkReal32 fVolumeDelta;

	AkSpeakerVolumesN( AkReal32 fPrevVol, AkReal32 fNextVol, AkReal32 fOneOverNumSamples )
	{
		fVolume = fPrevVol;
		fVolumeDelta = (fNextVol - fPrevVol) * fOneOverNumSamples;
	}
};

// Downmix structures; downmix equations are implemented in constructor and stored in structure.
// They all concern full band channels only. LFE needs to be handled separately.
struct AkSpeakerDownmixVolumes
{
	AkForceInline AkReal32 GetChannelVolume( unsigned int in_uChanIdx ) { return fOutputVolumes[in_uChanIdx]; }

	// Array of anonymous output volumes.
	// Identity of each channel depends on the configuration.
	AkReal32 fOutputVolumes[AK_VOICE_MAX_NUM_CHANNELS];

	AkSpeakerDownmixVolumes( 
		AkSpeakerVolumes & in_volumes,
		AkUInt32 in_uOutputConfig
		)
	{
		AkDownmix::ComputeVolumes( in_volumes, in_uOutputConfig, fOutputVolumes );
	}
};

struct AkSpeakerDownmixVolumesDelta : public AkSpeakerDownmixVolumes
{
	AkForceInline AkReal32 GetChannelVolumeDelta( unsigned int in_uChanIdx ) { return fOutputVolumeDeltas[in_uChanIdx]; }

	// Array of output anonymous volume deltas.
	// Identity of each channel depends on the configuration.
	AkReal32 fOutputVolumeDeltas[AK_VOICE_MAX_NUM_CHANNELS];

	AkSpeakerDownmixVolumesDelta( AkAudioMix& audioMix, AkUInt32 in_uOutputConfig, AkUInt32 in_uNumFullBandOutputChannels, AkReal32 in_fOneOverNumSamples )
		:AkSpeakerDownmixVolumes( audioMix.Previous.volumes, in_uOutputConfig )
	{
		// Compute deltas.
		AkReal32 fNextVolumes[AK_VOICE_MAX_NUM_CHANNELS];
 		AkDownmix::ComputeVolumes( audioMix.Next.volumes, in_uOutputConfig, fNextVolumes );
		for ( AkUInt32 uChan = 0; uChan < in_uNumFullBandOutputChannels; uChan++ )
		{
			fOutputVolumeDeltas[uChan] = ( fNextVolumes[uChan] - fOutputVolumes[uChan] ) * in_fOneOverNumSamples;
		}
	}
};

//====================================================================================================
class CAkMixer
{
public:
	CAkMixer();

	inline void Init( AkUInt16 in_uMaxFrames )
	{
		m_usMaxFrames = in_uMaxFrames;
		m_fOneOverNumFrames = (1.0f / ((AkReal32)in_uMaxFrames));
	}

	// Note: Needed for PS3 until we implement out-of-place effect bypass on SPUs.
	static void DownMix( AkPipelineBufferBase * in_pInputBuffer, AkPipelineBufferBase* in_pOutputBuffer, AkSIMDSpeakerVolumes in_volumes[], AkUInt32 in_uNumSamples );
	
#ifdef AK_PS3
	// PS3
	// ----------------------------------------------------------
	void ExecuteSPU(
		class AkVPLState * in_pVPLState, 
		AkPipelineBufferBase&	in_OutBuffer,
		AkAudioMix in_AudioMix[],
		AkInt8 in_VPLEnvironmentIndex,
		AkVolumeOffset* in_pAttenuation
		);
	void ExecuteSPU(
		AkAudioBufferBus*	in_pInputBuffer,
		AkPipelineBufferBase&	in_OutBuffer,
		AkAudioMix				in_AudioMix[] 
		);
	void FinalInterleaveExecuteSPU(
		AkAudioBufferBus*	in_pInputBuffer,
		AkPipelineBufferBase*	in_pOutputBuffer
		);
	// ----------------------------------------------------------
#else

	// NOT PS3
	// ----------------------------------------------------------

	void Mix(
		AkAudioBufferBus*		in_pInputBuffer,
		AkPipelineBufferBase*	io_pOutputBuffer,
		bool					in_bPan,
		AkAudioMix				in_PanMix[]
		)
	{
		AkChannelMask fromFormat = in_pInputBuffer->GetChannelMask();
		AkChannelMask toFormat = io_pOutputBuffer->GetChannelMask();
		if( fromFormat == toFormat && !in_bPan )
		{
			// "Mix N"; direct channel assignment.
			AkSpeakerVolumesN Volumes( in_pInputBuffer->m_fPreviousVolume, in_pInputBuffer->m_fNextVolume, m_fOneOverNumFrames );

			AkUInt32 uNumOutputChannels = AK::GetNumChannels( toFormat );
			AKASSERT( uNumOutputChannels > 0 || !"0.1 downmix not supported" );
			AkUInt32 uChannel = 0;
			do
			{
				MixChannelSIMD( in_pInputBuffer->GetChannel( uChannel ), io_pOutputBuffer->GetChannel( uChannel ), Volumes.fVolume, Volumes.fVolumeDelta, m_usMaxFrames );
			}
			while ( ++uChannel < uNumOutputChannels );
		}
		else
		{
			Mix3D( in_pInputBuffer, io_pOutputBuffer, in_PanMix );
		}

		// set the number of output bytes
		io_pOutputBuffer->uValidFrames = m_usMaxFrames;
	}

	void Mix3D(
		AkPipelineBufferBase*	in_pInputBuffer,
		AkPipelineBufferBase*	in_pOutputBuffer,
		AkAudioMix in_AudioMix[]
		);

	void MixFinalStereo(
		AkAudioBufferBus*	in_pInputBuffer,
		AkPipelineBufferBase*	in_pOutputBuffer
		);

#if defined AK_APPLE || defined AK_PS4
	void MixFinalMono(
		AkAudioBufferBus*	in_pInputBuffer,
		AkPipelineBufferBase*	in_pOutputBuffer
		);
#endif

#if defined(AK_LFECENTER) && defined(AK_REARCHANNELS)
	void MixFinal51(
		AkAudioBufferBus*	in_pInputBuffer,
		AkPipelineBufferBase*	in_pOutputBuffer
		);
#ifdef AK_71FROMSTEREOMIXER
	void MixFinal71FromStereo(
		AkAudioBufferBus*	in_pInputBuffer,
		AkPipelineBufferBase*	in_pOutputBuffer
		);
#endif
#ifdef AK_71FROM51MIXER
	void MixFinal71From51(
		AkAudioBufferBus*	in_pInputBuffer,
		AkPipelineBufferBase*	in_pOutputBuffer
		);
#endif
#endif // defined(AK_LFECENTER) && defined(AK_REARCHANNELS)

#ifdef AK_71AUDIO
	void MixFinal71(
		AkAudioBufferBus*	in_pInputBuffer,
		AkPipelineBufferBase*	in_pOutputBuffer
		);
#endif
	
#if defined AK_APPLE || defined AK_PS4
	void ProcessVolume(
		AkAudioBufferBus* in_pInputBuffer, 
		AkPipelineBufferBase* in_pOutputBuffer 
		);
	
	void ApplyVolume(	
		AkReal32*	in_pSourceData,
		AkReal32*	in_pDestData,
		AkReal32	in_fVolume,
		AkReal32	in_fVolumeDelta
		);
#endif

private:

	void MixAndInterleaveStereo( AkAudioBufferBus* in_pInputBuffer, AkPipelineBufferBase* in_pOutputBuffer );

	void MixAndInterleave51( AkAudioBufferBus* in_pInputBuffer, AkPipelineBufferBase* in_pOutputBuffer );
	void MixAndInterleave71FromStereo( AkAudioBufferBus* in_pInputBuffer, AkPipelineBufferBase* in_pOutputBuffer );
	void MixAndInterleave71From51( AkAudioBufferBus* in_pInputBuffer, AkPipelineBufferBase* in_pOutputBuffer );

#ifdef AK_71AUDIO
	void MixAndInterleave71( AkAudioBufferBus* in_pInputBuffer, AkPipelineBufferBase* in_pOutputBuffer );
#endif

#ifdef USE_SIMD
	AkForceInline void VolumeInterleavedStereo( AkAudioBuffer*	in_pSource, AkReal32* in_pDestData, AkSpeakerVolumesN in_Volumes );
#ifdef AK_ANDROID
	AkForceInline void VolumeInterleavedStereo( AkAudioBuffer*	in_pSource, AkInt16* in_pDestData, AkSpeakerVolumesN in_Volumes );
#endif	
#if defined(AK_REARCHANNELS) && defined(AK_LFECENTER)
	AkForceInline void VolumeInterleaved51( AkAudioBuffer*	in_pSource, AkReal32* in_pDestData, AkSpeakerVolumesN in_Volumes );
#ifdef AK_71FROMSTEREOMIXER
	void VolumeInterleaved71FromStereo( AkAudioBuffer*	in_pSource, AkReal32* in_pDestData, AkSpeakerVolumesN in_Volumes );
#endif
#ifdef AK_71FROM51MIXER
	AkForceInline void VolumeInterleaved71From51( AkAudioBuffer*	in_pSource, AkReal32* in_pDestData, AkSpeakerVolumesN in_Volumes );
#endif
#endif // defined(AK_REARCHANNELS) && defined(AK_LFECENTER)
#ifdef AK_71AUDIO
	AkForceInline void VolumeInterleaved71( AkAudioBuffer*	in_pSource, AkReal32* in_pDestData, AkSpeakerVolumesN in_Volumes );
#endif
#else
	AkForceInline void VolumeInterleaved( AkReal32* in_pSourceData, AkReal32* in_pDestData, AkSpeakerVolumesN in_Volumes );
#endif

	// ----------------------------------------------------------
#endif	// PS3 / Not PS3.

	// Assumes in_uNumSamples % 8 == 0.
	// Implemented in AkMixerSIMD.cpp
	static void MixChannelSIMD(
		AkReal32*	in_pSourceData,
		AkReal32*	in_pDestData,
		AkReal32	in_fVolume,
		AkReal32	in_fVolumeDelta,
		AkUInt32	in_uNumSamples
		);

	// N-sample version.
	// Important: source and destination buffer addresses still need to be properly aligned for SIMD.
	static void MixChannel( 
		AkReal32*	in_pSourceData, 
		AkReal32*	in_pDestData, 
		AkReal32	in_fVolume, 
		AkUInt32	in_uNumSamples 
		);

	AkUInt16		m_usMaxFrames;
	AkReal32		m_fOneOverNumFrames;
};
#endif
