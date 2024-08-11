/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "messagePump.h"

IMessagePump* GMessagePump = nullptr;

#ifdef RED_PLATFORM_DURANGO
Bool GEnableMessagePumpDurangoCertHack = false;
Bool GReentrantMessagePumpCheck = false;
#endif