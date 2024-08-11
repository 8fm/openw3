/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

/// Shadow border size - needs to be large enough to account for filtering
#define SHADOW_BORDER_SIZE		4

class CRenderShadowDynamicRegion;
class CRenderShadowStaticCube;
class IRenderProxyDrawable;

/// Allocators
class CRenderShadowDynamicAllocator;
class CRenderShadowStaticAllocator;

/// General shadow manager
class CRenderShadowManager
{
public:

	static const Uint32						NUM_DYNAMIC_SLICES = 2;
	static const Uint32						DYNAMIC_SHADOWMAP_RESOLUTION = 1024;
	static const Uint32						NUM_STATIC_CUBES = 16;
	static const Uint32						STATIC_SHADOWMAP_RESOLUTION = 256;

private:
	// Dynamic region to render
	struct DrawEntryDynamic
	{
		DrawEntryDynamic*				m_pointer;			//!< Internal list pointer
		CRenderShadowDynamicRegion*		m_region;			//!< Texture region
		CRenderCamera					m_camera;			//!< Shadow camera ( JUST A REFERENCE )
		Uint32							m_firstProxy;		//!< Index of first proxy to draw
		Uint32							m_numProxies;		//!< Number of proxies to draw
	};

	// Static region to render
	struct DrawEntryStatic
	{
		CRenderShadowStaticCube*		m_cube;				//!< Cube to update
		Vector							m_lightPosition;	//!< Position of the light
		Float							m_lightRadius;		//!< Radius of the light
		Uint32							m_firstProxy;		//!< Index of first proxy to draw
		Uint32							m_numProxies;		//!< Number of proxies to draw
	};

private:
	//! Shadow space allocator and manager for dynamic shadows
	CRenderShadowDynamicAllocator*			m_dynamicAllocator;

	//! Shadow space allocator and manager for static shadows
	CRenderShadowStaticAllocator*			m_staticAllocator;

	//! Global proxy list for dynamic shadows
	TDynArray< IRenderProxyDrawable* >		m_drawProxiesDynamic;

	//! Global proxy list for static shadows
	TDynArray< IRenderProxyDrawable* >		m_drawProxiesStatic;

	//! Draw list for dynamic objects
	TDynArray< DrawEntryDynamic >			m_drawListDynamic;

	//! Draw list for static objects
	TDynArray< DrawEntryStatic >			m_drawListStatic;

	GpuApi::TextureRef						m_sharedDepth;

public:
	CRenderShadowManager();
	~CRenderShadowManager();

	//! Get the dynamic atlas texture
	GpuApi::TextureRef GetDynamicAtlas() const;

	//! Get the static atlas texture
	GpuApi::TextureRef GetStaticAtlas() const;

	//! Get shared depth for dynamic and static lights
	GpuApi::TextureRef GetDepth() const { return m_sharedDepth; }

	//! Reset dynamic shadow region packing
	void ResetDynamicPacking();

	//! Reset static shadow region packing
	void ResetStaticPacking();

	//! Allocate dynamic region of shadowmap ( can return empty region )
	CRenderShadowDynamicRegion* AllocateDynamicRegion( Uint16 size );

	//! Allocate static cube for static shadows
	CRenderShadowStaticCube* AllocateCube( Uint32 currentFrameIndex, const Box& lightBounds );

	//! Invalidate cached static lights touching given bounds
	void InvalidateStaticLights( const Box& box );

	//! Add a draw list for dynamic region
	void AddDynamicRegionToRender( CRenderShadowDynamicRegion* region, const CRenderCamera& shadowCamera, const TDynArray< IRenderProxyDrawable* >& dynamicProxies );

	//! Add a draw list for a static cubemap shadow
	void AddStaticCubeToRender( CRenderShadowStaticCube* cube, const Vector& lightPos, Float lightRadius, const TDynArray< IRenderProxyDrawable* >& staticProxies );

	//! Render all dynamic shadowaps
	void RenderDynamicShadowmaps( const CRenderCollector& collector );

	//! Render all static shadowmaps that requested updating
	void RenderStaticShadowmaps( const CRenderCollector& collector );
};
