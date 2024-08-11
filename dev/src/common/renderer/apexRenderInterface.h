/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#ifdef USE_APEX
#include "NxUserRenderResourceManager.h"
#include "NxUserRenderer.h"
#include "NxUserRenderResource.h"
#include "NxApexRenderable.h"
#include "NxUserRenderBoneBuffer.h"
#include "NxUserRenderBoneBufferDesc.h"
#include "NxUserRenderVertexBuffer.h"
#include "NxUserRenderVertexBufferDesc.h"
#include "NxUserRenderIndexBuffer.h"
#include "NxUserRenderIndexBufferDesc.h"
#include "NxApexRenderContext.h"
#include "NxUserRenderResourceDesc.h"
#include "NxUserRenderVertexBuffer.h"

#include "../physics/physicsIncludes.h"
#include "../engine/apexDebugVisualizer.h"


class CRenderMaterial;
class CRenderMaterialParameters;
class CRenderProxy_Apex;
class CRenderElement_Apex;
class MeshDrawingStats;


#ifdef APEX_ENABLE_DEBUG_VISUALIZATION

class CApexDebugVisualizerResource : public CApexDebugVisualizerResourceBase
{
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_Default, MC_ApexRenderObject );

public:
	CApexDebugVisualizerResource( const physx::apex::NxUserRenderResourceDesc& rrDesc );
	virtual ~CApexDebugVisualizerResource();
};

#endif // APEX_ENABLE_DEBUG_VISUALIZATION



struct SApexRenderContext
{
	const physx::apex::NxApexRenderContext*	m_apexContext;		// Not null. Context provided by Apex.
	const CRenderFrameInfo*					m_frameInfo;		// Could be null, if rendering debug fragments.
	const RenderingContext*					m_renderContext;	// Not null.
	CRenderProxy_Apex*						m_currentProxy;		// Proxy being rendered. Null if rendering debug fragments.
	Matrix									m_localToWorld;
	Bool									m_dissolve;
	MeshDrawingStats*						m_meshStats;		// Collector for rendering stats. Could be null.

	SApexRenderContext()
		: m_apexContext( nullptr )
		, m_frameInfo( nullptr )
		, m_renderContext( nullptr )
		, m_currentProxy( nullptr )
		, m_dissolve( false )
		, m_meshStats( nullptr )
	{}
};


class CApexVertexBuffer : public physx::apex::NxUserRenderVertexBuffer
{
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_Default, MC_ApexRenderObject );

public:
	CApexVertexBuffer( const physx::apex::NxUserRenderVertexBufferDesc& vbDesc );
	virtual ~CApexVertexBuffer();

	Bool IsValid() const { return !m_vb.isNull(); }

	virtual void writeBuffer(const physx::NxApexRenderVertexBufferData& data, physx::PxU32 firstVertex, physx::PxU32 numVertices);

private:
	Bool writeBufferFastPath( const physx::NxApexRenderVertexBufferData& data, physx::PxU32 firstVertex, physx::PxU32 numVertices );
	void* GetLockedBufferData( physx::apex::NxRenderVertexSemantic::Enum semantic );
	Bool LockBuffer( physx::PxU32 firstVertex, physx::PxU32 numVertices );
	void UnlockBuffer();
	
public:
	physx::apex::NxUserRenderVertexBufferDesc	m_desc;

	GpuApi::VertexPacking::PackingElement		m_layout[GPUAPI_VERTEX_LAYOUT_MAX_ELEMENTS];

	// Offset of each element in m_layout within its slot.
	ptrdiff_t									m_elementOffsets[GPUAPI_VERTEX_LAYOUT_MAX_ELEMENTS];

	GpuApi::BufferRef							m_vb;
	Uint32										m_stride;

#ifndef RED_FINAL_BUILD
	// Valid range of the vertex buffer. We use discard lock when writing, so this lets us confirm we aren't losing anything we need.
	Uint32										m_validRangeBegin;
	Uint32										m_validRangeEnd;
#endif

protected:
	// Element index in m_layout for each Apex semantic. -1 if it's not in the layout.
	Int32										m_semanticElements[ physx::apex::NxRenderVertexSemantic::NUM_SEMANTICS ];

	void*										m_lockedBuffer;
	Bool										m_hasOneSemantic;

#ifdef APEX_ENABLE_DEBUG_VISUALIZATION
public:
	// If debug visualization is on, we need a way to push vertex data back to the render resource. We don't know
	// when creating the vertex buffer what it will be used for, so the debug resource needs to be able to give its
	// buffers a pointer to itself...
	CApexDebugVisualizerResource*				m_debugVisRenderResource;
#endif
};

class CApexIndexBuffer : public physx::apex::NxUserRenderIndexBuffer
{
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_SmallObjects, MC_ApexRenderObject );

public:
	CApexIndexBuffer( const physx::apex::NxUserRenderIndexBufferDesc& ibDesc );
	virtual ~CApexIndexBuffer();

	virtual void writeBuffer( const void* srcData, physx::PxU32 srcStride, physx::PxU32 firstDestElement, physx::PxU32 numElements );

	void Bind();

public:
	GpuApi::BufferRef							m_ib;
	Uint32										m_numPrimitives;
	physx::apex::NxUserRenderIndexBufferDesc	m_desc;

#ifndef RED_FINAL_BUILD
	// Range of index values used in the last writeBuffer.
	Uint32				m_vertexRangeBegin;
	Uint32				m_vertexRangeEnd;
#endif
};

class CApexBoneBuffer : public physx::apex::NxUserRenderBoneBuffer
{
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_SmallObjects, MC_ApexRenderObject );

public:
	CApexBoneBuffer( const physx::apex::NxUserRenderBoneBufferDesc& bbDesc );
	virtual ~CApexBoneBuffer();
	virtual void writeBuffer( const physx::apex::NxApexRenderBoneBufferData& data, physx::PxU32 firstBone, physx::PxU32 numBones );

	void Bind();

public:
	physx::apex::NxUserRenderBoneBufferDesc		m_desc;
	GpuApi::BufferRef							m_buffer;
	Vector										m_bindInfo;
	Bool										m_isFaded;
};

class CApexRenderResource : public physx::apex::NxUserRenderResource
{
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_Default, MC_ApexRenderObject );

public:
	CApexRenderResource( const physx::apex::NxUserRenderResourceDesc& rrDesc );
	virtual ~CApexRenderResource();

	virtual void setVertexBufferRange	( physx::PxU32 firstVertex,		physx::PxU32 numVerts		);
	virtual void setIndexBufferRange	( physx::PxU32 firstIndex,		physx::PxU32 numIndices		);
	virtual void setBoneBufferRange		( physx::PxU32 firstBone,		physx::PxU32 numBones		);
	virtual void setInstanceBufferRange	( physx::PxU32 firstInstance,	physx::PxU32 numInstances	) {}
	virtual void setSpriteBufferRange	( physx::PxU32 firstSprite,		physx::PxU32 numSprites		) {}
	virtual void setMaterial			( void* material );

	virtual physx::PxU32								getNbVertexBuffers() const					{ return m_numVB; }
	virtual physx::apex::NxUserRenderVertexBuffer*		getVertexBuffer(physx::PxU32 index) const;
	virtual physx::apex::NxUserRenderIndexBuffer*		getIndexBuffer() const						{ return m_indexBuffer; }
	virtual physx::apex::NxUserRenderBoneBuffer*		getBoneBuffer() const						{ return m_boneBuffer; }
	virtual physx::apex::NxUserRenderInstanceBuffer*	getInstanceBuffer() const					{ return nullptr; }
	virtual physx::apex::NxUserRenderSpriteBuffer*		getSpriteBuffer() const						{ return nullptr; }

	// I use capital here to show that this is not a virtual function derived from apex and because that is our coding convention
	void Render( const SApexRenderContext& context );

	physx::apex::NxUserRenderResourceDesc	m_desc;
	Uint32									m_numVB;
	CApexVertexBuffer**						m_vertexBuffers;
	CApexIndexBuffer*						m_indexBuffer;
	CApexBoneBuffer*						m_boneBuffer;
	GpuApi::ePrimitiveType					m_primitiveType;

	GpuApi::VertexLayoutRef					m_vertexLayout;

	CRenderMaterial*						m_lastRenderMaterial;
	CRenderMaterialParameters*				m_lastRenderMaterialParameters;

	Uint32 m_firstVertex, m_numVertices;
	Uint32 m_firstIndex, m_numIndices;
	Uint32 m_firstBone, m_numBones;
};


class CApexRenderer : public physx::apex::NxUserRenderer
{
public:
	CApexRenderer();
	virtual void renderResource( const physx::apex::NxApexRenderContext& context );

	const CRenderFrameInfo*		m_frameInfo;
	const RenderingContext*		m_context;

	// The material we should be rendering right now.
	CRenderMaterial*			m_currentMaterial;
	CRenderMaterialParameters*	m_currentMaterialParameters;

	// The render proxy we're currently rendering.
	CRenderProxy_Apex*			m_currentProxy;

	// The render element we're currently rendering
	CRenderElement_Apex*		m_currentElement;

	MeshDrawingStats*			m_meshStats;

	// Required for binding materials.
	// TODO: Have Apex Batcher bind the materials, so we have less stuff done here?
	Bool						m_dissolve;
};



class CApexRenderResourceManager : public physx::apex::NxUserRenderResourceManager
{
public:
	physx::apex::NxUserRenderVertexBuffer*		createVertexBuffer	( const physx::apex::NxUserRenderVertexBufferDesc& vertexBufferDesc		);
	physx::apex::NxUserRenderIndexBuffer*		createIndexBuffer	( const physx::apex::NxUserRenderIndexBufferDesc& indexBufferDesc		);
	physx::apex::NxUserRenderBoneBuffer*		createBoneBuffer	( const physx::apex::NxUserRenderBoneBufferDesc& boneBufferDesc			);
	physx::apex::NxUserRenderInstanceBuffer*	createInstanceBuffer( const physx::apex::NxUserRenderInstanceBufferDesc& instanceBufferDesc	) { return nullptr; }
	physx::apex::NxUserRenderSpriteBuffer*		createSpriteBuffer	( const physx::apex::NxUserRenderSpriteBufferDesc& spriteBufferDesc		) { return nullptr; }
	physx::apex::NxUserRenderResource*			createResource		( const physx::apex::NxUserRenderResourceDesc& resourceDesc				);

	void			releaseVertexBuffer		( physx::apex::NxUserRenderVertexBuffer& vertexBuffer		);
	void			releaseIndexBuffer		( physx::apex::NxUserRenderIndexBuffer& indexBuffer			);
	void			releaseBoneBuffer		( physx::apex::NxUserRenderBoneBuffer& boneBuffer			);
	void			releaseInstanceBuffer	( physx::apex::NxUserRenderInstanceBuffer& instanceBuffer	) {}
	void			releaseSpriteBuffer		( physx::apex::NxUserRenderSpriteBuffer& spriteBuffer		) {}
	void			releaseResource			( physx::apex::NxUserRenderResource& resource				);
	physx::PxU32	getMaxBonesForMaterial	( void*	material											);

	bool getSpriteLayoutData(physx::PxU32 spriteCount, physx::PxU32 spriteSemanticsBitmap, physx::apex::NxUserRenderSpriteBufferDesc* textureDescArray)
	{
		return false;
	}

	bool getInstanceLayoutData(physx::PxU32 spriteCount, physx::PxU32 spriteSemanticsBitmap, physx::apex::NxUserRenderInstanceBufferDesc* instanceDescArray)
	{
		return false;
	}

};

#endif
