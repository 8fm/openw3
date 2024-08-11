/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../../common/engine/renderObject.h"


class CRenderDynamicDecal;
class IRenderProxyDrawable;
struct SRenderProxyDrawableClippingEllipseParams;
class CRenderSkinningData;


// The part of a decal that is attached to a single render proxy.
class CRenderDynamicDecalChunk : public IRenderObject
{
	friend CRenderDynamicDecal;

private:
	Red::Threads::CAtomic< Int32 >				m_registrationRefCount;

protected:
	CRenderDynamicDecal*	m_ownerDecal;				// Does not addref
	IRenderProxyDrawable*	m_targetProxy;				// Does not addref

public:
	CRenderDynamicDecalChunk( CRenderDynamicDecal* ownerDecal, IRenderProxyDrawable* target );
	virtual ~CRenderDynamicDecalChunk();

	RED_FORCE_INLINE CRenderDynamicDecal* GetOwnerDecal() const { return m_ownerDecal; }
	RED_FORCE_INLINE IRenderProxyDrawable* GetTargetProxy() const { return m_targetProxy; }

	RED_INLINE void OnRegistered()		{ m_registrationRefCount.Increment(); }
	RED_INLINE void OnUnregistered()		{ m_registrationRefCount.Decrement(); }
	RED_INLINE Int32 GetRegCount() const	{ return m_registrationRefCount.GetValue(); }

	Float GetTimeToLive() const;

	virtual void Render( const RenderingContext& context, const CRenderFrameInfo& frameInfo ) = 0;

	// By default, these assume they are the same as the target proxy.
	virtual const Box& GetBoundingBox() const;
	virtual const Matrix& GetLocalToWorld() const;
	virtual const SRenderProxyDrawableClippingEllipseParams* GetClippingEllipseParams() const;

	virtual Uint32 GetUsedVideoMemory() const = 0;


	// Unlink this chunk from both the owner decal and the target. After this call, the chunk is probably destroyed... unless
	// something else is still holding a reference... The caller should assume it is destroyed, unless they addref'd it themselves.
	void DestroyDecalChunk();

	// Get skinning data for this chunk. If this is not null, the decal batcher will bind the skinning data before calling Render.
	virtual CRenderSkinningData* GetSkinningData() const = 0;
	// Get vertex layout for this chunk. Used for determining a shader to bind. Can be null if desired.
	virtual GpuApi::VertexLayoutRef GetVertexLayout() const = 0;


	// Get a shader for this chunk, based on the skinning data and vertex layout given by the above functions. May return null if
	// no matching shader could be found, or vertex layout was null. If this is not null, the decal batcher will bind the shader before
	// calling Render.
	CRenderShaderTriple* GetShader() const { return SelectShader( GetSkinningData() != nullptr, GetVertexLayout() ); }

protected:
	// Select a decal rendering shader based on a few different conditions.
	CRenderShaderTriple* SelectShader( Bool isSkinned, const GpuApi::VertexLayoutRef& decalVertexLayout ) const;
};

