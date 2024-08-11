/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "redIOPlatform.h"

#if defined( RED_PLATFORM_ORBIS )

typedef Red::System::Int32			SceFiosHandle;
typedef SceFiosHandle				SceFiosFH;
typedef SceFiosHandle				SceFiosOp;
typedef Red::System::Uint8			SceFiosOpEvent; 

#endif