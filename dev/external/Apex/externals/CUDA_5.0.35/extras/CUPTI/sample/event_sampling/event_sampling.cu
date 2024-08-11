/*
 * Copyright 2011-2012 NVIDIA Corporation. All rights reserved
 *
 * Sample app to demonstrate use of CUPTI library to obtain profiler
 * event values by sampling.
 */


#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#endif

#include <stdio.h> 
#include <cuda_runtime_api.h>
#include <cupti_events.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <pthread.h>
#endif

#define CHECK_CU_ERROR(err, cufunc)                                     \
  if (err != CUDA_SUCCESS)                                              \
    {                                                                   \
      printf ("Error %d for CUDA Driver API function '%s'.\n",          \
              err, cufunc);                                             \
      exit(-1);                                                         \
    }

#define CHECK_CUPTI_ERROR(err, cuptifunc)                       \
  if (err != CUPTI_SUCCESS)                                     \
    {                                                           \
      const char *errstr;                                       \
      cuptiGetResultString(err, &errstr);                       \
      printf ("%s:%d:Error %s for CUPTI API function '%s'.\n",  \
              __FILE__, __LINE__, errstr, cuptifunc);           \
      exit(-1);                                                 \
    }

#define EVENT_NAME "inst_executed"
#define N 100000
#define TESLA_ITERS 500
#define FERMI_ITERS 2000
#define SAMPLE_PERIOD_MS 50

// used to signal from the compute thread to the sampling thread
static volatile int testComplete = 0;

static CUcontext context;
static CUdevice device;
static char *eventName;


// Device code
__global__ void VecAdd(const int* A, const int* B, int* C, int size)
{
  int i = blockDim.x * blockIdx.x + threadIdx.x;
  for(int n = 0 ; n < 100; n++) {
    if (i < size)
      C[i] = A[i] + B[i];
  }
}

static void
initVec(int *vec, int n)
{
  for (int i=0; i< n; i++)
    vec[i] = i;
}

void *
sampling_func(void *arg)
{
  CUptiResult cuptiErr;
  CUpti_EventGroup eventGroup;
  CUpti_EventID eventId;
  size_t bytesRead; 
  uint64_t eventVal;

  cuptiErr = cuptiSetEventCollectionMode(context, 
                                         CUPTI_EVENT_COLLECTION_MODE_CONTINUOUS);
  CHECK_CUPTI_ERROR(cuptiErr, "cuptiSetEventCollectionMode");

  cuptiErr = cuptiEventGroupCreate(context, &eventGroup, 0);
  CHECK_CUPTI_ERROR(cuptiErr, "cuptiEventGroupCreate");

  cuptiErr = cuptiEventGetIdFromName(device, eventName, &eventId);
  CHECK_CUPTI_ERROR(cuptiErr, "cuptiEventGetIdFromName");

  cuptiErr = cuptiEventGroupAddEvent(eventGroup, eventId);
  CHECK_CUPTI_ERROR(cuptiErr, "cuptiEventGroupAddEvent");

  cuptiErr = cuptiEventGroupEnable(eventGroup);
  CHECK_CUPTI_ERROR(cuptiErr, "cuptiEventGroupEnable");

  while (!testComplete) {
    bytesRead = sizeof(eventVal);
    cuptiErr = cuptiEventGroupReadEvent(eventGroup, 
                                        CUPTI_EVENT_READ_FLAG_NONE, 
                                        eventId, &bytesRead, &eventVal);
    CHECK_CUPTI_ERROR(cuptiErr, "cuptiEventGroupReadEvent");
    if (bytesRead != sizeof(eventVal)) {
      printf("Failed to read value for \"%s\"\n", eventName);
      exit(-1);
    }

    printf("%s: %llu\n", eventName, (unsigned long long)eventVal);

#ifdef _WIN32
    Sleep(SAMPLE_PERIOD_MS);
#else
    usleep(SAMPLE_PERIOD_MS * 1000);
#endif
  }

  cuptiErr = cuptiEventGroupDisable(eventGroup);
  CHECK_CUPTI_ERROR(cuptiErr, "cuptiEventGroupDisable");

  cuptiErr = cuptiEventGroupDestroy(eventGroup);
  CHECK_CUPTI_ERROR(cuptiErr, "cuptiEventGroupDestroy");

  return NULL;
}

static void
compute(int iters)
{
  size_t size = N * sizeof(int);
  int threadsPerBlock = 0;
  int blocksPerGrid = 0;
  int sum, i;
  int *h_A, *h_B, *h_C;
  int *d_A, *d_B, *d_C;

  // Allocate input vectors h_A and h_B in host memory
  h_A = (int*)malloc(size);
  h_B = (int*)malloc(size);
  h_C = (int*)malloc(size);

  // Initialize input vectors
  initVec(h_A, N);
  initVec(h_B, N);
  memset(h_C, 0, size);

  // Allocate vectors in device memory
  cudaMalloc((void**)&d_A, size);
  cudaMalloc((void**)&d_B, size);
  cudaMalloc((void**)&d_C, size);

  // Copy vectors from host memory to device memory
  cudaMemcpy(d_A, h_A, size, cudaMemcpyHostToDevice);
  cudaMemcpy(d_B, h_B, size, cudaMemcpyHostToDevice);

  // Invoke kernel (multiple times to make sure we have time for
  // sampling)
  threadsPerBlock = 256;
  blocksPerGrid = (N + threadsPerBlock - 1) / threadsPerBlock;
  for (i = 0; i < iters; i++) {
    VecAdd<<<blocksPerGrid, threadsPerBlock>>>(d_A, d_B, d_C, N);
  }

    
  // Copy result from device memory to host memory
  // h_C contains the result in host memory
  cudaMemcpy(h_C, d_C, size, cudaMemcpyDeviceToHost);
    
  // Verify result
  for (i = 0; i < N; ++i) {
    sum = h_A[i] + h_B[i];
    if (h_C[i] != sum) {
      printf("kernel execution FAILED\n");
      exit(-1);
    }
  }
}

int
main(int argc, char *argv[])
{
#ifdef _WIN32
  HANDLE hThread;
#else
  int status;
  pthread_t pThread;
#endif
  CUresult err;
  int computeCapabilityMajor=0;
  int computeCapabilityMinor=0;
  int deviceNum;
  int deviceCount;
  char deviceName[32];

  printf("Usage: %s [device_num] [event_name]\n", argv[0]);
    
  err = cuInit(0);
  CHECK_CU_ERROR(err, "cuInit");

  err = cuDeviceGetCount(&deviceCount);
  CHECK_CU_ERROR(err, "cuDeviceGetCount");

  if (deviceCount == 0) {
    printf("There is no device supporting CUDA.\n");
    exit(-1);
  }

  if (argc > 1)
    deviceNum = atoi(argv[1]);
  else
    deviceNum = 0;
  printf("CUDA Device Number: %d\n", deviceNum);

  err = cuDeviceGet(&device, deviceNum);
  CHECK_CU_ERROR(err, "cuDeviceGet");

  err = cuDeviceGetName(deviceName, 32, device);
  CHECK_CU_ERROR(err, "cuDeviceGetName");

  printf("CUDA Device Name: %s\n", deviceName);

  err = cuDeviceComputeCapability(&computeCapabilityMajor, 
                                  &computeCapabilityMinor, 
                                  device);
  CHECK_CU_ERROR(err, "cuDeviceComputeCapability");

  if (argc > 2) {
    eventName = argv[2];
  }
  else {
    if (computeCapabilityMajor > 1) {
      eventName = EVENT_NAME;
    } 
    else {
      printf("Event sampling is not supported for Tesla family devices.\n");
      return 0;
    }
  }

  err = cuCtxCreate(&context, 0, device);
  CHECK_CU_ERROR(err, "cuCtxCreate");

  
  testComplete = 0;

  printf("Creating sampling thread\n");
#ifdef _WIN32
  hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) sampling_func,
                         NULL, 0, NULL );
  if (!hThread) {
    printf("CreateThread failed\n");
    exit(-1);
  }
#else
  status = pthread_create(&pThread, NULL, sampling_func, NULL);
  if (status != 0) {
    perror("pthread_create");
    exit(-1);
  }
#endif

  // run kernel while sampling
  compute((computeCapabilityMajor > 1) ? FERMI_ITERS : TESLA_ITERS);

  // "signal" the sampling thread to exit and wait for it
  testComplete = 1;
#ifdef _WIN32
  WaitForSingleObject(hThread, INFINITE);
#else
  pthread_join(pThread, NULL);
#endif

  cudaDeviceSynchronize();
  return 0;
}
