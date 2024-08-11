/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "inputDeviceManagerOrbis.h"

#ifdef RED_ORBIS_MOUSE_KEYBOARD

#include "inputDeviceKeyboardOrbisDebug.h"

#include <dbg_keyboard.h>

#include "../../common/engine/inputUtils.inl"

namespace
{
	EInputKey m_orbisKeyMapping [IK_Count] = {IK_None};
	void SetupOrbisKeyMapping();
}

//////////////////////////////////////////////////////////////////////////
// CInputDeviceKeyboardOrbisDebug
//////////////////////////////////////////////////////////////////////////
CInputDeviceKeyboardOrbisDebug::CInputDeviceKeyboardOrbisDebug( SceUserServiceUserId userId )
:	m_platformData( userId )
{
	SetupOrbisKeyMapping();

	Red::System::MemoryZero( m_keyDown, sizeof( m_keyDown ) );
}

CInputDeviceKeyboardOrbisDebug::~CInputDeviceKeyboardOrbisDebug()
{
	if ( m_platformData.IsPortHandleValid() )
	{
		::sceDbgKeyboardClose( m_platformData.m_portHandle );
		m_platformData.m_portHandle = SInputDevicePlatformDataOrbis::INVALID_PORT_HANDLE;
	}
}

Bool CInputDeviceKeyboardOrbisDebug::Init()
{
	const Int32 portHandle = ::sceDbgKeyboardOpen( m_platformData.m_userId, SCE_DBG_KEYBOARD_PORT_TYPE_STANDARD, 0, nullptr );
	if ( portHandle <= SInputDevicePlatformDataOrbis::INVALID_PORT_HANDLE )
	{
		ERR_ENGINE( TXT( "Failed to open debug keyboard for user %d. Internal error code 0x%08x" ), static_cast< Int32 >( m_platformData.m_userId ), portHandle );
		return false;
	}

	m_platformData.m_portHandle = portHandle;

	return true;
}

void CInputDeviceKeyboardOrbisDebug::Reset()
{
	// Do nothing
}

void CInputDeviceKeyboardOrbisDebug::Update( BufferedInput& outBufferedInput )
{
	SceDbgKeyboardData keyboardData[ SCE_DBG_KEYBOARD_MAX_KEYCODES ];
	Red::System::MemoryZero( &keyboardData, sizeof( SceDbgKeyboardData ) );
	const SceInt32 sceErr = ::sceDbgKeyboardRead( m_platformData.m_portHandle, &keyboardData[ 0 ], SCE_DBG_KEYBOARD_MAX_KEYCODES );
	if ( sceErr < 0 )
	{
		ERR_ENGINE( TXT("Failed to read debug keyboard. Internal error code 0x%08x"), sceErr );
		return;
	}

	if ( !keyboardData[ 0 ].connected )
	{
		return;
	}

	const Int32 numEvents = sceErr;

	if ( numEvents == 0 )
	{
		for ( Uint32 i = 0; i < IK_Count; ++i )
		{
			if ( m_keyDown[ i ] )
			{
				outBufferedInput.PushBack( SBufferedInputEvent( static_cast< EInputKey >( i ), IACT_Release, 1.0f ) );
				m_keyDown[ i ] = false;
			}
		}
	}
	
	for ( Int32 i = 0; i < numEvents; ++i )
	{
		const SceDbgKeyboardData& evt = keyboardData[ i ];

		Bool keyState[ IK_Count ] = {0};

		Uint32 mod = evt.modifierKey;
		keyState[IK_LControl] = (mod & SCE_DBG_KEYBOARD_MKEY_L_CTRL) != 0;
		keyState[IK_RControl] = (mod & SCE_DBG_KEYBOARD_MKEY_R_CTRL) != 0;
		keyState[IK_LShift] =	(mod & SCE_DBG_KEYBOARD_MKEY_L_SHIFT) != 0;
		keyState[IK_RShift] =	(mod & SCE_DBG_KEYBOARD_MKEY_R_SHIFT) != 0;
		keyState[IK_Alt] =		((mod & SCE_DBG_KEYBOARD_MKEY_L_ALT) | (mod & SCE_DBG_KEYBOARD_MKEY_R_ALT)) != 0;

		for (Int32 j = 0; j < evt.length; ++j)
		{
			Uint16 sce_keycode = evt.keyCode[j];
			EInputKey red_keycode = IK_None;

			if (sce_keycode < IK_Count)
			{
				red_keycode = m_orbisKeyMapping [sce_keycode];
			}

			if (red_keycode != IK_None)
			{
				keyState[red_keycode] = true;
			}
		}

		for ( Uint32 j = 0; j < IK_Count; ++j )
		{
			if ( m_keyDown[ j ] != keyState[ j ] )
			{
				EInputAction action = keyState[j] ? IACT_Press : IACT_Release;
				outBufferedInput.PushBack( SBufferedInputEvent( static_cast< EInputKey >( j ), action, 1.0f ) );
			}
		}

		Red::System::MemoryCopy( m_keyDown, keyState, sizeof( keyState ) );
	}
}

SInputDevicePlatformData* CInputDeviceKeyboardOrbisDebug::GetPlatformData()
{
	return &m_platformData;
}

namespace
{
	void SetupOrbisKeyMapping()
	{
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_BS				] = IK_Backspace;
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_TAB				] = IK_Tab;
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_ESC				] = IK_Escape;
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_SPACE				] = IK_Space;
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_ENTER				] = IK_Enter;
						    
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_PAGE_UP			] = IK_PageUp;
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_PAGE_DOWN			] = IK_PageDown;
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_END				] = IK_End;
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_HOME				] = IK_Home;
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_LEFT_ARROW		] = IK_Left;
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_UP_ARROW			] = IK_Up;
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_RIGHT_ARROW		] = IK_Right;
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_DOWN_ARROW		] = IK_Down;
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_PRINTSCREEN		] = IK_PrintScrn;
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_INSERT			] = IK_Insert;
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_DELETE			] = IK_Delete;
						    
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_0					] = IK_0;
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_1					] = IK_1;
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_2					] = IK_2;
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_3					] = IK_3;
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_4					] = IK_4;
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_5					] = IK_5;
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_6					] = IK_6;
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_7					] = IK_7;
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_8					] = IK_8;
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_9					] = IK_9;

		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_A					] = IK_A;
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_B					] = IK_B;
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_C					] = IK_C;
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_D					] = IK_D;
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_E					] = IK_E;
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_F					] = IK_F;
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_G					] = IK_G;
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_H					] = IK_H;
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_I					] = IK_I;
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_J					] = IK_J;
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_K					] = IK_K;
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_L					] = IK_L;
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_M					] = IK_M;
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_N					] = IK_N;
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_O					] = IK_O;
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_P					] = IK_P;
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_Q					] = IK_Q;
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_R					] = IK_R;
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_S					] = IK_S;
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_T					] = IK_T;
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_U					] = IK_U;
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_V					] = IK_V;
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_W					] = IK_W;
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_X					] = IK_X;
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_Y					] = IK_Y;
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_Z					] = IK_Z;
						    
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_F1				] = IK_F1;
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_F2				] = IK_F2;
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_F3				] = IK_F3;
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_F4				] = IK_F4;
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_F5				] = IK_F5;
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_F6				] = IK_F6;
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_F7				] = IK_F7;
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_F8				] = IK_F8;
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_F9				] = IK_F9;
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_F10				] = IK_F10;
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_F11				] = IK_F11;
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_F12				] = IK_F12;
						    
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_KPAD_0			] = IK_NumPad0;
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_KPAD_1			] = IK_NumPad1;
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_KPAD_2			] = IK_NumPad2;
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_KPAD_3			] = IK_NumPad3;
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_KPAD_4			] = IK_NumPad4;
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_KPAD_5			] = IK_NumPad5;
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_KPAD_6			] = IK_NumPad6;
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_KPAD_7			] = IK_NumPad7;
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_KPAD_8			] = IK_NumPad8;
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_KPAD_9			] = IK_NumPad9;
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_KPAD_ASTERISK		] = IK_NumStar;
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_KPAD_PLUS			] = IK_NumPlus;
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_KPAD_MINUS		] = IK_NumMinus;
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_KPAD_PERIOD		] = IK_NumPeriod;
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_KPAD_SLASH		] = IK_NumSlash;
						    
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_SEMICOLON			] = IK_Semicolon;
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_EQUAL_101			] = IK_Equals;
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_COMMA				] = IK_Comma;
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_MINUS				] = IK_Minus;
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_PERIOD			] = IK_Period;
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_SLASH				] = IK_Slash;
		m_orbisKeyMapping [ 0x35									] = IK_Tilde;				// Grave does not have a SCE keycode...
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_LEFT_BRACKET_101	] = IK_LeftBracket;
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_BACKSLASH_101		] = IK_Backslash;
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_RIGHT_BRACKET_101	] = IK_RightBracket;
		m_orbisKeyMapping [ SCE_DBG_KEYBOARD_CODE_QUOTATION_101		] = IK_SingleQuote;
	}
}

#endif // ! RED_FINAL_BUILD
