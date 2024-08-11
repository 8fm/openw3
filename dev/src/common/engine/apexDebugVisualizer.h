/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifdef USE_APEX

#include "renderVertices.h"

#include "NxUserRenderer.h"
#include "NxUserRenderResource.h"
#include "NxUserRenderResourceDesc.h"
#include "NxUserRenderVertexBuffer.h"


#ifndef RED_FINAL_BUILD
#define APEX_ENABLE_DEBUG_VISUALIZATION
#endif


class CRenderFrameInfo;


#ifdef APEX_ENABLE_DEBUG_VISUALIZATION


class CApexDebugVisualizerResourceBase : public physx::apex::NxUserRenderResource
{
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_Default, MC_ApexRenderObject );

public:
	CApexDebugVisualizerResourceBase( const physx::apex::NxUserRenderResourceDesc& rrDesc );
	virtual ~CApexDebugVisualizerResourceBase();


	void FillVertices( physx::apex::NxUserRenderVertexBuffer* vb, const physx::apex::NxApexRenderVertexBufferData& data, Uint32 firstVertex, Uint32 numVertices );

	// Render the debug render resource, by adding shapes to the render frame.
	void Render( CRenderFrame* frame, const physx::apex::NxApexRenderContext& context );


	//////////////////////////////////////////////////////////////////////////
	// physx::apex::NxUserRenderResource
	// Most of these will be empty stubs, since we only need a small subset of
	// the available features for debug rendering.
	virtual void setVertexBufferRange	( physx::PxU32 firstVertex,		physx::PxU32 numVerts		) override;
	virtual void setIndexBufferRange	( physx::PxU32 firstIndex,		physx::PxU32 numIndices		) override;
	virtual void setBoneBufferRange		( physx::PxU32 firstBone,		physx::PxU32 numBones		) override;
	virtual void setInstanceBufferRange	( physx::PxU32 firstInstance,	physx::PxU32 numInstances	) override;
	virtual void setSpriteBufferRange	( physx::PxU32 firstSprite,		physx::PxU32 numSprites		) override;
	virtual void setMaterial			( void* material ) override;

	virtual physx::PxU32								getNbVertexBuffers() const override;
	virtual physx::apex::NxUserRenderVertexBuffer*		getVertexBuffer( physx::PxU32 index ) const override;
	virtual physx::apex::NxUserRenderIndexBuffer*		getIndexBuffer() const override;
	virtual physx::apex::NxUserRenderBoneBuffer*		getBoneBuffer() const override;
	virtual physx::apex::NxUserRenderInstanceBuffer*	getInstanceBuffer() const override;
	virtual physx::apex::NxUserRenderSpriteBuffer*		getSpriteBuffer() const override;
	//////////////////////////////////////////////////////////////////////////

	physx::apex::NxUserRenderResourceDesc				m_desc;
	physx::apex::NxUserRenderVertexBuffer*				m_vertexBuffer;
	GpuApi::ePrimitiveType								m_primitiveType;

	// TODO : Vertex with normals might be nice
	TDynArray< DebugVertex >							m_vertices;
};


class CApexDebugVisualizerRenderer : public physx::apex::NxUserRenderer
{
private:
	CRenderFrame*		m_frame;

public:
	CApexDebugVisualizerRenderer( CRenderFrame* frame );

	virtual void renderResource( const physx::apex::NxApexRenderContext& context ) override;
};

#endif // APEX_ENABLE_DEBUG_VISUALIZATION


#endif // USE_APEX
