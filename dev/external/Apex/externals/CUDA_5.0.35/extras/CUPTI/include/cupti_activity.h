/*
 * Copyright 2011-2012 NVIDIA Corporation.  All rights reserved.
 *
 * NOTICE TO LICENSEE:
 *
 * This source code and/or documentation ("Licensed Deliverables") are
 * subject to NVIDIA intellectual property rights under U.S. and
 * international Copyright laws.
 *
 * These Licensed Deliverables contained herein is PROPRIETARY and
 * CONFIDENTIAL to NVIDIA and is being provided under the terms and
 * conditions of a form of NVIDIA software license agreement by and
 * between NVIDIA and Licensee ("License Agreement") or electronically
 * accepted by Licensee.  Notwithstanding any terms or conditions to
 * the contrary in the License Agreement, reproduction or disclosure
 * of the Licensed Deliverables to any third party without the express
 * written consent of NVIDIA is prohibited.
 *
 * NOTWITHSTANDING ANY TERMS OR CONDITIONS TO THE CONTRARY IN THE
 * LICENSE AGREEMENT, NVIDIA MAKES NO REPRESENTATION ABOUT THE
 * SUITABILITY OF THESE LICENSED DELIVERABLES FOR ANY PURPOSE.  IT IS
 * PROVIDED "AS IS" WITHOUT EXPRESS OR IMPLIED WARRANTY OF ANY KIND.
 * NVIDIA DISCLAIMS ALL WARRANTIES WITH REGARD TO THESE LICENSED
 * DELIVERABLES, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY,
 * NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE.
 * NOTWITHSTANDING ANY TERMS OR CONDITIONS TO THE CONTRARY IN THE
 * LICENSE AGREEMENT, IN NO EVENT SHALL NVIDIA BE LIABLE FOR ANY
 * SPECIAL, INDIRECT, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, OR ANY
 * DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THESE LICENSED DELIVERABLES.
 *
 * U.S. Government End Users.  These Licensed Deliverables are a
 * "commercial item" as that term is defined at 48 C.F.R. 2.101 (OCT
 * 1995), consisting of "commercial computer software" and "commercial
 * computer software documentation" as such terms are used in 48
 * C.F.R. 12.212 (SEPT 1995) and is provided to the U.S. Government
 * only as a commercial end item.  Consistent with 48 C.F.R.12.212 and
 * 48 C.F.R. 227.7202-1 through 227.7202-4 (JUNE 1995), all
 * U.S. Government End Users acquire the Licensed Deliverables with
 * only those rights set forth herein.
 *
 * Any use of the Licensed Deliverables in individual and commercial
 * software must include, in the user documentation and internal
 * comments to the code, the above Disclaimer and U.S. Government End
 * Users Notice.
 */

#if !defined(_CUPTI_ACTIVITY_H_)
#define _CUPTI_ACTIVITY_H_

#include <cupti_callbacks.h>
#include <cupti_events.h>
#include <cupti_metrics.h>
#include <cupti_result.h>

#ifndef CUPTIAPI
#ifdef _WIN32
#define CUPTIAPI __stdcall
#else 
#define CUPTIAPI
#endif 
#endif 

#if defined(__LP64__)
#define CUPTILP64 1
#elif defined(_WIN64)
#define CUPTILP64 1
#else
#undef CUPTILP64
#endif

#define ACTIVITY_RECORD_ALIGNMENT 8
#if defined(_WIN32) // Windows 32- and 64-bit
#define START_PACKED_ALIGNMENT __pragma(pack(push,1)) // exact fit - no padding
#define PACKED_ALIGNMENT __declspec(align(ACTIVITY_RECORD_ALIGNMENT))
#define END_PACKED_ALIGNMENT __pragma(pack(pop))
#elif defined(__GNUC__) // GCC
#define START_PACKED_ALIGNMENT
#define PACKED_ALIGNMENT __attribute__ ((__packed__)) __attribute__ ((aligned (ACTIVITY_RECORD_ALIGNMENT)))
#define END_PACKED_ALIGNMENT
#else // all other compilers
#define START_PACKED_ALIGNMENT
#define PACKED_ALIGNMENT
#define END_PACKED_ALIGNMENT
#endif

#if defined(__cplusplus)
extern "C" {
#endif 

/**
 * \defgroup CUPTI_ACTIVITY_API CUPTI Activity API
 * Functions, types, and enums that implement the CUPTI Activity API.
 * @{
 */

/**
 * \brief The kinds of activity records.
 *
 * Each activity record kind represents information about a GPU or an
 * activity occurring on a CPU or GPU. Each kind is associated with a
 * activity record structure that holds the information associated
 * with the kind.
 * \see CUpti_Activity
 * \see CUpti_ActivityAPI
 * \see CUpti_ActivityContext
 * \see CUpti_ActivityDevice
 * \see CUpti_ActivityEvent
 * \see CUpti_ActivityKernel
 * \see CUpti_ActivityMemcpy
 * \see CUpti_ActivityMemset
 * \see CUpti_ActivityMetric
 * \see CUpti_ActivityName
 * \see CUpti_ActivityMarker
 * \see CUpti_ActivityMarkerData
 * \see CUpti_ActivitySourceLocator
 * \see CUpti_ActivityGlobalAccess
 * \see CUpti_ActivityBranch
 * \see CUpti_ActivityOverhead
 */
typedef enum {
  /**
   * The activity record is invalid.
   */
  CUPTI_ACTIVITY_KIND_INVALID  = 0,
  /**
   * A host<->host, host<->device, or device<->device memory copy. The
   * corresponding activity record structure is \ref
   * CUpti_ActivityMemcpy.
   */
  CUPTI_ACTIVITY_KIND_MEMCPY   = 1,
  /**
   * A memory set executing on the GPU. The corresponding activity
   * record structure is \ref CUpti_ActivityMemset.
   */
  CUPTI_ACTIVITY_KIND_MEMSET   = 2,
  /**
   * A kernel executing on the GPU. The corresponding activity record
   * structure is \ref CUpti_ActivityKernel.
   */
  CUPTI_ACTIVITY_KIND_KERNEL   = 3,
  /**
   * A CUDA driver API function execution. The corresponding activity
   * record structure is \ref CUpti_ActivityAPI.
   */
  CUPTI_ACTIVITY_KIND_DRIVER   = 4,
  /**
   * A CUDA runtime API function execution. The corresponding activity
   * record structure is \ref CUpti_ActivityAPI.
   */
  CUPTI_ACTIVITY_KIND_RUNTIME  = 5,
  /**
   * An event value. The corresponding activity record structure is
   * \ref CUpti_ActivityEvent.
   */
  CUPTI_ACTIVITY_KIND_EVENT    = 6,
  /**
   * A metric value. The corresponding activity record structure is
   * \ref CUpti_ActivityMetric.
   */
  CUPTI_ACTIVITY_KIND_METRIC   = 7,
  /**
   * Information about a device. The corresponding activity record
   * structure is \ref CUpti_ActivityDevice.
   */
  CUPTI_ACTIVITY_KIND_DEVICE   = 8,
  /**
   * Information about a context. The corresponding activity record
   * structure is \ref CUpti_ActivityContext.
   */
  CUPTI_ACTIVITY_KIND_CONTEXT  = 9,
  /**
   * A (potentially concurrent) kernel executing on the GPU. The
   * corresponding activity record structure is \ref
   * CUpti_ActivityKernel.
   */
  CUPTI_ACTIVITY_KIND_CONCURRENT_KERNEL = 10,
  /**
   * Thread, device, context, etc. name. The corresponding activity
   * record structure is \ref CUpti_ActivityName.
   */
  CUPTI_ACTIVITY_KIND_NAME     = 11,
  /**
   * Instantaneous, start, or end marker.
   */
  CUPTI_ACTIVITY_KIND_MARKER = 12,
  /**
   * Extended, optional, data about a marker.
   */
  CUPTI_ACTIVITY_KIND_MARKER_DATA = 13,
  /**
   * Source information about source level result. The corresponding
   * activity record structure is \ref CUpti_ActivitySourceLocator.
   */
  CUPTI_ACTIVITY_KIND_SOURCE_LOCATOR = 14,
  /**
   * Results for source-level global acccess. The
   * corresponding activity record structure is \ref
   * CUpti_ActivityGlobalAccess.
   */
  CUPTI_ACTIVITY_KIND_GLOBAL_ACCESS = 15,
  /**
   * Results for source-level branch. The corresponding
   * activity record structure is \ref CUpti_ActivityBranch.
   */
  CUPTI_ACTIVITY_KIND_BRANCH = 16,
  /**
   * Overhead activity records. The
   * corresponding activity record structure is
   * \ref CUpti_ActivityOverhead.
   */
  CUPTI_ACTIVITY_KIND_OVERHEAD = 17,

  CUPTI_ACTIVITY_KIND_FORCE_INT     = 0x7fffffff
} CUpti_ActivityKind;

/**
 * \brief The kinds of activity objects.
 * \see CUpti_ActivityObjectKindId
 */
typedef enum {
  /**
   * The object kind is not known.
   */
  CUPTI_ACTIVITY_OBJECT_UNKNOWN  = 0,
  /**
   * A process.
   */
  CUPTI_ACTIVITY_OBJECT_PROCESS  = 1,
  /**
   * A thread.
   */
  CUPTI_ACTIVITY_OBJECT_THREAD   = 2,
  /**
   * A device.
   */
  CUPTI_ACTIVITY_OBJECT_DEVICE   = 3,
  /**
   * A context.
   */
  CUPTI_ACTIVITY_OBJECT_CONTEXT  = 4,
  /**
   * A stream.
   */
  CUPTI_ACTIVITY_OBJECT_STREAM   = 5,

  CUPTI_ACTIVITY_OBJECT_FORCE_INT = 0x7fffffff
} CUpti_ActivityObjectKind;

/**
 * \brief Identifiers for object kinds as specified by
 * CUpti_ActivityObjectKind.
 * \see CUpti_ActivityObjectKind
 */
  typedef union {
    /**
     * A process object requires that we identify the process ID. A
     * thread object requires that we identify both the process and
     * thread ID.
     */
    struct {
      uint32_t processId;
      uint32_t threadId;
    } pt;
    /**
     * A device object requires that we identify the device ID. A
     * context object requires that we identify both the device and
     * context ID. A stream object requires that we identify device,
     * context, and stream ID.
     */
    struct {
      uint32_t deviceId;
      uint32_t contextId;
      uint32_t streamId;
    } dcs;
} CUpti_ActivityObjectKindId;

/**
 * \brief The kinds of activity overhead.
 */
typedef enum {
  /**
   * The overhead kind is not known.
   */
  CUPTI_ACTIVITY_OVERHEAD_UNKNOWN               = 0,
  /**
   * Compiler(JIT) overhead.
   */
  CUPTI_ACTIVITY_OVERHEAD_DRIVER_COMPILER       = 1,
  /**
   * Activity buffer flush overhead.
   */
  CUPTI_ACTIVITY_OVERHEAD_CUPTI_BUFFER_FLUSH    = 1<<16,
  /**
   * CUPTI instrumentation overhead.
   */
  CUPTI_ACTIVITY_OVERHEAD_CUPTI_INSTRUMENTATION = 2<<16,
  /**
   * CUPTI resource creation and destruction overhead.
   */
  CUPTI_ACTIVITY_OVERHEAD_CUPTI_RESOURCE        = 3<<16,
  CUPTI_ACTIVITY_OVERHEAD_FORCE_INT             = 0x7fffffff
} CUpti_ActivityOverheadKind;

/**
 * \brief The kind of a compute API.
 */
typedef enum {
  /**
   * The compute API is not known.
   */
  CUPTI_ACTIVITY_COMPUTE_API_UNKNOWN = 0,
  /**
   * The compute APIs are for CUDA.
   */
  CUPTI_ACTIVITY_COMPUTE_API_CUDA    = 1,

  CUPTI_ACTIVITY_COMPUTE_API_FORCE_INT = 0x7fffffff
} CUpti_ActivityComputeApiKind;

/**
 * \brief Flags associated with activity records.
 *
 * Activity record flags. Flags can be combined by bitwise OR to
 * associated multiple flags with an activity record. Each flag is
 * specific to a certain activity kind, as noted below.
 */
typedef enum {
  /**
   * Indicates the activity record has no flags.
   */
  CUPTI_ACTIVITY_FLAG_NONE          = 0,

  /**
   * Indicates the activity represents a device that supports
   * concurrent kernel execution. Valid for
   * CUPTI_ACTIVITY_KIND_DEVICE.
   */
  CUPTI_ACTIVITY_FLAG_DEVICE_CONCURRENT_KERNELS  = 1 << 0,

  /**
   * Indicates the activity represents an asychronous memcpy
   * operation. Valid for CUPTI_ACTIVITY_KIND_MEMCPY.
   */
  CUPTI_ACTIVITY_FLAG_MEMCPY_ASYNC  = 1 << 0,

  /**
   * Indicates the activity represents an instantaneous marker. Valid
   * for CUPTI_ACTIVITY_KIND_MARKER.
   */
  CUPTI_ACTIVITY_FLAG_MARKER_INSTANTANEOUS  = 1 << 0,

  /**
   * Indicates the activity represents a region start marker. Valid
   * for CUPTI_ACTIVITY_KIND_MARKER.
   */
  CUPTI_ACTIVITY_FLAG_MARKER_START  = 1 << 1,

  /**
   * Indicates the activity represents a region end marker. Valid for
   * CUPTI_ACTIVITY_KIND_MARKER.
   */
  CUPTI_ACTIVITY_FLAG_MARKER_END  = 1 << 2,

  /**
   * Indicates the activity represents a marker that does not specify
   * a color. Valid for CUPTI_ACTIVITY_KIND_MARKER_DATA.
   */
  CUPTI_ACTIVITY_FLAG_MARKER_COLOR_NONE  = 1 << 0,

  /**
   * Indicates the activity represents a marker that specifies a color
   * in alpha-red-green-blue format. Valid for
   * CUPTI_ACTIVITY_KIND_MARKER_DATA.
   */
  CUPTI_ACTIVITY_FLAG_MARKER_COLOR_ARGB  = 1 << 1,

  /**
   * The number of bytes requested by each thread 
   * Valid for CUpti_ActivityGlobalAccess.
   */
  CUPTI_ACTIVITY_FLAG_GLOBAL_ACCESS_KIND_SIZE_MASK  = 0xFF << 0,
  /**
   * If bit in this flag is set, the access was load, else it is a
   * store access.  Valid for CUpti_ActivityGlobalAccess.
   */
  CUPTI_ACTIVITY_FLAG_GLOBAL_ACCESS_KIND_LOAD       = 1 << 8,
  /**
   * If this bit in flag is set, the load access was cached else it is
   * uncached.  Valid for CUpti_ActivityGlobalAccess.
   */
  CUPTI_ACTIVITY_FLAG_GLOBAL_ACCESS_KIND_CACHED     = 1 << 9,
  CUPTI_ACTIVITY_FLAG_FORCE_INT = 0x7fffffff
} CUpti_ActivityFlag;

/**
 * \brief The kind of a memory copy, indicating the source and
 * destination targets of the copy.
 *
 * Each kind represents the source and destination targets of a memory
 * copy. Targets are host, device, and array.
 */
typedef enum {
  /**
   * The memory copy kind is not known.
   */
  CUPTI_ACTIVITY_MEMCPY_KIND_UNKNOWN = 0,
  /**
   * A host to device memory copy.
   */
  CUPTI_ACTIVITY_MEMCPY_KIND_HTOD    = 1,
  /**
   * A device to host memory copy.
   */
  CUPTI_ACTIVITY_MEMCPY_KIND_DTOH    = 2,
  /**
   * A host to device array memory copy.
   */
  CUPTI_ACTIVITY_MEMCPY_KIND_HTOA    = 3,
  /**
   * A device array to host memory copy.
   */
  CUPTI_ACTIVITY_MEMCPY_KIND_ATOH    = 4,
  /**
   * A device array to device array memory copy.
   */
  CUPTI_ACTIVITY_MEMCPY_KIND_ATOA    = 5,
  /**
   * A device array to device memory copy.
   */
  CUPTI_ACTIVITY_MEMCPY_KIND_ATOD    = 6,
  /**
   * A device to device array memory copy.
   */
  CUPTI_ACTIVITY_MEMCPY_KIND_DTOA    = 7,
  /**
   * A device to device memory copy.
   */
  CUPTI_ACTIVITY_MEMCPY_KIND_DTOD    = 8,
  /**
   * A host to host memory copy.
   */
  CUPTI_ACTIVITY_MEMCPY_KIND_HTOH    = 9,
  CUPTI_ACTIVITY_MEMCPY_KIND_FORCE_INT = 0x7fffffff
} CUpti_ActivityMemcpyKind;

/**
 * \brief The kinds of memory accessed by a memory copy.
 *
 * Each kind represents the type of the source or destination memory
 * accessed by a memory copy.
 */
typedef enum {
  /**
   * The source or destination memory kind is unknown.
   */
  CUPTI_ACTIVITY_MEMORY_KIND_UNKNOWN      = 0,
  /**
   * The source or destination memory is pageable.
   */
  CUPTI_ACTIVITY_MEMORY_KIND_PAGEABLE     = 1,
  /**
   * The source or destination memory is pinned.
   */
  CUPTI_ACTIVITY_MEMORY_KIND_PINNED       = 2,
  /**
   * The source or destination memory is on the device.
   */
  CUPTI_ACTIVITY_MEMORY_KIND_DEVICE       = 3,
  /**
   * The source or destination memory is an array.
   */
  CUPTI_ACTIVITY_MEMORY_KIND_ARRAY        = 4,
  CUPTI_ACTIVITY_MEMORY_KIND_FORCE_INT    = 0x7fffffff
} CUpti_ActivityMemoryKind;

/**
 * The source-locator ID that indicates an unknown source
 * location. There is not an actual CUpti_ActivitySourceLocator object
 * corresponding to this value.
 */
#define CUPTI_SOURCE_LOCATOR_ID_UNKNOWN 0


START_PACKED_ALIGNMENT
/**
 * \brief The base activity record.
 *
 * The activity API uses a CUpti_Activity as a generic representation
 * for any activity. The 'kind' field is used to determine the
 * specific activity kind, and from that the CUpti_Activity object can
 * be cast to the specific activity record type appropriate for that kind.
 *
 * Note that all activity record types are padded and aligned to
 * ensure that each member of the record is naturally aligned.
 *
 * \see CUpti_ActivityKind
 */
typedef struct PACKED_ALIGNMENT {
  /**
   * The kind of this activity.
   */
  CUpti_ActivityKind kind;
} CUpti_Activity;

/**
 * \brief The activity record for memory copies.
 *
 * This activity record represents a memory copy
 * (CUPTI_ACTIVITY_KIND_MEMCPY).
 */
typedef struct PACKED_ALIGNMENT {
  /**
   * The activity record kind, must be CUPTI_ACTIVITY_KIND_MEMCPY.
   */
  CUpti_ActivityKind kind;

  /**
   * The kind of the memory copy, stored as a byte to reduce record
   * size.  \see CUpti_ActivityMemcpyKind
   */
  /*CUpti_ActivityMemcpyKind*/ uint8_t copyKind;

  /**
   * The source memory kind read by the memory copy, stored as a byte
   * to reduce record size.  \see CUpti_ActivityMemoryKind
   */
  /*CUpti_ActivityMemoryKind*/ uint8_t srcKind;

  /**
   * The destination memory kind read by the memory copy, stored as a
   * byte to reduce record size.  \see CUpti_ActivityMemoryKind
   */
  /*CUpti_ActivityMemoryKind*/ uint8_t dstKind;

  /**
   * The flags associated with the memory copy. \see CUpti_ActivityFlag
   */
  uint8_t flags;

  /**
   * The number of bytes transferred by the memory copy.
   */
  uint64_t bytes;
  
  /**
   * The start timestamp for the memory copy, in ns.
   */
  uint64_t start;

  /**
   * The end timestamp for the memory copy, in ns.
   */
  uint64_t end;

  /**
   * The ID of the device where the memory copy is occurring.
   */
  uint32_t deviceId;

  /**
   * The ID of the context where the memory copy is occurring.
   */
  uint32_t contextId;

  /**
   * The ID of the stream where the memory copy is occurring.
   */
  uint32_t streamId;

  /**
   * The correlation ID of the memory copy. Each memory copy is
   * assigned a unique correlation ID that is identical to the
   * correlation ID in the driver API activity record that launched
   * the memory copy.
   */
  uint32_t correlationId;

  /**
   * The runtime correlation ID of the memory copy. Each memory copy
   * is assigned a unique runtime correlation ID that is identical to
   * the correlation ID in the runtime API activity record that
   * launched the memory copy.
   */
  uint32_t runtimeCorrelationId;

#ifdef CUPTILP64
  /**
   * Undefined. Reserved for internal use.
   */
  uint32_t pad;
#endif

  /**
   * Undefined. Reserved for internal use.
   */
  void *reserved0;
} CUpti_ActivityMemcpy;

/**
 * \brief The activity record for memset.
 *
 * This activity record represents a memory set operation
 * (CUPTI_ACTIVITY_KIND_MEMSET).
 */
typedef struct PACKED_ALIGNMENT {
  /**
   * The activity record kind, must be CUPTI_ACTIVITY_KIND_MEMSET.
   */
  CUpti_ActivityKind kind;
  
  /**
   * The value being assigned to memory by the memory set.
   */
  uint32_t value;

  /**
   * The number of bytes being set by the memory set.
   */
  uint64_t bytes;

  /**
   * The start timestamp for the memory set, in ns.
   */
  uint64_t start;

  /**
   * The end timestamp for the memory set, in ns.
   */
  uint64_t end;

  /**
   * The ID of the device where the memory set is occurring.
   */
  uint32_t deviceId;

  /**
   * The ID of the context where the memory set is occurring.
   */
  uint32_t contextId;

  /**
   * The ID of the stream where the memory set is occurring.
   */
  uint32_t streamId;

  /**
   * The correlation ID of the memory set. Each memory set is assigned
   * a unique correlation ID that is identical to the correlation ID
   * in the driver API activity record that launched the memory set.
   */
  uint32_t correlationId;

  /**
   * The runtime correlation ID of the memory set. Each memory set
   * is assigned a unique runtime correlation ID that is identical to
   * the correlation ID in the runtime API activity record that
   * launched the memory set.
   */
  uint32_t runtimeCorrelationId;

#ifdef CUPTILP64
  /**
   * Undefined. Reserved for internal use.
   */
  uint32_t pad;
#endif

  /**
   * Undefined. Reserved for internal use.
   */
  void *reserved0;
} CUpti_ActivityMemset;

/**
 * \brief The activity record for kernel.
 *
 * This activity record represents a kernel execution
 * (CUPTI_ACTIVITY_KIND_KERNEL and CUPTI_ACTIVITY_KIND_CONCURRENT_KERNEL).
 */
typedef struct PACKED_ALIGNMENT {
  /**
   * The activity record kind, must be CUPTI_ACTIVITY_KIND_KERNEL
   * or CUPTI_ACTIVITY_KIND_CONCURRENT_KERNEL.
   */
  CUpti_ActivityKind kind;

  /**
   * The cache configuration requested by the kernel. The value is one
   * of the CUfunc_cache enumeration values from cuda.h.
   */
  uint8_t cacheConfigRequested;

  /**
   * The cache configuration used for the kernel. The value is one of
   * the CUfunc_cache enumeration values from cuda.h.
   */
  uint8_t cacheConfigExecuted;
  
  /**
   * The number of registers required for each thread executing the
   * kernel.
   */
  uint16_t registersPerThread;

  /**
   * The start timestamp for the kernel execution, in ns.
   */
  uint64_t start;

  /**
   * The end timestamp for the kernel execution, in ns.
   */
  uint64_t end;

  /**
   * The ID of the device where the kernel is executing.
   */
  uint32_t deviceId;

  /**
   * The ID of the context where the kernel is executing.
   */
  uint32_t contextId;

  /**
   * The ID of the stream where the kernel is executing.
   */
  uint32_t streamId;

  /**
   * The X-dimension grid size for the kernel.
   */
  int32_t gridX;

  /**
   * The Y-dimension grid size for the kernel.
   */
  int32_t gridY;

  /**
   * The Z-dimension grid size for the kernel.
   */
  int32_t gridZ;

  /**
   * The X-dimension block size for the kernel.
   */
  int32_t blockX;

  /**
   * The Y-dimension block size for the kernel.
   */
  int32_t blockY;

  /**
   * The Z-dimension grid size for the kernel.
   */
  int32_t blockZ;
  
  /**
   * The static shared memory allocated for the kernel, in bytes.
   */
  int32_t staticSharedMemory;

  /**
   * The dynamic shared memory reserved for the kernel, in bytes.
   */
  int32_t dynamicSharedMemory;

  /**
   * The amount of local memory reserved for each thread, in bytes.
   */
  uint32_t localMemoryPerThread;
  
  /**
   * The total amount of local memory reserved for the kernel, in
   * bytes.
   */
  uint32_t localMemoryTotal;

  /**
   * The correlation ID of the kernel. Each kernel execution is
   * assigned a unique correlation ID that is identical to the
   * correlation ID in the driver API activity record that launched
   * the kernel.
   */
  uint32_t correlationId;

  /**
   * The runtime correlation ID of the kernel. Each kernel execution
   * is assigned a unique runtime correlation ID that is identical to
   * the correlation ID in the runtime API activity record that
   * launched the kernel.
   */
  uint32_t runtimeCorrelationId;

  /**
   * Undefined. Reserved for internal use.
   */
  uint32_t pad;

  /**
   * The name of the kernel. This name is shared across all activity
   * records representing the same kernel, and so should not be
   * modified.
   */
  const char *name;

  /**
   * Undefined. Reserved for internal use.
   */
  void *reserved0;
} CUpti_ActivityKernel;

/**
 * \brief The activity record for a driver or runtime API invocation.
 *
 * This activity record represents an invocation of a driver or
 * runtime API (CUPTI_ACTIVITY_KIND_DRIVER and
 * CUPTI_ACTIVITY_KIND_RUNTIME).
 */
typedef struct PACKED_ALIGNMENT {
  /**
   * The activity record kind, must be CUPTI_ACTIVITY_KIND_DRIVER or
   * CUPTI_ACTIVITY_KIND_RUNTIME.
   */
  CUpti_ActivityKind kind;

  /**
   * The ID of the driver or runtime function.
   */
  CUpti_CallbackId cbid;

  /**
   * The start timestamp for the function, in ns.
   */
  uint64_t start;

  /**
   * The end timestamp for the function, in ns.
   */
  uint64_t end;

  /**
   * The ID of the process where the driver or runtime CUDA function
   * is executing.
   */
  uint32_t processId;

  /**
   * The ID of the thread where the driver or runtime CUDA function is
   * executing.
   */
  uint32_t threadId;

  /**
   * The correlation ID of the driver or runtime CUDA function. Each
   * function invocation is assigned a unique correlation ID that is
   * identical to the correlation ID in the memcpy, memset, or kernel
   * activity record that is associated with this function.
   */
  uint32_t correlationId;

  /**
   * The return value for the function. For a CUDA driver function
   * with will be a CUresult value, and for a CUDA runtime function
   * this will be a cudaError_t value.
   */
  uint32_t returnValue;
} CUpti_ActivityAPI;

/**
 * \brief The activity record for a CUPTI event.
 *
 * This activity record represents the collection of a CUPTI event
 * value (CUPTI_ACTIVITY_KIND_EVENT). This activity record kind is not
 * produced by the activity API but is included for completeness and
 * ease-of-use. Profile frameworks built on top of CUPTI that collect
 * event data may choose to use this type to store the collected event
 * data.
 */
typedef struct PACKED_ALIGNMENT {
  /**
   * The activity record kind, must be CUPTI_ACTIVITY_KIND_EVENT.
   */
  CUpti_ActivityKind kind;
  
  /**
   * The event ID.
   */
  CUpti_EventID id;

  /**
   * The event value.
   */
  uint64_t value;

  /**
   * The event domain ID.
   */
  CUpti_EventDomainID domain;

  /**
   * The correlation ID of the event. Use of this ID is user-defined,
   * but typically this ID value will equal the correlation ID of the
   * kernel for which the event was gathered.
   */
  uint32_t correlationId;
} CUpti_ActivityEvent;

/**
 * \brief The activity record for a CUPTI metric.
 *
 * This activity record represents the collection of a CUPTI metric
 * value (CUPTI_ACTIVITY_KIND_METRIC). This activity record kind is not
 * produced by the activity API but is included for completeness and
 * ease-of-use. Profile frameworks built on top of CUPTI that collect
 * metric data may choose to use this type to store the collected metric
 * data.
 */
typedef struct PACKED_ALIGNMENT {
  /**
   * The activity record kind, must be CUPTI_ACTIVITY_KIND_METRIC.
   */
  CUpti_ActivityKind kind;

  /**
   * The metric ID.
   */
  CUpti_MetricID id;
  
  /**
   * The metric value.
   */
  CUpti_MetricValue value;

  /**
   * The correlation ID of the metric. Use of this ID is user-defined,
   * but typically this ID value will equal the correlation ID of the
   * kernel for which the metric was gathered.
   */
  uint32_t correlationId;

  /**
   * Undefined. Reserved for internal use.
   */
  uint32_t pad;
} CUpti_ActivityMetric;

/**
 * \brief The activity record for source locator.
 *
 * This activity record represents a source locator
 * (CUPTI_ACTIVITY_KIND_SOURCE_LOCATOR).
 */
typedef struct PACKED_ALIGNMENT {
  /**
   * The activity record kind, must be CUPTI_ACTIVITY_KIND_SOURCE_LOCATOR.
   */
  CUpti_ActivityKind kind;

  /**
   * The ID for the source path, will be used in all the source level
   * results.
   */
  uint32_t id;

  /**
   * The line number in the source .
   */
  uint32_t lineNumber;

#ifdef CUPTILP64
  /**
   * Undefined. Reserved for internal use.
   */
  uint32_t pad;
#endif

  /**
   * The path for the file.
   */
  const char *fileName;
} CUpti_ActivitySourceLocator;

/**
 * \brief The activity record for source-level global
 * access.
 *
 * This activity records the locations of the global
 * accesses in the source
 * (CUPTI_ACTIVITY_KIND_GLOBAL_ACCESS).
 */
typedef struct PACKED_ALIGNMENT {
  /**
   * The activity record kind, must be CUPTI_ACTIVITY_KIND_GLOBAL_ACCESS.
   */
  CUpti_ActivityKind kind;

  /**
   * The properties of this global access.
   */
  CUpti_ActivityFlag flags;

  /**
   * The ID for source locator.
   */
  uint32_t sourceLocatorId;

  /**
   * The correlation ID of the kernel to which this result is associated.
   */
  uint32_t correlationId;

  /**
   * The pc offset for the access.
   */
  uint32_t pcOffset;

  /**
   * The number of times this instruction was executed
   */
  uint32_t executed;

  /**
   * This increments each time when this instruction is executed by number
   * of threads that executed this instruction
   */
  uint64_t threadsExecuted;

  /**
   * The total number of 32 bytes transactions to L2 cache generated by this access
   */
  uint64_t l2_transactions;
} CUpti_ActivityGlobalAccess;

/**
 * \brief The activity record for source level result
 * branch.
 *
 * This activity record the locations of the branches in the
 * source (CUPTI_ACTIVITY_KIND_BRANCH).
 */
typedef struct PACKED_ALIGNMENT {
  /**
   * The activity record kind, must be CUPTI_ACTIVITY_KIND_BRANCH.
   */
  CUpti_ActivityKind kind;

  /**
   * The ID for source locator.
   */
  uint32_t sourceLocatorId;

  /**
   * The correlation ID of the kernel to which this result is associated.
   */
  uint32_t correlationId;

  /**
   * The pc offset for the branch.
   */
  uint32_t pcOffset;

  /**
   * The number of times this branch was executed
   */
  uint32_t executed;

  /**
   * Number of times this branch diverged
   */
  uint32_t diverged;

  /**
   * This increments each time when this instruction is executed by number
   * of threads that executed this instruction
   */
  uint64_t threadsExecuted;
} CUpti_ActivityBranch;

/**
 * \brief The activity record for a device.
 *
 * This activity record represents information about a GPU device
 * (CUPTI_ACTIVITY_KIND_DEVICE).
 */
typedef struct PACKED_ALIGNMENT {
  /**
   * The activity record kind, must be CUPTI_ACTIVITY_KIND_DEVICE.
   */
  CUpti_ActivityKind kind;

  /**
   * The flags associated with the device. \see CUpti_ActivityFlag
   */
  CUpti_ActivityFlag flags;
  
  /**
   * The global memory bandwidth available on the device, in
   * kBytes/sec.
   */
  uint64_t globalMemoryBandwidth;

  /**
   * The amount of global memory on the device, in bytes.
   */
  uint64_t globalMemorySize;

  /**
   * The amount of constant memory on the device, in bytes.
   */
  uint32_t constantMemorySize; 
  
  /**
   * The size of the L2 cache on the device, in bytes.
   */
  uint32_t l2CacheSize;

  /**
   * The number of threads per warp on the device.
   */
  uint32_t numThreadsPerWarp;

  /**
   * The core clock rate of the device, in kHz.
   */
  uint32_t coreClockRate;

  /**
   * Number of memory copy engines on the device.
   */
  uint32_t numMemcpyEngines;
  
  /**
   * Number of multiprocessors on the device.
   */
  uint32_t numMultiprocessors;

  /**
   * The maximum "instructions per cycle" possible on each device
   * multiprocessor.
   */
  uint32_t maxIPC;

  /**
   * Maximum number of warps that can be present on a multiprocessor
   * at any given time. 
   */
  uint32_t maxWarpsPerMultiprocessor;

  /**
   * Maximum number of blocks that can be present on a multiprocessor
   * at any given time.
   */
  uint32_t maxBlocksPerMultiprocessor;

  /**
   * Maximum number of registers that can be allocated to a block.
   */
  uint32_t maxRegistersPerBlock;

  /**
   * Maximum amount of shared memory that can be assigned to a block,
   * in bytes.
   */
  uint32_t maxSharedMemoryPerBlock;

  /**
   * Maximum number of threads allowed in a block.
   */
  uint32_t maxThreadsPerBlock;
  
  /**
   * Maximum allowed X dimension for a block.
   */
  uint32_t maxBlockDimX;
  
  /**
   * Maximum allowed Y dimension for a block.
   */
  uint32_t maxBlockDimY;
  
  /**
   * Maximum allowed Z dimension for a block.
   */
  uint32_t maxBlockDimZ;

  /**
   * Maximum allowed X dimension for a grid.
   */
  uint32_t maxGridDimX;

  /**
   * Maximum allowed Y dimension for a grid.
   */
  uint32_t maxGridDimY;

  /**
   * Maximum allowed Z dimension for a grid.
   */
  uint32_t maxGridDimZ;

  /**
   * Compute capability for the device, major number.
   */
  uint32_t computeCapabilityMajor;

  /**
   * Compute capability for the device, minor number.
   */
  uint32_t computeCapabilityMinor;

  /**
   * The device ID.
   */
  uint32_t id;
  
#ifdef CUPTILP64
  /**
   * Undefined. Reserved for internal use.
   */
  uint32_t pad;
#endif

  /**
   * The device name. This name is shared across all activity records
   * representing instances of the device, and so should not be
   * modified.
   */
  const char *name;
} CUpti_ActivityDevice;

/**
 * \brief The activity record for a context.
 *
 * This activity record represents information about a context
 * (CUPTI_ACTIVITY_KIND_CONTEXT).
 */
typedef struct PACKED_ALIGNMENT {
  /**
   * The activity record kind, must be CUPTI_ACTIVITY_KIND_CONTEXT.
   */
  CUpti_ActivityKind kind;

  /**
   * The context ID.
   */
  uint32_t contextId;

  /**
   * The device ID.
   */
  uint32_t deviceId;

  /**
   * The compute API kind. \see CUpti_ActivityComputeApiKind
   */
  CUpti_ActivityComputeApiKind computeApiKind;
} CUpti_ActivityContext;

/**
 * \brief The activity record providing a name.
 *
 * This activity record provides a name for a device, context, thread,
 * etc.  (CUPTI_ACTIVITY_KIND_NAME).
 */
typedef struct PACKED_ALIGNMENT {
  /**
   * The activity record kind, must be CUPTI_ACTIVITY_KIND_NAME.
   */
  CUpti_ActivityKind kind;

  /**
   * The kind of activity object being named.
   */
  CUpti_ActivityObjectKind objectKind;

  /**
   * The identifier for the activity object. 'objectKind' indicates
   * which ID is valid for this record.
   */
  CUpti_ActivityObjectKindId objectId;

#ifdef CUPTILP64
  /**
   * Undefined. Reserved for internal use.
   */
  uint32_t pad;
#endif

  /**
   * The name.
   */
  const char *name;

} CUpti_ActivityName;

/**
 * \brief The activity record providing a marker which is an
 * instantaneous point in time.
 *
 * The marker is specified with a descriptive name and unique id
 * (CUPTI_ACTIVITY_KIND_MARKER).
 */
typedef struct PACKED_ALIGNMENT {
  /**
   * The activity record kind, must be CUPTI_ACTIVITY_KIND_MARKER.
   */
  CUpti_ActivityKind kind;

  /**
   * The flags associated with the marker. \see CUpti_ActivityFlag
   */
  CUpti_ActivityFlag flags;

  /**
   * The timestamp for the marker, in ns.
   */
  uint64_t timestamp;

  /**
   * The marker ID.
   */
  uint32_t id;
  
  /**
   * The kind of activity object associated with this marker.
   */
  CUpti_ActivityObjectKind objectKind;

  /**
   * The identifier for the activity object associated with this
   * marker. 'objectKind' indicates which ID is valid for this record.
   */
  CUpti_ActivityObjectKindId objectId;

#ifdef CUPTILP64
  /**
   * Undefined. Reserved for internal use.
   */
  uint32_t pad;
#endif

  /**
   * The marker name for an instantaneous or start marker. This will
   * be NULL for an end marker.
   */
  const char *name;

} CUpti_ActivityMarker;

/**
 * \brief The activity record providing detailed information for a marker.
 *
 * The marker data contains color, payload, and category.
 * (CUPTI_ACTIVITY_KIND_MARKER_DATA).
 */
typedef struct PACKED_ALIGNMENT {
  /**
   * The activity record kind, must be
   * CUPTI_ACTIVITY_KIND_MARKER_DATA.
   */
  CUpti_ActivityKind kind;

  /**
   * The flags associated with the marker. \see CUpti_ActivityFlag
   */
  CUpti_ActivityFlag flags;

  /**
   * The marker ID.
   */
  uint32_t id;
  
  /**
   * Defines the payload format for the value associated with the marker.
   */
  CUpti_MetricValueKind payloadKind;
  
  /**
   * The payload value.
   */
  CUpti_MetricValue payload;
  
  /**
   * The color for the marker.
   */
  uint32_t color;

  /**
   * The category for the marker.
   */
  uint32_t category;

} CUpti_ActivityMarkerData;

/**
 * \brief The activity record for CUPTI and driver overheads.
 *
 * This activity record provides CUPTI and driver overhead information 
 * (CUPTI_ACTIVITY_OVERHEAD).
 */
typedef struct PACKED_ALIGNMENT {
  /**
   * The activity record kind, must be CUPTI_ACTIVITY_OVERHEAD.
   */
  CUpti_ActivityKind kind;

  /**
   * The kind of overhead, CUPTI, DRIVER, COMPILER etc.
   */
  CUpti_ActivityOverheadKind overheadKind;

  /**
   * The kind of activity object that the overhead is associated with.
   */
  CUpti_ActivityObjectKind objectKind;

  /**
   * The identifier for the activity object. 'objectKind' indicates
   * which ID is valid for this record.
   */
  CUpti_ActivityObjectKindId objectId;

  /**
   * The start timestamp for the overhead, in ns.
   */
  uint64_t start;

  /**
   * The end timestamp for the overhead, in ns.
   */
  uint64_t end;
} CUpti_ActivityOverhead;

END_PACKED_ALIGNMENT

/**
 * \brief Get the CUPTI timestamp.
 *
 * Returns a timestamp normalized to correspond with the start and end
 * timestamps reported in the CUPTI activity records. The timestamp is
 * reported in nanoseconds.
 *
 * \param timestamp Returns the CUPTI timestamp
 *
 * \retval CUPTI_SUCCESS
 * \retval CUPTI_ERROR_INVALID_PARAMETER if \p timestamp is NULL
 */
CUptiResult CUPTIAPI cuptiGetTimestamp(uint64_t *timestamp);

/**
 * \brief Get the ID of a stream.
 *
 * Get the ID of a stream. The stream ID is unique within a context
 * (i.e. all streams within a context will have unique stream
 * IDs).
 *
 * \param context If non-NULL then the stream is checked to ensure
 * that it belongs to this context. Typically this parameter should be
 * null.
 * \param stream The stream
 * \param streamId Returns a context-unique ID for the stream
 *
 * \retval CUPTI_SUCCESS
 * \retval CUPTI_ERROR_NOT_INITIALIZED
 * \retval CUPTI_ERROR_INVALID_STREAM if unable to get stream ID, or
 * if \p context is non-NULL and \p stream does not belong to the
 * context
 * \retval CUPTI_ERROR_INVALID_PARAMETER if \p streamId is NULL 
 *
 * \see cuptiActivityEnqueueBuffer
 * \see cuptiActivityDequeueBuffer
 */
CUptiResult CUPTIAPI cuptiGetStreamId(CUcontext context, CUstream stream, uint32_t *streamId);

/**
 * \brief Get the ID of a device
 *
 * If \p context is NULL, returns the ID of the device that contains
 * the currently active context. If \p context is non-NULL, returns
 * the ID of the device which contains that context. Operates in a
 * similar manner to cudaGetDevice() or cuCtxGetDevice() but may be
 * called from within callback functions.
 *
 * \param context The context, or NULL to indicate the current context.
 * \param deviceId Returns the ID of the device that is current for
 * the calling thread.
 *
 * \retval CUPTI_SUCCESS
 * \retval CUPTI_ERROR_NOT_INITIALIZED
 * \retval CUPTI_ERROR_INVALID_DEVICE if unable to get device ID
 * \retval CUPTI_ERROR_INVALID_PARAMETER if \p deviceId is NULL 
 */
CUptiResult CUPTIAPI cuptiGetDeviceId(CUcontext context, uint32_t *deviceId);

/**
 * \brief Enable collection of a specific kind of activity record.
 *
 * Enable collection of a specific kind of activity record. Multiple
 * kinds can be enabled by calling this function multiple times. By
 * default all activity kinds are disabled for collection.
 *
 * \param kind The kind of activity record to collect
 *
 * \retval CUPTI_SUCCESS
 * \retval CUPTI_ERROR_NOT_INITIALIZED
 * \retval CUPTI_ERROR_NOT_COMPATIBLE if the activity kind cannot be enabled
 */
CUptiResult CUPTIAPI cuptiActivityEnable(CUpti_ActivityKind kind); 

/**
 * \brief Disable collection of a specific kind of activity record.
 *
 * Disable collection of a specific kind of activity record. Multiple
 * kinds can be disabled by calling this function multiple times. By
 * default all activity kinds are disabled for collection.
 *
 * \param kind The kind of activity record to stop collecting
 *
 * \retval CUPTI_SUCCESS
 * \retval CUPTI_ERROR_NOT_INITIALIZED
 */
CUptiResult CUPTIAPI cuptiActivityDisable(CUpti_ActivityKind kind); 

/**
 * \brief Enable collection of a specific kind of activity record for
 * a context.
 *
 * Enable collection of a specific kind of activity record for a context.
 * This setting done by this API will supercede the global settings
 * for activity records enabled by \ref cuptiActivityEnable 
 * Multiple kinds can be enabled by calling this function multiple times.
 *
 * \param context The context for which activity is to be enabled
 * \param kind The kind of activity record to collect
 *
 * \retval CUPTI_SUCCESS
 * \retval CUPTI_ERROR_NOT_INITIALIZED
 * \retval CUPTI_ERROR_NOT_COMPATIBLE if the activity kind cannot be enabled
 */
CUptiResult CUPTIAPI cuptiActivityEnableContext(CUcontext context, CUpti_ActivityKind kind); 

/**
 * \brief Disable collection of a specific kind of activity record for
 * a context.
 *
 * Disable collection of a specific kind of activity record for a context.
 * This setting done by this API will supercede the global settings
 * for activity records.
 * Multiple kinds can be enabled by calling this function multiple times.
 *
 * \param context The context for which activity is to be disabled
 * \param kind The kind of activity record to stop collecting
 *
 * \retval CUPTI_SUCCESS
 * \retval CUPTI_ERROR_NOT_INITIALIZED
 */
CUptiResult CUPTIAPI cuptiActivityDisableContext(CUcontext context, CUpti_ActivityKind kind); 

/**
 * \brief Queue a buffer for activity record collection.
 *
 * Queue a buffer for activity record collection. Calling this
 * function transfers ownership of the buffer to CUPTI. The buffer
 * should not be accessed or modified until ownership is regained by
 * calling cuptiActivityDequeueBuffer().
 *
 * There are three types of queues:
 *
 * Global Queue: The global queue collects all activity records that
 * are not associated with a valid context. All device and API
 * activity records are collected in the global queue. A buffer is
 * enqueued in the global queue by specifying \p context == NULL.
 *
 * Context Queue: Each context queue collects activity records
 * associated with that context that are not associated with a
 * specific stream or that are associated with the default stream. A
 * buffer is enqueued in a context queue by specifying the context and
 * a \p streamId of 0.
 *
 * Stream Queue: Each stream queue collects memcpy, memset, and kernel
 * activity records associated with the stream. A buffer is enqueued
 * in a stream queue by specifying a context and a non-zero stream ID.
 *
 * Multiple buffers can be enqueued on each queue, and buffers can be
 * enqueue on multiple queues.
 *
 * When a new activity record needs to be recorded, CUPTI searches for
 * a non-empty queue to hold the record in this order: 1) the
 * appropriate stream queue, 2) the appropriate context queue.
 * If the search does not find any queue with a buffer then
 * the activity record is dropped. If the search finds a
 * queue containing a buffer, but that buffer is full, then the
 * activity record is dropped and the dropped record count for the
 * queue is incremented. If the search finds a queue containing a
 * buffer with space available to hold the record, then the record is
 * recorded in the buffer.
 *
 * At a minimum, one or more buffers must be queued in the global
 * queue and context queue at all times to avoid dropping activity
 * records. Global queue will not store any activity records for gpu
 * activity(kernel, memcpy, memset).
 * It is also necessary to enqueue at least one buffer in
 * the context queue of each context as it is created. The stream
 * queues are optional and can be used to reduce or eliminate
 * application perturbations caused by the need to process or save the
 * activity records returned in the buffers. For example, if a stream
 * queue is used, that queue can be flushed when the stream is
 * synchronized.
 *
 * \param context The context, or NULL to enqueue on the global queue
 * \param streamId The stream ID
 * \param buffer The pointer to user supplied buffer for storing activity
 * records.The buffer must be at least 8 byte aligned, and the size of the
 * buffer must be at least 1024 bytes.
 * \param bufferSizeBytes The size of the buffer, in bytes. The size of
 * the buffer must be at least 1024 bytes.
 *
 * \retval CUPTI_SUCCESS
 * \retval CUPTI_ERROR_NOT_INITIALIZED
 * \retval CUPTI_ERROR_INVALID_PARAMETER if \p buffer is NULL, does not have
 *alignment of at least 8 bytes, or is not at least 1024 bytes in size
 */
CUptiResult CUPTIAPI cuptiActivityEnqueueBuffer(CUcontext context, uint32_t streamId,
                                                uint8_t *buffer, size_t bufferSizeBytes); 

/**
 * \brief Dequeue a buffer containing activity records.
 *
 * Remove the buffer from the head of the specified queue. See
 * cuptiActivityEnqueueBuffer() for description of queues. Calling
 * this function transfers ownership of the buffer from CUPTI. CUPTI
 * will no add any activity records to the buffer after it is
 * dequeued.
 * 
 * \param context The context, or NULL to dequeue from the global queue
 * \param streamId The stream ID
 * \param buffer Returns the dequeued buffer
 * \param validBufferSizeBytes Returns the number of bytes in the
 * buffer that contain activity records
 *
 * \retval CUPTI_SUCCESS
 * \retval CUPTI_ERROR_NOT_INITIALIZED
 * \retval CUPTI_ERROR_INVALID_PARAMETER if \p buffer or \p validBufferSizeBytes are NULL
 * \retval CUPTI_ERROR_QUEUE_EMPTY the queue is empty, \p buffer
 * returns NULL and \p validBufferSizeBytes returns 0
 */
CUptiResult CUPTIAPI cuptiActivityDequeueBuffer(CUcontext context, uint32_t streamId, 
                                                uint8_t **buffer, size_t *validBufferSizeBytes); 

/**
 * \brief Query the status of the buffer at the head of a queue.
 *
 * Query the status of buffer at the head in the queue.  See
 * cuptiActivityEnqueueBuffer() for description of queues. Calling
 * this function does not transfer ownership of the buffer.
 *
 * \param context The context, or NULL to query the global queue
 * \param streamId The stream ID
 * \param validBufferSizeBytes Returns the number of bytes in the
 * buffer that contain activity records
 *
 * \retval CUPTI_SUCCESS
 * \retval CUPTI_ERROR_NOT_INITIALIZED
 * \retval CUPTI_ERROR_INVALID_PARAMETER if \p buffer or \p validBufferSizeBytes are NULL
 * \retval CUPTI_ERROR_MAX_LIMIT_REACHED if buffer is full
 * \retval CUPTI_ERROR_QUEUE_EMPTY the queue is empty, \p
 * validBufferSizeBytes returns 0
 */
CUptiResult CUPTIAPI cuptiActivityQueryBuffer(CUcontext context, uint32_t streamId,
                                              size_t *validBufferSizeBytes); 

/**
 * \brief Get the number of activity records that were dropped from a
 * queue because of insufficient buffer space.
 *
 * Get the number of records that were dropped from a queue because
 * all the buffers in the queue are full.  See
 * cuptiActivityEnqueueBuffer() for description of queues. Calling
 * this function does not transfer ownership of the buffer. The
 * dropped count maintained for the queue is reset to zero when this
 * function is called.
 *
 * \param context The context, or NULL to get dropped count from global queue
 * \param streamId The stream ID
 * \param dropped The number of records that were dropped since the last call 
 * to this function.
 *
 * \retval CUPTI_SUCCESS
 * \retval CUPTI_ERROR_NOT_INITIALIZED
 * \retval CUPTI_ERROR_INVALID_PARAMETER if \p dropped is NULL
 */
CUptiResult CUPTIAPI cuptiActivityGetNumDroppedRecords(CUcontext context, uint32_t streamId,
                                                       size_t *dropped);

/**
 * \brief Iterate over the activity records in a buffer. 
 *
 * This is a helper function to iterate over the activity records in a
 * buffer. A buffer of activity records is typically obtained by
 * using the cuptiActivityDequeueBuffer() function.
 *
 * An example of typical usage:
 * \code
 * CUpti_Activity *record = NULL;
 * CUptiResult status = CUPTI_SUCCESS;
 *   do {
 *      status = cuptiActivityGetNextRecord(buffer, validSize, &record);
 *      if(status == CUPTI_SUCCESS) {
 *           // Use record here...
 *      }
 *      else if (status == CUPTI_ERROR_MAX_LIMIT_REACHED)
 *          break;
 *      else {
 *          goto Error;
 *      }
 *    } while (1);
 * \endcode
 *
 * \param buffer The buffer containing activity records
 * \param record Inputs the previous record returned by
 * cuptiActivityGetNextRecord and returns the next activity record
 * from the buffer. If input value if NULL, returns the first activity
 * record in the buffer.
 * \param validBufferSizeBytes The number of valid bytes in the buffer.
 *
 * \retval CUPTI_SUCCESS
 * \retval CUPTI_ERROR_NOT_INITIALIZED
 * \retval CUPTI_ERROR_MAX_LIMIT_REACHED if no more records in the buffer
 * \retval CUPTI_ERROR_INVALID_PARAMETER if \p buffer is NULL.
 */
CUptiResult CUPTIAPI cuptiActivityGetNextRecord(uint8_t* buffer, size_t validBufferSizeBytes,
                                                CUpti_Activity **record); 

/** @} */ /* END CUPTI_ACTIVITY_API */

#if defined(__cplusplus)
}
#endif 

#endif /*_CUPTI_ACTIVITY_H_*/
