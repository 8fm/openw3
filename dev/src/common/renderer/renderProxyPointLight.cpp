/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "renderProxyPointLight.h"
#include "renderProxyDrawable.h"
#include "renderCollector.h"

#include "renderShadowManager.h"
#include "renderShadowRegions.h"
#include "renderScene.h"
#include "../engine/pointLightComponent.h"

CRenderProxy_PointLight::CRenderProxy_PointLight( const RenderProxyInitInfo& initInfo )
	: IRenderProxyLight( RPT_PointLight, initInfo )
	, m_cubeFaceMask( 0xFF )
	, m_staticShadowCube( NULL )
{
	// reset the dynamic shadows region
	Red::System::MemoryZero( m_dynamicShadowRegions, sizeof(m_dynamicShadowRegions) );

	// get the face mask from component
	if ( initInfo.m_component && initInfo.m_component->IsA< CPointLightComponent >() )
	{
		const CPointLightComponent* plc = static_cast< const CPointLightComponent* >( initInfo.m_component );
		m_cubeFaceMask = plc->GetDynamicShadowsFaceMask();
	}
	else if ( initInfo.m_packedData && initInfo.m_packedData->GetType() == RPT_PointLight )
	{
		const RenderProxyPointLightInitData* lightData = static_cast< const RenderProxyPointLightInitData* >( initInfo.m_packedData );
		m_cubeFaceMask = lightData->m_dynamicShadowsFaceMask;
	}
}

CRenderProxy_PointLight::~CRenderProxy_PointLight()
{
	// Release the dynamic regions
	for ( Uint32 i=0; i<6; ++i )
	{
		SAFE_RELEASE( m_dynamicShadowRegions[i] );
	}

	// Release static cube if allocated
	SAFE_RELEASE( m_staticShadowCube );
}

void CRenderProxy_PointLight::CalcLightCamera( CRenderCamera& outCamera, Uint32 side ) const
{
	// Evaluate light position
	const Vector lightPos = GetFinalPosition();

	// calculate spot camera rotation
	// THIS LIST IS SACRED - do not change the order or rotations
	EulerAngles cameraRotation;
	switch ( side )
	{
		case 0: cameraRotation = EulerAngles(0,0,90); break; // +X
		case 1: cameraRotation = EulerAngles(0,0,-90);  break; // -X
		case 2: cameraRotation = EulerAngles(0,0,180); break; // +Y
		case 3: cameraRotation = EulerAngles(0,0,0); break; // -Y
		case 4: cameraRotation = EulerAngles(0,-90,0); break; // +Z
		case 5: cameraRotation = EulerAngles(0,90,0); break; // -Z
	}

	// Setup camera for spotlight
	outCamera = CRenderCamera(
		lightPos,
		cameraRotation,
		90.0f, // fixed FOV
		1.0f,
		0.05f,
		m_radius,
		1.0f
	);
}

IShadowmapQuery* CRenderProxy_PointLight::CreateShadowmapQuery()
{
	m_shadowProxies.ClearFast();
	return new BoxQuery( this, Box( GetFinalPosition(), GetRadius() ), m_shadowProxies );
}

Bool CRenderProxy_PointLight::PrepareDynamicShadowmaps( const CRenderCollector& collector, CRenderShadowManager* shadowManager, const TDynArray< IRenderProxyBase* >& proxies )
{
	// Result code
	Bool result = true;

	// Reset shadowmap region info
	SAFE_RELEASE( m_dynamicShadowRegions[0] );
	SAFE_RELEASE( m_dynamicShadowRegions[1] );
	SAFE_RELEASE( m_dynamicShadowRegions[2] );
	SAFE_RELEASE( m_dynamicShadowRegions[3] );
	SAFE_RELEASE( m_dynamicShadowRegions[4] );
	SAFE_RELEASE( m_dynamicShadowRegions[5] );

	// Get the effective flag that filter the static geometry
	// If we can cache the static shadows AND we sucessfully allocated a cube 
	// for that than we can skip putting them in dynamic shadows
	Bool canDrawDynamicShadowsFromStaticGeometry = m_isCastingStaticShadows;
	if ( m_staticShadowsCached && ( m_staticShadowCube != NULL ) )
	{
		canDrawDynamicShadowsFromStaticGeometry = false;
	}

	if ( proxies.Empty() )
	{
		// There's nothing to cast the shadow, do not render it
		return result;
	}

	TDynArray< IRenderProxyDrawable* > allShadowProxiesForSide;
	allShadowProxiesForSide.Reserve( proxies.Size() );

	// Process all 6 cube faces
	for ( Uint32 cubeSide = 0; cubeSide < 6; ++cubeSide )
	{
		// Filter some of the cube faces from having shadows
		const Uint8 sideMask = 1 << cubeSide;
		if ( ( sideMask & m_cubeFaceMask ) == 0 )
		{
			continue;
		}

		// start with empty array per face
		allShadowProxiesForSide.ClearFast();

		// Calculate camera
		CRenderCamera lightCamera;
		CalcLightCamera( lightCamera, cubeSide );

		// Calculate camera frustum
		CFrustum cameraFrustum( lightCamera.GetWorldToScreen() );

		// Filter the proxies collected for the whole light and get only those visible in this side		
		for ( Uint32 j = 0; j < proxies.Size(); ++j )
		{
			IRenderProxyDrawable* proxy = proxies[ j ]->AsDrawable();

			// filter by geometric position
			if ( cameraFrustum.TestBox( proxy->GetBoundingBox() ) != 0 )
			{
				// filter by proxy dynamic flag
				const Bool isProxyDynamic = proxy->IsDynamic();
				if ( (isProxyDynamic && m_isCastingDynamicShadows) || 
					(!isProxyDynamic && canDrawDynamicShadowsFromStaticGeometry) )
				{
					allShadowProxiesForSide.PushBack( proxy );
				}
			}
		}

		// No shadow casting proxies visible for this side, do not allocate the shadowmap region
		if ( allShadowProxiesForSide.Empty() )
		{
			continue;
		}

		// Allocate the slot for shadows
		CRenderShadowDynamicRegion* region = shadowManager->AllocateDynamicRegion( m_currentShadowResolution );
		if ( region )
		{
			// Register the proxies for rendering
			m_dynamicShadowRegions[ cubeSide ] = region;
			shadowManager->AddDynamicRegionToRender( region, lightCamera, allShadowProxiesForSide );
		}
		else
		{
			// failed to allocate dynamic region
			result = false;
		}
	}

	// return the result code
	return result;
}

Bool CRenderProxy_PointLight::PrepareStaticShadowmaps( const CRenderCollector& collector, CRenderShadowManager* shadowManager, const TDynArray< IRenderProxyBase* >& proxies )
{
	RED_ASSERT( m_staticShadowsCached );

	// Well, this should not be called here
	if ( !m_staticShadowsCached )
	{
		return true;
	}

	// If we have valid cube than do nothing
	if ( m_staticShadowCube && m_staticShadowCube->IsValid() )
	{
		m_staticShadowCube->UpdateFrameIndex( collector.m_frameIndex );
		return true;
	}

	// Release current cube
	SAFE_RELEASE( m_staticShadowCube );

	// Well, we don't have a valid shadow cube and we should have one, allocate it
	m_staticShadowCube = shadowManager->AllocateCube( collector.m_frameIndex, GetBoundingBox() );
	if ( m_staticShadowCube )
	{
		// Filter to get the list of proxies that are static
		TDynArray< IRenderProxyDrawable* > allShadowProxiesStatic;
		for ( Uint32 j = 0; j < proxies.Size(); ++j )
		{
			IRenderProxyDrawable* proxy= proxies[ j ]->AsDrawable();
			RED_FATAL_ASSERT( proxy, "Invalid proxy - collected not a drawable?" );
			if ( !proxy->IsDynamic() )
			{
				allShadowProxiesStatic.PushBack( proxy );
			}
		}

		// Request an update for this cube
		shadowManager->AddStaticCubeToRender( m_staticShadowCube, GetFinalPosition(), GetRadius(), allShadowProxiesStatic );
		return true;
	}

	// Not added
	return false;
}

void CRenderProxy_PointLight::Relink( const Box& boundingBox, const Matrix& localToWorld )
{
	// Pass to base class
	IRenderProxyLight::Relink( boundingBox, localToWorld );

	// Release any cached static shadows
	if ( m_staticShadowCube != NULL )
	{
		m_staticShadowCube->Release();
		m_staticShadowCube = NULL;
	}
}
