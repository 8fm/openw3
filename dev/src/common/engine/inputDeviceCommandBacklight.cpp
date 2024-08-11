#include "build.h"
#include "inputDeviceCommandBacklight.h"
#include "inputDeviceGamepad.h"



CInputDeviceCommandBacklight::CInputDeviceCommandBacklight( const Color& color )
	: m_color( color )
	, m_reset( false )
{
}

void CInputDeviceCommandBacklight::PerformCommand( IInputDeviceGamepad* device ) const
{
	RED_ASSERT( device );
	
	if( !device)
	{
		return;
	}

	if( m_reset )
	{
		device->ResetBacklightColor();
	}
	else
	{
		device->SetBacklightColor( m_color );
	}
}

