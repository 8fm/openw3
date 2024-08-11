/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "inputDeviceCommand.h"

#define MAX_RUMBLE_MOTORS_NUM 3

class CInputDeviceCommandRumble : public IInputDeviceCommand
{
private:
	Float m_leftVal;
	Float m_rightVal;

public:

	CInputDeviceCommandRumble();

	virtual void PerformCommand( IInputDeviceGamepad*	device )  const;
	virtual void AfterCommandPerformed() override;

	void SetPadVibrate( Float leftVal, Float rightVal );
};
