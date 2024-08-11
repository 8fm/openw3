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


#ifndef PX_PHYSICS_COMMON_TASK
#define PX_PHYSICS_COMMON_TASK

#include "PxTask.h"
#include "PxTaskManager.h"
#include "CmPhysXCommon.h"
#include "PsUserAllocated.h"
#include "PsAtomic.h"
#include "PsMutex.h"

namespace physx
{
namespace Cm
{
	template <class T, void (T::*Fn)(PxBaseTask*) >
	class DelegateTask : public PxLightCpuTask, public shdfnd::UserAllocated
	{
	public:

		DelegateTask(T* obj, const char* name) : 
		  mObj(obj), mName(name) { }

		virtual void run()
		{
			(mObj->*Fn)((PxBaseTask*)mCont);
		}

		virtual const char* getName() const
		{
			return mName;
		}

		void setObject(T* obj) { mObj = obj; }

	private:
		T* mObj;
		const char* mName;
	};


	/**
	\brief A task that maintains a list of dependent tasks.
	
	The task still supports a continuation task with the same functionality as PxLightCpuTask.
	Additionally the task maintains a list of dependent tasks that have their reference counts 
	reduced on completion of the task.
	*/
	class FanoutTask : public PxLightCpuTask
	{
	public:
		FanoutTask(const char* name) : PxLightCpuTask(), mName(name) {}

		virtual void run() 
		{}

		virtual const char* getName() const
		{
			return mName;
		}
		
		/**
		Adds a dependent task. If the task is still uninitialized this function performs the same 
		as PxLightCpuTask::setContinuation, setting the continuation task and the task manager.
		*/
		PX_INLINE void addDependent(PxBaseTask& dependent)
		{
			if (!physx::shdfnd::atomicCompareExchange(&mRefCount, 1, 0))
			{
				mCont = &dependent;
				mTm = mCont->getTaskManager();
			}
			else
			{
				Ps::Mutex::ScopedLock lock(mMutex);
				mDependents.pushBack(&dependent);
			}
			dependent.addReference();
		}

		/**
		Reduces reference counts of the continuation task and the dependent tasks, also 
		clearing the dependents task list.

		The reference update and clearing of the dependent tasks are in a critical section
		to avoid other worker threads reusing the task concurrently accessing the mDependents list
		before it is cleared.
		*/
		PX_INLINE void release()
		{
			PxLightCpuTask::release();

			if (mDependents.size())
			{
				Ps::Mutex::ScopedLock lock(mMutex);

				for (PxU32 i = 0; i < mDependents.size(); i++)
					mDependents[i]->removeReference();

				mDependents.clear();
			}
		}

	protected:
		const char* mName;
		Ps::InlineArray<PxBaseTask*, 4> mDependents;
		Ps::Mutex mMutex; //Mutex necessary to protect parallel access to the mDependents task list. 
	};


	/**
	\brief Specialization of FanoutTask class in order to provide the delegation mechanism.
	*/
	template <class T, void (T::*Fn)(PxBaseTask*) >
	class DelegateFanoutTask : public FanoutTask, public shdfnd::UserAllocated
	{
	public:

		DelegateFanoutTask(T* obj, const char* name) : 
		  FanoutTask(name), mObj(obj) { }

		  virtual void run()
		  {
			  (mObj->*Fn)((PxBaseTask*)mCont);
		  }

		  void setObject(T* obj) { mObj = obj; }

	private:
		T* mObj;
	};

} // namespace Cm

}

#endif
