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

#include <AK/SoundEngine/Common/AkSoundEngine.h>

#include "AkSettings.h"

#include <alsa/asoundlib.h>

// TODO: complete migration to CAkSinkBase like CAkPulseAudioSink

class CAkAlsaSink : public AK::IAkSinkPlugin, public IDynamicAPI
{
public:
	CAkAlsaSink();
	
	virtual ~CAkAlsaSink();
	
	virtual AKRESULT Init( 
		AK::IAkPluginMemAlloc* in_pAllocator,
		AK::IAkSinkPluginContext* in_pSinkPluginContext,
		AK::IAkPluginParam* in_pParams,
		AkAudioFormat& io_rFormat
	);
	
	virtual AKRESULT Term( 
		AK::IAkPluginMemAlloc* in_pAllocator
	);
	
	virtual AKRESULT Reset();
	
	virtual AKRESULT DynamicLoadAPI();

	virtual void Consume(
		AkAudioBuffer* in_pInputBuffer,
		AkRamp in_gain
	);
	virtual void OnFrameEnd();
	
	virtual bool IsStarved();
	
	virtual void ResetStarved();
	
	virtual AKRESULT GetPluginInfo(AkPluginInfo& out_rPluginInfo);

	virtual AKRESULT IsDataNeeded(AkUInt32 & out_uBuffersNeeded);

	// TODO: Avoid public data (currently necessary to expose to static callback functions)

	AK::IAkSinkPluginContext* m_pSinkPluginContext;
	unsigned int m_uOutNumChannels;
	unsigned int* m_pChannelMap;
	AkUInt32 m_uNumBuffers;
	AkChannelMask m_uChannelMask;
	AkUInt16 m_uFrameSize;
	AkInt16* m_pfSinkOutBuffer;
	AkUInt32 m_uUnderflowCount;
	AkUInt32 m_uSampleRate;
	bool m_bDataReady;
	bool m_bIsPrimary;
	bool m_bIsRunning;   

	snd_pcm_t* m_pPcmHandle; // Alsa pcm handle
	snd_pcm_hw_params_t* m_phwparams; // Alsa hardware configuration parameters
	snd_pcm_sw_params_t* m_pswparams; // Alsa software configuration parameters
	AkUInt32 m_uOutputID;

	// Asynchronous
	CAkLock m_threadlock;
	bool m_bThreadRun;
	bool m_bCustomThreadEnable;
	snd_async_handler_t* m_pPcm_callback_handle;
	AkThread m_Alsathread;
};
