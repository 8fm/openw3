#include "AkSink.h"

#ifdef AK_WASAPI

#include <audioclient.h>
#include <mmdeviceapi.h>
#include <propidl.h>
#ifdef AK_WIN
#include <propkey.h>
#endif

// REFERENCE_TIME time units per millisecond
#define MFTIMES_PER_MILLISEC  10000


class CAkSinkWasapi 
	: public CAkSink
{
public:
	CAkSinkWasapi();
	~CAkSinkWasapi();

	AKRESULT Init( interface IMMDeviceEnumerator * in_pEnumerator, AkChannelMask in_uChannelMask, AkSinkType in_eType );

	// CAkSink overrides
	virtual AKRESULT Play();
	virtual void Term();

	virtual DWORD GetThreadWaitTime();
	virtual bool IsStarved();
	virtual void ResetStarved();

	virtual AKRESULT IsDataNeeded( AkUInt32 & out_uBuffersNeeded );

	virtual AKRESULT PassData();

	virtual AKRESULT PassSilence();

private:
	interface IMMDevice * m_pDeviceOut;
	interface IAudioClient * m_pClientOut;
	interface IAudioRenderClient * m_pRenderClient;
	HANDLE m_hEvent;

	AkUInt32 m_uBufferFrames;	// Number of audio frames in output buffer.
	AkUInt32 m_uNumChannels;
};

#endif	// defined AK_WASAPI
