/*
 * Copyright 2011-2012 NVIDIA Corporation. All rights reserved
 *
 * Sample CUPTI app to print a trace of CUDA API and GPU activity
 * 
 */

#include <stdio.h>
#include <cuda.h>
#include <cupti.h>

#define CUPTI_CALL(call)                                                \
  do {                                                                  \
    CUptiResult _status = call;                                         \
    if (_status != CUPTI_SUCCESS) {                                     \
      const char *errstr;                                               \
      cuptiGetResultString(_status, &errstr);                           \
      fprintf(stderr, "%s:%d: error: function %s failed with error %s.\n", \
              __FILE__, __LINE__, #call, errstr);                       \
      exit(-1);                                                         \
    }                                                                   \
  } while (0)

#define BUF_SIZE (32 * 1024)
#define ALIGN_SIZE (8)
#define ALIGN_BUFFER(buffer, align)                                            \
  (((uintptr_t) (buffer) & ((align)-1)) ? ((buffer) + (align) - ((uintptr_t) (buffer) & ((align)-1))) : (buffer)) 

// Timestamp at trace initialization time. Used to normalized other
// timestamps
static uint64_t startTimestamp;

static const char *
getMemcpyKindString(CUpti_ActivityMemcpyKind kind)
{
  switch (kind) {
  case CUPTI_ACTIVITY_MEMCPY_KIND_HTOD:
    return "HtoD";
  case CUPTI_ACTIVITY_MEMCPY_KIND_DTOH:
    return "DtoH";
  case CUPTI_ACTIVITY_MEMCPY_KIND_HTOA:
    return "HtoA";
  case CUPTI_ACTIVITY_MEMCPY_KIND_ATOH:
    return "AtoH";
  case CUPTI_ACTIVITY_MEMCPY_KIND_ATOA:
    return "AtoA";
  case CUPTI_ACTIVITY_MEMCPY_KIND_ATOD:
    return "AtoD";
  case CUPTI_ACTIVITY_MEMCPY_KIND_DTOA:
    return "DtoA";
  case CUPTI_ACTIVITY_MEMCPY_KIND_DTOD:
    return "DtoD";
  case CUPTI_ACTIVITY_MEMCPY_KIND_HTOH:
    return "HtoH";
  default:
    break;
  }

  return "<unknown>";
}

const char *
getActivityOverheadKindString(CUpti_ActivityOverheadKind kind)
{
  switch (kind) {
  case CUPTI_ACTIVITY_OVERHEAD_DRIVER_COMPILER:
    return "COMPILER";
  case CUPTI_ACTIVITY_OVERHEAD_CUPTI_BUFFER_FLUSH:
    return "BUFFER_FLUSH";
  case CUPTI_ACTIVITY_OVERHEAD_CUPTI_INSTRUMENTATION:
    return "INSTRUMENTATION";
  case CUPTI_ACTIVITY_OVERHEAD_CUPTI_RESOURCE:
    return "RESOURCE";
  default:
    break;
  }
  
  return "<unknown>";
}

const char *
getActivityObjectKindString(CUpti_ActivityObjectKind kind)
{
  switch (kind) {
  case CUPTI_ACTIVITY_OBJECT_PROCESS:
    return "PROCESS";
  case CUPTI_ACTIVITY_OBJECT_THREAD:
    return "THREAD";
  case CUPTI_ACTIVITY_OBJECT_DEVICE:
    return "DEVICE";
  case CUPTI_ACTIVITY_OBJECT_CONTEXT:
    return "CONTEXT";
  case CUPTI_ACTIVITY_OBJECT_STREAM:
    return "STREAM";
  default:
    break;
  }
  
  return "<unknown>";
}

uint32_t
getActivityObjectKindId(CUpti_ActivityObjectKind kind, CUpti_ActivityObjectKindId *id)
{
  switch (kind) {
  case CUPTI_ACTIVITY_OBJECT_PROCESS:
    return id->pt.processId;
  case CUPTI_ACTIVITY_OBJECT_THREAD:
    return id->pt.threadId;
  case CUPTI_ACTIVITY_OBJECT_DEVICE:
    return id->dcs.deviceId;
  case CUPTI_ACTIVITY_OBJECT_CONTEXT:
    return id->dcs.contextId;
  case CUPTI_ACTIVITY_OBJECT_STREAM:
    return id->dcs.streamId;
  default:
    break;
  }

  return 0xffffffff;
}

static void
printActivity(CUpti_Activity *record)
{
  switch (record->kind) {
  case CUPTI_ACTIVITY_KIND_DEVICE:
    {
      CUpti_ActivityDevice *device = (CUpti_ActivityDevice *)record;
      printf("DEVICE %s (%u), capability %u.%u, global memory (bandwidth %u GB/s, size %u MB), "
             "multiprocessors %u, clock %u MHz\n",
             device->name, device->id, 
             device->computeCapabilityMajor, device->computeCapabilityMinor,
             (unsigned int)(device->globalMemoryBandwidth / 1024 / 1024),
             (unsigned int)(device->globalMemorySize / 1024 / 1024),
             device->numMultiprocessors, (unsigned int)(device->coreClockRate / 1000));
      break;
    }
  case CUPTI_ACTIVITY_KIND_CONTEXT:
    {
      CUpti_ActivityContext *context = (CUpti_ActivityContext *)record;
      printf("CONTEXT %u, device %u, compute API %s\n",
             context->contextId, context->deviceId, 
             (context->computeApiKind == CUPTI_ACTIVITY_COMPUTE_API_CUDA) ? "CUDA" : "unknown");
      break;
    }
  case CUPTI_ACTIVITY_KIND_MEMCPY:
    {
      CUpti_ActivityMemcpy *memcpy = (CUpti_ActivityMemcpy *)record;
      printf("MEMCPY %s [ %llu - %llu ] device %u, context %u, stream %u, correlation %u/r%u\n",
             getMemcpyKindString((CUpti_ActivityMemcpyKind)memcpy->copyKind),
             (unsigned long long)(memcpy->start - startTimestamp),
             (unsigned long long)(memcpy->end - startTimestamp),
             memcpy->deviceId, memcpy->contextId, memcpy->streamId, 
             memcpy->correlationId, memcpy->runtimeCorrelationId);
      break;
    }
  case CUPTI_ACTIVITY_KIND_MEMSET:
    {
      CUpti_ActivityMemset *memset = (CUpti_ActivityMemset *)record;
      printf("MEMSET value=%u [ %llu - %llu ] device %u, context %u, stream %u, correlation %u/r%u\n",
             memset->value,
             (unsigned long long)(memset->start - startTimestamp),
             (unsigned long long)(memset->end - startTimestamp),
             memset->deviceId, memset->contextId, memset->streamId, 
             memset->correlationId, memset->runtimeCorrelationId);
      break;
    }
  case CUPTI_ACTIVITY_KIND_KERNEL:
  case CUPTI_ACTIVITY_KIND_CONCURRENT_KERNEL:
    {
      const char* kindString = (record->kind == CUPTI_ACTIVITY_KIND_KERNEL) ? "KERNEL" : "CONC KERNEL";
      CUpti_ActivityKernel *kernel = (CUpti_ActivityKernel *)record;
      printf("%s \"%s\" [ %llu - %llu ] device %u, context %u, stream %u, correlation %u/r%u\n",
             kindString,
             kernel->name,
             (unsigned long long)(kernel->start - startTimestamp),
             (unsigned long long)(kernel->end - startTimestamp),
             kernel->deviceId, kernel->contextId, kernel->streamId, 
             kernel->correlationId, kernel->runtimeCorrelationId);
      printf("    grid [%u,%u,%u], block [%u,%u,%u], shared memory (static %u, dynamic %u)\n",
             kernel->gridX, kernel->gridY, kernel->gridZ,
             kernel->blockX, kernel->blockY, kernel->blockZ,
             kernel->staticSharedMemory, kernel->dynamicSharedMemory);
      break;
    }
  case CUPTI_ACTIVITY_KIND_DRIVER:
    {
      CUpti_ActivityAPI *api = (CUpti_ActivityAPI *)record;
      printf("DRIVER cbid=%u [ %llu - %llu ] process %u, thread %u, correlation %u\n",
             api->cbid,
             (unsigned long long)(api->start - startTimestamp),
             (unsigned long long)(api->end - startTimestamp),
             api->processId, api->threadId, api->correlationId);
      break;
    }
  case CUPTI_ACTIVITY_KIND_RUNTIME:
    {
      CUpti_ActivityAPI *api = (CUpti_ActivityAPI *)record;
      printf("RUNTIME cbid=%u [ %llu - %llu ] process %u, thread %u, correlation %u\n",
             api->cbid,
             (unsigned long long)(api->start - startTimestamp),
             (unsigned long long)(api->end - startTimestamp),
             api->processId, api->threadId, api->correlationId);
      break;
    }
  case CUPTI_ACTIVITY_KIND_NAME:
    {
      CUpti_ActivityName *name = (CUpti_ActivityName *)record;
      switch(name->objectKind) {
        case CUPTI_ACTIVITY_OBJECT_CONTEXT:
             printf("NAME  %s %u %s id %u, name %s\n",
                  getActivityObjectKindString(name->objectKind), 
                  getActivityObjectKindId(name->objectKind, &name->objectId), 
                  getActivityObjectKindString(CUPTI_ACTIVITY_OBJECT_DEVICE), 
                  getActivityObjectKindId(CUPTI_ACTIVITY_OBJECT_DEVICE, &name->objectId), 
                  name->name);
             break;
        case CUPTI_ACTIVITY_OBJECT_STREAM:
             printf("NAME %s %u %s %u %s id %u, name %s\n",
                  getActivityObjectKindString(name->objectKind), 
                  getActivityObjectKindId(name->objectKind, &name->objectId), 
                  getActivityObjectKindString(CUPTI_ACTIVITY_OBJECT_CONTEXT), 
                  getActivityObjectKindId(CUPTI_ACTIVITY_OBJECT_CONTEXT, &name->objectId), 
                  getActivityObjectKindString(CUPTI_ACTIVITY_OBJECT_DEVICE), 
                  getActivityObjectKindId(CUPTI_ACTIVITY_OBJECT_DEVICE, &name->objectId), 
                  name->name);
             break;
        default:
             printf("NAME %s id %u, name %s\n",
                  getActivityObjectKindString(name->objectKind), 
                  getActivityObjectKindId(name->objectKind, &name->objectId), 
                  name->name);
             break;
      }
      break;
    }
  case CUPTI_ACTIVITY_KIND_MARKER:
    {
      CUpti_ActivityMarker *marker = (CUpti_ActivityMarker *)record;
      printf("MARKER id %u [ %llu ], name %s\n",
             marker->id, (unsigned long long)marker->timestamp, marker->name);
      break;
    }
  case CUPTI_ACTIVITY_KIND_MARKER_DATA:
    {
      CUpti_ActivityMarkerData *marker = (CUpti_ActivityMarkerData *)record;
      printf("MARKER_DATA id %u, color 0x%x, category %u, payload %llu/%f\n",
             marker->id, marker->color, marker->category,
             (unsigned long long)marker->payload.metricValueUint64, 
             marker->payload.metricValueDouble);
      break;
    }
  case CUPTI_ACTIVITY_KIND_OVERHEAD:
    {
        CUpti_ActivityOverhead *overhead = (CUpti_ActivityOverhead *)record;
        printf("OVERHEAD %s [ %llu, %llu ] %s id %u\n",
               getActivityOverheadKindString(overhead->overheadKind),
               (unsigned long long)overhead->start -startTimestamp,
               (unsigned long long)overhead->end - startTimestamp,
               getActivityObjectKindString(overhead->objectKind), 
               getActivityObjectKindId(overhead->objectKind, &overhead->objectId));
        break;
    }
  default:
    printf("  <unknown>\n");
    break;
  }
}

/**
 * Allocate a new BUF_SIZE buffer and add it to the queue specified by
 * 'context' and 'streamId'.
 */
static void
queueNewBuffer(CUcontext context, uint32_t streamId)
{
  size_t size = BUF_SIZE;
  uint8_t *buffer = (uint8_t *)malloc(size+ALIGN_SIZE);
  CUPTI_CALL(cuptiActivityEnqueueBuffer(context, streamId, ALIGN_BUFFER(buffer, ALIGN_SIZE), size));
}

/**
 * Dump the contents of the top buffer in the queue specified by
 * 'context' and 'streamId', and return the top buffer. If the queue
 * is empty return NULL.
 */
static uint8_t *
dump(CUcontext context, uint32_t streamId)
{
  uint8_t *buffer = NULL;
  size_t validBufferSizeBytes;
  CUptiResult status;
  status = cuptiActivityDequeueBuffer(context, streamId, &buffer, &validBufferSizeBytes);
  if (status == CUPTI_ERROR_QUEUE_EMPTY) {
    return NULL;
  }
  CUPTI_CALL(status);
  
  if (context == NULL) {
    printf("==== Starting dump for global ====\n");
  } else if (streamId == 0) {
    printf("==== Starting dump for context %p ====\n", context);
  } else {
    printf("==== Starting dump for context %p, stream %u ====\n", context, streamId);
  }

  CUpti_Activity *record = NULL;
  do {
    status = cuptiActivityGetNextRecord(buffer, validBufferSizeBytes, &record);
    if(status == CUPTI_SUCCESS) {
      printActivity(record);
    }
    else if (status == CUPTI_ERROR_MAX_LIMIT_REACHED) {
      break;
    }
    else {
      CUPTI_CALL(status);
    }
  } while (1);

  // report any records dropped from the queue
  size_t dropped;
  CUPTI_CALL(cuptiActivityGetNumDroppedRecords(context, streamId, &dropped));
  if (dropped != 0) {
    printf("Dropped %u activity records\n", (unsigned int)dropped);
  }

  if (context == NULL) {
    printf("==== Finished dump for global ====\n");
  } else if (streamId == 0) {
    printf("==== Finished dump for context %p ====\n", context);
  } else {
    printf("==== Finished dump for context %p, stream %u ====\n", context, streamId);
  }

  return buffer;
}

/**
 * If the top buffer in the queue specified by 'context' and
 * 'streamId' is full, then dump its contents and return the
 * buffer. If the top buffer is not full, return NULL.
 */
static uint8_t *
dumpIfFull(CUcontext context, uint32_t streamId)
{
  size_t validBufferSizeBytes;
  CUptiResult status;
  status = cuptiActivityQueryBuffer(context, streamId, &validBufferSizeBytes);
  if (status == CUPTI_ERROR_MAX_LIMIT_REACHED) {
    return dump(context, streamId);
  } else if ((status != CUPTI_SUCCESS) && (status != CUPTI_ERROR_QUEUE_EMPTY)) {
    CUPTI_CALL(status);
  }

  return NULL;
}

static void
handleSync(CUpti_CallbackId cbid, const CUpti_SynchronizeData *syncData)
{
  // check the top buffer of the global queue and dequeue if full. If
  // we dump a buffer add it back to the queue
  uint8_t *buffer = dumpIfFull(NULL, 0);
  if (buffer != NULL) {
    CUPTI_CALL(cuptiActivityEnqueueBuffer(NULL, 0, buffer, BUF_SIZE));
  }

  // dump context buffer on context sync
  if (cbid == CUPTI_CBID_SYNCHRONIZE_CONTEXT_SYNCHRONIZED) {
    buffer = dumpIfFull(syncData->context, 0);
    if (buffer != NULL) {
      CUPTI_CALL(cuptiActivityEnqueueBuffer(syncData->context, 0, buffer, BUF_SIZE));
    }
  }
  // dump stream buffer on stream sync
  else if (cbid == CUPTI_CBID_SYNCHRONIZE_STREAM_SYNCHRONIZED) {
    uint32_t streamId;
    CUPTI_CALL(cuptiGetStreamId(syncData->context, syncData->stream, &streamId));
    buffer = dumpIfFull(syncData->context, streamId);
    if (buffer != NULL) {
      CUPTI_CALL(cuptiActivityEnqueueBuffer(syncData->context, streamId, buffer, BUF_SIZE));
    }
  }
}

static void
handleResource(CUpti_CallbackId cbid, const CUpti_ResourceData *resourceData)
{
  // enqueue buffers on a context's queue when the context is created
  if (cbid == CUPTI_CBID_RESOURCE_CONTEXT_CREATED) {
    queueNewBuffer(resourceData->context, 0);
    queueNewBuffer(resourceData->context, 0);
  }
  // dump all buffers on a context destroy
  else if (cbid == CUPTI_CBID_RESOURCE_CONTEXT_DESTROY_STARTING) {
    while (dump(resourceData->context, 0) != NULL) ;
  }

  // enqueue buffers on a stream's queue when a non-default stream is created
  if (cbid == CUPTI_CBID_RESOURCE_STREAM_CREATED) {
    uint32_t streamId;
    CUPTI_CALL(cuptiGetStreamId(resourceData->context, resourceData->resourceHandle.stream, &streamId));
    queueNewBuffer(resourceData->context, streamId);
    queueNewBuffer(resourceData->context, streamId);
  }
  // dump all buffers on a stream destroy
  else if (cbid == CUPTI_CBID_RESOURCE_STREAM_DESTROY_STARTING) {
    uint32_t streamId;
    CUPTI_CALL(cuptiGetStreamId(resourceData->context, resourceData->resourceHandle.stream, &streamId));
    while (dump(resourceData->context, streamId) != NULL) ;
  }
}

static void CUPTIAPI
traceCallback(void *userdata, CUpti_CallbackDomain domain,
              CUpti_CallbackId cbid, const void *cbdata)
{
  if (domain == CUPTI_CB_DOMAIN_RESOURCE) {
    handleResource(cbid, (CUpti_ResourceData *)cbdata);
  } else if (domain == CUPTI_CB_DOMAIN_SYNCHRONIZE) {
    handleSync(cbid, (CUpti_SynchronizeData *)cbdata);
  }
}

void
initTrace()
{
  // Enqueue a couple of buffers in the global queue
  queueNewBuffer(NULL, 0);
  queueNewBuffer(NULL, 0);

  // device activity record is created when CUDA initializes, so we
  // want to enable it before cuInit() or any CUDA runtime call
  CUPTI_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_DEVICE));

  CUpti_SubscriberHandle subscriber;
  CUPTI_CALL(cuptiSubscribe(&subscriber, (CUpti_CallbackFunc)traceCallback, NULL));
  CUPTI_CALL(cuptiEnableDomain(1, subscriber, CUPTI_CB_DOMAIN_RESOURCE));
  CUPTI_CALL(cuptiEnableDomain(1, subscriber, CUPTI_CB_DOMAIN_SYNCHRONIZE));

  CUPTI_CALL(cuptiGetTimestamp(&startTimestamp));
}

void
finiTrace()
{
  // dump any remaining records from the global queue
  while (dump(NULL, 0) != NULL) ;
}

