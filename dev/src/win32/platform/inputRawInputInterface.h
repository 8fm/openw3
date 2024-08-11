/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */
 
#pragma once

 class IInputRawInput
 {
 public:
	 virtual ~IInputRawInput() {}

	 virtual void SetWindowHandle( HWND hWnd ) = 0;
	 virtual void OnWindowsInput( WPARAM wParam, LPARAM lParam ) = 0;

 };
