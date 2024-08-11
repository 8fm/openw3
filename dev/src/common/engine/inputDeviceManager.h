/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "inputBufferedInputEvent.h"
#include "inputGameInputModeListener.h"
#include "inputDeviceCommandsManager.h"
#include "notifier.h"

//////////////////////////////////////////////////////////////////////////
// Forward Declarations
//////////////////////////////////////////////////////////////////////////
class IInputDevice;
class IInputEditorInterface;

//////////////////////////////////////////////////////////////////////////
// Types
//////////////////////////////////////////////////////////////////////////

enum class EControllerEventType
{
	CET_Disconnected,
	CET_Reconnected
};

enum class ESteamController
{
	AutoEnable		= 0,
	ForceEnable		= 1,
	ForceDisable	= 2,
};

//////////////////////////////////////////////////////////////////////////
// IInputDeviceManager
//////////////////////////////////////////////////////////////////////////
class IInputDeviceManager : public IGameInputModeListener, public Events::CNotifier< EControllerEventType >
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Engine );

protected:
	CInputDeviceCommandsManager				m_deviceCommandsManager;

public:
											IInputDeviceManager();
	virtual									~IInputDeviceManager() override;

	virtual Bool							Init() = 0;
	virtual void							Update( TDynArray< SBufferedInputEvent >& outBufferedInput ) = 0;
	virtual void							Shutdown() = 0;
	virtual void							RequestReset() = 0;

	RED_INLINE CInputDeviceCommandsManager&	Commands() { return m_deviceCommandsManager; }

	virtual const CName 					GetLastUsedDeviceName() const = 0;

#ifndef NO_EDITOR
public:
	virtual IInputEditorInterface*			GetEditorInterface() { return nullptr; }
#endif
};
