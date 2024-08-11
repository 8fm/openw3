/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "renderProxyLight.h"

/// Proxy for spot light
class CRenderProxy_SpotLight : public IRenderProxyLight
{
public:
	CRenderTexture*				m_projectionTexture;
	CRenderCamera*				m_projectionCamera;
	Float						m_innerAngle;
	Float						m_outerAngle;
	Float						m_softness;
	Float						m_cachedProjectionTextureAngle;
	Float						m_projectionTextureUBias;
	Float						m_projectionTextureVBias;
	Float						m_lastFrameTime;

protected:
	// Allocated slots for shadowmap
	CRenderShadowDynamicRegion*		m_dynamicShadowRegion;

public:
	// Get the dynamic shadowmap region
	RED_FORCE_INLINE const CRenderShadowDynamicRegion* GetDynamicShadowsRegion() const { return m_dynamicShadowRegion; }

public:
	CRenderProxy_SpotLight( const RenderProxyInitInfo& initInfo );
	virtual ~CRenderProxy_SpotLight();

	//! Relink proxy - update bounding box, local to world matrix, spatial caching etc
	virtual void Relink( const Box& boundingBox, const Matrix& localToWorld );

	//! Update param
	virtual void UpdateParameter( const CName& name, Float param );

	//! Recalculate projection texture camera
	void RecalculateProjectionTextureCamera();

	virtual IShadowmapQuery* CreateShadowmapQuery() override;

	virtual Bool ShouldPrepareDynamicShadowmaps() const override { return true; }

	virtual Bool ShouldPrepareStaticShadowmaps() const override { return false; }

	// Allocate space for shadows and render shadow maps using shadow manager
	// This only returns false in one condition: when allocating texture space has failed
	virtual Bool PrepareDynamicShadowmaps( const CRenderCollector& collector, CRenderShadowManager* shadowManager, const TDynArray< IRenderProxyBase* >& proxies );

	// Allocate space for static (cached) shadowmap
	// This only returns false in one condition: when allocating texture space has failed
	virtual Bool PrepareStaticShadowmaps( const CRenderCollector& collector, CRenderShadowManager* shadowManager, const TDynArray< IRenderProxyBase* >& proxies );

private:
	// Calculate light POV camera 
	void CalcLightCamera( CRenderCamera& outCamera ) const;
};