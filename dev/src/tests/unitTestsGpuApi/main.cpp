/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "utility.h"
#include "gatTTYWriter.h"

#if defined(RED_PLATFORM_WINPC)

#include "memoryInitializationWin.h"
#include "../../common/redSystem/windowsDebuggerWriter.h"
typedef Red::System::Log::WindowsDebuggerWriter LogTTYWriter;

/////////////////////////////////// VERY TEMPSHIT SOLUTION OF FIXING A BUILD ////////////////////////////////////
void EntityHandleDataGetObjectHandle( const void *handleData, THandle< CObject >& objectHandle ){}
void EntityHandleDataSetObjectHandle( void *handleData, THandle< CObject >& objectHandle ){}

#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "Advapi32.lib")
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#elif defined(RED_PLATFORM_ORBIS)

#include "memoryInitiaizationOrbis.h"
#include "../../common/redSystem/ttyWriter.h"
typedef Red::System::Log::TTYWriter LogTTYWriter;

#elif defined(RED_PLATFORM_DURANGO)

#include "../../common/redSystem/windowsDebuggerWriter.h"
typedef Red::System::Log::WindowsDebuggerWriter LogTTYWriter;

#endif

#if defined(RED_PLATFORM_WINPC)
static CGatTTYWriter gatTTYWriter;
#endif

#ifdef RED_LOGGING_ENABLED
	static LogTTYWriter logTTYWriter;

#	if defined( RED_PLATFORM_DURANGO )
		Red::System::Log::File fileLogger( TXT( "d:\\unittests_gpuapi_durango.log" ), true );
#	elif defined( RED_PLATFORM_ORBIS )
		Red::System::Log::File fileLogger( TXT( "/hostapp/bin/unittests_gpuapi_orbis.log" ), true );
#	elif defined( RED_PLATFORM_WINPC )
		Red::System::Log::File fileLogger( TXT( "..\\unittests_gpuapi_windows.log" ), true );
#	endif
#endif

#if defined(RED_PLATFORM_ORBIS)
void BindNativeOOMHandlerForAllocator( Red::MemoryFramework::MemoryManager* )
{
}
#endif

Red::System::Error::EAssertAction AssertMessageImp( const Red::System::Char* , Red::System::Uint32 , const Red::System::Char* , const Red::System::Char* , const Red::System::Char*  )
{
	return Red::System::Error::AA_Continue;
}

#if defined(RED_PLATFORM_WINPC) || defined(RED_PLATFORM_ORBIS)

int main( int argc, char** argv )
{
	// Increase this if, for whatever reason, you need a longer one
	const int c_maxCommandLineLength = 2048;
	// Build a single string containing the command line arguments
	char commandLineConcat[ c_maxCommandLineLength ] = {'\0'};

	for( GpuApi::Int32 i = 0; i < argc; ++i )
	{
		Red::System::StringConcatenate( commandLineConcat, argv[ i ], c_maxCommandLineLength );
		Red::System::StringConcatenate( commandLineConcat, " ", c_maxCommandLineLength );
	}

	wchar_t wCommandLineConcat[ c_maxCommandLineLength ] = {'\0'};
	Red::System::StringConvert( wCommandLineConcat, commandLineConcat, c_maxCommandLineLength );

	CommandLineArguments arguments;
	ExtractCommandLineArguments( wCommandLineConcat, arguments );

	MemoryInitializer::InitializeMemory();
	SInitializePlatform( arguments );

	int retval = Run();

	// Shutdown engine platform
	SShutdownPlatform();

	return retval;
}

#endif // _RED_PLATFORM_DURANGO_