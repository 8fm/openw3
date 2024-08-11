#include "build.h"
#include "inputUserConfig.h"
#include "../core/configVar.h"
#include "../core/configVarSystem.h"
#include "game.h"
#include "inputManager.h"

namespace Config
{
	TConfigVar< Bool >		cvInputInvertCameraX( "Input", "InvertCameraX", false, eConsoleVarFlag_Save );
	TConfigVar< Bool >		cvInputInvertCameraY( "Input", "InvertCameraY", false, eConsoleVarFlag_Save );
	TConfigVar< Bool >		cvInputInvertCameraXOnMouse( "Input", "InvertCameraXOnMouse", false, eConsoleVarFlag_Save );
	TConfigVar< Bool >		cvInputInvertCameraYOnMouse( "Input", "InvertCameraYOnMouse", false, eConsoleVarFlag_Save );
	TConfigVar< Bool >		cvInputCameraAutoRotX( "Input", "CameraAutoRotX", true, eConsoleVarFlag_Save );
	TConfigVar< Bool >		cvInputCameraAutoRotY( "Input", "CameraAutoRotY", true, eConsoleVarFlag_Save );
	TConfigVar<Bool>		cvPadVibrationEnabled( "Input", "PadVibrationEnabled", true, eConsoleVarFlag_Save );

	TConfigVar< Float, Validation::FloatRange< 1, 1000, 100 > >		cvInputMouseSensitivity( "Input", "MouseSensitivity", 0.3f, eConsoleVarFlag_Save );
	TConfigVar< Float, Validation::FloatRange< 0, 1000, 1000 > >	cvInputKeyboardCooldownSpeed( "Input", "KeyboardCooldownSpeed", 0.1f, eConsoleVarFlag_Save );

	TConfigVar< Float, Validation::FloatRange< 1, 1000, 100 > >		cvInputRightStickCameraSensitivity( "Input", "RightStickCameraSensitivity", 1.0f, eConsoleVarFlag_Save );
	TConfigVar< Float, Validation::FloatRange< 1, 1000, 100 > >		cvInputRightStickAimSensitivity( "Input", "RightStickAimSensitivity", 1.0f, eConsoleVarFlag_Save );
}

Float SInputUserConfig::GetMouseSensitivity()
{
	return Config::cvInputMouseSensitivity.Get();
}

Float SInputUserConfig::GetRightStickCameraSensitivity()
{
	return Config::cvInputRightStickCameraSensitivity.Get();
}

Float SInputUserConfig::GetRightStickAimSensitivity()
{
	return Config::cvInputRightStickAimSensitivity.Get();
}

Float SInputUserConfig::GetKeyboardCooldownSpeed()
{
	return Config::cvInputKeyboardCooldownSpeed.Get();
}

Bool SInputUserConfig::GetIsInvertCameraX()
{
	return Config::cvInputInvertCameraX.Get();
}

Bool SInputUserConfig::GetIsInvertCameraY()
{
	return Config::cvInputInvertCameraY.Get();
}

Bool SInputUserConfig::GetIsInvertCameraXOnMouse()
{
	return Config::cvInputInvertCameraXOnMouse.Get();
}

Bool SInputUserConfig::GetIsInvertCameraYOnMouse()
{
	return Config::cvInputInvertCameraYOnMouse.Get();
}


Bool SInputUserConfig::GetIsCameraAutoRotX()
{
	return Config::cvInputCameraAutoRotX.Get();
}

Bool SInputUserConfig::GetIsCameraAutoRotY()
{
	return Config::cvInputCameraAutoRotY.Get();
}

void SInputUserConfig::SetMouseSensitivity(const Float value)
{
	Config::cvInputMouseSensitivity.Set(value);
}

void SInputUserConfig::SetKeyboardCooldownSpeed(const Float value)
{
	Config::cvInputKeyboardCooldownSpeed.Set(value);
}

void SInputUserConfig::SetIsInvertCameraX(const Bool value)
{
	Config::cvInputInvertCameraX.Set(value);
}

void SInputUserConfig::SetIsInvertCameraY(const Bool value)
{
	Config::cvInputInvertCameraY.Set(value);
}

void SInputUserConfig::SetIsInvertCameraXOnMouse(const Bool value)
{
	Config::cvInputInvertCameraXOnMouse.Set(value);
}

void SInputUserConfig::SetIsInvertCameraYOnMouse(const Bool value)
{
	Config::cvInputInvertCameraYOnMouse.Set(value);
}

void SInputUserConfig::SetIsCameraAutoRotX(const Bool value)
{
	Config::cvInputCameraAutoRotX.Set(value);
}

void SInputUserConfig::SetIsCameraAutoRotY(const Bool value)
{
	Config::cvInputCameraAutoRotY.Set(value);
}

Bool SInputUserConfig::GetIsVibrationEnabled(Bool& outIsVibration)
{
	outIsVibration = Config::cvPadVibrationEnabled.Get();
	return true;
}

Bool SInputUserConfig::SetIsVibrationEnabled(const Bool isVibration)
{
	Config::cvPadVibrationEnabled.Set( isVibration );
	return true;
}

void SInputUserConfig::Save()
{
	SConfig::GetInstance().Save();
	if( GGame != nullptr )
	{
		if( GGame->GetInputManager() != nullptr )
		{
			GGame->GetInputManager()->SaveUserMappings();
		}
	}
}
