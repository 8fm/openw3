
#include "build.h"

#include "../../common/engine/inputBufferedInputEvent.h"
#include "../../common/engine/inputUtils.inl"
#include "userProfileManagerLinux.h"

#include "inputDeviceManagerLinux.h"

//////////////////////////////////////////////////////////////////////////
// CInputDeviceManagerLinux
//////////////////////////////////////////////////////////////////////////
CInputDeviceManagerLinux::CInputDeviceManagerLinux()
{
}

Bool CInputDeviceManagerLinux::Init()
{
	return true;
}

CInputDeviceManagerLinux::~CInputDeviceManagerLinux()
{
}

void CInputDeviceManagerLinux::Update( TDynArray< SBufferedInputEvent >& outBufferedInput )
{
}

void CInputDeviceManagerLinux::RequestReset()
{
}

void CInputDeviceManagerLinux::Shutdown()
{
}

const CName CInputDeviceManagerLinux::GetLastUsedDeviceName() const
{
	return m_lastUsedDeviceName;
}
