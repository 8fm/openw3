/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#ifndef _RED_SYSTEM_ERROR_H_
#define _RED_SYSTEM_ERROR_H_

#include "settings.h"
#include "types.h"
#include "utility.h"
#include "threads.h"
#include "nameHash.h"
#include "crt.h"

#define RED_MAX_STACK_FRAMES													( 64 )
#define RED_SYMBOL_MAX_LENGTH													( 1024 )
#define RED_MAX_THREAD_CALLSTACKS												( 32 )
#define RED_APPEND_ERROR_STRING( output, outputSize, outputUsed, format, ... )	outputUsed += Red::System::SNPrintF( &output[ outputUsed ], outputSize - outputUsed, format, ##__VA_ARGS__ )

namespace Red
{
	namespace System
	{
		namespace Log
		{
			class File;
		}

		namespace Error
		{
			enum EDebugInfoFlags
			{
				DIF_CurrentThreadCallStack		= FLAG( 0 ),							//!< Callstack for the calling thread
				DIF_AllThreadCallStacks			= FLAG( 1 ),							//!< Callstack for all threads
				DIF_ExcludeThreadCallstack		= FLAG( 2 ) | DIF_AllThreadCallStacks,	//!< All callstacks except for the specified thread
				DIF_SpecifiedThreadCallstack	= FLAG( 3 ) | DIF_AllThreadCallStacks,	//!< Just the specified callstack
				DIF_CallBackInformation			= FLAG( 4 ),							//!< All information available through registered debug information callbacks
				DIF_AllCallBackInformation		= FLAG( 5 ),							//!< All information available through registered debug information callbacks
			};

			enum EAssertConfiguration
			{
				AC_PrintToLog,			//!< Write all assertion information to the log
				AC_PopupHook,			//!< Present a popup to the user upon assertion failure
				AC_SilentCrashHook,		//!< Automate the crash handler in the event of reporting an assert or crash
				AC_Verbose,				//!< All thread callstacks get added to the log information
				AC_AppendDebugInfo,		//!< Append data from debug information callbacks on assert
				
				// Mutually exclusive flags
				AC_Break,				//!< Break on assert by default
				AC_Continue,			//!< Continue (and report) by default
				AC_ContinueAlways,		//!< Continue by default
			};

			enum EAssertAction
			{
				AA_Break,
				AA_Continue,
				AA_ContinueAlways,
				AA_Stop
			};

			struct Callstack
			{
				struct CallstackFrames
				{
					Char file[ RED_SYMBOL_MAX_LENGTH ];
					Char symbol[ RED_SYMBOL_MAX_LENGTH ];
					Uint32 line;
				};

				CallstackFrames frame[ RED_MAX_STACK_FRAMES ];
				Uint32 numFrames;
			};

			class CrashReportDataBuffer;
			class MessageStack;

			class StackMessage
			{
			public:
				StackMessage();
				virtual ~StackMessage();

				// compile a text report
				virtual void Report( Char* outText, Uint32 outTextLength ) = 0;
			};

			class MessageStack
			{
			private:
				struct ThreadStack
				{
					static const Uint32 MAX_STACK_MESSAGES = 128;

					StackMessage*		m_messages[ MAX_STACK_MESSAGES ];
					Uint32				m_numStackMessages;
				};

				// messages per thread
				static RED_TLS ThreadStack* m_stack;

			public:
				// push message onto the current thread message stack, message will create a report if we crash while the message is on the stack
				static void Push( StackMessage& message );

				// pop message from the current thread message stack
				static void Pop( StackMessage& message ); 

				// using messages on the message stack compile a text report
				static Uint32 CompileReport( Char* outText, Uint32 outTextLength );

				// returns true if there are stack messages for current thread
				static Bool HasStackMessages();
			};

			class Handler
			{
			public:
				typedef Uint32 (*DebugInfoFunc)( Char*, Uint32 );
				typedef EAssertAction (*AssertHook)( const Char* cppFile, Uint32 line, const Char* expression, const Char* message, const Char* details );

			public:
				Handler();
				virtual ~Handler();

				static Handler* GetInstance(); // CREATION IS NOT THREAD SAFE! <- nice how it gets called on different threads then.

				void SetAssertFlag( EAssertConfiguration flag, Bool enabled = true );
				RED_INLINE Bool HasAssertFlag( EAssertConfiguration flag ) const { return ( m_assertFlags & FLAG( flag ) ) ? true : false; }

				RED_MOCKABLE EAssertAction Assert( const Char* message, const Char* cppFile, Uint32 line, const Char* expression, Uint32 skipFrames );

				virtual Bool IsDebuggerPresent() const = 0;

				// Write a mini dump file
				// The threadId parameter specifies the default "problem" thread
				// All threads are included in the dump itself
				virtual Bool WriteDump( const Char* filename ) = 0;
				
				// Callstack of current thread
				// Write formatted callstack information to output (truncated if it reaches outputSize)
				// Returns number of chars written (as with sprintf)
				virtual Uint32 GetCallstack( Char* output, Uint32 outputSize, Uint32 skipFrames = 0 ) = 0;

				// Callstack of Specified thread
				// Write formatted callstack information to output (truncated if it reaches outputSize)
				// Returns number of chars written (as with sprintf)
				virtual Uint32 GetCallstack( Char* output, Uint32 outputSize, const Internal::ThreadId& threadId, Uint32 skipFrames = 0 ) = 0;

				// Callstack of current thread
				// Write raw callstack data into parameter "stack"
				virtual void GetCallstack( Callstack& stack, Uint32 skipFrames = 0 ) = 0;
				virtual void GetCallstack( Callstack& stack, const Internal::ThreadId& threadId, Uint32 skipFrames = 0 ) = 0;

				// Callstack for all threads
				// threadIds should be a pointer to an array, and maxThreads the size of that array
				// Returns number of threads found
				virtual Uint32 EnumerateThreads( Internal::ThreadId* threadIds, Uint32 maxThreads ) = 0;

				// All information
				RED_MOCKABLE Uint32 GetDebugInformation( Char* output, Uint32 outputSize, Uint32 flags = DIF_AllThreadCallStacks | DIF_AllCallBackInformation, Uint32 skipCurrentThreadFrames = 0, CNameHash callbackTag = CNameHash(), const Internal::ThreadId& specifiedThread = Internal::ThreadId() );

				// Register a callback that provides information when GetDebugInformation() is called
				// These callbacks take as parameters a Char buffer and the size of that buffer
				// They return the number of chars written into the buffer
				void RegisterDebugInformationCallback( CNameHash tag, const AnsiChar* title, DebugInfoFunc infoFunc );
				void UnregisterDebugInformationCallback( CNameHash tag );

				void RegisterAssertHook( AssertHook func );

				virtual void SetLogFile( const Char* logFilename, Log::File* instance );

				void SetVersion( const Char* verison ) { StringCopy( m_version, verison, ARRAY_COUNT( m_version ) ); }

				static void SetInternalInstance( Handler* handler ); // ONLY FOR UNIT TEST PURPOSE !

			protected:
				const Char* GetVersion() const;

			private:
				virtual void HandleUserChoice( EAssertAction chosenAction, const Char* cppFile, Uint32 line, const Char* expression, const Char* details ) = 0;
				virtual const Char* GetCommandline() const = 0;

				Uint32 GetDebugItemInformation( Char* output, Uint32 outputSize, Uint32 callbackIndex );

				static const Uint32 MAX_CALLBACKS = 8;
				static const Uint32 MAX_CRASH_DATA_REPORTERS = 8;

				struct CallBack
				{
					static const Uint32 c_maxTitleChars = 32;
					DebugInfoFunc m_function;
					CNameHash m_tag;
					AnsiChar m_title[ c_maxTitleChars ];
				};

				Uint32 m_assertFlags;
				Uint32 m_numCallbacks;
				CallBack m_callbacks[ MAX_CALLBACKS ];

				AssertHook m_assertHook;

				static Handler* m_handlerInstance;

				Char m_version[ 128 ];
			};
		
			EAssertAction HandleAssertion( const Char* filename, Uint32 line, const Char* expression, const Char* message, ... );
			EAssertAction HandleAssertion( const Char* filename, Uint32 line, const Char* expression );
			
			void HandleWarning( const Char* filename, Uint32 line, const Char* expression, const Char* message, ... );
			void HandleWarning( const Char* filename, Uint32 line, const Char* expression );
		}
	}
}

// RED_STATIC_ASSERT emulated C++17 static assert without a message. Remove this macro when C++17 is supported. 
#define RED_STATIC_ASSERT( expression ) static_assert( expression, #expression )

#ifdef RED_ASSERTS_ENABLED

#define RED_FATAL_ASSERT( expression, message, ... ) \
	do { if( !( expression ) ) { Red::System::Error::HandleAssertion(  MACRO_TXT(__FILE__), __LINE__, MACRO_TXT( #expression ), MACRO_TXT( message ), ##__VA_ARGS__ ); __debugbreak(); } } while ( (void)0,0 )

#define RED_FATAL( message, ... ) \
	do { Red::System::Error::HandleAssertion(  MACRO_TXT(__FILE__), __LINE__, MACRO_TXT( "Fatal" ), MACRO_TXT( message ), ##__VA_ARGS__ ); __debugbreak(); } while ( (void)0,0 )

#define RED_WARNING( expression, message, ... ) \
	do { if( !( expression ) ) { Red::System::Error::HandleWarning( MACRO_TXT(__FILE__), __LINE__, MACRO_TXT( #expression ), MACRO_TXT( message ), ##__VA_ARGS__ ); } } while ( (void)0,0 )

#define RED_WARNING_ONCE( expression, message, ... ) \
	do { if( !( expression ) ) { static Red::System::Bool warningTriggered = false; if(!warningTriggered){ warningTriggered = true; Red::System::Error::HandleWarning( MACRO_TXT(__FILE__), __LINE__, MACRO_TXT( #expression ), MACRO_TXT( message ), ##__VA_ARGS__ ); } } } while ( (void)0,0 )

#define RED_VERIFY( expression, ... ) \
	do { if( !( expression ) ) { static Red::System::Bool verifyTriggered = false; if(!verifyTriggered){ verifyTriggered = true; Red::System::Error::HandleWarning( MACRO_TXT(__FILE__), __LINE__, MACRO_TXT( #expression ), ##__VA_ARGS__ ); } } } while ( (void)0,0 )

#define RED_ASSERT( expression, ... )																																			\
{																																												\
	static Red::System::Error::EAssertAction lastAction = Red::System::Error::AA_Break;																							\
	if ( lastAction != Red::System::Error::AA_ContinueAlways  && !Red::System::Error::Handler::GetInstance()->HasAssertFlag( Red::System::Error::AC_ContinueAlways ) )			\
	{																																											\
		if ( !( expression ) )																																					\
		{																																										\
			lastAction = Red::System::Error::HandleAssertion( MACRO_TXT( __FILE__ ), __LINE__, MACRO_TXT( #expression ), ##__VA_ARGS__ );										\
			if ( lastAction == Red::System::Error::AA_Break )																													\
			{																																									\
				RED_BREAKPOINT();																																				\
			}																																									\
		}																																										\
	}																																											\
}

#define RED_HALT( message, ... )																																				\
{																																												\
	static Red::System::Error::EAssertAction lastAction = Red::System::Error::AA_Break;																							\
	if ( lastAction != Red::System::Error::AA_ContinueAlways && !Red::System::Error::Handler::GetInstance()->HasAssertFlag( Red::System::Error::AC_ContinueAlways ) )			\
	{																																											\
		{																																										\
			lastAction = Red::System::Error::HandleAssertion( MACRO_TXT( __FILE__ ), __LINE__, MACRO_TXT( "Halt" ), MACRO_TXT( message ), ##__VA_ARGS__ );						\
			if ( lastAction == Red::System::Error::AA_Break )																													\
			{																																									\
				RED_BREAKPOINT();																																				\
			}																																									\
		}																																										\
	}																																											\
}

#define RED_DECLARE_DEBUG_CALLBACK_TAG( name )				namespace DebugCallbackTags { Red::CNameHash 	GErrorReporter_Callback_##name( #name ); }
#define RED_DEBUG_CALLBACK_TAG( name )						DebugCallbackTags::GErrorReporter_Callback_##name
#define RED_REGISTER_DEBUG_CALLBACK( tag, callback )		Red::System::Error::Handler::GetInstance()->RegisterDebugInformationCallback( RED_DEBUG_CALLBACK_TAG( tag ), #tag, callback )
#define RED_UNREGISTER_DEBUG_CALLBACK( tag )				Red::System::Error::Handler::GetInstance()->UnregisterDebugInformationCallback( RED_DEBUG_CALLBACK_TAG( tag ) )

#else // RED_ASSERTS_ENABLED

#define RED_ASSERT( ... )
#define RED_HALT( ... )

#define RED_FATAL_ASSERT( expression, message, ... ) do { } while ( (void)0,0 )
#define RED_FATAL( message, ... ) do { } while ( (void)0,0 )
#define RED_WARNING( expression, message, ... ) do { } while ( (void)0,0 )
#define RED_WARNING_ONCE( expression, message, ... ) do { } while ( (void)0,0 )
#define RED_VERIFY( expression, ... ) do { if( !( expression ) ) {} } while ( (void)0,0 )

// Debug callbacks should always be registered so they can be called from exception handler

#define RED_DECLARE_DEBUG_CALLBACK_TAG( name )				namespace DebugCallbackTags { Red::CNameHash 	GErrorReporter_Callback_##name( #name ); }
#define RED_DEBUG_CALLBACK_TAG( name )						DebugCallbackTags::GErrorReporter_Callback_##name
#define RED_REGISTER_DEBUG_CALLBACK( tag, callback )		Red::System::Error::Handler::GetInstance()->RegisterDebugInformationCallback( RED_DEBUG_CALLBACK_TAG( tag ), #tag, callback )
#define RED_UNREGISTER_DEBUG_CALLBACK( tag )				Red::System::Error::Handler::GetInstance()->UnregisterDebugInformationCallback( RED_DEBUG_CALLBACK_TAG( tag ) )

#endif // RED_ASSERTS_ENABLED

#endif //_RED_SYSTEM_ERROR_H_
