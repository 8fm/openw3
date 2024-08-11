/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#ifndef _RED_ERROR_ORBIS_H_
#define _RED_ERROR_ORBIS_H_

#include "error.h"

namespace Red
{
	namespace System
	{
		namespace Error
		{
			class OrbisHandler : public Handler
			{
				virtual Bool IsDebuggerPresent() const override;
				virtual Bool WriteDump( const Char* filename );
				virtual Uint32 GetCallstack( Char* output, Uint32 outputSize, Uint32 skipFrames );
				virtual Uint32 GetCallstack( Char* output, Uint32 outputSize, const Internal::ThreadId& threadId, Uint32 skipFrames );
				virtual void GetCallstack( Error::Callstack& stack, Uint32 skipFrames = 0 );
				virtual void GetCallstack( Callstack& stack, const Internal::ThreadId& threadId, Uint32 skipFrames = 0 );
				virtual Uint32 EnumerateThreads( Internal::ThreadId* threadIds, Uint32 maxThreads );
				virtual void HandleUserChoice( EAssertAction chosenAction, const Char* cppFile, Uint32 line, const Char* expression, const Char* details );
				virtual const Char *GetCommandline( ) const;
			};
		}
	}
}

#endif //_RED_ERROR_ORBIS_H_
