/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#if !defined( NO_SECOND_SCREEN )

#include "secondScreenDevice.h"

//! CSecondScreenDeviceWatcher class serves as unified for all platforms access point to second screen devices
//! Platform dependent implementation of CSecondScreenDeviceWatcher:
//! Xbox One: CSecondScreenDeviceWatcherDurango
//! PC		: CSecondScreenDeviceWatcherPC
class CSecondScreenDeviceWatcher
{
public:
	CSecondScreenDeviceWatcher( ISecondScreenDeviceDelegate* delegeate ){ m_delegate = delegeate; }
	virtual ~CSecondScreenDeviceWatcher(){}

	ISecondScreenDeviceDelegate* GetDelegate() const { return m_delegate; }
	
	virtual void FindAllDevicesAsync() = 0;

	virtual void Update( Float timeDelta ) { RED_UNUSED(timeDelta); };

protected:

	ISecondScreenDeviceDelegate* m_delegate;
	
};
#endif