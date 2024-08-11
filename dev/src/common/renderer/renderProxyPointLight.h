/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "renderProxyLight.h"

//dex++: redone almost whole file

/// Proxy for point light
class CRenderProxy_PointLight : public IRenderProxyLight
{
private:
	// Cube mask
	Uint8		m_cubeFaceMask;

	// Slots for dynamic shadows
	CRenderShadowDynamicRegion*		m_dynamicShadowRegions[6];

	// Cached static shadows ( in cubemap )
	CRenderShadowStaticCube*		m_staticShadowCube;

public:
	// Get the dynamic shadowmap region
	RED_FORCE_INLINE const CRenderShadowDynamicRegion* GetDynamicShadowRegion( Uint32 side ) const { return m_dynamicShadowRegions[side]; }
	
	// Get the static shadowmap slot
	RED_FORCE_INLINE const CRenderShadowStaticCube* GetStaticShadowCube() const { return m_staticShadowCube; }

public:
	CRenderProxy_PointLight( const RenderProxyInitInfo& initInfo );
	virtual ~CRenderProxy_PointLight();

	virtual IShadowmapQuery* CreateShadowmapQuery() override;

	virtual Bool ShouldPrepareDynamicShadowmaps() const override { return true; }

	virtual Bool ShouldPrepareStaticShadowmaps() const override { return true; }

	// Allocate space for shadows and render shadow maps using shadow manager
	// This only returns false in one condition: when allocating texture space has failed
	virtual Bool PrepareDynamicShadowmaps( const CRenderCollector& collector, CRenderShadowManager* shadowManager, const TDynArray< IRenderProxyBase* >& proxies );

	// Allocate space for static (cached) shadowmap
	// This only returns false in one condition: when allocating texture space has failed
	virtual Bool PrepareStaticShadowmaps( const CRenderCollector& collector, CRenderShadowManager* shadowManager, const TDynArray< IRenderProxyBase* >& proxies );

	// Relink proxy - update bounding box, local to world matrix, spatial caching etc
	virtual void Relink( const Box& boundingBox, const Matrix& localToWorld );

private:
	//! Calculate light POV camera 
	void CalcLightCamera( CRenderCamera& outCamera, Uint32 side ) const;
};
