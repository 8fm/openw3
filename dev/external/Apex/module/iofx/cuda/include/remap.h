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

APEX_CUDA_TEXTURE_1D(texRefRemapPositions,      float4)
APEX_CUDA_TEXTURE_1D(texRefRemapActorIDs,       unsigned int)
APEX_CUDA_TEXTURE_1D(texRefRemapInStateToInput, unsigned int)

APEX_CUDA_BOUND_KERNEL(REMAP_WARPS_PER_BLOCK, makeSortKeys,
                       ((const physx::PxU32*, inStateToInput))((physx::PxU32, maxInputID))
                       ((const NiIofxActorID*, actorID))((physx::PxU32, numActorClasses))((physx::PxU32, numActorIDs))
                       ((const float4*, positionMass))((bool, outputDensityKeys))
                       ((physx::PxVec3, eyePos))((physx::PxVec3, eyeDir))((physx::PxF32, zNear))
                       ((physx::PxU32*, sortKey))((physx::PxU32*, sortValue))
                      )

APEX_CUDA_BOUND_KERNEL(REMAP_WARPS_PER_BLOCK, remapKernel,
                       ((const physx::PxU32*, inStateToInput))((physx::PxU32, maxInputID))
                       ((const NiIofxActorID*, actorID))((physx::PxU32, numActorClasses))((physx::PxU32, numActorIDs))
                       ((const unsigned int*, inSortedValue))((unsigned int*, outSortKey))
                      )
