/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.

* Modified from Scaleform sources to reduce video microstuttering. Temporarily here
* to avoid having to rebuild the video library while tweaking.
*/

/**************************************************************************

Filename    :   Video_VideoSoundSystemXA2.cpp
Content     :   Video sound system implementation based on XAudio2
Created     :   March 2009
Authors     :   Maxim Didenko, Vladislav Merker

Copyright   :   Copyright 2011 Autodesk, Inc. All Rights reserved.

Use of this software is subject to the terms of the Autodesk license
agreement provided at the time of installation or download, or which
otherwise accompanies this software in either electronic or hard copy form.

**************************************************************************/

#include "build.h"
#include "videoSoundSystemXA2.h"

namespace Config
{
	/*
	typedef enum XAUDIO2_FILTER_TYPE { 
	LowPassFilter          = 0,
	BandPassFilter         = 1,
	HighPassFilter         = 2,
	NotchFilter            = 3,
	LowPassOnePoleFilter   = 4, // Xbox Only
	HighPassOnePoleFilter  = 5	// Xbox Only
	} XAUDIO2_FILTER_TYPE;
	*/
	TConfigVar< Bool >	cvXAudio2FilterEnabled( "Video/Audio/XAudio2/Filter", "Enabled", false );
	TConfigVar< Int32 > cvXAudio2FilterType( "Video/Audio/XAudio2/Filter", "Type", 0 );
	TConfigVar< Float > cvXAudio2FilterFrequency( "Video/Audio/XAudio2/Filter", "Frequency", 1.f );
	TConfigVar< Float > cvXAudio2FilterOneOverQ( "Video/Audio/XAudio2/Filter", "OneOverQ", 1.f );
}

#ifdef USE_SCALEFORM
#if defined( RED_PLATFORM_WINPC ) || defined( RED_PLATFORM_DURANGO )

#ifdef GFX_ENABLE_VIDEO

#ifdef SF_OS_XBOX360
#include <xtl.h>
#else
#define _WIN32_DCOM
#define INITGUID
#include <windows.h>
#include <process.h>
#endif

// FIXME include paths: Use the non-inlined, non-DX version on Durango
#ifdef _DURANGO
# ifdef M_PI
#  undef M_PI
# endif
# include "C:\Program Files (x86)\Microsoft Durango XDK\xdk\Include\um\xaudio2.h"
#else
// Include our DX header explicitly to avoid version mismatches and inexplicable API call failures
//# include <XAudio2.h>
#include "../../../external/dxsdk(June2010)/include/XAudio2.h"
#endif

#ifdef RED_LOGGING_ENABLED
static const Char* GetFilterParametersTypeTxtForLog( XAUDIO2_FILTER_TYPE type )
{
	switch (type)
	{
	case LowPassFilter:				return TXT("LowPassFilter");
	case BandPassFilter:			return TXT("BandPassFilter");
	case HighPassFilter:			return TXT("HighPassFilter");
	case NotchFilter:				return TXT("NotchFilter");
#ifdef RED_PLATFORM_DURANGO
	case LowPassOnePoleFilter:		return TXT("LowPassOnePoleFilter");
	case HighPassOnePoleFilter:		return TXT("HighPassOnePoleFilter");
#endif
	default:
		return TXT("<Unknown>");
	}
}

static String GetFilterParametersStringForLog( const XAUDIO2_FILTER_PARAMETERS& params )
{
	return ::String::Printf(TXT("[XAUDIO2_FILTER_PARAMETERS Type=%ls, Frequency=%.2f, OneOverQ=%.2f"),
		GetFilterParametersTypeTxtForLog( params.Type ), params.Frequency, params.OneOverQ);
}
#endif // RED_LOGGING_ENABLED

#include "Video/Video_SystemSoundInterface.h"
#include "Kernel/SF_HeapNew.h"

extern Float GHackVideoSFXVolume;
extern Float GHackVideoVoiceVolume;

namespace Config
{
	extern TConfigVar< Bool > cvVideoVolumeLinearScale;
}

static void SetVolumeXA2( IXAudio2SourceVoice* pSourceVoice, Float sfx, Float voice )
{
	Float normSFx = 0.f;
	Float normVoice = 0.f;

	if ( Config::cvVideoVolumeLinearScale.Get() )
	{
		normSFx = sfx;
		normVoice = voice;
	}
	else
	{
		// [-60, 0] dB
		const Float a = 0.001f;
		const Float b = 6.908f;

		normSFx = sfx == 0.f ? 0.f : a * exp( sfx * b );
		normVoice = voice == 0.f ? 0.f : a * exp( voice * b );
	}

	normSFx = Clamp< Float >( normSFx, 0.f, 1.f );
	normVoice = Clamp< Float >( normVoice, 0.f, 1.f );

	LOG_ENGINE(TXT("SetVolumeXA2 sfx=%.6f, voice=%.6f -> normSFx=%.6f, normVoice=%.6f"), sfx, voice, normSFx, normVoice );

	// FrontLeft, FrontRight, FrontCenter, LowFrequency, SideLeft, SideRight (5.1 configuration)
	const Float vols[] = { normSFx, normSFx, normVoice, normSFx, normSFx, normSFx };
	HRESULT hr = pSourceVoice->SetChannelVolumes( ARRAY_COUNT_U32(vols), vols, XAUDIO2_COMMIT_NOW );
	if ( FAILED(hr) )
	{
		ERR_ENGINE(TXT("SetChannelVolumes failed HRESULT=0x%08X"), hr);
	}
}

namespace Scaleform { namespace GFx { namespace Video {

//////////////////////////////////////////////////////////////////////////
//

class VideoSoundXA2Tmp : public VideoSoundStream
{
public:
	VideoSoundXA2Tmp(MemoryHeap* pheap, IXAudio2* pxa2, IXAudio2MasteringVoice* pmasteringVoice);
	virtual ~VideoSoundXA2Tmp();

	virtual bool        CreateOutput(UInt32 channel, UInt32 samplerate);
	virtual void        DestroyOutput();

	virtual PCMFormat   GetPCMFormat() const       { return PCM_Float; }
	virtual void        Start(PCMStream* pstream);
	virtual void        Stop();
	virtual Status      GetStatus() const          { return SoundStatus; }

	virtual void        Pause(bool sw);
	virtual void        GetTime(UInt64* count, UInt64* unit) const;

	virtual void        SetVolume(float volume);
	virtual float       GetVolume() { return Volume; }

	static const UInt32 SAMPLING_RATE = 48000;
	static const UInt32 NUM_CHANNELS = 6;
	static const UInt32 SAMPLE_SIZE = sizeof(float);

	static const UInt32 SAMPLES_PER_PACKET = 256;
	static const UInt32 AUDIO_FRAME_INTERVAL = 4;

	static const UInt32 PACKET_SIZE = (NUM_CHANNELS * SAMPLE_SIZE * SAMPLES_PER_PACKET);

	// Process more packets of smaller size (256 samples per packet)
	// That way we don't starve XAudio2 and keep the TotalSamples increasing more often
	// so video doesn't microstutter
#ifdef RED_PLATFORM_WINPC
	static const UInt32 MAX_PACKETS = 6;
#else
	// TBD: maybe 6 is good here too.
	static const UInt32 MAX_PACKETS = 3;
#endif

private:
	MemoryHeap*         pHeap;
	PCMStream*          pPCMStream;

	UInt32              ChannelCount;
	Status              SoundStatus;
	UInt64              TotalSamples;
	UInt64              SampleRate;
	float               Volume;

	IXAudio2*               pXA2;
	IXAudio2MasteringVoice* pMasteringVoice;
	// Source voice
	IXAudio2SourceVoice*    pSourceVoice;

	// Temporary buffer to receive PCM data
	float               temp_buffer[NUM_CHANNELS][SAMPLES_PER_PACKET];
	// Buffers to submit to source voice
	float               sound_buffer[MAX_PACKETS][PACKET_SIZE];
	// Write position
	UInt32              WritePosition;

	CRITICAL_SECTION    cs;
	bool                cs_initialized;

	HANDLE              hthread;
	volatile bool       thread_exit_flag;

#if defined(SF_OS_XBOX360)
	static DWORD WINAPI     AudioProc(void *arg);
#elif defined(_DURANGO)
	static DWORD WINAPI     AudioProc(LPVOID arg);
#else
	static unsigned WINAPI  AudioProc(void *arg);
#endif
	void                    ExecuteAudioFrame(void);
	UInt32                  GetSoundData(UInt32 nch, float *sample[], UInt32 nsmpl);
};

//////////////////////////////////////////////////////////////////////////
//

VideoSoundXA2Tmp::VideoSoundXA2Tmp(MemoryHeap* pheap, IXAudio2* pxa2, IXAudio2MasteringVoice* pmasteringVoice) :
	pHeap(pheap), pPCMStream(0), ChannelCount(0), SoundStatus(Sound_Stopped),
	TotalSamples(0), SampleRate(0), Volume(1.0f)
{
	SF_ASSERT(pxa2 && pmasteringVoice);
	pXA2 = pxa2;
	pMasteringVoice = pmasteringVoice;
	pSourceVoice = NULL;
	hthread = NULL;
	WritePosition = 0;
	ZeroMemory(&cs, sizeof(cs));
	cs_initialized    = FALSE;

	InitializeCriticalSection(&cs);
	cs_initialized = TRUE;
}

VideoSoundXA2Tmp::~VideoSoundXA2Tmp()
{
	if (cs_initialized)
	{
		DeleteCriticalSection(&cs);
		cs_initialized = FALSE;
	}
}



bool VideoSoundXA2Tmp::CreateOutput(UInt32 channel, UInt32 samplerate)
{
	if (channel == 0)
		return false;

	ChannelCount = channel;
	SampleRate = samplerate;

	hthread = NULL;
	WritePosition = 0;

	// Configure WAVEFORMATEX structure
	HRESULT hr;
	WAVEFORMATEX wfex;
	ZeroMemory(&wfex, sizeof(wfex));
	wfex.cbSize = sizeof(wfex);
	wfex.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
	wfex.nChannels = NUM_CHANNELS;
	wfex.wBitsPerSample = SAMPLE_SIZE * 8;
	wfex.nSamplesPerSec = samplerate;
	wfex.nBlockAlign = (wfex.wBitsPerSample / 8) * wfex.nChannels;
	wfex.nAvgBytesPerSec = wfex.nBlockAlign * wfex.nSamplesPerSec;

	UINT32 flags = Config::cvXAudio2FilterEnabled.Get() ? XAUDIO2_VOICE_USEFILTER : 0;
	
	// Creation of source voice
	hr = pXA2->CreateSourceVoice(
		&pSourceVoice, &wfex, flags, XAUDIO2_DEFAULT_FREQ_RATIO, NULL);

	if (FAILED(hr)) 
	{
		ERR_ENGINE(TXT("Failed XAudio2 CreateSourceVoice: HRESULT=0x%08X, flags=0x%08X"), hr, flags);
		DestroyOutput();
		return false;
	}

	XAUDIO2_FILTER_PARAMETERS filterParams;
	if ( Config::cvXAudio2FilterEnabled.Get() )
	{
		filterParams.Type = (XAUDIO2_FILTER_TYPE)Config::cvXAudio2FilterType.Get();
		filterParams.Frequency  = Config::cvXAudio2FilterFrequency.Get();
		filterParams.OneOverQ = Config::cvXAudio2FilterOneOverQ.Get();
		hr = pSourceVoice->SetFilterParameters( &filterParams, XAUDIO2_COMMIT_ALL );
		if ( FAILED(hr))
		{
			WARN_ENGINE(TXT("Failed XAudio2 SetFilterParameters: HRESULT=0x%08X, %ls"), hr, GetFilterParametersStringForLog( filterParams ).AsChar());
		}
		else
		{
			LOG_ENGINE(TXT("Set XAudio2 filter params: %ls"), GetFilterParametersStringForLog( filterParams ).AsChar());
		}
	}
	else
	{
		LOG_ENGINE(TXT("XAudio2 filters disabled"));
	}

	thread_exit_flag = false;
	// Creation of thread
#if defined(SF_OS_XBOX360) || defined(_DURANGO)
	hthread = CreateThread(NULL, 0, AudioProc, this, CREATE_SUSPENDED, NULL);
#else
	hthread = (HANDLE)_beginthreadex(NULL, 0, AudioProc, this, CREATE_SUSPENDED, NULL);
#endif

	if (hthread == NULL)
	{
	#if defined(SF_OS_XBOX360) || defined(_DURANGO)
		const DWORD dwErr = ::GetLastError();
		RED_UNUSED(dwErr);
		ERR_ENGINE(TXT("Failed to create audio thread! GetLastError()=%u"), dwErr );
	#else
		ERR_ENGINE(TXT("Failed to create audio thread! errno=%d"), errno );
	#endif
		DestroyOutput();
		return false;
	}

	LOG_ENGINE(TXT("XAudio2 source voice ready for video"));

	// Change the priority of the thread to THREAD_PRIORITY_HIGHEST
	SetThreadPriority(hthread, THREAD_PRIORITY_HIGHEST);

//https://forums.xboxlive.com/questions/42897/what-kinds-of-processing-should-and-should-not-be.html
// #ifdef RED_PLATFORM_DURANGO
// 	SetThreadAffinityMask( hthread, 1 << 6 );
// #endif

#ifdef SF_OS_XBOX360
	// If needed, change the processor of the thread
	XSetThreadProcessor(hthread, 5);
#endif

	// Start thread processing
	ResumeThread(hthread);

	// Call ExecuteAudioFrame function to invoke callback
	ExecuteAudioFrame();

	// Start playback
	pSourceVoice->Start(0, 0);
	return true;
}

void VideoSoundXA2Tmp::DestroyOutput()
{
	// Release of the thread
	if (hthread) {
		// Wait until thread processing ends
		thread_exit_flag = true;
		WaitForSingleObject(hthread, INFINITE);

		// Release of the thread handle
		CloseHandle(hthread);
		hthread = NULL;
	}

	// Release of the source voice
	if (pSourceVoice != NULL) {
		pSourceVoice->Stop(0);
		pSourceVoice->FlushSourceBuffers();
		pSourceVoice->DestroyVoice();
		pSourceVoice = NULL;
	}
}

void VideoSoundXA2Tmp::Start(PCMStream* pstream)
{
	EnterCriticalSection(&cs);
	pPCMStream  = pstream;
	SoundStatus = Sound_Playing;
	LeaveCriticalSection(&cs);

	SetVolumeXA2( pSourceVoice, GHackVideoSFXVolume, GHackVideoVoiceVolume );
	//pSourceVoice->SetVolume(Volume);
	TotalSamples = 0;
}

void VideoSoundXA2Tmp::Stop()
{
	EnterCriticalSection(&cs);
	SoundStatus = Sound_Stopped;
	pPCMStream  = 0;
	LeaveCriticalSection(&cs);
	TotalSamples = 0;
}

void VideoSoundXA2Tmp::Pause(bool sw)
{
	EnterCriticalSection(&cs);
	if (sw)
	{
		if (SoundStatus == Sound_Playing)
			SoundStatus = Sound_Paused;
	}
	else
	{
		if (SoundStatus == Sound_Paused)
			SoundStatus = Sound_Playing;
	}    
	LeaveCriticalSection(&cs);
}

void VideoSoundXA2Tmp::GetTime(UInt64* count, UInt64* unit) const
{
	if (TotalSamples == 0)
	{
		*count = 0;
		*unit  = 1000;
	}
	else
	{
		*count = TotalSamples * 1000 / SampleRate;
		*unit = 1000;
	}    
}



void VideoSoundXA2Tmp::SetVolume(float volume)
{
	RED_UNUSED(volume);

	Volume = GHackVideoSFXVolume;
	
	if (SoundStatus == Sound_Playing)
	{
		SetVolumeXA2( pSourceVoice, GHackVideoSFXVolume, GHackVideoVoiceVolume );
		//pSourceVoice->SetVolume(volume);
	}
}

void VideoSoundXA2Tmp::ExecuteAudioFrame(void)
{
	UInt32 i, j, n;
	float *sample[MaxChannels];
	UInt32 wsmpl;

	EnterCriticalSection(&cs);

	// Get PCM data
	for (i = 0; i < NUM_CHANNELS; i++) 
		sample[i] = temp_buffer[i];

	if (SoundStatus == Sound_Playing) 
	{
		wsmpl = GetSoundData(NUM_CHANNELS, sample, SAMPLES_PER_PACKET);
	} 
	else 
	{
		ZeroMemory(temp_buffer, sizeof(temp_buffer));
		wsmpl = SAMPLES_PER_PACKET;
	}

	// Interleaving
	float *pcmbuf = sound_buffer[WritePosition];
	for (j = 0; j < wsmpl; j++) {
		static UInt32 ch_map[8]={ 0, 1, 4, 5, 2, 3, 6, 7 };
		for (n = 0; n < NUM_CHANNELS; n++) {
			pcmbuf[j * NUM_CHANNELS + n] = sample[ch_map[n]][j];
		}
	}

	// Submit PCM buffer
	if (wsmpl > 0) {
		XAUDIO2_BUFFER buf = {0};
		buf.AudioBytes = wsmpl * NUM_CHANNELS * SAMPLE_SIZE;
		buf.pAudioData = (BYTE *)pcmbuf;
		pSourceVoice->SubmitSourceBuffer(&buf);
	}

	// Update write position
	WritePosition++;
	if(WritePosition >= MAX_PACKETS) {
		WritePosition = 0;
	}

	LeaveCriticalSection(&cs);

	return;
}

// Audio processing thread
// This thread is waked up from XAudio2 callback
#if defined(SF_OS_XBOX360)
DWORD WINAPI VideoSoundXA2Tmp::AudioProc(void *arg)
#elif defined(_DURANGO)
DWORD WINAPI VideoSoundXA2Tmp::AudioProc(LPVOID arg)
#else
unsigned WINAPI VideoSoundXA2Tmp::AudioProc(void *arg)
#endif
{
	VideoSoundXA2Tmp *sndout = (VideoSoundXA2Tmp*)arg;

	while (sndout->thread_exit_flag == false) 
	{
		// Transfer audio data. Send all available packets
		XAUDIO2_VOICE_STATE state;
		sndout->pSourceVoice->GetState(&state);
		for (int i = state.BuffersQueued; i < MAX_PACKETS; i++) 
		{
			// Execute sound output processing
			sndout->ExecuteAudioFrame();
		}

		// Sleep until next audio frame
		Sleep(AUDIO_FRAME_INTERVAL);
	}

	return (0);
}

UInt32 VideoSoundXA2Tmp::GetSoundData(UInt32 nch, float *sample[], UInt32 nsmpl)
{
	UInt32 recv_nsmpl = 0;

	if (pPCMStream)
	{
		if (ChannelCount == 1)
		{
			// Mono
			UInt32 i, center_ch = 4;
			for (i = 0; i < nch; i++)
			{
				if (i != center_ch)
					ZeroMemory(sample[i], nsmpl * sizeof(float));
			}
			recv_nsmpl = pPCMStream->GetDataFloat(1, &sample[4], nsmpl);
		}
		else
		{
			// Stereo or 5.1ch
			recv_nsmpl = pPCMStream->GetDataFloat(nch, sample, nsmpl);
		}
	}

	TotalSamples += recv_nsmpl;
	return recv_nsmpl;
}

#ifdef RED_PLATFORM_WINPC
static HRESULT DEBUG_XAudio2Create(__deref_out IXAudio2** ppXAudio2, UINT32 Flags X2DEFAULT(0),
							   XAUDIO2_PROCESSOR XAudio2Processor X2DEFAULT(XAUDIO2_DEFAULT_PROCESSOR))
{
	// Instantiate the appropriate XAudio2 engine
	IXAudio2* pXAudio2;

	GUID clsid = (Flags & XAUDIO2_DEBUG_ENGINE) ? __uuidof(XAudio2_Debug) : __uuidof(XAudio2);
	OLECHAR clsidstr[64] = {0};
	StringFromGUID2( clsid, clsidstr, 64 );
	LOG_ENGINE(TXT("DEBUG_XAudio2Create XAudio2Create XAudio2 clsid '%ls'"), clsidstr );

	GUID riid = __uuidof(IXAudio2);
	OLECHAR riidstr[64] = {0};
	StringFromGUID2( riid, riidstr, 64 );
	LOG_ENGINE(TXT("DEBUG_XAudio2Create XAudio2Create riid '%ls'"), riidstr );

	HRESULT hr = CoCreateInstance(clsid,
		NULL, CLSCTX_INPROC_SERVER, riid, (void**)&pXAudio2);
	if (SUCCEEDED(hr))
	{
		hr = pXAudio2->Initialize(Flags, XAudio2Processor);

		if (SUCCEEDED(hr))
		{
			*ppXAudio2 = pXAudio2;
		}
		else
		{
			pXAudio2->Release();
		}
	}

	return hr;
}
#endif

//////////////////////////////////////////////////////////////////////////
//
class VideoSoundSystemXA2ImplTmp : public NewOverrideBase<StatMD_Other_Mem>
{
public:
	VideoSoundSystemXA2ImplTmp(IXAudio2* pxa2, IXAudio2MasteringVoice* pmasteringVoice)
	{
		InitializedByUs = false;
		InitializedCom = false;

		HRESULT hr;
		UINT32 flags;

		if (pxa2)
		{
			SF_ASSERT(pmasteringVoice);
			pXAUDIO2     = pxa2;
			pMasterVoice = pmasteringVoice;
			return;
		}
#if !(defined(SF_OS_XBOX360) || defined(_DURANGO))
		// First just try to do CoInitialize( NULL ) like most code already in the engine expects
		// This is important so it calls the correct number of CoUninitialize if it's not checking for errors
		hr = CoInitializeEx( nullptr, COINIT_APARTMENTTHREADED );
		if ( FAILED(hr) )
		{
			hr = CoInitializeEx( nullptr, COINIT_MULTITHREADED );
		}
		if ( SUCCEEDED(hr) )
		{
			InitializedCom = true;
		}
		else
		{
			ERR_ENGINE(TXT("Failed CoInitialize for XAudio2: HRESULT=0x%08X"), hr);
			return;
		}
		LOG_ENGINE(TXT("Initialized COM for XAudio2"));
#endif

		// Selection of initialization method
		flags = 0;
#if (_WIN32_WINNT < 0x0602 /*_WIN32_WINNT_WIN8*/) && defined(_DEBUG)
		//flags |= XAUDIO2_DEBUG_ENGINE;
#endif

// Getting HRESULT - 0x80040154 - class not registered. Maybe was using XAUDIO2_DEBUG_ENGINE on a non-dev machine?
#ifdef _DEBUG
		LOG_ENGINE(TXT("_DEBUG is defined during VideoSoundSystemXA2ImplTmp"));
#else
		LOG_ENGINE(TXT("_DEBUG *not* defined during VideoSoundSystemXA2ImplTmp"));
#endif

		// Avoid ACP_E_ALREADY_ALLOCATED (0x8AC80001)
		// https://forums.xboxlive.com/AnswerPage.aspx?qid=275b3656-c888-4c7f-ad8e-4f6d5c41b5b8&tgt=1
		/*
		If you create an XAudio2 instance in the Game OS, whether you intend to render XMA voices or not, it will, by default, allocate 512 XMA contexts.
		This will cause any other SHAPE context allocations to fail. 

		This is intentional, and won’t be changed, because: The 99% usage scenario for XAudio2 in the Game OS is for XMA voice playback.

		If you don’t want XAudio2 to use any XMA / SHAPE resources, pass the flag XAUDIO2_DO_NOT_USE_SHAPE to XAudio2Create.
		This change was made to the Chat APIs to prevent XAudio2 from allocating any SHAPE/XMA resources. All XMA voice allocations will fail with this flag.
		*/
#ifdef _DURANGO
		flags |= XAUDIO2_DO_NOT_USE_SHAPE;
#endif

		// Creation of XAudio2 object
#ifndef RED_PLATFORM_WINPC
		hr = XAudio2Create(&pXAUDIO2.GetRawRef(), flags, 0x00000040);
#else
 		hr = XAudio2Create(&pXAUDIO2.GetRawRef(), flags, XAUDIO2_DEFAULT_PROCESSOR);
 		if (FAILED(hr)) 
 		{
 			ERR_ENGINE(TXT("Failed XAudio2Create: HRESULT=0x%08X, flag=0x%08X"), hr, flags );
 			ERR_ENGINE(TXT("Trying DEBUG_XAudio2Create to see if the DX version seems wrong..."));
			hr = DEBUG_XAudio2Create(&pXAUDIO2.GetRawRef(), flags, XAUDIO2_DEFAULT_PROCESSOR);
		}
#endif
		if (FAILED(hr)) 
		{
			ERR_ENGINE(TXT("Failed XAudio2Create: HRESULT=0x%08X, flag=0x%08X"), hr, flags );
			Finalize();
			return;
		}
		// Creation of mastering voice
 		hr = pXAUDIO2->CreateMasteringVoice(&pMasterVoice,
 			XAUDIO2_DEFAULT_CHANNELS, VideoSoundXA2Tmp::SAMPLING_RATE, 0, 0, NULL);

		if (FAILED(hr)) 
		{
			ERR_ENGINE(TXT("Failed XAudio2 CreateMasteringVoice: HRESULT=0x%08X"), hr );
			pMasterVoice = NULL;
			Finalize();
			return;
		}

		LOG_ENGINE(TXT("XAudio2 soundsystem ready for video"));

		InitializedByUs = true;
	}

	~VideoSoundSystemXA2ImplTmp()
	{
		Finalize();
	}

	void Finalize(void)
	{
		// Release of the mastering voice
		if (pMasterVoice && InitializedByUs) 
		{
			pMasterVoice->DestroyVoice();
			pMasterVoice = NULL;
		}

// 		if( pXAUDIO2 )
// 		{
// 			pXAUDIO2->StopEngine();
// 		}

		// Release of the XAudio2 object
		pXAUDIO2 = NULL;

#if !(defined(SF_OS_XBOX360) || defined(_DURANGO))
		if ( InitializedCom ) 
		{
			CoUninitialize();
		}
#endif
	}

	IXAudio2*                GetXAudio2() const      { return pXAUDIO2.GetPtr(); }
	IXAudio2MasteringVoice*  GetMasterVoice() const  { return pMasterVoice; }

protected:
	Ptr<IXAudio2>            pXAUDIO2;
	IXAudio2MasteringVoice*  pMasterVoice;

	// InitializedByUs flag is set to 'true' if XAudio2 object has been
	// created locally instead of bing passed by the user
	bool InitializedByUs;

	// Whether the COM refcount was incremented
	bool InitializedCom;
};

//////////////////////////////////////////////////////////////////////////
//

VideoSoundSystemXA2Tmp::VideoSoundSystemXA2Tmp(void* /*IXAudio2**/ pxa2, void* /*IXAudio2MasteringVoice**/ pmasteringVoice, 
	MemoryHeap* pheap) : VideoSoundSystem(pheap)
{
	pImpl = SF_HEAP_NEW(pheap) VideoSoundSystemXA2ImplTmp((IXAudio2*)pxa2, (IXAudio2MasteringVoice*)pmasteringVoice);
}

VideoSoundSystemXA2Tmp::~VideoSoundSystemXA2Tmp()
{
	delete pImpl;
}

VideoSound* VideoSoundSystemXA2Tmp::Create()
{
	IXAudio2* pxa2 = pImpl->GetXAudio2();
	IXAudio2MasteringVoice* pmasteringVoice = pImpl->GetMasterVoice();
	if (pxa2 && pmasteringVoice)
		return SF_HEAP_NEW(GetHeap()) VideoSoundXA2Tmp(GetHeap(), pxa2, pmasteringVoice);
	return NULL;
}

Bool VideoSoundSystemXA2Tmp::IsValid() const
{
	return pImpl->GetXAudio2() != nullptr;
}

}}} // Scaleform::GFx::Video

#endif // GFX_ENABLE_VIDEO

#endif // defined( RED_PLATFORM_WINPC ) || defined( RED_PLATFORM_DURANGO )
#endif // USE_SCALEFORM
