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

#ifndef __IOFX_RENDER_DATA_H__
#define __IOFX_RENDER_DATA_H__

#include "PsShare.h"
#include "PsUserAllocated.h"

namespace physx
{
namespace apex
{
namespace iofx
{

class IofxActor;
class IosObjectBaseData;

class IofxSharedRenderData : public physx::UserAllocated
{
public:
	virtual ~IofxSharedRenderData() = 0 {}

	virtual void alloc(IosObjectBaseData*, PxCudaContextManager*) = 0;

	virtual bool update(IosObjectBaseData* objData) = 0;

#if defined(APEX_CUDA_SUPPORT)
	static const PxU32 RESOURCE_LIST_MAX_COUNT = 8;

	virtual bool getResourceList(PxU32& count, CUgraphicsResource* list) = 0;
	virtual bool resolveResourceList(CUdeviceptr& ptr, PxU32& arrayCount, CUarray* arrayList) = 0;

	void addToMapArray(physx::Array<CUgraphicsResource> &toMapArray)
	{
		PxU32 resCount;
		CUgraphicsResource resList[RESOURCE_LIST_MAX_COUNT];

		if (!bufferIsMapped && getResourceList(resCount, resList))
		{
			for( PxU32 i = 0; i < resCount; ++i )
			{
				toMapArray.pushBack( resList[i] );
			}
		}
	}
	void addToUnmapArray(physx::Array<CUgraphicsResource> &toUnmapArray)
	{
		PxU32 resCount;
		CUgraphicsResource resList[RESOURCE_LIST_MAX_COUNT];

		if (bufferIsMapped)
		{
			if (getResourceList(resCount, resList))
			{
				for( PxU32 i = 0; i < resCount; ++i )
				{
					toUnmapArray.pushBack( resList[i] );
				}
			}
			else
			{
				bufferIsMapped = false;
			}
		}
	}
	void onMapSuccess()
	{
		PxU32 resCount;
		CUgraphicsResource resList[RESOURCE_LIST_MAX_COUNT];

		if (!bufferIsMapped && getResourceList(resCount, resList))
		{
			bufferIsMapped = true;
		}
	}
	void onUnmapSuccess()
	{
		bufferIsMapped = false;
	}
#endif

	virtual bool checkSemantics(PxU32 semantics) const = 0;

	PX_INLINE bool getUseInterop() const
	{
		return useInterop;
	}
	PX_INLINE void setUseInterop(bool value)
	{
		useInterop = value;
	}
	PX_INLINE void setBufferIsMapped(bool value)
	{
		bufferIsMapped = value;
	}

	PX_INLINE PxU32 getAllocSemantics() const
	{
		return allocSemantics;
	}
	PX_INLINE PxU32 getAllocDWords() const
	{
		return allocDWords;
	}
	PX_INLINE bool getBufferIsMapped() const
	{
		return bufferIsMapped;
	}

	PX_INLINE PxU32 getInstanceID() const
	{
		return instanceID;
	}

protected:
	IofxSharedRenderData(PxU32 instance)
		: instanceID(instance)
	{
		allocSemantics = 0;
		allocDWords = 0;

		useInterop = false;
		bufferIsMapped = false;
	}

	const PxU32	instanceID;

	PxU32	allocSemantics;
	PxU32	allocDWords;

	bool	useInterop;
	bool	bufferIsMapped;

	template<typename SemaTy>
	static bool checkSemantics(PxU32 semantics, PxU32 allocSemantics)
	{
		return (semantics & allocSemantics) == semantics;
	}

private:
	IofxSharedRenderData& operator=(const IofxSharedRenderData&);
};

class IofxSharedRenderDataMesh : public IofxSharedRenderData
{
public:
	IofxSharedRenderDataMesh(PxU32 instance);
	virtual ~IofxSharedRenderDataMesh();

	virtual void alloc(IosObjectBaseData*, PxCudaContextManager*);

	virtual bool update(IosObjectBaseData* objData);

#if defined(APEX_CUDA_SUPPORT)
	virtual bool getResourceList(PxU32& count, CUgraphicsResource* list);
	virtual bool resolveResourceList(CUdeviceptr& ptr, PxU32& arrayCount, CUarray* arrayList);
#endif

	virtual bool checkSemantics(PxU32 semantics) const
	{
		return IofxSharedRenderData::checkSemantics<NxRenderInstanceSemantic>(semantics, allocSemantics);
	}

	PX_INLINE NxUserRenderInstanceBuffer* getInstanceBuffer() const
	{
		return instanceBuffer;
	}
	PX_INLINE const NxUserRenderInstanceBufferDesc& getInstanceBufferDesc() const
	{
		return instanceBufferDesc;
	}

private:
	IofxSharedRenderDataMesh& operator=(const IofxSharedRenderDataMesh&);

	NxUserRenderInstanceBuffer*		instanceBuffer;
	NxUserRenderInstanceBufferDesc	instanceBufferDesc;
};

class IofxSharedRenderDataSprite : public IofxSharedRenderData
{
public:
	IofxSharedRenderDataSprite(PxU32 instance);
	virtual ~IofxSharedRenderDataSprite();

	virtual void alloc(IosObjectBaseData*, PxCudaContextManager*);

	virtual bool update(IosObjectBaseData* objData);

#if defined(APEX_CUDA_SUPPORT)
	virtual bool getResourceList(PxU32& count, CUgraphicsResource* list);
	virtual bool resolveResourceList(CUdeviceptr& ptr, PxU32& arrayCount, CUarray* arrayList);
#endif

	virtual bool checkSemantics(PxU32 semantics) const
	{
		return IofxSharedRenderData::checkSemantics<NxRenderSpriteSemantic>(semantics, allocSemantics);
	}

	PX_INLINE NxUserRenderSpriteBuffer* getSpriteBuffer() const
	{
		return spriteBuffer;
	}
	PX_INLINE const NxUserRenderSpriteBufferDesc& getSpriteBufferDesc() const
	{
		return spriteBufferDesc;
	}

	//TODO: maybe remove these methods and use getSpriteBufferDesc() instead
	PX_INLINE PxU32 getMaxSprites() const
	{
		return spriteBufferDesc.maxSprites;
	}
	PX_INLINE PxU32 getTextureCount() const
	{
		return spriteBufferDesc.textureCount;
	}
	PX_INLINE const NxUserRenderSpriteTextureDesc* getTextureDescArray() const
	{
		return spriteBufferDesc.textureDescs;
	}
	PX_INLINE const NxUserRenderSpriteTextureDesc& getTextureDesc(PxU32 i) const
	{
		PX_ASSERT(i < spriteBufferDesc.textureCount);
		return spriteBufferDesc.textureDescs[i];
	}

private:
	IofxSharedRenderDataSprite& operator=(const IofxSharedRenderDataSprite&);

	NxUserRenderSpriteBuffer*		spriteBuffer;
	NxUserRenderSpriteBufferDesc	spriteBufferDesc;
};


class IofxActorRenderData : public physx::UserAllocated
{
public:
	virtual void updateRenderResources(bool rewriteBuffers, void* userRenderData) = 0;
	virtual void dispatchRenderResources(NxUserRenderer& renderer) = 0;

	virtual ~IofxActorRenderData() {}

	void setSharedRenderData(IofxSharedRenderData* sharedRenderData)
	{
		mSharedRenderData = sharedRenderData;
	}

protected:
	IofxActorRenderData(IofxActor* iofxActor) : mIofxActor(iofxActor), mSharedRenderData(NULL) {}

	IofxActor* mIofxActor;
	IofxSharedRenderData* mSharedRenderData;
};

class IofxActorRenderDataMesh : public IofxActorRenderData
{
public:
	IofxActorRenderDataMesh(IofxActor* iofxActor, NxRenderMeshActor* renderMeshActor)
		: IofxActorRenderData(iofxActor), mRenderMeshActor(renderMeshActor)
	{
	}
	virtual ~IofxActorRenderDataMesh()
	{
		if (mRenderMeshActor != NULL)
		{
			mRenderMeshActor->release();
		}
	}

	virtual void updateRenderResources(bool rewriteBuffers, void* userRenderData);
	virtual void dispatchRenderResources(NxUserRenderer& renderer);

private:
	NxRenderMeshActor*			mRenderMeshActor;
};

class IofxActorRenderDataSprite : public IofxActorRenderData
{
public:
	IofxActorRenderDataSprite(IofxActor* iofxActor, void* spriteMaterial)
		: IofxActorRenderData(iofxActor), mSpriteMaterial(spriteMaterial), mRenderResource(NULL)
	{
	}
	virtual ~IofxActorRenderDataSprite()
	{
		if (mRenderResource != NULL)
		{
			NiGetApexSDK()->getUserRenderResourceManager()->releaseResource(*mRenderResource);
		}
	}

	virtual void updateRenderResources(bool rewriteBuffers, void* userRenderData);
	virtual void dispatchRenderResources(NxUserRenderer& renderer);

private:
	void*						mSpriteMaterial;
	NxUserRenderResource*		mRenderResource;
};

}
}
} // namespace apex

#endif /* __IOFX_RENDER_DATA_H__ */
