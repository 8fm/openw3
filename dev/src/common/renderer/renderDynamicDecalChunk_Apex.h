/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "renderDynamicDecalChunk.h"

#ifdef USE_APEX

class IRenderProxyDrawable;
class CRenderProxy_Apex;

namespace physx { namespace apex { class NxApexRenderable; } }


class CRenderDynamicDecalChunk_Apex : public CRenderDynamicDecalChunk
{
	friend class ApexDecalRenderer;
	friend class ApexDecalGenerator;

protected:
	CRenderProxy_Apex*						m_renderProxy;		// Apex render proxy we're targeting.
	GpuApi::BufferRef						m_vertexBuffer;		// Vertex buffer holding the extra decal information.
	TDynArray< struct RenderResourceInfo >	m_resourceInfo;		// Information about each render resource in the render proxy. One apex renderable can have multiple render resources.


	CRenderDynamicDecalChunk_Apex( CRenderDynamicDecal* ownerDecal, CRenderProxy_Apex* renderProxy );
public:
	virtual ~CRenderDynamicDecalChunk_Apex();

	virtual void Render( const RenderingContext& context, const CRenderFrameInfo& frameInfo );

	// The following all return null, even though when we render we might actually have something.
	//  1) The Apex decal chunk wraps all sub-chunks of a single apex proxy. We could split it up and have a separate chunk for each piece
	//     of the proxy, but we still wouldn't gain much because:
	//  2) Even if a single apex proxy was split into multiple chunks, we'd only really be able to have the skinning data and vertex layout
	//     from the previous frame. This could cause Bad Things when, for example, a cloth switches from skinned to simulated -- we would have
	//     skinning data from the previous frame, and a vertex layout that expects it, so GetShader would return a skinned version. But then
	//     we get to rendering and now we have no skinning data and a new layout. Since the batcher sets these things for us if they aren't
	//     null, we have a shader bound that expects to be skinned, and then we drop in non-skinned vertices.
	//
	// We could run through the current apex drawable and pull out skinning data, vertex layout, etc from there when these are called, but that
	// just seems rather inefficient... maybe if apex decals are a problem we can look into it more. But until then, returning null will just
	// put all the apex decals together in the batcher.
	virtual CRenderSkinningData* GetSkinningData() const { return nullptr; }
	virtual GpuApi::VertexLayoutRef GetVertexLayout() const { return GpuApi::VertexLayoutRef::Null(); }

	virtual Uint32 GetUsedVideoMemory() const;


	static CRenderDynamicDecalChunk_Apex* Create( CRenderDynamicDecal* ownerDecal, CRenderProxy_Apex* renderProxy );
};

#endif
