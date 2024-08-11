/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#ifdef RED_PLATFORM_ORBIS

#include "../../common/game/configParser.h"

bool InitializeGameConfiguration()
{
	GameConfig::LoadConfig( TXT("r4") );

	return true;
}

void GetPlatformDefaultResolution( Int32 & width, Int32 & height )
{
	width = 1920;
	height = 1080;
}


Red::System::Error::EAssertAction AssertMessage( const Red::System::Char* cppFile, Red::System::Uint32 line, const Red::System::Char* expression, const Red::System::Char* message, const Red::System::Char* details )
{
	RED_UNUSED( cppFile );
	RED_UNUSED( line );
	RED_UNUSED( expression );
	RED_UNUSED( details );
	return Red::System::Error::AA_Continue;
}


#endif 
