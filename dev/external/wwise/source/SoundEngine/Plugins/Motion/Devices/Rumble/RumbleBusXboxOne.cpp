#include "stdafx.h"
#include "RumbleBusXboxOne.h"

using namespace Windows::Xbox::Input;
using namespace Windows::Foundation::Collections;


RumbleMixBus::RumbleMixBus() 
: RumbleBusBase()
{
	m_fRightMotorLevel = 0;
	m_fLeftMotorLevel = 0;
	m_fRightTriggerLevel = 0;
	m_fLeftTriggerLevel = 0;
	m_idDevice = 0;
}

AKRESULT RumbleMixBus::Init(AK::IAkPluginMemAlloc * in_pAllocator, AkPlatformInitSettings * io_pPDSettings, AkUInt8 in_iPlayer, void * in_pDevice )
{
	AKRESULT res = RumbleBusBase::Init(in_pAllocator, io_pPDSettings, in_iPlayer, in_pDevice);
	if (res != AK_Success)
		return res;

	m_fRightMotorLevel = 0;
	m_fLeftMotorLevel = 0;
	m_fRightTriggerLevel = 0;
	m_fLeftTriggerLevel = 0;

	if (in_pDevice == NULL)
		return AK_Fail;

	m_idDevice = reinterpret_cast<AkUInt64>(in_pDevice);

	return AK_Success;
}

bool RumbleMixBus::SendSample()
{
	AKRESULT result = AK_Success;

	GamepadVibration newVib;
	newVib.LeftMotorLevel = AkMin(m_pData[m_usReadBuffer], 1.f);	//Large
	newVib.RightMotorLevel = AkMin(m_pData[m_usReadBuffer+1], 1.f); //Small
	newVib.LeftTriggerLevel = AkMin(m_pData[m_usReadBuffer+2], 1.f);
	newVib.RightTriggerLevel = AkMin(m_pData[m_usReadBuffer+3], 1.f);

	m_bStopped = (newVib.RightMotorLevel + newVib.LeftMotorLevel + newVib.RightTriggerLevel + newVib.LeftTriggerLevel) == 0.0f;

	if (newVib.RightMotorLevel != m_fRightMotorLevel ||
		newVib.RightTriggerLevel != m_fRightTriggerLevel ||
		newVib.LeftMotorLevel != m_fLeftMotorLevel ||
		newVib.LeftTriggerLevel != m_fLeftTriggerLevel)
	{
		try
		{
		
			IVectorView< IGamepad^ >^ allGamepads = Gamepad::Gamepads;
			for(AkUInt32 i = 0; i < allGamepads->Size; i++)
			{
				IGamepad^ gamepad = allGamepads->GetAt( i );	
				if (gamepad->Id == m_idDevice)
				{
					gamepad->SetVibration( newVib );
					break;
				}
			}
		}
		catch (...)
		{
		}

		m_fRightMotorLevel = newVib.RightMotorLevel;
		m_fLeftMotorLevel = newVib.LeftMotorLevel;
		m_fRightTriggerLevel = newVib.RightTriggerLevel;
		m_fLeftTriggerLevel = newVib.LeftTriggerLevel;
	}

#ifndef AK_OPTIMIZED
	if (m_pCapture != NULL)
	{
		AkInt16 output[4];
		output[0] = (AkInt16)(newVib.RightMotorLevel * 32767.f);
		output[1] = (AkInt16)(newVib.LeftMotorLevel * 32767.f);
		output[2] = (AkInt16)(newVib.RightTriggerLevel * 32767.f);
		output[3] = (AkInt16)(newVib.LeftTriggerLevel * 32767.f);
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

	IVectorView< IGamepad^ >^ allGamepads = Gamepad::Gamepads;
	for(AkUInt32 i = 0; i < allGamepads->Size; i++)
	{
		IGamepad^ gamepad = allGamepads->GetAt( i );				
		if (gamepad->Id == m_idDevice)
			return true;
	}
	return false;
}