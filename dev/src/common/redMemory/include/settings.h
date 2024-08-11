/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#ifndef _RED_MEMORY_SETTINGS_H_
#define _RED_MEMORY_SETTINGS_H_

//////////////////////////////////////////////////////////////////////////

// #define RED_MEMORY_UNIT_TEST					-> enable this for all target that will compile memory unit test.
// #define RED_MEMORY_ENABLE_HOOKS				-> This is use to hook debug metadata or metrics in the allocation process. This need to be enable or no debug/metrics/metadata will be present. 
// #define RED_MEMORY_ENABLE_METRICS			-> Basic metrics system. Only compute memory allocated per pool by default
// #define RED_MEMORY_ENABLE_EXTENDED_METRICS	-> Add more info about pool allocations. (Like alloc count, alloc per frame etc..)
// #define RED_MEMORY_ENABLE_LOGGING			-> Enable Extended logging. It will forward to System logger.
// #define RED_MEMORY_USE_DEBUG_ALLOCATOR		-> Route all allocation the DebugAllocator. Use with GFlags to trap memory stomp
// #define RED_MEMORY_ENABLE_ASSERTS			-> Compile with assertion code.

//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////

#if defined( RED_LOGGING_ENABLED )

	#define RED_MEMORY_ENABLE_LOGGING

#endif

//////////////////////////////////////////////////////////////////////////

#if defined( RED_CONFIGURATION_DEBUG )

	#define RED_MEMORY_UNIT_TEST
	//#define RED_MEMORY_ENABLE_METRICS
	//#define RED_MEMORY_ENABLE_EXTENDED_METRICS
	#define RED_MEMORY_ENABLE_ASSERTS

#elif defined( RED_CONFIGURATION_NOPTS )

	#define RED_MEMORY_UNIT_TEST
	//#define RED_MEMORY_ENABLE_METRICS
	//#define RED_MEMORY_ENABLE_EXTENDED_METRICS
	#define RED_MEMORY_ENABLE_ASSERTS

#elif defined( RED_CONFIGURATION_RELEASE )

	#ifndef RED_PROFILE_BUILD
		#define RED_MEMORY_UNIT_TEST
		//#define RED_MEMORY_ENABLE_METRICS
		//#define RED_MEMORY_ENABLE_EXTENDED_METRICS
		#define RED_MEMORY_ENABLE_ASSERTS
	#endif

#elif defined( RED_CONFIGURATION_FINAL )

#endif

//////////////////////////////////////////////////////////////////////////

#endif