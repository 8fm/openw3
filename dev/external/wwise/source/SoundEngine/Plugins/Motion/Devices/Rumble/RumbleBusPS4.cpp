#include "stdafx.h"
#include "RumbleBusPS4.h"
#include "user_service/user_service_defs.h"
#include "user_service/user_service_api.h"

RumbleMixBus::RumbleMixBus() 
: RumbleBusBase()
{
	m_handle = NULL;
}

AKRESULT RumbleMixBus::Init(AK::IAkPluginMemAlloc * in_pAllocator, AkPlatformInitSettings * io_pPDSettings, AkUInt8 in_iPlayer, void * in_pDevice )
{
	if (in_pDevice == NULL)
	{
		return AK_Fail;
	}
	
	AKRESULT res = RumbleBusBase::Init(in_pAllocator, io_pPDSettings, in_iPlayer, in_pDevice);
	if (res != AK_Success)
		return res;
	//memset(&m_oCurrent, 0, sizeof(m_oCurrent));

	m_handle = (int64_t)in_pDevice;

	m_oCurrent = (ScePadVibrationParam*)malloc(sizeof(ScePadVibrationParam));

	return AK_Success;
}

AKRESULT RumbleMixBus::Term( AK::IAkPluginMemAlloc * in_pAllocator )
{
	return RumbleBusBase::Term(in_pAllocator);
}

bool RumbleMixBus::SendSample()
{
	AkReal32 in_fLarge = m_pData[m_usReadBuffer];
	AkReal32 in_fSmall = m_pData[m_usReadBuffer+1];

	AKRESULT result = AK_Success;

	AKASSERT( m_handle != NULL );
	AKASSERT(in_fLarge >= 0.f && in_fSmall >= 0.f);

	m_bStopped = (in_fLarge + in_fSmall) == 0.0f;
	if (in_fLarge > 1.0f)
		in_fLarge = 1.0f;

	if (in_fSmall > 1.0f)
		in_fSmall = 1.0f;

	uint8_t uLarge = (uint8_t)(in_fLarge * 255);
	uint8_t uSmall = (uint8_t)(in_fSmall * 255);

	if ( (m_oCurrent->largeMotor != uLarge || m_oCurrent->smallMotor != uSmall))
	{
		m_oCurrent->largeMotor = uLarge;
		m_oCurrent->smallMotor = uSmall;
		AKVERIFY( scePadSetVibration(m_handle, m_oCurrent) == SCE_OK );
	}

#ifndef AK_OPTIMIZED
	if (m_pCapture != NULL)
	{
		AkInt16 output[2];
		output[0] = (AkInt16)(m_oCurrent->largeMotor * 32767.f/256.f);
		output[1] = (AkInt16)(m_oCurrent->smallMotor * 32767.f/256.f);
		m_pCapture->PassSampleData(output, CHANNEL_COUNT*sizeof(AkInt16));
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

	return true; //todo
}