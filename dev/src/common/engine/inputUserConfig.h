#pragma once

///////////////////////////////////////////////////////////////////////////////

struct SInputUserConfig
{
	static Float GetMouseSensitivity();
	static Float GetRightStickCameraSensitivity();
	static Float GetRightStickAimSensitivity();
	static Float GetKeyboardCooldownSpeed();
	static Bool	GetIsInvertCameraX();
	static Bool	GetIsInvertCameraY();
	static Bool	GetIsInvertCameraXOnMouse();
	static Bool	GetIsInvertCameraYOnMouse();
	static Bool	GetIsCameraAutoRotX();
	static Bool	GetIsCameraAutoRotY();

	static void SetMouseSensitivity(const Float value);
	static void SetKeyboardCooldownSpeed(const Float value);
	static void SetIsInvertCameraX(const Bool value);
	static void SetIsInvertCameraY(const Bool value);
	static void SetIsInvertCameraXOnMouse(const Bool value);
	static void SetIsInvertCameraYOnMouse(const Bool value);
	static void SetIsCameraAutoRotX(const Bool value);
	static void SetIsCameraAutoRotY(const Bool value);

	static Bool GetIsVibrationEnabled( Bool& outIsVibration );
	static Bool SetIsVibrationEnabled( const Bool isVibration );

	static void Save();
};
