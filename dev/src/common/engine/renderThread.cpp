/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "renderThread.h"

IRenderThread::IRenderThread()
	: Red::Threads::CThread( "RenderThread" )
{
};