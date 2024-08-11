
#include "build.h"
#include "cutsceneModifier.h"

IMPLEMENT_ENGINE_CLASS( ICutsceneModifier );
IMPLEMENT_ENGINE_CLASS( CCutsceneModifierFreezer );

//////////////////////////////////////////////////////////////////////////

ICutsceneModifierInstance* CCutsceneModifierFreezer::CreateInstance( CCutsceneInstance* instance ) const
{
	return NULL;
}
