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

#include "AkSinkALSA.h"

#include <AK/AkWwiseSDKVersion.h>
#include <AK/SoundEngine/Common/AkSoundEngine.h>

#include <alsa/pcm.h>
#include <sched.h>

extern AkPlatformInitSettings g_PDSettings;

#define AKALSASINK_MAXCHANNEL 16 //FTReview: revisit this (switch to dynamic memory alloc?)
#define AKALSASINK_DEFAULT_NBCHAN 2
#define AKALSASINK_MIN_NB_OUTBUFFER 2
#define AKALSASINK_DEFAULTDEVICENAME "default" // "hw:0" // Init settings don't provide device name, use default as defined in ALSA config
// Note1: that asynchronous API may not be provided on all devices, i.e. not on the pulse audio device which does not implement async API
// Note2: the use of "default" which is a virtual device routing to another device is also subject to this (i.e. will work if routed to hw:0 but not if routed to pulse)

// Declare global names for dynamically imported PA API functions
DEFINE_DL_FUNCTION(snd_pcm_open);
DEFINE_DL_FUNCTION(snd_pcm_close);
DEFINE_DL_FUNCTION(snd_pcm_nonblock);
DEFINE_DL_FUNCTION(snd_async_add_pcm_handler);
DEFINE_DL_FUNCTION(snd_async_handler_get_pcm);
DEFINE_DL_FUNCTION(snd_async_handler_get_callback_private);
DEFINE_DL_FUNCTION(snd_pcm_hw_params);
DEFINE_DL_FUNCTION(snd_pcm_hw_params_malloc);
DEFINE_DL_FUNCTION(snd_pcm_sw_params_current);
DEFINE_DL_FUNCTION(snd_pcm_sw_params);
DEFINE_DL_FUNCTION(snd_pcm_sw_params_malloc);
DEFINE_DL_FUNCTION(snd_pcm_prepare);
DEFINE_DL_FUNCTION(snd_pcm_drop);
DEFINE_DL_FUNCTION(snd_pcm_resume);
DEFINE_DL_FUNCTION(snd_pcm_avail);
DEFINE_DL_FUNCTION(snd_pcm_avail_update);
DEFINE_DL_FUNCTION(snd_pcm_writei);
DEFINE_DL_FUNCTION(snd_pcm_hw_params_any);
DEFINE_DL_FUNCTION(snd_pcm_hw_params_set_access);
DEFINE_DL_FUNCTION(snd_pcm_hw_params_set_format);
DEFINE_DL_FUNCTION(snd_pcm_hw_params_set_rate_near);
DEFINE_DL_FUNCTION(snd_pcm_query_chmaps);
DEFINE_DL_FUNCTION(snd_pcm_hw_params_set_channels);
DEFINE_DL_FUNCTION(snd_pcm_hw_params_set_buffer_size_near);
DEFINE_DL_FUNCTION(snd_pcm_hw_params_set_periods_near);
DEFINE_DL_FUNCTION(snd_pcm_set_chmap);
DEFINE_DL_FUNCTION(snd_pcm_free_chmaps);
DEFINE_DL_FUNCTION(snd_pcm_sw_params_set_start_threshold);
DEFINE_DL_FUNCTION(snd_pcm_sw_params_set_avail_min);
DEFINE_DL_FUNCTION(snd_strerror);
DEFINE_DL_FUNCTION(snd_pcm_hw_params_free);
DEFINE_DL_FUNCTION(snd_pcm_sw_params_free);

#define snd_pcm_open _import_snd_pcm_open
#define snd_pcm_close _import_snd_pcm_close
#define snd_pcm_nonblock _import_snd_pcm_nonblock
#define snd_async_add_pcm_handler _import_snd_async_add_pcm_handler
#define snd_async_handler_get_pcm _import_snd_async_handler_get_pcm
#define snd_async_handler_get_callback_private _import_snd_async_handler_get_callback_private
#define snd_pcm_hw_params _import_snd_pcm_hw_params
#define snd_pcm_hw_params_malloc _import_snd_pcm_hw_params_malloc
#define snd_pcm_sw_params_current _import_snd_pcm_sw_params_current
#define snd_pcm_sw_params _import_snd_pcm_sw_params
#define snd_pcm_sw_params_malloc _import_snd_pcm_sw_params_malloc
#define snd_pcm_prepare _import_snd_pcm_prepare
#define snd_pcm_drop _import_snd_pcm_drop
#define snd_pcm_resume _import_snd_pcm_resume
#define snd_pcm_avail _import_snd_pcm_avail
#define snd_pcm_avail_update _import_snd_pcm_avail_update
#define snd_pcm_writei _import_snd_pcm_writei
#define snd_pcm_hw_params_any _import_snd_pcm_hw_params_any
#define snd_pcm_hw_params_set_access _import_snd_pcm_hw_params_set_access
#define snd_pcm_hw_params_set_format _import_snd_pcm_hw_params_set_format
#define snd_pcm_hw_params_set_rate_near _import_snd_pcm_hw_params_set_rate_near
#define snd_pcm_query_chmaps _import_snd_pcm_query_chmaps
#define snd_pcm_hw_params_set_channels _import_snd_pcm_hw_params_set_channels
#define snd_pcm_hw_params_set_buffer_size_near _import_snd_pcm_hw_params_set_buffer_size_near
#define snd_pcm_hw_params_set_periods_near _import_snd_pcm_hw_params_set_periods_near
#define snd_pcm_set_chmap _import_snd_pcm_set_chmap
#define snd_pcm_free_chmaps _import_snd_pcm_free_chmaps
#define snd_pcm_sw_params_set_start_threshold _import_snd_pcm_sw_params_set_start_threshold
#define snd_pcm_sw_params_set_avail_min _import_snd_pcm_sw_params_set_avail_min
#define snd_strerror _import_snd_strerror
#define snd_pcm_hw_params_free _import_snd_pcm_hw_params_free
#define snd_pcm_sw_params_free _import_snd_pcm_sw_params_free

AKRESULT CAkAlsaSink::DynamicLoadAPI()
{
	// Dynamically load ALSA if present and load associated symbols before returning
	void *hLib = dlopen("libasound.so.2", RTLD_NOW | RTLD_DEEPBIND); //RTLD_DEEPBIND force local symbol to be check first then global
	// Using libasound.so.2 directly because using libasound.so (symlink) is only installed by development package
	if (hLib == NULL) 
	{
		return AK_FileNotFound;
	}

	// Macro returns AK_Fail if symbol is not found
	LOAD_DL_FUNCTION(hLib, snd_pcm_open);
	LOAD_DL_FUNCTION(hLib, snd_pcm_close);
	LOAD_DL_FUNCTION(hLib, snd_pcm_nonblock);
	LOAD_DL_FUNCTION(hLib, snd_async_add_pcm_handler);
	LOAD_DL_FUNCTION(hLib, snd_async_handler_get_pcm);
	LOAD_DL_FUNCTION(hLib, snd_async_handler_get_callback_private)
	LOAD_DL_FUNCTION(hLib, snd_pcm_hw_params);
	LOAD_DL_FUNCTION(hLib, snd_pcm_hw_params_malloc);
	LOAD_DL_FUNCTION(hLib, snd_pcm_sw_params_current);
	LOAD_DL_FUNCTION(hLib, snd_pcm_sw_params);
	LOAD_DL_FUNCTION(hLib, snd_pcm_sw_params_malloc);
	LOAD_DL_FUNCTION(hLib, snd_pcm_prepare);
	LOAD_DL_FUNCTION(hLib, snd_pcm_drop);
	LOAD_DL_FUNCTION(hLib, snd_pcm_resume);
	LOAD_DL_FUNCTION(hLib, snd_pcm_avail);
	LOAD_DL_FUNCTION(hLib, snd_pcm_avail_update);
	LOAD_DL_FUNCTION(hLib, snd_pcm_writei);
	LOAD_DL_FUNCTION(hLib, snd_pcm_hw_params_any);
	LOAD_DL_FUNCTION(hLib, snd_pcm_hw_params_set_access);
	LOAD_DL_FUNCTION(hLib, snd_pcm_hw_params_set_format);
	LOAD_DL_FUNCTION(hLib, snd_pcm_hw_params_set_rate_near);
	LOAD_DL_FUNCTION(hLib, snd_pcm_query_chmaps);
	LOAD_DL_FUNCTION(hLib, snd_pcm_hw_params_set_channels);
	LOAD_DL_FUNCTION(hLib, snd_pcm_hw_params_set_buffer_size_near);
	LOAD_DL_FUNCTION(hLib, snd_pcm_hw_params_set_periods_near);
	LOAD_DL_FUNCTION(hLib, snd_pcm_set_chmap);
	LOAD_DL_FUNCTION(hLib, snd_pcm_free_chmaps);
	LOAD_DL_FUNCTION(hLib, snd_pcm_sw_params_set_start_threshold);
	LOAD_DL_FUNCTION(hLib, snd_pcm_sw_params_set_avail_min);
	LOAD_DL_FUNCTION(hLib, snd_strerror);
	LOAD_DL_FUNCTION(hLib, snd_pcm_hw_params_free);
	LOAD_DL_FUNCTION(hLib, snd_pcm_sw_params_free);

	return AK_Success;
}

static int alsa_error_recovery(void *in_puserdata, snd_pcm_t *in_phandle, int in_err)
{
	CAkAlsaSink *  pAlsaSinkDevice = (CAkAlsaSink *)in_puserdata;
	int returnVal=0;

	if (in_err == -EPIPE) //Underrun
	{
		pAlsaSinkDevice->m_uUnderflowCount++;
		returnVal = snd_pcm_prepare(in_phandle);
		return returnVal;
	}
	else if (in_err == -ESTRPIPE) //Suspend
	{
		while ((returnVal = snd_pcm_resume(in_phandle)) == -EAGAIN)
		{
			sleep(1);       /* wait until the suspend flag is released */
		}

		if (returnVal < 0)
		{
			returnVal = snd_pcm_prepare(in_phandle);
		}

		return returnVal;
	}

	return in_err;
}

// Preferred API is asynchronous using callback mechanism below. However, some devices (notably pulse) don't implement it, so in these cases
// we need a thread to periodically request data from the sound engine
AK_DECLARE_THREAD_ROUTINE(AlsaSinkAudioThread)
{
	CAkAlsaSink *  pAlsaSinkDevice = (CAkAlsaSink*)lpParameter;
	AK_INSTRUMENT_THREAD_START( "AlsaSinkAudioThread" );

	unsigned int uSampleRate = pAlsaSinkDevice->m_uSampleRate;
	unsigned int uWaitSleepTimeUs = ((float)pAlsaSinkDevice->m_uFrameSize/(float)uSampleRate)*1000000/3;
	unsigned int uCompSleepTimeUs = 0;

	while(pAlsaSinkDevice->m_bThreadRun)
	{
		snd_pcm_sframes_t frameAvail;

		//Check how much buffer space we have
		frameAvail = snd_pcm_avail(pAlsaSinkDevice->m_pPcmHandle);
		if(frameAvail< 0)
		{
			;
			/*returnVal = alsa_error_recovery(pAlsaSinkDevice, pAlsaSinkDevice->m_pPcmHandle, frameAvail);
			if(returnVal < 0)
			{
				frameAvail = 0;
				pAlsaSinkDevice->m_bThreadRun = false;
				break;
			}
			else
			{
				//Check again how much space is available
				frameAvail = snd_pcm_avail_update(pAlsaSinkDevice->m_pPcmHandle);
				if(frameAvail<0)
				{
					//After prepare not supposed to underrun again
					frameAvail = 0;
				}
			}*/
		}

		if (frameAvail >= pAlsaSinkDevice->m_uFrameSize)
		{
			unsigned int nbFrames = frameAvail/pAlsaSinkDevice->m_uFrameSize;
			//Generate SignalThread
			if(pAlsaSinkDevice->m_bIsPrimary == true)
			{
				pAlsaSinkDevice->m_pSinkPluginContext->SignalAudioThread();
			}
			uCompSleepTimeUs = (unsigned int)((((float)(nbFrames*pAlsaSinkDevice->m_uFrameSize)/(float)uSampleRate)*1000000)) - uWaitSleepTimeUs;
			usleep(uCompSleepTimeUs);
		}
		else
		{
			//Wait for next blocksize
			usleep(uWaitSleepTimeUs);
		}
	}
	AkExitThread(AK_RETURN_THREAD_OK);
}

//Callback to check audio avaibility and wake up Wwise audio thread
void asyncAudioCb(snd_async_handler_t *pPcmCallback)
{
	if(pPcmCallback!=NULL)
	{
		CAkAlsaSink *pAlsaSinkDevice = (CAkAlsaSink*)snd_async_handler_get_callback_private(pPcmCallback);

		//Need to be protect, Alsa is not thread safe.
		pAlsaSinkDevice->m_threadlock.Lock();
		snd_pcm_t *pPcmHandle = snd_async_handler_get_pcm(pPcmCallback);
		snd_pcm_sframes_t frameAvail;
		int returnVal;

		//Check how much buffer space we have
		frameAvail = snd_pcm_avail_update(pPcmHandle);
		if(frameAvail< 0)
		{
			returnVal = alsa_error_recovery(pAlsaSinkDevice, pPcmHandle, frameAvail);
			if(returnVal < 0)
			{
				frameAvail = 0;
			}
			else
			{
				//Check again how much space is available
				frameAvail = snd_pcm_avail_update(pPcmHandle);
				if(frameAvail<0)
				{
					//After prepare not supposed to underrun again
					frameAvail = 0;
				}
			}
		}

		if (frameAvail >= pAlsaSinkDevice->m_uFrameSize)
		{
			if(pAlsaSinkDevice->m_bIsPrimary == true)
			{
				pAlsaSinkDevice->m_pSinkPluginContext->SignalAudioThread();
			}
		}

		pAlsaSinkDevice->m_threadlock.Unlock();
	}
}

// Initializes the ALSA
static AKRESULT alsa_stream_connect(void *in_puserdata)
{
	CAkAlsaSink *  pAlsaSinkDevice = (CAkAlsaSink *)in_puserdata;

	// Name of the PCM device, like plughw:0,0, hw:0,0, default, pulse, etc
	// The first number is the number of the soundcard,
	// the second number is the number of the device.

	// Open PCM. The last parameter of this function is the mode.
	// If this is set to 0, the standard mode is used. Possible
	// other values are SND_PCM_NONBLOCK and SND_PCM_ASYNC.
	// If SND_PCM_NONBLOCK is used, read / write access to the
	// PCM device will return immediately. If SND_PCM_ASYNC is
	// specified, SIGIO will be emitted whenever a period has
	// been completely processed by the soundcard.
	snd_pcm_t *pPcmHandle = NULL;	// Handle for the PCM device
	int retValue = snd_pcm_open(&pPcmHandle, AKALSASINK_DEFAULTDEVICENAME, SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
	if (retValue  < 0 || (pPcmHandle ==NULL))
	{
	  return AK_Fail;
	}

  	// Init hwparams with full configuration space
	retValue = snd_pcm_hw_params_any(pPcmHandle, pAlsaSinkDevice->m_phwparams);
  	if (retValue < 0)
	{
		return AK_Fail;
	}

	//Set access type
	//Possible value are SND_PCM_ACCESS_RW_INTERLEAVED or SND_PCM_ACCESS_RW_NONINTERLEAVED
	retValue = snd_pcm_hw_params_set_access(pPcmHandle, pAlsaSinkDevice->m_phwparams, SND_PCM_ACCESS_RW_INTERLEAVED);
	if (retValue < 0)
	{
	  return AK_Fail;
	}

	// Set sample format
	retValue = snd_pcm_hw_params_set_format(pPcmHandle, pAlsaSinkDevice->m_phwparams, SND_PCM_FORMAT_S16_LE);
	if(retValue < 0)
	{
	  return AK_Fail;
	}

	// Set sample rate. If the exact rate is not supported
	// by the hardware, use nearest possible rate.
	// Use near to be able to see hardware closest supported rate even if we only accept the exact rate
	unsigned int uRate = pAlsaSinkDevice->m_uSampleRate;	//Get Wwise sample rate
	unsigned int uSampleRate = uRate; // exact rate returned by ALSA
	retValue = snd_pcm_hw_params_set_rate_near(pPcmHandle, pAlsaSinkDevice->m_phwparams, &uSampleRate, 0);
	if (retValue  < 0)
	{
	  return AK_Fail;
	}

	if (uRate != uSampleRate)
	{
		return AK_Fail;
	}

	snd_pcm_chmap_query_t **ppChMaps = NULL;
	snd_pcm_chmap_t *FoundChMap = NULL;

	ppChMaps = snd_pcm_query_chmaps(pPcmHandle);
	if (ppChMaps == NULL)
	{
		// "Unable to detect number of audio channels supported by the Device. Stereo mode is selected by default"
		//Channel mapping is not set because no api to set channel mapping available.
		//Assuming channel mapping in stereo mode will match Wwise stereo channel mapping
		pAlsaSinkDevice->m_uOutNumChannels = AKALSASINK_DEFAULT_NBCHAN;
		pAlsaSinkDevice->m_uChannelMask = AK::ChannelMaskFromNumChannels(pAlsaSinkDevice->m_uOutNumChannels);
	}
	else
	{
		FoundChMap = &ppChMaps[0]->map; // Take first one as default, until we encounter something better

		// Auto-detect mode (pick first channel map)
		if(pAlsaSinkDevice->m_uOutNumChannels ==0)
		{
			//In Auto-detect mode we select the first channel mapping available.
			pAlsaSinkDevice->m_uOutNumChannels = FoundChMap->channels;
			pAlsaSinkDevice->m_uChannelMask = AK::ChannelMaskFromNumChannels(FoundChMap->channels);

		}
		else // Looking for suggested number of channels (i.e. not channel mask specific)
		{
			bool bChannelMapConfigFound = false;
			int iIndexStereo = -1; // Keep an index for stereo channel map (fallback option)
			snd_pcm_chmap_query_t *pChMap = ppChMaps[0];
			int i=0;
			//ppChMaps is always terminated with NULL
			while(pChMap!=NULL)
			{
				if (pChMap->map.channels == 2)
				{
					iIndexStereo = i; // Cache the index of this map for stereo fallback option
				}

				if(pChMap->map.channels == pAlsaSinkDevice->m_uOutNumChannels)
				{
					bChannelMapConfigFound = true;
					FoundChMap = &ppChMaps[i]->map;
					pChMap = NULL;
				}
				else
				{
					i++;
					pChMap = ppChMaps[i];
				}
			}

			if(bChannelMapConfigFound == false)
			{
				// Audio device does not support number of audio channels
				// Policy, pick stereo channel map if found otherwise //  use first channel map (typically ordered by number of channels)
				if (iIndexStereo != -1) 
				{
					FoundChMap = &ppChMaps[iIndexStereo]->map;
					pAlsaSinkDevice->m_uOutNumChannels = FoundChMap->channels;
					pAlsaSinkDevice->m_uChannelMask = AK::ChannelMaskFromNumChannels(FoundChMap->channels);
				}
			}
		}

		if(pAlsaSinkDevice->m_uOutNumChannels > AKALSASINK_MAXCHANNEL )
		{
			// Alsa sink supported a maximum off 16 channels (FTReview: revisit this)
			return AK_Fail;
		}
	}

	// Set number of channels
	retValue = snd_pcm_hw_params_set_channels(pPcmHandle,pAlsaSinkDevice->m_phwparams, pAlsaSinkDevice->m_uOutNumChannels);
	if (retValue < 0)
	{
	  return AK_Fail;
	}

	//buffer_size = periodsize * periods
	//Latency important parameters
	//The latency is calculated as follow:
	//latency = periodsize*period / (rate * bytes_per_frame)
	//Typical example is as follow:
	//period=2; periodsize=8192 bytes; rate=48000Hz; 16 bits stereo data (bytes_per_frame=4bytes)
	//latency = 8192*2/(48000*4) = 85.3 ms of latency
	snd_pcm_uframes_t buffer_size = pAlsaSinkDevice->m_uFrameSize*pAlsaSinkDevice->m_uNumBuffers;
	//Set the buffer size (may be changed by the library)
	retValue = snd_pcm_hw_params_set_buffer_size_near (pPcmHandle, pAlsaSinkDevice->m_phwparams, &buffer_size);
	if (retValue  < 0) {
	  return AK_Fail;
	}

	unsigned int period = pAlsaSinkDevice->m_uNumBuffers;
	int direction = 0; // Set period to at least the desired parameter buffering
	retValue = snd_pcm_hw_params_set_periods_near(pPcmHandle, pAlsaSinkDevice->m_phwparams, &period, &direction);
	if (retValue < 0)
	{
		return AK_Fail;
	}

	// Apply HW parameter and prepare device
	retValue = snd_pcm_hw_params(pPcmHandle, pAlsaSinkDevice->m_phwparams);
	if (retValue < 0)
	{
		return AK_Fail;
	}

	//Assign pcmHandle
	pAlsaSinkDevice->m_pPcmHandle = pPcmHandle;

	if(FoundChMap!= NULL)
	{
		//Wwise standard mapping L-R-C-LFE-RL-RR-RC-SL-SR-HL-HR-HC-HRL-HRR-HRC-T
		//In order to support up to 16 channels
		//Always open the stream with Wwise Standard channel mapping, since it does not change based on channel number
		//The we use m_pChannelMap to remap into run time mapping.
		//Since the remapping does not cost an extra memcpy this solution should keep the same performance.
		//Another solution will be to directly map into Wwise runtime mapping but this has a lot of case to handle.
		if(pAlsaSinkDevice->m_uOutNumChannels > 1) //Channel map only for stereo and up.
		{
			for(unsigned int y=0; y<pAlsaSinkDevice->m_uOutNumChannels; y++)
			{
				switch(y)
				{
					case 0:
							FoundChMap->pos[y] = SND_CHMAP_FL;
							break;
					case 1:
							FoundChMap->pos[y]  = SND_CHMAP_FR;
							break;
					case 2:
							FoundChMap->pos[y]  = SND_CHMAP_FC;
							break;
					case 3:
							FoundChMap->pos[y]  = SND_CHMAP_LFE;
							break;
					case 4:
							FoundChMap->pos[y]  = SND_CHMAP_RL;
							break;
					case 5:
							FoundChMap->pos[y]  = SND_CHMAP_RR;
							break;
					case 6:
							FoundChMap->pos[y]  = SND_CHMAP_RC;
							break;
					case 7:
							FoundChMap->pos[y]  = SND_CHMAP_SL;
							break;
					case 8:
							FoundChMap->pos[y]  = SND_CHMAP_SR;
							break;
					case 9:
							FoundChMap->pos[y]  = SND_CHMAP_TFL;
							break;
					case 10:
							FoundChMap->pos[y]  = SND_CHMAP_TFR;
							break;
					case 11:
							FoundChMap->pos[y]  = SND_CHMAP_TFC;
							break;
					case 12:
							FoundChMap->pos[y]  = SND_CHMAP_TRL;
							break;
					case 13:
							FoundChMap->pos[y]  = SND_CHMAP_TRR;
							break;
					case 14:
							FoundChMap->pos[y]  = SND_CHMAP_TRC;
							break;
					case 15:
							FoundChMap->pos[y]  = SND_CHMAP_TC;
							break;
				}
			}

			retValue =  snd_pcm_set_chmap(pPcmHandle, FoundChMap);
		}

		//Free channel maps
		snd_pcm_free_chmaps	(ppChMaps);
	}

	//SW params setting
	retValue = snd_pcm_sw_params_current (pPcmHandle, pAlsaSinkDevice->m_pswparams);
	if(retValue < 0)
	{
		return AK_Fail;
	}

	//Set start value, minimum is 2 frames
	retValue = snd_pcm_sw_params_set_start_threshold(pPcmHandle, pAlsaSinkDevice->m_pswparams, pAlsaSinkDevice->m_uFrameSize*AKALSASINK_MIN_NB_OUTBUFFER);
	if( retValue < 0)
	{
		return AK_Fail;
	}

	retValue = snd_pcm_sw_params_set_avail_min(pPcmHandle, pAlsaSinkDevice->m_pswparams, pAlsaSinkDevice->m_uFrameSize);
	if(retValue < 0)
	{
		return AK_Fail;
	}

	retValue = snd_pcm_sw_params(pPcmHandle, pAlsaSinkDevice->m_pswparams);
	if(retValue < 0)
	{
		return AK_Fail;
	}

	snd_async_handler_t *pcm_callback;
	if(pAlsaSinkDevice->m_bIsPrimary)
	{
		retValue = snd_async_add_pcm_handler(&pcm_callback, pPcmHandle, asyncAudioCb, pAlsaSinkDevice);
		if(retValue < 0)
		{
			//const char * errStr = snd_strerror(retValue);
			pAlsaSinkDevice->m_bCustomThreadEnable = true;
			//ALSA Async Callback API not supported, custom thread will be created
		}
		else
		{
			pAlsaSinkDevice->m_pPcm_callback_handle = pcm_callback;
		}
	}

	//Init stream
	retValue = snd_pcm_prepare (pPcmHandle);
	if (retValue  < 0)
	{
		return AK_Fail;
	}

	return AK_Success;
}


CAkAlsaSink::CAkAlsaSink()
	: m_pSinkPluginContext( NULL )
	, m_uOutNumChannels(0)
	, m_pChannelMap(NULL)
	, m_uNumBuffers(0)
	, m_uChannelMask(0)
	, m_uFrameSize(0)
	, m_pfSinkOutBuffer(NULL)
	, m_uUnderflowCount(0)
	, m_uSampleRate(AK_CORE_SAMPLERATE)
	, m_bDataReady(false)
	, m_bIsPrimary(false)
	, m_bIsRunning(false)
	, m_pPcmHandle(NULL)
	, m_phwparams(NULL)
	, m_pswparams(NULL)
	, m_uOutputID(0)
	, m_bThreadRun(false)
	, m_bCustomThreadEnable(false)
	, m_pPcm_callback_handle(NULL)
	, m_Alsathread(AK_NULL_THREAD)
{
}

CAkAlsaSink::~CAkAlsaSink()
{
}

AKRESULT CAkAlsaSink::Init(
	AK::IAkPluginMemAlloc *		in_pAllocator,			// Interface to memory allocator to be used by the effect.
	AK::IAkSinkPluginContext *	in_pSinkPluginContext,	// Interface to sink plug-in's context.
  	AK::IAkPluginParam *        in_pParams,             // Interface to parameter node
	AkAudioFormat &				io_rFormat				// Audio data format of the input signal. 
	)
{
	m_pSinkPluginContext = in_pSinkPluginContext;
	m_bIsPrimary = m_pSinkPluginContext->IsPrimary();
	AkUInt32 uDeviceType = 0;
	AKRESULT eResult = m_pSinkPluginContext->GetOutputID(m_uOutputID, uDeviceType);
	if (eResult != AK_Success)
	{
		return AK_NotImplemented;
	}

	m_uNumBuffers = g_PDSettings.uNumRefillsInVoice;
	if(m_uNumBuffers < AKALSASINK_MIN_NB_OUTBUFFER)
	{
		// Minimum number of output buffer is 2
		return AK_Fail;
	}

	m_uFrameSize = m_pSinkPluginContext->GlobalContext()->GetMaxBufferLength();

	m_pChannelMap = (unsigned int *)AK_PLUGIN_ALLOC(in_pAllocator, sizeof(unsigned int)*AKALSASINK_MAXCHANNEL);
	if(m_pChannelMap == NULL)
	{
		return AK_InsufficientMemory;
	}

	//Allocate necessary structure for alsa
	//Can't use Ak malloc because snd_pcm_hw_params_t is opaque
	int returnVal=0;
	returnVal = snd_pcm_hw_params_malloc(&m_phwparams);
	if (returnVal < 0)
	{
		return AK_InsufficientMemory;
	}

	returnVal = snd_pcm_sw_params_malloc(&m_pswparams);
	if (returnVal < 0)
	{
		return AK_InsufficientMemory;
	}

	// Attempt to use suggested output configuration, if not possible they will be changed to what device can support
	m_uOutNumChannels = io_rFormat.channelConfig.uNumChannels;
	m_uChannelMask = io_rFormat.channelConfig.uChannelMask;

	eResult = alsa_stream_connect(this);
	if (eResult != AK_Success)
	{
		return AK_Fail;
	}

	if (m_uOutNumChannels == 0)
	{
		return AK_Fail;
	}

	io_rFormat.channelConfig.SetStandardOrAnonymous(m_uOutNumChannels, m_uChannelMask);
	io_rFormat.uBlockAlign = m_uOutNumChannels*sizeof(AkReal32);
	m_uSampleRate = g_PDSettings.uSampleRate;
	io_rFormat.uSampleRate = m_uSampleRate;

	//assign Channel Mapping.
	for (unsigned int i = 0; i < m_uOutNumChannels; i++)
	{
		m_pChannelMap[i] = AK::StdChannelIndexToDisplayIndex(AK::ChannelOrdering_Standard, m_uChannelMask, i);
	}

	m_pfSinkOutBuffer = (int16_t *)AK_PLUGIN_ALLOC(in_pAllocator, m_uFrameSize*sizeof(int16_t)*m_uOutNumChannels);
	if (m_pfSinkOutBuffer == NULL)
	{
		return AK_InsufficientMemory;
	}
	AKPLATFORM::AkMemSet(m_pfSinkOutBuffer, 0, m_uFrameSize*sizeof(int16_t)*m_uOutNumChannels);

	return AK_Success;
}

AKRESULT CAkAlsaSink::Term( AK::IAkPluginMemAlloc * in_pAllocator )
{
	//Stop the thread
	if ( AKPLATFORM::AkIsValidThread( &m_Alsathread ) )
	{
		m_bThreadRun = false;
		AKPLATFORM::AkWaitForSingleThread( &m_Alsathread );
		AKPLATFORM::AkCloseThread( &m_Alsathread );
	}

	if(m_phwparams)
	{
		snd_pcm_hw_params_free(m_phwparams);
		m_phwparams = NULL;
	}

	if(m_pswparams)
	{
		snd_pcm_sw_params_free(m_pswparams);
		m_pswparams = NULL;
	}

	//Drop PCM Stream
	if (m_pPcmHandle)
	{
		//Stopping Alsa Stream, drop all remaining Audio data
		snd_pcm_drop(m_pPcmHandle);
	}

	if(m_pPcm_callback_handle)
	{
		//Only assign to NULL don't need to free the pointer because it will be done by snd_pcm_close below.
		m_pPcm_callback_handle = NULL;
	}

	if(m_pPcmHandle != NULL)
	{
		snd_pcm_close (m_pPcmHandle);
	}
	m_pPcmHandle = NULL;

	if (m_pfSinkOutBuffer)
	{
		AK_PLUGIN_FREE(in_pAllocator, m_pfSinkOutBuffer);
		m_pfSinkOutBuffer = NULL;
	}

	if (m_pChannelMap)
	{
		AK_PLUGIN_FREE(in_pAllocator, m_pChannelMap);
		m_pChannelMap = NULL;
	}

	m_bIsRunning = false;
	
	AK_PLUGIN_DELETE( in_pAllocator, this );
	return AK_Success;
}

AKRESULT CAkAlsaSink::Reset()
{
	if(m_pPcmHandle)
	{
		//Start stream
		if(m_bIsRunning == false )
		{
			if(m_bCustomThreadEnable)
			{
				if(m_bThreadRun == false && m_bIsPrimary == true)
				{
					m_bThreadRun = true;
					AkThreadProperties AlsaThreadProp;

					AKPLATFORM::AkGetDefaultThreadProperties(AlsaThreadProp);
					AlsaThreadProp.nPriority = AK_THREAD_PRIORITY_ABOVE_NORMAL;
					AKPLATFORM::AkCreateThread(AlsaSinkAudioThread, (void*)this, AlsaThreadProp, &m_Alsathread, "AlsaSinkThread");
				}
			}

			m_bIsRunning = true;
		}
	}
	else
	{
		// Starting ALSA device with invalid PCM Handle, device is not started  
		return AK_Fail;
	}

	return AK_Success;
}

bool CAkAlsaSink::IsStarved()
{
	//Only report underflow when it is primary output	
	if(m_bIsPrimary == true)
	{
		return m_uUnderflowCount > 0;
	}

	return false;
}

void CAkAlsaSink::ResetStarved()
{
	m_uUnderflowCount = 0;
}

AKRESULT CAkAlsaSink::GetPluginInfo(AkPluginInfo& out_rPluginInfo)
{
	out_rPluginInfo.eType = AkPluginTypeSink;
	out_rPluginInfo.bIsInPlace = false;
	out_rPluginInfo.uBuildVersion = AK_WWISESDK_VERSION_COMBINED;
	return AK_Success;
}

void CAkAlsaSink::Consume(
	AkAudioBuffer *			in_pInputBuffer,		///< Input audio buffer data structure. Plugins should avoid processing data in-place.
	AkRamp							in_gain					///< Volume gain to apply to this input (prev corresponds to the beginning, next corresponds to the end of the buffer).
	)
{
	if ( in_pInputBuffer->uValidFrames > 0 )
	{
		// Interleave channels and apply volume
		const unsigned int uNumFrames = in_pInputBuffer->uValidFrames;
		const unsigned int uOutStride = m_uOutNumChannels;
		const AkReal32 fGainStart = in_gain.fPrev;
		const AkReal32 fDelta = (in_gain.fNext - in_gain.fPrev) / (AkReal32) uNumFrames;

		// Channels are in AK pipeline order (i.e. LFE at the end)
		for (unsigned int i = 0; i < m_uOutNumChannels; i++)
		{
			float * AK_RESTRICT pfSource = in_pInputBuffer->GetChannel(m_pChannelMap[i]);		
			int16_t * AK_RESTRICT pfDest = m_pfSinkOutBuffer + i; // Channel offset
			AkReal32 fGain = fGainStart;	
			for (unsigned int j = 0; j < uNumFrames; j++)
			{
				const float f = pfSource[j] * fGain;

				//FT Review: Should be SIMD optimized
				if (f >= 1.0f)
					*pfDest = 32767;
				else if (f <= -1.0f)
					*pfDest = -32768;
				else
					*pfDest = (int16_t) (f * 32767.0f);

				pfDest += uOutStride;
				fGain += fDelta;
			}
		}

		m_bDataReady = true;
	}
}


void CAkAlsaSink::OnFrameEnd()
{
	// When no data has been produced (consume is not being called since SE is idle), produce silence to the RB
	if (!m_bDataReady)
	{
		AKPLATFORM::AkMemSet(m_pfSinkOutBuffer, 0, m_uFrameSize*sizeof(int16_t)*m_uOutNumChannels);
	}

	snd_pcm_sframes_t numberFrameAvailable;
	numberFrameAvailable = snd_pcm_avail(m_pPcmHandle);
	if(numberFrameAvailable < 0)
	{
		int returnVal = alsa_error_recovery(this, m_pPcmHandle, numberFrameAvailable);
		if(returnVal<0)
		{
			//Cannot recover from error getting available frames
			m_bDataReady = false;
			return;
		}
		else 
		{
			// Recovered error getting available frames
			numberFrameAvailable = snd_pcm_avail(m_pPcmHandle);
			// Let it go if still no available
		}	
	}

	if(numberFrameAvailable > 0 && numberFrameAvailable >= m_uFrameSize)
	{
		int numberByteWritten=0;

		//this function accept number of frame, not number of bytes
		numberByteWritten = snd_pcm_writei(m_pPcmHandle,m_pfSinkOutBuffer, m_uFrameSize);
		if(numberByteWritten < 0)
		{
			alsa_error_recovery(this, m_pPcmHandle, numberByteWritten);
		}
	}

	m_bDataReady = false;
}


AKRESULT CAkAlsaSink::IsDataNeeded( AkUInt32 & out_uBuffersNeeded )
{
	// This will only get called on the "main" sink
	out_uBuffersNeeded = 0;
	int returnVal=0;

	// Query ALsa audio if it can accept more Wwise audio frames
	snd_pcm_sframes_t numberFrameAvailable;
	numberFrameAvailable = snd_pcm_avail_update(m_pPcmHandle);
	if(numberFrameAvailable < 0)
	{
		returnVal = alsa_error_recovery(this, m_pPcmHandle, numberFrameAvailable);
		if(returnVal<0)
		{
			//Cannot recover from error
			numberFrameAvailable = 0;
		}

		numberFrameAvailable = snd_pcm_avail_update(m_pPcmHandle);
		if(numberFrameAvailable<0)
		{
				numberFrameAvailable = 0;
		}
	}

	out_uBuffersNeeded = numberFrameAvailable / (m_uFrameSize);

	return AK_Success; 
}
