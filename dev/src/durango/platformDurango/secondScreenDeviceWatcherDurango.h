/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#if !defined( NO_SECOND_SCREEN )
#include "../../common/platformCommon/secondScreenDeviceWatcher.h"

class CSecondScreenDeviceWatcherDurango: public CSecondScreenDeviceWatcher
{
public:
	CSecondScreenDeviceWatcherDurango( ISecondScreenDeviceDelegate* delegeate );
	~CSecondScreenDeviceWatcherDurango();

	void OnDeviceAddedDurango( CSecondScreenDevice* device );
	void OnDeviceRemovedDurango( const CSecondScreenDevice& device );
	
	//! CSecondScreenDeviceWatcher
public:
	void FindAllDevicesAsync();

private:
	void*	m_watcherDurango;
};
#endif