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

// This file is used to define a list of AgPerfMon events.
//
// This file is included exclusively by AgPerfMonEventSrcAPI.h
// and by AgPerfMonEventSrcAPI.cpp, for the purpose of building
// an enumeration (enum xx) and an array of strings ()
// that contain the list of events.
//
// This file should only contain event definitions, using the
// DEFINE_EVENT macro.  E.g.:
//
//     DEFINE_EVENT(sample_name_1)
//     DEFINE_EVENT(sample_name_2)
//     DEFINE_EVENT(sample_name_3)

DEFINE_EVENT(DestructibleFetchResults)
DEFINE_EVENT(DestructibleUpdateRenderResources)
DEFINE_EVENT(DestructibleDispatchRenderResources)
DEFINE_EVENT(DestructibleSeparateUnsupportedIslands)
DEFINE_EVENT(DestructibleCreateDynamicIsland)
DEFINE_EVENT(DestructibleCookChunkCollisionMeshes)
DEFINE_EVENT(DestructibleAddActors)
DEFINE_EVENT(DestructibleCreateRoot)
DEFINE_EVENT(DestructibleAppendShape)
DEFINE_EVENT(DestructibleStructureTick)
DEFINE_EVENT(DestructiblePhysBasedStressSolver)
DEFINE_EVENT(DestructiblePhysBasedStressSolverInitial)
DEFINE_EVENT(DestructiblePhysBasedStressSolverCalculateForce)
DEFINE_EVENT(DestructiblePhysBasedStressSolverRemoveChunk)
DEFINE_EVENT(DestructibleStructureBoundsCalculateOverlaps)
DEFINE_EVENT(DestructibleStructureDetailedOverlapTest)
DEFINE_EVENT(DestructibleCreateActor)
DEFINE_EVENT(DestructibleCacheChunkCookedCollisionMeshes)
DEFINE_EVENT(DestructibleFractureChunk)
DEFINE_EVENT(DestructibleChunkReport)
DEFINE_EVENT(DestructibleRayCastFindVisibleChunk)
DEFINE_EVENT(DestructibleRayCastFindDeepestChunk)
DEFINE_EVENT(DestructibleRayCastProcessChunk)
DEFINE_EVENT(DestructibleProcessFIFOForLOD)
DEFINE_EVENT(DestructibleRemoveChunksForBudget)
DEFINE_EVENT(DestructibleProcessFractureBuffer)
DEFINE_EVENT(DestructibleKillStructures)
DEFINE_EVENT(DestructibleOnContactConstraint)
DEFINE_EVENT(DestructibleOnContactNotify)
DEFINE_EVENT(DestructibleBeforeTickLockRenderables)
DEFINE_EVENT(DestructibleUpdateRenderMeshBonePoses)
DEFINE_EVENT(DestructibleStructureBuildSupportGraph)
DEFINE_EVENT(DestructibleSimulate)
DEFINE_EVENT(DestructibleCalcStaticChunkBenefit)
DEFINE_EVENT(DestructibleGatherNonessentialChunks)
DEFINE_EVENT(DestructibleCalculateBenefit)
DEFINE_EVENT(DestructibleGetChunkGlobalPose)
DEFINE_EVENT(GrbScene_fetchResults)
DEFINE_EVENT(GrbScene_syncPhysXsdk)
DEFINE_EVENT(GrbScene_createActor)
DEFINE_EVENT(GrbScene_releaseActor)
DEFINE_EVENT(GrbScene_simulate)
DEFINE_EVENT(GrbContactReport_onContactNotify)
