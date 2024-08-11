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

#ifndef __APEX_CUDA_KERNEL_MANAGER__
#define __APEX_CUDA_KERNEL_MANAGER__

#include "NxApexDefs.h"
#include "NxApexCudaProfileManager.h"

#include "PxMemoryBuffer.h"
#include "ApexString.h"
#include "NiApexScene.h"

namespace physx
{
namespace apex
{

class ApexCudaObj;
class ApexCudaFunc;
class ApexCudaProfileManager;

class ApexCudaProfileSession
{
	struct ProfileData
	{
		PxU32 id;
		void* start;
		void* stop;
	};
public:
	ApexCudaProfileSession();
	~ApexCudaProfileSession();

	PX_INLINE void	init(ApexCudaProfileManager* manager)
	{
		mManager = manager;
	}
	void		nextFrame();
	void		start();
	bool		stopAndSave();
	PxU32		getProfileId(const char* name, const char* moduleName);

	void		onFuncStart(PxU32 id, void* stream);
	void		onFuncFinish(PxU32 id, void* stream);

protected:
	PxF32		flushProfileInfo(ProfileData& pd);

	ApexCudaProfileManager* mManager;
	void*			mTimer;
	PxMemoryBuffer	mMemBuf;
	physx::Mutex	mLock;
	Array <ProfileData> mProfileDataList;
	physx::PxF32	mFrameStart;
	physx::PxF32	mFrameFinish;
};

/** 
 */
class ApexCudaProfileManager : public NxApexCudaProfileManager
{
public:
	struct KernelInfo
	{
		ApexSimpleString functionName;
		ApexSimpleString moduleName;
		PxU32 id;
		
		KernelInfo(const char* functionName, const char* moduleName, PxU32 id = 0) 
			: functionName(functionName), moduleName(moduleName), id(id) {}

		bool operator!= (const KernelInfo& ki)
		{
			return		(this->functionName != "*" && this->functionName != ki.functionName) 
					|| 	(this->moduleName != ki.moduleName);
		}
	};

	ApexCudaProfileManager();

	virtual ~ApexCudaProfileManager();

	PX_INLINE void setNiApexScene(NiApexScene* scene)
	{
		mApexScene = scene;
	}
	void nextFrame();

	// interface for NxApexCudaProfileManager
	PX_INLINE void setPath(const char* path)
	{
		mPath = ApexSimpleString(path);
		enable(false);
	}
	void setKernel(const char* functionName, const char* moduleName);
	PX_INLINE void setTimeFormat(TimeFormat tf)
	{
		mTimeFormat = tf;
	}
	void enable(bool state);
	PX_INLINE bool isEnabled() const
	{
		return mState;
	}
		
private:
	bool			mState;
	PxU32			mSessionCount;
	TimeFormat		mTimeFormat;
	PxU32			mReservedId;
	ApexSimpleString			mPath;
	Array <KernelInfo>			mKernels;
	ApexCudaProfileSession		mSession;
	NiApexScene*	mApexScene;
	friend class ApexCudaProfileSession;
};

}
} // namespace physx::apex

#endif // __APEX_CUDA_KERNEL_MANAGER__
