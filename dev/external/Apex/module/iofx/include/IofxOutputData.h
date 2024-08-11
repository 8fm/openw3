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

#ifndef __IOFX_OUTPUT_DATA_H__
#define __IOFX_OUTPUT_DATA_H__

#include "PsShare.h"
#include "PsUserAllocated.h"

namespace physx
{
namespace apex
{
namespace iofx
{

class IofxOutputBuffer : public physx::UserAllocated
{
	PX_NOCOPY(IofxOutputBuffer)
public:

	IofxOutputBuffer()
		: mCapacity(0)
		, mSize(0)
		, mPtr(0)
#if defined(APEX_CUDA_SUPPORT)
		, mBuffer(0)
#endif
	{
	}

	~IofxOutputBuffer()
	{
		release();
	}

	PX_INLINE size_t getCapacity() const
	{
		return mCapacity;
	}
	PX_INLINE size_t getSize() const
	{
		return mSize;
	}
	PX_INLINE void* getPtr() const
	{
		return mPtr;
	}

	bool resize(size_t capacity, size_t size, physx::PxCudaContextManager* ctx = 0)
	{
		PX_ASSERT(capacity >= size);
		if (capacity > mCapacity)
		{
			release();

#if defined(APEX_CUDA_SUPPORT)
			if (ctx != 0)
			{
				physx::PxCudaBufferType bufferType(physx::PxCudaBufferMemorySpace::T_PINNED_HOST, physx::PxCudaBufferFlags::F_READ_WRITE);
				mBuffer = ctx->getMemoryManager()->alloc(bufferType, capacity);
				if (mBuffer == 0)
				{
					PX_ASSERT(!"IofxOutputBuffer: failed to allocate Pinned Host memory!");
					return false;
				}
				mPtr = reinterpret_cast<void*>(mBuffer->getPtr());
				PX_ASSERT(mPtr != 0);
			}
			else
#endif
			{
				mPtr = physx::Allocator().allocate(capacity, __FILE__, __LINE__);
				if (mPtr == 0)
				{
					PX_ASSERT(!"IofxOutputBuffer: failed to allocate CPU memory!");
					return false;
				}
			}
			mCapacity = capacity;
		}
		mSize = size;
		return true;
	}

	void release()
	{
#if defined(APEX_CUDA_SUPPORT)
		if (mBuffer != 0)
		{
			mBuffer->free();
			mPtr = 0;
		}
		else
#endif
		if (mPtr != 0)
		{
			physx::Allocator().deallocate(mPtr);
			mPtr = 0;
		}
		mCapacity = 0;
		mSize = 0;
	}

private:
	size_t	mCapacity;
	size_t  mSize;
	void*   mPtr;
#if defined(APEX_CUDA_SUPPORT)
	physx::PxCudaBuffer* mBuffer;
#endif
};


class IofxOutputData : public physx::UserAllocated
{
public:
	virtual ~IofxOutputData() {}

	IofxOutputBuffer& getDefaultBuffer()
	{
		return mDefaultBuffer;
	}
	const IofxOutputBuffer& getDefaultBuffer() const
	{
		return mDefaultBuffer;
	}

	virtual physx::PxU32 updateSemantics(PxU32 semantics, PxU32 maxObjectCount)
	{
		PX_UNUSED(semantics);
		PX_UNUSED(maxObjectCount);
		return 0;
	}
	virtual void alloc(PxU32 numParticles, size_t capacity, size_t size, physx::PxCudaContextManager* ctx = 0)
	{
		PX_UNUSED(numParticles);
		mDefaultBuffer.resize(capacity, size, ctx);
	}

protected:
	IofxOutputData() {}

	IofxOutputBuffer mDefaultBuffer;
};

class IofxOutputDataMesh : public IofxOutputData
{
public:
	IofxOutputDataMesh() {}

	virtual physx::PxU32 updateSemantics(PxU32 semantics, PxU32 maxObjectCount)
	{
		NxUserRenderResourceManager* rrm = NiGetApexSDK()->getUserRenderResourceManager();
		bool ok = rrm->getInstanceLayoutData(maxObjectCount, semantics, &mVertexBufferDesc);
		if(!ok)
		{
			physx::PxU32 offset = 0;
			for(physx::PxU32 i = 0; i < NxRenderInstanceSemantic::NUM_SEMANTICS; ++i)
			{
				NxRenderInstanceLayoutElement::Enum semanticFormat = NxRenderInstanceLayoutElement::NUM_SEMANTICS;
				if(semantics & (1 << i))
				{
					if(i == NxRenderInstanceSemantic::POSITION)
					{
						semanticFormat = NxRenderInstanceLayoutElement::POSITION_FLOAT3;
						mVertexBufferDesc.semanticOffsets[semanticFormat] = offset;
						offset += NxRenderDataFormat::getFormatDataSize(NxRenderInstanceLayoutElement::getSemanticFormat(semanticFormat));
					}
					else if(i == NxRenderInstanceSemantic::ROTATION_SCALE)
					{
						semanticFormat = NxRenderInstanceLayoutElement::ROTATION_SCALE_FLOAT3x3;
						mVertexBufferDesc.semanticOffsets[semanticFormat] = offset;
						offset += NxRenderDataFormat::getFormatDataSize(NxRenderInstanceLayoutElement::getSemanticFormat(semanticFormat));
					}
					else if(i == NxRenderInstanceSemantic::VELOCITY_LIFE)
					{
						semanticFormat = NxRenderInstanceLayoutElement::VELOCITY_LIFE_FLOAT4;
						mVertexBufferDesc.semanticOffsets[semanticFormat] = offset;
						offset += NxRenderDataFormat::getFormatDataSize(NxRenderInstanceLayoutElement::getSemanticFormat(semanticFormat));
					}
					else if(i == NxRenderInstanceSemantic::DENSITY)
					{
						semanticFormat = NxRenderInstanceLayoutElement::DENSITY_FLOAT1;
						mVertexBufferDesc.semanticOffsets[semanticFormat] = offset;
						offset += NxRenderDataFormat::getFormatDataSize(NxRenderInstanceLayoutElement::getSemanticFormat(semanticFormat));
					}
					else if(i == NxRenderInstanceSemantic::COLOR)
					{
						semanticFormat = NxRenderInstanceLayoutElement::COLOR_FLOAT4;
						mVertexBufferDesc.semanticOffsets[semanticFormat] = offset;
						offset += NxRenderDataFormat::getFormatDataSize(NxRenderInstanceLayoutElement::getSemanticFormat(semanticFormat));
					}
					else if(i == NxRenderInstanceSemantic::UV_OFFSET)
					{
						semanticFormat = NxRenderInstanceLayoutElement::UV_OFFSET_FLOAT2;
						mVertexBufferDesc.semanticOffsets[semanticFormat] = offset;
						offset += NxRenderDataFormat::getFormatDataSize(NxRenderInstanceLayoutElement::getSemanticFormat(semanticFormat));
					}
					else if(i == NxRenderInstanceSemantic::LOCAL_OFFSET)
					{
						semanticFormat = NxRenderInstanceLayoutElement::LOCAL_OFFSET_FLOAT3;
						mVertexBufferDesc.semanticOffsets[semanticFormat] = offset;
						offset += NxRenderDataFormat::getFormatDataSize(NxRenderInstanceLayoutElement::getSemanticFormat(semanticFormat));
					}
					else if(i == NxRenderInstanceSemantic::USER_DATA)
					{
						semanticFormat = NxRenderInstanceLayoutElement::USER_DATA_UINT1;
						mVertexBufferDesc.semanticOffsets[semanticFormat] = offset;
						offset += NxRenderDataFormat::getFormatDataSize(NxRenderInstanceLayoutElement::getSemanticFormat(semanticFormat));
					}
				}
				mVertexBufferDesc.maxInstances = maxObjectCount;
				mVertexBufferDesc.stride = offset;
			}
		}
		mVertexBufferSize = mVertexBufferDesc.maxInstances * mVertexBufferDesc.stride;			
		PX_ASSERT(mVertexBufferDesc.stride % 4 == 0);
		return mVertexBufferDesc.stride >> 2;
	}

	virtual void alloc(PxU32 numParticles, size_t capacity, size_t size, physx::PxCudaContextManager* ctx = 0)
	{
		PX_UNUSED(size);
		IofxOutputData::alloc(numParticles, capacity, mVertexBufferSize, ctx);
	}

	PX_INLINE const NxUserRenderInstanceBufferDesc& getVertexDesc() const
	{
		return mVertexBufferDesc;
	}
private:
	PxU32								mVertexBufferSize;
	NxUserRenderInstanceBufferDesc		mVertexBufferDesc;
};

class IofxOutputDataSprite : public IofxOutputData
{
public:
	IofxOutputDataSprite()
	{
	}

	virtual physx::PxU32 updateSemantics(PxU32 semantics, PxU32 maxObjectCount)
	{
		NxUserRenderResourceManager* rrm = NiGetApexSDK()->getUserRenderResourceManager();
		bool ok = rrm->getSpriteLayoutData(maxObjectCount, semantics, &mVertexBufferDesc);
		/* User prefers render from texture */
		if(ok) 
		{
			if(mVertexBufferDesc.textureCount > 0)
			{
				physx::PxU32 stride = 0;
				for (PxU32 i = 0; i < mVertexBufferDesc.textureCount; ++i)
				{
					mTextureElemSizeArray[i] = NxRenderDataFormat::getFormatDataSize( NxRenderSpriteTextureLayout::getLayoutFormat(mVertexBufferDesc.textureDescs[i].layout) );
					stride += mTextureElemSizeArray[i];
				}
				return stride >> 2;
			}
			/* If user prefers custom VBO layout */
			else 
			{
				if(mVertexBufferDesc.maxSprites * mVertexBufferDesc.stride == 0)
				{
					ok = false;
				}
			}
		}

		/* Either user returned true, but set zero mVertexBufferDesc.maxSprites or mVertexBufferDesc.stride
		    or user return false. In both cases construct default layout containing all present semantics */
		if(!ok)
		{
			physx::PxU32 offset = 0;
			for(physx::PxU32 i = 0; i < NxRenderSpriteSemantic::NUM_SEMANTICS; ++i)
			{
				NxRenderSpriteLayoutElement::Enum semanticFormat = NxRenderSpriteLayoutElement::NUM_SEMANTICS;
				if(semantics & (1 << i))
				{
					if(i == NxRenderSpriteSemantic::POSITION)
					{
						semanticFormat = NxRenderSpriteLayoutElement::POSITION_FLOAT3;
						mVertexBufferDesc.semanticOffsets[semanticFormat] = offset;
						offset += NxRenderDataFormat::getFormatDataSize(NxRenderSpriteLayoutElement::getSemanticFormat(semanticFormat));
					}
					else if(i == NxRenderSpriteSemantic::COLOR)
					{
						semanticFormat = NxRenderSpriteLayoutElement::COLOR_FLOAT4;
						mVertexBufferDesc.semanticOffsets[semanticFormat] = offset;
						offset += NxRenderDataFormat::getFormatDataSize(NxRenderSpriteLayoutElement::getSemanticFormat(semanticFormat));
					}
					else if(i == NxRenderSpriteSemantic::VELOCITY)
					{
						semanticFormat = NxRenderSpriteLayoutElement::VELOCITY_FLOAT3;
						mVertexBufferDesc.semanticOffsets[semanticFormat] = offset;
						offset += NxRenderDataFormat::getFormatDataSize(NxRenderSpriteLayoutElement::getSemanticFormat(semanticFormat));
					}
					else if(i == NxRenderSpriteSemantic::SCALE)
					{
						semanticFormat = NxRenderSpriteLayoutElement::SCALE_FLOAT2;
						mVertexBufferDesc.semanticOffsets[semanticFormat] = offset;
						offset += NxRenderDataFormat::getFormatDataSize(NxRenderSpriteLayoutElement::getSemanticFormat(semanticFormat));
					}
					else if(i == NxRenderSpriteSemantic::LIFE_REMAIN)
					{
						semanticFormat = NxRenderSpriteLayoutElement::LIFE_REMAIN_FLOAT1;
						mVertexBufferDesc.semanticOffsets[semanticFormat] = offset;
						offset += NxRenderDataFormat::getFormatDataSize(NxRenderSpriteLayoutElement::getSemanticFormat(semanticFormat));
					}
					else if(i == NxRenderSpriteSemantic::DENSITY)
					{
						semanticFormat = NxRenderSpriteLayoutElement::DENSITY_FLOAT1;
						mVertexBufferDesc.semanticOffsets[semanticFormat] = offset;
						offset += NxRenderDataFormat::getFormatDataSize(NxRenderSpriteLayoutElement::getSemanticFormat(semanticFormat));
					}
					else if(i == NxRenderSpriteSemantic::SUBTEXTURE)
					{
						semanticFormat = NxRenderSpriteLayoutElement::SUBTEXTURE_FLOAT1;
						mVertexBufferDesc.semanticOffsets[semanticFormat] = offset;
						offset += NxRenderDataFormat::getFormatDataSize(NxRenderSpriteLayoutElement::getSemanticFormat(semanticFormat));
					}
					else if(i == NxRenderSpriteSemantic::ORIENTATION)
					{
						semanticFormat = NxRenderSpriteLayoutElement::ORIENTATION_FLOAT1;
						mVertexBufferDesc.semanticOffsets[semanticFormat] = offset;
						offset += NxRenderDataFormat::getFormatDataSize(NxRenderSpriteLayoutElement::getSemanticFormat(semanticFormat));
					}
					else if(i == NxRenderSpriteSemantic::USER_DATA)
					{
						semanticFormat = NxRenderSpriteLayoutElement::USER_DATA_UINT1;
						mVertexBufferDesc.semanticOffsets[semanticFormat] = offset;
						offset += NxRenderDataFormat::getFormatDataSize(NxRenderSpriteLayoutElement::getSemanticFormat(semanticFormat));
					}
				}
				mVertexBufferDesc.maxSprites = maxObjectCount;
				mVertexBufferDesc.stride = offset;
				mVertexBufferDesc.textureCount = 0;
			}
		}
		mVertexBufferSize = mVertexBufferDesc.maxSprites * mVertexBufferDesc.stride;
		PX_ASSERT(mVertexBufferDesc.stride % 4 == 0);
		return mVertexBufferDesc.stride >> 2;
	}

	virtual void alloc(PxU32 numParticles, size_t capacity, size_t size, physx::PxCudaContextManager* ctx = 0)
	{
		PX_UNUSED(size);
		if (mVertexBufferDesc.textureCount > 0)
		{
			for (PxU32 i = 0; i < mVertexBufferDesc.textureCount; ++i)
			{
				const NxUserRenderSpriteTextureDesc& textureDesc = mVertexBufferDesc.textureDescs[i];
				const PxU32 textureElemSize = mTextureElemSizeArray[i];

				const size_t textureCapacity = textureDesc.pitchBytes * textureDesc.height;
				const size_t textureSize = (numParticles / textureDesc.width) * textureDesc.pitchBytes + (numParticles % textureDesc.width) * textureElemSize;
				
				mTextureBuffer[i].resize(textureCapacity, textureSize, ctx);
			}
			for (PxU32 i = mVertexBufferDesc.textureCount; i < NxUserRenderSpriteBufferDesc::MAX_SPRITE_TEXTURES; ++i)
			{
				mTextureBuffer[i].release();
			}
		}
		else
		{
			IofxOutputData::alloc(numParticles, capacity, mVertexBufferSize, ctx);
		}
	}


	PX_INLINE PxU32 getTextureCount() const
	{
		return mVertexBufferDesc.textureCount;
	}

	PX_INLINE const NxUserRenderSpriteTextureDesc* getTextureDescArray() const
	{
		return mVertexBufferDesc.textureDescs;
	}

	PX_INLINE const NxUserRenderSpriteBufferDesc& getVertexDesc() const
	{
		return mVertexBufferDesc;
	}

	const IofxOutputBuffer& getTextureBuffer(PxU32 index) const
	{
		PX_ASSERT(index < mVertexBufferDesc.textureCount);
		return mTextureBuffer[index];
	}

private:
	PxU32							mVertexBufferSize;
	NxUserRenderSpriteBufferDesc	mVertexBufferDesc;
	PxU32							mTextureElemSizeArray[NxUserRenderSpriteBufferDesc::MAX_SPRITE_TEXTURES];

	IofxOutputBuffer				mTextureBuffer[NxUserRenderSpriteBufferDesc::MAX_SPRITE_TEXTURES];
};

}
}
} // namespace apex

#endif /* __IOFX_OUTPUT_DATA_H__ */
