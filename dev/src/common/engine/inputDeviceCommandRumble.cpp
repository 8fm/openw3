#include "build.h"
#include "inputDeviceCommandRumble.h"
#include "inputDeviceGamepad.h"

CInputDeviceCommandRumble::CInputDeviceCommandRumble()
	: m_leftVal( 0.f )
	, m_rightVal( 0.f )
{
}

void CInputDeviceCommandRumble::PerformCommand( IInputDeviceGamepad* device ) const
{
	RED_ASSERT( device );
	if( device)
	{
		device->SetPadVibrate( m_leftVal, m_rightVal );
	}
}


void CInputDeviceCommandRumble::AfterCommandPerformed()
{
	m_leftVal = m_rightVal = 0.f;
}


void CInputDeviceCommandRumble::SetPadVibrate( Float leftVal, Float rightVal )
{
	m_leftVal = leftVal;
	m_rightVal = rightVal;
}
