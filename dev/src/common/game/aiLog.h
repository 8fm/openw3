/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#ifndef NO_LOG
	#define AI_LOG( format, ... ) RED_LOG( AI, format, ## __VA_ARGS__ )
#else
	#define AI_LOG( format, ... )
#endif

//#define AI_ASSERTS_ENABLED


#ifdef AI_ASSERTS_ENABLED
#define AI_ASSERT( ... ) RED_ASSERT( ## __VA_ARGS__ )
#else
#define AI_ASSERT( ... )
#endif