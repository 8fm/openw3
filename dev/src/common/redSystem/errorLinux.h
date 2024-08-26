/**
* Copyright (c) 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#ifndef _RED_ERROR_LINUX_H_
#define _RED_ERROR_LINUX_H_

#include "error.h"

namespace Red
{
	namespace System
	{
		namespace Error
		{
			class LinuxHandler : public Handler
			{
				virtual Bool IsDebuggerPresent() const override;
				virtual Bool WriteDump( const Char* filename ) override;
				virtual Uint32 GetCallstack( Char* output, Uint32 outputSize, Uint32 skipFrames ) override;
				virtual Uint32 GetCallstack( Char* output, Uint32 outputSize, const Internal::ThreadId& threadId, Uint32 skipFrames ) override;
				virtual void GetCallstack( Error::Callstack& stack, Uint32 skipFrames = 0 ) override;
				virtual void GetCallstack( Callstack& stack, const Internal::ThreadId& threadId, Uint32 skipFrames = 0 ) override;
				virtual Uint32 EnumerateThreads( Internal::ThreadId* threadIds, Uint32 maxThreads ) override;
				virtual void HandleUserChoice( EAssertAction chosenAction, const Char* cppFile, Uint32 line, const Char* expression, const Char* details ) override;
				virtual const Char *GetCommandline( ) const override;
			};
		}
	}
}

#endif //_RED_ERROR_LINUX_H_
