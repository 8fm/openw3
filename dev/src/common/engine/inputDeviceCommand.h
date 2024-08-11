/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once

class IInputDeviceGamepad;
class IInputDeviceKeyboard;
class IInputDeviceMouse;
class IInputDeviceTablet;

/// @author M.Sobiecki
/// @created 2014-03-31
class IInputDeviceCommand
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Engine );

	Bool m_buffered;

public:

	IInputDeviceCommand() : m_buffered( false ) { }
	virtual ~IInputDeviceCommand() {}

	/// By default - do nothing, should be implemented only for devices that are interested in a command
	/// They are const cause it's performed sequentially over a couple of devices, so the command should not change
	virtual void PerformCommand( IInputDeviceGamepad*	device ) const {}
	virtual void PerformCommand( IInputDeviceKeyboard*	device ) const {}
	virtual void PerformCommand( IInputDeviceMouse*		device ) const {}
	virtual void PerformCommand( IInputDeviceTablet*	device ) const {}

	virtual void AfterCommandPerformed() {}
	
	RED_INLINE void SetBuffered( Bool val ) { m_buffered = val; }
	RED_INLINE Bool IsBuffered() const		{ return m_buffered; }


	friend class CInputDeviceCommandsManager;
};
