// InputMgr.cpp
// Copyright (C) 2010 Audiokinetic Inc 
/// \file 
/// Defines the methods declared in InputMgr.h

#include "stdafx.h"

#include "InputMgr.h"

InputMgr::InputMgr()
	: m_pUInput( NULL )
{
}

InputMgr::~InputMgr()
{
}

bool InputMgr::Init(
	void* in_pParam,
	AkOSChar* in_szErrorBuffer,
	unsigned int in_unErrorBufferCharCount
)
{

	m_pUInput = new UniversalInput;

	for ( int i = 0; i < MAX_INPUT; i++ )
	{
		m_pUInput->AddDevice( i + 1, UGDeviceType_GAMEPAD );
	}
	
	return true;
}

UniversalInput* InputMgr::UniversalInputAdapter() const
{
	return m_pUInput;
}

void InputMgr::Update()
{
	for ( int i = 0; i < MAX_INPUT; i++ )
	{
		TranslateInput( NULL, i );
	}
}

void InputMgr::Release()
{
	if ( m_pUInput )
	{
		delete m_pUInput;
		m_pUInput = NULL;
	}
}

void InputMgr::TranslateInput( void * in_pad, int in_iPlayerIndex )
{
	UGBtnState iState = 0;
	bool bConnected = false;

	UGStickState joysticks[2];
	memset(joysticks, 0, sizeof(joysticks));

//	if ( in_pad->bConnected )
	{
		// Using our own bool to avoid performance warning when converting type BOOL to bool.
		bConnected = true;

/*		if ( in_pad->wButtons & XINPUT_GAMEPAD_DPAD_UP )
		{
			iState |= UG_DPAD_UP;
		}
		if ( in_pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN )
		{
			iState |= UG_DPAD_DOWN;
		}
		if ( in_pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT )
		{
			iState |= UG_DPAD_LEFT;
		}
		if ( in_pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT )
		{
			iState |= UG_DPAD_RIGHT;
		}
		if ( in_pad->wButtons & XINPUT_GAMEPAD_A )
		{
			iState |= UG_BUTTON1;
		}
		if ( in_pad->wButtons & XINPUT_GAMEPAD_B )
		{
			iState |= UG_BUTTON2;
		}
		if ( in_pad->wButtons & XINPUT_GAMEPAD_X )
		{
			iState |= UG_BUTTON3;
		}
		if ( in_pad->wButtons & XINPUT_GAMEPAD_Y )
		{
			iState |= UG_BUTTON4;
		}
		if ( in_pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER )
		{
			iState |= UG_BUTTON5;
		}
		if ( in_pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER )
		{
			iState |= UG_BUTTON6;
		}
		if ( in_pad->wButtons & XINPUT_GAMEPAD_START )
		{
			iState |= UG_BUTTON7;
		}
		if ( in_pad->wButtons & XINPUT_GAMEPAD_BACK )
		{
			iState |= UG_BUTTON8;
		}
		if ( in_pad->wButtons & XINPUT_GAMEPAD_LEFT_THUMB )
		{
			iState |= UG_BUTTON9;
		}
		if ( in_pad->wButtons & XINPUT_GAMEPAD_RIGHT_THUMB )
		{
			iState |= UG_BUTTON10;
		}
		joysticks[UG_STICKLEFT].x = in_pad->sThumbLX / 32768.f;
		joysticks[UG_STICKLEFT].y = in_pad->sThumbLY / 32768.f;
		joysticks[UG_STICKRIGHT].x = in_pad->sThumbRX / 32768.f;
		joysticks[UG_STICKRIGHT].y = in_pad->sThumbRY / 32768.f;
		*/
	}


	m_pUInput->SetState( in_iPlayerIndex + 1, bConnected, iState, joysticks );
}