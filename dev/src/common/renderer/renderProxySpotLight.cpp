/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#include "renderProxySpotLight.h"
#include "renderProxyDrawable.h"

#include "renderShadowManager.h"
#include "renderCollector.h"
#include "renderScene.h"
#include "renderTexture.h"
#include "../engine/spotLightComponent.h"
#include "../engine/bitmapTexture.h"

RED_DEFINE_STATIC_NAME( projectionTexureAngle );
RED_DEFINE_STATIC_NAME( projectionTexureFactor );
RED_DEFINE_STATIC_NAME( projectionTexureUBias );
RED_DEFINE_STATIC_NAME( projectionTexureVBias );
RED_DEFINE_STATIC_NAME( innerAngle );
RED_DEFINE_STATIC_NAME( outerAngle );
RED_DEFINE_STATIC_NAME( softness );

CRenderProxy_SpotLight::CRenderProxy_SpotLight( const RenderProxyInitInfo& initInfo )
	: IRenderProxyLight( RPT_SpotLight, initInfo )
	, m_innerAngle( 0.0f )
	, m_outerAngle( 0.0f )
	, m_softness( 1.0f )
	, m_projectionTexture( nullptr )
	, m_projectionCamera( nullptr )
	, m_dynamicShadowRegion( nullptr )
{
	// Copy light parameters
	if ( initInfo.m_component && initInfo.m_component->IsA< CSpotLightComponent >() )
	{
		const CSpotLightComponent* lightComponent = static_cast< const CSpotLightComponent* >( initInfo.m_component );
		m_innerAngle = lightComponent->GetInnerAngle();
		m_outerAngle = lightComponent->GetOuterAngle();
		m_softness = lightComponent->GetSoftness();

		if ( lightComponent->GetProjectionTexture() )
		{
			ExtractRenderResource( lightComponent->GetProjectionTexture(), m_projectionTexture );
			RED_ASSERT( m_projectionTexture != nullptr, TXT("No projection texture") );

			m_cachedProjectionTextureAngle = lightComponent->GetProjectionTextureAngle();
			Vector offsetVector;
			CalcOffsetVector( offsetVector );
			m_projectionCamera = new CRenderCamera( m_localToWorld.GetTranslation() + offsetVector, m_localToWorld.ToEulerAngles(), lightComponent->GetProjectionTextureAngle(), 1.0f, -100.0f, 100.0f );
			m_projectionTextureUBias = lightComponent->GetProjectionTextureUBias();
			m_projectionTextureVBias = lightComponent->GetProjectionTextureVBias();
		}
	}
	else if ( initInfo.m_packedData && initInfo.m_packedData->GetType() == RPT_SpotLight )
	{
		const RenderProxySpotLightInitData* lightData = static_cast< const RenderProxySpotLightInitData* >( initInfo.m_packedData );
		m_innerAngle = lightData->m_innerAngle;
		m_outerAngle = lightData->m_outerAngle;
		m_softness = lightData->m_softness;

		if ( lightData->m_projectionTexture )
		{
			ExtractRenderResource( lightData->m_projectionTexture, m_projectionTexture );
			RED_ASSERT( m_projectionTexture != nullptr, TXT("No projection texture") );

			m_cachedProjectionTextureAngle = lightData->m_projectionTextureAngle;

			Vector offsetVector;
			CalcOffsetVector( offsetVector );
			m_projectionCamera = new CRenderCamera( m_localToWorld.GetTranslation() + offsetVector, m_localToWorld.ToEulerAngles(), lightData->m_projectionTextureAngle, 1.0f, -100.0f, 100.0f );
			m_projectionTextureUBias = lightData->m_projectionTexureUBias;
			m_projectionTextureVBias = lightData->m_projectionTexureVBias;
		}
	}
}

CRenderProxy_SpotLight::~CRenderProxy_SpotLight()
{
	// Release extra camera data
	if ( m_projectionCamera )
	{
		delete m_projectionCamera;
	}

	// Release the projection texture resource
	SAFE_RELEASE( m_projectionTexture );

	// Release the dynamic shadow region
	SAFE_RELEASE( m_dynamicShadowRegion );
}

void CRenderProxy_SpotLight::Relink( const Box& boundingBox, const Matrix& localToWorld )
{
	IRenderProxyLight::Relink( boundingBox, localToWorld );

	if ( m_projectionCamera )
	{
		RecalculateProjectionTextureCamera();
	}
}

void CRenderProxy_SpotLight::UpdateParameter( const CName& name, Float param )
{
	IRenderProxyLight::UpdateParameter( name, param );

	if ( name == CNAME( projectionTexureAngle ) || name == CNAME( ProjectionTexureAngle ) )
	{
		m_cachedProjectionTextureAngle = param;
		RecalculateProjectionTextureCamera();
	}
	else if ( name == CNAME( projectionTexureUBias ) || name == CNAME( ProjectionTexureUBias ) )
	{
		m_projectionTextureUBias = param;
	}
	else if ( name == CNAME( projectionTexureVBias ) || name == CNAME( ProjectionTexureVBias ) )
	{
		m_projectionTextureVBias = param;
	}
	else if ( name == CNAME( innerAngle ) )
	{
		m_innerAngle = param;
	}
	else if ( name == CNAME( outerAngle ) )
	{
		m_outerAngle = param;
	}
	else if ( name == CNAME( softness ) )
	{
		m_softness = param;
	}
}

void CRenderProxy_SpotLight::RecalculateProjectionTextureCamera()
{
	Vector offsetVector;
	CalcOffsetVector( offsetVector );
	*m_projectionCamera = CRenderCamera( m_localToWorld.GetTranslation() + offsetVector, m_localToWorld.ToEulerAngles(), m_cachedProjectionTextureAngle, 1.0f, -100.0f, 100.0f );
}

void CRenderProxy_SpotLight::CalcLightCamera( CRenderCamera& outCamera ) const
{
	// Evaluate light position
	const Vector lightPos = GetFinalPosition();
	const EulerAngles lightRotation = m_localToWorld.ToEulerAngles();

	// Setup camera for spotlight
	outCamera = CRenderCamera(
		lightPos,
		lightRotation,
		m_outerAngle,
		1.0f,
		0.05f,
		m_radius,
		1.0f
	);
}

IShadowmapQuery* CRenderProxy_SpotLight::CreateShadowmapQuery()
{
	m_shadowProxies.ClearFast();
	CRenderCamera lightCamera;
	CalcLightCamera( lightCamera );
	return new FrustumQuery( this, CFrustum( lightCamera.GetWorldToScreen() ), m_shadowProxies );
}

Bool CRenderProxy_SpotLight::PrepareDynamicShadowmaps( const CRenderCollector& collector, CRenderShadowManager* shadowManager, const TDynArray< IRenderProxyBase* >& proxies )
{
	// Filter the light components
	TDynArray< IRenderProxyDrawable* > allShadowProxies;
	// prepare for the worst
	allShadowProxies.Reserve( proxies.Size() );
	for ( Uint32 i = 0; i < proxies.Size(); ++i )
	{
		if ( IRenderProxyDrawable* proxy = proxies[ i ]->AsDrawable() )
		{
			if ( (proxy->IsDynamic() && m_isCastingDynamicShadows) || 
				(!proxy->IsDynamic() && m_isCastingStaticShadows) )
			{
				allShadowProxies.PushBack( proxy );
			}
		}
	}

	// Cleanup
	SAFE_RELEASE( m_dynamicShadowRegion );

	// There's nothing to cast the shadow, do not render it
	if ( allShadowProxies.Empty() )
	{
		m_currentShadowResolution = 0; // not needing shadows this frame
		return true;
	}

	// Allocate the slot for shadows
	m_dynamicShadowRegion = shadowManager->AllocateDynamicRegion( m_currentShadowResolution );
	if ( m_dynamicShadowRegion )
	{
		// Register the proxies for rendering
		// Calculate camera
		CRenderCamera lightCamera;
		CalcLightCamera( lightCamera );
		shadowManager->AddDynamicRegionToRender( m_dynamicShadowRegion, lightCamera, allShadowProxies );
		return true;
	}

	// well, allocation didn't go well
	m_currentShadowResolution = 0; // not needing shadows this frame
	return false; // failed to allocate the region
}

Bool CRenderProxy_SpotLight::PrepareStaticShadowmaps( const CRenderCollector& collector, CRenderShadowManager* shadowManager, const TDynArray< IRenderProxyBase* >& proxies )
{
	// Spots are not caching shadowS
	return true;
}
