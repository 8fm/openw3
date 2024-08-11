/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "../../common/core/types.h"
#include "../r4/gameEngine.h"

bool InitializeGameConfiguration();

struct CommandLineArguments
{
	CommandLineArguments();
	CR4GameEngine::Parameters engineParameters;
	bool silentScripts;
};

void ExtractCommandLineArguments( const String & commandLine, CommandLineArguments & result );

void GetPlatformDefaultResolution( Int32 & width, Int32 & height );

Red::System::Error::EAssertAction AssertMessage( const Red::System::Char* cppFile, Red::System::Uint32 line, const Red::System::Char* expression, const Red::System::Char* message, const Red::System::Char* details );
