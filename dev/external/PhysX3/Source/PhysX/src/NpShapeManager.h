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


#ifndef PX_PHYSICS_NP_SHAPE_MANAGER
#define PX_PHYSICS_NP_SHAPE_MANAGER

#include "NpShape.h"
#include "CmPtrTable.h"

#if PX_ENABLE_DEBUG_VISUALIZATION
#include "CmRenderOutput.h"
#endif

namespace physx
{

namespace Sq
{
	class SceneQueryManager;
	struct ActorShape;
}

class NpShapeManager : public Ps::UserAllocated
{
public:
	static			void			getBinaryMetaData(PxOutputStream& stream);
// PX_SERIALIZATION
									NpShapeManager(const PxEMPTY&);
					void			exportExtraData(PxSerializationContext& stream);
					void			importExtraData(PxDeserializationContext& context);
//~PX_SERIALIZATION
									NpShapeManager();
									~NpShapeManager() { }

	PX_FORCE_INLINE	PxU32			getNbShapes()	const	{ return mShapes.mCount; }

	PxU32			getShapes(PxShape** buffer, PxU32 bufferSize, PxU32 startIndex=0) const;

	PX_FORCE_INLINE	NpShape* const*	getShapes()		const	{ return reinterpret_cast<NpShape*const*>(mShapes.getPtrs());	}
	PX_FORCE_INLINE	Sq::ActorShape*const *
									getSceneQueryData()	const { return reinterpret_cast<Sq::ActorShape*const*>(mSceneQueryData.getPtrs()); }

					void			attachShape(NpShape& shape, PxRigidActor& actor);
					void			detachShape(NpShape& s, PxRigidActor &actor, bool wakeOnLostTouch);
					bool			shapeIsAttached(NpShape& s) const;
					void			detachAll(NpScene *scene);

					void			teardownSceneQuery(Sq::SceneQueryManager& sqManager, const NpShape& shape);
					void			setupSceneQuery(Sq::SceneQueryManager& sqManager, const PxRigidActor& actor, const NpShape& shape);

					void			setupSceneQueryInBatch(Sq::SceneQueryManager& sqManager, const PxRigidActor& actor, const NpShape& shape, PxU32 index, bool isDynamic, PxBounds3* bounds);

					PX_FORCE_INLINE void setSceneQueryData(PxU32 index, Sq::ActorShape* data)	
					{
						// for batch insertion optimization; be careful not to violate the invariant that the ActorShape matches the shape and the shape is SQ, which we can't check here.
						// because we don't have the SQ manager
						PX_ASSERT(index<getNbShapes());
						mSceneQueryData.getPtrs()[index] = data;
					}

					void			setupAllSceneQuery(const PxRigidActor& actor);
					void			teardownAllSceneQuery(Sq::SceneQueryManager& sqManager);
					void			markAllSceneQueryForUpdate(Sq::SceneQueryManager& shapeManager);
					
					Sq::ActorShape*
									findSceneQueryData(const NpShape& shape);

					PxBounds3		getWorldBounds(const PxRigidActor&) const;

					void			clearShapesOnRelease(Scb::Scene& s, PxRigidActor&);
					void			releaseExclusiveUserReferences();

#if PX_ENABLE_DEBUG_VISUALIZATION
public:
					void			visualize(Cm::RenderOutput& out, NpScene* scene, const PxRigidActor& actor);
#endif

					// for batching
					const Cm::PtrTable&
									getShapeTable() const 		{	return mShapes; }



protected:
					Sq::ActorShape**	getSqDataInternal()	{ return reinterpret_cast<Sq::ActorShape**>(mSceneQueryData.getPtrs()); }
					void			setupSceneQuery(Sq::SceneQueryManager& sqManager, const PxRigidActor& actor, PxU32 index);
					void			teardownSceneQuery(Sq::SceneQueryManager& sqManager, PxU32 index);
					bool			checkSqActorShapeInternal();


					Cm::PtrTable	mShapes;
					Cm::PtrTable	mSceneQueryData;	// 1-1 correspondence with shapes - TODO: allocate on scene insertion or combine with the shape array for better caching
};

}

#endif
