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

#ifndef __APEX_CUDA_TEST__
#define __APEX_CUDA_TEST__

#include "NxApexDefs.h"
#include "NxApexCudaTestManager.h"

#include "PxMemoryBuffer.h"
#include "ApexString.h"
#include "ApexMirroredArray.h"
#include "NiApexScene.h"

namespace physx
{
namespace apex
{

struct ApexCudaMemRefBase;
class ApexCudaObj;
class ApexCudaFunc;

namespace apexCudaTest
{
struct MemRef
{
	const void*	gpuPtr;
	size_t		size;
	PxI32		dataOffset;
	PxU32		bufferOffset;	
	PxI32		fpType; // Floating point type, if 0 - not a float, else if 4 - float, else if 8 - double

	MemRef(const void* gpuPtr, size_t size, PxI32 dataOffset, PxU32 bufferOffset, int fpType = 0) 
		: gpuPtr(gpuPtr),  size(size), dataOffset(dataOffset), bufferOffset(bufferOffset), fpType(fpType) {}
};
}

/** Read cuda kernel context from specified file. Run kernel ant compare output with results from file
*/
class ApexCudaTestKernelContextReader : public UserAllocated
{
	enum KernelType
	{
		KT_SYNC,
		KT_FREE,
		KT_FREE2D,
		KT_FREE3D,
		KT_BOUND
	};
	struct Dim3
	{
		int x,y,z;
	};
public:
	ApexCudaTestKernelContextReader(const char* path, NiApexScene* scene);
	~ApexCudaTestKernelContextReader();

	bool	runKernel();

private:
	void	loadContext();
	void	loadTexRef(PxU32& memOffset);
	void	loadConstMem();
	PxU32	getParamSize();
	void	loadParam(PxU32& memOffset);

	PxMemoryBuffer*		mMemBuf;

	PxU32				mCudaObjOffset;
	PxU32				mParamOffset;

	int					mCuOffset;
	void*				mCuStream;

	ApexSimpleString	mName;
	ApexSimpleString	mModuleName;
	PxU32				mFrame;
	PxU32				mCallPerFrame;
	Dim3				mBlockDim;
	Dim3				mGridDim;
	KernelType			mKernelType;
	PxU32				mThreadCount[3];
	PxU32				mBlockCountY;

	ApexCudaObj*		mHeadCudaObj;
	ApexCudaFunc*		mFunc;

	NiApexScene*		mApexScene;
	Array <PxU8*>		mArgSeq;
	ApexMirroredArray <PxU8>		mTmpArray;
	physx::PxGpuCopyDescQueue	mCopyQueue;

	Array <apexCudaTest::MemRef>	mMemRefs;
	Array <apexCudaTest::MemRef>	mOutMemRefs;
	Array <void*>		mTexRefs;
};

/** Extract context data from CudaModuleScene about cuda kernel and save it to specified file
*/
class ApexCudaTestKernelContext : public UserAllocated
{
public:
	ApexCudaTestKernelContext(const char* path, const char* functionName, const char* moduleName, PxU32 frame, PxU32 callPerFrame, bool isWriteForNonSuccessfulKernel, bool isKernelForSave);
	~ApexCudaTestKernelContext();

	bool saveToFile();

	PX_INLINE void setCuStream(void* cuStream)
	{
		mCuStream = cuStream;
	}
	PX_INLINE void setCuModule(void* cuModule)
	{
		mCuModule = cuModule;
	}
	PX_INLINE void* getCuModule()
	{
		return mCuModule;
	}

	void				setFreeKernel(int threadCount);
	void				setFreeKernel(int threadCountX, int threadCountY);
	void				setFreeKernel(int threadCountX, int threadCountY, int threadCountZ, int blockCountY);
	void				setBoundKernel(int threadCount);
	void				setSyncKernel();

	void				setBlockDim(int x, int y, int z);
	void				setGridDim(int x, int y);
	
	void				addParam(PxU32 align, void *val, size_t size, int isMemRef = 0, int dataOffset = 0, int fpType = 0);
	void				addOutMemRef(PxU32 align, const ApexCudaMemRefBase* memRef);
	void				addTexRef(	const char* name,
									const void* mem, 
									size_t size	);
	void				addConstMem(const char* name, const void* mem, size_t size);
	void				setKernelStatus();

private:
	void				copyMemRefs();

	void				completeCudaObjsBlock();
	void				completeCallParamsBlock();

	void*				mCuModule;
	void*				mCuStream;

	PxU32				mVersion;
	PxU32				mFrame;
	PxU32				mCallPerFrame;
	ApexSimpleString	mName;
	ApexSimpleString	mErrorCode;
	ApexSimpleString	mModuleName;
	ApexSimpleString	mPath;
	PxMemoryBuffer		mMemBuf;

	PxU32				mOffset;
	PxU32				mCudaObjsOffset;
	PxU32				mCallParamsOffset;
	
	PxU32				mCudaObjsCounter;
	PxU32				mCallParamsCounter;

	Array <apexCudaTest::MemRef> mMemRefs;
	Array <PxU8*>		mTmpBuffer;

	bool				mIsFirstParamCall;
	bool				mIsCompleteContext;
	bool				mIsWriteForNonSuccessfulKernel;
	bool				mIsContextForSave;
};


/** Class get information what kernels should be tested and give directive for creation ApexCudaTestContext
 */
class ApexCudaTestManager : public NxApexCudaTestManager
{
	struct KernelInfo
	{
		ApexSimpleString functionName;
		ApexSimpleString moduleName;
		PxU32 callCount;
		
		KernelInfo(const char* functionName, const char* moduleName) 
			: functionName(functionName), moduleName(moduleName), callCount(0) {}

		bool operator!= (const KernelInfo& ki)
		{
			return this->functionName != ki.functionName || this->moduleName != ki.moduleName;
		}
	};

public:
	
	ApexCudaTestManager();
	virtual ~ApexCudaTestManager();

	PX_INLINE void setNiApexScene(NiApexScene* scene)
	{
		mApexScene = scene;
	}
	void nextFrame();
	ApexCudaTestKernelContext* isTestKernel(const char* functionName, const char* moduleName);

	// interface for NxApexCudaTestManager
	PX_INLINE void setWritePath(const char* path)
	{
		mPath = ApexSimpleString(path);
	}
	void setWriteForFunction(const char* functionName, const char* moduleName);

	PX_INLINE void setMaxSamples(PxU32 maxSamples)
	{
		mMaxSamples = maxSamples;
	}
	void setFrames(PxU32 numFrames, const PxU32* frames)
	{
		for(PxU32 i = 0; i < numFrames && mSampledFrames.size() < mMaxSamples; i++)
		{
			if (frames == NULL) // write next numFrames frames after current
			{
				mSampledFrames.pushBack(mCurrentFrame + i + 1);
			}
			else
			{
				mSampledFrames.pushBack(frames[i]);
			}
		}
	}
	void setFramePeriod(PxU32 period)
	{
		mFramePeriod = period;
	}
	void setCallPerFrameMaxCount(PxU32 cpfMaxCount)
	{
		mCallPerFrameMaxCount = cpfMaxCount;
	}
	void setWriteForNotSuccessfulKernel(bool flag)
	{
		mIsWriteForNonSuccessfulKernel = flag;
	}
/*	void setCallPerFrameSeries(PxU32 callsCount, const PxU32* calls)
	{
		for(PxU32 i = 0; i < callsCount && mSampledCallsPerFrame.size() < mCallPerFrameMaxCount; i++)
		{
			mSampledCallsPerFrame.pushBack(calls[i]);
		}
	}*/
	bool runKernel(const char* path);
	
private:
	NiApexScene*	mApexScene;
	PxU32	mCurrentFrame;
	PxU32	mMaxSamples;
	PxU32	mFramePeriod;
	PxU32	mCallPerFrameMaxCount;
	bool	mIsWriteForNonSuccessfulKernel;
	ApexSimpleString					mPath;
	Array <PxU32>						mSampledFrames;
	//Array <PxU32>						mSampledCallsPerFrame;
	Array <KernelInfo>					mKernels;
	Array <ApexCudaTestKernelContext*>	mContexts;
};

}
} // namespace physx::apex

#endif // __APEX_CUDA_TEST__
