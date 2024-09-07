/**
 * Copyright (c) 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"

# include "../../common/core/loadingProfiler.h"
#include "gameApplicationLinux.h"

int main( int argc, char** argv )
{
	GLoadingProfiler.Start();

	// Increase this if, for whatever reason, you need a longer one
	const int c_maxCommandLineLength = 2048;

	// Build a single string containing the command line arguments
	char commandLineConcat[ c_maxCommandLineLength ] = {'\0'};

	for( Int32 i = 1; i < argc; ++i )	// The first one appears to always by " ", so well ...
	{
		Red::System::StringConcatenate( commandLineConcat, argv[ i ], c_maxCommandLineLength );
		Red::System::StringConcatenate( commandLineConcat, " ", c_maxCommandLineLength );
	}

	wchar_t wCommandLineConcat[ c_maxCommandLineLength ] = {'\0'};
	Red::System::StringConvert( wCommandLineConcat, commandLineConcat, c_maxCommandLineLength );

	return mainLinux( wCommandLineConcat );
}
