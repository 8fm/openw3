/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#include "errorOrbis.h"

#ifndef RED_FINAL_BUILD		// Can't have that in the package.
#include <libdbg.h>
#pragma comment ( lib, "libSceDbg_stub_weak.a" )
#endif

namespace Red
{
	namespace System
	{
		namespace Error
		{
			//////////////////////////////////////////////////////////////////////////
			// Public Interface

			Bool OrbisHandler::IsDebuggerPresent() const
			{
#ifndef RED_FINAL_BUILD
				return ( sceDbgIsDebuggerAttached() )? true : false;
#else
				return false;
#endif
			}

			Bool OrbisHandler::WriteDump( const Char* filename )
			{
				RED_UNUSED( filename );
				return false;
			}

			Uint32 OrbisHandler::GetCallstack( Char* output, Uint32 outputSize, Uint32 skipFrames )
			{
				RED_UNUSED( output );
				RED_UNUSED( outputSize );
				RED_UNUSED( skipFrames );
				return 0;
			}

			Uint32 OrbisHandler::GetCallstack( Char* output, Uint32 outputSize, const Internal::ThreadId& threadId, Uint32 skipFrames )
			{
				RED_UNUSED( output );
				RED_UNUSED( outputSize );
				RED_UNUSED( threadId );
				RED_UNUSED( skipFrames );
				return 0;
			}

			void OrbisHandler::GetCallstack( Error::Callstack& stack, Uint32 skipFrames )
			{
				RED_UNUSED( stack );
				RED_UNUSED( skipFrames );

			}

			void OrbisHandler::GetCallstack( Callstack& stack, const Internal::ThreadId& threadId, Uint32 skipFrames /*= 0 */ )
			{
				RED_UNUSED( stack );
				RED_UNUSED( skipFrames );
				RED_UNUSED( threadId );
			}

			Uint32 OrbisHandler::EnumerateThreads( Internal::ThreadId* threadIds, Uint32 maxThreads )
			{
				RED_UNUSED( threadIds );
				RED_UNUSED( maxThreads );
				return 0;
			}

			const Char *OrbisHandler::GetCommandline() const
			{
				return TXT("");
			}

			void OrbisHandler::HandleUserChoice( EAssertAction chosenAction, const Char* cppFile, Uint32 line, const Char* expression, const Char* details )
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
					static OrbisHandler instance;
					Handler::SetInternalInstance( &instance );
				}
	
				return m_handlerInstance;
			}
		}
	}
}
