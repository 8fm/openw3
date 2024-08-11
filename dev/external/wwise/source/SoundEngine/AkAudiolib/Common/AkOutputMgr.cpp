#include "stdafx.h"
#include "AkOutputMgr.h"
#include "Ak3DListener.h"
#include "AkProfile.h"
#include "AkLEngine.h"

#if !defined(AK_WII) && !defined(AK_3DS)
#include "AkVPLFinalMixNode.h"
#endif

AkArray<AkDevice, AkDevice&, ArrayPoolLEngineDefault> CAkOutputMgr::m_Devices;

void AkDevice::Cleanup()
{
	if (pSink != NULL)
	{
		pSink->Term();
		AkDelete(g_LEngineDefaultPoolId, pSink);
		pSink = NULL;
	}

#if !defined(AK_WII) && !defined(AK_3DS)
	if (pFinalMix != NULL)
	{
		pFinalMix->Term();
		AkDelete(g_LEngineDefaultPoolId, pFinalMix);
		pFinalMix = NULL;
	}
#endif

	if ( puSpeakerAngles )
	{
		AkFree( g_LEngineDefaultPoolId, puSpeakerAngles );
	}

	CAkSpeakerPan::MapConfig2PanPlane::Iterator it = m_mapConfig2PanPlane.Begin();
	while ( it != m_mapConfig2PanPlane.End() )
	{
		AkFree( g_DefaultPoolId, (*it).item );
		++it;
	}
	m_mapConfig2PanPlane.Term();
}

void AkDevice::PushData()
{
#if defined AK_WII || defined AK_3DS
	AKASSERT(!"Should not use software path");
#else
	AkPipelineBufferBase& rBuffer = pSink->NextWriteBuffer();
	rBuffer.uValidFrames = 0;
	pFinalMix->GetResultingBuffer(rBuffer
#ifdef AK_PS4
	, pSink->Force71()
#endif
	);

	if (rBuffer.uValidFrames > 0)
		pSink->PassData();
	else
		pSink->PassSilence();
#endif
}

AKRESULT AkDevice::SetSpeakerAngles( 
	const AkReal32 *	in_pfSpeakerAngles,	// Array of loudspeaker pair angles, expressed in degrees relative to azimuth ([0,180]).
	AkUInt32			in_uNumAngles		// Number of loudspeaker pair angles.
	)
{
	AKASSERT( pSink != NULL && in_uNumAngles > 0 );

	// Allocate memory for angles.
	AkUInt32 uTotalAngles = AkMax( uNumAngles, in_uNumAngles );
	AkUInt32 * puNewSpeakerAngles = (AkUInt32*)AkAlloc( g_LEngineDefaultPoolId, uTotalAngles * sizeof( AkUInt32 ) );
	if ( !puNewSpeakerAngles )
		return AK_Fail;

	// Create temporary array with new and previous angles (if applicable).
	AkReal32 * pfSpeakerAngles = (AkReal32 *)AkAlloca( uTotalAngles * sizeof(AkReal32) );
	if ( in_uNumAngles > 0 )
		memcpy( pfSpeakerAngles, in_pfSpeakerAngles, in_uNumAngles * sizeof( AkUInt32 ) );
	for ( AkUInt32 uAngle = in_uNumAngles; uAngle < uNumAngles ; uAngle ++ )
	{
		pfSpeakerAngles[uAngle] = 360.f * puSpeakerAngles[uAngle] / (AkReal32)(PAN_CIRCLE);		
	}
			
	AkUInt32 uNewMinAngleBetweenSpeakers;
	AKRESULT eResult = CAkSpeakerPan::SetSpeakerAngles( 
		pfSpeakerAngles,
		in_uNumAngles,
		puNewSpeakerAngles,
		uNewMinAngleBetweenSpeakers
		);
	if ( eResult == AK_Success )
	{
		if ( puSpeakerAngles )
		{
			AkFree( g_LEngineDefaultPoolId, puSpeakerAngles );
		}

		puSpeakerAngles = puNewSpeakerAngles;
		uNumAngles = uTotalAngles;
		fOneOverMinAngleBetweenSpeakers = 1.f / (AkReal32)( uNewMinAngleBetweenSpeakers );
	}
	else
	{
		AkFree( g_LEngineDefaultPoolId, puNewSpeakerAngles );
	}

	// Refresh pan cache.
	CAkSpeakerPan::MapConfig2PanPlane::Iterator it = m_mapConfig2PanPlane.Begin();
	while ( it != m_mapConfig2PanPlane.End() )
	{
		CAkSpeakerPan::CreatePanCache( (*it).key, puSpeakerAngles, (*it).item );
		++it;
	}
	
	return eResult;
}

AKRESULT AkDevice::CreatePanCache( 
	AkChannelMask	in_uOutputConfig	// config of bus to which this signal is routed.
	)
{
	CAkSpeakerPan::PanPair ** ppConfigPanGains = m_mapConfig2PanPlane.Set( in_uOutputConfig );
	if ( ppConfigPanGains )
	{
		AKASSERT( AK::GetNumberOfAnglesForConfig( in_uOutputConfig ) <= uNumAngles );
		*ppConfigPanGains = (CAkSpeakerPan::PanPair*)AkAlloc( g_DefaultPoolId, PAN_TABLE_SIZE * sizeof( CAkSpeakerPan::PanPair ) );
		if ( *ppConfigPanGains )
			CAkSpeakerPan::CreatePanCache( in_uOutputConfig, puSpeakerAngles, *ppConfigPanGains );
	}
	if ( !ppConfigPanGains || !*ppConfigPanGains )
	{
		m_mapConfig2PanPlane.Unset( in_uOutputConfig );
		return AK_Fail;
	}
	return AK_Success;
}

void CAkOutputMgr::Term()
{	
	m_Devices.Term();
}

AKRESULT CAkOutputMgr::_AddOutputDevice( AkOutputDeviceID in_uKey, AkOutputSettings & in_settings, AkSinkType in_eSinkType, AkUInt32 in_uDeviceInstance, AkUInt32 in_uListeners, void* in_pUserData )
{
	AKASSERT( GetDevice(in_uKey) == NULL );
	
	//Allocate a new device
	AkDevice *pDevice = m_Devices.AddLast();
	if (!pDevice)
		return AK_InsufficientMemory;

	if (AK_PERF_OFFLINE_RENDERING)
	{
		in_eSinkType = AkSink_Dummy;
		in_settings.uChannelMask = AK_PERF_OFFLINE_SPEAKERCONFIG;
	}

	pDevice->pSink = CAkSink::Create(in_settings, in_eSinkType, in_uDeviceInstance);

	bool bError = pDevice->pSink == NULL;

#if !defined(AK_WII) && !defined(AK_3DS)
	AkNew2( pDevice->pFinalMix, g_LEngineDefaultPoolId, CAkVPLFinalMixNode, CAkVPLFinalMixNode() );
	bError |= pDevice->pFinalMix == NULL;
#endif
	if (bError)
	{		
		m_Devices.Erase(m_Devices.Length()-1);
		return AK_InsufficientMemory;
	}

	// Get default speaker angles if they haven't been set already.
	AkUInt32 uNumAngles = AK::GetNumberOfAnglesForConfig( AK_SUPPORTED_SPEAKER_SETUP );
	AkReal32 * pfSpeakerAngles = (AkReal32*)AkAlloca( uNumAngles * sizeof(AkReal32) );
	CAkSpeakerPan::GetDefaultSpeakerAngles( pDevice->pSink->GetSpeakerConfig(), pfSpeakerAngles );
	AKRESULT eResult = pDevice->SetSpeakerAngles( pfSpeakerAngles, uNumAngles );
	if ( eResult != AK_Success )
		return AK_Fail;

	pDevice->uListeners = in_uListeners;
	pDevice->pUserData = in_pUserData;
	pDevice->uDeviceID = in_uKey;
	pDevice->ePanningRule = in_settings.ePanningRule;

	SetListenersOnDevice( in_uListeners, in_uKey );
	CAkListener::RouteListenersToDevice(in_uListeners, in_uKey);

#if !defined(AK_WII) && !defined(AK_3DS)
	eResult = pDevice->pFinalMix->Init(pDevice->pSink->GetSpeakerConfig());
	if (eResult == AK_Success)
		CAkLEngine::ReevaluateBussesForDevice(in_uKey, in_uListeners);
	return eResult;
#else
	return AK_Success;
#endif
}

AKRESULT CAkOutputMgr::AddMainDevice( AkOutputSettings & in_outputSettings, AkSinkType in_eSinkType, AkUInt32 in_uListeners, void *in_pUserData)
{
	return _AddOutputDevice( AK_MAIN_OUTPUT_DEVICE, in_outputSettings, in_eSinkType, 0, in_uListeners, in_pUserData );
}

AKRESULT CAkOutputMgr::AddOutputDevice( AkOutputSettings & in_settings, AkSinkType in_eSinkType, AkUInt32 in_uDeviceInstance, AkUInt32 in_uListeners, void* in_pUserData )
{
	//Check if device exists
	AkOutputDeviceID uKey = AK_MAKE_DEVICE_KEY(in_eSinkType, in_uDeviceInstance);
	if ( uKey == AK_MAIN_OUTPUT_DEVICE )
	{
		AKASSERT( !"Add output device trying to add another main output" );
		return AK_Fail;
	}

	if (GetDevice(uKey) != NULL)
		return AK_Success;	//Already done.

	return _AddOutputDevice( uKey, in_settings, in_eSinkType, in_uDeviceInstance, in_uListeners, in_pUserData );
}

AKRESULT CAkOutputMgr::RemoveOutputDevice( AkOutputDeviceID in_uDeviceID )
{
	AKASSERT(in_uDeviceID != AK_MAIN_OUTPUT_DEVICE); //Hey!  We need an output!		

	for(AkDeviceArray::Iterator it = m_Devices.Begin(); it != m_Devices.End(); ++it)
	{
		if ((*it).uDeviceID == in_uDeviceID)
		{
			CAkLEngine::ReevaluateBussesForDevice(in_uDeviceID, 0 /*No listeners attached, we are removing*/);
			m_Devices.EraseSwap(it);
			return AK_Success;
		}
	}
	return AK_Fail;
}

AKRESULT CAkOutputMgr::SetListenersOnDevice( AkUInt32 in_uListeners, AkOutputDeviceID in_uDeviceID )
{
	//Always remove the listeners on the main device.  All listener start on the main device so each time we add a device, we steal a listener.
	m_Devices[AK_MAIN_OUTPUT_DEVICE].uListeners &= ~in_uListeners;

	for(AkUInt32 i = 0; i < m_Devices.Length(); i++)
	{
		if (m_Devices[i].uDeviceID == in_uDeviceID)
		{
			m_Devices[i].uListeners = in_uListeners;
			return AK_Success;
		}
	}

	return AK_Fail;
}

AKRESULT CAkOutputMgr::ReplaceSink( AkOutputDeviceID in_uDeviceID, CAkSink* in_pSink )
{
	for(AkDeviceArray::Iterator it = m_Devices.Begin(); it != m_Devices.End(); ++it)
	{
		if ((*it).uDeviceID == in_uDeviceID)
		{
			(*it).pSink->Term();
			AkDelete(g_LEngineDefaultPoolId, (*it).pSink);
			(*it).pSink = in_pSink;
			return AK_Success;
		}
	}

	return AK_Fail;
}

void CAkOutputMgr::StartOutputCapture( const AkOSChar* in_CaptureFileName )
{
	size_t len = AKPLATFORM::OsStrLen(in_CaptureFileName);
	AkOSChar *szName = (AkOSChar *)AkAlloca((len + 2) * sizeof(AkOSChar));
	memcpy(szName, in_CaptureFileName, len* sizeof(AkOSChar));

	//Recopy the extension
	for(AkUInt32 c = 0; c < 4; c++)
		szName[len-c] = szName[len-c-1];

	szName[len+1] = 0;

	AkInt8 i = 0;
	for(AkDeviceArray::Iterator it = m_Devices.Begin(); it != m_Devices.End(); ++it)
	{
		if ((*it).uDeviceID == AK_MAIN_OUTPUT_DEVICE)
			(*it).pSink->StartOutputCapture(in_CaptureFileName);
		else
		{
			i++;
			szName[len-4] = '0' + i;
			(*it).pSink->StartOutputCapture(szName);
		}
	}
}

void CAkOutputMgr::StopOutputCapture()
{
	for(AkDeviceArray::Iterator it = m_Devices.Begin(); it != m_Devices.End(); ++it)
		(*it).pSink->StopOutputCapture();
}


