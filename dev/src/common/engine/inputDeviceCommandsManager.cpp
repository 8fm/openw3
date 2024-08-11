#include "build.h"
#include "inputDevice.h"
#include "inputDeviceCommandsManager.h"
#include "inputDeviceCommandBacklight.h"
#include "inputDeviceCommandRumble.h"

CInputDeviceCommandsManager::CInputDeviceCommandsManager()
{
	m_backlightCommandPool = new CInputDeviceCommandBacklight( Color::BLUE );
	m_rumbleCommandPool = new CInputDeviceCommandRumble;
}

CInputDeviceCommandsManager::~CInputDeviceCommandsManager()
{
	delete m_backlightCommandPool;
	delete m_rumbleCommandPool;
}


void CInputDeviceCommandsManager::Update()
{
	if( m_currentDevice.Empty() )
	{
		return;
	}

	for( Uint32 i = 0; i < m_bufferedCommand.Size(); ++i )
	{
		RED_ASSERT( m_bufferedCommand[ i ] );
		RED_ASSERT( m_bufferedCommand[ i ]->IsBuffered(), TXT( "Command was buffered but not notified about it?" ) );

		for( Uint32 j = 0; j < m_currentDevice.Size(); ++j )
		{
			RED_ASSERT( m_currentDevice[ j ] );
			
			m_currentDevice[ j ]->PerformCommand( m_bufferedCommand[ i ] );
		}

		m_bufferedCommand[ i ]->AfterCommandPerformed();
		m_bufferedCommand[ i ]->SetBuffered( false );
	}

	m_bufferedCommand.ClearFast();
}

void CInputDeviceCommandsManager::SetBacklight( const Color& color )
{
	RED_ASSERT( m_backlightCommandPool, TXT( "This command should have been created in a constructor!" ) );

	m_backlightCommandPool->SetColor( color );

	if( !(m_backlightCommandPool->IsBuffered()) )
	{
		BufferCommand( m_backlightCommandPool );
	}
}


void CInputDeviceCommandsManager::ResetBacklight()
{
	RED_ASSERT( m_backlightCommandPool, TXT( "This command should have been created in a constructor!" ) );

	m_backlightCommandPool->ResetColor();

	if( !(m_backlightCommandPool->IsBuffered()) )
	{
		BufferCommand( m_backlightCommandPool );
	}
}

void CInputDeviceCommandsManager::SetPadVibrate( Float leftVal, Float rightVal )
{
	RED_ASSERT( m_rumbleCommandPool, TXT( "This command should have been created in a constructor!" ) );

	const Float clampedLeftVal = Clamp<Float>( leftVal, 0.f, 1.f );
	const Float clampedRightVal = Clamp<Float>( rightVal, 0.f, 1.f );

	m_rumbleCommandPool->SetPadVibrate( clampedLeftVal, clampedRightVal );

	if( !(m_rumbleCommandPool->IsBuffered()) )
	{
		BufferCommand( m_rumbleCommandPool );
	}
}

void CInputDeviceCommandsManager::BufferCommand( IInputDeviceCommand* command )
{
	RED_ASSERT( !command->IsBuffered(), TXT( "Input device command should never be buffered twice!!!" ) );
	command->SetBuffered( true );
	m_bufferedCommand.PushBack( command );
}