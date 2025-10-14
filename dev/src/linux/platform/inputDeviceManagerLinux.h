#pragma once

#include "../../common/engine/inputDeviceManager.h"
#include "../../common/engine/inputDeviceKeyboard.h"

//////////////////////////////////////////////////////////////////////////
// Forward Declarations
//////////////////////////////////////////////////////////////////////////
class IInputDevice;
class IInputDeviceMouse;

//////////////////////////////////////////////////////////////////////////
// CInputDeviceManagerLinux
//////////////////////////////////////////////////////////////////////////
class CInputDeviceManagerLinux
	: public IInputDeviceManager
{
private:
	CName									m_lastUsedDeviceName;

public:
											CInputDeviceManagerLinux();
	virtual									~CInputDeviceManagerLinux();

public:
	virtual Bool							Init() override;
	virtual void							Shutdown() override;
	virtual void							Update( TDynArray< SBufferedInputEvent >& outBufferedInput ) override;
	virtual void							RequestReset() override;

public:
	virtual const CName 					GetLastUsedDeviceName() const override;
};
