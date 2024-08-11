/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#ifndef RED_FINAL_BUILD

#include <user_service/user_service_defs.h>

#include "inputDevicePlatformDataOrbis.h"

#include "../../common/engine/inputDeviceKeyboard.h"

//////////////////////////////////////////////////////////////////////////
// CInputDeviceKeyboardOrbisDebug
//////////////////////////////////////////////////////////////////////////
class CInputDeviceKeyboardOrbisDebug : public IInputDeviceKeyboard
{
private:
	typedef IInputDeviceKeyboard TBaseClass;

private:
	SInputDevicePlatformDataOrbis			m_platformData;
	Bool									m_keyDown[ IK_Count ];

public:
											CInputDeviceKeyboardOrbisDebug( SceUserServiceUserId userId );
	virtual									~CInputDeviceKeyboardOrbisDebug();
	Bool									Init();

	virtual void							Update( BufferedInput& outBufferedInput ) override;
	virtual void							Reset() override;

	virtual SInputDevicePlatformData*		GetPlatformData() override final;
};

#endif // ! RED_FINAL_BUILD
