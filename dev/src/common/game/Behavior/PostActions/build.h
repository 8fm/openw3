/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

// This is a wrapper file for ORBIS
// Windows expects #include "build.h" regardless of sub directory
// whereas orbis expects the correct path to be specified
// This wrapper file solves the problem, put a copy of this file in every
// sub directory in order for PCHs to work with both windows and orbis

#include "../../build.h"