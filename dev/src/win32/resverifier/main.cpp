
#include "build.h"

#pragma comment(lib, "Winmm.lib")

int main( int argc, const char* argv[] )
{
	String basePath;
	if (!ParseCommandLine(TXT("-d"), basePath, argc, argv))
	{
		wprintf(TXT("resverifier -d<depot>\n") );
		wprintf(TXT("Options:\n") );
		wprintf(TXT("   -d data directory, required\n"));
		wprintf(TXT("   -t, target directories separated with ; (default: data directory)\n"));
		wprintf(TXT("ex : resverifier -dc:\\XXX\\ -> set data dir\n") );
		wprintf(TXT("ex : resverifier -dc:\\XXX\\ -tcharacters;templates -> set data dirs and target directory\n") );
		return 1;
	}

	if (basePath[basePath.GetLength()-1] != TXT('\\') && basePath[basePath.GetLength()-1] != TXT('/'))
		basePath += TXT("\\");

	String targetDirectory = TXT("\\");
	ParseCommandLine(TXT("-t"), targetDirectory, argc, argv);

	CTokenizer tokenizer(targetDirectory, TXT(";"));

	TDynArray<String> targetDirs;
	for (Uint i = 0; i < tokenizer.GetNumTokens(); i++)
	{
		String dir = tokenizer.GetToken( i );
		if (dir[dir.GetLength()-1] != TXT('\\') && dir[dir.GetLength()-1] != TXT('/'))
			dir += TXT("\\");
		targetDirs.PushBack( dir );
	}


	// Initialize platform
	SpecialInitializePlatform(basePath.AsChar());



	// Setting custom error system
	GError = &GResVerifierErrorSystem;

	// Start resource verifier engine
	CResVerifierEngine* engine = new CResVerifierEngine();
	if ( engine->Initialize() )
	{


		// Create or open existing PhysicsCache
		/*GPhysicsCache = CPhysicsCache::Create( TXT("physics.cache") );
		if ( !GPhysicsCache )
		{
			WARN( TXT("Unable to open/create Physics Cache") );
			return false;
		}
		GPhysicsCache->AddToRootSet();*/

		for (Uint i = 0; i < targetDirs.Size(); i++)
			engine->ParseDepot( basePath + targetDirs[i] );

		// Finished
		RESV_LOG( TXT("Finished."));

		// Time to exit, remove PhysicsCache
		//GPhysicsCache->RemoveFromRootSet();
		//GPhysicsCache = NULL;

		SGarbageCollector::GetInstance().Collect();

		SetConsoleTextAttribute( GetStdHandle( STD_OUTPUT_HANDLE ), 7 );

		// Close the engine
		engine->Shutdown();
		delete engine;
	}

	// Shutdown
	SShutdownPlatform();

	return 0;
}
