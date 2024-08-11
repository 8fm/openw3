/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "inputDevice.h"
#include "inputDeviceCommand.h"

//////////////////////////////////////////////////////////////////////////
// IInputDeviceTablet
//////////////////////////////////////////////////////////////////////////
class IInputDeviceTablet : public IInputDevice
{
public:
	virtual void								SetEnabled( Bool enabled )=0;
	virtual Float								GetPresureState() const=0;

public:

	virtual void PerformCommand( IInputDeviceCommand* command ) override
	{
		command->PerformCommand( this );
	}

	virtual const CName GetDeviceName() const override
	{
		return CNAME( tablet );
	}
};