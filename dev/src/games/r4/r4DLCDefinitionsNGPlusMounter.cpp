#include "build.h"
#include "r4DLCDefinitionsNGPlusMounter.h"


IMPLEMENT_ENGINE_CLASS( CR4DefinitionsNGPlusDLCMounter );

Bool CR4DefinitionsNGPlusDLCMounter::ShouldLoad()
{
	return GCommonGame->IsNewGamePlus();
}

void CR4DefinitionsNGPlusDLCMounter::Activate()
{
	if( ShouldLoad() )
		LoadDefinitions();
}

void CR4DefinitionsNGPlusDLCMounter::Deactivate()
{
	if( ShouldLoad() )
		UnloadDefinitions();
}