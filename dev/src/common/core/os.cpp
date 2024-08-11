/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "dependencyMapper.h"

// True if we are running under editor
Bool GIsEditor = false;

// True if we are running cooker
Bool GIsCooker = false;

// Set to destination platform for cooking (***ab> troche slabe bo juz jest CookingContext zawierajacy platforme, ale metody OnSerialize nie maja do niego dostepu)
Uint32 GCookingPlatform = PLATFORM_PC;

// True if we are closing
Bool GIsClosing = false;

// True if we are inside an editor game ( PIE )
Bool GIsEditorGame = false;

// True if we are running game.exe
Bool GIsGame = false;
