/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#ifndef _RED_ERROR_WINDOWS_H_
#define _RED_ERROR_WINDOWS_H_

#include "os.h"
#include "types.h"
#include "error.h"

// Creates memory file with dump that can be passed to testtracker instead of creating minidump by itself
#define RED_CRASH_HANDLER_CREATES_DUMP

namespace Red
{
	namespace System
	{
		namespace Error
		{
			class WindowsHandler : public Handler
			{
			private:
				struct StackInfo
				{
					DWORD64 frameAddress;

					Char symbol[ RED_SYMBOL_MAX_LENGTH ];
					DWORD64 symbolDisplacement;

					Char filename[ RED_SYMBOL_MAX_LENGTH ];
					DWORD lineDisplacement;
					Uint32 lineNumber;
				};

				struct ThreadHandle
				{
					ThreadHandle();

					HANDLE handle;
					Bool opened;
				};

				struct ScopedThreadHandle : ThreadHandle
				{
					~ScopedThreadHandle();
				};

			public:
				WindowsHandler();
				virtual ~WindowsHandler() override final;

			public:
				virtual void SetLogFile( const Char* filepath, Log::File* ) override final;
				virtual Bool IsDebuggerPresent() const override;
				virtual Bool WriteDump( const Char* filename ) override final;
				virtual Uint32 GetCallstack( Char* output, Uint32 outputSize, Uint32 skipFrames = 0 ) override final;
				virtual Uint32 GetCallstack( Char* output, Uint32 outputSize, const Internal::ThreadId& threadId, Uint32 skipFrames = 0 ) override final;
				virtual void GetCallstack( Callstack& stack, Uint32 skipFrames = 0 ) override final;
				virtual void GetCallstack( Callstack& stack, const Internal::ThreadId& threadId, Uint32 skipFrames = 0 ) override final;
				virtual Uint32 EnumerateThreads( Internal::ThreadId* threadIds, Uint32 maxThreads ) override final;

			private:
				virtual void HandleUserChoice( EAssertAction chosenAction, const Char* cppFile, Uint32 line, const Char* expression, const Char* details ) override final;
				virtual const Char* GetCommandline() const override final;

				Uint32 WalkStack( StackInfo* stack, Uint32 size, ThreadHandle& threadHandle, PCONTEXT threadContext, Uint32 framesToSkip = 0 );
				void GetStackSymbols( StackInfo* stack, Uint32 numValidStackFrames );
				Uint32 PrintStack( StackInfo* stack, Uint32 numFrames, const Internal::ThreadId& threadId, Char* output, Uint32 outputSize );
				Bool GetThreadContext( const Internal::ThreadId& threadId, ThreadHandle& thread, CONTEXT& context );
				void GetThreadContext( ThreadHandle& thread, CONTEXT& context );
				Uint32 GetLastError( Char* output, Uint32 outputSize );
				Uint32 ExceptionAsString( PEXCEPTION_POINTERS exceptionPointers, Char* output, Uint32 outputSize );
				const Char* ExceptionAsString( DWORD code );
				Bool WriteDump( const Char* filename, const Internal::ThreadId& threadId );
				Bool WriteDump( PEXCEPTION_POINTERS exceptionPointers, DWORD threadId, const Char* filename );
				Bool GenerateExceptionPointers( const Internal::ThreadId& threadId, EXCEPTION_POINTERS& exceptionInfo, EXCEPTION_RECORD& exceptionRecord, CONTEXT& context );

			private:

#ifdef RED_CRASH_HANDLER_CREATES_DUMP
				struct CrashInfo
				{
					EXCEPTION_RECORD record;
					CONTEXT context;
				};

				HANDLE m_exceptionInfoMapFile;
				void* m_exceptionInfoBuffer;
#endif

				Char m_logFilepath[ 1024 ];

				static LONG CALLBACK DebuggableHandleException( PEXCEPTION_POINTERS exceptionInfo );
				void DoHandleException( EXCEPTION_POINTERS* exceptionInfo );
				static LONG WINAPI HandleException( EXCEPTION_POINTERS* exceptionInfo );
				static void PureCallHandler();

				CRITICAL_SECTION m_criticalSection;
			};
		}
	}
}

#endif //_RED_ERROR_WINDOWS_H_
