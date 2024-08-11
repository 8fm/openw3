/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#pragma once
#ifndef _RED_SYSTEM_SETTINGS_H_
#define _RED_SYSTEM_SETTINGS_H_

#include "architecture.h"

#define RED_USE_NEW_MEMORY_SYSTEM

#define RED_LOG_TIMESTAMPS
#define RED_LOG_PRIORITY
#define RED_LOG_CHANNELNAMES

// Uncomment this to remove storage of string data from Names pool
#define RED_NAMES_DEBUG_STRINGS

// Uncomment this to store and process strings as Unicode
#define RED_NAMES_USE_ANSI

#if !defined( RED_FINAL_BUILD ) || defined( LOG_IN_FINAL )
	// Uncomment this to remove all debug logging from the codebase
	#define RED_LOGGING_ENABLED

	// Uncomment this to remove asserts from the codebase
	#define RED_ASSERTS_ENABLED
#endif

#if defined( RED_PLATFORM_WINPC ) || ( (!defined( RED_FINAL_BUILD ) || defined( RED_PROFILE_BUILD )) && defined( RED_PLATFORM_CONSOLE ) )
	#define RED_NETWORK_ENABLED
#endif

// This needs to be available in "FinalWithLogging" configuration, but unavailable in "Final"
#ifdef RED_LOGGING_ENABLED
	#define SAVE_SERVER_ENABLED
#endif

#endif // _RED_SYSTEM_SETTINGS_H_
