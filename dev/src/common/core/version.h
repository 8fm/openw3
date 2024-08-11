/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

//! application major version - 0 - prior to release, 1 - after release
#define APP_VERSION_MAJOR	"3"

//! application minor version - basically last milestone number
#define APP_VERSION_MINOR	"0"

// build number, incremented with every build
#define APP_VERSION_BUILD	"BUILD_VERSION_FULL"

//! last P4 change used to compile new build
#define APP_LAST_P4_CHANGE 	"CL_INTERNAL"

//! the p4 stream name
#define APP_P4_STREAM 	"STREAM_NAME"

#define APP_P4_SHELF "P4_SHELF"

#define GAME_VERSION "v 1.60"

#if defined(BUILD_MACHINE)
#define PRODUCT_VERSION_STR "3.0.0.BUILD_VERSION_FULL(Build Machine)"
#define PRODUCT_VERSION 3,0,BUILD_VERSION_MAJOR,BUILD_VERSION_MINOR
#else
#define PRODUCT_VERSION_STR "3.0.0.0(Local Build)"
#define PRODUCT_VERSION 3,0,0,0
#endif

#define APP_VERSION_NUMBER TXT2(APP_VERSION_MAJOR) TXT(".") TXT2(APP_VERSION_MINOR) TXT(".") TXT2(APP_VERSION_BUILD) TXT("  P4CL: ") TXT2(APP_LAST_P4_CHANGE) TXT("  Stream: ") TXT2(APP_P4_STREAM) TXT(" ") TXT2(APP_P4_SHELF)


#define APP_DATE TXT2(__DATE__)

// TIMESTAMP: 2010-05-12 16:06:44
