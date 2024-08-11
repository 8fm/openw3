// This code contains NVIDIA Confidential Information and is disclosed to you
// under a form of NVIDIA software license agreement provided separately to you.
//
// Notice
// NVIDIA Corporation and its licensors retain all intellectual property and
// proprietary rights in and to this software and related documentation and
// any modifications thereto. Any use, reproduction, disclosure, or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA Corporation is strictly prohibited.
//
// ALL NVIDIA DESIGN SPECIFICATIONS, CODE ARE PROVIDED "AS IS.". NVIDIA MAKES
// NO WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ALL IMPLIED WARRANTIES OF NONINFRINGEMENT,
// MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Information and code furnished is believed to be accurate and reliable.
// However, NVIDIA Corporation assumes no responsibility for the consequences of use of such
// information or for any infringement of patents or other rights of third parties that may
// result from its use. No license is granted by implication or otherwise under any patent
// or patent rights of NVIDIA Corporation. Details are subject to change without notice.
// This code supersedes and replaces all information previously supplied.
// NVIDIA Corporation products are not authorized for use as critical
// components in life support devices or systems without express written approval of
// NVIDIA Corporation.
//
// Copyright (c) 2008-2013 NVIDIA Corporation. All rights reserved.

APEX_CUDA_TEXTURE_1D(texRefPositionMass,         float4)
APEX_CUDA_TEXTURE_1D(texRefVelocityLife,         float4)
APEX_CUDA_TEXTURE_1D(texRefCollisionNormalFlags, float4)
APEX_CUDA_TEXTURE_1D(texRefDensity,              float)

APEX_CUDA_TEXTURE_1D(texRefUserData,             unsigned int)

APEX_CUDA_TEXTURE_1D(texRefSpritePrivState0,     uint4)

APEX_CUDA_TEXTURE_1D(texRefMeshPrivState0,       uint4)
APEX_CUDA_TEXTURE_1D(texRefMeshPrivState1,       uint4)
APEX_CUDA_TEXTURE_1D(texRefMeshPrivState2,       uint4)

APEX_CUDA_TEXTURE_1D(texRefInStateToInput,       unsigned int)
APEX_CUDA_TEXTURE_1D(texRefStateSpawnSeed,       unsigned int)

APEX_CUDA_TEXTURE_2D(texRefCurveSamples,         float)

APEX_CUDA_CONST_MEM(modifierConstMem, MAX_CONST_MEM_SIZE - 32)


APEX_CUDA_SURFACE_2D(surfRefOutput0)
APEX_CUDA_SURFACE_2D(surfRefOutput1)
APEX_CUDA_SURFACE_2D(surfRefOutput2)
APEX_CUDA_SURFACE_2D(surfRefOutput3)


APEX_CUDA_BOUND_KERNEL(SPRITE_MODIFIER_WARPS_PER_BLOCK, spriteModifiersKernel,
                       ((unsigned int, inStateOffset))((unsigned int, outStateOffset))
                       ((InplaceHandle<AssetParamsHandleArray>, assetParamsHandleArrayHandle))
                       ((ModifierCommonParams, commonParams))((unsigned int, numActorIDs))
                       ((unsigned int*, g_sortedActorIDs))((unsigned int*, g_sortedStateIDs))((unsigned int*, g_outStateToInput))
                       ((SpritePrivateStateArgs, privStateArgs))
                       ((PRNGInfo, rand))((unsigned int*, g_outputBuffer))
					   ((InplaceHandle<SpriteOutputLayout>, outputLayoutHandle))
                      )

APEX_CUDA_BOUND_KERNEL(SPRITE_MODIFIER_WARPS_PER_BLOCK, spriteTextureModifiersKernel,
                       ((unsigned int, inStateOffset))((unsigned int, outStateOffset))
                       ((InplaceHandle<AssetParamsHandleArray>, assetParamsHandleArrayHandle))
                       ((ModifierCommonParams, commonParams))((unsigned int, numActorIDs))
                       ((unsigned int*, g_sortedActorIDs))((unsigned int*, g_sortedStateIDs))((unsigned int*, g_outStateToInput))
                       ((SpritePrivateStateArgs, privStateArgs))
                       ((PRNGInfo, rand))((SpriteTextureOutputLayout, outputLayout))
                      )

APEX_CUDA_BOUND_KERNEL(MESH_MODIFIER_WARPS_PER_BLOCK, meshModifiersKernel,
                       ((unsigned int, inStateOffset))((unsigned int, outStateOffset))
                       ((InplaceHandle<AssetParamsHandleArray>, assetParamsHandleArrayHandle))
                       ((ModifierCommonParams, commonParams))((unsigned int, numActorIDs))
                       ((unsigned int*, g_sortedActorIDs))((unsigned int*, g_sortedStateIDs))((unsigned int*, g_outStateToInput))
                       ((MeshPrivateStateArgs, privStateArgs))
                       ((PRNGInfo, rand))((unsigned int*, g_outputBuffer))
					   ((InplaceHandle<MeshOutputLayout>, outputLayoutHandle))
                      )
