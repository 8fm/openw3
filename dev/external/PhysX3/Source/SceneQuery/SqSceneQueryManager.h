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


#ifndef PX_PHYSICS_SCENEQUERYMANAGER
#define PX_PHYSICS_SCENEQUERYMANAGER
/** \addtogroup physics 
@{ */

#include "PxSceneDesc.h" // PxPruningStructure
#include "SqPruner.h"
#include "CmBitMap.h"
#include "PsArray.h"

// threading
#include "PsSync.h"

namespace physx
{

class PxBounds3;
class NpShape;
class NpBatchQuery;

namespace Scb
{
	class Scene;
	class Actor;
	class Shape;
}

namespace Sq
{
	struct ActorShape;

	class SceneQueryManager : public Ps::UserAllocated
	{
	public:
														SceneQueryManager(Scb::Scene& scene, const PxSceneDesc& desc);
														~SceneQueryManager();

						ActorShape*						addShape(const NpShape& shape, const PxRigidActor& actor, bool dynamic, PxBounds3* bounds=NULL);
						void							removeShape(ActorShape* shapeData);
						const PrunerPayload&			getPayload(const ActorShape* shapeData) const;
						// returns true if current combination of pruners is accepted on SPU:
						bool							canRunOnSPU(const NpBatchQuery& bq) const; 
						// sets runOnPPU (fallback) accordingly for each query type
						void							fallbackToPPUByType(const NpBatchQuery& bq, bool runOnPPU[3]) const; 
						// asserts if !canRunOnSPU()
						void							blockingSPURaycastOverlapSweep(NpBatchQuery* bq, bool runOnPPU[3]); 
						void							freeSPUTasks(NpBatchQuery* bq);

	public:
		PX_FORCE_INLINE	Scb::Scene&						getScene()						const	{ return mScene;							}
						PxScene*						getPxScene()					const;
		PX_FORCE_INLINE	PxU32							getDynamicTreeRebuildRateHint()	const	{ return mRebuildRateHint;					}

		PX_FORCE_INLINE	PxPruningStructure::Enum		getStaticStructure()			const	{ return mPrunerType[0];					}
		PX_FORCE_INLINE	PxPruningStructure::Enum		getDynamicStructure()			const	{ return mPrunerType[1];			}

		PX_FORCE_INLINE	const Pruner*					getStaticPruner()				const	{ return mPruners[0];						}
		PX_FORCE_INLINE	const Pruner*					getDynamicPruner()				const	{ return mPruners[1];						}

		PX_FORCE_INLINE void							invalidateStaticTimestamp()				{ mTimestamp[0]++;	}
		PX_FORCE_INLINE void							invalidateDynamicTimestamp()			{ mTimestamp[1]++;	}
		PX_FORCE_INLINE PxU32							getStaticTimestamp()			const	{ return mTimestamp[0];	}
		PX_FORCE_INLINE PxU32							getDynamicTimestamp()			const	{ return mTimestamp[1];	}

						void							preallocate(PxU32 staticShapes, PxU32 dynamicShapes);
						void							markForUpdate(Sq::ActorShape* s);
						void							setDynamicTreeRebuildRateHint(PxU32 dynTreeRebuildRateHint);
						
						void							flushUpdates();
						void							forceDynamicTreeRebuild(bool rebuildStaticStructure, bool rebuildDynamicStructure);

		// Force a rebuild of the aabb/loose octree etc to allow raycasting on multiple threads.
						void							processSimUpdates();

						void							shiftOrigin(const PxVec3& shift);

	private:
						Pruner*							mPruners[2];	// 0 = static, 1 = dynamic
						PxU32							mTimestamp[2];

						Cm::BitMap						mDirtyMap[2];
						Ps::Array<ActorShape*>			mDirtyList;

						PxPruningStructure::Enum		mPrunerType[2];
						PxU32							mRebuildRateHint;

						Scb::Scene&						mScene;

						// threading
						shdfnd::Mutex					mSceneQueryLock;

						void							flushShapes();
		PX_FORCE_INLINE	bool							updateObject(PxU32 index, PrunerHandle handle);
		PX_FORCE_INLINE void							processActiveShapes(ActorShape** PX_RESTRICT shapes, PxU32 nb);
		
		static PxU32 getPrunerIndex(const ActorShape* ref)		{ return static_cast<PxU32>(reinterpret_cast<size_t>(ref))&1;				}
		static PxU32 getPrunerHandle(const ActorShape* ref)		{ return static_cast<PxU32>(reinterpret_cast<size_t>(ref))>>2;				}

		// ensure there is always one set bit, so there is never confusion with null ptr
		static ActorShape* createRef(PxU32 index, PxU32 handle)	{ return reinterpret_cast<ActorShape*>(size_t((handle<<2) | 2 | index));	}

		static Pruner*									createPruner(PxPruningStructure::Enum type);
	};

} // namespace Sq

}

/** @} */
#endif
