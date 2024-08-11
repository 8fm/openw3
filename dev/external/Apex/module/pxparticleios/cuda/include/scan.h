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

APEX_CUDA_SYNC_KERNEL(SCAN_WARPS_PER_BLOCK, scanKernel, ((unsigned int, count))
                      ((float, benefitMin))((float, benefitMax))
                      ((unsigned int*, g_indices))((const float*, g_benefits))
                      ((unsigned int*, g_boundParams))
                      ((unsigned int*, g_tmpCounts))((unsigned int*, g_tmpCounts1))
                     )

APEX_CUDA_BOUND_KERNEL(SCAN_WARPS_PER_BLOCK, scan1Kernel,
                       ((float, benefitMin))((float, benefitMax))
                       ((APEX_MEM_BLOCK(unsigned int), g_indices))((APEX_MEM_BLOCK(float), g_benefits))
                       ((APEX_MEM_BLOCK(unsigned int), g_boundParams))
                       ((APEX_MEM_BLOCK(unsigned int), g_tmpCounts))((APEX_MEM_BLOCK(unsigned int), g_tmpCounts1))
                      )

APEX_CUDA_BOUND_KERNEL(SCAN_WARPS_PER_BLOCK, scan2Kernel,
                       ((float, benefitMin))((float, benefitMax))
                       ((APEX_MEM_BLOCK(unsigned int), g_indices))((APEX_MEM_BLOCK(float), g_benefits))
                       ((APEX_MEM_BLOCK(unsigned int), g_boundParams))
                       ((APEX_MEM_BLOCK(unsigned int), g_tmpCounts))((APEX_MEM_BLOCK(unsigned int), g_tmpCounts1))
                       ((unsigned int, gridSize))
                      )

APEX_CUDA_BOUND_KERNEL(SCAN_WARPS_PER_BLOCK, scan3Kernel,
                       ((float, benefitMin))((float, benefitMax))
                       ((APEX_MEM_BLOCK(unsigned int), g_indices))((APEX_MEM_BLOCK(float), g_benefits))
                       ((APEX_MEM_BLOCK(unsigned int), g_boundParams))
                       ((APEX_MEM_BLOCK(unsigned int), g_tmpCounts))((APEX_MEM_BLOCK(unsigned int), g_tmpCounts1))
                      )
