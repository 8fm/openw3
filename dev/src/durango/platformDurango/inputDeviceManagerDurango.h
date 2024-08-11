/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "../../common/engine/inputDeviceManager.h"

#include "userProfileDurango.h"

//////////////////////////////////////////////////////////////////////////
// Forward Declarations
//////////////////////////////////////////////////////////////////////////
class IInputDeviceGamepad;
class CInputDeviceGamepadDurango;

#ifndef RED_FINAL_BUILD
class CInputDeviceKeyboardDurangoDebug;
#endif

//////////////////////////////////////////////////////////////////////////
// CInputDeviceManagerDurango
//////////////////////////////////////////////////////////////////////////
class CInputDeviceManagerDurango : public IInputDeviceManager, public IUserProfileGamepadListenerDurango
{
private:
	TDynArray< IInputDevice* >				m_inputDeviceList;
	CInputDeviceGamepadDurango*				m_inputDeviceGamepadDurango;
	IGamepad^								m_queuedPad;
	Uint64									m_activeGamepadId;
	Bool									m_activePadDisconnected;
	Bool									m_padChangeQueued;



public:
											CInputDeviceManagerDurango();
	virtual									~CInputDeviceManagerDurango();

	virtual Bool							Init() override;
	virtual void							Shutdown() override;

	virtual void							Update( TDynArray< SBufferedInputEvent >& outBufferedInput ) override;
	virtual void							RequestReset() override;

	Windows::Xbox::Input::IGamepad^			GetActiveGamepad() const;

private:
	virtual void							OnUserProfileGamepadChanged( IGamepad^ gamepad ) override final;
	void									UpdateProfileGamepadChange();

public:
	virtual const CName						GetLastUsedDeviceName() const override;

};
