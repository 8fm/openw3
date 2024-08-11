/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "renderProxyFadeable.h"
#include "renderShadowRegions.h"
#include "../engine/renderCommands.h"

class CRenderDistantLightBatcher;

class IShadowmapQuery
{
public:
	IShadowmapQuery( IRenderProxyLight* light, TDynArray< IRenderProxyBase* >& proxies ) : m_light( light ), m_proxies( proxies ) {}
	virtual ~IShadowmapQuery() {}
	virtual Bool Test( const Vector3& min, const Vector3& max ) const = 0;
	TDynArray< IRenderProxyBase* >& m_proxies;
	IRenderProxyLight* m_light;
};

class BoxQuery : public IShadowmapQuery
{
	Box	m_box;
public:
	BoxQuery( IRenderProxyLight* light, const Box& box, TDynArray< IRenderProxyBase* >& proxies ) : IShadowmapQuery( light, proxies ), m_box( box ) {}	
	virtual Bool Test( const Vector3& min, const Vector3& max ) const override { return m_box.Touches( min, max ); }
};

class FrustumQuery : public IShadowmapQuery
{
	CFrustum m_frustum;
public:
	FrustumQuery( IRenderProxyLight* light, const CFrustum& frustum, TDynArray< IRenderProxyBase* >& proxies ) : IShadowmapQuery( light, proxies ), m_frustum( frustum ) {}
	virtual Bool Test( const Vector3& min, const Vector3& max ) const override { return m_frustum.TestBox( min, max ) != FR_Outside; }
};

struct SLightFactorInfo
{
	Bool m_isBlack;
	Bool m_isDistantLight;

	SLightFactorInfo()
		: m_isBlack( false )
		, m_isDistantLight( false )
	{}

	RED_FORCE_INLINE Bool Visible() const { return !m_isBlack || m_isDistantLight; }
	RED_FORCE_INLINE Bool VisibleNear() const { return !m_isBlack; }
	RED_FORCE_INLINE Bool VisibleDistance() const { return m_isDistantLight; }

};

struct SLightFlickerInfo
{
	DECLARE_STRUCT_MEMORY_POOL( MemoryPool_SmallObjects, MC_RenderProxy );
		
	Vector			m_nextOffset;
	Float			m_maxOffset;
	Float			m_flickeringStrength;
	Float			m_flickerPeriod;	
	Float			m_flickerTimeCounter;
	Float			m_positionFlickerTimeCounter;
	Float			m_prevMultiplier;
	Float			m_nextMultiplier;

	Float			m_oscillatingTimeDelta;
	
};

/// Base proxy for light
class IRenderProxyLight : public IRenderProxyFadeable
{
protected:
	Matrix									m_lightToWorld;				//!< Light to world matrix
	Matrix									m_worldToLight;				//!< Light matrix
	Color									m_color;					//!< Light color
	Float									m_radius;					//!< Light radius
	Float									m_brightness;				//!< Light brightness
	Float									m_attenuation;				//!< Light attenuation
	Float									m_autoHideRange;			//!< Auto hide range
	Float									m_autoHideInvRange;			//!< Auto hide inverse range
	Float									m_shadowBlendFactor;		//!< Shadow blend factor
	SLightFlickerInfo*						m_lightFlickerInfo;			//!< Light flicker info
	EEnvColorGroup							m_envColorGroup;			//!< Environment color group
	Bool									m_allowDistantFade;			//!< Allow to become distant light
	Bool									m_forcedDynamic;			//!< Does component have a parent transform
	Int32									m_sceneEnvProbeLightIndex;	//!< Scene env probe light index

	// new code for shadows
	Bool									m_isCastingStaticShadows:1;		//!< Light is casting shadows from STATIC geometry
	Bool									m_isCastingDynamicShadows:1;	//!< Light is casting shadows from DYNAMIC geometry
	Bool									m_staticShadowsCached:1;		//!< Static shadows are cached
	Float									m_shadowFadeDistance;			//!< Distance from light at which the shadow should start to fade away
	Float									m_shadowFadeInvRange;			//!< Shadow fading 1/range
	Uint16									m_currentShadowResolution;		//!< Calculated resolution of shadows
	Uint16									m_lightUsageMask;				//!< Light flags determining in which situations light is used (Uint16 instead of 32)
	Float									m_currentShadowAlpha;			//!< Calculated shadow visibility

	//!< Last frame id this proxy was updated
	Red::Threads::CAtomic< Int32 >			m_updateFrameIndex;		

	TDynArray< IRenderProxyBase* >			m_shadowProxies;


public:
	//! Get environment color group
	RED_INLINE EEnvColorGroup GetEnvColorGroup() const { return m_envColorGroup; }

	//! Get auto hide inverse range
	RED_INLINE Float GetAutoHideRange() const { return m_autoHideRange; }

	//! Get auto hide inverse range
	RED_INLINE Float GetAutoHideInvRange() const { return m_autoHideInvRange; }

	//! Get light radius
	RED_INLINE Float GetRadius() const { return m_radius; }

	//! Get light attenuation
	RED_INLINE Float GetAttenuation() const { return m_attenuation; }

	//! Get light usage mask
	RED_INLINE Uint32 GetLightUsageMask() const { return m_lightUsageMask; }

	//! Get light to world matrix
	RED_INLINE const Matrix& GetLightToWorld() const { return m_lightToWorld; }

	//! Get requested shadow resolution
	RED_INLINE Uint16 GetCurrentShadowResolution() const { return m_currentShadowResolution; }

	//! Get current (calculated) shadow alpha
	RED_INLINE Float GetCurrentShadowAlpha() const { return m_currentShadowAlpha; }

	//! Is this light casting shadows from static geometry
	RED_INLINE Bool IsCastingStaticShadows() const { return m_isCastingStaticShadows; }

	//! Is this light casting shadows from dynamic geometry
	RED_INLINE Bool IsCastingDynamicShadows() const { return m_isCastingDynamicShadows; }

	//! Does this light have cached static shadows ?
	RED_INLINE Bool IsStaticShadowsCached() const { return m_staticShadowsCached; }

	RED_INLINE Bool IsForcedDynamic() const { return m_forcedDynamic; }

	RED_INLINE Float GetLightHideDistance() const { return GetAutoHideDistance() + GetRadius() + GetAutoHideRange(); }
public:
	/// Get scene envprobe light index
	Int32 GetSceneEnvProbeLightIndex() const;

	/// Set scene envprobe light index
	void SetSceneEnvProbeLightIndex( Int32 index );

public:
	IRenderProxyLight( ERenderProxyType type, const RenderProxyInitInfo& initInfo );
	virtual ~IRenderProxyLight();

	// Returns true if light affects given bounds
	virtual Bool AffectsBounds( const Box& bounds ) const;

	//! Relink proxy - update bounding box, local to world matrix, spatial caching etc
	virtual void Relink( const Box& boundingBox, const Matrix& localToWorld );

	//! Update param
	virtual void UpdateParameter( const CName& name, Float param );

	//! Update lighting parameter
	virtual void UpdateLightParameter( const Vector& data, ERenderLightParameter param );

	//! Update color
	virtual void UpdateColor( Color color );

	//! Attach to scene
	virtual void AttachToScene( CRenderSceneEx* scene );

	//! Flickering
	virtual void CalcOffsetVector( Vector& outVector ) const;

	//! Calculate brightness multiplier
	virtual Float CalcBrightnessMultiplier() const;

	//! Collect elements for rendering
	virtual void CollectElements( CRenderCollector& collector );

	//! This is a drawable proxy
	virtual Bool IsLight() const override { return true; }

	//! Fast access interface for drawable proxies
	virtual IRenderProxyLight* AsLight() override { return this; }
	virtual const IRenderProxyLight* AsLight() const override { return this; }

public:
	//! Calculate auto hide factor for given camera position ( 1.f means full visibility )
	Vector2 CalculateAutoHideFactor( const Vector &cameraPosition, const SWorldRenderSettings &worldRenderSettings, Bool allowDistantLight = true ) const;

	//! Return true if light is black
	SLightFactorInfo GetDistanceInfo( const Vector &cameraPosition, const SWorldRenderSettings &worldRenderSettings, const CEnvColorGroupsParametersAtPoint &colorGroupsParams, Bool allowDistantLight = true ) const;

	//! Is rendered into scene
	Bool IsRenderedToScene() const;

	//! Is rendered into envprobes
	Bool IsRenderedToEnvProbes() const;

	//! Render light (actually add to the instance batcher redner list)
	void GatherDistantLight( const CRenderFrameInfo& info, CRenderDistantLightBatcher& batcher );

	Vector GetFinalColor( const Vector &cameraPosition, const SWorldRenderSettings &worldRenderSettings, const CEnvColorGroupsParametersAtPoint &colorGroupsParams, Bool allowDistantLight = true ) const;

	Vector GetFinalColorNoFade( const CEnvColorGroupsParametersAtPoint &colorGroupsParams ) const;

	Vector GetFinalPosition() const;

	Vector GetBasePosition() const;

	//: update shadow related parameters, return true if decided to cast dynamic shadows this frame
	Bool UpdateShadowParams( const CRenderCollector& collector );

	// Try to reduce the shadowmap allocation
	void ReduceDynamicShadowmapSize();

	virtual IShadowmapQuery* CreateShadowmapQuery() = 0;

	virtual Bool ShouldPrepareDynamicShadowmaps() const = 0;

	virtual Bool ShouldPrepareStaticShadowmaps() const = 0;

	// allocate space for dynamic shadows and render shadow maps using shadow manager
	// This only returns false in one condition: when allocating texture space has failed
	virtual Bool PrepareDynamicShadowmaps( const CRenderCollector& collector, CRenderShadowManager* shadowManager, const TDynArray< IRenderProxyBase* >& proxies )=0;

	// allocate space for static (cached) shadowmap
	// This only returns false in one condition: when allocating texture space has failed
	virtual Bool PrepareStaticShadowmaps( const CRenderCollector& collector, CRenderShadowManager* shadowManager, const TDynArray< IRenderProxyBase* >& proxies )=0;

	/// Chain for update function that should happen only once per frame
	const EFrameUpdateState UpdateOncePerFrame( const CRenderCollector& collector );

private:

	void UpdateFlickering();

};
