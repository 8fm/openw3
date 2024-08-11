/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "renderProxy.h"
#include "renderProxyFadeable.h"
#include "../engine/drawableComponent.h"

class IDynamicDecalTarget;
struct SMergedShadowCascades;

/// Render proxy flags
enum ERenderProxyDrawableFlags
{
	RPF_CastingShadows					= FLAG( 0 ),	//!< This component can cast dynamic shadows
	RPF_Dynamic							= FLAG( 1 ),	//!< This proxy is considered to be dynamic (shadow casting option)
	RPF_IsSelected						= FLAG( 2 ),	//!< Proxy is selected in editor, draw selection highlights on it
	RPF_CastShadowsFromLocalLightsOnly	= FLAG( 3 ),	//!< This proxy casts shadows from local lights only
	RPF_IsVisible						= FLAG( 4 ),	//!< Component is visible - flag used in proxies casting shadows yet not visible
	RPF_IsTwoSided						= FLAG( 5 ),	//!< Is two-sided rendering required for the proxy
	RPF_CustomReferencePoint			= FLAG( 6 ),	//!< Object is using custom reference point for calculating distance
	RPF_NoFOVAdjustedDistance			= FLAG( 7 ),	//!< FOV is not used to adjust render distance of this proxy
};

/// Category whether the proxy is OnScreen or OffScreen
enum EScreenCategory
{
	SCREENCAT_OnScreen,
	SCREENCAT_OffScreen,

	SCREENCAT_Max,
};

/// Proxy effect parameters
struct SRenderProxyDrawableEffectParams
{
	DECLARE_STRUCT_MEMORY_POOL_ALIGNED( MemoryPool_Default, MC_RenderData, __alignof( SRenderProxyDrawableEffectParams ) );

	Vector		m_customParam0;					//!< Effect custom param 0
	Vector		m_customParam1;					//!< Effect custom param 1
	Vector		m_overrideParams;
};

/// Proxy coloring parameters
struct SRenderProxyDrawableMeshColoringParams
{
	DECLARE_STRUCT_MEMORY_POOL_ALIGNED( MemoryPool_Default, MC_RenderData, __alignof( SRenderProxyDrawableMeshColoringParams ) );

	Matrix		m_colorShiftMatrix0;			//!< Matrix for color shift, region one
	Matrix		m_colorShiftMatrix1;			//!< Matrix for color shift, region two
};

/// Clipping ellipse parameters
struct SRenderProxyDrawableClippingEllipseParams
{
	DECLARE_STRUCT_MEMORY_POOL_ALIGNED( MemoryPool_Default, MC_RenderData, __alignof( SRenderProxyDrawableClippingEllipseParams ) );

	Matrix		m_localToEllipse;				//!< Transform taking a point in the drawable's local space to the clipping ellipse.
};


/// Proxy that can be actually drawn :)
class IRenderProxyDrawable : public IRenderProxyFadeable
{
protected:	
#ifndef NO_COMPONENT_GRAPH
	CHitProxyID									m_hitProxyID;						//!< ID for selection
#endif

protected:	
	SRenderProxyDrawableEffectParams*			m_effectParams;						//!< Effect params
	SRenderProxyDrawableMeshColoringParams*		m_coloringParams;					//!< Mesh coloring parameters

	SRenderProxyDrawableClippingEllipseParams*	m_clippingEllipseParams;			//!< Parameters for clipping ellipse, if used.

	Float										m_maxExtentSquared;					//!< maximum extent from bounding box.
	Float										m_textureDistanceMultiplier;		//!< Additional multiplier for the texture distance

	struct
	{
		Uint8									m_drawableFlags:8;					//!< Flags for drawable component
		Uint8									m_lightChannels:8;					//!< Light channels
		ERenderingPlane							m_renderingPlane:8;					//!< Rendering plane (foreground/scene/background)
	};

public:
	//! Get rendering plane
	RED_INLINE ERenderingPlane GetRenderingPlane(){ return m_renderingPlane; }

	//! Rendering distance (cached)
	RED_INLINE Float GetCachedDistanceSquared() const { return m_cachedDistanceSquared; }
	
	//! Can this proxy cast shadows ?
	RED_INLINE Bool CanCastShadows() const { return 0 != ( m_drawableFlags & RPF_CastingShadows ); }		

	//! This proxy cast shadows from local lights only
	RED_INLINE Bool IsCastingShadowsFromLocalLightsOnly() const { return 0 != ( m_drawableFlags & RPF_CastShadowsFromLocalLightsOnly ); }
	
	//! Is the proxy selected
	RED_INLINE Bool IsSelected() const { return 0 != ( m_drawableFlags & RPF_IsSelected ); }

	//! Is visible?
	RED_INLINE Bool IsVisible() const { return 0 != ( m_drawableFlags & RPF_IsVisible ); }

	// Is this a dynamic geometry proxy ?
	RED_INLINE Bool IsDynamic() const { return 0 != ( m_drawableFlags & RPF_Dynamic ); }

	// Is this a two sided proxy
	RED_INLINE Bool IsTwoSided() const { return 0 != ( m_drawableFlags & RPF_IsTwoSided ); }

	// Is this proxy using custom reference point for visibility
	RED_INLINE Bool HasCustomReferencePoint() const { return 0 != ( m_drawableFlags & RPF_CustomReferencePoint ); }

	// Some proxies should not have the distance adjusted based on the FOV, it this the case ?
	RED_INLINE Bool HasNoFOCAdjustedDistance() const { return 0 != ( m_drawableFlags & RPF_NoFOVAdjustedDistance ); }

	//! Is character?
	RED_INLINE Bool IsCharacterProxy() const { return 0 != ( m_lightChannels & LC_Characters ); }

#ifndef NO_COMPONENT_GRAPH
	//! Get hit proxy ID for this object
	RED_INLINE const CHitProxyID& GetHitProxyID() const { return m_hitProxyID; }
#endif

	//! Get assigned light channel mask
	RED_INLINE Uint8 GetLightChannels() const { return m_lightChannels; }
	RED_INLINE void SetLightChannels( Uint8 lc ) { m_lightChannels = lc; }

	//! Get effect parameters
	RED_INLINE const SRenderProxyDrawableEffectParams* GetEffectParams() const { return m_effectParams; }

	//! Get coloring parameters
	RED_INLINE const SRenderProxyDrawableMeshColoringParams* GetColoringParams() const { return m_coloringParams; }

	//! Get reference point
	RED_INLINE const Vector GetCustomReferencePoint() const { return Vector( m_customReferencePoint[0], m_customReferencePoint[1], m_customReferencePoint[2] ); }

	//! Get proxy visibility query
	RED_INLINE TRenderVisibilityQueryID GetVisibilityQuery() const { return m_visibilityQuery; }

	/// Check visibility range (CachedRange() < m_autoHide)
	/// NOTE: This requires UpdateOncePerFrame to be called to give accurate results
	RED_FORCE_INLINE const Bool IsVisibleInCurrentFrame() const { return GetCachedDistanceSquared() < (m_autoHideDistance*m_autoHideDistance); }

public:
	IRenderProxyDrawable( ERenderProxyType type, const RenderProxyInitInfo& initInfo );
	virtual ~IRenderProxyDrawable();

	//! This is a drawable proxy
	virtual Bool IsDrawable() const override { return true; }

	//! Fast access interface for drawable proxies
	virtual IRenderProxyDrawable* AsDrawable() override { return this; }
	virtual const IRenderProxyDrawable* AsDrawable() const override { return this; }
	virtual IDynamicDecalTarget* ToDynamicDecalTarget() { return nullptr; }

	//! Attach to scene
	virtual void AttachToScene( CRenderSceneEx* scene );

	//! Update hit proxy ID
	virtual void UpdateHitProxyID( const CHitProxyID& id );

	//! Update selection flag
	virtual void UpdateSelection( Bool isSelected );

	//! Collect elements for rendering
	virtual void CollectElements( CRenderCollector& collector )=0;

	//! Allow processing when proxy is past autohide distance
	virtual void OnNotVisibleFromAutoHide( CRenderCollector& collector ) {}

	//! Collect elements for a list of shadow cascades
	virtual void CollectCascadeShadowElements( SMergedShadowCascades& cascades, Uint32 perCascadeTestResults ) {}

	//! Update color shift matrices
	virtual void UpdateColorShiftMatrices( const Matrix& region0, const Matrix& region1 );

	//! Update effect parameters
	virtual void UpdateEffectParameters( const Vector& paramValue, Int32 paramIndex );

	//! Update effect parameters for override
	virtual void UpdateEffectParametersOverride( const Vector& paramValue );

	//! Get selection color
	virtual void GetSelectionColor( Vector &selectionColor /* out */ ){};

	//! Set selection color
	virtual void SetSelectionColor( const Vector &selectionColor ){};

	//! Set material replacement
	virtual void SetReplacedMaterial( IRenderResource* material, IRenderResource* parameters, Bool drawOriginal ) {};

	//! Disable material replacement for proxy
	virtual void DisableMaterialReplacement() {};
	
	//! Is this drawable allowed to be rendered into envProbes
	virtual Bool IsAllowedForEnvProbes() const;

	//! Get proxy rendering mask - helps with visibility filtering (avoids testing proxy in cases when it does not want to be rendered any way)
	virtual const Uint8 GetRenderMask() const;

	//! Prevent proxy from being collected by render collector.
	virtual void SetVisible( Bool visible );

	//! Does this proxy have a clipping ellipse
	RED_FORCE_INLINE Bool HasClippingEllipse() const { return m_clippingEllipseParams != nullptr; }

	//! Get parameters for the clipping ellipse (null if it doesn't use it).
	RED_FORCE_INLINE const SRenderProxyDrawableClippingEllipseParams* GetClippingEllipseParams() const { return m_clippingEllipseParams; }

	//! Set the local-to-ellipse transform matrix for the clipping ellipse. This is a matrix that takes a point from
	//! the proxy's local space to the clipping ellipse's space. Anything in the ellipse's space inside the unit sphere
	//! centered at (0,0,0) is clipped.
	virtual void SetClippingEllipseMatrix( const Matrix& localToEllipse );

	//! Clear clipping ellipse data.
	virtual void ClearClippingEllipse();

	//! Set custom reference point for distance calculations (for visibility and LOD)
	void SetCustomReferencePoint( const Vector& customReferencePoint );

	//! Clear custom reference point for distance calculations 
	void ClearCustomReferencePoint();



	virtual void Relink( const Box& boundingBox, const Matrix& localToWorld ) override;
	virtual void RelinkBBoxOnly( const Box& boundingBox ) override;


	Float CalcCameraDistanceSq( const Vector& camPos, Float camFovMultiplier ) const;
	//! Take a previously calculated squared distance, and adjust it to something better suited for texture streaming purposes.
	//! Takes into account the object's size (bounding box's maximum extent) to give a rough approximation of squared distance to edge.
	//! Returned value tends to overestimate the distance, especially for larger objects. So distant large objects may be reported as
	//! quite a bit further away than they actually are. But at that point we'll probably only need small mips anyways, so there
	//! shouldn't be much visual difference.
	Float AdjustCameraDistanceSqForTexturesNoMultiplier( Float distanceSq ) const
	{
		return Max( distanceSq - m_maxExtentSquared, 0.0f );
	}
	Float AdjustCameraDistanceSqForTextures( Float distanceSq ) const
	{
		return AdjustCameraDistanceSqForTexturesNoMultiplier( distanceSq*m_textureDistanceMultiplier );
	}

	//! Convenience wrapper around CalcCameraDistanceSq and AdjustCameraDistanceSqForTextures.
	RED_INLINE Float CalcCameraDistanceSqForTextures( const Vector& camPos, Float camFovMultiplier ) const
	{
		return AdjustCameraDistanceSqForTexturesNoMultiplier( CalcCameraDistanceSq( camPos, camFovMultiplier ) );
	}


protected:
	/// Chain for update function that should happen only once per frame
	const EFrameUpdateState UpdateOncePerFrame( const CRenderCollector& collector );

	/// Update attached visibility query
	void UpdateVisibilityQueryState( Uint8 flags );

private:
	//! Refresh (calculate) distance to this proxy
	void RefreshCachedDistance( const CRenderCollector& collector );

	//! After the bounding box is changed, this should be called to update the cached maximum extent.
	void UpdateMaxExtent();

	// This is read in the mesh batcher in the same place where the drawable flags are read and it should be on the same cache line
	Float										m_cachedDistanceSquared;			//!< Cached distance from the LOD calculation

	// Custom reference point for distance calculation
	Float										m_customReferencePoint[3];

	//!< Visibility query we should report to when drawing elements of this proxy
	TRenderVisibilityQueryID					m_visibilityQuery;
			
};
