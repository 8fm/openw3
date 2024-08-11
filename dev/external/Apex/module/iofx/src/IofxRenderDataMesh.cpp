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

#include "NxApex.h"
#include "IofxScene.h"
#include "IosObjectData.h"
#include "IofxRenderData.h"
#include "IofxActor.h"

namespace physx
{
namespace apex
{
namespace iofx
{

void IofxActorRenderDataMesh::updateRenderResources(bool rewriteBuffers, void* userRenderData)
{
	PX_ASSERT(mRenderMeshActor != NULL);
	if (mRenderMeshActor == NULL)
	{
		return;
	}

	NxUserRenderInstanceBuffer* instanceBuffer = DYNAMIC_CAST(IofxSharedRenderDataMesh*)(mSharedRenderData)->getInstanceBuffer();
	if (mRenderMeshActor->getInstanceBuffer() != instanceBuffer)
	{
		mRenderMeshActor->setInstanceBuffer(instanceBuffer);
		//mSemantics = obj->allocSemantics;
	}

	PX_ASSERT( mRenderMeshActor->getInstanceBuffer() == instanceBuffer );

	const ObjectRange& range = mIofxActor->mRenderRange;
	mRenderMeshActor->setInstanceBufferRange(range.startIndex, range.objectCount);
	mRenderMeshActor->updateRenderResources(rewriteBuffers, userRenderData);
}

void IofxActorRenderDataMesh::dispatchRenderResources(NxUserRenderer& renderer)
{
	PX_ASSERT(mRenderMeshActor != NULL);
	if (mRenderMeshActor == NULL)
	{
		return;
	}

	mRenderMeshActor->dispatchRenderResources(renderer);
}


IofxSharedRenderDataMesh::IofxSharedRenderDataMesh(PxU32 instance)
	: IofxSharedRenderData(instance)
{
	instanceBuffer = NULL;
}

IofxSharedRenderDataMesh::~IofxSharedRenderDataMesh()
{
	NxUserRenderResourceManager* rrm = NiGetApexSDK()->getUserRenderResourceManager();
	if (instanceBuffer != NULL)
	{
		rrm->releaseInstanceBuffer(*instanceBuffer);
		instanceBuffer = NULL;
	}
}

void IofxSharedRenderDataMesh::alloc(IosObjectBaseData* objData, PxCudaContextManager* ctxman)
{
	if (useInterop ^ (ctxman != NULL))
	{
		//ignore this call
		return;
	}

	NxUserRenderResourceManager* rrm = NiGetApexSDK()->getUserRenderResourceManager();
	const IofxOutputDataMesh* outputData = static_cast<const IofxOutputDataMesh*>(objData->outputData);
	//TODO: check instanceBufferDesc difference here
	if (objData->outputSemantics != allocSemantics)
	{
		if (instanceBuffer != NULL)
		{
			rrm->releaseInstanceBuffer(*instanceBuffer);
			instanceBuffer = NULL;
			instanceBufferDesc.setDefaults();

			allocSemantics = 0;
			allocDWords = 0;
		}

		if (objData->outputSemantics != 0)
		{
			NxUserRenderInstanceBufferDesc desc = outputData->getVertexDesc();

			desc.hint = NxRenderBufferHint::DYNAMIC;
			desc.registerInCUDA = useInterop;
			desc.interopContext = useInterop ? ctxman : NULL;

			for(physx::PxU32 i = 0; i < NxRenderInstanceLayoutElement::NUM_SEMANTICS; ++i)
			{
				if(desc.semanticOffsets[i] != -1)
				{
					desc.semanticFormats[NxRenderInstanceLayoutElement::getSemantic(static_cast<NxRenderInstanceLayoutElement::Enum>(i))] = 
						NxRenderInstanceLayoutElement::getSemanticFormat(static_cast<NxRenderInstanceLayoutElement::Enum>(i));
				}
			}

			PX_ASSERT(objData->outputDWords <= MESH_MAX_DWORDS_PER_OUTPUT);
			instanceBuffer = rrm->createInstanceBuffer(desc);
			if (instanceBuffer != NULL)
			{
				instanceBufferDesc = desc;

				allocSemantics = objData->outputSemantics;
				allocDWords = objData->outputDWords;
			}
		}
		bufferIsMapped = false;
		objData->writeBufferCalled = false;
	}
}

bool IofxSharedRenderDataMesh::update(IosObjectBaseData* objData)
{
	if (useInterop)
	{
		//ignore this call in case of interop
		return false;
	}

	// IOFX manager will set writeBufferCalled = true when it writes directly to the mapped buffer
	if (objData->writeBufferCalled == false)
	{
		if (objData->outputData->getDefaultBuffer().getSize() == 0)
		{
			return false;
		}
		const IofxOutputDataMesh* outputData = static_cast<const IofxOutputDataMesh*>(objData->outputData);

		PX_ASSERT(objData->outputSemantics == allocSemantics);
		PX_ASSERT(objData->outputDWords == allocDWords);

		PxU32* outputBuffer = static_cast<PxU32*>( outputData->getDefaultBuffer().getPtr() );
		instanceBuffer->writeBuffer(outputBuffer, 0, objData->numParticles);
		objData->writeBufferCalled = true;
	}
	return true;
}


#if defined(APEX_CUDA_SUPPORT)
bool IofxSharedRenderDataMesh::getResourceList(PxU32& count, CUgraphicsResource* list)
{
	if (instanceBuffer)
	{
		count = 1;
		return instanceBuffer->getInteropResourceHandle(list[0]);
	}
	else
	{
		count = 0;
		return false;
	}
}

bool IofxSharedRenderDataMesh::resolveResourceList(CUdeviceptr& ptr, PxU32& arrayCount, CUarray* arrayList)
{
	PX_UNUSED(arrayList);
	if (instanceBuffer)
	{
		CUgraphicsResource resourceHandle;
		if (instanceBuffer->getInteropResourceHandle(resourceHandle))
		{
			size_t size = 0;
			CUresult ret = cuGraphicsResourceGetMappedPointer(&ptr, &size, resourceHandle);
			if (ret == CUDA_SUCCESS && size > 0)
			{
				arrayCount = 0;
				return true;
			}
		}
	}
	return false;
}
#endif

}
}
} // namespace physx::apex
