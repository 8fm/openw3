/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#ifndef DIRECTINPUT_VERSION
# define DIRECTINPUT_VERSION 0x800
#endif
#include "../../../external/dxsdk(June2010)/include/dinput.h"

#include "../../common/engine/inputDeviceMouse.h"

#include "inputDInputInterface.h"

//////////////////////////////////////////////////////////////////////////
// CInputDeviceMouseDInput
//////////////////////////////////////////////////////////////////////////
class CInputDeviceMouseDInput : public IInputDeviceMouse, public IDInputInterface
{
private:
	typedef IInputDeviceMouse TBaseClass;

private:
	static const Uint32						DINPUT_BUFFER_SIZE = 16384;

private:
	Bool									m_buttonDownEventResetTable[ EButton::Count ];
	Bool									m_axisMovementResetTable[ EAxis::Count ];

private:
	IDirectInputDevice8*					m_directInputDevice;
	HWND									m_topLevelHWnd;

private:
	IDInputInterface::ECaptureMode			m_captureMode;

private:
	Bool									m_needsClear;
	Bool									m_needsReset;

public:
											CInputDeviceMouseDInput( IDirectInputDevice8* directInputDevice );
	virtual									~CInputDeviceMouseDInput();
	Bool									Init();

public:
	virtual void							Update( BufferedInput& outBufferedInput ) override;
	virtual void							Reset() override;

public:
	virtual void							SetCaptureMode( IDInputInterface::ECaptureMode captureMode ) override;
	virtual void							SetTopLevelHWnd( HWND topLevelHWnd ) override;
	virtual void							Unacquire() override;

private:
	void									Clear();
	void									Reset( BufferedInput& outBufferedInput );

private:
	void									Cleanup();
};