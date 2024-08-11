/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#ifndef _RED_ERROR_ORBIS_H_
#define _RED_ERROR_ORBIS_H_

#include "error.h"
#include "os.h"

namespace Red
{
	namespace System
	{
		namespace Error
		{
			class DurangoHandler : public Handler
			{
			private:
				struct DumpInfo
				{
					PEXCEPTION_POINTERS exceptionPointers;
					DWORD threadId;
					const Char* filename;
				};

			public:
				DurangoHandler();
				~DurangoHandler();

			private:

				static LONG WINAPI HandleException( EXCEPTION_POINTERS* exceptionInfo );
				static LONG CALLBACK DurangoHandler::DebuggableHandleException( PEXCEPTION_POINTERS exceptionInfo );

				Bool WriteDump( PEXCEPTION_POINTERS exceptionPointers, DWORD threadId, const Char* filename );
				static DWORD WINAPI WriteDumpThread( LPVOID );

			public:
				static void SetMiniDumpFileName( const Char* fileName );

#ifdef RED_LOGGING_ENABLED
				static void SetCrashLogFileName( const Char* crashLogFileName );
#endif

			public:

				// Public interface
				virtual Bool IsDebuggerPresent() const override;
				virtual Bool WriteDump( const Char* filename );
				virtual Uint32 GetCallstack( Char* output, Uint32 outputSize, Uint32 skipFrames );
				virtual Uint32 GetCallstack( Char* output, Uint32 outputSize, const Internal::ThreadId& threadId, Uint32 skipFrames );
				virtual void GetCallstack( Error::Callstack& stack, Uint32 skipFrames );
				virtual void GetCallstack( Callstack& stack, const Internal::ThreadId& threadId, Uint32 skipFrames = 0 );
				virtual Uint32 EnumerateThreads( Internal::ThreadId* threadIds, Uint32 maxThreads );
				virtual void HandleUserChoice( EAssertAction chosenAction, const Char* cppFile, Uint32 line, const Char* expression, const Char* details );
				virtual const Char* GetCommandline() const;

				virtual void SetLogFile( const Char* logFilename, Log::File* instance ) override final;

			private:
				static CRITICAL_SECTION m_miniDumpMutex;
				static DumpInfo m_dumpInfo;
				static Char m_miniDumpFileName[ MAX_PATH ];

#ifdef RED_LOGGING_ENABLED
				static Char m_origLogFileName[ MAX_PATH ];
				static Char m_crashLogFileName[ MAX_PATH ];
#endif // RED_LOGGING_ENABLED

				static Bool m_hasDumped;

				Log::File* m_logFile;
				static const Uint32 NON_FATAL_EXCEPTIONS[];
			};
		}
	}
}

#endif //_RED_ERROR_ORBIS_H_
