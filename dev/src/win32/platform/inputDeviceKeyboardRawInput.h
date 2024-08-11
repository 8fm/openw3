/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */
 
#include "../../common/engine/inputDeviceKeyboard.h"
#include "inputRawInputInterface.h"
#include <winuser.h>

 class CInputDeviceKeyboardRawInput : public IInputDeviceKeyboard, public IInputRawInput, private Red::System::NonCopyable
 {
 public:
	 enum class EKeyEvent
	 {
		 Pressed,
		 None,
		 Released,
	 };

	 CInputDeviceKeyboardRawInput();
	 ~CInputDeviceKeyboardRawInput();

	 // IInputDeviceKeyboard interface
	 virtual void Update(BufferedInput& outBufferedInput) override;
	 virtual void Reset() override;

	 // IInputRawInput interface
	 virtual void SetWindowHandle( HWND hWnd ) override;
	 virtual void OnWindowsInput( WPARAM wParam, LPARAM lParam ) override;

 protected:
 private:
	 void LazyInit( HWND hWnd );
	 void ExtractInputFromRawData( const RAWKEYBOARD& rawKeyboard );

	 RAWINPUTDEVICE m_device;
	 EKeyEvent m_keyStates[IK_Count];
	 Bool m_isInitialized;

};
