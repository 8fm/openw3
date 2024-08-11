/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"
#include "idHud.h"

IMPLEMENT_RTTI_ENUM( EHudChoicePosition )
IMPLEMENT_RTTI_ENUM( EDialogDisplayMode )

void IDialogHud::UpdateInput()
{
	RED_LOG( Dialog, TXT("Bla bla bla") ); // to be removed - i needed to place something in the .cpp because otherwise IMPLEMENT_RTTI_ENUM() dosn't get linked into the exe
}