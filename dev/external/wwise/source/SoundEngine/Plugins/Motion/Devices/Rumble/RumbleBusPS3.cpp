#include "stdafx.h"
#include "RumbleBusPS3.h"

#ifdef AK_PS3_MOVE
#include <cell/gem.h>
#endif

RumbleMixBus::RumbleMixBus() : RumbleBusBase()
{
	m_iRefreshCount = 0;
	m_bIsGem = false;
	memset(&m_oCurrent, 0, sizeof(m_oCurrent));
}

AKRESULT RumbleMixBus::Init(AK::IAkPluginMemAlloc * in_pAllocator, AkPlatformInitSettings * io_pPDSettings, AkUInt8 in_iPlayer, void * in_pDevice )
{
	AKRESULT res = RumbleBusBase::Init(in_pAllocator, io_pPDSettings, in_iPlayer, in_pDevice);
	if (res != AK_Success)
		return res;

	//Detect what kind of device is connected: regular controller or PS3 Move.
#ifdef AK_PS3_MOVE
	CellGemInfo gemInfo;
	int32_t errCode = cellGemGetInfo(&gemInfo);
	m_bIsGem = (errCode == CELL_OK && gemInfo.status[m_iPlayer] == CELL_GEM_STATUS_READY);
#endif
	memset(&m_oCurrent, 0, sizeof(m_oCurrent));

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
	//These values were obtained through experimentation.  Below 60, there is little happening.  
	//To maintain better parity with Xbox, keep the signal at 100% longer (values 240 to 255).
	AkUInt16 usLarge = 0;
	if (in_fLarge > 0.05)
	{
		usLarge = (AkUInt16)(in_fLarge * (255.f - 60.0f)) + 75.0f;
		usLarge = AkMin(usLarge, 255);
	}

	//Copy, in case of failure
	CellPadActParam oTemp;
	oTemp.motor[0] = m_oCurrent.motor[0];
	oTemp.motor[1] = m_oCurrent.motor[1];

	if ( PulseMod<uint8_t>( in_fSmall, 0, 1, m_oCurrent.motor[0] )
		|| m_oCurrent.motor[1] != usLarge
		//The PS3 turns off vibration after 3 seconds.  So refresh the value after 2.5 second.
		|| ((in_fLarge != 0.0f || in_fSmall == 1.0f) && m_iRefreshCount > 2500/AK_PC_WAIT_TIME)) 
	{
		//We need to change the speed. 
		m_oCurrent.motor[1] = (AkUInt8)usLarge;

		if (!AK_PERF_OFFLINE_RENDERING)
		{
			if (
#ifdef AK_PS3_MOVE
				m_bIsGem && cellGemSetRumble(m_iPlayer, m_oCurrent.motor[1]) != CELL_OK ||	//Try the Gem first.
#endif
				!m_bIsGem && cellPadSetActDirect(m_iPlayer, &m_oCurrent) != CELL_PAD_OK)

			{
				m_oCurrent.motor[0] = oTemp.motor[0];
				m_oCurrent.motor[1] = oTemp.motor[1];
			}

			m_bStopped = m_oCurrent.motor[0] == 0 && m_oCurrent.motor[1] == 0;
		}
		m_iRefreshCount = 0;
	}
	else
		m_iRefreshCount++;

#ifndef AK_OPTIMIZED
	if (m_pCapture != NULL)
	{
		AkInt16 output[2];
		//Scale to the full wav output scale
		output[0] = m_oCurrent.motor[0] * 32767;
		output[1] = (AkInt16)(m_oCurrent.motor[1]) * 128;
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

	CellPadInfo2 info;
	if (cellPadGetInfo2(&info) != CELL_PAD_OK)
		return false;
	if ((info.device_capability[m_iPlayer] & CELL_PAD_CAPABILITY_ACTUATOR) == 0)
		return false;

	return true;
}