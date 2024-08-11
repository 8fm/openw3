#include "stdafx.h"
#include "RumbleBusWindowsXBox.h"

#ifdef AK_XBOX360
#include <Xffb.h>
#endif

RumbleMixBus::RumbleMixBus() 
: RumbleBusBase()
, m_pRumbleDevice(NULL)
{
	memset(&m_oCurrent, 0, sizeof(m_oCurrent));
}

AKRESULT RumbleMixBus::Init(AK::IAkPluginMemAlloc * in_pAllocator, AkPlatformInitSettings * io_pPDSettings, AkUInt8 in_iPlayer, void * in_pDevice )
{
	AKRESULT res = RumbleBusBase::Init(in_pAllocator, io_pPDSettings, in_iPlayer, in_pDevice);
	if (res != AK_Success)
		return res;
	memset(&m_oCurrent, 0, sizeof(m_oCurrent));
	m_pRumbleDevice = RumbleDeviceHelper::InitRumbleController(in_pAllocator, in_iPlayer, in_pDevice);
	return m_pRumbleDevice ? AK_Success : AK_Fail;
}

AKRESULT RumbleMixBus::Term( AK::IAkPluginMemAlloc * in_pAllocator )
{
	if (m_pRumbleDevice)
	{
		m_pRumbleDevice->Term( in_pAllocator);
		m_pRumbleDevice = NULL;
	}
	return RumbleBusBase::Term(in_pAllocator);;
}

bool RumbleMixBus::SendSample()
{
	AKRESULT result = AK_Success;

	AkReal32 in_fLarge = AkMax(0,m_pData[m_usReadBuffer]);
	AkReal32 in_fSmall = AkMax(0,m_pData[m_usReadBuffer+1]);

	m_bStopped = (in_fLarge + in_fSmall) == 0.0f;
	if (in_fLarge > 1.0f)
		in_fLarge = 1.0f;

	if (in_fSmall > 1.0f)
		in_fSmall = 1.0f;

	if (m_pRumbleDevice && (m_oCurrent.m_fLarge != in_fLarge || m_oCurrent.m_fSmall != in_fSmall))
	{
		//We need to change the speed. (No overlapped IO for now)	
		result = m_pRumbleDevice->SetRumble(in_fLarge, in_fSmall);
		if (result == AK_Busy && !AK_PERF_OFFLINE_RENDERING)
		{
			m_bStopped = false;
		}
		else
		{
			m_oCurrent.m_fLarge = in_fLarge;
			m_oCurrent.m_fSmall = in_fSmall;
		}
	}

#ifndef AK_OPTIMIZED
	if (m_pCapture != NULL)
	{
		AkInt16 output[2];
		output[0] = (AkInt16)(m_oCurrent.m_fLarge * 32767.f);
		output[1] = (AkInt16)(m_oCurrent.m_fSmall * 32767.f);
		m_pCapture->PassSampleData(output, 2*sizeof(AkInt16));
	}
#endif
	return m_bStopped;
}

bool RumbleMixBus::IsActive()
{
#ifndef AK_OPTIMIZED
	//For automated tests, assume the controller is connected.
	if(AK_PERF_OFFLINE_RENDERING)
		return true;
#endif

	if ( !m_pRumbleDevice )
		return false;
	return m_pRumbleDevice->IsActive();
}