/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"

#include "inputDeviceManager.h"

namespace Config
{

	TConfigVar< Bool >  cvForcePad( "Input", "ForcePad", false ); // eventually - AllowMouseKeyboard, etc
	TConfigVar< Bool >  cvForceDisablePad( "Input", "ForceDisablePad", false, eConsoleVarFlag_Save ); // disable gamepad input
	TConfigVar< Int32 >	cvSteamController( "Input", "SteamController", (Int32)ESteamController::AutoEnable, eConsoleVarFlag_Save );
}

// Ugh, hate adding another global, but these need to be created before base engine exists
// Please access the input device manager though base engine and not through this global
IInputDeviceManager* GInputDeviceManager = nullptr;

IInputDeviceManager::IInputDeviceManager()
{
}

IInputDeviceManager::~IInputDeviceManager()
{
	// It seems that virtual destructors get confused when multiple inheritance is involved, so let's manually clear out the listeners
	Events::CNotifier< EControllerEventType >::Clear();
}
