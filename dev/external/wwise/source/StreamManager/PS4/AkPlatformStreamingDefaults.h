//////////////////////////////////////////////////////////////////////
//
// AkPlatformStreamingDefaults.h
//
// Platform-specific default values for streaming and I/O device settings.
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#ifndef _AK_PLATFORM_STREAMING_DEFAULTS_H_
#define _AK_PLATFORM_STREAMING_DEFAULTS_H_

// I/O pool settings.

#define AK_DEFAULT_BLOCK_ALLOCATION_TYPE	(AkMalloc)// Block allocation type.

// Note that the I/O pool is a FixedSizedBlock-style pool and it has no lock: all allocations have 
// the same size, which makes it very efficient. System memory alignment depends on the allocation
// hook. Otherwise, all allocations are aligned to multiples of the granularity. 

#define AK_REQUIRED_IO_POOL_ALIGNMENT		(4)				// No requirement. If granularity is not a multiple of 2 KB, I/O data will be buffered by the system.

#endif //_AK_PLATFORM_STREAMING_DEFAULTS_H_
