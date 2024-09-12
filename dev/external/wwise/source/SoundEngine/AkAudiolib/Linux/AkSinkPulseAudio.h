/***********************************************************************
  The content of this file includes source code for the sound engine
  portion of the AUDIOKINETIC Wwise Technology and constitutes "Level
  Two Source Code" as defined in the Source Code Addendum attached
  with this file.  Any use of the Level Two Source Code shall be
  subject to the terms and conditions outlined in the Source Code
  Addendum and the End User License Agreement for Wwise(R).

  Version:  Build: 
  Copyright (c) 2006-2020 Audiokinetic Inc.
 ***********************************************************************/

#pragma once

#include "AkSinkBase.h"
#include "IDynamicAPI.h"

#include <AK/SoundEngine/Common/AkSoundEngine.h>

#include "AkSettings.h"

#include <pulse/pulseaudio.h>

class CAkPulseAudioSink : public CAkSinkBase, public IDynamicAPI
{
public:
	CAkPulseAudioSink();

	virtual ~CAkPulseAudioSink();

	virtual AKRESULT Init( 
		AK::IAkPluginMemAlloc* in_pAllocator,
		AK::IAkSinkPluginContext* in_pSinkPluginContext,
		AK::IAkPluginParam* in_pParams,
		AkAudioFormat& io_rFormat
	);
	
	virtual AKRESULT Term( 
		AK::IAkPluginMemAlloc* in_pAllocator
	);
	
	virtual AKRESULT DynamicLoadAPI();

	virtual AKRESULT Reset();
	
	virtual bool IsStarved();
	
	virtual void ResetStarved();
	
	virtual AKRESULT IsDataNeeded(AkUInt32 & out_uBuffersNeeded);

	virtual void PassData();

	virtual void PassSilence();

	virtual AkAudioBuffer& NextWriteBuffer()
	{
		m_pipelineBuffer.AttachInterleavedData(
			m_buffer,
			m_nbSamplesPerBuffer,
			0,
			m_pipelineBuffer.GetChannelConfig()
		);

		return m_pipelineBuffer;
	}

	// PulseAudio Callbacks Handlers
	void HandleStreamState();
	void HandleStreamOverflow();
	void HandleStreamUnderflow();
	void HandleContextState();
	void HandleServerInfo(const pa_server_info* serverInfo);
	void HandleStreamRequest(size_t nbBytes);

private:
	AKRESULT InitPulseAudio();
	void CleanupPulseAudio();

	AKRESULT CreateStream();
	void StopStream();

	bool StreamWait();
	bool OperationWait(pa_operation* op);

	bool GetSampleSpec(pa_sample_spec& out_sampleSpec);

	AkUInt32 GetBitsPerSample()
	{ 
		return m_outputDataType == AK_INT ? 16 : 32;
	}

	pa_sample_format_t GetSampleFormat()
	{
		return m_outputDataType == AK_INT ? PA_SAMPLE_S16NE : PA_SAMPLE_FLOAT32NE;
	}

	static pa_channel_map GetChannelMap(uint8_t nbChannels);

	AK::IAkSinkPluginContext* m_pSinkPluginContext;
	AK::IAkPluginMemAlloc* m_pAllocator;
	AkUInt32 m_uOutputID;

	unsigned int m_nbBuffers;
	unsigned int m_bufferSize;
	unsigned int m_nbSamplesPerBuffer;
	unsigned int m_nbChannels;
	unsigned int m_sampleRate;

	void* m_buffer;

	AkPipelineBufferBase m_pipelineBuffer;

	unsigned int m_uUnderflowCount;
	unsigned int m_uOverflowCount;

	bool m_bIsPrimary;

	struct PulseAudioObjects
	{
		PulseAudioObjects() : stream(NULL), context(NULL), mainloop(NULL) {}
		pa_stream* stream;
		pa_context* context;
		pa_threaded_mainloop* mainloop;
	};
	PulseAudioObjects m_pa;

	AkPluginID m_uPluginID;
	
	struct StreamInfo
	{
		StreamInfo() : numChannels(0), format(PA_SAMPLE_FLOAT32NE), name("") {}
		unsigned int numChannels;
		pa_sample_format_t format;
		char name[AK_MAX_PATH];
	};
	StreamInfo m_outputStreamInfo;

	bool m_bDeviceFound;
};
