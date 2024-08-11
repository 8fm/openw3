
// Core
#include "..\..\common\core\core.h"

// Version control
#include "reslinkerVersionControlInterface.h"
#include "..\..\win32\versionControl\versionControlP4.h"
#include "..\..\win32\versionControl\versionControlAlienBrain.h"

// Engine
#include "..\..\common\engine\engine.h"

// Win32 platform
#include "..\..\win32\platform\win32.h"

// Exception handling
#include "..\platform\errorWin32.h"

// Resource linker engine
#include "reslinkerEngine.h"
#include "reslinkerDependencyLoader.h"
#include "versionControl.h"

Bool ParseCommandLine( String tag, String& output, Int argCount, const char* argV[]);
void SpecialInitializePlatform(const Char* inputBasePath);
