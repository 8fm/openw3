/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#include "error.h"
#include "crt.h"
#include "log.h"
#include "crashReportDataBuffer.h"
#include "nameHash.h"

#define RED_ERROR_HEADER	TXT( "Debug Information\n================================================================\n\n" )
#define RED_ERROR_DIVIDER	TXT( "\n================================================================\n\n" )

namespace Red { namespace System { namespace Error {

const Uint32 MAX_MESSAGE_LENGTH = 1024;
const Uint32 MAX_DETAILS_LENGTH = Log::MAX_LINE_LENGTH;
Handler* Handler::m_handlerInstance = nullptr;
const Char* defaultAssertMessage = TXT( "A message needs to be defined with this assert" );
const Char* defaultWarningMessage = TXT( "A message needs to be defined with this warning" );

//////////////////////////////////////////////////////////////////////////
// 

StackMessage::StackMessage()
{
	Error::MessageStack::Push( *this );
}

StackMessage::~StackMessage()
{
	Error::MessageStack::Pop( *this );
}

RED_TLS Error::MessageStack::ThreadStack* Error::MessageStack::m_stack = NULL;

void MessageStack::Push( StackMessage& message )
{
	if ( m_stack == nullptr )
	{
		m_stack = new ThreadStack();
		m_stack->m_numStackMessages = 0;
	}

	if ( m_stack->m_numStackMessages < ThreadStack::MAX_STACK_MESSAGES )
	{
		m_stack->m_messages[m_stack->m_numStackMessages] = &message;
	}

	m_stack->m_numStackMessages += 1;
}

void MessageStack::Pop( StackMessage& message )
{
	RED_UNUSED( message ); // used only in assertion

	if ( m_stack != nullptr )
	{
		m_stack->m_numStackMessages -= 1;

		if ( m_stack->m_numStackMessages < ThreadStack::MAX_STACK_MESSAGES )
		{
			RED_ASSERT( m_stack->m_messages[m_stack->m_numStackMessages] == &message );
			m_stack->m_messages[m_stack->m_numStackMessages] = nullptr;
		}
	}
}

Bool MessageStack::HasStackMessages()
{
	return ( m_stack != nullptr ) && ( m_stack->m_numStackMessages > 0 );
}

Uint32 MessageStack::CompileReport( Char* outText, Uint32 outTextLength )
{
	Uint32 outputUsed = 0;

	if ( m_stack != nullptr )
	{
		for ( Uint32 i=0; i<m_stack->m_numStackMessages; ++i )
		{
			Char localBuffer[ 256 ];
			localBuffer[0] = '\0';
			m_stack->m_messages[i]->Report( localBuffer, ARRAY_COUNT( localBuffer ) );

			outputUsed += SNPrintF( 
				outText + outputUsed, 
				outTextLength - outputUsed, 
				TXT("[%d]: %ls\n"), i, localBuffer );
		}
	}

	return outputUsed;
}

//////////////////////////////////////////////////////////////////////////
// 

Handler::Handler()
:	m_assertFlags( FLAG( AC_PrintToLog ) | FLAG( AC_Break ) | FLAG( AC_PopupHook ) | FLAG( AC_AppendDebugInfo ) )
,	m_numCallbacks( 0 )
,	m_assertHook( nullptr )
{
	StringCopy( m_version, TXT( "Not Set" ), ARRAY_COUNT( m_version ) );
}

Handler::~Handler()
{

}

void Handler::SetAssertFlag( EAssertConfiguration flag, Bool enabled )
{
#define MUTUAL_EXCLUSION_MASK ( FLAG( AC_Break ) | FLAG( AC_Continue ) | FLAG( AC_ContinueAlways ) )
#define MUTUAL_EXCLUSION_START AC_Break

	if( enabled )
	{
		if( flag >= MUTUAL_EXCLUSION_START )
		{
			m_assertFlags &= ~MUTUAL_EXCLUSION_MASK;
		}

		m_assertFlags |= FLAG( flag );
	}
	else
	{
		m_assertFlags &= ~FLAG( flag );
	}
}

EAssertAction Error::Handler::Assert( const Char* message, const Char* cppFile, Uint32 line, const Char* expression, Uint32 skipFrames )
{
	Char details[ MAX_DETAILS_LENGTH ];
	Uint32 detailsUsed = 0;
	
	RED_APPEND_ERROR_STRING( details, MAX_DETAILS_LENGTH, detailsUsed, TXT( "Assertion Failed: %" ) MACRO_TXT( RED_PRIs ) TXT( "\n" ), expression );
	RED_APPEND_ERROR_STRING( details, MAX_DETAILS_LENGTH, detailsUsed, TXT( "%" ) MACRO_TXT( RED_PRIs ) TXT( "\n" ), message );
	RED_APPEND_ERROR_STRING( details, MAX_DETAILS_LENGTH, detailsUsed, TXT( "%" ) MACRO_TXT( RED_PRIs ) TXT( "(%u)\n\n" ), cppFile, line );
	
	if( HasAssertFlag( AC_AppendDebugInfo ) )
	{
		if( HasAssertFlag( AC_Verbose ) )
		{
			detailsUsed += GetDebugInformation( &details[ detailsUsed ], MAX_DETAILS_LENGTH - detailsUsed, DIF_AllThreadCallStacks | DIF_AllCallBackInformation, ++skipFrames );
		}
		else
		{
			detailsUsed += GetDebugInformation( &details[ detailsUsed ], MAX_DETAILS_LENGTH - detailsUsed, DIF_CurrentThreadCallStack | DIF_AllCallBackInformation, ++skipFrames );
		}
	}

	if( HasAssertFlag( AC_PrintToLog ) )
	{
		Red::System::Log::Manager::GetInstance().SetCrashModeActive( true );
		HandleLogRequest( ::Red::CNameHash( "Assertion" ), TXT( "Assertion" ), Red::System::Log::P_Error, details );
		Red::System::Log::Manager::GetInstance().SetCrashModeActive( false );
	}

	EAssertAction action = AA_ContinueAlways;

	if( HasAssertFlag( AC_Break ) )
	{
		action = AA_Break;
	}
	else if( HasAssertFlag( AC_Continue ) )
	{
		action = AA_Continue;
	}

	// The hook will contain the code that presents a popup to the user 
	if( m_assertHook && HasAssertFlag( AC_PopupHook ) )
	{
		action = (*m_assertHook)( cppFile, line, expression, message, details );
	}

	// The Assert function will allow for the system to take action based on the user's choice
	HandleUserChoice( action, cppFile, line, expression, details );

	if( action == AA_Stop )
	{
		// Don't perform a proper shutdown, just quit
		Internal::EmergencyExit();
	}

	return action;
}

Uint32 Handler::GetDebugInformation( Char* output, Uint32 outputSize, Uint32 flags, Uint32 skipCurrentThreadFrames, CNameHash callbackTag, const Internal::ThreadId& specifiedThread )
{
	Uint32 outputUsed = 0;
	Log::Manager::GetInstance().SetCrashModeActive( true );		// only use 'safe' logging systems during asserts
			
	outputUsed = SNPrintF( output, outputSize, RED_ERROR_HEADER );
	outputUsed += SNPrintF( &output[ outputUsed ], outputSize - outputUsed,
		TXT( "Commandline: %" ) MACRO_TXT( RED_PRIs ) TXT( " \n\n" ), GetCommandline()); 

	if( flags & DIF_AllThreadCallStacks )
	{
		if( outputUsed < outputSize )
		{
			Internal::ThreadId threads[ RED_MAX_THREAD_CALLSTACKS ];
			Uint32 numThreads = 0;

			// DIF_SpecifiedThreadCallstack is a composite flag, so we need to make sure all the bits are set
			if( ( flags & DIF_SpecifiedThreadCallstack ) == DIF_SpecifiedThreadCallstack )
			{
				numThreads = 1;
				threads[ 0 ] = specifiedThread;
			}
			else
			{
				numThreads = EnumerateThreads( threads, ARRAY_COUNT( threads ) );
			}


			for( Uint32 i = 0; i < numThreads; ++i )
			{
				// DIF_ExcludeThreadCallstack is a composite flag, so we need to make sure all the bits are set
				if( !( ( flags & DIF_ExcludeThreadCallstack ) == DIF_ExcludeThreadCallstack ) || threads[ i ] != specifiedThread )
				{
					if( outputUsed < outputSize )
					{
						outputUsed += GetCallstack( &output[ outputUsed ], outputSize - outputUsed, threads[ i ], 1 );

						// Add a dividing line to space the information out
						RED_APPEND_ERROR_STRING( output, outputSize, outputUsed, RED_ERROR_DIVIDER );
					}
					else
					{
						break;
					}
				}
			}
		}
	}
	// This flag is ignored if DIF_AllThreadCallStacks is specified
	else if( flags & DIF_CurrentThreadCallStack )
	{
		outputUsed += SNPrintF( &output[ outputUsed ], outputSize - outputUsed, TXT( "Current Thread:\n\n" ) );

		if( outputUsed < outputSize )
		{
			outputUsed += GetCallstack( &output[ outputUsed ], outputSize - outputUsed, ++skipCurrentThreadFrames );

			// Add a dividing line to space the information out
			RED_APPEND_ERROR_STRING( output, outputSize, outputUsed, RED_ERROR_DIVIDER );
		}
	}

	if( flags & DIF_AllCallBackInformation )
	{
		for( Uint32 i = 0; i < m_numCallbacks; ++i )
		{
			outputUsed += GetDebugItemInformation( &output[ outputUsed ], outputSize - outputUsed, i );

			if( outputUsed >= outputSize )
			{
				break;
			}
		}
	}

	// This flag is ignored if DIF_AllCallBackInformation is specified
	else if( flags & DIF_CallBackInformation )
	{
		for( Uint32 i = 0; i < m_numCallbacks; ++i )
		{
			if( m_callbacks[ i ].m_tag == callbackTag )
			{
				outputUsed += GetDebugItemInformation( &output[ outputUsed ], outputSize - outputUsed, i );
				break;
			}
		}
	}

	Log::Manager::GetInstance().SetCrashModeActive( false );		// only use 'safe' logging systems during asserts

	return outputUsed;
}

Uint32 Handler::GetDebugItemInformation( Char* output, Uint32 outputSize, Uint32 callbackIndex )
{
	Uint32 outputUsed = 0;

	outputUsed += SNPrintF( &output[ outputUsed ], outputSize - outputUsed, TXT( "%S" ), m_callbacks[ callbackIndex ].m_title );
	 
	// Space it out from the contents of the function
	outputUsed += SNPrintF( &output[ outputUsed ], outputSize - outputUsed, TXT( "\n\n" ) );
	 
	if( outputUsed < outputSize )
	{
	 	// Call the debug function and add it's contents to the buffer
	 	outputUsed += ( *( m_callbacks[ callbackIndex ] ).m_function )( &output[ outputUsed ], outputSize - outputUsed );
	 
	 	// Add a dividing line to space the information out
	 	RED_APPEND_ERROR_STRING( output, outputSize, outputUsed, RED_ERROR_DIVIDER );
	}

	return outputUsed;
}

void Handler::RegisterDebugInformationCallback( CNameHash tag, const AnsiChar* title, DebugInfoFunc infoFunc )
{
	//TODO: Assert infoFunc != nullptr
	RED_ASSERT( infoFunc, TXT( "Trying to register null debug info callback" ) );
	RED_ASSERT( m_numCallbacks < MAX_CALLBACKS, TXT( "Adding too many debug info callbacks!" ) );
	if( m_numCallbacks < MAX_CALLBACKS )
	{
		m_callbacks[ m_numCallbacks ].m_tag			= tag;
		m_callbacks[ m_numCallbacks ].m_function	= infoFunc;
		StringCopy( m_callbacks[ m_numCallbacks ].m_title, title, CallBack::c_maxTitleChars );
		++m_numCallbacks;
	}
}

void Handler::UnregisterDebugInformationCallback( CNameHash tag )
{
	for( Uint32 i = 0; i < m_numCallbacks; ++i )
	{
		if( m_callbacks[ i ].m_tag == tag )
		{
			//Replace this callback with the tag at the end of the array
			--m_numCallbacks;

			if( i != m_numCallbacks )
			{
				m_callbacks[ i ].m_tag = m_callbacks[ m_numCallbacks ].m_tag;
				m_callbacks[ i ].m_function = m_callbacks[ m_numCallbacks ].m_function;
				StringCopy( m_callbacks[ i ].m_title, m_callbacks[ m_numCallbacks ].m_title, CallBack::c_maxTitleChars );
			}

			break;
		}
	}
}

void Handler::RegisterAssertHook( AssertHook func )
{
	m_assertHook = func;
}

void Handler::SetInternalInstance( Handler* handler )
{
	m_handlerInstance = handler;
}

const Char* Handler::GetVersion() const
{
	return m_version;
}

void Handler::SetLogFile( const Char*, Log::File* )
{

}

EAssertAction HandleAssertion( const Char* filename, Uint32 line, const Char* expression, const Char* message, ... )
{
	static RED_TLS bool isHandlingAssertion	= false;
	
	if( !isHandlingAssertion )
	{
		isHandlingAssertion = true;
		ScopedFlag< Bool > scopedRecursionCheck( isHandlingAssertion, false );

		Char formattedAssertionMessage[ MAX_MESSAGE_LENGTH ];

		if( !message )
		{
			message = defaultAssertMessage;
		}

		va_list arglist;
		va_start( arglist, message );
		VSNPrintF( formattedAssertionMessage, ARRAY_COUNT( formattedAssertionMessage ), message, arglist );
		va_end( arglist );
		
		Handler* handler = Handler::GetInstance();
		return handler->Assert( formattedAssertionMessage, filename, line, expression, 1 );
	}

	Red::System::Log::Manager::GetInstance().SetCrashModeActive( true );
	RED_LOG_ERROR( Assert, TXT( "Recursive assert detected!" ) );
	Red::System::Log::Manager::GetInstance().SetCrashModeActive( false );

	return AA_Break;
}

EAssertAction HandleAssertion( const Char* filename, Uint32 line, const Char* expression )
{
	return HandleAssertion( filename, line, expression, defaultAssertMessage );
}

void HandleWarning( const Char* filename, Uint32 line, const Char* expression, const Char* message, ... )
{
	static RED_TLS bool isHandlingWarning = false;
	if( !isHandlingWarning )
	{
		isHandlingWarning = true;
		ScopedFlag< Bool > scopedRecursionCheck( isHandlingWarning, false );
	
		Char formattedWarningMessage[ MAX_MESSAGE_LENGTH ];
		
		if( !message )
		{
			message = defaultWarningMessage;
		}
		
		va_list arglist;
		va_start( arglist, message );
		VSNPrintF( formattedWarningMessage, ARRAY_COUNT( formattedWarningMessage ), message, arglist );
		va_end( arglist );

		Char details[ MAX_DETAILS_LENGTH ];
		Uint32 detailsUsed = 0;
	
		RED_APPEND_ERROR_STRING( details, MAX_DETAILS_LENGTH, detailsUsed, TXT( "%" ) MACRO_TXT( RED_PRIs ) TXT( "\n" ), expression );
		RED_APPEND_ERROR_STRING( details, MAX_DETAILS_LENGTH, detailsUsed, TXT( "%" ) MACRO_TXT( RED_PRIs ) TXT( "\n" ), formattedWarningMessage );
		RED_APPEND_ERROR_STRING( details, MAX_DETAILS_LENGTH, detailsUsed, TXT( "%" ) MACRO_TXT( RED_PRIs ) TXT( "(%u)\n\n" ), filename, line );
	
		Handler* errorHandler = Handler::GetInstance();
		errorHandler->GetDebugInformation( &details[ detailsUsed ], MAX_DETAILS_LENGTH - detailsUsed, DIF_CurrentThreadCallStack | DIF_AllCallBackInformation, 1 );
	
		HandleLogRequest( Red::CNameHash( "System" ), TXT( "System" ), Red::System::Log::P_Warning, details );
	}
}

void HandleWarning( const Char* filename, Uint32 line, const Char* expression )
{
	HandleWarning( filename, line, expression, defaultWarningMessage );
}

}}}
