/**
* Copyright © 2007 CDProjekt Red, Inc. All Rights Reserved.
*
* This file contains operating system functions
*/
#include "build.h"
#include "explatformos.h"
#include "string.h"

Red::System::Internal::ThreadId GMainThreadID;
String* GCommandLine = NULL;

Bool SIsMainThread()
{
	extern Red::System::Internal::ThreadId GMainThreadID;
	return ! GMainThreadID.IsValid() || GMainThreadID == Red::System::Internal::ThreadId::CurrentThread();
}

const Red::System::Internal::ThreadId& SGetMainThreadId()
{
	extern Red::System::Internal::ThreadId GMainThreadID;
	return GMainThreadID;
}

const Char* SGetCommandLine()
{
	return GCommandLine ? GCommandLine->AsChar() : TXT("");
}

