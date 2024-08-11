/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "../../common/engine/inputDeviceManager.h"
#include "../../common/engine/inputDeviceKeyboard.h"

#ifndef NO_EDITOR
# include "../../common/engine/inputEditorInterface.h"
#endif

#ifndef DIRECTINPUT_VERSION
# define DIRECTINPUT_VERSION 0x800
#endif

#include "../../../external/dxsdk(June2010)/include/dinput.h"
#include "../../../external/dxsdk(June2010)/include/XInput.h"

#ifdef RED_ARCH_X64
# define INPUTDEVMGR_ARCH "x64"
#else
# define INPUTDEVMGR_ARCH "x86"
#endif

#define INPUTDEVMGR_DXSDK_LIB_PATH "../../../external/dxsdk(June2010)/lib/" INPUTDEVMGR_ARCH "/"

#pragma comment ( lib, INPUTDEVMGR_DXSDK_LIB_PATH "dinput8.lib" )
//FIXME: Linked in baseEngine. Happy now WWise? #pragma comment ( lib, INPUTDEVMGR_DXSDK_LIB_PATH "xinput.lib" )

//////////////////////////////////////////////////////////////////////////
// Forward Declarations
//////////////////////////////////////////////////////////////////////////
class IInputDevice;
class IInputDeviceMouse;
class IInputDeviceGamepad;
class IInputDeviceTablet;
class IInputRawInput;

class CInputDeviceKeyboardRawInput;
class CInputDeviceKeyboardDInput;
class CInputDeviceMouseDInput;
class CInputDeviceGamepadXInput;

#ifdef RED_INPUT_DEVICE_GAMEPAD_PS4FORPC
class CInputDeviceGamepadPS4ForPC;
#endif

#include "inputDInputInterface.h"

//////////////////////////////////////////////////////////////////////////
// CInputDeviceManagerWin32
//////////////////////////////////////////////////////////////////////////
class CInputDeviceManagerWin32
	: public IInputDeviceManager
#ifndef NO_EDITOR
	, private IInputEditorInterface
#endif // !NO_EDITOR
{
private:
	typedef TStaticArray<IInputDeviceKeyboard::EKey, Uint32(IInputDeviceKeyboard::EKey::Count)> TKeyboardKeyTable;

private:
	typedef TDynArray< IDInputInterface* >	TDInputInterfaceList;
	typedef TDynArray< IInputDevice* >		TInputDeviceList;
	typedef TDynArray< IInputRawInput* >	TInputRawInputList;

private:
	struct SDirectInputKeyboardKeyMapping
	{
		Uint32								m_directInputKey;
		IInputDeviceKeyboard::EKey			m_key;
	};

private:
#if defined( RED_PLATFORM_DURANGO ) || !defined( NO_EDITOR )
	static const IDInputInterface::ECaptureMode DEFAULT_DINPUT_CAPTUREMODE = IDInputInterface::ECaptureMode::BackgroundShared;
#else
	static const IDInputInterface::ECaptureMode DEFAULT_DINPUT_CAPTUREMODE = IDInputInterface::ECaptureMode::ForegroundExclusive;
#endif

	// The mouse actually does foreground non-exclusive in the non-final game, so no need to change this value for debugging.
	static const IDInputInterface::ECaptureMode GAME_DINPUT_CAPTUREMODE = IDInputInterface::ECaptureMode::ForegroundExclusive;

private:
#if defined( RED_ASSERTS_ENABLED ) && !defined( NO_EDITOR )
	mutable Red::Threads::CMutex			m_assertMutex;
#endif // defined( RED_ASSERTS_ENABLED ) && !defined( NO_EDITOR )

private:
	static const SDirectInputKeyboardKeyMapping	DIRECTINPUT_KEYBOARD_KEY_MAPPING[ IInputDeviceKeyboard::EKey::Count ];

private:		
	IDirectInput8*							m_directInput;
	TKeyboardKeyTable						m_directInputKeyboardKeyLUT;
	HWND									m_topLevelHWnd;

	TDInputInterfaceList					m_dinputInterfaceList;
	TInputRawInputList						m_inputRawInputList;

	TDynArray< IInputDevice* >				m_inputDeviceList;
	
	CName									m_lastUsedDeviceName;

	// For rumble
	//TDynArray< IInputDeviceGamepad* >		m_gamepads;

#ifndef NO_TABLET_INPUT_SUPPORT
	IInputDeviceTablet*						m_editorTablet;
#endif

private:
#ifndef NO_EDITOR
	// Not counted since we have things like the review system calling it constantly...
	Red::Threads::CAtomic< Bool >			m_inputCaptureOverride;
#endif

	Red::Threads::CAtomic< Bool >			m_requestReset;

private:
#ifdef RED_INPUT_DEVICE_GAMEPAD_PS4FORPC
	Bool m_isPS4ForPCGamepadInit;
#endif

#ifndef NO_TABLET_INPUT_SUPPORT
	Bool									m_isWintabInit;
#endif

public:
											CInputDeviceManagerWin32();
	virtual									~CInputDeviceManagerWin32();

public:
	virtual Bool							Init() override;
	virtual void							Shutdown() override;
	virtual void							Update( TDynArray< SBufferedInputEvent >& outBufferedInput ) override;
	virtual void							RequestReset() override;


public:
#ifndef NO_EDITOR
	virtual IInputEditorInterface*			GetEditorInterface() override;

private:
	virtual void							SetInputCaptureOverride( Bool enable ) override;
	virtual void							SetAssertHookInputCaptureOverride( Bool enable ) override;
	virtual IInputDeviceTablet*				GetTablet() const override;

#endif // !NO_EDITOR

public:
	virtual void							OnGameInputMode( Bool enabled ) override;

public:
	void									SetTopLevelHwnd( HWND topLevelHWnd );
	void									OnWindowsInput( WPARAM wParam, LPARAM lParam );

private:
	void									CreateInputDevices();

	void CreateAndAddDirectInputKeyboardDevice();

	void CreateAndAddRawInputKeyboardDevice();

private:
	CInputDeviceMouseDInput*				CreateMouseDevice();
	CInputDeviceKeyboardDInput*				CreateKeyboardDevice();
	CInputDeviceKeyboardRawInput*			CreateKeyboardRawInputDevice();

#ifdef RED_INPUT_DEVICE_GAMEPAD_PS4FORPC
	CInputDeviceGamepadPS4ForPC*			CreateGamepadPS4ForPC();
#endif

private:
	void									SetCaptureMode( IDInputInterface::ECaptureMode captureMode );

private:
	void									Cleanup();

public:
	virtual const CName 					GetLastUsedDeviceName() const override;

};
