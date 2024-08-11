
#include "build.h"

#pragma comment(lib, "Winmm.lib")

int main( int argc, const char* argv[] )
{
	// Initialize platform
	
	String basePath;
	if (!ParseCommandLine(TXT("-d"), basePath, argc, argv))
	{
		wprintf( TXT("reslinker -d<depot> [-t<target>] [-vcp4] [-u<user>] [-p<password>] [-w<workspace>] [-link] [-ns] [-fl] [-s] [-m<address>]\n") );
		wprintf( TXT("Options:\n") );
		wprintf( TXT("   -d data directory, required\n") );
		wprintf( TXT("   -t target directory (default data directory)\n") );
		wprintf( TXT("   -vcp4 use preforce version control\n") );
		wprintf( TXT("   -u perforce user name\n") );
		wprintf( TXT("   -p perforce password\n") );
		wprintf( TXT("   -w perforce workspace\n") );
		wprintf( TXT("   -link don't delete link files after remapping\n") );
		wprintf( TXT("   -ns don't submit files after remapping\n") );
		wprintf( TXT("   -fl force load\n") );
		wprintf( TXT("   -s silent mode, repairs only unchecked files\n") ); 
		wprintf( TXT("   -m send an e-mail to the specified address (must be @cdprojektred.com)\n") );
		wprintf( TXT("   -rs skip resources, go directly to world files\n") );
		wprintf( TXT("ex : reslinker -dc:\\     -> set data dir, don't use preforce ,default user sett\n") );
		wprintf( TXT("ex : reslinker -dc:\\data\\ -ttests\\    -> set target dir data\\tests\\\n") );
		wprintf( TXT("ex : reslinker -dDDD -p4   -> use preforce and default settings\n") );
		wprintf( TXT("ex : reslinker -dDDD -uXXX -pYYY -wZZZ -> use preforce and user settings\n") );
		wprintf( TXT("ex : reslinker -dDDD -msomeone@cdprojektred.com   -> send e-mail to someone@cdprojektred.com\n") );
		return 1;
	}

	SpecialInitializePlatform(basePath.AsChar());
	//SInitializePlatform( "" );

	// Start resource linker engine
	CResLinkerEngine* engine = new CResLinkerEngine();
	if ( engine->Initialize() )
	{
		// Register engine as output device
		SLog::GetInstance().AddOutput( engine );

		// Create or open existing PhysicsCache
		/*GPhysicsCache = CPhysicsCache::Create( TXT("physics.cache") );
		if ( !GPhysicsCache )
		{
			WARN( TXT("Unable to open/create Physics Cache") );
			return false;
		}
		GPhysicsCache->AddToRootSet();*/

		// Process
		if ( !engine->Main( argc, argv ) )
		{
			//errorCode = 1;
		}

		// Time to exit, remove PhysicsCache
		//GPhysicsCache->RemoveFromRootSet();
		//GPhysicsCache = NULL;

		SGarbageCollector::GetInstance().Collect();

		// Unregister output device
		SLog::GetInstance().RemoveOutput( engine );
		SetConsoleTextAttribute( GetStdHandle( STD_OUTPUT_HANDLE ), 7 );

		// Close the engine
		engine->Shutdown();
		delete engine;
	}

	// Shutdown
	SShutdownPlatform();

	return 0;
}