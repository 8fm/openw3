#pragma once
#if !defined( NO_SECOND_SCREEN )

#include "../../common/platformCommon/secondScreenDevice.h"

//! This class serves as access point to second screen devices.
class CSecondScreenDeviceDurango: public CSecondScreenDevice
{
public:
	CSecondScreenDeviceDurango( void* secondScreenDeviceDurango, ISecondScreenDeviceDelegate* _delegate );
	virtual ~CSecondScreenDeviceDurango();

	
	void OnMessageReceived( const Char* message );
	bool operator==( const CSecondScreenDevice& rhs );

	//! CSecondScreenDevice
private:
	Bool SendMessageAsync( const Char* message ) const;

private:
	void* m_secondScreenDeviceDurango;

};
#endif