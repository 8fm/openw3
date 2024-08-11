#include "stdafx.h"
#include "RumbleBusWiiWiiU.h"

#ifdef AK_WIIU
#include <cafe/vpad.h>
#define VPAD_SAMPLE_RATE 120
#define VPAD_SAMPLES_PER_AKBUFFER ((float)VPAD_SAMPLE_RATE/((float)DEFAULT_NATIVE_FREQUENCY/(float)AK_NUM_VOICE_REFILL_FRAMES))
#endif


RumbleMixBus::RumbleMixBus() : RumbleBusBase()
{
	m_iStopping = 0;
	m_fAverageSpeed = 0;
	m_bIsWiimote = true;
	m_fVPADRequired = 0;
	m_uVPADProduced = 0;
}

AKRESULT RumbleMixBus::Init(AK::IAkPluginMemAlloc * in_pAllocator, AkPlatformInitSettings * io_pPDSettings, AkUInt8 in_iPlayer, void * in_pDevice )
{
	AKRESULT res = RumbleBusBase::Init(in_pAllocator, io_pPDSettings, in_iPlayer, in_pDevice);
	if (res != AK_Success)
		return res;

	m_bIsWiimote = (in_pDevice == AK_MOTION_WIIMOTE_DEVICE);
	if (m_bIsWiimote)
	{
		const s32 wpadStatus = WPADGetStatus();
		if ( wpadStatus != WPAD_STATE_SETUP )
		{
	#ifndef AK_OPTIMIZED
			OSReport( "Rumble: Note: Wwimote rumble support will not be available because WPadInit() was not called before initializing the Wwise sound engine\n" );
	#endif
		}
#ifdef AK_WIIU
		//The 2 DRC are tied to player 0 and 1 by default.  
		AKASSERT(m_iPlayer >= 2);
		m_iPlayer = in_iPlayer - 2;	
#endif
	}
#ifdef AK_WIIU
	else
	{
		VPADStatus buf;
		s32 vpadStatus;
		VPADRead(in_iPlayer, &buf, 1, &vpadStatus);
		if (vpadStatus == VPAD_READ_ERR_INIT)
		{
	#ifndef AK_OPTIMIZED
			OSReport( "Rumble: Note: DRC rumble support will not be available because VPadInit() was not called before initializing the Wwise sound engine\n" );
	#endif
		}
	}
	#endif

	return AK_Success;
}

AKRESULT RumbleMixBus::Term( AK::IAkPluginMemAlloc * in_pAllocator )
{
	if (m_bIsWiimote)
	{
		while(m_iStopping > 0)
		{
			m_pData[m_usReadBuffer] = 0;
			SendSample();
		}
	}
#ifdef AK_WIIU
	else
	{
		VPADStopMotor(m_iPlayer);
	}
#endif

	return RumbleBusBase::Term(in_pAllocator);
}

AKRESULT RumbleMixBus::RenderData()
{
	bool bGotData = m_bGotData;
	AKRESULT res = RumbleBusBase::RenderData();
	if (res != AK_Success)
		return res;

#ifdef AK_WIIU
	if (!m_bIsWiimote)
	{
		//Convert all values to bits and upsample.  We need to upsample because the control rate of the VPAD is 120hz
		//We have normally 2 samples (SAMPLE_COUNT) per buffer.
		AkUInt8 data = 0;
		AkUInt8 uBit = 0;

		while(m_usReadBuffer != m_usWriteBuffer)
		{
			m_fVPADRequired += VPAD_SAMPLES_PER_AKBUFFER / SAMPLE_COUNT;
			AkUInt32 uRequired = 0;
			if (m_fVPADRequired > m_uVPADProduced)
				uRequired = m_fVPADRequired - m_uVPADProduced;

			for( ; uRequired !=0; uRequired--, uBit++, m_uVPADProduced++)
			{
				AkUInt32 uValue;
				PulseMod<AkUInt32>( m_pData[m_usReadBuffer], WPAD_MOTOR_STOP, WPAD_MOTOR_RUMBLE, uValue );

				data |= (uValue << uBit);

#ifndef AK_OPTIMIZED 
				if (m_pCapture != NULL) 
				{ 
					AkInt16 output[2]; 
					//Scale to the full wav output scale 
					output[0] = uValue * 32767; 
					output[1] = 0; 
					m_pCapture->PassSampleData(output, CHANNEL_COUNT*sizeof(AkInt16)); 
				} 
#endif 
			}
			m_usReadBuffer = (m_usReadBuffer + CHANNEL_COUNT) & BUFFER_MASK;
		}

		if (uBit > 0 && bGotData)
		{
			AKASSERT(uBit <= 8);	//Normally should have only 2-3 samples per buffer.
			data = data << (8-uBit);
			s32 err = VPADControlMotor(m_iPlayer, &data, uBit);
			m_bStopped = false;
		}
		else if (!m_bStopped)
		{
			VPADControlMotor(m_iPlayer, &data, 0);
			m_bStopped = true;
		}
	}
#endif
	return res;
}

bool RumbleMixBus::SendSample()
{
#ifdef AK_WIIU
	if (!m_bIsWiimote)
		return false;	//VPAD runs differently
#endif

	AkReal32 in_fSmall = m_pData[m_usReadBuffer];

	if (in_fSmall > 1.0f)
		in_fSmall = 1.0f;

	//Make sure we send 5 stops on the Wii.  Sometimes the Wiimote communication fails without us knowing.
	if (in_fSmall > 0.0f)
		m_iStopping = 5;	

	if ( PulseMod<AkUInt32>( in_fSmall, WPAD_MOTOR_STOP, WPAD_MOTOR_RUMBLE, m_oCurrent ) || m_iStopping > 0)
	{
		//Make sure we send 5 stops on the Wii.  Sometimes the Wiimote communication fails without us knowing.
		if (m_oCurrent == 0)
		{
			m_iStopping--;
			AKASSERT(m_iStopping >= 0);
		}

		WPADControlMotor(m_iPlayer, m_oCurrent);
	}

	m_bStopped = m_iStopping <= 0;

#ifndef AK_OPTIMIZED
	if (m_pCapture != NULL)
	{
		AkInt16 output[2];
		//Scale to the full wav output scale
		output[0] = m_oCurrent * 32767;
		output[1] = 0;
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

	if (m_bIsWiimote)
	{
		u32 type;
		return WPADProbe(m_iPlayer, &type) == WPAD_ERR_NONE;
	}

#ifdef AK_WIIU
	return m_iPlayer == 0;	//Assume a DRC is connected.  We can't know without doing a VPADRead, but that would consume the VPAD data for the game.
#else
	return false;
#endif
}