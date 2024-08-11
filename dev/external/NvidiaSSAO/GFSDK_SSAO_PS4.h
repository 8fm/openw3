// This code contains NVIDIA Confidential Information and is disclosed 
// under the Mutual Non-Disclosure Agreement. 
// 
// Notice 
// ALL NVIDIA DESIGN SPECIFICATIONS AND CODE ("MATERIALS") ARE PROVIDED "AS IS" NVIDIA MAKES 
// NO REPRESENTATIONS, WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO 
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ANY IMPLIED WARRANTIES OF NONINFRINGEMENT, 
// MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE. 
// 
// NVIDIA Corporation assumes no responsibility for the consequences of use of such 
// information or for any infringement of patents or other rights of third parties that may 
// result from its use. No license is granted by implication or otherwise under any patent 
// or patent rights of NVIDIA Corporation. No third party distribution is allowed unless 
// expressly authorized by NVIDIA.  Details are subject to change without notice. 
// This code supersedes and replaces all information previously supplied. 
// NVIDIA Corporation products are not authorized for use as critical 
// components in life support devices or systems without express written approval of 
// NVIDIA Corporation. 
// 
// Copyright (c) 2008-2014 NVIDIA Corporation. All rights reserved.
//
// NVIDIA Corporation and its licensors retain all intellectual property and proprietary
// rights in and to this software and related documentation and any modifications thereto.
// Any use, reproduction, disclosure or distribution of this software and related
// documentation without an express license agreement from NVIDIA Corporation is
// strictly prohibited.
//

#pragma once
#pragma pack(push,8) // Make sure we have consistent structure packings

#include <stddef.h>
#include "GFSDK_SSAO_PC.h"

namespace sce
{
    namespace Gnmx
    {
        class GfxContext;
    }

    namespace Gnm
    {
        class RenderTarget;
        class Texture;
        class BlendControl;
    }
}
typedef unsigned int uint32_t;

struct GFSDK_SSAO_InputDepthData_GNM : GFSDK_SSAO_InputDepthData
{
    sce::Gnm::Texture*			pFullResDepthTextureSRV;    // Full-resolution depth texture
    uint32_t					left;						// The X coordinate of the left edge of the rendering surface in pixels.   
    uint32_t					top;						// The Y coordinate of the top edge of the rendering surface in pixels. 
    uint32_t					right;						// The X coordinate of the right edge of the rendering surface in pixels.  
    uint32_t					bottom;						// The Y coordinate of the bottom edge of the rendering surface in pixels.   
    float						zScale;						// The scale value for the Z transform from clip-space to screen-space. The correct value depends on which convention you are following in your projection matrix.
    float						zOffset;					// The offset value for the Z transform from clip-space to screen-space. The correct value depends on which convention you are following in your projection matrix.
     
    GFSDK_SSAO_InputDepthData_GNM()
        : pFullResDepthTextureSRV(NULL)
        , left( 0 )
        , top( 0 )
        , right( 0 )
        , bottom( 0 )
        , zScale( 1.f )
        , zOffset( 0.f )
     {
     }
};

struct GFSDK_SSAO_InputNormalData_GNM : GFSDK_SSAO_InputNormalData
{
    sce::Gnm::Texture*   pFullResNormalTextureSRV;       // Full-resolution world-space normal texture

    GFSDK_SSAO_InputNormalData_GNM()
        : pFullResNormalTextureSRV(NULL)
    {
    }
};

//---------------------------------------------------------------------------------------------------
// Input data.
//---------------------------------------------------------------------------------------------------

struct GFSDK_SSAO_InputData_GNM
{
    GFSDK_SSAO_InputDepthData_GNM         DepthData;          // Required
    GFSDK_SSAO_InputNormalData_GNM	      NormalData;         // Optional GBuffer normals
};
struct GFSDK_SSAO_CustomBlendState_GNM
{
    sce::Gnm::BlendControl*         pBlendState;                    // Custom blend state to composite the AO with
    const GFSDK_SSAO_FLOAT*         pBlendFactor;                   // Relevant only if pBlendState uses D3D11_BLEND_BLEND_FACTOR

    GFSDK_SSAO_CustomBlendState_GNM()
        : pBlendState(NULL)
        , pBlendFactor(NULL)
    {
    }
};

struct GFSDK_SSAO_OutputParameters_GNM : GFSDK_SSAO_OutputParameters
{
    GFSDK_SSAO_CustomBlendState_GNM		CustomBlendState;       // Relevant only if BlendMode is CUSTOM_BLEND
    GFSDK_SSAO_MSAAMode                 MSAAMode;               // Relevant only if the input and output textures are multisample

    GFSDK_SSAO_OutputParameters_GNM()
        : MSAAMode(GFSDK_SSAO_PER_PIXEL_AO)
    {
    }
};

struct GFSDK_SSAO_Parameters_GNM : GFSDK_SSAO_Parameters
{
    GFSDK_SSAO_OutputParameters_GNM   Output;                 // To composite the AO with the output render target
};

/*====================================================================================================
  [Optional] Let the library allocate its memory on a custom heap.
====================================================================================================*/

typedef void* (__cdecl *GFSDK_SSAO_MALLOC)			(size_t size);
typedef void  (__cdecl *GFSDK_SSAO_FREE)			(void *p);
typedef void* (__cdecl *GFSDK_SSAO_ALIGNED_MALLOC)	(size_t size, size_t alignment);
typedef void  (__cdecl *GFSDK_SSAO_ALIGNED_FREE)	(void *p);

struct GFSDK_SSAO_Malloc_Hooks
{
    GFSDK_SSAO_ALIGNED_MALLOC	pOnionAlloc;
    GFSDK_SSAO_ALIGNED_FREE		pOnionFree;
    GFSDK_SSAO_ALIGNED_MALLOC	pGarlicAlloc;
    GFSDK_SSAO_ALIGNED_FREE		pGarlicFree;
};

/*====================================================================================================
  GNM interface.
====================================================================================================*/
//---------------------------------------------------------------------------------------------------
// Note: The RenderAO, PreCreateRTs and Release entry points should not be called simultaneously from different threads.
//---------------------------------------------------------------------------------------------------
class GFSDK_SSAO_Context_GNM : public GFSDK_SSAO_Context
{
public:

//---------------------------------------------------------------------------------------------------
// Renders SSAO to pOutputColorRT.
//
// Remarks:
//    * Allocates internal D3D render targets on first use, and re-allocates them when the depth-texture resolution changes.
//    * All the relevant device-context states are saved and restored internally when entering and exiting the call.
//    * Setting RenderMask = GFSDK_SSAO_DRAW_DEBUG_N can be useful to visualize the normals used for the AO rendering.
//
// Returns:
//     GFSDK_SSAO_NULL_ARGUMENT                        - One of the required argument pointers is NULL
//     GFSDK_SSAO_INVALID_PROJECTION_MATRIX            - The projection matrix is not valid (off-centered?)
//     GFSDK_SSAO_INVALID_VIEWPORT_DIMENSIONS          - The viewport rectangle expands outside of the input depth texture
//     GFSDK_SSAO_INVALID_VIEWPORT_DEPTH_RANGE         - The viewport depth range is not a sub-range of [0.f,1.f]
//     GFSDK_SSAO_INVALID_DEPTH_TEXTURE                - The depth texture is not valid (MSAA view-depth textures are not supported)
//     GFSDK_SSAO_INVALID_WORLD_TO_VIEW_MATRIX         - The world-to-view matrix is not valid (transposing it may help)
//     GFSDK_SSAO_INVALID_NORMAL_TEXTURE_RESOLUTION    - The normal-texture resolution does not match the depth-texture resolution
//     GFSDK_SSAO_INVALID_NORMAL_TEXTURE_SAMPLE_COUNT  - The normal-texture sample count does not match the depth-texture sample count
//     GFSDK_SSAO_INVALID_OUTPUT_MSAA_SAMPLE_COUNT     - (MSAAMode == PER_SAMPLE_AO) && (SampleCount(OutputColorRT) != SampleCount(InputDepthTexture))
//     GFSDK_SSAO_VIEWPORT_DIMENSIONS_NOT_SUPPORTED    - View depths are used as input together with a partial viewport (not supported)
//     GFSDK_SSAO_D3D_RESOURCE_CREATION_FAILED         - A D3D resource-creation call has failed (running out of memory?)
//     GFSDK_SSAO_OK                                   - Success
//---------------------------------------------------------------------------------------------------
virtual GFSDK_SSAO_Status RenderAO(sce::Gnmx::GfxContext *pDeviceContext,
                                   const GFSDK_SSAO_InputData_GNM* pInputData,
                                   const GFSDK_SSAO_Parameters_GNM* pParameters,
                                   sce::Gnm::RenderTarget* pOutputColorRT,
                                   GFSDK_SSAO_RenderMask RenderMask = GFSDK_SSAO_RENDER_AO) = 0;

//---------------------------------------------------------------------------------------------------
// [Optional] Pre-creates all internal render targets for RenderAO.
//
// Remarks:
//    * This call may be safely skipped since RenderAO creates its render targets on demand if they were not pre-created.
//    * This call releases and re-creates the internal render targets if the provided resolution changes.
//    * This call performs CreateTexture calls for all the relevant render targets.
//
// Returns:
//     GFSDK_SSAO_NULL_ARGUMENT                        - One of the required argument pointers is NULL
//     GFSDK_SSAO_D3D_RESOURCE_CREATION_FAILED         - A D3D resource-creation call has failed (running out of memory?)
//     GFSDK_SSAO_OK                                   - Success
//---------------------------------------------------------------------------------------------------
virtual GFSDK_SSAO_Status PreCreateRTs(const GFSDK_SSAO_Parameters_GNM* pParameters,
                                       GFSDK_SSAO_UINT ViewportWidth,
                                       GFSDK_SSAO_UINT ViewportHeight) = 0;

//---------------------------------------------------------------------------------------------------
// Releases all GNM objects created by the library (to be called right before releasing the GNM device).
//---------------------------------------------------------------------------------------------------
virtual void Release() = 0;

}; //class GFSDK_SSAO_Context_D3D11

//---------------------------------------------------------------------------------------------------
// Creates a GFSDK_SSAO_Context associated with the GNM device.
//
// Remarks:
//    * Allocates GNM resources internally.
//    * Allocates memory using the default "::operator new", or "pCustomHeap->new_" if provided.
//
// Returns:
//     GFSDK_SSAO_NULL_ARGUMENT                        - One of the required argument pointers is NULL
//     GFSDK_SSAO_VERSION_MISMATCH                     - Invalid HeaderVersion (have you set HeaderVersion = GFSDK_SSAO_Version()?)
//     GFSDK_SSAO_MEMORY_ALLOCATION_FAILED             - Failed to allocate memory on the heap
//     GFSDK_SSAO_D3D_FEATURE_LEVEL_NOT_SUPPORTED      - The D3D11 feature level of pD3DDevice is lower than 11_0
//     GFSDK_SSAO_D3D_RESOURCE_CREATION_FAILED         - A resource-creation call has failed (running out of memory?)
//     GFSDK_SSAO_OK                                   - Success
//---------------------------------------------------------------------------------------------------
GFSDK_SSAO_DECL(GFSDK_SSAO_Status) GFSDK_SSAO_CreateContext_GNM(sce::Gnmx::GfxContext* pDeviceContext,
                                                                GFSDK_SSAO_Malloc_Hooks* pMallocHooks,
                                                                GFSDK_SSAO_Context_GNM** ppContext,
                                                                const GFSDK_SSAO_CustomHeap* pCustomHeap = NULL,
                                                                GFSDK_SSAO_Version HeaderVersion = GFSDK_SSAO_Version());


#pragma pack(pop)
