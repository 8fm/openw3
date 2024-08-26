/**
* Copyright (c) 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#include "errorLinux.h"

namespace Red
{
	namespace System
	{
		namespace Error
		{
			//////////////////////////////////////////////////////////////////////////
			// Public Interface

			Bool LinuxHandler::IsDebuggerPresent() const
			{
#ifndef RED_FINAL_BUILD
				Bool present = ( ptrace( PTRACE_TRACEME, 0, 1, 0 ) < 0 );
				if ( !present ) { ptrace( PTRACE_DETACH, 0, 1, 0 ); }
				return present;
#else
				return false;
#endif
			}

			Bool LinuxHandler::WriteDump( const Char* filename )
			{
				RED_UNUSED( filename );
				return false;
			}

			Uint32 LinuxHandler::GetCallstack( Char* output, Uint32 outputSize, Uint32 skipFrames )
			{
				RED_UNUSED( output );
				RED_UNUSED( outputSize );
				RED_UNUSED( skipFrames );
				return 0;
			}

			Uint32 LinuxHandler::GetCallstack( Char* output, Uint32 outputSize, const Internal::ThreadId& threadId, Uint32 skipFrames )
			{
				RED_UNUSED( output );
				RED_UNUSED( outputSize );
				RED_UNUSED( threadId );
				RED_UNUSED( skipFrames );
				return 0;
			}

			void LinuxHandler::GetCallstack( Error::Callstack& stack, Uint32 skipFrames )
			{
				RED_UNUSED( stack );
				RED_UNUSED( skipFrames );

			}

			void LinuxHandler::GetCallstack( Callstack& stack, const Internal::ThreadId& threadId, Uint32 skipFrames /*= 0 */ )
			{
				RED_UNUSED( stack );
				RED_UNUSED( skipFrames );
				RED_UNUSED( threadId );
			}

			Uint32 LinuxHandler::EnumerateThreads( Internal::ThreadId* threadIds, Uint32 maxThreads )
			{
				RED_UNUSED( threadIds );
				RED_UNUSED( maxThreads );
				return 0;
			}

			const Char *LinuxHandler::GetCommandline() const
			{
				return TXT("");
			}

			void LinuxHandler::HandleUserChoice( EAssertAction chosenAction, const Char* cppFile, Uint32 line, const Char* expression, const Char* details )
			{
				RED_UNUSED( chosenAction );
				RED_UNUSED( cppFile );
				RED_UNUSED( line );
				RED_UNUSED( expression );
				RED_UNUSED( details );
			}

			//////////////////////////////////////////////////////////////////////////
			// Create Instance
			Error::Handler* Error::Handler::GetInstance()
			{
				if( m_handlerInstance == nullptr )
				{
					static LinuxHandler instance;
					Handler::SetInternalInstance( &instance );
				}

				return m_handlerInstance;
			}
		}
	}
}
