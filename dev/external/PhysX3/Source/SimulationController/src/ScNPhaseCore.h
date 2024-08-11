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


#ifndef PX_PHYSICS_SCP_NPHASE_CORE
#define PX_PHYSICS_SCP_NPHASE_CORE

#include "CmPhysXCommon.h"
#include "CmRenderOutput.h"
#include "PxPhysXConfig.h"
#include "PsUserAllocated.h"
#include "PsMutex.h"
#include "PsAtomic.h"
#include "PxSimulationEventCallback.h"
#include "ScTriggerPairs.h"
#include "ScInteractionType.h"
#include "CmIndexedPool.h"
#include "CmFlushPool.h"
#include "ScRigidSim.h"
#include "ScRigidCore.h"
#include "ScContactReportBuffer.h"


namespace physx
{

struct PxvBroadPhaseOverlap;

namespace Sc
{
	class Scene;

	class ActorSim;
	class ElementSim;
	class ShapeSim;

	class Actor;
	class Element;
	class ElementInteraction;

#if PX_USE_PARTICLE_SYSTEM_API
	class ParticlePacketShape;
#endif

	class CoreInteraction;
	class ElementActorInteraction;
	class ElementInteractionMarker;
	class RbElementInteraction;
	class TriggerInteraction;
#if PX_USE_PARTICLE_SYSTEM_API 
	class ParticleElementRbElementInteraction;
#endif

	class ShapeInstancePairLL;
	class ActorElementPair;
	class ActorPair;

	class ActorPairContactReportData;
	struct ContactShapePair;

	class NPhaseContext;
	class ContactStreamManager;

	struct FilterPair;
	struct FilterInfo;


	struct PairReleaseFlag
	{
		enum Enum
		{
			eSECONDARY_BROADPHASE	=	(1 << 0),
			eELEMENT_DELETED		=	(1 << 1),
			eRB_SHAPE_DELETED		=	(1 << 2) | eELEMENT_DELETED,
			eWAKE_ON_LOST_TOUCH		=	(1 << 3),
		};
	};

	/*
	Description: NPhaseCore encapsulates the near phase processing to allow multiple implementations(eg threading and non
	threaded).

	The broadphase inserts shape pairs into the NPhaseCore, which are then processed into contact point streams.
	Pairs can then be processed into AxisConstraints by the GroupSolveCore.

	*/
	class NPhaseCore : public Ps::UserAllocated
	{
	public:
		NPhaseCore(Scene& scene, const PxSceneDesc& desc);
		~NPhaseCore();

		void onOverlapCreated(const PxvBroadPhaseOverlap* PX_RESTRICT pairs, PxU32 pairCount, bool secondaryBroadphase);
		void onOverlapCreated(Element* volume0, Element* volume1, bool secondaryBroadphase);
		void onOverlapRemoved(Element* volume0, Element* volume1, bool secondaryBroadphase);
		void onVolumeRemoved(Element* volume, PxU32 flags);

#if PX_USE_PARTICLE_SYSTEM_API
		// The secondaryBroadphase parameter is needed to avoid concurrent interaction updates while the gpu particle pipeline in running.
		ParticleElementRbElementInteraction* insertParticleElementRbElementPair(ParticlePacketShape* particleShape, ShapeSim* rbShape, ActorElementPair* actorElementPair, bool secondaryBroadphase);
#endif

		PxU32 getDefaultContactReportStreamBufferSize() const;

		void fireCustomFilteringCallbacks();

		void addToDirtyInteractionList(CoreInteraction* interaction);
		bool removeFromDirtyInteractionList(CoreInteraction* interaction);
		void updateDirtyInteractions();


		/*
		Description: Perform/Complete processing of shape instance pairs into contacts streams. Also
		links shape instance pairs into bodies to allow the later retrieval of the contacts.
		*/
		void narrowPhase();


		/*
		Description: Adds contacts to contactReport buffer to be fed to the user.
		*/
		void processContactNotifications(bool pendingReportsOnly);

		/*
		Description: Displays visualizations associated with the near phase.
		*/
		void visualize(Cm::RenderOutput& out);

		PX_FORCE_INLINE Scene& getScene() const	{ return mOwnerScene;	}

		/*
		Description: Generate contacts for a specific shape instance pair.

		Threading: Should be thread safe as it can be called from multiple threads by 
		threaded implementations.
		*/
		void findTriggerContacts(TriggerInteraction* tri, bool toBeDeleted, bool shapeDeleted);

		PX_FORCE_INLINE void addToContactReportActorPairSet(ActorPair* pair) { mContactReportActorPairSet.pushBack(pair); }
		void clearContactReportActorPairs(bool shrinkToZero);
		PX_FORCE_INLINE PxU32 getNbContactReportActorPairs() const { return mContactReportActorPairSet.size(); }
		PX_FORCE_INLINE ActorPair* const* getContactReportActorPairs() const { return mContactReportActorPairSet.begin(); }

		void addToPersistentContactEventPairs(ShapeInstancePairLL*);
		void removeFromPersistentContactEventPairs(ShapeInstancePairLL* sip);
		PX_FORCE_INLINE PxU32 getCurrentPersistentContactEventPairCount() const { return mNextFramePersistentContactEventPairIndex; }
		PX_FORCE_INLINE ShapeInstancePairLL* const* getCurrentPersistentContactEventPairs() const { return mPersistentContactEventPairList.begin(); }
		PX_FORCE_INLINE PxU32 getAllPersistentContactEventPairCount() const { return mPersistentContactEventPairList.size(); }
		PX_FORCE_INLINE ShapeInstancePairLL* const* getAllPersistentContactEventPairs() const { return mPersistentContactEventPairList.begin(); }

		void addToForceThresholdContactEventPairs(ShapeInstancePairLL*);
		void removeFromForceThresholdContactEventPairs(ShapeInstancePairLL*);
		PX_FORCE_INLINE PxU32 getForceThresholdContactEventPairCount() const { return mForceThresholdContactEventPairList.size(); }
		PX_FORCE_INLINE ShapeInstancePairLL* const* getForceThresholdContactEventPairs() const { return mForceThresholdContactEventPairList.begin(); }

		PX_FORCE_INLINE ContactShapePair* getContactReportShapePairs(const PxU32& bufferIndex) const { return reinterpret_cast<Sc::ContactShapePair*> (mContactReportBuffer.getData(bufferIndex)); }
		ContactShapePair* reserveContactShapePairs(PxU32 pairCount, PxU32& bufferIndex);
		ContactShapePair* resizeContactShapePairs(PxU32 pairCount, Sc::ContactStreamManager& csm);
		PX_FORCE_INLINE void clearContactReportStream() { mContactReportBuffer.reset(); }  // Do not free memory at all
		PX_FORCE_INLINE void freeContactReportStreamMemory() { mContactReportBuffer.flush(); }
		void convertDeletedShapesInContactStream(ContactShapePair* shapePairs, PxU32 pairCount);

		ActorPairContactReportData* createActorPairContactReportData();
		void releaseActorPairContactReportData(ActorPairContactReportData* data);

	private:
		ElementSimInteraction* createRbElementInteraction(ShapeSim& s0, ShapeSim& s1);
#if PX_USE_PARTICLE_SYSTEM_API
		ElementSimInteraction* createParticlePacketBodyInteraction(ParticlePacketShape& ps, ShapeSim& s, bool secondaryBroadphase);
#endif
		void releaseElementPair(ElementSimInteraction* pair, PxU32 flags);
		void releaseShapeInstancePair(ShapeInstancePairLL* pair, PxU32 flags);

		ShapeInstancePairLL* createShapeInstancePairLL(ShapeSim& s0, ShapeSim& s1, PxPairFlags pairFlags);
		TriggerInteraction* createTriggerInteraction(ShapeSim& s0, ShapeSim& s1, PxPairFlags triggerFlags);
		ElementInteractionMarker* createElementInteractionMarker(ElementSim& e0, ElementSim& e1);

		//------------- Filtering -------------

		// Filter pair tracking for filter callbacks
		FilterPair* createFilterPair();
		void deleteFilterPair(FilterPair* pair);
		FilterPair* fetchFilterPair(PxU32 pairID);
		FilterPair* fetchFilterPair(void* reference);

		ElementSimInteraction* refilterInteraction(ElementSimInteraction* pair, const FilterInfo* filterInfo);

		PX_INLINE void callPairLost(const ElementSim& e0, const ElementSim& e1, PxU32 pairID, bool objDeleted) const;
		PX_INLINE void runFilterShader(const ElementSim& e0, const ElementSim& e1,
			PxFilterObjectAttributes& attr0, PxFilterData& filterData0,
			PxFilterObjectAttributes& attr1, PxFilterData& filterData1,
			FilterInfo& filterInfo);
		PX_INLINE FilterInfo runFilter(const ElementSim& e0, const ElementSim& e1, FilterPair* filterPair);
		FilterInfo filterRbCollisionPair(const ShapeSim& s0, const ShapeSim& s1, FilterPair* filterPair);
		//-------------------------------------

		void updatePair(CoreInteraction* pair);
		ElementSimInteraction* convert(ElementSimInteraction* pair, InteractionType type, FilterInfo& filterInfo);

		ActorPair* findActorPair(ShapeSim* s0, ShapeSim* s1);

		// Pooling
#if PX_USE_PARTICLE_SYSTEM_API
		void pool_deleteParticleElementRbElementPair(ParticleElementRbElementInteraction* pair, PxU32 flags);
#endif

		Scene&											mOwnerScene;

		Ps::Array<ActorPair*>							mContactReportActorPairSet;
		Ps::Array<ShapeInstancePairLL*>					mPersistentContactEventPairList;	// Pairs which request events which do not get triggered by the sdk and thus need to be tested actively every frame.
																							// May also contain force threshold event pairs (see mForceThresholdContactEventPairList)
																							// This list is split in two, the elements in front are for the current frame, the elements at the
																							// back will get added next frame.
		PxU32											mNextFramePersistentContactEventPairIndex;  // start index of the pairs which need to get added to the persistent list for next frame

		Ps::Array<ShapeInstancePairLL*>					mForceThresholdContactEventPairList;	// Pairs which request force threshold contact events. A pair is only in this list if it does have contact.
																								// Note: If a pair additionally requests PxPairFlag::eNOTIFY_TOUCH_PERSISTS events, then it
																								// goes into mPersistentContactEventPairList instead. This allows to share the list index.

		//
		//  data layout:
		//  ContactActorPair0, ContactShapePair0_0, ContactShapePair0_1, ... ContactShapePair0_N, ContactActorPair1, ContactShapePair1_0, ...
		//
		ContactReportBuffer								mContactReportBuffer;				// Shape pair information for contact reports

		Ps::Array<CoreInteraction*>						mDirtyInteractions;
		Cm::IndexedPool<FilterPair, 32>					mFilterPairPool;

		// Pools
		Ps::Pool<ActorPair>								mActorPairPool;
		Ps::Pool<ActorElementPair>						mActorElementPairPool;
		Ps::Pool<ShapeInstancePairLL>					mLLSipPool;
		Ps::Pool<TriggerInteraction>					mTriggerPool;
		Ps::Pool<ActorPairContactReportData>			mActorPairContactReportDataPool;
		Ps::Pool<ElementInteractionMarker>				mInteractionMarkerPool;
#if PX_USE_PARTICLE_SYSTEM_API
		Ps::Pool<ParticleElementRbElementInteraction>	mParticleBodyPool;
#endif
	};

} // namespace Sc


}

#endif
