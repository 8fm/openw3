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

#include "stdafx.h"

#include "IDynamicAPI.h"

#include <pulse/pulseaudio.h>

// Define global variables before we include "PulseAudioAPI.h"
// so before we remap _import_function_name -> function_name

DEFINE_DL_FUNCTION(pa_threaded_mainloop_new);
DEFINE_DL_FUNCTION(pa_mainloop_new);
DEFINE_DL_FUNCTION(pa_mainloop_iterate);
DEFINE_DL_FUNCTION(pa_mainloop_free);
DEFINE_DL_FUNCTION(pa_mainloop_get_api);
DEFINE_DL_FUNCTION(pa_threaded_mainloop_get_api);
DEFINE_DL_FUNCTION(pa_context_new);
DEFINE_DL_FUNCTION(pa_context_set_state_callback);
DEFINE_DL_FUNCTION(pa_threaded_mainloop_lock);
DEFINE_DL_FUNCTION(pa_threaded_mainloop_unlock);
DEFINE_DL_FUNCTION(pa_threaded_mainloop_start);
DEFINE_DL_FUNCTION(pa_context_connect);
DEFINE_DL_FUNCTION(pa_context_get_state);
DEFINE_DL_FUNCTION(pa_threaded_mainloop_wait);
DEFINE_DL_FUNCTION(pa_stream_new);
DEFINE_DL_FUNCTION(pa_stream_set_state_callback);
DEFINE_DL_FUNCTION(pa_operation_unref);
DEFINE_DL_FUNCTION(pa_operation_get_state);
DEFINE_DL_FUNCTION(pa_stream_cork);
DEFINE_DL_FUNCTION(pa_threaded_mainloop_stop);
DEFINE_DL_FUNCTION(pa_stream_disconnect);
DEFINE_DL_FUNCTION(pa_stream_unref);
DEFINE_DL_FUNCTION(pa_context_disconnect);
DEFINE_DL_FUNCTION(pa_context_unref);
DEFINE_DL_FUNCTION(pa_threaded_mainloop_free);
DEFINE_DL_FUNCTION(pa_threaded_mainloop_signal);
DEFINE_DL_FUNCTION(pa_stream_get_state);
DEFINE_DL_FUNCTION(pa_strerror);
DEFINE_DL_FUNCTION(pa_context_get_server_info);
DEFINE_DL_FUNCTION(pa_context_set_event_callback);
DEFINE_DL_FUNCTION(pa_sample_spec_valid);
DEFINE_DL_FUNCTION(pa_stream_write);
DEFINE_DL_FUNCTION(pa_stream_set_underflow_callback);
DEFINE_DL_FUNCTION(pa_stream_flush);
DEFINE_DL_FUNCTION(pa_stream_drain);
DEFINE_DL_FUNCTION(pa_stream_set_write_callback);
DEFINE_DL_FUNCTION(pa_stream_set_overflow_callback);
DEFINE_DL_FUNCTION(pa_stream_connect_playback);
DEFINE_DL_FUNCTION(pa_stream_writable_size);
DEFINE_DL_FUNCTION(pa_stream_get_buffer_attr);
DEFINE_DL_FUNCTION(pa_stream_is_corked);
DEFINE_DL_FUNCTION(pa_channel_map_valid);
DEFINE_DL_FUNCTION(pa_stream_begin_write);
DEFINE_DL_FUNCTION(pa_stream_cancel_write);
DEFINE_DL_FUNCTION(pa_context_get_sink_info_list);

#include "PulseAudioAPI.h"

#include <AK/Tools/Common/AkPlatformFuncs.h>
#include <AK/SoundEngine/Common/AkSoundEngine.h>

bool g_isInitialized = false;

namespace PulseAudioAPI
{
	AKRESULT Load()
	{
		// Dynamic load only once
		if (g_isInitialized)
		{
			return AK_Success;
		}

		// Dynamically load PulseAudio if present and load associated symbols before returning
		void *hLib = dlopen("libpulse.so.0", RTLD_NOW | RTLD_DEEPBIND); // RTLD_DEEPBIND force local symbol to be check first then global
		// Using libpulse.so.0 directly because using libpulse.so (symlink) is only installed by development package
		if (hLib == nullptr) 
		{
			return AK_FileNotFound;
		}

		// Macro returns AK_Fail if symbol is not found
		LOAD_DL_FUNCTION(hLib,pa_threaded_mainloop_new);
		LOAD_DL_FUNCTION(hLib,pa_mainloop_new);
		LOAD_DL_FUNCTION(hLib,pa_mainloop_iterate);
		LOAD_DL_FUNCTION(hLib,pa_mainloop_free);
		LOAD_DL_FUNCTION(hLib,pa_mainloop_get_api);
		LOAD_DL_FUNCTION(hLib,pa_threaded_mainloop_get_api);
		LOAD_DL_FUNCTION(hLib,pa_context_new);
		LOAD_DL_FUNCTION(hLib,pa_context_set_state_callback);
		LOAD_DL_FUNCTION(hLib,pa_threaded_mainloop_lock);
		LOAD_DL_FUNCTION(hLib,pa_threaded_mainloop_unlock);
		LOAD_DL_FUNCTION(hLib,pa_threaded_mainloop_start);
		LOAD_DL_FUNCTION(hLib,pa_context_connect);
		LOAD_DL_FUNCTION(hLib,pa_context_get_state);
		LOAD_DL_FUNCTION(hLib,pa_threaded_mainloop_wait);
		LOAD_DL_FUNCTION(hLib,pa_stream_new);
		LOAD_DL_FUNCTION(hLib,pa_stream_set_state_callback);
		LOAD_DL_FUNCTION(hLib,pa_operation_unref);
		LOAD_DL_FUNCTION(hLib,pa_operation_get_state);
		LOAD_DL_FUNCTION(hLib,pa_stream_cork);
		LOAD_DL_FUNCTION(hLib,pa_threaded_mainloop_stop);
		LOAD_DL_FUNCTION(hLib,pa_stream_disconnect);
		LOAD_DL_FUNCTION(hLib,pa_stream_unref);
		LOAD_DL_FUNCTION(hLib,pa_context_disconnect);
		LOAD_DL_FUNCTION(hLib,pa_context_unref);
		LOAD_DL_FUNCTION(hLib,pa_threaded_mainloop_free);
		LOAD_DL_FUNCTION(hLib,pa_threaded_mainloop_signal);
		LOAD_DL_FUNCTION(hLib,pa_stream_get_state);
		LOAD_DL_FUNCTION(hLib,pa_strerror);
		LOAD_DL_FUNCTION(hLib,pa_context_get_server_info);
		LOAD_DL_FUNCTION(hLib,pa_context_set_event_callback);
		LOAD_DL_FUNCTION(hLib,pa_sample_spec_valid);
		LOAD_DL_FUNCTION(hLib,pa_stream_write);
		LOAD_DL_FUNCTION(hLib,pa_stream_set_underflow_callback);
		LOAD_DL_FUNCTION(hLib,pa_stream_flush);
		LOAD_DL_FUNCTION(hLib,pa_stream_drain);
		LOAD_DL_FUNCTION(hLib,pa_stream_set_write_callback);
		LOAD_DL_FUNCTION(hLib,pa_stream_set_overflow_callback);
		LOAD_DL_FUNCTION(hLib,pa_stream_connect_playback);
		LOAD_DL_FUNCTION(hLib,pa_stream_writable_size);
		LOAD_DL_FUNCTION(hLib,pa_stream_get_buffer_attr);
		LOAD_DL_FUNCTION(hLib,pa_channel_map_valid);
		LOAD_DL_FUNCTION(hLib,pa_stream_begin_write);
		LOAD_DL_FUNCTION(hLib,pa_stream_cancel_write);
		LOAD_DL_FUNCTION(hLib,pa_stream_is_corked);
		LOAD_DL_FUNCTION(hLib,pa_context_get_sink_info_list);

		g_isInitialized = true;

		return AK_Success;
	}

	bool ContextWait(pa_context* ctx, pa_threaded_mainloop* mainloop)
	{
		// Assumes mainloop is locked

		pa_context_state_t state;

		while ((state = pa_context_get_state(ctx)) != PA_CONTEXT_READY)
		{
			if (state == PA_CONTEXT_FAILED || state == PA_CONTEXT_TERMINATED)
				return false;

			pa_threaded_mainloop_wait(mainloop);
		}

		return true;
	}

	bool ContextWait(pa_context* ctx, pa_mainloop* mainloop)
	{
		pa_context_state_t state;

		while ((state = pa_context_get_state(ctx)) != PA_CONTEXT_READY)
		{
			if (state == PA_CONTEXT_FAILED || state == PA_CONTEXT_TERMINATED)
				return false;

			pa_mainloop_iterate(mainloop, 1, nullptr);
		}

		return true;
	}

	bool OperationWait(pa_operation* op, pa_threaded_mainloop* mainloop)
	{
		if (op == nullptr)
			return false;

		pa_operation_state state;

		while ((state = pa_operation_get_state(op)) == PA_OPERATION_RUNNING)
		{
			pa_threaded_mainloop_wait(mainloop);
		}

		pa_operation_unref(op);

		return state == PA_OPERATION_DONE;
	}

	bool OperationWait(pa_operation* op, pa_mainloop* mainloop)
	{
		if (op == nullptr)
			return false;

		pa_operation_state state;

		while ((state = pa_operation_get_state(op)) == PA_OPERATION_RUNNING)
		{
			pa_mainloop_iterate(mainloop, 1, nullptr);
		}

		pa_operation_unref(op);

		return state == PA_OPERATION_DONE;
	}

	bool StreamWait(pa_stream* stream, pa_threaded_mainloop* mainloop)
	{
		// Assumes the mainloop is locked

		pa_stream_state_t state;

		while ((state = pa_stream_get_state(stream)) != PA_STREAM_READY)
		{
			if (state == PA_STREAM_FAILED || state == PA_STREAM_TERMINATED)
				return false;

			pa_threaded_mainloop_wait(mainloop);
		}

		return true;
	}

	// ---------------------------------------------------------------
	// ----------------- PulseAudio sink enumeration -----------------
	// ---------------------------------------------------------------

	class SinkInfoUserData
	{
	public:
		SinkInfoUserData(int in_maxNbDevices, SinkInfo* out_sinkInfo)
			: maxNbDevices(in_maxNbDevices)
			, sinkInfo(out_sinkInfo)
		{
		}

		void SetDefaultDevice(const char* in_defaultDeviceName)
		{
			defaultDeviceID = AK::SoundEngine::GetIDFromString(in_defaultDeviceName);
		}

		void AddDevice(const pa_sink_info* i)
		{
			if (deviceIndex < maxNbDevices)
			{
				SinkInfo& deviceSinkInfo = sinkInfo[deviceIndex];

				deviceSinkInfo.nbChannels = i->sample_spec.channels;
				deviceSinkInfo.deviceID = AK::SoundEngine::GetIDFromString(i->name);
				deviceSinkInfo.isDefaultDevice = deviceSinkInfo.deviceID == defaultDeviceID;
				AKPLATFORM::SafeStrCpy(deviceSinkInfo.name, i->name, AK_MAX_PATH);
				AKPLATFORM::SafeStrCpy(deviceSinkInfo.description, i->description, AK_MAX_PATH);
				deviceSinkInfo.state = i->state;
			}

			deviceIndex++;
		}

		AkUInt32 GetNumberOfDevices() const { return deviceIndex; }

	private:
		int maxNbDevices{ 0 };
		SinkInfo* sinkInfo{ nullptr };
		AkUInt32 deviceIndex{ 0 };
		AkDeviceID defaultDeviceID{ 0 };
	};

	void ServerInfoCallback(pa_context* ctx, const pa_server_info* serverInfo, void* userdata)
	{
		SinkInfoUserData* info = reinterpret_cast<SinkInfoUserData*>(userdata);

		info->SetDefaultDevice(serverInfo->default_sink_name);
	}

	void SinkInfoCallback(pa_context* c, const pa_sink_info* i, int eol, void* userdata)
	{
		if (eol > 0)
			return;

		SinkInfoUserData* info = reinterpret_cast<SinkInfoUserData*>(userdata);
		info->AddDevice(i);
	}

	AKRESULT GetSinkInfoList(AkUInt32 maxNbSinks, SinkInfo* out_sinkInfo, AkUInt32& out_realNbSinks)
	{
		// Lifetime management of PulseAudio objects
		struct PulseAudioObjects
		{
			~PulseAudioObjects()
			{
				if (context != nullptr)
				{
					if (PA_CONTEXT_IS_GOOD(pa_context_get_state(context)))
					{
						pa_context_disconnect(context);
					}
					pa_context_unref(context);
				}

				if (mainloop != nullptr)
				{
					pa_mainloop_free(mainloop);
				}
			}

			pa_mainloop* mainloop{ nullptr };
			pa_mainloop_api* api{ nullptr };
			pa_context* context{ nullptr };
		};

		PulseAudioAPI::Load();

		PulseAudioObjects pa; // manages object destruction

		if ((pa.mainloop = pa_mainloop_new()) == nullptr)
			return AK_Fail;

		if ((pa.api = pa_mainloop_get_api(pa.mainloop)) == nullptr)
			return AK_Fail;

		if ((pa.context = pa_context_new(pa.api, "CAkLEngine::GetPlatformDeviceList")) == nullptr)
			return AK_Fail;

		if (pa_context_connect(pa.context, nullptr, PA_CONTEXT_NOFLAGS, nullptr) < 0 ||
			PulseAudioAPI::ContextWait(pa.context, pa.mainloop) == false)
		{
			return AK_Fail;
		}

		SinkInfoUserData info(maxNbSinks, out_sinkInfo);

		// Get default device
		pa_context_get_server_info(pa.context, ServerInfoCallback, reinterpret_cast<void*>(&info));

		pa_operation* op = pa_context_get_sink_info_list(pa.context, SinkInfoCallback, reinterpret_cast<void*>(&info));

		if (PulseAudioAPI::OperationWait(op, pa.mainloop) == false)
			return AK_Fail;

		out_realNbSinks = info.GetNumberOfDevices();

		return AK_Success;
	}
}
