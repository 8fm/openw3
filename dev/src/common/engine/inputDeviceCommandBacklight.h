/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "inputDeviceCommand.h"


class CInputDeviceCommandBacklight : public IInputDeviceCommand
{
	Color	m_color;
	Bool	m_reset;

public:

	CInputDeviceCommandBacklight( const Color& color );

	virtual void PerformCommand( IInputDeviceGamepad*	device )  const;

	RED_INLINE void SetColor( const Color& color )
	{
		m_color = color;
		m_reset = false;
	}

	RED_INLINE void ResetColor()
	{
		m_reset = true;
	}
};
