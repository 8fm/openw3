/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "../../common/engine/inputDeviceMouse.h"

#ifndef RED_FINAL_BUILD

#include <user_service/user_service_defs.h>

#include "inputDevicePlatformDataOrbis.h"

//////////////////////////////////////////////////////////////////////////
// CInputDeviceKeyboardOrbisDebug
//////////////////////////////////////////////////////////////////////////
class CInputDeviceMouseOrbisDebug : public IInputDeviceMouse
{
private:
	typedef IInputDeviceMouse TBaseClass;

private:
	SInputDevicePlatformDataOrbis			m_platformData;
	bool									m_wasMiddleDown;
	bool									m_wasRightDown;
	bool									m_wasLeftDown;

public:
											CInputDeviceMouseOrbisDebug( SceUserServiceUserId userId );
	virtual									~CInputDeviceMouseOrbisDebug();
	Bool									Init();

	virtual void							Update( BufferedInput& outBufferedInput ) override;
	virtual void							Reset() override;

	virtual SInputDevicePlatformData*		GetPlatformData() override final;
};

#endif // ! RED_FINAL_BUILD
