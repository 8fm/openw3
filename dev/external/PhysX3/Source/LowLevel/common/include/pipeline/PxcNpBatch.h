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
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  



#ifndef PXC_NP_BATCH_H
#define PXC_NP_BATCH_H

#include "PxvConfig.h"
#include "PxGeometry.h"
#include "PxcNpWorkUnit.h"

namespace physx
{

struct PxcNpWorkUnit;
class PxsThreadContext;
class PxsContactManager;

namespace Cm
{
	class FlushPool;
}

class PxLightCpuTask;


void PxcDiscreteNarrowPhase(PxcNpThreadContext& context, PxcNpWorkUnit& n);
void PxcDiscreteNarrowPhasePCM(PxcNpThreadContext& context, PxcNpWorkUnit& n);
void PxcSkipNarrowPhase(PxcNpWorkUnit& n);

struct PxcNpBatchEntry
{
	PxcNpWorkUnit*		workUnit;
	PxU32				cmIndex;
	PxsContactManager*	cm;

	PxcNpBatchEntry()
	{}
	PxcNpBatchEntry(PxcNpWorkUnit* workUnit, PxU32 cmIndex, PxsContactManager* cm)
		: workUnit(workUnit), cmIndex(cmIndex), cm(cm)
	{}
};

void PxcRunNpBatch(const PxU32 numSpusPrim, const PxU32 numSpusCnvx, const PxU32 numSpusHF, const PxU32 numSpusMesh, const PxU32 numSpusCnvxMesh,
				   PxsThreadContext* context, PxcNpMemBlockPool& memBlockPool,
				   const PxU32 numContraintBlocks, const PxU32 numFrictionBlocks,
				   PxcNpBatchEntry* entriesPrimVsPrim, PxU32 numEntriesPrimVsPrim,
				   PxcNpBatchEntry* entriesPrimOrCnvxVsCnvx, PxU32 numEntriesPrimOrCnvxVsCnvx,
				   PxcNpBatchEntry* entriesPrimOrCnvxVsHF, PxU32 numEntriesPrimOrCnvxVsHF,
				   PxcNpBatchEntry* entriesPrimVsTrimesh, PxU32 numEntriesPrimVsTrimesh,
				   PxcNpBatchEntry* entriesCvxBoxVsTrimesh, PxU32 numEntriesCvxBoxVsTrimesh,
				   PxcNpBatchEntry* entriesOther, PxU32 numEntriesOther,
				   PxU32* bitmapBase, PxU32 bitmapWordCount,
				   PxU32& touchLost, PxU32& touchFound,
				   physx::PxLightCpuTask* continuation, Cm::FlushPool& taskPool);


//void PxcRunNpPCMBatch(const PxU32 numSpusCnvx, const PxU32 numSpusHF, const PxU32 numSpusMesh, const PxU32 numSpusCnvxMesh,
//				   PxsThreadContext* context, PxcNpMemBlockPool& memBlockPool,
//				   const PxU32 numContraintBlocks, const PxU32 numFrictionBlocks,
//				   PxcNpBatchEntry* entriesPrimVsPrim, PxU32 numEntriesPrimVsPrim,
//				   PxcNpBatchEntry* entriesPrimOrCnvxVsHF, PxU32 numEntriesPrimOrCnvxVsHF,
//				   PxcNpBatchEntry* entriesPrimVsTrimesh, PxU32 numEntriesPrimVsTrimesh,
//				   PxcNpBatchEntry* entriesCvxBoxVsTrimesh, PxU32 numEntriesCvxBoxVsTrimesh,
//				   PxcNpBatchEntry* entriesOther, PxU32 numEntriesOther,
//				   PxU32* bitmapBase, PxU32 bitmapWordCount,
//				   PxU32& touchLost, PxU32& touchFound,
//				   physx::PxLightCpuTask* continuation, Cm::FlushPool& taskPool);

void PxcRunNpPCMBatch(	const PxU32 numSpusCnvx, const PxU32 numSpusHF, const PxU32 numSpusMesh,
								PxsThreadContext* context, PxcNpMemBlockPool& memBlockPool,
								const PxU32 numContactBlocks, const PxU32 numNpCacheBlocks,
								PxcNpBatchEntry* entriesPrimVsPrim, PxU32 numEntriesPrimVsPrim,
								PxcNpBatchEntry* entriesPrimOrCnvxVsHF, PxU32 numEntriesPrimOrCnvxVsHF,
								PxcNpBatchEntry* entriesPrimOrCnvxVsTrimesh, PxU32 numEntriesPrimOrCnvxVsTrimesh,
								PxcNpBatchEntry* entriesOther, PxU32 numEntriesOther,
								PxU32* changeBitmapBase, PxU32 changeBitmapWordCount,
								PxU32& touchLost, PxU32& touchFound,
								physx::PxLightCpuTask* continuation, Cm::FlushPool& taskPool);

}

#endif
