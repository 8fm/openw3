/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once
#ifndef __INPUT_DEVICE_PLATFORM_DATA_ORBIS_H__
#define __INPUT_DEVICE_PLATFORM_DATA_ORBIS_H__

#include "../../common/engine/inputDevice.h"

#include <user_service/user_service_defs.h>

//////////////////////////////////////////////////////////////////////////
// SInputDevicePlatformDataOrbis
//////////////////////////////////////////////////////////////////////////
struct SInputDevicePlatformDataOrbis : public SInputDevicePlatformData
{
	typedef Int32 TPortHandle;
	static const TPortHandle INVALID_PORT_HANDLE = -1;

	TPortHandle m_portHandle;
	SceUserServiceUserId m_userId;

	SInputDevicePlatformDataOrbis( SceUserServiceUserId userId )
	:	m_portHandle( INVALID_PORT_HANDLE )
	,	m_userId( userId )
	{
	}

	RED_INLINE Bool IsPortHandleValid() const { return m_portHandle != INVALID_PORT_HANDLE; }
};

#endif // __INPUT_DEVICE_PLATFORM_DATA_ORBIS_H__
