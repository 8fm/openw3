/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#ifndef NO_EDITOR

//////////////////////////////////////////////////////////////////////////
// Forward Declarations
//////////////////////////////////////////////////////////////////////////
class IInputDeviceTablet;

//////////////////////////////////////////////////////////////////////////
// IInputEditorInterface. AKA hacks to avoid refactoring way too much at once.
//////////////////////////////////////////////////////////////////////////
class IInputEditorInterface
{
public:
	virtual void							SetInputCaptureOverride( Bool enable )=0;			// When you need to use Windows messages for input etc. Be careful if enabling/disabling from different threads, so use sparingly!
	virtual void							SetAssertHookInputCaptureOverride( Bool enable )=0; // Because threads may be immediately suspended, so need something more immediate.
	virtual IInputDeviceTablet*				GetTablet() const=0;

protected:
											IInputEditorInterface() {}
											virtual ~IInputEditorInterface() {}
};

#endif // !NO_EDITOR