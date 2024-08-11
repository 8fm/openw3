/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"

#include "../../common/engine/inputBufferedInputEvent.h"
#include "../../common/engine/inputUtils.inl"
#include "userProfileManagerWindows.h"

#ifndef NO_TABLET_INPUT_SUPPORT
# include "inputLibWintab.h"
# include "inputDeviceTabletWintab.h"
#endif

#include "inputDeviceManagerWin32.h"
#include "inputDeviceKeyboardDInput.h"
#include "inputDeviceKeyboardRawInput.h"
#include "inputDeviceMouseDInput.h"
#include "inputDeviceGamepadXInput.h"
#ifdef RED_INPUT_DEVICE_GAMEPAD_PS4FORPC
	#include "inputDeviceGamepadPS4ForPC.h"
#endif

// FIXME<<<: Cheat and make sure not the game without NO_EDITOR
#ifndef NO_EDITOR
# include "../editor/editorEngine.h"
#endif
#include "inputRawInputInterface.h"

namespace Config
{
	extern TConfigVar< Bool >	cvForcePad;
	extern TConfigVar< Bool >	cvForceDisablePad;
	extern TConfigVar< Int32 >	cvSteamController;
}

RED_DEFINE_STATIC_NAME( steampad );

//////////////////////////////////////////////////////////////////////////
// CInputDeviceManagerWin32
//////////////////////////////////////////////////////////////////////////
CInputDeviceManagerWin32::CInputDeviceManagerWin32()
	: m_directInput( nullptr )
	, m_directInputKeyboardKeyLUT( Uint32(IInputDeviceKeyboard::EKey::Count) )
	, m_topLevelHWnd( nullptr )
#ifndef NO_EDITOR
	, m_inputCaptureOverride( false )
#endif // !NO_EDITOR
	, m_requestReset( false )
#ifdef RED_INPUT_DEVICE_GAMEPAD_PS4FORPC
	, m_isPS4ForPCGamepadInit( false )
#endif
#ifndef NO_TABLET_INPUT_SUPPORT
	, m_isWintabInit( false )
	, m_editorTablet( nullptr )
#endif
{
	for ( Uint32 i = 0; i < ARRAY_COUNT( DIRECTINPUT_KEYBOARD_KEY_MAPPING ); ++i )
	{
		const SDirectInputKeyboardKeyMapping& mapping = DIRECTINPUT_KEYBOARD_KEY_MAPPING[ i ];
		m_directInputKeyboardKeyLUT[ mapping.m_directInputKey ] = mapping.m_key;
	}
}

Bool CInputDeviceManagerWin32::Init()
{
	RED_ASSERT( ::SIsMainThread() );

	if ( FAILED( ::DirectInput8Create( ::GetModuleHandle(nullptr), DIRECTINPUT_VERSION, IID_IDirectInput8, reinterpret_cast<void**>(&m_directInput), nullptr ) ) )
	{
		ERR_ENGINE( TXT("Failed to get DirectInput8 interface") );
		return false;
	}

#ifndef NO_TABLET_INPUT_SUPPORT
	extern Bool SInitLibWintab();
	if ( SInitLibWintab() )
	{
		m_isWintabInit = true;
	}
	else
	{
		WARN_ENGINE(TXT("Failed to initialize tablet library"));
	}
#endif

#ifdef RED_INPUT_DEVICE_GAMEPAD_PS4FORPC
	if( CInputDeviceGamepadPS4ForPC::InitializePadLibrary() )
	{
		m_isPS4ForPCGamepadInit = true;
	}
	else
	{
		WARN_ENGINE(TXT("Failed to load or initialize PS4 dev padlib"));
	}
#endif

	CreateInputDevices();
	
	return true;
}

CInputDeviceManagerWin32::~CInputDeviceManagerWin32()
{
	RED_ASSERT( ! m_directInput, TXT("~CInputDeviceManagerWin32: DirectInput8 not cleaned up!") );
}

void CInputDeviceManagerWin32::Update( TDynArray< SBufferedInputEvent >& outBufferedInput )
{
#if defined( RED_ASSERTS_ENABLED ) && !defined( NO_EDITOR )
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_assertMutex );
#endif // defined( RED_ASSERTS_ENABLED ) && !defined( NO_EDITOR )

	RED_ASSERT( ::SIsMainThread() );

#ifndef NO_TABLET_INPUT_SUPPORT
	if ( m_editorTablet )
	{
		m_editorTablet->Update( outBufferedInput );
	}
#endif

	//FIXME: Keyboard and "mouse buttons"

#ifndef NO_EDITOR
	// Per Update() because a common theme in the editor/engine was to keep setting the capture mode to none every tick, without an explicit
	// way of restoring it
	if ( m_inputCaptureOverride.Exchange( false ) )
	{
		ForEach( m_dinputInterfaceList, []( IDInputInterface* dinputInterface) { dinputInterface->Unacquire(); } );
		return;
	}
#endif

	if ( m_requestReset.Exchange( false ) )
	{
		for ( auto devIt = m_inputDeviceList.Begin(); devIt != m_inputDeviceList.End(); ++devIt )
		{
			IInputDevice* inputDevice = *devIt;
			RED_ASSERT( inputDevice );
			inputDevice->Reset();
		}
	}

	m_deviceCommandsManager.ResetCurrentDevices();

	// TBD: Optimize. Update all devices even if not used, to ensure any of their side effects and only one update per frame
	// (or best attempt). E.g., mouse capture mode.
	// Also, should hook into Windows device added/removed events so can add remove an XInput device dynamically, so not just updating it when useless
	for ( auto devIt = m_inputDeviceList.Begin(); devIt != m_inputDeviceList.End(); ++devIt )
	{
		IInputDevice* inputDevice = *devIt;
		RED_ASSERT( inputDevice );
		Uint32 previousBufferSize = outBufferedInput.Size();
		inputDevice->Update( outBufferedInput );

		Uint32 currentBufferSize = outBufferedInput.Size();

		if( currentBufferSize != previousBufferSize )
		{
			m_lastUsedDeviceName = inputDevice->GetDeviceName();
		}

		m_deviceCommandsManager.AddCurrentDevice( inputDevice );
	}

	switch( (ESteamController)Config::cvSteamController.Get() )
	{
	case ESteamController::ForceEnable:
		m_lastUsedDeviceName = CNAME(steampad);
		break;
	case ESteamController::ForceDisable:
		break;
	default:
		if ( GUserProfileManager && GUserProfileManager->HACK_IsUsingSteamController() )
			m_lastUsedDeviceName = CNAME(steampad);
		break;
	}

	m_deviceCommandsManager.Update();
}

void CInputDeviceManagerWin32::RequestReset()
{
	m_requestReset.SetValue( true );
}

void CInputDeviceManagerWin32::Shutdown()
{
	RED_ASSERT( ::SIsMainThread() );

	Cleanup();

#ifdef RED_INPUT_DEVICE_GAMEPAD_PS4FORPC
	CInputDeviceGamepadPS4ForPC::ShutdownPadLibrary();
	m_isPS4ForPCGamepadInit = false;
#endif

#ifndef NO_TABLET_INPUT_SUPPORT
	extern void SShutdownLibWintab();
	SShutdownLibWintab();
	m_isWintabInit = false;
#endif
}

void CInputDeviceManagerWin32::Cleanup()
{
	m_inputDeviceList.ClearPtrFast();

	if ( m_directInput )
	{
		m_directInput->Release();
		m_directInput = nullptr;
	}
}

void CInputDeviceManagerWin32::SetTopLevelHwnd( HWND topLevelHWnd )
{
#if defined( RED_ASSERTS_ENABLED ) && !defined( NO_EDITOR )
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_assertMutex );
#endif // defined( RED_ASSERTS_ENABLED ) && !defined( NO_EDITOR )

	RED_ASSERT( topLevelHWnd );
	RED_ASSERT( ::SIsMainThread() );
	
	const Bool wasHwndNull = !m_topLevelHWnd;

	m_topLevelHWnd = topLevelHWnd;

	ForEach( m_dinputInterfaceList, [=]( IDInputInterface* dinputInterface ){ dinputInterface->SetTopLevelHWnd( topLevelHWnd ); } );

	for( IInputRawInput* rawInput : m_inputRawInputList )
	{
		rawInput->SetWindowHandle( topLevelHWnd );
	}

	// Default capture mode
	if ( wasHwndNull )
	{
		SetCaptureMode( DEFAULT_DINPUT_CAPTUREMODE );

		// FIXME: Lousy place to create it
#ifndef NO_TABLET_INPUT_SUPPORT
		if ( m_isWintabInit )
		{
			RED_ASSERT( ! m_editorTablet );
			CInputDeviceTabletWintab* editorTablet = new CInputDeviceTabletWintab;
			if ( ! editorTablet->Init( topLevelHWnd ) )
			{
				WARN_ENGINE(TXT("Failed to initialize tablet"));
				delete editorTablet;
			}
			else
			{
				m_editorTablet = editorTablet;
			}
		}
#endif // NO_TABLET_INPUT_SUPPORT
	}
}

void CInputDeviceManagerWin32::SetCaptureMode( IDInputInterface::ECaptureMode captureMode )
{
#if defined( RED_ASSERTS_ENABLED ) && !defined( NO_EDITOR )
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_assertMutex );
#endif // defined( RED_ASSERTS_ENABLED ) && !defined( NO_EDITOR )
	
	RED_ASSERT( ::SIsMainThread() );

	RED_ASSERT( m_topLevelHWnd );
	RED_ASSERT( captureMode != IDInputInterface::ECaptureMode::None, TXT("Use the platform API to unacquire the devices instead! It may be hard to recapture if the editor ignores input!") );

	ForEach( m_dinputInterfaceList, [=]( IDInputInterface* dinputInterface ){ dinputInterface->SetCaptureMode( captureMode ); } );
}

void CInputDeviceManagerWin32::CreateInputDevices()
{
	// FIXME: TBD: Test with keyboard and/or mouse unplugged

#ifdef NO_EDITOR
	if ( !Config::cvForcePad.Get() )
#endif
	{
		CInputDeviceMouseDInput* inputDeviceMouse = CreateMouseDevice();
		if ( inputDeviceMouse )
		{
			m_inputDeviceList.PushBack( inputDeviceMouse );
			m_dinputInterfaceList.PushBack( inputDeviceMouse);
		}

		// We can't have both DirectInput and RawInput devices, as DirectInput prevents RawInput from receiving WM_INPUT message
#ifdef USE_RAW_INPUT_FOR_KEYBOARD_DEVICE
	#ifndef NO_EDITOR
		// When not final
		// Use RawInput for game and DirectInput for editor
		if( GIsEditor == true )
		{
			// It's safer to use DirectInput with editor
			CreateAndAddDirectInputKeyboardDevice();
		}
		else
		{
			CreateAndAddRawInputKeyboardDevice();
		}
	#else
		// When final
		// Use RawInput for game
		CreateAndAddRawInputKeyboardDevice();
	#endif
#else
		// When not using RawInput at all
		// Use DirectInput
		CreateAndAddDirectInputKeyboardDevice();
#endif
	}

#ifdef RED_INPUT_DEVICE_GAMEPAD_PS4FORPC
	if ( m_isPS4ForPCGamepadInit )
	{
		IInputDevice* inputDeviceGamepadPS4 = CreateGamepadPS4ForPC();
		if ( inputDeviceGamepadPS4 )
		{
			m_inputDeviceList.PushBack( inputDeviceGamepadPS4 );
		}
	}
#endif

	if ( !Config::cvForceDisablePad.Get() )
	{
		// TBD: Poll to find *actual* user index, update based on Windows events etc.
		CInputDeviceGamepadXInput* inputDeviceGamepadXInput = new CInputDeviceGamepadXInput( 0 );
		m_inputDeviceList.PushBack( inputDeviceGamepadXInput );
	}
}

CInputDeviceKeyboardDInput* CInputDeviceManagerWin32::CreateKeyboardDevice()
{
	RED_ASSERT( ::SIsMainThread() );

	RED_ASSERT( m_directInput );

	IDirectInputDevice8* directInputDevice = nullptr;

	if ( FAILED( m_directInput->CreateDevice( GUID_SysKeyboard, &directInputDevice, nullptr ) ) )
	{
		WARN_ENGINE( TXT("Failed to create DirectInput keyboard device") );
		return nullptr;
	}

	CInputDeviceKeyboardDInput* inputDeviceKeyboard = new CInputDeviceKeyboardDInput( directInputDevice, m_directInputKeyboardKeyLUT );

	if ( ! inputDeviceKeyboard->Init() )
	{
		delete inputDeviceKeyboard;
		inputDeviceKeyboard = nullptr;	
	}

	return inputDeviceKeyboard;
}

CInputDeviceKeyboardRawInput* CInputDeviceManagerWin32::CreateKeyboardRawInputDevice()
{
	RED_ASSERT( ::SIsMainThread() );
	CInputDeviceKeyboardRawInput* inputDebiceKeyboardRawInput = new CInputDeviceKeyboardRawInput();

	return inputDebiceKeyboardRawInput;
}

CInputDeviceMouseDInput* CInputDeviceManagerWin32::CreateMouseDevice()
{
	RED_ASSERT( ::SIsMainThread() );

	RED_ASSERT( m_directInput );

	IDirectInputDevice8* directInputDevice = nullptr;

	if ( FAILED( m_directInput->CreateDevice( GUID_SysMouse, &directInputDevice, nullptr ) ) )
	{
		WARN_ENGINE( TXT("Failed to create DirectInput mouse device") );
		return nullptr;
	}

	CInputDeviceMouseDInput* inputDeviceMouse = new CInputDeviceMouseDInput( directInputDevice );
	if ( ! inputDeviceMouse->Init() )
	{
		delete inputDeviceMouse;
		inputDeviceMouse = nullptr;
	}

	return inputDeviceMouse;
}

#ifdef RED_INPUT_DEVICE_GAMEPAD_PS4FORPC
CInputDeviceGamepadPS4ForPC* CInputDeviceManagerWin32::CreateGamepadPS4ForPC()
{
	RED_ASSERT( ::SIsMainThread() );

	RED_ASSERT( m_isPS4ForPCGamepadInit);
	CInputDeviceGamepadPS4ForPC* inputDeviceGamepad = nullptr;
	if ( m_isPS4ForPCGamepadInit )
	{
		inputDeviceGamepad = new CInputDeviceGamepadPS4ForPC;
		if ( ! inputDeviceGamepad->Init() )
		{
			delete inputDeviceGamepad;
			inputDeviceGamepad = nullptr;
		}
	}

	return inputDeviceGamepad;
}
#endif

#ifndef NO_EDITOR
IInputEditorInterface* CInputDeviceManagerWin32::GetEditorInterface()
{
	// FIXME: Recursive asserts...
	// FIXME: Cheat and make sure not the game without NO_EDITOR
	//RED_ASSERT( dynamic_cast< CEditorEngine* >( GEngine ), TXT("GetEditorInterface - !!!Editor only!!!" ));

	return this;
}
#endif // !NO_EDITOR

#ifndef NO_EDITOR
void CInputDeviceManagerWin32::SetInputCaptureOverride( Bool enable )
{
	// If main thread, also do it know in case somebody relied on this before the next tick (some modal loop).
	// Yes, this is kind of a hack, but so is how all the editor/game input interacts.
	if ( ::SIsMainThread() && enable )
	{
#if defined( RED_ASSERTS_ENABLED ) && !defined( NO_EDITOR )
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_assertMutex );
#endif // defined( RED_ASSERTS_ENABLED ) && !defined( NO_EDITOR )

		ForEach( m_dinputInterfaceList, []( IDInputInterface* dinputInterface) { dinputInterface->Unacquire(); } );
	}

	RequestReset();

	m_inputCaptureOverride.SetValue( enable );


}
#endif // !NO_EDITOR

#ifndef NO_EDITOR
void CInputDeviceManagerWin32::SetAssertHookInputCaptureOverride( Bool enable )
{
# if defined( RED_ASSERTS_ENABLED )
	// So retarded, but allow the assert handler to restore mouse/keyboard control. It suspends our threads, so need to unacquire asap.
	// Don't set capture mode to none then restore it - think what happens when (not IF) multiple assert handlers fire off from
	// different threads at once. Just unacquire the device - we'll reacquire the next time input is updating normally again.
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_assertMutex );

	RequestReset();

	ForEach( m_dinputInterfaceList, []( IDInputInterface* dinputInterface) { dinputInterface->Unacquire(); } );
# endif // RED_ASSERTS_ENABLED
}
#endif // !NO_EDITOR

#ifndef NO_EDITOR
IInputDeviceTablet* CInputDeviceManagerWin32::GetTablet() const 
{
#ifndef NO_TABLET_INPUT_SUPPORT
	return m_editorTablet;
#else
	return nullptr;
#endif
}
#endif // !NO_EDITOR

void CInputDeviceManagerWin32::OnGameInputMode( Bool enabled )
{
	RED_ASSERT( ::SIsMainThread() );
	SetCaptureMode( enabled ? GAME_DINPUT_CAPTUREMODE : DEFAULT_DINPUT_CAPTUREMODE );
}

const CInputDeviceManagerWin32::SDirectInputKeyboardKeyMapping CInputDeviceManagerWin32::DIRECTINPUT_KEYBOARD_KEY_MAPPING[] =
{
	{ DIK_BACKSPACE, IInputDeviceKeyboard::EKey::K_Backspace },
	{ DIK_TAB, IInputDeviceKeyboard::EKey::K_Tab },
	{ DIK_RETURN, IInputDeviceKeyboard::EKey::K_Enter },
	{ DIK_ESCAPE, IInputDeviceKeyboard::EKey::K_Escape },  
	{ DIK_LSHIFT, IInputDeviceKeyboard::EKey::K_LShift },
	{ DIK_RSHIFT, IInputDeviceKeyboard::EKey::K_RShift },
	{ DIK_LALT, IInputDeviceKeyboard::EKey::K_Alt },
	{ DIK_RALT, IInputDeviceKeyboard::EKey::K_Alt },
	{ DIK_LCONTROL, IInputDeviceKeyboard::EKey::K_LControl },
	{ DIK_RCONTROL, IInputDeviceKeyboard::EKey::K_RControl },
	{ DIK_SPACE, IInputDeviceKeyboard::EKey::K_Space },
	{ DIK_PAUSE, IInputDeviceKeyboard::EKey::K_Pause },
	{ DIK_END, IInputDeviceKeyboard::EKey::K_End },
	{ DIK_HOME, IInputDeviceKeyboard::EKey::K_Home },
	{ DIK_DELETE, IInputDeviceKeyboard::EKey::K_Delete },
	{ DIK_INSERT, IInputDeviceKeyboard::EKey::K_Insert },
	{ DIK_PRIOR, IInputDeviceKeyboard::EKey::K_PageUp },
	{ DIK_NEXT, IInputDeviceKeyboard::EKey::K_PageDown },  
	{ DIK_NUMLOCK, IInputDeviceKeyboard::EKey::K_NumLock },
	{ DIK_SCROLL, IInputDeviceKeyboard::EKey::K_ScrollLock },
	{ DIK_CAPITAL, IInputDeviceKeyboard::EKey::K_CapsLock },
	{ DIK_MINUS, IInputDeviceKeyboard::EKey::K_Minus },
	{ DIK_EQUALS, IInputDeviceKeyboard::EKey::K_Equals },
	{ DIK_LBRACKET, IInputDeviceKeyboard::EKey::K_LeftBracket },
	{ DIK_RBRACKET, IInputDeviceKeyboard::EKey::K_RightBracket },
	{ DIK_SEMICOLON, IInputDeviceKeyboard::EKey::K_Semicolon },
	{ DIK_APOSTROPHE , IInputDeviceKeyboard::EKey::K_SingleQuote },
	{ DIK_BACKSLASH, IInputDeviceKeyboard::EKey::K_Backslash },
	{ DIK_COMMA, IInputDeviceKeyboard::EKey::K_Comma },
	{ DIK_PERIOD, IInputDeviceKeyboard::EKey::K_Period },
	{ DIK_SLASH, IInputDeviceKeyboard::EKey::K_Slash },
	{ DIK_GRAVE, IInputDeviceKeyboard::EKey::K_Tilde },  
	{ DIK_NUMPAD0, IInputDeviceKeyboard::EKey::K_NumPad0 },
	{ DIK_NUMPAD1, IInputDeviceKeyboard::EKey::K_NumPad1 },
	{ DIK_NUMPAD2, IInputDeviceKeyboard::EKey::K_NumPad2 },
	{ DIK_NUMPAD3, IInputDeviceKeyboard::EKey::K_NumPad3 },
	{ DIK_NUMPAD4, IInputDeviceKeyboard::EKey::K_NumPad4 },
	{ DIK_NUMPAD5, IInputDeviceKeyboard::EKey::K_NumPad5 },
	{ DIK_NUMPAD6, IInputDeviceKeyboard::EKey::K_NumPad6 },
	{ DIK_NUMPAD7, IInputDeviceKeyboard::EKey::K_NumPad7 },
	{ DIK_NUMPAD8, IInputDeviceKeyboard::EKey::K_NumPad8 },
	{ DIK_NUMPAD9, IInputDeviceKeyboard::EKey::K_NumPad9 },
	{ DIK_MULTIPLY, IInputDeviceKeyboard::EKey::K_NumStar },
	{ DIK_SUBTRACT, IInputDeviceKeyboard::EKey::K_NumMinus },
	{ DIK_ADD, IInputDeviceKeyboard::EKey::K_NumPlus },
	{ DIK_DIVIDE, IInputDeviceKeyboard::EKey::K_NumSlash },
	{ DIK_DECIMAL, IInputDeviceKeyboard::EKey::K_NumPeriod },    
	{ DIK_UP, IInputDeviceKeyboard::EKey::K_Up },
	{ DIK_DOWN, IInputDeviceKeyboard::EKey::K_Down },
	{ DIK_LEFT, IInputDeviceKeyboard::EKey::K_Left },
	{ DIK_RIGHT, IInputDeviceKeyboard::EKey::K_Right },       
	{ DIK_A, IInputDeviceKeyboard::EKey::K_A },
	{ DIK_B, IInputDeviceKeyboard::EKey::K_B },
	{ DIK_C, IInputDeviceKeyboard::EKey::K_C },
	{ DIK_D, IInputDeviceKeyboard::EKey::K_D },
	{ DIK_E, IInputDeviceKeyboard::EKey::K_E },
	{ DIK_F, IInputDeviceKeyboard::EKey::K_F },
	{ DIK_G, IInputDeviceKeyboard::EKey::K_G },
	{ DIK_H, IInputDeviceKeyboard::EKey::K_H },
	{ DIK_I, IInputDeviceKeyboard::EKey::K_I },
	{ DIK_J, IInputDeviceKeyboard::EKey::K_J },
	{ DIK_K, IInputDeviceKeyboard::EKey::K_K },
	{ DIK_L, IInputDeviceKeyboard::EKey::K_L },
	{ DIK_M, IInputDeviceKeyboard::EKey::K_M },
	{ DIK_N, IInputDeviceKeyboard::EKey::K_N },
	{ DIK_O, IInputDeviceKeyboard::EKey::K_O },
	{ DIK_P, IInputDeviceKeyboard::EKey::K_P },
	{ DIK_Q, IInputDeviceKeyboard::EKey::K_Q },
	{ DIK_R, IInputDeviceKeyboard::EKey::K_R },
	{ DIK_S, IInputDeviceKeyboard::EKey::K_S },
	{ DIK_T, IInputDeviceKeyboard::EKey::K_T },
	{ DIK_U, IInputDeviceKeyboard::EKey::K_U },
	{ DIK_V, IInputDeviceKeyboard::EKey::K_V },
	{ DIK_W, IInputDeviceKeyboard::EKey::K_W },
	{ DIK_X, IInputDeviceKeyboard::EKey::K_X },
	{ DIK_Y, IInputDeviceKeyboard::EKey::K_Y },
	{ DIK_Z, IInputDeviceKeyboard::EKey::K_Z },
	{ DIK_0, IInputDeviceKeyboard::EKey::K_0 },
	{ DIK_1, IInputDeviceKeyboard::EKey::K_1 },
	{ DIK_2, IInputDeviceKeyboard::EKey::K_2 },
	{ DIK_3, IInputDeviceKeyboard::EKey::K_3 },
	{ DIK_4, IInputDeviceKeyboard::EKey::K_4 },
	{ DIK_5, IInputDeviceKeyboard::EKey::K_5 },
	{ DIK_6, IInputDeviceKeyboard::EKey::K_6 },
	{ DIK_7, IInputDeviceKeyboard::EKey::K_7 },
	{ DIK_8, IInputDeviceKeyboard::EKey::K_8 },
	{ DIK_9, IInputDeviceKeyboard::EKey::K_9 },
	{ DIK_F1, IInputDeviceKeyboard::EKey::K_F1 },
	{ DIK_F2, IInputDeviceKeyboard::EKey::K_F2 },
	{ DIK_F3, IInputDeviceKeyboard::EKey::K_F3 },
	{ DIK_F4, IInputDeviceKeyboard::EKey::K_F4 },
	{ DIK_F5, IInputDeviceKeyboard::EKey::K_F5 },
	{ DIK_F6, IInputDeviceKeyboard::EKey::K_F6 },
	{ DIK_F7, IInputDeviceKeyboard::EKey::K_F7 },
	{ DIK_F8, IInputDeviceKeyboard::EKey::K_F8 },
	{ DIK_F9, IInputDeviceKeyboard::EKey::K_F9 },
	{ DIK_F10, IInputDeviceKeyboard::EKey::K_F10 },
	{ DIK_F11, IInputDeviceKeyboard::EKey::K_F11 },
	{ DIK_F12, IInputDeviceKeyboard::EKey::K_F12 },
	{ DIK_SYSRQ, IInputDeviceKeyboard::EKey::K_PrintScrn }, 
};

const CName CInputDeviceManagerWin32::GetLastUsedDeviceName() const 
{
	return m_lastUsedDeviceName;
}

void CInputDeviceManagerWin32::OnWindowsInput(WPARAM wParam, LPARAM lParam)
{
	for( IInputRawInput* rawInput : m_inputRawInputList )
	{
		rawInput->OnWindowsInput( wParam, lParam );
	}
}

void CInputDeviceManagerWin32::CreateAndAddRawInputKeyboardDevice()
{
	CInputDeviceKeyboardRawInput* inputDeviceKeyboardRawInput = CreateKeyboardRawInputDevice();
	if ( inputDeviceKeyboardRawInput )
	{
		m_inputDeviceList.PushBack( inputDeviceKeyboardRawInput );
		m_inputRawInputList.PushBack( inputDeviceKeyboardRawInput );
	}
}

void CInputDeviceManagerWin32::CreateAndAddDirectInputKeyboardDevice()
{
	CInputDeviceKeyboardDInput* inputDeviceKeyboard = CreateKeyboardDevice();
	if ( inputDeviceKeyboard )
	{
		m_inputDeviceList.PushBack( inputDeviceKeyboard );
		m_dinputInterfaceList.PushBack( inputDeviceKeyboard );
	}
}
