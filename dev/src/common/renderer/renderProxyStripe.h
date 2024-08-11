/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#ifndef ONLY_RENDER_PROXY_STRIPE_PROPERTIES
#include "renderProxyDrawable.h"
#endif

class IRenderResource;
class CRenderTextureBase;

/// Stripe proxy properties
struct SRenderProxyStripeProperties
{
	DECLARE_STRUCT_MEMORY_POOL( MemoryPool_SmallObjects, MC_RenderProxy );
	
	struct Vertex
	{
		Float		x, y, z;		//!< Position
		Uint32		color;			//!< Color
		Float		u, v;			//!< Diffuse texture coordinates
		Float		bu, bv;			//!< Blend texture coordinates
		Float		offset;			//!< Blend value offset
		Float		tx, ty, tz;		//!< Tangent
		Float		cx, cy, cz, sw;	//!< Stripe segment center position and width

		RED_FORCE_INLINE Uint32 CalcHash() const
		{
			return GetHash( x ) ^ GetHash( y ) ^ GetHash( z ) ^ 
				GetHash( u ) ^ GetHash( v ) ^ GetHash( color ) ^
				GetHash( bu ) ^ GetHash( bv ) ^
				GetHash( offset ) ^
				GetHash( tx ) ^ GetHash( ty ) ^ GetHash( tz );
		}
	};

	IRenderResource*					m_diffuseTexture;		//!< Diffuse texture
	IRenderResource*					m_diffuseTexture2;		//!< Diffuse texture 2
	IRenderResource*					m_normalTexture;		//!< Normal texture
	IRenderResource*					m_normalTexture2;		//!< Normal texture 2
	IRenderResource*					m_blendTexture;			//!< Blending texture
	IRenderResource*					m_depthTexture;			//!< Depth texture
	Box									m_boundingBox;			//!< Stripe bounding box
	Bool								m_projectToTerrain;		//!< Project the stripe to terrain

	// these two are used temporarily when updating the proxy
	TDynArray< Vertex >	m_vertices;				//!< Vertex buffer data
	TDynArray< Uint16 >	m_indices;				//!< Index buffer data


	SRenderProxyStripeProperties()
		: m_diffuseTexture( nullptr )
		, m_diffuseTexture2( nullptr )
		, m_normalTexture( nullptr )
		, m_normalTexture2( nullptr )
		, m_blendTexture( nullptr )
		, m_depthTexture( nullptr )
		, m_projectToTerrain( true )
	{
	}

	SRenderProxyStripeProperties( const SRenderProxyStripeProperties& src )
		: m_diffuseTexture( src.m_diffuseTexture )
		, m_diffuseTexture2( src.m_diffuseTexture2 )
		, m_normalTexture( src.m_normalTexture )
		, m_normalTexture2( src.m_normalTexture2 )
		, m_blendTexture( src.m_blendTexture )
		, m_depthTexture( src.m_depthTexture )
		, m_projectToTerrain( src.m_projectToTerrain )
		, m_vertices( src.m_vertices )
		, m_indices( src.m_indices )
	{
	}
};

#ifndef ONLY_RENDER_PROXY_STRIPE_PROPERTIES
/// Stripe proxy
class CRenderProxy_Stripe : public IRenderProxyDrawable
{
private:
	SRenderProxyStripeProperties				m_properties;			//!< Stripe properties (geometry)
	GpuApi::BufferRef							m_vertexBuffer;			//!< Vertex buffer for the stripe
	GpuApi::BufferRef							m_indexBuffer;			//!< Index buffer for the stripe
	Uint32										m_vertexCount;
	Uint32										m_indexCount;

public:
	CRenderProxy_Stripe( const RenderProxyInitInfo& info );
	virtual ~CRenderProxy_Stripe();

	//! Set the properties for this stripe
	void UpdateProperties( const SRenderProxyStripeProperties& properties );

	virtual void Prefetch( CRenderFramePrefetch* prefetch ) const override;

	//! Render?
	void Render( const class RenderingContext& context, const CRenderFrameInfo& frameInfo, CRenderCollector* renderCollector );

	//! Returns true if the stripe is projected to terrain
	RED_INLINE Bool IsProjected() const { return m_properties.m_projectToTerrain; }

	//! Collect elements for rendering
	virtual void CollectElements( CRenderCollector& collector ) override;

	//! Get all non-null textures used by this stripe.
	void CollectTextures( TDynArray< CRenderTextureBase* >& outTextures ) const;
};
#endif
