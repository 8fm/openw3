/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../redSystem/settings.h"

#ifndef RED_ASSERTS_ENABLED
#	define NO_GPU_ASSERTS
#endif

#ifndef RED_LOGGING_ENABLED
#	define NO_GPU_LOGGING
#endif

#ifndef NO_GPU_LOGGING
	#include "..\\redSystem\\log.h"
	#define GPUAPI_LOG( format, ... )		RED_LOG( RED_LOG_CHANNEL( GpuApi ), format, ##__VA_ARGS__ )
	#define GPUAPI_LOG_WARNING( format, ... )	RED_LOG_WARNING( RED_LOG_CHANNEL( GpuApi ), format, ##__VA_ARGS__ )
	#define GPUAPI_ERROR( format, ... )		RED_LOG_ERROR( RED_LOG_CHANNEL( GpuApi ), format, ##__VA_ARGS__ )

#else
	#define GPUAPI_LOG( format, ... )
	#define GPUAPI_LOG_WARNING( format, ... )
	#define GPUAPI_ERROR( format, ... )
#endif

#define GPU_API_USES_RED_MEMORY_FRAMEWORK
#ifndef NO_GPU_ASSERTS
	#include "../redSystem/error.h"
	#define GPUAPI_ASSERT				RED_ASSERT
	#define GPUAPI_WARNING1				RED_WARNING
	#define GPUAPI_HALT					RED_HALT
	#define GPUAPI_FATAL( format, ... )	RED_FATAL_ASSERT( (false), format, ##__VA_ARGS__ )
	#define GPUAPI_FATAL_ASSERT			RED_FATAL_ASSERT
#else
	#define GPUAPI_ASSERT( ... )
	#define GPUAPI_HALT( ... )
	#define GPUAPI_WARNING1( ... )
	#define GPUAPI_FATAL( ... )
	#define GPUAPI_FATAL_ASSERT( ... )
#endif
