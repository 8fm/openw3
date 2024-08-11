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

#ifndef SQ_PRUNINGPOOL_H
#define SQ_PRUNINGPOOL_H

#include "SqPruner.h"

namespace physx
{
namespace Sq
{
	class PruningPool 
	{

		public:

							PruningPool();
#if __SPU__
			PX_FORCE_INLINE ~PruningPool() {}
#else
			virtual			~PruningPool();
#endif

			virtual	const PrunerPayload&	getPayload(const PrunerHandle& h) const { return mObjects[getIndex(h)]; }	
			virtual void					shiftOrigin(const PxVec3& shift);

					PrunerHandle			addObject(const PxBounds3& worldAABB, const PrunerPayload& payload);
					PxU32					removeObject(PrunerHandle h);
				PX_FORCE_INLINE void		updateObject(PrunerHandle h, const PxBounds3& worldAABB)
											{
												mWorldBoxes[getIndex(h)] = worldAABB;
											}

		// Data access
		PX_FORCE_INLINE	PxU32				getIndex(PrunerHandle h)const	{ return mHandleToIndex[h];	}
		PX_FORCE_INLINE	PrunerPayload*		getObjects()			const	{ return mObjects;		}
		PX_FORCE_INLINE	PxU32				getNbActiveObjects()	const	{ return mNbObjects;	}
		PX_FORCE_INLINE	const PxBounds3*	getCurrentWorldBoxes()	const	{ return mWorldBoxes;	}
		PX_FORCE_INLINE	PxBounds3*			getCurrentWorldBoxes()			{ return mWorldBoxes;	}

		PX_FORCE_INLINE	const PxBounds3&	getWorldAABB(PrunerHandle h) const
											{
												return getWorldAABBbyIndex(getIndex(h));
											}
						void				preallocate(PxU32 entries);
	protected:
		PX_FORCE_INLINE	const PxBounds3&	getWorldAABBbyIndex(PxU32 index) const
											{
												// return cached box
												#if __SPU__
													static PxU8 PX_ALIGN(16,worldBoxBuffer[(sizeof(PxBounds3)+31)&~15]);
													PxBounds3* worldBox = Cm::memFetchAsync<PxBounds3>(worldBoxBuffer,(Cm::MemFetchPtr)&mWorldBoxes[index],sizeof(PxBounds3),1);
													Cm::memFetchWait(1);
													return *worldBox;
												#else
													return mWorldBoxes[index];
												#endif
											}


				PxU32						mNbObjects;		//!< Current number of objects
				PxU32						mMaxNbObjects;	//!< Max. number of objects

				PxBounds3*					mWorldBoxes;	//!< List of world boxes
				PrunerPayload*				mObjects;		//!< List of objects

	private:			
				PxU32*						mHandleToIndex;
				PxU32*						mIndexToHandle;
				PxU32						mFirstFreshHandle;
				PxU32						mHandleFreeList;

				void						resize(PxU32 newCapacity);


	};

} // namespace Sq

}

#endif // SQ_PRUNINGPOOL_H
