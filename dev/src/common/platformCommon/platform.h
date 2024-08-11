/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#include "platformFeature.h"

#include "../core/commandLineParams.h"

class IPlatformViewport;

// This class contains all platform-specific (but not executable specific) code required
// to start the engine
class CPlatform
{
public:
	// Platform initialization (stuff needed by the engine). Implement these for your platform of choice!
	static void Initialize( const String& commandLine, const Core::CommandLineArguments& coreArguments );
	static void Shutdown();
	static void InitializeVersionControl();
	static IPlatformViewport* CreateViewport( bool isEditor );

	// Executable-specific functions that are called during platform init
	typedef void (*VersionControlFn)( void );
	static void SetVersionControlInitialiser( VersionControlFn versionControlFn );
	static void SetAssertHandler( Red::System::Error::Handler::AssertHook assertFn );
		
	static CPlatformFeature* GetFeature( EPlatforFeature featur );

private:
	// These should return the various paths used to initialise the engine systems
	static void SetupPlatformPaths( String &rootPath, String &workingPath, String &dataPath, String &bundlePath, String &userPath, String &partPath, String &configPath, String &scriptPath );

	static Red::System::Error::Handler::AssertHook m_assertHandlerFn;
	static VersionControlFn m_versionControlFn;
};