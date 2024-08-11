/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.

* Modified from Scaleform sources to reduce video microstuttering. Temporarily here
* to avoid having to rebuild the video library while tweaking.
*/

/**************************************************************************

Filename    :   Video_VideoSoundSystemPS4.cpp
Content     :   Video sound system implementation based on PS4 AudioOut library
Created     :   May 2014
Authors     :   Vladislav Merker

Copyright   :   Copyright 2014 Autodesk, Inc. All Rights reserved.

Use of this software is subject to the terms of the Autodesk license
agreement provided at the time of installation or download, or which
otherwise accompanies this software in either electronic or hard copy form.

**************************************************************************/

#include "build.h"
#include "videoSoundSystemPS4.h"

#ifdef USE_SCALEFORM
#ifdef RED_PLATFORM_ORBIS

#ifdef GFX_ENABLE_VIDEO

#include "Video/Video_SystemSoundInterface.h"
#include "Kernel/SF_HeapNew.h"

#include <audioout.h>
#include <sceerror.h>
#include <pthread.h>

#include "../core/configVar.h"

extern Float GHackVideoSFXVolume;
extern Float GHackVideoVoiceVolume;
namespace Config
{
	extern TConfigVar< Bool > cvVideoVolumeLinearScale;
}

namespace Scaleform { namespace GFx { namespace Video {

//////////////////////////////////////////////////////////////////////////
//

class VideoSoundPS4Tmp : public VideoSoundStream
{
public:
	VideoSoundPS4Tmp(MemoryHeap *pheap) :
		pHeap(pheap), pPCMStream(0),
		SoundStatus(Sound_Stopped), NumChannels(0), SampleRate(0), Volume(1.0f), TotalSamples(0),
		AudioPort(0), SoundThrExit(false) {}
	virtual ~VideoSoundPS4Tmp() {}

	virtual bool        CreateOutput(UInt32 channels, UInt32 sample_rate);
	virtual void        DestroyOutput();

	virtual PCMFormat   GetPCMFormat() const { return PCM_Float; }

	virtual void        Start(PCMStream *pstream);
	virtual void        Stop();
	virtual Status      GetStatus() const { return SoundStatus; }
	virtual void        Pause(bool sw);
	virtual void        GetTime(UInt64 *count, UInt64 *unit) const;

	virtual void        SetVolume(float volume);
	virtual float       GetVolume() { return Volume; }

private:
	static const UInt32 NUM_CHANNELS = 8;     // SCE_AUDIO_OUT_CHANNEL_MAX
	static const UInt32 SAMPLES_PER_PACKET = 256;
	static const UInt32 SAMPLE_SIZE = sizeof(float);
	static const UInt32 PACKET_SIZE = (NUM_CHANNELS * SAMPLES_PER_PACKET * SAMPLE_SIZE);
	static const UInt32 AUDIO_FRAME_INTERVAL = 4;

	MemoryHeap*         pHeap;
	PCMStream*          pPCMStream;

	Status              SoundStatus;
	UInt32              NumChannels;
	UInt32              SampleRate;
	float               Volume;
	UInt64              TotalSamples;

	// AudioOut port handle
	int32_t             AudioPort;
	// Buffers to receive and to submit PCM data
	float               TempBuffer[NUM_CHANNELS][SAMPLES_PER_PACKET];
	float               SampleBuffer[PACKET_SIZE][2];

	pthread_t           SoundThr;
	volatile bool       SoundThrExit;
	static void*        AudioProc(void* arg);
	void                ExecuteAudioFrame( UInt32 sampleBufferIndex );
};


//////////////////////////////////////////////////////////////////////////
//

static void DefaultInitVideoSoundThrd( ScePthread pid, int requestedPriority )
{
	RED_UNUSED( pid );
	RED_UNUSED( requestedPriority );

 	// Thread attributes already appropritate set by default! No need to reset the schedParam
	// 	const SceKernelSchedParam schedParam = { requestedPriority };
	// 	scePthreadSetprio( pid, requestedPriority );
	// 	scePthreadSetaffinity( pid, 1U<<4 );
}

typedef void (*FUNC_INIT_VIDEO_SOUND_THRD)(ScePthread pid,int);
FUNC_INIT_VIDEO_SOUND_THRD InitVideoSoundThrd = &DefaultInitVideoSoundThrd;

bool VideoSoundPS4Tmp::CreateOutput(UInt32 channels, UInt32 sample_rate)
{   
	NumChannels = channels < NUM_CHANNELS ? channels : NUM_CHANNELS;
	SampleRate = sample_rate;

	// Initialize audio system
	int32_t ret = sceAudioOutInit();
	SF_ASSERT(ret == SCE_OK || ret == SCE_AUDIO_OUT_ERROR_ALREADY_INIT);

	int32_t portType = SCE_AUDIO_OUT_PORT_TYPE_MAIN;
	int32_t dataFormat = SCE_AUDIO_OUT_PARAM_FORMAT_FLOAT_MONO; // Float 32-bit monaural by default
	if (NumChannels == 2)
	{   // Float 32-bit 2ch stereo
		dataFormat = SCE_AUDIO_OUT_PARAM_FORMAT_FLOAT_STEREO;
	}
	else if(NumChannels > 2)
	{   // Float 32-bit 7.1 multi-channel (L-R-C-LFE-Lsur-Rsur-Lext-Rext interleaved)
		NumChannels = 8;
		dataFormat = SCE_AUDIO_OUT_PARAM_FORMAT_FLOAT_8CH;
	}

	// Open an audio port. Note: only 48000Hz sampling frequency is supported by AudioOut library
	int32_t	handle = sceAudioOutOpen(SCE_USER_SERVICE_USER_ID_SYSTEM, 
		portType, 0,  SAMPLES_PER_PACKET, 48000 /*SampleRate*/, dataFormat);
	SF_ASSERT(handle > 0);
	AudioPort = handle;

	// Create sound thread
	SoundThrExit = false;

	const SceKernelSchedParam schedParam = { SCE_KERNEL_PRIO_FIFO_HIGHEST };
	ScePthreadAttr attr;
 	::scePthreadAttrInit( &attr );
 	::scePthreadAttrSetstacksize( &attr, PTHREAD_STACK_MIN );
	::scePthreadAttrSetinheritsched( &attr, SCE_PTHREAD_EXPLICIT_SCHED );
	::scePthreadAttrSetschedpolicy( &attr, SCE_KERNEL_SCHED_FIFO );
	::scePthreadAttrSetschedparam( &attr, &schedParam );
	::scePthreadAttrSetaffinity( &attr, 1<<4 );
	::scePthreadAttrSetdetachstate( &attr, SCE_PTHREAD_CREATE_JOINABLE ); 
 	ret = ::scePthreadCreate( &SoundThr, &attr, VideoSoundPS4Tmp::AudioProc, this, "Scaleform Video Sound" );
	if ( ret != SCE_OK )
	{
		::scePthreadAttrDestroy( &attr );

		DestroyOutput();
		return false;
	}

	::scePthreadAttrDestroy( &attr );

	InitVideoSoundThrd( SoundThr, SCE_KERNEL_PRIO_FIFO_HIGHEST );

	return true;
}

void VideoSoundPS4Tmp::DestroyOutput()
{
	// Shutdown sound thread
	if(SoundThr)
	{
		SoundThrExit = true;
		pthread_join(SoundThr, NULL);
		SoundThr = 0;
	}

	// Release an audio port
	if(AudioPort)
	{
		int32_t ret = sceAudioOutClose(AudioPort);
		SF_ASSERT(ret == SCE_OK);
	}
}

void VideoSoundPS4Tmp::Start(PCMStream *pstream)
{
	pPCMStream = pstream;
	TotalSamples = 0;
	SoundStatus = Sound_Playing;

#ifndef GFC_NO_SOUND
	SetVolume(pChannel->GetVolume());
#endif
}

void VideoSoundPS4Tmp::Stop()
{
	SoundStatus = Sound_Stopped;
	pPCMStream = 0;
	TotalSamples = 0;
}

void VideoSoundPS4Tmp::Pause(bool sw)
{
	if(sw) {
		if (SoundStatus == Sound_Playing)
		{
			SoundStatus = Sound_Paused;

			// For consideration: flush all pending audio buffers. Doesn't appear to be needed to keep A/V sync when constraining/pausing.
			//const int32_t ret = sceAudioOutOutput(AudioPort, NULL);
			//SF_ASSERT(ret > 0);
		}
	}
	else {
		if (SoundStatus == Sound_Paused)
			SoundStatus = Sound_Playing;
	}
}

void VideoSoundPS4Tmp::GetTime(UInt64* count, UInt64* unit) const
{
	if(TotalSamples == 0) {
		*count = 0;
		*unit  = 1000;
	}
	else {
		*count = TotalSamples * 1000 / SampleRate;
		*unit = 1000;
	}    
}



void VideoSoundPS4Tmp::SetVolume(float volume)
{
	RED_UNUSED(volume);

	Volume = GHackVideoSFXVolume;

	if(SoundStatus == Sound_Playing)
	{
		const Float sfx = GHackVideoSFXVolume;
		const Float voice = GHackVideoVoiceVolume;

		Int32 normSFx = 0;
		Int32 normVoice = 0;

		if ( Config::cvVideoVolumeLinearScale.Get() )
		{
			normSFx = SCE_AUDIO_OUT_VOLUME_0DB * sfx;
			normVoice = SCE_AUDIO_OUT_VOLUME_0DB * voice;
		}
		else
		{
			// [-60, 0] dB
			const Float a = 0.001f;
			const Float b = 6.908f;

			normSFx = sfx == 0.f ? 0 : SCE_AUDIO_OUT_VOLUME_0DB * a * exp( sfx * b );
			normVoice = voice == 0.f ? 0 : SCE_AUDIO_OUT_VOLUME_0DB * a * exp( voice * b );
		}

		normSFx = Clamp< Int32 >( normSFx, 0, SCE_AUDIO_OUT_VOLUME_0DB );
		normVoice = Clamp< Int32 >( normVoice, 0, SCE_AUDIO_OUT_VOLUME_0DB );

		LOG_ENGINE(TXT("VideoSoundPS4Tmp SetVolume sfx=%.6f, voice=%.6f [0.f, 1.f] -> normSFx=%d, normVoice=%d [0, %d]"), sfx, voice, normSFx, normVoice, SCE_AUDIO_OUT_VOLUME_0DB );

		int32_t vol[NUM_CHANNELS];
		for(UInt32 i = 0; i < NUM_CHANNELS; ++i)
		{
			vol[i] = normSFx;
		}

		vol[ SCE_AUDIO_OUT_CHANNEL_C ] = normVoice;

		// Set volume for all 8 channels
		int32_t ret = sceAudioOutSetVolume(AudioPort,
			(SCE_AUDIO_VOLUME_FLAG_FL_CH  | SCE_AUDIO_VOLUME_FLAG_FR_CH | SCE_AUDIO_VOLUME_FLAG_CNT_CH | SCE_AUDIO_VOLUME_FLAG_LFE_CH 
			| SCE_AUDIO_VOLUME_FLAG_RL_CH | SCE_AUDIO_VOLUME_FLAG_RR_CH | SCE_AUDIO_VOLUME_FLAG_BL_CH  | SCE_AUDIO_VOLUME_FLAG_BR_CH), vol);
		SF_ASSERT(ret == SCE_OK);
	}
}


//////////////////////////////////////////////////////////////////////////
//

void *VideoSoundPS4Tmp::AudioProc(void* arg)
{
	VideoSoundPS4Tmp* psnd = (VideoSoundPS4Tmp *)arg;

	UInt32 sampleBufferIndex = 0;
	while(!psnd->SoundThrExit)
	{
		psnd->ExecuteAudioFrame( sampleBufferIndex );
		sampleBufferIndex ^= 1;
	
		//trying to have one buffered audioout submit in the pipe before yielding
		// NOTE: we yield simply because we're FIFO scheduled and if some other thread has the same pri as ourselves
		// then let it run
		if ( sampleBufferIndex < 1 )
		{
			::sceKernelUsleep(AUDIO_FRAME_INTERVAL);
			//scePthreadYield();
		}
	}

	// Flush all audio buffers in use before destroying them
 	const int32_t ret = sceAudioOutOutput(psnd->AudioPort, NULL);
 	SF_ASSERT(ret > 0);

	return NULL;
}

void VideoSoundPS4Tmp::ExecuteAudioFrame( UInt32 sampleBufferIndex )
{
	if(SoundStatus == Sound_Stopped)
		return;

	// Get PCM data
	float *sample[NUM_CHANNELS];
	for(UInt32 i = 0; i < NumChannels; ++i)
	{
		sample[i] = TempBuffer[i];
	}
	UInt32 wsmpl = 0;
	if (SoundStatus == Sound_Playing  && pPCMStream) 
	{
		wsmpl = pPCMStream->GetDataFloat(NumChannels, sample, SAMPLES_PER_PACKET);
	} 
	else 
	{
		memset(TempBuffer, 0, sizeof(TempBuffer));
		wsmpl = 0;
	}

	// Interleaving
	float *pcmbuf = SampleBuffer[sampleBufferIndex];

	// Seems to prevent sound glitches when abruptly stopping video
	// or stalls like in loading a savegame image
	memset( pcmbuf, 0, sizeof(float)*PACKET_SIZE );

	for(UInt32 n = 0; n < wsmpl; ++n)
	{
		static UInt32 ch_map[NUM_CHANNELS] = {0, 1, 4, 5, 2, 3, 6, 7};
		for(UInt32 i = 0; i < NumChannels; ++i)
		{
			pcmbuf[n * NumChannels + i] = sample[ch_map[i]][n];
		}
	}

	// Submit PCM buffer
	int32_t ret = sceAudioOutOutput(AudioPort, pcmbuf);
	SF_ASSERT(ret > 0);

	TotalSamples += wsmpl;
}


//////////////////////////////////////////////////////////////////////////
//

VideoSound* VideoSoundSystemPS4Tmp::Create()
{
	return SF_HEAP_NEW(pHeap) VideoSoundPS4Tmp(pHeap);
}

}}} // Scaleform::GFx::Video

#endif // GFX_ENABLE_VIDEO

#endif // RED_PLATFORM_ORBIS
#endif // USE_SCALEFORM