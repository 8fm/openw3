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


#ifndef NP_JOINTCONSTRAINT_H
#define NP_JOINTCONSTRAINT_H

#include "PsUserAllocated.h"
#include "PsUtilities.h"
#include "PxConstraint.h"
#include "PxConstraintExt.h"
#include "PxJoint.h"
#include "PxD6Joint.h"
#include "PxRigidDynamic.h"
#include "PxRigidStatic.h"
#include "PxDeletionListener.h"
#include "ExtVisualDebugger.h"
#include "PxMetaData.h"
#include "CmRenderOutput.h"
#include "PxPhysics.h"

#if PX_SUPPORT_VISUAL_DEBUGGER
#include "PxScene.h"
#include "PxVisualDebugger.h"
#endif

// PX_SERIALIZATION

namespace physx
{

PxConstraint* resolveConstraintPtr(PxDeserializationContext& v,
								   PxConstraint* old, 
								   PxConstraintConnector* connector,
								   PxConstraintShaderTable& shaders);

// ~PX_SERIALIZATION

namespace Ext
{
	struct JointData
	{	
							PxTransform					c2b[2];		
							PxConstraintInvMassScale	invMassScale;
	};

	template <class Base, class ValueStruct>
	class Joint : public Base, 
				  public PxConstraintConnector, 
				  public Ps::UserAllocated
	{
  
    public:
// PX_SERIALIZATION
						Joint(PxBaseFlags baseFlags) : Base(baseFlags) {}
		virtual	void	requires(PxProcessPxBaseCallback& c)
		{			
			c.process(*mPxConstraint);
			
			{
				PxRigidActor* a0 = NULL;
				PxRigidActor* a1 = NULL;
				mPxConstraint->getActors(a0,a1);
				
				if (a0)
				{
					c.process(*a0);
				}
				if (a1)
				{
					c.process(*a1);
				}
			}
		}	
//~PX_SERIALIZATION
		
#if PX_SUPPORT_VISUAL_DEBUGGER

		virtual bool updatePvdProperties(physx::debugger::comm::PvdDataStream& pvdConnection, const PxConstraint* c, PxPvdUpdateType::Enum updateType) const
		{
			if(updateType == PxPvdUpdateType::UPDATE_SIM_PROPERTIES)
			{
				Ext::VisualDebugger::simUpdate<Base>(pvdConnection, *this);
				return true;
			}
			else if(updateType == PxPvdUpdateType::UPDATE_ALL_PROPERTIES)
			{
				Ext::VisualDebugger::updatePvdProperties<Base, ValueStruct>(pvdConnection, *this);
				return true;
			}
			else if(updateType == PxPvdUpdateType::CREATE_INSTANCE)
			{
				Ext::VisualDebugger::createPvdInstance<Base>(pvdConnection, *c, *this);
				return true;
			}
			else if(updateType == PxPvdUpdateType::RELEASE_INSTANCE)
			{
				Ext::VisualDebugger::releasePvdInstance(pvdConnection, *c, *this);
				return true;
			}
			return false;
		}
#else
		virtual bool updatePvdProperties(physx::debugger::comm::PvdDataStream&, const PxConstraint*, PxPvdUpdateType::Enum) const
		{
			return false;
		}
#endif



		void getActors(PxRigidActor*& actor0, PxRigidActor*& actor1)	const		
		{	
			if ( mPxConstraint ) mPxConstraint->getActors(actor0,actor1);
			else
			{
				actor0 = NULL;
				actor1 = NULL;
			}
		}
		
		void setActors(PxRigidActor* actor0, PxRigidActor* actor1)
		{	
			//TODO SDK-DEV
			//You can get the debugger stream from the NpScene
			//Ext::VisualDebugger::setActors( stream, this, mPxConstraint, actor0, actor1 );
			PX_CHECK_AND_RETURN(actor0 != actor1, "PxJoint::setActors: actors must be different");

#if PX_SUPPORT_VISUAL_DEBUGGER
			PxScene* scene = getScene();
			PxVisualDebugger* debugger = scene ? scene->getPhysics().getVisualDebugger() : NULL;
			if( debugger != NULL)
			{
				debugger::comm::PvdDataStream* conn = debugger->getPvdDataStream(*scene);
				if( conn != NULL )
					Ext::VisualDebugger::setActors(
					*conn,
					*this,
					*mPxConstraint,
					actor0, 
					actor1
					);
			}
#endif

			mPxConstraint->setActors(actor0, actor1);
			mData->c2b[0] = getCom(actor0).transformInv(mLocalPose[0]);
			mData->c2b[1] = getCom(actor1).transformInv(mLocalPose[1]);
			mPxConstraint->markDirty();
		}

		// this is the local pose relative to the actor, and we store internally the local
		// pose relative to the body 

		PxTransform getLocalPose(PxJointActorIndex::Enum actor) const
		{	
			return mLocalPose[actor];
		}
		
		void setLocalPose(PxJointActorIndex::Enum actor, const PxTransform& pose)
		{
			PX_CHECK_AND_RETURN(pose.isSane(), "PxJoint::setLocalPose: transform is invalid");
			PxTransform p = pose.getNormalized();
			mLocalPose[actor] = p;
			mData->c2b[actor] = getCom(actor).transformInv(p); 
			mPxConstraint->markDirty();
		}

		PxTransform			getBodyPose(const PxRigidActor* actor) const
		{
			if(!actor)
				return PxTransform(PxIdentity);
			else if(actor->is<PxRigidStatic>())
				return actor->getGlobalPose();
			else
				return actor->getGlobalPose() * static_cast<const PxRigidBody*>(actor)->getCMassLocalPose();
		}


		void getActorVelocity(const PxRigidActor* actor, PxVec3& linear, PxVec3& angular) const
		{
			if(!actor || actor->is<PxRigidStatic>())
			{
				linear = angular = PxVec3(0);
				return;
			}
			
			linear = static_cast<const PxRigidBody*>(actor)->getLinearVelocity();
			angular = static_cast<const PxRigidBody*>(actor)->getAngularVelocity();
		}


		PxTransform			getRelativeTransform()					const
		{
			PxRigidActor* actor0, * actor1;
			mPxConstraint->getActors(actor0, actor1);
			PxTransform t0 = getBodyPose(actor0) * mLocalPose[0],
						t1 = getBodyPose(actor1) * mLocalPose[1];
			return t0.transformInv(t1);
		}

		PxVec3	getRelativeLinearVelocity()			const
		{
			PxRigidActor* actor0, * actor1;
			PxVec3 l0, a0, l1, a1;
			mPxConstraint->getActors(actor0, actor1);

			PxTransform t0 = getCom(actor0), t1 = getCom(actor1);
			getActorVelocity(actor0, l0, a0);
			getActorVelocity(actor1, l1, a1);

			PxVec3 p0 = t0.q.rotate(mLocalPose[0].p), 
				   p1 = t1.q.rotate(mLocalPose[1].p);
			return t0.transformInv(l1 - a1.cross(p1) - l0 + a0.cross(p0));
		}

		PxVec3				getRelativeAngularVelocity()		const
		{
			PxRigidActor* actor0, * actor1;
			PxVec3 l0, a0, l1, a1;
			mPxConstraint->getActors(actor0, actor1);

			PxTransform t0 = getCom(actor0);
			getActorVelocity(actor0, l0, a0);
			getActorVelocity(actor1, l1, a1);

			return t0.transformInv(a1 - a0);
		}


		void getBreakForce(PxReal& force, PxReal& torque)	const
		{
			mPxConstraint->getBreakForce(force,torque);
		}

		void setBreakForce(PxReal force, PxReal torque)
		{
			mPxConstraint->setBreakForce(force,torque);
		}


		PxConstraintFlags getConstraintFlags()									const
		{
			return mPxConstraint->getFlags();
		}

		void setConstraintFlags(PxConstraintFlags flags)
		{
			mPxConstraint->setFlags(flags);
		}

		void setConstraintFlag(PxConstraintFlag::Enum flag, bool value)
		{
			mPxConstraint->setFlag(flag, value);
		}

		void setInvMassScale0(PxReal invMassScale)
		{
			PX_CHECK_AND_RETURN(PxIsFinite(invMassScale) && invMassScale>=0, "PxJoint::setInvMassScale0: scale must be non-negative");
			mData->invMassScale.linear0 = invMassScale;
			mPxConstraint->markDirty();
		}

		PxReal getInvMassScale0() const
		{
			return mData->invMassScale.linear0;
		}

		void setInvInertiaScale0(PxReal invInertiaScale)
		{
			PX_CHECK_AND_RETURN(PxIsFinite(invInertiaScale) && invInertiaScale>=0, "PxJoint::setInvInertiaScale0: scale must be non-negative");
			mData->invMassScale.angular0 = invInertiaScale;
			mPxConstraint->markDirty();
		}

		PxReal getInvInertiaScale0() const
		{
			return mData->invMassScale.angular0;
		}

		void setInvMassScale1(PxReal invMassScale)
		{
			PX_CHECK_AND_RETURN(PxIsFinite(invMassScale) && invMassScale>=0, "PxJoint::setInvMassScale1: scale must be non-negative");
			mData->invMassScale.linear1 = invMassScale;
			mPxConstraint->markDirty();
		}

		PxReal getInvMassScale1() const
		{
			return mData->invMassScale.linear1;
		}

		void setInvInertiaScale1(PxReal invInertiaScale)
		{
			PX_CHECK_AND_RETURN(PxIsFinite(invInertiaScale) && invInertiaScale>=0, "PxJoint::setInvInertiaScale: scale must be non-negative");
			mData->invMassScale.angular1 = invInertiaScale;
			mPxConstraint->markDirty();
	}

		PxReal getInvInertiaScale1() const
		{
			return mData->invMassScale.angular1;
		}

		const char* getName() const
		{
			return mName;
		}

		void setName(const char* name)
		{
			mName = name;
		}

		void onComShift(PxU32 actor)
		{
			mData->c2b[actor] = getCom(actor).transformInv(mLocalPose[actor]); 
			markDirty();
		}

		void onOriginShift(const PxVec3& shift)
		{
			PxRigidActor* a[2];
			mPxConstraint->getActors(a[0], a[1]);

			if (!a[0])
			{
				mLocalPose[0].p -= shift;
				mData->c2b[0].p -= shift;
				markDirty();
			}
			else if (!a[1])
			{
				mLocalPose[1].p -= shift;
				mData->c2b[1].p -= shift;
				markDirty();
			}
		}

		void* prepareData()
		{
			return mData;
		}

		PxJoint* getPxJoint()
		{
			return this;
		}

		void* getExternalReference(PxU32& typeID)
		{
			typeID = PxConstraintExtIDs::eJOINT;
			return static_cast<PxJoint*>( this );
		}

		PxConstraintConnector* getConnector()
		{
			return this;
		}

		PX_INLINE void setPxConstraint(PxConstraint* pxConstraint)
		{
			mPxConstraint = pxConstraint;
		}

		PX_INLINE PxConstraint* getPxConstraint()
		{
			return mPxConstraint;
		}

		PX_INLINE const PxConstraint* getPxConstraint() const
		{
			return mPxConstraint;
		}

		void release()
		{
			mPxConstraint->release();
		}

		PxBase* getSerializable()
		{
			return this;
		}

		void onConstraintRelease()
		{
			PX_FREE_AND_RESET(mData);
			delete this;
		}

		PxScene* getScene() const
		{
			return mPxConstraint ? mPxConstraint->getScene() : NULL;
		}

	private:
		PxConstraint* getConstraint() const { return mPxConstraint; }

	protected:
		
		PxTransform getCom(PxU32 index) const
		{
			PxRigidActor* a[2];
			mPxConstraint->getActors(a[0],a[1]);
			return getCom(a[index]);
		}

		PxTransform getCom(PxRigidActor* actor) const
		{
			if (!actor)
				return PxTransform(PxIdentity);
			else if (actor->getType() == PxActorType::eRIGID_DYNAMIC || actor->getType() == PxActorType::eARTICULATION_LINK)
				return static_cast<PxRigidBody*>(actor)->getCMassLocalPose();
			else
			{
				PX_ASSERT(actor->getType() == PxActorType::eRIGID_STATIC);
				return static_cast<PxRigidStatic*>(actor)->getGlobalPose().getInverse();
			}
		}

		void initCommonData(JointData& data,
							PxRigidActor* actor0, const PxTransform& localFrame0, 
							PxRigidActor* actor1, const PxTransform& localFrame1)
		{
			mLocalPose[0] = localFrame0.getNormalized();
			mLocalPose[1] = localFrame1.getNormalized();
			data.c2b[0] = getCom(actor0).transformInv(localFrame0);
			data.c2b[1] = getCom(actor1).transformInv(localFrame1);
			data.invMassScale.linear0 = 1.0f;
			data.invMassScale.angular0 = 1.0f;
			data.invMassScale.linear1 = 1.0f;
			data.invMassScale.angular1 = 1.0f;
		}


		Joint(PxType concreteType, PxBaseFlags baseFlags)
		: Base(concreteType, baseFlags)
		, mName(NULL)
		, mPxConstraint(0)
		{
			Base::userData = NULL;
		}


		void markDirty()
		{ 
			mPxConstraint->markDirty();
		}

		const char*						mName;
		PxTransform						mLocalPose[2];
		PxConstraint*					mPxConstraint;
		JointData*						mData;
	};

} // namespace Ext

}

#endif
