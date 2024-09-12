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

#include "AkSinkPulseAudio.h"

#include "AkLEngine.h"

#include "PulseAudioAPI.h"

#define AKPULSEAUDIOSINK_MAXDEVICENAME_LENGTH 256

using PulseAudioAPI::ThreadedMainloopLock;

extern AkPlatformInitSettings g_PDSettings;

AKRESULT CAkPulseAudioSink::DynamicLoadAPI()
{
	return PulseAudioAPI::Load();
}

void stream_state_callback(pa_stream* s, void* userdata)
{
	reinterpret_cast<CAkPulseAudioSink*>(userdata)->HandleStreamState();
}

void stream_overflow_callback(pa_stream* s, void* userdata)
{
	reinterpret_cast<CAkPulseAudioSink*>(userdata)->HandleStreamOverflow();
}

void stream_underflow_callback(pa_stream* s, void* userdata)
{
	reinterpret_cast<CAkPulseAudioSink*>(userdata)->HandleStreamUnderflow();
}

void context_state_callback(pa_context* ctx, void* userdata)
{
	reinterpret_cast<CAkPulseAudioSink*>(userdata)->HandleContextState();
}

void server_info_callback(pa_context* ctx, const pa_server_info* serverInfo, void* userdata)
{
	reinterpret_cast<CAkPulseAudioSink*>(userdata)->HandleServerInfo(serverInfo);
}

void stream_request_callback(pa_stream* s, size_t nbBytes, void* userdata)
{
	reinterpret_cast<CAkPulseAudioSink*>(userdata)->HandleStreamRequest(nbBytes);
}

void CAkPulseAudioSink::StopStream()
{
	if (m_pa.stream == NULL)
	{
		return;
	}

	// lock
	{
		ThreadedMainloopLock lock(m_pa.mainloop);
	
		if (PA_STREAM_IS_GOOD(pa_stream_get_state(m_pa.stream)))
		{
			// Pause
			if (pa_stream_is_corked(m_pa.stream) == 0)
			{
				if (pa_operation* op = pa_stream_cork(m_pa.stream, 1, NULL, NULL))
				{
					pa_operation_unref(op);
				}
			}

			// Flush
			if (pa_operation* op = pa_stream_flush(m_pa.stream, NULL, NULL))
			{
				pa_operation_unref(op);
			}

			// Disconnect
			if (pa_stream_disconnect(m_pa.stream) != 0) { /* "PulseAudio stream disconnect failed."*/ }
		}
	}
	// unlock
}

CAkPulseAudioSink::CAkPulseAudioSink()
	: m_pSinkPluginContext(NULL)
	, m_pAllocator(NULL)
	, m_uOutputID(AK_INVALID_OUTPUT_DEVICE_ID)
	, m_nbBuffers(0)
	, m_bufferSize(0)
	, m_nbSamplesPerBuffer(0)
	, m_nbChannels(0)
	, m_sampleRate(0)
	, m_buffer(nullptr)
	, m_uUnderflowCount(0)
	, m_uOverflowCount(0)
	, m_bIsPrimary(false)
	, m_uPluginID(AK_INVALID_PLUGINID)
	, m_bDeviceFound(false)
{
}

CAkPulseAudioSink::~CAkPulseAudioSink()
{
}

AKRESULT CAkPulseAudioSink::Init( 
	AK::IAkPluginMemAlloc *		in_pAllocator,			// Interface to memory allocator to be used by the effect.
	AK::IAkSinkPluginContext *	in_pSinkPluginContext,	// Interface to sink plug-in's context.
    AK::IAkPluginParam *        in_pParams,             // Interface to parameter node
	AkAudioFormat &				io_rFormat				// Audio data format of the input signal. 
	)
{
	m_pAllocator = in_pAllocator;
	m_pSinkPluginContext = in_pSinkPluginContext;
	m_bIsPrimary = m_pSinkPluginContext->IsPrimary();
	m_bDeviceFound = false;

	AKRESULT eResult = m_pSinkPluginContext->GetOutputID(m_uOutputID, m_uPluginID);
	if (eResult != AK_Success)
		return eResult;

	m_uPluginID = AKGETPLUGINIDFROMCLASSID(m_uPluginID);

	// Convert to this data type in CAkSinkBase during ApplyGainAndInterleave
	m_outputDataType = g_PDSettings.sampleType;

	IAkLinuxContext* platformContext = static_cast<IAkLinuxContext*>(in_pSinkPluginContext->GlobalContext()->GetPlatformContext());

	if (platformContext->IsPluginSupported(m_uPluginID) == false)
	{
		return AK_DeviceNotCompatible; // revert to dummy
	}

	if (platformContext->IsStreamReady(m_uPluginID) == false)
	{
		return AK_Fail; // try again later
	}

	m_nbBuffers = AkMax(g_PDSettings.uNumRefillsInVoice, 1);
	m_nbSamplesPerBuffer = m_pSinkPluginContext->GlobalContext()->GetMaxBufferLength();
	m_nbChannels = io_rFormat.GetNumChannels(); // Desired number of channels (may be changed upon connection if not supported by device)
	m_sampleRate = m_pSinkPluginContext->GlobalContext()->GetSampleRate(); // Try with default sample rate

	// Save the requested stream parameters for CreateStream()
	const char* streamName = platformContext->GetStreamName(m_uPluginID, m_uOutputID);
	AKPLATFORM::SafeStrCpy(m_outputStreamInfo.name, streamName, AK_MAX_PATH);

	m_outputStreamInfo.numChannels = platformContext->GetChannelCount(m_uPluginID, m_uOutputID);

	AKRESULT res = InitPulseAudio();
    if (res != AK_Success)
    {
		CleanupPulseAudio();
		return res;
    }

	// Update output configuration in case it changed
	AkChannelMask channelMask = AK::ChannelMaskFromNumChannels(m_nbChannels);
	io_rFormat.channelConfig.SetStandardOrAnonymous(m_nbChannels, channelMask);
	io_rFormat.uSampleRate = m_sampleRate;
	io_rFormat.uBitsPerSample = GetBitsPerSample();
	io_rFormat.uBlockAlign = m_nbChannels * io_rFormat.uBitsPerSample / 8;
	io_rFormat.uTypeID = m_outputDataType;

	m_pipelineBuffer.Clear();
	m_pipelineBuffer.SetChannelConfig(io_rFormat.channelConfig);

	platformContext->SetSinkInitialized(m_uPluginID, true);

	return AK_Success;
}

// Initializes the PulseAudio main loop and connects to the PulseAudio server.
AKRESULT CAkPulseAudioSink::InitPulseAudio()
{
    // Enforce library versions with pa_get_library_version() compared to compiled version pa_get_headers_version() and protocol version PA_PROTOCOL_VERSION ?

    // Initialize main loop
    m_pa.mainloop = pa_threaded_mainloop_new();
    if (m_pa.mainloop == NULL)
	{
        return AK_Fail;
	}

    if (pa_threaded_mainloop_start(m_pa.mainloop) < 0)
    {
        pa_threaded_mainloop_free(m_pa.mainloop);
        m_pa.mainloop = NULL;
        return AK_Fail;
    }

	// lock
	{
		ThreadedMainloopLock lock(m_pa.mainloop);

		// Create PulseAudio Context
		pa_mainloop_api* api = pa_threaded_mainloop_get_api(m_pa.mainloop);
		m_pa.context = pa_context_new(api, "Wwise");
		if (m_pa.context == NULL)
		{
			return AK_Fail;
		}

		pa_context_set_state_callback(m_pa.context, context_state_callback, this);

		pa_context_flags_t contextFlags = PA_CONTEXT_NOFLAGS;
		if (pa_context_connect(m_pa.context, NULL, contextFlags, NULL) < 0 ||
			PulseAudioAPI::ContextWait(m_pa.context, m_pa.mainloop) == false)
		{
			return AK_Fail;
		}

		// Retrieve default device from server
		// This fills default device name and output number of channels
		IAkLinuxContext* platformContext = static_cast<IAkLinuxContext*>(m_pSinkPluginContext->GlobalContext()->GetPlatformContext());

		// If we don't need specific stream properties, fallback to default server properties
		if (m_uOutputID == 0 && platformContext->UsePulseAudioServerInfo())
		{
			if (PulseAudioAPI::OperationWait(pa_context_get_server_info(m_pa.context, server_info_callback, this), m_pa.mainloop) == false)
			{
				return AK_Fail;
			}
		}
		else
		{
			m_bDeviceFound = true; // Use stream info from PlatformContext
		}
	}
	// unlock

	if (!m_bDeviceFound)
	{
		return AK_Fail; // Device information not available (no device?)
	}

	AKRESULT res = CreateStream();
	if (res != AK_Success)
	{
		return res;
	}

	return AK_Success;
}

AKRESULT CAkPulseAudioSink::CreateStream()
{
	ThreadedMainloopLock lock(m_pa.mainloop);

	// Set stream name and channel count
	char streamName[AK_MAX_PATH];
	if (m_bIsPrimary)
		AKPLATFORM::SafeStrCpy(streamName, m_outputStreamInfo.name, AK_MAX_PATH);
	else
		AKPLATFORM::SafeStrCpy(streamName, "Wwise Secondary", AK_MAX_PATH);

	// Sample format specification
	struct pa_sample_spec ss;
	if (GetSampleSpec(ss) == false)
	{
		return AK_NotCompatible;
	}
	m_nbChannels = ss.channels;
	m_sampleRate = ss.rate;
	m_bufferSize = m_nbChannels * m_nbSamplesPerBuffer * GetBitsPerSample() / 8;

	// Init buffer
	m_buffer = AK_PLUGIN_ALLOC(m_pAllocator, m_bufferSize);
	if (m_buffer == NULL)
	{
		return AK_InsufficientMemory;
	}
	memset(m_buffer, 0, m_bufferSize);

	// Channel Mapping
	pa_channel_map requestChannelMap = GetChannelMap(m_nbChannels);
	if (pa_channel_map_valid(&requestChannelMap) < 0)
	{
		return AK_NotCompatible;
	}

	// Create stream
	m_pa.stream = pa_stream_new(m_pa.context, streamName, &ss, &requestChannelMap);
	if (m_pa.stream == NULL)
	{
		return AK_InsufficientMemory;
	}

	pa_stream_set_state_callback(m_pa.stream, stream_state_callback, this);
	pa_stream_set_overflow_callback(m_pa.stream, stream_overflow_callback, this);
	pa_stream_set_underflow_callback(m_pa.stream, stream_underflow_callback, this);
	pa_stream_set_write_callback(m_pa.stream, stream_request_callback, this);

	// Stream buffering attributes.
	// Note: Pulse Audio has it's own ring buffer mechanism so no need for a ring buffer,
	// having an additional ring buffer will only add latency.
	// Instead, set the maxlength to the "ring buffer" size you want to have
	// and have PulseAudio ask for chunks of size minreq. This is equivalent
	// to having our own ring buffer of size m_bufferSize*m_nbBuffers and
	// writing/reading m_bufferSize bytes from it each time.
	struct pa_buffer_attr attr;
	attr.tlength = m_bufferSize * m_nbBuffers; // Target length of the server playback buffer
	attr.maxlength = m_bufferSize * m_nbBuffers; // Max length of the server playback buffer
	attr.minreq = m_bufferSize; // How much the buffer needs to be empty before pulse asks for more (prevent requests smaller than a whole buffer)
	attr.prebuf = -1; // Set fill level before playback starts automatically
	attr.fragsize = -1; // not used for output

	pa_stream_flags_t flags = (pa_stream_flags_t)PA_STREAM_START_CORKED;

	if (pa_stream_connect_playback(m_pa.stream, streamName, &attr, flags, NULL, NULL) < 0 ||
		PulseAudioAPI::StreamWait(m_pa.stream, m_pa.mainloop) == false)
	{
		return AK_Fail;
	}

	return AK_Success;
}

pa_channel_map CAkPulseAudioSink::GetChannelMap(uint8_t nbChannels)
{
	// Wwise standard mapping L-R-C-LFE-RL-RR-RC-SL-SR-HL-HR-HC-HRL-HRR-HRC-T
	// In order to support up to 16 channels.
	// Always open the stream with Wwise Standard channel mapping, since it does not change based on channel number
	// since the remapping does not cost an extra memcpy this solution should keep the same performance.
	// Another solution will be to directly map into Wwise runtime mapping but this has a lot of case to handle.
	pa_channel_map channelMap;
	channelMap.channels = nbChannels;
	channelMap.map[0] = PA_CHANNEL_POSITION_FRONT_LEFT;
	channelMap.map[1] = PA_CHANNEL_POSITION_FRONT_RIGHT;
	channelMap.map[2] = PA_CHANNEL_POSITION_FRONT_CENTER;
	channelMap.map[3] = PA_CHANNEL_POSITION_LFE;
	channelMap.map[4] = PA_CHANNEL_POSITION_REAR_LEFT;
	channelMap.map[5] = PA_CHANNEL_POSITION_REAR_RIGHT;
	channelMap.map[6] = PA_CHANNEL_POSITION_REAR_CENTER;
	channelMap.map[7] = PA_CHANNEL_POSITION_SIDE_LEFT;
	channelMap.map[8] = PA_CHANNEL_POSITION_SIDE_RIGHT;
	channelMap.map[9] = PA_CHANNEL_POSITION_TOP_FRONT_LEFT;
	channelMap.map[10] = PA_CHANNEL_POSITION_TOP_FRONT_RIGHT;
	channelMap.map[11] = PA_CHANNEL_POSITION_TOP_FRONT_CENTER;
	channelMap.map[12] = PA_CHANNEL_POSITION_TOP_REAR_LEFT;
	channelMap.map[13] = PA_CHANNEL_POSITION_TOP_REAR_RIGHT;
	channelMap.map[14] = PA_CHANNEL_POSITION_TOP_REAR_CENTER;
	channelMap.map[15] = PA_CHANNEL_POSITION_TOP_CENTER;
	return channelMap;
}

bool CAkPulseAudioSink::GetSampleSpec(pa_sample_spec& out_sampleSpec)
{
	out_sampleSpec.format = m_outputDataType == AK_INT ? PA_SAMPLE_S16NE : PA_SAMPLE_FLOAT32NE;

	// PulseAudio will resample if necessary
	out_sampleSpec.rate = m_pSinkPluginContext->GlobalContext()->GetSampleRate();

	// Try with desired number of channels first
	out_sampleSpec.channels = m_nbChannels;
	if (pa_sample_spec_valid(&out_sampleSpec))
		return true;

	// Unsupported number of channels, revert to what the stream can offer
	out_sampleSpec.channels = m_outputStreamInfo.numChannels;
	if (pa_sample_spec_valid(&out_sampleSpec))
		return true;

	// Perhaps unsupported sample format, try the alternative format (int or float)
	out_sampleSpec.format = m_outputDataType == AK_INT ? PA_SAMPLE_FLOAT32NE : PA_SAMPLE_S16NE;
	if (!pa_sample_spec_valid(&out_sampleSpec))
	{
		// Remaining sample spec is the sample rate.
		// We can't change our mind now on the sample rate
		// since the sound engine is already initialized
		return false;
	}

	if (out_sampleSpec.format != PA_SAMPLE_S16NE &&
		out_sampleSpec.format != PA_SAMPLE_FLOAT32NE)
	{
		return false; // we don't support other formats
	}

	// We changed the output format, adjust internal format
	m_outputDataType = out_sampleSpec.format == PA_SAMPLE_S16NE ? AK_INT : AK_FLOAT;

	return true;
}

void CAkPulseAudioSink::CleanupPulseAudio()
{
	StopStream();

	if (m_pa.mainloop)
	{
		// lock
		{
			ThreadedMainloopLock lock(m_pa.mainloop);

			if (m_pa.stream)
			{
				// Clear all callbacks
				pa_stream_set_write_callback(m_pa.stream, NULL, NULL);
				pa_stream_set_state_callback(m_pa.stream, NULL, NULL);
				pa_stream_set_overflow_callback(m_pa.stream, NULL, NULL);
				pa_stream_set_underflow_callback(m_pa.stream, NULL, NULL);

				// Cleanup memory
				pa_stream_unref(m_pa.stream);
				m_pa.stream = NULL;
			}

			if (m_pa.context)
			{
				if (PA_CONTEXT_IS_GOOD(pa_context_get_state(m_pa.context)))
				{
					pa_context_disconnect(m_pa.context);
				}
				pa_context_unref(m_pa.context);
				m_pa.context = NULL;
			}
		}
		// unlock

		pa_threaded_mainloop_stop(m_pa.mainloop);
		pa_threaded_mainloop_free(m_pa.mainloop);
		m_pa.mainloop = NULL;
	}

	if (m_buffer)
	{
		AK_PLUGIN_FREE(m_pAllocator, m_buffer);
		m_buffer = NULL;
	}

	m_bDeviceFound = false;
}

AKRESULT CAkPulseAudioSink::Term( AK::IAkPluginMemAlloc * in_pAllocator )
{
	CleanupPulseAudio();

	// Sink is not initialized anymore
	auto* platformContext = static_cast<IAkLinuxContext*>(m_pSinkPluginContext->GlobalContext()->GetPlatformContext());
	platformContext->SetSinkInitialized(m_uPluginID, false);

	AK_PLUGIN_DELETE( in_pAllocator, this );
	return AK_Success;
}

AKRESULT CAkPulseAudioSink::Reset()
{
	if (m_pa.stream)
	{
		ThreadedMainloopLock lock(m_pa.mainloop);

		// Start Stream
		if (pa_stream_is_corked(m_pa.stream) == 1)
		{
			if (pa_operation* op = pa_stream_cork(m_pa.stream, 0, NULL, NULL))
			{
				pa_operation_unref(op);
			}
		}

		return AK_Success;
	}
	return AK_Fail;
}

bool CAkPulseAudioSink::IsStarved()
{
	return m_uUnderflowCount > 0; 
}

void CAkPulseAudioSink::ResetStarved()
{
	m_uUnderflowCount = 0;
}

void CAkPulseAudioSink::PassData()
{
	AKASSERT(m_pipelineBuffer.HasData() && m_pipelineBuffer.uValidFrames == AK_NUM_VOICE_REFILL_FRAMES);

	ThreadedMainloopLock lock(m_pa.mainloop);

	void* buffer = NULL;
	size_t size = m_bufferSize;
	pa_stream_begin_write(m_pa.stream, &buffer, &size);

	if (size < m_bufferSize)
	{
		pa_stream_cancel_write(m_pa.stream);
		return;
	}

	memcpy(buffer, m_buffer, m_bufferSize);

	pa_stream_write(m_pa.stream, buffer, m_bufferSize, NULL, 0, PA_SEEK_RELATIVE);
}

void CAkPulseAudioSink::PassSilence()
{
	ThreadedMainloopLock lock(m_pa.mainloop);

	void* buffer = NULL;
	size_t size = m_bufferSize;
	pa_stream_begin_write(m_pa.stream, &buffer, &size);

	if (size < m_bufferSize)
	{
		pa_stream_cancel_write(m_pa.stream);
		return;
	}

	memset(buffer, 0, m_bufferSize);

	pa_stream_write(m_pa.stream, buffer, m_bufferSize, NULL, 0, PA_SEEK_RELATIVE);
}

AKRESULT CAkPulseAudioSink::IsDataNeeded(AkUInt32& out_uBuffersNeeded)
{
	size_t writableSize = pa_stream_writable_size(m_pa.stream);

	out_uBuffersNeeded = writableSize / m_bufferSize;
	
	return AK_Success;
}

// ---- Callback Handlers ---- //

void CAkPulseAudioSink::HandleStreamState()
{
    switch (pa_stream_get_state(m_pa.stream))
    {
        case PA_STREAM_READY:
        case PA_STREAM_FAILED:
        case PA_STREAM_TERMINATED:
			pa_threaded_mainloop_signal(m_pa.mainloop, 0);
        default:
            break;
    }
}

void CAkPulseAudioSink::HandleStreamOverflow()
{
	m_uOverflowCount++;

	if (pa_operation* op = pa_stream_flush(m_pa.stream, NULL, NULL))
	{
		pa_operation_unref(op);
	}
}

void CAkPulseAudioSink::HandleStreamUnderflow()
{
	m_uUnderflowCount++;
}

void CAkPulseAudioSink::HandleContextState()
{
    switch (pa_context_get_state(m_pa.context))
    {
        case PA_CONTEXT_READY:
        case PA_CONTEXT_FAILED:
        case PA_CONTEXT_TERMINATED:
			pa_threaded_mainloop_signal(m_pa.mainloop, 0);
        default:
            break;
    }
}

void CAkPulseAudioSink::HandleServerInfo(const pa_server_info* serverInfo)
{
	AKPLATFORM::SafeStrCpy(m_outputStreamInfo.name, serverInfo->default_sink_name, AKPULSEAUDIOSINK_MAXDEVICENAME_LENGTH);
	m_outputStreamInfo.numChannels = serverInfo->sample_spec.channels;
	m_outputStreamInfo.format = serverInfo->sample_spec.format;
	m_bDeviceFound = true;

	pa_threaded_mainloop_signal(m_pa.mainloop, 0);
}

void CAkPulseAudioSink::HandleStreamRequest(size_t nbBytes)
{
	if (m_bIsPrimary)
	{
		m_pSinkPluginContext->SignalAudioThread();
	}
}
