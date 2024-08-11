/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#ifndef _RED_LOG_H_
#define _RED_LOG_H_

#include "os.h"
#include "settings.h"
#include "types.h"
#include "nameHash.h"
#include "clock.h"
#include "utility.h"
#include "threads.h"
#include "crt.h"

#if defined( RED_LOGGING_ENABLED ) && defined( RED_PLATFORM_ORBIS )
	extern bool YouMayLogOMightyThread();
#	define THREAD_CHECK() if ( !YouMayLogOMightyThread() ) break;
#else
#	define THREAD_CHECK()
#endif

namespace Red { namespace System { namespace Log {

	//////////////////////////////////////////////////////////////////////////
	// Log message verbosity
	enum EPriority
	{
		P_Error = 0,
		P_Warning,
		P_Information,
		P_Spam,

		P_Count
	};

	enum EFocusState
	{
		FS_Isolate = 0,
		FS_Ignore,

		FS_Max
	};

	const Uint32 MAX_FOCUSED_CHANNELS = 5;
	const Uint32 MAX_LINE_LENGTH = ( 1024 * 80 );

	//////////////////////////////////////////////////////////////////////////
	// Structure containing data about the latest log output
	struct Message
	{
		EPriority		priority;
		Uint64			tick;
		DateTime		dateTime;
		CNameHash		channelHash;
		const Char*		channelText;
		const Char*		text;
	};

	//////////////////////////////////////////////////////////////////////////
	// Base Interface class
	class OutputDevice
	{
	public:
		OutputDevice();
		virtual ~OutputDevice();

		virtual void Write( const Message& message ) = 0;

		virtual void Flush() {}

		void SetNext( OutputDevice* next ) { m_next = next; }
		OutputDevice* GetNext() { return m_next; }

		RED_MOCKABLE Bool IsSafeToCallOnCrash() const;

		void SetPriorityVisibile( EPriority priority );
		
	protected:
		// Indicates that this device is safe to log to in the event of a crash or Out of Memory
		// Devices that set this to true must not allocate any memory on the heap or interact with the engine/game code
		void SetSafeToCallOnCrash();
		void SetUnsafeToCallOnCrash();
		EPriority GetPriorityVisible() const;

	private:
		OutputDevice* m_next;
		EPriority m_priority;
		bool m_safeOnCrash;
	};

	//////////////////////////////////////////////////////////////////////////
	// Main Log system
	class Manager
	{
	public:
		Manager();

		RED_MOCKABLE void Write( const Message& message );

		void Flush();

		void Register( OutputDevice* device );
		void Unregister( OutputDevice* device );

		RED_MOCKABLE Bool IsEnabled() const { return m_enabled; }
		void SetEnabled( Bool enabled ) { m_enabled = enabled; }
		void SetPriorityVisible( EPriority priority );

		void SetCrashModeActive( Bool active );

		static Manager& GetInstance();

		static void SetInternalInstance( Manager* instance );

	private:

		OutputDevice* m_first;
		Bool m_enabled;
		EPriority m_priority;

#ifdef RED_PLATFORM_ORBIS
		RED_ALIGNED_VAR( Int32, 4 ) m_crashModeStackCount;
#else
		RED_ALIGNED_VAR( Uint32, 4 ) m_crashModeStackCount;
#endif

		static Manager * m_instance;
	};

	void FormatAndHandleLogRequest( CNameHash channelHash, const Char* channelText, EPriority priority, const Char* format, ... );
	void HandleLogRequest( CNameHash channelHash, const Char* channelText, EPriority priority, const Char* message );

} } } // namespace Red { namespace System { namespace Log {

#ifdef RED_LOGGING_ENABLED

#	define INTERNAL_RED_LOG_ERROR( channel, message, ... )		do{ THREAD_CHECK(); ::Red::System::Log::FormatAndHandleLogRequest( ::Red::CNameHash( #channel ), MACRO_TXT( #channel ), Red::System::Log::P_Error, message, ##__VA_ARGS__ ); } while( (void)0, 0 )
#	define INTERNAL_RED_LOG_WARNING( channel, message, ... )	do{ THREAD_CHECK(); ::Red::System::Log::FormatAndHandleLogRequest( ::Red::CNameHash( #channel ), MACRO_TXT( #channel ), Red::System::Log::P_Warning, message, ##__VA_ARGS__ ); } while( (void)0, 0 )
#	define INTERNAL_RED_LOG( channel, message, ... )			do{ THREAD_CHECK(); ::Red::System::Log::FormatAndHandleLogRequest( ::Red::CNameHash( #channel ), MACRO_TXT( #channel ), Red::System::Log::P_Information, message, ##__VA_ARGS__ ); } while( (void)0, 0 )
#	define INTERNAL_RED_LOG_SPAM( channel, message, ... )		do{ THREAD_CHECK(); ::Red::System::Log::FormatAndHandleLogRequest( ::Red::CNameHash( #channel ), MACRO_TXT( #channel ), Red::System::Log::P_Spam, message, ##__VA_ARGS__ ); } while( (void)0, 0 )

// To evaluate macros first so the channel names aren't full of CName or RED_LOG_CHANNEL
#	define RED_LOG_ERROR( channel, message, ... )				INTERNAL_RED_LOG_ERROR( channel, message, ##__VA_ARGS__ )
#	define RED_LOG_WARNING( channel, message, ... )				INTERNAL_RED_LOG_WARNING( channel, message, ##__VA_ARGS__ )
#	define RED_LOG( channel, message, ... )						INTERNAL_RED_LOG( channel, message, ##__VA_ARGS__ )
#	define RED_LOG_SPAM( channel, message, ... )				INTERNAL_RED_LOG_SPAM( channel, message, ##__VA_ARGS__ )

#	define RED_LOG_FLUSH()										Red::System::Log::Manager::GetInstance().Flush()

// Use this to create a name with the express purpose of being a channel log (so are not created when logging is disabled)
#	define RED_LOG_CHANNEL( name )								name

#else

#	define RED_LOG_ERROR(...)
#	define RED_LOG_WARNING(...)
#	define RED_LOG_WARNING_ONCE( ... )
#	define RED_LOG(...)
#	define RED_LOG_SPAM(...)
#	define RED_LOG_FLUSH()
#	define RED_LOG_CHANNEL(...)
#endif

#endif // _RED_LOG_H_
