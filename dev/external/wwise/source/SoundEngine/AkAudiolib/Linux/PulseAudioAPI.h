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

#include "IDynamicAPI.h"

#include <pulse/pulseaudio.h>

// Declare extern _import_function_name for users of PulseAudioAPI.h
// Definitions are in PulseAudioAPI.cpp

DECLARE_EXTERN_DL_FUNCTION(pa_threaded_mainloop_new);
DECLARE_EXTERN_DL_FUNCTION(pa_mainloop_new);
DECLARE_EXTERN_DL_FUNCTION(pa_mainloop_iterate);
DECLARE_EXTERN_DL_FUNCTION(pa_mainloop_free);
DECLARE_EXTERN_DL_FUNCTION(pa_mainloop_get_api);
DECLARE_EXTERN_DL_FUNCTION(pa_threaded_mainloop_get_api);
DECLARE_EXTERN_DL_FUNCTION(pa_context_new);
DECLARE_EXTERN_DL_FUNCTION(pa_context_set_state_callback);
DECLARE_EXTERN_DL_FUNCTION(pa_threaded_mainloop_lock);
DECLARE_EXTERN_DL_FUNCTION(pa_threaded_mainloop_unlock);
DECLARE_EXTERN_DL_FUNCTION(pa_threaded_mainloop_start);
DECLARE_EXTERN_DL_FUNCTION(pa_context_connect);
DECLARE_EXTERN_DL_FUNCTION(pa_context_get_state);
DECLARE_EXTERN_DL_FUNCTION(pa_threaded_mainloop_wait);
DECLARE_EXTERN_DL_FUNCTION(pa_stream_new);
DECLARE_EXTERN_DL_FUNCTION(pa_stream_set_state_callback);
DECLARE_EXTERN_DL_FUNCTION(pa_operation_unref);
DECLARE_EXTERN_DL_FUNCTION(pa_operation_get_state);
DECLARE_EXTERN_DL_FUNCTION(pa_stream_cork);
DECLARE_EXTERN_DL_FUNCTION(pa_threaded_mainloop_stop);
DECLARE_EXTERN_DL_FUNCTION(pa_stream_disconnect);
DECLARE_EXTERN_DL_FUNCTION(pa_stream_unref);
DECLARE_EXTERN_DL_FUNCTION(pa_context_disconnect);
DECLARE_EXTERN_DL_FUNCTION(pa_context_unref);
DECLARE_EXTERN_DL_FUNCTION(pa_threaded_mainloop_free);
DECLARE_EXTERN_DL_FUNCTION(pa_threaded_mainloop_signal);
DECLARE_EXTERN_DL_FUNCTION(pa_stream_get_state);
DECLARE_EXTERN_DL_FUNCTION(pa_strerror);
DECLARE_EXTERN_DL_FUNCTION(pa_context_get_server_info);
DECLARE_EXTERN_DL_FUNCTION(pa_context_set_event_callback);
DECLARE_EXTERN_DL_FUNCTION(pa_sample_spec_valid);
DECLARE_EXTERN_DL_FUNCTION(pa_stream_write);
DECLARE_EXTERN_DL_FUNCTION(pa_stream_set_underflow_callback);
DECLARE_EXTERN_DL_FUNCTION(pa_stream_flush);
DECLARE_EXTERN_DL_FUNCTION(pa_stream_drain);
DECLARE_EXTERN_DL_FUNCTION(pa_stream_set_write_callback);
DECLARE_EXTERN_DL_FUNCTION(pa_stream_set_overflow_callback);
DECLARE_EXTERN_DL_FUNCTION(pa_stream_connect_playback);
DECLARE_EXTERN_DL_FUNCTION(pa_stream_writable_size);
DECLARE_EXTERN_DL_FUNCTION(pa_stream_get_buffer_attr);
DECLARE_EXTERN_DL_FUNCTION(pa_stream_is_corked);
DECLARE_EXTERN_DL_FUNCTION(pa_channel_map_valid);
DECLARE_EXTERN_DL_FUNCTION(pa_stream_begin_write);
DECLARE_EXTERN_DL_FUNCTION(pa_stream_cancel_write);
DECLARE_EXTERN_DL_FUNCTION(pa_context_get_sink_info_list);

// Remap standard PulseAudio symbols to dynamically imported symbols (so that the rest of the code remains the same)

#define pa_threaded_mainloop_new _import_pa_threaded_mainloop_new
#define pa_mainloop_new _import_pa_mainloop_new
#define pa_mainloop_iterate _import_pa_mainloop_iterate
#define pa_mainloop_free _import_pa_mainloop_free
#define pa_mainloop_get_api _import_pa_mainloop_get_api
#define pa_threaded_mainloop_get_api _import_pa_threaded_mainloop_get_api
#define pa_context_new _import_pa_context_new
#define pa_context_set_state_callback _import_pa_context_set_state_callback
#define pa_threaded_mainloop_lock _import_pa_threaded_mainloop_lock
#define pa_threaded_mainloop_unlock _import_pa_threaded_mainloop_unlock
#define pa_threaded_mainloop_start _import_pa_threaded_mainloop_start
#define pa_context_connect _import_pa_context_connect
#define pa_context_get_state _import_pa_context_get_state
#define pa_threaded_mainloop_wait _import_pa_threaded_mainloop_wait
#define pa_stream_new _import_pa_stream_new
#define pa_stream_set_state_callback _import_pa_stream_set_state_callback
#define pa_operation_unref _import_pa_operation_unref
#define pa_operation_get_state _import_pa_operation_get_state
#define pa_stream_cork _import_pa_stream_cork
#define pa_threaded_mainloop_stop _import_pa_threaded_mainloop_stop
#define pa_stream_disconnect _import_pa_stream_disconnect
#define pa_stream_unref _import_pa_stream_unref
#define pa_context_disconnect _import_pa_context_disconnect
#define pa_context_unref _import_pa_context_unref
#define pa_threaded_mainloop_free _import_pa_threaded_mainloop_free
#define pa_threaded_mainloop_signal _import_pa_threaded_mainloop_signal
#define pa_stream_get_state _import_pa_stream_get_state
#define pa_strerror _import_pa_strerror
#define pa_context_get_server_info _import_pa_context_get_server_info
#define pa_context_set_event_callback _import_pa_context_set_event_callback
#define pa_sample_spec_valid _import_pa_sample_spec_valid
#define pa_stream_write _import_pa_stream_write
#define pa_stream_set_underflow_callback _import_pa_stream_set_underflow_callback
#define pa_stream_flush _import_pa_stream_flush
#define pa_stream_drain _import_pa_stream_drain
#define pa_stream_set_write_callback _import_pa_stream_set_write_callback
#define pa_stream_set_overflow_callback _import_pa_stream_set_overflow_callback
#define pa_stream_connect_playback _import_pa_stream_connect_playback
#define pa_stream_writable_size _import_pa_stream_writable_size
#define pa_stream_get_buffer_attr _import_pa_stream_get_buffer_attr
#define pa_channel_map_valid _import_pa_channel_map_valid
#define pa_stream_begin_write _import_pa_stream_begin_write
#define pa_stream_cancel_write _import_pa_stream_cancel_write
#define pa_stream_is_corked _import_pa_stream_is_corked
#define pa_context_get_sink_info_list _import_pa_context_get_sink_info_list

namespace PulseAudioAPI
{
	AKRESULT Load();

	bool ContextWait(pa_context* ctx, pa_threaded_mainloop* mainloop);

	bool ContextWait(pa_context* ctx, pa_mainloop* mainloop);

	bool OperationWait(pa_operation* op, pa_threaded_mainloop* mainloop);

	bool OperationWait(pa_operation* op, pa_mainloop* mainloop);

	bool StreamWait(pa_stream* stream, pa_threaded_mainloop* mainloop);

	// Important members of pa_sink_info
	struct SinkInfo
	{
		char name[AK_MAX_PATH];
		char description[AK_MAX_PATH];
		int nbChannels{ 0 };
		pa_sink_state state{ PA_SINK_INVALID_STATE };
		AkDeviceID deviceID{ AK_INVALID_DEVICE_ID };
		bool isDefaultDevice{ false };
	};

	// Returns the real number of sinks
	AKRESULT GetSinkInfoList(AkUInt32 maxNbSinks, SinkInfo* out_sinkInfo, AkUInt32& out_realNbSinks);

	// To make sure we always release the lock, else pa_threaded_mainloop_stop will hang
	class ThreadedMainloopLock
	{
	public:
		ThreadedMainloopLock(pa_threaded_mainloop* mainloop)
			: mainloop(mainloop)
		{
			pa_threaded_mainloop_lock(mainloop);
		}

		~ThreadedMainloopLock()
		{
			pa_threaded_mainloop_unlock(mainloop);
		}
		
	private:
		pa_threaded_mainloop* mainloop;
	};
}
