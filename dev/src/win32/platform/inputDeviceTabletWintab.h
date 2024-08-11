/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#ifndef NO_TABLET_INPUT_SUPPORT

#define NOWTFUNCTIONS
// Tablet includes
#include "../../../external/wintab/include/wintab.h"
#include "../../../external/wintab/include/wintabx.h"
#define NPACKETQSIZE 32
#define PACKETDATA	(PK_CURSOR | PK_X | PK_Y | PK_NORMAL_PRESSURE | PK_BUTTONS | PK_ORIENTATION)
#define PACKETMODE	0
#include "../../../external/wintab/include/pktdef.h"

#include "../../common/engine/inputDeviceTablet.h"

class CInputDeviceTabletWintab : public IInputDeviceTablet
{
private:
	typedef IInputDeviceTablet TBaseClass;

private:
	HCTX									m_contextHandle;
	Uint32									m_activeCursor;
	Uint32									m_oldCursor;
	Uint8									m_pressureButton;
	Uint32									m_pressureYesButtonOrg;
	Uint32									m_pressureYesButtonExt;
	Uint32									m_pressureNoButtonOrg;
	Uint32									m_pressureNoButtonExt;
	Int32									m_vkLeftMouseButton;
	Float									m_prevPressure;
	Float									m_pressure;
	Bool									m_prevLeftMouseButtonDown;
	Bool									m_invert;
	Bool									m_enabled;

public:
											CInputDeviceTabletWintab();
	virtual									~CInputDeviceTabletWintab();

public:
	virtual void							Update( BufferedInput& outBufferedInput ) override;
	virtual void							Reset() override;

public:
	virtual void							SetEnabled( Bool enabled ) override;
	virtual Float							GetPresureState() const override { return m_pressure; }

private:
	Float									UpdatePressure();
	Uint32									GetPressure( const PACKET& packet );

public:
	Bool									Init( HWND hWnd );

private:
	void									TabletSetup( PLOGCONTEXT pLc );
	void									PressureInit();

private:
	void									Cleanup();

};

#endif // NO_TABLET_INPUT_SUPPORT