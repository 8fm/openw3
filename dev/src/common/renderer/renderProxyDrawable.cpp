/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "../../common/engine/meshEnum.h" // just for the render mask
#include "renderProxyDrawable.h"
#include "renderVisibilityQuery.h"
#include "renderVisibilityQueryManager.h"
#include "renderScene.h"
#include "renderElementMap.h"

IRenderProxyDrawable::IRenderProxyDrawable( ERenderProxyType type, const RenderProxyInitInfo& initInfo )
	: IRenderProxyFadeable( type, initInfo )
	, m_effectParams( nullptr )
	, m_coloringParams( nullptr )
	, m_clippingEllipseParams( nullptr )
	, m_maxExtentSquared( 0.0f )
	, m_textureDistanceMultiplier( 1.0f )
	, m_visibilityQuery( 0 )
	, m_drawableFlags( 0 )
	, m_lightChannels( LC_Default )
	, m_renderingPlane( (ERenderingPlane)initInfo.ExtractRenderingPlane() )
{
	ASSERT( m_renderingPlane < RPl_Max );

	if ( initInfo.IsCameraTransform() )
	{
		m_localToWorld = initInfo.ExtractLocalMatrix();
	}

	// Specials for drawable component
	if ( initInfo.m_component && initInfo.m_component->IsA< CDrawableComponent >() )
	{
		const CDrawableComponent* dc = static_cast< const CDrawableComponent* >( initInfo.m_component );
		m_drawableFlags |= dc->IsCastingShadows() ? RPF_CastingShadows : 0;
		m_drawableFlags |= dc->IsVisible() ? RPF_IsVisible : 0;
		m_drawableFlags |= dc->IsTwoSided() ? RPF_IsTwoSided : 0;

		m_lightChannels = dc->NoLighting() ? 0 : dc->GetLightChannels();

		if ( dc->IsDynamicGeometryComponent() )
		{
			m_drawableFlags |= RPF_Dynamic;
			m_lightChannels |= LC_DynamicObject;
		}

		if ( ! ( dc->IsForceNoAutohide() 
#ifndef NO_EDITOR
			 || CDrawableComponent::IsForceNoAutohideDebug() 
#endif
			) )
		{
			m_autoHideDistance = dc->GetAutoHideDistance();
			m_autoHideDistanceSquared = m_autoHideDistance * m_autoHideDistance;
		}
	}

	m_drawableFlags |= initInfo.IsCastingShadowsFromLocalLightsOnly() ? RPF_CastShadowsFromLocalLightsOnly : 0;

	if ( initInfo.IsCameraTransformWithoutRotation() || initInfo.IsCameraTransformWithRotation() )
	{
		RED_ASSERT(TXT("Camera transform drawables are not supported anymore"));
	}

	// Specials for cooked mesh
	if ( initInfo.m_packedData && initInfo.m_packedData->GetType() == RPT_Mesh )
	{
		const RenderProxyMeshInitData* meshData = static_cast< const RenderProxyMeshInitData* >( initInfo.m_packedData );
		m_drawableFlags |= ( meshData->m_castingShadows ) ? RPF_CastingShadows : 0;
		m_drawableFlags |= ( meshData->m_castingShadowsFromLocalLightsOnly ) ? RPF_CastShadowsFromLocalLightsOnly : 0;
		m_drawableFlags |= ( meshData->m_visible ) ? RPF_IsVisible : 0;
		m_lightChannels = meshData->m_lightChannels;
	}

	// Pass the visibility query ID
	m_visibilityQuery = initInfo.m_visibilityQuery;

	// Distance calculation
	m_customReferencePoint[0] = 0.0f;
	m_customReferencePoint[1] = 0.0f;
	m_customReferencePoint[2] = 0.0f;
	m_cachedDistanceSquared = FLT_MAX;

	UpdateMaxExtent();
}

IRenderProxyDrawable::~IRenderProxyDrawable()
{
	// Release effect parameters
	if ( m_effectParams )
	{
		delete m_effectParams;
		m_effectParams = nullptr;
	}

	// Release coloring parameters
	if ( m_coloringParams )
	{
		delete m_coloringParams;
		m_coloringParams = nullptr;
	}

	if ( m_clippingEllipseParams )
	{
		delete m_clippingEllipseParams;
		m_clippingEllipseParams = nullptr;
	}
}

void IRenderProxyDrawable::AttachToScene( CRenderSceneEx* scene )
{
	IRenderProxyBase::AttachToScene( scene );
}

void IRenderProxyDrawable::UpdateHitProxyID( const CHitProxyID& id )
{
#ifndef NO_COMPONENT_GRAPH
	m_hitProxyID = id;
#endif
}

void IRenderProxyDrawable::UpdateSelection( Bool isSelected )
{
	if ( isSelected )
	{
		m_drawableFlags |= RPF_IsSelected;
	}
	else
	{
		m_drawableFlags &= ~RPF_IsSelected;
	}
}


Bool IRenderProxyDrawable::IsAllowedForEnvProbes() const
{
	return !IsDynamic();
}

const Uint8 IRenderProxyDrawable::GetRenderMask() const
{
	return MCR_Scene | MCR_LocalShadows | MCR_Cascade1 | MCR_Cascade2;
}

void IRenderProxyDrawable::UpdateColorShiftMatrices( const Matrix& region0, const Matrix& region1 )
{
	// Create on first use
	if ( !m_coloringParams )
	{
		m_coloringParams = new SRenderProxyDrawableMeshColoringParams();
	}

	// Initialize
	m_coloringParams->m_colorShiftMatrix0 = region0;
	m_coloringParams->m_colorShiftMatrix1 = region1;
}

void IRenderProxyDrawable::UpdateEffectParameters( const Vector& paramValue, Int32 paramIndex )
{
	// Create on first use
	if ( !m_effectParams )
	{
		m_effectParams = new SRenderProxyDrawableEffectParams();
	}

	// Update value of selected parmaeter
	switch ( paramIndex )
	{
		case 0: m_effectParams->m_customParam0.X = paramValue.X; break;
		case 1: m_effectParams->m_customParam0.Y = paramValue.Y; break;
		case 2: m_effectParams->m_customParam0.Z = paramValue.Z; break;
		case 3: m_effectParams->m_customParam0.W = paramValue.W; break;
		case 4: m_effectParams->m_customParam1 = paramValue; break;

		// Not pretty, but consistent with current system
		case 5: m_effectParams->m_customParam0 = paramValue; break;
	}
}

void IRenderProxyDrawable::UpdateEffectParametersOverride( const Vector& paramValue )
{
	// Create on first use
	if ( !m_effectParams )
	{
		m_effectParams = new SRenderProxyDrawableEffectParams();
	}

	m_effectParams->m_overrideParams = paramValue;
}

void IRenderProxyDrawable::SetVisible( Bool visible )
{
	if ( visible )
	{
		m_drawableFlags |= RPF_IsVisible;
	}
	else
	{
		m_drawableFlags &= ~RPF_IsVisible;
	}
}


void IRenderProxyDrawable::SetClippingEllipseMatrix( const Matrix& localToEllipse )
{
	if ( !m_clippingEllipseParams )
	{
		m_clippingEllipseParams = new SRenderProxyDrawableClippingEllipseParams();
	}
	m_clippingEllipseParams->m_localToEllipse = localToEllipse;
}

void IRenderProxyDrawable::ClearClippingEllipse()
{
	if ( m_clippingEllipseParams != nullptr )
	{
		delete m_clippingEllipseParams;
		m_clippingEllipseParams = nullptr;
	}
}

#ifdef USE_ANSEL
extern Bool isAnselSessionActive;
#endif // USE_ANSEL

void IRenderProxyDrawable::RefreshCachedDistance( const CRenderCollector& collector )
{
	const Vector& cameraPos = collector.m_lodCamera->GetPosition();

	m_textureDistanceMultiplier = collector.m_camera->GetFOVMultiplierUnclamped();

	if ( HasCustomReferencePoint() )
	{
		const Vector customPoint( m_customReferencePoint[0], m_customReferencePoint[1], m_customReferencePoint[2] );
		m_cachedDistanceSquared = cameraPos.DistanceSquaredTo( customPoint );
	}
	else
	{
		m_cachedDistanceSquared = cameraPos.DistanceSquaredTo( GetBoundingBox().CalcCenter() );

		if ( !HasNoFOCAdjustedDistance() )
		{
			const Float fovMultiplier = 
#ifdef USE_ANSEL				
				isAnselSessionActive ? 0.2f : 
#endif // USE_ANSEL
				collector.m_camera->GetFOVMultiplier();

			m_cachedDistanceSquared *= fovMultiplier;

			// If regular FOV multiplier isn't 1, then the FOV is already taken into account with the cached distanced
			// and we don't need the additional multiplier. It's only needed when the regular FOV multiplier is 1
			// because it may have been clamped for a narrow FOV.
			if ( collector.m_camera->GetFOVMultiplier() > 1.0f )
			{
				m_textureDistanceMultiplier = 1.0f;
			}
		}
	}
}

void IRenderProxyDrawable::SetCustomReferencePoint( const Vector& customReferencePoint )
{
	m_customReferencePoint[0] = customReferencePoint.X;
	m_customReferencePoint[1] = customReferencePoint.Y;
	m_customReferencePoint[2] = customReferencePoint.Z;
	m_drawableFlags |= RPF_CustomReferencePoint;
}

void IRenderProxyDrawable::ClearCustomReferencePoint()
{
	m_drawableFlags &= ~RPF_CustomReferencePoint;
}

void IRenderProxyDrawable::UpdateVisibilityQueryState( Uint8 flags )
{
	if ( m_visibilityQuery && m_scene && m_scene->GetVisibilityQueryManager() )
	{
		m_scene->GetVisibilityQueryManager()->MarkQuery( m_visibilityQuery, (CRenderVisibilityQueryManager::EFlags) flags );
	}
}

const EFrameUpdateState IRenderProxyDrawable::UpdateOncePerFrame( const CRenderCollector& collector )
{
	const auto ret = IRenderProxyFadeable::UpdateOncePerFrame( collector );
	if( ret == FUS_AlreadyUpdated )
	{
		return ret;
	}
	// Recompute and cache distance to this proxy, it's used by all LOD/Visibility calculations
	// Do it only once per frame
	RefreshCachedDistance( collector );

	if ( m_scene != nullptr && m_scene->GetRenderElementMap() != nullptr && GetRegCount() > 0 )
	{
		m_scene->GetRenderElementMap()->SetProxyPastAutoHide( GetEntryID(), m_cachedDistanceSquared > m_autoHideDistanceSquared );
	}

	return ret;
}


void IRenderProxyDrawable::UpdateMaxExtent()
{
	const Vector extents = m_boundingBox.CalcExtents();
	m_maxExtentSquared = Max( extents.X, extents.Y, extents.Z );
	m_maxExtentSquared *= m_maxExtentSquared;
}


void IRenderProxyDrawable::Relink( const Box& boundingBox, const Matrix& localToWorld )
{
	IRenderProxyFadeable::Relink( boundingBox, localToWorld );
	UpdateMaxExtent();
}

void IRenderProxyDrawable::RelinkBBoxOnly( const Box& boundingBox )
{
	IRenderProxyFadeable::RelinkBBoxOnly( boundingBox );
	UpdateMaxExtent();
}


Float IRenderProxyDrawable::CalcCameraDistanceSq( const Vector& camPos, Float camFovMultiplier ) const
{
	switch ( m_type )
	{
	case RPT_Particles:
		return camPos.DistanceSquaredTo( m_localToWorld.GetTranslationRef() ) * camFovMultiplier;

	default:
		return camPos.DistanceSquaredTo( m_boundingBox.CalcCenter() ) * camFovMultiplier;
	}
}
