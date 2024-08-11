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

#include "ApexCudaTest.h"
#include "ApexCudaWrapper.h"
#include "PsFile.h"
#include <cuda.h>
#include "NiModule.h"
#include "ApexSDKHelpers.h"

# define CUT_SAFE_CALL(call)  { CUresult ret = call;                         \
		if( CUDA_SUCCESS != ret ) {                                              \
			APEX_INTERNAL_ERROR("Cuda Error %d", ret);                         \
			PX_ASSERT(!ret); } }

#define CU_MEMCPY_D_TO_MEM_BUF(dptr, size) \
	CUT_SAFE_CALL(cuMemcpyDtoHAsync( \
		(void*)(mMemBuf.getWriteBuffer() + mOffset), (CUdeviceptr)dptr, size, (CUstream)mCuStream) \
	);
	
#define CU_MEMCPY_MEM_BUF_TO_D(dptr, size) \
	CUT_SAFE_CALL(cuMemcpyHtoDAsync( \
		(void*)&(mMemBuf.getWriteBuffer() + mOffset), (CUdeviceptr)dptr, size, (CUstream)mCuStream) \
	);

#define ALIGN_OFFSET(offset, alignment) (offset) = ((offset) + (alignment) - 1) & ~((alignment) - 1)

#define WRITE_SCALAR(val) mMemBuf.alignWrite(4); mMemBuf.write(&val, sizeof(val)); \
	mOffset += sizeof(val); ALIGN_OFFSET(mOffset, 4);

#define WRITE_ALIGN_ARRAY(ptr, size, align) {PxU32 nsz = size; \
	mMemBuf.alignWrite(4); mMemBuf.write(&nsz, sizeof(nsz)); mMemBuf.alignWrite(align); mMemBuf.write(ptr, nsz); \
	ALIGN_OFFSET(mOffset, 4); mOffset += sizeof(nsz); ALIGN_OFFSET(mOffset, align); mOffset += nsz;}

#define WRITE_ARRAY(ptr, size) WRITE_ALIGN_ARRAY(ptr, size, 4)

#define WRITE_STRING(str) {PxU32 nsz = str.size(); mMemBuf.alignWrite(4); \
	mMemBuf.write(&nsz, sizeof(nsz)); mMemBuf.write(str.c_str(), nsz); \
	mOffset += sizeof(nsz) + nsz; ALIGN_OFFSET(mOffset, 4);}

#define READ_SCALAR(val) mMemBuf->alignRead(4); mMemBuf->read(&val, sizeof(val));

#define READ_STRING(str) {PxU32 nsz; mMemBuf->alignRead(4); mMemBuf->read(&nsz, sizeof(PxU32)); \
	str.resize(nsz); mMemBuf->read(&str.front(), nsz);}

namespace physx
{
namespace apex
{

	ApexCudaTestKernelContextReader::ApexCudaTestKernelContextReader(const char* path, NiApexScene* scene)
		:	mMemBuf(NULL)
		,	mHeadCudaObj(NULL)
		,	mFunc(NULL)
		,	mApexScene(scene)
		,	mCuStream(NULL)
		,	mTmpArray(*scene, __FILE__, __LINE__)
		,	mCopyQueue(*(scene->getTaskManager()->getGpuDispatcher()))
	{
		FILE* loadFile;
		loadFile = fopen(path, "rb");
		if (loadFile)
		{
			PxU32 serviceInfo[5];

			fread(serviceInfo, sizeof(PxU32), 5, loadFile);
			if (serviceInfo[0] != 100)
			{
				PX_ASSERT(!"Unknown version of cuda context file");
			}
			fseek(loadFile, 0, 0);

			mMemBuf = PX_NEW(PxMemoryBuffer)(serviceInfo[1]);
			mMemBuf->initWriteBuffer(serviceInfo[1]);
			mCudaObjOffset = serviceInfo[3];
			mParamOffset = serviceInfo[4];
			fread((void*)mMemBuf->getWriteBuffer(), 1, serviceInfo[1], loadFile);
			
			// Header
			mMemBuf->seekRead(serviceInfo[2]);
			READ_STRING(mName);
			READ_STRING(mModuleName);
			READ_SCALAR(mFrame);
			READ_SCALAR(mCallPerFrame);
			READ_SCALAR(mBlockDim.x);
			READ_SCALAR(mBlockDim.y);
			READ_SCALAR(mBlockDim.z);
			READ_SCALAR(mGridDim.x);
			READ_SCALAR(mGridDim.y);
			READ_SCALAR(mKernelType);
			READ_SCALAR(mThreadCount[0]);
			READ_SCALAR(mThreadCount[1]);
			READ_SCALAR(mThreadCount[2]);
			READ_SCALAR(mBlockCountY);

			NiModuleScene* moduleScene = scene->getNiModuleScene(mModuleName.c_str());
			if (moduleScene)
			{
				mHeadCudaObj = static_cast<ApexCudaObj*>(moduleScene->getHeadCudaObj());
			}

			ApexCudaObj* obj = mHeadCudaObj;
			while(obj)
			{
				if (obj->getType() == ApexCudaObj::FUNCTION)				
				{				
					if (ApexSimpleString(DYNAMIC_CAST(ApexCudaFunc*)(obj)->getName()) == mName)
					{
						mFunc = DYNAMIC_CAST(ApexCudaFunc*)(obj);
						break;
					}
				}
				obj = obj->next();
			}
		}
	}

	ApexCudaTestKernelContextReader::~ApexCudaTestKernelContextReader()
	{
		if (mMemBuf)
		{
			PX_DELETE(mMemBuf);
		}
	}

	bool ApexCudaTestKernelContextReader::runKernel()
	{
		return false;
		/*if (mFunc)
		{
			//launch1			
			mCuOffset = 0;
			int* tmp = NULL;
			int itmp = 0;
		
			physx::PxScopedCudaLock _lock_(*mApexScene->getTaskManager()->getGpuDispatcher()->getCudaContextManager());
						
			mFunc->setParam(mCuOffset, tmp);	// profile buffer (NULL)
			mFunc->setParam(mCuOffset, itmp);	// kernelID (0)

			switch(mKernelType)
			{
			case KT_SYNC :
				PX_ASSERT(!"Not implemented!");
				break;			
			case KT_FREE2D :
				mFunc->setParam(mCuOffset, mThreadCount[0]);
				mFunc->setParam(mCuOffset, mThreadCount[1]);
				break;
			case KT_FREE3D :
				mFunc->setParam(mCuOffset, mThreadCount[0]);
				mFunc->setParam(mCuOffset, mThreadCount[1]);
				mFunc->setParam(mCuOffset, mThreadCount[2]);
				mFunc->setParam(mCuOffset, mBlockCountY);
				break;
			case KT_BOUND :
			case KT_FREE :
				mFunc->setParam(mCuOffset, mThreadCount[0]);
				break;
			default :
				PX_ASSERT(!"Wrong kernel type");
			}

			loadContext();
				
			CUT_SAFE_CALL(cuParamSetSize(mFunc->mFunc, mCuOffset));
			CUT_SAFE_CALL(cuFuncSetBlockShape(mFunc->mFunc, mBlockDim.x, mBlockDim.y, mBlockDim.z));
			CUT_SAFE_CALL(cuLaunchGridAsync(mFunc->mFunc, mGridDim.x, mGridDim.y, (CUstream)mCuStream));

			mTmpArray.copyDeviceToHostQ(mCopyQueue);
			mCopyQueue.flushEnqueued();

			//CUT_SAFE_CALL(cuStreamSynchronize((CUstream)mCuStream));

			for (PxU32 i = 0; i < mTexRefs.size(); i++)
			{
				//ApexCudaTexRef* texRef = (ApexCudaTexRef*)mTexRefs[i];
				//texRef->unbind();
			}

			bool isOk = true;
			for (PxU32 i = 0; i < mOutMemRefs.size() && isOk; i++)
			{
				if (mOutMemRefs[i].fpType)
				{	
					if (mOutMemRefs[i].fpType == 4)
					{
						for (PxU32 j = 0; j < mOutMemRefs[i].size && isOk; j += 4)
						{
							float ref = *reinterpret_cast<float*>((PxU8*)mOutMemRefs[i].gpuPtr + j);
							float res = *reinterpret_cast<float*>((PxU8*)mTmpArray.getPtr() + mOutMemRefs[i].bufferOffset + j);
							isOk = physx::PxAbs(res - ref) <= 2.5e-7 * physx::PxMax(2.f, abs(res + ref));
						}
					}
					else
					{
						for (PxU32 j = 0; j < mOutMemRefs[i].size && isOk; j += 8)
						{
							double ref = *reinterpret_cast<double*>((PxU8*)mOutMemRefs[i].gpuPtr + j);
							double res = *reinterpret_cast<double*>((PxU8*)mTmpArray.getPtr() + mOutMemRefs[i].bufferOffset + j);
							isOk = physx::PxAbs(res - ref) <= 2.5e-14 * physx::PxMax(2., abs(res + ref));
						}
					}
				}
				else
				{
					for (PxU32 j = 0; j < mOutMemRefs[i].size && isOk; j += 4)
					{
						int ref = *reinterpret_cast<int*>((PxU8*)mOutMemRefs[i].gpuPtr + j);
						int res = *reinterpret_cast<int*>((PxU8*)mTmpArray.getPtr() + mOutMemRefs[i].bufferOffset + j);
						isOk = (res == ref);
					}
				}			
			}
			return isOk;
		}

		return false;*/
	}

	void ApexCudaTestKernelContextReader::loadContext()
	{
		PxU32 n;
		PxU32 cudaMemOffset = 0;

		//Read cuda objs
		mMemBuf->seekRead(mCudaObjOffset);
		READ_SCALAR(n)
		for (PxU32 i = 0; i < n; i++)
		{
			PxU32 t;
			READ_SCALAR(t);
			switch(t)
			{
			case 1:
				loadTexRef(cudaMemOffset);
				break;
			case 2:
				loadConstMem();
				break;
			default:
				PX_ASSERT(!"Wrong type");
				return;
			}
		}


		//Read call params
		mMemBuf->seekRead(mParamOffset);
		READ_SCALAR(n);
		PxU32 cudaMemOffsetPS = 0;
		for (PxU32 i = 0; i < n; i++)
		{
			cudaMemOffsetPS += getParamSize();
			ALIGN_OFFSET(cudaMemOffsetPS, 1 << 9);
		}

		PxU32 arrSz = physx::PxMax(cudaMemOffset + cudaMemOffsetPS, 4U);
		mTmpArray.reserve(arrSz, ApexMirroredPlace::CPU_GPU);
		mTmpArray.setSize(arrSz);	

		mMemBuf->seekRead(this->mParamOffset + sizeof(n));
		for (PxU32 i = 0; i < n; i++)
		{
			loadParam(cudaMemOffset);
		}

		for (PxU32 i = 0; i < mMemRefs.size(); i++)
		{
			memcpy(mTmpArray.getPtr() + mMemRefs[i].bufferOffset, mMemRefs[i].gpuPtr, mMemRefs[i].size);
		}

		if (cudaMemOffset > 0)
		{
			mCopyQueue.reset((CUstream)mCuStream, 1);
			mTmpArray.copyHostToDeviceQ(mCopyQueue, cudaMemOffset);
			mCopyQueue.flushEnqueued();
		}

		/*	for (PxU32 i = 0; i < mTexRefs.size(); i++)
		{
		ApexCudaTexRef* texRef = (ApexCudaTexRef*)mTexRefs[i];
		texRef->bindTo(mTmpArray.getGpuPtr() + mMemRefs[i].bufferOffset, mMemRefs[i].size);
		}*/
	}

	void ApexCudaTestKernelContextReader::loadTexRef(PxU32& memOffset)
	{
		PxU32 size;
		ApexSimpleString name;
		READ_STRING(name);
		READ_SCALAR(size);
		//if (size > 0)
		{
			//Load texture
			ApexCudaObj* obj = mHeadCudaObj;
			while(obj)
			{
				if (obj->getType() == ApexCudaObj::TEXTURE)
				{
					ApexCudaTexRef* texRef = DYNAMIC_CAST(ApexCudaTexRef*)(obj);
					if (ApexSimpleString(texRef->getName()) == name)
					{
						mTexRefs.pushBack(texRef);
						mMemRefs.pushBack(apexCudaTest::MemRef(mMemBuf->getReadLoc(), size, 0, memOffset));
						memOffset += size; ALIGN_OFFSET(memOffset, 1 << 9);
						break;
					}
				}
				obj = obj->next();
			}
			mMemBuf->advanceReadLoc(size);
		}
	}

	void ApexCudaTestKernelContextReader::loadConstMem()
	{
		//PxU32 size;
		//ApexSimpleString name;
		//READ_STRING(name);
		//READ_SCALAR(size);

		////Load const mem
		//ApexCudaObj* obj = mHeadCudaObj;
		//while(obj)
		//{
		//	if (obj->getType() == ApexCudaObj::CONST_MEM)				
		//	{
		//		ApexCudaConstMem* constMem = DYNAMIC_CAST(ApexCudaConstMem*)(obj);
		//		if (ApexSimpleString(constMem->getName()) == name)
		//		{
		//			mMemBuf->read(constMem->mHostPtr, size);
		//			//CUT_SAFE_CALL(cuMemcpyHtoDAsync(constMem->mDevPtr, constMem->mHostPtr, size, NULL));
		//			break;
		//		}
		//	}
		//	obj = obj->next();
		//}
	}

	PxU32 ApexCudaTestKernelContextReader::getParamSize()
	{
		PxU32 size, align, intent;
		PxI32 dataOffset;
		READ_SCALAR(align);
		READ_SCALAR(intent);
		READ_SCALAR(dataOffset);
		READ_SCALAR(size);
		if (size > 0)
		{
			mMemBuf->alignRead(align);
			mMemBuf->advanceReadLoc(size);
		
			if ((intent & 3) == 3)
			{
				mMemBuf->alignRead(align);
				mMemBuf->advanceReadLoc(size);
			}
			if (intent & 3)
			{
				return size;
			}
		}
		return 0;
	}

	void ApexCudaTestKernelContextReader::loadParam(PxU32& memOffset)
	{
		PxU32 size, align, intent;
		PxI32 dataOffset;
		READ_SCALAR(align);
		READ_SCALAR(intent);
		READ_SCALAR(dataOffset);
		READ_SCALAR(size);
		if (size > 0)
		{
			if (!intent)	// scalar param
			{
			//	mFunc->setParam(mCuOffset, align, size, (void*)(mMemBuf->getReadLoc()));
				mMemBuf->advanceReadLoc(size);
			}
			else
			{
			//	mMemBuf->alignRead(align);
			//	mMemRefs.pushBack(apexCudaTest::MemRef(mMemBuf->getReadLoc(), size, dataOffset, memOffset));
			//	if (intent & 0x01)	// input intent
			//	{
			//		mMemBuf->advanceReadLoc(size);
			//	}
			//	if (intent & 0x02)	// output intent
			//	{
			//		mMemBuf->alignRead(align);
			//		mOutMemRefs.pushBack(apexCudaTest::MemRef(mMemBuf->getReadLoc(), size, dataOffset, memOffset, intent >> 2));
			//		mMemBuf->advanceReadLoc(size);
			//	}
			////	void* ptr = mTmpArray.getGpuPtr() + memOffset - dataOffset;
			////	mFunc->setParam(mCuOffset, align, sizeof(void*), &ptr);
			//	memOffset += size; 
				ALIGN_OFFSET(memOffset, 1 << 9);
			}
		}
		else
		{
			//void* ptr = NULL;//mTmpArray.getGpuPtr() + memOffset - dataOffset;
			//mFunc->setParam(mCuOffset, align, sizeof(void*), &ptr);
		}
	}
	
	ApexCudaTestKernelContext::ApexCudaTestKernelContext(const char* path, const char* functionName, const char* moduleName, PxU32 frame, PxU32 callPerFrame, 
		bool isWriteForNonSuccessfulKernel, bool isContextForSave)
		:	mVersion(100)
		,	mFrame(frame)
		,	mCallPerFrame(callPerFrame)
		,	mPath(path)
		,	mName(functionName)
		,	mModuleName(moduleName)
		,	mCudaObjsCounter(0)
		,	mCallParamsCounter(0)
		,	mIsFirstParamCall(true)
		,	mIsCompleteContext(false)
		,	mIsWriteForNonSuccessfulKernel(isWriteForNonSuccessfulKernel)
		,	mIsContextForSave(isContextForSave)
	{
		// service info
		mMemBuf.setEndianMode(PxMemoryBuffer::ENDIAN_LITTLE);
		mMemBuf.write(&mVersion, sizeof(PxU32));		// Version of format
		mMemBuf.seekWrite(2 * sizeof(PxU32));			// Space for file size
		mOffset = 32;									// Offset for header block
		mMemBuf.write(&mOffset, sizeof(PxU32));
		
		// header info
		mMemBuf.seekWrite(mOffset);		
		WRITE_STRING(mName)								// Name of function
		WRITE_STRING(mModuleName)						// Name of module
		WRITE_SCALAR(frame)								// Current frame
		WRITE_SCALAR(callPerFrame)						// Call of kernel per current frame
		mOffset += 10 * sizeof(PxU32);					// Space for cuda kernel parameters

		mCudaObjsOffset = mOffset;						// Offset for cuda objects block
		mMemBuf.seekWrite(3 * sizeof(PxU32));	
		mMemBuf.write(&mCudaObjsOffset, sizeof(PxU32));

		mOffset = mCudaObjsOffset + sizeof(PxU32);		// Space for N of cuda objs
		mMemBuf.seekWrite(mOffset);
	}

	ApexCudaTestKernelContext::~ApexCudaTestKernelContext()
	{
	}

	void ApexCudaTestKernelContext::completeCudaObjsBlock()
	{
		mMemBuf.seekWrite(4 * sizeof(PxU32));			// Offset for call param block
		mMemBuf.write(&mOffset, sizeof(PxU32));
		mCallParamsOffset = mOffset;

		mMemBuf.seekWrite(mCudaObjsOffset);				// Write N of cuda objs
		mMemBuf.write(&mCudaObjsCounter, sizeof(PxU32));

		mOffset += sizeof(PxU32);						// Space for N of call params
		mMemBuf.seekWrite(mOffset);
	}

	void ApexCudaTestKernelContext::completeCallParamsBlock()
	{
		mMemBuf.seekWrite(mCallParamsOffset);			// Write N of call params
		mMemBuf.write(&mCallParamsCounter, sizeof(PxU32));
	}

	void ApexCudaTestKernelContext::setFreeKernel(int threadCount)
	{
		mMemBuf.seekWrite(mCudaObjsOffset - 5 * sizeof(PxU32));
		PxU32 tmp = 1;
		mMemBuf.write(&tmp, sizeof(tmp));		
		mMemBuf.write(&threadCount, sizeof(threadCount));
		mMemBuf.seekWrite(mOffset);
	}
	void ApexCudaTestKernelContext::setFreeKernel(int threadCountX, int threadCountY)
	{
		mMemBuf.seekWrite(mCudaObjsOffset - 5 * sizeof(PxU32));
		PxU32 tmp = 2;
		mMemBuf.write(&tmp, sizeof(tmp));		
		mMemBuf.write(&threadCountX, sizeof(threadCountX));
		mMemBuf.write(&threadCountY, sizeof(threadCountY));
		mMemBuf.seekWrite(mOffset);
	}
	void ApexCudaTestKernelContext::setFreeKernel(int threadCountX, int threadCountY, int threadCountZ, int blockCountY)
	{
		mMemBuf.seekWrite(mCudaObjsOffset - 5 * sizeof(PxU32));
		PxU32 tmp = 3;
		mMemBuf.write(&tmp, sizeof(tmp));		
		mMemBuf.write(&threadCountX, sizeof(threadCountX));
		mMemBuf.write(&threadCountY, sizeof(threadCountY));
		mMemBuf.write(&threadCountZ, sizeof(threadCountZ));
		mMemBuf.write(&blockCountY, sizeof(blockCountY));
		mMemBuf.seekWrite(mOffset);
	}
	void ApexCudaTestKernelContext::setBoundKernel(int threadCount)
	{
		mMemBuf.seekWrite(mCudaObjsOffset - 5 * sizeof(PxU32));
		PxU32 tmp = 4;
		mMemBuf.write(&tmp, sizeof(tmp));		
		mMemBuf.write(&threadCount, sizeof(threadCount));
		mMemBuf.seekWrite(mOffset);
	}
	void ApexCudaTestKernelContext::setSyncKernel()
	{
		mMemBuf.seekWrite(mCudaObjsOffset - 5 * sizeof(PxU32));
		PxU32 tmp = 0;
		mMemBuf.write(&tmp, sizeof(tmp));
		mMemBuf.seekWrite(mOffset);
	}

	void ApexCudaTestKernelContext::setBlockDim(int x, int y, int z)
	{
		mMemBuf.seekWrite(mCudaObjsOffset - 10 * sizeof(PxU32));
		mMemBuf.write(&x, sizeof(int));
		mMemBuf.write(&y, sizeof(int));
		mMemBuf.write(&z, sizeof(int));
		mMemBuf.seekWrite(mOffset);
	}

	void ApexCudaTestKernelContext::setGridDim(int x, int y)
	{
		mMemBuf.seekWrite(mCudaObjsOffset - 7 * sizeof(PxU32));
		mMemBuf.write(&x, sizeof(int));
		mMemBuf.write(&y, sizeof(int));
		mMemBuf.seekWrite(mOffset);
	}

	void ApexCudaTestKernelContext::addParam(PxU32 align, void *val, size_t size, int memRefIntent, int dataOffset, int fpType)
	{
		PxU32 sz = (PxU32)size;
		if (mIsFirstParamCall)
		{
			PX_ASSERT(!mIsCompleteContext);
			completeCudaObjsBlock();
			copyMemRefs(); // copy textures
			mIsFirstParamCall = false;
		}
		mCallParamsCounter++;
		WRITE_SCALAR(align);
		PxU32 intent = memRefIntent;
		intent += fpType << 2;
		WRITE_SCALAR(intent);
		WRITE_SCALAR(dataOffset);
		if (!memRefIntent)
		{
			WRITE_ALIGN_ARRAY(val, sz, align);
		}
		else if ((memRefIntent & 0x01) &&  sz > 0)
		{			
			//mTmpBuffer.resize(sz);
			//WRITE_ALIGN_ARRAY((void*)&(mTmpBuffer[0]), sz, align);
			///*mMemBuf.alignWrite(4); mMemBuf.write(&nsz, sizeof(sz)); 
			//mMemBuf.alignWrite(align); mMemBuf.write((void*)&(mTmpBuffer[0]), sz); 
			//ALIGN_OFFSET(mOffset, 4); mOffset += sizeof(sz);
			//ALIGN_OFFSET(mOffset, align); mOffset += sz;*/ 
			//mOffset -= sz;
			////CU_MEMCPY_D_TO_MEM_BUF(val + dataOffset, sz);			
			//mOffset += sz;
		}
		else
		{
			WRITE_SCALAR(sz);
		}
	}

	void ApexCudaTestKernelContext::addOutMemRef(PxU32 align, const ApexCudaMemRefBase* memRef)
	{
		if (memRef->size > 0)
		{
			mTmpBuffer.resize((PxU32)memRef->size);
			ALIGN_OFFSET(mOffset, align);
			mMemBuf.alignWrite(align);
			mMemRefs.pushBack(apexCudaTest::MemRef(memRef->ptr, memRef->size, memRef->offset, mOffset));
			mMemBuf.write((void*)&(mTmpBuffer[0]),(PxU32)memRef->size);
			mOffset += (PxU32)memRef->size;
			ALIGN_OFFSET(mOffset, 4);
		}
	}

	void ApexCudaTestKernelContext::addTexRef(const char* name, const void* mem, size_t size)
	{
		PX_ASSERT(mIsFirstParamCall && !mIsCompleteContext);
		PxU32 objType = 0x01;
		mCudaObjsCounter++;
		WRITE_SCALAR(objType);
		ApexSimpleString tName(name);
		WRITE_STRING(tName);
		PxU32 sz = (PxU32)size;
		WRITE_SCALAR(sz);
		if (sz)
		{
			mTmpBuffer.resize(sz);
			mMemBuf.write((void*)&(mTmpBuffer[0]), sz); 
			mMemRefs.pushBack(apexCudaTest::MemRef(mem, size, 0, mOffset));		
			mOffset += sz;
			ALIGN_OFFSET(mOffset, 4);
		}
	}

	void ApexCudaTestKernelContext::addConstMem(const char* name, const void* mem, size_t size)
	{
		PX_ASSERT(mIsFirstParamCall && !mIsCompleteContext);
		PxU32 objType = 0x02;
		mCudaObjsCounter++;
		WRITE_SCALAR(objType);
		ApexSimpleString cmName(name);
		WRITE_STRING(cmName);
		WRITE_ARRAY(mem, (PxU32)size);
	}
	
	void ApexCudaTestKernelContext::copyMemRefs()
	{
		PxU32 offset = mOffset;
		for (PxU32 i = 0; i < mMemRefs.size(); i++)
		{
			//mOffset = mMemRefs[i].bufferOffset;
			//CU_MEMCPY_D_TO_MEM_BUF(mMemRefs[i].gpuPtr + mMemRefs[i].dataOffset, (PxU32)mMemRefs[i].size);
		}
		mMemRefs.clear();
		mOffset = offset;
	}

	void ApexCudaTestKernelContext::setKernelStatus()
	{
		if (mIsWriteForNonSuccessfulKernel)
		{
			//int cuResult = cuCtxSynchronize();//= cudaPeekAtLastError();
			////cudaDeviceSynchronize();
		
			//if (cuResult)
			//{
			//	mErrorCode += 'E';
			//	mErrorCode += ApexSimpleString(cuResult, 3);
			//	saveToFile();
			//	APEX_INTERNAL_ERROR("Cuda Error %d", cuResult);
			//}
			//else if (mIsContextForSave)
			//{
			//	copyMemRefs();
			//}
		}
		else
		{
			copyMemRefs();
		}
	}

	bool ApexCudaTestKernelContext::saveToFile()
	{
		if (!mIsContextForSave && mErrorCode.size() == 0)
		{
			return false;
		}
		if (!mIsCompleteContext)
		{
			completeCallParamsBlock();

			mMemBuf.seekWrite(sizeof(PxU32));				// Write size of file
			mMemBuf.write(&mOffset, sizeof(PxU32));
			mIsCompleteContext = true;

			mMemBuf.seekWrite(mOffset);
		}
		
		ApexSimpleString path(mPath);
		path += mName;
		path += '_';
		path += ApexSimpleString(mCallPerFrame, 3);
		path += ApexSimpleString(mFrame, 5);
		path += mErrorCode;
		FILE* saveFile;
		physx::shdfnd::fopen_s(&saveFile,path.c_str(),"wb");
		if (saveFile)
		{
			fwrite(mMemBuf.getWriteBuffer(), mMemBuf.getWriteBufferSize(), 1, saveFile);
			return !fclose(saveFile);
		}
		
		return false;
	}

	
	ApexCudaTestManager::ApexCudaTestManager()
		:	mCurrentFrame(0)
		,	mMaxSamples(0)
		,	mFramePeriod(0)		
		,	mCallPerFrameMaxCount(1)
		,	mIsWriteForNonSuccessfulKernel(false)
	{
	}

	ApexCudaTestManager::~ApexCudaTestManager()
	{
		for (PxU32 i = 0; i < mContexts.size(); i++)
		{
			PX_DELETE(mContexts[i]);
		}
	}

	void ApexCudaTestManager::setWriteForFunction(const char* functionName, const char* moduleName)
	{
		ApexSimpleString fName(moduleName);
		fName += '_';
		fName += ApexSimpleString(functionName);
		mKernels.pushBack(KernelInfo(fName.c_str(), moduleName));
	}

	bool ApexCudaTestManager::runKernel(const char* path)
	{
		if (mApexScene)
		{			
			ApexCudaTestKernelContextReader contextReader(path, mApexScene);
			return contextReader.runKernel();
		}
		return false;
	}

	void ApexCudaTestManager::nextFrame()
	{
		mCurrentFrame++;

		if (mContexts.size() > 0)
		{
			for (PxU32 i = 0; i < mContexts.size(); i++)
			{
				mContexts[i]->saveToFile();
				PX_DELETE(mContexts[i]);
			}
			mContexts.clear();

			for (PxU32 i = 0; i < mKernels.size(); i++)
			{
				mKernels[i].callCount = 0;
			}
		}
	}

	ApexCudaTestKernelContext* ApexCudaTestManager::isTestKernel(const char* functionName, const char* moduleName)
	{
		KernelInfo* kernel = NULL;
		if  (	mContexts.size() < mMaxSamples 
			&&	(	mSampledFrames.find(mCurrentFrame) != mSampledFrames.end()
				||	mFramePeriod && (mCurrentFrame % mFramePeriod) == 0
				)
			&&	(kernel = mKernels.find(KernelInfo(functionName, moduleName))) != mKernels.end()
			&&  (kernel->callCount < mCallPerFrameMaxCount)
			)
		{
			mContexts.pushBack(PX_NEW(ApexCudaTestKernelContext)(mPath.c_str(), functionName, moduleName, mCurrentFrame, ++(kernel->callCount), mIsWriteForNonSuccessfulKernel, true));
			return mContexts.back();
		}
		else if (mIsWriteForNonSuccessfulKernel && (mKernels.size() == 0 || (mKernels.find(KernelInfo(functionName, moduleName)) != mKernels.end())))
		{
			mContexts.pushBack(PX_NEW(ApexCudaTestKernelContext)(mPath.c_str(), functionName, moduleName, mCurrentFrame, 0, true, false));
			return mContexts.back();
		}
		return NULL;
	}
}
} // namespace physx::apex

#endif
