/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "..\core\dynarray.h"


class IInputDevice;
class IInputDeviceCommand;
class CInputDeviceCommandBacklight;
class CInputDeviceCommandRumble;

/// @author M.Sobiecki
/// @created 2014-04-02
class CInputDeviceCommandsManager
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Engine );

	TDynArray< IInputDevice* >			m_currentDevice;
	TDynArray< IInputDeviceCommand* >	m_bufferedCommand;

private: // commands pools (so that they are not allocated every time command is required)
	CInputDeviceCommandBacklight*		m_backlightCommandPool;
	CInputDeviceCommandRumble*			m_rumbleCommandPool;

public:

	CInputDeviceCommandsManager();
	~CInputDeviceCommandsManager();

	RED_INLINE void AddCurrentDevice( IInputDevice* device ) { m_currentDevice.PushBack( device ); }
	RED_INLINE void ResetCurrentDevices() { m_currentDevice.ClearFast(); }

	void Update();


	void SetBacklight( const Color& color );
	void ResetBacklight();

	void SetPadVibrate( Float leftVal, Float rightVal );

private:

	void BufferCommand( IInputDeviceCommand* command );
};
