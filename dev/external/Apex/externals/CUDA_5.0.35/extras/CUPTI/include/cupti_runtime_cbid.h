
// *************************************************************************
//      Definitions of indices for API functions, unique across entire API
// *************************************************************************

// This file is generated.  Any changes you make will be lost during the next clean build.
// CUDA public interface, for type definitions and cu* function prototypes

typedef enum CUpti_runtime_api_trace_cbid_enum {
    CUPTI_RUNTIME_TRACE_CBID_INVALID                                                       = 0,
    CUPTI_RUNTIME_TRACE_CBID_cudaDriverGetVersion_v3020                                    = 1,
    CUPTI_RUNTIME_TRACE_CBID_cudaRuntimeGetVersion_v3020                                   = 2,
    CUPTI_RUNTIME_TRACE_CBID_cudaGetDeviceCount_v3020                                      = 3,
    CUPTI_RUNTIME_TRACE_CBID_cudaGetDeviceProperties_v3020                                 = 4,
    CUPTI_RUNTIME_TRACE_CBID_cudaChooseDevice_v3020                                        = 5,
    CUPTI_RUNTIME_TRACE_CBID_cudaGetChannelDesc_v3020                                      = 6,
    CUPTI_RUNTIME_TRACE_CBID_cudaCreateChannelDesc_v3020                                   = 7,
    CUPTI_RUNTIME_TRACE_CBID_cudaConfigureCall_v3020                                       = 8,
    CUPTI_RUNTIME_TRACE_CBID_cudaSetupArgument_v3020                                       = 9,
    CUPTI_RUNTIME_TRACE_CBID_cudaGetLastError_v3020                                        = 10,
    CUPTI_RUNTIME_TRACE_CBID_cudaPeekAtLastError_v3020                                     = 11,
    CUPTI_RUNTIME_TRACE_CBID_cudaGetErrorString_v3020                                      = 12,
    CUPTI_RUNTIME_TRACE_CBID_cudaLaunch_v3020                                              = 13,
    CUPTI_RUNTIME_TRACE_CBID_cudaFuncSetCacheConfig_v3020                                  = 14,
    CUPTI_RUNTIME_TRACE_CBID_cudaFuncGetAttributes_v3020                                   = 15,
    CUPTI_RUNTIME_TRACE_CBID_cudaSetDevice_v3020                                           = 16,
    CUPTI_RUNTIME_TRACE_CBID_cudaGetDevice_v3020                                           = 17,
    CUPTI_RUNTIME_TRACE_CBID_cudaSetValidDevices_v3020                                     = 18,
    CUPTI_RUNTIME_TRACE_CBID_cudaSetDeviceFlags_v3020                                      = 19,
    CUPTI_RUNTIME_TRACE_CBID_cudaMalloc_v3020                                              = 20,
    CUPTI_RUNTIME_TRACE_CBID_cudaMallocPitch_v3020                                         = 21,
    CUPTI_RUNTIME_TRACE_CBID_cudaFree_v3020                                                = 22,
    CUPTI_RUNTIME_TRACE_CBID_cudaMallocArray_v3020                                         = 23,
    CUPTI_RUNTIME_TRACE_CBID_cudaFreeArray_v3020                                           = 24,
    CUPTI_RUNTIME_TRACE_CBID_cudaMallocHost_v3020                                          = 25,
    CUPTI_RUNTIME_TRACE_CBID_cudaFreeHost_v3020                                            = 26,
    CUPTI_RUNTIME_TRACE_CBID_cudaHostAlloc_v3020                                           = 27,
    CUPTI_RUNTIME_TRACE_CBID_cudaHostGetDevicePointer_v3020                                = 28,
    CUPTI_RUNTIME_TRACE_CBID_cudaHostGetFlags_v3020                                        = 29,
    CUPTI_RUNTIME_TRACE_CBID_cudaMemGetInfo_v3020                                          = 30,
    CUPTI_RUNTIME_TRACE_CBID_cudaMemcpy_v3020                                              = 31,
    CUPTI_RUNTIME_TRACE_CBID_cudaMemcpy2D_v3020                                            = 32,
    CUPTI_RUNTIME_TRACE_CBID_cudaMemcpyToArray_v3020                                       = 33,
    CUPTI_RUNTIME_TRACE_CBID_cudaMemcpy2DToArray_v3020                                     = 34,
    CUPTI_RUNTIME_TRACE_CBID_cudaMemcpyFromArray_v3020                                     = 35,
    CUPTI_RUNTIME_TRACE_CBID_cudaMemcpy2DFromArray_v3020                                   = 36,
    CUPTI_RUNTIME_TRACE_CBID_cudaMemcpyArrayToArray_v3020                                  = 37,
    CUPTI_RUNTIME_TRACE_CBID_cudaMemcpy2DArrayToArray_v3020                                = 38,
    CUPTI_RUNTIME_TRACE_CBID_cudaMemcpyToSymbol_v3020                                      = 39,
    CUPTI_RUNTIME_TRACE_CBID_cudaMemcpyFromSymbol_v3020                                    = 40,
    CUPTI_RUNTIME_TRACE_CBID_cudaMemcpyAsync_v3020                                         = 41,
    CUPTI_RUNTIME_TRACE_CBID_cudaMemcpyToArrayAsync_v3020                                  = 42,
    CUPTI_RUNTIME_TRACE_CBID_cudaMemcpyFromArrayAsync_v3020                                = 43,
    CUPTI_RUNTIME_TRACE_CBID_cudaMemcpy2DAsync_v3020                                       = 44,
    CUPTI_RUNTIME_TRACE_CBID_cudaMemcpy2DToArrayAsync_v3020                                = 45,
    CUPTI_RUNTIME_TRACE_CBID_cudaMemcpy2DFromArrayAsync_v3020                              = 46,
    CUPTI_RUNTIME_TRACE_CBID_cudaMemcpyToSymbolAsync_v3020                                 = 47,
    CUPTI_RUNTIME_TRACE_CBID_cudaMemcpyFromSymbolAsync_v3020                               = 48,
    CUPTI_RUNTIME_TRACE_CBID_cudaMemset_v3020                                              = 49,
    CUPTI_RUNTIME_TRACE_CBID_cudaMemset2D_v3020                                            = 50,
    CUPTI_RUNTIME_TRACE_CBID_cudaMemsetAsync_v3020                                         = 51,
    CUPTI_RUNTIME_TRACE_CBID_cudaMemset2DAsync_v3020                                       = 52,
    CUPTI_RUNTIME_TRACE_CBID_cudaGetSymbolAddress_v3020                                    = 53,
    CUPTI_RUNTIME_TRACE_CBID_cudaGetSymbolSize_v3020                                       = 54,
    CUPTI_RUNTIME_TRACE_CBID_cudaBindTexture_v3020                                         = 55,
    CUPTI_RUNTIME_TRACE_CBID_cudaBindTexture2D_v3020                                       = 56,
    CUPTI_RUNTIME_TRACE_CBID_cudaBindTextureToArray_v3020                                  = 57,
    CUPTI_RUNTIME_TRACE_CBID_cudaUnbindTexture_v3020                                       = 58,
    CUPTI_RUNTIME_TRACE_CBID_cudaGetTextureAlignmentOffset_v3020                           = 59,
    CUPTI_RUNTIME_TRACE_CBID_cudaGetTextureReference_v3020                                 = 60,
    CUPTI_RUNTIME_TRACE_CBID_cudaBindSurfaceToArray_v3020                                  = 61,
    CUPTI_RUNTIME_TRACE_CBID_cudaGetSurfaceReference_v3020                                 = 62,
    CUPTI_RUNTIME_TRACE_CBID_cudaGLSetGLDevice_v3020                                       = 63,
    CUPTI_RUNTIME_TRACE_CBID_cudaGLRegisterBufferObject_v3020                              = 64,
    CUPTI_RUNTIME_TRACE_CBID_cudaGLMapBufferObject_v3020                                   = 65,
    CUPTI_RUNTIME_TRACE_CBID_cudaGLUnmapBufferObject_v3020                                 = 66,
    CUPTI_RUNTIME_TRACE_CBID_cudaGLUnregisterBufferObject_v3020                            = 67,
    CUPTI_RUNTIME_TRACE_CBID_cudaGLSetBufferObjectMapFlags_v3020                           = 68,
    CUPTI_RUNTIME_TRACE_CBID_cudaGLMapBufferObjectAsync_v3020                              = 69,
    CUPTI_RUNTIME_TRACE_CBID_cudaGLUnmapBufferObjectAsync_v3020                            = 70,
    CUPTI_RUNTIME_TRACE_CBID_cudaWGLGetDevice_v3020                                        = 71,
    CUPTI_RUNTIME_TRACE_CBID_cudaGraphicsGLRegisterImage_v3020                             = 72,
    CUPTI_RUNTIME_TRACE_CBID_cudaGraphicsGLRegisterBuffer_v3020                            = 73,
    CUPTI_RUNTIME_TRACE_CBID_cudaGraphicsUnregisterResource_v3020                          = 74,
    CUPTI_RUNTIME_TRACE_CBID_cudaGraphicsResourceSetMapFlags_v3020                         = 75,
    CUPTI_RUNTIME_TRACE_CBID_cudaGraphicsMapResources_v3020                                = 76,
    CUPTI_RUNTIME_TRACE_CBID_cudaGraphicsUnmapResources_v3020                              = 77,
    CUPTI_RUNTIME_TRACE_CBID_cudaGraphicsResourceGetMappedPointer_v3020                    = 78,
    CUPTI_RUNTIME_TRACE_CBID_cudaGraphicsSubResourceGetMappedArray_v3020                   = 79,
    CUPTI_RUNTIME_TRACE_CBID_cudaVDPAUGetDevice_v3020                                      = 80,
    CUPTI_RUNTIME_TRACE_CBID_cudaVDPAUSetVDPAUDevice_v3020                                 = 81,
    CUPTI_RUNTIME_TRACE_CBID_cudaGraphicsVDPAURegisterVideoSurface_v3020                   = 82,
    CUPTI_RUNTIME_TRACE_CBID_cudaGraphicsVDPAURegisterOutputSurface_v3020                  = 83,
    CUPTI_RUNTIME_TRACE_CBID_cudaD3D11GetDevice_v3020                                      = 84,
    CUPTI_RUNTIME_TRACE_CBID_cudaD3D11GetDevices_v3020                                     = 85,
    CUPTI_RUNTIME_TRACE_CBID_cudaD3D11SetDirect3DDevice_v3020                              = 86,
    CUPTI_RUNTIME_TRACE_CBID_cudaGraphicsD3D11RegisterResource_v3020                       = 87,
    CUPTI_RUNTIME_TRACE_CBID_cudaD3D10GetDevice_v3020                                      = 88,
    CUPTI_RUNTIME_TRACE_CBID_cudaD3D10GetDevices_v3020                                     = 89,
    CUPTI_RUNTIME_TRACE_CBID_cudaD3D10SetDirect3DDevice_v3020                              = 90,
    CUPTI_RUNTIME_TRACE_CBID_cudaGraphicsD3D10RegisterResource_v3020                       = 91,
    CUPTI_RUNTIME_TRACE_CBID_cudaD3D10RegisterResource_v3020                               = 92,
    CUPTI_RUNTIME_TRACE_CBID_cudaD3D10UnregisterResource_v3020                             = 93,
    CUPTI_RUNTIME_TRACE_CBID_cudaD3D10MapResources_v3020                                   = 94,
    CUPTI_RUNTIME_TRACE_CBID_cudaD3D10UnmapResources_v3020                                 = 95,
    CUPTI_RUNTIME_TRACE_CBID_cudaD3D10ResourceSetMapFlags_v3020                            = 96,
    CUPTI_RUNTIME_TRACE_CBID_cudaD3D10ResourceGetSurfaceDimensions_v3020                   = 97,
    CUPTI_RUNTIME_TRACE_CBID_cudaD3D10ResourceGetMappedArray_v3020                         = 98,
    CUPTI_RUNTIME_TRACE_CBID_cudaD3D10ResourceGetMappedPointer_v3020                       = 99,
    CUPTI_RUNTIME_TRACE_CBID_cudaD3D10ResourceGetMappedSize_v3020                          = 100,
    CUPTI_RUNTIME_TRACE_CBID_cudaD3D10ResourceGetMappedPitch_v3020                         = 101,
    CUPTI_RUNTIME_TRACE_CBID_cudaD3D9GetDevice_v3020                                       = 102,
    CUPTI_RUNTIME_TRACE_CBID_cudaD3D9GetDevices_v3020                                      = 103,
    CUPTI_RUNTIME_TRACE_CBID_cudaD3D9SetDirect3DDevice_v3020                               = 104,
    CUPTI_RUNTIME_TRACE_CBID_cudaD3D9GetDirect3DDevice_v3020                               = 105,
    CUPTI_RUNTIME_TRACE_CBID_cudaGraphicsD3D9RegisterResource_v3020                        = 106,
    CUPTI_RUNTIME_TRACE_CBID_cudaD3D9RegisterResource_v3020                                = 107,
    CUPTI_RUNTIME_TRACE_CBID_cudaD3D9UnregisterResource_v3020                              = 108,
    CUPTI_RUNTIME_TRACE_CBID_cudaD3D9MapResources_v3020                                    = 109,
    CUPTI_RUNTIME_TRACE_CBID_cudaD3D9UnmapResources_v3020                                  = 110,
    CUPTI_RUNTIME_TRACE_CBID_cudaD3D9ResourceSetMapFlags_v3020                             = 111,
    CUPTI_RUNTIME_TRACE_CBID_cudaD3D9ResourceGetSurfaceDimensions_v3020                    = 112,
    CUPTI_RUNTIME_TRACE_CBID_cudaD3D9ResourceGetMappedArray_v3020                          = 113,
    CUPTI_RUNTIME_TRACE_CBID_cudaD3D9ResourceGetMappedPointer_v3020                        = 114,
    CUPTI_RUNTIME_TRACE_CBID_cudaD3D9ResourceGetMappedSize_v3020                           = 115,
    CUPTI_RUNTIME_TRACE_CBID_cudaD3D9ResourceGetMappedPitch_v3020                          = 116,
    CUPTI_RUNTIME_TRACE_CBID_cudaD3D9Begin_v3020                                           = 117,
    CUPTI_RUNTIME_TRACE_CBID_cudaD3D9End_v3020                                             = 118,
    CUPTI_RUNTIME_TRACE_CBID_cudaD3D9RegisterVertexBuffer_v3020                            = 119,
    CUPTI_RUNTIME_TRACE_CBID_cudaD3D9UnregisterVertexBuffer_v3020                          = 120,
    CUPTI_RUNTIME_TRACE_CBID_cudaD3D9MapVertexBuffer_v3020                                 = 121,
    CUPTI_RUNTIME_TRACE_CBID_cudaD3D9UnmapVertexBuffer_v3020                               = 122,
    CUPTI_RUNTIME_TRACE_CBID_cudaThreadExit_v3020                                          = 123,
    CUPTI_RUNTIME_TRACE_CBID_cudaSetDoubleForDevice_v3020                                  = 124,
    CUPTI_RUNTIME_TRACE_CBID_cudaSetDoubleForHost_v3020                                    = 125,
    CUPTI_RUNTIME_TRACE_CBID_cudaThreadSynchronize_v3020                                   = 126,
    CUPTI_RUNTIME_TRACE_CBID_cudaThreadGetLimit_v3020                                      = 127,
    CUPTI_RUNTIME_TRACE_CBID_cudaThreadSetLimit_v3020                                      = 128,
    CUPTI_RUNTIME_TRACE_CBID_cudaStreamCreate_v3020                                        = 129,
    CUPTI_RUNTIME_TRACE_CBID_cudaStreamDestroy_v3020                                       = 130,
    CUPTI_RUNTIME_TRACE_CBID_cudaStreamSynchronize_v3020                                   = 131,
    CUPTI_RUNTIME_TRACE_CBID_cudaStreamQuery_v3020                                         = 132,
    CUPTI_RUNTIME_TRACE_CBID_cudaEventCreate_v3020                                         = 133,
    CUPTI_RUNTIME_TRACE_CBID_cudaEventCreateWithFlags_v3020                                = 134,
    CUPTI_RUNTIME_TRACE_CBID_cudaEventRecord_v3020                                         = 135,
    CUPTI_RUNTIME_TRACE_CBID_cudaEventDestroy_v3020                                        = 136,
    CUPTI_RUNTIME_TRACE_CBID_cudaEventSynchronize_v3020                                    = 137,
    CUPTI_RUNTIME_TRACE_CBID_cudaEventQuery_v3020                                          = 138,
    CUPTI_RUNTIME_TRACE_CBID_cudaEventElapsedTime_v3020                                    = 139,
    CUPTI_RUNTIME_TRACE_CBID_cudaMalloc3D_v3020                                            = 140,
    CUPTI_RUNTIME_TRACE_CBID_cudaMalloc3DArray_v3020                                       = 141,
    CUPTI_RUNTIME_TRACE_CBID_cudaMemset3D_v3020                                            = 142,
    CUPTI_RUNTIME_TRACE_CBID_cudaMemset3DAsync_v3020                                       = 143,
    CUPTI_RUNTIME_TRACE_CBID_cudaMemcpy3D_v3020                                            = 144,
    CUPTI_RUNTIME_TRACE_CBID_cudaMemcpy3DAsync_v3020                                       = 145,
    CUPTI_RUNTIME_TRACE_CBID_cudaThreadSetCacheConfig_v3020                                = 146,
    CUPTI_RUNTIME_TRACE_CBID_cudaStreamWaitEvent_v3020                                     = 147,
    CUPTI_RUNTIME_TRACE_CBID_cudaD3D11GetDirect3DDevice_v3020                              = 148,
    CUPTI_RUNTIME_TRACE_CBID_cudaD3D10GetDirect3DDevice_v3020                              = 149,
    CUPTI_RUNTIME_TRACE_CBID_cudaThreadGetCacheConfig_v3020                                = 150,
    CUPTI_RUNTIME_TRACE_CBID_cudaPointerGetAttributes_v4000                                = 151,
    CUPTI_RUNTIME_TRACE_CBID_cudaHostRegister_v4000                                        = 152,
    CUPTI_RUNTIME_TRACE_CBID_cudaHostUnregister_v4000                                      = 153,
    CUPTI_RUNTIME_TRACE_CBID_cudaDeviceCanAccessPeer_v4000                                 = 154,
    CUPTI_RUNTIME_TRACE_CBID_cudaDeviceEnablePeerAccess_v4000                              = 155,
    CUPTI_RUNTIME_TRACE_CBID_cudaDeviceDisablePeerAccess_v4000                             = 156,
    CUPTI_RUNTIME_TRACE_CBID_cudaPeerRegister_v4000                                        = 157,
    CUPTI_RUNTIME_TRACE_CBID_cudaPeerUnregister_v4000                                      = 158,
    CUPTI_RUNTIME_TRACE_CBID_cudaPeerGetDevicePointer_v4000                                = 159,
    CUPTI_RUNTIME_TRACE_CBID_cudaMemcpyPeer_v4000                                          = 160,
    CUPTI_RUNTIME_TRACE_CBID_cudaMemcpyPeerAsync_v4000                                     = 161,
    CUPTI_RUNTIME_TRACE_CBID_cudaMemcpy3DPeer_v4000                                        = 162,
    CUPTI_RUNTIME_TRACE_CBID_cudaMemcpy3DPeerAsync_v4000                                   = 163,
    CUPTI_RUNTIME_TRACE_CBID_cudaDeviceReset_v3020                                         = 164,
    CUPTI_RUNTIME_TRACE_CBID_cudaDeviceSynchronize_v3020                                   = 165,
    CUPTI_RUNTIME_TRACE_CBID_cudaDeviceGetLimit_v3020                                      = 166,
    CUPTI_RUNTIME_TRACE_CBID_cudaDeviceSetLimit_v3020                                      = 167,
    CUPTI_RUNTIME_TRACE_CBID_cudaDeviceGetCacheConfig_v3020                                = 168,
    CUPTI_RUNTIME_TRACE_CBID_cudaDeviceSetCacheConfig_v3020                                = 169,
    CUPTI_RUNTIME_TRACE_CBID_cudaProfilerInitialize_v4000                                  = 170,
    CUPTI_RUNTIME_TRACE_CBID_cudaProfilerStart_v4000                                       = 171,
    CUPTI_RUNTIME_TRACE_CBID_cudaProfilerStop_v4000                                        = 172,
    CUPTI_RUNTIME_TRACE_CBID_cudaDeviceGetByPCIBusId_v4010                                 = 173,
    CUPTI_RUNTIME_TRACE_CBID_cudaDeviceGetPCIBusId_v4010                                   = 174,
    CUPTI_RUNTIME_TRACE_CBID_cudaGLGetDevices_v4010                                        = 175,
    CUPTI_RUNTIME_TRACE_CBID_cudaIpcGetEventHandle_v4010                                   = 176,
    CUPTI_RUNTIME_TRACE_CBID_cudaIpcOpenEventHandle_v4010                                  = 177,
    CUPTI_RUNTIME_TRACE_CBID_cudaIpcGetMemHandle_v4010                                     = 178,
    CUPTI_RUNTIME_TRACE_CBID_cudaIpcOpenMemHandle_v4010                                    = 179,
    CUPTI_RUNTIME_TRACE_CBID_cudaIpcCloseMemHandle_v4010                                   = 180,
    CUPTI_RUNTIME_TRACE_CBID_cudaArrayGetInfo_v4010                                        = 181,
    CUPTI_RUNTIME_TRACE_CBID_cudaFuncSetSharedMemConfig_v4020                              = 182,
    CUPTI_RUNTIME_TRACE_CBID_cudaDeviceGetSharedMemConfig_v4020                            = 183,
    CUPTI_RUNTIME_TRACE_CBID_cudaDeviceSetSharedMemConfig_v4020                            = 184,
    CUPTI_RUNTIME_TRACE_CBID_cudaCreateTextureObject_v5000                                 = 185,
    CUPTI_RUNTIME_TRACE_CBID_cudaDestroyTextureObject_v5000                                = 186,
    CUPTI_RUNTIME_TRACE_CBID_cudaGetTextureObjectResourceDesc_v5000                        = 187,
    CUPTI_RUNTIME_TRACE_CBID_cudaGetTextureObjectTextureDesc_v5000                         = 188,
    CUPTI_RUNTIME_TRACE_CBID_cudaCreateSurfaceObject_v5000                                 = 189,
    CUPTI_RUNTIME_TRACE_CBID_cudaDestroySurfaceObject_v5000                                = 190,
    CUPTI_RUNTIME_TRACE_CBID_cudaGetSurfaceObjectResourceDesc_v5000                        = 191,
    CUPTI_RUNTIME_TRACE_CBID_cudaMallocMipmappedArray_v5000                                = 192,
    CUPTI_RUNTIME_TRACE_CBID_cudaGetMipmappedArrayLevel_v5000                              = 193,
    CUPTI_RUNTIME_TRACE_CBID_cudaFreeMipmappedArray_v5000                                  = 194,
    CUPTI_RUNTIME_TRACE_CBID_cudaBindTextureToMipmappedArray_v5000                         = 195,
    CUPTI_RUNTIME_TRACE_CBID_cudaGraphicsResourceGetMappedMipmappedArray_v5000             = 196,
    CUPTI_RUNTIME_TRACE_CBID_cudaStreamAddCallback_v5000                                   = 197,
    CUPTI_RUNTIME_TRACE_CBID_cudaStreamCreateWithFlags_v5000                               = 198,
    CUPTI_RUNTIME_TRACE_CBID_cudaGetTextureObjectResourceViewDesc_v5000                    = 199,
    CUPTI_RUNTIME_TRACE_CBID_cudaDeviceGetAttribute_v5000                                  = 200,
    CUPTI_RUNTIME_TRACE_CBID_SIZE                                                          = 201,
    CUPTI_RUNTIME_TRACE_CBID_FORCE_INT                                                     = 0x7fffffff
} CUpti_runtime_api_trace_cbid;

