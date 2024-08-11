/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "../../common/engine/inputDeviceKeyboard.h"

#if !defined( RED_FINAL_BUILD )

#include "../../common/engine/rawInputGamepadReading.h"

//////////////////////////////////////////////////////////////////////////
// CInputDeviceKeyboardDurangoDebug
//////////////////////////////////////////////////////////////////////////
class CInputDeviceKeyboardDurangoDebug : public IInputDeviceKeyboard
{
private:
	typedef IInputDeviceKeyboard TBaseClass;

private:
	SRawKeyboardEvent						m_rawKeyboardEvents[ CRawKeyboardReadingBuffer::READBUFSIZE ];
	Uint32									m_numEvents;

public:
											CInputDeviceKeyboardDurangoDebug();
	virtual									~CInputDeviceKeyboardDurangoDebug();

public:
	virtual void							Update( BufferedInput& outBufferedInput ) override;
	virtual void							Reset() override;
};

#endif // !defined( RED_FINAL_BUILD ) || defined( DEMO_BUILD )