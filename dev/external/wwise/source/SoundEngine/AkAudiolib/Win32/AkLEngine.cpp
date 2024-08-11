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
// AkLEngine.cpp
//
// Implementation of the IAkLEngine interface. Win32 version.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "AkLEngine.h"

#include "Ak3DListener.h"
#include "AkAudioLibTimer.h"
#include "AkAudioMgr.h"
#include "AkSink.h"
#include "AkProfile.h"
#include "AkSpeakerPan.h"
#include "AkEffectsMgr.h"
#include "AkOutputMgr.h"
#ifdef AK_MOTION
#include "AkFeedbackMgr.h"
#endif
#ifdef AK_USE_METRO_API
#include <combaseapi.h>
#endif
#include <ks.h>
#include <ksmedia.h>

//-----------------------------------------------------------------------------
//Static variables.
//-----------------------------------------------------------------------------
extern AkInitSettings		g_settings;
extern AkPlatformInitSettings g_PDSettings;
AkEvent						CAkLEngine::m_EventStop;
bool						CAkLEngine::m_bCoInitializeSucceeded = false;
bool						CAkLEngine::m_bResetAudioDevice = false;
HANDLE						CAkLEngine::m_eventProcess = NULL;

extern CAkAudioMgr*		g_pAudioMgr;

#ifndef AK_USE_METRO_API

#include <Mmdeviceapi.h>

WNDPROC						CAkLEngine::m_WndProc = NULL;
HDEVNOTIFY					CAkLEngine::m_hDevNotify = NULL;
CAkMMNotificationClient *	CAkLEngine::m_pMMNotifClient = NULL;

class CAkMMNotificationClient
	: public IMMNotificationClient
{
public:
	CAkMMNotificationClient( IMMDeviceEnumerator * in_pEnumerator ) 
		: m_cRef( 1 )
		, m_pEnumerator( in_pEnumerator )
	{
		m_pEnumerator->AddRef();
		m_pEnumerator->RegisterEndpointNotificationCallback( this );
	}

	~CAkMMNotificationClient()
	{
		m_pEnumerator->UnregisterEndpointNotificationCallback( this );
		m_pEnumerator->Release();
	}

    virtual HRESULT STDMETHODCALLTYPE QueryInterface( REFIID riid, void ** ppvObject )
	{
		const IID IID_IMMNotificationClient = __uuidof(IMMNotificationClient);

		if ( riid == IID_IUnknown )
		{
			*ppvObject = static_cast<IUnknown *>( this );
			m_cRef++;
			return S_OK;
		}
		else if ( riid == IID_IMMNotificationClient )
		{
			*ppvObject = static_cast<IMMNotificationClient *>( this );
			m_cRef++;
			return S_OK;
		}

		*ppvObject = NULL;
		return E_NOINTERFACE;
	}

    virtual ULONG STDMETHODCALLTYPE AddRef( void )
	{
		return ++m_cRef;
	}

    virtual ULONG STDMETHODCALLTYPE Release( void )
	{
		ULONG cRef = --m_cRef;
		AKASSERT( cRef >= 0 );
		if ( cRef == 0 )
		{
			AkDelete( g_LEngineDefaultPoolId, this );
		}

		return cRef;
	}
	
	virtual HRESULT STDMETHODCALLTYPE OnDeviceStateChanged( LPCWSTR pwstrDeviceId, DWORD dwNewState ) { return S_OK; }
	virtual HRESULT STDMETHODCALLTYPE OnDeviceAdded( LPCWSTR pwstrDeviceId ) { return S_OK; }
	virtual HRESULT STDMETHODCALLTYPE OnDeviceRemoved( LPCWSTR pwstrDeviceId ) { return S_OK; }

	virtual HRESULT STDMETHODCALLTYPE OnDefaultDeviceChanged( EDataFlow flow, ERole role, LPCWSTR pwstrDefaultDeviceId )
	{
		if ( flow == eRender
			&& role == eMultimedia )
		{
			CAkLEngine::ResetAudioDevice();
		}
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE OnPropertyValueChanged( LPCWSTR pwstrDeviceId, const PROPERTYKEY key ) { return S_OK; }

private:
	ULONG m_cRef;
	IMMDeviceEnumerator * m_pEnumerator;
};

#endif

namespace AK
{
	IXAudio2 * GetWwiseXAudio2Interface()
	{
		return g_pAkSink ? g_pAkSink->GetWwiseXAudio2Interface() : NULL;
	}

	IDirectSound8 * GetDirectSoundInstance()
	{
		return g_pAkSink ? g_pAkSink->GetDirectSoundInstance() : NULL;
	}
};

void CAkLEngine::GetDefaultPlatformInitSettings( 
	AkPlatformInitSettings &      out_pPlatformSettings      // Platform specific settings. Can be changed depending on hardware.
	)
{
	memset( &out_pPlatformSettings, 0, sizeof( AkPlatformInitSettings ) );
#ifndef AK_USE_METRO_API
	out_pPlatformSettings.hWnd = ::GetForegroundWindow( );
#endif
	out_pPlatformSettings.threadLEngine.nPriority = AK_THREAD_PRIORITY_ABOVE_NORMAL;
	out_pPlatformSettings.threadLEngine.dwAffinityMask = 0;
	out_pPlatformSettings.threadLEngine.uStackSize = AK_DEFAULT_STACK_SIZE;
	out_pPlatformSettings.threadBankManager.nPriority = AK_THREAD_BANK_MANAGER_PRIORITY;
	out_pPlatformSettings.threadBankManager.dwAffinityMask = 0;
	out_pPlatformSettings.threadBankManager.uStackSize = AK_BANK_MGR_THREAD_STACK_BYTES;
	out_pPlatformSettings.uLEngineDefaultPoolSize = LENGINE_DEFAULT_POOL_SIZE;
	out_pPlatformSettings.fLEngineDefaultPoolRatioThreshold = 1.0f;// 1.0f == means 100% == disabled by default
	out_pPlatformSettings.uNumRefillsInVoice = AK_DEFAULT_NUM_REFILLS_IN_VOICE_BUFFER;
#ifdef AK_WIN
	out_pPlatformSettings.eAudioQuality = AkSoundQuality_High;
	out_pPlatformSettings.bGlobalFocus = true;
#endif

	out_pPlatformSettings.threadMonitor.nPriority = AK_THREAD_PRIORITY_ABOVE_NORMAL;
	out_pPlatformSettings.threadMonitor.dwAffinityMask = 0;
	out_pPlatformSettings.threadMonitor.uStackSize = AK_DEFAULT_STACK_SIZE;
	out_pPlatformSettings.pXAudio2 = NULL;
}

//-----------------------------------------------------------------------------
// Name: Init
// Desc: Initialise the object.
//
// Parameters:
//
// Return: 
//	Ak_Success:          Object was initialised correctly.
//  AK_InvalidParameter: Invalid parameters.
//  AK_Fail:             Failed to initialise the object correctly.
//-----------------------------------------------------------------------------
AKRESULT CAkLEngine::Init()
{
#ifdef AK_WIN
	AkUInt32 sampleFrequency;
	switch( g_PDSettings.eAudioQuality )
	{
	case AkSoundQuality_Low:
		sampleFrequency = 24000;
		break;
	case AkSoundQuality_High:
	default:// AkSoundQuality_High, Default value is high quality for backward compatibility. 
		sampleFrequency = 48000;
		break;
	}

	AkAudioLibSettings::SetSampleFrequency( sampleFrequency );
#endif
	// Necessary for DirectSound / Vista audio
	// Here we try for the multi-threaded model, but if it fails, we'll have to CoInitialize on the sound engine thread.
	m_bCoInitializeSucceeded = SUCCEEDED( CoInitializeEx(NULL, COINIT_MULTITHREADED) ); 	

	if ( AkCreateEvent( m_eventProcess ) != AK_Success )
		return AK_Fail;

	AKRESULT eResult = SoftwareInit();
	if (eResult == AK_Success)
		RegisterDeviceChange();

	return eResult;
} // Init

//-----------------------------------------------------------------------------
// Name: Term
// Desc: Terminate the object.
//
// Parameters:
//	None.
//
// Return:
//	Ak_Success: Object was terminated correctly.
//  AK_Fail:    Failed to terminate correctly.
//-----------------------------------------------------------------------------
void CAkLEngine::Term()
{
	UnregisterDeviceChange();

	SoftwareTerm();

	if( m_bCoInitializeSucceeded )
	{
		CoUninitialize(); // Necessary for DirectSound / Vista audio
		m_bCoInitializeSucceeded = false;
	}

	AkDestroyEvent(m_eventProcess);
	m_eventProcess = NULL;
} // Term

AkUInt32 CAkLEngine::GetNumBufferNeededAndSubmit()
{
	AkUInt32 uBuffersNeeded = 0;
	static int uNumRetries = 0;

	AKRESULT eResult;
	if ( uNumRetries )
	{
		--uNumRetries;

		if (uNumRetries % 20 == 0)	//Don't retry every frame.  For now, try only once every half-second.
		{
#ifdef AK_WIN
			CAkSink * pSink = CAkSink::Create( g_settings.settingsMainOutput, g_settings.eMainOutputType, 0 );
#else
			CAkSink * pSink = CAkSink::Create( g_settings.settingsMainOutput, AkSink_Main, 0 );
#endif
			if ( pSink )
			{
				eResult = pSink->Play();
				if ( eResult == AK_Success )
				{
					// Managed to init new sink: get rid of temporary dummy sink,
					// and set this one as the real sink.
					CAkOutputMgr::ReplaceSink(AK_MAIN_OUTPUT_DEVICE, pSink);
					g_pAkSink = pSink;
					uNumRetries = 0;
				}
				else
				{
					pSink->Term();
					AkDelete( g_LEngineDefaultPoolId, pSink ); 
				}
			}
		}
	}

	eResult = g_pAkSink->IsDataNeeded( uBuffersNeeded );
	if ( eResult != AK_Success || m_bResetAudioDevice )
	{
		m_bResetAudioDevice = false;

		// Create dummy sink while we try to recreate 'real' sink
		CAkSink * pSink = CAkSink::Create( g_settings.settingsMainOutput, AkSink_Dummy, 0 );
		AKASSERT( pSink );
		CAkOutputMgr::ReplaceSink(AK_MAIN_OUTPUT_DEVICE, pSink);
		g_pAkSink = pSink;
		
		// These should always succeed with dummy sink
		AKVERIFY( g_pAkSink->Play() == AK_Success ); 

		uNumRetries = 200; // try recreating non-dummy sink for 5 seconds (10 half seconds).  It could take some time for the driver to select a new device.
	}

	//nothing to submit, simply return num buffers
	return uBuffersNeeded;
}

//-----------------------------------------------------------------------------
// Name: Perform
// Desc: Perform all VPLs.
//-----------------------------------------------------------------------------
void CAkLEngine::Perform()
{
	// Avoid denormal problems in audio processing
#if defined AK_CPU_X86 || defined AK_CPU_X86_64
#if defined (_MSC_VER) && (_MSC_VER < 1700)
	AkUInt32 uFlushZeroMode = _MM_GET_FLUSH_ZERO_MODE(dummy);
#else
	AkUInt32 uFlushZeroMode = _MM_GET_FLUSH_ZERO_MODE();
#endif
	_MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
#endif

	SoftwarePerform();

#if defined AK_CPU_X86 || defined AK_CPU_X86_64
	_MM_SET_FLUSH_ZERO_MODE(uFlushZeroMode);
#endif
} // Perform

void CAkLEngine::StartVoice()
{
	g_pAkSink->Play();
}

#ifndef AK_USE_METRO_API

LRESULT CALLBACK CAkLEngine::InternalWindowProc( HWND in_hWnd, UINT in_unMsg, WPARAM in_wParam, LPARAM in_lParam )
{
	//Handle the connection of headphones
	if(in_unMsg == WM_DEVICECHANGE && in_wParam == DBT_DEVICEARRIVAL && g_pAudioMgr /*Sound engine is not terminated*/)
	{
		DEV_BROADCAST_DEVICEINTERFACE *devHdr = (DEV_BROADCAST_DEVICEINTERFACE *)in_lParam;		
		if (devHdr->dbcc_classguid == KSCATEGORY_AUDIO || devHdr->dbcc_classguid == KSCATEGORY_RENDER)
			ResetAudioDevice();
	}

	return CallWindowProc(m_WndProc, in_hWnd, in_unMsg, in_wParam, in_lParam);
}

#endif

void CAkLEngine::ResetAudioDevice()
{
	m_bResetAudioDevice = true;

	//Be careful, we might not be completely initialized yet!
	if (AK::SoundEngine::IsInitialized())
		g_pAudioMgr->WakeupEventsConsumer(); // Make sure event thread wakes up (XAudio2)
}

void CAkLEngine::RegisterDeviceChange()
{
#ifndef AK_USE_METRO_API
	IMMDeviceEnumerator * pEnumerator;

	const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
	const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
	HRESULT hr = CoCreateInstance(
		CLSID_MMDeviceEnumerator, NULL,
		CLSCTX_ALL, IID_IMMDeviceEnumerator,
		(void**)&pEnumerator);

	// Windows Vista or higher: preferred notification system available
	if ( hr == S_OK && pEnumerator )
	{
		m_pMMNotifClient = AkNew( g_LEngineDefaultPoolId, CAkMMNotificationClient( pEnumerator ) );
		pEnumerator->Release();
	}

	// Fallback for Windows XP: intermittently working device notification
	else
	{
		DEV_BROADCAST_DEVICEINTERFACE NotificationFilter;

		ZeroMemory( &NotificationFilter, sizeof(NotificationFilter) );
		NotificationFilter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
		NotificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
		NotificationFilter.dbcc_classguid = GUID_NULL;

		m_hDevNotify = ::RegisterDeviceNotification(g_PDSettings.hWnd, &NotificationFilter, DEVICE_NOTIFY_ALL_INTERFACE_CLASSES | DEVICE_NOTIFY_WINDOW_HANDLE);
		m_WndProc = (WNDPROC)::SetWindowLongPtr(g_PDSettings.hWnd, GWLP_WNDPROC, (AkUIntPtr)InternalWindowProc);
	}
#endif
}

void CAkLEngine::UnregisterDeviceChange()
{
#ifndef AK_USE_METRO_API
	if ( m_pMMNotifClient )
	{
		m_pMMNotifClient->Release();
		m_pMMNotifClient = NULL;
	}
	else
	{
		if ( m_hDevNotify != NULL )
			::UnregisterDeviceNotification( m_hDevNotify );

		//Reset the original WndProc
		if (::GetWindowLongPtr( g_PDSettings.hWnd, GWLP_WNDPROC) == (AkUIntPtr)InternalWindowProc )
			::SetWindowLongPtr( g_PDSettings.hWnd, GWLP_WNDPROC, (AkUIntPtr)m_WndProc );
	}
#endif
}

void CAkLEngine::GetDefaultOutputSettings( AkSinkType in_eSinkType, AkOutputSettings & out_settings )
{
	GetDefaultOutputSettingsCommon(out_settings);
}

bool CAkLEngine::GetSinkTypeText( AkSinkType in_sinkType, AkUInt32 in_uBufSize, char* out_pszBuf )
{
	// NOTE: if this function is changed, be sure to update GetNumSinkTypes and GetMaxSinkTypeTextLen !!
	if ( in_uBufSize > 4 )
	{
		switch ( in_sinkType )
		{
		case AkSink_Main:
		case AkSink_Main_XAudio2:
		case AkSink_Main_DirectSound:
			strcpy( out_pszBuf, "Main" );
			return true;
		case AkSink_Dummy:
		case AkSink_MergeToMain:
			strcpy( out_pszBuf, "" );
			return true;
		default:
			strcpy( out_pszBuf, "" );
		}
	}
	else if ( in_uBufSize != 0 )
	{
		strcpy( out_pszBuf, "" );
	}
	return false;
}
