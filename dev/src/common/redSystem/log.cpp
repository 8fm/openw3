/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#include "log.h"

#ifdef RED_PLATFORM_ORBIS
	#include <sce_atomic.h>
#endif

using namespace Red::System;

#if defined( RED_LOGGING_ENABLED ) && defined( RED_PLATFORM_ORBIS )
#  include <kernel.h>
static const size_t MIGHTY_STACK_SIZE = 1024 * 1024;
// Not sure what is and isn't under a lock in the logging system, so maybe the stack heavy logging with
// a shared static buffer...? But our logs are reentrant (lol), so that's more of a pain.
bool YouMayLogOMightyThread()
{
	ScePthreadAttr attr;
	if ( scePthreadAttrInit(&attr) != SCE_OK )
	{
		return false;
	}

	const ScePthread self = ::scePthreadSelf();
	if ( ::scePthreadAttrGet( self, &attr ) != SCE_OK )
	{
		::scePthreadAttrDestroy(&attr);
		return false;
	}

	void* stackaddr = nullptr;
	size_t stacksize = 0;
	if ( ::scePthreadAttrGetstack( &attr, &stackaddr, &stacksize ) != SCE_OK )
	{
		::scePthreadAttrDestroy( &attr );
		return false;
	}

	// Don't risk it. Clang uses VMOVAPS for xmm* in varargs.
	if ( reinterpret_cast<uintptr_t>( stackaddr ) & 15 )
	{
		::scePthreadAttrDestroy( &attr );
		return false;
	}

	if ( stacksize < MIGHTY_STACK_SIZE )
	{
		::scePthreadAttrDestroy( &attr );
		return false;
	}

	::scePthreadAttrDestroy( &attr );
	return true;
}
#endif // RED_LOGGING_ENABLED && RED_PLATFORM_ORBIS

namespace Red { namespace System { namespace Log {

	Manager* Manager::m_instance = nullptr;
	
	//////////////////////////////////////////////////////////////////////////
	// Output Device
	OutputDevice::OutputDevice()
	:	m_safeOnCrash( false )
	{
		Manager::GetInstance().Register( this );
	}

	OutputDevice::~OutputDevice()
	{
		Manager::GetInstance().Unregister( this );
	}

	void OutputDevice::SetSafeToCallOnCrash()
	{
		m_safeOnCrash = true;
	}

	void OutputDevice::SetUnsafeToCallOnCrash()
	{
		m_safeOnCrash = false;
	}
	
	Bool OutputDevice::IsSafeToCallOnCrash() const 
	{ 
		return m_safeOnCrash; 
	}

	void OutputDevice::SetPriorityVisibile( EPriority priority )
	{
		m_priority = priority;
	}

	EPriority OutputDevice::GetPriorityVisible() const
	{
		return m_priority;
	}


	//////////////////////////////////////////////////////////////////////////
	// Log Manager
	Manager::Manager()
	:	m_first ( nullptr )
	,	m_enabled( true )
	,	m_priority( P_Spam )
	,	m_crashModeStackCount( 0 )
	{
	}

	void Manager::SetCrashModeActive( Bool active )
	{
		if( active )
		{
#ifdef RED_PLATFORM_ORBIS
			sceAtomicIncrement32( &m_crashModeStackCount );
#else
			InterlockedIncrement( &m_crashModeStackCount );
#endif
		}
		else
		{
#ifdef RED_PLATFORM_ORBIS
			sceAtomicDecrement32( &m_crashModeStackCount );
#else
			InterlockedDecrement( &m_crashModeStackCount );
#endif
		}
	}

	void Manager::Register( OutputDevice* device )
	{
		// Insert device at the front of the device list
		device->SetNext( m_first );
		device->SetPriorityVisibile( m_priority );
		m_first = device;
	}

	void Manager::Unregister( OutputDevice* device )
	{
		if( device == m_first )
		{
			m_first = m_first->GetNext();
		}
		else
		{
			OutputDevice* currentDevice = m_first;

			while( currentDevice )
			{
				if( currentDevice->GetNext() == device )
				{
					currentDevice->SetNext( currentDevice->GetNext()->GetNext() );
					break;
				}
				else
				{
					currentDevice = currentDevice->GetNext();
				}
			}
		}
	}

	void Manager::Write( const Message& message )
	{
		OutputDevice* device = m_first;
		Bool crashModeActive = m_crashModeStackCount != 0;

		while( device != nullptr )
		{
			if( !crashModeActive || device->IsSafeToCallOnCrash() )
			{
				device->Write( message );
			}

			// Move on to next output device
			device = device->GetNext();
		}
	}

	void Manager::Flush()
	{
		// Flush each device in turn
		OutputDevice* device = m_first;

		while( device != nullptr )
		{
			//
			device->Flush();

			// Move on to next output device
			device = device->GetNext();
		}
	}

	void Manager::SetPriorityVisible( EPriority priority )
	{
		m_priority = priority;

		OutputDevice* device = m_first;

		while( device != nullptr )
		{
			device->SetPriorityVisibile( m_priority );

			// Move on to next output device
			device = device->GetNext();
		}
	}

	void Manager::SetInternalInstance( Manager* instance )
	{
		m_instance = instance;
	}

	Manager& Manager::GetInstance() 
	{
		if( m_instance == nullptr )
		{
			static Manager manager;
			Manager::SetInternalInstance( &manager );
		}

		return *m_instance;
	}

	void FormatAndHandleLogRequest( CNameHash channelHash, const Char* channelText, EPriority priority, const Char* format, ... )
	{
		Char buffer[ MAX_LINE_LENGTH ];

		va_list arglist;
		va_start( arglist, format );
		VSNPrintF( buffer, MAX_LINE_LENGTH, format, arglist );
		va_end( arglist );

		HandleLogRequest( channelHash, channelText, priority, buffer );
	}

	void HandleLogRequest( CNameHash channelHash, const Char* channelText, EPriority priority, const Char* message )
	{
		Manager& logManager = Manager::GetInstance();

		if( logManager.IsEnabled() )
		{
			static RED_TLS bool isHandlingLogRequest = false;

			if( !isHandlingLogRequest )
			{
				isHandlingLogRequest = true;
				ScopedFlag< Bool > scopedRecursionCheck( isHandlingLogRequest, false );

				Message data;
				Clock::GetInstance().GetLocalTime( data.dateTime );
				data.tick = Clock::GetInstance().GetTimer().GetTicks();
				data.channelHash = channelHash;
				data.channelText = channelText;
				data.text = message;
				data.priority = priority;

				logManager.Write( data );
			}
		}
	}

} } } // namespace Red { namespace System { namespace Log {
