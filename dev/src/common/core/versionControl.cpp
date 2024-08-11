/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "versionControl.h"

#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT

#include "changelist.h"

const SChangelist SChangelist::DEFAULT;

// The NULL version control, default one
ISourceControl *GVersionControl = new ISourceControl();

#endif