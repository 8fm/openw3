/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "renderProxyDrawable.h"

enum EDynamicDecalRenderIndex
{
	EDDI_REGULAR = 0,
	EDDI_SPECULARED ,
	EDDI_NORMALMAPPED ,
	EDDI_SPECULARED_NORMALMAPPED ,
	EDDI_MAX
};

struct SDynamicDecalInitInfo;

/// Screenspace decal proxy
class CRenderProxy_Decal : public IRenderProxyDrawable
{
	DECLARE_RENDER_OBJECT_MEMORYCLASS( MC_RenderProxyDecal )
public:
	RED_INLINE CRenderTexture*		GetTexture() const { return m_diffuseTexture; }
	
public:
	CRenderProxy_Decal( const RenderProxyInitInfo& info );
	CRenderProxy_Decal( const SDynamicDecalInitInfo& decalInfo );
	virtual ~CRenderProxy_Decal();

public:
	virtual void Prefetch( CRenderFramePrefetch* prefetch ) const override;

	virtual void AttachToScene( CRenderSceneEx* scene ) override;
	virtual void DetachFromScene( CRenderSceneEx* scene ) override;

	virtual void Relink( const Box& boundingBox, const Matrix& localToWorld ) override;
	virtual void RelinkTransformOnly( const Matrix& localToWorld ) override;
	virtual void RelinkBBoxOnly( const Box& boundingBox ) override;

	void Render( const class RenderingContext& context, const CRenderFrameInfo& frameInfo );

	void SetAlpha( Float alpha ) { m_alpha = Clamp( alpha, 0.0f, 1.0f ); }
	void SetScale( Float scale ) { m_scale = Clamp( scale, 0.001f, 1.0f ); }

	// Bucketing shit
	RED_INLINE EDynamicDecalRenderIndex	GetRenderIndex() const 
	{ 
		if( m_normalTexture )
			return m_specularity < 0.0f ? EDDI_NORMALMAPPED : EDDI_SPECULARED_NORMALMAPPED;  
		return m_specularity < 0.0f ? EDDI_REGULAR : EDDI_SPECULARED;  
	};

public:
	//! Collect elements for rendering
	virtual void CollectElements( CRenderCollector& collector ) override;

	virtual void OnNotVisibleFromAutoHide( CRenderCollector& collector ) override;

	struct SDecalConstants
	{
		Matrix	m_cachedProjection;
		Vector	m_cachedSubUVClip;
		Vector	m_specularColor;
		Float	m_normalThreshold;
		Float	m_decalFade;
		Float	m_specularity;
		Float	m_scale;
		Vector	m_diffuseColor;
		Vector	m_tangent;
		Vector	m_up;
		Float	m_Zdepthfade;
	};

private:
	CRenderTexture*					m_diffuseTexture;
	CRenderTexture*					m_normalTexture;

	Float							m_normalThreshold;	//!< in radians
	Float							m_alpha;
	Float							m_scale;			// multiplier used to scale the decal inside the UVs over time
	Bool							m_useVerticalProjection; 
	Bool							m_verticalFlip;
	Bool							m_horizontalFlip;
	Color							m_diffuseColor;
	Vector							m_atlasVector;
	Color							m_specularColor;
	Float							m_specularity;

	Int32							m_lastUpdateFrame;	//!< Last frame we were collected and updated.

	Matrix							m_cachedProjection;
	Vector							m_cachedSubUVClip;
	Vector							m_tangent;		//!< Cached values for transforming texture normal from projection space to world frame
	Vector							m_up;
	Float							m_Zdepthfade;

private:
	static GpuApi::BufferRef		g_vertices;
	static GpuApi::BufferRef		g_indices;
	static GpuApi::BufferRef		g_constants;

private:
	const EFrameUpdateState UpdateOncePerFrame( const CRenderCollector& collector );

	void UpdateFade( const CRenderCollector& collector, const Bool wasVisibleLastFrame );

	void CacheProjectionMatrix();
	void UpdateConstants();

};
