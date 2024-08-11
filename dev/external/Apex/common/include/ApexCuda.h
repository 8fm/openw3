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

#ifndef APEX_CUDA_H
#define APEX_CUDA_H

#include <cuda.h>

//change to 1 to check CUDA error state after each kernel!
#define APEX_CUDA_CHECK_ENABLED 0


#define APEX_CUDA_CONCAT_I(arg1, arg2) arg1 ## arg2
#define APEX_CUDA_CONCAT(arg1, arg2) APEX_CUDA_CONCAT_I(arg1, arg2)

#define APEX_CUDA_TO_STR_I(arg) # arg
#define APEX_CUDA_TO_STR(arg) APEX_CUDA_TO_STR_I(arg)

const unsigned int MAX_CONST_MEM_SIZE = 65536;

const unsigned int APEX_CUDA_MEM_ALIGNMENT = 256;

const unsigned int MAX_SMEM_BANKS = 32;

#define APEX_CUDA_ALIGN_UP APEX_ALIGN_UP
#define APEX_CUDA_MEM_ALIGN_UP_32BIT(count) APEX_CUDA_ALIGN_UP(count, APEX_CUDA_MEM_ALIGNMENT >> 2)

#ifndef __CUDACC__
const unsigned int LOG2_WARP_SIZE = 5;
const unsigned int WARP_SIZE = (1U << LOG2_WARP_SIZE);
#endif

#include <boost/preprocessor/seq.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/seq/for_each_i.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/preprocessor/punctuation/comma_if.hpp>

#define __APEX_CUDA_FUNC_ARG(r, data, i, elem) BOOST_PP_COMMA_IF(i) BOOST_PP_TUPLE_ELEM(2, 0, elem) BOOST_PP_TUPLE_ELEM(2, 1, elem)
#define __APEX_CUDA_FUNC_ARGS(argseq) BOOST_PP_SEQ_FOR_EACH_I(__APEX_CUDA_FUNC_ARG, _, argseq)

#define __APEX_CUDA_FUNC_ARG_NAME(r, data, i, elem) BOOST_PP_COMMA_IF(i) BOOST_PP_TUPLE_ELEM(2, 1, elem)
#define __APEX_CUDA_FUNC_ARG_NAMES(argseq) BOOST_PP_SEQ_FOR_EACH_I(__APEX_CUDA_FUNC_ARG_NAME, _, argseq)

#define __APEX_CUDA_FUNC_SET_PARAM(r, data, elem) setParam( data, BOOST_PP_TUPLE_ELEM(2, 1, elem) );

#define __APEX_CUDA_FUNC_COPY_PARAM(r, data, elem) copyParam( BOOST_PP_TUPLE_ELEM(2, 1, elem) );

#define APEX_CUDA_NAME(name) APEX_CUDA_CONCAT(APEX_CUDA_MODULE_PREFIX, name)
#define APEX_CUDA_NAME_STR(name) APEX_CUDA_TO_STR( APEX_CUDA_NAME(name) )

#ifdef __CUDACC__

#define APEX_MEM_BLOCK(format) format*

#define APEX_CUDA_TEXTURE_1D(name, format) texture<format, 1, cudaReadModeElementType> APEX_CUDA_NAME(name);
#define APEX_CUDA_TEXTURE_2D(name, format) texture<format, 2, cudaReadModeElementType> APEX_CUDA_NAME(name);

#if defined(__CUDA_ARCH__) && __CUDA_ARCH__ < 200

#define APEX_CUDA_SURFACE_1D(name)
#define APEX_CUDA_SURFACE_2D(name)

#else

#define APEX_CUDA_SURFACE_1D(name) surface<void, 1> APEX_CUDA_NAME(name);
#define APEX_CUDA_SURFACE_2D(name) surface<void, 2> APEX_CUDA_NAME(name);

#endif

#define APEX_CUDA_CONST_MEM(name, size) __constant__ unsigned char APEX_CUDA_NAME(name)[size];

#define APEX_CUDA_FREE_KERNEL(kernelWarps, kernelName, argseq) \
	extern "C" __global__ void APEX_CUDA_NAME(kernelName)(int* _extMem, physx::PxU16 _kernelEnum, physx::PxU32 _threadCount, __APEX_CUDA_FUNC_ARGS(argseq) );

#define APEX_CUDA_FREE_KERNEL_2D(kernelDim, kernelName, argseq) \
	extern "C" __global__ void APEX_CUDA_NAME(kernelName)(int* _extMem, physx::PxU16 _kernelEnum, physx::PxU32 _threadCountX, physx::PxU32 _threadCountY, __APEX_CUDA_FUNC_ARGS(argseq) );

#define APEX_CUDA_FREE_KERNEL_3D(kernelDim, kernelName, argseq) \
	extern "C" __global__ void APEX_CUDA_NAME(kernelName)(int* _extMem, physx::PxU16 _kernelEnum, physx::PxU32 _threadCountX, physx::PxU32 _threadCountY, physx::PxU32 _threadCountZ, physx::PxU32 _blockCountY, __APEX_CUDA_FUNC_ARGS(argseq) );

#define APEX_CUDA_BOUND_KERNEL(kernelWarps, kernelName, argseq) \
	extern "C" __global__ void APEX_CUDA_NAME(kernelName)(int* _extMem, physx::PxU16 _kernelEnum, physx::PxU32 _threadCount, __APEX_CUDA_FUNC_ARGS(argseq) );

#define APEX_CUDA_SYNC_KERNEL(kernelWarps, kernelName, argseq) \
	extern "C" __global__ void APEX_CUDA_NAME(kernelName)(int* _extMem, physx::PxU16 _kernelEnum, __APEX_CUDA_FUNC_ARGS(argseq) );

#else

#define APEX_CUDA_CLASS_NAME(name) APEX_CUDA_CONCAT(CudaClass_, APEX_CUDA_NAME(name) )
#define APEX_CUDA_OBJ_NAME(name) APEX_CUDA_CONCAT(cudaObj_, APEX_CUDA_NAME(name) )

#define APEX_MEM_BLOCK(format) const ApexCudaMemRef<format>&

#define __APEX_CUDA_TEXTURE(name) \
	class APEX_CUDA_CLASS_NAME(name) : public ApexCudaTexRef { \
	public: \
		APEX_CUDA_CLASS_NAME(name) () : ApexCudaTexRef( APEX_CUDA_NAME_STR(name) ) {} \
	} APEX_CUDA_OBJ_NAME(name); \
	 
#define APEX_CUDA_TEXTURE_1D(name, format) __APEX_CUDA_TEXTURE(name)
#define APEX_CUDA_TEXTURE_2D(name, format) __APEX_CUDA_TEXTURE(name)


#define __APEX_CUDA_SURFACE(name) \
	class APEX_CUDA_CLASS_NAME(name) : public ApexCudaSurfRef { \
	public: \
		APEX_CUDA_CLASS_NAME(name) () : ApexCudaSurfRef( APEX_CUDA_NAME_STR(name) ) {} \
	} APEX_CUDA_OBJ_NAME(name); \
	 
#define APEX_CUDA_SURFACE_1D(name) __APEX_CUDA_SURFACE(name)
#define APEX_CUDA_SURFACE_2D(name) __APEX_CUDA_SURFACE(name)


#define APEX_CUDA_CONST_MEM(name, size) \
	class APEX_CUDA_CLASS_NAME(name) : public ApexCudaConstMem { \
	public: \
		APEX_CUDA_CLASS_NAME(name) () : ApexCudaConstMem( APEX_CUDA_NAME_STR(name) ) {} \
	} APEX_CUDA_OBJ_NAME(name); \
	 

#define __APEX_CUDA_KERNEL_START(name, argseq) \
	class APEX_CUDA_CLASS_NAME(name) : public ApexCudaFunc \
	{ \
		DimBlock mBlockDim; \
		int mTotalSharedMem; \
	public: \
		APEX_CUDA_CLASS_NAME(name) () : ApexCudaFunc( APEX_CUDA_NAME_STR(name) ) {} \
		const DimBlock& getBlockDim() const { return mBlockDim; } \
	protected: \
		int launch1() \
		{ \
			PX_ASSERT( isValid() ); \
			int offset = 0; \
			setParam(offset, (int*)mGpuDispatcher->getCurrentProfileBuffer()); \
			setParam(offset, mKernelID); \
			mCTContext = mCudaTestManager->isTestKernel(mName, mNxModule->getName());\
			if (mCTContext) resolveContext(); \
			return offset; \
		} \
		void launch2( int offset, CUstream stream, const DimGrid& gridDim, __APEX_CUDA_FUNC_ARGS(argseq) ) \
		{ \
			launch2opts(mBlockDim, 0, offset, stream, gridDim, __APEX_CUDA_FUNC_ARG_NAMES(argseq) ); \
		} \
		void launch2opts( const DimBlock& blockDim, physx::PxU32 sharedSize, int offset, CUstream stream, const DimGrid& gridDim, __APEX_CUDA_FUNC_ARGS(argseq) ) \
		{ \
			if (mCTContext)	{ \
				mCTContext->setCuStream(stream); \
				mCTContext->setGridDim(gridDim.x, gridDim.y); \
				mCTContext->setBlockDim(blockDim.x, blockDim.y, blockDim.z); \
				BOOST_PP_SEQ_FOR_EACH(__APEX_CUDA_FUNC_COPY_PARAM, , argseq); \
			} \
			BOOST_PP_SEQ_FOR_EACH(__APEX_CUDA_FUNC_SET_PARAM, offset, argseq); \
			void *config[5] = { \
				CU_LAUNCH_PARAM_BUFFER_POINTER, mParams, \
				CU_LAUNCH_PARAM_BUFFER_SIZE,    &offset, \
				CU_LAUNCH_PARAM_END \
			}; \
			if (mCudaProfileSession) { \
				mCudaProfileSession->onFuncStart(mProfileId, stream); \
				CUT_SAFE_CALL(cuLaunchKernel(mFunc, gridDim.x, gridDim.y, 1, blockDim.x, blockDim.y, blockDim.z, sharedSize, stream, 0, (void **)config)); \
				mCudaProfileSession->onFuncFinish(mProfileId, stream); \
			} else CUT_SAFE_CALL(cuLaunchKernel(mFunc, gridDim.x, gridDim.y, 1, blockDim.x, blockDim.y, blockDim.z, sharedSize, stream, 0, (void **)config)); \
			if (mCTContext) mCTContext->setKernelStatus(); \
		} \
		virtual void init( physx::PxCudaContextManager* ctx ) \
		{ \
			mTotalSharedMem = ctx->getSharedMemPerBlock(); \
			cuFuncGetAttribute((int*)&mMaxThreads, CU_FUNC_ATTRIBUTE_MAX_THREADS_PER_BLOCK, mFunc); \
			cuFuncGetAttribute((int*)&mStaticShared, CU_FUNC_ATTRIBUTE_SHARED_SIZE_BYTES, mFunc); \
			 
#define __APEX_CUDA_KERNEL_START_WARPS(warps, name, argseq) \
	__APEX_CUDA_KERNEL_START(name, argseq) \
	int warpsPerBlock; \
	if (ctx->supportsArchSM30()) { \
		warpsPerBlock = warps##_300; \
		PX_ASSERT( warpsPerBlock <= 32 ); \
	} else if (ctx->supportsArchSM20()) { \
		warpsPerBlock = warps##_200; \
		PX_ASSERT( warpsPerBlock <= 32 ); \
	} else if (ctx->supportsArchSM12()) { \
		warpsPerBlock = warps##_120; \
		PX_ASSERT( warpsPerBlock <= 16 ); \
	} else { \
		warpsPerBlock = warps##_110; \
		PX_ASSERT( warpsPerBlock <= 16 ); \
	} \
	mBlockDim = DimBlock( warpsPerBlock * WARP_SIZE ); \
	 
#define __APEX_CUDA_KERNEL_START_DIM(dim, name, argseq) \
	__APEX_CUDA_KERNEL_START(name, argseq) \
	if (ctx->supportsArchSM30()) { \
		mBlockDim = DimBlock dim##_300; \
	} else if (ctx->supportsArchSM20()) { \
		mBlockDim = DimBlock dim##_200; \
	} else if (ctx->supportsArchSM12()) { \
		mBlockDim = DimBlock dim##_120; \
	} else { \
		mBlockDim = DimBlock dim##_110; \
	} \
	 

#define APEX_CUDA_FREE_KERNEL(warps, name, argseq) \
	__APEX_CUDA_KERNEL_START_WARPS(warps, name, argseq) \
	} \
	public: \
	int operator() ( CUstream stream, unsigned int _threadCount, __APEX_CUDA_FUNC_ARGS(argseq) ) \
	{ \
		int blocksPerGrid = (_threadCount + mBlockDim.x - 1) / mBlockDim.x; \
		int offset = launch1(); \
		if (mCTContext)	mCTContext->setFreeKernel(_threadCount); \
		setParam(offset, _threadCount); \
		launch2( offset, stream, DimGrid(blocksPerGrid), __APEX_CUDA_FUNC_ARG_NAMES(argseq) ); \
		return blocksPerGrid; \
	} \
	} APEX_CUDA_OBJ_NAME(name); \
	 
#define APEX_CUDA_FREE_KERNEL_2D(dim, name, argseq) \
	__APEX_CUDA_KERNEL_START_DIM(dim, name, argseq) \
	} \
	public: \
	void operator() ( CUstream stream, unsigned int _threadCountX, unsigned int _threadCountY, __APEX_CUDA_FUNC_ARGS(argseq) ) \
	{ \
		DimGrid gridDim; \
		gridDim.x = (_threadCountX + mBlockDim.x - 1) / mBlockDim.x; \
		gridDim.y = (_threadCountY + mBlockDim.y - 1) / mBlockDim.y; \
		int offset = launch1(); \
		if (mCTContext)	mCTContext->setFreeKernel(_threadCountX, _threadCountY); \
		setParam(offset, _threadCountX); \
		setParam(offset, _threadCountY); \
		launch2( offset, stream, gridDim, __APEX_CUDA_FUNC_ARG_NAMES(argseq) ); \
	} \
	} APEX_CUDA_OBJ_NAME(name); \

#define APEX_CUDA_FREE_KERNEL_3D(dim, name, argseq) \
	__APEX_CUDA_KERNEL_START_DIM(dim, name, argseq) \
	} \
	public: \
	void operator() ( CUstream stream, unsigned int _threadCountX, unsigned int _threadCountY, unsigned int _threadCountZ, __APEX_CUDA_FUNC_ARGS(argseq) ) \
	{ \
		const unsigned int blockCountX = (_threadCountX + mBlockDim.x - 1) / mBlockDim.x; \
		const unsigned int blockCountY = (_threadCountY + mBlockDim.y - 1) / mBlockDim.y; \
		const unsigned int blockCountZ = (_threadCountZ + mBlockDim.z - 1) / mBlockDim.z; \
		DimGrid gridDim(blockCountX, blockCountY * blockCountZ); \
		int offset = launch1(); \
		if (mCTContext)	mCTContext->setFreeKernel(_threadCountX, _threadCountY, _threadCountZ, blockCountY); \
		setParam(offset, _threadCountX); \
		setParam(offset, _threadCountY); \
		setParam(offset, _threadCountZ); \
		setParam(offset, blockCountY); \
		launch2( offset, stream, gridDim, __APEX_CUDA_FUNC_ARG_NAMES(argseq) ); \
	} \
	} APEX_CUDA_OBJ_NAME(name); \
	 
#define APEX_CUDA_BOUND_KERNEL(warps, name, argseq) \
	__APEX_CUDA_KERNEL_START_WARPS(warps, name, argseq) \
	mMaxBlocksPerGrid = ctx->getMultiprocessorCount(); \
	} \
	private: \
	int mMaxBlocksPerGrid; \
	public: \
	int operator() ( CUstream stream, unsigned int _threadCount, __APEX_CUDA_FUNC_ARGS(argseq) ) \
	{ \
		int blocksPerGrid = (_threadCount + mBlockDim.x - 1) / mBlockDim.x; \
		if (blocksPerGrid > mMaxBlocksPerGrid) blocksPerGrid = mMaxBlocksPerGrid; \
		int offset = launch1(); \
		if (mCTContext)	mCTContext->setBoundKernel(_threadCount); \
		setParam(offset, _threadCount); \
		launch2( offset, stream, DimGrid(blocksPerGrid), __APEX_CUDA_FUNC_ARG_NAMES(argseq) ); \
		return blocksPerGrid; \
	} \
	int operator() (PxU32 sa, PxU32 sb, CUstream stream, unsigned int _threadCount, __APEX_CUDA_FUNC_ARGS(argseq) ) \
	{ \
		int sharedSize; \
		DimBlock blockDim; \
		sa <<= 2; \
		sb <<= 2; \
		PX_ASSERT(mTotalSharedMem - mStaticShared - sa >= sb); \
		const int warpCount = physx::PxMin<int>((mMaxThreads / WARP_SIZE), (mTotalSharedMem - mStaticShared - sa) / sb); \
		blockDim.x = warpCount * WARP_SIZE; \
		blockDim.y = 1; \
		blockDim.z = 1; \
		sharedSize = sa + warpCount * sb; \
		int blocksPerGrid = (_threadCount + blockDim.x - 1) / blockDim.x; \
		if (blocksPerGrid > mMaxBlocksPerGrid) blocksPerGrid = mMaxBlocksPerGrid; \
		int offset = launch1(); \
		if (mCTContext)	mCTContext->setBoundKernel(_threadCount); \
		setParam(offset, _threadCount); \
		launch2opts( blockDim, sharedSize, offset, stream, DimGrid(blocksPerGrid), __APEX_CUDA_FUNC_ARG_NAMES(argseq) ); \
		return blocksPerGrid; \
	} \
	} APEX_CUDA_OBJ_NAME(name); \
	 
#define APEX_CUDA_SYNC_KERNEL(warps, name, argseq) \
	__APEX_CUDA_KERNEL_START_WARPS(warps, name, argseq) \
	mBlocksPerGrid = ctx->getMultiprocessorCount(); \
	int funcSharedMemSize; \
	cuFuncGetAttribute(&funcSharedMemSize, CU_FUNC_ATTRIBUTE_SHARED_SIZE_BYTES, mFunc); \
	int sharedMemGranularity = (ctx->supportsArchSM20() ? 128 : 512) - 1; \
	funcSharedMemSize = (funcSharedMemSize + sharedMemGranularity) & ~sharedMemGranularity; \
	mFreeSharedMemSize = ctx->getSharedMemPerBlock() - funcSharedMemSize; \
	if (mFreeSharedMemSize < 0) mFreeSharedMemSize = 0; \
	} \
	private: \
	int mBlocksPerGrid; \
	int mFreeSharedMemSize; \
	public: \
	void operator() ( CUstream stream, __APEX_CUDA_FUNC_ARGS(argseq) ) \
	{ \
		/* alloc full shared memory for correct block distrib. on GF100 */ \
		int offset = launch1(); \
		if (mCTContext)	mCTContext->setSyncKernel(); \
		launch2opts( mBlockDim, mFreeSharedMemSize, offset, stream, DimGrid(mBlocksPerGrid), __APEX_CUDA_FUNC_ARG_NAMES(argseq) ); \
	} \
	} APEX_CUDA_OBJ_NAME(name); \
	 

#endif // #ifdef __CUDACC__

#endif //APEX_CUDA_H
