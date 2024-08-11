/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "../../common/engine/inputDeviceManager.h"

#include <user_service/user_service_defs.h>

#include "orbisApiCall.h"

#if !defined( RED_FINAL_BUILD )
#	define RED_ORBIS_MOUSE_KEYBOARD
#endif

struct SOrbisGamepadEvent;
class IInputDeviceGamepad;

//////////////////////////////////////////////////////////////////////////
// CInputDeviceManagerOrbis
//////////////////////////////////////////////////////////////////////////
class CInputDeviceManagerOrbis : public IInputDeviceManager
{
private:
	TDynArray< IInputDevice* >		m_inputDeviceList;

	IInputDeviceGamepad*			m_inputDeviceGamepad;

	TOrbisUserEvent					m_onUserSelected;
	Bool							m_promiscuousMode;
	Bool							m_enableUserSelection;
	SceUserServiceUserId			m_activeUser;

public:
									CInputDeviceManagerOrbis();
	virtual							~CInputDeviceManagerOrbis() override final;

	virtual Bool					Init() override final;
	virtual void					Update( BufferedInput& outBufferedInput ) override final;
	virtual void					Shutdown() override final;

	Bool							SetActiveUserPromiscuous( const TOrbisUserEvent& callback );
	Bool							SetActiveUser( SceUserServiceUserId userId, Bool kbm = false );

	virtual void					RequestReset() override final;

	RED_INLINE Bool					PromiscuousModeActive() const { return m_promiscuousMode; }
	RED_INLINE void					EnableUserSelection( Bool enabled ) { m_enableUserSelection = enabled; }

private:
	void							UpdateDevice( BufferedInput& outBufferedInput, Uint32 i );
	Bool							PromiscuousModeUpdate( BufferedInput& outBufferedInput, Uint32 i );

private:
	template< typename TDevice >
	TDevice*						CreateDevice( SceUserServiceUserId userId );
	Bool							CreateDevices( SceUserServiceUserId userId, Bool kbm );
	void							ClearDeviceList();

	Bool							InitPadLibrary();

	void							OnPadEvent( const SOrbisGamepadEvent& event );

#ifdef RED_ORBIS_MOUSE_KEYBOARD
private:
	Bool							InitKeyboardLibrary();
	Bool							InitMouseLibrary();
#endif

private:
	Bool							InitSystemGestureLibrary();

public:
	virtual const CName 			GetLastUsedDeviceName() const override;

};
