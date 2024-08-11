/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#ifndef DIRECTINPUT_VERSION
# define DIRECTINPUT_VERSION 0x800
#endif
#include "../../../external/dxsdk(June2010)/include/dinput.h"

#include "../../common/engine/inputDeviceKeyboard.h"

#include "inputDInputInterface.h"

//////////////////////////////////////////////////////////////////////////
// CInputDeviceKeyboardDInput
//////////////////////////////////////////////////////////////////////////
class CInputDeviceKeyboardDInput : public IInputDeviceKeyboard, public IDInputInterface, private Red::System::NonCopyable
{
public:
	typedef TStaticArray<EKey, Uint32(EKey::Count)> TKeyTable;

private:
	typedef IInputDeviceKeyboard TBaseClass;

private:
	static const Uint32						DINPUT_BUFFER_SIZE = 256;

private:
	Bool									m_keyDownEventResetTable[ EKey::Count ];

private:
	IDirectInputDevice8*					m_directInputDevice;
	const TKeyTable&						m_directInputKeysLUT;
	HWND									m_topLevelHWnd;

private:
	IDInputInterface::ECaptureMode			m_captureMode;

private:
	Bool									m_needsClear;
	Bool									m_needsReset;

public:
											CInputDeviceKeyboardDInput( IDirectInputDevice8* directInputDevice, const TKeyTable& directInputKeysLUT );
	virtual									~CInputDeviceKeyboardDInput();
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