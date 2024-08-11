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

#include "NxApexDefs.h"
#if defined(APEX_CUDA_SUPPORT) //&& !defined(__CUDACC__)

#include "ApexCudaProfile.h"
#include "ApexCudaWrapper.h"
#include "PsFile.h"
#include <cuda.h>
#include "NiModule.h"
#include "ApexSDKHelpers.h"

namespace physx
{
namespace apex
{

	ApexCudaProfileSession::ApexCudaProfileSession()
		: mTimer(NULL)
		, mFrameStart(PX_MAX_F32)
		, mFrameFinish(0.f)
	{
		mMemBuf.setEndianMode(PxMemoryBuffer::ENDIAN_LITTLE);
	}
	ApexCudaProfileSession::~ApexCudaProfileSession()
	{
		if (mTimer)
		{
		//	CUT_SAFE_CALL(cuEventDestroy((CUevent)mTimer));
		}
	}

	void ApexCudaProfileSession::nextFrame()
	{
		mFrameStart = PX_MAX_F32;
		mFrameFinish = 0.f;
		PxF32 sumElapsed = 0.f;
		for (PxU32 i = 0; i < mProfileDataList.size(); i++)
		{
			sumElapsed += flushProfileInfo(mProfileDataList[i]);
		}
		
		// Write frame as fictive event
		PxU32 op = 1, id = 0;
		PxU64 start = static_cast<PxU64>(mFrameStart * mManager->mTimeFormat);
		mMemBuf.write(&op, sizeof(op));
		mMemBuf.write(&start, sizeof(start));
		mMemBuf.write(&id, sizeof(id));

		op = 2;
		PxU64 stop = static_cast<PxU64>(mFrameFinish * mManager->mTimeFormat);
		mMemBuf.write(&op, sizeof(op));
		mMemBuf.write(&stop, sizeof(stop));
		mMemBuf.write(&id, sizeof(id));

		// Write summary of elapsed gpu kernel time as event
		op = 1, id = 1;
		start = static_cast<PxU64>(mFrameStart * mManager->mTimeFormat);
		mMemBuf.write(&op, sizeof(op));
		mMemBuf.write(&start, sizeof(start));
		mMemBuf.write(&id, sizeof(id));

		op = 2;
		stop = static_cast<PxU64>((mFrameStart + sumElapsed) * mManager->mTimeFormat);
		mMemBuf.write(&op, sizeof(op));
		mMemBuf.write(&stop, sizeof(stop));
		mMemBuf.write(&id, sizeof(id));

		mProfileDataList.clear();
	}
	
	void ApexCudaProfileSession::start()
	{
		if (!mManager || !mManager->mApexScene) return;
		
		mLock.lock();

		mMemBuf.seekWrite(0);
		PxU32 op = 0, sz, id = 0;
		const char* frameEvent = "Frame"; sz = sizeof(frameEvent);
		mMemBuf.write(&op, sizeof(op));
		mMemBuf.write(&sz, sizeof(sz));
		mMemBuf.write(frameEvent, sz);
		mMemBuf.write(&id, sizeof(id));
		
		const char* summaryElapsed = "Summary of elapsed time"; sz = sizeof(summaryElapsed);
		id = 1;
		mMemBuf.write(&op, sizeof(op));
		mMemBuf.write(&sz, sizeof(sz));
		mMemBuf.write(summaryElapsed, sz);
		mMemBuf.write(&id, sizeof(id));

		//Register kernels
		for (PxU32 i = 0; i < mManager->mKernels.size(); i++)
		{
			ApexCudaProfileManager::KernelInfo& ki = mManager->mKernels[i];
			sz = ki.functionName.size();
			mMemBuf.write(&op, sizeof(op));
			mMemBuf.write(&sz, sizeof(sz));
			mMemBuf.write(ki.functionName.c_str(), sz);
			mMemBuf.write(&ki.id, sizeof(ki.id));
			
			NiModuleScene* moduleScene = mManager->mApexScene->getNiModuleScene(ki.moduleName.c_str());
			ApexCudaObj* obj = NULL;
			if (moduleScene)
			{
				obj = static_cast<ApexCudaObj*>(moduleScene->getHeadCudaObj());
			}
			while(obj)
			{
				if (obj->getType() == ApexCudaObj::FUNCTION)				
				{				
					if (ApexSimpleString(DYNAMIC_CAST(ApexCudaFunc*)(obj)->getName()) == ki.functionName)
					{
						DYNAMIC_CAST(ApexCudaFunc*)(obj)->setProfileSession(this);
						break;
					}
				}
				obj = obj->next();
			}
		}
		//Run timer
		//CUevent timer;
		//if (mTimer == NULL)
		//{
		//	//CUT_SAFE_CALL(cuEventCreate(&timer, CU_EVENT_DEFAULT));
		//	//mTimer = timer;
		//}
		//CUT_SAFE_CALL(cuEventRecord((CUevent)mTimer, 0));
		
		mLock.unlock();
	}

	PxU32 ApexCudaProfileSession::getProfileId(const char* name, const char* moduleName)
	{
		Array <ApexCudaProfileManager::KernelInfo>::Iterator it 
			= mManager->mKernels.find(ApexCudaProfileManager::KernelInfo(name, moduleName));
		if (it != mManager->mKernels.end())
		{
			return it->id;
		}
		return 0;
	}

	void ApexCudaProfileSession::onFuncStart(PxU32 id, void* stream)
	{
		PX_UNUSED(id);
		PX_UNUSED(stream);
		//mLock.lock();
		//CUevent start;
		//CUevent stop;

		////CUT_SAFE_CALL(cuEventCreate(&start, CU_EVENT_DEFAULT));
		////CUT_SAFE_CALL(cuEventCreate(&stop, CU_EVENT_DEFAULT));

		////CUT_SAFE_CALL(cuEventRecord(start, (CUstream)stream));

		//ProfileData data;
		//data.id = id;
		//data.start = start;
		//data.stop = stop;
		//mProfileDataList.pushBack(data);
		
	}
	void ApexCudaProfileSession::onFuncFinish(PxU32 id, void* stream)
	{
		PX_UNUSED(id);
		PX_UNUSED(stream);
		/*ProfileData& data = mProfileDataList.back();
		PX_ASSERT(data.id == id);

		CUT_SAFE_CALL(cuEventRecord((CUevent)data.stop, (CUstream)stream));

		mLock.unlock();*/
	}

	PxF32 ApexCudaProfileSession::flushProfileInfo(ProfileData& pd)
	{
		PX_UNUSED(pd);
		/*CUevent start = (CUevent)pd.start;
		CUevent stop = (CUevent)pd.stop;

		PxU32 op = 1;
		PxF32 startTf = 0.f, stopTf = 0.f;
		PxU64 startT = 0, stopT = 0;
		CUT_SAFE_CALL(cuEventSynchronize(start));		
		CUT_SAFE_CALL(cuEventElapsedTime(&startTf, (CUevent)mTimer, start));		
		startT = static_cast<PxU64>(startTf * mManager->mTimeFormat) ;
		mMemBuf.write(&op, sizeof(op));
		mMemBuf.write(&startT, sizeof(startT));
		mMemBuf.write(&pd.id, sizeof(pd.id));

		op = 2;
		CUT_SAFE_CALL(cuEventSynchronize((CUevent)stop));
		CUT_SAFE_CALL(cuEventElapsedTime(&stopTf, (CUevent)mTimer, (CUevent)stop));
		stopT = static_cast<PxU64>(stopTf * mManager->mTimeFormat);
		mMemBuf.write(&op, sizeof(op));
		mMemBuf.write(&stopT, sizeof(stopT));
		mMemBuf.write(&pd.id, sizeof(pd.id));

		CUT_SAFE_CALL(cuEventDestroy((CUevent)start));
		CUT_SAFE_CALL(cuEventDestroy((CUevent)stop));

		mFrameStart = physx::PxMin(mFrameStart, startTf);
		mFrameFinish = physx::PxMax(mFrameFinish, stopTf);
		return stopTf - startTf;*/
		return 0;
	}

	bool ApexCudaProfileSession::stopAndSave()
	{
		if (!mManager || !mManager->mApexScene) return false;

		//unregister functions
		for (PxU32 i = 0; i < mManager->mKernels.size(); i++)
		{
			ApexCudaProfileManager::KernelInfo& ki = mManager->mKernels[i];
			
			NiModuleScene* moduleScene = mManager->mApexScene->getNiModuleScene(ki.moduleName.c_str());
			ApexCudaObj* obj = NULL;
			if (moduleScene)
			{
				obj = static_cast<ApexCudaObj*>(moduleScene->getHeadCudaObj());
			}
			while(obj)
			{
				if (obj->getType() == ApexCudaObj::FUNCTION)				
				{				
					if (ApexSimpleString(DYNAMIC_CAST(ApexCudaFunc*)(obj)->getName()) == ki.functionName)
					{
						DYNAMIC_CAST(ApexCudaFunc*)(obj)->setProfileSession(NULL);
						break;
					}
				}
				obj = obj->next();
			}
		}

		//save to file
		ApexSimpleString path(mManager->mPath);
		path += ApexSimpleString("profileSesion_");
		path += ApexSimpleString(mManager->mSessionCount, 3);
		FILE* saveFile;
		physx::shdfnd::fopen_s(&saveFile,path.c_str(),"wb");
		if (saveFile)
		{
			fwrite(mMemBuf.getWriteBuffer(), mMemBuf.getWriteBufferSize(), 1, saveFile);
			return !fclose(saveFile);
		}
		return false;
	}

	ApexCudaProfileManager::ApexCudaProfileManager()
		: mState(false)
		, mTimeFormat(NANOSECOND)
		, mSessionCount(0)
		, mReservedId(2)
	{
		mSession.init(this);
	}

	ApexCudaProfileManager::~ApexCudaProfileManager()
	{
	}

	void ApexCudaProfileManager::setKernel(const char* functionName, const char* moduleName)
	{
		if (mKernels.find(KernelInfo(functionName, moduleName)) == mKernels.end())
		{
			if (ApexSimpleString(functionName) == "*")
			{
				//Add all function registered in module
				NiModuleScene* moduleScene = mApexScene->getNiModuleScene(moduleName);
				ApexCudaObj* obj = NULL;
				if (moduleScene)
				{
					obj = static_cast<ApexCudaObj*>(moduleScene->getHeadCudaObj());
				}
				while(obj)
				{
					if (obj->getType() == ApexCudaObj::FUNCTION)				
					{
						const char* name = DYNAMIC_CAST(ApexCudaFunc*)(obj)->getName();
						if (mKernels.find(KernelInfo(name, moduleName)) == mKernels.end())
						{
							mKernels.pushBack(KernelInfo(name, moduleName, mKernels.size() + mReservedId));
						}
					}
					obj = obj->next();
				}
			}
			else
			{
				mKernels.pushBack(KernelInfo(functionName, moduleName, mKernels.size() + mReservedId));
			}
			enable(false);
		}
	}

	void ApexCudaProfileManager::enable(bool state)
	{
		if (state != mState)
		{
			if (state)
			{
				mSession.start();
				mSessionCount++;
			}
			else
			{
				mSession.stopAndSave();
			}
		}
		mState = state;
	}

	void ApexCudaProfileManager::nextFrame()
	{
		if (mApexScene && mState)
		{
			mSession.nextFrame();
		}
	}
}
} // namespace physx::apex

#endif
