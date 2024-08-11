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

#ifndef __APEX_CUDA_WRAPPER_H__
#define __APEX_CUDA_WRAPPER_H__

#include <cuda.h>
#include "ApexCutil.h"
#include "vector_types.h"
#include "ApexMirroredArray.h"
#include "InplaceStorage.h"
#include "PsMutex.h"
#include "ApexCudaTest.h"
#include "ApexCudaProfile.h"

namespace physx
{
namespace apex
{

struct DimGrid
{
	int x, y;

	DimGrid() {}
	DimGrid(int x, int y = 1)
	{
		this->x = x;
		this->y = y;
	}
};
struct DimBlock
{
	int x, y, z;

	DimBlock() {}
	DimBlock(int x, int y = 1, int z = 1)
	{
		this->x = x;
		this->y = y;
		this->z = z;
	}
};

struct ApexCudaMemRefBase
{
	enum Intent
	{
		IN = 0x01,
		OUT = 0x02,
		IN_OUT = IN | OUT
	};
	void*		ptr;
	size_t		size;	//size in bytes
	PxI32		offset;	//data offset for ptr
	Intent		intent;
	
	ApexCudaMemRefBase(void* ptr, size_t byteSize, PxI32 offset, Intent intent)
		: ptr(ptr), size(byteSize), offset(offset), intent(intent) {}
	virtual ~ApexCudaMemRefBase() {}
};

template <class T>
struct ApexCudaMemRef : public ApexCudaMemRefBase
{
	ApexCudaMemRef(T* ptr, size_t byteSize, Intent intent = IN_OUT) 
		: ApexCudaMemRefBase(ptr, byteSize, 0, intent) {}

	ApexCudaMemRef(T* ptr, size_t byteSize, PxI32 offset, Intent intent) 
		: ApexCudaMemRefBase(ptr, byteSize, offset, intent) {}

	inline T* getPtr() const
	{
		return (T*)ptr;
	}

	virtual ~ApexCudaMemRef() {}
};

template <class T>
inline ApexCudaMemRef<T> createApexCudaMemRef(T* ptr, size_t size, ApexCudaMemRefBase::Intent intent = ApexCudaMemRefBase::IN_OUT)
{
	return ApexCudaMemRef<T>(ptr, sizeof(T) * size, intent);
}

template <class T>
inline ApexCudaMemRef<T> createApexCudaMemRef(T* ptr, size_t size, PxI32 offset, ApexCudaMemRefBase::Intent intent)
{
	return ApexCudaMemRef<T>(ptr, sizeof(T) * size, sizeof(T) * offset, intent);
}

template <class T>
inline ApexCudaMemRef<T> createApexCudaMemRef(const ApexMirroredArray<T>& ma, ApexCudaMemRefBase::Intent intent = ApexCudaMemRefBase::IN_OUT)
{
	return ApexCudaMemRef<T>(ma.getGpuPtr(), ma.getByteSize(), intent);
}

template <class T>
inline ApexCudaMemRef<T> createApexCudaMemRef(const ApexMirroredArray<T>& ma, size_t size, ApexCudaMemRefBase::Intent intent = ApexCudaMemRefBase::IN_OUT)
{
	return ApexCudaMemRef<T>(ma.getGpuPtr(), sizeof(T) * size, intent);
}

template <class T>
inline ApexCudaMemRef<T> createApexCudaMemRef(const ApexMirroredArray<T>& ma, size_t size, PxI32 offset, ApexCudaMemRefBase::Intent intent = ApexCudaMemRefBase::IN_OUT)
{
	return ApexCudaMemRef<T>(ma.getGpuPtr(), sizeof(T) * size, sizeof(T) * offset, intent);
}

#ifndef ALIGN_OFFSET
#define ALIGN_OFFSET(offset, alignment) (offset) = ((offset) + (alignment) - 1) & ~((alignment) - 1)
#endif

#define CUDA_MAX_PARAM_SIZE		256


class ApexCudaTestKernelContext;

class ApexCudaObj
{
	friend class ApexCudaObjList;
	ApexCudaObj* _objListNext;

protected:
	ApexCudaObj() : _objListNext(0) {}
	virtual ~ApexCudaObj() {}

public:
	enum ApexCudaObjType
	{
		UNKNOWN,
		FUNCTION,
		TEXTURE,
		CONST_MEM
	};
	virtual ApexCudaObjType getType()
	{
		return UNKNOWN;
	}

	PX_INLINE ApexCudaObj* next()
	{
		return _objListNext;
	}
	virtual void release() = 0;	
	virtual void formContext(ApexCudaTestKernelContext*) = 0;
};

class ApexCudaObjList
{
	ApexCudaObj* _objListHead;

public:
	ApexCudaObjList() : _objListHead(0) {}

	PX_INLINE void addToList(ApexCudaObj* obj)
	{
		obj->_objListNext = _objListHead;
		_objListHead = obj;
	}

	PX_INLINE ApexCudaObj* head()
	{
		return _objListHead;
	}

	void releaseAll()
	{
		for (ApexCudaObj* obj = _objListHead; obj != 0; obj = obj->_objListNext)
		{
			obj->release();
		}
	}
};


class ApexCudaFunc : public ApexCudaObj
{
public:
	const char* getName() const
	{
		return mName;
	}
	CUmodule getModule() const
	{
		return mModule;
	}
	void init(	ApexCudaObjList& objList, physx::PxGpuDispatcher* gd, CUfunction func, CUmodule module, PxU16 id, NxModule* nxModule, ApexCudaTestManager* cudaTestManager)
	{
		mCudaTestManager = cudaTestManager;	
		mObjList = &objList;
		objList.addToList(this);
		mFunc = func;
		mModule = module;
		mNxModule = nxModule;
		mKernelID = id;
		mGpuDispatcher = gd;
		init(gd->getCudaContextManager());
	}

	virtual ApexCudaObjType getType()
	{
		return FUNCTION;
	}
	virtual void release() {}

	virtual void formContext(ApexCudaTestKernelContext*) {}

	/** This function force cuda stream syncronization that may slowdown application
	 */
	PX_INLINE void setProfileSession(ApexCudaProfileSession* cudaProfileSession)
	{
		mCudaProfileSession = cudaProfileSession;
		if (mCudaProfileSession)
		{
			mProfileId = mCudaProfileSession->getProfileId(mName, mNxModule->getName());
		}
	}

protected:
	PxU32		mProfileId;
	const char* mName;
	CUfunction  mFunc;
	CUmodule    mModule;
	physx::PxGpuDispatcher* mGpuDispatcher;
	PxU16       mKernelID;
	PxU32		mMaxThreads;
	PxU32		mStaticShared;
	char		mParams[CUDA_MAX_PARAM_SIZE];

	NxModule*	mNxModule;
	ApexCudaObjList* mObjList;
	ApexCudaTestManager* mCudaTestManager;	
	ApexCudaTestKernelContext* mCTContext;
	ApexCudaProfileSession* mCudaProfileSession;

	ApexCudaFunc(const char* name) 
		: mName(name), mFunc(0), mModule(0), mGpuDispatcher(0), mKernelID(0)
		, mCudaProfileSession(NULL), mCTContext(NULL)
	{
	}
	virtual void init(physx::PxCudaContextManager*) {}

	//void setParam(int &offset, unsigned int i)
	//{
	//	ALIGN_OFFSET(offset, __alignof(i));
	//	CUT_SAFE_CALL(cuParamSeti( mFunc, offset, i ));
	//	offset += sizeof(i);
	//}

	//void setParam(int &offset, float f)
	//{
	//	ALIGN_OFFSET(offset, __alignof(f));
	//	CUT_SAFE_CALL(cuParamSetf( mFunc, offset, f ));
	//	offset += sizeof(f);
	//}

	template <typename T>
	void setParam(int& offset, T* ptr)
	{
		ALIGN_OFFSET(offset, __alignof(ptr));
		PX_ASSERT(offset + sizeof(ptr) <= CUDA_MAX_PARAM_SIZE);
		//CUT_SAFE_CALL(cuParamSetv(mFunc, offset, &ptr, sizeof(ptr)));
		physx::PxMemCopy(mParams + offset, &ptr, sizeof(ptr));
		offset += sizeof(ptr);
		mCTContext = NULL;	// context can't catch pointers, use instead ApexCudaMemRef
	}

	template <typename T>
	void setParam(int& offset, const ApexCudaMemRef<T>& memRef)
	{
		T* ptr = memRef.getPtr();
		ALIGN_OFFSET(offset, __alignof(ptr));
		PX_ASSERT(offset + sizeof(ptr) <= CUDA_MAX_PARAM_SIZE);
		//CUT_SAFE_CALL(cuParamSetv(mFunc, offset, &ptr, sizeof(ptr)));
		physx::PxMemCopy(mParams + offset, &ptr, sizeof(ptr));
		offset += sizeof(ptr);
	}

	template <typename T>
	void setParam(int& offset, const T& val)
	{
		ALIGN_OFFSET(offset, __alignof(val));
		PX_ASSERT(offset + sizeof(val) <= CUDA_MAX_PARAM_SIZE);
		//CUT_SAFE_CALL(cuParamSetv(mFunc, offset, (void*)&val, sizeof(val)));
		physx::PxMemCopy(mParams + offset, (void*)&val, sizeof(val));
		offset += sizeof(val);
	}

	bool isValid() const
	{
		return (mFunc != 0) && (mModule != 0);
	}

	void resolveContext()
	{
		mCTContext->setCuModule(&mModule);
		ApexCudaObj* obj = mObjList->head();
		while(obj)
		{
			obj->formContext(mCTContext);
			obj = obj->next();
		}
	}

	template <typename T>
	void copyParam(const ApexCudaMemRef<T>& memRef)
	{
		mCTContext->addParam(__alignof(void*), memRef.ptr, memRef.size, memRef.intent, memRef.offset);
		if (ApexCudaMemRefBase::OUT & memRef.intent)
		{
			mCTContext->addOutMemRef(__alignof(void*), &memRef);
		}
	}

	template <>
	void copyParam(const ApexCudaMemRef<float>& memRef)
	{
		copyParam(memRef, 4);
	}

	template <>
	void copyParam(const ApexCudaMemRef<float2>& memRef)
	{
		copyParam(memRef, 4);
	}

	template <>
	void copyParam(const ApexCudaMemRef<float3>& memRef)
	{
		copyParam(memRef, 4);
	}

	template <>
	void copyParam(const ApexCudaMemRef<float4>& memRef)
	{
		copyParam(memRef, 4);
	}

	template <>
	void copyParam(const ApexCudaMemRef<double>& memRef)
	{
		copyParam(memRef, 8);
	}

	template <typename T>
	void copyParam(const T& val)
	{
		mCTContext->addParam(__alignof(val), (void*)&val, sizeof(val));
	}

private:
	template <typename T>
	void copyParam(const ApexCudaMemRef<T>& memRef, int fpType)
	{
		mCTContext->addParam(__alignof(void*), memRef.ptr, memRef.size, memRef.intent, memRef.offset, fpType);
		if (ApexCudaMemRefBase::OUT & memRef.intent)
		{
			mCTContext->addOutMemRef(__alignof(void*), &memRef);
		}
	}
	void setParam(int& offset, int align, int size, void* ptr)
	{
		ALIGN_OFFSET(offset, align);
		PX_ASSERT(offset + size <= CUDA_MAX_PARAM_SIZE);
		CUT_SAFE_CALL(cuParamSetv(mFunc, offset, ptr, size));		
		offset += size;
	}	
	friend class ApexCudaTestKernelContextReader;
};


template <typename T>
class ApexCudaArray
{
public:
	ApexCudaArray() : mWidth(0), mHeight(0), mCuArray(NULL) {}

	void create(unsigned int width, unsigned int height, const T* data = 0)
	{
		mWidth = width;
		mHeight = height;

		// Allocate CUDA array in device memory
		CUDA_ARRAY_DESCRIPTOR desc;
		desc.Format = CU_AD_FORMAT_FLOAT;
		desc.NumChannels = 1;
		desc.Width = mWidth;
		desc.Height = mHeight;
		CUT_SAFE_CALL(cuArrayCreate(&mCuArray, &desc));

		copyFromHost(data);
	}

	void copyFromHost(const T* data)
	{
		if (data != 0)
		{
			CUDA_MEMCPY2D copyParam;
			memset(&copyParam, 0, sizeof(copyParam));
			copyParam.dstMemoryType = CU_MEMORYTYPE_ARRAY;
			copyParam.dstArray = mCuArray;
			copyParam.srcMemoryType = CU_MEMORYTYPE_HOST;
			copyParam.srcHost = data;
			copyParam.srcPitch = mWidth * sizeof(T);
			copyParam.WidthInBytes = copyParam.srcPitch;
			copyParam.Height = mHeight;
			CUT_SAFE_CALL(cuMemcpy2D(&copyParam));
		}
	}

	CUarray getGpuHandle() const
	{
		return mCuArray;
	}

private:
	unsigned int	mWidth;
	unsigned int	mHeight;
	CUarray			mCuArray;
};



class ApexCudaTexRef : public ApexCudaObj
{
public:
	const char* getName() const
	{
		return mName;
	}

	void init(ApexCudaObjList& objList, CUtexref texRef, CUmodule module, CUarray_format format, int numChannels, int dim, int flags)
	{
		objList.addToList(this);
		mTexRef = texRef;
		mModule = module;
		mDim = dim;
		mFormat = format;
		mNumChannels = numChannels;
		mFlags = flags;
		mIsBinded = false;
	}

	ApexCudaTexRef(const char* name) : mName(name), mTexRef(0), mModule(0)
	{
	}

	void setNormalizedCoords()
	{
		mFlags |= CU_TRSF_NORMALIZED_COORDINATES;
	}

	void bindTo(const void* ptr, size_t bytes, size_t* retByteOffset = 0)
	{
		CUT_SAFE_CALL(cuTexRefSetFormat(mTexRef, mFormat, mNumChannels));
		CUT_SAFE_CALL(cuTexRefSetFlags(mTexRef, mFlags));

		size_t byteOffset;
		CUT_SAFE_CALL(cuTexRefSetAddress(&byteOffset, mTexRef, CUT_TODEVICE(ptr), static_cast<unsigned int>(bytes)));

		if (retByteOffset != 0)
		{
			*retByteOffset = byteOffset;
		}
		else
		{
			PX_ASSERT(byteOffset == 0);
		}

		mBindedSize = bytes;
		mBindedPtr = ptr;
		mIsBinded = true;
	}

	template <typename T>
	void bindTo(ApexMirroredArray<T>& mem, size_t* retByteOffset = 0)
	{
		bindTo(mem.getGpuPtr(), mem.getByteSize(), retByteOffset);
	}

	template <typename T>
	void bindTo(ApexMirroredArray<T>& mem, size_t size, size_t* retByteOffset = 0)
	{
		bindTo(mem.getGpuPtr(), sizeof(T) * size, retByteOffset);
	}

	template <typename T>
	void bindTo(const ApexCudaArray<T>& cudaArray)
	{
		CUT_SAFE_CALL(cuTexRefSetFlags(mTexRef, mFlags));

		CUT_SAFE_CALL(cuTexRefSetArray(mTexRef, cudaArray.getGpuHandle(), CU_TRSA_OVERRIDE_FORMAT));
	}

	void unbind()
	{
		size_t byteOffset;
		CUT_SAFE_CALL(cuTexRefSetAddress(&byteOffset, mTexRef, CUdeviceptr(0), 0));
		mIsBinded = false;
	}
	
	virtual ApexCudaObjType getType()
	{
		return TEXTURE;
	}

	virtual void release() {}
	
	virtual void formContext(ApexCudaTestKernelContext* context) 
	{
		if (mIsBinded && *(CUmodule*)context->getCuModule() == mModule)
		{
			context->addTexRef(mName,/* (PxU32)mFormat, mNumChannels, mDim, mFlags,*/ mBindedPtr, mBindedSize);
		}
	}

private:
	const char* mName;
	CUtexref	mTexRef;
	CUmodule    mModule;

	CUarray_format mFormat;
	int			mNumChannels;
	int			mDim;
	int			mFlags;
	
	bool		mIsBinded;
	size_t		mBindedSize;
	const void*	mBindedPtr;
};


class ApexCudaSurfRef : public ApexCudaObj
{
public:
	const char* getName() const
	{
		return mName;
	}

	void init(ApexCudaObjList& objList, CUsurfref surfRef, CUmodule module)
	{
		objList.addToList(this);
		mSurfRef = surfRef;
		mModule = module;
	}
	void init(ApexCudaObjList& objList, CUmodule module)
	{
		if (mModule == 0)
		{
			objList.addToList(this);
			mModule = module;
			CUT_SAFE_CALL(cuModuleGetSurfRef(&mSurfRef, mModule, mName));
		}
	}

	ApexCudaSurfRef(const char* name) : mName(name), mSurfRef(0), mModule(0)
	{
	}

	void bindTo(CUarray cuArray)
	{
		CUDA_ARRAY3D_DESCRIPTOR desc;
		CUT_SAFE_CALL(cuArray3DGetDescriptor(&desc, cuArray));

		CUT_SAFE_CALL(cuSurfRefSetArray(mSurfRef, cuArray, 0));
	}

	template <typename T>
	void bindTo(const ApexCudaArray<T>& cudaArray)
	{
		bindTo(cudaArray.getGpuHandle());
	}

	virtual void release() {}
	virtual void formContext(ApexCudaTestKernelContext*) {}

private:
	const char* mName;
	CUsurfref   mSurfRef;
	CUmodule    mModule;
};

class ApexCudaTexRefScopeBind
{
private:
	ApexCudaTexRefScopeBind& operator=(const ApexCudaTexRefScopeBind&);
	ApexCudaTexRef& mTexRef;

public:
	ApexCudaTexRefScopeBind(ApexCudaTexRef& texRef, void* ptr, size_t bytes, size_t* retByteOffset = 0)
		: mTexRef(texRef)
	{
		mTexRef.bindTo(ptr, bytes, retByteOffset);
	}
	template <typename T>
	ApexCudaTexRefScopeBind(ApexCudaTexRef& texRef, ApexMirroredArray<T>& mem, size_t* retByteOffset = 0)
		: mTexRef(texRef)
	{
		mTexRef.bindTo(mem, retByteOffset);
	}
	template <typename T>
	ApexCudaTexRefScopeBind(ApexCudaTexRef& texRef, ApexMirroredArray<T>& mem, size_t size, size_t* retByteOffset = 0)
		: mTexRef(texRef)
	{
		mTexRef.bindTo(mem, size, retByteOffset);
	}
	template <typename T>
	ApexCudaTexRefScopeBind(ApexCudaTexRef& texRef, const ApexCudaArray<T>& cudaArray)
		: mTexRef(texRef)
	{
		mTexRef.bindTo(cudaArray);
	}
	~ApexCudaTexRefScopeBind()
	{
		mTexRef.unbind();
	}
};

#define APEX_CUDA_TEXTURE_SCOPE_BIND(texRef, mem) ApexCudaTexRefScopeBind scopeBind_##texRef (CUDA_OBJ(texRef), mem);
#define APEX_CUDA_TEXTURE_SCOPE_BIND_SIZE(texRef, mem, size) ApexCudaTexRefScopeBind scopeBind_##texRef (CUDA_OBJ(texRef), mem, size);
#define APEX_CUDA_TEXTURE_SCOPE_BIND_PTR(texRef, ptr, count) ApexCudaTexRefScopeBind scopeBind_##texRef (CUDA_OBJ(texRef), ptr, sizeof(*ptr) * count);

class ApexCudaVar : public ApexCudaObj
{
public:
	const char*		getName() const
	{
		return mName;
	}
	size_t			getSize() const
	{
		return mSize;
	}

	void init(ApexCudaObjList& objList, physx::PxCudaContextManager* ctx, CUdeviceptr devPtr, CUmodule module, size_t size)
	{
		objList.addToList(this);
		mDevPtr = devPtr;
		mModule = module;
		mSize = size;
		init(ctx);
	}

	virtual void release() {}
	virtual void formContext(ApexCudaTestKernelContext*) {}

protected:
	virtual void init(physx::PxCudaContextManager*) {}

	ApexCudaVar(const char* name) : mName(name), mDevPtr(0), mModule(0), mSize(0)
	{
	}

protected:
	const char*		mName;
	CUdeviceptr		mDevPtr;
	CUmodule		mModule;
	size_t			mSize;
};


class ApexCudaConstMem : public ApexCudaVar, public InplaceStorage
{
public:
	ApexCudaConstMem(const char* name) : ApexCudaVar(name)
	{
		mIsCopied = false;
		mHostBuffer = 0;
		mHostPtr = 0;
		mStorageBuffer = 0;
		mStoragePtr = 0;
	}

	bool copyToDevice(CUstream stream)
	{
		bool result = false;

		InplaceStorage* storage = static_cast<InplaceStorage*>(this);
		mMutex.lock();
		if (storage->isChanged())
		{
			size_t size = storage->mapTo(mHostPtr);
			// padding up to the next dword
			size = (size + 7) & ~7;
			if (size > mSize) size = mSize;
			CUT_SAFE_CALL(cuMemcpyHtoDAsync(mDevPtr, mHostPtr, size, stream));

			storage->setUnchanged();
			result = true;
		}
		mMutex.unlock();

		mIsCopied = true;

		return result;
	}
	
	virtual ApexCudaObjType getType()
	{
		return CONST_MEM;
	}

	virtual void release()
	{
		InplaceStorage::release();

		if (mHostBuffer != 0)
		{
			mHostBuffer->free();
			mHostBuffer = 0;
		}
		mHostPtr = 0;

		if (mStorageBuffer != 0)
		{
			mStorageBuffer->free();
			mStorageBuffer = 0;
		}
		mStoragePtr = 0;
	}

	virtual void formContext(ApexCudaTestKernelContext* context) 
	{
		if (mIsCopied && *(CUmodule*)context->getCuModule() == mModule)
		{
			context->addConstMem(mName, mHostPtr, mSize);
		}
	}

protected:
	virtual physx::PxU8* storageResizeBuffer(physx::PxU32 newSize)
	{
		if (newSize <= getSize())
		{
			return mStoragePtr;
		}
		else
		{
			PX_ASSERT(0 && "Out of CUDA constant memory!");
			return 0;
		}
	}
	virtual void storageLock()
	{
		mMutex.lock();
	}
	virtual void storageUnlock()
	{
		mMutex.unlock();
	}


	virtual void init(physx::PxCudaContextManager* ctx)
	{
		mHostBuffer = ctx->getMemoryManager()->alloc(
		                  physx::PxCudaBufferType(physx::PxCudaBufferMemorySpace::T_PINNED_HOST, physx::PxCudaBufferFlags::F_READ_WRITE),
		                  mSize);
		if (mHostBuffer == 0)
		{
			PX_ASSERT(0 && "Out of Pinned Host Memory!");
			return;
		}
		mHostPtr = reinterpret_cast<physx::PxU8*>(mHostBuffer->getPtr());
		PX_ASSERT(mHostPtr != 0);

		mStorageBuffer = ctx->getMemoryManager()->alloc(
		                     physx::PxCudaBufferType(physx::PxCudaBufferMemorySpace::T_HOST, physx::PxCudaBufferFlags::F_READ_WRITE),
		                     mSize);
		if (mStorageBuffer == 0)
		{
			PX_ASSERT(0 && "Out of Host Memory!");
			return;
		}
		mStoragePtr = reinterpret_cast<physx::PxU8*>(mStorageBuffer->getPtr());
		PX_ASSERT(mStoragePtr != 0);
	}


private:
	physx::PxCudaBuffer* mHostBuffer;
	physx::PxU8*                 mHostPtr;

	physx::PxCudaBuffer* mStorageBuffer;
	physx::PxU8*                 mStoragePtr;

	physx::Mutex                 mMutex;

	bool						 mIsCopied;

	friend class ApexCudaTestKernelContextReader;
};

typedef InplaceStorageGroup ApexCudaConstMemGroup;

#define APEX_CUDA_CONST_MEM_GROUP_SCOPE(group) INPLACE_STORAGE_GROUP_SCOPE(group)


class ApexCudaTimer
{
public:
	ApexCudaTimer()
		:	mIsStarted(false)
		,	mIsFinished(false)
		,	mStart(NULL)
		,	mFinish(NULL)
	{
	}
	~ApexCudaTimer()
	{
		if (mStart != NULL)
		{
			CUT_SAFE_CALL(cuEventDestroy(mStart));
		}
		if (mFinish != NULL)
		{
			CUT_SAFE_CALL(cuEventDestroy(mFinish));
		}
	}
	void init()
	{
		if (mStart == NULL)
		{
			CUT_SAFE_CALL(cuEventCreate(&mStart, CU_EVENT_DEFAULT));
		}
		if (mFinish == NULL)
		{
			CUT_SAFE_CALL(cuEventCreate(&mFinish, CU_EVENT_DEFAULT));
		}
	}

	void onStart(CUstream stream)
	{
		if (mStart != NULL)
		{
			mIsStarted = true;
			CUT_SAFE_CALL(cuEventRecord(mStart, stream));
		}
	}
	void onFinish(CUstream stream)
	{
		if (mFinish != NULL && mIsStarted)
		{
			mIsFinished = true;
			CUT_SAFE_CALL(cuEventRecord(mFinish, stream));
		}
	}

	float getElapsedTime()
	{
		if (mIsStarted && mIsFinished)
		{
			mIsStarted = false;
			mIsFinished = false;
			CUT_SAFE_CALL(cuEventSynchronize(mStart));
			CUT_SAFE_CALL(cuEventSynchronize(mFinish));
			float time;
			CUT_SAFE_CALL(cuEventElapsedTime(&time, mStart, mFinish));
			return time;
		}
		else
		{
			return 0.0f;
		}
	}
private:
	CUevent mStart, mFinish;
	bool mIsStarted;
	bool mIsFinished;
};

}
} // end namespace physx::apex

#endif //__APEX_CUDA_WRAPPER_H__
