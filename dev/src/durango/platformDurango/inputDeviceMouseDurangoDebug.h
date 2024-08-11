/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "../../common/engine/inputDeviceMouse.h"

#if !defined( RED_FINAL_BUILD )

#include "../../common/engine/rawInputGamepadReading.h"

class CInputDeviceMouseDurangoDebug : public IInputDeviceMouse
{
private:
	typedef IInputDeviceMouse TBaseClass;

	Bool									m_buttonDownEventResetTable[ EButton::Count ];

public:
											CInputDeviceMouseDurangoDebug();
	virtual									~CInputDeviceMouseDurangoDebug();
	virtual void							Update( BufferedInput& outBufferedInput ) override;
	virtual void							Reset() override;
};

#endif // !defined( RED_FINAL_BUILD ) || defined( DEMO_BUILD )
